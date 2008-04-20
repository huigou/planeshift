/*
* Author: Andrew Robberts
*
* Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2 of the License)
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

#ifndef PS_EFFECT_LIGHT_HEADER
#define PS_EFFECT_LIGHT_HEADER

#include <csgeom/matrix3.h>
#include <csgeom/vector3.h>
#include <csutil/weakref.h>
#include <csutil/scf_implementation.h>
#include <iengine/movable.h>
#include <iengine/sector.h>

class csColor;
struct iLight;
struct iMeshWrapper;

class psLight
{
public:
    psLight();
    ~psLight();

    unsigned int AttachLight(csRef<iLight> newLight, csRef<iMeshWrapper> mw);
    bool Update();

private:
    csRef<iLight> light;
    csVector3 lightBasePos;
    csWeakRef<iMovable> movable;
};

#endif
