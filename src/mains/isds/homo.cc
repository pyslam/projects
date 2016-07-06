// ==========================================================================
// Program HOMO reads in consolidated XY and UV feature information
// generated by program CONSOLIDATE.  It computes the homography which
// maps features in UV coordinates onto their XY values.  Entering
// into an infinite loop, it transforms user-entered XY coordinates to
// their UV imageplane counterparts.
// ==========================================================================
// Last updated on 6/19/06; 1/15/08
// ==========================================================================

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "general/filefuncs.h"
#include "geometry/homography.h"
#include "astro_geo/latlong2utmfuncs.h"
#include "general/outputfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "math/twovector.h"

using std::cin;
using std::cout;
using std::endl;
using std::ios;
using std::ofstream;
using std::ostream;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);
   const int PRECISION=12;
   cout.precision(PRECISION);

   string features_dir=
      "/home/cho/programs/c++/svn/projects/src/mains/sift/images/aerial_MIT/GC/";
//      "/home/cho/programs/c++/svn/projects/src/mains/sift/images/aerial_MIT/health_center/";
//      "/home/cho/programs/c++/svn/projects/src/mains/sift/images/aerial_MIT/dome/";
   string filename=features_dir+"features_consolidated.txt";
   filefunc::ReadInfile(filename);
   
   vector<twovector> XY,UV;
   for (unsigned int i=0; i<filefunc::text_line.size(); i++)
   {
      vector<string> substring=stringfunc::decompose_string_into_substrings(
         filefunc::text_line[i]);
      double X=stringfunc::string_to_number(substring[0]);
      double Y=stringfunc::string_to_number(substring[1]);
      double U=stringfunc::string_to_number(substring[2]);
      double V=stringfunc::string_to_number(substring[3]);
      XY.push_back(twovector(X,Y));
      UV.push_back(twovector(U,V));
      cout << "i = " << i 
           << " X = " << XY.back().get(0) 
           << " Y = " << XY.back().get(1) 
           << " U = " << UV.back().get(0) 
           << " V = " << UV.back().get(1)
           << endl;
   }


   homography H;
   H.parse_homography_inputs(XY,UV);
   H.compute_homography_matrix();
   double RMS_residual=H.check_homography_matrix(XY,UV);
   cout << "RMS_residual = " << RMS_residual << endl;
   cout << "H = " << H << endl;

/*
// Write out homography matrix elements in form useful form other C++
// programs such as mains/video/HOMOPROJECT:

   for (int r=0; r<3; r++)
   {
      for (int c=0; c<3; c++)
      {
         cout << "H_ptr->put(" 
              << r << " , " << c << " , "
              << H.get_H_ptr()->get(r,c) << " );" << endl;
      }
   }
*/
 
   while (true)
   {
      double X,Y,U,V;
      cout << "Enter X:" << endl;
      cin >> X;
      cout << "Enter Y:" << endl;
      cin >> Y;
      H.project_world_plane_to_image_plane(X,Y,U,V);
      cout << "U = " << U << " V = " << V << endl << endl;
   }
   
   


}
