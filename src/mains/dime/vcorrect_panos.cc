// =======================================================================
// Program UVCORRECT_PANOS imports fitted sinusoid horizon parameters
// generated by program FIT_HORIZONS.  Looping over a set of raw
// WISP panoramas, it applies U-dependent V-offsets to each input
// image.  The physical horizon within a corrected panorama appears
// as a horizontal line in the output image. 


//  UVCORRECT_PANOS also
// generates subsampled versions of the corrected panoramas on which
// SIFT/ASIFT can later be run in order to fix residual U-warping.

//			./uvcorrect_panos


// =======================================================================
// Last updated on 3/23/13; 3/24/13; 3/25/13
// =======================================================================

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "video/camerafuncs.h"
#include "math/constant_vectors.h"
#include "general/filefuncs.h"
#include "math/mathfuncs.h"
#include "general/outputfuncs.h"
#include "math/rotation.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "video/texture_rectangle.h"
#include "time/timefuncs.h"

using std::cout;
using std::endl;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

int main( int argc, char** argv ) 
{
   std::set_new_handler(sysfunc::out_of_memory);

   timefunc::initialize_timeofday_clock();

   int pixel_width=40000;
   int pixel_height=2200;
   double Umax=double(pixel_width)/double(pixel_height);
   cout << "Umax = " << Umax << endl;
   
   double IFOV=2*PI/pixel_width;
   double elevation_extent=pixel_height*IFOV; //  0 <= V <= 1
   cout << "elevation_extent = " << elevation_extent*180/PI << " degs" << endl;

   string panos_subdir=
      "/data_third_disk/DIME/panoramas/Feb2013_DeerIsland/wisp8-spin-.5hz-ocean/panos_360/";
   string horizons_subdir=panos_subdir+"horizons/";
   string params_filename=horizons_subdir+"sinusoid_params.dat";

   filefunc::ReadInfile(params_filename);
   
   string blank_image_filename="./blank_40Kx2.2K.jpg";
   texture_rectangle* input_texture_rectangle_ptr=
      new texture_rectangle(blank_image_filename,NULL);
   texture_rectangle* vcorrected_texture_rectangle_ptr=
      new texture_rectangle(blank_image_filename,NULL);
   texture_rectangle* uvcorrected_texture_rectangle_ptr=
      new texture_rectangle(blank_image_filename,NULL);

   double numer=0;
   double denom=0;
   for (int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> substrings=stringfunc::decompose_string_into_substrings(
         filefunc::text_line[i]);
      string image_basename=substrings[0];
      string image_filename=panos_subdir+image_basename;
      string vcorrected_image_filename=horizons_subdir+"vcorrected_"+
         image_basename;
      string uvcorrected_image_filename=horizons_subdir+"uvcorrected_"+
         image_basename;

      string banner="Correcting "+image_basename;
      outputfunc::write_banner(banner);

      double A=stringfunc::string_to_number(substrings[1]);
      double phi=stringfunc::string_to_number(substrings[2])*PI/180;
      double v_avg=stringfunc::string_to_number(substrings[3]);
      double max_score=stringfunc::string_to_number(substrings[4]);
      numer += max_score*v_avg;
      denom += max_score;

      bool export_images_flag=true;
//      bool export_images_flag=false;
      
      if (!export_images_flag) continue;

      input_texture_rectangle_ptr->import_photo_from_file(
         image_filename);

      camerafunc::vcorrect_WISP_image(
         input_texture_rectangle_ptr,vcorrected_texture_rectangle_ptr,A,phi);

      camerafunc::ucorrect_WISP_image(
         vcorrected_texture_rectangle_ptr,
         uvcorrected_texture_rectangle_ptr,
         0,v_avg,-A,phi);

      uvcorrected_texture_rectangle_ptr->write_curr_frame(
         uvcorrected_image_filename);

// Generate subsampled version of v-corrected panorama on which we can
// run SIFT and ASIFT:

      string subsampled_filename=horizons_subdir+
         "subsampled_vcorrected_"+image_basename;
      string unix_cmd="convert -resize 9.9% "+vcorrected_image_filename+" "+
         subsampled_filename;
      sysfunc::unix_command(unix_cmd);

      banner="Exported "+vcorrected_image_filename;
      outputfunc::write_banner(banner);

      outputfunc::print_elapsed_time();
   }

   delete input_texture_rectangle_ptr;
   delete vcorrected_texture_rectangle_ptr;
   delete uvcorrected_texture_rectangle_ptr;

// Compute py_horizon averaged over all input WISP panoramas:

   double mean_v_avg=numer/denom;
   double delta_v=mean_v_avg-0.5;
   int pv_horizon=(1-mean_v_avg)*pixel_height;

   cout << "<v_avg> = " << mean_v_avg << endl;
   cout << "Delta_v = " << delta_v << endl;
   cout << "pv_horizon = " << pv_horizon << endl;

// Extract WISP sensor's altitude above sea-level from py_horizon:

   double delta_vert_pixels=delta_v*pixel_height;
   cout << "delta_vert_pixels = " << delta_vert_pixels << endl;
   double delta_vert_theta=delta_vert_pixels*IFOV;
   cout << "delta_vert_theta = " << delta_vert_theta*180/PI << " degs" << endl;
   double theta=PI/2+delta_vert_theta;
   cout << "theta = " << theta*180/PI << endl;
   double Re=6371*1000;
   double a=Re*(1/sin(theta)-1);
   cout << "WISP altitude above sea-level a = " << a << " meters" << endl;

// Compute radial distance (range) to horizon:

   double r=sqrt(2*Re*a+a*a);
   cout << "Range to horizon r = " << r << " meters" << endl;
}
