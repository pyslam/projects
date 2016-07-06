// ==========================================================================
// Program FLIP_CLOUD is a special utility we wrote in order to be
// able to apply fishnet ground finding to level-2 TDP files not
// generated by program DENSITY_L1.  It takes in an L2 cloud within
// filtered_points.tdp and computes a reasonable XY bounding box.  All
// points within the bounding box have their Z values flipped so that
// the point cloud is effectively inverted.  FLIP_CLOUD exports the
// inverted cloud to TDP file flipped_filtered_points.tdp.

//				flip_cloud			

// ==========================================================================
// Last updated on 12/12/11
// ==========================================================================

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "geometry/bounding_box.h"
#include "image/drawfuncs.h"
#include "general/filefuncs.h"
#include "math/fourvector.h"
#include "math/prob_distribution.h"
#include "general/sysfuncs.h"
#include "osg/osg3D/tdpfuncs.h"
#include "image/TwoDarray.h"
#include "coincidence_processing/VolumetricCoincidenceProcessor.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);

// Constants:

   const double voxel_binsize=0.25;	// meter

// Repeated variable declarations:

   string banner,unix_cmd="";
   bool perturb_voxels_flag;
   double min_prob_threshold=0;

// Parse input TDP file:

   string tdp_filename="filtered_points.tdp";
//   cout << "Enter input TDP filename:" << endl;
//   cin >> tdp_filename;

   vector<double>* X_ptr=new vector<double>;
   vector<double>* Y_ptr=new vector<double>;
   vector<double>* Z_ptr=new vector<double>;
   vector<double>* P_ptr=new vector<double>;

   int npoints=tdpfunc::npoints_in_tdpfile(tdp_filename);
   X_ptr->reserve(npoints);
   Y_ptr->reserve(npoints);
   Z_ptr->reserve(npoints);
   P_ptr->reserve(npoints);
   tdpfunc::read_XYZP_points_from_tdpfile(
      tdp_filename,*X_ptr,*Y_ptr,*Z_ptr,*P_ptr);

// Calculate X and Y distributions and construct XY bounding box:

   prob_distribution Xprob(*X_ptr,1000);
   prob_distribution Yprob(*Y_ptr,1000);
   
   double x01=Xprob.find_x_corresponding_to_pcum(0.01);
   double x99=Xprob.find_x_corresponding_to_pcum(0.99);
   double y01=Yprob.find_x_corresponding_to_pcum(0.01);
   double y99=Yprob.find_x_corresponding_to_pcum(0.99);
   bounding_box bbox(x01,x99,y01,y99);

/*
   double x03=Xprob.find_x_corresponding_to_pcum(0.03);
   double x97=Xprob.find_x_corresponding_to_pcum(0.97);
   double y03=Yprob.find_x_corresponding_to_pcum(0.03);
   double y97=Yprob.find_x_corresponding_to_pcum(0.97);
   bounding_box bbox(x03,x97,y03,y97);
*/

// Export "flipped-Z" version of filtered point cloud for purposes of
// ground finding via fishnet:

// Invert all Z's so that Zmin <--> Zmax:

   double Zmin=mathfunc::minimal_value(*Z_ptr);
   double Zmax=mathfunc::maximal_value(*Z_ptr);
   cout << "Zmin = " << Zmin << " Zmax = " << Zmax << endl;

   vector<double>* Xcropped_ptr=new vector<double>;
   vector<double>* Ycropped_ptr=new vector<double>;
   vector<double>* Zflipped_ptr=new vector<double>;
   vector<double>* Pcropped_ptr=new vector<double>;
   Xcropped_ptr->reserve(npoints);
   Ycropped_ptr->reserve(npoints);
   Zflipped_ptr->reserve(npoints);
   Pcropped_ptr->reserve(npoints);

   for (unsigned int i=0; i<Z_ptr->size(); i++)
   {
      double curr_X=X_ptr->at(i);
      double curr_Y=Y_ptr->at(i);
      double curr_Z=Z_ptr->at(i);
      double curr_P=P_ptr->at(i);
      
      if (!bbox.point_inside(curr_X,curr_Y)) continue;

      Xcropped_ptr->push_back(curr_X);
      Ycropped_ptr->push_back(curr_Y);

      Zflipped_ptr->push_back(Zmin+Zmax-curr_Z);

      curr_P=basic_math::max(0.0,curr_P);
      curr_P=basic_math::min(1.0,curr_P);
      Pcropped_ptr->push_back(curr_P);
   }

   cout << "Xcropped.size() = " << Xcropped_ptr->size() << endl;
   cout << "Ycropped.size() = " << Ycropped_ptr->size() << endl;
   cout << "Zflipped.size() = " << Zflipped_ptr->size() << endl;
   cout << "Pcropped.size() = " << Pcropped_ptr->size() << endl;

/*   
   string flipped_tdp_filename="flipped_filtered_points.tdp";
   tdpfunc::write_xyzp_data(
      flipped_tdp_filename,Xcropped_ptr,Ycropped_ptr,
      Zflipped_ptr,Pcropped_ptr);
   unix_cmd="lodtree "+flipped_tdp_filename;
   sysfunc::unix_command(unix_cmd);
*/
 
// Instantiate Volumetric Coincidence Processor:

   threevector XYZ_min(x01,y01,Zmin);
   threevector XYZ_max(x99,y99,Zmax);
//   threevector XYZ_min(x03,y03,Zmin);
//   threevector XYZ_max(x97,y97,Zmax);

   VolumetricCoincidenceProcessor* vcp_ptr=new VolumetricCoincidenceProcessor;
   vcp_ptr->initialize_coord_system(XYZ_min,XYZ_max,voxel_binsize);

   for (unsigned int i=0; i<Xcropped_ptr->size(); i++)
   {
      double curr_x=Xcropped_ptr->at(i);
      double curr_y=Ycropped_ptr->at(i);
      double curr_z=Zflipped_ptr->at(i);
      vcp_ptr->accumulate_points(curr_x,curr_y,curr_z);
   }
   cout << "Aggregated number of points = " << vcp_ptr->size() << endl;

   delete Zflipped_ptr;

// Compute counts/probs distribution:

   vcp_ptr->renormalize_counts_into_probs();
   vcp_ptr->generate_p_distribution();
   
/*
   int delta_vox=1;
   int min_neighbors=1;
   int min_avg_counts=1;
   vcp_ptr->delete_isolated_voxels(delta_vox,min_neighbors,min_avg_counts);
*/
 
   X_ptr->clear();
   Y_ptr->clear();
   Z_ptr->clear();
   P_ptr->clear();

   perturb_voxels_flag=true;
   vcp_ptr->retrieve_XYZP_points(X_ptr,Y_ptr,Z_ptr,P_ptr,min_prob_threshold,
      perturb_voxels_flag);

   string vcp_tdp_filename="vcp_flipped_points.tdp";
   tdpfunc::write_xyzp_data(
      vcp_tdp_filename,X_ptr,Y_ptr,Z_ptr,P_ptr);
   unix_cmd="lodtree "+vcp_tdp_filename;
   sysfunc::unix_command(unix_cmd);

   delete X_ptr;
   delete Y_ptr;
   delete Z_ptr;
   delete P_ptr;

}
