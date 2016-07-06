// ==========================================================================
// Program FIT_IFSAR_MODEL
// ==========================================================================
// Last updated on 2/6/05; 1/29/12
// ==========================================================================

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "math/constants.h"
#include "threeDgraphics/draw3Dfuncs.h"
#include "general/filefuncs.h"
#include "space/spasefuncs.h"
#include "general/sysfuncs.h"
#include "threeDgraphics/xyzpfuncs.h"

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::flush;
   using std::ios;
   using std::ostream;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// ==========================================================================

// Generate and write SPASE model to same output XYZP file:

   bool draw_thick_lines=false;
   vector<fourvector>* xyzp_pnt_ptr=spasefunc::construct_SPASE_model(
      1,1,1,1,
      draw_thick_lines);

// Rotation matrix derived by Hyrum on 2/4/05 needed to bring model
// into alignment IFSAR image generated by Greg Ushomirsky on 2/4/05:

   rotation R;
   R.put(0,0,0.49785);
   R.put(0,1,0.29902);
   R.put(0,2,-0.81408);

   R.put(1,0,0.86579);
   R.put(1,1,-0.1166);
   R.put(1,2,0.48664);

   R.put(2,0,0.050593);
   R.put(2,1,-0.9471);
   R.put(2,2,-0.31694);
   R=R.transpose();

   xyzpfunc::rotate(R,xyzp_pnt_ptr);
//   xyzpfunc::scale(201.37,xyzp_pnt_ptr);
//   xyzpfunc::translate(threevector(179.5,297,-60.5),xyzp_pnt_ptr);

   rotation R_x(PI/2,0,0);
   xyzpfunc::rotate(R_x,xyzp_pnt_ptr);

   threevector COM=xyzpfunc::center_of_mass(xyzp_pnt_ptr);
   cout << "COM = " << COM << endl;

// Write all XYZP points to output_dir subdirectory:

   string output_subdir="./xyzp_files/";
   filefunc::dircreate(output_subdir);
   
   bool gzip_output_file=false;
   string xyzp_filename=output_subdir+"new_ifsar_model.xyzp";
   filefunc::deletefile(xyzp_filename);
   xyzpfunc::write_xyzp_data(xyzp_filename,xyzp_pnt_ptr,gzip_output_file);
   delete xyzp_pnt_ptr;

// Add 3D axes to output XYZP files:

//   spasefunc::draw_model_3D_axes(xyzp_filename,"X","Y","Z");

// Add fake color map points into final XYZP file:

   draw3Dfunc::append_fake_xyzp_points_for_dataviewer_coloring(
      xyzp_filename,COM);
   filefunc::gunzip_file_if_gzipped(xyzp_filename);
}
