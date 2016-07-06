// =======================================================================
// Program HOMORECT imports SIFT/ASIFT feature tracks generated by
// program ASIFTVID.  It also takes in package files for the n video
// frames processed by ASIFTVID which contain hardware-based camera
// parameters.  Working with adjacent pairs of video frames, HOMORECT
// purges any feature which doesn't reasonably satisfy a homography
// relationship between temporally neighboring image planes.  It then
// backprojects surviving 2D features onto a ground Z-plane.  HOMORECT
// recovers parameters for the "next" 3x4 projection matrix based upon
// the homography between the backprojected ground plane points and
// their "next" imageplane counterparts.

/*

  ./homorect \
  --region_filename ./packages/photo_0000.pkg \
  --region_filename ./packages/photo_0010.pkg \
  --region_filename ./packages/photo_0020.pkg \
  --region_filename ./packages/photo_0030.pkg 

*/

// =======================================================================
// Last updated on 2/6/13; 2/7/13
// =======================================================================

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "video/camerafuncs.h"
#include "osg/osgFeatures/FeaturesGroup.h"
#include "general/filefuncs.h"
#include "geometry/geometry_funcs.h"
#include "geometry/homography.h"
#include "geometry/linesegment.h"
#include "math/ltthreevector.h"
#include "passes/PassesGroup.h"
#include "video/photogroup.h"
#include "geometry/plane.h"
#include "general/sysfuncs.h"
#include "osg/osg3D/tdpfuncs.h"
#include "time/timefuncs.h"

using std::cout;
using std::endl;
using std::map;
using std::ofstream;
using std::pair;
using std::string;

int main( int argc, char** argv ) 
{
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);
   int videopass_ID=passes_group.get_videopass_ID();
//   cout << "videopass_ID = " << videopass_ID << endl;
   string image_list_filename=passes_group.get_image_list_filename();
   cout << "image_list_filename = " << image_list_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(image_list_filename);
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;

// Read photographs from input video passes:

   photogroup* photogroup_ptr=new photogroup;
   photogroup_ptr->read_photographs(passes_group);
   int n_images=photogroup_ptr->get_n_photos();
   cout << "n_images = " << n_images << endl;

   threevector camera_world_COM(Zero_vector);
   vector<double> xdims,ydims;
   vector<camera*> camera_ptrs;
   for (int n=0; n<n_images; n++)
   {
      photograph* photo_ptr=photogroup_ptr->get_photograph_ptr(n);
      camera* camera_ptr=photo_ptr->get_camera_ptr();
      camera_ptrs.push_back(camera_ptr);
      xdims.push_back(photo_ptr->get_xdim());
      ydims.push_back(photo_ptr->get_ydim());
      camera_world_COM += camera_ptr->get_world_posn();
   }
   camera_world_COM /= n_images;
   cout << "In UTM geocoords, camera_world_COM = " << camera_world_COM << endl;

   double ground_z=10;	// meters (HAFB)
//    outputfunc::enter_continue_char();

// Laterally translate camera world positions in order to avoid
// working with numerically large easting and northing coordinates.
// Vertically translate camera world positions so that ground plane is
// redefined as Z=0:

   int n_cameras=camera_ptrs.size();
   for (int n=0; n<n_cameras; n++)
   {
      camera* camera_ptr=camera_ptrs[n];
      threevector camera_world_posn=camera_ptr->get_world_posn();

//      camera_world_posn -= camera_world_COM;
      
      camera_world_posn=camera_world_posn 
         - camera_world_COM.get(0)*x_hat
         - camera_world_COM.get(1)*y_hat 
         - ground_z*z_hat;
      
      camera_ptr->set_world_posn(camera_world_posn);
      camera_ptr->construct_projection_matrix(false);
//      genmatrix* P_ptr=camera_ptr->get_P_ptr();
//      cout << "n = " << n << " reset P = " << *P_ptr << endl;
   }

   ground_z=0;

   timefunc::initialize_timeofday_clock();

// --------------------------------------------------------------------------

   FeaturesGroup* FeaturesGroup_ptr=new FeaturesGroup(
      ndims,passes_group.get_pass_ptr(videopass_ID),NULL);

   string features_subdir=bundler_IO_subdir+"/features/";
   FeaturesGroup_ptr->read_in_photo_features(photogroup_ptr,features_subdir);

   int n_orig_features=FeaturesGroup_ptr->get_n_Graphicals();
   double curr_time=FeaturesGroup_ptr->get_curr_t();
//   int max_n_passes=FeaturesGroup_ptr->get_max_n_passes(curr_time);
   int max_n_passes=n_images;
   cout << "n_orig_features = " << n_orig_features << endl;
   cout << "curr_time = " << curr_time << endl;
   cout << "max_n_passes = " << max_n_passes << endl;

//      const double max_d_chi=0.003;
//      const double max_d_chi=0.01;
   double max_d_chi=0.0335;
//   double max_d_chi=0.1;
//   cout << "Enter max_d_chi:" << endl;
//   cin >> max_d_chi;

// Scan through all imported features.  Purge any which are grossly
// inconsistent with homography relationships:

   for (int curr_pass_number=0; curr_pass_number<n_images-1; 
        curr_pass_number++)
   {
      int next_pass_number=curr_pass_number+1;

      threevector UVW;
      vector<int> feature_ID;
      vector<twovector> UV_curr,UV_next;

      int n_features=FeaturesGroup_ptr->get_n_Graphicals();
      for (int f=0; f<n_features; f++)
      {
         Feature* Feature_ptr=FeaturesGroup_ptr->get_Feature_ptr(f);

         instantaneous_obs* instantaneous_obs_ptr=
            Feature_ptr->get_all_particular_time_observations(curr_time);
         instantaneous_obs::THREEVECTOR_MAP* threevector_map_ptr=
            instantaneous_obs_ptr->get_multiple_image_coords_ptr();
         instantaneous_obs::THREEVECTOR_MAP::iterator iter;

         iter=threevector_map_ptr->find(curr_pass_number);
         if (iter==threevector_map_ptr->end()) continue;
         threevector curr_UVW=iter->second;

         iter=threevector_map_ptr->find(next_pass_number);
         if (iter==threevector_map_ptr->end()) continue;
         threevector next_UVW=iter->second;
      
//      cout << "f = " << f << " feature_ID = " << feature_ID
//           << " threevector_map.size() = " << threevector_map_ptr->size()
//           << endl;
//      cout << "   U0 = " << curr_UVW.get(0) 
//           << " V0 = " << curr_UVW.get(1) 
//           << " U1 = " << next_UVW.get(0)
//           << " V1 = " << next_UVW.get(1)
//           << endl;

         feature_ID.push_back(Feature_ptr->get_ID());
         UV_curr.push_back(twovector(curr_UVW));
         UV_next.push_back(twovector(next_UVW));

      } // loop over index f labeling features

//   cout << "UV_curr.size() = " << UV_curr.size()
//        << " UV_next.size() = " << UV_next.size() << endl;

      homography H;
      H.parse_homography_inputs(UV_curr,UV_next);
      H.compute_homography_matrix();

      bool print_flag=false;
      H.check_homography_matrix(UV_curr,UV_next,feature_ID,UV_curr.size(),
         print_flag);
   
      UV_curr.clear();
      UV_next.clear();
      UV_curr=H.get_XY_sorted();
      UV_next=H.get_UV_sorted();
      vector<double> d_chi=H.get_d_chi();

// Prune any feature whose d_chi value exceeds some maximal
// threshold.  Such features do NOT satisfy a reasonable homography
// relationship between the current and next image planes.  So its
// tiepoint values are suspect:

      for (unsigned int i=0; i<d_chi.size(); i++)
      {
         if (d_chi[i] > max_d_chi) 
         {
            int curr_ID=feature_ID[i];
            FeaturesGroup_ptr->destroy_feature(curr_ID);
            continue;
         }

//         cout << "i = " << i << " Feature ID = " << feature_ID[i]
//              << " U = " << UV_curr[i].get(0)
//              << " V = " << UV_curr[i].get(1)
//              << " U' = " << UV_next[i].get(0)
//              << " V' = " << UV_next[i].get(1)
//              << " d_chi = " << d_chi[i]
//              << endl;
      }
      int n_reduced_features=FeaturesGroup_ptr->get_n_Graphicals();
      cout << "Original n_features = " << n_orig_features << endl;
      cout << "Current pass # = " << curr_pass_number << endl;
      cout << "Number of features obeying homography relns ="
           << n_reduced_features << endl;

   } // loop over curr_pass_number

/*
// Export pruned features to output text files:

//   for (int curr_pass_number=0; curr_pass_number<n_images-1; 
   for (int curr_pass_number=0; curr_pass_number<2;
        curr_pass_number++)
   {
      int next_pass_number=curr_pass_number+1;

      photograph* photo_ptr=photogroup_ptr->get_photograph_ptr(
         curr_pass_number);
      string photo_filename=photo_ptr->get_filename();
      string basename=filefunc::getbasename(photo_filename);
      string prefix=stringfunc::prefix(basename);
      string features_subdir=bundler_IO_subdir+"images/";
      string features_filename=features_subdir+"features_2D_"+prefix+".txt";

      ofstream outstream;
      filefunc::openfile(features_filename,outstream);
      
      outstream << "# time feature-ID image-ID U V Freq Curr_image_feature_index" 
                << endl;
      outstream << endl;
      
      outstream.precision(12);
      for (int f=0; f<FeaturesGroup_ptr->get_n_Graphicals(); f++)
      {
         Feature* Feature_ptr=FeaturesGroup_ptr->get_Feature_ptr(f);

         instantaneous_obs* instantaneous_obs_ptr=
            Feature_ptr->get_all_particular_time_observations(curr_time);
         instantaneous_obs::THREEVECTOR_MAP* threevector_map_ptr=
            instantaneous_obs_ptr->get_multiple_image_coords_ptr();
         instantaneous_obs::THREEVECTOR_MAP::iterator iter;

         iter=threevector_map_ptr->find(curr_pass_number);
         if (iter==threevector_map_ptr->end()) continue;
         twovector curr_UV(iter->second);

         if (curr_pass_number==0)
         {
            iter=threevector_map_ptr->find(curr_pass_number+1);
            if (iter==threevector_map_ptr->end()) continue;
         }
         if (curr_pass_number==1)
         {
            iter=threevector_map_ptr->find(curr_pass_number-1);
            if (iter==threevector_map_ptr->end()) continue;
         }

         outstream << "0.00  " 
                   << Feature_ptr->get_ID() << "  "
                   << f << "  "
                   << curr_UV.get(0) << "  "
                   << curr_UV.get(1) << "  "
                   << " 2 "
                   << Feature_ptr->get_ID() << "  ";
         outstream << endl;
      }
      filefunc::closefile(features_filename,outstream);

   } // loop over curr_pass_number
*/

// Loop over pairs of adjacent images.  Backproject current frame's
// features into ground Z-plane assuming its camera's extrinsic and
// intrinsic parameters are fixed.  Compute 3D rays from next frame to
// Z-plane locations assuming next camera's intrinsic and geolocation
// parameters are fixed.  Adjust next camera's rotation so that it's
// consistent with the Z-plane induced-homography relationship:

   string rectified_images_subdir=bundler_IO_subdir+"rectified_images/";
   filefunc::dircreate(rectified_images_subdir);

   for (int curr_pass_number=0; curr_pass_number<n_images-1; 
        curr_pass_number++)
   {
      cout << "----------------------------------------------------" << endl;
      cout << "curr_pass_number = " << curr_pass_number << endl;
      
      camera* curr_camera_ptr=camera_ptrs[curr_pass_number];
      int next_pass_number=curr_pass_number+1;
      camera* next_camera_ptr=camera_ptrs[next_pass_number];
      threevector next_world_posn=next_camera_ptr->get_world_posn();

      cout << "curr_camera = " << *curr_camera_ptr << endl;

      vector<twovector> curr_UV,next_UV,curr_XY;
      vector<threevector> curr_XYZ;
      int n_features=FeaturesGroup_ptr->get_n_Graphicals();
      for (int f=0; f<n_features; f++)
      {
         Feature* Feature_ptr=FeaturesGroup_ptr->get_Feature_ptr(f);

         instantaneous_obs* instantaneous_obs_ptr=
            Feature_ptr->get_all_particular_time_observations(curr_time);
         instantaneous_obs::THREEVECTOR_MAP* threevector_map_ptr=
            instantaneous_obs_ptr->get_multiple_image_coords_ptr();
         instantaneous_obs::THREEVECTOR_MAP::iterator iter;

         iter=threevector_map_ptr->find(curr_pass_number);
         if (iter==threevector_map_ptr->end()) continue;
         threevector curr_UVW=iter->second;

         iter=threevector_map_ptr->find(next_pass_number);
         if (iter==threevector_map_ptr->end()) continue;
         threevector next_UVW=iter->second;

         curr_UV.push_back(twovector(curr_UVW));
         next_UV.push_back(twovector(next_UVW));

         curr_XYZ.push_back(curr_camera_ptr->backproject_imagepoint_to_zplane(
            curr_UV.back(),ground_z));
         curr_XY.push_back(twovector(curr_XYZ.back()));
      } // loop over index f labeling features

      homography curr_H,next_H;

      curr_H.parse_homography_inputs(curr_XY,curr_UV);
      curr_H.compute_homography_matrix();
//      curr_H.check_homography_matrix(curr_XY,curr_UV);
      cout << "curr_XY -> curr_UV" << endl;
//      outputfunc::enter_continue_char();

      next_H.parse_homography_inputs(curr_XY,next_UV);
      next_H.compute_homography_matrix();
//      double rms_residual=next_H.check_homography_matrix(curr_XY,next_UV);
      cout << "curr_XY -> next_UV" << endl;
//      outputfunc::enter_continue_char();

      genmatrix curr_P(3,4),next_P(3,4);

/*
// First print out parameters for current camera:

      genmatrix curr_K(3,3),curr_Kinv(3,3),curr_RT(3,4);


      curr_K=*(curr_camera_ptr->get_K_ptr());
      curr_Kinv=*(curr_camera_ptr->get_Kinv_ptr());
      cout << "curr_camera->get_K() = " << curr_K << endl;

      rotation* Rcamera_ptr=curr_camera_ptr->get_Rcamera_ptr();
      cout << "Rcamera = " << *Rcamera_ptr << endl;
      double curr_az=curr_camera_ptr->get_rel_az();
      double curr_el=curr_camera_ptr->get_rel_el();
      double curr_roll=curr_camera_ptr->get_rel_roll();
      cout << "curr_az = " << curr_az*180/PI << endl;
      cout << "curr_el = " << curr_el*180/PI << endl;
      cout << "curr_roll = " << curr_roll*180/PI << endl;

      curr_P=*(curr_camera_ptr->get_P_ptr());
      curr_RT=curr_Kinv*curr_P;
      cout << "curr_RT = " << curr_RT << endl;

      rotation curr_R;
      curr_RT.get_smaller_matrix(curr_R);
      cout << "curr_R = " << curr_R << endl;
      cout << "curr_R * currR.trans = " 
           << curr_R * curr_R.transpose() << endl;

      threevector curr_camera_posn=curr_camera_ptr->get_world_posn();
      cout << "curr_camera_posn = " << curr_camera_posn << endl;
      cout << "curr camera P = " << curr_P << endl;

//      outputfunc::enter_continue_char();
*/

// Next derive parameters for current camera from homography curr_H:

/*
      double curr_f;
//      double curr_az,curr_el,curr_roll;
//      threevector curr_camera_posn;
      curr_H.compute_camera_params_from_zplane_homography(
         curr_camera_ptr->get_u0(),
         curr_camera_ptr->get_v0(),
         curr_f,curr_az,curr_el,curr_roll,curr_camera_posn,curr_P);
      curr_camera_ptr->construct_seven_param_projection_matrix();      
*/

//      outputfunc::enter_continue_char();

      double next_f,next_az,next_el,next_roll;
      threevector next_camera_posn;
      next_H.compute_camera_params_from_zplane_homography(
         next_camera_ptr->get_u0(),
         next_camera_ptr->get_v0(),
         next_f,next_az,next_el,next_roll,next_camera_posn,next_P);

      next_camera_ptr->set_f(next_f);
      next_camera_ptr->set_Rcamera(next_az,next_el,next_roll);
      next_camera_ptr->set_world_posn(next_camera_posn);
      next_camera_ptr->construct_seven_param_projection_matrix();

      cout << "...................................................." << endl;
      cout << "next_pass_number = " << next_pass_number << endl;
      cout << "next_camera = " << *next_camera_ptr << endl;

//      outputfunc::enter_continue_char();


/*
// Check ground_z points backprojected from current and corrected next
// cameras:

      threevector avg_delta_XYZ(Zero_vector);
      for (int f=0; f<curr_UV.size(); f++)
      {
         threevector curr_XYZ=
            curr_camera_ptr->backproject_imagepoint_to_zplane(
               curr_UV[f],ground_z);
         threevector next_XYZ=
            next_camera_ptr->backproject_imagepoint_to_zplane(
               next_UV[f],ground_z);
         threevector delta_XYZ=next_XYZ-curr_XYZ;
         delta_XYZ=threevector(fabs(delta_XYZ.get(0)),fabs(delta_XYZ.get(1)),
         fabs(delta_XYZ.get(2)));
         avg_delta_XYZ += delta_XYZ;
         cout << "f = " << f << " curr_XYZ = " << curr_XYZ
              << " next_XYZ = " << next_XYZ << endl;
      }
      avg_delta_XYZ /= curr_UV.size();
      cout << "rms_residual = " << rms_residual << endl;
      cout << "max_d_chi = " << max_d_chi << endl;
      cout << "avg_delta_XYZ = " << avg_delta_XYZ << endl;
      cout << "avg_delta_XYZ.mag = " << avg_delta_XYZ.magnitude() << endl;
      outputfunc::enter_continue_char();
*/

// Export orthorectified versions of current and next images:

      photograph* curr_photo_ptr=photogroup_ptr->get_photograph_ptr(
         curr_pass_number);
      photograph* next_photo_ptr=photogroup_ptr->get_photograph_ptr(
         next_pass_number);
      
//      cout << "----------------------------------------------------" << endl;
//      cout << "*curr_camera_ptr = " << *curr_camera_ptr << endl;
//      cout << "...................................................." << endl;
//      cout << "*next_camera_ptr = " << *next_camera_ptr << endl;

      string curr_image_filename=curr_photo_ptr->get_filename();
      string next_image_filename=next_photo_ptr->get_filename();
      
      double Emin=-500;
      double Emax=-150;
      double Nmin=-2080;
      double Nmax=-1770;

      int rectified_width=curr_photo_ptr->get_xdim();
      int rectified_height=curr_photo_ptr->get_ydim();
      camerafunc::orthorectify_image(
         curr_image_filename,Emin,Emax,Nmin,Nmax,curr_H,
         rectified_images_subdir,rectified_width,rectified_height);

      rectified_width=next_photo_ptr->get_xdim();
      rectified_height=next_photo_ptr->get_ydim();
      camerafunc::orthorectify_image(
         next_image_filename,Emin,Emax,Nmin,Nmax,next_H,
         rectified_images_subdir,rectified_width,rectified_height);

      outputfunc::enter_continue_char();
      
   } // loop over curr_pass_number index

// -----------------------------------------------------------------------
   outputfunc::print_elapsed_time();
}
