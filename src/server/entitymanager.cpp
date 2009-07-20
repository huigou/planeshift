/*
* EntityManager.cpp
*
* Copyright (C) 2002-2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <ivideo/txtmgr.h>
#include <ivideo/texture.h>
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/object.h>
#include <csgeom/box.h>
#include <imesh/objmodel.h>
#include <csgeom/transfrm.h>
#include <csutil/csstring.h>
#include <csutil/snprintf.h>


//=============================================================================
// Project Includes
//=============================================================================
#include "net/message.h"
#include "net/msghandler.h"

#include "util/pserror.h"
#include "util/psdatabase.h"
#include "util/psstring.h"
#include "util/strutil.h"
#include "util/psconst.h"
#include "util/eventmanager.h"
#include "util/mathscript.h"
#include "util/psxmlparser.h"
#include "util/serverconsole.h"

#include "engine/psworld.h"
#include "engine/linmove.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pssectorinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "iserver/idal.h"
#include "client.h"
#include "clients.h"
#include "psproxlist.h"
#include "gem.h"
#include "entitymanager.h"
#include "usermanager.h"
#include "psserverdr.h"
#include "psserver.h"
#include "psserverchar.h"
#include "weathermanager.h"
#include "npcmanager.h"
#include "progressionmanager.h"
#include "cachemanager.h"
#include "playergroup.h"
#include "chatmanager.h"// included for say_range
#include "globals.h"

EntityManager::EntityManager()
{
    serverdr = NULL;
    ready = hasBeenReady = false;
    gameWorld = 0;
    moveinfomsg = NULL;
}

EntityManager::~EntityManager()
{
    delete serverdr;

    if (psserver->GetEventManager())
    {
        psserver->GetEventManager()->Unsubscribe(this, MSGTYPE_CELPERSIST);
        psserver->GetEventManager()->Unsubscribe(this, MSGTYPE_USERACTION);
        psserver->GetEventManager()->Unsubscribe(this, MSGTYPE_PERSIST_WORLD_REQUEST);
        psserver->GetEventManager()->Unsubscribe(this, MSGTYPE_PERSIST_ACTOR_REQUEST);
        psserver->GetEventManager()->Unsubscribe(this, MSGTYPE_PERSIST_ALL);
        psserver->GetEventManager()->Unsubscribe(this, MSGTYPE_REQUESTMOVEMENTS);
    }

    {
        csHash<psAffinityAttribute *>::GlobalIterator it(affinityAttributeList.GetIterator ());
        while (it.HasNext ())
        {
            psAffinityAttribute* affAtt = it.Next ();
            delete affAtt;
        }
    }
    {
      csHash<psFamiliarType *, PID>::GlobalIterator it(familiarTypeList.GetIterator ());
      while (it.HasNext ())
        {
        psFamiliarType* familiarType = it.Next ();
        delete familiarType;
        }
    }
    
    delete gameWorld;
    delete gem;
    delete moveinfomsg;
    //int sectorCount = engine->GetSectors()->GetCount();
}

bool EntityManager::Initialize(iObjectRegistry* object_reg, 
                               ClientConnectionSet* clients,
                               UserManager* umanager)
{
    csRef<iEngine> engine2 = csQueryRegistry<iEngine> (psserver->GetObjectReg());
    engine = engine2; // get out of csRef;

    usermanager = umanager;

    psserver->GetEventManager()->Subscribe(this, new NetMessageCallback<EntityManager>(this,&EntityManager::HandleUserAction), MSGTYPE_USERACTION,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);
    psserver->GetEventManager()->Subscribe(this, new NetMessageCallback<EntityManager>(this,&EntityManager::HandleWorld)     , MSGTYPE_PERSIST_WORLD_REQUEST,REQUIRE_ANY_CLIENT );
    psserver->GetEventManager()->Subscribe(this, new NetMessageCallback<EntityManager>(this,&EntityManager::HandleActor)     , MSGTYPE_PERSIST_ACTOR_REQUEST,REQUIRE_ANY_CLIENT );    
    psserver->GetEventManager()->Subscribe(this, new NetMessageCallback<EntityManager>(this,&EntityManager::HandleAllRequest), MSGTYPE_PERSIST_ALL,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this, new NetMessageCallback<EntityManager>(this,&EntityManager::SendMovementInfo), MSGTYPE_REQUESTMOVEMENTS,REQUIRE_ANY_CLIENT);

    EntityManager::clients = clients;

    gem = new GEMSupervisor(object_reg,psserver->GetDatabase());

    serverdr = new psServerDR;
    if (!serverdr->Initialize())
    {
        delete serverdr;
        serverdr = NULL;
        return false;
    }

    LoadFamiliarAffinityAttributes();
    LoadFamiliarTypes();

    gameWorld = new psWorld();        
    gameWorld->Initialize( object_reg );

    return true;
}

void EntityManager::LoadFamiliarAffinityAttributes()
{
    csString sql = "SELECT * FROM char_create_affinity";
    
    Result result( db->Select( sql ) );

    if (!result.IsValid())
        return;

    for ( unsigned long row = 0; row < result.Count(); row++)
    {
        psAffinityAttribute* newAttribute = new psAffinityAttribute();
        newAttribute->Attribute = csString( result[row]["attribute"] ).Downcase();
        newAttribute->Category  = csString( result[row]["category"]  ).Downcase();
        affinityAttributeList.Put( csHashCompute( newAttribute->Attribute + newAttribute->Category ), newAttribute );
    }
}

void EntityManager::LoadFamiliarTypes()
{
    csString sql = "SELECT * FROM familiar_types";

    Result result( db->Select( sql ) );

    if (!result.IsValid())
        return;

    for ( unsigned long row = 0; row < result.Count(); row++)
    {
        psFamiliarType* newFamiliar = new psFamiliarType();
        newFamiliar->Id = result[row].GetUInt32( "ID" );
        newFamiliar->Name  = csString( result[row]["name"] ).Downcase();
        newFamiliar->Type  = csString( result[row]["type"] ).Downcase();
        newFamiliar->Lifecycle  = csString( result[row]["lifecycle"] ).Downcase();
        newFamiliar->AttackTool  = csString( result[row]["attacktool"] ).Downcase();
        newFamiliar->AttackType  = csString( result[row]["attacktype"] ).Downcase();
        newFamiliar->MagicalAffinity  = csString( result[row]["magicalaffinity"] ).Downcase();
        familiarTypeList.Put( newFamiliar->Id, newFamiliar );
    }
}


iSector* EntityManager::FindSector(const char * sectorname)
{
    return engine->GetSectors()->FindByName(sectorname);
}

gemNPC* EntityManager::CreateFamiliar (gemActor *owner)
{
    psCharacter *chardata = owner->GetCharacterData();

    PID familiarID, masterFamiliarID;
    csString familiarname;
    csVector3 pos;
    float yrot = 0.0F;
    iSector *sector;

    // FIXME: This probably ought to be an assertion.
    if ( chardata == NULL )
    {
        CPrintf(CON_ERROR, "Couldn't load character for familiar %s.\n", ShowID(owner->GetPID()));
        return NULL;
    }
    
    masterFamiliarID = GetMasterFamiliarID( chardata );

    // Change Familiar's Name
    const char *charname = chardata->GetCharName();
    if ( charname[strlen(charname)-1] == 's' )
    {
        familiarname.Format("%s'", charname);
    }
    else
    {
        familiarname.Format("%s's", charname);
    }

    // Adjust Position of Familiar from owners pos
    owner->GetPosition( pos, yrot, sector );
    InstanceID instance = owner->GetInstance();

    familiarID = this->CopyNPCFromDatabase( masterFamiliarID, pos.x + 1.5, pos.y, pos.z + 1.5, yrot, sector->QueryObject()->GetName(), instance, familiarname, "Familiar" );
    if ( familiarID == 0 )
    {
        CPrintf(CON_ERROR, "CreateFamiliar failed for %s: Could not copy the master NPC %s\n", ShowID(owner->GetPID()), ShowID(masterFamiliarID));
        psserver->SendSystemError( owner->GetClientID(), "Could not copy the master NPC familiar." );
        return NULL;
    }

    // Prepare NPC client to the new npc
    psserver->npcmanager->NewNPCNotify(familiarID, masterFamiliarID, owner->GetPID() );

    // Create Familiar using new ID
    this->CreateNPC( familiarID , false); //Do not update proxList, we will do that later.

    gemNPC * npc = GEMSupervisor::GetSingleton().FindNPCEntity( familiarID );
    if (npc == NULL)
    {
        psserver->SendSystemError( owner->GetClientID(), "Could not find GEM and set its location.");
        return NULL;
    }

    npc->GetCharacterData()->NPC_SetSpawnRuleID( 0 );
    npc->SetOwner( owner );

    owner->GetClient()->SetFamiliar( npc );
    owner->GetCharacterData()->SetFamiliarID( familiarID );

    db->Command("INSERT INTO npc_knowledge_areas(player_id, area, priority) VALUES (%d, 'Pet %s 1', '1')", familiarID.Unbox(), npc->GetCharacterData()->GetRaceInfo()->name.GetData());

    psServer::CharacterLoader.SaveCharacterData( npc->GetCharacterData(), npc, false );

    psserver->npcmanager->ControlNPC( npc );
    psserver->npcmanager->CreatePetOwnerSession( owner, npc->GetCharacterData() );

    // For now familiars cannot be attacked as they are defenseless
    npc->GetCharacterData()->SetImperviousToAttack( ALWAYS_IMPERVIOUS );

    // Add npc to all nearby clients
    npc->UpdateProxList( true );

    return npc;
}

gemNPC* EntityManager::CloneNPC ( psCharacter *chardata )
{
    csVector3 pos;
    float yrot = 0.0F;
    iSector *sector;

    CS_ASSERT( chardata != NULL );
    
    // Adjust Position of npc from owners pos
    chardata->GetActor()->GetPosition( pos, yrot, sector );

    float deltax = psserver->GetRandom(6)/4 - 1.5;
    float deltaz = psserver->GetRandom(6)/4 - 1.5;
    

    PID npcPID(this->CopyNPCFromDatabase( chardata->GetPID(),
                                        pos.x + deltax, pos.y, pos.z + deltaz,  // Set position some distance from parent
                                        yrot, sector->QueryObject()->GetName(),
                                        0,NULL, NULL)); // Keep name of parent
    if (!npcPID.IsValid())
    {
        Error1( "Could not clone the master NPC .");
        return NULL;
    }

    // Prepare NPC client to the new npc
    psserver->npcmanager->NewNPCNotify(npcPID, chardata->GetPID(), 0 );

    // Create npc using new ID
    this->CreateNPC( npcPID , false); //Do not update proxList, we will do that later.

    gemNPC * npc = GEMSupervisor::GetSingleton().FindNPCEntity( npcPID );
    if (npc == NULL)
    {
        Error1("Could not find GEM and set its location.");
        return NULL;
    }

    db->Command("INSERT INTO npc_knowledge_areas(player_id, area, priority) VALUES (%d, 'Pet %s 1', '1')", 
                npcPID.Unbox(), npc->GetCharacterData()->GetRaceInfo()->name.GetData() );

    psServer::CharacterLoader.SaveCharacterData( npc->GetCharacterData(), npc, false );

    psserver->npcmanager->ControlNPC( npc );

    // Add npc to all nearby clients
    npc->UpdateProxList( true );

    return npc;
}

PID EntityManager::GetMasterFamiliarID(psCharacter *charData)
{
    csHash< csHash< size_t, csString > *, csString  > charAttributes;
    csHash< size_t, csString > *charAttributeList;
    size_t typeValue = 0, lifecycleValue = 0, attacktoolValue = 0, attacktypeValue = 0;
    int rank = 0;
    int currentRank = -1;
    psFamiliarType *currentFT = NULL;
    csHash<psFamiliarType*, PID>::GlobalIterator ftIter = familiarTypeList.GetIterator();
    psFamiliarType *ft = NULL;
    // Get Familiar Affinity 
    csString animalAffinity = charData->GetAnimalAffinity();

    if ( animalAffinity.Length() == 0 ) 
    {
        // No animal affinity data was found
        // just use a random number to grab the familiar type id
        size_t maxValue = familiarTypeList.GetSize();
        size_t pick = psserver->rng->Get( (uint32)maxValue ) + 1;

        size_t count = 0;
        while ( count++ < pick && ftIter.HasNext() )
        {
            ft = (psFamiliarType *)ftIter.Next();
        }
        return ft->Id;
    }

    // Parse the string into an XML document.
    csRef<iDocument> xmlDoc = ParseString( animalAffinity );

    if(!xmlDoc.IsValid())
    {
      csString msg("Error parsing animal affinity for character ");
      msg.AppendFmt("%s!\n", charData->fullname.GetData());
      CS_ASSERT_MSG(msg.GetData(), xmlDoc.IsValid());
    }
	
    // Find existing nodes
    csRef<iDocumentNodeIterator> iter = xmlDoc->GetRoot()->GetNodes();
    csRef<iDocumentNode> node;
    
    while ( iter->HasNext() )
    {
        node = iter->Next();
        
        csString attribute = csString( node->GetAttributeValue( "attribute" ) ).Downcase();
        csString name      = csString( node->GetAttributeValue( "name" ) ).Downcase();
        size_t value       = (size_t)node->GetAttributeValueAsInt( "value" );
        
        if ( attribute.Length() != 0 && name.Length() != 0 )
        {
            
            charAttributeList = charAttributes.Get( attribute, NULL );
            if ( charAttributeList == NULL )
            {
                charAttributeList = new csHash< size_t, csString >();
                charAttributes.Put( attribute, charAttributeList ); 
            }
            charAttributeList->Put( name, value );
        }
    }
    
    // For each entry in familiar_definitions
    while ( ftIter.HasNext() )
    {
        typeValue = lifecycleValue = attacktoolValue = attacktypeValue = 0;

        // Calculate affinity for all matching attribute categories
        psFamiliarType *ft = (psFamiliarType *)ftIter.Next();

        csHash< size_t, csString > *attribute;
    
        attribute = charAttributes.Get( "type", NULL);
        if ( attribute != NULL )
        {
            typeValue = attribute->Get( ft->Type,0 );
        }

        attribute = charAttributes.Get( "lifecycle", NULL );
        if ( attribute != NULL )
        {
            lifecycleValue = attribute->Get( ft->Lifecycle, 0 );
        }

        attribute = charAttributes.Get( "attacktool", NULL );
        if ( attribute != NULL )
        {
            attacktoolValue = attribute->Get( ft->AttackTool, 0 );
        }

        attribute = charAttributes.Get( "attacktype", NULL );
        if ( attribute != NULL )
        {
            attacktypeValue = attribute->Get( ft->AttackType, 0 );
        }

        rank = CalculateFamiliarAffinity( charData, typeValue, lifecycleValue, attacktoolValue, attacktypeValue );
        if ( rank > currentRank )
        {
            currentRank = rank;
            currentFT = ft;
        }
    }
    // highest value affinity is used as masterId

    charAttributes.DeleteAll();
    if ( currentFT )
       return currentFT->Id;
    else
        return 0;
}

int EntityManager::CalculateFamiliarAffinity( psCharacter * chardata, size_t type, size_t lifecycle, size_t attacktool, size_t attacktype )
{
    static MathScript *msAffinity;
    int affinityValue = 0;

    if (!msAffinity)
    {
        msAffinity = psserver->GetMathScriptEngine()->FindScript("CalculateFamiliarAffinity");
        CS_ASSERT(msAffinity != NULL);
    }

    // Determine Familiar Type using Affinity Values
    if ( msAffinity )
    {
        MathEnvironment env;
        env.Define("Actor",      chardata);
        env.Define("Type",       type);
        env.Define("Lifecycle",  lifecycle);
        env.Define("AttackTool", attacktool);
        env.Define("AttackType", attacktype);
        msAffinity->Evaluate(&env);
        MathVar *affinity = env.Lookup("Affinity");
        affinityValue = (int) affinity->GetValue();
    }

    return affinityValue;

}

gemNPC* EntityManager::CreatePet (Client *client, int masterFamiliarID)
{
    // FIXME: Currently, the code below has an EID/PID mismatch which would
    //        cause it to fail.  I don't think that pets are currently used
    //        anyway - only familiars.  It would be wise to look into sharing
    //        code between these two very similar concepts.
    return NULL;
#if 0
    psCharacter *charData = client->GetCharacterData();

    csString familiarname;

    // Change Character's Name
    const char *charname = charData->GetCharName();
    if ( charname[strlen(charname)-1] == 's' )
    {
        familiarname.Format("%s'", charname);
    }
    else
    {
        familiarname.Format("%s's", charname);
    }

    psCharacter *petData = psServer::CharacterLoader.LoadCharacterData( masterFamiliarID,true );
    petData->SetFullName( familiarname, "Pet" );

    // Prepare NPC client to the new npc
    psserver->npcmanager->NewNPCNotify(petData->GetPID(), masterFamiliarID, client->GetPID() );

    EID familiarID = this->CreateNPC(petData, false); // Do not update proxList, we will do that later.

    gemNPC * npc = GEMSupervisor::GetSingleton().FindNPCEntity( familiarID );
    if (npc == NULL)
    {
        psserver->SendSystemError( client->GetClientNum(), "Could not find GEM and set its location.");
        return NULL;
    }

    this->Teleport( npc, client->GetActor() );

    npc->GetCharacterData()->NPC_SetSpawnRuleID( 0 );
    npc->SetOwner( client->GetActor() );

    client->SetFamiliar( npc );
    client->GetCharacterData()->SetFamiliarID( familiarID );

    db->Command("INSERT INTO npc_knowledge_areas(player_id, area, priority) VALUES (%d, 'Pet %s 1', '1')", familiarID, npc->GetCharacterData()->GetRaceInfo()->name.GetData() );

    psserver->npcmanager->ControlNPC( npc );
    psserver->npcmanager->CreatePetOwnerSession( client->GetActor(), npc->GetCharacterData() );

    // For now familiars cannot be attacked as they are defenseless
    npc->GetCharacterData()->SetImperviousToAttack( ALWAYS_IMPERVIOUS );

    npc->UpdateProxList( true );

    return npc;
#endif
}

bool EntityManager::CreatePlayer (Client* client)
{
    psCharacter *chardata=psServer::CharacterLoader.LoadCharacterData(client->GetPID(),true);
    if (chardata==NULL)
    {
        CPrintf(CON_ERROR, "Couldn't load character for %s!\n", ShowID(client->GetPID()));
        psserver->RemovePlayer (client->GetClientNum(),"Your character data could not be loaded from the database.  Please contact tech support about this.");
        return false;
    }

    // FIXME: This should really be an assert in LoadCharacterData or such
    psRaceInfo *raceinfo=chardata->GetRaceInfo();
    if (raceinfo==NULL)
    {
        CPrintf(CON_ERROR, "Character load returned with NULL raceinfo pointer for %s!\n", ShowID(client->GetPID()));
        psserver->RemovePlayer (client->GetClientNum(),"Your character race could not be loaded from the database.  Please contact tech support about this.");
        delete chardata;
        return false;
    }

    csVector3 pos;
    float yrot;
    psSectorInfo *sectorinfo;
    iSector *sector;
    InstanceID instance;
    chardata->GetLocationInWorld(instance,sectorinfo,pos.x,pos.y,pos.z,yrot);
    sector=FindSector(sectorinfo->name);
    if (sector==NULL)
    {
        Error3("Could not resolve sector >%s< for %s.", sectorinfo->name.GetData(), ShowID(client->GetPID()));
        psserver->RemovePlayer (client->GetClientNum(),"The server could not create your character entity. (Sector not found)  Please contact tech support about this.");
        delete chardata;
        return false;
    }

    gemActor *actor = new gemActor(chardata,raceinfo->mesh_name,
                                   instance,sector,pos,yrot,
                                   client->GetClientNum());

    client->SetActor(actor);

    if (!actor || !actor->IsValid() )
    {
        Error2("Error while creating gemActor for Character '%s'\n", chardata->GetCharName());
        psserver->RemovePlayer (client->GetClientNum(),"The server could not create your character entity. (new gemActor() failed)  Please contact tech support about this.");
        return false;
    }
  
    chardata->LoadActiveSpells();

    // Check for buddy list members
    usermanager->NotifyBuddies(client, UserManager::LOGGED_ON);
    
    // Check for Guild members to notify
    usermanager->NotifyGuildBuddies(client, UserManager::LOGGED_ON);

    // Set default state
    actor->SetMode(PSCHARACTER_MODE_PEACE);

    // Add Player to all Super Clients
    psserver->npcmanager->AddEntity(actor);

    psSaveCharEvent *saver = new psSaveCharEvent(actor);
    saver->QueueEvent();    
    
    return true;
}

bool EntityManager::DeletePlayer(Client * client)
{
    gemActor *actor = client->GetActor();
    if (actor && actor->GetCharacterData()!=NULL)
    {
        // take the actor off his mount if he got one
        if(actor->GetMount())
            RemoveRideRelation(actor, actor->GetMount());
        
        // Check for buddy list members
        usermanager->NotifyBuddies(client, UserManager::LOGGED_OFF);
        
        // Check for Guild members to notify
        usermanager->NotifyGuildBuddies(client, UserManager::LOGGED_OFF);

        // Any objects wanting to know when the actor is 'gone' are callback'd here.
        actor->Disconnect();

        if (!dynamic_cast<gemNPC*> (actor))  // NPC cast null means a human player
        {
            // Save current character state in the database
            psServer::CharacterLoader.SaveCharacterData(actor->GetCharacterData(),actor);
        }
         
        gemActor *familiar = client->GetFamiliar();
        if ( familiar != NULL && familiar->IsValid())
        {
            // Send OwnerActionLogoff Perception

            //familiar->Disconnect();
            Debug3(LOG_NET,client->GetClientNum(),"EntityManager Removing actor %s from client %s.\n",familiar->GetName(),client->GetName() );
            psServer::CharacterLoader.SaveCharacterData(familiar->GetCharacterData(),familiar);
            client->SetFamiliar( NULL );
            RemoveActor( familiar );
        }

        // This removes the actor from the world data
        Debug3(LOG_NET,client->GetClientNum(),"EntityManager Removing actor %s from client %s.\n",actor->GetName(),client->GetName() );
        gem->RemoveClientFromLootables(client->GetClientNum());
        client->SetActor(NULL); // Prevent anyone from getting to a deleted actor through the client
        RemoveActor(actor);
    }
    return true;
}

PID EntityManager::CopyNPCFromDatabase(PID master_id, float x, float y, float z, float angle, const csString & sector, InstanceID instance, const char *firstName, const char *lastName)
{
    psCharacter * npc = NULL;
    PID new_id;

    npc = psServer::CharacterLoader.LoadCharacterData(master_id, false);
    if (npc == NULL)
    {
        return 0;
    }

    psSectorInfo* sectorInfo = CacheManager::GetSingleton().GetSectorInfoByName( sector );
    if (sectorInfo != NULL)
    {
        npc->SetLocationInWorld(instance,sectorInfo,x,y,z,angle);
    }

    if ( firstName && lastName )
    {
        npc->SetFullName( firstName, lastName );
    }

    if (psServer::CharacterLoader.NewNPCCharacterData(0, npc))
    {
        new_id = npc->GetPID();
        db->Command("UPDATE characters SET npc_master_id=%d WHERE id=%d", master_id.Unbox(), new_id.Unbox());
    }

    delete npc;

    return new_id;
}

EID EntityManager::CreateNPC(psCharacter *chardata, bool updateProxList, bool alwaysWatching)
{
    csVector3 pos;
    float yrot;
    psSectorInfo *sectorinfo;
    iSector *sector;
    InstanceID instance;

    chardata->GetLocationInWorld(instance, sectorinfo,pos.x,pos.y,pos.z,yrot);
    sector = FindSector(sectorinfo->name);

    if (sector == NULL)
    {
        Error3("Could not resolve sector >%s< for NPC %s.", sectorinfo->name.GetData(), ShowID(chardata->GetPID()));
        delete chardata;
        return false;
    }

    return CreateNPC(chardata, instance, pos, sector, yrot, updateProxList, alwaysWatching);
}

EID EntityManager::CreateNPC(psCharacter *chardata, InstanceID instance, csVector3 pos, iSector* sector, float yrot,
                             bool updateProxList, bool alwaysWatching)
{
    if (chardata==NULL)
        return false;

    // FIXME: This should be an assert elsewhere.
    psRaceInfo *raceinfo=chardata->GetRaceInfo();
    if (raceinfo==NULL)
    {
        CPrintf(CON_ERROR, "NPC ID %u: Character Load returned with NULL raceinfo pointer!\n", ShowID(chardata->GetPID()));
        delete chardata;
        return false;
    }

    gemNPC *actor = new gemNPC(chardata, raceinfo->mesh_name, instance, sector, pos, yrot, 0);
    actor->SetAlwaysWatching(alwaysWatching);

    if ( !actor->IsValid() )
    {
        CPrintf(CON_ERROR, "Error while creating Entity for NPC '%s'\n", ShowID(chardata->GetPID()));
        delete actor;
        delete chardata;
        return false;
    }

    // This is required to identify all managed npcs.
    actor->SetSuperclientID( chardata->GetAccount() );
    
    // Add NPC Dialog plugin if any knowledge areas are defined in db for him.
    actor->SetupDialog(chardata->GetPID(), true);

    // Setup prox list and send to anyone who needs him
    if ( updateProxList )
    {
        actor->UpdateProxList( true );
    
//        CPrintf(CON_NOTIFY,"------> Entity Manager Setting Imperv\n");
        psserver->npcmanager->ControlNPC( actor );
    }
    Debug3(LOG_NPC, 0, "Created NPC actor: <%s>[%s] in world\n", actor->GetName(), ShowID(actor->GetEID()));

    return actor->GetEID();
}


EID EntityManager::CreateNPC(PID npcID, bool updateProxList, bool alwaysWatching)
{
    psCharacter *chardata=psServer::CharacterLoader.LoadCharacterData(npcID,false);
    if (chardata==NULL)
    {
        CPrintf(CON_ERROR, "Couldn't load character for NPC %s.", ShowID(npcID));
        return 0;
    }

    return CreateNPC(chardata, updateProxList, alwaysWatching);
}


bool EntityManager::LoadMap (const char* mapname)
{
    if (!CreateRoom("world", mapname))
        return false;

    return true;
}

gemObject *EntityManager::MoveItemToWorld(psItem       *chrItem,
                                          InstanceID   instance,
                                          psSectorInfo *sectorinfo,
                                          float         loc_x,
                                          float         loc_y,
                                          float         loc_z,
                                          float         loc_yrot,
                                          psCharacter  *owner,
                                          bool          transient)
{
    chrItem->SetLocationInWorld(instance,sectorinfo,loc_x,loc_y,loc_z,loc_yrot);
    chrItem->UpdateInventoryStatus(NULL,0,PSCHARACTER_SLOT_NONE);

    gemObject *obj = CreateItem(chrItem,true);
    if (!obj)
    {
        return NULL;
    }
    chrItem->Save(false);
    return obj;
}


gemObject *EntityManager::CreateItem( psItem *& iteminstance, bool transient )
{
    psSectorInfo *sectorinfo;
    csVector3 newpos;
    float xrot;
    float yrot;
    float zrot;
    iSector *isec;
    InstanceID instance;

    iteminstance->GetLocationInWorld(instance, &sectorinfo,newpos.x,newpos.y,newpos.z,yrot);
    iteminstance->GetXZRotationInWorld(xrot,zrot);
    if (sectorinfo==NULL)
        return NULL;
    isec = FindSector(sectorinfo->name);
    if (isec==NULL)
        return NULL;

    // Try to stack this first
    csArray<gemObject*> nearlist = gem->FindNearbyEntities( isec, newpos, RANGE_TO_STACK );
    size_t count = nearlist.GetSize();
    for (size_t i=0; i<count; i++)
    {
        gemObject *nearobj = nearlist[i];
        if (!nearobj)
            continue;

        psItem *nearitem = nearobj->GetItem();
        if ( nearitem && nearitem->CheckStackableWith(iteminstance, false) )
        {
            // Put new item(s) into old stack
            nearitem->CombineStack(iteminstance);
            nearitem->Save(false);
            return nearitem->GetGemObject(); // Done
        }
    }

    // Cannot stack, so make a new one
    // Get the mesh for this object
    const char *meshname = iteminstance->GetMeshName();
    gemItem *obj;
    if (iteminstance->GetIsContainer())
    {
        obj = new gemContainer(iteminstance,meshname,instance,isec,newpos,xrot,yrot,zrot,0);
    }
    else
    {
        obj = new gemItem(iteminstance,meshname,instance,isec,newpos,xrot,yrot,zrot,0);
    }

    // Won't create item if gemItem entity was not created
    //CS_ASSERT(obj->GetEntity() != NULL);
    
    if (transient && !sectorinfo->GetIsNonTransient())
    {
        // don't create removal events for items in e.g guildhalls
        iteminstance->ScheduleRemoval();
    }
        
    obj->Move(newpos,yrot,isec);

    // Add object to all nearby clients
    obj->UpdateProxList( true );

    // Add object to all Super Clients
    psserver->npcmanager->AddEntity(obj);

    return obj;
}

bool EntityManager::CreateActionLocation(psActionLocation *al, bool transient = false)
{
    const csVector3 pos = al->GetPosition();
    iSector *sector = FindSector(al->GetSectorName());
    if (!sector)
    {
        CPrintf(CON_ERROR, "Action Location ID %u : Sector not found!\n", al->id);
        return false;
    }

    gemActionLocation *obj = new gemActionLocation(al, sector, 0);
    
    // Add action location to all nearby clients
    obj->UpdateProxList( true );

    // Add action location to all Super Clients
    psserver->npcmanager->AddEntity(obj);

    //Debug3(LOG_STARTUP ,0, "Action Location ID %u : Created successfully(EID: %u)!\n", instance->id,obj->GetEID());
    return true;
}

bool EntityManager::CreateRoom(const char* name, const char* mapfile)
{
    static bool first = true;
    
    if (first)
    {       
        gameWorld->CreateMap( name, mapfile,psWorld::LOAD_NOW, false );        
        first = false;
    }
    else
    {        
        gameWorld->NewRegion(mapfile,psWorld::LOAD_NOW, false);
    }
    return true;
}


void EntityManager::HandleAllRequest(MsgEntry* me, Client *client)
{
    // This is not available to regular clients!
    if ( client->IsSuperClient() )
    {
        csHash<gemObject*, EID>& gems = gem->GetAllGEMS();
        csHash<gemObject*, EID>::GlobalIterator i(gems.GetIterator());
        gemObject* obj;
    
        while ( i.HasNext() )
        {
            obj = i.Next();       
            // Send to superclient given by clientnum
            obj->Send( me->clientnum, false,  true );
        }

        psserver->npcmanager->SendNPCList(client);
    }
    else
    {
        Debug2(LOG_CHEAT, client->GetClientNum(),"Player %s trying to cheat by requesting all objects.", client->GetName());
    }
}


void EntityManager::HandleActor(MsgEntry* me, Client *client)
{
    if(!client || client->IsSuperClient())
        return;

    gemActor* actor = client->GetActor();
    
    if (!client->IsSuperClient() && !actor)
    {
        CPrintf (CON_WARNING, "*** Client has no entity assigned yet!\n");
        return;
    }
    
    // First send the actor to the client
    actor->Send( me->clientnum, true, false );

    // Then send stuff like HP and mana to player, flags=-1 force a update of all stats
    psCharacter * chardata = client->GetCharacterData();
    chardata->SendStatDRMessage(me->clientnum, actor->GetEID(), -1);

    //Store info about the character login
    chardata->SetLastLoginTime();
}

void EntityManager::SendMovementInfo(MsgEntry* me, Client *client)
{
    if (moveinfomsg == NULL)  // Construct once and reuse
    {
        CacheManager* mgr = CacheManager::GetSingletonPtr();
        const csPDelArray<psCharMode>& modes = mgr->GetCharModes();
        const csPDelArray<psMovement>& moves = mgr->GetMovements();

        moveinfomsg = new psMovementInfoMessage(modes.GetSize(), moves.GetSize());

        for (size_t i=0; i<modes.GetSize(); i++)
        {
            const psCharMode* mode = modes[i];
            moveinfomsg->AddMode(mode->id, mode->name, mode->move_mod, mode->rotate_mod, mode->idle_animation);
        }
    
        for (size_t i=0; i<moves.GetSize(); i++)
        {
            const psMovement* move = moves[i];
            moveinfomsg->AddMove(move->id, move->name, move->base_move, move->base_rotate);
        }

        moveinfomsg->msg->ClipToCurrentSize();

        CS_ASSERT( moveinfomsg->valid );
    }

    moveinfomsg->msg->clientnum = client->GetClientNum();
    moveinfomsg->SendMessage();

    // Send modifiers too
//    if(client->GetActor())
//        client->GetActor()->UpdateAllSpeedModifiers();
}

void EntityManager::HandleWorld( MsgEntry* me, Client *client )
{
    if (!client->GetActor() && !CreatePlayer (client))
    {
        Error1("Error while creating player in world!");
        return;
    }

    // Client needs to know the starting sector of the player when the world loads
    csVector3 pos;
    float     yrot;
    iSector *isector;
    client->GetActor()->GetPosition(pos,yrot,isector);
  
    psPersistWorld mesg( me->clientnum, isector->QueryObject()->GetName() );
    psserver->GetEventManager()->SendMessage( mesg.msg );    

    // Send the world time and weather here too
    psserver->GetWeatherManager()->UpdateClient(client->GetClientNum());
}

void EntityManager::HandleUserAction(MsgEntry* me, Client *client)
{
    psUserActionMessage actionMsg(me);
    csString action;

    if (!actionMsg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Received unparsable psUserActionMessage from client id %u.\n",me->clientnum);
        return;
    }

    gemObject *object = gem->FindObject(actionMsg.target);

    client->SetTargetObject(object);  // have special tracking for this for fast processing of other messages

    if (actionMsg.target.IsValid() && !object)
    {
        Debug2(LOG_ANY, me->clientnum, "User action on unknown entity (%s)!\n", ShowID(actionMsg.target));
        return;
    }

    if (!object)
    {
        // TODO: Evaluate if this output is needed. 
        Debug2(LOG_ANY, me->clientnum, "User action on none or unknown object (%s)!\n", ShowID(actionMsg.target));
        return;
    }

    // Resolve default behaviour
    action = actionMsg.action;
    if (action == "dfltBehavior")
    {
        action = object->GetDefaultBehavior(actionMsg.dfltBehaviors);
        if (action.IsEmpty())
        {
            return;
        }
    }

    if (action == "select" || action == "context")
    {
        if (object != NULL)
            object->SendTargetStatDR(client);
    }

    Debug4(LOG_USER,client->GetClientNum(), "User Action: %s %s %s\n",client->GetName(),
        (const char *)action,
        (object)?object->GetName():"None")

    object->SendBehaviorMessage(action, client->GetActor() );
}




bool EntityManager::SendActorList(Client *client)
{
    // Superclients get all actors in the world, while regular clients
    // get actors in proximity.
    if ( !client->IsSuperClient() )
    {
        return true;        
    }
    else
    {
        psserver->GetNPCManager()->SendNPCList(client);
        return true;
    }
}

bool EntityManager::RemoveActor(gemObject *actor)
{
    // Do network commmand to remove entity from all clients
    psRemoveObject msg(0, actor->GetEID());
    
    // Send to human clients in range
    psserver->GetEventManager()->Multicast(msg.msg, 
                          actor->GetMulticastClients(),
                          0,PROX_LIST_ANY_RANGE);

    // also notify superclients
    psserver->GetNPCManager()->RemoveEntity(msg.msg);

    delete actor;
    actor = NULL;

    return true;
}

bool EntityManager::AddRideRelation(gemActor *rider, gemActor *mount)
{
    if(!rider || !mount)
    {
        Error1("Wrong pointers in AddRideRelation()");
        return false;
    }

    if(rider->GetMount())
    {
        psserver->SendSystemError(rider->GetClientID(), "You are already on a mount.");
        return false;
    }

    if(rider->GetRider())
    {
        psserver->SendSystemError(rider->GetClientID(), "You are already the mount of %s.", rider->GetRider()->GetName());
        return false;
    }

    if(mount->GetMount())
    {
        psserver->SendSystemError(rider->GetClientID(), "%s is on a mount.", mount->GetName());
        return false;
    }

    if(mount->GetRider())
    {
        psserver->SendSystemError(rider->GetClientID(), "%s is already the mount of %s.", mount->GetName(), mount->GetRider()->GetName());
        return false;
    }
    
    rider->SetMount(mount);
    mount->SetRider(rider);
    
    psMountingMessage mountmsg(rider->GetClientID(), mount->GetEID(), rider->GetEID(), true);
    CS_ASSERT( mountmsg.valid );
    
    psserver->GetEventManager()->Multicast( mountmsg.msg, rider->GetMulticastClients(),
            0, // Multicast to all without exception
            PROX_LIST_ANY_RANGE );

    // the actor stays suspend in midair after dismounting, and this does not seem to help...
    //FallBegan(GetPosition(), GetSector());
    
    return true;
}

void EntityManager::RemoveRideRelation(gemActor *rider, gemActor *mount)
{
    if(rider != mount->GetRider() || mount != rider->GetMount())
        Error1("Invalid info in RemoveRideRelation");

    rider->SetMount(NULL);
    mount->SetRider(NULL);
    
    psMountingMessage mountmsg(rider->GetClientID(), mount->GetEID(), rider->GetEID(), false);
    CS_ASSERT( mountmsg.valid );
    
    psserver->GetEventManager()->Multicast( mountmsg.msg, rider->GetMulticastClients(),
            0, // Multicast to all without exception
            PROX_LIST_ANY_RANGE );
}

void EntityManager::SetReady(bool flag)
{
    ready = flag; 
    hasBeenReady |= ready;
    if (ready)
    {
        usermanager->Ready();
    }
}

