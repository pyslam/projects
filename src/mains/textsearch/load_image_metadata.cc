// ========================================================================
// This modified version of program LOAD_IMAGE_METADATA queries the
// user to enter campaign and mission IDs for a set of text document
// images to be loaded into the images table of the IMAGERY database.
// It also requests a subdirectory of /data/ImageEngine/ where a set
// of image and thumbnail files must already exist.
// LOAD_IMAGE_METADATA then inserts metadata for images within
// image_list_filename into the images database table.

// 	./load_image_metadata --GIS_layer ./packages/imagery_metadata.pkg

// ========================================================================
// Last updated on 12/25/12; 5/27/13; 5/29/13; 6/7/14
// ========================================================================

#include <iostream>
#include <string>
#include <vector>

#include "general/filefuncs.h"
#include "image/imagefuncs.h"
#include "video/imagesdatabasefuncs.h"
#include "passes/PassesGroup.h"
#include "osg/osgGIS/postgis_databases_group.h"
#include "general/sysfuncs.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::flush;
   using std::map;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   PassesGroup passes_group(&arguments);

   vector<int> GISlayer_IDs=passes_group.get_GISlayer_IDs();
//   cout << "GISlayer_IDs.size() = " << GISlayer_IDs.size() << endl;

   string bundle_filename=passes_group.get_bundle_filename();
//   cout << " bundle_filename = " << bundle_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(bundle_filename);
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;
   string image_list_filename=bundler_IO_subdir+"image_list.dat";
   cout << " image_list_filename = " << image_list_filename << endl;

// Instantiate postgis database objects to send data to and retrieve
// data from external Postgres database:

   postgis_databases_group* postgis_databases_group_ptr=
      new postgis_databases_group;
   postgis_database* postgis_db_ptr=postgis_databases_group_ptr->
      generate_postgis_database_from_GISlayer_IDs(
         passes_group,GISlayer_IDs);
//   cout << "postgis_db_ptr = " << postgis_db_ptr << endl;

   int campaign_ID,mission_ID;
   cout << "Enter campaign_ID:" << endl;
   cin >> campaign_ID;

   cout << "Enter mission_ID:" << endl;
   cin >> mission_ID;

   string images_subdir;
   cout << "Enter subdirectory of /data/ImageEngine/ in which images reside:"
        << endl;
   cin >> images_subdir;
   filefunc::add_trailing_dir_slash(images_subdir);
   images_subdir="/data/ImageEngine/"+images_subdir;
   cout << "images_subdir = " << images_subdir << endl;
 
   if (!filefunc::direxist(images_subdir)) 
   {
      cout << "Images subdir=" << images_subdir << endl;
      cout << "  not found!" << endl;
      exit(-1);
   }

   string thumbnails_subdir=images_subdir+"thumbnails/";
   if (!filefunc::direxist(thumbnails_subdir)) 
   {
      cout << "Thumbnails subdir=" << thumbnails_subdir << endl;
      cout << "  not found!" << endl;
      exit(-1);
   }

// As of 12/14/12, we only insert metadata for images within the
// image_list_filename and NOT for all images within images_subdir!

   vector<int> image_IDs;
   vector<string> image_filenames,thumbnail_filenames;
   filefunc::ReadInfile(image_list_filename);
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> column_values=
         stringfunc::decompose_string_into_substrings(filefunc::text_line[i]);
      image_IDs.push_back(i);

      string full_image_filename=filefunc::text_line[i];
      string image_filename=filefunc::getbasename(full_image_filename);
      string thumbnail_filename="thumbnail_"+image_filename;
      
      image_filename=images_subdir+image_filename;
      thumbnail_filename=images_subdir+"thumbnails/"+thumbnail_filename;
      image_filenames.push_back(image_filename);
      thumbnail_filenames.push_back(thumbnail_filename);

//      cout << "ID = " << image_IDs.back() << " "
//           << image_filenames.back() << " "
//           << thumbnail_filenames.back() << endl;
   }

   for (unsigned int i=0; i<image_filenames.size(); i++)
   {
      string URL=image_filenames[i];
      if (i%100==0) cout << "Processing image " << i << endl;
      cout << "i = " << i << " image_filename = " << URL << endl;
      string thumbnail_URL=thumbnail_filenames[i];
//      cout << "   thumbnailURL = " << thumbnail_URL << endl;

      int image_ID=image_IDs[i];
      int importance=1;
      unsigned int npx,npy;
      imagefunc::get_image_width_height(URL,npx,npy);
      unsigned int thumbnail_npx,thumbnail_npy;
      imagefunc::get_image_width_height(
         thumbnail_URL,thumbnail_npx,thumbnail_npy);

      imagesdatabasefunc::insert_image_metadata(
         postgis_db_ptr,campaign_ID,mission_ID,image_ID,importance,
         URL,npx,npy,thumbnail_URL,thumbnail_npx,thumbnail_npy);
   } // loop over index i labeling image filenames

}

