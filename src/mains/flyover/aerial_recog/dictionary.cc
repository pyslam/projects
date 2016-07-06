// ==========================================================================
// DICTIONARY imports whitened 192x1 RGB [64x1 greyscale ] descriptors
// generated by program WHITEN_DESCRIPTORS for 8x8 pixel patches
// randomly selected from positive class 32x32 image chips.  It
// initially fills dictionary matrix D with column vectors populated
// via a Normal distribution and normalized to unit magnitude.  It
// then iteratively updates score matrix S and dictionary D following
// the prescription laid out in "Learning feature representations with
// K-means" by Coates and Ng, 2013.  After each iteration, a PNG
// fileis generated which illustrates all K 8x8 dictionary elements.
// The final Kbest values in D are exported to a binary HDF5 file.

// 				dictionary

// ==========================================================================
// Last updated on 5/26/14; 6/13/14; 6/21/14; 6/22/14
// ==========================================================================

#include <iostream>
#include <string>
#include <vector>
#include <flann/flann.hpp>
#include <flann/io/hdf5.h>

#include "general/filefuncs.h"
#include "math/genmatrix.h"
#include "math/genvector.h"
#include "numrec/nrfuncs.h"
#include "general/outputfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "video/texture_rectangle.h"
#include "time/timefuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

double **mat_alloc(int n, int m)
{
  int i, j;
  double **ar;

  ar = (double **)malloc(((n + 1) & ~1) * sizeof(double *) + n * m * sizeof(double));
  ar[0] = (double *)(ar + ((n + 1) & ~1));

  for(i = 0; i < n; i++){
    ar[i] = ar[0] + i * m;
    for(j = 0; j < m; j++){
      ar[i][j] = 0.0;
    }
  }
  return(ar);
}

void mat_free(double **a1)
{
  free(a1);
}

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);

   const unsigned int D=64*3;	// 8x8 RGB patches
//   const unsigned int D=64;	// 8x8 greyscale patches

   string trees_subdir="./trees/";
   string dictionary_subdir=trees_subdir+"dictionary/";
   string dictionary_pngs_subdir=dictionary_subdir+"dictionary_pngs/";
   filefunc::dircreate(dictionary_pngs_subdir);

   unsigned int K=2000;
//   unsigned int K=3000;
   unsigned int Kbest=K / 2;

// Import whitened patch descriptors for all character images:

   flann::Matrix<float> patch_descriptors;
   string patches_hdf5_filename=dictionary_subdir+
      "whitened_patch_descriptors.hdf5";
   flann::load_from_file(
      patch_descriptors,patches_hdf5_filename.c_str(),
      "whitened_patch_descriptors");

   unsigned int N=patch_descriptors.rows;
   cout << "Number of patch descriptors N = " << N << endl;

// Dell laptop RAM is so limited that we must restrict N:

   N=basic_math::min(N,(unsigned int) 500000);	// Dell & MacPro
   cout << "Reduced N = " << N << endl;

   double** Dmat=mat_alloc(D,K);
   double** Xmat=mat_alloc(D,N);
   double** Smat=mat_alloc(K,N);

// Fill matrix Xmat with whitened descriptors in its columns:
   
   for (unsigned int n=0; n<N; n++)
   {
      for (unsigned int d=0; d<D; d++)
      {
         Xmat[d][n]=patch_descriptors[n][d];
      } // loop over index d
   } // loop over index n labeling rows in patch_descriptors array
   delete [] patch_descriptors.ptr();

// Initialize column entries within dictionary D from a normal
// distribution.  Then normalize each column of D to have unit length:

   genvector curr_Dcol(D);
   for (unsigned int k=0; k<K; k++)
   {
      for (unsigned int d=0; d<D; d++)
      {
         curr_Dcol.put(d,nrfunc::gasdev());
      }
      
      curr_Dcol=curr_Dcol.unitvector();

      for (unsigned int d=0; d<D; d++)
      {
         Dmat[d][k]=curr_Dcol.get(d);
      }
   } // loop over index k 
   
   int n_patches_per_row = 30;
   int total_width=n_patches_per_row * 9;
   int total_height=K/(n_patches_per_row) * 9;
   texture_rectangle* texture_rectangle_ptr=new texture_rectangle(
      total_width,total_height,1,3,NULL);

   string blank_filename="blank.jpg";
   texture_rectangle_ptr->generate_blank_image_file(
      total_width,total_height,blank_filename,0.5);
   texture_rectangle_ptr->import_photo_from_file(blank_filename);

//   unsigned int n_iters=10;
//   unsigned int n_iters=12;
   unsigned int n_iters=15;
//   cout << "Enter number of dictionary forming iterations ( < 25):" << endl;
//   cin >> n_iters;

   vector<unsigned int> k_index;
   vector<double> k_histogram;
   
   timefunc::initialize_timeofday_clock();
   for (unsigned int iter=0; iter<n_iters; iter++)
   {
      string banner="Processing iteration "+stringfunc::number_to_string(iter)+
         " of "+stringfunc::number_to_string(n_iters)+":";
      outputfunc::write_banner(banner);

// Reset k_histogram at start of each iteration:

      k_index.clear();
      k_histogram.clear();
      for (unsigned int k=0; k<K; k++)
      {
         k_index.push_back(k);
         k_histogram.push_back(0);
      }

// Update S matrix:

      for (unsigned int n=0; n<N; n++)
      {
         int best_k=-1;
         double max_dotproduct=NEGATIVEINFINITY;
         for (unsigned int k=0; k<K; k++)
         {
            double dotproduct=0;
            for (unsigned int d=0; d<D; d++)
            {
               dotproduct += Dmat[d][k] * Xmat[d][n];
            }

            if (dotproduct > max_dotproduct)
            {
               max_dotproduct=dotproduct;
               best_k=k;
            }

            Smat[k][n]=0;
         } // loop over index k labeling clusters

         Smat[best_k][n] = max_dotproduct;
         k_histogram[best_k] = k_histogram[best_k]+1;
      } // loop over index n labeling 8x8 pixel patches

// Update D matrix:

      for (unsigned int d=0; d<D; d++)
      {
         for (unsigned int k=0; k<K; k++)
         {
            for (unsigned int n=0; n<N; n++)
            {
               Dmat[d][k] += Xmat[d][n] * Smat[k][n];
            } // loop over index N
         } // loop over index k
      } // loop over index d
      
// Reset norm of each column within D to unity:

      for (unsigned int k=0; k<K; k++)
      {
         for (unsigned int d=0; d<D; d++)
         {
            curr_Dcol.put(d,Dmat[d][k]);
         }
         curr_Dcol=curr_Dcol.unitvector();
         for (unsigned int d=0; d<D; d++)
         {
            Dmat[d][k]=curr_Dcol.get(d);
         }
      } // loop over index k labeling dictionary columns

// Sort k_histogram and then print its current values:

      templatefunc::Quicksort_descending(k_histogram, k_index);
   
      unsigned int k_histogram_integral=0;
      unsigned int n_kcols = 20;
      unsigned int n_krows = K / n_kcols;
      unsigned int k=0;
      cout << "Sorted k_histogram values:" << endl;
      for (unsigned int r=0; r<n_krows; r++)
      {
         for (unsigned int c=0; c<n_kcols; c++)
         {
            k_histogram_integral += k_histogram[k];
            cout << k_histogram[k++] << " ";
         }
         cout << endl;
      }
      cout << "k_histogram_integral = " << k_histogram_integral
           << " N = " << N << endl;

      double median, quartile_width;
      mathfunc::median_value_and_quartile_width(
         k_histogram, median, quartile_width);
      
      cout << "k_histogram median = " << median 
           << " quartile_width = " << quartile_width << endl;

// Export PNG image containing current 64-dimensional dictionary
// elements visualized as 8x8 patches:

      texture_rectangle_ptr->clear_all_RGB_values();
      for (unsigned int k=0; k<K; k++)
      {
         unsigned int sorted_k=k_index[k];

         for (unsigned int d=0; d<D; d++)
         {
            curr_Dcol.put(d,Dmat[d][sorted_k]);
         }

// For visualization purposes only, compute min and max intensity
// values. Then rescale intensities so that extremal values equal +/- 1:

         double min_z=POSITIVEINFINITY;
         double max_z=NEGATIVEINFINITY;
         for (unsigned int d=0; d<D; d++)
         {
            double curr_z=curr_Dcol.get(d);
            min_z=basic_math::min(min_z,curr_z);
            max_z=basic_math::max(max_z,curr_z);
         }
         
         for (unsigned int d=0; d<D; d++)
         {
            double curr_z=curr_Dcol.get(d);
            curr_z=2*(curr_z-min_z)/(max_z-min_z)-1;
            curr_Dcol.put(d,curr_z);
         }

         int row=k/n_patches_per_row;
         int column=k%n_patches_per_row;

         int px_start=column*9;
         int py_start=row*9;
//         cout << "row = " << row << " column = " << column 
//              << " px_start = " << px_start 
//              << " py_start = " << py_start << endl;

         int counter=0;
         for (unsigned int pu=0; pu<8; pu++)
         {
            int px=px_start+pu;
            for (unsigned int pv=0; pv<8; pv++)
            {
               int py=py_start+pv;
               double z=curr_Dcol.get(counter++);
               int R=0.5*(z+1)*255;
               z=curr_Dcol.get(counter++);
               int G=0.5*(z+1)*255;
               z=curr_Dcol.get(counter++);
               int B=0.5*(z+1)*255;
               texture_rectangle_ptr->set_pixel_RGB_values(px,py,R,G,B);

//               texture_rectangle_ptr->set_pixel_RGB_values(px,py,R,R,R);
            } // loop over pv index
         } // loop over pu index

      } // loop over index k labeling dictionary descriptors

// Export current dictionary to PNG image file:

      string output_filename=dictionary_pngs_subdir+
         "dictionary_"+stringfunc::number_to_string(iter)+".png";
      texture_rectangle_ptr->write_curr_frame(output_filename);
      cout << "Exported "+output_filename << endl;

      double elapsed_time=timefunc::elapsed_timeofday_time();
      cout << "Elapsed time = " << elapsed_time << " secs = " 
           << elapsed_time / 60.0 << " minutes" << endl;

   } // loop over iter index

// Export "best" dictionary elements to binary HDF5 file and ignore
// "worst" dictionary elements:

   flann::Matrix<float>* dictionary_descriptors_ptr=
      new flann::Matrix<float>(new float[D*Kbest],D,Kbest);
   for (unsigned int k=0; k<Kbest; k++)
   {
      unsigned int sorted_k=k_index[k];
      for (unsigned int d=0; d<D; d++)
      {
         (*dictionary_descriptors_ptr)[d][k]=Dmat[d][sorted_k];
      }
   }

   string dictionary_hdf5_filename=dictionary_subdir+"dictionary.hdf5";
   filefunc::deletefile(dictionary_hdf5_filename);
   flann::save_to_file(
      *dictionary_descriptors_ptr,dictionary_hdf5_filename,
      "dictionary_descriptors");

   string banner="Wrote Kbest = "+stringfunc::number_to_string(Kbest)
      +" of K = "+stringfunc::number_to_string(K)
      +" calculated dictionary elements to "+dictionary_hdf5_filename;
   outputfunc::write_big_banner(banner);

   mat_free(Dmat);
   mat_free(Xmat);
   mat_free(Smat);
   delete texture_rectangle_ptr;

   delete [] dictionary_descriptors_ptr->ptr();
   delete dictionary_descriptors_ptr;
}

   


