// ========================================================================
// Program FACE_EDGELIST imports a set of feature descriptors
// generated by program FACE_FEATURES.  It renormalizes each
// descriptor so that it has unit L2-norm.  FACE_EDGELIST then
// computes dotproducts between every descriptor with every other
// descriptor.  But it retains only max_edges_per_node so that the
// total number of edges exported by this program grows linearly and
// not quadratically with the number of nodes.  

//                               ./face_edgelist
// ========================================================================
// Last updated on 9/17/16
// ========================================================================

#include <iostream>
#include <map>
#include <string>
#include <vector>
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

// First 6170 feature descriptors correspond to testing and validation
// face image chips

int main() 
{
   double min_edge_weight = 70;	 // VGG-16
   cout << "Enter minimum edge weight:" << endl;
   cin >> min_edge_weight;

   timefunc::initialize_timeofday_clock();

   string caffe_subdir = 
      "/home/pcho/programs/c++/git/projects/src/mains/machine_learning/caffe/";
   string base_features_subdir = caffe_subdir + 
      "vis_facenet/network/activations/model_2t/features/";
   string features_subdir = base_features_subdir + "fc6/";
   
   vector<string> allowed_suffixes;
   allowed_suffixes.push_back("dat");
   vector<string> feature_filenames = 
      filefunc::files_in_subdir_matching_specified_suffixes(
         allowed_suffixes, features_subdir);
   cout << "Total number of feature descriptor files = "
        << feature_filenames.size() << endl;
   cout << "Enter number of features to process:" << endl;

   int n_feature_descriptors;
   cin >> n_feature_descriptors;

   int n_feature_dims = 256; // fc6
   cout << "n_feature_dims = " << n_feature_dims << endl;
   vector<vector<float> > feature_descriptors;

   ofstream imagepaths_stream;
   string imagepaths_filename = base_features_subdir+"imagepaths.dat";
   filefunc::openfile(imagepaths_filename, imagepaths_stream);

   for(int f = 0; f < n_feature_descriptors; f++)
   {
      outputfunc::update_progress_and_remaining_time(
         f, 0.10 * n_feature_descriptors, n_feature_descriptors);

      filefunc::ReadInfile(feature_filenames[f]);
      string image_filename = filefunc::text_line[0];
      string label = filefunc::text_line[1];

      imagepaths_stream << f << " " << image_filename << endl;

      double dmag_sqr = 0;
      vector<float> curr_descriptor;
      for(int d = 0; d < n_feature_dims; d++)
      {
         float curr_val = stringfunc::string_to_number(
            filefunc::text_line[2+d]);
         curr_descriptor.push_back(curr_val);
         dmag_sqr += curr_val * curr_val;
      }
/*
      double dmag = sqrt(dmag_sqr);
      for(int d = 0; d < n_feature_dims; d++)
      {
         curr_descriptor[d] = curr_descriptor[d] / dmag;
      }
*/

      feature_descriptors.push_back(curr_descriptor);
   } // loop over index f labeling feature filenames
   filefunc::closefile(imagepaths_filename, imagepaths_stream);
   string banner="Exported all input image paths to "+imagepaths_filename;
   outputfunc::write_banner(banner);

// Open text file for edge list:

   banner="Exporting edge list:";
   outputfunc::write_banner(banner);
   string edgelist_filename = base_features_subdir + "initial_edgelist.dat";
   ofstream edgelist_stream;
   filefunc::openfile(edgelist_filename, edgelist_stream);
   edgelist_stream << "# NodeID  NodeID'  Edge weight" << endl << endl;

// Compute inner products between feature descriptors:

   int max_edges_per_node = 6;
//   int max_edges_per_node = 10;
   double dotproduct_counter = 0;
   vector<double> edge_weights;
   double n_dotproducts = double(n_feature_descriptors) * 
      double(n_feature_descriptors - 1) / 2;
   cout << "n_feature_descriptors = " << n_feature_descriptors << endl;
   cout << "n_dotproducts = " << n_dotproducts << endl;

   double edge_weight;
   vector<int> g_values;
   vector<float> feature_vec_f, delta_vec;
   vector<double> curr_edge_weights;
   g_values.reserve(n_feature_descriptors);
   feature_vec_f.reserve(n_feature_dims);

   for(int f = 0; f < n_feature_descriptors; f++)
   {
      outputfunc::update_progress_and_remaining_time(
        f, 0.01 * n_feature_descriptors, n_feature_descriptors);

      feature_vec_f.clear();
      for(int i = 0; i < n_feature_dims; i++)
      {
         feature_vec_f.push_back(feature_descriptors[f].at(i));
      }

      g_values.clear();
      curr_edge_weights.clear();
      for(int g = 0; g < n_feature_descriptors; g++)
      {
         if(f == g) continue;

// Take edge weight to equal 100 * dotproduct between feature descriptors
// labeled by indices f and g:

//         double dotproduct = 0;
         double delta_mag_sqr = 0;
         for(int i = 0; i < n_feature_dims; i++)
         {
            delta_mag_sqr += sqr(
               feature_vec_f[i] - (feature_descriptors[g])[i]);
//            dotproduct += 
//               feature_vec_f[i] * (feature_descriptors[g])[i];
         }
//         edge_weight = 100 * dotproduct;
         double delta_mag = sqrt(delta_mag_sqr);
         if(delta_mag > 10)
         {
            edge_weight = 0;
         }
         else
         {
            edge_weight = 100 * (1 - 0.1 * delta_mag);
         }

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

   edgelist_filename = base_features_subdir + "edgelist.dat";
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

   string unix_cmd="mv prob*.* "+base_features_subdir;
   sysfunc::unix_command(unix_cmd);
   banner="Moved edge weight density and cumulative dists into "+
      base_features_subdir;
   outputfunc::write_banner(banner);

   outputfunc::print_elapsed_time();
}
