// ========================================================================
// SetupGeomVisitor class member function definitions
// ========================================================================
// Last updated on 5/2/07; 11/27/11; 12/2/11; 12/28/11
// ========================================================================

#include <osg/Geometry>
#include <osg/StateSet>
#include "osg/osgSceneGraph/MyNodeInfo.h"
#include "osg/osgSceneGraph/scenegraphfuncs.h"
#include "osg/osgSceneGraph/SetupGeomVisitor.h"

#include "osg/osgfuncs.h"

using std::cout;
using std::endl;

// ---------------------------------------------------------------------
// Initialization, constructor and destructor functions:
// ---------------------------------------------------------------------

void SetupGeomVisitor::allocate_member_objects()
{
}

void SetupGeomVisitor::initialize_member_objects()
{
   Visitor_name="SetupGeomVisitor";
   MyHyperFilter_ptr=NULL;
}

SetupGeomVisitor::SetupGeomVisitor(osg::StateSet* SS_ptr):
   MyNodeVisitor()
{ 
//   cout << "inside SetupGeomVisitor constructor, this = " << this << endl;
   allocate_member_objects();
   initialize_member_objects();
   StateSet_refptr=SS_ptr;
} 

SetupGeomVisitor::~SetupGeomVisitor()
{
}

// ========================================================================
// Set & get member functions
// ========================================================================

void SetupGeomVisitor::set_MyHyperFilter_ptr(model::MyHyperFilter* hf_ptr)
{
   MyHyperFilter_ptr=hf_ptr;
}

void SetupGeomVisitor::add_HyperFilter_Callback()
{
//   cout << "inside SetupGeomVisitor::add_HyperFilter_Callback()" << endl;

   osg::NodeCallback* UpdateCallback_ptr=MyHyperFilter_ptr->
      get_UpdateCallback_ptr();
   addCullCallback(UpdateCallback_ptr);
}

// ------------------------------------------------------------------------
void SetupGeomVisitor::apply(osg::Node& currNode) 
{ 
//   cout << "inside SetupGeomVisitor::apply(Node), class = " 
//        << currNode.className() << endl;
   MyNodeVisitor::apply(currNode);
} 

// ------------------------------------------------------------------------
// This overloaded version of apply takes in a Geode which may either
// be a static resident in memory or may have been recently paged into
// memory.  It attempts to retrieve a Geometry containing vertices
// from the Geode and set some of its parameters (e.g. StateSet).  It
// also checks if the input color array is null.  If so, this method
// sets the input geometry's mutable color label string so that the
// geometry will be colored by subsequent ColorGeodeVisitor calls.

void SetupGeomVisitor::apply(osg::Geode& currGeode) 
{ 
//   cout << "Inside SetupGeomVisitor::apply(Geode)" << endl;
//   cout << "Geode = " << &currGeode << endl;
//   cout << "classname = " << currGeode.className() << endl;
//   cout << "LocalToWorld = " << endl;
//   osgfunc::print_matrix(LocalToWorld);

   osg::Geometry* curr_Geometry_ptr=scenegraphfunc::get_geometry(&currGeode);

   if (curr_Geometry_ptr != NULL)
   {
      model::MyNodeInfo* curr_nodeinfo_ptr=
         model::getOrCreateMyInfoForNode(currGeode);
      curr_nodeinfo_ptr->set_transform(LocalToWorld);

      osg::Vec4ubArray* curr_colors_ptr=dynamic_cast<osg::Vec4ubArray*>(
         curr_Geometry_ptr->getColorArray());
      if (curr_colors_ptr==NULL)
      {
         curr_Geometry_ptr->setName(
            scenegraphfunc::get_mutable_colors_label());
      }

// Recall StateSet_ptr is initialized to NULL within DataGraph class
// and remains NULL for any DataGraph which is read in from an OSG
// generated file (e.g. earth.osga).  For DataGraphs generated by
// osgdem (e.g. baghdad EO surface textures), geometry stateset
// information is already contained within the input .osga files and
// should not be overwritten.  

// As of 8/11/06, we should only execute the following setStateSet
// command for DataGraphs which are actually PointClouds and whose
// geometries initially have null StateSet pointers:

      if (curr_Geometry_ptr->getStateSet()==NULL &&
          StateSet_refptr.valid())
      {
         curr_Geometry_ptr->setStateSet(StateSet_refptr.get());
      }
      
// Next set statement avoids caching on GPU whose memory is not large
// enough to hold interestingly sized data sets:

      curr_Geometry_ptr->setUseDisplayList( false );	
      
// Next set statement instructs OSG to try to send an entire vertex
// array to the GPU at once rather than the individual vertices one at
// a time:

      curr_Geometry_ptr->setFastPathHint( true );	
   
   } // curr_Geometry_ptr != NULL conditional

   MyNodeVisitor::apply(currGeode);
}
