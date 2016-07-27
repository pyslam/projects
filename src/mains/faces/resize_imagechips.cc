// ==========================================================================
// Program RESIZE_IMAGECHIPS imports image chips whose pixel sizes are
// less than 256x256.  It calls ImageMagick and superposes the image
// chips onto black backgrounds which are 256x256 in size.  The
// trivially extended image chips are exported to an extended_chips
// subdirectory in JPG format.  

// Note: We discovered the hard and painful way on 7/22/2016 that
// Caffe's image --> LMDB converter appears to expect JPG [rather than
// PNG] imagery input by default!

// Run program EXTEND_IMAGECHIPS from within the subdirectory holding 
// image chips:

//                          ./resize_imagechips

// ==========================================================================
// Last updated on 7/23/16; 7/24/16; 7/27/16
// ==========================================================================

#include <iostream>
#include <Magick++.h>
#include <string>
#include <vector>
#include "color/colorfuncs.h"
#include "math/constants.h"
#include "general/filefuncs.h"
#include "image/imagefuncs.h"
#include "general/outputfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "time/timefuncs.h"
#include "video/videofuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);
   timefunc::initialize_timeofday_clock();

   string images_subdir = "./";
   bool search_all_children_dirs_flag = false;
   //   bool search_all_children_dirs_flag = true;
   vector<string> image_filenames=filefunc::image_files_in_subdir(
      images_subdir,search_all_children_dirs_flag);
   string resized_chips_subdir=images_subdir+"resized_chips/";
   filefunc::dircreate(resized_chips_subdir);

   int max_xdim = 96 + 10;
   int max_ydim = 96 + 10;

   int istart=0;
   int istop = image_filenames.size();
   int n_images = istop - istart;
   cout << "n_images within current working subdir = " << n_images << endl;
   
   for(int i = istart; i < istop; i++)
   {
      outputfunc::update_progress_fraction(i,1000,n_images);

      if ((i-istart)%1000 == 0)
      {
         double progress_frac = double(i - istart)/n_images;
         outputfunc::print_elapsed_and_remaining_time(progress_frac);
      }

      string downsized_image_filename="./tmp_downsized.png";
      videofunc::downsize_image(image_filenames[i], max_xdim, max_ydim,
                                downsized_image_filename);
      string basename = filefunc::getbasename(image_filenames[i]);
      
      string padded_jpg_filename=resized_chips_subdir
         +stringfunc::prefix(basename)+"_106x106.jpg";
      string unix_cmd="convert -size 106x106 xc:black ";
      unix_cmd += downsized_image_filename;
      unix_cmd += " -gravity center -composite ";
      unix_cmd += padded_jpg_filename;
//      cout << unix_cmd << endl;

      sysfunc::unix_command(unix_cmd);
   } // loop over index i 

} 


