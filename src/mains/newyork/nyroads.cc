// ========================================================================
// Program NYROADS is a variant of program ROADS which is a lab for
// incorporating NYC roads into the Manhattan ladar sets.

// 	nyroads --region_filename ./packages/nyc_rtv_EO.pkg --GIS_layer ./packages/nyc_streets.pkg

//   nyroads --region_filename ./packages/nyc_rtv_EO.pkg --region_filename ./packages/empire.pkg --region_filename ./packages/taxis.pkg --GIS_layer ./packagesnyc_streets.pkg

// ========================================================================
// Last updated on 5/18/08; 5/11/10; 5/12/10; 12/4/10
// ========================================================================

#include <iostream>
#include <string>
#include <vector>
#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "osg/osgGraphicals/AnimationController.h"
#include "osg/osgGraphicals/CentersGroup.h"
#include "osg/osgGraphicals/CentersKeyHandler.h"
#include "osg/osgGraphicals/CenterPickHandler.h"
#include "astro_geo/Clock.h"
#include "osg/osgOrganization/Decorations.h"
#include "osg/osgEarth/EarthRegionsGroup.h"
#include "osg/osgGrid/GridKeyHandler.h"
#include "osg/osgGrid/LatLongGridsGroup.h"
#include "osg/ModeController.h"
#include "osg/osgModels/ModelsGroup.h"
#include "osg/osgSceneGraph/MyDatabasePager.h"
#include "osg/osgModels/ObsFrustaGroup.h"
#include "osg/osgOperations/Operations.h"
#include "passes/PassesGroup.h"
#include "osg/osg3D/PointCloudsGroup.h"
#include "osg/osg3D/PointCloudKeyHandler.h"
#include "osg/osgGraphicals/PointFinder.h"
#include "osg/osgGeometry/PolyLinesGroup.h"
#include "osg/osgGeometry/PolyLinesKeyHandler.h"
#include "osg/osgGIS/postgis_databases_group.h"
#include "osg/osg3D/Terrain_Manipulator.h"
#include "passes/TextDialogBox.h"
#include "time/timefuncs.h"
#include "osg/osgWindow/ViewerManager.h"

#include "general/outputfuncs.h"
#include "templates/mytemplates.h"

// ==========================================================================
int main( int argc, char** argv )
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::string;
   using std::vector;
   std::set_new_handler(sysfunc::out_of_memory);

// Use an ArgumentParser object to manage the program arguments:

   osg::ArgumentParser arguments(&argc,argv);
   const int ndims=3;
   PassesGroup passes_group(&arguments);
   int cloudpass_ID=passes_group.get_curr_cloudpass_ID();
   vector<int> GISlayer_IDs=passes_group.get_GISlayer_IDs();

   for (int g=0; g<GISlayer_IDs.size(); g++)
   {
      cout << "g = " << g << " GISlayer_IDs[g] = " << GISlayer_IDs[g]
           << endl;
   }
   

// Instantiate clock to keep track of real time:

   Clock clock;
   clock.current_local_time_and_UTC();

// Instantiate postgis database objects to send data to and retrieve
// data from external Postgres database:

   postgis_databases_group* postgis_databases_group_ptr=
      new postgis_databases_group;
   postgis_database* postgis_db_ptr=postgis_databases_group_ptr->
      generate_postgis_database_from_GISlayer_IDs(passes_group,GISlayer_IDs);

// Construct the viewer and instantiate a ViewerManager:

   WindowManager* window_mgr_ptr=new ViewerManager();
   window_mgr_ptr->initialize_window("3D imagery");
//   window_mgr_ptr->getUsage(*arguments.getApplicationUsage());

// Create OSG root node:

   osg::Group* root = new osg::Group;

// Instantiate Operations object to handle mode, animation and image
// number control:

   Operations operations(ndims,window_mgr_ptr);

   ModeController* ModeController_ptr=operations.get_ModeController_ptr();
   AnimationController* AnimationController_ptr=
      operations.get_AnimationController_ptr();
   root->addChild(operations.get_OSGgroup_ptr());

// Add a custom manipulator to the event handler list:

   osgGA::Terrain_Manipulator* CM_3D_ptr=new osgGA::Terrain_Manipulator(
      ModeController_ptr,window_mgr_ptr);
   window_mgr_ptr->set_CameraManipulator(CM_3D_ptr);

// Instantiate group to hold all decorations:
   
   Decorations decorations(window_mgr_ptr,ModeController_ptr,CM_3D_ptr);

// Instantiate groups to hold multiple point clouds,
// latitude-longitude grids and associated earth regions:

   PointCloudsGroup clouds_group(
      passes_group.get_pass_ptr(cloudpass_ID));
   LatLongGridsGroup latlonggrids_group(
      ndims,passes_group.get_pass_ptr(cloudpass_ID),CM_3D_ptr);
   EarthRegionsGroup earth_regions_group(
      passes_group.get_pass_ptr(cloudpass_ID),      
      &clouds_group,&latlonggrids_group);
   earth_regions_group.generate_regions(passes_group);

   LatLongGrid* latlonggrid_ptr=latlonggrids_group.get_Grid_ptr(0);
   threevector* grid_origin_ptr=latlonggrid_ptr->get_world_origin_ptr();
   CM_3D_ptr->set_Grid_ptr(latlonggrid_ptr);
   decorations.set_grid_origin_ptr(grid_origin_ptr);

   decorations.set_PointCloudsGroup_ptr(&clouds_group);

   window_mgr_ptr->get_EventHandlers_ptr()->push_back(
      new PointCloudKeyHandler(&clouds_group,ModeController_ptr));
   window_mgr_ptr->get_EventHandlers_ptr()->push_back(
      new GridKeyHandler(ModeController_ptr,latlonggrid_ptr));

// Instantiate a PointFinder;

   PointFinder pointfinder(&clouds_group);
   CM_3D_ptr->set_PointFinder(&pointfinder);

// Instantiate a MyDatabasePager to handle paging of files from disk:

   viewer::MyDatabasePager* MyDatabasePager_ptr=new viewer::MyDatabasePager(
      clouds_group.get_SetupGeomVisitor_ptr(),
      clouds_group.get_ColorGeodeVisitor_ptr());

// Instantiate signposts, features, army symbol and sphere segments
// decoration groups:

   SignPostsGroup* SignPostsGroup_ptr=decorations.add_SignPosts(
      ndims,passes_group.get_pass_ptr(cloudpass_ID));
   SignPostsGroup_ptr->set_postgis_databases_group_ptr(
      postgis_databases_group_ptr);

   for (int n=0; n<decorations.get_n_SignPostsGroups(); n++)
   {
      decorations.get_SignPostsGroup_ptr(n)->set_EarthRegionsGroup_ptr(
         &earth_regions_group);
      decorations.get_SignPostsGroup_ptr(n)->set_Clock_ptr(&clock);
   }

   decorations.add_Features(ndims,passes_group.get_pass_ptr(cloudpass_ID));
   decorations.get_FeaturesGroup_ptr()->set_EarthRegionsGroup_ptr(
      &earth_regions_group);
   decorations.add_ArmySymbols(passes_group.get_pass_ptr(cloudpass_ID));

   ObsFrustaGroup* ObsFrustaGroup_ptr=decorations.add_ObsFrusta(
      passes_group.get_pass_ptr(cloudpass_ID),AnimationController_ptr);

   bool multicolor_frusta_flag=true;
   bool initially_mask_all_frusta_flag=true;
   ObsFrustaGroup_ptr->generate_still_imagery_frusta(
      passes_group,multicolor_frusta_flag,initially_mask_all_frusta_flag);

// Instantiate PolyLines decorations group to hold roadways:

   decorations.add_PolyLines(ndims,passes_group.get_pass_ptr(cloudpass_ID));
   PolyLinesGroup* roadlines_group_ptr=decorations.get_PolyLinesGroup_ptr();
   roadlines_group_ptr->set_width(3);
   window_mgr_ptr->get_EventHandlers_ptr()->push_back(
      new PolyLinesKeyHandler(roadlines_group_ptr,ModeController_ptr));
   roadlines_group_ptr->set_OSGgroup_nodemask(0);

   postgis_db_ptr->set_PolyLinesGroup_ptr(roadlines_group_ptr);
   postgis_db_ptr->setup_coordinate_transformation("WGS84",-1,18,true);

//   const double xmin = 582789;	// meters
//   const double xmax = 590686;	// meters
//   const double ymin = 4.50593e+06;	// meters
//   const double ymax = 4.52276e+06;	// meters

   double xmin=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().xMin();
   double xmax=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().xMax();
   double ymin=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().yMin();
   double ymax=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().yMax();
   double zmin=clouds_group.get_Cloud_ptr(0)->get_xyz_bbox().zMin();

   double delta_x=xmax-xmin;
   double delta_y=ymax-ymin;
   xmin += 0.4*delta_x;
   xmax -= 0.4*delta_x;
   ymin += 0.4*delta_y;
   ymax -= 0.4*delta_y;

   postgis_db_ptr->pushback_gis_bbox(xmin,xmax,ymin,ymax);
   postgis_db_ptr->set_altitude(zmin+10);	// meters

   string geom_name="name";
   string where_clause="borough='Manhattan' and feat_desc='Paved Road'";
   postgis_db_ptr->parse_table_contents(geom_name,where_clause);

   const osg::Vec4 road_color(1,1,1,1);
   roadlines_group_ptr->set_uniform_color(road_color);
   roadlines_group_ptr->attach_bunched_geometries_to_OSGsubPAT(
      *grid_origin_ptr,0);

//    roadlines_group_ptr->assign_heights_using_pointcloud(&pointfinder);

//   set_altitude_dependent_polyline_width();
   roadlines_group_ptr->update_display();

   cout << "# polylines = " 
        << roadlines_group_ptr->get_n_Graphicals() << endl;

// Instantiate CentersGroup and center pick handler to handle mid
// point selection:

   CentersGroup centers_group(
      ndims,passes_group.get_pass_ptr(cloudpass_ID),
      AnimationController_ptr,grid_origin_ptr);
   CenterPickHandler* CenterPickHandler_ptr=new CenterPickHandler(
      ndims,passes_group.get_pass_ptr(cloudpass_ID),
      CM_3D_ptr,&centers_group,ModeController_ptr,window_mgr_ptr,
      grid_origin_ptr);
   window_mgr_ptr->get_EventHandlers_ptr()->push_back(
      CenterPickHandler_ptr);
   window_mgr_ptr->get_EventHandlers_ptr()->push_back(
      new CentersKeyHandler(ndims,CenterPickHandler_ptr,ModeController_ptr));
   root->addChild(centers_group.get_OSGgroup_ptr());

// Attach all 3D graphicals to CentersGroup SpinTransform which can
// perform global rotation about some axis coming out of a user
// selected center location:

   centers_group.get_SpinTransform_ptr()->addChild(
      decorations.get_OSGgroup_ptr());
   centers_group.get_SpinTransform_ptr()->addChild(
      clouds_group.get_OSGgroup_ptr());
   centers_group.get_SpinTransform_ptr()->addChild(
      latlonggrids_group.get_OSGgroup_ptr());
   root->addChild(centers_group.get_SpinTransform_ptr());

// Attach scene graph to the viewer:

   window_mgr_ptr->setSceneData(root);

// Open text dialog box to display feature information:

//   decorations.get_FeaturesGroup_ptr()->get_TextDialogBox_ptr()->
//      open("Feature Information");
//   decorations.get_FeaturesGroup_ptr()->update_feature_text();

// Create the windows and run the threads.  Viewer's realize method
// calls the CUstomManipulator's home() method:

   window_mgr_ptr->realize();

// altitude according to grid's maximal linear dimension:

   CM_3D_ptr->set_worldspace_center(latlonggrid_ptr->get_world_middle());
   CM_3D_ptr->set_eye_to_center_distance(
      2*basic_math::max(
         latlonggrid_ptr->get_xsize(),latlonggrid_ptr->get_ysize()));
   CM_3D_ptr->update_M_and_Minv();

//   timefunc::initialize_timeofday_clock();
//   osg::FrameStamp* FrameStamp_ptr=window_mgr_ptr->getFrameStamp();

//   cout << "Before entering infinite viewer loop" << endl;
//   outputfunc::enter_continue_char();

//   clouds_group.set_OSGgroup_nodemask(0);

   while( !window_mgr_ptr->done() )
   {
      window_mgr_ptr->process();
   }
   
   delete postgis_databases_group_ptr;

   delete window_mgr_ptr;
}
