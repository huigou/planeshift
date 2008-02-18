/*
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/mesh.h>
#include <iengine/movable.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "engine/linmove.h"

#include "gem.h"
#include "npc.h"
#include "globals.h"
#include "networkmgr.h"
#include "util/consoleout.h"


//-------------------------------------------------------------------------------
psNPCClient *gemNPCObject::cel = NULL;

gemNPCObject::gemNPCObject( psNPCClient* cel, PS_ID id )
    :visible(true),invincible(false),instance(0)
{
    if (!this->cel)
        this->cel = cel;

    entity = cel->GetPlLayer()->CreateEntity(id);
}    

gemNPCObject::~gemNPCObject()
{
    cel->GetPlLayer()->RemoveEntity( entity );    
}

void gemNPCObject::Move(const csVector3& pos, float rotangle,  const char* room, int instance)
{
    SetInstance(instance);
    Move(pos,rotangle,room);
}


void gemNPCObject::Move(const csVector3& pos, float rotangle,  const char* room)
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (cel->GetObjectReg());

    // Position and sector
    iSector* sector = engine->FindSector(room);
    if ( sector == NULL )
        CPrintf(CON_DEBUG, "Sector %s not found!\n", room);
    pcmesh->MoveMesh( sector , pos );

    // Rotation
    csMatrix3 matrix = (csMatrix3) csYRotMatrix3 (rotangle);
    pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T (matrix);
   
}    

bool gemNPCObject::InitMesh(  
                                const char *factname,
                                const char *filename,
                                const csVector3& pos,
                                const float rotangle,
                                const char* room
                             )
{
    csRef<iCelPropertyClass> pc;
    
    pc = cel->GetPlLayer()->CreatePropertyClass(entity, "pcmesh");
    if ( !pc )
    {
        Error1("Could not create Item because pcmesh class couldn't be created.");
        return false;
    }

    pcmesh =  scfQueryInterface<iPcMesh > ( pc);
    if ( !pcmesh )
    {
        Error1("Could not create Item because pcmesh class doesn't implement iPcMesh.");
        return false;
    }

    if (!pcmesh->SetMesh(factname, filename))
    {
        Error3("Could not set mesh with factname=%s and filename=%s. Trying dummy model",factname,filename);                
        factname = "stonebm";
        filename = "/planeshift/models/stonebm/stonebm.cal3d";
        if ( !pcmesh->SetMesh(factname, filename) )
        {
            Error3("Could not use dummy CVS mesh with factname=%s and filename=%s",factname,filename);        
            return false;
        }
    }

    iMeshWrapper* mesh = pcmesh->GetMesh();
    if ( !mesh )
    {
        Error1("Could not create Item because pcmesh didn't have iMeshWrapper.");
        return false;
    }

    Move(pos,rotangle,room);

    return true;
}

void gemNPCObject::SetPosition(csVector3& pos, iSector* sector, int* instance)
{
    psGameObject::SetPosition(GetEntity(), pos, sector);

    if (instance)
    {
        SetInstance(*instance);
    }
}

iMeshWrapper *gemNPCObject::GetMeshWrapper()
{
    return pcmesh->GetMesh();
}

//-------------------------------------------------------------------------------
 

gemNPCActor::gemNPCActor( psNPCClient* cel, psPersistActor& mesg) 
    : gemNPCObject( cel, mesg.entityid ), npc(NULL)
{
    name = mesg.name;
    entity->SetName( mesg.name );
    id = mesg.entityid;
    type = mesg.type;
    playerID = mesg.playerID;
    ownerEID = mesg.ownerEID;
    race = mesg.race;

    SetVisible( ! (mesg.flags & psPersistActor::INVISIBLE)?  true : false );
    SetInvincible( (mesg.flags & psPersistActor::INVINCIBLE) ?  true : false );
    SetInstance( mesg.instance );

    Debug3( LOG_CELPERSIST, id, "Actor %s(%u) Received\n", mesg.name.GetData(), mesg.entityid );
    InitMesh(  mesg.factname, mesg.filename, mesg.pos, mesg.yrot, mesg.sectorName );
    InitLinMove( mesg.pos, mesg.yrot, mesg.sectorName, mesg.top, mesg.bottom, mesg.offset );
    InitCharData( mesg.texParts, mesg.equipment );  
}

gemNPCActor::~gemNPCActor()
{
    if (npc)
    {
        npcclient->UnattachNPC(entity,npc);
        npc->ClearState();
        npc->SetActor(NULL);
        npc = NULL;
    }
}

void gemNPCActor::AttachNPC(NPC * newNPC)
{
    npc = newNPC;
    npc->SetActor(this);
    npcclient->AttachNPC(entity,npc);
    npc->SetAlive(true);
}


bool gemNPCActor::InitCharData( const char* textParts, const char* equipment )
{                   
    return true;    
}    

bool gemNPCActor::InitLinMove(const csVector3& pos, float angle, const char* sector,
                              csVector3 top, csVector3 bottom, csVector3 offset )
{
    csRef<iCelPropertyClass> pc;
    pc = cel->GetPlLayer()->CreatePropertyClass(entity, "pclinearmovement");
    if ( !pc )
    {
        Error1("Could not create property class pclinearmovement.  Actor not created.");
        return false;
    }
    pcmove =  new psLinearMovement(cel->GetObjectReg());

    csRef<iEngine> engine =  csQueryRegistry<iEngine> (cel->GetObjectReg());

    pcmove->InitCD(top, bottom, offset, GetMeshWrapper()); 
    pcmove->SetPosition(pos,angle,engine->FindSector(sector));
    
    return true;  // right now this func never fail, but might later.
}


gemNPCItem::gemNPCItem( psNPCClient* cel, psPersistItem& mesg) 
    : gemNPCObject( cel, mesg.id ), flags(NONE)
{        
    name = mesg.name;
    Debug3( LOG_CELPERSIST, 0,"Item %s(%d) Received", mesg.name.GetData(), mesg.id );
    entity->SetName( mesg.name );
    id = mesg.id;
    type = mesg.type;
    InitMesh(  mesg.factname, mesg.filename, mesg.pos, mesg.yRot, mesg.sector );
    if (mesg.flags & psPersistItem::NOPICKUP) flags |= NOPICKUP;
}

//Here we check the flag to see if we can pick up this item
bool gemNPCItem::IsPickable() { return !(flags & NOPICKUP); }

gemNPCItem::~gemNPCItem()
{
}
