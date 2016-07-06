// ========================================================================
// Program GENERATE_SQL

// generate_SQL --region_filename ./bundler/MIT2317/packages/peter_inputs.pkg

// ========================================================================
// Last updated on 4/20/10; 4/21/10; 5/30/10
// ========================================================================

#include <iostream>
#include <string>
#include <vector>
#include "general/filefuncs.h"
#include "general/outputfuncs.h"
#include "passes/PassesGroup.h"
#include "video/photogroup.h"
#include "math/prob_distribution.h"
#include "general/sysfuncs.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::flush;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);

   string bundle_filename=passes_group.get_bundle_filename();
//   cout << " bundle_filename = " << bundle_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(bundle_filename);
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;
   string image_list_filename=passes_group.get_image_list_filename();
   cout << "image_list_filename = " << image_list_filename << endl;
   string image_sizes_filename=passes_group.get_image_sizes_filename();
   cout << "image_sizes_filename = " << image_sizes_filename << endl;
   string image_info_filename=bundler_IO_subdir+"image_info.dat";
   cout << "image_info_filename = " << image_info_filename << endl;

// Instantiate photogroup to hold Bundler photos:

   photogroup* photogroup_ptr=new photogroup;
   photogroup_ptr->set_UTM_zonenumber(19);	// Boston/Lowell

// Recall Michael Yee's GraphExplorer requires that image thumbnails
// reside within a subdirectory of webapps:

   string basename=filefunc::getbasename(bundler_IO_subdir);
//   cout << "basename = " << basename << endl;
   string URL="http://127.0.0.1:8080/photo/images/"+basename+"images/";
//   cout << "URL = " << URL << endl;
   photogroup_ptr->set_base_URL(URL);

   int n_photos_to_reconstruct=-1;
   photogroup_ptr->generate_bundler_photographs(
      bundler_IO_subdir,image_list_filename,image_sizes_filename,
      n_photos_to_reconstruct);

   int n_photos=photogroup_ptr->get_n_photos();
   cout << "n_photos = " << n_photos << endl;

// Fill STL map holding Noah's SIFT matches between input images:

   string edgelist_filename=bundler_IO_subdir+"edgelist.dat";
   cout << "edgelist_filename = " << edgelist_filename << endl;
   filefunc::ReadInfile(edgelist_filename);

   for (int i=0; i<filefunc::text_line.size(); i++)
   {
//      if (i%1000==0) cout << i << " " << flush;

      vector<double> curr_row_entries=
         stringfunc::string_to_numbers(filefunc::text_line[i]);
      int photo_ID_1=basic_math::round(curr_row_entries[0]);
      int photo_ID_2=basic_math::round(curr_row_entries[1]);
      int n_matches=basic_math::round(curr_row_entries[2]);

      if (!photogroup_ptr->node_in_graph(photo_ID_1))
      {
         photograph* photograph_ptr=new photograph();
         photograph_ptr->set_ID(photo_ID_1);
         photogroup_ptr->add_node(photograph_ptr);
      }

      if (!photogroup_ptr->node_in_graph(photo_ID_2))
      {
         photograph* photograph_ptr=new photograph();
         photograph_ptr->set_ID(photo_ID_2);
         photogroup_ptr->add_node(photograph_ptr);
      }

      photogroup_ptr->add_graph_edge(photo_ID_1,photo_ID_2,n_matches);
//      cout << "ID1 = " << photo_ID_1
//           << " ID2 = " << photo_ID_2
//           << " nmatches = " << n_matches << endl;

   } // loop over index i labeling lines in edgelist_filename
   cout << endl;

   cout << "photogroup_ptr->get_n_photos() = " 
        << photogroup_ptr->get_n_photos() << endl;
//   photogroup_ptr->compute_adjacency_matrix();
//   cout << "After computing adjacency matrix()" << endl;

   if (image_info_filename.size() > 0)
   {
      photogroup_ptr->import_image_info(image_info_filename);
   }

// Read in initial graph X and Y positions generated by FM3 algorithm
// and output by program EXTRACT_OGDF_LAYOUT:

//   string layout_filename=bundler_IO_subdir+"graph_XY_coords.fm3_layout";
//   photogroup_ptr->read_nodes_layout(layout_filename);

// Write children, parents and grandparents graphs to output SQL files:


//   string SQL_node_filename=bundler_IO_subdir+"insert_nodes.sql";
//   string SQL_link_filename=bundler_IO_subdir+"insert_links.sql";
//   photogroup_ptr->write_SQL_insert_photo_commands(
//      SQL_photo_filename,SQL_node_filename,SQL_link_filename);

   string SQL_photo_filename=bundler_IO_subdir+"insert_photos.sql";
   photogroup_ptr->write_SQL_insert_photo_commands(SQL_photo_filename);
   
}
