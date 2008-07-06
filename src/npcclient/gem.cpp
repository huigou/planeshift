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
#include "util/psxmlparser.h"


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
    {
        CPrintf(CON_DEBUG, "Can't move npc object %d. Sector %s not found!\n", GetID(),room);
        return;
    }
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

    // Replace helm group token with the default race.
    psString fact_name(factname);
    fact_name.ReplaceAllSubString("$H", "stonebm");
    factname = fact_name;

    psString file_name(filename);
    file_name.ReplaceAllSubString("$H", "stonebm");
    filename = file_name;

    csRef<iEngine> engine = csQueryRegistry<iEngine> (npcclient->GetObjectReg());
    csRef<iVFS> vfs = csQueryRegistry<iVFS> (npcclient->GetObjectReg());

    csRef<iMeshWrapper> mesh;
    csRef<iMeshFactoryWrapper> meshFact = engine->GetMeshFactories()->FindByName(factname);
    if(meshFact.IsValid())
    {
        mesh = meshFact->CreateMeshWrapper();
    }
    
    if(!mesh.IsValid())
    {
        bool failed = true;
        if(vfs->Exists(filename))
        {
            failed = false;
            while(!failed)
            {
                csRef<iDocument> doc = ParseFile(npcclient->GetObjectReg(), filename);
                if (!doc)
                {
                    Error2("Couldn't parse file %s", filename);
                    failed = true;
                    break;
                }

                csRef<iDocumentNode> root = doc->GetRoot();
                if (!root)
                {
                    Error2("The file(%s) doesn't have a root", filename);
                    failed = true;
                    break;
                }

                csRef<iDocumentNode> meshNode;
                csRef<iDocumentNode> libNode = root->GetNode("library");
                if (libNode)
                {
                    meshNode = libNode->GetNode("meshfact");

                    if(libNode->GetNode("shaders"))
                        libNode->RemoveNode(libNode->GetNode("shaders"));

                    if(libNode->GetNode("textures"))
                        libNode->RemoveNode(libNode->GetNode("textures"));

                    if(libNode->GetNode("materials"))
                        libNode->RemoveNode(libNode->GetNode("materials"));
                }
                else
                    meshNode = root->GetNode("meshfact");
                if (!meshNode)
                {
                    Error2("The file(%s) doesn't have a meshfact node", filename);
                    failed = true;
                    break;
                }

                csRef<iDocumentNode> params = meshNode->GetNode("params");
                if(meshNode->GetNode("params"))
                {
                    if(params->GetNode("material"))
                        params->RemoveNode(params->GetNode("material"));

                    params->RemoveNodes(params->GetNodes("animation"));

                    csRef<iDocumentNodeIterator> meshes = params->GetNodes("mesh");
                    while(meshes->HasNext())
                    {
                        csRef<iDocumentNode> mesh = meshes->Next();
                        csRef<iDocumentAttribute> att = mesh->GetAttribute("material");
                        mesh->RemoveAttribute(mesh->GetAttribute("material"));
                    }
                }

                csRef<iLoader> loader (csQueryRegistry<iLoader> (npcclient->GetObjectReg()));
                loader->Load(root);
                meshFact = engine->GetMeshFactories()->FindByName(factname);
                if(meshFact.IsValid())
                {
                    mesh = meshFact->CreateMeshWrapper();
                }
                failed = !mesh.IsValid();
                break;
            }
        }

        if(failed)
        {
            Error3("Could not set mesh with factname=%s and filename=%s. Trying dummy model", factname, filename);                
            factname = "stonebm";
            filename = "/planeshift/models/stonebm/stonebm.cal3d";
            if (!pcmesh->SetMesh(factname, filename))
            {
                Error3("Could not use dummy CVS mesh with factname=%s and filename=%s", factname, filename);        
                return false;
            }
        }
    }

    if(!pcmesh->GetMesh())
    {
        pcmesh->SetMesh(mesh);
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

    if(!mesg.factname.GetData())
    {
        Error2("Item %s has bad data! Check cstr_id_gfx_mesh for this item!\n", mesg.name.GetData());
    }

    InitMesh(  mesg.factname.GetDataSafe(), mesg.filename.GetDataSafe(), mesg.pos, mesg.yRot, mesg.sector );
    if (mesg.flags & psPersistItem::NOPICKUP) flags |= NOPICKUP;
}

//Here we check the flag to see if we can pick up this item
bool gemNPCItem::IsPickable() { return !(flags & NOPICKUP); }

gemNPCItem::~gemNPCItem()
{
}
