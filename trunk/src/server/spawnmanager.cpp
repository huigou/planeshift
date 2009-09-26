/*
 * spawnmanager.cpp by Keith Fulton <keith@paqrat.com>
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
 */


#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/randomgen.h>
#include <csutil/csstring.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psitemstats.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/pssectorinfo.h"

#include "net/msghandler.h"

#include "rpgrules/vitals.h"

#include "util/eventmanager.h"
#include "util/mathscript.h"
#include "util/psconst.h"
#include "util/psdatabase.h"
#include "util/psxmlparser.h"
#include "util/serverconsole.h"
#include "util/slots.h"

//=============================================================================
// Local Includes
//=============================================================================
#define SPAWNDEBUG
#include "spawnmanager.h"
#include "cachemanager.h"
#include "client.h"
#include "clients.h"
#include "entitymanager.h"
#include "events.h"
#include "gem.h"
#include "invitemanager.h"
#include "netmanager.h"
#include "playergroup.h"
#include "progressionmanager.h"
#include "psserver.h"
#include "psserverchar.h"
#include "serverstatus.h"
#include "globals.h"


/** A structure to hold the clients that are pending a group loot question.
 */
class PendingLootPrompt : public PendingQuestion
{
public:
    psItem* item;    ///< The looted item we're prompting for
    PID looterID;    ///< The ID of the player who attempted to pick up the item
    PID rollerID;    ///< The ID of the player who would have won it in a roll
    PID looteeID;

    csString lootername;
    csString rollername;
    csString looteename;

    PendingLootPrompt( Client *looter,
                       Client *roller,
                       psItem *loot,
                       psCharacter *dropper,
                       const char *question)
    : PendingQuestion( roller->GetClientNum(),
                       question,
                       psQuestionMessage::generalConfirm)
    {
        if ( !looter || !roller || !loot || !dropper )
        {
            if (loot) CacheManager::GetSingleton().RemoveInstance(loot);
            delete this;
            return;
        }

        item = loot;
        looterID = looter->GetActor()->GetPID();
        rollerID = roller->GetActor()->GetPID();
        looteeID = dropper->GetPID();

        // These might not be around later, so save their names now
        lootername = looter->GetName();
        rollername = roller->GetName();
        looteename = dropper->GetCharName();
    }

    virtual ~PendingLootPrompt() { }

    void HandleAnswer(const csString & answer)
    {
        PendingQuestion::HandleAnswer(answer);

        if ( dynamic_cast<psItem*>(item) == NULL )
        {
            Error2("Item held in PendingLootPrompt with id %u has been lost",id);
            return;
        }
        
        gemActor* looter = GEMSupervisor::GetSingleton().FindPlayerEntity(looterID);
        gemActor* roller = GEMSupervisor::GetSingleton().FindPlayerEntity(rollerID);

        gemActor* getter = (answer == "yes") ? looter : roller ;

        // If the getter left the world, default to the other player
        if (!getter /*|| !getter->InGroup()*/ )
        {
            getter = (answer == "yes") ? roller : looter ;

            // If the other player also vanished, get rid of the item and be done with it...
            if (!getter /*|| !getter->InGroup()*/ )
            {
                CacheManager::GetSingleton().RemoveInstance(item);
                return;
            }
        }

        // Create the loot message
        csString lootmsg;
        lootmsg.Format("%s was %s a %s by roll winner %s",
                       lootername.GetData(),
                       (getter == looter) ? "allowed to loot" : "stopped from looting",
                       item->GetName(),
                       rollername.GetData() );

        // Attempt to give to getter
        bool dropped = getter->GetCharacterData()->Inventory().AddOrDrop(item);

        if (!dropped)
            lootmsg.Append(", but can't hold anymore");

        // Send out the loot message
        psSystemMessage loot(getter->GetClientID(), MSG_LOOT, lootmsg.GetData() );
        getter->SendGroupMessage(loot.msg);

        psLootEvent evt(
                       looteeID,
                       getter->GetCharacterData()->GetPID(),
                       item->GetUID(),
                       item->GetStackCount(),
                       (int)item->GetCurrentStats()->GetQuality(),
                       0
                       );
        evt.FireEvent();
    }
    
    void HandleTimeout() { HandleAnswer("yes"); }
};



SpawnManager::SpawnManager(psDatabase *db)
{
    database  = db;

    lootRandomizer = new LootRandomizer();

    PreloadDatabase();

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<SpawnManager>(this,&SpawnManager::HandleLootItem),MSGTYPE_LOOTITEM,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<SpawnManager>(this,&SpawnManager::HandleDeathEvent),MSGTYPE_DEATH_EVENT,NO_VALIDATION);
}

SpawnManager::~SpawnManager()
{
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_LOOTITEM);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DEATH_EVENT);

    csHash<LootEntrySet *>::GlobalIterator it(looting.GetIterator ());
    while (it.HasNext ())
    {
        LootEntrySet* loot = it.Next ();
        delete loot;
    }
    
    csHash<SpawnRule*>::GlobalIterator ruleIt(rules.GetIterator());
    while(ruleIt.HasNext())
    	delete ruleIt.Next();

    delete lootRandomizer;

}

void SpawnManager::PreloadLootRules()
{
    // loot_rule_id=0 is not valid and not loaded
    Result result(db->Select("select * from loot_rule_details where loot_rule_id>0"));
    if (!result.IsValid() )
    {
        Error2("Could not load loot rule details due to database error: %s\n",
               db->GetLastError());
        return;
    }

    for (unsigned int i=0; i<result.Count(); i++)
    {
        int id = result[i].GetInt("loot_rule_id");
        LootEntrySet *currset = looting.Get(id,NULL);
        if (!currset)
        {
            currset = new LootEntrySet(id);
            currset->SetRandomizer( this->lootRandomizer );
            looting.Put(id,currset);
        }

        LootEntry *entry = new LootEntry;
        int item_id = result[i].GetInt("item_stat_id");

        entry->item = CacheManager::GetSingleton().GetBasicItemStatsByID( item_id );
        entry->probability = result[i].GetFloat("probability");
        entry->min_money = result[i].GetInt("min_money");
        entry->max_money = result[i].GetInt("max_money");
        entry->randomize = (result[i].GetInt("randomize") ? true : false);

        if (!entry->item && item_id != 0)
        {
            Error2("Could not find specified loot item stat: %d\n",item_id );
            delete entry;
            continue;
        }

        currset->AddLootEntry(entry);
    }
}

#if 0
bool SpawnManager::LoadWaypointsAsSpawnRanges()
{
    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
    const char *xmlfile = "/planeshift/data/npcdefs.xml";

    csRef<iDataBuffer> buff = vfs->ReadFile(xmlfile);

    if ( !buff || !buff->GetSize() )
    {
        return false;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error    = doc->Parse( buff );
    if ( error )
    {
        Error3("Error %s in %s", error, xmlfile);
        return false;
    }

    iDocumentNode *topNode = root->GetNode("waypoints");
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();
    
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        if (!strcmp( node->GetValue(), "waypointlist" ))
        {
            csRef<iDocumentNodeIterator> iter = node->GetNodes();
            while (iter->HasNext())
            {
                csRef<iDocumentNode> node = iter->Next();
                if ( node->GetType() != CS_NODE_ELEMENT )
                    continue;
                if (!strcmp(node->GetValue(),"waypoint"))
                {
                    Waypoint *wp = new Waypoint;
                    if (wp->Load(node))
                    {
                        waypoints.Insert(wp,true);
                    }
                    else
                    {
                        CPrintf(CON_ERROR, "Waypoint node had no name specified!\n");
                        delete wp;
                        return false;
                    }
                }
            }
        }
        else if (!strcmp(node->GetValue(), "waylinks" ))
        {
            csRef<iDocumentNodeIterator> iter = node->GetNodes();
            while (iter->HasNext())
            {
                csRef<iDocumentNode> node = iter->Next();
                if ( node->GetType() != CS_NODE_ELEMENT )
                    continue;
                if (!strcmp(node->GetValue(),"point"))
                {
                    Waypoint key;
                    key.name = node->GetAttributeValue("name");
                    Waypoint *wp = waypoints.Find(&key);
                    if (!wp)
                    {
                        CPrintf(CON_ERROR, "Waypoint called '%s' not defined.\n",key.name.GetData() );
                        return false;
                    }
                    else
                    {
                        csRef<iDocumentNodeIterator> iter = node->GetNodes();
                        while (iter->HasNext())
                        {
                            csRef<iDocumentNode> node = iter->Next();
                            if ( node->GetType() != CS_NODE_ELEMENT )
                                continue;
                            if (!strcmp(node->GetValue(),"link"))
                            {
                                Waypoint key;
                                key.name = node->GetAttributeValue("name");
                                Waypoint *wlink = waypoints.Find(&key);
                                if (!wlink)
                                {
                                    CPrintf(CON_ERROR, "Waypoint called '%s' not defined.\n",key.name.GetData() );
                                    return false;
                                }
                                else
                                {
                                    bool one_way = node->GetAttributeValueAsBool("oneway");                            
                                    wp->links.Push(wlink);
                                    if (!one_way)
                                        wlink->links.Push(wp);  // bi-directional link is implied
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}
#endif

void SpawnManager::PreloadLootModifiers()
{

    // Order by's are a little slower but it guarentees order
    Result result( db->Select( "SELECT * FROM loot_modifiers ORDER BY modifier_type, probability" ) );
    if ( !result.IsValid() )
    {
        Error2( "Could not load loot modifiers due to database error: %s\n",
               db->GetLastError() );
        return;
    }

    for ( unsigned int i = 0; i < result.Count(); i++ )
    {
        LootModifier *entry = new LootModifier;

        entry->modifier_type = result[i][ "modifier_type" ];
        entry->name = result[i][ "name" ];
        entry->effect = result[i][ "effect" ];
        entry->equip_script = result[i]["equip_script"];
        entry->effect = result[i][ "effect" ];
        entry->probability = result[i].GetFloat( "probability" );
        entry->stat_req_modifier = result[i][ "stat_req_modifier" ];
        entry->cost_modifier = result[i].GetFloat( "cost_modifier" );
        entry->mesh = result[i][ "mesh" ];
        entry->not_usable_with = result[i][ "not_usable_with" ];

        lootRandomizer->AddLootModifier( entry );
    }
}

void SpawnManager::LoadHuntLocations(psSectorInfo *sectorinfo)
{
    csString query;
    
    if ( sectorinfo )
    {
        query.Format("SELECT h.*,i.name FROM hunt_locations h JOIN item_stats i ON i.id = h.itemid WHERE sector='%s'", 
            sectorinfo->name.GetData());
    }
    else
        query = "SELECT h.*,i.name FROM hunt_locations h JOIN item_stats i ON i.id = h.itemid";
                
    Result result(db->Select(query));
    
    if (!result.IsValid() )
    {
        Error2("Could not load hunt_locations due to database error: %s\n", db->GetLastError());
        return;
    }

    for (unsigned int i=0; i<result.Count(); i++)
    {
        // Get some vars to work with
        csString sector = result[i]["sector"];
        csVector3 pos(result[i].GetFloat("x"),result[i].GetFloat("y"),result[i].GetFloat("z"));
        uint32 itemid = result[i].GetUInt32("itemid");
        int interval = result[i].GetInt("interval");
        int max_rnd = result[i].GetInt("max_random");
        int id      = result[i].GetInt("id");
		int amount = result[i].GetInt("amount");
		float range = result[i].GetFloat("range");
        csString name = result[i]["name"];
        
        // Schdule the item spawn
        psSectorInfo *spawnsector=CacheManager::GetSingleton().GetSectorInfoByName(sector);

        if (spawnsector==NULL)
        {
            Error2("hunt_location failed to load, wrong sector: %s\n", sector.GetData() );
            continue;
        }
		
		iSector *iSec = EntityManager::GetSingleton().FindSector(sector.GetData());
        if(!iSec)
        {
            Error2("Sector '%s' failed to be found when loading hunt location.", sector.GetData());
            continue;
        }

        GEMSupervisor* gem = GEMSupervisor::GetSingletonPtr();
        
        csArray<gemObject*> nearlist;
        size_t handledSpawnsCount = 0;
 
        if (gem)
        {
            // Look for nearby items to prevent rescheduling of existing items
            nearlist = gem->FindNearbyEntities(iSec, pos, range);
            size_t nearbyItemsCount = nearlist.GetSize();
                        
    		for (size_t i = 0; i < nearbyItemsCount; ++i)
    		{
                gemItem* gemitem = dynamic_cast<gemItem*>(nearlist[i]);
                if (gemitem)
                {
                	psItem* item = gemitem->GetItemData();
                    if (name == item->GetName()) // Correct item?
                    {
                        psScheduledItem* schedule = new psScheduledItem(id,itemid,pos,spawnsector,0,interval,max_rnd,range);
                        item->SetScheduledItem(schedule);
                        ++handledSpawnsCount;
                    }
                }
                
                if ((int) handledSpawnsCount == amount) // All schedules accounted for
                    break;
    		}
        }
				
		for (int i = 0; i < (amount - (int) handledSpawnsCount); ++i) //Make desired amount of items that are not already existing
		{
	        // This object won't get destroyed in a while (until something stops it or psItem is destroyed without moving)
	        psScheduledItem* item = new psScheduledItem(id,itemid,pos,spawnsector,0,interval,max_rnd,range);
	        
	        // Queue it 
	        psItemSpawnEvent *newevent = new psItemSpawnEvent(item);
	        psserver->GetEventManager()->Push(newevent);
		}
    }
}

void SpawnManager::LoadSpawnRanges(SpawnRule *rule)
{
    Result result(db->Select("select npc_spawn_ranges.id,x1,y1,z1,x2,y2,z2,name,range_type_code"
                             "  from npc_spawn_ranges, sectors "
                             "  where npc_spawn_ranges.sector_id = sectors.id"
                             "  and npc_spawn_rule_id=%d", rule->GetID() ));

    if (!result.IsValid() )
    {
        Error2("Could not load NPC spawn ranges due to database error: %s\n",
               db->GetLastError());
        return;
    }

    for (unsigned int i=0; i<result.Count(); i++)
    {
        SpawnRange *r = new SpawnRange;

        r->Initialize( result[i].GetInt("id"),
                       rule->GetID(),
                       result[i]["range_type_code"],
                       result[i].GetFloat("x1"),
                       result[i].GetFloat("y1"),
                       result[i].GetFloat("z1"),
                       result[i].GetFloat("x2"),
                       result[i].GetFloat("y2"),
                       result[i].GetFloat("z2"),
                       result[i]["name"]);


        rule->AddRange(r);
    }
}

void SpawnManager::PreloadDatabase()
{
    PreloadLootRules();

    PreloadLootModifiers();

    Result result(db->Select("select id,min_spawn_time,max_spawn_time,"
                 "       substitute_spawn_odds,substitute_player,"
                 "       fixed_spawn_x,fixed_spawn_y,fixed_spawn_z,"
                 "       fixed_spawn_rot,fixed_spawn_sector,loot_category_id,"
                 "       dead_remain_time,fixed_spawn_instance"
                 "  from npc_spawn_rules"));
    if (!result.IsValid() )
    {
        Error2("Could not load NPC spawn rules due to database error: %s\n",
               db->GetLastError());
        return;
    }

    for (unsigned int i=0; i<result.Count(); i++)
    {
        LootEntrySet *loot_set = looting.Get( result[i].GetInt("loot_category_id"), NULL );

        SpawnRule *newrule = new SpawnRule;

        newrule->Initialize(result[i].GetInt("id"),
                            result[i].GetInt("min_spawn_time"),
                            result[i].GetInt("max_spawn_time"),
                            result[i].GetFloat("substitute_spawn_odds"),
                            result[i].GetInt("substitute_player"),
                            result[i].GetFloat("fixed_spawn_x"),
                            result[i].GetFloat("fixed_spawn_y"),
                            result[i].GetFloat("fixed_spawn_z"),
                            result[i].GetFloat("fixed_spawn_rot"),
                            result[i]["fixed_spawn_sector"],
                            loot_set,
                            result[i].GetInt("dead_remain_time"),
                            result[i].GetUInt32("fixed_spawn_instance"));

        LoadSpawnRanges(newrule);

        rules.Put(newrule->GetID(), newrule);
    }
}

void SpawnManager::RepopulateLive(psSectorInfo *sectorinfo)
{
    psCharacter **chardatalist = NULL;
    int count;

    chardatalist = psServer::CharacterLoader.LoadAllNPCCharacterData(sectorinfo,count);
    if (chardatalist==NULL)
    {
        Error1("No NPCs found to repopulate.\n");
        return;
    }

    for (int i=0; i<count; i++)
    {
        if (chardatalist[i] != NULL)
            EntityManager::GetSingleton().CreateNPC(chardatalist[i]);
        else
            Error1("Failed to repopulate NPC!");
    }

    delete[] chardatalist;
}

void SpawnManager::RepopulateItems(psSectorInfo *sectorinfo)
{
    csArray<psItem*> items;

    // Load list from database
    if (!CacheManager::GetSingleton().LoadWorldItems(sectorinfo, items))
    {
        Error1("Failed to load world items.");
        return;
    }

    // Now create entities and meshes, etc. for each one
    int spawned = 0;
    for (size_t i = 0; i < items.GetSize(); i++)
    {
        psItem *item = items[i];
        CS_ASSERT(item);
        // load items not in containers
        if (item->GetContainerID() == 0)
        {
            //if create item returns false, then no spawn occurs
            if (EntityManager::GetSingleton().CreateItem( item, (item->GetFlags() & PSITEM_FLAG_TRANSIENT) ? true : false))
            {
                // printf("Created item %d: %s\n", item->GetUID(), item->GetName() );
                // item->Save(false);
                spawned++;
            }
            else
            {
                printf("Creating item '%s' (%i) failed.\n", item->GetName(), item->GetUID());
                delete item; // note that the dead item is still in the array
            }
        }
        // load items in containers
        else if (item->GetContainerID())
        {
            gemItem *citem = EntityManager::GetSingleton().GetGEM()->FindItemEntity(item->GetContainerID());
            gemContainer *container = dynamic_cast<gemContainer*> (citem);
            if (container)
            {
                if (!container->AddToContainer(item,NULL,item->GetLocInParent()))
                {
                    Error2("Cannot add item into container slot %i.\n",item->GetLocInParent());
                    delete item;
                }
            }
            else
            {
                Error3("Container with id %d not found, specified in item %d.", 
                       item->GetContainerID(), 
                       item->GetUID() );
                delete item;
            }
        }
    }

    Debug2(LOG_SPAWN,0,"Spawned %d items.\n",spawned);
}

void SpawnManager::KillNPC(gemObject *obj, gemActor* killer)
{
    int killer_cnum = 0;

    if (killer)
    {
        killer_cnum = killer->GetClientID();
        Debug3(LOG_SPAWN, 0, "Killer '%s' killed '%s'", killer->GetName(), obj->GetName() );
    }
    else
    {
        Debug2(LOG_SPAWN, 0, "Killed NPC:%s", obj->GetName() );
    }


    if (!obj->IsAlive())
        return;

    obj->SetAlive(false);

    /**
     * The goal is to have NPC drop exactly what they have in their
     * inventory and nothing more. So the loot has be generated when
     * the NPC is created. With the EXCEPTION of invulnerable NPCs
     * that have a predefined equipment, not generated. Today that's
     * not implemented, so we will just create loot when the NPC dies.
     * For this reason we temporarly remove the line below. When
     * loot will be generated at NPC creation, we should add again
     * the line below.
    // add all the character's inventory to the loot
    obj->GetCharacterData()->AddInventoryToLoot();
     */

    // Create his loot
    SpawnRule *respawn = NULL;
    int spawnruleid = obj->GetCharacterData()->NPC_GetSpawnRuleID();
    if (spawnruleid)
    {
        respawn = rules.Get(spawnruleid, NULL);
    }

    if (respawn)
    {
        if (respawn->GetLootRules())
        {
            respawn->GetLootRules()->CreateLoot( obj->GetCharacterData() );
        }
    }

    int loot_id = obj->GetCharacterData()->GetLootCategory();
    if (loot_id) // custom loot for this mob also
    {
        LootEntrySet *loot = looting.Get(loot_id,0);
        if (loot)
        {
            Debug2(LOG_LOOT, 0, "Creating loot %d.", loot_id);
            loot->CreateLoot( obj->GetCharacterData() );
        }
        else
        {
            Error3("Missing specified loot rule %d in character %s.",loot_id,obj->GetName() );
        }
    }

    if (killer)
    {
        csRef<PlayerGroup> grp = killer->GetGroup();
        if (!grp)
        {
            // Check if the killer is owned (pet/familiar)
            gemActor * owner = dynamic_cast<gemActor*>(killer->GetOwner());
            if (owner)
            {
                // Is the owner part of a group? The group code below will add the
                // group and the owner.
                grp = owner->GetGroup();
                if (!grp)
                {
                    // Not part of group so add the owner
                    Debug3(LOG_LOOT, killer_cnum, "Adding owner %d as able to loot %s.", 
                           owner->GetClientID(), obj->GetName() );
                    obj->AddLootableClient(owner->GetClientID());
                }
                else
                {
                    Debug2(LOG_LOOT, killer_cnum, "Adding from owners %s group.", owner->GetName());
                }
            }
        }
        
        if (grp)
        {
            for (size_t i=0; i<grp->GetMemberCount(); i++)
            {
                if (grp->GetMember(i)->RangeTo(obj) < RANGE_TO_RECV_LOOT)
                {
                    Debug3(LOG_LOOT, 0,"Adding %s as able to loot %s.",grp->GetMember(i)->GetName(),obj->GetName() );
                    obj->AddLootableClient(grp->GetMember(i)->GetClientID() );
                }
                else
                {
                    Debug3(LOG_LOOT, 0,"Not adding %s as able to loot %s, because out of range.",grp->GetMember(i)->GetName(),obj->GetName() );
                }
            }
        }
        else
        {
            Debug3(LOG_LOOT, killer_cnum, "Adding client %d as able to loot %s.", killer_cnum, obj->GetName() );
            obj->AddLootableClient(killer_cnum);
        }

    }

    obj->GetCharacterData()->ResetStats();

    // Set timer for when NPC will disappear
    csTicks delay = (respawn)?respawn->GetDeadRemainTime():5000;
    psDespawnGameEvent *newevent = new psDespawnGameEvent(this,delay,obj);
    psserver->GetEventManager()->Push(newevent);

    Notify3(LOG_SPAWN,"Scheduled NPC %s to be removed in %1.1f seconds.",obj->GetName(),(float)delay/1000.0);
}

void SpawnManager::RemoveNPC(gemObject *obj)
{
    Debug2(LOG_SPAWN,0,"RemoveNPC:%s\n",obj->GetName() );

    ServerStatus::mob_deathcount++;

    PID pid = obj->GetPID();

    Notify3(LOG_SPAWN, "Sending NPC %s disconnect msg to %zu clients.\n", ShowID(obj->GetEID()), obj->GetMulticastClients().GetSize());

    if (obj->GetCharacterData()==NULL)
    {
        Error2("Character data for npc character %s was not found! Entity stays dead.\n", ShowID(pid));
        return;
    }

    SpawnRule *respawn = NULL;
    int spawnruleid = obj->GetCharacterData()->NPC_GetSpawnRuleID();

    if (spawnruleid)
    {
        // Queue for respawn according to rules
        respawn = rules.Get(spawnruleid, NULL);
    }

    if (!respawn)
    {
        if (spawnruleid == 0) // spawnruleid 0 is for non-respawning NPCs
        {
            Notify2(LOG_SPAWN,"Temporary NPC based on player ID %s has died. Entity stays dead.\n", ShowID(pid));
        }
        else
        {
            Error3("Respawn rule for player %s, rule %d was not found! Entity stays dead.\n", ShowID(pid), spawnruleid);
        }

        // Remove mesh, etc from engine
        EntityManager::GetSingleton().RemoveActor(obj);
        return;
    }

//#ifdef SPAWNDEBUG
//    if (obj->GetEntity()->GetRefCount() != 2)
//        Error1("Dead NPC refcount is not 1.  Clients and server out of sync!\n");
//#endif


    csVector3 pos;
    float angle;
    csString sector;
    int delay = respawn->GetRespawnDelay();
    InstanceID instance;

    respawn->DetermineSpawnLoc(obj->GetCharacterData(),pos,angle,sector,instance);

    // Remove mesh, etc from engine
    EntityManager::GetSingleton().RemoveActor(obj);

    PID newplayer = respawn->CheckSubstitution(pid);

    psRespawnGameEvent *newevent = new psRespawnGameEvent(this,delay,pos,angle,sector,newplayer,instance);
    psserver->GetEventManager()->Push(newevent);
    
    csString msg;
    msg.Format("Scheduled NPC %s to be respawned in %1.1f seconds at (%1.0f,%1.0f,%1.0f) in %s.",
               ShowID(newplayer),
               (float)delay/1000.0,
               pos.x, pos.y, pos.z, sector.GetData() );

    Notify2(LOG_SPAWN, "%s", msg.GetData());
}


#define SPAWN_POINT_TAKEN 999
#define SPAWN_BASE_ITEM   1000


void SpawnManager::Respawn(InstanceID instance, csVector3& where, float rot, csString& sector, PID playerID)
{
    psSectorInfo* spawnsector = CacheManager::GetSingleton().GetSectorInfoByName(sector);
    if (spawnsector==NULL)
    {
        Error2("Spawn message indicated unresolvable sector '%s'\n",(const char*)sector);
        return;
    }

    psCharacter *chardata=psServer::CharacterLoader.LoadCharacterData(playerID,false);
    if (chardata==NULL)
    {
        Error2("Character %s to be respawned does not have character data to be loaded!\n", ShowID(playerID));
        return;
    }

    chardata->SetLocationInWorld(instance,spawnsector,where.x,where.y,where.z,rot);
    chardata->GetHPRate().SetBase(HP_REGEN_RATE);
    chardata->GetManaRate().SetBase(MANA_REGEN_RATE);

    // Here we restore status of equipped items to max quality as this is going to be a *newly born* npc
    // We will repair only equiped items.
    for(int cur_slot = PSCHARACTER_SLOT_RIGHTHAND; cur_slot < PSCHARACTER_SLOT_BULK1; cur_slot++)
    {
        //Get the item in the current equip slot
        psItem * cur_item = chardata->Inventory().GetInventoryItem((INVENTORY_SLOT_NUMBER)cur_slot);
        if(cur_item) //there is something in the slot?
            cur_item->SetItemQuality(cur_item->GetMaxItemQuality()); //Restore the item to it's max quality
    }

    // Now create the NPC as usual
    EntityManager::GetSingleton().CreateNPC(chardata);

    ServerStatus::mob_birthcount++;
}

void SpawnManager::HandleLootItem(MsgEntry *me,Client *client)
{
    psLootItemMessage msg(me);

    // Possible hack here?  We are trusting the client to send the right msg.entity?
    gemObject *obj = GEMSupervisor::GetSingleton().FindObject(msg.entity);
    if (!obj)
    {
        Error3("LootItem Message from %s specified an erroneous entity id: %s.\n", client->GetName(), ShowID(msg.entity));
        return;
    }

    psCharacter *chr = obj->GetCharacterData();
    if (!chr)
    {
        Error3("LootItem Message from %s specified a non-character entity id: %s.\n", client->GetName(), ShowID(msg.entity));
        return;
    }

    // Check the range to the lootable object. 
    if (client->GetActor()->RangeTo(obj) > RANGE_TO_LOOT )
    {
        psserver->SendSystemError(client->GetClientNum(), "Too far away to loot %s.", obj->GetName() );
        return;
    }

    if (!chr->RemoveLootItem(msg.lootitem))
    {
        // Take this out because it is just the result of duplicate loot commands due to lag
        //Warning3(LOG_COMBAT,"LootItem Message from %s specified bad item id of %d.\n",client->GetName(), msg.lootitem);
        return;
    }

    psItemStats *itemstat = CacheManager::GetSingleton().GetBasicItemStatsByID(msg.lootitem);
    size_t index = chr->Inventory().FindItemStatIndex(itemstat);
    psItem *item = NULL;
    if (index != SIZET_NOT_FOUND)
        item = chr->Inventory().RemoveItemIndex(index, 1);
    if (item == NULL && itemstat != NULL)
    {
        item = itemstat->InstantiateBasicItem();
        item->SetLoaded();
    }

    csRef<PlayerGroup> group = client->GetActor()->GetGroup();
    Client *randfriendclient = NULL;
    if (group.IsValid())
    {
        randfriendclient = obj->GetRandomLootClient(RANGE_TO_LOOT*10);
        if (!randfriendclient)
        {
            Error3("GetRandomLootClient failed for loot msg from %s, object %s.\n", client->GetName(), item->GetName() );
            return;
        }
    }

    csString type;
    Client *looterclient;  // Client that gets the item
    if ( msg.lootaction == msg.LOOT_SELF || !group.IsValid() )
    {
        looterclient = client;
        type = "Loot Self";
    }
    else
    {
        looterclient = randfriendclient;
        type = "Loot Roll";
    }

    // Ask group member before take
    if (msg.lootaction == msg.LOOT_SELF && group.IsValid() && client != randfriendclient)
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "Asking roll winner %s if you may take the %s...",
                                  randfriendclient->GetName(), item->GetName() );
        csString request;
        request.Format("You have won the roll for a %s, but %s wants to take it."
                       "  Will you allow this action?",
                       item->GetName(), client->GetName());

        // Item will be held in the prompt until answered.
        PendingLootPrompt *p = new PendingLootPrompt(client, randfriendclient, item, chr, request);
        psserver->questionmanager->SendQuestion(p);

        type.Append(" Pending");
    }
    // Continue with normal looting if not prompting
    else
    {
        // Create the loot message
        csString lootmsg;
        if (group.IsValid())
            lootmsg.Format("%s won the roll and",looterclient->GetName());
        else
            lootmsg.Format("You");
        lootmsg.AppendFmt(" looted a %s",item->GetName());

        // Attempt to give to looter
        bool dropped = looterclient->GetActor()->GetCharacterData()->Inventory().AddOrDrop(item);
        
        if (!dropped)
        {
            lootmsg.Append(", but can't hold anymore");
            type.Append(" (dropped)");
        }
            
        // Send out the loot message
        psSystemMessage loot(me->clientnum, MSG_LOOT, lootmsg.GetData() );
        looterclient->GetActor()->SendGroupMessage(loot.msg);

        item->Save(false);
    }

    // Trigger item removal on every client in the group which has intrest
    if (group.IsValid())
    {
        for (int i=0; i < (int)group->GetMemberCount(); i++)
        {
            int cnum = group->GetMember(i)->GetClientID();
            if (obj->IsLootableClient(cnum))
            {
                psLootRemoveMessage rem(cnum,msg.lootitem);
                rem.SendMessage();
            }
        }
    }
    else
    {
        psLootRemoveMessage rem(client->GetClientNum(),msg.lootitem);
        rem.SendMessage();
    }

    psLootEvent evt(
                   chr->GetPID(),
                   looterclient->GetCharacterData()->GetPID(),
                   item->GetUID(),
                   item->GetStackCount(),
                   (int)item->GetCurrentStats()->GetQuality(),
                   0
                   );
    evt.FireEvent();

}

void SpawnManager::HandleDeathEvent(MsgEntry *me,Client *notused)
{
    Debug1(LOG_SPAWN,0, "Spawn Manager handling Death Event\n");
    psDeathEvent death(me);
    
    death.deadActor->GetCharacterData()->KilledBy(death.killer ? death.killer->GetCharacterData() : NULL);
    if(death.killer)
        death.killer->GetCharacterData()->Kills(death.deadActor->GetCharacterData());

    // Respawning is handled with ResurrectEvents for players and by SpawnManager for NPCs
    if ( death.deadActor->GetClientID() )   // Handle Human Player dying
    {
        ServerStatus::player_deathcount++;
        psResurrectEvent *event = new psResurrectEvent(0,20000,death.deadActor);
        psserver->GetEventManager()->Push(event);
        Debug2(LOG_COMBAT, death.deadActor->GetClientID(), "Queued resurrect event for %s.\n",death.deadActor->GetName());
    }
    else  // Handle NPC dying
    {
        Debug1(LOG_NPC, 0,"Killing npc in spawnmanager.\n");
        // Remove NPC and queue for respawn
        KillNPC(death.deadActor, death.killer);
    }

    // Allow Actor to notify listeners of death
    death.deadActor->HandleDeath();
}

/*----------------------------------------------------------------*/

SpawnRule::SpawnRule()
{
    randomgen = psserver->rng;
    id = minspawntime = maxspawntime = 0;
    substitutespawnodds = 0;
    substituteplayer    = 0;
    fixedspawnx = fixedspawny = fixedspawnz = fixedspawnrot = 0;
}
SpawnRule::~SpawnRule()
{
	csHash<SpawnRange*>::GlobalIterator rangeIt(ranges.GetIterator());
    while(rangeIt.HasNext())
    	delete rangeIt.Next();
}

void SpawnRule::Initialize(int idval,
                           int minspawn,
                           int maxspawn,
                           float substodds,
                           int substplayer,
                           float x,float y,float z,float angle,
                           const char *sector,
                           LootEntrySet *loot_id,
                           int dead_time,
                           InstanceID instance)
{
    id = idval;
    minspawntime = minspawn;
    maxspawntime = maxspawn;
    substitutespawnodds = substodds;
    substituteplayer    = substplayer;
    fixedspawnx = x;
    fixedspawny = y;
    fixedspawnz = z;
    fixedspawnrot = angle;
    fixedspawnsector = sector;
    loot = loot_id;
    dead_remain_time = dead_time;
    fixedinstance = instance;
}


int SpawnRule::GetRespawnDelay()
{
    int ticks = maxspawntime-minspawntime;

    return minspawntime + randomgen->Get(ticks);
}


PID SpawnRule::CheckSubstitution(PID originalplayer)
{
    int score = randomgen->Get(10000);

    if (score < 10000.0*substitutespawnodds)
        return substituteplayer;
    else
        return originalplayer;
}

void SpawnRule::DetermineSpawnLoc(psCharacter *ch, csVector3& pos, float& angle, csString& sectorname, InstanceID& instance)
{
    // ignore fixed point if there are ranges in this rule

    size_t rcount = ranges.GetSize();

    if (rcount > 0)
    {
        // Got ranges
        // Pick a location using uniform probability:
        // 1. Pick a range with probability proportional to area
        // 2. Pick a point in that range

        csHash<SpawnRange*>::GlobalIterator rangeit(ranges.GetIterator());

        // Compute total area
        float totalarea = 0;
        SpawnRange* range;
        while(rangeit.HasNext())
        {
        	range = rangeit.Next();
        	totalarea += range->GetArea();
        }
        // aimed area level
        float aimed = randomgen->Get() * totalarea;
        // cumulative area level
        float cumul = 0;
        csHash<SpawnRange*>::GlobalIterator rangeit2(ranges.GetIterator());
        while(rangeit2.HasNext())
        {
        	range = rangeit.Next();
            cumul += range->GetArea();
            if (cumul >= aimed)
            {
                // got it!
                pos = range->PickPos();
                sectorname = range->GetSector();
                if (ch && sectorname == "startlocation")
                    sectorname = ch->spawn_loc.loc_sector->name;
                break;
            }
        }

        // randomly choose an angle in [0, 2*PI]
        angle = randomgen->Get() * TWO_PI;
        instance = fixedinstance;
    }
    else if (ch && fixedspawnsector == "startlocation")
    {
        pos = ch->spawn_loc.loc;
        angle = ch->spawn_loc.loc_yrot;
        sectorname = ch->spawn_loc.loc_sector->name;
        instance = ch->spawn_loc.worldInstance;
    }
    else
    {
        // Use fixed spawn point
        pos.x = fixedspawnx;
        pos.y = fixedspawny;
        pos.z = fixedspawnz;
        angle = fixedspawnrot;
        sectorname = fixedspawnsector;
        instance = fixedinstance;
    }
}

void SpawnRule::AddRange(SpawnRange *range)
{
    ranges.Put(range->GetID(), range);
}

/*----------------------------------------------------------------*/

SpawnRange::SpawnRange()
{
    id = npcspawnruleid = 0;
    area = 0;
    randomgen = psserver->rng;
}

#define RANGE_FICTITIOUS_WIDTH .5

void SpawnRange::Initialize(int idval,
                            int spawnruleid,
                            const char *type_code,
                            float rx1, float ry1, float rz1,
                            float rx2, float ry2, float rz2,
                            const char *sectorname)
{
    id = idval;
    npcspawnruleid = spawnruleid;
    type = *type_code;

    // make sure x1 < x2, y1 < y2, z1 < z2
    if (rx1 < rx2) { x1 = rx1; x2 = rx2; }
    else { x1 = rx2; x2 = rx1; }
    if (ry1 < ry2) { y1 = ry1; y2 = ry2; }
    else { y1 = ry2; y2 = ry1; }
    if (rz1 < rz2) { z1 = rz1; z2 = rz2; }
    else { z1 = rz2; z2 = rz1; }

    spawnsector = sectorname;
    float dx = x1 == x2 ? RANGE_FICTITIOUS_WIDTH : x2 - x1;
    float dz = z1 == z2 ? RANGE_FICTITIOUS_WIDTH : z2 - z1;
    area = dx * dz;

    if (type=='L')
    {
        area = (rx2-rx1)*(rx2-rx1) + (ry2-ry1)*(ry2-ry1) + (rz2-rz1)*(rz2-rz1);
        area = sqrt(area);
    }
}

const csVector3 SpawnRange::PickPos()
{
    if (type == 'A')
    {
        return csVector3(x1 + randomgen->Get() * (x2 - x1),
                         y1 + randomgen->Get() * (y2 - y1),
                         z1 + randomgen->Get() * (z2 - z1));
    }
    else // type 'L' means spawn along line segment
    {
        float d = randomgen->Get();

        return csVector3(x1 + d * (x2 - x1),
                         y1 + d * (y2 - y1),
                         z1 + d * (z2 - z1));
    }
}


/*----------------------------------------------------------------*/


psRespawnGameEvent::psRespawnGameEvent(SpawnManager *mgr,
                                       int delayticks,
                                       csVector3& pos,
                                       float angle,
                                       csString& sectorname,
                                       PID newplayer,
                                       InstanceID newinstance)
    : psGameEvent(0,delayticks,"psRespawnGameEvent")
{
    spawnmanager=mgr;
    where.x = pos.x;
    where.y = pos.y;
    where.z = pos.z;
    rot=angle;
    sector=sectorname;
    playerID=newplayer;
    instance = newinstance;
}


void psRespawnGameEvent::Trigger()
{
    spawnmanager->Respawn(instance,where,rot,sector,playerID);
}


psDespawnGameEvent::psDespawnGameEvent(SpawnManager *mgr,
                                       int delayticks,
                                       gemObject *obj)
    : psGameEvent(0,delayticks,"psDespawnGameEvent")
{
    spawnmanager = mgr;
    entity       = obj->GetEID();
}

void psDespawnGameEvent::Trigger()
{
    gemObject* object = GEMSupervisor::GetSingleton().FindObject(entity);
    if(!object)
    {
        csString status;
        status.Format("Despawn event triggered for non-existant NPC %s!", ShowID(entity));
        psserver->GetLogCSV()->Write(CSV_STATUS, status);

        Error2("%s", status.GetData());
        return;
    }
    spawnmanager->RemoveNPC(object);
}


LootEntrySet::~LootEntrySet()
{
    LootEntry *e;
    while (entries.GetSize())
    {
        e = entries.Pop();
        delete e;
    }
    lootRandomizer = NULL;
}

void LootEntrySet::AddLootEntry(LootEntry *entry)
{
    entries.Push(entry);
    total_prob += entry->probability;
}

void LootEntrySet::CreateLoot( psCharacter *chr, size_t numModifiers )
{
    // the idea behind this code is that if you have a total probability that's <1
    // then the items listed are mutually exclusive. If you have a total >1 then the
    // server rolls the probability on each item. This disables the possibility to have
    // items with low chances (0.1) that are not mutually exclusive.
    // We have to change this by using an additional field that identifies the item
    // as mutually exclusive. So for now this is commented.
    //if (total_prob < 1+EPSILON)
    //    CreateSingleLoot( chr );
    //else
          CreateMultipleLoot( chr, numModifiers );
}

void LootEntrySet::CreateSingleLoot(psCharacter *chr)
{
    float roll = psserver->rng->Get();
    float prob_so_far = 0;
    float maxcost = lootRandomizer->CalcModifierCostCap(chr);

    for (size_t i=0; i<entries.GetSize(); i++)
    {
        if (prob_so_far < roll && roll < prob_so_far + entries[i]->probability)
        {
            if(entries[i]->item) // We don't always have a item.
            {
                psItemStats *loot_item = entries[i]->item;
                Debug2(LOG_LOOT, 0,"Adding %s to the dead mob's loot.\n",entries[i]->item->GetName() );
                
                if ( entries[i]->randomize ) loot_item = lootRandomizer->RandomizeItem( loot_item, maxcost );
                chr->AddLootItem(loot_item);
            }

            float pct = psserver->rng->Get();
            int money = entries[i]->min_money + (int)(pct * (float)(entries[i]->max_money - entries[i]->min_money));
            chr->AddLootMoney(money);
            break;
        }
        prob_so_far += entries[i]->probability;
    }
}

void LootEntrySet::CreateMultipleLoot(psCharacter *chr, size_t numModifiers)
{
    float maxcost = 0.0;
    // if chr == NULL then its a randomized loot test; i.e. no character will receive it
    bool lootTesting = chr ? false : true;
    if (!lootTesting)
        maxcost = lootRandomizer->CalcModifierCostCap(chr);

    for (size_t i=0; i<entries.GetSize(); i++)
    {
      float roll = psserver->rng->Get();
      if (roll < entries[i]->probability)
      {
        if(entries[i]->item) // We don't always have a item.
        {
            psItemStats *loot_item = entries[i]->item;
            if ( entries[i]->randomize )
                loot_item = lootRandomizer->RandomizeItem( loot_item,
                                                           maxcost,
                                                           lootTesting,
                                                           numModifiers );

            if (!lootTesting)
                chr->AddLootItem(loot_item);
            else
            {
                // print out the stats
                CPrintf(CON_CMDOUTPUT,
                        "Randomized item (%d modifiers) : \'%s\', %s\n"
                        "  Quality : %.2f  Weight : %.2f  Size : %d  Price : %d\n",
                        numModifiers,
                        loot_item->GetName(), 
                        loot_item->GetDescription(),
                        loot_item->GetQuality(),
                        loot_item->GetWeight(),
                        loot_item->GetSize(),
                        loot_item->GetPrice().GetTrias());
                if (loot_item->GetIsArmor())
                {
                    CPrintf(CON_CMDOUTPUT,
                            "Armour stats:\n"
                            "  Class : %c  Hardness : %.2f\n"
                            "  Protection Slash : %.2f  Blunt : %.2f  Pierce : %.2f\n",
                            loot_item->Armor().Class(),
                            loot_item->Armor().Hardness(),
                            loot_item->Armor().Protection(PSITEMSTATS_DAMAGETYPE_SLASH),
                            loot_item->Armor().Protection(PSITEMSTATS_DAMAGETYPE_BLUNT),
                            loot_item->Armor().Protection(PSITEMSTATS_DAMAGETYPE_PIERCE));
                }
                else if (loot_item->GetIsMeleeWeapon() || loot_item->GetIsRangeWeapon())
                {
                    CPrintf(CON_CMDOUTPUT,
                            "Weapon stats:\n"
                            "  Latency : %.2f  Penetration : %.2f\n"
                            "  Damage Slash : %.2f  Blunt : %.2f  Pierce : %.2f\n"
                            "  BlockValue Untargeted : %.2f  Targeted : %.2f  Counter : %.2f\n",
                            loot_item->Weapon().Latency(),
                            loot_item->Weapon().Penetration(),
                            loot_item->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_SLASH),
                            loot_item->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_BLUNT),
                            loot_item->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_PIERCE),
                            loot_item->Weapon().UntargetedBlockValue(),
                            loot_item->Weapon().TargetedBlockValue(),
                            loot_item->Weapon().CounterBlockValue());
                }
                /*CPrintf(CON_CMDOUTPUT,
                    "Equip script: %s\n Un-equip script: %s\n",loot_item->GetProgressionEventEquip().GetData(),
                    loot_item->GetProgressionEventUnEquip().GetData());*/
            }
        }
        float pct = psserver->rng->Get();
        int money = entries[i]->min_money + (int)(pct * (float)(entries[i]->max_money - entries[i]->min_money));
        if (!lootTesting) chr->AddLootMoney(money);
      }
    }
}

psItemSpawnEvent::psItemSpawnEvent(psScheduledItem* item)
    : psGameEvent(0,item->MakeInterval(),"psItemSpawnEvent")
{
    if(item->WantToDie())
        return;

    schedule = item;
    Notify3(LOG_SPAWN,"Spawning item (%u) in %d",item->GetItemID(),triggerticks -csGetTicks());
}

psItemSpawnEvent::~psItemSpawnEvent()
{
    delete schedule;
}


void psItemSpawnEvent::Trigger()
{
    if(schedule->WantToDie())
        return;

    if(schedule->CreateItem() == NULL)
    {
        CPrintf(CON_ERROR,"Couldn't spawn item %u",schedule->GetItemID());
    }
}

LootRandomizer::LootRandomizer()
{
    prefix_max = 0;
    adjective_max = 0;
    suffix_max = 0;

    // Find any math scripts that are needed
    modifierCostCalc = psserver->GetMathScriptEngine()->FindScript("LootModifierCostCap");
}

LootRandomizer::~LootRandomizer()
{
    LootModifier *e;
    while (prefixes.GetSize())
    {
        e = prefixes.Pop();
        delete e;
    }
    while (suffixes.GetSize())
    {
        e = suffixes.Pop();
        delete e;
    }
    while (adjectives.GetSize())
    {
        e = adjectives.Pop();
        delete e;
    }
}

void LootRandomizer::AddLootModifier(LootModifier *entry)
{
    if (entry->modifier_type.CompareNoCase("prefix"))
    {
        prefixes.Push( entry );
        if (entry->probability > prefix_max) 
        {
            prefix_max = entry->probability;
        }
    }
    else if (entry->modifier_type.CompareNoCase("suffix"))
    {
        suffixes.Push( entry );
        if (entry->probability > suffix_max)
        {
            suffix_max = entry->probability;
        }
    }
    else if (entry->modifier_type.CompareNoCase("adjective"))
    {
        adjectives.Push( entry );
        if (entry->probability > adjective_max)
        {
            adjective_max = entry->probability;
        }
    }
}

psItemStats* LootRandomizer::RandomizeItem( psItemStats* itemstats, float maxcost, bool lootTesting, size_t numModifiers )
{
    uint32_t rand;
    psItemStats *newStats;
    csString modifierType;
    csArray< csString > selectedModifierTypes;
    LootModifier totalMods;
    float totalCost = itemstats->GetPrice().GetTrias();

    // Set up ModifierTypes
    // The Order of the modifiers is significant. It determines the priority of the modifiers, currently this is
    // Suffixes, Prefixes, Adjectives : So we add them in reverse order so the highest priority is applied last
    selectedModifierTypes.Push( "suffix" );
    selectedModifierTypes.Push( "prefix" );
    selectedModifierTypes.Push( "adjective" );

    // Get Defaults from Stats
    totalMods.cost_modifier = 1;
    totalMods.name = itemstats->GetName();

    // Determine Probability of number of modifiers ( 0-3 )
    if (!lootTesting)
    {
        rand = psserver->rng->Get( 100 ); // Range of 0 - 99
        if ( rand < 1 ) // 1% chance
            numModifiers = 3;
        else if ( rand < 8 ) // 7% chance
            numModifiers = 2;
        else if ( rand < 30 ) // 22% chance
            numModifiers = 1;
        else // 70% chance
            numModifiers = 0;
    }

    // If there are no additional modifiers return original stats
    if ( numModifiers == 0 ) 
        return itemstats;

    if ( numModifiers != 3 )
    {
        while ( selectedModifierTypes.GetSize() != numModifiers )
        {
            rand = psserver->rng->Get( 99 );
            if ( rand < 60 )
                selectedModifierTypes.Delete( "suffix" ); // higher chance to be removed
            else if ( rand < 85 )
                selectedModifierTypes.Delete( "prefix" );
            else
                selectedModifierTypes.Delete( "adjective" ); // lower chance to be removed
        }
    }

  // for each modifiertype roll a dice to see which modifier we get
    while ( selectedModifierTypes.GetSize() != 0 )
    {
        modifierType = selectedModifierTypes.Pop();
        int newModifier, probability;
	int max_probability = 0;
        LootModifier *lootModifier = NULL;
        csArray<LootModifier *> *modifierList = NULL;

        if (modifierType.CompareNoCase("prefix"))
        {
            modifierList = &prefixes; 
            max_probability=(int)prefix_max;
        }
        else if (modifierType.CompareNoCase("suffix"))
        {
            modifierList = &suffixes;
            max_probability=(int)suffix_max;
        }
        else if (modifierType.CompareNoCase("adjective"))
        {
            modifierList = &adjectives;
            max_probability=(int)adjective_max;
        }

        // Get min probability <= probability <= max probability in modifiers list
        //probability = psserver->rng->Get( (int)((*modifierList)[ modifierList->Length() - 1 ]->probability - (int) (*modifierList)[0]->probability ) + 1) + (int) (*modifierList)[0]->probability;
        probability = psserver->rng->Get( max_probability );
        for ( newModifier = (int)modifierList->GetSize() - 1; newModifier >= 0 ; newModifier-- )
        {
            float item_prob = ((*modifierList)[newModifier]->probability);
            if ( probability >=  item_prob)
            {
                if ( maxcost >= totalCost * (*modifierList)[newModifier]->cost_modifier ||
                    lootTesting )
                {
                    lootModifier = (*modifierList)[ newModifier ];
                    totalCost = totalCost * (*modifierList)[newModifier]->cost_modifier;
                    break;
                }
            }
        }

        // if just testing loot randomizing, then dont want equip/dequip events
        //if (lootTesting && lootModifier)
        //{
        //    lootModifier->prg_evt_equip.Empty();
        //}

        if(lootModifier) AddModifier( &totalMods, lootModifier );
    }

    // Check if item already present
    newStats = CacheManager::GetSingleton().GetBasicItemStatsByName( totalMods.name.GetData() );
    if ( !newStats ) // If it doesn't exist
    {
        // This will copy the current ItemsStats row in the DB and create a new one for us.
        // TODO: rewrite loot handling to not generate a bazillion item_stats entries
        newStats = CacheManager::GetSingleton().CopyItemStats(itemstats->GetUID(), totalMods.name);
        if ( !newStats )
        {
            return itemstats;
        }

        // Apply All Changes
        ApplyModifier(newStats, &totalMods);

        // Save Changes and return
        newStats->Save();
    }
    return newStats;
}

void LootRandomizer::AddModifier( LootModifier *oper1, LootModifier *oper2 )
{
    csString newName;
    // Change Name
    if (oper2->modifier_type.CompareNoCase("prefix"))
    {
        newName.Append( oper2->name );
        newName.Append( " " );
        newName.Append( oper1->name );
    }
    else if (oper2->modifier_type.CompareNoCase("suffix"))
    {
        newName.Append( oper1->name );
        newName.Append( " " );
        newName.Append( oper2->name );
    }
    else if (oper2->modifier_type.CompareNoCase("adjective"))
    {
        newName.Append( oper2->name );
        newName.Append( " " );
        newName.Append( oper1->name );
    }
    oper1->name = newName;

    // Adjust Price
    oper1->cost_modifier *= oper2->cost_modifier;
    oper1->effect.Append( oper2->effect );
    oper1->mesh = oper2->mesh;
    oper1->stat_req_modifier.Append( oper2->stat_req_modifier );

    // equip script
    oper1->equip_script.Append(oper2->equip_script);
}

void LootRandomizer::ApplyModifier( psItemStats* loot, LootModifier* mod)
{

    loot->SetName( mod->name );
    loot->SetPrice( (int)( loot->GetPrice().GetTrias() * mod->cost_modifier ) );
    if ( mod->mesh.Length() > 0 )
        loot->SetMeshName( mod->mesh );    

    // Apply effect
    csString xmlItemMod;

    xmlItemMod.Append( "<ModiferEffects>" );
    xmlItemMod.Append( mod->effect );
    xmlItemMod.Append( "</ModiferEffects>" );

    // Read the ModiferEffects XML into a doc
    csRef<iDocument> xmlDoc = ParseString( xmlItemMod );
    if(!xmlDoc)
    {
        Error1("Parse error in Loot Randomizer");
        return;
    }
    csRef<iDocumentNode> root    = xmlDoc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Loot Randomizer");
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("ModiferEffects");//Are we sure it is "Modifer"?
    if(!topNode)
    {
        Error1("No <ModiferEffects> in Loot Randomizer");
        return;
    }

    csRef<iDocumentNodeIterator> nodeList = topNode->GetNodes("ModiferEffect");

  // For Each ModiferEffect
    csRef<iDocumentNode> node;
    while ( nodeList->HasNext() )
    {
        node = nodeList->Next();
        //    Determine the Effect
        csString EffectOp = node->GetAttribute("operation")->GetValue();
        csString EffectName = node->GetAttribute("name")->GetValue();
        float EffectValue = node->GetAttribute("value")->GetValueAsFloat();
        //        Add to the Attributes
        if (!loot->SetAttribute(EffectOp, EffectName, EffectValue))
        {
            // display error and continue
            Error2("Unable to set attribute %s on new loot item.",EffectName.GetData());
        }
        //    Loop
        node = nodeList->Next();
    }

    // Apply stat_req_modifier
    csString xmlStatReq;

    xmlStatReq.Append( "<StatReqs>" );
    xmlStatReq.Append( mod->stat_req_modifier );
    xmlStatReq.Append( "</StatReqs>" );

    // Read the Stat_Req XML into a doc
    xmlDoc = ParseString( xmlStatReq );
    if(!xmlDoc)
    {
        Error1("Parse error in Loot Randomizer");
        return;
    }
    root    = xmlDoc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Loot Randomizer");
        return;
    }
    topNode = root->GetNode("StatReqs");
    if(!topNode)
    {
        Error1("No <statreqs> in Loot Randomizer");
        return;
    }

    nodeList = topNode->GetNodes("StatReq");
    // For Each Stat_Req
    while ( nodeList->HasNext() )
    {
        node = nodeList->Next();
        //        Determine the STAT
        csString StatName = node->GetAttribute("name")->GetValue();
        float StatValue = node->GetAttribute("value")->GetValueAsFloat();
        //        Add to the Requirements
        if (!loot->SetRequirement(StatName, StatValue))
        {
            // Too Many Requirements, display error and continue
            Error2("Unable to set requirement %s on new loot item.",StatName.GetData());
        }
        //    Loop
        node = nodeList->Next();
    }

    // Apply equip script
    if (!mod->equip_script.IsEmpty())
    {
        csString scriptXML;
        scriptXML.Format("<apply aim=\"Actor\" name=\"%s\" type=\"buff\">%s</apply>", mod->name.GetData(), mod->equip_script.GetData());
        loot->SetEquipScript(scriptXML);
    }
}

float LootRandomizer::CalcModifierCostCap(psCharacter *chr)
{
    // Use LootCostCap script to calculate loot modifier cost cap
    if( !modifierCostCalc )
    {
        CPrintf(CON_ERROR,"Couldn't load loot cost cap script!");
        return 1000.0;
    }

    // Use the mob's attributes to calculate modifier cost cap
    MathEnvironment env;
    env.Define("Str",     chr->Stats()[PSITEMSTATS_STAT_STRENGTH].Current());
    env.Define("End",     chr->Stats()[PSITEMSTATS_STAT_ENDURANCE].Current());
    env.Define("Agi",     chr->Stats()[PSITEMSTATS_STAT_AGILITY].Current());
    env.Define("Int",     chr->Stats()[PSITEMSTATS_STAT_INTELLIGENCE].Current());
    env.Define("Will",    chr->Stats()[PSITEMSTATS_STAT_WILL].Current());
    env.Define("Cha",     chr->Stats()[PSITEMSTATS_STAT_CHARISMA].Current());
    env.Define("MaxHP",   chr->GetMaxHP().Current());
    env.Define("MaxMana", chr->GetMaxMana().Current());

    modifierCostCalc->Evaluate(&env);
    MathVar *modcap = env.Lookup("ModCap");
    Debug2(LOG_LOOT,0,"DEBUG: Calculated cost cap %f\n", modcap->GetValue());
    return modcap->GetValue();
}


