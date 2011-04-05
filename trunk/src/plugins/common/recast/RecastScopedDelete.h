/*
    Crystal Space Entity Layer
    Copyright (C) 2009 by Jorrit Tyberghein
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <csutil/scopeddelete.h>
#include "recastnavigation/Recast.h"

#ifndef __CEL_NAVMESH_SCOPEDDELETE__
#define __CEL_NAVMESH_SCOPEDDELETE__

namespace CS
{
namespace Utility
{
#define IMPLEMENT_SCOPEDDELETE(T,U)\
  template<> class ScopedDelete< rc ## T > : private NonCopyable\
  {\
    rc ## T * ptr;\
  public:\
    ScopedDelete( rc ## T * ptr) : ptr (ptr) {}\
    ~ScopedDelete() { Free(); }\
    rc ## T * operator -> () const { return ptr; }\
    operator rc ## T * () const { return ptr; }\
    rc ## T & operator* () const { return *ptr; }\
    void Free() { if(ptr) rcFree ## U(ptr); ptr = 0; }\
  }

  IMPLEMENT_SCOPEDDELETE(Heightfield,HeightField);
  IMPLEMENT_SCOPEDDELETE(CompactHeightfield,CompactHeightfield);
  IMPLEMENT_SCOPEDDELETE(ContourSet,ContourSet);
  IMPLEMENT_SCOPEDDELETE(PolyMesh,PolyMesh);
  IMPLEMENT_SCOPEDDELETE(PolyMeshDetail,PolyMeshDetail);

#undef IMPLEMENT_SCOPEDDELETE
}
}

#endif
