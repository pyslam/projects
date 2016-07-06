// ==========================================================================
// Program CAMERA_PARAMS is a stripped down version of program
// BACKPROJECT.  It takes in filtered camera posn and attitude
// information from text file "TPA_filtered.txt".  It also reads in
// best chisq-fit internal camera parameter information derived for
// some small number of video images where 3D/2D tiepoint pairs have
// been manually extracted.  Internal camera parameters before the
// first and after the last of these fitted images are respectively
// set equal to the first/last image values.  Internal parameters for
// other images falling between two fitted images are linearly
// interpolated.

// The intrinsic and extrinsic camera parameters for each video image
// time are also saved to output text file "camera_params.txt".

// ==========================================================================
// Last updated on 10/4/05; 12/13/05
// ==========================================================================

#include <iostream>
#include <set>
#include <string>
#include <vector>
#include "video/camera.h"
#include "general/filefuncs.h"
#include "general/outputfuncs.h"
#include "numerical/param_range.h"
#include "general/stringfuncs.h"
#include "math/threevector.h"
#include "math/twovector.h"
#include "templates/mytemplates.h"

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::flush;
   using std::ofstream;
   using std::pair;
   using std::setw;
   using std::string;
   using std::vector;

// Open ascii file to hold temporal sequence of camera intrinsic and
// extrinsic parameters:

   string camera_params_filename="camera_params.txt";
   ofstream camera_params_stream;
   filefunc::deletefile(camera_params_filename);
   filefunc::openfile(camera_params_filename,camera_params_stream);

// First read in and parse camera position & attitude information:

   string TPA_filtered_filename="TPA_filtered.txt";
   filefunc::ReadInfile(TPA_filtered_filename);
   double X[13];

   vector<pair<int,double> > image_time;
   vector<threevector> rel_camera_XYZ;
   vector<threevector> camera_rpy;

   double best_roll,best_pitch,best_yaw;
   cout << "Enter best fit roll in degrees:" << endl;
   cin >> best_roll;
   cout << "Enter best fit pitch in degrees:" << endl;
   cin >> best_pitch;
   cout << "Enter best fit yaw in degrees:" << endl;
   cin >> best_yaw;

   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      stringfunc::string_to_n_numbers(8,filefunc::text_line[i],X);
      image_time.push_back(pair<int,double>(X[0],X[1]));
      rel_camera_XYZ.push_back(threevector(X[2],X[3],X[4]));

// As of 12/13/05, we adopt best fit values for roll, pitch and yaw
// rather than use filtered GPS/IMU pointing angles:

      camera_rpy.push_back(threevector(best_roll,best_pitch,best_yaw));
//      camera_rpy.push_back(threevector(X[5],X[6],X[7]));
  
 } // loop over index i labeling filtered GPS/IMU information

// Next read in and parse camera parameters determined from chisq
// minimization by program WANDER or BUNDLE:

   outputfunc::newline();
   string subdir="./manual_camera_params/";
   string input_params_filename;
   cout << "Enter file in subdir "+subdir
        << " containing intrinsic params generated by program INTERNALS:" 
        << endl;
   cin >> input_params_filename;
   input_params_filename=subdir+input_params_filename;

   filefunc::ReadInfile(input_params_filename);

//   int n_fields=stringfunc::compute_nfields(filefunc::text_line[0]);
   vector<int> tiepoint_imagenumber;
   vector<camera*> camera_ptrs;
   
   for (unsigned int n=0; n<filefunc::text_line.size(); n++)
   {
      cout << n << " " << flush;
      stringfunc::string_to_n_numbers(10,filefunc::text_line[n],X);
      tiepoint_imagenumber.push_back(X[0]);
      camera* curr_camera_ptr=new camera(X[1],X[2],X[3],X[4],X[5]);
      curr_camera_ptr->set_mount_rotation_angles(X[6],X[7],X[8]);

//      best_roll=X[9];
//      best_pitch=X[10];
//      best_yaw=X[11];
//      RMS_chisq=X[12];

      camera_ptrs.push_back(curr_camera_ptr);
   } // loop over index n labeling tiepoint camera information 
   outputfunc::newline();

// Linearly interpolate internal camera parameters determined via
// manual tie-point associations at a few number of images to the
// entire image sequence:

   int startbin=0;
   int stopbin=tiepoint_imagenumber.size()-1;

// Recall first image in HAFB_overlap_corrected_grey.vid actually
// corresponds to tiepoint_imagenumber 300 in uncut HAFB.vid file:

   const int imagenumber_offset=300;

   for (unsigned int i=imagenumber_offset; i<image_time.size(); i++)
   {
      int n=mathfunc::mylocate(tiepoint_imagenumber,int(i-imagenumber_offset));

      camera *camera0_ptr,*camera1_ptr;
      int nstart=0;
      int nbins=1;
      if (n < startbin)
      {
         camera0_ptr=camera_ptrs[startbin];
         camera1_ptr=camera_ptrs[startbin];
      }
      else if (n >= stopbin)
      {
         camera0_ptr=camera_ptrs[stopbin];
         camera1_ptr=camera_ptrs[stopbin];
      }
      else
      {
         camera0_ptr=camera_ptrs[n];
         camera1_ptr=camera_ptrs[n+1];
         nstart=tiepoint_imagenumber[n];
         nbins=tiepoint_imagenumber[n+1]-nstart;
      }

//      cout << "i-imagenumber_offset = " << i-imagenumber_offset 
//           << " n = " << n << " nbins = " << nbins << endl;

      param_range fu(camera0_ptr->get_fu(),camera1_ptr->get_fu(),nbins);
//      param_range fv(camera0_ptr->get_fv(),camera1_ptr->get_fv(),nbins);
      param_range u0(camera0_ptr->get_u0(),camera1_ptr->get_u0(),nbins);
      param_range v0(camera0_ptr->get_v0(),camera1_ptr->get_v0(),nbins);
      param_range kappa2(camera0_ptr->get_kappa2(),camera1_ptr->get_kappa2(),
                         nbins);
      param_range alpha(camera0_ptr->get_alpha(),camera1_ptr->get_alpha(),
                        nbins);
      param_range beta(camera0_ptr->get_beta(),camera1_ptr->get_beta(),nbins);
      param_range phi(camera0_ptr->get_phi(),camera1_ptr->get_phi(),nbins);

      int rel_imagenumber=i-imagenumber_offset-nstart;
      double curr_fu=fu.compute_value(rel_imagenumber);
//      double curr_fv=fv.compute_value(rel_imagenumber);
      double curr_u0=u0.compute_value(rel_imagenumber);
      double curr_v0=v0.compute_value(rel_imagenumber);
      double curr_kappa2=kappa2.compute_value(rel_imagenumber);
      double curr_alpha=alpha.compute_value(rel_imagenumber);
      double curr_beta=beta.compute_value(rel_imagenumber);
      double curr_phi=phi.compute_value(rel_imagenumber);

//      cout << "alpha = " << curr_alpha*180/PI
//           << " beta = " << curr_beta*180/PI 
//           << " phi = " << curr_phi*180/PI << endl;

      int curr_imagenumber=image_time[i].first;
      threevector rel_camera_posn=rel_camera_XYZ[curr_imagenumber];
      double roll=(camera_rpy[curr_imagenumber]).get(0)*PI/180;
      double pitch=(camera_rpy[curr_imagenumber]).get(1)*PI/180;
      double yaw=(camera_rpy[curr_imagenumber]).get(2)*PI/180;

      double curr_time=i-imagenumber_offset;

// Save intrinsic & extrinsic camera parameters to output ascii file:

      const int column_width=15;
      camera_params_stream << curr_time << endl;
      camera_params_stream << curr_fu << setw(column_width)
                           << curr_fu << setw(column_width)
                           << curr_u0 << setw(column_width)
                           << curr_v0 << setw(column_width)
                           << curr_kappa2 << endl;
      camera_params_stream << curr_alpha << setw(column_width)
                           << curr_beta << setw(column_width)
                           << curr_phi << endl;
      camera_params_stream << rel_camera_posn.get(0) << setw(column_width)
                           << rel_camera_posn.get(1) << setw(column_width)
                           << rel_camera_posn.get(2) << endl;
      camera_params_stream << pitch << setw(column_width)
                           << roll << setw(column_width)
                           << yaw << endl;
      camera_params_stream << endl;

   } // loop over index i labeling GPS/IMU information

   filefunc::closefile(camera_params_filename,camera_params_stream);

// Delete cameras dynamically allocated for parameters determined via
// chisq minimization by program BUNDLE:

   for (unsigned int n=0; n<camera_ptrs.size(); n++)
   {
      delete camera_ptrs[n];
   }
}
