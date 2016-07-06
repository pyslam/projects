// ========================================================================
// Program GENERATE_D7_PANELS reads in a set of ~5Kx1K JPG images
// which are assumed to have been generated by two back-to-back D7
// video cameras. It performs vertical and horizontal profiling to
// find the genuine edges of the input panoramas.  This program then
// utilizes ImageMagick's conversion program to crop away black
// borders from the panoramas.  GENERATE_D7_PANELS then splits each
// cropped D7 ~5Kx1K image into 5 panels, Each raw input D7 jpeg
// filename is assumed to be of the form XXX_a.jpg, XXX_A.jpg,
// XXX_b.jpg or XXX_B.jpg.

// To run this program, chant  

// 	~/programs/c++/svn/projects/src/mains/hmi/generate_D7_panels

// ========================================================================
// Last updated on 7/4/11; 7/13/11
// ========================================================================

#include <iostream>
#include <string>
#include <vector>
#include "image/imagefuncs.h"
#include "general/outputfuncs.h"
#include "image/raster_parser.h"
#include "general/stringfuncs.h"
#include "image/TwoDarray.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

   sysfunc::clearscreen();

   string images_subdir="./D7/";
   cout << "Enter full path for directory containing raw D7 jpeg images to be cropped and split into 5 separate panels:" << endl;
   cin >> images_subdir;
   filefunc::add_trailing_dir_slash(images_subdir);

   string cropped_images_subdir=images_subdir+"cropped/";
   filefunc::dircreate(cropped_images_subdir);

   string panels_subdir=cropped_images_subdir+"panels/";
   filefunc::dircreate(panels_subdir);

// First read in raw D7 jpeg images from specified input subdirectory:

   vector<string> allowed_suffixes;
   allowed_suffixes.push_back("jpg");
   vector<string> image_filenames=
      filefunc::files_in_subdir_matching_specified_suffixes(
         allowed_suffixes,images_subdir);

// Perform vertical and horizontal profiling to determine how raw D7
// jpegs should be cropped:

   raster_parser RasterParser;

   for (unsigned int iter=0; iter<image_filenames.size(); iter++)
   {
      string image_filename=image_filenames[iter];

      RasterParser.open_image_file(image_filename);
      twoDarray* RtwoDarray_ptr=RasterParser.get_RtwoDarray_ptr();
      int mdim=RtwoDarray_ptr->get_mdim();
      int ndim=RtwoDarray_ptr->get_ndim();

      RasterParser.fetch_raster_band(0);
      RasterParser.read_raster_data(RtwoDarray_ptr);

// Compute vertical profiles to find starting and stopping values for
// horizontal index m:

      int mstart=-1;
      for (int m=0; m<mdim; m++)
      {
         double column_counter=0;
         for (int n=0; n<ndim; n++)
         {
            if (RtwoDarray_ptr->get(m,n) > 0)
               column_counter++;
         }
         if (column_counter > ndim/2) 
         {
            mstart=m;
            break;
         }
//      cout << "m = " << m << "column_counter = " << column_counter << endl;
      } // loop over m index
      cout << "mstart = " << mstart << endl;

      int mstop=-1;
      for (int m=mdim-1; m>0; m--)
      {
         double column_counter=0;
         for (int n=0; n<ndim; n++)
         {
            if (RtwoDarray_ptr->get(m,n) > 0)
               column_counter++;
         }
         if (column_counter > ndim/2) 
         {
            mstop=m;
            break;
         }
//      cout << "m = " << m << "column_counter = " << column_counter << endl;
      } // loop over m index
      cout << "mstop = " << mstop << endl;

// Compute horizontal profiles to find starting and stopping values for
// horizontal index n:

      int nstart=-1;
      for (int n=0; n<mdim; n++)
      {
         double row_counter=0;
         for (int m=0; m<mdim; m++)
         {
            if (RtwoDarray_ptr->get(m,n) > 0)
               row_counter++;
         }
//      cout << "n = " << n << " row_counter = " << row_counter << endl;
         if (row_counter > mdim/2) 
         {
            nstart=n;
            break;
         }

      } // loop over m index
      cout << "nstart = " << nstart << endl;

      int nstop=-1;
      for (int n=ndim-1; n>0; n--)
      {
         double row_counter=0;
         for (int m=0; m<mdim; m++)
         {
            if (RtwoDarray_ptr->get(m,n) > 0)
               row_counter++;
         }
//      cout << "n = " << n << " row_counter = " << row_counter << endl;
         if (row_counter > mdim/2) 
         {
            nstop=n;
            break;
         }

      } // loop over m index
      cout << "nstop = " << nstop << endl;

      RasterParser.close_image_file();

      int xoffset=mstart;
      int yoffset=nstart;
      int xstop=mstop;
      int ystop=nstop;

      int xdim=xstop-xoffset;
      int ydim=ystop-yoffset;

      cout << "Cropping D7 jpeg file = " << image_filename << endl;
      string image_basename=filefunc::getbasename(image_filename);

      string cropped_image_filename=
         cropped_images_subdir+"cropped_"+image_basename;
      cout << "Cropped_image_filename = " << cropped_image_filename << endl;

      string unix_cmd="cp "+image_filename+" "+cropped_image_filename;
      sysfunc::unix_command(unix_cmd);

      imagefunc::crop_image(cropped_image_filename,xdim,ydim,xoffset,yoffset);

// Split each cropped D7 ~5Kx1K image into 5 panels:

      int n_panels=5;

//      cout << "n = " << n << " filename = " << filename 
//           << " xdim = " << xdim
//           << " ydim = " << ydim 
//           << endl;

      string filename_copy=cropped_images_subdir+"copy_"+image_basename;

//      cout << "filename_copy = " << filename_copy << endl;
//      cout << "panels_subdir = " << panels_subdir << endl;

      for (int p=0; p<n_panels; p++)
      {
         string unix_cmd="cp "+cropped_image_filename+" "+filename_copy;
         sysfunc::unix_command(unix_cmd);
         
         int width=double(xdim)/double(n_panels);
         int height=ydim;
         int xoffset=double(p*xdim)/double(n_panels);
         int yoffset=0;
         imagefunc::crop_image(filename_copy,width,height,xoffset,yoffset);
         
         string basename_prefix=stringfunc::prefix(image_basename);
         string basename_suffix=stringfunc::suffix(image_basename);

         cout << "Basename_prefix.size() = "
              << basename_prefix.size() << endl;
         string D7_char=basename_prefix.substr(
            basename_prefix.size()-1,1);
         cout << "D7_char = " << D7_char << endl;
         
         int p_offset=0;
         if (D7_char=="b" || D7_char=="B")
         {
            p_offset=5;
         }
         basename_prefix=basename_prefix.substr(
            0,basename_prefix.size()-1);

         unix_cmd="mv "+filename_copy+" "+panels_subdir+basename_prefix+
            "_p"+stringfunc::number_to_string(p+p_offset)+"."+basename_suffix;
         sysfunc::unix_command(unix_cmd);
      } // loop over index p labeling panels

   } // loop over iter index labeling input image filenames

   string banner="Cropped panels written to "+panels_subdir;
   outputfunc::write_big_banner(banner);

}
