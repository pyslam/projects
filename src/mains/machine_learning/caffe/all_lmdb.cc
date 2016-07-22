// ========================================================================
// Program ALL_LMDB imports LMDB database files generated by caffe's
// file extraction binary.  It retrieves the dimension of each
// "gist-like" feature vector for processed images.  ALL_LMDB
// generates an edge list text file for all (N choose 2) edges 
// among the N graph nodes.
// ========================================================================
// Last updated on 12/3/15; 12/4/15; 2/9/16; 2/10/16
// ========================================================================

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <lmdb++.h>
#include "caffe/proto/caffe.pb.h"
#include "math/basic_math.h"
#include "general/filefuncs.h"
#include "general/outputfuncs.h"
#include "math/prob_distribution.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::map;
using std::ofstream;
using std::pair;
using std::string;
using std::vector;

// ------------------------------------------------------------------
// Method extract_normalized_feature_vectors() retrieves feature
// descriptors stored within the input lmdb database.  It computes the
// l2-magnitude of each descriptor and returns feature vectors with
// unit l2-norm within an STL vector of vectors.

int extract_normalized_feature_vectors(
   lmdb::txn& rtxn, lmdb::dbi &dbi, vector<vector<float> >& feature_vectors)
{
   string key, value;
   int n_feature_dims = 0;
   auto cursor = lmdb::cursor::open(rtxn, dbi);

   while (cursor.get(key, value, MDB_NEXT)) 
   {
      caffe::Datum curr_datum;
      curr_datum.ParseFromString(value);
      vector<float> curr_feature_vector;
      n_feature_dims = curr_datum.channels();
      double sqrd_mag = 0;
      for (int j = 0; j < n_feature_dims; j++)
      {
         double curr_data_value = curr_datum.float_data(j);

// Recall Andrej Karpathy's hint: We need to apply "ReLU" operation to
// fc7 feature vectors!

         if(curr_data_value < 0) curr_data_value = 0;

         curr_feature_vector.push_back(curr_data_value);
         sqrd_mag += sqr(curr_data_value);
      }
      double mag = sqrt(sqrd_mag);
      for (int j = 0; j < n_feature_dims; ++j)
      {
         curr_feature_vector[j] = curr_feature_vector[j] / mag;
      }
      feature_vectors.push_back(curr_feature_vector);
   }

   cursor.close();

   return n_feature_dims;
}

// ------------------------------------------------------------------

int main() 
{
   double min_edge_weight = 70;	 
   cout << "Enter minimum edge weight:" << endl;
   cin >> min_edge_weight;

   string software_subdir = "/home/pcho/software/";
   string caffe_subdir = software_subdir+"caffe_custom/";
   string _temp_subdir = caffe_subdir+"examples/_temp/";
   string features_subdir = "features_roadsigns/";
  
/*
  cout << "Enter relative path to subdir of ./examples/_temp/ " << endl;
  cout << " containing extracted caffe features in lmdb database format: " << endl;
   cout << "  (e.g. features_tidmarsh )" << endl;

   
   cin >> features_subdir;
*/

   filefunc::add_trailing_dir_slash(features_subdir);
   features_subdir = _temp_subdir+features_subdir;
   cout << "Full features_subdir = " << features_subdir << endl;

   string files_list = features_subdir + "file_list.txt";
   filefunc::ReadInfile(files_list);

   // Create and open the LMDB environment:

   auto env = lmdb::env::create();
   env.set_mapsize(1UL * 1024UL * 1024UL *  1024UL); // 1 GiB 
   env.open(features_subdir.c_str(), 0, 0664);
   
   // Fetch key/value pairs in a read-only transaction: 

   auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
   auto dbi = lmdb::dbi::open(rtxn, nullptr);

   timefunc::initialize_timeofday_clock();

   vector<vector<float> > feature_vectors;
   int n_feature_dims = extract_normalized_feature_vectors(
      rtxn, dbi, feature_vectors);
   int n_feature_vectors = feature_vectors.size();
   cout << "n_feature_dims = " << n_feature_dims << endl;
   cout << "n_feature_vectors = " << n_feature_vectors << endl;
   outputfunc::print_elapsed_time();

   rtxn.abort();

// Open text file for edge list:

   string banner="Exporting edge list:";
   outputfunc::write_banner(banner);
   string edgelist_filename = features_subdir + "initial_edgelist.dat";
   ofstream edgelist_stream;
   filefunc::openfile(edgelist_filename, edgelist_stream);
   edgelist_stream << "# NodeID  NodeID'  Edge weight" << endl << endl;

// Compute inner products between feature vectors:

   int max_edges_per_node = 1000000;
//   int max_edges_per_node = 10;
   double dotproduct_counter = 0;
   vector<double> edge_weights;
   double n_dotproducts = double(n_feature_vectors) * 
      double(n_feature_vectors - 1) / 2;
   cout << "n_feature_vectors = " << n_feature_vectors << endl;
   cout << "n_dotproducts = " << n_dotproducts << endl;

   double edge_weight;
   vector<int> g_values;
   vector<float> feature_vec_f;
   vector<double> curr_edge_weights;
   g_values.reserve(n_feature_vectors);
   feature_vec_f.reserve(n_feature_dims);

   for(int f = 0; f < n_feature_vectors; f++)
   {
      outputfunc::update_progress_and_remaining_time(
        f, 0.01 * n_feature_vectors, n_feature_vectors);

      feature_vec_f.clear();
      for(int i = 0; i < n_feature_dims; i++)
      {
         feature_vec_f.push_back(feature_vectors[f].at(i));
      }

      g_values.clear();
      curr_edge_weights.clear();
      for(int g = 0; g < n_feature_vectors; g++)
      {
         if(f == g) continue;

// Take edge weight to equal 100 * dotproduct between feature vectors
// labeled by indices f and g:

         edge_weight = 0;
         for(int i = 0; i < n_feature_dims; i++)
         {
            edge_weight += 
              feature_vec_f[i] * (feature_vectors[g])[i];
         }
         edge_weight *= 100;

// Don't bother storing more than 10 million edge weights for prob
// distribution construction purposes:

         if(dotproduct_counter < 1E7)
         {
            edge_weights.push_back(edge_weight);
         }
         dotproduct_counter++;

// Ignore edge weight if it is less than minimal threshold value:

         if(edge_weight < min_edge_weight) continue;
         
         curr_edge_weights.push_back(edge_weight);
         g_values.push_back(g);
         
      } // loop over index g 

// Sort g_values according to curr_edge_weights in descending order:

      templatefunc::Quicksort_descending(curr_edge_weights, g_values);

      int n_edges = curr_edge_weights.size();
      if(n_edges > max_edges_per_node)
      {
         vector<int> indices;
         for(int i = 0; i < 0.7 * max_edges_per_node; i++)
         {
            indices.push_back(i);
         }
         vector<int> remaining_indices = mathfunc::random_sequence(
            indices.back()+1, n_edges - 1, 0.3 * max_edges_per_node);
         
         for(unsigned int i = 0; i < remaining_indices.size(); i++)
         {
            indices.push_back(remaining_indices[i]);
         }

         for(unsigned int i = 0; i < indices.size(); i++)
         {
            int curr_index = indices[i];
            int curr_g = g_values[curr_index];
            double curr_weight = curr_edge_weights[curr_index];
            edgelist_stream << f << "  " << curr_g << "  " 
                            << curr_weight << endl;
         }
      }
      else
      {
         for(unsigned int g = 0; g < g_values.size(); g++)
         {
            edgelist_stream << f << "  " << g_values[g] << "  " 
                            << curr_edge_weights[g] << endl;
         }
      }
   } // loop over index f
   filefunc::closefile(edgelist_filename, edgelist_stream);
   banner="Exported initial edgelist to "+edgelist_filename;
   outputfunc::write_banner(banner);
   outputfunc::print_elapsed_time();   

// Reimport edgelist into STL map.  Ignore any edge if it already
// exists within the map:

   typedef map<pair<int, int>, double > NODE_NODE_EDGE_MAP;
// independent pair var: (node ID f, node ID g)
// dependent double: edge weight
   NODE_NODE_EDGE_MAP node_node_edge_map;
   NODE_NODE_EDGE_MAP::iterator iter;

   pair<int, int> P;
   filefunc::ReadInfile(edgelist_filename);
   for(unsigned int i = 0; i < filefunc::text_line.size(); i++)
   {
      vector<double> curr_values = 
         stringfunc::string_to_numbers(filefunc::text_line[i]);
      P.first = basic_math::min(curr_values[0], curr_values[1]);
      P.second = basic_math::max(curr_values[0], curr_values[1]);
      double curr_weight = curr_values[2];

      iter = node_node_edge_map.find(P);
      if(iter == node_node_edge_map.end())
      {
         node_node_edge_map[P] = curr_weight;
      }
   }

// Rexport edgelist with unique node-node'-weight triples:

   edgelist_filename = features_subdir + "edgelist.dat";
   filefunc::openfile(edgelist_filename, edgelist_stream);
   for(iter = node_node_edge_map.begin(); iter != node_node_edge_map.end();
       iter++)
   {
      P = iter->first;
      double curr_weight = iter->second;
      edgelist_stream << P.first << "  " << P.second << "  " 
                      << curr_weight << endl;
   }
   filefunc::closefile(edgelist_filename, edgelist_stream);
   banner="Exported final edgelist to "+edgelist_filename;
   outputfunc::write_banner(banner);
   outputfunc::print_elapsed_time();   

// Generate probability distribution for edge weights:

   prob_distribution prob(edge_weights, 100);
   prob.set_xlabel("Edge weight");
   prob.set_xtic(20);
   prob.set_xsubtic(10);
   prob.writeprobdists(false,true);
   banner="Exported edge weight density and cumulative distributions";
   outputfunc::write_banner(banner);
   edge_weights.clear();
   outputfunc::print_elapsed_time();

   string unix_cmd="mv prob*.* "+features_subdir;
   sysfunc::unix_command(unix_cmd);
   banner="Moved edge weight density and cumulative dists into "+
     features_subdir;
   outputfunc::write_banner(banner);

   outputfunc::print_elapsed_time();

   return EXIT_SUCCESS;
}
