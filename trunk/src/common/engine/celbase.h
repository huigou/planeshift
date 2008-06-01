/*
 * celbase.h - author Matze Braun <MatzeBraun@gmx.de>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __CELBASE_H__
#define __CELBASE_H__

#ifdef USE_CEL

#include <csutil/ref.h>
#include <csutil/leakguard.h>

// This list of includes means that any source which uses CEL only has to 
// include one file->this one.
#include <physicallayer/pl.h>
#include <behaviourlayer/bl.h>
#include <physicallayer/entity.h>
#include <physicallayer/propfact.h>
#include <physicallayer/propclas.h>
#include <propclass/mesh.h>
#include <propclass/mesh.h>
#include <propclass/meshsel.h>
#include <propclass/inv.h>
#include <propclass/chars.h>
#include <propclass/move.h>
#include <propclass/tooltip.h>
#include <propclass/camera.h>
#include <propclass/gravity.h>
#include <propclass/timer.h>
#include <propclass/region.h>
#include <propclass/input.h>

#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <iengine/sector.h>
#include <imap/loader.h>
#include <imesh/object.h>
#include <iutil/plugin.h>
#include <ivideo/txtmgr.h>

struct iObjectRegistry;

/**
 * Base Class for cel managers in server and client.
 * This class mainly handles loading of cel plugins and managing of common cel
 * resources
 */
class CelBase
{
public:
    CelBase();
    virtual ~CelBase();

    virtual bool Initialize (iObjectRegistry* object_reg);
     
    /// Remove the entity from the actors list inventory.
    bool RemoveActor(iCelEntity* actor);
    
    /// Removes the entity from the world's inventory.
    bool RemoveItem( iCelEntity* item);
    
    iCelPlLayer* GetPlLayer()    { return pl; }
    
    iObjectRegistry* GetObjectReg()    { return object_reg; }
    
protected:
    bool LoadPlugin (const char* pcfactname);

    iObjectRegistry*        object_reg;
    csRef<iCelPlLayer>      pl;
    csRef<iPluginManager>   pluginMgr;
    csRef<iCelEntity>       actors;
};

#endif // USE_CEL

#endif

