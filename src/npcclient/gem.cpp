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
#include <iengine/engine.h>
#include <imap/loader.h>
#include <imesh/nullmesh.h>
#include <imesh/object.h>

//=============================================================================
// Project Library Includes
//=============================================================================
#include "util/consoleout.h"
#include "util/psxmlparser.h"

#include "engine/linmove.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "gem.h"
#include "npc.h"
#include "npcmesh.h"
#include "networkmgr.h"

//-----------------------------------------------------------------------------

csRef<iMeshFactoryWrapper> gemNPCObject::nullfact = NULL;

psNpcMeshAttach::psNpcMeshAttach(gemNPCObject* objectToAttach) : scfImplementationType(this)
{
    object = objectToAttach;
}


//-------------------------------------------------------------------------------

gemNPCObject::gemNPCObject(psNPCClient* npcclient, EID id)
    :eid(id), visible(true), invincible(false), instance(DEFAULT_INSTANCE)
{
    this->npcclient = npcclient;

}    

gemNPCObject::~gemNPCObject()
{
    delete pcmesh;
}

void gemNPCObject::Move(const csVector3& pos, float rotangle,  const char* room, InstanceID instance)
{
    SetInstance(instance);
    Move(pos,rotangle,room);
}


void gemNPCObject::Move(const csVector3& pos, float rotangle,  const char* room)
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (npcclient->GetObjectReg());

    // Position and sector
    iSector* sector = engine->FindSector(room);
    if ( sector == NULL )
    {
        CPrintf(CON_DEBUG, "Can't move npc object %s. Sector %s not found!\n", ShowID(eid), room);
        return;
    }
    pcmesh->MoveMesh( sector , pos );

    // Rotation
    csMatrix3 matrix = (csMatrix3) csYRotMatrix3 (rotangle);
    pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T (matrix);
   
}    

bool gemNPCObject::InitMesh(    const char *factname,
                                const char *filename,
                                const csVector3& pos,
                                const float rotangle,
                                const char* room
                             )
{
    pcmesh = new npcMesh(npcclient->GetObjectReg(), this, npcclient);

    csRef<iEngine> engine = csQueryRegistry<iEngine> (npcclient->GetObjectReg());    
    csRef<iMeshWrapper> mesh;

    if(csString("").Compare(filename))
    {
        if(!nullfact)
        {
            nullfact = engine->CreateMeshFactory("crystalspace.mesh.object.null", "Dummy", false);
            csRef<iNullFactoryState> nullstate = scfQueryInterface<iNullFactoryState> (nullfact->GetMeshObjectFactory());
            csBox3 bbox;
            bbox.AddBoundingVertex(pos);
            nullstate->SetBoundingBox(bbox);
        }

        mesh = engine->CreateMeshWrapper(nullfact, name);
    }
    else
    {
        // Replace helm group token with the default race.
        psString fact_name(factname);
        fact_name.ReplaceAllSubString("$H", "stonebm");
        factname = fact_name;

        psString file_name(filename);
        file_name.ReplaceAllSubString("$H", "stonebm");
        filename = file_name;

        csRef<iVFS> vfs = csQueryRegistry<iVFS> (npcclient->GetObjectReg());

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

                    csRef<iThreadedLoader> loader (csQueryRegistry<iThreadedLoader> (npcclient->GetObjectReg()));
                    csRef<iThreadReturn> ret = loader->LoadNode(root);
                    ret->Wait();
                    engine->SyncEngineListsNow(loader);
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
    }

    if(!pcmesh->GetMesh())
    {
        pcmesh->SetMesh(mesh);
    }

    Move(pos,rotangle,room);

    return true;
}

void gemNPCObject::SetPosition(csVector3& pos, iSector* sector, InstanceID* instance)
{
    psGameObject::SetPosition(this, pos, sector);

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
 

gemNPCActor::gemNPCActor( psNPCClient* npcclient, psPersistActor& mesg) 
    : gemNPCObject( npcclient, mesg.entityid ), npc(NULL)
{
    name = mesg.name;
    type = mesg.type;
    playerID = mesg.playerID;
    ownerEID = mesg.ownerEID;
    race = mesg.race;

    SetVisible( ! (mesg.flags & psPersistActor::INVISIBLE)?  true : false );
    SetInvincible( (mesg.flags & psPersistActor::INVINCIBLE) ?  true : false );
    SetInstance( mesg.instance );

    Debug3(LOG_CELPERSIST, eid.Unbox(), "Actor %s(%s) Received\n", mesg.name.GetData(), ShowID(mesg.entityid));

    csString filename;
    filename.Format("/planeshift/models/%s/%s.cal3d", mesg.factname.GetData(), mesg.factname.GetData());
    InitMesh(  mesg.factname, filename, mesg.pos, mesg.yrot, mesg.sectorName );
    InitLinMove( mesg.pos, mesg.yrot, mesg.sectorName, mesg.top, mesg.bottom, mesg.offset );
    InitCharData( mesg.texParts, mesg.equipment );  
}

gemNPCActor::~gemNPCActor()
{
    if (npc)
    {
        npc->ClearState();
        npc->SetActor(NULL);
        npc = NULL;
    }
}

void gemNPCActor::AttachNPC(NPC * newNPC)
{
    npc = newNPC;
    npc->SetActor(this);
    npc->SetAlive(true);
}


bool gemNPCActor::InitCharData( const char* textParts, const char* equipment )
{                   
    return true;    
}    

bool gemNPCActor::InitLinMove(const csVector3& pos, float angle, const char* sector,
                              csVector3 top, csVector3 bottom, csVector3 offset )
{
    pcmove =  new psLinearMovement(npcclient->GetObjectReg());

    csRef<iEngine> engine =  csQueryRegistry<iEngine> (npcclient->GetObjectReg());

    pcmove->InitCD(top, bottom, offset, GetMeshWrapper()); 
    pcmove->SetPosition(pos,angle,engine->FindSector(sector));
    
    return true;  // right now this func never fail, but might later.
}


gemNPCItem::gemNPCItem( psNPCClient* npcclient, psPersistItem& mesg) 
    : gemNPCObject(npcclient, mesg.eid), flags(NONE)
{        
    name = mesg.name;
    Debug3(LOG_CELPERSIST, 0, "Item %s(%s) Received", mesg.name.GetData(), ShowID(mesg.eid));
    type = mesg.type;

    if(!mesg.factname.GetData())
    {
        Error2("Item %s has bad data! Check cstr_id_gfx_mesh for this item!\n", mesg.name.GetData());
    }

    InitMesh(  mesg.factname.GetDataSafe(), "", mesg.pos, mesg.yRot, mesg.sector );
    if (mesg.flags & psPersistItem::NOPICKUP) flags |= NOPICKUP;
}

//Here we check the flag to see if we can pick up this item
bool gemNPCItem::IsPickable() { return !(flags & NOPICKUP); }

gemNPCItem::~gemNPCItem()
{
}
