// ==========================================================================
// Program PG_RECOG_PROCESSOR executes an infinite while loop.
// It constantly imports the next-to-latest image file residing within
// input_images_subdir.  It moves all other images in
// input_images_subdir to a time-stamped archive subdirectory.
// PG_RECOG_PROCESSOR generates text file output containing TOC12 sign
// positions relative to the instantaneous camera versus local computer
// time.

//	  	       	     pg_recog_processor

// ==========================================================================
// Last updated on 11/4/12; 11/5/12; 11/6/12
// ==========================================================================

#include  <iostream>
#include  <map>
#include  <set>
#include  <string>
#include  <vector>

#include "video/camera.h"
#include "astro_geo/Clock.h"
#include "general/filefuncs.h"
#include "general/outputfuncs.h"
#include "video/RGB_analyzer.h"
#include "classification/signrecogfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "classification/text_detector.h"
#include "video/texture_rectangle.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

int main(int argc, char* argv[])
{
   cout.precision(12);

   string input_images_subdir="./images/incoming_PointGrey_images/";
   string final_signs_subdir="./images/final_signs/";
   string archived_images_subdir=
      signrecogfunc::generate_timestamped_archive_subdir(final_signs_subdir);
   string output_subdir=final_signs_subdir+"sign_recog_results/";
   filefunc::dircreate(output_subdir);

// Initialize PointGrey cameras:

   camera* camera_ptr=new camera();
//   int PointGrey_camera_ID=-1;	// non-PointGrey camera
   int PointGrey_camera_ID=501207;	// "Mark's" camera
//   int PointGrey_camera_ID=501208;	// "Pat's" camera
//   int PointGrey_camera_ID=501890;	// "Bryce's" camera
   signrecogfunc::initialize_PointGrey_camera_params(
      PointGrey_camera_ID,camera_ptr);

// Initialize output relative sign position file:

   Clock clock;
   string output_filename=output_subdir+"rel_sign_posns.txt";
   ofstream output_stream;
   filefunc::openfile(output_filename,output_stream);
   output_stream << "# Frame   Image name	            Time stamp		  Sign          Rel X    Rel Y    Rel Z" << endl;
   output_stream << endl;

// Initialize TOC12 sign properties:
   
   signrecogfunc::SIGN_PROPERTIES curr_sign_properties,prev_sign_properties;
   vector<signrecogfunc::SIGN_PROPERTIES> sign_properties=
      signrecogfunc::initialize_sign_properties();

   cout << endl;
   cout << "------------------------------------------------------" << endl;
   cout << "Yellow radiation sign ID = 0" << endl;
   cout << "Orange biohazard sign ID = 1" << endl;
   cout << "Blue-purple radiation sign ID = 2" << endl;
   cout << "Blue water sign ID = 3" << endl;
   cout << "Blue gasoline sign ID = 4" << endl;
   cout << "Red stop sign ID = 5" << endl;
   cout << "Green start sign ID = 6" << endl;
   cout << "Black-white skull sign ID = 7" << endl;
   cout << "Black-white eat sign ID = 8" << endl;
   cout << "------------------------------------------------------" << endl;   

   int sign_ID_start=0;
   int sign_ID_stop=sign_properties.size()-1-2;

// Import quantized RGB lookup tables for all TOC12 signs:

   RGB_analyzer* RGB_analyzer_ptr=new RGB_analyzer();
   for (unsigned int s=0; s<sign_properties.size(); s++)
   {
      RGB_analyzer_ptr->import_quantized_RGB_lookup_table(
         sign_properties[s].sign_hue);
   }

// Import probabilistic decision functions generated by an SVM with a
// linear kernel on 12K symbol and 96K non-symbol images per TOC12 sign:

   vector<signrecogfunc::Ng_pfunct_type> Ng_pfuncts;
   vector<text_detector*> text_detector_ptrs=
      signrecogfunc::import_Ng_probabilistic_classification_functions(
         sign_properties,Ng_pfuncts);

   texture_rectangle* binary_texture_rectangle_ptr=new texture_rectangle();
   texture_rectangle* quantized_texture_rectangle_ptr=new texture_rectangle();
   texture_rectangle* raw_texture_rectangle_ptr=new texture_rectangle();
   texture_rectangle* selected_colors_texture_rectangle_ptr=
      new texture_rectangle();
   texture_rectangle* texture_rectangle_ptr=new texture_rectangle();
   texture_rectangle* text_texture_rectangle_ptr=NULL;

   timefunc::initialize_timeofday_clock();

// Infinite while loop starts here:

   int image_counter=0;
   while (true)
   {
      string next_to_latest_image_filename=
         signrecogfunc::archive_all_but_latest_image_files(
            input_images_subdir,archived_images_subdir);
      string orig_image_filename=next_to_latest_image_filename;
      if (orig_image_filename.size()==0) continue;

      cout << "Original image_filename = " << orig_image_filename << endl;

      if (image_counter >= 1)
      {
         signrecogfunc::print_processing_time(image_counter);
      }
         
      image_counter++;
      int frame_number=image_counter;

// Crop white border surrounding PointGrey images captured within
// Tennis Bubble on Saturday, Oct 27, 2012:

      string undistorted_image_filename=orig_image_filename;
      if (PointGrey_camera_ID > 0)
      {
//         string cropped_image_filename=orig_image_filename;
         string cropped_image_filename=signrecogfunc::crop_white_border(
            orig_image_filename);
         undistorted_image_filename=
            signrecogfunc::radially_undistort_PointGrey_image(
               PointGrey_camera_ID,
               cropped_image_filename,raw_texture_rectangle_ptr,
               texture_rectangle_ptr);

         string undistorted_filename=output_subdir+
            "undistorted_"+stringfunc::integer_to_string(frame_number,5)
            +".jpg";
//         texture_rectangle_ptr->write_curr_frame(undistorted_filename);

         string raw_filename=output_subdir+
            "raw_"+stringfunc::integer_to_string(frame_number,5)+".jpg";
//         raw_texture_rectangle_ptr->write_curr_frame(raw_filename);

// Search for grey-colored vertical stripe on RHS of raw PointGrey
// images.  If found, declare input image to be corrupted:
            
         if (signrecogfunc::detect_corrupted_PointGrey_image(
            raw_filename,raw_texture_rectangle_ptr))
         {
            cout << "Corrupted raw PointGrey image = " 
                 << raw_filename << endl;
         }
      } // PointGrey camera conditional

      int xdim,ydim;
      string image_filename=signrecogfunc::resize_input_image(
         undistorted_image_filename,xdim,ydim);
      texture_rectangle_ptr->reset_texture_content(image_filename);

      if (text_texture_rectangle_ptr==NULL)
      {
         text_texture_rectangle_ptr=new texture_rectangle(
            xdim,ydim,1,3,NULL);
      }

// Loop over TOC12 sign IDs starts here:

      vector<string> bbox_symbol_names;
      vector<polygon> bbox_polygons;
      vector<int> bbox_color_indices;

// Loop over TOC12 sign IDs starts here:

      for (int curr_sign_ID=sign_ID_start; curr_sign_ID <= sign_ID_stop;
           curr_sign_ID++)
      {
         curr_sign_properties=sign_properties[curr_sign_ID];   

         if (curr_sign_ID > sign_ID_start)
         {
            prev_sign_properties=sign_properties[curr_sign_ID-1];
         }
         else
         {
            prev_sign_properties=curr_sign_properties;
         }
            
         string lookup_map_name=curr_sign_properties.sign_hue;
         string symbol_name=curr_sign_properties.symbol_name;

         cout << "======================================================"
              << endl;
         cout << "symbol = " << symbol_name << endl;

         bool bw_sign_flag=false;
         if (curr_sign_properties.sign_hue=="black") bw_sign_flag=true;

         signrecogfunc::reset_texture_image(
            image_filename,binary_texture_rectangle_ptr);
         signrecogfunc::reset_texture_image(
            image_filename,quantized_texture_rectangle_ptr);
         signrecogfunc::reset_texture_image(
            image_filename,selected_colors_texture_rectangle_ptr);

         if (!bw_sign_flag)
            signrecogfunc::quantize_colors(
               RGB_analyzer_ptr,curr_sign_properties,
               texture_rectangle_ptr,
               quantized_texture_rectangle_ptr,
               selected_colors_texture_rectangle_ptr,
               binary_texture_rectangle_ptr);

         string binary_quantized_filename=output_subdir+
            "binary_colors_"+stringfunc::integer_to_string(frame_number,5)
            +".jpg";

         binary_texture_rectangle_ptr->write_curr_frame(
            binary_quantized_filename);

         vector<extremal_region*> extremal_region_ptrs,
            inverse_extremal_region_ptrs;
         signrecogfunc::compute_connected_components(
            binary_quantized_filename,curr_sign_properties,
            extremal_region_ptrs,inverse_extremal_region_ptrs);

         signrecogfunc::form_colored_sign_bbox_polygons(
            curr_sign_ID,symbol_name,image_filename,
            extremal_region_ptrs,inverse_extremal_region_ptrs,
            curr_sign_properties,RGB_analyzer_ptr,
            texture_rectangle_ptr,quantized_texture_rectangle_ptr,
            selected_colors_texture_rectangle_ptr,
            text_detector_ptrs,Ng_pfuncts,
            bbox_polygons,bbox_color_indices,bbox_symbol_names);

      } // loop over curr_sign_ID

      if (bbox_polygons.size() > 0)
      {
         texture_rectangle_ptr->reset_texture_content(image_filename);
         signrecogfunc::export_bbox_polygons(
            texture_rectangle_ptr,text_texture_rectangle_ptr,
            bbox_polygons,bbox_color_indices,bbox_symbol_names);

//         string bboxes_filename=output_subdir+
//            "bboxes_"+stringfunc::integer_to_string(frame_number,4)+".jpg";
//         texture_rectangle_ptr->write_curr_frame(bboxes_filename);
         string annotated_bboxes_filename=output_subdir+
            "annotated_bboxes_"+stringfunc::integer_to_string(
               frame_number,4)+".jpg";
         text_texture_rectangle_ptr->write_curr_frame(
            annotated_bboxes_filename);

         double TOC12_sign_diagonal_distance=1.724; // meters = 4 sqrt(2) ft
         vector<threevector> sign_posns_rel_to_camera=
            signrecogfunc::compute_relative_bbox_positions(
               camera_ptr,TOC12_sign_diagonal_distance,bbox_polygons);

         clock.set_time_based_on_local_computer_clock();
         string time_stamp=clock.YYYY_MM_DD_H_M_S();

         for (unsigned int s=0; s<bbox_symbol_names.size(); s++)
         {
            cout << "bbox symbol name = " << bbox_symbol_names[s]
                 << " sign posn rel to camera = " 
                 << sign_posns_rel_to_camera[s]
                 << endl;
            output_stream << frame_number << " "
                          << filefunc::getbasename(orig_image_filename) << " "
                          << time_stamp << " "
                          << bbox_symbol_names[s] << " "
                          << sign_posns_rel_to_camera[s].get(0) << " "
                          << sign_posns_rel_to_camera[s].get(1) << " "
                          << sign_posns_rel_to_camera[s].get(2) 
                          << endl;
         }
      } // bbox_polygons.size() > 0 conditional

      signrecogfunc::print_processing_time(image_counter);

   } // end of infinite while loop 

   filefunc::closefile(output_filename,output_stream);

   delete camera_ptr;
   delete RGB_analyzer_ptr;

   delete binary_texture_rectangle_ptr;
   delete quantized_texture_rectangle_ptr;
   delete raw_texture_rectangle_ptr;
   delete selected_colors_texture_rectangle_ptr;
   delete texture_rectangle_ptr;
   delete text_texture_rectangle_ptr;

}

