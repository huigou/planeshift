/*
 * celbase.cpp - author Matze Braun <MatzeBraun@gmx.de>
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
#include <psconfig.h>

#include "engine/celbase.h"
#include "util/consoleout.h"
#include "util/log.h"

#define QUERYPLUG(var, intf, name)        \
    var =  csQueryRegistry<intf> (object_reg);    \
    if (!var)\
    {                    \
        CPrintf (CON_ERROR, "CelBase: Couldn't find plugin for " name "\n"); \
        return false;                \
    }

CelBase::CelBase()
{
    pl = NULL;    
}

CelBase::~CelBase()
{
    if (pl)
    {
        pl->CleanCache();
        object_reg->Unregister (pl, "iCelPlLayer");
    }
}

static const char* ps_propfactlist[] = {
    "cel.pcfactory.movable",
    "cel.pcfactory.solid",
    "cel.pcfactory.movableconst_cd",
    "cel.pcfactory.gravity",
    "cel.pcfactory.region",
    "cel.pcfactory.defaultcamera",
    "cel.pcfactory.mesh",
    "cel.pcfactory.meshselect",
    "cel.pcfactory.pccommandinput",     
    "cel.pcfactory.linearmovement",
    "cel.pcfactory.collisiondetection",
    0
};

bool CelBase::Initialize(iObjectRegistry* object_reg)
{
    CelBase::object_reg = object_reg;

    QUERYPLUG(pluginMgr,iPluginManager, "iPluginManager");
    
    pl = CS_LOAD_PLUGIN(pluginMgr, "cel.physicallayer", iCelPlLayer);
    if (!pl) 
    {
        CPrintf (CON_ERROR, "Couldn't load plugin for PlLayer.\n");
        return false;
    }
    object_reg->Register (pl, "iCelPlLayer");


    // load plugins
    for (const char** pfname=ps_propfactlist; *pfname; pfname++)
    {
        if (!LoadPlugin (*pfname))
            return false;
    }    

    return true;
}

bool CelBase::LoadPlugin (const char* pcfactname)
{
    csRef<iBase> plug = CS_LOAD_PLUGIN_ALWAYS(pluginMgr, pcfactname);
    if (!plug)
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
            "planeshift.engine.celbase",
            "CEL '%s' plugin missing!", pcfactname);
        return false;
    }

    return true;
}

bool CelBase::RemoveActor(iCelEntity *entity)
{
    Debug3(LOG_CELPERSIST,0,"Removing ACTOR: <%s> [%u] from world\n", entity->GetName(), entity->GetID());

    pl->RemoveEntity( entity );
    return true;
}

bool CelBase::RemoveItem( iCelEntity* item )
{
    return RemoveActor(item);
}
