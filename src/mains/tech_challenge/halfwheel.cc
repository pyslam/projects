// ========================================================================
// Program HALFWHEEL is a variant of program NEW_FOV .  It's a
// testing lab for viewing D7 180 panorama videos as 3D half-cylinders.

/*

cd ~/programs/c++/svn/projects/src/mains/tech_challenge/

./halfwheel \
--region_filename ./packages/panel0.pkg \
--region_filename ./packages/panel1.pkg \
--region_filename ./packages/panel2.pkg \
--region_filename ./packages/panel3.pkg \
--region_filename ./packages/panel4.pkg \
--initial_mode Manipulate_Fused_Data_Mode

*/

// ========================================================================
// Last updated on 7/4/10; 11/20/10
// ========================================================================

#include <iostream>
#include <string>
#include <vector>
#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgText/Text>

#include "osg/osgGrid/AlirtGridsGroup.h"
#include "osg/osgGraphicals/AnimationController.h"
#include "video/camera.h"
#include "video/camerafuncs.h"
#include "osg/osgGraphicals/CentersGroup.h"
#include "osg/osgGraphicals/CenterPickHandler.h"
#include "math/constant_vectors.h"
#include "osg/osgOrganization/Decorations.h"
#include "geometry/homography.h"
#include "osg/osgSceneGraph/HiresDataVisitor.h"
#include "osg/ModeController.h"
#include "osg/osg2D/MoviesGroup.h"
#include "osg/osg2D/MovieKeyHandler.h"
#include "osg/osgOperations/Operations.h"
#include "passes/PassesGroup.h"
#include "video/photogroup.h"
#include "osg/osgGraphicals/PointFinder.h"
#include "geometry/polyline.h"
#include "osg/osgGeometry/PolyLinesGroup.h"
#include "osg/osg3D/Terrain_Manipulator.h"
#include "osg/osgWindow/ViewerManager.h"

using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::vector;

// ==========================================================================
int main( int argc, char** argv )
{

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);
   int videopass_ID=passes_group.get_videopass_ID();
//   cout << "videopass_ID = " << videopass_ID << endl;

// Read photographs from input video passes:

   photogroup* photogroup_ptr=new photogroup;
   photogroup_ptr->read_photographs(passes_group);
   int n_photos=photogroup_ptr->get_n_photos();
   cout << "n_photos = " << n_photos << endl;

   int n_horiz_pixels=233;
   int n_vertical_pixels=276;
   double FOV_u=(180/5)*PI/180;
   double aspect_ratio=double(n_horiz_pixels)/double(n_vertical_pixels);
   double FOV_v=camerafunc::vert_FOV_from_horiz_FOV_and_aspect_ratio(
      FOV_u,aspect_ratio);
   cout << "FOV_v = " << FOV_v*180/PI << endl;

   double f;
   camerafunc::f_and_aspect_ratio_from_horiz_vert_FOVs(
      FOV_u,FOV_v,f,aspect_ratio);
   cout << "Correct f = " << f << endl;
   cout << "aspect_ratio = " << aspect_ratio << endl;
   cout << "horiz/vert pixels = " << n_horiz_pixels/double(n_vertical_pixels)
        << endl;

// Construct the viewer and instantiate a ViewerManager:

   WindowManager* window_mgr_ptr=new ViewerManager();
   window_mgr_ptr->initialize_window("3D imagery");
//   window_mgr_ptr->getUsage(*arguments.getApplicationUsage());

// Create OSG root node:

   osg::Group* root = new osg::Group;

// Instantiate Operations object to handle mode, animation and image
// number control:

   bool display_movie_state=true;
   bool display_movie_number=true;
   bool display_movie_world_time=true;
//   bool display_movie_state=false;		// viewgraphs
//   bool display_movie_number=false;		// viewgraphs
//   bool display_movie_world_time=false;       // viewgraphs
   bool hide_Mode_HUD_flag=false;
//   bool hide_Mode_HUD_flag=true;

   Operations operations(
      ndims,window_mgr_ptr,passes_group,display_movie_state,
      display_movie_number,display_movie_world_time,hide_Mode_HUD_flag);
   root->addChild(operations.get_OSGgroup_ptr());

   ModeController* ModeController_ptr=operations.get_ModeController_ptr();

// Instantiate AnimationController which acts as master game clock:

   AnimationController* AnimationController_ptr=
      operations.get_AnimationController_ptr();
   AnimationController_ptr->setDelay(0.1);

// Specify start, stop and step times for master game clock:

   operations.set_master_world_start_UTC(
      passes_group.get_world_start_UTC_string());
   operations.set_master_world_stop_UTC(
      passes_group.get_world_stop_UTC_string());
   operations.set_delta_master_world_time_step_per_master_frame(
      passes_group.get_world_time_step());

   AnimationController_ptr->set_world_time_params(
      operations.get_master_world_start_time(),
      operations.get_master_world_stop_time(),
      operations.get_delta_master_world_time_step_per_master_frame());

// Add a custom manipulator to the event handler list:

   osgGA::Terrain_Manipulator* CM_3D_ptr=new osgGA::Terrain_Manipulator(
      ModeController_ptr,window_mgr_ptr);
   window_mgr_ptr->set_CameraManipulator(CM_3D_ptr);

   HiresDataVisitor* HiresDataVisitor_ptr=new HiresDataVisitor();

// Instantiate group to hold all decorations:
   
   Decorations decorations(
      window_mgr_ptr,ModeController_ptr,CM_3D_ptr);

// Instantiate AlirtGrid decorations group:

   double min_X=0;
   double max_X=150;
   double min_Y=0;
   double max_Y=150;
   double min_Z=0;

   cout << "min_X = " << min_X << " max_X = " << max_X << endl;
   cout << "min_Y = " << min_Y << " max_Y = " << max_Y << endl;
   AlirtGrid* grid_ptr=decorations.add_AlirtGrid(
      ndims,passes_group.get_pass_ptr(0),
      min_X,max_X,min_Y,max_Y,min_Z);

   threevector* grid_origin_ptr=grid_ptr->get_world_origin_ptr();
   CM_3D_ptr->set_Grid_ptr(grid_ptr);
   decorations.set_grid_origin_ptr(grid_origin_ptr);

   grid_ptr->set_axes_labels("X (Meters)","Y (Meters)");
   grid_ptr->set_delta_xy(10,10);
   grid_ptr->set_axis_char_label_size(5.0);
   grid_ptr->set_tick_char_label_size(5.0);

   grid_ptr->set_HiresDataVisitor_ptr(HiresDataVisitor_ptr);
   grid_ptr->set_root_ptr(root);

// Instantiate a PointFinder;

   PointFinder pointfinder;
   CM_3D_ptr->set_PointFinder(&pointfinder);

// Instantiate OBSFRUSTAGROUP decoration group:

   OBSFRUSTAGROUP* OBSFRUSTAGROUP_ptr=decorations.add_OBSFRUSTA(
      passes_group.get_pass_ptr(0),AnimationController_ptr);

// Instantiate an individual OBSFRUSTUM for every input video.  Each
// contains a separate movie object.

   threevector camera_posn=*grid_origin_ptr+threevector(100,100,50);

   double frustum_sidelength=-1;
   double movie_downrange_distance=-1;

// Instantiate group to hold movie:

   MoviesGroup movies_group(
      ndims,passes_group.get_pass_ptr(videopass_ID),
//      decorations.get_PointsGroup_ptr(),
//      decorations.get_PolygonsGroup_ptr(),
      AnimationController_ptr);

   string movie_filename=
      passes_group.get_videopass_ptr()->get_first_filename();
//   cout << "movie_filename = " << movie_filename << endl;
   string image_subdir=filefunc::getdirname(movie_filename);
   if (image_subdir.size()==0) image_subdir="./";
//   cout << "image_subdir = " << image_subdir << endl;
   AnimationController_ptr->store_ordered_image_filenames(image_subdir);

   threevector rotation_origin(0,0,0);
   threevector global_camera_translation(0,0,0);
   double global_daz=0*PI/180;
   double global_del=0*PI/180;
   double global_droll=0*PI/180;
   double local_spin_daz=0*PI/180;
   bool multicolor_frusta_flag=false;
   bool thumbnails_flag=false;

   OBSFRUSTAGROUP_ptr->generate_still_imagery_frusta_for_photogroup(
      photogroup_ptr,photogroup_ptr->get_n_photos(),camera_posn,
      global_camera_translation,
      global_daz,global_del,global_droll,rotation_origin,local_spin_daz,
      frustum_sidelength,movie_downrange_distance,multicolor_frusta_flag,
      thumbnails_flag);

// Scan through subdirectory containing video frames.  Set minimum and
// maximum photo numbers based upon the files' name:

   int min_photo_number=-1;
   int max_photo_number=-1;
   videofunc::find_min_max_photo_numbers(
      image_subdir,min_photo_number,max_photo_number);
   cout << "min_photo_number = " << min_photo_number
        << " max_photo_number = " << max_photo_number << endl;

   int Nimages=max_photo_number-min_photo_number+1;
   AnimationController_ptr->set_nframes(Nimages);
   for (int n=0; n<OBSFRUSTAGROUP_ptr->get_n_Graphicals(); n++)
   {
      OBSFRUSTUM* OBSFRUSTUM_ptr=OBSFRUSTAGROUP_ptr->get_OBSFRUSTUM_ptr(n);
      Movie* movie_ptr=OBSFRUSTUM_ptr->get_Movie_ptr();
      movie_ptr->get_texture_rectangle_ptr()->set_first_frame_to_display(
         min_photo_number);
      movie_ptr->get_texture_rectangle_ptr()->
         set_last_frame_to_display(max_photo_number);
      movie_ptr->get_texture_rectangle_ptr()->set_panel_number(n);
   }
   root->addChild( movies_group.get_OSGgroup_ptr() );

   MovieKeyHandler* MoviesKeyHandler_ptr=
      new MovieKeyHandler(ModeController_ptr,&movies_group);
   window_mgr_ptr->get_EventHandlers_ptr()->push_back(MoviesKeyHandler_ptr);
  
// Set mask_nonselected_OSGsubPATs_flag=false in order to
// simultatneously display OSGsubPATs 0 and 1 respectively containing
// static panorama and dynamic video images:

   decorations.get_OBSFRUSTUMPickHandler_ptr()->
      set_mask_nonselected_OSGsubPATs_flag(false);

/*
   SignPostsGroup* SignPostsGroup_ptr=decorations.add_SignPosts(
      ndims,passes_group.get_pass_ptr(0));
   SignPostsGroup_ptr->set_common_geometrical_size(0.03);

   twovector UV0(0.5 , 0.5);
   SignPost* SignPost0_ptr=OBSFRUSTAGROUP_ptr->
      generate_SignPost_at_imageplane_location(UV0,0,SignPostsGroup_ptr);

   SignPost0_ptr->set_label("Building one");
//   double extra_frac_cyl_height=0.5;
//   SignPost_ptr->set_label("Building one",extra_frac_cyl_height);
//   SignPost0_ptr->set_max_text_width("Build");

   twovector UV1(0.75 , 0.75);
   SignPost* SignPost1_ptr=OBSFRUSTAGROUP_ptr->
      generate_SignPost_at_imageplane_location(UV1,1,SignPostsGroup_ptr);
   SignPost1_ptr->set_label("Building two is a big structure");

//   double extra_frac_cyl_height=0.5;
//   SignPost_ptr->set_label("Building one",extra_frac_cyl_height);
//   SignPost1_ptr->set_max_text_width("Build");
*/

// Optimize the scene graph, remove redundent nodes and states, and
// then attach it to the viewer:

   osgUtil::Optimizer optimizer;
   optimizer.optimize(root);
   root->addChild(decorations.get_OSGgroup_ptr());

   window_mgr_ptr->setSceneData(root);

// Create the windows and run the threads.  Viewer's realize method
// calls the CustomManipulator's home() method:

   window_mgr_ptr->realize();

   while( !window_mgr_ptr->done() )
   {
      window_mgr_ptr->process();
   }

   delete window_mgr_ptr;
}

