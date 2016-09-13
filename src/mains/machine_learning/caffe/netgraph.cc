// ==========================================================================
// Program NETGRAPH imports a trained caffe model and text output
// generated by program ORDERED_ACTIVATIONS.  It first generates a
// layout for the neural net's layers with rows of a rectangle.  Nodes
// within a layer are grouped together within subrows of the
// rectangle.  The layout is exported to a text file which can be
// ingested by program mains/photosynth/fill_photo_hierarchy within
// the IMAGESEARCH pipeline.  NETGRAPH next generates an edgelist file
// which can be ingested by program mains/graphs/kmeans_clusters
// within the IMAGESEARCH pipeline.

//   ./netgraph
//    /data/caffe/faces/trained_models/test_160.prototxt                    
//    /data/caffe/faces/trained_models/Aug6_350K_96cap_T3/train_iter_702426.caffemodel 

// ==========================================================================
// Last updated on 8/25/16; 9/8/16; 9/12/16; 9/13/16
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
#include "numrec/nrfuncs.h"
#include "general/outputfuncs.h"
#include "math/prob_distribution.h"
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

   typedef std::map<int, pair<int,double> > ACTIVATIONS_MAP;
// independent int contains new global node ID
// dependent pair contains (old global node ID, mean nonzero activation value)

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




// Import activations for each FACENET node within each layer order by
// their stimulation frequencies generated via program
// ORDER_ACTIVATIONS:

   string network_subdir = "./vis_facenet/network/";
   string activations_subdir = network_subdir + "activations/";
   string ordered_activations_filename=activations_subdir+
      "ordered_activations.dat";

   vector< vector<double> > row_numbers = 
      filefunc::ReadInRowNumbers(ordered_activations_filename);

   ACTIVATIONS_MAP activations_map;
   ACTIVATIONS_MAP::iterator activations_iter;
   for(unsigned int r = 0; r < row_numbers.size(); r++)
   {
//      int layer = row_numbers.at(r).at(0);
      int old_global_node_id = row_numbers.at(r).at(2);
      int new_global_node_id = row_numbers.at(r).at(4);
      double median_activation = row_numbers.at(r).at(5);
      pair<int,double> p(old_global_node_id, median_activation);
      activations_map[new_global_node_id] = p;
   }

   string edgelist_filename="./edgelist.dat";
   ofstream edgelist_stream;
   filefunc::openfile(edgelist_filename, edgelist_stream);

   int minor_layer_skip = -1;
   int fc_layer_ID, reduced_chip_size;
   vector<string> major_layer_names;
   if (Alexnet_flag)
   {
      major_layer_names.push_back("conv1");
      major_layer_names.push_back("conv2");
      major_layer_names.push_back("conv3");
      major_layer_names.push_back("conv4");
      major_layer_names.push_back("conv5");
      major_layer_names.push_back("fc6");
      major_layer_names.push_back("fc7");
      major_layer_names.push_back("fc8");
   }
   else if(VGG_flag)
   {
      major_layer_names.push_back("conv1_1");
      major_layer_names.push_back("conv1_2");
      major_layer_names.push_back("conv2_1");
      major_layer_names.push_back("conv2_2");
      major_layer_names.push_back("conv3_1");
      major_layer_names.push_back("conv3_2");
      major_layer_names.push_back("conv3_3");
      major_layer_names.push_back("conv4_1");
      major_layer_names.push_back("conv4_2");
      major_layer_names.push_back("conv4_3");
      major_layer_names.push_back("conv5_1");
      major_layer_names.push_back("conv5_2");
      major_layer_names.push_back("conv5_3");
      major_layer_names.push_back("fc6");
      major_layer_names.push_back("fc7");
      major_layer_names.push_back("fc8");
   }
   else if (Resnet50_flag)
   {
      major_layer_names.push_back("input");
      major_layer_names.push_back("conv1");
      major_layer_names.push_back("res2a");
      major_layer_names.push_back("res2b");
      major_layer_names.push_back("res2c");
      major_layer_names.push_back("res3a");
      major_layer_names.push_back("res3b");
      major_layer_names.push_back("res3c");
      major_layer_names.push_back("res3d");
      major_layer_names.push_back("res4a");
      major_layer_names.push_back("res4b");
      major_layer_names.push_back("res4c");
      major_layer_names.push_back("res4d");
      major_layer_names.push_back("res4e");
      major_layer_names.push_back("res4f");
      major_layer_names.push_back("res5a");
      major_layer_names.push_back("res5b");
      major_layer_names.push_back("res5c");
      major_layer_names.push_back("fc1000");
   }
   else if (Facenet_flag)
   {

/*
// Facenet Model 2e:

      minor_layer_skip = 2;
      major_layer_names.push_back("data");
      major_layer_names.push_back("conv1");
      major_layer_names.push_back("conv2");
      major_layer_names.push_back("conv3");
      major_layer_names.push_back("conv4");
      major_layer_names.push_back("fc5");
      major_layer_names.push_back("fc6");
      major_layer_names.push_back("fc7_faces");
      fc_layer_ID = 5;
      reduced_chip_size = 12 * 12;
*/

// Facenet model 2n, 2q, 2r  (conv3a + conv3b + conv4a + conv4b)

//      minor_layer_skip = 2;	// Facenet model 2n
      minor_layer_skip = 6;	// Facenet model 2q, 2r
      major_layer_names.push_back("data");
      major_layer_names.push_back("conv1");
      major_layer_names.push_back("conv2");
      major_layer_names.push_back("conv3a");
      major_layer_names.push_back("conv3b");
      major_layer_names.push_back("conv4a");
      major_layer_names.push_back("conv4b");
      major_layer_names.push_back("fc5");
      major_layer_names.push_back("fc6");
      major_layer_names.push_back("fc7_faces");
      fc_layer_ID = 7;
      reduced_chip_size = 12 * 12;

// Facenet 1:

//      major_layer_names.push_back("data");
//      major_layer_names.push_back("conv1a");
//      major_layer_names.push_back("conv2a");
//      major_layer_names.push_back("conv3a");
//      major_layer_names.push_back("conv4a");
//      major_layer_names.push_back("fc5");
//      major_layer_names.push_back("fc6");
//      major_layer_names.push_back("fc7_faces");
   }

   caffe_classifier classifier(test_prototxt_filename, caffe_model_filename,
                               minor_layer_skip);

// Generate layout for network layers and nodes within a 0 < gx <
// n_layers , 0 < gy < 2 * n_layers rectangle.  Export layout to graph
// layout filename which can be ingested by program
// mains/photosynth/fill_photo_hierarchy within the IMAGESEARCH
// pipeline:

   vector<int> n_minor_layer_channels;
   n_minor_layer_channels.push_back(3); // Zeroth data layer contains 3 RGB channels
   int max_nodes_per_layer = 0;
   for(unsigned int n = 0; n < classifier.get_n_param_layer_nodes().size();
       n++)
   {
      n_minor_layer_channels.push_back(
         classifier.get_n_param_layer_nodes().at(n));
      max_nodes_per_layer = basic_math::max(max_nodes_per_layer,
                                            n_minor_layer_channels.back());
   }

   for(unsigned int minor_layer_index = 0; 
       minor_layer_index < n_minor_layer_channels.size(); 
       minor_layer_index++)
   {
      cout << "minor_layer_index = " << minor_layer_index
           <<  " n_minor_layer_channels = " 
           << n_minor_layer_channels[minor_layer_index]
           << endl;
   }
   cout << "max_nodes_per_layer = " << max_nodes_per_layer << endl;

   string graph_layout_filename="./graph_XY_coords.fm3_layout";
   ofstream graph_layout_stream;
   filefunc::openfile(graph_layout_filename, graph_layout_stream);
   graph_layout_stream << "# Image_ID  gX       gY  " << endl << endl;

   int new_global_node_ID = 0;
   int n_major_layers = classifier.get_n_major_layers();
   int n_minor_layers = classifier.get_n_minor_layers();
   cout << "n_major_layers = " << n_major_layers << endl;
   cout << "n_minor_layers = " << n_minor_layers << endl;
   cout << "minor_layer_skip = " << classifier.get_minor_layer_skip() << endl;

   for(int major_layer_ID = 0; major_layer_ID < n_major_layers; 
       major_layer_ID++)
   {
      int minor_layer_index;
      if(major_layer_ID == 0)
      {
         minor_layer_index = 0;
      }
      else
      {
         minor_layer_index = 1 + (major_layer_ID - 1) * minor_layer_skip;
      }
      
      int n_curr_layer_nodes = n_minor_layer_channels[minor_layer_index];

      cout << " Major_layer_ID = " << major_layer_ID 
           << " Minor_layer_index = " << minor_layer_index << " has " 
           << n_curr_layer_nodes << " nodes" << endl;

      int start_node_ID = new_global_node_ID;
      double gy = 2 * (n_major_layers - major_layer_ID) + 0.5;
      
      int max_n_nodes_per_subrow = 52;
      int n_layer_subrows = n_curr_layer_nodes / max_n_nodes_per_subrow + 1;
      for(int c = 0; c < n_layer_subrows - 1; c++)
      {
         int m_start = -max_n_nodes_per_subrow / 2;
         int m_stop = m_start + max_n_nodes_per_subrow;
         for(int m = m_start; m < m_stop; m++)
         {
            double numer = m + (c%2) * 0.5;
            double gx = n_major_layers * 
               (0.5 + numer / max_n_nodes_per_subrow);
            double gy_prime = gy + 0.25 * (n_layer_subrows - 1 - c);

            activations_iter = activations_map.find(new_global_node_ID);
            int old_global_node_ID = activations_iter->second.first;
//            graph_layout_stream << new_global_node_ID << "   " 
            graph_layout_stream << old_global_node_ID << "   " 
                                << gx << "   " << gy_prime << endl;

//            cout << "new_global_node_ID= " << new_global_node_ID
//                 << " old_global_node_ID = " << old_global_node_ID
//                 << endl;
            new_global_node_ID++;
         } // loop over index m labeling subcolumns within network graph
      }
      int remaining_subrow_nodes = n_curr_layer_nodes - max_n_nodes_per_subrow
         * (n_layer_subrows - 1);
      int m_start = -remaining_subrow_nodes / 2;
      int m_stop = m_start + remaining_subrow_nodes;
      for(int m = m_start; m < m_stop; m++)
      {
         double gx = n_major_layers * 
            (0.5 + double(m) / max_n_nodes_per_subrow);

         activations_iter = activations_map.find(new_global_node_ID);
         int old_global_node_ID = activations_iter->second.first;
//         graph_layout_stream << new_global_node_ID << "   " 
         graph_layout_stream << old_global_node_ID << "   " 
                             << gx << "   " << gy << endl;

//         cout << "new_global_node_id = " << new_global_node_ID
//              << " old_global_node_id = " << old_global_node_ID
//              << endl;

         new_global_node_ID++;
      }

      int stop_node_ID = new_global_node_ID - 1;
      cout << "  Node IDs range from " << start_node_ID << " to "
           << stop_node_ID << endl;

   } // loop over index n labeling layers within network graph
   filefunc::closefile(graph_layout_filename, graph_layout_stream);

// Generate edgelist for neural network graph which can be imported by
// program mains/graphs/kmeans_clusters within the IMAGESEARCH
// pipeline.

   EDGELIST_MAP layer_edgelist_map;
   EDGELIST_MAP::iterator layer_edgelist_iter;

   int n_middle = 0;
   int n_total = 0;

   for(int major_layer_ID = 0; major_layer_ID < n_major_layers-1 ; 
       major_layer_ID++)
   {
      string curr_major_layer_name=major_layer_names[major_layer_ID];
      string next_major_layer_name=major_layer_names[major_layer_ID+1];

      int curr_param_layer_index = (major_layer_ID) * minor_layer_skip;
      int n_curr_layer_nodes = classifier.get_param_layer_n_input_nodes(
         curr_param_layer_index);
      int n_next_layer_nodes = classifier.get_param_layer_n_output_nodes(
         curr_param_layer_index);
      
      layer_edgelist_map.clear();
      cout << "=====================================================" << endl;
      cout << "major_layer_ID = " << major_layer_ID << endl;
      cout << "   curr_major_layer_name = " << curr_major_layer_name << endl;
      cout << "   next_major_layer_name = " << next_major_layer_name << endl;
      cout << "   curr_param_layer_index = " << curr_param_layer_index << endl;
      cout << "   n_curr_layer_nodes = " << n_curr_layer_nodes << endl;
      cout << "   n_next_layer_nodes = " << n_next_layer_nodes << endl;


      for(int curr_node = 0; curr_node < n_curr_layer_nodes; curr_node++)
      {
         int curr_node_major_ID;
         if(major_layer_ID == fc_layer_ID - 1)
         {
            curr_node_major_ID = curr_node / reduced_chip_size;
         }
         else
         {
            curr_node_major_ID = classifier.get_major_weight_node_ID(
               major_layer_ID, curr_node);
         }

         for(int next_node = 0; next_node < n_next_layer_nodes; next_node++)
         {
            int next_node_major_ID = classifier.get_major_weight_node_ID(
               major_layer_ID+1, next_node);

            float weight_sum = 0;
            if(major_layer_ID == fc_layer_ID - 1)
            {
               weight_sum = nrfunc::ran1();
            }
            else
            {
               weight_sum = classifier.get_weight_sum(
                  curr_param_layer_index,curr_node,next_node);
            }

            DUPLE duple(curr_node_major_ID, next_node_major_ID);
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

      cout << " median_weight_sum = " << median_weight_sum
           << " quartile_width = " << quartile_width
           << endl;

      double min_weight_sum = mathfunc::minimal_value(weight_sums);
      double max_weight_sum = mathfunc::maximal_value(weight_sums);
      prob_distribution weights_prob(
         200, min_weight_sum, max_weight_sum, weight_sums);

      weights_prob.set_xmin(-0.05);
      weights_prob.set_xmax(0.05);
      weights_prob.set_xtic(0.01);
      weights_prob.set_xsubtic(0.005);

      weights_prob.set_xlabel("Weight sums");
      weights_prob.set_color(colorfunc::get_color(major_layer_ID));
      weights_prob.set_densityfilenamestr(
         "weights_sum_"+stringfunc::number_to_string(major_layer_ID)+".meta");
      weights_prob.write_density_dist(false,true);

// Renormalize weights so that they mostly lie within interval [0,100]

// 	median_weight_sum + Z * quartile_width --> Upper value = 95
// 	median_weight_sum - Z * quartile_width --> Lower value =  5

      int Z = 5;
      double Upper = 90;
      double Lower = 10;
      double beta = 0.5 * (Upper - Lower) / (Z * quartile_width);
      double alpha = 0.5 * (Upper + Lower) - beta * median_weight_sum;
    
//      double Upper = 95;
//      double Lower = 5;
//      double alpha = 50;
 //     double beta = 900;

//      double alpha = 50;
//      double beta = 2537;

      vector<double> renorm_weights;
      for(layer_edgelist_iter = layer_edgelist_map.begin();
          layer_edgelist_iter != layer_edgelist_map.end();
          layer_edgelist_iter++)
      {
         int curr_node_global_ID = layer_edgelist_iter->first.first;
         int next_node_global_ID = layer_edgelist_iter->first.second;
         double weight_sum = layer_edgelist_iter->second;
         double renorm_weight_sum = alpha + beta * weight_sum;
         renorm_weights.push_back(renorm_weight_sum);
         edgelist_stream << curr_node_global_ID << "  "
                         << next_node_global_ID << "  "
                         << renorm_weight_sum << endl;
         if(renorm_weight_sum > Lower && renorm_weight_sum < Upper)
         {
            n_middle++;
         }
         n_total++;
      }
      
      double median_renorm_weight;
      mathfunc::median_value_and_quartile_width(
         renorm_weights, median_renorm_weight, quartile_width);
      cout << "median renorm weight = " << median_renorm_weight
           << " quartile width = " << quartile_width << endl;

      double frac_middle = double(n_middle)/n_total;
      cout << "frac_middle = " << frac_middle << endl;

   } // loop over major_layer_ID index

   filefunc::closefile(edgelist_filename, edgelist_stream);

   string banner="Exported node graph coordinates to "+graph_layout_filename;
   outputfunc::write_banner(banner);
   banner="Exported renormalized weight edgelist to "+edgelist_filename;
   outputfunc::write_banner(banner);
}

