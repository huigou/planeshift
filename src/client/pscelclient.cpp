/*
 * psCelClient.cpp by Matze Braun <MatzeBraun@gmx.de>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
 *  Implements the varrious things relating to the CEL for the client.
 */
#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/scf.h>
#include <csutil/csstring.h>
#include <cstool/collider.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <ivaria/collider.h>
#include <iengine/mesh.h>
#include <iengine/region.h>
#include <iengine/movable.h>
#include <imesh/object.h>
#include <imesh/spritecal3d.h>
#include <imesh/nullmesh.h>
#include <imesh/nullmesh.h>
#include <csgeom/math3d.h>
#include <physicallayer/pl.h>
#include <behaviourlayer/bl.h>
#include <behaviourlayer/behave.h>
#include <physicallayer/entity.h>
#include <physicallayer/propfact.h>
#include <physicallayer/propclas.h>
#include <physicallayer/persist.h>
#include <propclass/mesh.h>
#include <propclass/meshsel.h>
#include <propclass/inv.h>
#include <propclass/chars.h>
#include <propclass/linmove.h>
#include <propclass/tooltip.h>
#include <propclass/camera.h>
#include <propclass/gravity.h>
#include <propclass/timer.h>
#include <propclass/input.h>
#include <propclass/colldet.h>
#include <propclass/solid.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "engine/celbase.h"
#include "engine/netpersist.h"
#include "engine/psworld.h"

#include "net/messages.h"
#include "net/msghandler.h"

#include "util/psxmlparser.h"
#include "util/psconst.h"
#include "util/psmeshutil.h"

#include "gui/pawsinfowindow.h"
#include "gui/pawsconfigkeys.h"
#include "gui/inventorywindow.h"
#include "gui/chatwindow.h"
#include "gui/pawslootwindow.h"

#include "paws/pawsmanager.h"


//=============================================================================
// Application Includes
//=============================================================================
#include "globals.h"
#include "charapp.h"
#include "clientvitals.h"
#include "modehandler.h"
#include "zonehandler.h"
#include "pscelclient.h"
#include "psclientdr.h"
#include "entitylabels.h"
#include "shadowmanager.h"
#include "clientcachemanager.h"
#include "pscharcontrol.h"
#include "psclientchar.h"

#include "pscal3dcallback.h"



psCelClient *GEMClientObject::cel = NULL;


psCelClient::psCelClient() : ignore_others(false)
{
    requeststatus = 0;
    
    clientdr        = NULL;
    modehandler     = NULL;
    zonehandler     = NULL;
    entityLabels    = NULL;
    shadowManager   = NULL;
    gameWorld       = NULL;
    local_player    = NULL;
    unresSector     = NULL;
}

psCelClient::~psCelClient()
{   
    delete gameWorld;
    Clear();

    if (msghandler)
    {
        msghandler->Unsubscribe( this, MSGTYPE_CELPERSIST);
        msghandler->Unsubscribe( this, MSGTYPE_PERSIST_WORLD );
        msghandler->Unsubscribe( this, MSGTYPE_PERSIST_ACTOR );
        msghandler->Unsubscribe( this, MSGTYPE_PERSIST_ITEM ); 
        msghandler->Unsubscribe( this, MSGTYPE_PERSIST_ACTIONLOCATION ); 
        msghandler->Unsubscribe( this, MSGTYPE_REMOVE_OBJECT ); 
        msghandler->Unsubscribe( this, MSGTYPE_NAMECHANGE ); 
        msghandler->Unsubscribe( this, MSGTYPE_GUILDCHANGE ); 
        msghandler->Unsubscribe( this, MSGTYPE_GROUPCHANGE ); 
        msghandler->Unsubscribe( this, MSGTYPE_STATS );
    }

    if (clientdr)
        delete clientdr;
    if (entityLabels)
        delete entityLabels;
    if (shadowManager)
        delete shadowManager;
        

    entities.DeleteAll();
    entities_hash.DeleteAll();    
}

iCelEntity* psCelClient::GetMainActor()
{
    return local_player ? local_player->GetEntity() : NULL; 
}
    

bool psCelClient::Initialize(iObjectRegistry* object_reg,
        MsgHandler* newmsghandler,ModeHandler *modehndlr,
        ZoneHandler *zonehndlr)
{
    if (!CelBase::Initialize(object_reg))
    {
        return false;
    }        

    entityLabels = new psEntityLabels();
    entityLabels->Initialize(object_reg, this);

    shadowManager = new psShadowManager();
    
    vfs =  csQueryRegistry<iVFS> (object_reg);
    if (!vfs)
    {
        return false;
    }        

    modehandler = modehndlr;
    zonehandler = zonehndlr;

    msghandler = newmsghandler;
    msghandler->Subscribe( this, MSGTYPE_CELPERSIST);
    msghandler->Subscribe( this, MSGTYPE_PERSIST_WORLD );
    msghandler->Subscribe( this, MSGTYPE_PERSIST_ACTOR );
    msghandler->Subscribe( this, MSGTYPE_PERSIST_ITEM ); 
    msghandler->Subscribe( this, MSGTYPE_PERSIST_ACTIONLOCATION ); 
    msghandler->Subscribe( this, MSGTYPE_REMOVE_OBJECT ); 
    msghandler->Subscribe( this, MSGTYPE_NAMECHANGE ); 
    msghandler->Subscribe( this, MSGTYPE_GUILDCHANGE ); 
    msghandler->Subscribe( this, MSGTYPE_GROUPCHANGE ); 
    msghandler->Subscribe( this, MSGTYPE_STATS );
                           
    clientdr = new psClientDR;
    if (!clientdr->Initialize(object_reg, this, msghandler ))
    {
        delete clientdr;
        clientdr = NULL;
        return false;
    }

    unresSector = psengine->GetEngine()->CreateSector("SectorWhereWeKeepEntitiesResidingInUnloadedMaps");
    
    return true;
}

GEMClientObject* psCelClient::FindObject( int id )
{
    return entities_hash.Get (id, NULL);
}

void psCelClient::SetMainActor(iCelEntity* entity)
{
    if (entity)
    {
        mainPlayerEntity = entity;

        // ModeHandler has no good way to find out the entity
        modehandler->SetEntity(entity);
    }        
}

void psCelClient::RequestServerWorld()
{
    psPersistWorldRequest world;
    msghandler->SendMessage( world.msg );
    
    requeststatus=1;
}

bool psCelClient::IsReady()
{
    if ( local_player == NULL || gameWorld == NULL )
        return false;
    else
        return true; 
}

void psCelClient::HandleWorld( MsgEntry* me )
{    
    psPersistWorld mesg( me );
    
    
    gameWorld = new psWorld;
    gameWorld->Initialize( object_reg );
    
    zonehandler->SetWorld(gameWorld);

    // Tell the user that we are loading the world
    psengine->AddLoadingWindowMsg( "Loading world" );

    gameWorld->Initialize( object_reg );

    zonehandler->LoadZone(mesg.sector);

    requeststatus = 0;        
    RequestActor();  
}

void psCelClient::RequestActor()
{
    psPersistActorRequest mesg;
    msghandler->SendMessage( mesg.msg );
}

void psCelClient::HandleActor( MsgEntry* me )
{
    // Check for loading errors first
    if (psengine->LoadingError() || !GetClientDR()->GetMsgStrings() )
    {
        Error1("Ignoring Actor message.  LoadingError or missing strings table!\n");
        psengine->FatalError("Cannot load main actor. Error during loading.\n");
        return;
    }
    psPersistActor mesg( me, GetClientDR()->GetMsgStrings(), psengine->GetEngine() );

    // We already have an entity with this id so we must have missed the remove object message
    // so delete and remake it.
    GEMClientObject* found = (GEMClientObject *) FindObject(mesg.entityid);
    if (found)
    {
        if ( found == local_player )
        {
            HandleMainActor( mesg );
            return;
        }
            
        Debug3(LOG_CELPERSIST, 0, "Found existing object<%s> with id %u, removing.\n", found->GetName(), mesg.entityid);
        RemoveObject(found);
    }

    if (ignore_others)
    {
        if (local_player != 0)
            return;

        // From here on, we have no main character
        // Ignore everything that isn't controllable
        if (!mesg.control)
            return;
    }

    GEMClientActor* actor = new GEMClientActor( this, mesg );

    // The first actor that is sent to the client is his own one.
    if ( local_player == NULL && actor->control )
    {        
        local_player = actor;
        SetMainActor( local_player->GetEntity() );

        // This triggers the server to update our proxlist
        local_player->SendDRUpdate(PRIORITY_LOW,GetClientDR()->GetMsgStrings());
    }

    if (actor->linmove)
        actor->linmove->SetDeltaLimit(0.2f);

//    csVector3 pos = actor->pcmesh->GetMesh()->GetMovable()->GetPosition();
//    iSector* sector = actor->pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0);
//    printf("Recevied actor(%d) '%s' Pos (%.2f,%.2f,%.2f) sector '%s'\n", actor->GetID(),actor->GetName(), pos.x,pos.y,pos.z, sector->QueryObject()->GetName() );

    entities.Push(actor);
    entities_hash.Put(actor->GetID(), actor);
}

void psCelClient::HandleMainActor( psPersistActor& mesg )
{
    
    // Saved for when we reset
    if ( local_player_defaultFactName.IsEmpty() )
    {
        local_player_defaultFactName = local_player->GetFactName();
        local_player_defaultFact = local_player->pcmesh->GetMesh()->GetFactory();
        local_player_defaultMesh = local_player->pcmesh->GetMesh()->GetMeshObject();
    }

    // Update equipment list
    local_player->equipment = mesg.equipment;

    if (mesg.factname != local_player->GetFactName())
    {
        local_player->charApp->ClearEquipment();
    
        csRef<iMeshFactoryWrapper> factory = psengine->GetEngine()->GetMeshFactories()->FindByName(mesg.factname);
        if (!factory)
        {
            // Try loading the mesh from file
            csString filename;
            if (!psengine->GetFileNameByFact(mesg.factname, filename))
            {
                Error2( "Mesh Factory %s not found", mesg.factname.GetData() );            
                return;
            }
            psengine->GetCacheManager()->LoadNewFactory(filename);
            factory = psengine->GetEngine()->GetMeshFactories()->FindByName(mesg.factname);
            if (!factory)
            {
                Error2("Couldn't morph main player! Factory %s doesn't exist!", mesg.factname.GetData() );
                return;
            }
        }
        
        // New or resetting?
        if (local_player_defaultFactName != mesg.factname)
        {
            csRef<iMeshWrapper> meshwrap = psengine->GetEngine()->CreateMeshWrapper(factory, mesg.factname);
            csRef<iMeshObject> mesh = meshwrap->GetMeshObject();

            // Update
            local_player->pcmesh->GetMesh()->SetMeshObject(mesh);
            local_player->pcmesh->GetMesh()->SetFactory(factory);
            local_player->charApp->SetMesh(local_player->pcmesh->GetMesh());
            
            // Cal3d
            csRef<iSpriteCal3DState> calstate = scfQueryInterface<iSpriteCal3DState> (local_player->pcmesh->GetMesh()->GetMeshObject());
            if (calstate)
            {
                calstate->SetUserData((void*)local_player);
                calstate->SetVelocity(0.0,&psengine->GetRandomGen());
            }
        }
        else
        {
            // Reset
            local_player->pcmesh->GetMesh()->SetMeshObject(local_player_defaultMesh);
            local_player->pcmesh->GetMesh()->SetFactory(local_player_defaultFact);
            local_player->charApp->SetMesh(local_player->pcmesh->GetMesh());            
        }

        // Update factory
        local_player->factname = mesg.factname;
       
        
        // Update cal3d
        local_player->RefreshCal3d();
       
        local_player->charApp->ApplyEquipment(local_player->equipment);
    }
}

void psCelClient::HandleItem( MsgEntry* me )
{
    psPersistItem mesg( me );

    // We already have an entity with this id so we must have missed the remove object message
    // so delete and remake it.
    GEMClientObject* found = (GEMClientObject *) FindObject(mesg.id);
    if(found)
    {
        Debug3(LOG_CELPERSIST, 0, "Found existing item<%s> object with id %u, removing.\n", found->GetName(), mesg.id);
        RemoveObject(found);
    }

    GEMClientItem * newItem = new GEMClientItem( this, mesg );

    entities.Push(newItem);    
    entities_hash.Put(newItem->GetID(), newItem);
}

void psCelClient::HandleActionLocation( MsgEntry* me )
{
    psPersistActionLocation mesg( me );

    // We already have an entity with this id so we must have missed the remove object message
    // so delete and remake it.
    GEMClientObject* found = (GEMClientObject *) FindObject(mesg.id);
    if ( found )
    {
        Debug3(LOG_CELPERSIST, 0, "Found existing location<%s> object with id %u, removing.\n", found->GetName(), mesg.id);
        RemoveObject( found );        
    }

    GEMClientActionLocation * newAction = new GEMClientActionLocation( this, mesg );
    entities.Push( newAction );   
    actions.Push( newAction );
    entities_hash.Put ( newAction->GetID (), newAction );
}

void psCelClient::HandleObjectRemoval( MsgEntry* me )
{
    psRemoveObject mesg(me);
    
    GEMClientObject* entity = FindObject( mesg.objectEID );
    
    Debug2( LOG_CELPERSIST, 0, "Object %d Removed",  mesg.objectEID );

    if (entity != NULL)
    {
        RemoveObject(entity);
    }        
}

csList<UnresolvedPos*>::Iterator psCelClient::FindUnresolvedPos(GEMClientObject * entity)
{
    csList<UnresolvedPos*>::Iterator posIter(unresPos);
    while (posIter.HasNext())
    {
        UnresolvedPos * pos = posIter.Next();
        if (pos->entity == entity)
            return posIter;
    }
    return csList<UnresolvedPos*>::Iterator();
}

void psCelClient::RemoveObject(GEMClientObject* entity)
{
    if (entity == local_player)
    {
        Error1("Nearly deleted player's object");
        return;
    }
    

//    csVector3 pos = entity->pcmesh->GetMesh()->GetMovable()->GetPosition();
//    iSector* sector = entity->pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0);
//    printf("Removing entity(%d) '%s' Pos (%.2f,%.2f,%.2f) sector '%s'\n", entity->GetID(),entity->GetName(), pos.x,pos.y,pos.z, sector->QueryObject()->GetName() );

    if ( psengine->GetCharManager()->GetTarget() == entity )
    {
        pawsWidget* widget = PawsManager::GetSingleton().FindWidget("InteractWindow");
        if(widget)
        {
            widget->Hide();
            psengine->GetCharManager()->SetTarget(NULL, "select");
        }
    }
  
    entityLabels->RemoveObject(entity);
    shadowManager->RemoveShadow(entity);
    pawsLootWindow* loot = (pawsLootWindow*)PawsManager::GetSingleton().FindWidget("LootWindow");
    if(loot && loot->GetLootingActor() == entity->GetID())
        loot->Hide();

    // delete record in unresolved position list
    csList<UnresolvedPos*>::Iterator posIter = FindUnresolvedPos(entity);
    if (posIter.HasCurrent())
    {
        //Error2("Removing deleted entity %s from unresolved",entity->GetName());
        delete *posIter;
        unresPos.Delete(posIter);
    }

    // Delete from action list if action
    if(dynamic_cast<GEMClientActionLocation*>(entity))
        actions.Delete( static_cast<GEMClientActionLocation*>(entity) );

    entities_hash.Delete (entity->GetID (), entity);
    entities.Delete(entity);
}

bool psCelClient::IsMeshSubjectToAction(const char* sector,const char* mesh)
{
    if(!mesh)
        return false;

    for(size_t i = 0; i < actions.GetSize();i++)
    {
        GEMClientActionLocation* action = actions[i];
        const char* sec = action->pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0)->QueryObject()->GetName();

        if(!strcmp(action->GetMesh(),mesh) && !strcmp(sector,sec))
            return true;
    }

    return false;
}

GEMClientActor * psCelClient::GetActorByName(const char * name, bool trueName) const
{
    size_t len = entities.GetSize();
    csString testName, firstName;
    for (size_t a=0; a<len; ++a)
    {
        GEMClientActor * actor = dynamic_cast<GEMClientActor*>(entities[a]);
        if (!actor)
            continue;

        testName = actor->GetName(trueName);
        firstName = testName.Slice(0, testName.FindFirst(' '));
        if (firstName == name)
            return actor;
    }
    return 0;
}

void psCelClient::HandleNameChange( MsgEntry* me )
{
    psUpdateObjectNameMessage msg (me);
    GEMClientObject* object = FindObject(msg.objectID);

    if(!object)
    {
        printf("Warning: Got rename message, but couldn't find actor %d!\n",msg.objectID);
        return;
    }

    // Slice the names into parts
    csString oldFirstName = object->GetName();
    oldFirstName = oldFirstName.Slice(0,oldFirstName.FindFirst(' '));

    csString newFirstName = msg.newObjName;
    newFirstName = newFirstName.Slice(0,newFirstName.FindFirst(' '));

    // Remove old name from chat auto complete and add new
    pawsChatWindow* chat = (pawsChatWindow*)(PawsManager::GetSingleton().FindWidget( "ChatWindow" ));
    chat->RemoveAutoCompleteName(oldFirstName);
    chat->AddAutoCompleteName(newFirstName);

    // Apply the new name
    object->ChangeName(msg.newObjName);

    // We don't have a label over our own char
    if (psengine->GetCelClient()->GetMainActor() != object->GetEntity())
        entityLabels->OnObjectArrived(object);

    // If object is targeted update the target information.
    if (psengine->GetCharManager()->GetTarget() == object)
    {
        if (object->GetType() == GEM_ACTOR)
            PawsManager::GetSingleton().Publish("sTargetName",((GEMClientActor*)object)->GetName(false) ); 
        else
            PawsManager::GetSingleton().Publish("sTargetName",object->GetName() ); 
    } 
}

void psCelClient::HandleGuildChange( MsgEntry* me )
{
    psUpdatePlayerGuildMessage msg (me);

    // Change every entity
    for(size_t i = 0; i < msg.objectID.GetSize();i++)
    {
        int id = (int)msg.objectID[i];
        GEMClientActor* actor = dynamic_cast<GEMClientActor*>(FindObject(id));

        //Apply the new name
        if(!actor)
        {
            Error2("Couldn't find actor %d!",id);
            continue;
        }
    
        actor->SetGuildName(msg.newGuildName);

        // we don't have a label over our own char
        if (psengine->GetCelClient()->GetMainActor() != actor->GetEntity())
            entityLabels->OnObjectArrived(actor);
    }
}

void psCelClient::HandleGroupChange(MsgEntry* me)
{
    psUpdatePlayerGroupMessage msg(me);
    GEMClientActor* actor = (GEMClientActor*)FindObject(msg.objectID);
    
    //Apply the new name
    if(!actor)
    {
        Error2("Couldn't find object %d, ignoring group change..",msg.objectID);
        return;
    }
    printf("Got group update for actor %d (%s) to group %d\n",actor->GetID(),actor->GetName(),msg.groupID);
    unsigned int oldGroup = actor->GetGroupID();
    actor->SetGroupID(msg.groupID);

    // repaint label
    if (GetMainActor() != actor->GetEntity())
        entityLabels->RepaintObjectLabel(actor);
    else // If it's us, we need to repaint every label with the group = ours
    {
        for(size_t i =0; i < entities.GetSize();i++)
        {
            GEMClientObject* object = entities[i];
            GEMClientActor* act = dynamic_cast<GEMClientActor*>(object);
            if(!act)
                continue;

            if(act != actor && (actor->IsGroupedWith(act) || (oldGroup != 0 && oldGroup == act->GetGroupID()) ) )
                entityLabels->RepaintObjectLabel(act);
        }
    }

}


void psCelClient::HandleStats( MsgEntry* me )
{
    psStatsMessage msg(me);
    
    PawsManager::GetSingleton().Publish( "fmaxhp", msg.hp );    
    PawsManager::GetSingleton().Publish( "fmaxmana", msg.mana );        
    PawsManager::GetSingleton().Publish( "fmaxweight", msg.weight );    
    PawsManager::GetSingleton().Publish( "fmaxcapacity", msg.capacity );    
    
}

void psCelClient::QueueNewActor(MsgEntry *me)
{
    me->IncRef();
    newActorQueue.Push(me);
}

void psCelClient::QueueNewItem(MsgEntry *me)
{
    me->IncRef();
    newItemQueue.Push(me);
}

void psCelClient::CheckEntityQueues()
{
    if (newActorQueue.GetSize())
    {
        MsgEntry *me = newActorQueue.Pop();
        HandleActor(me);
        me->DecRef();
        return;
    }
    if (newItemQueue.GetSize())
    {
        MsgEntry *me = newItemQueue.Pop();
        HandleItem(me);
        me->DecRef();
        return;
    }
}

void psCelClient::Update()
{
    shadowManager->UpdateShadows();
}

void psCelClient::HandleMessage(MsgEntry *me)
{
  switch ( me->GetType() )
  {     
        case MSGTYPE_STATS:
        {
            HandleStats(me);
            break;
        }
        
        case MSGTYPE_PERSIST_WORLD:
        {
            HandleWorld( me );
            break;
        }    
        
        case MSGTYPE_PERSIST_ACTOR:
        {
            QueueNewActor( me );
            break;
        }   
        
        case MSGTYPE_PERSIST_ITEM:
        {
            if (!ignore_others) 
                QueueNewItem( me );
            break;

        }
        case MSGTYPE_PERSIST_ACTIONLOCATION:
        {
            HandleActionLocation( me );
            break;
        }
        
        case MSGTYPE_REMOVE_OBJECT:
        {
            HandleObjectRemoval( me );
            break;
        }

        case MSGTYPE_NAMECHANGE:
        {
            HandleNameChange( me );
            break;
        }

        case MSGTYPE_GUILDCHANGE:
        {
            HandleGuildChange( me );
            break;
        }

        case MSGTYPE_GROUPCHANGE:
        {
            HandleGroupChange( me );
            break;
        }

        default:
        {
            Error1("CEL UNKNOWN MESSAGE!!!\n");
            break;
        }                       
    }
}


void psCelClient::SetPlayerReady(bool flag)
{
    if (local_player)
        local_player->ready = flag;
}

void psCelClient::OnRegionsDeleted(csArray<iRegion*>& regions)
{
    size_t entNum;

    for (entNum = 0; entNum < entities.GetSize(); entNum++)
    {
        iPcMesh * mesh = entities[entNum]->pcmesh;
        if (mesh != NULL  &&  mesh->GetMesh() )
        {
            iMovable* movable = mesh->GetMesh()->GetMovable();
            iSectorList* sectors = movable->GetSectors();
            // Shortcut the lengthy region check if possible
            if(IsUnresSector(movable->GetSectors()->Get(0)))
                continue;

            bool unresolved = true;

            // Are all the sectors going to be unloaded?
            for(int i = 0;i<sectors->GetCount();i++)
            {
                // Get the iRegion this sector belongs to
                csRef<iRegion> region =  scfQueryInterface<iRegion> (sectors->Get(i)->QueryObject()->GetObjectParent());
                if(regions.Find(region)==csArrayItemNotFound)
                {
                    // We've found a sector that won't be unloaded so the mesh won't need to be moved
                    unresolved = false;
                    break;
                }
            }

            if(unresolved)
            {
                // All the sectors the mesh is in are going to be unloaded
                Warning1(LOG_ANY,"Moving entity to temporary sector");
                // put the mesh to the sector that server uses for keeping meshes located in unload maps
                HandleUnresolvedPos(entities[entNum], movable->GetPosition(), 0.0f,sectors->Get(0)->QueryObject ()->GetName ());
            }
        }
    }
}

void psCelClient::HandleUnresolvedPos(GEMClientObject * entity, const csVector3 & pos, float rot, const csString & sector)
{
    //Error3("Handling unresolved %s at %s", entity->GetName(),sector.GetData());
    csList<UnresolvedPos*>::Iterator posIter = FindUnresolvedPos(entity);
    // if we already have a record about this entity, just update it, otherwise create new one
    if (posIter.HasCurrent())
    {
        (*posIter)->pos = pos;
        (*posIter)->rot = rot;
        (*posIter)->sector = sector;
        //Error1("-editing");
    }
    else
    {
        unresPos.PushBack(new UnresolvedPos(entity, pos, rot, sector));
        //Error1("-adding");
    }

    GEMClientActor* actor = dynamic_cast<GEMClientActor*> (entity);
    if(actor && actor->colldet)
    {
        // This will disable CD algorithms temporarily
        actor->colldet->SetOnGround(true);

        if (actor == local_player && psengine->GetCharControl())
            psengine->GetCharControl()->GetMovementManager()->StopAllMovement();
        actor->StopMoving(true);
    }

    // move the entity to special sector where we keep entities with unresolved position
    entity->SetPosition(pos, rot, unresSector);
}

void psCelClient::OnMapsLoaded()
{
    // look if some of the unresolved entities can't be resolved now
    csList<UnresolvedPos*>::Iterator posIter(unresPos);

    ++posIter;
    while (posIter.HasCurrent())
    {
        UnresolvedPos * pos = posIter.FetchCurrent();
        //Error3("Re-resolving %s at %s",pos->entity->GetName(), pos->sector.GetData());
        iSector * sector = psengine->GetEngine()->GetSectors ()->FindByName (pos->sector);
        if (sector)
        {
            //Error2("Successfuly resolved %s", pos->sector.GetData());
            if(pos->entity->pcmesh && pos->entity->pcmesh->GetMesh() && pos->entity->pcmesh->GetMesh()->GetMovable())
            {
                // If we have a mesh, no need to re set the position.
                iMovable* movable = pos->entity->pcmesh->GetMesh()->GetMovable();
                // Check if entity has moved to a different sector now, so no need to move back
                if(IsUnresSector(movable->GetSectors()->Get(0)))
                    movable->SetSector(sector);
            }
            else
                pos->entity->SetPosition(pos->pos, pos->rot, sector);

            GEMClientActor* actor = dynamic_cast<GEMClientActor*> (pos->entity);
            if(actor && actor->colldet)
                // we are now in a physical sector
                actor->colldet->SetOnGround(false);

            delete *posIter;
            // Deleting automatically increments the iterator.
            unresPos.Delete(posIter);
        }
        else
           ++posIter;
    }
}

void psCelClient::PruneEntities()
{
    // Only perform every 3 frames
    static int count = 0;
    count++;

    if (count % 3 != 0)
        return;
    else
        count = 0;

    for (size_t entNum = 0; entNum < entities.GetSize(); entNum++)
    {
        if ((GEMClientActor*) entities[entNum] == local_player)
            continue;

        iPcMesh * mesh = entities[entNum]->pcmesh;
        if (mesh != NULL  &&  mesh->GetMesh())
        {
            GEMClientActor* actor = dynamic_cast<GEMClientActor*> (entities[entNum]);
            if (actor)
            {
                csVector3 vel;
                vel = actor->linmove->GetVelocity();
                if (vel.y < -50)            // Large speed puts too much stress on CPU
                {
                    Debug3(LOG_ANY,0, "Disabling CD on actor(%d): %s", actor->GetID(),actor->GetName());
                    actor->colldet->SetOnGround(false);
                    // Reset velocity
                    actor->StopMoving(true);
                }
            }
        }
    }
}

//-------------------------------------------------------------------------------


GEMClientObject::GEMClientObject()
{
    entitylabel = NULL;
    shadow = 0;
    flags = 0;
    charApp = new psCharAppearance(psengine->GetObjectRegistry());
}

GEMClientObject::GEMClientObject( psCelClient* cel, PS_ID id )
{
    if (!this->cel)
        this->cel = cel;
                                
    entity = cel->GetPlLayer()->CreateEntity(id);
    entitylabel = NULL;
    shadow = 0;
    charApp = new psCharAppearance(psengine->GetObjectRegistry());
}    

GEMClientObject::~GEMClientObject()
{
    iMeshWrapper* mesh = pcmesh->GetMesh();
    if(mesh)
        psengine->GetEngine()->RemoveObject (mesh);
    
    cel->GetPlLayer()->RemoveEntity( entity );        
    delete charApp;
}

int GEMClientObject::GetMasqueradeType(void)
{
    return type;
}

bool GEMClientObject::SetPosition(const csVector3 & pos, float rot, iSector * sector)
{
    pcmesh->MoveMesh( sector , pos );

    // Rotation
    csMatrix3 matrix = (csMatrix3) csYRotMatrix3 (rot);
    pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T (matrix);

    return true;

}

void GEMClientObject::Move(const csVector3& pos,float rotangle,  const char* room)
{
    iSector* sector = psengine->GetEngine()->FindSector(room);
    if (sector)
        GEMClientObject::SetPosition(pos, rotangle, sector);
    else
        psengine->GetCelClient()->HandleUnresolvedPos(this, pos, rotangle, room);
}

bool GEMClientObject::InitMesh( const char *factname,
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
    
    // Helm Mesh Check
    // If there is helm specific item and we don't have any race yet, fall back to 
    // the stonebreaker model
    csString replacement("stonebm");        
    if ( cel->GetMainPlayer() )
    {
        replacement = cel->GetMainPlayer()->helmGroup;
    }                        
    psString factoryName(factname);
    factoryName.ReplaceAllSubString("$H", replacement); 
    
    csRef<iMeshFactoryWrapper> factory = psengine->GetEngine()->GetMeshFactories()->FindByName (factoryName);             
    if ( !factory )
    {
        // Try loading the mesh again
        csString filename;
        if (!psengine->GetFileNameByFact(factoryName, filename))
        {
            Error2( "Mesh Factory %s not found", factoryName.GetData() );            
            return false;
        }
        psengine->GetCacheManager()->LoadNewFactory(filename);
        factory = psengine->GetEngine()->GetMeshFactories()->FindByName (factoryName);             
        if (!factory)
        {
            Error2( "Mesh Factory %s not found", factoryName.GetData() );            
            return false;
        }
    }    

    csRef<iMeshWrapper> meshWrapper = factory->CreateMeshWrapper();

    pcmesh->SetMesh(meshWrapper);
    
    csRef<iSpriteCal3DState> calstate =  scfQueryInterface<iSpriteCal3DState> (pcmesh->GetMesh()->GetMeshObject());
    if (calstate)
        calstate->SetUserData((void *)this);


    iMeshWrapper* mesh = pcmesh->GetMesh();
    if ( !mesh )
    {
        Error2("Could not create Item because could not load %s file into mesh.",factname);
        return false;
    }

    charApp->SetMesh(mesh);
    Move(pos,rotangle,room);

    return true;
}

void GEMClientObject::ChangeName(const char* name)
{
    if (entity)
        entity->SetName(name);

    this->name = name;      
}

 

GEMClientActor::GEMClientActor( psCelClient* cel, psPersistActor& mesg ) 
               : GEMClientObject( cel, mesg.entityid )
{
    vel = 0;
    chatBubbleID = 0;
    name = mesg.name;
    race = mesg.race;
    helmGroup = mesg.helmGroup;
    entity->SetName( mesg.name );
    id = mesg.entityid;
    type = mesg.type;
    masqueradeType = mesg.masqueradeType;
    guildName = mesg.guild;
    control = mesg.control;
    flags   = mesg.flags;
    linmove = NULL;
    colldet = NULL;
    groupID = mesg.groupID;
    gender = mesg.gender;
    factname = mesg.factname;
    ownerEID = mesg.ownerEID;
    lastSentVelocity = lastSentRotation = 0.0f;
    stationary = true;
    movementMode = mesg.mode;
    serverMode = mesg.serverMode;
    alive = true;
    vitalManager = new psClientVitals;
    
    if ( helmGroup.Length() == 0 )
        helmGroup = factname;
    
    Debug3( LOG_CELPERSIST, 0, "Actor %s(%d) Received", mesg.name.GetData(), mesg.entityid );

    if ( !InitMesh(mesg.factname, mesg.filename, mesg.pos, mesg.yrot, mesg.sectorName) )
    {
        Error3("Fatal Error: Could not create actor %s(%d)", mesg.name.GetData(), mesg.entityid );
        return;
    }

    DRcounter = 0;  // mesg.counter cannot be trusted as it may have changed while the object was gone
    DRcounter_set = false;

    InitLinMove( mesg.pos, mesg.yrot, mesg.sectorName, mesg.top, mesg.bottom, mesg.offset );
    if (mesg.sector != NULL)
        linmove->SetDRData(mesg.on_ground,1.0f,mesg.pos,mesg.yrot,mesg.sector,mesg.vel,mesg.worldVel,mesg.ang_vel);
    else
        cel->HandleUnresolvedPos(this, mesg.pos, mesg.yrot, mesg.sectorName);
    
    
    InitCharData( mesg.texParts, mesg.equipment );

    RefreshCal3d();

    SetMode(serverMode, true);

    SetAnimationVelocity(mesg.vel);

    if (!control && (flags & psPersistActor::NAMEKNOWN))
        cel->GetEntityLabels()->OnObjectArrived(this);
    cel->GetShadowManager()->CreateShadow(this);
    
    lastDRUpdateTime = 0;

    ready = false;
}


GEMClientActor::~GEMClientActor()
{
    delete vitalManager;    
}

int GEMClientActor::GetAnimIndex (csStringHash* msgstrings, csStringID animid)
{
    if (!cal3dstate) 
    {
        return -1;
    }
    
    int idx = anim_hash.Get (animid, -1);
    if (idx >= 0)
    {
        return idx;
    }

    // Not cached yet.
    csString animName = msgstrings->Request (animid);
    idx = cal3dstate->FindAnim (animName);
    if (idx >= 0)
    {
        // Cache it.
        anim_hash.Put (animid, idx);
    }

    return idx;
}

csVector3 GEMClientActor::Pos()
{
    csVector3 pos;
    iSector* sector;
    float yrot;
    linmove->GetLastPosition (pos,yrot, sector);
    return pos;
}  

csVector3 GEMClientActor::Rot()
{
    csVector3 pos;
    csVector3 rot(0,0,0);
    iSector* sector;
    linmove->GetLastPosition (pos,rot.y, sector);
    return rot;
}  

iSector *GEMClientActor::GetSector()
{
    csVector3 pos;
    float yrot;
    iSector* sector;
    linmove->GetLastPosition (pos,yrot, sector);
    return sector;
}

bool GEMClientActor::NeedDRUpdate(unsigned char& priority)
{
    vel = linmove->GetVelocity();
    linmove->GetAngularVelocity(angularVelocity);

    // Never send DR messages until client is "ready"
    if (!ready )
        return false;

    if (linmove->IsPath() && !path_sent)
    {
        priority = PRIORITY_HIGH;
        return true;
    }

    csTicks timenow = csGetTicks();
    float delta =  timenow - lastDRUpdateTime;
    priority = PRIORITY_LOW;

    if (vel.IsZero () && angularVelocity.IsZero () &&
        lastSentVelocity.IsZero () && lastSentRotation.IsZero () &&
        delta>3000 && !stationary)
    {
        lastSentRotation = angularVelocity;
        lastSentVelocity = vel;
        priority = PRIORITY_HIGH;
        stationary = true;
        return true;
    }

    // has the velocity rotation changed since last DR msg sent?
    if ((angularVelocity != lastSentRotation || vel.x != lastSentVelocity.x
        || vel.z != lastSentVelocity.z) &&
        ((delta>250) || angularVelocity.IsZero ()
        || lastSentRotation.IsZero () || vel.IsZero ()
        || lastSentVelocity.IsZero ()))
    {
        lastSentRotation = angularVelocity;
        lastSentVelocity = vel;
        stationary = false;
        lastDRUpdateTime = timenow;
        return true;
    }

    if ((vel.y  && !lastSentVelocity.y) || (!vel.y && lastSentVelocity.y) || (vel.y > 0 && lastSentVelocity.y < 0) || (vel.y < 0 && lastSentVelocity.y > 0))
    {
        lastSentRotation = angularVelocity;
        lastSentVelocity = vel;
        stationary = false;
        lastDRUpdateTime = timenow;
        return true;
    }

    // if in motion, send every second instead of every 3 secs.
    if ((!lastSentVelocity.IsZero () || !lastSentRotation.IsZero () ) &&
        (delta > 1000))
    {
        lastSentRotation = angularVelocity;
        lastSentVelocity = vel;
        stationary = false;
        lastDRUpdateTime = timenow;
        return true;
    }

    // Real position/rotation/action is as calculated, no need for update.
    return false;
}

void GEMClientActor::SendDRUpdate(unsigned char priority, csStringHash* msgstrings)
{
    // send update out
    PS_ID mappedid = entity->GetID();  // no mapping anymore, IDs are identical
    bool on_ground;
    float speed,yrot,ang_vel;
    csVector3 pos, worldVel;
    iSector *sector;
//    bool hackflag=false;

    linmove->GetDRData(on_ground,speed,pos,yrot,sector,vel,worldVel,ang_vel);

    // Hack to guarantee out of order packet detection -- KWF
    //if (DRcounter%20 == 0)
    //{
    //    DRcounter-=2;
    //    hackflag = true;
    //}

    // ++DRcounter is the sequencer of these messages so the server and other 
    // clients do not use out of date messages when delivered out of order.
    psDRMessage drmsg(0, mappedid,on_ground,0,++DRcounter,pos,yrot,sector,vel,worldVel,ang_vel,msgstrings);
    drmsg.msg->priority = priority;

    //if (hackflag)
    //    DRcounter+=2;

    psengine->GetMsgHandler()->SendMessage(drmsg.msg);
}

void GEMClientActor::SetDRData(psDRMessage& drmsg)
{
    if (drmsg.sector != NULL)
    {
        if (!DRcounter_set || drmsg.IsNewerThan(DRcounter))
        {
            float cur_yrot;
            csVector3 cur_pos;
            iSector *cur_sector;
            linmove->GetLastPosition(cur_pos,cur_yrot,cur_sector);

            // Force hard DR update on sector change, low speed, or large delta pos
            if (drmsg.sector != cur_sector || (drmsg.vel < 0.1f) || (csSquaredDist::PointPoint(cur_pos,drmsg.pos) > 25.0f))
            {
                // Do hard DR when it would make you slide
                linmove->SetDRData(drmsg.on_ground,1.0f,drmsg.pos,drmsg.yrot,drmsg.sector,drmsg.vel,drmsg.worldVel,drmsg.ang_vel);
            }
            else
            {
                // Do soft DR when moving
                linmove->SetSoftDRData(drmsg.on_ground,1.0f,drmsg.pos,drmsg.yrot,drmsg.sector,drmsg.vel,drmsg.worldVel,drmsg.ang_vel);
            }

            DRcounter = drmsg.counter;
            DRcounter_set = true;
        }
        else
        {
            Error4("Ignoring DR pkt version %d for entity %s with version %d.", drmsg.counter, GetName(), DRcounter );
            return;
        }
    }
    else
    {
        psengine->GetCelClient()->HandleUnresolvedPos(this, drmsg.pos, drmsg.yrot, drmsg.sectorName);
    }
    
    // Update character mode and idle animation
    SetCharacterMode(drmsg.mode);
    
    // Update the animations to match velocity
    SetAnimationVelocity(drmsg.vel);
}

void GEMClientActor::StopMoving(bool worldVel)
{
    // stop this actor from moving
    csVector3 zeros(0.0f, 0.0f, 0.0f);
    linmove->SetVelocity(zeros);
    linmove->SetAngularVelocity(zeros);
    if(worldVel)
        linmove->ClearWorldVelocity();

}

bool GEMClientActor::SetPosition(const csVector3 & pos, float rot, iSector * sector)
{
    if (linmove)
        linmove->SetPosition(pos, rot, sector);
    return true;
}

bool GEMClientActor::InitCharData( const char* traits, const char* equipment )
{

    this->traits = traits;
    this->equipment = equipment;
    
    csString trt(traits);
    csString equip(equipment);
    
    charApp->ApplyTraits(trt);
    charApp->ApplyEquipment(equip);
    return true;    
}


bool GEMClientActor::InitLinMove(const csVector3& pos, float angle, const char* sector,
                                csVector3 top, csVector3 bottom, csVector3 offset )
{
    csRef<iCelPropertyClass> pc;
    pc = cel->GetPlLayer()->CreatePropertyClass(entity, "pclinearmovement");
    if ( !pc )
    {
        Error1("Could not create property class pclinearmovement.  Actor not created.");
        return false;
    }
    linmove =  scfQueryInterface<iPcLinearMovement> (pc);

    top.x *= .7f;
    top.z *= .7f;
    bottom.x *= .7f;
    bottom.z *= .7f;
    linmove->InitCD(top, bottom,offset, NULL);
    iSector * sectorObj = psengine->GetEngine()->FindSector(sector);
    if (sectorObj != NULL)
        linmove->SetPosition(pos,angle,sectorObj);
    else
        psengine->GetCelClient()->HandleUnresolvedPos(this, pos, angle, sector);
    colldet = CEL_QUERY_PROPCLASS(GetEntity()->GetPropertyClassList(), iPcCollisionDetection);

    return true;  // right now this func never fail, but might later.
}

bool GEMClientActor::IsGroupedWith(GEMClientActor* actor)
{
    if(actor && actor->GetGroupID() == groupID && groupID != 0)
        return true;
    else
        return false;
}

bool GEMClientActor::SetAnimation(const char* anim, int duration)
{
    // Check if the duration demands more than 1 playback?
    if (duration > cal3dstate->GetAnimationDuration())
    {
        // Yes. Set up callback to handle repetition
        int repeat = (int)(duration / cal3dstate->GetAnimationDuration());

        csRef<iSpriteCal3DFactoryState> sprite =
            
                                scfQueryInterface<iSpriteCal3DFactoryState> (pcmesh->GetMesh()->GetFactory()->GetMeshObjectFactory());

                                
        vmAnimCallback* callback = new vmAnimCallback;

        // Stuff callback's face with what he needs.
        callback->callbackcount = repeat;
        callback->callbacksprite = sprite;
        callback->callbackspstate = cal3dstate;
        callback->callbackanimation = anim;
        if (!sprite->RegisterAnimCallback(anim,callback,99999999999.9F)) // Make time huge as we do want only the end here
        {
            Error2("Failed to register callback for animation %s",anim);
            delete callback;
        }
        
    }

    float fadein = 0.25;
    float fadeout = 0.25;

    // TODO: Get these numbers from a file somewhere
    if ( strcmp(anim,"sit")==0 || strcmp(anim,"stand up")==0 )
    {
        fadein = 0.0;
        fadeout = 1.5;
    }

    return cal3dstate->SetAnimAction(anim,fadein,fadeout);
}

void GEMClientActor::SetAnimationVelocity(const csVector3& velocity)
{
    if (!alive)  // No zombies please
        return;

    // Taking larger of the 2 axis; cal3d axis are the opposite of CEL's
    bool useZ = ABS(velocity.z) > ABS(velocity.x);
    float cal3dvel = useZ ? velocity.z : velocity.x;

    // FIXME: Kludge.  When setting the velocity to 0 from a negative number,
    //        animations stay reversed unless we set it positive first.
    if (cal3dvel == 0)
    {
        cal3dstate->SetVelocity(0.1f, &psengine->GetRandomGen());
        cal3dstate->SetVelocity(0.0f, &psengine->GetRandomGen());
    }
    else
    {
        cal3dstate->SetVelocity(-cal3dvel, &psengine->GetRandomGen());
    }
}

void GEMClientActor::SetMode(uint8_t mode, bool newactor)
{
    if ((serverMode == psModeMessage::OVERWEIGHT || serverMode == psModeMessage::DEFEATED) && serverMode != mode)
        cal3dstate->SetAnimAction("stand up", 0.0f, 1.0f);

    SetAlive(mode != psModeMessage::DEAD, newactor);

    switch (mode)
    {
        case psModeMessage::PEACE:
        case psModeMessage::WORK:
        case psModeMessage::EXHAUSTED:
            SetIdleAnimation(psengine->GetCharControl()->GetMovementManager()->GetModeIdleAnim(movementMode));
            break;
        case psModeMessage::SPELL_CASTING:
            SetIdleAnimation("cast");
            break;

        case psModeMessage::COMBAT:
            // TODO: Get stance and set anim for that stance
            SetIdleAnimation("combat stand");
            break;
            
        case psModeMessage::DEAD:
            cal3dstate->ClearAllAnims();
            cal3dstate->SetAnimAction("death",0.0f,1.0f);
            if (newactor) // If we're creating a new actor that's already dead, we shouldn't show the animation...
                cal3dstate->SetAnimationTime(9999);  // the very end of the death animation ;)
            break;

        case psModeMessage::SIT:
        case psModeMessage::OVERWEIGHT:
        case psModeMessage::DEFEATED:
            cal3dstate->SetAnimAction("sit", 0.0f, 1.0f);
            SetIdleAnimation("sit idle");
            break;

        default:
            Error2("Unhandled mode: %d", mode);
            return;
    }
    serverMode = mode;
}

void GEMClientActor::SetCharacterMode(size_t newmode)
{
    if (newmode == movementMode)
        return;

    movementMode = newmode;

    if (serverMode == psModeMessage::PEACE)
        SetIdleAnimation(psengine->GetCharControl()->GetMovementManager()->GetModeIdleAnim(movementMode));
}

void GEMClientActor::SetAlive( bool newvalue, bool newactor )
{
    if (alive == newvalue)
        return;

    alive = newvalue;

    if (!newactor)
        psengine->GetCelClient()->GetEntityLabels()->RepaintObjectLabel(this);

    if (!alive)
        psengine->GetCelClient()->GetClientDR()->HandleDeath(this);
}

void GEMClientActor::SetIdleAnimation(const char* anim)
{
    cal3dstate->SetDefaultIdleAnim(anim);
    if (vel.IsZero())
        cal3dstate->SetVelocity(0);
}

void GEMClientActor::RefreshCal3d()
{
    cal3dstate =  scfQueryInterface<iSpriteCal3DState > ( pcmesh->GetMesh()->GetMeshObject());
    CS_ASSERT(cal3dstate);
}

void GEMClientActor::SetChatBubbleID(unsigned int chatBubbleID)
{
    this->chatBubbleID = chatBubbleID;
}

unsigned int GEMClientActor::GetChatBubbleID() const
{
    return chatBubbleID;
}

const char* GEMClientActor::GetName(bool trueName)
{
    static const char* strUnknown = "[Unknown]";
    if (trueName || (Flags() & psPersistActor::NAMEKNOWN) || ((uint)GetID() == psengine->GetCelClient()->GetMainActor()->GetID()))
        return name;
    return strUnknown;
}

GEMClientItem::GEMClientItem( psCelClient* cel, psPersistItem& mesg )
               : GEMClientObject( cel, mesg.id )
{        
    name = mesg.name;
    Debug3( LOG_CELPERSIST, 0, "Item %s(%d) Received", mesg.name.GetData(), mesg.id );
    entity->SetName( mesg.name );
    entity->SetID( mesg.id );
    id = mesg.id;
    type = mesg.type;
    factname = mesg.factname;

    InitMesh( mesg.factname, mesg.filename, mesg.pos, mesg.yRot, mesg.sector );

    if (mesg.flags & psPersistItem::COLLIDE)
    {
        cel->GetPlLayer()->CreatePropertyClass(entity, "pcsolid");
        solid = CEL_QUERY_PROPCLASS_ENT(entity, iPcSolid); 
        solid->SetMesh(pcmesh);
        solid->Setup();
    }

    cel->GetEntityLabels()->OnObjectArrived(this);
    cel->GetShadowManager()->CreateShadow(this);
}



GEMClientActionLocation::GEMClientActionLocation( psCelClient* cel, psPersistActionLocation& mesg ) 
               : GEMClientObject( cel, mesg.id )
{        
    name = mesg.name;

    Debug3( LOG_CELPERSIST, 0, "Action %s(%d) Received", mesg.name.GetData(), mesg.id );
    entity->SetName( mesg.name );
    entity->SetID( mesg.id );
    id = mesg.id;
    type = mesg.type;
    meshname = mesg.mesh;

    csRef<iCelPropertyClass> pc;

    pc = cel->GetPlLayer()->CreatePropertyClass(entity, "pcmesh");
    if ( !pc )
    {
        Error1("Could not create GEMClientActionLocation because pcmesh class couldn't be created.");
        return ;
    }

    pcmesh =  scfQueryInterface<iPcMesh > ( pc);
    if ( !pcmesh )
    {
        Error1("Could not create GEMClientActionLocation because pcmesh class doesn't implement iPcMesh.");
        return ;
    }

    csRef< iEngine > engine = psengine->GetEngine();
    csRef< iMeshWrapper > nullmesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", "ActionLocation");
    if ( !nullmesh )
    {
        Error1("Could not create GEMClientActionLocation because crystalspace.mesh.onbject.null could not be created.");
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


    Move( csVector3(0,0,0), 0.0f, mesg.sector);

}
