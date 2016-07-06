// ========================================================================
// Program DBVOLUME reads in calibration parameters for N <= 10
// fixed tripod cameras from the sensors table of the PLUME database.
// DBVOLUME first export package files corresponding to smoke photos
// using camera metadata retrieved from plume database. It also reads
// in masks from the photos table of the PLUME database. DBVOLUME then
// instantiates a VolumetricCoincidenceProcessor volume whose lateral
// dimensions are initially set by the cameras' positions.  DBVOLUME
// iterates over all voxels within the VCP volume and projects each
// into all cameras' image planes.  If the voxel lies inside some
// minimal number of the cameras' smoke contours, it is marked with
// the number of such contours.  DBVOLUME consequently computes a "3D
// visual hull" corresponding to 2D plume masks derived from "t>0"
// tripod camera photos.

// For each time slice, DBVOLUME generates an OpenSceneGraph OSGA file
// for the point cloud associated with all of the marked voxels.  The
// OSGA files are written to a dynamically generated subdirectory of
// bundler_IO_subdir/plume_results/.  Generating OSGA files
// for ~100 time slices takes approximately an hour on my fastest
// linux machine.

// The OSGA files can be viewed on the G47 laptop via the VIEWPOINTS
// program via commands like

// 		   viewpoints plume_hull_030.osga

// ========================================================================
// Last updated on 1/28/13; 1/29/13; 1/30/13
// ========================================================================

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <osgDB/FileUtils>

#include "osg/osgGraphicals/AnimationController.h"
#include "geometry/bounding_box.h"
#include "video/camera.h"
#include "osg/osgOrganization/Decorations.h"
#include "osg/osgSceneGraph/HiresDataVisitor.h"
#include "messenger/Messenger.h"
#include "osg/ModeController.h"
#include "osg/osgModels/MODELSGROUP.h"
#include "osg/osgSceneGraph/MyDatabasePager.h"
#include "osg/osgOperations/Operations.h"
#include "passes/PassesGroup.h"
#include "postgres/plumedatabasefuncs.h"
#include "osg/osgGIS/postgis_databases_group.h"
#include "osg/osg3D/tdpfuncs.h"
#include "osg/osg3D/Terrain_Manipulator.h"
#include "video/texture_rectangle.h"
#include "time/timefuncs.h"
#include "video/videofuncs.h"
#include "osg/osgWindow/ViewerManager.h"
#include "coincidence_processing/VolumetricCoincidenceProcessor.h"

#include "general/outputfuncs.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::map;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

   timefunc::initialize_timeofday_clock();

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);
   string image_list_filename=passes_group.get_image_list_filename();
//   cout << " image_list_filename = " << image_list_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(image_list_filename);
//   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;

   int cloudpass_ID=passes_group.get_curr_cloudpass_ID();
   vector<int> GISlayer_IDs=passes_group.get_GISlayer_IDs();

// Instantiate postgis database objects to send data to and retrieve
// data from external Postgres database:

   postgis_databases_group* postgis_databases_group_ptr=
      new postgis_databases_group;
   postgis_database* postgis_db_ptr=postgis_databases_group_ptr->
      generate_postgis_database_from_GISlayer_IDs(
         passes_group,GISlayer_IDs);
//   cout << "postgis_db_ptr = " << postgis_db_ptr << endl;

   int mission_ID=22;
   cout << "Enter mission ID:" << endl;
   cin >> mission_ID;

   int fieldtest_ID=plumedatabasefunc::retrieve_fieldtest_ID_given_mission_ID(
      postgis_db_ptr,mission_ID);
//   cout << "fieldtest_ID = " << fieldtest_ID << endl;

   string start_timestamp;
   plumedatabasefunc::retrieve_fieldtest_metadata_from_database(
      postgis_db_ptr,fieldtest_ID,start_timestamp);
   cout << "start_timestamp = " << start_timestamp << endl;
   Clock clock;
   cout.precision(13);

   bool UTC_flag=true;
   double epoch=
      clock.timestamp_string_to_elapsed_secs(start_timestamp,UTC_flag);
   int year=clock.get_year();
   string month_name=clock.get_month_name();

   int day_number;
   string experiment_label;
   plumedatabasefunc::retrieve_mission_metadata_from_database(
      postgis_db_ptr,fieldtest_ID,mission_ID,
      day_number,experiment_label);

   string experiment_subdir="/data/ImageEngine/plume/";
   experiment_subdir += month_name+stringfunc::number_to_string(year)+"/Day";
   experiment_subdir += stringfunc::number_to_string(day_number)+"/"+
      experiment_label+"/";
//   cout << "experiment_subdir = " << experiment_subdir << endl;

   string packages_subdir=experiment_subdir+"packages/";
   filefunc::dircreate(packages_subdir);

   string results_subdir=bundler_IO_subdir+"plume_results/";
   results_subdir += stringfunc::number_to_string(day_number)+
      experiment_label+"/";
   cout << "results_subdir = " << results_subdir << endl;
   filefunc::dircreate(results_subdir);
   string temporal_composites_subdir=results_subdir+"temporal_composites/";
   filefunc::dircreate(temporal_composites_subdir);

   string TDP_subdir=results_subdir+"TDP/";
   cout << "TDP_subdir = " << TDP_subdir << endl;
   filefunc::dircreate(TDP_subdir);

// Read tripod camera info from sensors table of plume database:

   vector<int> camera_ID;
   vector<double> focal_param,u0,v0,azimuth,elevation,roll;
   vector<double> x_posn,y_posn,z_posn;
   plumedatabasefunc::retrieve_camera_metadata_from_database(
      postgis_db_ptr,mission_ID,
      camera_ID,focal_param,u0,v0,azimuth,elevation,roll,
      x_posn,y_posn,z_posn);

   double camera_delta_z=0;
//   cout << "camera_delta_z in meters:" << endl;
//   cin >> camera_delta_z;
   
// Instantiate tripod cameras:

   int n_cameras=camera_ID.size();	// n_cameras = 10
   bounding_box bbox;
   vector<camera*> camera_ptrs;
   for (int i=0; i<n_cameras; i++)
   {
      camera* camera_ptr=new camera();
      camera_ptrs.push_back(camera_ptr);
      
      camera_ptr->set_internal_params(
         focal_param[i],focal_param[i],u0[i],v0[i]);
      camera_ptr->set_UV_corners(0,2*u0[i],0,2*v0[i]);
      camera_ptr->set_rel_az(azimuth[i]);
      camera_ptr->set_rel_el(elevation[i]);
      camera_ptr->set_rel_roll(roll[i]);

      double z_camera=z_posn[i]+camera_delta_z;
      camera_ptr->set_world_posn(threevector(x_posn[i],y_posn[i],z_camera));
      camera_ptr->construct_projection_matrix();

      camera_ptr->compute_fields_of_view(2*u0[i],0,2*v0[i],0);
      bbox.update_bounds(camera_ptr->get_world_posn());
//      cout << "i = " << i << " camera = " << *camera_ptr << endl;
   }
//   cout << "bbox = " << bbox << endl;

   int start_slice_number,stop_slice_number,slice_number_step;
   cout << endl;
   cout << "Enter starting time slice number:" << endl;
   cin >> start_slice_number;

   cout << "Enter stopping time slice number:" << endl;
   cin >> stop_slice_number;

   int min_slice_number,max_slice_number;
   plumedatabasefunc::retrieve_extremal_slice_IDs_from_database(
      postgis_db_ptr,mission_ID,min_slice_number,max_slice_number);
//   cout << "min_slice_number = " << min_slice_number
//        << " max_slice_number = " << max_slice_number << endl;
   start_slice_number=basic_math::max(start_slice_number,min_slice_number);
   stop_slice_number=basic_math::min(stop_slice_number,max_slice_number);
   cout << "start_slice_number = " << start_slice_number
        << " stop_slice_number = " << stop_slice_number << endl;

   slice_number_step=1;
   cout << "Enter time slice number step:" << endl;
   cin >> slice_number_step;

   int max_n_unseen_masks=2;
   cout << endl;
   cout << "Plume voxels are generally classified as 'smoke' by some number of tripod" << endl;
   cout << " cameras less than 10.  Enter maximum number of cameras allowed to " << endl;
   cout << " NOT classify voxel as 'smoke' and yet voxel will still be classified as 'smoke'" << endl;
   cout << " (Suggested value for max_n_unseen_masks=2):" 
        << endl;
   cin >> max_n_unseen_masks;

   double smoke_threshold_prob=0.3;	// Nov 2012 Expt 2H
   cout << "Enter voxel 'smokiness' threshold:" << endl;
   cin >> smoke_threshold_prob;

   int camera_ID_offset=-1;
   vector<texture_rectangle*> temporal_composite_texture_rectangle_ptrs;
   if (fieldtest_ID==1)
   {
      camera_ID_offset=17;
   }
   else if (fieldtest_ID==2)
   {
      camera_ID_offset=1;

// Import temporal composite masks generated by program TEMPORAL_COMPOSITES:

      for (int m=0; m<n_cameras; m++)
      {
         int camera_ID=m+camera_ID_offset;
         string temporal_composite_filename=temporal_composites_subdir+
            "temporal_composite_"+stringfunc::integer_to_string(camera_ID,2)
            +".jpg";

         texture_rectangle* texture_rectangle_ptr=
            new texture_rectangle(temporal_composite_filename,NULL);
         temporal_composite_texture_rectangle_ptrs.push_back(
            texture_rectangle_ptr);

// Convert temporally composited masks from hsv to greyscale.  Reset
// any "cold" entry in *texture_rectangle_ptr to black:

         double hue_threshold=120;	// green
         double value_threshold=0.5;
         int width=texture_rectangle_ptr->getWidth();
         int height=texture_rectangle_ptr->getHeight();

         int R,G,B;
         double h,s,v,r,g,b;
         for (int py=0; py<height; py++)
         {
            for (int px=0; px<width; px++)
            {
               texture_rectangle_ptr->get_pixel_hsv_values(px,py,h,s,v);

// On 1/16/13, we empirically found that some "red" hue values range
// around 358-360.  So we add a small fudge to h and then reset hue
// values to range from 0 to 360:

               h += 2;
               h=basic_math::phase_to_canonical_interval(h,0,360);

               R=G=B=0;
               if (h < hue_threshold && v > value_threshold)
               {
                  colorfunc::hsv_to_RGB(h,s,v,r,g,b);
                  R=255*r;
                  G=255*g;
                  B=255*b;
               }
               texture_rectangle_ptr->set_pixel_RGB_values(px,py,R,G,B);
            } // loop over px index
         } // loop over py index

      } // loop over index m labeling cameras

   } // fieldtest_ID conditional

//   double voxel_binsize=0.05; // meter  Expt 2B
//   double voxel_binsize=0.1;	 // meter  (OK for big plume events)
//   double voxel_binsize=0.15;	 // meter   Nov 2012 Expt 2H 
   double voxel_binsize=0.175;	 // meter   Nov 2012 Expt 2H 
//   double voxel_binsize=0.2;	 // meter   Nov 2012 Expt 2H 
   double voxel_volume=voxel_binsize*voxel_binsize*voxel_binsize;
   cout << "voxel_binsize = " << voxel_binsize << endl;
   outputfunc::enter_continue_char();
   
// ========================================================================
// Loop over slice number starts here
// ========================================================================

   VolumetricCoincidenceProcessor* VCP_ptr=NULL;
   for (int slice_number=start_slice_number; slice_number <= stop_slice_number;
        slice_number += slice_number_step)
   {
      cout << "**************************************************************"
           << endl;
      cout << "Processing slice_number = " << slice_number << endl;
      outputfunc::print_elapsed_time();

// Retrieve time-synced info from photos table of plume database:

      vector<int> photo_ID,sensor_ID;
      vector<string> URL,thumbnail_URL,mask_URL;
      plumedatabasefunc::retrieve_photo_metadata_from_database(
         postgis_db_ptr,fieldtest_ID,mission_ID,slice_number,
         photo_ID,sensor_ID,URL,thumbnail_URL,mask_URL);

      int n_photos=photo_ID.size();
      cout << "Number of tripod calibration photos  = " << n_photos << endl;

// Ignore time slice if its number of synchronized photos is less than
// some reasonable minimal number:

      const int min_n_photos=8;
//      const int min_n_photos=10;
      if (n_photos < min_n_photos) continue;

// Export package files corresponding to smoke photos using camera
// metadata retrieved from plume database:

      double frustum_sidelength=3;	// meters
      camera* camera_ptr=NULL;
      for (int i=0; i<n_photos; i++)
      {
         int camera_ID=sensor_ID[i];
//         cout << "i = " << i << " camera_ID = " << camera_ID << endl;
         camera_ptr=camera_ptrs[camera_ID-camera_ID_offset];
         
         string package_filename=packages_subdir+
            "smoke_"+stringfunc::number_to_string(camera_ID)
            +"_t"+stringfunc::integer_to_string(slice_number,3)+".pkg";
         cout << "Exported package_filename = " << package_filename << endl;
         string photo_filename=experiment_subdir+filefunc::getbasename(URL[i]);
         if (fieldtest_ID==2)
         {
            photo_filename=URL[i];
         }
//         cout << "photo_filename = " << photo_filename << endl;
         camera_ptr->write_camera_package_file(
            package_filename,photo_ID[i],photo_filename,frustum_sidelength);
      }

// Read mask information from photos table of plume database.
// Convert contents of imported masks into texture rectangles and
// store within STL vector mask_texture_rectangle_ptrs.  Also record
// non-zero mask intensities within an STL vector:

      vector<double> mask_intensities;
      vector<texture_rectangle*> mask_texture_rectangle_ptrs;
      for (int m=0; m<n_photos; m++)
      {
         string dirname=filefunc::getdirname(mask_URL[m]);
         string basename=filefunc::getbasename(mask_URL[m]);
         string prefix=stringfunc::prefix(basename);
         string suffix=stringfunc::suffix(basename);
         
         string time_avgd_mask_filename=dirname+"TimeAvgd_2_"+basename;
         if (fieldtest_ID >= 2)
            time_avgd_mask_filename=dirname+"TimeAvgd_0_"+basename;
         if (suffix=="bin") 
         {
            time_avgd_mask_filename += ".jpg";
         }

//         cout << "m = " << m 
//              << " mask filename = " << mask_URL[m] 
//              << " time avgd mask filename = " << time_avgd_mask_filename
//              << endl;

         texture_rectangle* mask_texture_rectangle_ptr=new texture_rectangle(
            time_avgd_mask_filename,NULL);
//            mask_URL[m],NULL);
         mask_texture_rectangle_ptrs.push_back(mask_texture_rectangle_ptr);

         for (int px=0; px<mask_texture_rectangle_ptr->getWidth(); px++)
         {
            for (int py=0; py<mask_texture_rectangle_ptr->getHeight(); py++)
            {
               int R,G,B;
               mask_texture_rectangle_ptr->get_pixel_RGB_values(
                  px,py,R,G,B);
               double r=double(R)/255.0;
               double g=double(G)/255.0;
               double b=double(B)/255.0;
               double h,s,v;
               colorfunc::RGB_to_hsv(r,g,b,h,s,v);

               const double TINY=0.0001;
               if (v > TINY) mask_intensities.push_back(v);
            } // loop over py index
         } // loop over px index
      } // loop over index m labeling masks

// Generate and export distribution functions for non-zero mask
// intensities:

      int n_output_bins=100;
      prob_distribution prob_mask_intensities(mask_intensities,n_output_bins);
      prob_mask_intensities.set_xmin(0);
      prob_mask_intensities.set_xlabel("Mask Intensities");
      prob_mask_intensities.writeprobdists(false);

      double mask_intensity_threshold_frac=0.05;       
//      double mask_intensity_threshold_frac=0.10;       
//      double mask_intensity_threshold_frac=0.20;       
//      if (fieldtest_ID==2) mask_intensity_threshold_frac=0.5;

      double mask_intensity_threshold=prob_mask_intensities.
         find_x_corresponding_to_pcum(mask_intensity_threshold_frac);
      cout << "Mask intensity threshold = " << mask_intensity_threshold 
           << endl;

      double xmin=bbox.get_xmin();
      double xmax=bbox.get_xmax();
      double ymin=bbox.get_ymin();
      double ymax=bbox.get_ymax();
      double zmin=0;
//      double zmax=5;	// meters  small plume expt 2B
//      double zmax=25;	// meters  (OK for big plume expt 5C)
      double zmax=22;	// Max visible height for all 10 video cameras 
			//   in Expt 2H = 19 meters

      double plume_volume=-1;

// ========================================================================
// Perform first round of smoke plume volume determination using
// coarse steps through VCP lattice.  After refining XYZ_min and
// XYZ_max, recompute plume volume using fine steps through smaller
// VPC lattice.

      int n_iters=2;
      int mstep=5;
      int nstep=5;
      int pstep=5;

      for (int iter=0; iter < n_iters; iter++)
      {
         if (iter > 0)
         {
            mstep=nstep=pstep=1;
         }

         string banner="Iteration "+stringfunc::number_to_string(iter)+
            " : mstep=nstep=pstep="+stringfunc::number_to_string(mstep);
         outputfunc::write_big_banner(banner);

// Instantiate VCP to hold 3D voxel lattice:

         delete VCP_ptr;
         VCP_ptr=new VolumetricCoincidenceProcessor();

         threevector XYZ_min(xmin,ymin,zmin);
         threevector XYZ_max(xmax,ymax,zmax);
         cout << "XYZ_min = " << XYZ_min << " XYZ_max = " << XYZ_max << endl;
      
         VCP_ptr->initialize_coord_system(XYZ_min,XYZ_max,voxel_binsize);
         cout << "VCP = " << *VCP_ptr << endl;

         xmin=POSITIVEINFINITY;
         xmax=NEGATIVEINFINITY;
         ymin=POSITIVEINFINITY;
         ymax=NEGATIVEINFINITY;
         zmin=POSITIVEINFINITY;
         zmax=NEGATIVEINFINITY;

         int mdim=VCP_ptr->get_mdim();
         int ndim=VCP_ptr->get_ndim();
         int pdim=VCP_ptr->get_pdim();
         int n_voxels=mdim*ndim*pdim;

// Iterate over all voxels in *VCP_ptr.  Project each voxel into all
// tripod cameras' image planes.  Mark those voxels whose projections
// land inside some specified number of image plane mask:

         int n_rejected_voxels=0;
         int R,G,B;
         double u,v;
         threevector voxel_posn;
         for (int m=0; m<mdim; m += mstep)
         {
            outputfunc::update_progress_fraction(m,0.05*mdim,mdim);
            double x=VCP_ptr->m_to_x(m);
            voxel_posn.put(0,x);
            for (int n=0; n<ndim; n += nstep)
            {
               double y=VCP_ptr->n_to_y(n);
               voxel_posn.put(1,y);

               for (int p=0; p<pdim; p += pstep)
               {
                  double z=VCP_ptr->p_to_z(p);
                  voxel_posn.put(2,z);

// As of 1/28/13, we have empirically found that arithmetic averaging
// of VIBE masks yields reasonable, filled 3D hulls whose volumes are
// very insensitive to the voxel smokiness threshold value:


// As of 12/22/11, Nick's proposal to use geometric rather than
// arithmetic means for voxel smoke probability voting yields the best
// density distribution with the fewest number of ad-hoc tuning
// parameters:

                  double smoke_prob=0;       	// arithmetic mean
//                  double smoke_prob=1;		// geometric mean

// Create STL vector containing boolean indicators for antipodal
// camera pairs:

                  vector<int> antipodal_pairs;
                  for (int ap=0; ap<5; ap++)
                  {
                     antipodal_pairs.push_back(0);
                  }

// FAKE FAKE:  Weds Jan 4, 2012 at 9:44 am
// Hardwire n_photos to 1 for viewgraph purposes only!!!

//                  n_photos=1;
//                  n_photos=2;
//                  n_photos=3;

                  int n_seen_masks=0;
                  vector<double> mask_values;
//                  double weight_sum=0;
//                   vector<double> weights;
                  for (int c=0; c<n_photos; c++)
                  {
                     int curr_camera_ID=sensor_ID[c];
                     int antipodal_pair_ID=modulo(
                        curr_camera_ID-camera_ID_offset,5);

/*
// FAKE FAKE:  Weds Jan 4, 2012 at 9:44 am
// Hardwire curr_camera_ID for viewgraph purposes only!!!

                     if (c==0)
                     {
                        curr_camera_ID=17;
                     }
                     else if (c==1)
                     {
                        curr_camera_ID=20;
                     }
                     else if (c==1)
                     {
                        curr_camera_ID=22;
                     }
*/
                  
                     camera_ptr=camera_ptrs.at(
                        curr_camera_ID-camera_ID_offset);

// Make sure threevector(x,y,z) lies in front of camera!

                     if (!camera_ptr->XYZ_in_front_of_camera(x,y,z)) continue;

                     camera_ptr->project_XYZ_to_UV_coordinates(x,y,z,u,v);
//                     cout << "x = " << x << " y = " << y << " z = " << z
//                          << " u = " << u << " v = " << v << endl;
 
// Make sure (U,V) lies within camera's image plane!

                     if (!camera_ptr->get_UV_bbox_ptr()->point_inside(u,v)) 
                        continue;

/*
// As of 1/16/13, we require U,V to land on a non-null entry within
// temporal_composite_texture_rectangle:
                  
                     if (temporal_composite_texture_rectangle_ptrs.size() > 0)
                     {
                        int R,G,B;
                        temporal_composite_texture_rectangle_ptrs[c]->
                           get_RGB_values(u,v,R,G,B);
                        if (R==0 && G==0 && B==0) continue;
                     }
*/

                     R=G=B=0;
                     mask_texture_rectangle_ptrs[c]->get_RGB_values(
                        u,v,R,G,B);
                     double r=double(R)/255.0;
                     double g=double(G)/255.0;
                     double b=double(B)/255.0;
                     double h,s,v;
                     colorfunc::RGB_to_hsv(r,g,b,h,s,v);

// Ignore v if it lies below non-vanishing mask intensity threshold:

                     mask_values.push_back(v);

                     double curr_factor=0;	// arithmetic mean
//                     double curr_factor=1;      // geometric mean 

                     if (v > mask_intensity_threshold)
                     {
                        curr_factor=v;
                        n_seen_masks++;
                        antipodal_pairs[antipodal_pair_ID]=1;
                     }

/*
                     double curr_sqr_range=(
                        voxel_posn-camera_ptr->get_world_posn()).
                        sqrd_magnitude();
                     double curr_weight=1.0/curr_sqr_range;
//                     curr_weight=basic_math::min(1.0,curr_weight);
                     curr_weight=1;
                     weights.push_back(curr_weight);
                     weight_sum += curr_weight;
*/
                   
                     smoke_prob += curr_factor;      	// arithmetic mean
//                     smoke_prob *= curr_factor;		// geometric mean
                  } // loop over index c labeling cameras

/*
// Renormalize weights so that they sum to unity:

                  smoke_prob=1;
                  for (int k=0; k<weights.size(); k++)
                  {
                     weights[k]=weights[k]/weight_sum;
                     smoke_prob *= pow(mask_values[k],weights[k]);
                  }
*/

// FAKE FAKE:  Thurs Jan 17, 2013 at 8:16 am
// Experiment with commenting out next set of antipodal camera pair
// conditions

/*
// Count number of antipodal camera pairs that observed the current
// voxel.  If the number is less than specified threshold, reject
// candidate voxel from further consideration:

                  int antipodal_pairs_integral=0;
                  for (int ap=0; ap<5; ap++)
                  {
                     antipodal_pairs_integral += antipodal_pairs[ap];
                  }

                  int antipodal_camera_pairs_threshold=0.5*n_photos;
                  if (antipodal_pairs_integral < 
                  antipodal_camera_pairs_threshold) 
                  {
                     n_rejected_voxels++;
                     continue;
                  }
*/

//                  n_seen_masks=mask_values.size();
                  if (n_seen_masks < n_photos-max_n_unseen_masks)
                  {
                     n_rejected_voxels++;
                     continue;
                  }

// Arithmetic averaging:

                  double avg_smoke_prob=smoke_prob/n_seen_masks;


/*
// Geometric averaging:

                  double avg_smoke_prob=0;
                  if (smoke_prob < 1)
                  {
                     avg_smoke_prob=pow(smoke_prob,1.0/double(n_seen_masks));
                  }
*/

// Median value:

//                  double avg_smoke_prob=mathfunc::median_value(mask_values);

                
                  if (avg_smoke_prob < smoke_threshold_prob) continue;
//                  cout << "avg_smoke_prob = " << avg_smoke_prob << endl;

//               long key=VCP_ptr->mnp_to_key(m,n,p);
//               VCP_ptr->increment_voxel_counts(key,n_projs_inside);
//               VCP_ptr->set_voxel_prob(key,avg_smoke_prob);

                  avg_smoke_prob=basic_math::min(1.0,avg_smoke_prob);
                  VCP_ptr->set_voxel_prob(m,n,p,avg_smoke_prob);

                  xmin=basic_math::min(xmin,x);
                  xmax=basic_math::max(xmax,x);
                  ymin=basic_math::min(ymin,y);
                  ymax=basic_math::max(ymax,y);
                  zmin=basic_math::min(zmin,z);
                  zmax=basic_math::max(zmax,z);

               } // loop over index p
            } // loop over index n
         } // loop over index m
         cout << endl;

         plume_volume=VCP_ptr->size()*voxel_volume;
         
         cout << "VCP frac = " << double(VCP_ptr->size())/double(n_voxels)
              << " plume volume = " << plume_volume << endl;

         cout << "n_rejected_voxels = " << n_rejected_voxels << endl;
         double rejected_voxel_frac=
            double(n_rejected_voxels*mstep*nstep*pstep)/
            double(mdim*ndim*pdim);
         cout << "rejected_voxel_frac = " << rejected_voxel_frac << endl;

//      VCP_ptr->renormalize_counts_into_probs();
//      cout << "VCP = " << *VCP_ptr << endl;

      } // loop over iter index

      for (int m=0; m<mask_texture_rectangle_ptrs.size(); m++)
      {
         delete mask_texture_rectangle_ptrs[m];
      }

// ========================================================================   

// If plume volume is very small, move on to next time slice:

      const double min_plume_volume=0.01;	// meter**2
      if (plume_volume < min_plume_volume) 
      {
         cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
         cout << "Negligibly small plume reconstructed for time_slice = "
              << slice_number << endl;
         cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
         continue;
      }

// Write out XYZP point cloud to TDP and OSGA files.  Calculate
// plume's total volume as product of its number of non-zero voxels
// times voxel_volume:

      double min_prob_threshold=0;
//   bool perturb_voxels_flag=false;
      bool perturb_voxels_flag=true;
      vector<double>* X_ptr=new vector<double>;
      vector<double>* Y_ptr=new vector<double>;
      vector<double>* Z_ptr=new vector<double>;
      vector<double>* P_ptr=new vector<double>;

      int npoints=VCP_ptr->size();
      X_ptr->reserve(npoints);
      Y_ptr->reserve(npoints);
      Z_ptr->reserve(npoints);
      P_ptr->reserve(npoints);

      VCP_ptr->retrieve_XYZP_points(
         X_ptr,Y_ptr,Z_ptr,P_ptr,min_prob_threshold,perturb_voxels_flag);

      cout << "npoints = " << npoints << endl;
      cout << "Z_ptr->size() = " << Z_ptr->size() << endl;
      cout << "Total plume volume = " << Z_ptr->size() * voxel_volume
           << " m**3 " << endl;
//   cout << "VCP volume frac = " << double(VCP_ptr->size())/double(n_voxels)
//        << endl;

// Generate and export distribution function for *P_ptr:

      n_output_bins=100;
      prob_distribution prob_P(*P_ptr,n_output_bins);
      prob_P.set_xmin(0);
      prob_P.set_xlabel("Voxel 'smokiness'");
      prob_P.writeprobdists(false);

// Ignore all voxels with p values less than some cumulative threshold
// distribution value:

      vector<double>* cropped_X_ptr=new vector<double>;
      vector<double>* cropped_Y_ptr=new vector<double>;
      vector<double>* cropped_Z_ptr=new vector<double>;
      vector<double>* cropped_P_ptr=new vector<double>;
      cropped_P_ptr->reserve(P_ptr->size());

//      const double threshold_frac=0.20;
//      double P_threshold=prob_P.find_x_corresponding_to_pcum(threshold_frac);
//      cout << "P_threshold = " << P_threshold << endl;
      double P_threshold=0;
      for (int i=0; i<P_ptr->size(); i++)
      {
         double curr_P=P_ptr->at(i);
         if (curr_P < P_threshold) continue;
         cropped_X_ptr->push_back(X_ptr->at(i));
         cropped_Y_ptr->push_back(Y_ptr->at(i));
         cropped_Z_ptr->push_back(Z_ptr->at(i));
         cropped_P_ptr->push_back(P_ptr->at(i));
      }

      cout << "P_ptr->size() = " << P_ptr->size()
           << " cropped P_ptr->size() = " << cropped_P_ptr->size()
           << endl;

      delete X_ptr;
      delete Y_ptr;
      delete Z_ptr;
      delete P_ptr;

// Write TDP & OSGA files containing XYZP points for 3D hull:

      string tdp_filename=
         "plume_hull_"+stringfunc::integer_to_string(slice_number,3)+".tdp";
      tdpfunc::write_xyzp_data(
         tdp_filename,"",Zero_vector,
         cropped_X_ptr,cropped_Y_ptr,cropped_Z_ptr,cropped_P_ptr);

      delete cropped_X_ptr;
      delete cropped_Y_ptr;
      delete cropped_Z_ptr;
      delete cropped_P_ptr;

      string unix_cmd="lodtree "+tdp_filename;
      sysfunc::unix_command(unix_cmd);
      string osga_filename=
         "plume_hull_"+stringfunc::integer_to_string(slice_number,3)+".osga";

      unix_cmd="mv "+tdp_filename+" "+TDP_subdir;
      sysfunc::unix_command(unix_cmd);
      unix_cmd="mv "+osga_filename+" "+results_subdir;
      sysfunc::unix_command(unix_cmd);

      string banner="Finished writing OSGA file "+results_subdir+osga_filename;
      outputfunc::write_big_banner(banner);

   } // loop over slice_number index

// ========================================================================
// Loop over slice number stops here
// ========================================================================

   delete VCP_ptr;

   for (int i=0; i<temporal_composite_texture_rectangle_ptrs.size(); i++)
   {
      delete temporal_composite_texture_rectangle_ptrs[i];
   }
   for (int i=0; i<n_cameras; i++)
   {
      delete camera_ptrs[i];
   }
   delete postgis_databases_group_ptr;

}


