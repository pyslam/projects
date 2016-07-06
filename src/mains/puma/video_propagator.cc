// ========================================================================
// Program VIDEO_PROPAGATOR is a variant of mains/photosynth/PROPAGATOR.  It
// pops open a 3D window displaying a bundle-adjusted point cloud and
// aerial OBSFRUSTA. It also opens a 2D window in which the video
// frame for the currently selected OBSFRUSTUM is displayed.
// When the 2D window works in RUN_MOVIE_MODE, all video frames may
// easily be viewed by pressing the left/right arrow keys.

// A user may mark some imageplane point of interest within the 2D
// window after entering into INSERT_FEATURE_MODE.  A 3D ray is then
// backprojected from the current OBSFRUSTUM down towards
// a ground Zplane.  Feature 0 appears in all other video frames
// within the lower left corner of each image plane.  Video tiepoints
// may be manually established by dragging the zeroth feature
// crosshairs from the lower left corner to an appropriate image plane
// location in MANIPULATE_FEATURE_MODE.  Within the 3D window, a
// colored ray moves as the 2D feature is dragged.

// Once 2 or more tiepoints have been established, the user may press
// 'm' within the 2D window provided it operates in
// MANIPULATE_FEATURE_MODE.  A set of 3D crosshairs then appears at
// the best-fit intersection of the backprojected rays.  The
// intersection point's easting, northing and height above ground are
// indicated via a SignPost at its 3D location.

// ========================================================================
// Last updated on 9/11/13; 9/15/13; 12/4/13
// ========================================================================

#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "osg/osgGraphicals/AnimationController.h"
#include "osg/osgGeometry/ArrowsGroup.h"
#include "models/Building.h"
#include "models/BuildingsGroup.h"
#include "bundler/bundlerfuncs.h"
#include "osg/Custom2DManipulator.h"
#include "osg/osgOrganization/Decorations.h"
#include "osg/osgEarth/EarthRegionsGroup.h"
#include "osg/osgGrid/GridKeyHandler.h"
#include "osg/osgGrid/LatLongGridsGroup.h"
#include "messenger/Messenger.h"
#include "osg/ModeController.h"
#include "osg/ModeKeyHandler.h"
#include "osg/osg2D/MoviesGroup.h"
#include "osg/osg2D/MovieKeyHandler.h"
#include "osg/osgSceneGraph/MyDatabasePager.h"
#include "osg/osgOperations/Operations.h"
#include "models/ParkingLotsGroup.h"
#include "passes/PassesGroup.h"
#include "video/photogroup.h"
#include "geometry/plane.h"
#include "osg/osg3D/PointCloudsGroup.h"
#include "osg/osg3D/PointCloudKeyHandler.h"
#include "osg/osgGraphicals/PointFinder.h"
#include "osg/osgGeometry/PointsGroup.h"
#include "osg/osgGeometry/PolygonsGroup.h"
#include "osg/osgGeometry/PolyhedraGroup.h"
#include "osg/osgGeometry/PolyLinesGroup.h"
#include "osg/osgGIS/postgis_databases_group.h"
#include "models/RoadsGroup.h"
#include "osg/osg3D/Terrain_Manipulator.h"
#include "osg/osgTiles/TilesGroup.h"
#include "time/timefuncs.h"
#include "osg/osgWindow/ViewerManager.h"

#include "general/outputfuncs.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::pair;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

   bool demo_flag=true;
   if (demo_flag)
   {
      string broker_URL = "tcp://127.0.0.1:61616";
      string message_queue_channel_name="127.0.0.1";
      string message_sender_ID="MESSAGE_SENDER";
      bool include_sender_and_timestamp_info_flag=false;
      Messenger m(broker_URL,message_queue_channel_name,message_sender_ID,
                  include_sender_and_timestamp_info_flag);

      string command="DISPLAY_NEXT_FLOW_DIAGRAM";
      m.sendTextMessage(command);
   }
   timefunc::initialize_timeofday_clock();      

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);

   int cloudpass_ID=passes_group.get_curr_cloudpass_ID();
   int videopass_ID=passes_group.get_videopass_ID();
   cout << "cloudpass_ID = " << cloudpass_ID << endl;
   cout << "videopass_ID = " << videopass_ID << endl;
   vector<int> GISlayer_IDs=passes_group.get_GISlayer_IDs();

   string image_list_filename=passes_group.get_image_list_filename();
   cout << "image_list_filename = " << image_list_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(image_list_filename);
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;
   string image_sizes_filename=passes_group.get_image_sizes_filename();
   cout << "image_sizes_filename = " << image_sizes_filename << endl;

/*
// Model ground via horizontal Z-plane:

   double Zground=bundlerfunc::extract_Zground(bundler_IO_subdir);
   plane ground_plane(z_hat,threevector(0,0,Zground));
//   cout << "ground_plane = " << ground_plane << endl;
*/

// Read photographs from input video passes:

   photogroup* photogroup_ptr=new photogroup;
   photogroup_ptr->read_photographs(passes_group,image_sizes_filename);
   int n_photos=photogroup_ptr->get_n_photos();
   cout << "n_photos = " << n_photos << endl;
   photogroup_ptr->average_camera_elevation();
//   outputfunc::enter_continue_char();

// Construct the viewer and instantiate a ViewerManager:

   WindowManager* window_mgr_2D_ptr=new ViewerManager();
   WindowManager* window_mgr_3D_ptr=new ViewerManager();
   string window_2D_title="2D imagery";
   string window_3D_title="3D imagery";

   window_mgr_2D_ptr->initialize_dual_windows(
      window_2D_title,window_3D_title,window_mgr_3D_ptr);

// Instantiate separate messengers for each Decorations group which
// needs to receive mail:

   int pass_ID=passes_group.get_n_passes()-1;
   string broker_URL=passes_group.get_pass_ptr(pass_ID)->
      get_PassInfo_ptr()->get_ActiveMQ_hostname();
   cout << "ActiveMQ broker_URL = " << broker_URL << endl;

   string message_sender_ID="PROPAGATOR";

// Instantiate photo network messengers for communication with Michael
// Yee's social network tool:

   string photo_network_message_queue_channel_name="photo_network";
   Messenger OBSFRUSTA_photo_network_messenger( 
      broker_URL,photo_network_message_queue_channel_name,message_sender_ID);
   Messenger Movies_photo_network_messenger( 
      broker_URL,photo_network_message_queue_channel_name,message_sender_ID);

// Create two OSG root nodes:

   osg::Group* root_2D = new osg::Group;
   osg::Group* root_3D = new osg::Group;

// Instantiate postgis database objects to send data to and retrieve
// data from external Postgres database:

   postgis_databases_group* postgis_databases_group_ptr=
      new postgis_databases_group;
   postgis_database* postgis_db_ptr=postgis_databases_group_ptr->
      generate_postgis_database_from_GISlayer_IDs(passes_group,GISlayer_IDs);
   
// Instantiate Operations object to handle mode, animation and image
// number control:

//   bool display_movie_state=false;
   bool display_movie_state=true;
//   bool display_movie_number=false;
   bool display_movie_number=true;
//   bool hide_Mode_HUD_flag=true;
   bool hide_Mode_HUD_flag=false;
   Operations operations(2,window_mgr_2D_ptr,display_movie_state,
                         display_movie_number,hide_Mode_HUD_flag);

   ModeController* ModeController_2D_ptr=operations.get_ModeController_ptr();
   ModeController_2D_ptr->setState(ModeController::RUN_MOVIE);
   AnimationController* AnimationController_ptr=
      operations.get_AnimationController_ptr();
   root_2D->addChild(operations.get_OSGgroup_ptr());

   ModeController* ModeController_3D_ptr=new ModeController();
   ModeController_3D_ptr->setState(ModeController::MANIPULATE_FUSED_DATA);
   window_mgr_3D_ptr->get_EventHandlers_ptr()->push_back( 
      new ModeKeyHandler(ModeController_3D_ptr) );
   root_3D->addChild(osgfunc::create_Mode_HUD(3,ModeController_3D_ptr));

// Add custom manipulators to the event handler list:

   osgGA::Custom2DManipulator* CM_2D_ptr = new 
      osgGA::Custom2DManipulator(ModeController_2D_ptr,window_mgr_2D_ptr);
   window_mgr_2D_ptr->set_CameraManipulator(CM_2D_ptr);

   osgGA::Terrain_Manipulator* CM_3D_ptr=new osgGA::Terrain_Manipulator(
      ModeController_3D_ptr,window_mgr_3D_ptr);
   window_mgr_3D_ptr->set_CameraManipulator(CM_3D_ptr);

// Instantiate group to hold all decorations:

   Decorations decorations_2D(
      window_mgr_2D_ptr,ModeController_2D_ptr,CM_2D_ptr);
   Decorations decorations_3D(
      window_mgr_3D_ptr,ModeController_3D_ptr,CM_3D_ptr);

// Instantiate group to hold 2D movies:

   MoviesGroup movies_2D_group(
      2,passes_group.get_pass_ptr(videopass_ID),AnimationController_ptr);
   movies_2D_group.set_photogroup_ptr(photogroup_ptr);
   movies_2D_group.pushback_Messenger_ptr(&Movies_photo_network_messenger);

   window_mgr_2D_ptr->get_EventHandlers_ptr()->push_back(
      new MovieKeyHandler(ModeController_2D_ptr,&movies_2D_group));

   Movie* movie_ptr=movies_2D_group.generate_new_Movie(passes_group);
   cout << "movie_ptr->get_Nimages() = "
        << movie_ptr->get_Nimages() << endl;

   root_2D->addChild( movies_2D_group.get_OSGgroup_ptr() );

// Instantiate group to hold 3D movies:

   MoviesGroup movies_3D_group(
      3,passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);
   root_3D->addChild( movies_3D_group.get_OSGgroup_ptr() );

// Instantiate groups to hold multiple point clouds,
// latitude-longitude grids and associated earth regions:

   PointCloudsGroup clouds_group(
      passes_group.get_pass_ptr(cloudpass_ID));
   decorations_3D.set_PointCloudsGroup_ptr(&clouds_group);
   clouds_group.set_Terrain_Manipulator_ptr(CM_3D_ptr);
   clouds_group.set_auto_resize_points_flag(false);
//   clouds_group.set_auto_resize_points_flag(true);

// The HAFB ALIRT minimap has very dark intensity values.  So for
// intensity display purposes, we artificially brighten all p-values
// by the following intensity magnification factor:

   double intensity_magnification=2.0;
//   cout << "Enter intensity magnification:" << endl;
//   cin >> intensity_magnification;
   clouds_group.get_ColorGeodeVisitor_ptr()->
      set_probabilities_magnification(intensity_magnification);

   LatLongGridsGroup latlonggrids_group(
      3,passes_group.get_pass_ptr(cloudpass_ID),CM_3D_ptr);
   EarthRegionsGroup earth_regions_group(
      passes_group.get_pass_ptr(cloudpass_ID),      
      &clouds_group,&latlonggrids_group);
   earth_regions_group.generate_regions(passes_group);

   LatLongGrid* LatLongGrid_ptr=latlonggrids_group.get_Grid_ptr(0);
//   LatLongGrid_ptr->set_dynamic_grid_flag(false);
   threevector* grid_origin_ptr=LatLongGrid_ptr->get_world_origin_ptr();
//   cout << "*grid_origin_ptr = " << *grid_origin_ptr << endl;

   LatLongGrid_ptr->set_axis_char_label_size(
      3*LatLongGrid_ptr->get_axis_char_label_size());
   LatLongGrid_ptr->update_grid();
   
   CM_3D_ptr->set_Grid_ptr(LatLongGrid_ptr);
   decorations_3D.set_grid_origin_ptr(grid_origin_ptr);

   double xmin=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().xMin();
   double xmax=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().xMax();
   double ymin=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().yMin();
   double ymax=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().yMax();
   if (postgis_db_ptr != NULL)
      postgis_db_ptr->pushback_gis_bbox(xmin,xmax,ymin,ymax);

   window_mgr_3D_ptr->get_EventHandlers_ptr()->push_back(
      new PointCloudKeyHandler(&clouds_group,ModeController_3D_ptr));
   window_mgr_3D_ptr->get_EventHandlers_ptr()->push_back(
      new GridKeyHandler(ModeController_3D_ptr,LatLongGrid_ptr));

   root_3D->addChild(clouds_group.get_OSGgroup_ptr());
   root_3D->addChild(latlonggrids_group.get_OSGgroup_ptr());

// Instantiate a MyDatabasePager to handle paging of files from disk:

   viewer::MyDatabasePager* MyDatabasePager_ptr=new viewer::MyDatabasePager(
      clouds_group.get_SetupGeomVisitor_ptr(),
      clouds_group.get_ColorGeodeVisitor_ptr());

// Instantiate a PointFinder;

   PointFinder pointfinder(&clouds_group);
   pointfinder.set_minimal_allowed_range(100);	// meters
//   pointfinder.set_maximal_rho(10);
   pointfinder.set_maximal_rho(20);	// meters
//   pointfinder.set_maximal_rho(30);	// meters
//   pointfinder.set_maximal_rho(70);	// meters
   pointfinder.set_max_cone_halfangle(4);	// degs
   CM_3D_ptr->set_PointFinder(&pointfinder);

// Instantiate Arrows decoration group:

   ArrowsGroup* ArrowsGroup_ptr=decorations_3D.add_Arrows(
      3,passes_group.get_pass_ptr(cloudpass_ID));
   ArrowsGroup_ptr->set_altitude_dependent_size_flag(false);
//   ArrowsGroup_ptr->set_CM_3D_ptr(CM_3D_ptr);

// Instantiate 3D & 2D features decorations group:

   FeaturesGroup* FeaturesGroup_3D_ptr=
      decorations_3D.add_Features(3,passes_group.get_pass_ptr(cloudpass_ID));
   double crosshairs_size=FeaturesGroup_3D_ptr->get_crosshairs_size();
   double text_size=FeaturesGroup_3D_ptr->get_crosshairs_text_size();
   FeaturesGroup_3D_ptr->set_crosshairs_size(30*crosshairs_size);
   FeaturesGroup_3D_ptr->set_crosshairs_text_size(10*text_size);
   FeaturesGroup_3D_ptr->set_maximum_raytrace_range(5*1000);	// meters

   FeaturesGroup* FeaturesGroup_2D_ptr=decorations_2D.add_Features(
      2,passes_group.get_pass_ptr(videopass_ID),
      NULL,movie_ptr,NULL,NULL,AnimationController_ptr);
   decorations_2D.set_DataNode_ptr(movie_ptr->getGeode());

   FeaturesGroup_2D_ptr->set_AnimationController_ptr(AnimationController_ptr);
   FeaturesGroup_2D_ptr->set_package_subdir(
      passes_group.get_videopass_ptr()->get_PassInfo_ptr()->
      get_package_subdir());
   FeaturesGroup_2D_ptr->set_package_filename_prefix(
      passes_group.get_videopass_ptr()->get_PassInfo_ptr()->
      get_package_filename_prefix());
   FeaturesGroup_2D_ptr->set_MoviesGroup_ptr(&movies_2D_group);
   FeaturesGroup_2D_ptr->set_photogroup_ptr(photogroup_ptr);
   FeaturesGroup_2D_ptr->set_PointFinder_ptr(&pointfinder);
   FeaturesGroup_2D_ptr->set_FeaturesGroup_3D_ptr(FeaturesGroup_3D_ptr);
   FeaturesGroup_2D_ptr->set_EarthRegionsGroup_ptr(&earth_regions_group);

//   FeaturesGroup_2D_ptr->set_display_geocoords_flag(false);
   FeaturesGroup_2D_ptr->set_display_geocoords_flag(true);
   FeaturesGroup_2D_ptr->set_display_range_alt_flag(true);

// Increase default size for 2D crosshairs and corresponding text:

   FeaturesGroup_2D_ptr->set_crosshairs_size(
      4*(FeaturesGroup_2D_ptr->get_crosshairs_size()));
   FeaturesGroup_2D_ptr->set_crosshairs_text_size(
      4*(FeaturesGroup_2D_ptr->get_crosshairs_text_size()));

// Instantiate 3D SignPosts decorations group:

   SignPostsGroup* SignPostsGroup_ptr=decorations_3D.add_SignPosts(
      3,passes_group.get_pass_ptr(cloudpass_ID));
   SignPostsGroup_ptr->set_postgis_databases_group_ptr(
      postgis_databases_group_ptr);

   for (int n=0; n<decorations_3D.get_n_SignPostsGroups(); n++)
   {
      decorations_3D.get_SignPostsGroup_ptr(n)->set_EarthRegionsGroup_ptr(
         &earth_regions_group);
   }

   SignPostsGroup_ptr->set_photogroup_ptr(photogroup_ptr);
   SignPostsGroup_ptr->set_package_subdir(
      passes_group.get_videopass_ptr()->get_PassInfo_ptr()->
      get_package_subdir());
   SignPostsGroup_ptr->set_package_filename_prefix(
      passes_group.get_videopass_ptr()->get_PassInfo_ptr()->
      get_package_filename_prefix());
//   cout << "package_subdir = " 
//        <<  passes_group.get_videopass_ptr()->get_PassInfo_ptr()->
//      get_package_subdir() << endl;
//   cout << "package_filename_prefix = "
//        <<  passes_group.get_videopass_ptr()->get_PassInfo_ptr()->
//      get_package_filename_prefix() << endl;
   SignPostsGroup_ptr->set_MoviesGroup_ptr(&movies_2D_group);

   SignPostsKeyHandler* SignPostsKeyHandler_ptr=decorations_3D.
      get_SignPostsKeyHandler_ptr();
   SignPostsKeyHandler_ptr->set_AnimationController_ptr(
      AnimationController_ptr);

// Instantiate 2D imageplane SignPosts group and pick handler:

   threevector* grid_world_origin_ptr=NULL;
   SignPostsGroup* imageplane_SignPostsGroup_ptr=new SignPostsGroup(
      2,passes_group.get_pass_ptr(videopass_ID),grid_world_origin_ptr);
   imageplane_SignPostsGroup_ptr->set_AnimationController_ptr(
      AnimationController_ptr);

   SignPostPickHandler* imageplane_SignPostPickHandler_ptr=
      new SignPostPickHandler(
         2,passes_group.get_pass_ptr(videopass_ID),CM_2D_ptr,
         imageplane_SignPostsGroup_ptr,ModeController_2D_ptr,
         window_mgr_2D_ptr,grid_world_origin_ptr);
   imageplane_SignPostPickHandler_ptr->
      set_allow_doubleclick_in_manipulate_fused_data_mode(true);
   imageplane_SignPostPickHandler_ptr->set_DataNode_ptr(
      movie_ptr->getGeode());
   window_mgr_2D_ptr->get_EventHandlers_ptr()->push_back(
      imageplane_SignPostPickHandler_ptr);

   SignPostsGroup_ptr->set_imageplane_SignPostsGroup_ptr(
      imageplane_SignPostsGroup_ptr);
   SignPostsGroup_ptr->set_imageplane_SignPostPickHandler_ptr(
      imageplane_SignPostPickHandler_ptr);

   decorations_2D.get_OSGgroup_ptr()->addChild(imageplane_SignPostsGroup_ptr->
                                               get_OSGgroup_ptr());

// Instantiate OBSFRUSTA decoration group:

   OBSFRUSTAGROUP* OBSFRUSTAGROUP_ptr=decorations_3D.add_OBSFRUSTA(
      passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);
   OBSFRUSTAGROUP_ptr->pushback_Messenger_ptr(
      &OBSFRUSTA_photo_network_messenger);
   OBSFRUSTAGROUP_ptr->set_enable_OBSFRUSTA_blinking_flag(true);
   OBSFRUSTAGROUP_ptr->set_erase_other_OBSFRUSTA_flag(true);
   OBSFRUSTAGROUP_ptr->set_play_OBSFRUSTA_as_movie_flag(true);

//    OBSFRUSTAGROUP_ptr->set_groundplane_pi(ground_plane.get_pi());

   FeaturesGroup_2D_ptr->set_OBSFRUSTAGROUP_ptr(OBSFRUSTAGROUP_ptr);
   FeaturesGroup_2D_ptr->set_SignPostsGroup_ptr(SignPostsGroup_ptr);

// Instantiate an individual OBSFRUSTUM for every still image:

   double frustum_sidelength=-1;
//   double frustum_sidelength=0.1;
//   double frustum_sidelength=1;
//   double frustum_sidelength=10;
   double movie_downrange_distance=-1;
   bool multicolor_frusta_flag=false;
   bool thumbnails_flag=true;
   OBSFRUSTAGROUP_ptr->generate_still_imagery_frusta_for_photogroup(
      photogroup_ptr,frustum_sidelength,movie_downrange_distance,
      multicolor_frusta_flag,thumbnails_flag);

// Set mask_nonselected_OSGsubPATs_flag=false in order to
// simultaneously display OSGsubPATs 0 and 1 respectively containing
// static panorama and dynamic video images:

   decorations_3D.get_OBSFRUSTUMPickHandler_ptr()->
      set_mask_nonselected_OSGsubPATs_flag(false);
   decorations_3D.get_OBSFRUSTAKeyHandler_ptr()->set_SignPostsGroup_ptr(
      SignPostsGroup_ptr);
   decorations_3D.get_OBSFRUSTAKeyHandler_ptr()->set_photogroup_ptr(
      photogroup_ptr);

// Instantiate Points decorations group:

   osgGeometry::PointsGroup* PointsGroup_ptr=decorations_3D.add_Points(
      3,passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);
   OBSFRUSTAGROUP_ptr->set_PointsGroup_ptr(PointsGroup_ptr);

// Instantiate PolyLines, Polygons and Polyhedra decoration groups:

   PolyLinesGroup* imageplane_PolyLinesGroup_ptr=
      decorations_2D.add_PolyLines(
         2,passes_group.get_pass_ptr(videopass_ID));
   osgGeometry::PolygonsGroup* imageplane_PolygonsGroup_ptr=
      decorations_2D.add_Polygons(
         2,passes_group.get_pass_ptr(videopass_ID),AnimationController_ptr);
   imageplane_PolygonsGroup_ptr->set_PolyLinesGroup_ptr(
      imageplane_PolyLinesGroup_ptr);

   PolyhedraGroup* PolyhedraGroup_ptr=
      decorations_3D.add_Polyhedra(passes_group.get_pass_ptr(cloudpass_ID));
//   PolyhedraGroup_ptr->generate_Building_Polyhedra(BuildingsGroup_ptr);

// Create the windows and run the threads:

   root_2D->addChild(decorations_2D.get_OSGgroup_ptr());
   root_3D->addChild(decorations_3D.get_OSGgroup_ptr());

   window_mgr_2D_ptr->setSceneData(root_2D);
   window_mgr_3D_ptr->setSceneData(root_3D);

   window_mgr_2D_ptr->realize();
   window_mgr_3D_ptr->realize();

// Adjust 3D viewer's field-of-view if some positive virtual_horiz_FOV
// parameter is passed an input argument:

   double virtual_horiz_FOV=passes_group.get_virtual_horiz_FOV();
   if (virtual_horiz_FOV > 0)
   {
      double FOV_h=window_mgr_3D_ptr->get_lens_horizontal_FOV();
      double angular_scale_factor=virtual_horiz_FOV/FOV_h;  
      cout << "virtual_horiz_FOV = " << virtual_horiz_FOV << endl;
//   cout << "angular_scale_factor = " << angular_scale_factor << endl;
      window_mgr_3D_ptr->rescale_viewer_FOV(angular_scale_factor);
   }

// For viewchart purposes only, we try to suppress point cloud display

//   clouds_group.set_OSGgroup_nodemask(0);
//   OBSFRUSTAGROUP_ptr->set_OSGgroup_nodemask(0);
//   PolyhedraGroup_ptr->set_OSGgroup_nodemask(0);

   AnimationController_ptr->set_nframes(n_photos);

// ========================================================================

   while( !window_mgr_2D_ptr->done() && !window_mgr_3D_ptr->done() )
   {
      window_mgr_2D_ptr->process();
      window_mgr_3D_ptr->process();
   }
   delete window_mgr_2D_ptr;
   delete window_mgr_3D_ptr;

   delete postgis_databases_group_ptr;

}
