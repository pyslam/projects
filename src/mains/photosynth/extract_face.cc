// ==========================================================================
// Program EXTRACT_FACE reads in file "face_bbox.dat" generated by
// Karl Ni's face finding algorithms.  It converts the bounding box's
// coordinates in Karl's pixel conventions to our UV image plane
// coordinates.  EXTRACT_FACE then outputs the face bounding box info
// to bundler_IO_subdir/face_bbox_uv.dat.

//  extract_face --region_filename ./bundler/individual_photo/packages/peter_inputs.pkg

// ==========================================================================
// Last updated on 5/27/10; 5/28/10
// ==========================================================================

#include <iostream>
#include <string>
#include <vector>
#include "geometry/bounding_box.h"
#include "general/filefuncs.h"
#include "passes/PassesGroup.h"
#include "video/photogroup.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"

using std::cin;
using std::cout;
using std::endl;
using std::ofstream;
using std::string;
using std::vector;

// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   std::set_new_handler(sysfunc::out_of_memory);
   cout.precision(15);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   PassesGroup passes_group(&arguments);
   
   string bundler_IO_subdir="./bundler/individual_photo/";
   string image_list_filename=bundler_IO_subdir+"image_list.dat";
   cout << "image_list_filename = " << image_list_filename << endl;
   string image_sizes_filename=bundler_IO_subdir+"image_sizes.dat";
   cout << "image_sizes_filename = " << image_sizes_filename << endl;
   string bundle_filename=bundler_IO_subdir+"bundle.out";
   cout << "bundle_filename = " << bundle_filename << endl;

// ------------------------------------------------------------------------
// Instantiate reconstructed photo:

   cout << "Instantiating photogroup:" << endl;
   photogroup* bundler_photogroup_ptr=new photogroup();

   int n_photos_to_reconstruct=-1;
   bundler_photogroup_ptr->reconstruct_bundler_cameras(
      bundler_IO_subdir,image_list_filename,image_sizes_filename,
      bundle_filename,n_photos_to_reconstruct);

   photogroup* photogroup_ptr=new photogroup(*bundler_photogroup_ptr);

   string packages_subdir=bundler_IO_subdir+"packages/";
   cout << "packages_subdir = " << packages_subdir << endl;
   filefunc::dircreate(packages_subdir);

// ==========================================================================
// First convert reconstructed camera from Noah's bundler coordinate
// system into georegistered coordinates.  Then instantiate and write
// out package file for new photo:

   photograph* photograph_ptr=photogroup_ptr->get_photograph_ptr(0);

   cout << "photo filename = " << photograph_ptr->get_filename() << endl;

   int xdim=photograph_ptr->get_xdim();
   int ydim=photograph_ptr->get_ydim();
   cout << "xdim = " << xdim << " ydim = " << ydim << endl;
 
   string face_bbox_filename=bundler_IO_subdir+"face_box.dat";
   cout << "face_bbox_filename = " << face_bbox_filename << endl;
   filefunc::ReadInfile(face_bbox_filename);
   vector<double> bbox_inputs=stringfunc::string_to_numbers(
      filefunc::text_line[0]);

// Recover lower left and upper right corners for face bounding box
// measured in pixels (py=0 corresponds to box's LOWER edge):
   
   int start_px=bbox_inputs[0];
   int stop_py=ydim-bbox_inputs[1];
   int delta_px=bbox_inputs[2];
   int delta_py=bbox_inputs[3];
   int stop_px=start_px+delta_px;
   int start_py=stop_py-delta_py;
//   cout << "start_px = " << start_px << " stop_px = " << stop_px << endl;
//   cout << "start_py = " << start_py << " stop_py = " << stop_py << endl;

// Convert lower left and upper right bbox corners from pixel to U,V
// image plane coordinates.  Recall 0 <= V <= 1, whereas U ranges from
// 0 to some arbitrary upper limit:

   double v_lo=double(start_py)/double(ydim);
   double v_hi=double(stop_py)/double(ydim);
   double u_lo=double(start_px)/double(ydim);
   double u_hi=double(stop_px)/double(ydim);
   cout << "u_lo = " << u_lo << " u_hi = " << u_hi << endl;
   cout << "v_lo = " << v_lo << " v_hi = " << v_hi << endl;

   bounding_box* bbox_ptr=new bounding_box(u_lo,u_hi,v_lo,v_hi);
//   region_height=0.241;	// Average head height = 24.1 cm ?
   double region_height=8*2.54/100.0;	// Karl's box height is 8" in size

   double delta_UV_ratio=(u_hi-u_lo)/(v_hi-v_lo);
   double region_width=delta_UV_ratio*region_height;

   bbox_ptr->set_physical_deltaX(region_width);
   bbox_ptr->set_physical_deltaY(region_height);
//   cout << "*bbox_ptr = " << *bbox_ptr << endl;

// Save face bounding box in image plane coordinates to ascii file:
  
   string output_filename=bundler_IO_subdir+"face_bbox_uv.dat";
   ofstream outstream;
   filefunc::openfile(output_filename,outstream);

   outstream << "# UV coordinates for human face bounding box" << endl;
   outstream << endl;
   outstream << u_lo << " = u_lo" << endl;
   outstream << u_hi << " = u_hi" << endl;
   outstream << v_lo << " = v_lo" << endl;
   outstream << v_hi << " = v_hi" << endl;
   outstream << region_height << " = region_height (meter)" << endl;
   outstream << region_width << " = region_width (meter)" << endl;
   filefunc::closefile(output_filename,outstream);
}


