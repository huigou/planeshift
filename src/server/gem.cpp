/*
* gem.cpp by Keith Fulton <keith@paqrat.com>
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
#include <iengine/engine.h>
#include <iengine/campos.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <ivideo/txtmgr.h>
#include <ivideo/texture.h>
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/object.h>

#include <csgeom/box.h>
#include <imesh/objmodel.h>
#include <csgeom/transfrm.h>
#include <csutil/snprintf.h>
#include <csutil/hash.h>
#include <imesh/object.h>
#include <imesh/spritecal3d.h>
#include <imesh/nullmesh.h>
#include <imap/loader.h>
#include <csqsqrt.h>  // CS quick square root

//=============================================================================
// Project Space Includes
//=============================================================================
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/servervitals.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psnpcdialog.h"
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/psquest.h"

#include "rpgrules/factions.h"

#include "util/gameevent.h"
#include "util/pserror.h"
#include "util/strutil.h"
#include "util/eventmanager.h"
#include "util/psutil.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"

#include "net/npcmessages.h"
#include "net/message.h"
#include "net/msghandler.h"

#include "engine/psworld.h"
#include "engine/linmove.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "client.h"
#include "clients.h"
#include "playergroup.h"
#include "gem.h"
#include "gemmesh.h"
#include "invitemanager.h"
#include "chatmanager.h"
#include "groupmanager.h"
#include "entitymanager.h"
#include "spawnmanager.h"
#include "usermanager.h"
#include "exchangemanager.h"
#include "events.h"
#include "psserverdr.h"
#include "psserver.h"
#include "weathermanager.h"
#include "npcmanager.h"
#include "netmanager.h"
#include "globals.h"
#include "progressionmanager.h"
#include "workmanager.h"
#include "cachemanager.h"
#include "psproxlist.h"
#include "spellmanager.h"
#include "psserverchar.h"
#include "adminmanager.h"
#include "commandmanager.h"
#include "combatmanager.h"

// #define PSPROXDEBUG

#define SPEED_WALK 2.0f

/// Number of messages in the chat history buffer
#define CHAT_HISTORY_SIZE 50

//-----------------------------------------------------------------------------

psGemServerMeshAttach::psGemServerMeshAttach(gemObject* objectToAttach) : scfImplementationType(this)
{
    object = objectToAttach;
}


//-----------------------------------------------------------------------------

GEMSupervisor *gemObject::cel = NULL;

GEMSupervisor::GEMSupervisor(iObjectRegistry *objreg,
                             psDatabase *db)
{
    object_reg = objreg;
    database = db;
    npcmanager = NULL;

    // Start eids at 10000. This to give nice aligned outputs for debuging.
    // Default celID scope has max of 100000 IDs so to support more than
    // 90000 enties another scope should be added to cel
    nextEID = 10000;

    psserver->GetEventManager()->Subscribe(this,MSGTYPE_DAMAGE_EVENT,NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_STATDRUPDATE, REQUIRE_READY_CLIENT );
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_STATS, REQUIRE_READY_CLIENT);
}

GEMSupervisor::~GEMSupervisor()
{
    // Slow but safe method of deleting.
    size_t count = entities_by_eid.GetSize();
    while(count > 0)
    {
        csHash<gemObject*, EID>::GlobalIterator i(entities_by_eid.GetIterator());
        gemObject* obj = i.Next();
        delete obj;

        // Make sure the gemObject has deleted itself from our list otherwise
        // something has gone very wrong.
        CS_ASSERT(count > entities_by_eid.GetSize());
        count = entities_by_eid.GetSize();
        continue;
    }
    if (psserver->GetEventManager())
    {
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DAMAGE_EVENT);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_STATDRUPDATE);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_STATS);
    }
}

void GEMSupervisor::HandleMessage(MsgEntry *me,Client *client)
{
    switch ( me->GetType() )
    {
        case MSGTYPE_STATS:
        {
            psCharacter* psChar = client->GetActor()->GetCharacterData();

            psStatsMessage msg(client->GetClientNum(),
                               psChar->GetHitPointsMax(),
                               psChar->GetManaMax(),
                               psChar->Inventory().MaxWeight(),
                               psChar->Inventory().GetCurrentMaxSpace() );

            msg.SendMessage();
            break;
        }

        case MSGTYPE_DAMAGE_EVENT:
        {
            psDamageEvent evt(me);
            evt.target->BroadcastTargetStatDR(EntityManager::GetSingleton().GetClients());
            break;
        }

        case MSGTYPE_STATDRUPDATE:
        {
            gemActor *actor = client->GetActor();
            psCharacter *psChar = actor->GetCharacterData();

            psChar->SendStatDRMessage(client->GetClientNum(), actor->GetEID(), DIRTY_VITAL_ALL);
            break;
        }
    }
}

EID GEMSupervisor::GetNextID()
{
    return EID(nextEID++);
}

EID GEMSupervisor::CreateEntity(gemObject *obj)
{
    EID eid = GetNextID();

    if (obj)
    {
        entities_by_eid.Put(eid, obj);
        Debug3(LOG_CELPERSIST,0,"Entity <%s> added to supervisor as %s\n", obj->GetName(), ShowID(eid));
    }

    return eid;
}

void GEMSupervisor::AddActorEntity(gemActor *actor)
{
    actors_by_pid.Put(actor->GetPID(), actor);
    Debug3(LOG_CELPERSIST,0,"Actor added to supervisor with %s and %s.\n", ShowID(actor->GetEID()), ShowID(actor->GetPID()));
}

void GEMSupervisor::RemoveActorEntity(gemActor *actor)
{
    actors_by_pid.Delete(actor->GetPID(), actor);
    Debug3(LOG_CELPERSIST,0,"Actor <%s, %s> removed from supervisor.\n", ShowID(actor->GetEID()), ShowID(actor->GetPID()));
}

void GEMSupervisor::AddItemEntity(gemItem *item)
{
    items_by_uid.Put(item->GetItem()->GetUID(), item);
    Debug3(LOG_CELPERSIST,0,"Item added to supervisor with %s and UID:%u.\n", ShowID(item->GetEID()), item->GetItem()->GetUID());
}

void GEMSupervisor::RemoveItemEntity(gemItem *item)
{
    items_by_uid.Delete(item->GetItem()->GetUID(), item);
    Debug3(LOG_CELPERSIST,0,"Item <%s, %u> removed from supervisor.\n", ShowID(item->GetEID()), item->GetItem()->GetUID());
}

void GEMSupervisor::RemoveEntity(gemObject *which)
{
    if (!which)
        return;

    entities_by_eid.Delete(which->GetEID(), which);
    Debug3(LOG_CELPERSIST,0,"Entity <%s, %s> removed from supervisor.\n", which->GetName(), ShowID(which->GetEID()));

}

void GEMSupervisor::RemoveClientFromLootables(int cnum)
{
    csHash<gemObject*, EID>::GlobalIterator i(entities_by_eid.GetIterator());
    while ( i.HasNext() )
    {
        gemObject* obj = i.Next();
        gemNPC * npc = obj->GetNPCPtr();

        if ( npc)
            npc->RemoveLootableClient(cnum);
    }
}


gemObject *GEMSupervisor::FindObject(EID id)
{
    if (!id.IsValid())
        return NULL;

    csHash<gemObject*, EID>::Iterator i(entities_by_eid.GetIterator(id));
    gemObject* obj;

    while ( i.HasNext() )
    {
        obj = i.Next();
        if (obj->GetEID() == id)
        {
            return obj;
        }
    }
    return NULL;
}

gemObject *GEMSupervisor::FindObject(const csString& name)
{
    csHash<gemObject*, EID>::GlobalIterator i(entities_by_eid.GetIterator());

    while ( i.HasNext() )
    {
        gemObject* obj = i.Next();
        if ( name.CompareNoCase(obj->GetName()) )
                return obj;
        else if ( (csString) obj->GetObjectType() == "NPC" ) //Allow search for first name only with NPCs
        {
            WordArray names (obj->GetName(), false);
            if(name.CompareNoCase(names[0]))
                return obj;
        }
    }
    Error2("No object with the name of '%s' was found.", name.GetData());
    return NULL;
}

gemActor *GEMSupervisor::FindPlayerEntity(PID player_id)
{
    return actors_by_pid.Get(player_id, NULL);
}

gemNPC *GEMSupervisor::FindNPCEntity(PID npc_id)
{
    return dynamic_cast<gemNPC*>(actors_by_pid.Get(npc_id, NULL));
}

gemItem *GEMSupervisor::FindItemEntity(uint32 item_id)
{
    return items_by_uid.Get(item_id, NULL);
}

int GEMSupervisor::CountManagedNPCs(AccountID superclientID)
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    int count_players=0;
    while (iter.HasNext())
    {
        gemObject *obj = iter.Next();
        if (obj->GetSuperclientID() == superclientID)
        {
            count_players++;
        }
    }
    return count_players;
}

void GEMSupervisor::FillNPCList(MsgEntry *msg, AccountID superclientID)
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    while (iter.HasNext())
    {
        gemObject *obj = iter.Next();
        if (obj->GetSuperclientID() == superclientID)
        {
            msg->Add(obj->GetPID().Unbox());
            msg->Add(obj->GetEID().Unbox());

            // Turn off any npcs about to be managed from being temporarily impervious
            // CPrintf(CON_NOTIFY,"---------> GemSuperVisor Setting imperv\n");
            obj->GetCharacterData()->SetImperviousToAttack(obj->GetCharacterData()->GetImperviousToAttack() & ~TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later
        }
    }
}

void GEMSupervisor::StopAllNPCs(AccountID superclientID)
{
    CPrintf(CON_NOTIFY, "Shutting down entities managed by superclient %s.\n", ShowID(superclientID));

    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    count_players=0;
    while (iter.HasNext())
    {
        gemObject *obj = iter.Next();
        if (obj->GetSuperclientID() == superclientID && obj->IsAlive())
        {
            // CPrintf(CON_DEBUG, "  Deactivating %s...\n",obj->GetName() );
            gemActor *actor = obj->GetActorPtr();
            actor->pcmove->SetVelocity(csVector3(0,0,0));
            actor->pcmove->SetAngularVelocity(csVector3(0,0,0));
            actor->pcmove->SetOnGround(true);

            csTicks timeDelay=0;
            actor->SetAction("idle",timeDelay);
            actor->SetMode(PSCHARACTER_MODE_PEACE);
            // actor->MoveToValidPos();  // Khaki added this a year ago but it causes pathed npcs to teleport on npcclient exit.
            // CPrintf(CON_NOTIFY,"--------> STOP ALL NPCS Setting Imperv\n");
            actor->GetCharacterData()->SetImperviousToAttack(actor->GetCharacterData()->GetImperviousToAttack() | TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later
            actor->MulticastDRUpdate();
        }
    }
}

void GEMSupervisor::UpdateAllDR()
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    count_players=0;
    while (iter.HasNext())
    {
        gemObject *obj = iter.Next();
        if (obj->GetPID().IsValid())
        {
            obj->UpdateDR();
            count_players++;
        }
    }
}

void GEMSupervisor::UpdateAllStats()
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    count_players=0; // FIXME: This is zeroed out but not recounted?
    while (iter.HasNext())
    {
        gemObject *obj = iter.Next();
        gemActor *actor = dynamic_cast<gemActor *>(obj);
        if (actor)
            actor->UpdateStats();
    }
}

void GEMSupervisor::GetPlayerObjects(PID playerID, csArray<gemObject*> &list )
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());
    while (iter.HasNext())
    {
        gemObject* obj = iter.Next();
        gemItem* item = dynamic_cast<gemItem*>(obj);
        if (item && item->GetItem()->GetOwningCharacterID() == playerID)
        {
            list.Push(item);
        }
    }
}


void GEMSupervisor::GetAllEntityPos(psAllEntityPosMessage& update)
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    update.SetLength(count_players,0);  // Theoretical max of how many

    int count_actual = 0;
    while (iter.HasNext())
    {
        gemObject *obj = iter.Next();
        if (obj->GetPID().IsValid())
        {
            gemActor *actor = dynamic_cast<gemActor *>(obj);
            if (actor)
            {
                csVector3 pos,pos2;
                float yrot;
                InstanceID instance,oldInstance;
                iSector *sector;
                obj->GetPosition(pos,yrot,sector);
                instance = obj->GetInstance();
                obj->GetLastSuperclientPos(pos2,oldInstance);

                float dist2 = (pos.x - pos2.x) * (pos.x - pos2.x) +
                    (pos.y - pos2.y) * (pos.y - pos2.y) +
                    (pos.z - pos2.z) * (pos.z - pos2.z);

                if (dist2 > .04 || instance != oldInstance)
                {
                    count_actual++;
                    update.Add(obj->GetEID(), pos, sector, obj->GetInstance(),
                               CacheManager::GetSingleton().GetMsgStrings());
                    obj->SetLastSuperclientPos(pos,instance);
                }
            }
        }
    }
    update.msg->ClipToCurrentSize();  // Actual Data size
    update.msg->Reset();
    update.msg->Add((int16_t)count_actual);  // Now correct the first value, which is the count of following entities
}



void GEMSupervisor::Teleport( gemObject* object, float x, float y, float z, float rot, const char* sectorname )
{
    csVector3 pos( x,y,z );
    csRef<iEngine> engine = csQueryRegistry<iEngine> (psserver->GetObjectReg());
    iSector * sector = engine->GetSectors()->FindByName(sectorname);

    if ( !sector )
    {
        Bug2("Sector %s is not found!", sectorname );
        return;
    }

    object->Move( pos, rot, sector );


    gemActor* actor = (gemActor*)object;
    actor->SetPosition( pos, rot, sector );
    actor->UpdateProxList(true);  // true=force update
    actor->MulticastDRUpdate();
}


void GEMSupervisor::AttachObject( iObject* object, gemObject* gobject )
{
    csRef<psGemServerMeshAttach> attacher = csPtr<psGemServerMeshAttach>(new psGemServerMeshAttach(gobject));
    attacher->SetName( object->GetName() );
    csRef<iObject> attacher_obj(scfQueryInterface<iObject>(attacher));

    object->ObjAdd( attacher_obj );
}

void GEMSupervisor::UnattachObject( iObject* object, gemObject* gobject )
{
    csRef<psGemServerMeshAttach> attacher (CS::GetChildObject<psGemServerMeshAttach>(object));
    if (attacher)
    {
        if (attacher->GetObject() == gobject)
        {
            csRef<iObject> attacher_obj(scfQueryInterface<iObject>(attacher));
            object->ObjRemove(attacher_obj);
        }
    }
}

gemObject* GEMSupervisor::FindAttachedObject( iObject* object )
{
    gemObject* found = 0;

    csRef<psGemServerMeshAttach> attacher(CS::GetChildObject<psGemServerMeshAttach>(object));
    if ( attacher )
    {
        found = attacher->GetObject();
    }

    return found;
}

csArray<gemObject*> GEMSupervisor::FindNearbyEntities( iSector* sector, const csVector3& pos, float radius, bool doInvisible )
{
    csArray<gemObject*> list;
    csRef<iEngine> engine = csQueryRegistry<iEngine> (psserver->GetObjectReg());

    csRef<iMeshWrapperIterator> obj_it =  engine->GetNearbyMeshes( sector, pos, radius );
    while (obj_it->HasNext())
    {
        iMeshWrapper* m = obj_it->Next();
        if (!doInvisible)
        {
            bool invisible = m->GetFlags().Check(CS_ENTITY_INVISIBLE);
            if (invisible)
                continue;
        }

        gemObject* object = FindAttachedObject(m->QueryObject());

        if (object)
        {
            list.Push( object );
        }
    }

    return list;
}



/*****************************************************************/

csString GetDefaultBehavior(const csString & dfltBehaviors, int behaviorNum)
{
    csStringArray behaviors;
    psString dflt(dfltBehaviors);

    dflt.Split(behaviors);
    if (behaviorNum < (int)behaviors.GetSize())
        return behaviors[behaviorNum];
    else
        return "";
}

gemObject::gemObject(const char *name)
{
    this->valid    = true;
    this->name     = name;
    pcmesh=NULL;
    sector=NULL;
    worldInstance=0;
    prox_distance_desired=DEF_PROX_DIST;
    prox_distance_current=DEF_PROX_DIST;
    eid = 0;
}

gemObject::gemObject(const char* name,
                     const char* factname,
                     const char* filename,
                     InstanceID myInstance,
                     iSector* room,
                     const csVector3& pos,
                     float rotangle,
                     int clientnum)
{
    if (!this->cel)
        this->cel = GEMSupervisor::GetSingletonPtr();

    this->valid    = true;
    this->name     = name;
    this->factname = factname;
    this->filename = filename;
    this->yRot     = rotangle;
    this->worldInstance = myInstance;

    proxlist = NULL;
    is_alive = false;

    eid = cel->CreateEntity(this);

    prox_distance_desired=DEF_PROX_DIST;
    prox_distance_current=DEF_PROX_DIST;

    if (!InitMesh(name,factname,filename,pos,rotangle,room))
    {
        Error1("Could not create gemObject because mesh could not be Init'd.");
        Error4("Name: %s Factory: %s File: %s\n", name, factname, filename );
        return;
    }

    if (!InitProximityList(DEF_PROX_DIST,clientnum))
    {
        Error1("Could not create gemObject because prox list could not be Init'd.");
        return;
    }
}

gemObject::~gemObject()
{
    // Assert on any dublicate delete
    assert(valid);
    valid = false;

    Disconnect();
    cel->RemoveEntity(this);
    delete proxlist;
    proxlist = NULL;
    delete pcmesh;
}


const char *gemObject::GetName()
{
    return name;
}

void gemObject::SetName(const char *n)
{
    name = n;
}

void gemObject::Disconnect()
{
    while(receivers.GetSize() > 0)
    {
        iDeleteObjectCallback *receiver = receivers.Pop();
        receiver->DeleteObjectCallback(this);
    }
}


void gemObject::Broadcast(int clientnum, bool control )
{
    CPrintf(CON_DEBUG, "Base Object Broadcast!\n");
}

void gemObject::SetAlive(bool flag)
{
    is_alive = flag;
    if (!flag)
    {
        GetCharacterData()->SetHitPointsRate(0);  // This keeps dead guys from regen'ing HP
        GetCharacterData()->SetManaRate(0);  // This keeps dead guys from regen'ing Mana
        BroadcastTargetStatDR( psserver->GetNetManager()->GetConnections() );
    }
}


bool gemObject::InitMesh(const char *name,
                           const char *factname,
                           const char *filename,
                           const csVector3& pos,
                           const float rotangle,
                           iSector* room)
{
    pcmesh = new gemMesh(psserver->GetObjectReg(), this, cel);

    // Replace helm group token with the default race.
    psString fact_name(factname);
    fact_name.ReplaceAllSubString("$H", "stonebm");
    factname = fact_name;

    psString file_name(filename);
    file_name.ReplaceAllSubString("$H", "stonebm");
    filename = file_name;

    csRef<iEngine> engine = csQueryRegistry<iEngine> (psserver->GetObjectReg());
    csRef<iVFS> vfs = csQueryRegistry<iVFS> (psserver->GetObjectReg());

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
                csRef<iDocument> doc = ParseFile(psserver->GetObjectReg(), filename);
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

                csRef<iThreadedLoader> loader (csQueryRegistry<iThreadedLoader> (psserver->GetObjectReg()));
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
        else
        {
          Error2("The file(%s) could not be found", filename);
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

iMeshWrapper *gemObject::GetMeshWrapper()
{
    return pcmesh->GetMesh();
}

void gemObject::Move(const csVector3& pos,float rotangle, iSector* room)
{
    // Position and sector
    pcmesh->MoveMesh(room, pos);

    // Rotation
    csMatrix3 matrix = (csMatrix3) csYRotMatrix3 (rotangle);
    pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T (matrix);

    this->pos    = pos;
    this->sector = room;
}

#define PSABS(x)    ((x) < 0 ? -(x) : (x))

bool gemObject::IsNear(gemObject *obj, float radius)
{
    // Find the current position of the specified entity
    csVector3 pos1,pos2;
    float yrot1,yrot2;
    iSector *sector1,*sector2;

    GetPosition(pos1,yrot1,sector1);
    obj->GetPosition(pos2,yrot2,sector2);

    EntityManager::GetSingleton().GetWorld()->WarpSpace(sector2, sector1, pos2);

    float squaredDistance = (pos1.x - pos2.x)*(pos1.x - pos2.x)+
        (pos1.y - pos2.y)*(pos1.y - pos2.y)+
        (pos1.z - pos2.z)*(pos1.z - pos2.z);

    if ( squaredDistance < radius*radius)
        return true;
    else
        return false;
}


float gemObject::RangeTo(gemObject* obj, bool ignoreY)
{
    return proxlist->RangeTo(obj, ignoreY);
}

bool gemObject::IsUpdateReq (csVector3 const &pos,csVector3 const &oldPos)
{
    return (pos-oldPos).SquaredNorm() >= DEF_UPDATE_DIST*DEF_UPDATE_DIST;
}

int gemObject::FindAnimIndex(const char *name)
{
    return CacheManager::GetSingleton().GetMsgStrings()->Request(name);
}


bool gemObject::InitProximityList(float radius,int clientnum)
{
    proxlist = new ProximityList(cel->object_reg,this);

    proxlist->Initialize(clientnum,this); // store these for fast access later

    bool subscribed_self=false;
    // A client should always subscribe to itself
    if (!subscribed_self && clientnum)
    {
        // CPrintf(CON_DEBUG, "Forcing a self-subscription for %s\n",GetName());
        proxlist->StartMutualWatching(clientnum,this,0.0);
    }
    return true;
}

void gemObject::UpdateProxList( bool force )
{
#ifdef PSPROXDEBUG
    psString log;
#endif

    if (!force && !proxlist->CheckUpdateRequired())  // This allows updates only if moved some way away
        return;

#ifdef PSPROXDEBUG
    log.AppendFmt("Generating proxlist for %s\n", GetName());
    //proxlist->DebugDumpContents();
#endif

    // Find nearby entities
    csVector3 pos;
    float yrot;
    iSector *sector;

    csTicks time = csGetTicks();

    GetPosition(pos,yrot,sector);
    csArray<gemObject*> nearlist = cel->FindNearbyEntities(sector,pos,prox_distance_current, true);

    //CPrintf(CON_SPAM, "\nUpdating proxlist for %s\n--------------------------\n",GetName());

    // Cycle through list and add any entities
    // that represent players to the proximity subscription list.
    size_t count = nearlist.GetSize();
    size_t player_count = 0;

    proxlist->ClearTouched();
    for (size_t i=0; i<count; i++)
    {
        gemObject *nearobj = nearlist[i];
        if (!nearobj)
            continue;

        // npcs and objects don't watch each other
        if (!GetClientID() && !nearobj->GetClientID())
            continue;

        float range = proxlist->RangeTo(nearobj);
#ifdef PSPROXDEBUG
        log.AppendFmt("%s is %1.2fm away from %s\n",GetName(),range,nearobj->GetName() );
#endif

        if (SeesObject(nearobj, range))
        {
            //CPrintf(CON_SPAM, " and is seen.\n");
#ifdef PSPROXDEBUG
            log.AppendFmt("-%s (client %i) can see %s\n",GetName(),GetClientID(),nearobj->GetName());
#endif
            if (proxlist->StartWatching(nearobj, range) || force)
            {
#ifdef PSPROXDEBUG
                log.AppendFmt("-%s (client %i) started watching %s\n",GetName(),GetClientID(),nearobj->GetName());
#endif
                if (GetClientID()!=0)
                {
#ifdef PSPROXDEBUG
                    log.AppendFmt("-%s sent to client %s\n",nearobj->GetName(),GetName());
#endif
                    nearobj->Send(GetClientID(),false,false);
                    player_count++;
                }
            }
        }
        else
        {
#ifdef PSPROXDEBUG
            log.AppendFmt("-%s can not see %s\n",GetName(), nearobj->GetName());
#endif
        }

        if ((!nearobj->GetClient() || (nearobj->GetClient() && nearobj->GetClient()->IsReady())) && nearobj->SeesObject(this, range))
        {
#ifdef PSPROXDEBUG
            log.AppendFmt("-%s can see %s\n",nearobj->GetName(),GetName());
#endif
            // true means new addition, false means already on list
            if (nearobj->GetProxList()->StartWatching(this, range) || force)
            {
#ifdef PSPROXDEBUG
                log.AppendFmt("-%s started watching %s\n",nearobj->GetName(),GetName());
#endif
                if (nearobj->GetClientID()!=0)
                {
#ifdef PSPROXDEBUG
                    log.AppendFmt("Big send -%s sent to client %s\n",GetName(),nearobj->GetName());
#endif
                    Send(nearobj->GetClientID(),false,false);
                }
            }
        }
        else
        {
#ifdef PSPROXDEBUG
            log.AppendFmt("-%s can not see %s\n", nearobj->GetName(),GetName());
#endif
        }
    }

    // See how our tally did, and step down on the next update if it's too big.
    if (player_count > PROX_LIST_SHRINK_THRESHOLD)
    {
        prox_distance_current-=PROX_LIST_STEP_SIZE;
        if (prox_distance_current<0)
        {
            prox_distance_current=0;
        }
    }
    /* Step up if we're small enough
     *
     *  FIXME:  There's a chance that we can bounce back and forth between various step sizes if
     *           (PROX_LIST_SHRINK_THRESHOLD - PROX_LIST_REGROW_THRESHOLD) players happen to be in
     *          the ring between the radius defined by the two steps.  This probably isn't harmful -
     *          as long as SHRINK, REGROW and STEP are defined with some sanity it should be a rare
     *          occurance.
     */
    if (player_count < PROX_LIST_REGROW_THRESHOLD && prox_distance_current < prox_distance_desired)
    {
        prox_distance_current+=PROX_LIST_STEP_SIZE;
        if (prox_distance_current>prox_distance_desired)
        {
            prox_distance_current=prox_distance_desired;
        }
    }

    // Now remove those that should be no more connected to out object

    gemObject *obj;

    size_t debug_count = 0;
    if (GetClientID() != 0)
    {
        while (proxlist->GetUntouched_ObjectThatIWatch(obj))
        {
            debug_count++;
#ifdef PSPROXDEBUG
            log.AppendFmt("-removing %s from client %s\n",obj->GetName(),GetName());
#endif
            CS_ASSERT(obj != this);

            //               csVector3 pos; float yrot; iSector *sector;
            //               obj->GetPosition(pos,yrot,sector);
            //                CPrintf(CON_SPAM, "Removing %s at (%1.2f, %1.2f, %1.2f) in sector %s.\n",obj->GetName(),pos.x,pos.y,pos.z,sector->QueryObject()->GetName() );

            psRemoveObject remove( GetClientID(), obj->GetEID() );
            remove.SendMessage();
            proxlist->EndWatching(obj);
        }
    }

    if (csGetTicks() - time > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time getting untouched objects in proxlist for %s, %zu nearby entities!",
                      csGetTicks() - time, GetName(), count);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    while (proxlist->GetUntouched_ObjectThatWatchesMe(obj))
    {
        if (obj->GetClientID() != 0)
        {
#ifdef PSPROXDEBUG
            log.AppendFmt("-removing %s from client %s\n",GetName(),obj->GetName());
#endif
            CS_ASSERT(obj != this);
            psRemoveObject msg(obj->GetClientID(), eid);
            msg.SendMessage();
            obj->GetProxList()->EndWatching(this);
        }
    }

    if (csGetTicks() - time > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time touching entities in proxlist for %s,"
                      " counted %zu nearby entities, %zu untouched objects that it watches!",
                      csGetTicks() - time, GetName(), count, debug_count);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

#ifdef PSPROXDEBUG
    CPrintf(CON_DEBUG, "%s\n", log.GetData());    //use CPrintf because Error1() is breaking lines
#endif
}


csArray<PublishDestination>& gemObject::GetMulticastClients()
{
    return proxlist->GetClients();
}

uint32  gemObject::GetClientID()
{
    return proxlist->GetClientID();
}

csArray< gemObject* > *gemObject::GetObjectsInRange( float range )
{
    csArray< gemObject* > *objectsInRange = new csArray< gemObject* >();

    // Find nearby entities
    csVector3 pos;
    iSector *sector;
    GetPosition(pos,sector);
    csArray<gemObject*> nearlist = cel->FindNearbyEntities( sector, pos, range);

    size_t count = nearlist.GetSize();

    for (size_t i=0; i<count; i++)
    {
        gemObject *nearobj = nearlist[i];
        if (!nearobj)
            continue;

        objectsInRange->Push( nearobj );

        /*
        float range = proxlist->RangeTo(nearobj);
        CPrintf(CON_DEBUG, "%s is %1.2fm away from %s",GetName(),range,nearobj->GetName() );

        if (SeesObject(nearobj, range))
        {
            CPrintf(CON_DEBUG, " and is seen.\n");
        }
        else
        {
            CPrintf(CON_DEBUG, " and is not seen.\n");
        }
        */
    }

    return objectsInRange;
}

void gemObject::GetPosition(csVector3& pos, float& yrot,iSector*& sector)
{
    // Position
    pos = GetMeshWrapper()->GetMovable()->GetPosition();

    // Rotation
    yrot = GetAngle();

    // Sector
    sector = GetSector();
}

void gemObject::GetPosition(csVector3& pos, iSector*& sector)
{
    // Position
    pos = GetMeshWrapper()->GetMovable()->GetPosition();

    // Sector
    sector = GetSector();
}

float gemObject::GetAngle()
{
    // Rotation
    csMatrix3 transf = GetMeshWrapper()->GetMovable()->GetTransform().GetT2O();
    return psWorld::Matrix2YRot(transf);
}

iSector* gemObject::GetSector()
{
    // Sector
    if (GetMeshWrapper()->GetMovable()->GetSectors()->GetCount())
        return GetMeshWrapper()->GetMovable()->GetSectors()->Get(0);
    else
        return NULL;
}

void gemObject::SendBehaviorMessage(const csString & str, gemObject *actor)
{
    Error3("gemObject %s got behavior message %s in error.",GetName(),str.GetData());
}

csString gemObject::GetDefaultBehavior(const csString & dfltBehaviors)
{
    Error3("gemObject %s got default behavior message %s in error.",GetName(),dfltBehaviors.GetData());
    return "";
}

void gemObject::Dump()
{
    csString out;
    CPrintf(CON_CMDOUTPUT ,"ProxList:\n");
    CPrintf(CON_CMDOUTPUT ,"Distance: %.2f <= Desired: %.2f \n",
            prox_distance_current,prox_distance_desired);
    GetProxList()->DebugDumpContents(out);
    CPrintf(CON_CMDOUTPUT, out.GetData());
}

//--------------------------------------------------------------------------------------
// gemActiveObject
//--------------------------------------------------------------------------------------

gemActiveObject::gemActiveObject( const char* name )
                                : gemObject( name )
{
}

gemActiveObject::gemActiveObject( const char* name,
                                     const char* factname,
                                     const char* filename,
                                     InstanceID myInstance,
                                     iSector* room,
                                     const csVector3& pos,
                                     float rotangle,
                                     int clientnum)
                                     : gemObject(name,factname,filename,myInstance,room,pos,rotangle,clientnum)
{
    //if entity is not set, object is not a success
//    if (entity != NULL)
//    {
//        CPrintf(CON_DEBUG, "New object: %s at %1.2f,%1.2f,%1.2f, sector %s.\n",
//            factname,pos.x,pos.y,pos.z,    sector->QueryObject()->GetName());
//    }
//    else CPrintf(CON_DEBUG, "Object %s was not created.\n", factname);

}

void gemActiveObject::Broadcast(int clientnum, bool control )
{
    CPrintf(CON_DEBUG, "Active Object Broadcast" );
}

csString gemActiveObject::GetDefaultBehavior(const csString & dfltBehaviors)
{
    return ::GetDefaultBehavior(dfltBehaviors, 0);
}

void gemActiveObject::SendBehaviorMessage(const csString & msg_id, gemObject *actor)
{
    unsigned int clientnum = actor->GetClientID();

    if ( msg_id == "select" )
    {
        // If the player is in range of the item.
        if ( RangeTo(actor) < RANGE_TO_SELECT )
        {
            int options = 0;

            psGUIInteractMessage guimsg(clientnum,options);
            guimsg.SendMessage();
        }

        return;
    }
    else if ( msg_id == "context" )
    {
        // If the player is in range of the item.
        if ( RangeTo(actor) < RANGE_TO_SELECT && actor->IsAlive() )
        {
            int options = psGUIInteractMessage::EXAMINE | psGUIInteractMessage::USE;

            if (IsContainer()) options |= psGUIInteractMessage::COMBINE;
            if (IsPickable())  options |= psGUIInteractMessage::PICKUP;
            if (IsLockable())
            {
                if( IsLocked())
                    options |= psGUIInteractMessage::UNLOCK;
                else
                    options |= psGUIInteractMessage::LOCK;
            }

            // Check if there are something to send
            if (!options)
                return;

            // Always possible to close menu
            options |= psGUIInteractMessage::CLOSE;

            psGUIInteractMessage guimsg(clientnum,options);
            guimsg.SendMessage();
        }
        return;
    }
    else if ( msg_id == "pickup")
    {
        // Check if the char is dead
        if (!actor->IsAlive())
        {
            psserver->SendSystemInfo(clientnum,"You can't pick up items when you're dead.");
            return;
        }
        // Check if the item is pickupable
        if (!IsPickable())
        {
            psserver->SendSystemInfo(clientnum,"You can't pick up %s.", GetName());
            return;
        }

        // Check if the item is in range
        if (!(RangeTo(actor) < RANGE_TO_SELECT))
        {
            if (!CacheManager::GetSingleton().GetCommandManager()->Validate(actor->GetClient()->GetSecurityLevel(), "pickup override"))
            {
                psserver->SendSystemInfo(clientnum,"You're too far away to pick up %s.", GetName());
                return;
            }
        }

        psItem* item = GetItem();

        // Check if item is guarded
        if (!CacheManager::GetSingleton().GetCommandManager()->Validate(actor->GetClient()->GetSecurityLevel(), "pickup override"))
        {
            PID guardCharacterID = item->GetGuardingCharacterID();
            gemActor* guardActor = GEMSupervisor::GetSingleton().FindPlayerEntity(guardCharacterID);
            if (guardCharacterID.IsValid() &&
                guardCharacterID != actor->GetCharacterData()->GetPID() &&
                guardActor &&
                guardActor->RangeTo(item->GetGemObject()) < RANGE_TO_SELECT)
            {
                psserver->SendSystemInfo(clientnum,"You notice that the item is being guarded by %s",
                    guardActor->GetCharacterData()->GetCharFullName());
                return;
            }
        }

        // Cache values from item, because item might be deleted by Add
        csString qname = item->GetQuantityName();
        gemContainer *container = dynamic_cast<gemContainer*> (this);

        uint32 origUID = item->GetUID();
        unsigned short origStackCount = item->GetStackCount();

        gemActor* gActor = dynamic_cast<gemActor*>(actor);
        psCharacter* chardata = NULL;
        if (gActor) chardata = gActor->GetCharacterData();
        if (chardata && chardata->Inventory().Add(item,false, true, PSCHARACTER_SLOT_NONE, container))
        {
            Client* client = actor->GetClient();
            if (!client)
            {
                Debug2(LOG_ANY,clientnum,"User action from unknown client!  Clientnum:%d\n",clientnum);
                return;
            }
            client->SetTargetObject(NULL);
            // item is deleted if we pickup money
            if(item)
            {
                item->ScheduleRespawn();
                psPickupEvent evt(
                           chardata->GetPID(),
                           item->GetUID(),
                           item->GetStackCount(),
                           (int)item->GetCurrentStats()->GetQuality(),
                           0
                           );
                evt.FireEvent();
            }
            else
            {
                psPickupEvent evt(
                               chardata->GetPID(),
                               origUID,
                               origStackCount,
                               0,
                               0
                               );
                evt.FireEvent();
            }

            psSystemMessage newmsg(clientnum, MSG_INFO_BASE, "%s picked up %s", actor->GetName(), qname.GetData() );
            newmsg.Multicast(actor->GetMulticastClients(),0,RANGE_TO_SELECT);

            psserver->GetCharManager()->UpdateItemViews(clientnum);

            EntityManager::GetSingleton().RemoveActor(this);  // Destroy this
        }
        else
        {

            /* TODO: Include support for partial pickup of stacks

            // Check if part of a stack where picked up
            if (item && count > item->GetStackCount())
            {
                count = count - item->GetStackCount();
                qname = psItem::GetQuantityName(item->GetName(),count);

                psSystemMessage newmsg(client, MSG_INFO, "%s picked up %s", actor->GetName(), qname.GetData() );
                newmsg.Multicast(actor->GetMulticastClients(),0,RANGE_TO_SELECT);

                psserver->GetCharManager()->UpdateItemViews(clientnum);

                csString buf;
                buf.Format("%s, %s, %s, \"%s\", %d, %d", actor->GetName(), "World", "Pickup", GetName(), 0, 0);
                psserver->GetLogCSV()->Write(CSV_EXCHANGES, buf);

            }
            */

            // Assume inventory is full so tell player about that to.
            psserver->SendSystemInfo(clientnum, "You can't carry anymore %s",GetName());
        }
    }

    else if (msg_id == "use")
        psserver->GetWorkManager()->HandleUse(actor->GetClient());
    else if (msg_id == "combine")
        psserver->GetWorkManager()->HandleCombine(actor->GetClient());
    else if (msg_id == "unlock")
        ; //???????
    else if (msg_id == "examine")
        psserver->GetCharManager()->ViewItem(actor->GetClient(), actor->GetEID().Unbox(), PSCHARACTER_SLOT_NONE);

    return;
}

//--------------------------------------------------------------------------------------
// gemItem
//--------------------------------------------------------------------------------------

gemItem::gemItem(csWeakRef<psItem> item,
                     const char* factname,
                     const char* filename,
                     InstanceID instance,
                     iSector* room,
                     const csVector3& pos,
                     float xrotangle,
                     float yrotangle,
                     float zrotangle,
                     int clientnum)
                     : gemActiveObject(item->GetName(),factname,filename,instance,room,pos,yrotangle,clientnum)
{
    itemdata=item;
    xRot=xrotangle;
    zRot=zrotangle;
    itemType.Format("Item(%s)",itemdata->GetItemType());
    itemdata->SetGemObject( this );
    cel->AddItemEntity(this);
}

void gemItem::Broadcast(int clientnum, bool control )
{
    int flags = 0;
    if (!IsPickable()) flags |= psPersistItem::NOPICKUP;
    if (IsUsingCD()) flags |= psPersistItem::COLLIDE;

    psPersistItem mesg(
                         clientnum,
                         eid,
                         -2,
                         name,
                         factname,
                         filename,
                         sector->QueryObject()->GetName(),
                         pos,
                         xRot,
                         yRot,
                         zRot,
                         flags
                         );

    mesg.Multicast(GetMulticastClients(),clientnum,PROX_LIST_ANY_RANGE);
    cel->RemoveItemEntity(this);
}

void gemItem::SetPosition(const csVector3& pos,float angle, iSector* sector, InstanceID instance)
{
    this->pos = pos;
    this->yRot = angle;
    this->sector = sector;
    this->worldInstance = instance;

    psSectorInfo* sectorInfo = NULL;
    if (sector != NULL)
        sectorInfo = CacheManager::GetSingleton().GetSectorInfoByName(sector->QueryObject()->GetName());
    if (sectorInfo != NULL)
    {
        itemdata->SetLocationInWorld(instance, sectorInfo, pos.x, pos.y, pos.z, angle );
        itemdata->Save(false);
    }

    UpdateProxList(true);
}

void gemItem::SetRotation(float xrotangle, float yrotangle, float zrotangle)
{
    this->xRot = xrotangle;
    this->yRot = yrotangle;
    this->zRot = zrotangle;
    
    itemdata->SetRotationInWorld(xrotangle,yrotangle,zrotangle);
	itemdata->Save(false);
}

void gemItem::GetRotation(float& xrotangle, float& yrotangle, float& zrotangle)
{
    xrotangle = this->xRot;
    yrotangle = this->yRot;
    zrotangle = this->zRot;
}

psItem* gemItem::GetItem()
{
    return itemdata;
}

float gemItem::GetBaseAdvertiseRange()
{
    if (itemdata==NULL)
    {
        return gemObject::GetBaseAdvertiseRange();
    }
    return itemdata->GetVisibleDistance();
}


void gemItem::Send( int clientnum, bool , bool to_superclient)
{
    int flags = 0;
    if (!IsPickable()) flags |= psPersistItem::NOPICKUP;
    if (IsUsingCD() ||
        GetItem()->GetSector()->GetIsColliding()) flags |= psPersistItem::COLLIDE;

    psPersistItem mesg(
                         clientnum,
                         eid,
                         -2,
                         name,
                         factname,
                         filename,
                         sector->QueryObject()->GetName(),
                         pos,
                         xRot,
                         yRot,
                         zRot,
                         flags
                         );

    if (clientnum)
    {
        mesg.SendMessage();
    }

    if (to_superclient)
    {
        mesg.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
    }

}

//Here we check the flag to see if we can pick up this item
bool gemItem::IsPickable() { return !(itemdata->GetFlags() & PSITEM_FLAG_NOPICKUP); }

bool gemItem::IsLockable() { return itemdata->GetIsLockable();}

bool gemItem::IsLocked() { return itemdata->GetIsLocked();}

bool gemItem::IsUsingCD() { return itemdata->GetIsCD();}

bool gemItem::IsSecurityLocked() { return itemdata->GetIsSecurityLocked();}

bool gemItem::IsContainer() { return itemdata->GetIsContainer();}

bool gemItem::GetCanTransform() { return itemdata->GetCanTransform();}

bool gemItem::GetVisibility()
{
    /* This function is called after itemdata might be deleted so never
       include use of itemdata in this function. */
    return true;
}

//--------------------------------------------------------------------------------------
// gemContainer
//--------------------------------------------------------------------------------------

gemContainer::gemContainer(csWeakRef<psItem> item,
             const char* factname,
             const char* filename,
             InstanceID myInstance,
             iSector* room,
             const csVector3& pos,
             float xrotangle,
             float yrotangle,
             float zrotangle,
             int clientnum)
             : gemItem(item,factname,filename,myInstance,room,pos,xrotangle,yrotangle,zrotangle,clientnum)
{
}

bool gemContainer::CanAdd(unsigned short amountToAdd, psItem *item, int slot)
{
    if (!item)
        return false;

    PID guard = GetItem()->GetGuardingCharacterID();
    gemActor* guardingActor = GEMSupervisor::GetSingleton().FindPlayerEntity(guard);

    // Test if container is guarded by someone else who is near
    if (guard.IsValid() && guard != item->GetOwningCharacterID()
        && guardingActor && (guardingActor->RangeTo(this) <= 5))
        return false;

    /* We often want to see if a partial stack could be added, but we want to
     * check before actually doing the splitting.  So, we take an extra parameter
     * and fake the stack count before calling AddToContainer with the test flag;
     * we put it back when we're done. */

    uint currentSize = 0;
    gemContainer::psContainerIterator iter(this);
    while (iter.HasNext())
    {
        psItem* child = iter.Next();
        currentSize += child->GetTotalStackSize();
    }
    if (item->GetItemSize()*amountToAdd + currentSize > itemdata->GetContainerMaxSize())
        return false;


    unsigned short savedCount = item->GetStackCount();
    item->SetStackCount(amountToAdd);
    bool result = AddToContainer(item, NULL, slot, true);
    item->SetStackCount(savedCount);
    return result;
}

bool gemContainer::CanTake(Client *client, psItem* item)
{
    if (!client || !item)
        return false;

    // Allow developers to take anything
    if (client->GetSecurityLevel() >= GM_DEVELOPER)
        return true;

    /* \note
     * The check if the item is NPCOwned or NoPickup only makes
     * sense if it is not possible to have NPCOwned items in a not
     * NPCOwned container, or NoPickup items in a PickUpable container
     */
    //not allowed to take npcowned or nopickup items in a container
    if (item->GetIsNpcOwned() || item->GetIsNoPickup())
        return false;

    psItem *containeritem = GetItem();
    CS_ASSERT(containeritem);

    // Check for npc-owned or locked container
    if (client->GetSecurityLevel() < GM_LEVEL_2)
    {
        if (containeritem->GetIsNpcOwned())
        {
            return false;
        }
        if (containeritem->GetIsLocked())
        {
            psserver->SendSystemError(client->GetClientNum(), "You cannot take an item from a locked container!");
            return false;
        }
    }

    // Allow if the item is pickupable and either: public, guarded by the character, or the guarding character is offline
    PID guard = item->GetGuardingCharacterID();
    gemActor* guardingActor = GEMSupervisor::GetSingleton().FindPlayerEntity(guard);

    if ((!guard.IsValid() || guard == client->GetCharacterData()->GetPID() || !guardingActor) || (guardingActor->RangeTo(this) > 5))
    {
        return true;
    }

    return false;
}

bool gemContainer::AddToContainer(psItem *item, Client *fromClient, int slot, bool test)
{
    // Slot changing and item creation in craft is done at the very end of event.
    // A new item is created from scratch and replaces the previous item stack and all.
    // Auto-transform containers will interupt event if item is changed.
    // Checks in craft code to make sure original item is what is expected before the item is replaced.

    // If slot number not specified put into first open slot
    if (slot == PSCHARACTER_SLOT_NONE)
    {
        for (int i = 0; i < SlotCount(); i++)
        {
            if( FindItemInSlot(i) == NULL)
            {
                slot = i;
                break;
            }
        }
    }
    if (slot < 0 || slot >= SlotCount())
        return false;

    // printf("Adding %s to GEM container %s in slot %d.\n", item->GetName(), GetName(), slot);

    // If the destination slot is occupied, try and look for an empty slot.
    if (FindItemInSlot(slot))
        return AddToContainer(item, fromClient, PSCHARACTER_SLOT_NONE, test);

    if (test)
        return true;

    // If the gemContainer we are dropping the item into is not pickupable then we
    // guard the item placed inside.  Otherwise the item becomes public.
    if (fromClient)
    {
        /* put the player behind the client as guard */
        item->SetGuardingCharacterID(fromClient->GetCharacterData()->GetPID());
    }
    if (item->GetOwningCharacterID().IsValid())
    {
        /* item comes from a player () */
        item->SetGuardingCharacterID(item->GetOwningCharacterID());
    }

    if (!GetItem()->GetIsNoPickup() && item->GetOwningCharacterID().IsValid())
        GetItem()->SetGuardingCharacterID(item->GetOwningCharacterID());

    item->SetOwningCharacter(NULL);
    item->SetContainerID(GetItem()->GetUID());
    item->SetLocInParent((INVENTORY_SLOT_NUMBER)(slot));
    item->Save(false);
    itemlist.PushSmart(item);

    item->UpdateView(fromClient, eid, false);
    return true;
}

bool gemContainer::RemoveFromContainer(psItem *item,Client *fromClient)
{
    // printf("Removing %s from container now.\n", item->GetName() );
    if (itemlist.Delete(item))
    {
        item->UpdateView(fromClient, eid, true);
        return true;
    }
    else
        return false;
}

psItem* gemContainer::RemoveFromContainer(psItem *itemStack, int fromSlot, Client *fromClient, int stackCount)
{
    // printf("Removing %s from container now.\n", item->GetName() );

    psItem* item = NULL;
    bool clear = false;

    // If the stack count is the taken amount then we can delete the entire stack from the itemlist
    if ( itemStack->GetStackCount() == stackCount )
    {
        item = itemStack;
        itemlist.Delete(itemStack);
        clear = true;   // Clear out the slot on the client
    }
    else
    {
        // Split out the stack for the required amount.
        item = itemStack->SplitStack(stackCount);

        // Save the lowered value
        itemStack->Save(false);
    }

    // Send out messages about the change in the item stack.
    itemStack->UpdateView(fromClient, eid, clear);
    return item;
}


psItem *gemContainer::FindItemInSlot(int slot, int stackCount)
{
    if (slot < 0 || slot >= SlotCount())
        return NULL;
    psContainerIterator it(this);
    while (it.HasNext())
    {
        // Check for specific slot
        //  These are small arrays so no need to hash index
        psItem* item = it.Next();
        if (item == NULL)
        {
            Error1("Null item in container list.");
            return NULL;
        }

        // Return the item in parrent location slot
        if (item->GetLocInParent() == slot)
        {
            // If the item is here and the stack count we want is available.
            if ( stackCount == -1 || item->GetStackCount() >= stackCount )
            {
                return item;
            }
            else
            {
                return NULL;
            }
        }
    }
    return NULL;
}

gemContainer::psContainerIterator::psContainerIterator(gemContainer *containerItem)
{
    UseContainerItem(containerItem);
}

bool gemContainer::psContainerIterator::HasNext()
{
    if (current + 1 < container->CountItems() )
        return true;
    return false;
}

psItem *gemContainer::psContainerIterator::Next()
{
    if (!HasNext())
        return NULL;

    current++;
    return container->GetIndexItem(current);
}

psItem *gemContainer::psContainerIterator::RemoveCurrent(Client *fromClient)
{
    if (current < container->CountItems() )
    {
        psItem *item = container->GetIndexItem(current);
        container->RemoveFromContainer(item,fromClient);
        current--; // This adjusts so that the next "Next()" call actually returns the next item and doesn't skip one.
        return item;
    }
    return NULL;
}

void gemContainer::psContainerIterator::UseContainerItem(gemContainer *containerItem)
{
    container=containerItem;
    current = SIZET_NOT_FOUND;
}

//--------------------------------------------------------------------------------------
// gemActionLocation
//--------------------------------------------------------------------------------------

gemActionLocation::gemActionLocation(psActionLocation *action, iSector *isec, int clientnum)
                                     : gemActiveObject(action->name)
{
    this->yRot  = 0;
    this->action = action;
    action->SetGemObject( this );

    // action locations use the AL ID as their EID for some reason
    eid = action->id;

    this->prox_distance_desired = 0.0F;
    this->prox_distance_current = 0.0F;

    visible = false;

    SetName(name);

    pcmesh = new gemMesh(psserver->GetObjectReg(), this, cel);

    csRef< iEngine > engine =  csQueryRegistry<iEngine> (psserver->GetObjectReg());
    csRef< iMeshWrapper > nullmesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", "pos_anchor", isec, action->position);
    if ( !nullmesh )
    {
        Error1("Could not create gemActionLocation because crystalspace.mesh.onbject.null could not be created.");
        return ;
    }
    csRef<iNullMeshState> state =  scfQueryInterface<iNullMeshState> (nullmesh->GetMeshObject());
    if (!state)
    {
        Error1("No NullMeshState.");
        return ;
    }
    state->SetRadius(1.0);

    pcmesh->SetMesh( nullmesh );

    if (!InitProximityList(DEF_PROX_DIST,clientnum))
    {
        Error1("Could not create gemActionLocation because prox list could not be Init'd.");
        return;
    }

}

void gemActionLocation::Broadcast(int clientnum, bool control )
{
    psPersistActionLocation mesg(
                                    clientnum,
                                    eid,
                                    -2,
                                    name,
                                    sector->QueryObject()->GetName(),
                                    action->meshname
                                 );

    mesg.Multicast(GetMulticastClients(),clientnum,PROX_LIST_ANY_RANGE);

}

float gemActionLocation::GetBaseAdvertiseRange()
{
    /*if ( action == NULL )
        return gemObject::GetBaseAdvertiseRange();
    return action->radius;*/

    // todo - check if action location has proximity item
    //     if so check itemdata->GetVisibleDistance();
    return 10000.0f;
}

bool gemActionLocation::SeesObject(gemObject * object, float range)
{
    if (worldInstance == object->GetInstance())
    {
        return true;
    }
    else
    {
        return false;
    }
}


void gemActionLocation::Send( int clientnum, bool , bool to_superclient )
{
    psPersistActionLocation mesg(
                                    clientnum,
                                    eid,
                                    -2,
                                    name.GetData(),
                                    sector->QueryObject()->GetName(),
                                    action->meshname
                                 );


    if (clientnum && !to_superclient)
    {
        mesg.SendMessage();
    }

    /* TODO: Include if superclient need action locations
    if (to_superclient)
    {
        if (clientnum == 0) // Send to all superclients
        {
            mesg.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
        }
        else
        {
            mesg.SendMessage();
        }
    }
    */
}
//-====================================================================================-

//--------------------------------------------------------------------------------------
// gemActor
//--------------------------------------------------------------------------------------

gemActor::gemActor( psCharacter *chardata,
                       const char* factname,
                       const char* filename,
                       InstanceID myInstance,
                       iSector* room,
                       const csVector3& pos,
                       float rotangle,
                       int clientnum) :
  gemObject(chardata->GetCharFullName(),factname,filename,myInstance,room,pos,rotangle,clientnum),
psChar(chardata), factions(NULL), DRcounter(0), lastDR(0), lastV(0), lastSentSuperclientPos(0, 0, 0),
lastSentSuperclientInstance(-1), numReports(0), reportTargetId(0), isFalling(false), invincible(false), visible(true), viewAllObjects(false), meshcache(factname),
movementMode(0), isAllowedToMove(true), atRest(true), pcmove(NULL),
nevertired(false), infinitemana(false), instantcast(false), safefall(false), givekillexp(false)
{
    pid = chardata->GetPID();

    cel->AddActorEntity(this);

    // Store a safe reference to the client
    Client* client = psserver->GetConnections()->FindAny(clientnum);
    if (client) clientRef = client;
    if (clientnum && !client)
    {
        Error3("Failed to find client %d for %s!", clientnum, ShowID(pid));
        return;
    }

    SetAlive(true);

    if (!InitLinMove(pos,rotangle,room))
    {
        Error1("Could not initialize LinMove prop class, so actor not created.");
        return;
    }

    chardata->SetActor(this);

    if (!InitCharData(client))
    {
        Error1("Could not init char data.  Actor not created.");
        return;
    }

    Debug6(LOG_NPC,0,"Successfully created actor %s at %1.2f,%1.2f,%1.2f in sector %s.\n",
        factname,pos.x,pos.y,pos.z,sector->QueryObject()->GetName() );

    SetPrevTeleportLocation(pos, rotangle, sector);

    // Set the initial valid location to be the spot the actor was created at.
    UpdateValidLocation(pos, 0.0f, rotangle, sector, true);

    GetCharacterData()->SetStaminaRegenerationStill();
}

gemActor::~gemActor()
{
    // Disconnect while pointers are still valid
    Disconnect();

    cel->RemoveActorEntity(this);

    if (factions)
    {
        delete factions;
        factions = NULL;
    }

    if (psChar)
    {
        // Not deleting here anymore, but pushing to the global cache

        // delete psChar;
        // psChar = NULL;
        psChar->SetActor(NULL); // clear so cached object doesn't attempt to retain this
        CacheManager::GetSingleton().AddToCache(psChar, CacheManager::GetSingleton().MakeCacheName("char",psChar->GetPID().Unbox()),120);
        psChar = NULL;
    }

    if (numReports > 0)
        logging_chat_file = 0;  //This should close the file.

    delete pcmove;
}

Client* gemActor::GetClient() const
{
    return clientRef.IsValid() ? clientRef : NULL ;
}

bool gemActor::MoveToSpawnPos()
{
    csVector3 startingPos;
    float startingYrot;
    iSector* startingSector;

    if (!GetSpawnPos(startingPos,startingYrot,startingSector))
        return false;

    pcmove->SetOnGround(false);
    SetPosition(startingPos,startingYrot, startingSector);
    pcmove->SetVelocity(csVector3(0,0,0));  // clear velocity also, so falling people stop falling
    UpdateProxList(true);  // true= force update
    MulticastDRUpdate();
    return true;
}

bool gemActor::GetSpawnPos(csVector3& pos, float& yrot, iSector*& sector)
{
    float x, y, z;
    const char* sectorname;

    psRaceInfo *raceinfo = psChar->GetRaceInfo();
    if (!raceinfo)
        return false;

    raceinfo->GetStartingLocation(x,y,z,yrot,sectorname);

    if(!sectorname)
    {
        x = -19;
        y = -4;
        z = -144;
        yrot = 3.85f;
        sectorname = "hydlaa_plaza";
    }

    pos = csVector3(x, y, z);

    sector = EntityManager::GetSingleton().FindSector(sectorname);
    if (!sector)
        return false;
    else
        return true;
}

void gemActor::SetAllowedToMove(bool newvalue)
{
    if (isAllowedToMove == newvalue)
        return;

    isAllowedToMove = newvalue;

    // The server will ignore any attempts to move,
    // and we'll request the client not to send any.
    psMoveLockMessage msg(GetClientID(),!isAllowedToMove);
    msg.SendMessage();
}

void gemActor::SetAllowedToDisconnect(bool allowed)
{
    Client * client = GetClient();
    if (client)
    {
        client->SetAllowedToDisconnect(allowed);
    }
}



bool gemActor::MoveToValidPos(bool force)
{
    // Don't allow /unstick within 15 seconds of starting a fall...that way,
    // people can't jump off a cliff and use it to save themselves.
    if (!force && (!isFalling || (csGetTicks() - fallStartTime < UNSTICK_TIME)))
        return false;

    isFalling = false;
    pcmove->SetOnGround(false);
    SetPosition(valid_location.pos,valid_location.yrot, valid_location.sector);
    StopMoving(true);
    pcmove->AddVelocity(csVector3(0.0f, valid_location.vel_y, 0.0f));
    UpdateProxList(true);  // true= force update
    MulticastDRUpdate();
    return true;
}

void gemActor::GetValidPos(csVector3& pos, float& yrot, iSector*& sector)
{
    pos = valid_location.pos;
    yrot = valid_location.yrot;
    sector = valid_location.sector;
}

void gemActor::SetVisibility(bool visible)
{
    this->visible = visible;
    UpdateProxList(true);
    /* Queue Visibility and Invincibility flags to the superclient.
       The superclient should than select whether to react on the actor.
    */
    psserver->GetNPCManager()->QueueFlagPerception(this);
}

void gemActor::SetInvincibility(bool invincible)
{
    this->invincible = invincible;
    /* Queue Visibility and Invincibility flags to the superclient.
       The superclient should than select whether to react on the actor.
    */
    psserver->GetNPCManager()->QueueFlagPerception(this);
}

void gemActor::SetSecurityLevel(int level)
{
    securityLevel = level;
    masqueradeLevel = level; // override any current masquerading setting
}

void gemActor::SetMasqueradeLevel(int level)
{
    masqueradeLevel = level;
    UpdateProxList(true);
    /* Queue Visibility and Invincibility flags to the superclient.
       The superclient should than select whether to react on the actor.
    */
    psserver->GetNPCManager()->QueueFlagPerception(this);
}

bool gemActor::SeesObject(gemObject * object, float range)
{
    bool res = (worldInstance == object->GetInstance() || object->GetInstance() == INSTANCE_ALL || GetInstance() == INSTANCE_ALL)
                  &&
               (GetBaseAdvertiseRange() >= range)
                  &&
               (object==this  ||  object->GetVisibility()  ||  GetViewAllObjects() );

#ifdef PSPROXDEBUG
    psString log;
    log.AppendFmt("%s object: %s %s in Instance %d vs %s %s in %d and baseadvertise %.2f vs range %.2f\n",
                  res?"Sees":"Dosn't see", name.GetData(), GetViewAllObjects()?"sees all":"see visible",worldInstance,
                  object->GetVisibility()?"Visible":"Invisible",object->GetName(), object->GetInstance(),
                  GetBaseAdvertiseRange(),range);
    CPrintf(CON_DEBUG, "%s\n", log.GetData());    //use CPrintf because Error1() is breaking lines
#endif
    return res;
}

void gemActor::SetViewAllObjects(bool v)
{
    viewAllObjects = v;
    UpdateProxList(true);
}

void gemActor::StopMoving(bool worldVel)
{
    // stop this actor from moving
    csVector3 zeros(0.0f, 0.0f, 0.0f);
    pcmove->SetVelocity(zeros);
    pcmove->SetAngularVelocity(zeros);
    if(worldVel)
        pcmove->ClearWorldVelocity();
}

csPtr<PlayerGroup> gemActor::GetGroup()
{
    return csPtr<PlayerGroup>(group);
}

void gemActor::Resurrect()
{
    // Note that this does not handle NPCs...see SpawnManager::Respawn.

    Debug2(LOG_COMBAT, pid.Unbox(), "Resurrect event triggered for %s.\n", GetName());

    // Check if the current player is in npcroom or tutorial. In that case we
    // teleport back to npcroom or tutorial on resurrect. Otherwise we teleport
    // to DR.
    csVector3 pos;
    float yRot;
    iSector * sector;
    pcmove->GetLastPosition(pos, yRot, sector);
    pcmove->SetOnGround(false);
    StopMoving(true);

    // Set the mode before teleporting so people in the vicinity of the destination
    // get the right mode (peace) in the psPersistActor message they receive.
    // Note that characters are still movelocked when this is reset.
    SetMode(PSCHARACTER_MODE_PEACE);

    if (sector && !strncmp ("NPCroom", sector->QueryObject()->GetName(), 7))
        cel->Teleport( this, -20.0f, 1.0f, -180.0f, 0.0f, "NPCroom" );
    else if (sector && !strncmp ("tutorial", sector->QueryObject()->GetName(), 8))
        cel->Teleport( this, -232.0f, 21.31f, 31.5f, 4.0f, "tutorial" );
    else
    {
        // TODO: Get Death Realm location from db somewhere
        cel->Teleport( this, -29.2f, -119.0f, 28.2f, 0.0f, "DR01");
        SetInstance(DEFAULT_INSTANCE);
    }

    psChar->SetHitPoints(psChar->GetHitPointsMax());

    //Do not reset mana to max while in DR, to prevent exploits using /die
    if (sector && strncmp ("DR", sector->QueryObject()->GetName(), 2))
        psChar->SetMana(psChar->GetManaMax());

    psChar->SetStamina(psChar->GetStaminaMax(true), true);
    psChar->SetStamina(psChar->GetStaminaMax(false), false);
    psChar->SetHitPointsRate(HP_REGEN_RATE);
    psChar->SetManaRate(MANA_REGEN_RATE);

    BroadcastTargetStatDR( psserver->GetNetManager()->GetConnections() );
}

void InvokeScripts(csArray<csString> & scripts, gemActor * actor, gemActor * target, psItem * item)
{
    unsigned int scriptID;

    if (scripts.GetSize())
    {
        Debug4(LOG_COMBAT, actor->GetPID().Unbox(), "-----InvokeScripts %zu  %s %s", scripts.GetSize(), actor->GetName(), target->GetName());
        for (scriptID=0; scriptID < scripts.GetSize(); scriptID++)
        {
            if (scripts[scriptID].Length() > 0)
            {
                Debug2(LOG_COMBAT, actor->GetPID().Unbox(), "-----InvokeScripts script %s", scripts[scriptID].GetData());
                psserver->GetProgressionManager()->ProcessEvent(scripts[scriptID], actor, target, item);
            }
        }
    }
}

void gemActor::InvokeAttackScripts(gemActor * target, psItem * item)
{
    InvokeScripts(onAttackScripts, this, target, item);
}

void gemActor::InvokeDamageScripts(gemActor * attacker, psItem * item)
{
    InvokeScripts(onDamageScripts, this, attacker, item);
}

void gemActor::DoDamage(gemActor * attacker, float damage, float damageRate, csTicks duration)
{
    // Handle trivial "already dead" case
    if ( !IsAlive())
    {
        if (attacker && attacker->GetClientID() )
            psserver->SendSystemInfo(attacker->GetClientID(),"It's already dead.");
        return;
    }

    // Successful attack, if >30% max hp then interrupt spell
    if ( damage > (this->GetCharacterData()->GetHitPointsMax() * 0.3F ) )
        this->GetCharacterData()->InterruptSpellCasting();

    if (damageRate == 0)
        if(GetCharacterData()->GetHP() - damage < 0)
            damage = GetCharacterData()->GetHP();


    // Add dmg to history
    AddAttackerHistory( attacker, damage, damageRate, duration );

    float hp = psChar->AdjustHitPoints(-damage);
    if (damageRate)
        psChar->AdjustHitPointsRate(damageRate);

    if (damage != 0.0)
    {
        if (attacker != NULL)
        {
            Debug5(LOG_COMBAT, attacker->GetPID().Unbox(), "%s do damage %1.2f on %s to new hp %1.2f", attacker->GetName(), damage, GetName(), hp);
        }
        else
        {
            Debug4(LOG_COMBAT,0,"damage %1.2f on %s to new hp %1.2f", damage,GetName(),hp);
        }

        /// Allow rest of server to process this damage
        psDamageEvent evt(attacker, this, damage);
        evt.FireEvent();
    }

    // Death takes some special work
    if (fabs(hp) < 0.0005f || GetMode() == PSCHARACTER_MODE_DEFEATED)  // check for dead
    {
        // If no explicit killer, look for the last person to cast a DoT spell.
        if (!attacker && dmgHistory.GetSize() > 0)
        {
            for (int i = (int) dmgHistory.GetSize() - 1; i >= 0; i--)
            {
                if (dmgHistory[i]->damageRate < 0)
                {
                    attacker = dmgHistory[i]->attacker_ref;
                    break;
                }
            }
        }
        // If in a duel, don't immediately kill...defeat.
        if (attacker && GetMode() != PSCHARACTER_MODE_DEFEATED && GetClient() && GetClient()->IsDuelClient(attacker->GetClientID()))
        {
            // Auto-unlock after a few seconds if not explicitly killed first
            psSpareDefeatedEvent *evt = new psSpareDefeatedEvent(this);
            psserver->GetEventManager()->Push(evt);
            SetMode(PSCHARACTER_MODE_DEFEATED);

            psserver->SendSystemError(GetClientID(), "You've been defeated by %s!", attacker->GetName());
            GetClient()->AnnounceToDuelClients(attacker, "defeated");
        }
        else
        {
            psDeathEvent death(this,attacker);
            death.FireEvent();

            if (GetMode() == PSCHARACTER_MODE_DEFEATED && attacker)
            {
                psserver->SendSystemError(GetClientID(), "You've been slain by %s!", attacker->GetName());
                GetClient()->AnnounceToDuelClients(attacker, "slain");
            }
            SetMode(PSCHARACTER_MODE_DEAD);
        }

        // if damage due to spell then spell is ending anyway, so no need to force
        // 'stop attack.'
        if (attacker && attacker->GetMode() == PSCHARACTER_MODE_COMBAT)
        {
            psserver->combatmanager->StopAttack(attacker);
        }
    }

    // Update group stats and it's own
    SendGroupStats();
}

void gemActor::AddAttackerHistory(gemActor * attacker, float damage, float damageRate, csTicks duration )
{
    if(attacker)
    {
        DamageHistory* dmg = new DamageHistory();
        dmg->attacker_ref = attacker;
        dmg->timestamp = csGetTicks();
        dmg->hp = (int) GetCharacterData()->GetHP();

        if (damageRate < 0)
        {
            dmg->damageRate = damageRate;
            dmg->damage = (-damageRate/1000) * duration;  // max theoretical dmg from DoT spell

            // Add it
            dmgHistory.Push(dmg);
        }
        else if (damageRate == 0)
        {
            dmg->damageRate = damageRate;
            dmg->damage = damage;

            // Add it
            dmgHistory.Push(dmg);
        }
    }
}

void gemActor::RemoveAttackerHistory(gemActor * attacker)
{
    if (attacker && dmgHistory.GetSize() > 0)
    {
        // Count backwards to avoid trouble with shifting indexes
        for (size_t i = dmgHistory.GetSize() - 1; i != (size_t) -1; i--)
        {
            if (dmgHistory[i]->attacker_ref == attacker)
                dmgHistory.DeleteIndex(i);
        }
    }
}

bool gemActor::CanBeAttackedBy(gemActor *attacker, gemActor ** lastAttacker) const
{
    *lastAttacker = NULL;

    if (GetDamageHistoryCount() == 0)
    {
        return true;
    }

    csTicks lasttime = csGetTicks();

    for (int i = (int)GetDamageHistoryCount(); i>0; i--)
    {
        const DamageHistory *lasthit = GetDamageHistory(i-1);

        // any 15 second gap is enough to make us stop looking
        if (lasttime - lasthit->timestamp > 15000)
        {
            return true;
        }
        else
        {
            lasttime = lasthit->timestamp;
        }

        if (!lasthit->attacker_ref.IsValid())
            continue;  // ignore disconnects

        *lastAttacker = dynamic_cast<gemActor*>((gemObject*) lasthit->attacker_ref);
        if (*lastAttacker == NULL)
            continue;  // shouldn't happen

        // If someone else, except for attacker pet, hit first and attacker not grouped with them,
        // attacker locked out
        if ( *lastAttacker != attacker && !attacker->IsGroupedWith(*lastAttacker) &&
             !attacker->IsMyPet(*lastAttacker) )
        {
            return false;
        }
    }

    return true;
}


void gemActor::UpdateStats()
{
    if (psChar)
    {
        if (!psChar->UpdateStatDRData(csGetTicks()))
            return;

        SendGroupStats();
    }
}

float gemActor::DrainMana(float adjust,bool absolute)
{
    // GM flag
    if (infinitemana)
        return GetCharacterData()->GetMana();

    float finalMana = 0;
    if(absolute)
    {
        GetCharacterData()->SetMana(adjust);
        finalMana = GetCharacterData()->GetMana();
    }
    else
    {
        //mental stamina is only adjusted, never set
        GetCharacterData()->AdjustStamina(adjust, false);
        finalMana = GetCharacterData()->AdjustMana(adjust);
    }
    SendGroupStats();
    return finalMana;
}

void gemActor::HandleDeath()
{
    // Notifiy recievers that this actor has died.
    // Recievers will have to register again if they
    // want to be continually notified of deaths
    while(deathReceivers.GetSize() > 0)
    {
        iDeathCallback *receiver = deathReceivers.Pop();
        receiver->DeathCallback(this);
    }
}

void gemActor::SendGroupStats()
{
    if (InGroup() || GetClientID())
    {
        psChar->SendStatDRMessage(GetClientID(), eid, 0, InGroup() ? GetGroup() : NULL);
    }

    gemNPC* npc = dynamic_cast<gemNPC*>(this);
    if (npc && npc->GetCharacterData()->IsPet())
    {
        // Get Client ID of Owner
        gemObject *owner = npc->GetOwner();
        if (owner && owner->GetClientID())
        {
            psChar->SendStatDRMessage(owner->GetClientID(), eid, 0);
        }
    }
}

void gemActor::Send( int clientnum, bool control, bool to_superclient  )
{
    csRef<PlayerGroup> group = this->GetGroup();
    csString texparts;
    csString equipmentParts;

    psChar->MakeTextureString( texparts );
    psChar->MakeEquipmentString( equipmentParts );

    csVector3 pos;
    float yRot;
    iSector * sector;
    pcmove->GetLastPosition(pos, yRot, sector);

    csString guildName;
    guildName.Clear();
    if ( psChar->GetGuild() && !psChar->GetGuild()->IsSecret() )
    {
        guildName = psChar->GetGuild()->name;
    }

    uint32_t groupID = 0;
    if(group)
    {
        groupID = group->GetGroupID();
    }

    uint32_t flags = 0;
    csString helmGroup = psChar->GetHelmGroup();

    if (!GetVisibility())    flags |= psPersistActor::INVISIBLE;
    if (GetInvincibility())  flags |= psPersistActor::INVINCIBLE;

    Client* targetClient = psserver->GetConnections()->Find(clientnum);
    if (targetClient && targetClient->GetCharacterData())
    {
        if (((GetClient()->GetSecurityLevel() >= GM_LEVEL_0) &&
            GetCharacterData()->GetGuild()) ||
            (targetClient->GetSecurityLevel() >= GM_LEVEL_0) ||
            targetClient->GetCharacterData()->Knows(psChar))
        {
            flags |= psPersistActor::NAMEKNOWN;
        }
    }

    psPersistActor mesg( clientnum,
                         securityLevel,
                         masqueradeLevel,
                         control,
                         name,
                         guildName,
                         factname,
                         filename,
                         psChar->GetRaceInfo()->name,
                         psChar->GetRaceInfo()->gender,
                         helmGroup,
                         top, bottom,offset,
                         texparts,
                         equipmentParts,
                         DRcounter,
                         eid,
                         CacheManager::GetSingleton().GetMsgStrings(),
                         pcmove,
                         movementMode,
                         GetMode(),
                         0, // playerID should not be distributed to clients
                         groupID,
                         0, // ownerEID
                         flags
                         );

    // Only send to client
    if (clientnum && !to_superclient)
    {
        mesg.SendMessage();
    }

    if (to_superclient)
    {
        mesg.SetPlayerID(pid); // Insert player id before sending to super client.
        mesg.SetInstance(GetInstance());
        if (clientnum == 0) // Send to all superclients
        {
            Debug1(LOG_SUPERCLIENT, clientnum, "Sending gemActor to superclients.\n");
            mesg.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
        }
        else
        {
            mesg.SendMessage();
        }
    }
}


void gemActor::Broadcast(int clientnum, bool control)
{
    csArray<PublishDestination>& dest = GetMulticastClients();
    for (unsigned long i = 0 ; i < dest.GetSize(); i++)
    {
        if (dest[i].dist < PROX_LIST_ANY_RANGE)
            Send(dest[i].client, control, false);
    }

    // Update the labels of people in your secret guild
    if ( psChar->GetGuild() && psChar->GetGuild()->IsSecret() )
    {
        psUpdatePlayerGuildMessage update(
            clientnum,
            eid,
            psChar->GetGuild()->GetName());

        psserver->GetEventManager()->Broadcast(update.msg,NetBase::BC_GUILD,psChar->GetGuild()->id);
    }
}

gemObject *gemActor::FindNearbyActorName(const char *name)
{
    return proxlist->FindObjectName(name);
}

const char* gemActor::GetGuildName()
{
    if(GetGuild())
        return GetGuild()->GetName();
    else
        return NULL;
}

void gemActor::AddChatReport(gemActor *target)
{
    numReports++;

    if (numReports == 1)
    {
        csRef<iVFS> vfs =  csQueryRegistry<iVFS> (cel->object_reg);
        if (!vfs)
        {
            numReports = 0;
            return;
        }
        csString tmp;

        reportTargetId = target->GetClientID();

        tmp.Format("/this/logs/report_chat_%s_by_%s.log",
                   target->GetCharacterData()->GetCharName(),
                   GetCharacterData()->GetCharName());

        logging_chat_file = vfs->Open(tmp, VFS_FILE_APPEND);
        if (!logging_chat_file)
        {
            Error1("Error, could not open log file.");
            return;
        }
        logging_chat_file->Write("Starting to log ", 16);
        logging_chat_file->Write(target->GetCharacterData()->GetCharName(),
                                 strlen(target->GetCharacterData()->GetCharName()));
        logging_chat_file->Write("\n", 1);
        logging_chat_file->Write("Log by the request of ", 22);
        logging_chat_file->Write(GetCharacterData()->GetCharName(),
                                 strlen(GetCharacterData()->GetCharName()));
        logging_chat_file->Write("\n\n", 2);
        logging_chat_file->Write("Chat history from ", 18);
        logging_chat_file->Write(GetCharacterData()->GetCharName(),
                                 strlen(GetCharacterData()->GetCharName()));
        logging_chat_file->Write(":\n", 2);
        logging_chat_file->Write("=====================================\n", 38);
        for (size_t i = 0; i < chatHistory.GetSize(); i++)
        {
            csString *s = chatHistory.Get(i);
            logging_chat_file->Write(s->GetData(), s->Length());
            logging_chat_file->Write("\n", 1);
        }
        logging_chat_file->Write("=====================================\n\n", 39);
    }
}

void gemActor::RemoveChatReport()
{
    numReports--;

    if (numReports == 0)
    {
        Notify2( LOG_ANY, "Logging of chat messages for '%s' stopped\n", ShowID(pid));

        logging_chat_file->Write("=====================================\n\n", 39);

        //This should close the file.
        logging_chat_file = 0;

        reportTargetId = 0;

        // Inform the reporter
        psserver->SendSystemInfo(GetClientID(), "Logging of chat messages stopped.");
    }
}

bool gemActor::LogMessage(const char *who, const psChatMessage &msg)
{
    csString *s = new csString;

    // Format the message
    switch (msg.iChatType)
    {
        case CHAT_AUCTION:
        {
            s->Format("Auction from %s: %s", who, msg.sText.GetData());
            break;
        }
        case CHAT_SHOUT:
        {
            s->Format("%s shouts: %s", who, msg.sText.GetData());
            break;
        }
        case CHAT_SAY:
        {
            s->Format("%s says: %s", who, msg.sText.GetData());
            break;
        }
        case CHAT_TELL:
        {
            s->Format("%s tells to %s: %s", who, msg.sPerson.GetData(),
                      msg.sText.GetData());
            break;
        }
        default:
            return false; // Currently we ignore any other chat types
    }

    // Add to the chat history
    chatHistory.Push(s);

    // Delete older message in the history buffer
    while (chatHistory.GetSize() > CHAT_HISTORY_SIZE)
        chatHistory.DeleteIndex(0);

    // Nothing else to do if the number of reports is 0
    if (numReports == 0)
        return false;

    // Write to the log file
    csString tod;
    GetTimeOfDay(tod);
    csString line;
    line.Format("[%s] :", tod.GetData());

    logging_chat_file->Write(line.GetData(), line.Length());
    logging_chat_file->Write(s->GetData(), s->Length());
    logging_chat_file->Write("\n", 1);

    return true;
}

/**
* Determines the right size and height for the collider for the sprite
* and sets the actual position of the sprite.
*/
bool gemActor::InitLinMove (const csVector3& pos,
                            float angle, iSector* sector)
{
    pcmove = new psLinearMovement(psserver->GetObjectReg());

    // Check if we're using sizes from the db.
    csVector3 size;
    psRaceInfo *raceinfo = psChar->GetRaceInfo();
    raceinfo->GetSize(size);
    float width = 0.0f;
    float height = 0.0f;
    float depth = 0.0f;

    if(!(size.x && size.y && size.z))
    {
        // Now Determine CD bounding boxes for upper and lower colliders
        csRef<iSpriteCal3DState> cal3d;
        cal3d = scfQueryInterface<iSpriteCal3DState> ( GetMeshWrapper()->GetMeshObject ());
        if (cal3d)
        {
            cal3d->SetAnimCycle("stand",1);
        }
        const csBox3& box = GetMeshWrapper()->GetMeshObject()->GetObjectModel()->GetObjectBoundingBox();

        width  = box.MaxX() - box.MinX();
        height = box.MaxY() - box.MinY();
        depth  = box.MaxZ() - box.MinZ();
    }

    if(size.x != 0)
        width = size.x;
    if(size.y != 0)
        height = size.y;
    if(size.z != 0)
        depth = size.z;

    // Add a fudge factor to the depth to allow for feet
    // sticking forward while running
    depth *= 1.33F;

    // Now check if the size within limits.
    // Can't be to small or the ExtrapolatePosition in NPCClient
    // will take to long to process.
    if (width < 0.2)
    {
        Warning4(LOG_ANY, "Width %.2f to small for %s(%s)", width, GetName(), ShowID(pid));
        width = 0.2F;
    }
    if (depth < 0.2*1.33)
    {
        Warning4(LOG_ANY, "Depth %.2f to small for %s(%s)", depth, GetName(), ShowID(pid));
        depth = 0.2F*1.33F;
    }
    if (height < 0.2F)
    {
        Warning4(LOG_ANY, "Height %.2f to small for %s(%s)", height, GetName(), ShowID(pid));
        height = 0.2F;
    }

    float legSize;
    if(height > 0.8F)
    {
        legSize = 0.7F;
    }
    else
    {
        legSize = height * 0.5f;
    }

    top = csVector3(width,height-legSize,depth);
    bottom = csVector3(width-0.1,legSize,depth-0.1);
    offset = csVector3(0,0,0);
    top.x *= .7f;
    top.z *= .7f;
    bottom.x *= .7f;
    bottom.z *= .7f;

    pcmove->InitCD(top, bottom,offset, GetMeshWrapper());
    // Disable CD checking because server does not need it.
    pcmove->UseCD(false);

    SetPosition(pos,angle,sector);

    return true;  // right now this func never fail, but might later.
}

bool gemActor::SetupCharData()
{
    csString texparts;
    csString race,file,gender;
    csString faction_standings;

    faction_standings = psChar->GetFactionStandings();

    factions = new FactionSet(faction_standings,CacheManager::GetSingleton().GetFactionHash() );

    return true;  // right now this func never fail, but might later.
}


bool gemActor::InitCharData(Client* c)
{
    if ( !c )
    {
        // NPC
        SetSecurityLevel( -1 );
    }
    else
    {
        SetSecurityLevel( c->GetSecurityLevel() );
        SetGMDefaults();
    }

    return SetupCharData();

}

void gemActor::SetTextureParts(const char *parts)
{
}

void gemActor::SetEquipment(const char *equip)
{
}

// This function should only be run on initial load
void gemActor::SetGMDefaults()
{
    if ( CacheManager::GetSingleton().GetCommandManager()->Validate(securityLevel, "default invincible") )
    {
        invincible = true;
        safefall = true;
        nevertired = true;
        infinitemana = true;
    }

    if ( CacheManager::GetSingleton().GetCommandManager()->Validate(securityLevel, "default invisible") )
    {
        visible = false;
        viewAllObjects = true;
    }

    if ( CacheManager::GetSingleton().GetCommandManager()->Validate(securityLevel, "default infinite inventory") )
    {
        SetFiniteInventory(false);
    }

    questtester = false;  // Always off by default
}

void gemActor::SetInstance(InstanceID worldInstance)
{
    this->worldInstance = worldInstance;
}

void gemActor::SetPosition(const csVector3& pos,float angle, iSector* sector)
{
    this->pos = pos;
    this->yRot = angle;
    this->sector = sector;

    DRcounter += 3;

    pcmove->SetPosition(pos,angle,sector);

    psSectorInfo* sectorInfo = NULL;
    if (sector != NULL)
        sectorInfo = CacheManager::GetSingleton().GetSectorInfoByName( sector->QueryObject()->GetName() );
    if (sectorInfo != NULL)
    {
        psChar->SetLocationInWorld(worldInstance, sectorInfo, pos.x, pos.y, pos.z, angle );
        UpdateValidLocation(pos, 0.0f, angle, sector, true);
    }
}

//#define STAMINA_PROCESS_DEBUG

void gemActor::ProcessStamina()
{
    csVector3 vel = pcmove->GetVelocity();
    ProcessStamina(vel, true);
}

void gemActor::ProcessStamina(const csVector3& velocity, bool force)
{
    // GM flag
    if (nevertired)
        return;

    csTicks now = csGetTicks();

    if (lastDR == 0)
        lastDR = now;

    csTicks elapsed = now - lastDR;

    // Update once a second, or on any updates with upward motion
    if (elapsed > 1000 || velocity.y > SMALL_EPSILON || force)
    {
        lastDR += elapsed;

        float times = 1.0f;

            /* If we're slow or forced, we need to multiply the stamina with the secs passed.
             * DR updates are sent when speed changes, so it should be quite accurate.
             */
            times = float(elapsed) / 1000.0f;

        atRest = velocity.IsZero();

        #ifdef STAMINA_PROCESS_DEBUG
            printf("Processing stamina with %u elapsed since last\n", elapsed );
        #endif

        ApplyStaminaCalculations(velocity,times);
    }
}

/// Event to recheck stamina after a delay
class psStaminaRest : public psGameEvent
{
protected:
    csWeakRef<gemObject> actor;
public:
    psStaminaRest(int length, gemActor* a) : psGameEvent(0,length,"psStaminaRest"), actor(a) {}
    virtual void Trigger()
    {
        gemActor* a = static_cast<gemActor*>((gemObject *) actor);
        if (a) a->ProcessStamina(0.0f,true);
    }
};

void gemActor::ApplyStaminaCalculations(const csVector3& v, float times)
{
    csVector3 thisV = lastV;
    lastV=v;

    // Script
    static MathScript* script = psserver->GetMathScriptEngine()->FindScript("StaminaMove");
    if (!script)
    {
        Error1("Missing script \"StaminaMove\"");
        return;
    }

    // Inputs
    static MathScriptVar* speed         =  script->GetOrCreateVar("Speed");        // How fast you're moving
    static MathScriptVar* ascent_angle  =  script->GetOrCreateVar("AscentAngle");  // How steep your climb is
    static MathScriptVar* weight        =  script->GetOrCreateVar("Weight");       // How much you're carrying
    static MathScriptVar* max_weight    =  script->GetOrCreateVar("MaxWeight");    // How much you can carry

    // Output
    static MathScriptVar* drain = script->GetOrCreateVar("Drain");  // The resultant drain in stamina this script produces

    if ( atRest )
    {
        #ifdef STAMINA_PROCESS_DEBUG
            printf("Not moving\n");
        #endif

        if (psChar->GetMode() == PSCHARACTER_MODE_PEACE || psChar->GetMode() == PSCHARACTER_MODE_SPELL_CASTING)
            psChar->SetStaminaRegenerationStill();
    }
    else // Moving
    {
        // unwork stuff
        if (psChar->GetMode() == PSCHARACTER_MODE_WORK )
        {
            SetMode(PSCHARACTER_MODE_PEACE);
            psserver->SendSystemInfo(GetClientID(),"You stop working.");
        }

        #ifdef STAMINA_PROCESS_DEBUG
            printf("Moving @");
        #endif

        // Compute stuff
        /*
                  ^
             v  / |
              /   | y
            /a}   |
           o------>
              xz
        */
        double Speed; ///< magnitude of the velocity vector
        double Angle; ///< angle between velocity vector and X-Z plane

        double XZvel = csQsqrt(thisV.x*thisV.x + thisV.z*thisV.z);
        if (thisV.y > EPSILON)
        {
            if (XZvel > EPSILON)
            {
                Speed = csQsqrt(thisV.y*thisV.y + XZvel*XZvel);
                Angle = atan(thisV.y / XZvel);
            }
            else // straight up
            {
                Speed = thisV.y;
                Angle = HALF_PI;
            }
        }
        else // flat ground
        {
            Speed = XZvel;
            Angle = 0.0f;
        }

        #ifdef STAMINA_PROCESS_DEBUG
            printf(" %f %f =>", float(Speed), float(Angle) );
        #endif

        // Stuff goes in
        speed->         SetValue(Speed);
        ascent_angle->  SetValue(Angle);
        weight->        SetValue((double)psChar->Inventory().GetCurrentTotalWeight());
        max_weight->    SetValue((double)psChar->Inventory().MaxWeight());

        // Do stuff with stuff
        script->Execute();

        // Stuff comes out
        float value = drain->GetValue();
        //value *= times;

        #ifdef STAMINA_PROCESS_DEBUG
            printf(" %f\n", value );
        #endif

        // Apply stuff
        if (psChar->GetMode() == PSCHARACTER_MODE_PEACE || psChar->GetMode() == PSCHARACTER_MODE_SPELL_CASTING)
        {
            psChar->SetStaminaRegenerationWalk();
            psChar->AdjustStaminaRate(-value,true);
            psChar->AdjustStamina(-value*times,true);
        }
        else  // Another regen in place
        {
            psChar->AdjustStamina(-value*times,true);
        }
    }

    // Check stuff
    if (psChar->GetStamina(true) < 0.05f)
    {
        #ifdef STAMINA_PROCESS_DEBUG
            printf("Stopping\n");
        #endif

        SetMode(PSCHARACTER_MODE_EXHAUSTED);
        psChar->SetStaminaRegenerationStill();
        psserver->SendSystemResult(GetClientID(),"You're too exhausted to move");

        // Wake us up in a couple seconds
        psserver->GetEventManager()->Push( new psStaminaRest(2000,this) );
    }
    else if (GetMode() == PSCHARACTER_MODE_EXHAUSTED)
    {
        SetMode(PSCHARACTER_MODE_PEACE);
    }
}

bool gemActor::SetDRData(psDRMessage& drmsg)
{
    if (drmsg.IsNewerThan(DRcounter))
    {
        pcmove->SetDRData(drmsg.on_ground,1.0f,drmsg.pos,drmsg.yrot,drmsg.sector,drmsg.vel,drmsg.worldVel,drmsg.ang_vel);
        DRcounter = drmsg.counter;
    }
    else
    {
        //printf("Entity %d [%s] has DRcounter of %d, received DRcounter of %d. Ignoring it.\n",
        //       this->GetEntity()->GetID(), this->name.GetData(), DRcounter, drmsg.counter);
        return false;  // don't do the rest of this if this msg is out of date
    }

    // Apply stamina only on PCs
    if (GetClientID())
        ProcessStamina(drmsg.vel + drmsg.worldVel);

    if (drmsg.sector != NULL)
    {
        UpdateValidLocation(drmsg.pos, drmsg.vel.y, drmsg.yrot, drmsg.sector);
        psSectorInfo* sectorInfo = CacheManager::GetSingleton().GetSectorInfoByName( drmsg.sector->QueryObject()->GetName() );
        if (sectorInfo != NULL)
        {
            psChar->SetLocationInWorld(worldInstance,sectorInfo, drmsg.pos.x, drmsg.pos.y, drmsg.pos.z, drmsg.yrot );

            if ( psChar->IsSpellCasting() && ( PSABS( drmsg.vel.SquaredNorm() ) > 13.0f ) )
            {
                psChar->InterruptSpellCasting();
            }
        }
    }
    return true;
}

void gemActor::UpdateValidLocation(const csVector3& pos, float vel_y, float yrot, iSector* sector, bool force)
{
    // 10m hops
    if (force || (!isFalling && (pos - newvalid_location.pos).SquaredNorm() > 100.0f))
    {
        valid_location = newvalid_location;
        newvalid_location.pos = pos;
        newvalid_location.yrot = yrot;
        newvalid_location.vel_y = vel_y;
        newvalid_location.sector = sector;
        if(force)
            valid_location = newvalid_location;
    }

    last_location.pos = pos;
    last_location.yrot = yrot;
    last_location.vel_y = vel_y;
    last_location.sector = sector;
}

void gemActor::MoveToLastPos()
{
    pcmove->SetOnGround(false);
    SetPosition(last_location.pos, last_location.yrot, last_location.sector);
    StopMoving(true);
    UpdateProxList();
    MulticastDRUpdate();
}

void gemActor::GetLastLocation(csVector3& pos, float& vel_y, float& yrot, iSector*& sector)
{
    pos = last_location.pos;
    vel_y = last_location.vel_y;
    yrot = last_location.yrot;
    sector = last_location.sector;
}

void gemActor::SetPrevTeleportLocation(const csVector3& pos, float yrot, iSector* sector)
{
    prev_teleport_location.pos = pos;
    prev_teleport_location.yrot = yrot;
    prev_teleport_location.sector = sector;
}

void gemActor::GetPrevTeleportLocation(csVector3& pos, float& yrot, iSector*& sector)
{
    pos = prev_teleport_location.pos;
    yrot = prev_teleport_location.yrot;
    sector = prev_teleport_location.sector;
}

void gemActor::MulticastDRUpdate(MsgEntry *resend)
{
    if (!resend)
    {
        bool on_ground;
        float speed,yrot,ang_vel;
        csVector3 pos,vel,worldVel;
        iSector *sector;
        pcmove->GetDRData(on_ground,speed,pos,yrot,sector,vel,worldVel,ang_vel);
        psDRMessage drmsg(0, eid, on_ground, movementMode, DRcounter,
                          pos,yrot,sector, "", vel,worldVel,ang_vel,
                          CacheManager::GetSingleton().GetMsgStrings() );
        drmsg.msg->priority = PRIORITY_HIGH;
        drmsg.Multicast(GetMulticastClients(),0,PROX_LIST_ANY_RANGE);
    }
    else
    {
        psserver->GetEventManager()->Multicast(resend,GetMulticastClients(),0,PROX_LIST_ANY_RANGE);
    }
}

bool gemActor::UpdateDR()
{
    bool on_ground = pcmove->IsOnGround();
    // We do not want gravity enforced since server is not using CD.
    pcmove->SetOnGround(true);
    pcmove->UpdateDR();
    pcmove->SetOnGround(on_ground);
    return true;
}

void gemActor::GetLastSuperclientPos(csVector3& pos, InstanceID& instance) const
{
    pos = lastSentSuperclientPos;
    instance = lastSentSuperclientInstance;
}

void gemActor::SetLastSuperclientPos(const csVector3& pos, InstanceID instance)
{
    lastSentSuperclientPos = pos;
    lastSentSuperclientInstance = instance;
}


float gemActor::GetRelativeFaction(gemActor *speaker)
{
    return factions->FindWeightedDiff(speaker->factions);
}

void gemActor::SetGroup(PlayerGroup * group)
{
    PlayerGroup* oldGroup = this->group;
    this->group = group;

    int groupID = 0;
    bool self = false;
    if(group)
    {
        groupID = group->GetGroupID();
    }

    // Update group clients for group removal
    if(oldGroup != NULL)
    {
        for(int i = 0; i < (int)oldGroup->GetMemberCount(); i++)
        {
            gemActor* memb = oldGroup->GetMember(i);
            if(!memb)
                continue;

            if(memb == this)
                self = true; // No need to send after

            // Send group update
            psUpdatePlayerGroupMessage update(memb->GetClientID(), eid, groupID);

            // CPrintf(CON_DEBUG,"Sending oldgroup update to client %d (%s)\n",memb->GetClientID(),memb->GetName());

            update.SendMessage();
        }
    }

    // Send this actor to the new group to update their entity labels
    if(group != NULL)
    {
        for(int i = 0; i < (int)group->GetMemberCount(); i++)
        {
            gemActor* memb = group->GetMember(i);
            if(!memb)
                continue;

            if(memb == this && self) // already sent to ourselves
                continue;

            // Send us to the rest of the group
            psUpdatePlayerGroupMessage update1(memb->GetClientID(), eid, groupID);

            // Send the rest of the group to us (To make sure we got all the actors)
            psUpdatePlayerGroupMessage update2(GetClientID(),
                                               memb->GetEID(),
                                               groupID);

            // CPrintf(CON_DEBUG,"Sending group update to client %d (%s)\n",memb->GetClientID(),memb->GetName());
            update1.SendMessage();
            update2.SendMessage();
        }
    }

    // Finaly send the message if it hasn't been
    if(!self)
    {
        psUpdatePlayerGroupMessage update(GetClientID(), eid, groupID);
        update.SendMessage();
    }
}

int gemActor::GetGroupID()
{
    csRef<PlayerGroup> group = GetGroup();
    if(group)
    {
        return group->GetGroupID();
    }

    return 0;
}

bool gemActor::InGroup() const
{
    return group.IsValid();
}

bool gemActor::IsGroupedWith(gemActor *other) const
{
    return group.IsValid() && group->HasMember(other);
}

void gemActor::RemoveFromGroup()
{
    if (group.IsValid())
    {
        group->Remove(this);
    }
}

bool gemActor::IsMyPet(gemActor *other) const
{
    Client * client = GetClient();

    if (!client) return false; // Can't own a pet if no client

    return GetClient()->IsMyPet(other);
}



void gemActor::SendGroupMessage(MsgEntry *me)
{
    if (group.IsValid())
    {
        group->Broadcast(me);
    }
    else
    {
        psserver->GetEventManager()->SendMessage(me);
    }
}


void gemActor::SendTargetStatDR(Client *client)
{
    psChar->SendStatDRMessage(client->GetClientNum(), eid, DIRTY_VITAL_ALL);
}

void gemActor::BroadcastTargetStatDR(ClientConnectionSet *clients)
{

    // Send statDR to all clients that have this client targeted
    csArray<PublishDestination> &c = GetMulticastClients();
    for (size_t i=0; i < c.GetSize(); i++)
    {
        Client *cl = clients->Find(c[i].client);
        if (!cl)  // not ready yet
            continue;
        if ((gemActor*)cl->GetTargetObject() == this)  // client has this actor targeted
        {
            SendTargetStatDR(cl);
        }
    }

    // Send statDR to player's client
    Client * actorClient = clients->FindPlayer(pid);
    if (actorClient != NULL)
    {
        psChar->SendStatDRMessage(actorClient->GetClientNum(), eid, 0);
    }
}

csString gemActor::GetDefaultBehavior(const csString & dfltBehaviors)
{
    return ::GetDefaultBehavior(dfltBehaviors, 1);
}

void gemActor::SendBehaviorMessage(const csString & msg_id, gemObject *actor)
{
    if ( msg_id == "context" )
    {
        // If the player is in range of the item.
        if ( RangeTo(actor) < RANGE_TO_SELECT && actor->IsAlive() )
        {
            if (actor->GetClientID() == 0)
                return;
            if (!actor->GetCharacterData())
                return;

            int options = 0;

            options |= psGUIInteractMessage::PLAYERDESC;

            // Get the actor who is targetting. The target is *this.
            gemActor* activeActor = dynamic_cast<gemActor*>(actor);
            if (activeActor && activeActor->GetClientID() != 0)
            {
                if ( (activeActor->GetMode() == PSCHARACTER_MODE_PEACE || activeActor->GetMode() == PSCHARACTER_MODE_SIT) && this != activeActor )
                    options |= psGUIInteractMessage::EXCHANGE;

                // Can we attack this player?
                Client* meC = psserver->GetNetManager()->GetClient(GetClientID());
                if (IsAlive() && activeActor->IsAlive() && meC && meC->IsAllowedToAttack(actor,false))
                    options |= psGUIInteractMessage::ATTACK;

                /*Options for a wedding or a divorce, in order to show the proper button.
                 *If the character is married with the target, the only option is to divorce.*/

                if ( activeActor->GetCharacterData()->GetIsMarried()
                     && this != activeActor
                     && !strcmp( activeActor->GetCharacterData()->GetSpouseName(), GetCharacterData()->GetActor()->GetName() ) )
                {
                    options |= psGUIInteractMessage::DIVORCE;
                }
                //If the target is not married, then we can ask to marry him/her. Otherwise, no other option is valid.
                //However the target must be of the opposite sex, except in the Kran case.
                else if ( !activeActor->GetCharacterData()->GetIsMarried() &&
                          !GetCharacterData()->GetIsMarried() && this != activeActor )
                {
                    if ( ( GetCharacterData()->GetRaceInfo()->gender == PSCHARACTER_GENDER_NONE &&
                           activeActor->GetCharacterData()->GetRaceInfo()->gender == PSCHARACTER_GENDER_NONE ) ||
                         ( GetCharacterData()->GetRaceInfo()->gender !=
                           activeActor->GetCharacterData()->GetRaceInfo()->gender ) )
                    {
                        options |= psGUIInteractMessage::MARRIAGE;
                    }
                }

                // Introduce yourself
                if (IsAlive() && GetCharacterData()->Knows(activeActor->GetCharacterData()))
                {
                    options |= psGUIInteractMessage::INTRODUCE;
                }
            }

            if (!options)
                return;

            // Always possible to close menu
            options |= psGUIInteractMessage::CLOSE;

            unsigned int client = actor->GetClientID();
            psGUIInteractMessage guimsg(client,options);
            guimsg.SendMessage();
        }
    }
    else if (msg_id == "playerdesc")
        psserver->usermanager->SendCharacterDescription(actor->GetClient(),
                                                             GetCharacterData(), false, false, "behaviorMsg");
    else if (msg_id == "exchange")
        psserver->exchangemanager->StartExchange(actor->GetClient(), true);
    else if (msg_id == "attack")
        psserver->usermanager->Attack(actor->GetCharacterData()->getStance("Normal"), actor->GetClient(), actor->GetClientID());
}

void gemActor::SetAction(const char *anim,csTicks& timeDelay)
{
    if (!anim)
        return;

    csRef<iSpriteCal3DState> spstate = scfQueryInterface<iSpriteCal3DState> (GetMeshWrapper()->GetMeshObject());

    // Player must be standing for anim to happen
    spstate->SetAnimAction(anim,.25,.25);
    psOverrideActionMessage action(0, eid, anim);
    action.Multicast(GetMulticastClients(),-1,0);
}

void gemActor::ActionCommand(bool actionMy, bool actionNarrate, const char *actText,int destClientID, csTicks& timeDelay)
{
    int chtype = CHAT_NPC_ME;

    if (actionMy)
        chtype = CHAT_NPC_MY;
    else if (actionNarrate)
        chtype = CHAT_NPC_NARRATE;

    // first response gets 1 second delay to simulate NPC thinking
    // subsequent ones add to the current time delay, and send delayed
    if (timeDelay==0)
        timeDelay = (csTicks)(1000);
    psChatMessage msg(destClientID,GetName(),0,actText,chtype,false);
    psserver->GetEventManager()->SendMessageDelayed(msg.msg,timeDelay);

    timeDelay += (csTicks)(1000 + 30*strlen(actText));
}

float gemActor::FallEnded(const csVector3& pos, iSector* sector)
{
    isFalling = false;

    psWorld *psworld = EntityManager::GetSingleton().GetWorld();

    // Convert fallStartPos into coordinate system of fall end sector.
    if (!psworld->WarpSpace(fallStartSector,sector,fallStartPos))
    {
        // If we can't warp space, someone probably teleported between two
        // sectors without portals to one another.

        // Error3("Don't know how to calculate damage for fall between %s and %s", fallStartSector->QueryObject()->GetName(),sector->QueryObject()->GetName());
        return 0.0; // Don't give any fall damage if we don't know how to transform
    }

    return fallStartPos.y - pos.y;
}

void gemActor::FallBegan(const csVector3& pos, iSector* sector)
{
    isFalling = true;

    this->fallStartPos = pos;
    this->fallStartSector = sector;
    this->fallStartTime = csGetTicks();
}

int GetFreeScriptSlot(csArray<csString> & scripts)
{
    size_t scriptID;

    scriptID = 0;
    while (scriptID<scripts.GetSize()  &&  scripts[scriptID].Length()>0)
        scriptID++;

    if (scriptID<(scripts.GetSize()))
        return (int)scriptID;
    else
    {
        scripts.Push("");
        return (int)scripts.GetSize()-1;
    }
}

int gemActor::AttachAttackScript(const csString & scriptName)
{
    Debug3(LOG_COMBAT, pid.Unbox(), "---attach %s %s", GetName(), scriptName.GetData());

    int scriptID = GetFreeScriptSlot(onAttackScripts);
    onAttackScripts[ scriptID ] = scriptName;
    Debug2(LOG_COMBAT, pid.Unbox(), "---%i", scriptID);
    return scriptID;
}

void gemActor::DetachAttackScript(int scriptID)
{
    //Error3("---detach %s %i",GetName(),scriptID);

    if (scriptID<0  ||  scriptID>=(int)onAttackScripts.GetSize())
    {
        Error3("Invalid attack script scriptID %i for actor %s", scriptID, name.GetData());
        return;
    }
    onAttackScripts[scriptID].Clear();
}

int  gemActor::AttachDamageScript(const csString & scriptName)
{
    Debug3(LOG_COMBAT, pid.Unbox(), "---attach dam %s %s", GetName(), scriptName.GetData());

    int scriptID = GetFreeScriptSlot(onDamageScripts);
    onDamageScripts[ scriptID ] = scriptName;
    return scriptID;
}

void gemActor::DetachDamageScript(int scriptID)
{
    Debug3(LOG_COMBAT, pid.Unbox(), "---detach dam %s %i", GetName(), scriptID);

    if (scriptID<0  ||  scriptID>=(int)onDamageScripts.GetSize())
    {
        Error3("Invalid damage script scriptID %i for actor %s", scriptID, name.GetData());
        return;
    }
    onDamageScripts[scriptID].Clear();
}

bool gemActor::AddActiveMagicCategory(const csString & category)
{
    if (IsMagicCategoryActive(category))
        return false;
    active_spell_categories.Push(category);
    psGUIActiveMagicMessage outgoing(this->GetClientID(), psGUIActiveMagicMessage::addCategory, category, true);
    outgoing.SendMessage();
    return true;
}

bool gemActor::RemoveActiveMagicCategory(const csString & category)
{
    if (active_spell_categories.Delete(category))
    {
        psGUIActiveMagicMessage outgoing(this->GetClientID(), psGUIActiveMagicMessage::removeCategory, category, true);
        outgoing.SendMessage();
        return true;
    }
    return false;
}

bool gemActor::IsMagicCategoryActive(const csString & category)
{
    return active_spell_categories.Find(category) != csArrayItemNotFound;
}

bool gemActor::SetMesh(const char* meshname)
{
    csString newmesh;
    newmesh.Format("/planeshift/models/%s/%s.cal3d", meshname, meshname );

    if ( psserver->vfs->Exists(newmesh) )
    {
        // Get current position to give to the newly set mesh
        csVector3 pos;
        float angle;
        iSector* sector;
        GetPosition(pos,angle,sector);

        if ( pcmesh->SetMesh(meshname,newmesh) )
        {
            if (pcmove)
            {
                delete pcmove;
                pcmove = NULL;
            }
            InitLinMove(pos, angle, sector);

            SetPosition(pos,angle,sector);
            MulticastDRUpdate();

            if ( pcmesh->GetMesh() )
            {
                factname = meshname;
                filename = newmesh;

                UpdateProxList(true);
                return true;
            }
        }

        // Setting the mesh failed. Resetting back to the original mesh assuming
        // that the mesh factory for the original mesh is already loaded.
        if (!pcmesh->SetMesh(meshcache, 0))
        {
            // Last attempt with the full file name
            newmesh.Format("/planeshift/models/%s/%s.cal3d", meshcache.GetData(), meshcache.GetData());
            if (!pcmesh->SetMesh(meshcache, newmesh))
            {
                //Previously was CS_ASSERT(ResetMesh());, CS_ASSERT disappears in release mode
                CS_ASSERT(false);
            }
        }
        SetPosition(pos,angle,sector);
        MulticastDRUpdate();

    }

    return false;
}

//--------------------------------------------------------------------------------------

gemNPC::gemNPC( psCharacter *chardata,
                   const char* factname,
                   const char* filename,
                   InstanceID instance,
                   iSector* room,
                   const csVector3& pos,
                   float rotangle,
                   int clientnum)
                   : gemActor(chardata,factname,filename,instance,room,pos,rotangle,clientnum)
{
    npcdialog = NULL;
    superClientID = 0;
    nextVeryShortRangeAvail = 0; /// When can npc respond to very short range prox trigger again
    nextShortRangeAvail = 0;     /// When can npc respond to short range prox trigger again
    nextLongRangeAvail = 0;      /// When can npc respond to long range prox trigger again

    //if( !GetEntity() )
    //{
    //    Error3("Error in GemNPC %s constructor File: %s", factname, filename);
    //    return;
    //}

    pcmove->SetOnGround(true);

    if (chardata->GetOwnerID().IsValid())
    {
        this->SetOwner( cel->FindPlayerEntity( chardata->GetOwnerID() ) );
    }
    Debug3(LOG_NPC,0, "Created npc firstname:%s, lastname:%s\n",chardata->GetCharName(), chardata->GetCharLastName());
}

gemNPC::~gemNPC()
{
    Disconnect();
    delete npcdialog;
    npcdialog = NULL;
}

void gemNPC::SetPosition(const csVector3& pos,float angle, iSector* sector)
{
    gemActor::SetPosition(pos,angle,sector);
    UpdateProxList(true);
}

void gemNPC::SetupDialog(PID npcID, bool force)
{
    if (force || db->SelectSingleNumber("SELECT count(*) FROM npc_knowledge_areas WHERE player_id=%d", npcID.Unbox()) > 0)
    {
        npcdialog = new psNPCDialog(this);
        if (!npcdialog->Initialize(db,npcID))
        {
            Error2("Failed to initialize NPC dialog for %s\n", ShowID(npcID));
        }
    }
}

void gemNPC::ReactToPlayerApproach(psNPCCommandsMessage::PerceptionType type,gemActor *player)
{
    // printf("%s received Player approach message %d about player %s.\n",
    //        GetName(), type, player->GetName() );
    csTicks *which_avail = NULL;  // points to 1 of 3 times in gemNPC

    if (npcdialog)
    {
        csString trigger;
        csTicks now = csGetTicks();
        switch(type)
        {
            case psNPCCommandsMessage::PCPT_LONGRANGEPLAYER:

                if (now > nextLongRangeAvail)
                {
                    trigger = "!longrange";
                    which_avail = &nextLongRangeAvail;
                    nextLongRangeAvail = now + (psserver->GetRandom(30) + 10) * 1000;
                }
                break;

            case psNPCCommandsMessage::PCPT_SHORTRANGEPLAYER:

                if (now > nextShortRangeAvail)
                {
                    trigger = "!shortrange";
                    which_avail = &nextShortRangeAvail;
                    nextShortRangeAvail = now + (psserver->GetRandom(30) + 10) * 1000;
                }
                break;

            case psNPCCommandsMessage::PCPT_VERYSHORTRANGEPLAYER:

                if (now > nextVeryShortRangeAvail)
                {
                    trigger = "!veryshortrange";
                    which_avail = &nextVeryShortRangeAvail;
                    nextVeryShortRangeAvail = now + (psserver->GetRandom(30) + 10) * 1000;
                }
                break;
            default:
                break;
        }

        if (which_avail)
        {
            NpcResponse *resp = npcdialog->FindXMLResponse(player->GetClient(),trigger);
            if (resp)
            {
                resp->ExecuteScript(player->GetClient(), this);
            }
            else // not found, so don't try again to find it later
            {
                *which_avail += 3600 * 1000;  // only recheck once an hour if not found the first time
            }
        }
    }
}

void gemNPC::ShowPopupMenu(Client *client)
{
    NpcResponse *resp = NULL;
    NpcDialogMenu menu;

    csArray<QuestAssignment*>& quests = client->GetCharacterData()->GetAssignedQuests();

    // Merge current spot in active quests first
    for (size_t i=0; i < quests.GetSize(); i++)
    {
        psQuest *q = quests[i]->GetQuest();
        // If the quest is completed or the last response was not from this NPC, then skip
        if (quests[i]->last_response_from_npc_pid != pid || quests[i]->status == 'C')
        {
            printf("Skipping completed or irrelevant quest: %s\n", q->GetName() );
            continue;
        }
        printf("Checking quest %u: %s.  ", i, q->GetName() );
        int last_response = quests[i]->last_response;
        printf("Got last response %d\n", last_response);

        if (last_response != -1) // within a quest step
        {
            resp = dict->FindResponse(last_response);
            menu.Add(resp->menu);
        }
        else
        {
            printf("Got last_response==-1 for quest %d.\n",i);
        }
    }

    // Also offer default choices in case a new quest should be started
    NpcDialogMenu *npcmenu = dict->FindMenu( name );
    if (npcmenu)
        menu.Add(npcmenu);

    if (menu.triggers.GetSize())
        menu.ShowMenu(client);
    else
        psserver->SendSystemError(client->GetClientNum(), "This NPC has nothing to say to you.");
}

csString gemNPC::GetDefaultBehavior(const csString & dfltBehaviors)
{
    int behNum;
    if (GetCharacterData()->IsMerchant())
        behNum = 2;
    else if (IsAlive())
        behNum = 3;
    else
        behNum = 4;

    return ::GetDefaultBehavior(dfltBehaviors, behNum);
}

void gemNPC::SendBehaviorMessage(const csString & msg_id, gemObject *actor)
{
    unsigned int client = actor->GetClientID();

    if ( msg_id == "select" )
    {
        // If the player is in range of the item.
        if ( RangeTo(actor) < RANGE_TO_SELECT )
        {
            int options = 0;

            psGUIInteractMessage guimsg(client,options);
            guimsg.SendMessage();
        }

        return;
    }
    else if ( msg_id == "context" )
    {
        // If the player is in range of the item.
        if ( RangeTo(actor) < RANGE_TO_SELECT && actor->IsAlive() )
        {
            Client* clientC = psserver->GetNetManager()->GetClient(client);

            int options = 0;

            // Pet?
            if (psChar->IsPet())
            {
                // Mine?
                if (psChar->GetOwnerID() == actor->GetCharacterData()->GetPID())
                {
                    options |= psGUIInteractMessage::VIEWSTATS;
                    options |= psGUIInteractMessage::DISMISS;

                    // If we are in a peaceful mode we can possibly do some trading.
                    if (actor->GetMode() == PSCHARACTER_MODE_PEACE)
                        options |= psGUIInteractMessage::GIVE;

                    //If we are alive then we can talk with an NPC
                    if (IsAlive())
                        options |= psGUIInteractMessage::NPCTALK;
                }
                else
                    options |= psGUIInteractMessage::PLAYERDESC;
            }
            else // Normal NPCs
            {
                options |= psGUIInteractMessage::PLAYERDESC;

                // Loot a dead character
                if (!IsAlive())
                    options |= psGUIInteractMessage::LOOT;

                // If we are in a peaceful mode we can possibly do some trading.
                if (actor->GetMode() == PSCHARACTER_MODE_PEACE)
                {
                    options |= psGUIInteractMessage::GIVE;

                    // Trade with a merchant
                    if (psChar->IsMerchant() && IsAlive())
                        options |= psGUIInteractMessage::BUYSELL;

                    // Bank with a banker
                    if(psChar->IsBanker() && IsAlive())
                        options |= psGUIInteractMessage::BANK;

                    // Train with a trainer
                    if (psChar->IsTrainer() && IsAlive())
                        options |= psGUIInteractMessage::TRAIN;

                }

                // If we are alive then we can talk with an NPC
                if (IsAlive())
                    options |= psGUIInteractMessage::NPCTALK;

                // Can we attack this NPC?
                if (IsAlive() && clientC && clientC->IsAllowedToAttack(this,false))
                    options |= psGUIInteractMessage::ATTACK;
            }

            // Check if there is something to send
            if (!options)
                return;

            // Always possible to close menu
            options |= psGUIInteractMessage::CLOSE;

            psGUIInteractMessage guimsg(client,options);
            guimsg.SendMessage();
        }
    }
    else if (msg_id == "buysell")
        psserver->GetCharManager()->BeginTrading(actor->GetClient(), this, "sell");
    else if (msg_id == "give")
        psserver->exchangemanager->StartExchange(actor->GetClient(), false);
    else if (msg_id == "playerdesc")
        psserver->usermanager->SendCharacterDescription(actor->GetClient(),
                                                        GetCharacterData(), false, false, "behaviorMsg");
    else if (msg_id == "attack")
        psserver->usermanager->Attack(actor->GetCharacterData()->getStance("Normal"), actor->GetClient(), actor->GetClientID());
    else if (msg_id == "loot")
        psserver->usermanager->HandleLoot(actor->GetClient());
}

void gemNPC::AddLootableClient(int cnum)
{
    lootable_clients.Push(cnum);
}

void gemNPC::RemoveLootableClient(int cnum)
{
    for (size_t i=0; i<lootable_clients.GetSize(); i++)
    {
        if (lootable_clients[i] == cnum)
            lootable_clients[i] = -1; // Should not be possible to find a client with this id.
                                      // Fast and client is removed from table;
    }
}


bool gemNPC::IsLootableClient(int cnum)
{
    for (size_t i=0; i<lootable_clients.GetSize(); i++)
    {
        if (lootable_clients[i] == cnum)
            return true;
    }
    return false;
}

Client *gemNPC::GetRandomLootClient(int range)
{
    if (lootable_clients.GetSize() == 0)
        return NULL;

    csArray<int> temp;
    int which = psserver->rng->Get((int)lootable_clients.GetSize());
    int first = which;

    do
    {
        if (lootable_clients[which] != -1)
            temp.Push(lootable_clients[which]);

        ++which;
        if (which == (int)lootable_clients.GetSize())
            which = 0;
    } while (which != first);

    Client *found;

    for (size_t i=0; i<temp.GetSize(); i++)
    {
        found = psserver->GetNetManager()->GetClient(temp[i]);

        if (found && found->GetActor() &&
            found->GetActor()->RangeTo(this) < range)
        {
            return found;
        }
    }

    return NULL;
}

void gemNPC::Say(const char *strsay,Client *who, bool saypublic,csTicks& timeDelay)
{
    if (strsay)
    {
        if (who && !saypublic) // specific client specified means use /tell not /say
        {
            Notify2(LOG_CHAT,"Private NPC Response: %s\n",strsay);

            // Some NPC responses are now in the form of private tells.
            psChatMessage newMsg(who->GetClientNum(), GetName(), 0, strsay, CHAT_NPC, false);

            // first response gets 1 second delay to simulate NPC thinking
            // subsequent ones add to the current time delay, and send delayed
            if (timeDelay==0)
                timeDelay = (csTicks)(1000);
            psserver->GetEventManager()->SendMessageDelayed(newMsg.msg,timeDelay);
            timeDelay += (csTicks)(2000 + 50*strlen(strsay));
        }
        else
        {
            Notify2(LOG_CHAT,"Public NPC Response: %s\n",strsay);
            // Some NPC responses are now in the form of public /says.
            psChatMessage newMsg(0, GetName(), 0, strsay, CHAT_NPC, false);
            newMsg.Multicast(GetMulticastClients(), 0, CHAT_SAY_RANGE );
        }

        if (who)
        {
            // This perception allows the superclient to know about the dialog with a specific person.
            psserver->GetNPCManager()->QueueTalkPerception(who->GetActor(), this);
        }
    }
}

void gemNPC::AddBadText(const char *playerSaid, const char *trigger)
{
    for (size_t i=0; i< badText.GetSize(); i++)
    {
        if (badText[i]->said == playerSaid)
        {
            badText[i]->count++;
            badText[i]->when = csGetTicks();
            return;
        }
    }
    DialogCounter *dlg = new DialogCounter;
    dlg->count   = 1;
    dlg->said    = playerSaid;
    dlg->trigger = trigger;
    dlg->when    = csGetTicks();
    badText.Push(dlg);
    badText.Sort( &gemNPC::DialogCounter::Compare );
    if (badText.GetSize() > 64)
        delete badText.Pop();  // never let >64 items in
}

void gemNPC::GetBadText(size_t first,size_t last, csStringArray& saidArray, csStringArray& trigArray)
{
    saidArray.Empty();
    trigArray.Empty();

    for (size_t i=first-1; i<last; i++)
    {
        if (i>=badText.GetSize())
            continue;

        saidArray.Push(badText[i]->said);
        csString temp = badText[i]->trigger;
        temp.AppendFmt("(%d)", badText[i]->count);
        trigArray.Push(temp);
    }
}



void gemNPC::Send( int clientnum, bool control, bool to_superclient )
{
    csString texparts;
    csString equipmentParts;
    EID ownerEID;

    psChar->MakeTextureString( texparts );
    psChar->MakeEquipmentString( equipmentParts );
    GetPosition(pos,yRot,sector);

    csVector3 pos;
    float yRot;
    iSector * sector;
    pcmove->GetLastPosition(pos, yRot, sector);

    csString guildName;
    guildName.Clear();
    if ( psChar->GetGuild() && !psChar->GetGuild()->IsSecret() )
        guildName = psChar->GetGuild()->name;

    if ( this->owner.IsValid())
    {
        ownerEID = owner->GetEID();
    }

    uint32_t flags = psPersistActor::NPC;

    if (!GetVisibility())    flags |= psPersistActor::INVISIBLE;
    if (GetInvincibility())  flags |= psPersistActor::INVINCIBLE;

    Client* targetClient = psserver->GetConnections()->Find(clientnum);
    if (targetClient && targetClient->GetCharacterData())
    {
        flags |= psPersistActor::NAMEKNOWN;

        // Enable to enable introductions between player and NPCs
        /*if ((targetClient->GetSecurityLevel() >= GM_LEVEL_0) ||
              targetClient->GetCharacterData()->Knows(psChar))
                                                             flags |= psPersistActor::NAMEKNOWN;*/
    }

    csString helmGroup = psChar->GetHelmGroup();

    psPersistActor mesg(
                         clientnum,
                         securityLevel,
                         masqueradeLevel,
                         control,
                         name,
                         guildName,
                         factname,
                         filename,
                         psChar->GetRaceInfo()->name,
                         psChar->GetRaceInfo()->gender,
                         helmGroup,
                         top, bottom,offset,
                         texparts,
                         equipmentParts,
                         DRcounter,
                         eid,
                         CacheManager::GetSingleton().GetMsgStrings(),
                         pcmove,
                         movementMode,
                         GetMode(),
                         0, // playerID should not be distributed to clients
                         0, // groupID
                         ownerEID,
                         flags
                         );

    if (clientnum && !to_superclient)
    {
        mesg.SendMessage();
    }

    if (to_superclient)
    {
        mesg.SetPlayerID(pid); // Insert player id before sending to super client.
        mesg.SetInstance(GetInstance());
        if (clientnum == 0) // Send to all superclients
        {
            CPrintf(CON_DEBUG, "Sending gemNPC to superclients.\n");
            mesg.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
        }
        else
        {
            mesg.SendMessage();
        }
    }
}

void gemNPC::Broadcast(int clientnum, bool control)
{
    csArray<PublishDestination>& dest = GetMulticastClients();
    for (unsigned long i = 0 ; i < dest.GetSize(); i++)
    {
        if (dest[i].dist < PROX_LIST_ANY_RANGE)
            Send(dest[i].client, control, false);
    }

    csArray<PublishDestination>& destSuper = psserver->GetNPCManager()->GetSuperClients();
    for (unsigned long i = 0 ; i < destSuper.GetSize(); i++)
    {
        if (destSuper[i].dist < PROX_LIST_ANY_RANGE)
            Send(destSuper[i].client, control, true);
    }

}

#if 0   // This function is redundant with npc->Say()
void gemNPC::NPCTalk(const csString & text)
{
    psSystemMessage talkMsg(0, MSG_INFO, text);
    psserver->GetEventManager()->Broadcast(talkMsg.msg);
    talkMsg.Multicast(GetMulticastClients(), 0, CHAT_SAY_RANGE );
}
#endif

