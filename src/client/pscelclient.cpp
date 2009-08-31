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
 *  Implements the various things relating to the CEL for the client.
 */
#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/scf.h>
#include <csutil/csstring.h>
#include <cstool/collider.h>
#include <iutil/cfgmgr.h>
#include <iutil/objreg.h>
#include <iutil/stringarray.h>
#include <iutil/vfs.h>
#include <ivaria/collider.h>
#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iengine/scenenode.h>
#include <imesh/object.h>
#include <imesh/objmodel.h>
#include <imesh/spritecal3d.h>
#include <imesh/nullmesh.h>
#include <imesh/nullmesh.h>
#include <ivideo/material.h>
#include <csgeom/math3d.h>

//============================
// Cal3D includes
//============================
#include "cal3d/cal3d.h"

//=============================================================================
// Library Includes
//=============================================================================
#include "effects/pseffect.h"
#include "effects/pseffectmanager.h"

#include "engine/psworld.h"
#include "engine/solid.h"
#include "engine/linmove.h"
#include "engine/colldet.h"

#include "net/messages.h"
#include "net/clientmsghandler.h"

#include "util/psconst.h"

#include "gui/pawsinfowindow.h"
#include "gui/pawsconfigkeys.h"
#include "gui/inventorywindow.h"
#include "gui/chatwindow.h"
#include "gui/pawslootwindow.h"

#include "paws/pawsmanager.h"


//=============================================================================
// Application Includes
//=============================================================================
#include "iclient/ibgloader.h"
#include "pscelclient.h"
#include "charapp.h"
#include "clientvitals.h"
#include "modehandler.h"
#include "zonehandler.h"
#include "psclientdr.h"
#include "entitylabels.h"
#include "shadowmanager.h"
#include "pscharcontrol.h"
#include "pscamera.h"
#include "psclientchar.h"
#include "pscal3dcallback.h"
#include "meshattach.h"
#include "globals.h"

psCelClient *GEMClientObject::cel = NULL;

psCelClient::psCelClient() : ignore_others(false)
{
    requeststatus = 0;

    clientdr        = NULL;
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



bool psCelClient::Initialize(iObjectRegistry* object_reg,
        MsgHandler* newmsghandler,
        ZoneHandler *zonehndlr)
{
    this->object_reg = object_reg;
    entityLabels = new psEntityLabels();
    entityLabels->Initialize(object_reg, this);

    shadowManager = new psShadowManager();

    vfs =  csQueryRegistry<iVFS> (object_reg);
    if (!vfs)
    {
        return false;
    }

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

    // Init nullmesh factory.
    nullfact = psengine->GetEngine()->CreateMeshFactory("crystalspace.mesh.object.null", "nullmesh");
    csRef<iNullFactoryState> nullstate = scfQueryInterface<iNullFactoryState> (nullfact->GetMeshObjectFactory());
    csBox3 bbox;
    bbox.AddBoundingVertex(csVector3(0.0f));
    nullstate->SetBoundingBox(bbox);

    LoadEffectItems();

    return true;
}

GEMClientObject* psCelClient::FindObject(EID id)
{
    return entities_hash.Get (id, NULL);
}

void psCelClient::SetMainActor(GEMClientActor* actor)
{
    if (actor)
    {
        local_player = actor;
        // ModeHandler has no good way to find out the entity
        psengine->GetModeHandler()->SetEntity(actor);
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
    if ( local_player == NULL || local_player->linmove == NULL ||
        (!psengine->ThreadedWorldLoading() && gameWorld == NULL))
        return false;
    else
        return true;
}

void psCelClient::HandleWorld( MsgEntry* me )
{
    if(!psengine->ThreadedWorldLoading())
    {
        psPersistWorld mesg( me );
        gameWorld = new psWorld;
        gameWorld->Initialize(object_reg, psengine->GetGFXFeatures());

        zonehandler->SetWorld(gameWorld);
        zonehandler->LoadZone(mesg.sector);
    }

    // Tell the user that we are loading the world
    psengine->AddLoadingWindowMsg("Loading world");

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

    // If this is the first actor to be sent...
    if ( local_player == NULL )
    {
        // Trigger a world load, as we now know where we are.
        psengine->GetLoader()->UpdatePosition(mesg.pos, mesg.sectorName, true);

        // Set the sector.
        mesg.sector = psengine->GetEngine()->FindSector(mesg.sectorName);
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

    GEMClientObject* found = FindObject(mesg.entityid);
    
    GEMClientActor* actor = new GEMClientActor( this, mesg );

    // The first actor that is sent to the client is his own one.
    if ( actor->control || (GetMainPlayer() && mesg.entityid == GetMainPlayer()->GetEID()) )
    {
        // cache the camera current view
        psengine->SetMainActor(actor);
        SetPlayerReady(true);

        // This triggers the server to update our proxlist
        local_player->SendDRUpdate(PRIORITY_LOW,GetClientDR()->GetMsgStrings());
        
        //update the window title with the char name
        psengine->UpdateWindowTitleInformations();
    }

    // We already have an entity with this id so we must have missed the remove object message,
    // or an update has been sent. delete the now outdated object
    if( found)
    {
        Debug3(LOG_CELPERSIST, 0, "Found existing object <%s> with %s, removing.\n", found->GetName(), ShowID(mesg.entityid));
        RemoveObject(found);
    }

    entities.Push(actor);
    entities_hash.Put(actor->GetEID(), actor);
}

void psCelClient::HandleMainActor( psPersistActor& mesg )
{

    // Saved for when we reset
    if ( local_player_defaultFactName.IsEmpty() )
    {
        local_player_defaultFactName = local_player->GetFactName();
        local_player_defaultFact = local_player->GetMesh()->GetFactory();
        local_player_defaultMesh = local_player->GetMesh()->GetMeshObject();
    }

    // Update equipment list
    local_player->equipment = mesg.equipment;
    local_player->type = mesg.type;

    if (mesg.factname != local_player->GetFactName())
    {
        local_player->charApp->ClearEquipment();

        csRef<iMeshFactoryWrapper> factory;
        while(!factory.IsValid())
        {
            factory = psengine->GetLoader()->LoadFactory(mesg.factname);
        }

        // New or resetting?
        if (local_player_defaultFactName != mesg.factname)
        {
            csRef<iMeshWrapper> meshwrap = psengine->GetEngine()->CreateMeshWrapper(factory, mesg.factname);
            csRef<iMeshObject> mesh = meshwrap->GetMeshObject();

            // Update
            local_player->GetMesh()->SetMeshObject(mesh);
            local_player->GetMesh()->SetFactory(factory);
            local_player->charApp->SetMesh(local_player->GetMesh());

            // Cal3d
            csRef<iSpriteCal3DState> calstate = scfQueryInterface<iSpriteCal3DState> (local_player->GetMesh()->GetMeshObject());
            if (calstate)
            {
                calstate->SetUserData((void*)local_player);
                calstate->SetVelocity(0.0,&psengine->GetRandomGen());
            }
        }
        else
        {
            // Reset
            local_player->GetMesh()->SetMeshObject(local_player_defaultMesh);
            local_player->GetMesh()->SetFactory(local_player_defaultFact);
            local_player->charApp->SetMesh(local_player->GetMesh());
        }

        // Update factory
        local_player->factName = mesg.factname;

        // Update Bracer/Helm Group
        if (mesg.helmGroup.Length() == 0)
            local_player->helmGroup = local_player->GetMesh()->GetFactory()->QueryObject()->GetName();
        else
            local_player->helmGroup = mesg.helmGroup;
        
        if (mesg.BracerGroup.Length() == 0)
            local_player->BracerGroup = local_player->GetMesh()->GetFactory()->QueryObject()->GetName();
        else
            local_player->BracerGroup = mesg.BracerGroup;

        if (mesg.BeltGroup.Length() == 0)
            local_player->BeltGroup = local_player->GetMesh()->GetFactory()->QueryObject()->GetName();
        else
            local_player->BeltGroup = mesg.BeltGroup;

        if (mesg.CloakGroup.Length() == 0)
            local_player->CloakGroup = local_player->GetMesh()->GetFactory()->QueryObject()->GetName();
        else
            local_player->CloakGroup = mesg.CloakGroup;

        // Update cal3d
        local_player->RefreshCal3d();

        local_player->charApp->ApplyEquipment(local_player->equipment);
    }

    if(mesg.mountFactname != local_player->mountFactname)
    {
        local_player->mountFactname = mesg.mountFactname;
        local_player->CheckLoadStatus();
    }
}

void psCelClient::HandleItem( MsgEntry* me )
{
    psPersistItem mesg( me );

    // if we already have an entity with this id, update the item
    GEMClientItem *foundItem = dynamic_cast<GEMClientItem*>(FindObject(mesg.eid));
    if(foundItem)
    {
        Debug3(LOG_CELPERSIST, 0, "Found existing item<%s> object with %s, updating.\n", foundItem->GetName(), ShowID(mesg.eid));
        foundItem->UpdateItem( mesg );
        return;
    }

    GEMClientItem* newItem = new GEMClientItem( this, mesg );

    entities.Push(newItem);
    entities_hash.Put(newItem->GetEID(), newItem);
}

void psCelClient::HandleActionLocation( MsgEntry* me )
{
    psPersistActionLocation mesg( me );

    // We already have an entity with this id so we must have missed the remove object message
    // so delete and remake it.
    GEMClientObject *found = (GEMClientObject*) FindObject(mesg.eid);
    if ( found )
    {
        Debug3(LOG_CELPERSIST, 0, "Found existing location<%s> object with %s, removing.\n", found->GetName(), ShowID(mesg.eid));
        RemoveObject( found );
    }

    GEMClientActionLocation * newAction = new GEMClientActionLocation( this, mesg );
    entities.Push( newAction );
    actions.Push( newAction );
    entities_hash.Put (newAction->GetEID(), newAction);
}

void psCelClient::HandleObjectRemoval( MsgEntry* me )
{
    psRemoveObject mesg(me);

    ForceEntityQueues();

    GEMClientObject* entity = FindObject( mesg.objectEID );

    if (entity)
    {
        RemoveObject(entity);
        Debug2(LOG_CELPERSIST, 0, "Object %s Removed", ShowID(mesg.objectEID));
    }
    else
    {
        Debug2(LOG_CELPERSIST, 0, "Server told us to remove %s, but it does not seem to exist.", ShowID(mesg.objectEID));
    }
}

void psCelClient::LoadEffectItems()
{
    if(vfs->Exists("/planeshift/art/itemeffects.xml"))
    {
        csRef<iDocument> doc = ParseFile(psengine->GetObjectRegistry(), "/planeshift/art/itemeffects.xml");
        if (!doc)
        {
            Error2("Couldn't parse file %s", "/planeshift/art/itemeffects.xml");
            return;
        }

        csRef<iDocumentNode> root = doc->GetRoot();
        if (!root)
        {
            Error2("The file(%s) doesn't have a root", "/planeshift/art/itemeffects.xml");
            return;
        }

        csRef<iDocumentNodeIterator> itemeffects = root->GetNodes("itemeffect");
        while(itemeffects->HasNext())
        {
            csRef<iDocumentNode> itemeffect = itemeffects->Next();
            csRef<iDocumentNodeIterator> effects = itemeffect->GetNodes("pseffect");
            csRef<iDocumentNodeIterator> lights = itemeffect->GetNodes("light");
            if(effects->HasNext() || lights->HasNext())
            {
                ItemEffect* ie = new ItemEffect();
                while(effects->HasNext())
                {
                    csRef<iDocumentNode> effect = effects->Next();
                    Effect* eff = new Effect();
                    eff->rotateWithMesh = effect->GetAttributeValueAsBool("rotatewithmesh", false);
                    eff->effectname = csString(effect->GetNode("effectname")->GetContentsValue());
                    eff->effectoffset = csVector3(effect->GetNode("offset")->GetAttributeValueAsFloat("x"),
                        effect->GetNode("offset")->GetAttributeValueAsFloat("y"),
                        effect->GetNode("offset")->GetAttributeValueAsFloat("z"));
                    ie->effects.PushSmart(eff);
                }
                while(lights->HasNext())
                {
                    csRef<iDocumentNode> light = lights->Next();
                    Light* li = new Light();
                    li->colour = csColor(light->GetNode("colour")->GetAttributeValueAsFloat("r"),
                        light->GetNode("colour")->GetAttributeValueAsFloat("g"),
                        light->GetNode("colour")->GetAttributeValueAsFloat("b"));
                    li->lightoffset = csVector3(light->GetNode("offset")->GetAttributeValueAsFloat("x"),
                        light->GetNode("offset")->GetAttributeValueAsFloat("y"),
                        light->GetNode("offset")->GetAttributeValueAsFloat("z"));
                    li->radius = light->GetNode("radius")->GetContentsValueAsFloat();
                    ie->lights.PushSmart(li);
                }
                ie->activeOnGround = itemeffect->GetAttributeValueAsBool("activeonground");
                effectItems.PutUnique(csString(itemeffect->GetAttributeValue("meshname")), ie);
            }
        }
    }
    else
    {
        printf("Could not load itemeffects.xml!\n");
    }
}

void psCelClient::HandleItemEffect( const char* factName, csRef<iMeshWrapper> mw, bool onGround, const char* slot,
                                    csHash<int, csString> *effectids, csHash<int, csString> *lightids )
{
    ItemEffect* ie = effectItems.Get(factName, 0);
    if(ie)
    {
        if(onGround && !ie->activeOnGround)
        {
            return;
        }

        if(!mw)
        {
            Error2("Error loading effect for item %s. iMeshWrapper is null.\n", factName);
            return;
        }

        for(size_t i=0; i<ie->lights.GetSize(); i++)
        {
            Light* l = ie->lights.Get(i);
            unsigned int id = psengine->GetEffectManager()->AttachLight(factName, l->lightoffset,
                l->radius, l->colour, mw);

            if(!id)
            {
              printf("Failed to create light on item %s!\n", factName);
            }
            else if(slot && lightids)
            {
                lightids->PutUnique(slot, id);
            }
        }

        for(size_t i=0; i<ie->effects.GetSize(); i++)
        {
            Effect* e = ie->effects.Get(i);
            unsigned int id = psengine->GetEffectManager()->RenderEffect(e->effectname, e->effectoffset, mw, 0,
                                                                         csVector3(0,1,0), 0, e->rotateWithMesh);
            if(!id)
            {
              printf("Failed to load effect %s on item %s!\n", e->effectname.GetData(), factName);
            }
            else if(slot && effectids)
            {
                effectids->PutUnique(slot, id);
            }
        }
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
    if(loot && loot->GetLootingActor() == entity->GetEID())
    {
        loot->Hide();
    }

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

    entities_hash.Delete (entity->GetEID(), entity);
    entities.Delete(entity);
}

bool psCelClient::IsMeshSubjectToAction(const char* sector,const char* mesh)
{
    if(!mesh)
        return false;

    for(size_t i = 0; i < actions.GetSize();i++)
    {
        GEMClientActionLocation* action = actions[i];
        const char* sec = action->GetSector()->QueryObject()->GetName();

        if(!strcmp(action->GetMeshName(),mesh) && !strcmp(sector,sec))
            return true;
    }

    return false;
}

GEMClientActor * psCelClient::GetActorByName(const char * name, bool trueName) const
{
    size_t len = entities.GetSize();
    csString testName, firstName;
    csString csName(name);
    csName = csName.Downcase();

    for (size_t a=0; a<len; ++a)
    {
        GEMClientActor * actor = dynamic_cast<GEMClientActor*>(entities[a]);
        if (!actor)
            continue;

        testName = actor->GetName(trueName);
        firstName = testName.Slice(0, testName.FindFirst(' ')).Downcase();
        if (firstName == csName)
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
        printf("Warning: Got rename message, but couldn't find actor %s!\n", ShowID(msg.objectID));
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
    if (GetMainPlayer() != object)
        entityLabels->OnObjectArrived(object);
    else //we have to update the window title
        psengine->UpdateWindowTitleInformations();

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
        if (GetMainPlayer() != actor)
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
        Error2("Couldn't find %s, ignoring group change.", ShowID(msg.objectID));
        return;
    }
    printf("Got group update for actor %s (%s) to group %d\n", actor->GetName(), ShowID(actor->GetEID()), msg.groupID);
    unsigned int oldGroup = actor->GetGroupID();
    actor->SetGroupID(msg.groupID);

    // repaint label
    if (GetMainPlayer() != actor)
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

void psCelClient::ForceEntityQueues()
{
    while (!newActorQueue.IsEmpty())
    {
        csRef<MsgEntry> me = newActorQueue.Pop();
        HandleActor(me);
    }
    if (!newItemQueue.IsEmpty())
    {
        csRef<MsgEntry> me = newItemQueue.Pop();
        HandleItem(me);
    }
}

void psCelClient::CheckEntityQueues()
{
    if (newActorQueue.GetSize())
    {
        csRef<MsgEntry> me = newActorQueue.Pop();
        HandleActor(me);
        return;
    }
    if (newItemQueue.GetSize())
    {
        csRef<MsgEntry> me = newItemQueue.Pop();
        HandleItem(me);
        return;
    }
}

void psCelClient::Update(bool loaded)
{
    if(local_player)
    {
        if(psengine->ThreadedWorldLoading())
        {
            const char* sectorName = local_player->GetSector()->QueryObject()->GetName();
            if(!sectorName)
                return;

            // Update loader.
            psengine->GetLoader()->UpdatePosition(local_player->Pos(), sectorName, false);
        }
/*
        // Check if we're inside a water area.
        csColor4* waterColour = 0;
        if(psengine->GetLoader()->InWaterArea(sectorName, &psengine->GetPSCamera()->GetPosition(), &waterColour))
        {
            psWeatherMessage::NetWeatherInfo fog;
            fog.fogType = CS_FOG_MODE_EXP;
            fog.fogColour = *waterColour;
            fog.has_downfall = false;
            fog.has_fog = true;
            fog.has_lightning = false;
            fog.sector = sectorName;
            fog.fog_density = 100000;
            fog.fog_fade = 0;
            psengine->GetModeHandler()->ProcessFog(fog);
        }
        else
        {
            printf("Removing\n");
            psWeatherMessage::NetWeatherInfo fog;
            fog.has_downfall = false;
            fog.has_fog = false;
            fog.has_lightning = false;
            fog.sector = sectorName;
            fog.fog_density = 0;
            fog.fog_fade = 0;
            psengine->GetModeHandler()->ProcessFog(fog);
        }*/
    }

    if(loaded)
    {
      for(size_t i =0; i < entities.GetSize();i++)
      {
          entities[i]->Update();
      }

      shadowManager->UpdateShadows();
    }
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
            newActorQueue.Push(me);
            break;
        }

        case MSGTYPE_PERSIST_ITEM:
        {
            if (!ignore_others)
            {
                newItemQueue.Push(me);
            }
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

void psCelClient::OnRegionsDeleted(csArray<iCollection*>& regions)
{
    size_t entNum;

    for (entNum = 0; entNum < entities.GetSize(); entNum++)
    {
        csRef<iSector> sector = entities[entNum]->GetSector();
        if (sector.IsValid())
        {
            // Shortcut the lengthy region check if possible
            if(IsUnresSector(sector))
                continue;

            bool unresolved = true;

            iSector* sectorToBeDeleted = 0;

            // Are all the sectors going to be unloaded?
            iSectorList* sectors = entities[entNum]->GetSectors();
            for(int i = 0;i<sectors->GetCount();i++)
            {
                // Get the iRegion this sector belongs to
                csRef<iCollection> region =  scfQueryInterfaceSafe<iCollection> (sectors->Get(i)->QueryObject()->GetObjectParent());
                if(regions.Find(region)==csArrayItemNotFound)
                {
                    // We've found a sector that won't be unloaded so the mesh won't need to be moved
                    unresolved = false;
                    break;
                } else {
                    sectorToBeDeleted = sectors->Get(i);
                }
            }

            if(unresolved)
            {
                // All the sectors the mesh is in are going to be unloaded
                Warning1(LOG_ANY,"Moving entity to temporary sector");
                // put the mesh to the sector that server uses for keeping meshes located in unload maps
                HandleUnresolvedPos(entities[entNum], entities[entNum]->GetPosition(), 0.0f, sectorToBeDeleted->QueryObject ()->GetName ());
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
    if(actor)
    {
        // This will disable CD algorithms temporarily
        actor->GetMovement()->SetOnGround(true);

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
            if(pos->entity->GetMesh() && pos->entity->GetMesh()->GetMovable())
            {
                // If we have a mesh, no need to re set the position.
                iMovable* movable = pos->entity->GetMesh()->GetMovable();
                // Check if entity has moved to a different sector now, so no need to move back
                if(IsUnresSector(movable->GetSectors()->Get(0)))
                    movable->SetSector(sector);
            }
            else
                pos->entity->SetPosition(pos->pos, pos->rot, sector);

            GEMClientActor* actor = dynamic_cast<GEMClientActor*> (pos->entity);
            if(actor)
                // we are now in a physical sector
                actor->GetMovement()->SetOnGround(false);

            delete *posIter;
            unresPos.Delete(posIter);
            ++posIter;
        }
        else
           ++posIter;
    }
    GEMClientActor* actor = GetMainPlayer();
    if (actor)
        actor->GetMovement()->SetOnGround(false);
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

        csRef<iMeshWrapper> mesh = entities[entNum]->GetMesh();
        if (mesh != NULL)
        {
            GEMClientActor* actor = dynamic_cast<GEMClientActor*> (entities[entNum]);
            if (actor)
            {
                csVector3 vel;
                vel = actor->GetMovement()->GetVelocity();
                if (vel.y < -50)            // Large speed puts too much stress on CPU
                {
                    Debug3(LOG_ANY, 0, "Disabling CD on actor %s(%s)", actor->GetName(), ShowID(actor->GetEID()));
                    actor->GetMovement()->SetOnGround(false);
                    // Reset velocity
                    actor->StopMoving(true);
                }
            }
        }
    }
}


void psCelClient::AttachObject( iObject* object, GEMClientObject* clientObject)
{
  csRef<psGemMeshAttach> attacher = csPtr<psGemMeshAttach> (new psGemMeshAttach(clientObject));
  attacher->SetName (clientObject->GetName()); // @@@ For debugging mostly.
  csRef<iObject> attacher_obj (scfQueryInterface<iObject> (attacher));
  object->ObjAdd (attacher_obj);
}


void psCelClient::UnattachObject( iObject* object, GEMClientObject* clientObject)
{
    csRef<psGemMeshAttach> attacher (CS::GetChildObject<psGemMeshAttach>(object));
    if (attacher)
    {
        if ( attacher->GetObject () == clientObject )
        {
            csRef<iObject> attacher_obj (scfQueryInterface<iObject> (attacher));
            object->ObjRemove (attacher_obj);
        }
    }
}

GEMClientObject* psCelClient::FindAttachedObject(iObject* object)
{
    GEMClientObject* found = 0;

    csRef<psGemMeshAttach> attacher (CS::GetChildObject<psGemMeshAttach>(object));
    if (attacher)
    {
        found = attacher->GetObject();
    }

    return found;
}


csArray<GEMClientObject*> psCelClient::FindNearbyEntities (iSector* sector, const csVector3& pos, float radius, bool doInvisible)
{
    csArray<GEMClientObject*> list;

    csRef<iMeshWrapperIterator> obj_it = psengine->GetEngine()->GetNearbyMeshes (sector, pos, radius);

    while (obj_it->HasNext ())
    {
        iMeshWrapper* m = obj_it->Next ();
        if (!doInvisible)
        {
            bool invisible = m->GetFlags ().Check (CS_ENTITY_INVISIBLE);
            if (invisible)
                continue;
        }
        GEMClientObject* object = FindAttachedObject(m->QueryObject());

        if (object)
        {
            list.Push(object);
        }
    }
    return list;
}

csPtr<InstanceObject> psCelClient::FindInstanceObject(const char* name) const
{
    return csPtr<InstanceObject> (instanceObjects.Get(name, csRef<InstanceObject>()));
}

void psCelClient::AddInstanceObject(const char* name, csRef<InstanceObject> object)
{
    instanceObjects.Put(name, object);
}

//-------------------------------------------------------------------------------


GEMClientObject::GEMClientObject()
{
    entitylabel = NULL;
    shadow = 0;
    hasShadow = true;
    flags = 0;
    charApp = new psCharAppearance(psengine->GetObjectRegistry());
}

GEMClientObject::GEMClientObject(psCelClient* cel, EID id) : eid(id)
{
    if (!this->cel)
        this->cel = cel;

    eid = id;
    //entity = cel->GetPlLayer()->CreateEntity(id);
    entitylabel = NULL;
    shadow = 0;
    hasShadow = true;
    charApp = new psCharAppearance(psengine->GetObjectRegistry());
}

GEMClientObject::~GEMClientObject()
{
    if(pcmesh)
    {
        cel->GetShadowManager()->RemoveShadow(this);
        cel->UnattachObject(pcmesh->QueryObject(), this);
        psengine->GetEngine()->RemoveObject (pcmesh);
    }

    delete charApp;
}

int GEMClientObject::GetMasqueradeType(void)
{
    return type;
}

bool GEMClientObject::SetPosition(const csVector3 & pos, float rot, iSector * sector)
{
    if(pcmesh.IsValid())
    {
        if (sector)
            pcmesh->GetMovable ()->SetSector (sector);

        pcmesh->GetMovable ()->SetPosition (pos);

        // Rotation
        csMatrix3 matrix = (csMatrix3) csYRotMatrix3 (rot);
        pcmesh->GetMovable()->GetTransform().SetO2T (matrix);
        
        pcmesh->GetMovable ()->UpdateMove ();

        if(instance.IsValid())
        {
            // Update the sector and position of real mesh.
            if (sector)
            {
                instance->pcmesh->GetMovable()->SetSector (sector);
                instance->pcmesh->GetMovable()->SetPosition(0.0f);
                instance->pcmesh->GetMovable()->UpdateMove();
            }

            // Set instancing transform.
            position->SetValue(pcmesh->GetMovable()->GetTransform());
        }
    }

    return true;

}

void GEMClientObject::Rotate(float xRot, float yRot, float zRot)
{
    // calculate the rotation matrix from the axis rotation
    csMatrix3 xmatrix = (csMatrix3) csXRotMatrix3 (xRot);

    csMatrix3 ymatrix = (csMatrix3) csYRotMatrix3 (yRot);

    csMatrix3 zmatrix = (csMatrix3) csZRotMatrix3 (zRot);
    
    // multiply the matrices for the three axis together, then we apply it to the mesh 
    pcmesh->GetMovable ()->GetTransform().SetO2T (xmatrix*ymatrix*zmatrix);
    
    pcmesh->GetMovable ()->UpdateMove ();

    // Set instancing transform.
    if(instance.IsValid())
    {
        position->SetValue(pcmesh->GetMovable ()->GetTransform());
    }
}

csVector3 GEMClientObject::GetPosition()
{
    return pcmesh->GetMovable ()->GetFullPosition();
}

float GEMClientObject::GetRotation()
{
    // Rotation
    csMatrix3 transf = pcmesh->GetMovable()->GetTransform().GetT2O();
    return psWorld::Matrix2YRot(transf);
}

iSector* GEMClientObject::GetSector() const
{
    if(pcmesh && pcmesh->GetMovable()->InSector())
    {
        return pcmesh->GetMovable()->GetSectors()->Get(0);
    }
    return NULL;
}

iSectorList* GEMClientObject::GetSectors() const
{
  return pcmesh->GetMovable()->GetSectors();
}

const csBox3& GEMClientObject::GetBBox() const
{
    if(instance.IsValid())
    {
        return instance->bbox;
    }
    else
    {
        return pcmesh->GetMeshObject()->GetObjectModel()->GetObjectBoundingBox();
    }
}

csRef<iMeshWrapper> GEMClientObject::GetMesh() const
{
    return pcmesh;
}

void GEMClientObject::Update()
{
}

void GEMClientObject::Move(const csVector3& pos,float rotangle,  const char* room)
{
    iSector* sector = psengine->GetEngine()->FindSector(room);
    if (sector)
        GEMClientObject::SetPosition(pos, rotangle, sector);
    else
        cel->HandleUnresolvedPos(this, pos, rotangle, room);
}

bool GEMClientObject::InitMesh()
{
    // Check for nullmesh.
    if(factName == "nullmesh")
    {
        csRef<iMeshFactoryWrapper> nullmesh = psengine->GetEngine()->FindMeshFactory("nullmesh");
        if(!nullmesh)
        {
            nullmesh = psengine->GetEngine()->CreateMeshFactory("crystalspace.mesh.object.null", "nullmesh");
            csRef<iNullFactoryState> nullstate = scfQueryInterface<iNullFactoryState> (nullmesh->GetMeshObjectFactory());
            csBox3 bbox;
            bbox.AddBoundingVertex(csVector3(0.0f));
            nullstate->SetBoundingBox(bbox);
        }

        pcmesh = psengine->GetEngine()->CreateMeshWrapper(nullmesh, name);
        pcmesh->GetFlags().Set(CS_ENTITY_NODECAL);
        cel->AttachObject(pcmesh->QueryObject(), this);
        hasShadow = false;

        PostLoad(true);

        return true;
    }

    // Helm/bracer Mesh Check
    // If there is helm/bracer specific item and we don't have any race yet, fall back to
    // the stonebreaker model
    csString HelmReplacement("stonebm");
    csString BracerReplacement("stonebm");
    csString BeltReplacement("stonebm");
    csString CloakReplacement("stonebm");
    if ( cel->GetMainPlayer() )
    {
        HelmReplacement = cel->GetMainPlayer()->helmGroup;
        BracerReplacement = cel->GetMainPlayer()->BracerGroup;
        BeltReplacement = cel->GetMainPlayer()->BeltGroup;
        CloakReplacement = cel->GetMainPlayer()->CloakGroup;
    }
    psString factoryName(factName);
    factoryName.ReplaceAllSubString("$H", HelmReplacement);
    factoryName.ReplaceAllSubString("$B", BracerReplacement);
    factoryName.ReplaceAllSubString("$H", BeltReplacement);
    factoryName.ReplaceAllSubString("$B", CloakReplacement);
    factName = factoryName;

    // Set up callback.
    psengine->RegisterDelayedLoader(this);

    // Check if the mesh is already loaded.
    CheckLoadStatus();

    return true;
}

void GEMClientObject::ChangeName(const char* name)
{
    this->name = name;
}

float GEMClientObject::RangeTo(GEMClientObject * obj, bool ignoreY)
{
    if ( ignoreY )
    {
        csVector3 pos1 = this->GetPosition();
        csVector3 pos2 = obj->GetPosition();
        return ( sqrt(  (pos1.x - pos2.x)*(pos1.x - pos2.x)+
                    (pos1.z - pos2.z)*(pos1.z - pos2.z)));
    }
    else
    {
        return ( cel->GetWorld()->Distance(this->GetMesh() , obj->GetMesh()));
    }
}

GEMClientActor::GEMClientActor( psCelClient* cel, psPersistActor& mesg )
               : GEMClientObject( cel, mesg.entityid ), post_load(new PostLoadData)
{
    chatBubbleID = 0;
    name = mesg.name;
    race = mesg.race;
    mountFactname = mesg.mountFactname;
    helmGroup = mesg.helmGroup;
    BracerGroup = mesg.BracerGroup;
    BeltGroup = mesg.BeltGroup;
    CloakGroup = mesg.CloakGroup;
    type = mesg.type;
    masqueradeType = mesg.masqueradeType;
    guildName = mesg.guild;
    control = mesg.control;
    flags   = mesg.flags;
    linmove = 0;
    groupID = mesg.groupID;
    gender = mesg.gender;
    factName = mesg.factname;
    ownerEID = mesg.ownerEID;
    lastSentVelocity = lastSentRotation = 0.0f;
    stationary = true;
    movementMode = mesg.mode;
    serverMode = mesg.serverMode;
    alive = true;
    vitalManager = new psClientVitals;
    equipment = mesg.equipment;
    post_load->pos = mesg.pos;
    post_load->yrot = mesg.yrot;
    post_load->sectorName = mesg.sectorName;
    post_load->top = mesg.top;
    post_load->bottom = mesg.bottom;
    post_load->offset = mesg.offset;
    post_load->on_ground = mesg.on_ground;
    post_load->sector = mesg.sector;
    post_load->vel = mesg.vel;
    post_load->worldVel = mesg.worldVel;
    post_load->ang_vel = mesg.ang_vel;
    post_load->texParts = mesg.texParts;

    if (helmGroup.Length() == 0)
        helmGroup = factName;
        
    if (BracerGroup.Length() == 0)
        BracerGroup = factName;
        
    if (BeltGroup.Length() == 0)
        BeltGroup = factName;

    if (CloakGroup.Length() == 0)
        CloakGroup = factName;

    Debug3(LOG_CELPERSIST, 0, "Actor %s(%s) Received", mesg.name.GetData(), ShowID(mesg.entityid));

    if (!InitMesh())
    {
        Error3("Fatal Error: Could not create actor %s(%s)", mesg.name.GetData(), ShowID(mesg.entityid));
        return;
    }

    DRcounter = 0;  // mesg.counter cannot be trusted as it may have changed while the object was gone
    DRcounter_set = false;
}


GEMClientActor::~GEMClientActor()
{
    delete vitalManager;
    delete linmove;
}

void GEMClientActor::PostLoad(bool nullmesh)
{
    InitLinMove(post_load->pos, post_load->yrot, post_load->sectorName,
        post_load->top, post_load->bottom, post_load->offset);
    if (post_load->sector != NULL)
        linmove->SetDRData(post_load->on_ground, 1.0f, post_load->pos,
        post_load->yrot, post_load->sector, post_load->vel, post_load->worldVel, post_load->ang_vel);
    else
        cel->HandleUnresolvedPos(this, post_load->pos, post_load->yrot, post_load->sectorName);

    if(!nullmesh)
    {
        InitCharData(post_load->texParts, equipment);
        RefreshCal3d();
        SetAnimationVelocity(post_load->vel);
        SetMode(serverMode, true);
        if (!control && (flags & psPersistActor::NAMEKNOWN))
            cel->GetEntityLabels()->OnObjectArrived(this);
        cel->GetShadowManager()->CreateShadow(this);
    }

    // Move into position
    Move(post_load->pos, post_load->yrot, post_load->sectorName);

    lastDRUpdateTime = 0;

    ready = false;

    linmove->SetDeltaLimit(0.2f);

    delete post_load;
    post_load = NULL;
}

int GEMClientActor::GetAnimIndex (csStringHashReversible* msgstrings, csStringID animid)
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

    if(animName.IsEmpty()) //check if we have an hit else bug the user for the bad data
    {
        Error2("Missing animName from common strings for animid %lu!\n", (unsigned long int)animid);
    }
    else //no need to call this with an empty string in case of bad data so let's skip it in that case
    {
        idx = cal3dstate->FindAnim (animName.GetData());
    }

    if (idx >= 0)
    {
        // Cache it.
        anim_hash.Put (animid, idx);
    }

    return idx;
}

void GEMClientActor::Update()
{
    if(linmove)
        linmove->TickEveryFrame();
}

void GEMClientActor::GetLastPosition (csVector3& pos, float& yrot, iSector*& sector)
{
    if(linmove)
        linmove->GetLastPosition (pos,yrot,sector);
    else
        sector = cel->unresSector;
}

const csVector3 GEMClientActor::GetVelocity () const
{
    return linmove->GetVelocity();
}

csVector3 GEMClientActor::Pos() const
{
    csVector3 pos(0.0f);

    if(linmove)
    {
        float yrot;
        iSector* sector;
        linmove->GetLastPosition (pos, yrot, sector);
    }

    return pos;
}

csVector3 GEMClientActor::Rot() const
{
    float yrot = 0.0f;

    if(linmove)
    {
        csVector3 pos(0.0f);
        iSector* sector;
        linmove->GetLastPosition (pos, yrot, sector);
    }

    return yrot;
}

iSector *GEMClientActor::GetSector() const
{
    csVector3 pos;
    float yrot;
    iSector* sector = cel->unresSector;
    if(linmove)
        linmove->GetLastPosition (pos,yrot, sector);
    return sector;
}

bool GEMClientActor::NeedDRUpdate(unsigned char& priority)
{
    csVector3 vel = linmove->GetVelocity();
    csVector3 angularVelocity = linmove->GetAngularVelocity();

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

void GEMClientActor::SendDRUpdate(unsigned char priority, csStringHashReversible* msgstrings)
{
    if(!linmove)
        return;

    // send update out
    EID mappedid = eid;  // no mapping anymore, IDs are identical
    float speed, yrot, ang_vel;
    bool on_ground;
    csVector3 pos, vel, worldVel;
    iSector* sector;

    linmove->GetDRData(on_ground, speed, pos, yrot, sector, vel, worldVel, ang_vel);

    ZoneHandler* zonehandler = cel->GetZoneHandler();
    if (zonehandler && zonehandler->IsMapLoadNeeded())
    {
        // disable movement to stop stamina drain while map is loading
        on_ground = true;
        speed = 0;
        vel = 0;
        ang_vel = 0;
    }

    // Hack to guarantee out of order packet detection -- KWF
    //if (DRcounter%20 == 0)
    //{
    //    DRcounter-=2;
    //    hackflag = true;
    //}

    // ++DRcounter is the sequencer of these messages so the server and other
    // clients do not use out of date messages when delivered out of order.
    psDRMessage drmsg(0, mappedid, on_ground, 0, ++DRcounter, pos, yrot, sector,
        sector->QueryObject()->GetName(), vel, worldVel, ang_vel, msgstrings);
    drmsg.msg->priority = priority;

    //if (hackflag)
    //    DRcounter+=2;

    psengine->GetMsgHandler()->SendMessage(drmsg.msg);
}

void GEMClientActor::SetDRData(psDRMessage& drmsg)
{
    if(!linmove)
        return;

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
        cel->HandleUnresolvedPos(this, drmsg.pos, drmsg.yrot, drmsg.sectorName);
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

psLinearMovement * GEMClientActor::GetMovement()
{
    return linmove;
}

bool GEMClientActor::InitLinMove(const csVector3& pos, float angle, const char* sector,
                                csVector3 top, csVector3 bottom, csVector3 offset )
{
    linmove = new psLinearMovement(psengine->GetObjectRegistry());

    top.x *= .7f;
    top.z *= .7f;
    bottom.x *= .7f;
    bottom.z *= .7f;
    linmove->InitCD(top, bottom,offset, pcmesh);
    iSector * sectorObj = psengine->GetEngine()->FindSector(sector);
    if (sectorObj != NULL)
        linmove->SetPosition(pos,angle,sectorObj);
    else
        psengine->GetCelClient()->HandleUnresolvedPos(this, pos, angle, sector);

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
    if(!cal3dstate)
        return true;

    int animation = cal3dstate->FindAnim(anim);
    if (animation < 0)
    {
        Error3("Didn't find animation '%s' for '%s'.",anim , GetName());
        return false;
    }

    if (!cal3dstate->GetCal3DModel()->getCoreModel()->getCoreAnimation(animation))
    {
        Error3("Could not get core animation '%s' for '%s'.",anim , GetName());
        return false;
    }

    float ani_duration = cal3dstate->GetCal3DModel()->getCoreModel()->getCoreAnimation(animation)->getDuration();

    // Check if the duration demands more than 1 playback?
    if (duration > ani_duration)
    {
        // Yes. Set up callback to handle repetition
        int repeat = (int)(duration / ani_duration);

        csRef<iSpriteCal3DFactoryState> sprite =

                                scfQueryInterface<iSpriteCal3DFactoryState> (pcmesh->GetFactory()->GetMeshObjectFactory());


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

    //removes the idle_var animation if it's in execution to avoid interferences.
    cal3dstate->GetCal3DModel()->getMixer()->removeAction(cal3dstate->FindAnim("idle_var"));

    return cal3dstate->SetAnimAction(anim,fadein,fadeout);
}

void GEMClientActor::SetAnimationVelocity(const csVector3& velocity)
{
    if (!alive)  // No zombies please
        return;

    if(!cal3dstate)
        return;

    // Taking larger of the 2 axis; cal3d axis are the opposite of CEL's
    bool useZ = ABS(velocity.z) > ABS(velocity.x);
    float cal3dvel = useZ ? velocity.z : velocity.x;
    cal3dstate->SetVelocity(-cal3dvel, &psengine->GetRandomGen());

    if((velocity.x != 0 || velocity.z != 0) && velocity.Norm() < 2)
    {
        charApp->SetSneak(true);
    }
    else
    {
        charApp->SetSneak(false);
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
            break;

        case psModeMessage::COMBAT:
            // TODO: Get stance and set anim for that stance
            SetIdleAnimation("combat stand");
            break;

        case psModeMessage::DEAD:
            cal3dstate->ClearAllAnims();
            cal3dstate->SetAnimAction("death",0.0f,1.0f);
            GetMovement()->SetVelocity(0);
            GetMovement()->SetAngularVelocity(0);
            if (newactor) // If we're creating a new actor that's already dead, we shouldn't show the animation...
                cal3dstate->SetAnimationTime(9999);  // the very end of the death animation ;)
            break;

        case psModeMessage::SIT:
        case psModeMessage::OVERWEIGHT:
        case psModeMessage::DEFEATED:
            if (!newactor) //we do this only if this isn't a new actor
                cal3dstate->SetAnimAction("sit", 0.0f, 1.0f);
            SetIdleAnimation("sit idle");
            break;
        
        case psModeMessage::STATUE: //used to make statue like character which don't move
            cal3dstate->ClearAllAnims();
            cal3dstate->SetAnimAction("statue",0.0f,1.0f);
            GetMovement()->SetVelocity(0);
            GetMovement()->SetAngularVelocity(0);
            cal3dstate->SetAnimationTime(9999);  // the very end of the animation
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
    if (lastSentVelocity.IsZero())
        cal3dstate->SetVelocity(0);
}

void GEMClientActor::RefreshCal3d()
{
    cal3dstate =  scfQueryInterface<iSpriteCal3DState > ( pcmesh->GetMeshObject());
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
    if (trueName || (Flags() & psPersistActor::NAMEKNOWN) || (eid == psengine->GetCelClient()->GetMainPlayer()->GetEID()))
        return name;
    return strUnknown;
}

bool GEMClientActor::CheckLoadStatus()
{
    csRef<iMeshFactoryWrapper> factory = psengine->GetLoader()->LoadFactory(factName);
    if(!factory.IsValid())
    {
        printf("status\n");
        return true;
    }

    csRef<iMeshWrapper> mesh = factory->CreateMeshWrapper();
    pcmesh = mesh;
    charApp->SetMesh(mesh);
    psengine->GetEngine()->GetMeshes()->Add(pcmesh);

    if(!mountFactname.Compare("null"))
    {
        csRef<iMeshFactoryWrapper> mountFactory = psengine->GetLoader()->LoadFactory(mountFactname);
        if(!mountFactory.IsValid())
        {
            Error2("Couldn't find the mount's mesh factory, %s", mountFactname.GetData());
            return false;
        }
        pcmesh = mountFactory->CreateMeshWrapper();
        psengine->GetEngine()->GetMeshes()->Add(pcmesh);
        charApp->ApplyRider(pcmesh);
        csRef<iSpriteCal3DState> riderstate = scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
        riderstate->SetDefaultIdleAnim("ride");
    }

    pcmesh->GetFlags().Set(CS_ENTITY_NODECAL);

    csRef<iSpriteCal3DState> calstate = scfQueryInterface<iSpriteCal3DState> (pcmesh->GetMeshObject());
    if (calstate)
        calstate->SetUserData((void *)this);

    cel->AttachObject(pcmesh->QueryObject(), this);

    psengine->UnregisterDelayedLoader(this);

    PostLoad(false);

    return true;
}

GEMClientItem::GEMClientItem( psCelClient* cel, psPersistItem& mesg )
               : GEMClientObject(cel, mesg.eid), post_load(new PostLoadData())
{
    name = mesg.name;
    Debug3(LOG_CELPERSIST, 0, "Item %s(%s) Received", mesg.name.GetData(), ShowID(mesg.eid));
    type = mesg.type;
    factName = mesg.factname;
    solid = 0;
    post_load->pos = mesg.pos;
    post_load->xRot = mesg.xRot;
    post_load->yRot = mesg.yRot;
    post_load->zRot = mesg.zRot;
    post_load->sector = mesg.sector;
    post_load->flags = mesg.flags;

    if (!InitMesh())
    {
        Error3("Fatal Error: Could not create item %s(%s)", mesg.name.GetData(), ShowID(mesg.eid));
        return;
    }
}

GEMClientItem::~GEMClientItem()
{
    if(instance)
    {
        // Remove instance.
        instance->pcmesh->RemoveInstance(position);
    }

    delete solid;
}

bool GEMClientItem::CheckLoadStatus()
{
    csRef<iMeshFactoryWrapper> factory;

    // Check if an instance of the mesh already exists.
    instance = cel->FindInstanceObject(factName);
    if(!instance.IsValid())
    {
        bool failed = false;
        factory = psengine->GetLoader()->LoadFactory(factName, &failed);
        if(!factory.IsValid())
        {
            if(failed)
            {
                Error2("Unable to load item with factory %s!\n", factName.GetData());
                psengine->UnregisterDelayedLoader(this);
            }

            return true;
        }

        // Create the mesh.
        instance = csPtr<InstanceObject>(new InstanceObject());
        instance->pcmesh = factory->CreateMeshWrapper();
        instance->pcmesh->GetFlags().Set(CS_ENTITY_NODECAL | CS_ENTITY_NOHITBEAM);
        psengine->GetEngine()->GetMeshes()->Add(instance->pcmesh);
        psengine->GetEngine()->PrecacheMesh(instance->pcmesh);
        cel->AddInstanceObject(factName, instance);

        // Set appropriate shader.
        csRef<iShaderManager> shman = csQueryRegistry<iShaderManager>(psengine->GetObjectRegistry());
        csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet>(
            psengine->GetObjectRegistry(), "crystalspace.shared.stringset");
        csStringID shadertype = strings->Request("base");

        iMaterial* material = factory->GetMeshObjectFactory()->GetMaterialWrapper()->GetMaterial();
        iShader* shader = material->GetShader(shadertype);

        csRef<iStringArray> shaders = psengine->GetLoader()->GetShaderName("default");
        csRef<iStringArray> shadersa = psengine->GetLoader()->GetShaderName("default_alpha");
        if(!shader || shaders->Contains(shader->QueryObject()->GetName()) != csArrayItemNotFound)
        {
            csRef<iStringArray> shaderName = psengine->GetLoader()->GetShaderName("instance");
            shader = shman->GetShader(shaderName->Get(0));
        }
        else if(shadersa->Contains(shader->QueryObject()->GetName()) != csArrayItemNotFound)
        {
            csRef<iStringArray> shaderName = psengine->GetLoader()->GetShaderName("instance_alpha");
            shader = shman->GetShader(shaderName->Get(0));
        }
        else
        {
            Error3("Unhandled shader %s for mesh %s!\n", shader->QueryObject()->GetName(), factName.GetData());
        }

        // Construct a new material using the selected shaders.
        csRef<iTextureWrapper> tex = psengine->GetEngine()->GetTextureList()->CreateTexture(material->GetTexture());
        csRef<iMaterial> mat = psengine->GetEngine()->CreateBaseMaterial(tex);

        mat->SetShader(shadertype, shader);
        shadertype = strings->Request("diffuse");
        mat->SetShader(shadertype, shader);

        // Copy all shadervars over.
        for(size_t i=0; i<material->GetShaderVariables().GetSize(); ++i)
        {
            mat->AddVariable(material->GetShaderVariables().Get(i));
        }

        csRef<iMaterialWrapper> matwrap = psengine->GetEngine()->GetMaterialList()->CreateMaterial(mat, factName + "_instancemat");
        instance->pcmesh->GetMeshObject()->SetMaterialWrapper(matwrap);

        // Set biggest bbox so that instances aren't wrongly culled.
        instance->bbox = factory->GetMeshObjectFactory()->GetObjectModel()->GetObjectBoundingBox();
        factory->GetMeshObjectFactory()->GetObjectModel()->SetObjectBoundingBox(csBox3(-CS_BOUNDINGBOX_MAXVALUE,
            -CS_BOUNDINGBOX_MAXVALUE, -CS_BOUNDINGBOX_MAXVALUE, CS_BOUNDINGBOX_MAXVALUE, CS_BOUNDINGBOX_MAXVALUE,
            CS_BOUNDINGBOX_MAXVALUE));
    }
 
    csVector3 Pos = csVector3(0.0f);
    csMatrix3 Rot = csMatrix3();
    position = instance->pcmesh->AddInstance(Pos, Rot);

    // Init nullmesh factory.
    factory = psengine->GetEngine()->CreateMeshFactory("crystalspace.mesh.object.null", factName + "_nullmesh");
    csRef<iNullFactoryState> nullstate = scfQueryInterface<iNullFactoryState> (factory->GetMeshObjectFactory());
    nullstate->SetBoundingBox(instance->bbox);

    pcmesh = factory->CreateMeshWrapper();
    pcmesh->GetFlags().Set(CS_ENTITY_NODECAL);
    csRef<iNullMeshState> nullmeshstate = scfQueryInterface<iNullMeshState> (pcmesh->GetMeshObject());
    nullmeshstate->SetHitBeamMeshObject(instance->pcmesh->GetMeshObject());
    psengine->GetEngine()->GetMeshes()->Add(pcmesh);

    cel->AttachObject(pcmesh->QueryObject(), this);

    psengine->UnregisterDelayedLoader(this);

    PostLoad(false);

    // Handle item effect if there is one.
    cel->HandleItemEffect(factName, pcmesh);

    return true;
}

void GEMClientItem::PostLoad(bool nullmesh)
{
    Move(post_load->pos, post_load->yRot, post_load->sector);
    
    Rotate(post_load->xRot, post_load->yRot, post_load->zRot);

    if (flags & psPersistItem::COLLIDE)
    {
        solid = new psSolid(psengine->GetObjectRegistry());
        solid->SetMesh(pcmesh);
        solid->Setup();
    }

    cel->GetEntityLabels()->OnObjectArrived(this);
    cel->GetShadowManager()->CreateShadow(this);

    delete post_load;
    post_load = NULL;
}

void GEMClientItem::UpdateItem( psPersistItem& mesg )
{    
    name = mesg.name;
    Debug3(LOG_CELPERSIST, 0, "Item %s(%s) Updated", mesg.name.GetData(), ShowID(mesg.eid));
    type = mesg.type;
    factName = mesg.factname;

    post_load = new PostLoadData();
    post_load->pos = mesg.pos;
    post_load->xRot = mesg.xRot;
    post_load->yRot = mesg.yRot;
    post_load->zRot = mesg.zRot;
    post_load->sector = mesg.sector;
    post_load->flags = mesg.flags;

    if(pcmesh.IsValid())
    {
      PostLoad(false);
    }
}

GEMClientActionLocation::GEMClientActionLocation( psCelClient* cel, psPersistActionLocation& mesg )
               : GEMClientObject(cel, mesg.eid)
{
    name = mesg.name;

    Debug3(LOG_CELPERSIST, 0, "Action location %s(%s) Received", mesg.name.GetData(), ShowID(mesg.eid));
    type = mesg.type;
    meshname = mesg.mesh;

    csRef< iEngine > engine = psengine->GetEngine();
    pcmesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", "ActionLocation");
    if ( !pcmesh )
    {
        Error1("Could not create GEMClientActionLocation because crystalspace.mesh.onbject.null could not be created.");
        return ;
    }

    csRef<iNullMeshState> state =  scfQueryInterface<iNullMeshState> (pcmesh->GetMeshObject());
    if (!state)
    {
        Error1("No NullMeshState.");
        return ;
    }
    state->SetRadius(1.0);

    Move( csVector3(0,0,0), 0.0f, mesg.sector);
}
