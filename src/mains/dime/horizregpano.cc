// ========================================================================
// Program HORIZREGPANO attempts to rotate WISP panel cameras so that
// they align with the sinusoidal-fit parameters generated by program
// HORIZON.  But as of 3/15/13, we were not able to generate
// consistent package files from this program...
// ========================================================================
// Last updated on 3/15/13
// ========================================================================

#include <iostream>
#include <string>
#include <vector>

#include "video/camera.h"
#include "video/camerafuncs.h"
#include "numerical/param_range.h"
#include "passes/PassesGroup.h"
#include "video/photogroup.h"

using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::ifstream;
using std::string;
using std::vector;

// ==========================================================================
int main( int argc, char** argv )
{
   string bundler_IO_subdir="./bundler/DIME/Feb2013_DeerIsland/";
   string packages_subdir=bundler_IO_subdir+"packages/panels/";

   double f,FOV_v,FOV_u=36*PI/180;
   double width=4000;
   double height=2200;
   double aspect_ratio=width/height;
   double U0=0.5*aspect_ratio;
   double V0=0.5;
   camerafunc::f_and_vert_FOV_from_horiz_FOV_and_aspect_ratio(
      FOV_u,aspect_ratio,f,FOV_v);
   cout << "f = " << f 
        << " FOV_v = " << FOV_v*180/PI 
        << " U0 = " << U0
        << endl;
//   cout << "aspect_ratio = " << aspect_ratio 
//        << " 0.5*aspect_ratio = " << 0.5*aspect_ratio << endl;

// f = -2.79789 
// FOV_v = 20.2643
// U0 = 0.909091

/*

Number of input theta,py horizon pairs = 25
Average residual = 0.246382 pixels

Best A value = 10.1289 pixels 
Best phi_zero value = 309.848 degs
Best py_avg value = 1114.04 pixels

*/

   cout.precision(12);
   double dtheta_0=10.1289*(2*PI)/40000;
   cout << "dtheta_0 = " << dtheta_0*180/PI << endl;

   cout << "Enter manual dtheta_0 in degs:" << endl;
   cin >> dtheta_0;
   dtheta_0 *= PI/180;

// FAKE FAKE:  Fri Mar 15, 2013 at 4:27 pm

//   dtheta_0=0;
//   dtheta_0 *= 3;
//   dtheta_0 *= 4;


   double theta_0=PI/2-dtheta_0;

   double phi_0=309.848*PI/180;

// FAKE FAKE:  Fri Mar 15, 2013 at 5:45 pm


//   phi_0=0;
   
//   cout << "Enter phi_0 in degs:" << endl;
//   cin >> phi_0;
//   phi_0 *= PI/180;

   threevector n_hat=mathfunc::construct_direction_vector(phi_0,theta_0);
   cout << "n_hat = " << n_hat << endl;

   double TINY=1E-12;
   rotation Rglobal;
   Rglobal=Rglobal.rotation_taking_u_to_v(z_hat,n_hat,TINY);
   cout << "Rglobal = " << Rglobal << endl;
//    outputfunc::enter_continue_char();

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   PassesGroup passes_group(&arguments);
   int n_passes=passes_group.get_n_passes();
   cout << "n_passes = " << n_passes << endl;

// Read photographs from input video passes:

   photogroup* photogroup_ptr=new photogroup;
   photogroup_ptr->read_photographs(passes_group);

   string order_filename="./packages/calib/panels_order.dat";
   photogroup_ptr->set_photo_order(order_filename);
   
   int n_photos(photogroup_ptr->get_n_photos());
   cout << "n_photos = " << n_photos << endl;

//   threevector camera_posn(500,500,10);
   threevector camera_posn(339105 , 4690479 , 10);
   double az_0=-78*PI/180;	// hardwire reasonable estimage for az_0
//   double el=0;
//   double roll=0;

   int n_panels=10;
   for (int p=0; p<n_panels; p++)
   {
      photograph* photo_ptr=photogroup_ptr->get_photograph_ptr(p);
      camera* camera_ptr=photo_ptr->get_camera_ptr();
      camera_ptr->set_internal_params(f,f,U0,V0);
      camera_ptr->set_world_posn(camera_posn);

      double curr_az=az_0-p*36*PI/180;
//      camera_ptr->set_Rcamera(curr_az,el,roll);
      cout << "p = " << p << " curr_az = " << curr_az << endl;
//      cout << "cos(curr_az) = " << cos(curr_az)
//           << " sin(curr_az) = " << sin(curr_az) << endl;
//      cout << "cos(curr_az-90) = " << cos(curr_az-PI/2)
//           << " sin(curr_az-90) = " << sin(curr_az-PI/2) << endl;
      threevector Uhat(cos(curr_az-PI/2),sin(curr_az-PI/2),0);
//      cout << "Uhat = " << camera_ptr->get_Uhat() << endl;
      threevector Vhat(0,0,1);

// FAKE FAKE:  Fri Mar 15, 2013 at 4:22 pm


      Uhat=Rglobal*Uhat;
      Vhat=Rglobal*Vhat;
      cout << "Uhat = " << Uhat << " Vhat = " << Vhat << endl;

      camera_ptr->set_Rcamera(Uhat,Vhat);

/*
      threevector What=Uhat.cross(Vhat);
      rotation Rcamera;
      Rcamera.put_row(0,Uhat);
      Rcamera.put_row(1,Vhat);
      Rcamera.put_row(2,What);
      cout << "Rcamera = " << Rcamera << endl;
*/

      cout << endl;
   }
   
// Note added on 2/8/09: When we globally rescale and rotate
// imagespace rays onto worldspace rays, photographs are not
// reordered.  So photos_ordered_flag is set equal to false below:
 
   bool photos_ordered_flag=false;
   double frustum_sidelength=50;	// meters
   photogroup_ptr->export_photo_parameters(
      packages_subdir,photos_ordered_flag,frustum_sidelength);

}
