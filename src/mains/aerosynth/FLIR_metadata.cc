// ==========================================================================
// Program FLIR_METADATA reads in aircraft metadata generated by
// Ross Anderson's program running in conjunction with the FLIR
// camera.  It reformats thet metadata so that it matches sailplane
// metadata read in by GPSREGISTER.

// 				FLIR_metadata

// ==========================================================================
// Last updated on 1/25/11
// ==========================================================================

#include <iostream>
#include <string>
#include <vector>

#include "osg/osgGraphicals/AnimationController.h"
#include "video/camera.h"
#include "general/filefuncs.h"
#include "general/outputfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::ios;
using std::ofstream;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);
   string subdir="/home/cho/programs/c++/svn/projects/src/mains/photosynth/bundler/FLIR/";
   string lighthawk_metadata_filename=subdir+"cropped_20091208_191409.dat";

   vector<string> basename,field_of_view,latitude,longitude,altitude;
   filefunc::ReadInfile(lighthawk_metadata_filename);
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> column_values=
         stringfunc::decompose_string_into_substrings(
            filefunc::text_line[i],",");
      basename.push_back(column_values[0]);
      field_of_view.push_back(column_values[3]);
      latitude.push_back(column_values[5]);
      longitude.push_back(column_values[6]);
      altitude.push_back(column_values[7]);
   } // loop over index i labeling text lines

   const double meters_per_ft=0.3048;
   string output_filename=subdir+"aircraft_gps.dat";
   ofstream outstream;
   filefunc::openfile(output_filename,outstream);
   outstream << "# image name, secs since midnight Jan 1, 1970, longitude, latitude," << endl;
   outstream << "# altitude (ft above WGS84 ellipsoid)" << endl << endl;

   for (unsigned int i=0; i<basename.size(); i++)
   {
      string image_filename=basename[i]+".jpg";
      string curr_line=image_filename+"  ";
//      curr_line += stringfunc::number_to_string(unix_time[i])+"  ";
      curr_line += "0  ";
      curr_line += longitude[i]+"  ";
      curr_line += latitude[i]+"  ";

      double alt=stringfunc::string_to_number(altitude[i]);
      curr_line += stringfunc::number_to_string(alt/meters_per_ft);
      outstream << curr_line << endl;
   }
   
   filefunc::closefile(output_filename,outstream);


   string banner="Wrote aircraft metadata to "+output_filename;
   outputfunc::write_banner(banner);

}
