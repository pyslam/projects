// ==========================================================================
// DT_list class member function definitions
// ==========================================================================
// Last modified on 12/14/05
// ==========================================================================

#include "delaunay/DT_list.h"
#include "delaunay/DT_node.h"

// ---------------------------------------------------------------------
DT_list::DT_list(DT_list* l, DT_node* k) 
{
   next=l; 
   key=k;
}
