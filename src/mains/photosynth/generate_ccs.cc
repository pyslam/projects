// ========================================================================
// Program GENERATE_CCS reads in the graph edge list
// generated by SIFT_PARSER which establishes links between two
// photos if they share SIFT features in common.  It also reads in
// hierarchical graph clustering information generated by the Markov
// Cluster Algorithm or K-means algorithm.  

// GENERATE_CCS exports text files containing image URLs versus node
// IDs for each connected component.  It also writes out SQL scripts
// which load metadata into the graph_hierarchies and
// connected_components tables of the imagery database.

// generate_ccs                                               		 
// --region_filename ./bundler/baseball/packages/peter_inputs.pkg        
// --GIS_layer ./packages/imagery_metadata.pkg 

// ========================================================================
// Last updated on 10/24/13; 10/28/13; 10/29/13
// ========================================================================

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "general/filefuncs.h"
#include "graphs/graphdbfuncs.h"
#include "graphs/graph_hierarchy.h"
#include "video/imagesdatabasefuncs.h"
#include "general/outputfuncs.h"
#include "passes/PassesGroup.h"
#include "video/photodbfuncs.h"
#include "video/photogroup.h"
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
   using std::ofstream;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   PassesGroup passes_group(&arguments);
   vector<int> GISlayer_IDs=passes_group.get_GISlayer_IDs();

// Instantiate postgis database objects to send data to and retrieve
// data from external Postgres database:

   postgis_databases_group* postgis_databases_group_ptr=
      new postgis_databases_group;
   postgis_database* postgis_db_ptr=postgis_databases_group_ptr->
      generate_postgis_database_from_GISlayer_IDs(
         passes_group,GISlayer_IDs);
//   cout << "postgis_db_ptr = " << postgis_db_ptr << endl;

   string bundle_filename=passes_group.get_bundle_filename();
//   cout << " bundle_filename = " << bundle_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(bundle_filename);
//   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;
   string graphs_subdir=bundler_IO_subdir+"graphs/";
   filefunc::dircreate(graphs_subdir);

   int campaign_ID,mission_ID;
//   campaign_ID=6;	// Reuters 43K
//   mission_ID=3;	// Reuters 43K
   cout << "Enter campaign_ID:" << endl;
   cin >> campaign_ID;
   cout << "Enter mission_ID:" << endl;
   cin >> mission_ID;

   if (!imagesdatabasefunc::images_in_database(
      postgis_db_ptr,campaign_ID,mission_ID))
   {
      cout << "ERROR: No images corresponding to campaign_ID = "
           << campaign_ID << " and mission_ID = " << mission_ID
           << " exist in images table of IMAGERY database!" << endl;
      exit(-1);
   }

   vector<int> hierarchy_IDs;
   vector<string> hierarchy_descriptions;
   graphdbfunc::retrieve_and_display_hierarchy_IDs_from_database(
      postgis_db_ptr,hierarchy_IDs,hierarchy_descriptions);

   int next_available_hierarchy_ID=0;
   if (hierarchy_IDs.size() > 0)
      next_available_hierarchy_ID=hierarchy_IDs.back()+1;
   cout << endl;
   cout << "Next available hierarchy ID = " << next_available_hierarchy_ID
        << endl << endl;

   int hierarchy_ID=-1;
   cout << "Enter ID for new graph hierarchy to be entered into IMAGERY database:" << endl;
   cin >> hierarchy_ID;

   if (hierarchy_ID < 0) exit(-1);

   string hierarchy_name;
   cout << "Enter name for new graph hierarchy:" << endl;
   cin >> hierarchy_name;

   int levelzero_graph_ID=0;
   graph_hierarchy graphs_pyramid(hierarchy_ID,levelzero_graph_ID);
   
// Import number of graph nodes per level and connected graph
// component from text file generated by program KMEANS_CLUSTERS:

   graph_hierarchy::COMPONENTS_LEVELS_MAP components_levels_map;

   int n_levels=1;
   int n_connected_components=0;
   string cluster_info_filename=graphs_subdir+"clusters_info.dat";
//   cout << "cluster_info_filename = " << cluster_info_filename << endl;
   bool connected_components_calculated_flag=
      filefunc::ReadInfile(cluster_info_filename);

   if (connected_components_calculated_flag)
   {
      n_levels=stringfunc::string_to_number(filefunc::text_line[0]);
      n_connected_components=stringfunc::string_to_number(
         filefunc::text_line[1]);
      int max_child_node_ID=
         stringfunc::string_to_number(filefunc::text_line[2]);

      cout << "Number of graph levels = " << n_levels << endl;
      cout << "Number of connected graph components = " 
           << n_connected_components << endl;
      cout << "Maximum child node ID = " << max_child_node_ID << endl;

// Generate text files containing image URLs versus node IDs for each
// connected component:

      bool export_cc_node_URLs_flag=false;
      if (export_cc_node_URLs_flag)
      {
         for (int c=0; c<n_connected_components; c++)
         {
            string connected_nodes_filename=graphs_subdir+
               "connected_nodes_C"+stringfunc::number_to_string(c)+".dat";
            cout << "c = " << c << " connected_nodes_filename = "
                 << connected_nodes_filename << endl;
            vector<double> node_IDs=
               filefunc::ReadInNumbers(connected_nodes_filename);

            string output_filename=graphs_subdir+"connected_node_URLs_C"+
               stringfunc::number_to_string(c)+".dat";
            ofstream outstream;
            filefunc::openfile(output_filename,outstream);
            outstream << "# NodeID           Image URL" << endl << endl;

            for (unsigned int n=0; n<node_IDs.size(); n++)
            {
               int image_ID=node_IDs[n];
               string image_URL=imagesdatabasefunc::get_image_URL(
                  postgis_db_ptr,campaign_ID,mission_ID,image_ID);
//            string image_URL=imagesdatabasefunc::get_image_URL(
//               postgis_db_ptr,hierarchy_ID,int(node_IDs[n]));
               outstream << node_IDs[n] << "   " << image_URL << endl;
            } // loop over index n labeling nodes within curr connected component
            filefunc::closefile(output_filename,outstream);
         } // loop over index c labeling connected components
      } // export_cc_node_URLs_flag conditional
      
// Export SQL commands which insert connected graph component
// information into connected_components table of IMAGERY database:

      string cc_filename=graphs_subdir+"insert_all_ccs.sql";
      cout << "cc_filename = " << cc_filename << endl;
      ofstream ccstream;
      filefunc::openfile(cc_filename,ccstream);
   
      for (unsigned int i=3; i<filefunc::text_line.size(); i++)
      {
         vector<double> column_values=
            stringfunc::string_to_numbers(filefunc::text_line[i]);
         int level=column_values[0];
         int graph_ID=level;
         int connected_component_ID=column_values[1];
         int n_nodes=column_values[2];
         pair<int,int> p(connected_component_ID,level);

         twovector posn;
         string cc_label;
         vector<string> topic_labels;

         graph_hierarchy::NNODES_POSN_LABELS 
            nnodes_posn_labels(n_nodes,posn,cc_label,topic_labels);

         components_levels_map[p]=nnodes_posn_labels;

         cout << "level = " << level
              << " connected_component_ID = " << connected_component_ID
              << " n_nodes = " << n_nodes
              << endl;

         int n_links=-1;
         string SQL_cmd=
            graphdbfunc::generate_insert_connected_component_SQL_command(
               hierarchy_ID,graph_ID,level,connected_component_ID,
               n_nodes,n_links);
         ccstream << SQL_cmd << endl;
      } // loop over index i labeling filefunc::text_line
      filefunc::closefile(cc_filename,ccstream);

      string banner="Exported SQL script for inserting connected components into images database to "+cc_filename;
      outputfunc::write_big_banner(banner);

   } // connected_components_calculated_flag conditional

   int n_graphs=n_levels;
   string SQL_cmd=graphdbfunc::generate_insert_graph_hierarchy_SQL_command(
      hierarchy_ID,hierarchy_name,n_graphs,n_levels,n_connected_components);

   string graph_hierarchy_sql_filename=
      graphs_subdir+"insert_graph_hierarchy.sql";
   ofstream sql_stream;
   filefunc::openfile(graph_hierarchy_sql_filename,sql_stream);
   sql_stream << SQL_cmd << endl;
   filefunc::closefile(graph_hierarchy_sql_filename,sql_stream);

   string banner="Exported SQL script for inserting graph hierarchy into images database to "
      +graph_hierarchy_sql_filename;
   outputfunc::write_big_banner(banner);


   banner="Manually run exported SQL scripts before proceeding to next step in IMAGE SEARCH database loading procedure!";
   outputfunc::write_banner(banner);
}
