// =======================================================================
// Program ERSA_FEATURES imports projected ERSA track information
// generated by ERSA_TRACKS.  In particular, it reads in track label,
// panel_ID, panel_U and panel_V information as a function of ERSA
// frame number.  Looping over all 10 wagonwheel panels, ERSA_FEATURES
// exports 2D feature files which contain U,V feature track
// coordinates as functions of video framenumber.  The time offset
// between ERSA and video frames is a free variable which the user is
// queried to enter.  The resulting UV feature files can be imported
// into program VPLAYER in order to directly compare WISP video with
// ERSA tracks projected into georegistered image planes.

//			      ./ersa_features

// =======================================================================
// Last updated on 4/1/13; 4/2/13; 12/23/13
// =======================================================================

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "astro_geo/Clock.h"
#include "general/filefuncs.h"
#include "general/outputfuncs.h"
#include "general/sysfuncs.h"
#include "track/tracks_group.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

int main( int argc, char** argv ) 
{
   std::set_new_handler(sysfunc::out_of_memory);
   
   double WISP_easting=339106.063;
   double WISP_northing=4690476.511;
   double WISP_altitude=12.7341503;
   threevector WISP_posn(WISP_easting,WISP_northing,WISP_altitude);

// Import ERSA tracks:

   string ERSA_subdir="./ERSA/";
   string ERSA_tracks_filename=ERSA_subdir+"timestamps.dat";
   filefunc::ReadInfile(ERSA_tracks_filename);

   typedef std::map<int,std::vector<fourvector> > FEATURES_MAP;
   FEATURES_MAP* features_map_ptr=new FEATURES_MAP;
   FEATURES_MAP::iterator iter;

// independent integer = ersa_framenumber
// Dependent STL vec of fourvectors = (track_label,panel_ID,panel_U,panel_V)

   int curr_ersa_framenumber=0;
   double prev_elapsed_secs=-1;

   vector<double> elapsed_secs;   
   vector<fourvector> feature_fourvectors;
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<double> column_values=stringfunc::string_to_numbers(
         filefunc::text_line[i]);

      double curr_elapsed_secs=column_values[0];
      elapsed_secs.push_back(curr_elapsed_secs);

      if (prev_elapsed_secs < 0)
      {
         prev_elapsed_secs=curr_elapsed_secs;
      }
      else if (!nearly_equal(curr_elapsed_secs,prev_elapsed_secs))
      {
         (*features_map_ptr)[curr_ersa_framenumber]=feature_fourvectors;
         feature_fourvectors.clear();

         prev_elapsed_secs=curr_elapsed_secs;
         curr_ersa_framenumber++;
      }
      
      int curr_track_label=column_values[1];
      double curr_easting=column_values[2];
      double curr_northing=column_values[3];
      double curr_altitude=column_values[4];
      int curr_panel_ID=column_values[9];
      double curr_panel_U=column_values[10];
      double curr_panel_V=column_values[11];

      threevector curr_aircraft_posn(curr_easting,curr_northing,curr_altitude);
      double curr_range=(curr_aircraft_posn-WISP_posn).magnitude();

      double track_label_and_panel_ID=curr_track_label+0.1*curr_panel_ID;

      fourvector curr_fourvector(
         track_label_and_panel_ID,curr_range,curr_panel_U,curr_panel_V);
//      fourvector curr_fourvector(
//         curr_track_label,curr_panel_ID,curr_panel_U,curr_panel_V);

      feature_fourvectors.push_back(curr_fourvector);

   } // loop over index i labeling lines in ERSA_tracks_filename

   double total_elapsed_time=elapsed_secs.back()-elapsed_secs.front();
   const double time_per_frame=2;	// secs
   int n_total_frames=total_elapsed_time/time_per_frame;
   
   cout << "total_elapsed_time = "
        << total_elapsed_time << " secs = "
        << total_elapsed_time/60 << " mins" << endl;
   cout << "n_total_frames = " << n_total_frames << endl;

   double t_offset=0;
//   cout << "Enter offset time in minutes:" << endl;
   cout << "Enter offset time in seconds:" << endl;
   cin >> t_offset;
//   t_offset *= 60;	// secs

   int frame_offset=t_offset/time_per_frame;
   cout << "frame_offset = " << frame_offset << endl;

   int n_video_frames=429;

   int n_panels=10;
   for (int panel=0; panel<n_panels; panel++)
   {
      ofstream feature_stream;
      feature_stream.precision(10);
   
      string output_filename=ERSA_subdir+"features_2D_ERSA_"+
         stringfunc::number_to_string(panel)+".txt";
      filefunc::openfile(output_filename,feature_stream);
      feature_stream << "# Frame   Feature_ID   Passnumber   U 			 V  	   Range (kms)" << endl;
      feature_stream << endl;

      for (int video_framenumber=0; video_framenumber<n_video_frames; 
           video_framenumber++)
      {
         int curr_ersa_framenumber=video_framenumber+frame_offset;
         iter=features_map_ptr->find(curr_ersa_framenumber);
         if (iter==features_map_ptr->end()) continue;

         vector<fourvector> feature_fourvectors=iter->second;
         for (unsigned int feature_index=0; feature_index<
                 feature_fourvectors.size(); feature_index++)
         {
            fourvector curr_fourvector=feature_fourvectors[feature_index];

            double track_label_and_panel_ID=curr_fourvector[0];
            int curr_track_label=basic_math::mytruncate(
               track_label_and_panel_ID);
            track_label_and_panel_ID -= curr_track_label;
            int curr_panel_ID=basic_math::round(track_label_and_panel_ID*10);

            double curr_range_in_kms=0.001*curr_fourvector[1];
            double curr_panel_U=curr_fourvector[2];
            double curr_panel_V=curr_fourvector[3];

            if (curr_panel_ID != panel) continue;
         
            int video_passnumber=0;
//            double score=NEGATIVEINFINITY;

            feature_stream << video_framenumber << "\t\t"
                           << curr_track_label << "\t"
                           << video_passnumber << "\t"
                           << curr_panel_U << "\t\t"
                           << curr_panel_V << "\t"
                           << stringfunc::number_to_string(
                              curr_range_in_kms,0) << endl;
         } // loop over feature_index
      }
      filefunc::closefile(output_filename,feature_stream);
   
      string banner="Exported "+output_filename;
      outputfunc::write_big_banner(banner);

   } // loop over panel index

   delete features_map_ptr;
}
