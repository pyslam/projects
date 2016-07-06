// ==========================================================================
// Text_Detector class member function definitions
// ==========================================================================
// Last modified on 6/3/14; 6/22/14; 6/24/14; 6/26/14
// ==========================================================================

#include <iostream>
#include <vector>

#include "general/filefuncs.h"
#include "general/stringfuncs.h"
#include "classification/text_detector.h"
#include "time/timefuncs.h"

using std::cout;
using std::endl;
using std::flush;
using std::ofstream;
using std::ostream;
using std::string;
using std::vector;

// ---------------------------------------------------------------------
// Initialization, constructor and destructor functions:
// ---------------------------------------------------------------------

void text_detector::initialize_member_objects()
{
   RGB_pixels_flag=false;
//   D=64;	// Descriptors correspond to sqrt(D) x sqrt(D) image patches

   window_width=32;
   window_height=32;

   avg_window_width=3;
   avg_window_height=3;
}

void text_detector::allocate_member_objects()
{
//   cout << "inside text_detector::allocate_member_objects()" << endl;
   inverse_sqrt_covar_ptr=new flann::Matrix<float>(new float[D*D],D,D);
   memset(inverse_sqrt_covar_ptr->ptr(),0,D*D*sizeof(float));

   patch_descriptor=new float[D];
   whitened_descriptor=new float[D];
   window_histogram=new float[9*K];
   Dtrans_inverse_sqrt_covar_ptr=new flann::Matrix<float>(new float[K*D],K,D);

   curr_feature_ptr=new float[K];

   texture_rectangle_ptr=new texture_rectangle();

   initialize_avg_window_features_vector();
}		       

// ---------------------------------------------------------------------
text_detector::text_detector(string dictionary_subdir,bool RGB_pixels_flag)
{
   cout << "inside text_detector constructor #1" << endl;

   this->dictionary_subdir=dictionary_subdir;
   this->RGB_pixels_flag=RGB_pixels_flag;

   import_dictionary();
   initialize_member_objects();
   allocate_member_objects();
}

// Copy constructor:

text_detector::text_detector(const text_detector& t)
{
   initialize_member_objects();
   allocate_member_objects();
   docopy(t);
}

text_detector::~text_detector()
{
//    delete mean_coeffs_ptr;
   delete inverse_sqrt_covar_ptr;
   delete [] patch_descriptor;
   delete [] whitened_descriptor;
   delete [] window_histogram;
   delete Dtrans_inverse_sqrt_covar_ptr;
   delete texture_rectangle_ptr;
   destroy_avg_window_features_vector();
   delete [] curr_feature_ptr;
}

// ---------------------------------------------------------------------
void text_detector::docopy(const text_detector& t)
{
//   cout << "inside text_detector::docopy()" << endl;
}

// Overload = operator:

text_detector& text_detector::operator= (const text_detector& t)
{
   if (this==&t) return *this;
   docopy(t);
   return *this;
}

// ---------------------------------------------------------------------
// Overload << operator:

ostream& operator<< (ostream& outstream,const text_detector& t)
{
   outstream << endl;

//   cout << "inside text_detector::operator<<" << endl;

   return outstream;
}

// ==========================================================================
// Dictionary importing and computation member functions
// ==========================================================================

// Member function import_inverse_sqrt_covar_matrix() imports 
// inverse square root covariance matrix elements from the text file
// files generated by program WHITEN_DESCRIPTORS.

void text_detector::import_inverse_sqrt_covar_matrix()
{
//   cout << "inside text_detector::import_inverse_sqrt_covar_matrix()" << endl;

   string inverse_covar_sqrt_filename=
      dictionary_subdir+"inverse_sqrt_covar_matrix.dat";
   filefunc::ReadInfile(inverse_covar_sqrt_filename);

   unsigned int line_counter=0;
   for (unsigned int i=0; i<D; i++)
   {
      for (unsigned int j=0; j<D; j++)
      {
         (*inverse_sqrt_covar_ptr)[i][j]=stringfunc::string_to_number(
            filefunc::text_line[line_counter]);
         line_counter++;         
//         cout << "i = " << i << " j = " << j 
//              <<  " inverse_sqrt_covar = " 
//              << (*inverse_sqrt_covar_ptr)[i][j] << endl;
      }
   }
}

// ---------------------------------------------------------------------
// Member function import_dictionary() imports DxK dictionary elements
// from input binary file dictionary.hdf5 into Dictionary.

void text_detector::import_dictionary()
{
//   cout << "inside text_detector::import_dictionary()" << endl;
   string dictionary_hdf5_filename=dictionary_subdir+"dictionary.hdf5";

   cout << "Importing dictionary from  = " << dictionary_hdf5_filename << endl;
   flann::load_from_file(
      Dictionary,dictionary_hdf5_filename.c_str(),"dictionary_descriptors");

   K = Dictionary.cols;
   D = Dictionary.rows;

   if (RGB_pixels_flag)
   {
      sqrt_D=sqrt(double(D/3.0));
   }
   else
   {
      sqrt_D=sqrt(double(D));
   }

   cout << "D = " << D << endl;
   cout << "K = " << K << endl;
   cout << "sqrt_D = " << sqrt_D << endl;
}

// ---------------------------------------------------------------------
// Member function compute_constant_matrices() fills
// *Dtrans_inverse_sqrt_covar_ptr = dictionary^T * inverse_sqrt_covar

void text_detector::compute_Dtrans_inverse_sqrt_covar_matrix()
{
//   cout << "inside text_detector::compute_Dtrans_inverse_sqrt_covar_matrix()" << endl;

   for (unsigned int k=0; k<K; k++)
   {
//      cout << "k = " << k << endl;
      for (unsigned int d=0; d<D; d++)   
      {
//         cout << "d = " << d << endl;
         (*Dtrans_inverse_sqrt_covar_ptr)[k][d]=0;
         for (unsigned int i=0; i<D; i++)
         {
//            cout << "i = " << i << endl;

// Note:  Dtrans[k][i] = D[i][k]

            (*Dtrans_inverse_sqrt_covar_ptr)[k][d] += 
               Dictionary[i][k] * (*inverse_sqrt_covar_ptr)[i][d];
         } // loop over index i

//         cout << "k = " << k << " d = " << d
//              <<  " Dtrans_inv_sqrt_covar = " 
//              << (*Dtrans_inverse_sqrt_covar_ptr)[k][d] << endl;
         
      } // loop over index d
   } // loop over index k 
}

// ---------------------------------------------------------------------
// Member function whiten_patch() takes in a D-dimensional array
// which is assumed to hold grey-scale intensity values (ranging from
// 0 to 1) corresponding to some 8x8 pixel patch within an image.
// This method subtracts off a mean D-dim vector from the input array
// and then multiplies by an inverse square root covariance matrix.
// The whitened output vector is returned as an array of floats.

float* text_detector::whiten_patch(float* curr_patch_descriptor)
{
//   cout << "inside text_detector::whiten_patch()" << endl;
   for (unsigned int i=0; i<D; i++)
   {
      whitened_descriptor[i]=0;

      for (unsigned int j=0; j<D; j++)
      {
         whitened_descriptor[i] += (*inverse_sqrt_covar_ptr)[i][j]*
            (curr_patch_descriptor[j]-(*mean_coeffs_ptr)[j][0]);
//         cout << "i = " << i << "j = " << j 
//              << " curr_patch_descrip = " << curr_patch_descriptor[j]
//              << endl;
      }

//      cout << "i = " << i 
//           << " whitened descriptor = " << whitened_descriptor[i] << endl;
   }
   
   return whitened_descriptor;
}

// ==========================================================================
// Window features maps member functions
// ==========================================================================

void text_detector::initialize_avg_window_features_vector()
{
   for (unsigned int i=0; i<avg_window_width; i++)
   {
      for (unsigned int j=0; j<avg_window_height; j++)
      {
         avg_window_features_vector.push_back(new float[K]);
      } // loop over index j 
   } // loop over index i
}

// ---------------------------------------------------------------------
// Member function clear_avg_window_features_vector() 

void text_detector::clear_avg_window_features_vector()
{
   for (unsigned int i=0; i<avg_window_features_vector.size(); i++)
   {
      memset(avg_window_features_vector[i],0,K*sizeof(float));
   }
}

// ---------------------------------------------------------------------
// Member function destroy_avg_window_features_vector() 

void text_detector::destroy_avg_window_features_vector()
{
   for (unsigned int i=0; i<avg_window_features_vector.size(); i++)
   {
      delete [] avg_window_features_vector[i];
   }
}

// ==========================================================================
// Feature patch member functions
// ==========================================================================

// Member function import_image_from_file() reads in the input image.

bool text_detector::import_image_from_file(string image_filename)
{
//   cout << "inside text_detector::import_image_from_file()" << endl;
//   cout << "image_filename = " << image_filename << endl;
   bool flag=texture_rectangle_ptr->import_photo_from_file(image_filename);
   if (!flag) 
   {
      cout << "Failed to import " << image_filename << endl;
      outputfunc::enter_continue_char();
   }
   return flag;
}

// ---------------------------------------------------------------------
// Member function average_window_features() takes in pixel integer
// offsets px,py.  It first computes K-dimensional descriptors for
// each 8x8 patch within the ~32x32 window in the current
// *texture_rectangle_ptr.  This method next partitions
// *texture_rectangle_ptr into 3x3 sectors which are not generally of
// precisely the same pixel size.  For each of the 9 sectors, the
// K-dimensional descriptors are averaged together and stored in
// *average_features_map_ptr.  If the averaged window features are not
// successfully calculated in all 3x3 sectors, this boolean method
// returns false.


bool text_detector::average_window_features(unsigned int px,unsigned int py)
{
//   cout << "inside text_detector::average_window_features()" << endl;

   clear_avg_window_features_vector();

   vector<threevector> height_sector_start_stop=
      compute_sector_start_stop(window_height);
   int npy=height_sector_start_stop.size();
//   cout << "npy = " << npy << endl;
   if (npy==0) return false;

   vector<threevector> width_sector_start_stop=
      compute_sector_start_stop(window_width);
   int npx=width_sector_start_stop.size();
//   cout << "npx = " << npx << endl;
   if (npx==0) return false;
   double denom=npx*npy;

   for (int H=0; H<npy; H++)
   {
      int j=height_sector_start_stop[H].get(0);
      int dpy=height_sector_start_stop[H].get(1);
      unsigned int pv=py+dpy;
//      cout << "j = " << j << " dpy = " << dpy << endl;
      
      for (int W=0; W<npx; W++)
      {
         int i=width_sector_start_stop[W].get(0);
         int dpx=width_sector_start_stop[W].get(1);
         unsigned int pu=px+dpx;
//         cout << "i = " << i << " dpx = " << dpx << endl;

         int avg_ID=get_avg_pixel_ID(i,j);
         float* curr_avg_patch_histogram=avg_window_features_vector[avg_ID];

         prepare_patch(pu,pv);

         for (unsigned int k=0; k<K; k++)
         {
            float curr_val=encode_patch_values_into_feature(k)/denom;
            curr_avg_patch_histogram[k] += curr_val;
         } // loop over index k 

      } // loop over index W labeling width partitionings
   } // loop over index H labeling height partitionings

   return true;
}

float text_detector::compute_recog_prob(unsigned int px,unsigned int py,
                                        const vector<float>& weights)
{
//   cout << "inside text_detector::average_window_features()" << endl;

   clear_avg_window_features_vector();

   vector<threevector> height_sector_start_stop=
      compute_sector_start_stop(window_height);
   int npy=height_sector_start_stop.size();
//   cout << "npy = " << npy << endl;
   if (npy==0) return false;

   vector<threevector> width_sector_start_stop=
      compute_sector_start_stop(window_width);
   int npx=width_sector_start_stop.size();
//   cout << "npx = " << npx << endl;
   if (npx==0) return false;
   double denom=npx*npy;

   double dotproduct=0;

   for (int H=0; H<npy; H++)
   {
      int j=height_sector_start_stop[H].get(0);
      int dpy=height_sector_start_stop[H].get(1);
      unsigned int pv=py+dpy;
//      cout << "j = " << j << " dpy = " << dpy << endl;
      
      for (int W=0; W<npx; W++)
      {
         int i=width_sector_start_stop[W].get(0);
         int dpx=width_sector_start_stop[W].get(1);
         unsigned int pu=px+dpx;
//         cout << "i = " << i << " dpx = " << dpx << endl;

         int avg_ID=get_avg_pixel_ID(i,j);
//          float* curr_avg_patch_histogram=avg_window_features_vector[avg_ID];

         prepare_patch(pu,pv);

         for (unsigned int k=0; k<K; k++)
         {
            float curr_val=encode_patch_values_into_feature(k)/denom;
            dotproduct += weights[avg_ID*K+k]*curr_val;
         } // loop over index k 

      } // loop over index W labeling width partitionings
   } // loop over index H labeling height partitionings

   return dotproduct;
}

// ---------------------------------------------------------------------
// Member function compute_patch_feature() takes in pixel
// coordinates (px,py) for the upper left corner of a ~32x~32 pixel
// window.  If the entire window doesn't lie inside
// *texture_rectangle_ptr, this boolean method returns false.
// Otherwise, it overwrites each of ~25x~25 K-dimensional float arrays
// within window_features_vector with newly computed patch histograms.

void text_detector::prepare_patch(unsigned int pu,unsigned int pv)
{
   if (RGB_pixels_flag)
   {
      fill_RGB_patch_descriptor(pu,pv);
   }
   else
   {
      fill_greyscale_patch_descriptor(pu,pv);
   }
   contrast_normalize_patch_descriptor();
}

// ---------------------------------------------------------------------
void text_detector::fill_RGB_patch_descriptor(
   unsigned int px,unsigned int py)
{
   int counter=0;
   int R[sqrt_D],G[sqrt_D],B[sqrt_D];

   for (unsigned int pv=py; pv<py+sqrt_D; pv++)
   {
      texture_rectangle_ptr->get_pixel_row_RGB_values(px,pv,sqrt_D,R,G,B);
      for (unsigned int i=0; i<sqrt_D; i++)
      {
         patch_descriptor[counter]=R[i]/255.0;
         patch_descriptor[counter+1]=G[i]/255.0;
         patch_descriptor[counter+2]=B[i]/255.0;
         counter += 3;
      }
   } // loop over pv
}

// ---------------------------------------------------------------------
void text_detector::fill_greyscale_patch_descriptor(
   unsigned int px,unsigned int py)
{
   int counter=0;
   int R[sqrt_D],G[sqrt_D],B[sqrt_D];

   for (unsigned int pv=py; pv<py+sqrt_D; pv++)
   {
      texture_rectangle_ptr->get_pixel_row_RGB_values(px,pv,sqrt_D,R,G,B);
      for (unsigned int i=0; i<sqrt_D; i++)
      {
         double normalized_luminosity=
            colorfunc::RGB_to_luminosity(R[i],G[i],B[i])/255.0;
         patch_descriptor[counter]=normalized_luminosity;
         counter++;
      }
   } // loop over pv
}

// ---------------------------------------------------------------------
// Member function contrast_normalize_patch_descriptor() computes and
// removes the mean from each of the descriptor's d_dims components.
// It also divides the translated component values by (approximately)
// their standard deviation.  Coates suggests setting eps_norm = 10 if
// the descriptor's raw intensity values range over [0,255].
// Equivalently, eps_norm = 10/(255)**2 = 1.537E-4 if the raw
// descriptor components range over [0,1].

// See "Learning feature representations with K-means" by Coates and
// Ng, 2012.

void text_detector::contrast_normalize_patch_descriptor()
{
   float mean=0;
   float mean_sqr=0;
   for (unsigned int d=0; d<D; d++)
   {
      mean += patch_descriptor[d];
      mean_sqr += sqr(patch_descriptor[d]);
   }
   mean /= D;
   mean_sqr /= D;
   float variance = mean_sqr - sqr(mean);

   const float eps_norm = 10.0/sqr(255.0);
   float denom=sqrt(variance+eps_norm);
   for (unsigned int d=0; d<D; d++)
   {
      float numer=patch_descriptor[d]-mean;
      patch_descriptor[d] = numer/denom;
   }
}

// ---------------------------------------------------------------------
// Member function encode_patch_values_into_feature() implements the
// "soft" non-linear mapping of sqrt_D x sqrt_D image patches onto
// K-dimensional histograms described at the end of section III.A in
// "Text Detection and Character Recognition in Scene Images with
// Unsupervised Feature Learning" by Coates et al.  This form of
// encoding is described more clearly (and we believe
// correctly) in eqn (3) of "An Analysis of Single-Layer Networks in
// Unsupervised Feature Learning" by Coates et al.  The encoded
// feature descriptor corresponding to the input patch is
// stored within member float curr_feature_ptr.
// window_features_vector.

float text_detector::encode_patch_values_into_feature(unsigned int k)
{
   const double alpha = 0.5;

   float component_value=0;
   for (unsigned int d=0; d<D; d++)
   {
      component_value += (*Dtrans_inverse_sqrt_covar_ptr)[k][d] * 
         patch_descriptor[d];
   } // loop over index d labeling rows in Dictionary

// Following Coates, we use a simple but nonlinear "encoder"
// function to transform contrast-normalized & whitened patch
// descriptors into K-dimensional feature vectors.

   float curr_val=fabs(component_value)-alpha;
   if (curr_val < 0) curr_val=0;
   return curr_val;
}

// ---------------------------------------------------------------------
void text_detector::encode_patch_values_into_feature()
{
//   cout << "inside text_detector::encode_patch_values_into_feature()" << endl;

   const double alpha = 0.5;
   for (unsigned int k=0; k<K; k++)
   {
      float component_value=0;
      for (unsigned int d=0; d<D; d++)
      {
         component_value += (*Dtrans_inverse_sqrt_covar_ptr)[k][d] * 
            patch_descriptor[d];
//         cout << "d = " << d << " component_val = " << component_value << endl;
      } // loop over index d labeling rows in Dictionary

// Following Coates, we use a simple but nonlinear "encoder"
// function to transform contrast-normalized & whitened patch
// descriptors into K-dimensional feature vectors.

      float curr_val=fabs(component_value)-alpha;
      if (curr_val < 0) curr_val=0;

// Note added on 6/26/14 at 10:26 am... This assignment cmd is slow!

// FAKE FAKE:  

//      curr_feature_ptr[k]=curr_val;

   } // loop over index k labeling columns in Dictionary
}

// ---------------------------------------------------------------------
// Member function compute_sector_start_stop() is a specialized method
// which takes in width w measured in pixels.  It returns a set of
// threevectors that partition the width into 3 sectors labeled by
// first component integers 0, 1 and 2.  The starting and stopping
// pixel numbers for each partition are contained in the second and
// third components of the threevectors.

vector<threevector> text_detector::compute_sector_start_stop(int w) const
{
//   cout << "inside text_detector::compute_sector_start_stop(), w = " 
//        << w << endl;
   
   vector<threevector> sector_start_stop;

   if (w==8)
   {
      sector_start_stop.push_back(threevector(0,0,7));
      sector_start_stop.push_back(threevector(1,0,7));
      sector_start_stop.push_back(threevector(2,0,7));
   }
   else if (w==9)
   {
      sector_start_stop.push_back(threevector(0,0,7));
      sector_start_stop.push_back(threevector(1,1,8));
      sector_start_stop.push_back(threevector(2,1,8));
   }
   else if (w==10)
   {
      sector_start_stop.push_back(threevector(0,0,7));
      sector_start_stop.push_back(threevector(1,1,8));
      sector_start_stop.push_back(threevector(2,2,9));
   }
   else if (w > 10)
   {
      int delta_x=basic_math::round((w-8)/3.0);
      for (int dpx=0; dpx<delta_x; dpx++)
      {
         sector_start_stop.push_back(threevector(0,0+dpx,7+dpx));
      }
      for (int dpx=delta_x; dpx<=2*delta_x; dpx++)
      {
         sector_start_stop.push_back(threevector(1,0+dpx,7+dpx));
      }
      for (int dpx=2*delta_x+1; dpx<w-8+1; dpx++)
      {
         sector_start_stop.push_back(threevector(2,0+dpx,7+dpx));
      }
   }

   return sector_start_stop;
}

// ---------------------------------------------------------------------
// Member function get_nineK_window_descriptor() retrieves the K-dimensional
// averaged histograms for each of the avg_window_width *
// avg_window_height window sectors.  It
// concatenates them together into a single
// avg_window_width*avg_window_height*K column vector and
// returns the results within float array window_histogram.

float* text_detector::get_nineK_window_descriptor()
{
   int counter=0;
   for (unsigned int i=0; i<avg_window_width; i++)
   {
      for (unsigned int j=0; j<avg_window_height; j++)
      {
         int avg_ID=get_avg_pixel_ID(i,j);
         float* curr_histogram=avg_window_features_vector[avg_ID];
         
         for (unsigned int k=0; k<K; k++)
         {
            window_histogram[counter]=curr_histogram[k];
            if (fabs(window_histogram[counter]) > POSITIVEINFINITY)
            {
               cout << "inside text_detector::get_nineK_window_descriptor()"
                    << endl;
               cout << "i = " << i << " j = " << j << endl;
               cout << "k = " << k << " counter = " << counter
                    << " window_histogram = " << window_histogram[counter]
                    << endl;
               outputfunc::enter_continue_char();
            }
            counter++;
         }
      } // loop over index j 
   } // loop over index i

   return window_histogram;
}
