// ==========================================================================
// Program PARSE_CLUSTER_CENTROIDS imports a binary file generated by
// Wei Dong's out-of-core kmeans clustering program.  It extracts the
// number of clusters K and identifies zero-valued centroid vectors.
// ==========================================================================
// Last updated on 8/25/13; 8/26/13; 8/30/13
// ==========================================================================

#include <iostream>
#include <math.h>
#include <string>
#include <vector>
#include "math/constants.h"
#include "general/filefuncs.h"
#include "general/outputfuncs.h"
#include "numrec/nrfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "math/genvector.h"
#include "math/threevector.h"

using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);
   cout.precision(15);

   int d_dims=128;

   string sift_keys_subdir="/data/sift_keyfiles/";
//   string clusters_filename=sift_keys_subdir+"clusters_10M_descriptors.bin";
   string clusters_filename=sift_keys_subdir+"clusters_32M_descriptors.bin";
   long long bytecount=filefunc::size_of_file_in_bytes(clusters_filename);
   cout << "bytecount = " << bytecount << endl;
   
   int K=bytecount/(sizeof(float)*d_dims);
   cout << "K = " << K << endl;
   outputfunc::enter_continue_char();

   ifstream instream;
   filefunc::open_binaryfile(clusters_filename,instream);

   int n_zero_descriptors=0;
   float curr_val;
   genvector centroid(d_dims);
   for (int k=0; k<K; k++)
   {
      bool zero_valued_descriptor_flag=true;
      for (int d=0; d<d_dims; d++)
      {
         filefunc::readobject(instream,curr_val);
         if (!nearly_equal(curr_val,0,1E-8)) zero_valued_descriptor_flag=false;
         centroid.put(d,curr_val);
      }
//      cout << "k = " << k << " centroid = " << centroid << endl;
      if (zero_valued_descriptor_flag) n_zero_descriptors++;
   }
   filefunc::closefile(clusters_filename,instream);

   double zero_descriptors_frac=double(n_zero_descriptors)/K;
   cout << "zero_descriptors_frac = " << zero_descriptors_frac << endl;
}

