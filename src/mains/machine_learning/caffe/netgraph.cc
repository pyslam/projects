// ==========================================================================
// Program NETGRAPH imports a trained caffe model.  It first generates
// a layout for the neural net's layers with rows of a rectangle.
// Nodes within a layer are grouped together within subrows of the
// rectangle.  The layout is exported to a text file which can be
// ingested by program mains/photosynth/fill_photo_hierarchy wtihin
// the IMAGESEARCH pipeline.  NETGRAPH next generates an edgelist file
// which can be ingested by program mains/graphs/kmeans_clusters
// within the IMAGESEARCH pipeline.

//   ./netgraph
//    /data/caffe/faces/trained_models/test_160.prototxt                    
//    /data/caffe/faces/trained_models/Aug6_350K_96cap_T3/train_iter_702426.caffemodel 

// ==========================================================================
// Last updated on 8/19/16; 8/20/16
// ==========================================================================

#include  <algorithm>
#include  <fstream>
#include  <iostream>
#include  <map>
#include  <set>
#include  <string>
#include  <vector>

#include "math/basic_math.h"
#include "classification/caffe_classifier.h"
#include "general/filefuncs.h"
#include "math/mathfuncs.h"
#include "general/outputfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::ifstream;
using std::ios;
using std::map;
using std::ofstream;
using std::pair;
using std::string;
using std::vector;

// ==========================================================================

int main(int argc, char* argv[])
{
   timefunc::initialize_timeofday_clock();

   typedef std::map<DUPLE, float, ltduple> EDGELIST_MAP;
// independent DUPLE contains (global ID for node in layer L, 
//                             global ID for node in layer L+1)
// dependent float contains edge weight

   string test_prototxt_filename = argv[1];
   string caffe_model_filename = argv[2];

   bool Alexnet_flag = false;
   bool VGG_flag = false;
   bool Resnet50_flag = false;
   bool Facenet_flag = false;
   string caffe_model_basename=filefunc::getbasename(caffe_model_filename);

   if(caffe_model_basename == "bvlc_reference_caffenet.caffemodel")
   {
      Alexnet_flag = true;
   }
   else if(caffe_model_basename == "VGG_ILSVRC_16_layers.caffemodel")
   {
      VGG_flag = true;
   }
   else if(caffe_model_basename == "ResNet-50-model.caffemodel")
   {
      Resnet50_flag = true;
   }
   else 
   {
      Facenet_flag = true;
   }

   cout << "caffe_model_basename = " << caffe_model_basename << endl;
   cout << "Alexnet_flag = " << Alexnet_flag << endl;
   cout << "VGG_flag = " << VGG_flag << endl;
   cout << "Resnet50_flag = " << Resnet50_flag << endl;
   cout << "Facenet_flag = " << Facenet_flag << endl;

   caffe_classifier classifier(test_prototxt_filename, caffe_model_filename);

   string edgelist_filename="./edgelist.dat";
   ofstream edgelist_stream;
   filefunc::openfile(edgelist_filename, edgelist_stream);

   vector<string> param_layer_names;
   if (Alexnet_flag)
   {
      param_layer_names.push_back("conv1");
      param_layer_names.push_back("conv2");
      param_layer_names.push_back("conv3");
      param_layer_names.push_back("conv4");
      param_layer_names.push_back("conv5");
      param_layer_names.push_back("fc6");
      param_layer_names.push_back("fc7");
      param_layer_names.push_back("fc8");
   }
   else if(VGG_flag)
   {
      param_layer_names.push_back("conv1_1");
      param_layer_names.push_back("conv1_2");
      param_layer_names.push_back("conv2_1");
      param_layer_names.push_back("conv2_2");
      param_layer_names.push_back("conv3_1");
      param_layer_names.push_back("conv3_2");
      param_layer_names.push_back("conv3_3");
      param_layer_names.push_back("conv4_1");
      param_layer_names.push_back("conv4_2");
      param_layer_names.push_back("conv4_3");
      param_layer_names.push_back("conv5_1");
      param_layer_names.push_back("conv5_2");
      param_layer_names.push_back("conv5_3");
      param_layer_names.push_back("fc6");
      param_layer_names.push_back("fc7");
      param_layer_names.push_back("fc8");
   }
   else if (Resnet50_flag)
   {
      param_layer_names.push_back("input");
      param_layer_names.push_back("conv1");
      param_layer_names.push_back("res2a");
      param_layer_names.push_back("res2b");
      param_layer_names.push_back("res2c");
      param_layer_names.push_back("res3a");
      param_layer_names.push_back("res3b");
      param_layer_names.push_back("res3c");
      param_layer_names.push_back("res3d");
      param_layer_names.push_back("res4a");
      param_layer_names.push_back("res4b");
      param_layer_names.push_back("res4c");
      param_layer_names.push_back("res4d");
      param_layer_names.push_back("res4e");
      param_layer_names.push_back("res4f");
      param_layer_names.push_back("res5a");
      param_layer_names.push_back("res5b");
      param_layer_names.push_back("res5c");
      param_layer_names.push_back("fc1000");
   }
   else if (Facenet_flag)
   {
      param_layer_names.push_back("data");
      param_layer_names.push_back("conv1a");
      param_layer_names.push_back("conv2a");
      param_layer_names.push_back("conv3a");
      param_layer_names.push_back("conv4a");
      param_layer_names.push_back("fc5");
      param_layer_names.push_back("fc6");
      param_layer_names.push_back("fc7_faces");
   }

// Generate layout for network layers and nodes within a 0 <gx <
// n_layers , 0 < gy < 2 * n_layers rectangle.  Export layout to graph
// layout filename which can be ingested by program
// mains/photosynth/fill_photo_hierarchy within the IMAGESEARCH
// pipeline:

   vector<int> n_layer_channels;
   n_layer_channels.push_back(3); // Zeroth data layer contains 3 RGB channels
   int max_nodes_per_layer = 0;
   for(unsigned int n = 0; n < classifier.get_n_param_layer_nodes().size();
       n++)
   {
      n_layer_channels.push_back(
         classifier.get_n_param_layer_nodes().at(n));
      max_nodes_per_layer = basic_math::max(max_nodes_per_layer,
                                            n_layer_channels.back());
   }
   cout << "max_nodes_per_layer = " << max_nodes_per_layer << endl;

   string graph_layout_filename="./graph_XY_coords.fm3_layout";
   ofstream graph_layout_stream;
   filefunc::openfile(graph_layout_filename, graph_layout_stream);
   graph_layout_stream << "# Image_ID  gX       gY  " << endl << endl;

   int node_counter = 0;
   int n_layers = n_layer_channels.size();
   for(int n = 0; n < n_layers; n++)
   {
      int n_curr_layer_nodes = n_layer_channels[n];
      cout << "Network layer = " << n << " has " 
           << n_curr_layer_nodes << " nodes" << endl;
      int start_node_ID = node_counter;
      double gy = 2 * (n_layers - n);
      
      int max_n_nodes_per_subrow = 52;
//      int max_n_nodes_per_subrow = 64;
      int n_layer_columns = n_curr_layer_nodes / max_n_nodes_per_subrow + 1;
      for(int c = 0; c < n_layer_columns - 1; c++)
      {
         int m_start = -max_n_nodes_per_subrow / 2;
         int m_stop = m_start + max_n_nodes_per_subrow;
         for(int m = m_start; m < m_stop; m++)
         {
            double numer = m + (c%2) * 0.5;
            double gx = n_layers * (0.5 + numer / max_n_nodes_per_subrow);
            double gy_prime = gy - 0.25 * (n_layer_columns - 1 - c);
            graph_layout_stream << node_counter++ << "   " 
                                << gx << "   " << gy_prime << endl;
         } // loop over index m labeling rows within network graph
      }
      int remaining_column_nodes = n_curr_layer_nodes - max_n_nodes_per_subrow
         * (n_layer_columns - 1);
      int m_start = -remaining_column_nodes / 2;
      int m_stop = m_start + remaining_column_nodes;
      for(int m = m_start; m < m_stop; m++)
      {
         double gx = n_layers * (0.5 + double(m) / max_n_nodes_per_subrow);
         graph_layout_stream << node_counter++ << "   " 
                             << gx << "   " << gy << endl;

      }

      int stop_node_ID = node_counter - 1;
      cout << "  Node IDs range from " << start_node_ID << " to "
           << stop_node_ID << endl;

   } // loop over index n labeling columns within network graph
   filefunc::closefile(graph_layout_filename, graph_layout_stream);

// Generate edgelist for neural network graph which can be imported by
// program mains/graphs/kmeans_clusters within the IMAGESEARCH
// pipeline.

   EDGELIST_MAP layer_edgelist_map;
   EDGELIST_MAP::iterator layer_edgelist_iter;

   int n_middle = 0;
   int n_total = 0;

   int layer_start = 0;  
   int layer_stop = n_layer_channels.size() - 1;
   cout << "layer_start = " << layer_start
        << " layer_stop = " << layer_stop << endl << endl;

   for(int layer_index = layer_start; layer_index < layer_stop; layer_index++)
   {
      layer_edgelist_map.clear();
      cout << "Processing layer_index = " << layer_index << endl;
      
      for(int curr_node = 0; curr_node < n_layer_channels[layer_index]; 
          curr_node++)
      {
         int curr_node_global_ID = classifier.get_global_weight_node_ID(
            layer_index, curr_node);
         string curr_layer_name=param_layer_names[layer_index];

         for(int next_node = 0; next_node < n_layer_channels[layer_index+1];
             next_node++)
         {
            int next_node_global_ID = classifier.get_global_weight_node_ID(
               layer_index+1, next_node);
            float weight_sum = 
               classifier.get_weight_sum(layer_index,curr_node,next_node);

            DUPLE duple(curr_node_global_ID, next_node_global_ID);
            layer_edgelist_map[duple] = weight_sum;
         } // loop over next_node index
      } // loop over curr_node index

      vector<double> weight_sums;
      for(layer_edgelist_iter = layer_edgelist_map.begin();
          layer_edgelist_iter != layer_edgelist_map.end();
          layer_edgelist_iter++)
      {
         weight_sums.push_back(layer_edgelist_iter->second);
      }

      double median_weight_sum, quartile_width;
      mathfunc::median_value_and_quartile_width(
         weight_sums, median_weight_sum, quartile_width);

      cout << "layer = " << layer_index
           << " curr_layer_name = " << param_layer_names[layer_index]
           << " next_layer_name = " << param_layer_names[layer_index+1]
           << " median_weight_sum = " << median_weight_sum
           << " quartile_width = " << quartile_width
           << endl;

// Renormalize weights:

// 	median_weight_sum + Z * quartile_width --> Upper value = 90
// 	median_weight_sum - Z * quartile_width --> Lower value = 10

      int Z = 10;
      double Upper = 90;
      double Lower = 10;
      double beta = 0.5 * (Upper - Lower) / (Z * quartile_width);
      double alpha = 0.5 * (Upper + Lower) - beta * median_weight_sum;
      
      for(layer_edgelist_iter = layer_edgelist_map.begin();
          layer_edgelist_iter != layer_edgelist_map.end();
          layer_edgelist_iter++)
      {
         int curr_node_global_ID = layer_edgelist_iter->first.first;
         int next_node_global_ID = layer_edgelist_iter->first.second;
         double weight_sum = layer_edgelist_iter->second;
         double renorm_weight_sum = alpha + beta * weight_sum;
         edgelist_stream << curr_node_global_ID << "  "
                         << next_node_global_ID << "  "
                         << renorm_weight_sum << endl;
         if(renorm_weight_sum > Lower && renorm_weight_sum < Upper)
         {
            n_middle++;
         }
         n_total++;
         
      }
      double frac_middle = double(n_middle)/n_total;
      cout << "frac_middle = " << frac_middle << endl;

   } // loop over layer index

   filefunc::closefile(edgelist_filename, edgelist_stream);

   string banner="Exported node graph coordinates to "+graph_layout_filename;
   outputfunc::write_banner(banner);
   banner="Exported renormalized weight edgelist to "+edgelist_filename;
   outputfunc::write_banner(banner);
}
