// ==========================================================================
// Program CLASSIFY_IMAGES recursively imports all image files from
// some specified subdirectory.  It also imports probabilistic
// decision functions for several scene categories previously
// generated via progarm SVM_GIST.  Looping over each input image,
// CLASSIFY_IMAGES computes its GIST descriptor and scene category
// probability.  Any probability below some input threshold is
// ignored.  The maximal surviving probability is used to classify the
// scene for the input image.  A soft link is generated between
// classified images and scene subdirectories.

//			     ./classify_images

// ==========================================================================
// Last updated on 4/7/13; 4/8/13; 4/14/13; 10/2/13
// ==========================================================================

#include  <iostream>
#include  <map>
#include  <set>
#include  <string>
#include  <vector>
#include "dlib/svm.h"

#include "general/filefuncs.h"
#include "video/descriptorfuncs.h"
#include "general/outputfuncs.h"
#include "video/RGB_analyzer.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "video/texture_rectangle.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::map;
using std::pair;
using std::string;
using std::vector;


int main(int argc, char* argv[])
{
   cout.precision(12);

   RGB_analyzer* RGB_analyzer_ptr=new RGB_analyzer();
   RGB_analyzer_ptr->import_quantized_RGB_lookup_table();
   string lookup_map_name=RGB_analyzer_ptr->get_lookup_filename();
   cout << "lookup_map_name = " << lookup_map_name << endl;
   texture_rectangle* texture_rectangle_ptr=new texture_rectangle();


   string mains_gist_subdir=
      "/home/cho/programs/c++/svn/projects/src/mains/gist/";
   string all_images_subdir=mains_gist_subdir+"all_images/";
   string input_images_subdir=all_images_subdir+"testing_images/";
   bool search_all_children_dirs_flag=true;
   vector<string> image_filenames=filefunc::image_files_in_subdir(
      input_images_subdir,search_all_children_dirs_flag);
   int n_images=image_filenames.size();
   cout << "n_images = " << n_images << endl;

   std::vector<string> scene;
   scene.push_back("coast");
   scene.push_back("forest");
   scene.push_back("highway");
   scene.push_back("insidecity");
   scene.push_back("mountain");
   scene.push_back("opencountry");
   scene.push_back("street");
   scene.push_back("tallbldg");
   int n_scenes=scene.size();

// Import probabilistic decision functions generated by an SVM with a
// linear kernel on N scene and N' non-scene GIST descriptors:

   const int K=3*512;

   typedef dlib::matrix<double, K, 1> Ng_sample_type;
   typedef dlib::linear_kernel<Ng_sample_type> Ng_kernel_type;
   Ng_sample_type Ng_sample;

   typedef dlib::probabilistic_decision_function<Ng_kernel_type> 
      Ng_probabilistic_funct_type;  
   typedef dlib::normalized_function<Ng_probabilistic_funct_type> 
      Ng_pfunct_type;
   Ng_pfunct_type Ng_pfunct;
   vector<Ng_pfunct_type> Ng_pfuncts;

   for (unsigned int scene_index=0; scene_index < scene.size(); scene_index++)
   {
      cout << "--------------------------------------------------------------" 
           << endl;
      cout << "s = " << scene_index
           << " scene label = " << scene[scene_index]
           << endl;

      string learned_funcs_subdir="./learned_functions/"+scene[scene_index]
         +"/";
      string learned_Ng_pfunct_filename=learned_funcs_subdir;
      learned_Ng_pfunct_filename += "symbols_Ng_pfunct.dat";
      cout << "learned_Ng_pfunct_filename = "
           << learned_Ng_pfunct_filename << endl;
      ifstream fin6(learned_Ng_pfunct_filename.c_str(),ios::binary);
      deserialize(Ng_pfunct, fin6);
      Ng_pfuncts.push_back(Ng_pfunct);
   } // loop over scene_index
   
// Compute GIST descriptors for each input image:

   double scene_gist_threshold=0.90;
   cout << endl;
   cout << "Enter scene gist threshold:" << endl;
   cin >> scene_gist_threshold;

// Loop over input images starts here:

   string classified_images_subdir=all_images_subdir+"classified_images/";

   int i_start=0;
//   int i_start=12090;
   int i_stop=n_images-1;

   int n_rejections=0;
   int n_acceptances=0;

   timefunc::initialize_timeofday_clock();
   
   for (int i=i_start; i<=i_stop; i++)
   {
      outputfunc::print_elapsed_time();
      cout << "Classifying image " << i << " of " << n_images << endl;
      cout << "scene gist threshold = " << scene_gist_threshold << endl;
      cout << "image filename = " << image_filenames[i] << endl;

      string curr_image_subdir=filefunc::getdirname(image_filenames[i]);
      string gist_subdir=curr_image_subdir+"gist_files/";
      filefunc::dircreate(gist_subdir);
      string colorhist_subdir=curr_image_subdir+"color_histograms/";
      filefunc::dircreate(colorhist_subdir);

      string image_basename=filefunc::getbasename(image_filenames[i]);
      string image_filename_prefix=stringfunc::prefix(image_basename);
      string gist_filename=gist_subdir+image_filename_prefix+".gist";
      string color_histogram_filename=colorhist_subdir+
         image_filename_prefix+".colorhist";

// Check whether GIST descriptor file already exists.  If not, generate it:

      if (!filefunc::fileexist(gist_filename)) 
      {
         bool gist_calculated_flag=descriptorfunc::compute_gist_descriptor(
            image_filenames[i],gist_filename);
         if (!gist_calculated_flag) continue;
      }

// Check whether color histogram file already exists.  If not, generate it:
      
      if (!filefunc::fileexist(color_histogram_filename)) 
      {
         vector<double> color_histogram=
            descriptorfunc::compute_color_histogram(
               image_filenames[i],color_histogram_filename,
               texture_rectangle_ptr,RGB_analyzer_ptr);
         if (color_histogram.size()==0) continue;
      }

// Import GIST descriptor (and eventually color histogram).  Assemble
// into feature vector and feed into SVM:

      filefunc::ReadInfile(gist_filename);
      vector<double> column_values=stringfunc::string_to_numbers(
         filefunc::text_line[0]);
      
      for (int k=0; k<K; k++)
      {
         Ng_sample(k)=column_values[k];
      } // loop over index k labeling gist descriptors

// Looping over all decision functions, evaluate classification
// probabilities.  Ignore any probability which falls below
// scene_gist_threshold.  Classify image based upon maximal surviving
// probability:

      int max_s=-1;
      double max_classification_prob=-1;
      for (int s=0; s<n_scenes; s++)
      {
         double classification_prob=Ng_pfuncts[s](Ng_sample);
         if (classification_prob > max_classification_prob)
         {
            max_classification_prob=classification_prob;
            max_s=s;
         }
      } // loop over index s labeling scenes

      if (max_classification_prob < scene_gist_threshold)
      {
         n_rejections++;
      }
      else
      {
         n_acceptances++;
         string classified_scene_images_subdir=classified_images_subdir+
            scene[max_s]+"/";
         string threshold_str="threshold_"+
            stringfunc::number_to_string(scene_gist_threshold,3);
         classified_scene_images_subdir=classified_scene_images_subdir+
            threshold_str+"/";
         filefunc::dircreate(classified_scene_images_subdir);
         string unix_cmd="ln -s "+image_filenames[i]+" "+
            classified_scene_images_subdir;
//         cout << "unix_cmd = " << unix_cmd << endl;
         sysfunc::unix_command(unix_cmd);

         string banner="Classified image scene as "+scene[max_s];
         outputfunc::write_big_banner(banner);
//          outputfunc::enter_continue_char();
      }
   } // loop over index i labeling image filenames

   delete RGB_analyzer_ptr;
   delete texture_rectangle_ptr;

   cout << "n_rejections = " << n_rejections << endl;
   cout << "n_acceptances = " << n_acceptances << endl;
   
   double rejection_frac=double(n_rejections)/n_images;
   double acceptance_frac=double(n_acceptances)/n_images;
   cout << "Rejection fraction = " << rejection_frac << endl;
   cout << "Acceptance fraction = " << acceptance_frac << endl;
}
