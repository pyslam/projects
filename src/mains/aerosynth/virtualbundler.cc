// ========================================================================
// Program VIRTUALBUNDLER is a variant of VIEWBUNDLER where we
// experiment with reprojecting light hawk imagery into virtual
// OBSFRUSTA image planes.
// ========================================================================
// Last updated on 3/2/11; 3/4/11; 11/5/11
// ========================================================================

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <osgDB/FileUtils>

#include "osg/osgGraphicals/AnimationController.h"
#include "geometry/bounding_box.h"
#include "osg/osgGraphicals/CentersGroup.h"
#include "osg/osgGraphicals/CentersKeyHandler.h"
#include "osg/osgGraphicals/CenterPickHandler.h"
#include "osg/osgOrganization/Decorations.h"
#include "osg/osgSceneGraph/HiresDataVisitor.h"
#include "messenger/Messenger.h"
#include "osg/ModeController.h"
#include "osg/osgModels/MODELSGROUP.h"
#include "osg/osg2D/MoviesGroup.h"
#include "osg/osgSceneGraph/MyDatabasePager.h"
#include "osg/osgOperations/Operations.h"
#include "passes/PassesGroup.h"
#include "video/photogroup.h"
#include "osg/osgModels/PhotoToursGroup.h"
#include "osg/osgModels/PhotoToursKeyHandler.h"
#include "osg/osg3D/PointCloudsGroup.h"
#include "osg/osg3D/PointCloudKeyHandler.h"
#include "osg/osgGraphicals/PointFinder.h"
#include "osg/osgGeometry/PointsGroup.h"
#include "osg/osgGeometry/PolygonsGroup.h"
#include "osg/osgGeometry/PolyLinesGroup.h"
#include "osg/osgGIS/postgis_database.h"
#include "osg/osg3D/Terrain_Manipulator.h"
#include "time/timefuncs.h"
#include "video/videofuncs.h"
#include "osg/osgWindow/ViewerManager.h"

#include "general/outputfuncs.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::map;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);

   int cloudpass_ID=passes_group.get_curr_cloudpass_ID();
//   int texturepass_ID=passes_group.get_curr_texturepass_ID();

   string image_list_filename=passes_group.get_image_list_filename();
   cout << "image_list_filename = " << image_list_filename << endl;
   string bundler_IO_subdir=filefunc::getdirname(image_list_filename);
   cout << "bundler_IO_subdir = " << bundler_IO_subdir << endl;

   string image_sizes_filename=passes_group.get_image_sizes_filename();
   cout << "image_sizes_filename = " << image_sizes_filename << endl;
   string xyz_points_filename=passes_group.get_xyz_points_filename();
   cout << "xyz_points_filename = " << xyz_points_filename << endl;
   string camera_views_filename=passes_group.get_camera_views_filename();
   cout << "camera_views_filename = " << camera_views_filename << endl;
   string face_bbox_uv_filename=bundler_IO_subdir+"face_bbox_uv.dat";
   cout << "face_bbox_uv_filename = " << face_bbox_uv_filename << endl;
//   string common_planes_filename=passes_group.get_common_planes_filename();
//   cout << "common_planes_filename = " << common_planes_filename << endl;

// Read photographs from input video passes:

   photogroup* photogroup_ptr=new photogroup;
   photogroup_ptr->read_photographs(passes_group,image_sizes_filename);
   int n_photos=photogroup_ptr->get_n_photos();
   cout << "n_photos = " << n_photos << endl;

// Import common planes for pairs of overlapping cameras:

//   photogroup_ptr->import_common_photo_planes(common_planes_filename);

// Read in reconstructed XYZ points plus their IDs along with visible
// camera IDs into videofunc map *xyz_map_ptr:

   videofunc::CAMERAID_XYZ_MAP* cameraid_xyz_map_ptr=NULL;
   if (xyz_points_filename.size() > 0)
   {
      videofunc::import_reconstructed_XYZ_points(xyz_points_filename);

//  Generate STL map containing STL vectors of reconstructed XYZ
//  points as a function of visible camera ID:

      cameraid_xyz_map_ptr=videofunc::sort_XYZ_points_by_camera_ID();
   }

// Construct the viewer and instantiate a ViewerManager:

   WindowManager* window_mgr_ptr=new ViewerManager();
   window_mgr_ptr->initialize_window("3D imagery");

// Instantiate separate messengers for each Decorations group which
// needs to receive mail:

   int pass_ID=passes_group.get_n_passes()-1;
   string broker_URL=passes_group.get_pass_ptr(pass_ID)->
      get_PassInfo_ptr()->get_ActiveMQ_hostname();
   cout << "ActiveMQ broker_URL = " << broker_URL << endl;

   string message_sender_ID="VIEWBUNDLER";

// Instantiate urban network messengers for communication with Michael
// Yee's social network tool:

   string photo_network_message_queue_channel_name="photo_network";
   Messenger OBSFRUSTA_photo_network_messenger( 
      broker_URL,photo_network_message_queue_channel_name,message_sender_ID);

   string GPS_message_queue_channel_name="viewer_update";
   Messenger GPS_messenger( 
      broker_URL, GPS_message_queue_channel_name,message_sender_ID);

// Create OSG root node:

   osg::Group* root = new osg::Group;
   
// Instantiate Operations object to handle mode, animation and image
// number control:

   bool display_movie_state=false;
//   bool display_movie_state=true;
   bool display_movie_number=false;
//   bool display_movie_number=true;
   bool display_movie_world_time=false;
   bool hide_Mode_HUD_flag=false;
//   bool hide_Mode_HUD_flag=true;
   Operations operations(
      ndims,window_mgr_ptr,passes_group,
      display_movie_state,display_movie_number,display_movie_world_time,
      hide_Mode_HUD_flag);

   ModeController* ModeController_ptr=operations.get_ModeController_ptr();
   AnimationController* AnimationController_ptr=
      operations.get_AnimationController_ptr();
   AnimationController_ptr->setDelay(2);

   root->addChild(operations.get_OSGgroup_ptr());

// Add a custom manipulator to the event handler list:

   osgGA::Terrain_Manipulator* CM_3D_ptr=new osgGA::Terrain_Manipulator(
      ModeController_ptr,window_mgr_ptr);
   window_mgr_ptr->set_CameraManipulator(CM_3D_ptr);

// Instantiate group to hold all decorations:

   Decorations decorations(
      window_mgr_ptr,ModeController_ptr,CM_3D_ptr);

// Instantiate Grid:

   AlirtGrid* grid_ptr=decorations.add_AlirtGrid(
      ndims,passes_group.get_pass_ptr(cloudpass_ID));
   threevector* grid_origin_ptr=grid_ptr->get_world_origin_ptr();
   CM_3D_ptr->set_Grid_ptr(grid_ptr);
   decorations.set_grid_origin_ptr(grid_origin_ptr);

// Instantiate group to hold pointcloud information:

   PointCloudsGroup clouds_group(
      passes_group.get_pass_ptr(cloudpass_ID),grid_origin_ptr);
   clouds_group.generate_Clouds(passes_group);
   clouds_group.set_Terrain_Manipulator_ptr(CM_3D_ptr);
   clouds_group.set_auto_resize_points_flag(false);
   decorations.set_PointCloudsGroup_ptr(&clouds_group);

   window_mgr_ptr->get_EventHandlers_ptr()->push_back(
      new PointCloudKeyHandler(&clouds_group,ModeController_ptr,CM_3D_ptr));

// Initialize ALIRT grid based upon cloud's bounding box:

   osg::BoundingBox bbox=clouds_group.get_xyz_bbox();

   if (filefunc::getbasename(bundler_IO_subdir)=="Noah_MIT2328/" ||
       filefunc::getbasename(bundler_IO_subdir)=="MIT2328/")
   {

// FAKE FAKE:  hack for MIT2328 case...

      double z_grid=-450;	// meters
      decorations.get_AlirtGridsGroup_ptr()->initialize_grid(
         grid_ptr,bbox.xMin(),bbox.xMax(),bbox.yMin(),bbox.yMax(),z_grid);
   }
   else
   {
      decorations.get_AlirtGridsGroup_ptr()->initialize_grid(grid_ptr,bbox);
   }
   grid_ptr->set_delta_xy(1,1);
   cout << "*grid_origin_ptr = " << *grid_origin_ptr << endl;

// Instantiate a PointFinder;

   PointFinder pointfinder(&clouds_group);
   if (!passes_group.get_pick_points_on_Zplane_flag())
   {
      CM_3D_ptr->set_PointFinder(&pointfinder);
   }
   
// Instantiate a MyDatabasePager to handle paging of files from disk:

   viewer::MyDatabasePager* MyDatabasePager_ptr=new viewer::MyDatabasePager(
      clouds_group.get_SetupGeomVisitor_ptr(),
      clouds_group.get_ColorGeodeVisitor_ptr());
   clouds_group.get_HiresDataVisitor_ptr()->setDatabasePager(
      MyDatabasePager_ptr);

// Instantiate signposts, features, army symbol and sphere segments
// decoration groups:

   SignPostsGroup* SignPostsGroup_ptr=decorations.add_SignPosts(
      ndims,passes_group.get_pass_ptr(cloudpass_ID));

// Instantiate OBSFRUSTA decoration group:

   OBSFRUSTAGROUP* OBSFRUSTAGROUP_ptr=decorations.add_OBSFRUSTA(
      passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);
   OBSFRUSTAGROUP_ptr->pushback_Messenger_ptr(
      &OBSFRUSTA_photo_network_messenger);
//   OBSFRUSTAGROUP_ptr->set_enable_OBSFRUSTA_blinking_flag(true);
   OBSFRUSTAGROUP_ptr->set_enable_OBSFRUSTA_blinking_flag(false);
   OBSFRUSTAGROUP_ptr->set_erase_other_OBSFRUSTA_flag(true);
//   OBSFRUSTAGROUP_ptr->set_erase_other_OBSFRUSTA_flag(false);
   OBSFRUSTAGROUP_ptr->set_cameraid_xyz_map_ptr(cameraid_xyz_map_ptr);

// Instantiate an individual OBSFRUSTUM for every photograph:

   bool multicolor_frusta_flag=false;
   bool thumbnails_flag=true;
//   bool thumbnails_flag=false;
   OBSFRUSTAGROUP_ptr->generate_still_imagery_frusta_for_photogroup(
      photogroup_ptr,multicolor_frusta_flag,thumbnails_flag);

   decorations.get_OBSFRUSTUMPickHandler_ptr()->
      set_mask_nonselected_OSGsubPATs_flag(true);
   decorations.get_OBSFRUSTAKeyHandler_ptr()->set_SignPostsGroup_ptr(
      SignPostsGroup_ptr);

// Instantiate a virtual OBSFRUSTUM located at grid origin and
// oriented in canonical east direction:

   double horiz_FOV=30;
   double vert_FOV=25;
   double frustum_sidelength=100;
   OBSFRUSTAGROUP_ptr->instantiate_virtual_photo(
      horiz_FOV,vert_FOV,frustum_sidelength);

// Reposition and reorient virtual OBSFRUSTUM so that it stares
// towards big, complicated building nearby stadium:

   double z_ground=976;	// meters (in vicinity of Texas Tech)

   threevector ground_target_posn(233738,3720303,z_ground);
   double virtual_camera_height_above_ground=250;
   OBSFRUSTAGROUP_ptr->compute_virtual_OBSFRUSTA_for_circular_staring_orbit(
      ground_target_posn,virtual_camera_height_above_ground);

/*
   string path_filename="/media/10E8-0ED2/Bluegrass/ladar/osga_files/fused/";
   path_filename += "interpolated.path";
   OBSFRUSTAGROUP_ptr->virtual_tour_from_animation_path(
      path_filename,z_ground);
*/

// Instantiate second OBSFRUSTAGROUP to hold sub frusta:

   if (filefunc::ReadInfile(face_bbox_uv_filename))
   {
      OBSFRUSTAGROUP* SUBFRUSTAGROUP_ptr=new OBSFRUSTAGROUP(
         passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr,
         grid_origin_ptr);
      SUBFRUSTAGROUP_ptr->set_PARENT_OBSFRUSTAGROUP_ptr(OBSFRUSTAGROUP_ptr);
      SUBFRUSTAGROUP_ptr->set_photogroup_ptr(photogroup_ptr);
      decorations.get_OSGgroup_ptr()->
         addChild(SUBFRUSTAGROUP_ptr->get_OSGgroup_ptr());

// Instantiate bounding box for subfrustum:

      double u_lo=stringfunc::string_to_number(filefunc::text_line[0]);
      double u_hi=stringfunc::string_to_number(filefunc::text_line[1]);
      double v_lo=stringfunc::string_to_number(filefunc::text_line[2]);
      double v_hi=stringfunc::string_to_number(filefunc::text_line[3]);
      double region_height=stringfunc::string_to_number(
         filefunc::text_line[4]);
//      double region_width=stringfunc::string_to_number(
//         filefunc::text_line[5]);

      bounding_box subfrustum_bbox(u_lo,u_hi,v_lo,v_hi);
      subfrustum_bbox.set_physical_deltaY(region_height);

      SUBFRUSTAGROUP_ptr->set_subfrustum_bbox_ptr(&subfrustum_bbox);
      SUBFRUSTAGROUP_ptr->set_new_subfrustum_ID(0);
      OBSFRUSTAGROUP_ptr->set_selected_Graphical_ID(0);

// Instantiate MODEL decorations group to hold 3D human figure:

      MODELSGROUP* human_MODELSGROUP_ptr=decorations.add_MODELS(
         passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);
      human_MODELSGROUP_ptr->pushback_Messenger_ptr(&GPS_messenger);
      SUBFRUSTAGROUP_ptr->set_target_MODELSGROUP_ptr(human_MODELSGROUP_ptr);

   } // face_bbox_uv_filename successfully read conditional

// Instantiate Polygons & PolyLines decoration groups:

   osgGeometry::PolygonsGroup* PolygonsGroup_ptr=decorations.add_Polygons(
      ndims,passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);
   OBSFRUSTAGROUP_ptr->set_PolygonsGroup_ptr(PolygonsGroup_ptr);

   PolyLinesGroup* PolyLinesGroup_ptr=decorations.add_PolyLines(
      ndims,passes_group.get_pass_ptr(cloudpass_ID));
   PolygonsGroup_ptr->set_PolyLinesGroup_ptr(PolyLinesGroup_ptr);

   PolyLinesGroup_ptr->set_width(8);

   PolyLinePickHandler* PolyLinePickHandler_ptr=
      decorations.get_PolyLinePickHandler_ptr();
   PolyLinePickHandler_ptr->set_min_doubleclick_time_spread(0.05);
   PolyLinePickHandler_ptr->set_max_doubleclick_time_spread(0.25);
   PolyLinePickHandler_ptr->set_z_offset(5);

//   PolyLinePickHandler_ptr->set_form_polyhedron_skeleton_flag(true);
//   PolyLinePickHandler_ptr->set_form_polygon_flag(true);

   osgGeometry::PointsGroup* PointsGroup_ptr=decorations.add_Points(
      ndims,passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);
   OBSFRUSTAGROUP_ptr->set_PointsGroup_ptr(PointsGroup_ptr);

// Instantiate PhotoToursGroup:

   PhotoToursGroup* PhotoToursGroup_ptr=new PhotoToursGroup(
      OBSFRUSTAGROUP_ptr);
   PhotoToursKeyHandler* PhotoToursKeyHandler_ptr=
      new PhotoToursKeyHandler(
         PhotoToursGroup_ptr,OBSFRUSTAGROUP_ptr,&clouds_group,
         ModeController_ptr);
   window_mgr_ptr->get_EventHandlers_ptr()->push_back(
      PhotoToursKeyHandler_ptr);

// Attach all data and decorations to scenegraph:

   root->addChild(clouds_group.get_OSGgroup_ptr());
   root->addChild(decorations.get_OSGgroup_ptr());
   root->addChild(PhotoToursGroup_ptr->get_OSGgroup_ptr());

// Attach scene graph to the viewer:

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
