// ==========================================================================
// DICTIONARY imports whitened 64x1 descriptors generated by program
// WHITEN_DESCRIPTORS for 8x8 pixel patches X^i randomly selected from
// text character images.  It initially assigns each patch to
// some random cluster labeled by 0 <= k < K. Score vectors S^i
// associated with each pixel patch are then computed which have
// precisely one non-zero entry.  We then iteratively solve for the
// 64xK dictionary matrix D which minimizes sum_i |D S^i - X^i|.
// Columns within D are subsequently renormalized to have unit
// magnitude.  The final values in D are exported to a binary HDF5
// file.

// After each iteration, a PNG file is generated which shows all K 8x8
// descriptors within the dictionary.

// 				dictionary


// ==========================================================================
// Last updated on 5/31/12; 6/2/12; 7/12/12; 8/23/12; 11/24/12
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

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);

   const int D=64;
   int K=1024;
   string dictionary_subdir="./training_data/dictionary/";
   string dictionary_pngs_subdir=dictionary_subdir+"dictionary_pngs/";
   filefunc::dircreate(dictionary_pngs_subdir);

// Import whitened patch descriptors for all character images:

   flann::Matrix<float> patch_descriptors;
   string patches_hdf5_filename=dictionary_subdir+
      "whitened_patch_features.hdf5";
   flann::load_from_file(
      patch_descriptors,patches_hdf5_filename.c_str(),"patch_descriptors");

   int N=patch_descriptors.rows;
   cout << "Number of patch descriptors N = " << N << endl;

// Dell laptop RAM is so limited that we must restrict N:

   N=basic_math::min(N,250000);
   cout << "N = " << N << endl;

   vector<genvector*>* S_ptrs_ptr=new vector<genvector*>;
   vector<genvector*>* X_ptrs_ptr=new vector<genvector*>;

   double sum_S_sqrd_mags=0;
   for (int n=0; n<N; n++)
   {
      if (n%1000==0) cout << n << " " << flush;
      genvector* curr_X_ptr=new genvector(D);         
      X_ptrs_ptr->push_back(curr_X_ptr);

      for (int d=0; d<D; d++)
      {
         curr_X_ptr->put(d,patch_descriptors[n][d]);
      } // loop over index d

//      cout << "n = " << n << endl;
//      cout << "X = " << *curr_X_ptr << endl;
      
// Initially assign each 8x8 pixel patch to some random dictionary
// element:

      int curr_k=K*nrfunc::ran1();
      genvector* curr_S_ptr=new genvector(K);
      S_ptrs_ptr->push_back(curr_S_ptr);

      curr_S_ptr->clear_values();
      curr_S_ptr->put(curr_k,1);

      sum_S_sqrd_mags += curr_S_ptr->sqrd_magnitude();
//      cout << "S = " << *curr_S_ptr << endl;
   } // loop over index n labeling rows in patch_descriptors array
//   cout << endl;

   genmatrix* D_ptr=new genmatrix(D,K);

   cout << "S_ptrs_ptr->size() = " << S_ptrs_ptr->size() << endl;
   cout << "X_ptrs_ptr->size() = " << X_ptrs_ptr->size() << endl;

   int total_width=16*9;
   int total_height=9*K/(total_width/9);
//   int total_height=8*9;
//   int total_height=16*9;
//   int total_height=32*9;

   texture_rectangle* texture_rectangle_ptr=new texture_rectangle(
      total_width,total_height,1,3,NULL);

   string blank_filename="blank.jpg";
   texture_rectangle_ptr->generate_blank_image_file(
      total_width,total_height,blank_filename,0.5);
   texture_rectangle_ptr->import_photo_from_file(blank_filename);

   int n_iters=10;
   cout << "Enter number of dictionary forming iterations ( < 25):" << endl;
   cin >> n_iters;

   timefunc::initialize_timeofday_clock();
   
   for (int iter=0; iter<n_iters; iter++)
   {
      string banner="Processing iteration "+stringfunc::number_to_string(iter)+
         " of "+stringfunc::number_to_string(n_iters)+":";
      outputfunc::write_banner(banner);

// Update *D_ptr:

      D_ptr->clear_values();
      for (int n=0; n<N; n++)
      {
         genvector* curr_S_ptr=S_ptrs_ptr->at(n);
         genvector* curr_X_ptr=X_ptrs_ptr->at(n);

//      cout << "n = " << n << " S = " << *curr_S_ptr << endl;
//      cout << "X = " << *curr_X_ptr << endl;
      
         *D_ptr += curr_X_ptr->outerproduct(*curr_S_ptr);
      } // loop over index n labeing 8x8 pixel patches
      *D_ptr /= sum_S_sqrd_mags;

// Reset norm of each column within D to one:

      genvector curr_patch(D);
      for (int k=0; k<K; k++)
      {
         D_ptr->get_column(k,curr_patch);
         curr_patch=curr_patch.unitvector();
         D_ptr->put_column(k,curr_patch);
      } // loop over index k labeling clusters
//      cout << "*D_ptr = " << *D_ptr << endl;

// Update texture rectangle output:

      texture_rectangle_ptr->clear_all_RGB_values();
      for (int k=0; k<K; k++)
      {
         D_ptr->get_column(k,curr_patch);

// For visualization purposes only, compute min and max intensity
// values. Then rescale intensities so that extremal values equal +/- 1:

         double min_z=POSITIVEINFINITY;
         double max_z=NEGATIVEINFINITY;
         for (int d=0; d<D; d++)
         {
            double curr_z=curr_patch.get(d);
            min_z=basic_math::min(min_z,curr_z);
            max_z=basic_math::max(max_z,curr_z);
         }
         
         for (int d=0; d<D; d++)
         {
            double curr_z=curr_patch.get(d);
            curr_z=2*(curr_z-min_z)/(max_z-min_z)-1;
            curr_patch.put(d,curr_z);
         }

         int row=k/16;
         int column=k%16;

         int px_start=column*9;
         int py_start=row*9;
//         cout << "row = " << row << " column = " << column 
//              << " px_start = " << px_start 
//              << " py_start = " << py_start << endl;

         int counter=0;
         for (int pu=0; pu<8; pu++)
         {
            int px=px_start+pu;
            for (int pv=0; pv<8; pv++)
            {
               int py=py_start+pv;
               double z=curr_patch.get(counter);
               counter++;
               int R=0.5*(z+1)*255;
               texture_rectangle_ptr->set_pixel_RGB_values(px,py,R,R,R);
            } // loop over pv index
         } // loop over pu index

      } // loop over index k labeling dictionary descriptors

      string output_filename=dictionary_pngs_subdir+
         "dictionary_"+stringfunc::number_to_string(iter)+".png";
      texture_rectangle_ptr->write_curr_frame(output_filename);
      cout << "Exported "+output_filename << endl;

// Update S vectors:

      for (int n=0; n<N; n++)
      {
         genvector* curr_S_ptr=S_ptrs_ptr->at(n);
         genvector* curr_X_ptr=X_ptrs_ptr->at(n);

         int best_k=-1;
         double max_dotproduct=NEGATIVEINFINITY;
         for (int k=0; k<K; k++)
         {
            D_ptr->get_column(k,curr_patch);
            double dotproduct=curr_X_ptr->dot(curr_patch);
            if (dotproduct > max_dotproduct)
            {
               max_dotproduct=dotproduct;
               best_k=k;
            }
         } // loop over index k labeling clusters
         curr_S_ptr->clear_values();
         curr_S_ptr->put(best_k,max_dotproduct);
      } // loop over index n labeling 8x8 pixel patches

      double elapsed_time=timefunc::elapsed_timeofday_time();
      cout << "Elapsed time = " << elapsed_time << " secs = " 
           << elapsed_time / 60.0 << " minutes" << endl;

   } // loop over iter index
   delete texture_rectangle_ptr;

// Export dictionary descriptors to binary HDF5 file:

   flann::Matrix<float>* dictionary_descriptors_ptr=
      new flann::Matrix<float>(new float[D*K],D,K);
   for (int d=0; d<D; d++)
   {
      for (int k=0; k<K; k++)
      {
         (*dictionary_descriptors_ptr)[d][k]=D_ptr->get(d,k);
      }
   }

   string dictionary_hdf5_filename=dictionary_subdir+"dictionary.hdf5";
   flann::save_to_file(
      *dictionary_descriptors_ptr,dictionary_hdf5_filename,
      "dictionary_descriptors");

   delete dictionary_descriptors_ptr;

   string banner="Wrote K = "+stringfunc::number_to_string(K)
      +" dictionary descriptors to "+dictionary_hdf5_filename;
   outputfunc::write_big_banner(banner);


}

   
