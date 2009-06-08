/*
* workmanager.cpp
*
* Copyright (C) 2001-2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <math.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"

#include "util/eventmanager.h"
#include "util/serverconsole.h"
#include "util/psdatabase.h"
#include "util/mathscript.h"

#include "bulkobjects/dictionary.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psglyph.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psmerchantinfo.h"
#include "bulkobjects/psnpcdialog.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/pstrade.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psguildinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "workmanager.h"
#include "gem.h"
#include "netmanager.h"
#include "psserver.h"
#include "psserverchar.h"
#include "playergroup.h"
#include "entitymanager.h"
#include "weathermanager.h"
#include "cachemanager.h"
#include "globals.h"
#include "adminmanager.h"

//#define DEBUG_WORKMANAGER         // debugging only
//#define NO_RANDOM_QUALITY         // do not apply randomness to calculations

/*
 *  There are four types of work that can be done:
 *  1. Manufacturing work harvests raw materials.
 *  2. Production work creates new items from raw materials.
 *  3. Lock picking opens locked containers.
 *  4. Cleanup work removes the ownership of abandoned items.
 *
 *  Go to each section of the code for a full description of each process.
**/


//  To add a new constraint you need to:
//    put a reference to the new constraint function in this struct with constraint string and player message
//    declare a new constraint function in the header like: static bool constraintNew();
//    code the constaint function elsewhere in this module
const constraint constraints[] =
{
    // Time of day.
    // Parameter: hh; where hh is hour of 24 hour clock.
    // Example: TIME(12) is noon.
    {WorkManager::constraintTime, "TIME", "You can not do this work at this time of the day!"},

    // People in area.
    // Parameter: n,r; where n is number of people and r is the range.
    // Example: FRIENDS(6,4) is six people within 4.
    {WorkManager::constraintFriends, "FRIENDS", "You need more people for this work!"},

    // Location of player.
    // Parameter: s,x,y,z,r; where s is sector, x is x-coord, y is y-coord, z is z-coord, and r is rotation.
    // Example: LOCATION(3,-10.53,176.36,,) is at [-10.53,176.36] any hight and any direction in sector 3.
    {WorkManager::constraintLocation, "LOCATION","You can not do this work here!"},

    // Player mode.
    // Parameter: mode; where mode is psCharacter mode string.
    // Example: MODE(sitting) is player needs to be sitting as work is started.
    {WorkManager::constraintMode, "MODE","You are not in the right position to complete this work!"},

    // Player gender.
    // Parameter: gender; where gender is psCharacter's gender.
    // Example: GENDER(F) is player needs to be female as work is completed.
    {WorkManager::constraintGender, "GENDER","You are not in the right gender to complete this work!"},

    // Player race.
    // Parameter: race; where race is psCharacter's race.
    // Example: RACE(ylianm) is player needs to be ylianm as work is completed.
    {WorkManager::constraintRace, "RACE","You do not have right racial background to complete this work!"},

    // Array end.
    {NULL, "", ""}
};

//-----------------------------------------------------------------------------

WorkManager::WorkManager()
{
    currentQuality = 1.00;

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<WorkManager>(this,&WorkManager::HandleWorkCommand),MSGTYPE_WORKCMD,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<WorkManager>(this,&WorkManager::HandleLockPick),MSGTYPE_LOCKPICK,REQUIRE_READY_CLIENT|REQUIRE_ALIVE|REQUIRE_TARGET);

    calc_repair_rank    = psserver->GetMathScriptEngine()->FindScript("Calculate Repair Rank");
    calc_repair_time    = psserver->GetMathScriptEngine()->FindScript("Calculate Repair Time");
    calc_repair_result  = psserver->GetMathScriptEngine()->FindScript("Calculate Repair Result");
    calc_repair_quality = psserver->GetMathScriptEngine()->FindScript("Calculate Repair Quality");
    calc_repair_exp     = psserver->GetMathScriptEngine()->FindScript("Calculate Repair Experience");
    calc_mining_chance  = psserver->GetMathScriptEngine()->FindScript("Calculate Mining Odds");
    calc_mining_exp     = psserver->GetMathScriptEngine()->FindScript("Calculate Mining Experience");
    calc_transform_exp   = psserver->GetMathScriptEngine()->FindScript("Calculate Trasformation Experience");
    calc_lockpick_time  = psserver->GetMathScriptEngine()->FindScript("Lockpicking Time");

    if (!calc_repair_rank)
    {
        Error1("Could not find mathscript 'Calculate Repair Rank'");
    }
    if (!calc_repair_time)
    {
        Error1("Could not find mathscript 'Calculate Repair Time'");
    }
    if (!calc_repair_result)
    {
        Error1("Could not find mathscript 'Calculate Repair Result'");
    }
    if (!calc_repair_quality)
    {
        Error1("Could not find mathscript 'Calculate Repair Quality'");
    }
    if (!calc_repair_exp)
    {
        Error1("Could not find mathscript 'Calculate Repair Experience'");
    }
    if (!calc_mining_chance)
    {
        Error1("Could not find mathscript 'Calculate Mining Odds'");
    }
    if (!calc_mining_exp)
    {
        Error1("Could not find mathscript 'Calculate Mining Experience'");
    }
    if (!calc_transform_exp)
    {
        Error1("Could not find mathscript 'Calculate Trasformation Experience'");
    }
    if (!calc_lockpick_time)
    {
        Error1("Could not find mathscript 'Lockpicking Time'");
    }

    Initialize();
};


WorkManager::~WorkManager()
{
    if (psserver->GetEventManager())
    {
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_WORKCMD);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_LOCKPICK);
    }
}

void WorkManager::Initialize()
{
    Result res(db->Select("select * from natural_resources"));

    if (res.IsValid())
    {
        for (unsigned int i=0; i<res.Count(); i++)
        {
            NaturalResource *nr = new NaturalResource;

            nr->sector = res[i].GetInt("loc_sector_id");
            nr->loc.x  = res[i].GetFloat("loc_x");
            nr->loc.y  = res[i].GetFloat("loc_y");
            nr->loc.z  = res[i].GetFloat("loc_z");
            nr->radius = res[i].GetFloat("radius");
            nr->visible_radius = res[i].GetFloat("visible_radius");
            nr->probability = res[i].GetFloat("probability");
            nr->skill = CacheManager::GetSingleton().GetSkillByID( res[i].GetInt("skill") );
            nr->skill_level = res[i].GetInt("skill_level");
            nr->item_cat_id = res[i].GetUInt32("item_cat_id");
            nr->item_quality = res[i].GetFloat("item_quality");
            nr->anim = res[i]["animation"];
            nr->anim_duration_seconds = res[i].GetInt("anim_duration_seconds");
            nr->reward = res[i].GetInt("item_id_reward");
            nr->reward_nickname = res[i]["reward_nickname"];
            nr->action = res[i]["action"];

            resources.Push(nr);
        }
    }
    else
    {
        Error2("Database error loading natural_resources: %s\n",
               db->GetLastError() );
    }
}

void WorkManager::HandleWorkCommand(MsgEntry* me,Client *client)
{
    psWorkCmdMessage msg(me);

    if (!msg.valid)
    {
        psserver->SendSystemError(me->clientnum,"Invalid work command.");
        return;
    }

    if (msg.command == "/use" )
    {
        HandleUse(client);
    }
    else if (msg.command == "/combine")
    {
        HandleCombine(client);
    }
    else if (msg.command == "/dig")
    {
        HandleProduction(client,"dig",msg.filter);
    }
    else if (msg.command == "/fish")
    {
        HandleProduction(client, "fish", msg.filter);
    }
    else if (msg.command == "/repair")
    {
        HandleRepair(client, msg);
    }
    else if (msg.command == "/construct")
    {
        HandleConstruct(client);
    }
}

void WorkManager::HandleLockPick(MsgEntry* me,Client *client)
{
    gemObject* target = client->GetTargetObject();

    // Check if target is action item
    gemActionLocation* gemAction = dynamic_cast<gemActionLocation*>(target);
    if(gemAction) {
        psActionLocation *action = gemAction->GetAction();

        // Check if the actionlocation is linked to real item
        InstanceID InstanceID = action->GetInstanceID();
        if( InstanceID!= INSTANCE_ALL )
        {
            target = GEMSupervisor::GetSingleton().FindItemEntity( InstanceID );
        }
    }

    // Check target gem
    if (!target)
    {
        Error1("No gemItem target!\n");
        return;
    }

    // Check range ignoring Y co-ordinate
    if (client->GetActor()->RangeTo(target, true) > RANGE_TO_USE)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You need to be closer to the lock to try this.");
        return;
    }

    // Get item
    psItem* item = target->GetItem();
    if ( !item )
    {
        Error1("Found gemItem but no psItem was attached!\n");
        return;
    }

    StartLockpick(client,item);
}


//-----------------------------------------------------------------------------
// Repair
//-----------------------------------------------------------------------------

void WorkManager::HandleRepair(Client *client, psWorkCmdMessage &msg)
{
    // Make sure client isn't already busy digging, etc.
    if ( client->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE )
    {
        psserver->SendSystemError(client->GetClientNum(),"You cannot repair anything because you are %s.", client->GetActor()->GetModeStr());
        return;
    }

    // No stamina checking yet for repairs.  Never discussed.

    // Check for repairable item in precised or default(right hand) slot
    int slotTarget;
    if ( msg.repairSlotName.IsEmpty() )
        slotTarget = PSCHARACTER_SLOT_RIGHTHAND;
    else
        slotTarget = CacheManager::GetSingleton().slotNameHash.GetID(msg.repairSlotName);

    psItem *repairTarget = client->GetCharacterData()->Inventory().GetInventoryItem((INVENTORY_SLOT_NUMBER)slotTarget);
    if (repairTarget==NULL)
    {
        if(slotTarget == -1)
        {
            psserver->SendSystemError(client->GetClientNum(),"The Slot %s doesn't exists.", msg.repairSlotName.GetData() );
        }
        else if(msg.repairSlotName.IsEmpty())
        {
            psserver->SendSystemError(client->GetClientNum(),"The Default Slot (Right Hand) is empty.", msg.repairSlotName.GetData() );
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(),"The Slot %s is empty.", msg.repairSlotName.GetData() );
        }
        return;
    }
    if (repairTarget->GetItemQuality() >= repairTarget->GetMaxItemQuality() )
    {
        psserver->SendSystemError(client->GetClientNum(),"Your %s is in perfect condition.", repairTarget->GetName() );
        return;
    }

    // Check for required repair kit item in any inventory slot
    int toolstatid     = repairTarget->GetRequiredRepairTool();
    psItemStats *tool  = CacheManager::GetSingleton().GetBasicItemStatsByID(toolstatid);
    psItem *repairTool = NULL;
    if (tool)
    {
        size_t index = client->GetCharacterData()->Inventory().FindItemStatIndex(tool);
        if (index == SIZET_NOT_FOUND)
        {
            psserver->SendSystemError(client->GetClientNum(),"You must have a %s to repair your %s.", tool->GetName(), repairTarget->GetName() );
            return;
        }
        repairTool = client->GetCharacterData()->Inventory().GetInventoryIndexItem(index);
    }
    else
    {
         psserver->SendSystemError(client->GetClientNum(),"The %s cannot be repaired!", repairTarget->GetName() );
         return;
    }

    // Calculate if current skill is enough to repair the item
    int rankneeded;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        calc_repair_rank->Evaluate(&env);
        rankneeded = env.Lookup("Result")->GetValue();
    }

    int skillid = repairTarget->GetBaseStats()->GetCategory()->repairSkillId;
    int repairskillrank = client->GetCharacterData()->Skills().GetSkillRank(PSSKILL(skillid)).Current();
    if (repairskillrank<rankneeded)
    {
        psserver->SendSystemError(client->GetClientNum(),"This item is too complex for your current repair skill. You cannot repair it." );
        return;
    }

    // If skill=0, check if it has at least theoretical training in that skill
    if (repairskillrank == 0 && client->GetCharacterData()->Skills().Get(PSSKILL(skillid)).CanTrain())
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You don't have the skill to repair your %s.",repairTarget->GetName());
        return;
    }

    // Calculate time required for repair based on item and skill level
    csTicks repairDuration;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        env.Define("Worker", client->GetCharacterData());
        calc_repair_time->Evaluate(&env);
        repairDuration = (csTicks)(env.Lookup("Result")->GetValue() * 1000); // convert secs to msec
    }

    // Calculate result after repair
    float repairResult;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        env.Define("Worker", client->GetCharacterData());
        calc_repair_result->Evaluate(&env);
        repairResult = env.Lookup("Result")->GetValue();
    }

    // Queue time event to trigger when repair is complete, if not canceled.
    csVector3 dummy = csVector3(0,0,0);

    psWorkGameEvent *evt = new psWorkGameEvent(this, client->GetActor(),repairDuration,REPAIR,dummy,NULL,client,repairTarget,repairResult);
    psserver->GetEventManager()->Push(evt);  // wake me up when repair is done

    client->GetActor()->SetTradeWork(evt);

    repairTarget->SetInUse(true);

    psserver->SendSystemInfo(client->GetClientNum(),"You start repairing the %s and continue for %d seconds.",repairTarget->GetName(),repairDuration/1000);
    client->GetActor()->SetMode(PSCHARACTER_MODE_WORK);
}

void WorkManager::HandleRepairEvent(psWorkGameEvent* workEvent)
{
    psItem *repairTarget = workEvent->object;

    // We're done work, clear the mode
    workEvent->client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
    repairTarget->SetInUse(false);
    // Check for presence of required tool
    int toolstatid     = repairTarget->GetRequiredRepairTool();
    psItemStats *tool  = CacheManager::GetSingleton().GetBasicItemStatsByID(toolstatid);
    psItem *repairTool = NULL;
    if (tool)
    {
        size_t index = workEvent->client->GetCharacterData()->Inventory().FindItemStatIndex(tool);
        if (index == SIZET_NOT_FOUND)
        {
            psserver->SendSystemError(workEvent->client->GetClientNum(),"You must have a %s to repair your %s.", tool->GetName(), repairTarget->GetName() );
            return;
        }
        repairTool = workEvent->client->GetCharacterData()->Inventory().GetInventoryIndexItem(index);
        if (repairTool == repairTarget)
        {
            psserver->SendSystemError(workEvent->client->GetClientNum(), "You can't use your repair tool on itself.");
            return;
        }
    }

    // Now consume the item if we need to
    if (repairTarget->GetRequiredRepairToolConsumed() && repairTool)
    {
        // Always assume 1 item is consumed, not stacked
        psItem *item = workEvent->client->GetCharacterData()->Inventory().RemoveItemID(repairTool->GetUID(),1);
        if (item)
        {
            // This message must come first because item is about to be deleted.
            psserver->SendSystemResult(workEvent->client->GetClientNum(),
                                       "You used a %s in your repair work.",
                                       item->GetName());

            CacheManager::GetSingleton().RemoveInstance(item);
            psserver->GetCharManager()->UpdateItemViews( workEvent->client->GetClientNum() );
        }
    }
    else
    {
        // TODO: Implement decay of quality of repair tool here if not consumed.
    }

    // Calculate resulting qualities after repair
    float resultQuality;
    float resultMaxQuality;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        env.Define("Worker", workEvent->client->GetCharacterData());
        env.Define("RepairAmount", workEvent->repairAmount);
        calc_repair_quality->Evaluate(&env);
        resultMaxQuality = env.Lookup("ResultMaxQ")->GetValue();
        resultQuality = env.Lookup("ResultQ")->GetValue();
    }

    repairTarget->SetItemQuality(resultQuality);
    repairTarget->SetMaxItemQuality(resultMaxQuality);

    psserver->SendSystemResult(workEvent->client->GetClientNum(),
                               "You have repaired your %s to %.0f out of %.0f",
                               repairTarget->GetName(),
                               repairTarget->GetItemQuality(),
                               repairTarget->GetMaxItemQuality());

    // calculate practice points

    int practicePoints;
    float modifier;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        env.Define("Worker", workEvent->client->GetCharacterData());
        env.Define("RepairAmount", workEvent->repairAmount);
        calc_repair_exp->Evaluate(&env);
        practicePoints   = env.Lookup("ResultPractice")->GetValue();
        modifier = env.Lookup("ResultModifier")->GetValue();
    }

    int skillid = repairTarget->GetBaseStats()->GetCategory()->repairSkillId;
    //assigns points and exp
    workEvent->client->GetCharacterData()->CalculateAddExperience((PSSKILL)skillid, practicePoints, modifier);

    repairTarget->Save(false);
}



//-----------------------------------------------------------------------------
// Production
//-----------------------------------------------------------------------------

void WorkManager::HandleProduction(Client *client,const char *type,const char *reward)
{
    if ( !LoadLocalVars(client) )
    {
        return;
    }

    int mode = client->GetActor()->GetMode();

    // Make sure client isn't already busy digging, etc.
    if ( mode != PSCHARACTER_MODE_PEACE )
    {
        psserver->SendSystemError(client->GetClientNum(),"You cannot %s because you are already busy.",type);
        return;
    }

    // check stamina
    if ( !CheckStamina( client->GetCharacterData() ) )
    {
        psserver->SendSystemError(client->GetClientNum(),"You cannot %s because you are too tired.",type);
        return;
    }

    // Make sure they specified a resource type
    if (reward == NULL || !strcmp(reward,""))
    {
        psserver->SendSystemError(client->GetClientNum(),"Please specify which resource you want to %s for. (/%s <some resource>)",type,type);
        return;
    }

    csVector3 pos;

    // Make sure they are not in the same loc as the last dig.
    client->GetActor()->GetLastProductionPos(pos);
    if (SameProductionPosition(client->GetActor(), pos))
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You cannot %s in the same place twice in a"
                                  " row.", type);
        return;
    }

    iSector *sector;

    client->GetActor()->GetPosition(pos, sector);

    NaturalResource *nr = FindNearestResource(reward,sector,pos,type);
    if (!nr)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You don't see a good place to %s.",type);
        return;
    }

    // Validate the skill
    // If skill == 0 and can still be trained it's not possible to execute this operation
    if (client->GetCharacterData()->Skills().GetSkillRank((PSSKILL)nr->skill->id).Current() == 0 &&
        client->GetCharacterData()->Skills().Get((PSSKILL)nr->skill->id).CanTrain())
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You don't have the skill to %s for %s.",type,reward);
        return;
    }

    // Validate category of equipped item
    psItem *item = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if (!item || nr->item_cat_id != item->GetCategory()->id)
    {
        psserver->SendSystemError(client->GetClientNum(),"You don't have a good tool to %s with, equipped in your right hand.",type);
        return;
    }

    // Calculate time required
    int time_req = nr->anim_duration_seconds;

    // Send anim and confirmation message to client and nearby people
    psOverrideActionMessage msg(client->GetClientNum(),client->GetActor()->GetEID(),nr->anim,nr->anim_duration_seconds);
    psserver->GetEventManager()->Multicast(msg.msg, client->GetActor()->GetMulticastClients(),0,PROX_LIST_ANY_RANGE);
    client->GetActor()->SetMode(PSCHARACTER_MODE_WORK);

    client->GetCharacterData()->SetStaminaRegenerationWork(nr->skill->id);
    client->GetActor()->SetProductionStartPos(pos);

    // Queue up game event for success
    psWorkGameEvent *ev = new psWorkGameEvent(this,client->GetActor(),time_req*1000,PRODUCTION,pos,nr,client);
    psserver->GetEventManager()->Push(ev);  // wake me up when digging is done

    client->GetActor()->SetTradeWork(ev);

    psserver->SendSystemInfo(client->GetClientNum(),"You start to %s.",type);
}

// Function used by super client
void WorkManager::HandleProduction(gemActor *actor,const char *type,const char *reward)
{
    int mode = actor->GetMode();

    // Make sure client isn't already busy digging, etc.
    if ( mode != PSCHARACTER_MODE_PEACE )
    {
        // psserver->SendSystemError(client->GetClientNum(),"You cannot %s because you are already busy.",type);
        Warning3(LOG_SUPERCLIENT,"%s cannot %s because you are already busy.",actor->GetName(),type);
        return;
    }

    // check stamina
    if ( !CheckStamina( actor->GetCharacterData() ) )
    {
        // psserver->SendSystemError(client->GetClientNum(),"You cannot %s because you are too tired.",type);
        Warning3(LOG_SUPERCLIENT,"%s cannot %s because you are too tired.",actor->GetName(),type);
        return;
    }


    csVector3 pos;
    iSector *sector;

    actor->GetPosition(pos, sector);

    NaturalResource *nr = FindNearestResource(reward,sector,pos,type);
    if (!nr)
    {
        // psserver->SendSystemInfo(client->GetClientNum(),"You don't see a good place to %s.",type);
        Warning4(LOG_SUPERCLIENT,"%s doesn't see a good place to %s for %s.",actor->GetName(),type,reward);
        return;
    }

    // Validate the skill
    // If skill == 0 and can still be trained it's not possible to execute this operation
    if (actor->GetCharacterData()->Skills().GetSkillRank((PSSKILL)nr->skill->id).Current() == 0 &&
        actor->GetCharacterData()->Skills().Get((PSSKILL)nr->skill->id).CanTrain())
    {
        //psserver->SendSystemInfo(client->GetClientNum(),"You don't have the skill to %s for %s.",type,reward);
        Warning6(LOG_SUPERCLIENT,"%s(%s) don't have the skill(%d) to %s for %s.",
                 actor->GetName(), ShowID(actor->GetEID()), nr->skill->id, type, reward);
        return;
    }

    // Validate category of equipped item
    psItem *item = actor->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if (!item || nr->item_cat_id != item->GetCategory()->id)
    {
        // psserver->SendSystemError(client->GetClientNum(),"You don't have a good tool to %s with, equipped in your right hand.",type);
        Warning3(LOG_SUPERCLIENT,"%s don't have a good tool to %s with, equipped in right hand.",actor->GetName(),type);
        return;
    }

    // Calculate time required
    int time_req = nr->anim_duration_seconds;

    // Send anim and confirmation message to client and nearby people
    psOverrideActionMessage msg(0,actor->GetEID(),nr->anim,nr->anim_duration_seconds);
    psserver->GetEventManager()->Multicast(msg.msg, actor->GetMulticastClients(),0,PROX_LIST_ANY_RANGE);

    actor->SetMode(PSCHARACTER_MODE_WORK);

    actor->GetCharacterData()->SetStaminaRegenerationWork(nr->skill->id);
    actor->SetProductionStartPos(pos);

    // Queue up game event for success
    psWorkGameEvent *ev = new psWorkGameEvent(this,actor,time_req*1000,PRODUCTION,pos,nr,NULL);
    psserver->GetEventManager()->Push(ev);  // wake me up when digging is done

    actor->SetTradeWork(ev);

    //psserver->SendSystemInfo(client->GetClientNum(),"You start to %s.",type);
    Debug4(LOG_SUPERCLIENT,0,"%s start to %s for %s",actor->GetName(),type,reward);
}


bool WorkManager::SameProductionPosition(gemActor *actor,
                                           const csVector3& startPos)
{
    csVector3 pos;
    iSector *sector;

    actor->GetPosition(pos, sector);

    return ((startPos - pos).SquaredNorm() < 1);
}

NaturalResource *WorkManager::FindNearestResource(const char *reward,iSector *sector, csVector3& pos, const char *action)
{
    NaturalResource *nr=NULL;

    psSectorInfo *playersector= CacheManager::GetSingleton().GetSectorInfoByName(sector->QueryObject()->GetName());
    int sectorid = playersector->uid;

    Debug2(LOG_TRADE,0, "Finding nearest resource for %s\n", reward);

    float mindist = 100000;
    for (size_t i=0; i<resources.GetSize(); i++)
    {
        NaturalResource *curr=resources[i];
        if (curr->sector==sectorid)
        {
            if (reward && curr->reward_nickname.CompareNoCase( reward ) &&
                curr->action.CompareNoCase(action))
            {
                csVector3 diff = curr->loc - pos;
                float dist = diff.Norm();
                // Update nr if dist is less than radius and closer than previus nr or nr isn't found yet
                if (dist < curr->visible_radius && (dist < mindist || nr == NULL))
                {
                    mindist = dist;
                    nr = curr;
                }
            }
        }
    }

    if (nr == NULL)
        Debug2(LOG_TRADE,0, "No resource found for %s\n", reward);

    return nr;
}

void WorkManager::HandleProductionEvent(psWorkGameEvent* workEvent)
{
    if (!workEvent->worker.IsValid()) // Worker has disconnected
        return;

    // Make sure clients are in the same loc as they started to dig.
    if (workEvent->client && !SameProductionPosition(workEvent->client->GetActor(),
                                workEvent->client->GetActor()->GetProductionStartPos()))
    {
        psserver->SendSystemError(workEvent->client->GetClientNum(),
                                  "You were unsuccessful since you moved away "
                                  "from where you started.");

        // Actor isn't working anymore.
        workEvent->worker->SetMode(PSCHARACTER_MODE_PEACE);
        return;
    }

    psCharacter *workerchar = workEvent->worker->GetCharacterData();
    psItem* tool = workerchar->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if (!tool || workEvent->nr->item_cat_id != tool->GetCategory()->id)
    {
        if (workEvent->client)
        {
            psserver->SendSystemInfo(workEvent->worker->GetClientID(),"You were unsuccessful since you no longer have the tool.");
        }

        workEvent->worker->SetMode(PSCHARACTER_MODE_PEACE); // Actor isn't working anymore
        return;
    }

    float roll = psserver->GetRandom();

    // Get player skill value
    float cur_skill = workerchar->Skills().GetSkillRank((PSSKILL)workEvent->nr->skill->id).Current();

    // If skill=0, check if it has at least theoretical training in that skill
    if (cur_skill==0) {
        bool fullTrainingReceived = !workerchar->Skills().Get((PSSKILL)workEvent->nr->skill->id).CanTrain();
        if (fullTrainingReceived)
            cur_skill = 0.7F; // consider the skill somewhat usable
    }

    // Calculate complexity factor for skill
    float f1 = cur_skill / workEvent->nr->skill_level;
    if (f1 > 1.0) f1 = 1.0; // Clamp value 0..1

    // Calculate factor for tool quality
    float f2 = tool->GetItemQuality() / workEvent->nr->item_quality;
    if (f2 > 1.0) f2 = 1.0; // Clamp value 0..1

    // Calculate factor for distance from center of resource
    csVector3 diff = workEvent->nr->loc - workEvent->position;
    float dist = diff.Norm();
    float f3 = 1 - (dist / workEvent->nr->radius);
    if (f3 < 0.0) f3 = 0.0f; // Clamp value 0..1

    MathEnvironment env;
    env.Define("Distance",    f3);                         // Distance from mine to the actual mining
    env.Define("Probability", workEvent->nr->probability); // Probability of successful mining
    env.Define("Quality",     f2);                         // Quality of mining equipment
    env.Define("Skill",       f1);                         // Mining skill
    calc_mining_chance->Evaluate(&env);
    MathVar *varTotal = env.Lookup("Total");
    float total = varTotal->GetValue();

    psString debug;
    debug.AppendFmt( "Probability:     %1.3f\n",workEvent->nr->probability);
    debug.AppendFmt( "Skill Factor:    %1.3f\n",f1);
    debug.AppendFmt( "Quality Factor:  %1.3f\n",f2);
    debug.AppendFmt( "Distance Factor: %1.3f\n",f3);
    debug.AppendFmt( "Total Factor:    %1.3f\n",total);
    debug.AppendFmt( "Roll:            %1.3f\n",roll);
    if (workEvent->client)
    {
        Debug2(LOG_TRADE, workEvent->client->GetClientNum(), "%s", debug.GetData());
    }
    else
    {
        Debug2(LOG_TRADE, 0, "%s", debug.GetData());
    }


    if (roll < total)  // successful!
    {
        psItemStats *newitem = CacheManager::GetSingleton().GetBasicItemStatsByID(workEvent->nr->reward);
        if (!newitem)
        {
            Bug2("Natural Resource reward item #%d not found!\n",workEvent->nr->reward);
        }
        else
        {
            psItem *item = newitem->InstantiateBasicItem();
            if (!item)
            {
                Bug2("Failed instantizing reward item #%d!\n",workEvent->nr->reward);
                return;
            }

            if (!workerchar->Inventory().AddOrDrop(item))
            {
                Debug5(LOG_ANY, workerchar->GetPID().Unbox(), "HandleProductionEvent() could not give item of stat %u (%s) to character %s[%s])",
                    newitem->GetUID(), newitem->GetName(), workerchar->GetCharName(), ShowID(workerchar->GetPID()));

                if (workEvent->client)
                {
                    // Assume it's full
                    psserver->SendSystemInfo(workEvent->client->GetClientNum(),"You found %s, but you can't carry anymore of it so you dropped it", newitem->GetName() );
                }
                else
                {
                    Debug5(LOG_SUPERCLIENT,0,"%s(%s) found %s, but dropped it: %s",workEvent->worker->GetName(),
                           ShowID(workEvent->worker->GetEID()), newitem->GetName(), workerchar->Inventory().lastError.GetDataSafe());
                }

            }
            else
            {
                if (workEvent->client)
                {
                    psserver->GetCharManager()->UpdateItemViews(workEvent->client->GetClientNum());
                    psserver->SendSystemResult(workEvent->client->GetClientNum(),"You've got some %s!", newitem->GetName() );
                }
                else
                {
                    Debug4(LOG_SUPERCLIENT,0,"%s(%s) got some %s.",workEvent->worker->GetName(),
                           ShowID(workEvent->worker->GetEID()), newitem->GetName());
                }
            }

            item->SetLoaded();  // Item is fully created
            item->Save(false);    // First save

            // No matter if the item could be moved to inventory reset the last position and
            // give out skills.
            csVector3 pos;
            iSector* sector;

            workEvent->worker->GetPosition(pos, sector);

            // Store the mining position for the next check.
            if (workEvent->client)
            {
                workEvent->client->GetActor()->SetLastProductionPos(pos);
            }
        }
    }
    else
    {
        if (workEvent->client)
        {
            psserver->SendSystemInfo(workEvent->worker->GetClientID(),"You were not successful.");
        } 
        else
        {
            Debug2(LOG_SUPERCLIENT,0,"%s where not successful.",workEvent->worker->GetName());
        }
    }

    //Assign experience and practice points
    int practicePoints;
    int experiencePoints;
    float modifier;
    {
        MathEnvironment env;
        env.Define("Success", roll < total);
        env.Define("Worker", workEvent->client->GetCharacterData());
        env.Define("Probability", workEvent->nr->probability); // Probability of successful mining
        calc_mining_exp->Evaluate(&env);
        practicePoints   = env.Lookup("ResultPractice")->GetValue();
        modifier = env.Lookup("ResultModifier")->GetValue();
        MathVar *varResult = env.Lookup("Exp"); //optional variable
        if(varResult)
            experiencePoints = varResult->GetValue();
        else
            experiencePoints = 0;
    }


    // assign practice and experience
    if(practicePoints != 0)
        workEvent->client->GetCharacterData()->CalculateAddExperience((PSSKILL)workEvent->nr->skill->id, practicePoints, modifier);
    else
        workEvent->client->GetCharacterData()->AddExperiencePointsNotify(experiencePoints);

    workEvent->worker->SetMode(PSCHARACTER_MODE_PEACE); // Actor isn't working anymore
}


//-----------------------------------------------------------------------------
// Manufacture/cleanup
//-----------------------------------------------------------------------------

/*
*    There are four trade DB tables; trade_patterns, trade_processes, trade_transofrmations, and trade_combinations.
* The pattern data represents the intent of the work.  The patterns are divided into group patterns and the specific
* patterns that go with the design items that are placed in the mind slot.  The group patterns allow work to be associated
* with a group of patterns; This saves a lot of effort in creating individual transformations and combinations
* for similar patterns.  The process table holds all the common transform data, such as equipment, animations and skills.
* The transform table holds a list of transforms.  These are the starting item, quantity, and the result item and
* quantity. It also hold the duration of the trasform.  The combination table contains all the one to many items
* that make up a single combination.
*
*    As items are moved into and out of containers the entry functions are called to check
* if work can be done on the items.  Other entry functions are called when /combine or /use
* commands are issued.  Here are the different types of work that can be done:
*
* 1.    Command /use (targeted item) Specific types of items can be targeted and used to change items held
*       in the hand into other items.  Example:  Steel stock can be transformed into shield pieces by holding
*       hot stock in one hand and a hammer in the other while targeting an anvil.
* 2.    Command /use (targeted container) Containers can be targeted and used to change items contained into
*       other items.  Example:  Mechanical mixers can be targeted and used to mix large amounts of batter into dough.
* 3.    Command /combine (targeted container) Containers can be targeted and used to combine items into other items.
*       Example: Coal and iron ore can be combined in a foundry to make super sharp steel.
* 4.    Drag and drop Single items that are put into special auto-transformation containers can be changed into other
*       items.  Example: Heating iron ore in a forge creates iron stock.
* 5.    Issue a progression script to initiate craft tranformation.  Example:  Issue creat reagent spell.
*
*
* A set of member variables is used to guide the work process.  These are usually loaded at the start of the
*    entry functions, before validations functions are called to make sure the player meets the work requirements.
* There are four calls to IsTransformable().  The first is checking for a single transform that matches all the player
* item, tool, and skill criteria.  The second checks the same criteria looking for any possible group transform.  The
* next check is checking equipment and items for patternless transforms.  These transforms are possible without
* any item in the mind slot.  The transforms that allow any item to be transformed come next.  These are used in the
* oven for example where any item can be changed into dust.  Next the any item group transforms are checked.
* transform.  Finally the patternless any item transforms are checked.
*
* The IsTransformable() function will then loop through all the transforms associated with the current design item
*    and the current work item looking for a match.  It will check for required equipment, skills, quantities,
*    training, as well as any specific constraints made on doing the work.
* If a valid transformation is found StartTransformationEvent() is called that creates a work event and
*    returns control back.  A combination transformation causes the set of combining items into a temporary item
*    and then creates starts the transformation for the temporary item.
* Depending on the type of work being done more calls IsTransformable() might be made.  For example, when
*    issuing a /use command on an item both the right and left hand are checked for transformations.
* If no transformations are found an appropriate message is displayed to the user.
* Once the work event is fired skill points are collected and some calculations are done to determine any random
*    changes to the planned result.
* At this point the item is transformed into a different item and the user is informed of the change.
* If the transformation is caused by an auto-transform container then IsTransformable() is called again to
*    see if a second transformation needs to be started.
*
* Practice points are awarded only if skills are indicated in the transformation processes.  Experience
*    is awarded based on the difference of the original quality of the item and the resulting item's quality.
*
* Entry Points:
*    StartAutoWork - Item moved into container: SlotManager::MoveFromInventory()
*                  - Item stack split in container: SlotManager::MoveFromWorldContainer()
*    HandleUse - Client issues /use command from buttons: gemActiveObject::SendBehaviorMessage()
*              - Workmanager gets /use message: WorkManager::HandleMessage()
*    HandleCombine - Client issues /combine command from buttons: gemActiveObject::SendBehaviorMessage()
*                  - Workmanager gets /combine message: WorkManager::HandleMessage()
*    HandleWorkEvent - Work event triggers: psWorkGameEvent::Trigger()
*    StopWork - Items removed from containers: SlotManager::MoveFromWorldContainer();
*    StartScriptWork - Progression scripts pass pattern information: CraftOp::Run()
*
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing work
void WorkManager::StopWork(Client* client, psItem* item)
{
    // Assign the member vars
    if ( !LoadLocalVars(client) )
    {
        return;
    }

    // Check for bad item pointer
    if (!item)
    {
        Error1("Bad item pointer in StopWork");
        return;
    }

    // if no event for item then return
    psWorkGameEvent* curEvent = item->GetTransformationEvent();
    if( !curEvent )
    {
        return;
    }

    // Check for ending clean up event
    if (curEvent->category == CLEANUP)
    {
      Debug2(LOG_TRADE,clientNum, "Stopping cleaning up %s item.\n", item->GetName());
      StopCleanupWork(client, item);
    }
    else
    {
        Debug3(LOG_TRADE, clientNum, "Player %s stopped working on %s item.\n", ShowID(worker->GetPID()), item->GetName());

        // Handle all the different transformation types
        if( curEvent->GetTransformationType() == TRANSFORMTYPE_AUTO_CONTAINER )
        {
            StopAutoWork(client, item);
        }
        else
        {
            StopUseWork(client);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle /use command
void WorkManager::HandleUse(Client *client)
{
    // Assign the member vars
    if ( !LoadLocalVars(client) )
    {
        return;
    }

    //Check if we are starting or stopping use
    if ( owner->GetMode() != PSCHARACTER_MODE_WORK )
    {
        StartUseWork(client);
    }
    else
    {
        StopUseWork(client);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if possible to do some use work
void WorkManager::StartUseWork(Client* client)
{
    // Check to see if we have everything we need to do any trade work
    if (!ValidateWork())
    {
        return;
    }

    // Check to see if we have pattern
    if (!ValidateMind())
    {
        return;
    }

    // Check to see if player has stamina for work
    if (!ValidateStamina(client))
    {
        return;
    }

    // Check for any targeted item or container in hand
    if (!ValidateTarget(client))
    {
        return;
    }

    if ( workItem && workItem->GetIsContainer() ) // Check if the target is a container
    {
        // cast a gem container to iterate through
        gemContainer *container = dynamic_cast<gemContainer*> (workItem->GetGemObject());
        if (!container)
        {
            Error1("Could not instantiate gemContainer");
            return;
        }

        // Load item array from the container
        csArray<psItem*> itemArray;
        gemContainer::psContainerIterator it(container);
        while (it.HasNext())
        {
            // Only check the stuff that player owns
            psItem* item = it.Next();
            if ((item->GetGuardingCharacterID() == owner->GetPID())
                || (item->GetGuardingCharacterID() == 0))
            {
                itemArray.Push(item);
            }
        }

        // Check for too many items
        if (itemArray.GetSize() > 1 )
        {
            // Tell the player they are trying to work on too many items
            SendTransformError( clientNum, TRANSFORM_TOO_MANY_ITEMS );
            return;
        }

        // Only one item from container can be transformed at once
        if (itemArray.GetSize() == 1)
        {
            // Check to see if item can be transformed
            uint32 itemID = itemArray[0]->GetBaseStats()->GetUID();
            int count = itemArray[0]->GetStackCount();

            // Verify there is a valid transformation for the item that was dropped
            unsigned int transMatch = AnyTransform( patternId, groupPatternId, itemID, count );
            if (( transMatch == TRANSFORM_MATCH ) || (transMatch == TRANSFORM_GARBAGE ))
            {
                // Set up event for transformation
                StartTransformationEvent(
                    TRANSFORMTYPE_CONTAINER, PSCHARACTER_SLOT_NONE, count, itemArray[0]->GetItemQuality(), itemArray[0]);
                psserver->SendSystemOK(clientNum,"You start work on %d %s.", itemArray[0]->GetStackCount(), itemArray[0]->GetName());
                return;
            }
            else
            {
                // The transform could not be created so send the reason back to the user.
                SendTransformError( clientNum, transMatch, itemID, count );
                return;
            }
        }
        // nothing (owned by the player) in the container to use
        else
        {
            SendTransformError( clientNum, TRANSFORM_BAD_USE );
        }
    }
    else // check for in hand use
    {
        // Check if player has any transformation items in right hand
        psItem* rhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
        if ( rhand != NULL )
        {
            // Find out if we can do a transformation on items in hand
            uint32 handId = rhand->GetBaseStats()->GetUID();
            int handCount = rhand->GetStackCount();

            // Verify there is a valid transformation for the item that was dropped
            unsigned int rhandMatch = AnyTransform( patternId, groupPatternId, handId, handCount );
            if (( rhandMatch == TRANSFORM_MATCH ) || (rhandMatch == TRANSFORM_GARBAGE ))
            {
                // Set up event for transformation
                StartTransformationEvent(
                    TRANSFORMTYPE_SLOT, PSCHARACTER_SLOT_RIGHTHAND, handCount, rhand->GetItemQuality(), rhand);
                psserver->SendSystemOK(clientNum,"You start work on %d %s.", rhand->GetStackCount(), rhand->GetName());
                return;
            }
            else if (rhandMatch != TRANSFORM_UNKNOWN_ITEM)
            {
                // The transform could not be created so send the reason back to the user.
                SendTransformError( clientNum, rhandMatch, handId, handCount );
                return;
            }
        }

        // Check if player has any transformation items in left hand
        psItem* lhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
        if ( lhand != NULL )
        {
            // Find out if we can do a transformation on items in hand
            uint32 handId = lhand->GetBaseStats()->GetUID();
            int handCount = lhand->GetStackCount();

            // Verify there is a valid transformation for the item that was dropped
            unsigned int lhandMatch = AnyTransform( patternId, groupPatternId, handId, handCount );
            if (( lhandMatch == TRANSFORM_MATCH ) || (lhandMatch == TRANSFORM_GARBAGE ))
            {
                // Set up event for transformation
                StartTransformationEvent(
                    TRANSFORMTYPE_SLOT, PSCHARACTER_SLOT_LEFTHAND, handCount, lhand->GetItemQuality(), lhand);
                psserver->SendSystemOK(clientNum,"You start work on %d %s.", lhand->GetStackCount(), lhand->GetName());
                return;
            }
            else if (lhandMatch != TRANSFORM_UNKNOWN_ITEM)
            {
                // The transform could not be created so send the reason back to the user.
                SendTransformError( clientNum, lhandMatch, handId, handCount );
            }
        }

        // Since either hand can fail to transform normally send general message if we get to here
        SendTransformError( clientNum, TRANSFORM_MISSING_ITEM );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing use work
void WorkManager::StopUseWork(Client* client)
{
    // Check for any targeted item or container in hand
    if ( !ValidateTarget(client))
    {
        return;
    }

    // Kill the work event if it exists
    if( worker->GetMode() == PSCHARACTER_MODE_WORK )
    {
        worker->SetMode(PSCHARACTER_MODE_PEACE);
        psserver->SendSystemOK(clientNum,"You stop working.");
    }
}

////////////////////////////////////////////////////////////////////////
// Handle /combine command
void WorkManager::HandleCombine(Client *client)
{
    // Assign the member vars
    if ( !LoadLocalVars(client) )
    {
        return;
    }

    // Check if we are starting or stopping use
    if ( owner->GetMode() != PSCHARACTER_MODE_WORK )
    {
        StartCombineWork(client);
    }
    else
    {
        StopCombineWork(client);
    }
}

////////////////////////////////////////////////////////////////////////
// Check if possible to do some use work
void WorkManager::StartCombineWork(Client* client)
{
    // Check to see if we have everything we need to do any trade work
    if (!ValidateWork())
    {
        return;
    }

    // Check to see if we have pattern
    if (!ValidateMind())
    {
        return;
    }

    // Check for any targeted item or container in hand
    if (!ValidateTarget(client))
    {
        return;
    }

    // Check if targeted item is container
    if (  workItem && workItem->GetIsContainer() )
    {
        // Combine anything can be combined in container
        CombineWork();
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do combine work if possible
bool WorkManager::CombineWork()
{
    // Find out if anything can be combined in container
    uint32 combinationId = 0;
    int combinationQty = 0;
    if ( IsContainerCombinable( combinationId, combinationQty ) )
    {
        // Transform all items in container into the combination item
        psItem* newItem = CombineContainedItem( combinationId, combinationQty, currentQuality, workItem );
        if (newItem)
        {
            // Find out if we can do a combination transformation
            unsigned int transMatch = AnyTransform( patternId, groupPatternId, combinationId, combinationQty );
            if (( transMatch == TRANSFORM_MATCH ) || (transMatch == TRANSFORM_GARBAGE ))
            {
                // Set up event for transformation
                if (workItem->GetCanTransform())
                {
                    StartTransformationEvent(
                        TRANSFORMTYPE_AUTO_CONTAINER, PSCHARACTER_SLOT_NONE,
                        combinationQty, currentQuality, newItem);
                }
                else
                {
                    StartTransformationEvent(
                        TRANSFORMTYPE_CONTAINER, PSCHARACTER_SLOT_NONE,
                        combinationQty, currentQuality, newItem);
                }
                psserver->SendSystemOK(clientNum,"You start to work on combining items.");
                return true;
            }
            else
            {
                // The transform could not be created so send "wrong" message and the reason back to the user.
                psserver->SendSystemError(clientNum, "You started to combine items, but could not go any further.");
                SendTransformError( clientNum, transMatch, combinationId, combinationQty );
                return true;
            }
        }
    }
    else
    {
        SendTransformError( clientNum, TRANSFORM_BAD_COMBINATION);
        return false;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing combine work
void WorkManager::StopCombineWork(Client* client)
{
    // Check for any targeted item or container in hand
    if ( !ValidateTarget(client))
    {
        return;
    }

    // Kill the work event if it exists
    if( worker->GetMode() == PSCHARACTER_MODE_WORK )
    {
        worker->SetMode(PSCHARACTER_MODE_PEACE);
    }

    // Tell the user
    psserver->SendSystemOK(clientNum,"You stop working.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle /construct command
void WorkManager::HandleConstruct(Client *client)
{
    // Assign the member vars
    if ( !LoadLocalVars(client) )
    {
        return;
    }

    //Check if we are starting or stopping use
    if ( owner->GetMode() != PSCHARACTER_MODE_WORK )
    {
        StartConstructWork(client);
    }
    else
    {
        StopConstructWork(client);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if possible to do some use work
void WorkManager::StartConstructWork(Client* client)
{
    // Check to see if we have everything we need to do any trade work
    if (!ValidateWork())
    {
        return;
    }

    // Check to see if we have pattern
    if (!ValidateMind())
    {
        return;
    }

    // Check for any targeted item or container in hand
    if (!ValidateTarget(client))
    {
        return;
    }

    // Check if targeted item is constructible
    if(workItem && workItem->GetIsConstructible())
    {
        if(workItem->GetIsTrap())
        {
            StartTransformationEvent(TRANSFORMTYPE_TARGET_TO_NPC, PSCHARACTER_SLOT_NONE, 1, workItem->GetItemQuality(), workItem);
            psserver->SendSystemOK(clientNum, "You begin constructing %s.", workItem->GetName());
        }
        else if(workItem->GetIsContainer())
        {
            uint32 combinationId = 0;
            int combinationQty = 0;
            if(IsContainerCombinable(combinationId, combinationQty))
            {
                // Check to see if item can be transformed
                uint32 itemID = workItem->GetBaseStats()->GetUID();
                unsigned int transMatch = AnyTransform( patternId, groupPatternId, itemID, 1 );
                if (transMatch == TRANSFORM_MATCH)
                {
                    // Set up event for transformation
                    StartTransformationEvent(
                        TRANSFORMTYPE_SELF_CONTAINER, PSCHARACTER_SLOT_NONE, 1, workItem->GetItemQuality(), workItem);
                    psserver->SendSystemOK(clientNum,"You start work on %s.", workItem->GetName());
                    return;
                }
                else
                {
                    // The transform could not be created so send the reason back to the user.
                    SendTransformError( clientNum, transMatch, itemID, 1 );
                    return;
                }
            }
        }
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing combine work
void WorkManager::StopConstructWork(Client* client)
{
    // Check for any targeted item or container in hand
    if ( !ValidateTarget(client))
    {
        return;
    }

    // Kill the work event if it exists
    if( worker->GetMode() == PSCHARACTER_MODE_WORK )
    {
        worker->SetMode(PSCHARACTER_MODE_PEACE);
    }

    // Tell the user
    psserver->SendSystemOK(clientNum, "You stop constructing.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if possible to do some automatic work
void WorkManager::StartAutoWork(Client* client, gemContainer* container, psItem* droppedItem, int count)
{
    // Assign the member vars
    if ( !LoadLocalVars(client) )
    {
        return;
    }

    if ( !droppedItem )
    {
        Error1("Bad item passed to start AutoWork.");
        return;
    }

    // Interrupt the first event of the dropped item if any to handle stacking
    psWorkGameEvent* stackEvent = droppedItem->GetTransformationEvent();
    if( stackEvent )
    {
        droppedItem->SetTransformationEvent(NULL);
        stackEvent->Interrupt();
    }

    // We know it is a container so check if it's a transform container
    if ( container->GetCanTransform() )
    {
        // Check to see if we have everything we need to do any trade work
        if (!ValidateWork())
        {
            return;
        }

        // Work item is container into which items were dropped
        workItem = container->GetItem();
        autoItem = droppedItem;
        uint32 autoID = autoItem->GetBaseStats()->GetUID();

        // Check to see if we have pattern
        if (ValidateMind())
        {
            // Verify there is a valid transformation for the item that was dropped
            unsigned int transMatch = AnyTransform( patternId, groupPatternId, autoID, count );
            switch( transMatch )
            {
                case TRANSFORM_MATCH:
                {
                    // Set up event for auto transformation
                    StartTransformationEvent(
                        TRANSFORMTYPE_AUTO_CONTAINER, PSCHARACTER_SLOT_NONE, count, autoItem->GetItemQuality(), autoItem);
                    psserver->SendSystemOK(clientNum,"You start work on %d %s.", autoItem->GetStackCount(), autoItem->GetName());
                    return;
                }
                case TRANSFORM_GARBAGE:
                {
                    // Set up event for auto transformation
                    StartTransformationEvent(
                        TRANSFORMTYPE_AUTO_CONTAINER, PSCHARACTER_SLOT_NONE, count, autoItem->GetItemQuality(), autoItem);
                    psserver->SendSystemError(clientNum,"You are not sure what is going to happen to %d %s.", autoItem->GetStackCount(), autoItem->GetName());
                    return;
                }
                default:
                {
                    // The transform could not be created so send the reason back to the user.
                    SendTransformError( clientNum, transMatch, autoID, count );
                    break;
                }
            }
        }
        // If no transformations started go ahead and start a cleanup event
        StartCleanupEvent(TRANSFORMTYPE_AUTO_CONTAINER, client, droppedItem, client->GetActor());
        return;
    }
    // If this is a non-autotransform container go ahead and start a cleanup event
    StartCleanupEvent(TRANSFORMTYPE_CONTAINER, client, droppedItem, client->GetActor());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing automatic work
void WorkManager::StopAutoWork(Client* client, psItem* autoItem)
{
    // Check for proper autoItem
    if ( !autoItem )
    {
        Error1("StopAutoWork does not have a good autoItem pointer.");
        return;
    }

    // Stop the auto item's work event
    psWorkGameEvent* workEvent = autoItem->GetTransformationEvent();
    if (workEvent)
    {
        if( !workEvent->GetTransformation())
        {
            Error1("StopAutoWork does not have a good transformation pointer.");
        }

        // Unless it's some garbage item let players know
        if( workEvent->GetTransformation()->GetResultId() > 0)
        {
            psserver->SendSystemOK(clientNum,"You stop working on %s.", autoItem->GetName());
        }
        autoItem->SetTransformationEvent(NULL);
        workEvent->Interrupt();
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if possible to do some script work
bool WorkManager::StartScriptWork(Client* client, gemObject *target, csString pattern)
{
    // Assign the member vars
    if (!LoadLocalVars(client, target))
    {
        return false;
    }

    // Check to see if we have everything we need to do any trade work
    if (!ValidateWork())
    {
        return false;
    }

    // Check to see if player has stamina for work
    if (!ValidateStamina(client))
    {
        return false;
    }

    // Check for the existance of a pattern
    if ( pattern.Length() == 0 )
    {
        Error1("StartScriptWork() got no pattern.");
        if (secure) psserver->SendSystemInfo(clientNum,"No pattern given in script.");
        return false;
    }

    // Get pattern info
    psTradePatterns* newPattern = CacheManager::GetSingleton().GetTradePatternByName(pattern);
    if ( !newPattern )
    {
        Error2("StartScriptWork() could not find pattern for name (%s).",pattern.GetData());
        if (secure) psserver->SendSystemInfo(clientNum,"Could not find pattern for name (%s).",pattern.GetData());
        return false;
    }

    // Assign pattern variables
    patternId = newPattern->GetId();
    groupPatternId = newPattern->GetGroupPatternId();
    patternKFactor = newPattern->GetKFactor();
    workItem = NULL;

    // Check for no target
    if(!target)
    {
        return ScriptNoTarget();
    }

    // Check for actor target
    gemActor* gemAct = dynamic_cast <gemActor*> (target);
    if(gemAct)
    {
        return ScriptActor(gemAct);
    }

    // Check if target is action location
    gemActionLocation* gemAction = dynamic_cast<gemActionLocation*>(target);
    if(gemAction)
    {
        return ScriptAction(gemAction);
    }

    // Check if target is item
    gemItem* gemItm = dynamic_cast<gemItem*>(target);
    if(gemItm)
    {
        return ScriptItem(gemItm);
    }

    if (secure) psserver->SendSystemInfo(clientNum,"No valid script crafting found.");
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do no item script work
bool WorkManager::ScriptNoTarget()
{
    // Check if player has any items in right hand
    psItem* rhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if ( rhand )
    {
        // Find out if we can do a transformation on items in hand
        uint32 handId = rhand->GetBaseStats()->GetUID();
        int handCount = rhand->GetStackCount();

        // Verify there is a valid transformation for the item targetted
        unsigned int rhandMatch = AnyTransform( patternId, groupPatternId, handId, handCount );
        if (( rhandMatch == TRANSFORM_MATCH ) || (rhandMatch == TRANSFORM_GARBAGE ))
        {
            // Set up event for transformation
            StartTransformationEvent(
                TRANSFORMTYPE_SLOT, PSCHARACTER_SLOT_RIGHTHAND, handCount, rhand->GetItemQuality(), rhand);
            psserver->SendSystemOK(clientNum,"You start to transform %d %s.", rhand->GetStackCount(), rhand->GetName());
            return true;
        }
        else if (rhandMatch != TRANSFORM_UNKNOWN_ITEM)
        {
            // The transform could not be created so send the reason back to the user.
            SendTransformError( clientNum, rhandMatch, handId, handCount );
        }
    }

    // Check if player has any items in left hand
    psItem* lhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
    if ( lhand )
    {
        // Find out if we can do a transformation on items in hand
        uint32 handId = lhand->GetBaseStats()->GetUID();
        int handCount = lhand->GetStackCount();

        // Verify there is a valid transformation for the item that was dropped
        unsigned int lhandMatch = AnyTransform( patternId, groupPatternId, handId, handCount );
        if (( lhandMatch == TRANSFORM_MATCH ) || (lhandMatch == TRANSFORM_GARBAGE ))
        {
            // Set up event for transformation
            StartTransformationEvent(
                TRANSFORMTYPE_SLOT, PSCHARACTER_SLOT_LEFTHAND, handCount, lhand->GetItemQuality(), lhand);
            psserver->SendSystemOK(clientNum,"You start to transform %d %s.", lhand->GetStackCount(), lhand->GetName());
            return true;
        }
        else if (lhandMatch != TRANSFORM_UNKNOWN_ITEM)
        {
            // The transform could not be created so send the reason back to the user.
            SendTransformError( clientNum, lhandMatch, handId, handCount );
        }
    }

    // Since either hand can fail to transform normally send general message if we get to here
    SendTransformError( clientNum, TRANSFORM_MISSING_ITEM );
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do actor script work
bool WorkManager::ScriptActor(gemActor* gemAct)
{
    // Set target if any
    if (gemAct && gemAct->GetCharacterData() == NULL )
    {
        Error1("Target actor has bad character pointer.");
        return false;
    }

    // Check if targeted actor has any items in right hand
    psItem* rhand = gemAct->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if ( rhand )
    {
        // Find out if we can do a transformation on items in hand
        uint32 handId = rhand->GetBaseStats()->GetUID();
        int handCount = rhand->GetStackCount();

        // Verify there is a valid transformation for the item targetted
        unsigned int rhandMatch = AnyTransform( patternId, groupPatternId, handId, handCount );
        if (( rhandMatch == TRANSFORM_MATCH ) || (rhandMatch == TRANSFORM_GARBAGE ))
        {
            // Set up event for transformation
            StartTransformationEvent(
                TRANSFORMTYPE_TARGETSLOT, PSCHARACTER_SLOT_RIGHTHAND, handCount, rhand->GetItemQuality(), rhand);
            psserver->SendSystemOK(clientNum,"You start to transform %d %s.", rhand->GetStackCount(), rhand->GetName());
            return true;
        }
        else if (rhandMatch != TRANSFORM_UNKNOWN_ITEM)
        {
            // The transform could not be created so send the reason back to the user.
            SendTransformError( clientNum, rhandMatch, handId, handCount );
        }
    }

    // Check if player has any items in left hand
    psItem* lhand = gemAct->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
    if ( lhand )
    {
        // Find out if we can do a transformation on items in hand
        uint32 handId = lhand->GetBaseStats()->GetUID();
        int handCount = lhand->GetStackCount();

        // Verify there is a valid transformation for the item that was dropped
        unsigned int lhandMatch = AnyTransform( patternId, groupPatternId, handId, handCount );
        if (( lhandMatch == TRANSFORM_MATCH ) || (lhandMatch == TRANSFORM_GARBAGE ))
        {
            // Set up event for transformation
            StartTransformationEvent(
                TRANSFORMTYPE_TARGETSLOT, PSCHARACTER_SLOT_LEFTHAND, handCount, lhand->GetItemQuality(), lhand);
            psserver->SendSystemOK(clientNum,"You start to transform %d %s.", lhand->GetStackCount(), lhand->GetName());
            return true;
        }
        else if (lhandMatch != TRANSFORM_UNKNOWN_ITEM)
        {
            // The transform could not be created so send the reason back to the user.
            SendTransformError( clientNum, lhandMatch, handId, handCount );
        }
    }

    // Since either hand can fail to transform normally send general message if we get to here
    SendTransformError( clientNum, TRANSFORM_MISSING_ITEM );
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do item script work
bool WorkManager::ScriptItem(gemItem* gemItm)
{
    // Check range ignoring Y co-ordinate
    if (worker->RangeTo(gemItm, true) > RANGE_TO_USE)
    {
        psserver->SendSystemError(clientNum,"You are not in range to transform %s.",gemItm->GetName());
        return false;
    }

    // Check if the target is container
    workItem = gemItm->GetItem();
    if ( workItem->GetIsContainer() )
    {
        // Combine anything can be combined in container
        return CombineWork();
    }

    // Find out if we can do a no work item transformation on targetted item
    workItem = NULL;
    uint32 itemId = gemItm->GetItem()->GetBaseStats()->GetUID();
    int itemCount = gemItm->GetItem()->GetStackCount();

    // Verify there is a valid transformation for the item that was dropped
    unsigned int transMatch = AnyTransform( patternId, groupPatternId, itemId, itemCount );
    switch( transMatch )
    {
        case TRANSFORM_MATCH:
        {
            // Set up event for auto transformation
            StartTransformationEvent(
                TRANSFORMTYPE_TARGET, PSCHARACTER_SLOT_NONE, itemCount, gemItm->GetItem()->GetItemQuality(), gemItm->GetItem());
            psserver->SendSystemOK(clientNum,"You start to transform %d %s.", itemCount, gemItm->GetItem()->GetName());
            return true;
        }
        case TRANSFORM_GARBAGE:
        {
            // Set up event for auto transformation
            StartTransformationEvent(
                TRANSFORMTYPE_TARGET, PSCHARACTER_SLOT_NONE, itemCount, gemItm->GetItem()->GetItemQuality(), gemItm->GetItem());
            psserver->SendSystemError(clientNum,"You are not sure what is going to happen to %d %s.", gemItm->GetItem()->GetStackCount(), gemItm->GetItem()->GetName());
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do action location script work
bool WorkManager::ScriptAction(gemActionLocation* gemAction)
{
    psActionLocation *action = gemAction->GetAction();

    // check if the actionlocation is linked to real item
    InstanceID InstanceID = action->GetInstanceID();
    if (InstanceID==INSTANCE_ALL)
    {
        InstanceID = action->GetGemObject()->GetEID().Unbox(); // FIXME: Need to understand & comment on ID interaction here.
    }
    gemItem* target = GEMSupervisor::GetSingleton().FindItemEntity( InstanceID );

    // Cast to item and call craft item
    return ScriptItem(target);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns with the result ID and quantity of the combination
//  if work item container has the correct items in the correct amounts
// Note:  This assumes that the combination items array is sorted by
//  resultId and then itemId
bool WorkManager::IsContainerCombinable(uint32 &resultId, int &resultQty)
{
    // cast a gem container to iterate thru
    gemContainer *container = dynamic_cast<gemContainer*> (workItem->GetGemObject());
    if (!container)
    {
        Error1("Could not instantiate gemContainer");
        return false;
    }

    // Load item array from the conatiner
    csArray<psItem*> itemArray;
    gemContainer::psContainerIterator it(container);
    currentQuality = 1.00;
    while (it.HasNext())
    {
        // Only check the stuff that player owns or is public
        psItem* item = it.Next();
        if ((item->GetGuardingCharacterID() == owner->GetPID())
            || (item->GetGuardingCharacterID() == 0))
        {
            itemArray.Push(item);

            // While we are here find the item with the least quality
            float quality = item->GetItemQuality();
            if (currentQuality < quality)
            {
                currentQuality = quality;
            }
        }
    }

    // Check if specific combination is possible
    if( ValidateCombination(itemArray, resultId, resultQty))
        return true;

    // Check if any combination is possible
    return AnyCombination(itemArray, resultId, resultQty);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks to see if every item in list matches every item in a valid combination
bool WorkManager::ValidateCombination(csArray<psItem*> itemArray, uint32 &resultId, int &resultQty)
{
    // check if player owns anything in conatiner
    size_t itemCount = itemArray.GetSize();
    if (itemCount != 0)
    {
        // Get all possible combinations for that pattern
        csPDelArray<CombinationConstruction>* combArray = CacheManager::GetSingleton().FindCombinationsList(patternId);
        csPDelArray<CombinationConstruction>* combGroupArray = CacheManager::GetSingleton().FindCombinationsList(groupPatternId);
        if (combArray == NULL)
        {
            // Check for group pattern combinations
            if (combGroupArray == NULL)
            {
                if (secure) psserver->SendSystemInfo(clientNum,"Failed to find any combinations in patterns %d and group %d.", patternId, groupPatternId );
                return false;
            }
        }

        // Go through all of the combinations looking for exact match
        resultId = 0;
        resultQty = 0;

        // Check all the possible combination in this data set
        if (combArray != NULL)
        {
            if (secure) psserver->SendSystemInfo(clientNum,"Checking combinations for patterns %d.", patternId );
            for (size_t i=0; i<combArray->GetSize(); i++)
            {
                // Check for matching lists
                CombinationConstruction* current = combArray->Get(i);
                if (secure) psserver->SendSystemInfo(clientNum,"Checking combinations for result id %u quantity %d.", current->resultItem, current->resultQuantity);
                if( MatchCombinations(itemArray,current) )
                {
                    resultId = current->resultItem;
                    resultQty = current->resultQuantity;
                    if (secure) psserver->SendSystemInfo(clientNum,"Found mathcing combination.");
                    return true;
                }
            }
        }

        // Check all the possible combination in this data set
        if (combGroupArray != NULL)
        {
            if (secure) psserver->SendSystemInfo(clientNum,"Checking combinations for group pattern %d.", groupPatternId );
            for (size_t i=0; i<combGroupArray->GetSize(); i++)
            {
                // Check for matching lists
                CombinationConstruction* current = combGroupArray->Get(i);
                if (secure) psserver->SendSystemInfo(clientNum,"Checking combinations for result id %u quantity %d.", current->resultItem, current->resultQuantity);
                if( MatchCombinations(itemArray,current) )
                {
                    resultId = current->resultItem;
                    resultQty = current->resultQuantity;
                    if (secure) psserver->SendSystemInfo(clientNum,"Found mathcing group combination.");
                    return true;
                }
            }
        }
    }
    else
    {
        if (secure) psserver->SendSystemInfo(clientNum,"Failed to find any items you own in container.");
        return false;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks to see if every item in list is in the set of ingredients for this pattern
bool WorkManager::AnyCombination(csArray<psItem*> itemArray, uint32 &resultId, int &resultQty)
{
    // check if player owns anything in conatiner
    size_t itemCount = itemArray.GetSize();
    if (itemCount != 0)
    {
        // Get list of unique ingredients for this pattern
        uint32 activePattern = patternId;
        csArray<uint32>* uniqueArray = CacheManager::GetSingleton().GetTradeTransUniqueByID(activePattern);
        if (uniqueArray == NULL)
        {
            // Try the group pattern
            activePattern = groupPatternId;
            csArray<uint32>* uniqueArray = CacheManager::GetSingleton().GetTradeTransUniqueByID(activePattern);
            if (uniqueArray == NULL)
            {
                return false;
            }
        }

        // Go through all of the items making sure they are ingredients
        for (size_t i=0; i<itemArray.GetSize(); i++)
        {
            psItem* item = itemArray.Get(i);
            if ( uniqueArray->Find(item->GetBaseStats()->GetUID()) == csArrayItemNotFound)
            {
                return false;
            }
        }

        // Now find the patternless transform to get results
        resultId = 0;
        resultQty = 0;

        // Get all unknow item transforms for this pattern
        csPDelArray<psTradeTransformations>* transArray =
            CacheManager::GetSingleton().FindTransformationsList(activePattern, 0);
        if (transArray == NULL)
        {
            return false;
        }

        // Go thru list of transforms
        for (size_t j=0; j<transArray->GetSize(); j++)
        {
            // Get first transform with a 0 process ID this indicates processless any ingredient transform
            psTradeTransformations* trans = transArray->Get(j);
            if( trans->GetProcessId() == 0)
            {
                resultId = trans->GetResultId();
                resultQty = trans->GetResultQty();
                return true;
            }
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks for matching combination list to item array
bool WorkManager::MatchCombinations(csArray<psItem*> itemArray, CombinationConstruction* current)
{
    // If the items count match then this is a possible valid combination.
    if ( itemArray.GetSize() == current->combinations.GetSize() )
    {
        // Setup two arrays for comparison
        csArray<psItem*> itemsMatched;
        csArray<psItem*> itemsLeft = itemArray;

        // Iterate over the items in the construction set looking for matches.
        for( size_t j = 0; j < current->combinations.GetSize(); j++ )
        {
            uint32 combId  = current->combinations[j]->GetItemId();
            int combMinQty = current->combinations[j]->GetMinQty();
            int combMaxQty = current->combinations[j]->GetMaxQty();

            // Iterate again over all items left in match set.
            for ( size_t z = 0; z < itemsLeft.GetSize(); z++ )
            {
                uint32 id =  itemsLeft[z]->GetCurrentStats()->GetUID();
                int stackQty = itemsLeft[z]->GetStackCount();
                if ( id ==  combId && stackQty >= combMinQty && stackQty <= combMaxQty )
                {
                    // We have matched the ids and the stack counts are in range.
                    itemsMatched.Push(itemsLeft[z]);
                    itemsLeft.DeleteIndexFast(z);
                    break;
                }
            }
        }

        // If all items matched up then we have a valid combination
        if( (itemsLeft.GetSize() == 0) && (itemsMatched.GetSize() == current->combinations.GetSize()) )
        {
            return true;
        }
        else
        {
            if (secure)
            {
                psserver->SendSystemInfo(clientNum,"Combination match fail.");
                psserver->SendSystemInfo(clientNum,"Items left unmatched:");
                for ( size_t z = 0; z < itemsLeft.GetSize(); z++ )
                {
                    psserver->SendSystemInfo(clientNum,"Item id %u quantity %d",itemsLeft[z]->GetCurrentStats()->GetUID(),itemsLeft[z]->GetStackCount());
                }
            }
            return false;
        }
    }
    if (secure) psserver->SendSystemInfo(clientNum,"Bad count of items %u compared to combination count %u.", itemArray.GetSize(), current->combinations.GetSize() );
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if any transformation is possible
//   We check the more specific transforms for matches first
unsigned int WorkManager::AnyTransform(uint32 singlePatternId, uint32 groupPatternId, uint32 targetId, int targetQty)
{
    // First check for specific single transform match
    if (secure) psserver->SendSystemInfo(clientNum,"Checking single transforms for pattern id %u, target id %u, and target qty %u.", singlePatternId,targetId,targetQty );
    unsigned int singleMatch = IsTransformable( singlePatternId, targetId, targetQty );
    if (singleMatch == TRANSFORM_MATCH)
        return TRANSFORM_MATCH;

    // Then check for a group transform match
    if (secure) psserver->SendSystemInfo(clientNum,"Checking group transforms for pattern id %u, target id %u, and target qty %u.",groupPatternId, targetId, targetQty);
    unsigned int groupMatch = IsTransformable( groupPatternId, targetId, targetQty );
    if (groupMatch == TRANSFORM_MATCH)
        return TRANSFORM_MATCH;

    // Check for patternless transforms
    if (secure) psserver->SendSystemInfo(clientNum,"Checking patternless transforms for pattern id %u, target id %u, and target qty %u.", 0, targetId, targetQty );
    unsigned int lessMatch = IsTransformable( 0, targetId, targetQty );
    if (lessMatch == TRANSFORM_MATCH)
        return TRANSFORM_MATCH;

    // Check for transforms of any ingredients
    if (secure) psserver->SendSystemInfo(clientNum,"Checking for any ingredient transforms for single pattern id %u  group pattern id %u, and target id %u.", singlePatternId, groupPatternId, targetId );
    if (IsIngredient( singlePatternId, groupPatternId, targetId ))
        return TRANSFORM_MATCH;

    // Check for specific single any item transform match
    if (secure) psserver->SendSystemInfo(clientNum,"Checking single any item transforms for pattern id %u, target id %u, and target qty %u.",singlePatternId, 0, targetQty);
    unsigned int singleGarbMatch = IsTransformable( singlePatternId, 0, targetQty );
    if (singleGarbMatch == TRANSFORM_MATCH)
        return TRANSFORM_GARBAGE;

    // Then check for a group any item transform match
    if (secure) psserver->SendSystemInfo(clientNum,"Checking group any item transforms for pattern id %u, target id %u, and target qty %u.", groupPatternId, 0, targetQty);
    unsigned int groupGarbMatch = IsTransformable( groupPatternId, 0, targetQty );
    if (groupGarbMatch == TRANSFORM_MATCH)
        return TRANSFORM_GARBAGE;

    // If no other non-garbage/unknown failures then check for pattern-less any item transforms
    if ((singleMatch == TRANSFORM_GARBAGE || singleMatch == TRANSFORM_UNKNOWN_ITEM)
        && (groupMatch == TRANSFORM_GARBAGE || groupMatch == TRANSFORM_UNKNOWN_ITEM)
        && (lessMatch == TRANSFORM_GARBAGE || lessMatch == TRANSFORM_UNKNOWN_ITEM))
    {
        if (secure) psserver->SendSystemInfo(clientNum,"Checking patternless any item tranforms for pattern id %u, target id %u, and target qty %u.", 0, 0, 0);
        unsigned int lessGarbMatch = IsTransformable( 0, 0, 0 );
        if (lessGarbMatch == TRANSFORM_MATCH)
            return TRANSFORM_GARBAGE;
    }

    // Otherwise return a combination of all of them except garbage
    return (singleMatch | groupMatch | lessMatch);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if a transformation is possible
unsigned int WorkManager::IsTransformable(uint32 patternId, uint32 targetId, int targetQty)
{
    unsigned int match = TRANSFORM_GARBAGE;
    uint32 workID = 0;

    // Only get those with correct target item and pattern
    csPDelArray<psTradeTransformations>* transArray =
        CacheManager::GetSingleton().FindTransformationsList(patternId, targetId);
    if (transArray == NULL)
    {
        if (secure) psserver->SendSystemInfo(clientNum,"Found no transformations for item %d and pattern %d.", targetId, patternId );
        return TRANSFORM_UNKNOWN_ITEM;
    }

    // Go thru all the trasnformations and check if one is possible
    if (secure) psserver->SendSystemInfo(clientNum,"Found %u transformations.", transArray->GetSize() );
    if (workItem)
    {
        workID = workItem->GetCurrentStats()->GetUID();
    }
    psTradeTransformations* transCandidate;
    psTradeProcesses* procCandidate;
    for (size_t i=0; i<transArray->GetSize(); i++)
    {
        transCandidate = transArray->Get(i);
        if (secure) psserver->SendSystemInfo(clientNum,"Testing transformation id %u.", transCandidate->GetId() );

        // Go thru all the possable processes and check if one is possible
        csArray<psTradeProcesses*>* procArray = CacheManager::GetSingleton().GetTradeProcessesByID(
            transCandidate->GetProcessId());

        // If no process for this transform then just continue on
        if (!procArray)
        {
            if (secure) psserver->SendSystemInfo(clientNum,"No match, transformation has no process array.");
            continue;
        }

        for (size_t j=0; j<procArray->GetSize(); j++)
        {
            // Check the non-zero work item does not match
            procCandidate = procArray->Get(j);
            if( procCandidate->GetWorkItemId() != 0 && procCandidate->GetWorkItemId() != workID )
            {
                    if (secure) psserver->SendSystemInfo(clientNum,"No match, transformation for wrong item candidate id=%u workitem Id=%u.", procCandidate->GetWorkItemId(), workID);
                    match |= TRANSFORM_MISSING_ITEM;
                    continue;
            }

            // Check if we do not have the correct quantity
            int itemQty = transCandidate->GetItemQty();
            if ( itemQty != 0 && itemQty != targetQty )
            {
                if (secure) psserver->SendSystemInfo(clientNum,"No match, transformation for wrong quantity candidateQty=%d itemQty=%d.", transCandidate->GetItemQty(), itemQty);
                match |= TRANSFORM_BAD_QUANTITY;
                continue;
            }

            // Check if any equipement is required
            uint32 equipmentId = procCandidate->GetEquipementId();
            if ( equipmentId > 0 )
            {
                // Check if required equipment is on hand
                if ( !IsOnHand( equipmentId ) )
                {
                    if (secure) psserver->SendSystemInfo(clientNum,"No match, transformation for wrong equipment transform id=%u equipmentId=%u.", transCandidate->GetId(), equipmentId);
                    match |= TRANSFORM_MISSING_EQUIPMENT;
                    continue;
                }
            }

            // Check if the player has the sufficient training
            if ( !ValidateTraining(transCandidate, procCandidate) )
            {
                if (secure) psserver->SendSystemInfo(clientNum,"No match, transformation fails training check.");
                match |= TRANSFORM_BAD_TRAINING;
                continue;
            }

            // Check if the player has the correct skills
            if ( !ValidateSkills(transCandidate, procCandidate) )
            {
                if (secure) psserver->SendSystemInfo(clientNum,"No match, transformation fails skill check.");
                match |= TRANSFORM_BAD_SKILLS;
                continue;
            }

            // Check if the transformation constraint is meet
            if ( !ValidateConstraints(transCandidate, procCandidate) )
            {
                // Player message comes from database
                if (secure) psserver->SendSystemInfo(clientNum,"No match, transformation fails constraint check.\n");
                match |= TRANSFORM_FAILED_CONSTRAINTS;
                continue;
            }

            // Good match
            if (secure) psserver->SendSystemInfo(clientNum,"Good match for transformation id=%u.\n", transCandidate->GetId());
            trans = transCandidate;
            process = procCandidate;
            return TRANSFORM_MATCH;
        }
    }

    // Return all the errors
    return match;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if any ingredient transformation is possible
bool WorkManager::IsIngredient(uint32 patternId, uint32 groupPatternId, uint32 targetId)
{
    // Check if ingredient is on list of unique ingredients for this pattern
    csArray<uint32>* itemArray = CacheManager::GetSingleton().GetTradeTransUniqueByID(patternId);
    if (itemArray == NULL)
    {
        csArray<uint32>* itemArray = CacheManager::GetSingleton().GetTradeTransUniqueByID(groupPatternId);
        if (itemArray == NULL)
        {
            if (secure) psserver->SendSystemInfo(clientNum,"No items for this pattern or group pattern.");
            return false;
        }
    }

    // Go thru list
    for (size_t i=0; i<itemArray->GetSize(); i++)
    {
        // Check item on ingredient list
        if( itemArray->Get(i) == targetId )
        {
            // Get all unknow item transforms for this pattern
            csPDelArray<psTradeTransformations>* transArray =
                CacheManager::GetSingleton().FindTransformationsList(patternId, 0);
            if (transArray == NULL)
            {
                if (secure) psserver->SendSystemInfo(clientNum,"No known transformations for this item.");
                return false;
            }

            // Go thru list of transforms
            for (size_t j=0; j<transArray->GetSize(); j++)
            {
                // Get first transform with a 0 process ID this indicates processless any ingredient transform
                trans = transArray->Get(j);
                process = NULL;
                if( trans->GetProcessId() == 0)
                {
                    return true;
                }
            }
        }
    }
    if (secure) psserver->SendSystemInfo(clientNum,"Not an ingredient for this pattern.");
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do transformation
// This is called once all the transformation requirements have been verified.
void WorkManager::StartTransformationEvent(int transType, INVENTORY_SLOT_NUMBER transSlot, int resultQty,
                                             float resultQuality, psItem* item)
{
    // Get transformation delay
    int delay;

    if(trans)
    {
        delay = CalculateEventDuration( trans->GetTransPoints() );
    }
    else
    {
        // Used in the case of a transformation like item->npc.
        delay = resultQuality/10;
    }

    // Let the server admin know
    if (secure) psserver->SendSystemInfo(clientNum,"Scheduled to finish work on %s in %1.1f seconds.\n", item->GetName(), (float)delay);

    // Make sure we have an item
    if (!item)
    {
        Error1("Bad item in StartTransformationEvent().");
        return;
    }

    // kill old event - just in case
    psWorkGameEvent* oldEvent = item->GetTransformationEvent();
    if (oldEvent)
    {
        Error1("Had to kill old event in StartTransformationEvent.");
        item->SetTransformationEvent(NULL);
        oldEvent->Interrupt();
    }

    // Set event
    csVector3 pos(0,0,0);
    psWorkGameEvent* workEvent = new psWorkGameEvent(
        this, owner, delay*1000, MANUFACTURE, pos, NULL, worker->GetClient() );
    workEvent->SetTransformationType(transType);
    workEvent->SetTransformationSlot(transSlot);
    workEvent->SetResultQuality(resultQuality);
    workEvent->SetTransformationItem(item);
    workEvent->SetWorkItem(workItem);
    workEvent->SetTargetGem(gemTarget);
    workEvent->SetResultQuantity(resultQty);
    workEvent->SetTransformation(trans);
    workEvent->SetProcess(process);
    workEvent->SetKFactor(patternKFactor);
    item->SetTransformationEvent(workEvent);

    // Send render effect to client and nearby people
    if ( process && process->GetRenderEffect().Length() > 0 )
    {
        csVector3 offset(0,0,0);
        workEvent->effectID =  CacheManager::GetSingleton().NextEffectUID();
        psEffectMessage newmsg( 0, process->GetRenderEffect(), offset, owner->GetEID(),0 ,workEvent->effectID );
        newmsg.Multicast(workEvent->multi,0,PROX_LIST_ANY_RANGE);
    }

    // If it's not an autotransformation event run the animation and setup the owner with the event
    if( transType != TRANSFORMTYPE_AUTO_CONTAINER)
    {
        // Send anim and confirmation message to client and nearby people
        if( process && !process->GetAnimation().IsEmpty())
        {
            psOverrideActionMessage msg(clientNum, worker->GetEID(), process->GetAnimation(), delay);
            psserver->GetEventManager()->Multicast(msg.msg, worker->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
        }

        // Setup work owner
        worker->SetMode(PSCHARACTER_MODE_WORK);
        owner->SetTradeWork(workEvent);
        if (process)
        {
            owner->GetCharacterData()->SetStaminaRegenerationWork(process->GetPrimarySkillId());
        }
    }

    // Start event
    psserver->GetEventManager()->Push(workEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Loads up local member variables
bool WorkManager::LoadLocalVars(Client* client, gemObject *target)
{
    if ( client == NULL )
    {
        Error1("Bad client pointer.");
        return false;
    }

    clientNum = client->GetClientNum();
    if ( clientNum == 0 )
    {
        Error1("Bad client number.");
        return false;
    }

    worker = client->GetActor();
    if ( !worker.IsValid())
    {
        Error1("Bad client GetActor pointer.");
        return false;
    }

    owner = worker;
    if (!owner || !owner->GetCharacterData())
    {
        Error1("Bad client actor character data pointer.");
        return false;
    }

    // Setup security check
    secure = (client->GetSecurityLevel() > GM_LEVEL_0 ) ? true : false;

    // Setup mode string pointer
    preworkModeString = owner->GetModeStr();

    // Null out everything else
    gemTarget = target;
    workItem = NULL;
    patternId = 0;
    groupPatternId = 0;
    patternKFactor = 0.0;
    currentQuality = 0.0;
    trans = NULL;
    process = NULL;

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check to see if client can do any trade work
bool WorkManager::ValidateWork()
{
    if ( worker->GetMode() == PSCHARACTER_MODE_WORK)
    {
        psserver->SendSystemInfo(clientNum,"You can not practice your trade when working." );
        return false;
    }
    // Check if not in normal mode
    else if ( worker->GetMode() == PSCHARACTER_MODE_COMBAT)
    {
        psserver->SendSystemInfo(clientNum,"You can not practice your trade during combat." );
        return false;
    }
    return true;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check to see if client can do any trade work and
// that we have everything we need to do any trade work
bool WorkManager::ValidateMind()
{
    // Check for the existance of a design item
    psItem* designitem = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_MIND);
    if ( !designitem )
    {
        patternId = 0;
        groupPatternId = 0;
        patternKFactor = 1.0;
        return true;
    }

    // Check if there is a pattern for that design item
    psItemStats* itemStats = designitem->GetCurrentStats();
    psTradePatterns* pattern = CacheManager::GetSingleton().GetTradePatternByItemID( itemStats->GetUID() );
    if ( pattern == NULL )
    {
        Error3("ValidateWork() could not find pattern ID for item %u (%s) in mind slot",
            itemStats->GetUID(),itemStats->GetName());
        return false;
    }

    // Setup patterns
    patternId = pattern->GetId();
    groupPatternId = pattern->GetGroupPatternId();
    patternKFactor = pattern->GetKFactor();

    return true;
}

////////////////////////////////////////////////////////////////////////
// Check for any targeted item or container in hand
bool WorkManager::ValidateTarget(Client* client)
{
    // Check if player has something targeted
    gemObject* target = client->GetTargetObject();

    gemActionLocation* gemAction = dynamic_cast<gemActionLocation*>(target);
    if(gemAction) {
        psActionLocation *action = gemAction->GetAction();

      // check if the actionlocation is linked to real item
      InstanceID InstanceID = action->GetInstanceID();
      if (InstanceID==INSTANCE_ALL)
      {
          InstanceID = action->GetGemObject()->GetEID().Unbox(); // FIXME: Understand & comment on ID magic here.
      }
      target = GEMSupervisor::GetSingleton().FindItemEntity( InstanceID );
    }

    if (target)
    {
        // Make sure it's not character
        if (target->GetActorPtr())
        {
            psserver->SendSystemInfo(clientNum,"Only items can be targeted for use.");
            return false;
        }

        // Check range ignoring Y co-ordinate
        if (worker->RangeTo(target, true, true) > RANGE_TO_USE)
        {
            psserver->SendSystemError(clientNum,"You are not in range to use %s.",target->GetItem()->GetName());
            return false;
        }

        // Only legit items
        if (!target->GetItem())
        {
            psserver->SendSystemInfo(clientNum,"That item can not be used in this way.");
            return false;
        }

        // Otherwise assign item
        workItem = target->GetItem();
        return true;
    }

    // if nothing targeted check for containers in hand
    else
    {
        // Check if player has any containers in right hand
        psItem* rhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
        if ( rhand )
        {
            // If it's a container it's our target
            if ( rhand->GetIsContainer() )
            {
                workItem = rhand;
                return true;
            }
        }

        // Check if player has any containers in left hand
        psItem* lhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
        if ( lhand )
        {
            // If it's a container it's our target
            if ( lhand->GetIsContainer() )
            {
                workItem = lhand;
                return true;
            }
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if client is too tired
bool WorkManager::ValidateStamina(Client* client)
{
    //TODO: use factors based on the work to determine required stamina
    // check stamina
    if (client->GetActor()->GetCharacterData()->GetStamina(true)
           <= client->GetActor()->GetCharacterData()->GetMaxPStamina().Current()*.1
     || client->GetActor()->GetCharacterData()->GetStamina(false)
           <= client->GetActor()->GetCharacterData()->GetMaxMStamina().Current()*.1)
    {
        SendTransformError( clientNum, TRANSFORM_NO_STAMINA );
        return false;
    }
    return true;
}

bool WorkManager::CheckStamina(psCharacter * owner) const
{
//todo- use factors based on the work to determine required stamina
    return (owner->GetStamina(true) >= owner->GetMaxPStamina().Current()*.1  // physical
         && owner->GetStamina(false) >= owner->GetMaxMStamina().Current()*.1 // mental
        );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if equipment is in hand
bool WorkManager::IsOnHand( uint32 equipId )
{
    // Check right hand
    psItem* rhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if ( rhand )
    {
        if ( equipId == rhand->GetCurrentStats()->GetUID() )
        {
            return true;
        }
    }

    // Check left hand
    psItem* lhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
    if ( lhand )
    {
        if ( equipId == lhand->GetCurrentStats()->GetUID() )
        {
            return true;
        }
    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validates player has enough training
bool WorkManager::ValidateTraining(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate)
{
    // Check primary skill training if any skill required
    int priSkill = processCandidate->GetPrimarySkillId();
    if ( priSkill > 0 )
    {
        // If primary skill is zero, check if this skill should be trained first
        unsigned int basePriSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)priSkill).Current();
        if ( basePriSkill == 0 )
        {
            if (owner->GetCharacterData()->Skills().Get((PSSKILL)priSkill).CanTrain())
                return false;
        }
    }

    // Check secondary skill training if any skill required
    int secSkill = processCandidate->GetSecondarySkillId();
    if ( secSkill > 0 )
    {
        // If secondary skill is zero, check if this skill should be trained first
        unsigned int baseSecSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)secSkill).Current();
        if ( baseSecSkill == 0 )
        {
            if (owner->GetCharacterData()->Skills().Get((PSSKILL)secSkill).CanTrain())
                return false;
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validates player has the correct skills
bool WorkManager::ValidateSkills(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate)
{
    // Check if players primary skill levels are less then minimum for that skill
    int priSkill = processCandidate->GetPrimarySkillId();
    if ( priSkill > 0 )
    {
        unsigned int minPriSkill = processCandidate->GetMinPrimarySkill();
        unsigned int basePriSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)priSkill).Current();
        if ( minPriSkill > basePriSkill )
        {
            return false;
        }
    }

    // Check if players secondary skill levels are less then minimum for that skill
    int secSkill = processCandidate->GetSecondarySkillId();
    if ( secSkill > 0 )
    {
        unsigned int minSecSkill = processCandidate->GetMinSecondarySkill();
        unsigned int baseSecSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)secSkill).Current();
        if ( minSecSkill > baseSecSkill )
        {
            return false;
        }
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validates player is not over skilled
bool WorkManager::ValidateNotOverSkilled(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate)
{
    // Check if players primary skill levels are less then maximum for that skill
    int priSkill = processCandidate->GetPrimarySkillId();
    if ( priSkill >= 0 )
    {
        unsigned int maxPriSkill = processCandidate->GetMaxPrimarySkill();
        unsigned int basePriSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)priSkill).Current();
        if ( maxPriSkill < basePriSkill )
        {
            return false;
        }
    }

    // Check if players secondary skill levels are less then maximum for that skill
    int secSkill = processCandidate->GetSecondarySkillId();
    if ( secSkill >= 0 )
    {
        unsigned int maxSecSkill = processCandidate->GetMaxSecondarySkill();
        unsigned int baseSecSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)secSkill).Current();
        if ( maxSecSkill < baseSecSkill )
        {
            return false;
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validate if all the transformation constraints are meet
// Note: This assumes that each of the constraints have paramaters
bool WorkManager::ValidateConstraints(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate)
{
    // Set up to go through the constraint string picking out the functions and parameters
    const char constraintSeperators[] = "\t(),";
    char constraintPtr[256];
    char* param;

    // Get a work copy of the constraint string to token up
    const char* constraintStr = processCandidate->GetConstraintString();
    strcpy(constraintPtr,constraintStr);

    // Loop through all the constraint strings
    char* funct = strtok( constraintPtr, constraintSeperators);
    while( funct != NULL)
    {
        // Save the pointer to the constraint and get the parameter string
        param = strtok( NULL, constraintSeperators);

        // Check all the constraints
        int constraintId = -1;
        while( constraints[++constraintId].constraintFunction != NULL )
        {
            // Check if the transformation constraint matches
            if( strcmp(constraints[constraintId].name,funct) == 0 )
            {

                // Call the associated function
                if ( !constraints[constraintId].constraintFunction( this, param) )
                {
                    // Send constraint specific message to client
                    psserver->SendSystemError(clientNum, constraints[constraintId].message );
                    return false;
                }
                break;
            }
        }

        // Get the next constraint
        funct = strtok( NULL, constraintSeperators);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculate how long it will take to complete event
//  based on the transformation's point quanity
int WorkManager::CalculateEventDuration(int pointQty)
{
    // Translate the points into seconds
    // ToDo: For now the point quantity is the duration
    return pointQty;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Combine transform all items in container into a new item
psItem* WorkManager::CombineContainedItem(uint32 newId, int newQty, float itemQuality, psItem* containerItem)
{
#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "deleting items from container...\n");
#endif

    // cast a gem container to iterate thru
    gemContainer *container = dynamic_cast<gemContainer*> (containerItem->GetGemObject());
    if (!container)
    {
        Error1("Could not instantiate gemContainer");
        return NULL;
    }

    // Remove all items in container and destroy them
    gemContainer::psContainerIterator it(container);
    while (it.HasNext())
    {
        // Remove all items from container that player owns
        psItem* item = it.Next();
        if ((item->GetGuardingCharacterID() == owner->GetPID())
            || (item->GetGuardingCharacterID() == 0) )
        {
            // Kill any trade work started
            psWorkGameEvent* workEvent = item->GetTransformationEvent();
            if (workEvent)
            {
                item->SetTransformationEvent(NULL);
                workEvent->Interrupt();
            }

            // Remove items and delete
            it.RemoveCurrent(owner->GetClient());
            if (!item->Destroy())
            {
                Error2("CombineContainedItem() could not remove old item ID #%u from database.", item->GetUID());
                return NULL;
            }
            delete item;
        }
    }

    // Create combined item
    psItem* newItem = CreateTradeItem(newId, newQty, itemQuality);
    if(!newItem)
    {
        Error2("CreateTradeItem() could not create new item ID #%u", newId);
        return NULL;
    }

    newItem->SetOwningCharacter(owner->GetCharacterData());
    // this is done by AddToContainer, as long as SetOwningCharacter is set _before_
    // newItem->SetGuardingCharacterID(owner->GetPID());

    // Locate item in container and save container
    if (!container->AddToContainer(newItem, owner->GetClient()))
    {
        Error3("Bad container slot %i when trying to add item instance #%u.", PSCHARACTER_SLOT_NONE, newItem->GetUID());
        return NULL;
    }
    workItem->Save(true);

    // Zero out x,y,z location because it is in container
    float xpos,ypos,zpos,yrot;
    psSectorInfo* sectorinfo;
    InstanceID instance;
    workItem->GetLocationInWorld(instance, &sectorinfo, xpos, ypos, zpos, yrot );
    newItem->SetLocationInWorld(instance,sectorinfo, 0.00, 0.00, 0.00, 0.00 );

    newItem->SetLoaded();  // Item is fully created

    // Check for conditions that would cause assert on newItem->Save(true)
    if (newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
          newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
          worker->GetGuildID(), owner->GetFirstName(), owner);
        if (workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        return NULL;
    }
    newItem->Save(true);

    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform the container into a new item
psItem* WorkManager::TransformSelfContainerItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality)
{
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform the one items in container into a new item
psItem* WorkManager::TransformContainedItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality)
{
    if (!oldItem)
    {
        Error1("TransformAutoContainedItem() had no item to remove." );
        return NULL;
    }

#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "deleting item from container...\n");
#endif

    gemContainer *container = dynamic_cast<gemContainer*> (workItem->GetGemObject());
    if (!container)
    {
        Error1("Could not instantiate gemContainer");
        return NULL;
    }

    INVENTORY_SLOT_NUMBER oldslot = oldItem->GetLocInParent(false);

    // Remove items from container and destroy it
    container->RemoveFromContainer(oldItem, owner->GetClient());
    if (!oldItem->Destroy())
    {
        Error2("TransformContainedItem() could not remove old item ID #%u from database", oldItem->GetUID());
        return NULL;
    }
    delete oldItem;

    // Check for consumed item
    if (newId <= 0)
        return NULL;

    // Create item and save it to item instances
    psItem* newItem = CreateTradeItem(newId, newQty, itemQuality);
    if(!newItem)
    {
        Error2("CreateTradeItem() could not create new item ID #%u", newId);
        return NULL;
    }

    newItem->SetOwningCharacter(owner->GetCharacterData());
    // this is done by AddToContainer, as long as SetOwningCharacter is set _before_
    // newItem->SetGuardingCharacterID(owner->GetPID());

    // Locate item in container and save container
    if (!container->AddToContainer(newItem, owner->GetClient(), oldslot))
    {
        Error3("Bad container slot %i when trying to add item instance #%u.", PSCHARACTER_SLOT_NONE, newItem->GetUID());
        return NULL;
    }
    workItem->Save(true);

    // Zero out x,y,z location because it is in container
    float xpos,ypos,zpos,yrot;
    psSectorInfo* sectorinfo;
    InstanceID instance;
    workItem->GetLocationInWorld(instance, &sectorinfo, xpos, ypos, zpos, yrot );
    newItem->SetLocationInWorld(instance,sectorinfo, 0.00, 0.00, 0.00, 0.00 );

    newItem->SetLoaded();  // Item is fully created

    // Check for conditions that would cause assert on newItem->Save(true)
    if (newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
          newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
          worker->GetGuildID(), owner->GetFirstName(), owner);
        if (workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        return NULL;
    }
    newItem->Save(true);

    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform all items in equipment into a new item
psItem* WorkManager::TransformSlotItem(INVENTORY_SLOT_NUMBER slot, uint32 newId, int newQty, float itemQuality)
{
    // Remove items from slot and destroy it
    psItem *oldItem = owner->GetCharacterData()->Inventory().RemoveItem(NULL,slot);
    if (!oldItem)
    {
        Error2("TransformSlotItem() could not remove item in slot #%i", slot );
        return NULL;
    }

    // Delete the old item
    if (!oldItem->Destroy())
    {
        Error2("TransformSlotItem() could not remove old item ID #%u from database", oldItem->GetUID());
        return NULL;
    }
    delete oldItem;

    // Check for consumed item
    if (newId <= 0)
        return NULL;

    // Create item
    psItem* newItem = CreateTradeItem(newId, newQty, itemQuality);
    if(!newItem)
    {
        Error2("CreateTradeItem() could not create new item ID #%u", newId);
        return NULL;
    }

    // Check for valid slot
    if (slot < 0)
    {
        Error1("No good slot to locate new item");
        return NULL;
    }

    // Locate item in owner's slot
    newItem->SetOwningCharacter(owner->GetCharacterData());
    owner->GetCharacterData()->Inventory().Add(newItem);
    if (!owner->GetCharacterData()->Inventory().EquipItem(newItem,slot))
    {
        // If can't equip then drop
        owner->GetCharacterData()->DropItem(newItem);
        psserver->SendSystemError(clientNum,
            "Item %s was dropped because it could not be equiped.",newItem->GetName() );
    }
    newItem->SetLoaded();  // Item is fully created

    // Check for conditions that would cause assert on newItem->Save(true)
    if (newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
          newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
          worker->GetGuildID(), owner->GetFirstName(), owner);
        if (workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        return NULL;
    }
    newItem->Save(true);

    // Send equip message to client
    psserver->GetCharManager()->SendInventory(clientNum);
    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform all items in equipment into a new item
psItem* WorkManager::TransformTargetSlotItem(INVENTORY_SLOT_NUMBER slot, uint32 newId, int newQty, float itemQuality)
{
    // Remove items from targeted slot and destroy it
    psItem *oldItem = gemTarget->GetCharacterData()->Inventory().RemoveItem(NULL,slot);
    if (!oldItem)
    {
        Error2("TransformSlotItem() could not remove item in slot #%i", slot );
        return NULL;
    }

    // Delete the old item
    if (!oldItem->Destroy())
    {
        Error2("TransformSlotItem() could not remove old item ID #%u from database", oldItem->GetUID());
        return NULL;
    }
    delete oldItem;

    // Check for consumed item
    if (newId <= 0)
        return NULL;

    // Create item
    psItem* newItem = CreateTradeItem(newId, newQty, itemQuality);
    if(!newItem)
    {
        Error2("CreateTradeItem() could not create new item ID #%u", newId);
        return NULL;
    }

    // Check for valid slot
    if (slot < 0)
    {
        Error1("No good slot to locate new item");
        return NULL;
    }

    // Check for valid target
    if ( !gemTarget || !gemTarget->GetCharacterData() )
    {
        Error1("No good target to locate new item");
        return NULL;
    }

    // Locate in target slot
    newItem->SetOwningCharacter(gemTarget->GetCharacterData());
    gemTarget->GetCharacterData()->Inventory().Add(newItem);
    if (!gemTarget->GetCharacterData()->Inventory().EquipItem(newItem,slot))
    {
        // If can't equip then drop
        gemTarget->GetCharacterData()->DropItem(newItem);
        psserver->SendSystemError(gemTarget->GetClientID(),
                                  "Item %s was dropped because it could not be equiped.", newItem->GetName() );
    }
    newItem->SetLoaded();  // Item is fully created

    // Check for conditions that would cause assert on newItem->Save(true)
    if (newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
          newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
          worker->GetGuildID(), gemTarget->GetCharacterData()->GetCharName(), gemTarget->GetCharacterData());
        if (workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        return NULL;
    }
    newItem->Save(true);

    // Send equip message to target
    psserver->GetCharManager()->SendInventory(gemTarget->GetClientID());
    if (gemTarget->GetClientID()) // No superclient
    {
        psserver->GetCharManager()->UpdateItemViews(gemTarget->GetClientID());
    }
    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transforms the targeted item into a new item
psItem* WorkManager::TransformTargetItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality)
{
    if (!oldItem)
    {
        Error1("TransformTargetItem() had no item to transform." );
        return NULL;
    }

#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "deleting item from world...\n");
#endif

    // Get the location of what will be replaced
    InstanceID instance;
    psSectorInfo* sectorinfo;
    float xpos,ypos,zpos,yrot;
    oldItem->GetLocationInWorld(instance, &sectorinfo, xpos, ypos, zpos, yrot );

    // Destroy gem and item
    EntityManager::GetSingleton().RemoveActor(oldItem->GetGemObject());
    if (!oldItem->Destroy())
    {
        Error2("TransformTargetItem() could not destroy old item ID #%u from database", oldItem->GetUID());
        return NULL;
    }
    delete oldItem;



    // Check for consumed item
    if (newId <= 0)
        return NULL;

    // Create item
    psItem* newItem = CreateTradeItem(newId, newQty, itemQuality, true);
    if(!newItem)
    {
        Error2("CreateTradeItem() could not create new item ID #%u", newId);
        return NULL;
    }

    // Locate the new item where the old one was
    newItem->SetLocationInWorld(instance,sectorinfo,xpos,ypos,zpos,yrot);
    if (!EntityManager::GetSingleton().CreateItem(newItem, true))
    {
        delete newItem;
        return NULL;
    }


    // Check for conditions that would cause assert on newItem->Save(true)
    if (newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
          newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
          worker->GetGuildID(), owner->GetFirstName(), owner);
        if (workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        delete newItem;
        return NULL;
    }

    // Target it and set it loaded
    worker->GetClient()->SetTargetObject(newItem->GetGemObject());
    newItem->SetLoaded();
    newItem->Save(true);
    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transforms the targeted item into an NPC.
void WorkManager::TransformTargetItemToNpc(psItem* workItem, Client* client)
{
    InstanceID instance;
    psSectorInfo* sectorinfo;
    float loc_x;
    float loc_y;
    float loc_z;
    float loc_yrot;
    workItem->GetLocationInWorld(instance, &sectorinfo, loc_x, loc_y, loc_z, loc_yrot);

    psCharacter* charData = new psCharacter();
    charData->SetCharType(PSCHARACTER_TYPE_NPC);
    psRaceInfo* raceinfo = CacheManager::GetSingleton().GetRaceInfoByNameGender("Special", PSCHARACTER_GENDER_NONE);
    charData->SetPID(psserver->GetUnusedPID());
    charData->SetName(charData->GetPID().Show());
    charData->SetRaceInfo(raceinfo);
    charData->SetOwnerID(client->GetPID());
    charData->SetLocationInWorld(instance, sectorinfo, loc_x, loc_y, loc_z, loc_yrot);

    csString script;
    script.Format("<response><run script='%s'/></response>", workItem->GetBaseStats()->GetConsumeScriptName().GetData());
    NpcResponse* response = dict->AddResponse(script);
    dict->AddTrigger(charData->GetPID().Show(), "!anyrange", 0, response->id);

    EntityManager::GetSingleton().RemoveActor(workItem->GetGemObject());
    workItem->Destroy();

    EntityManager::GetSingleton().CreateNPC(charData, true, true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create the new item and return newly created item pointer
//  The location and saving of item is up to calling routine
psItem* WorkManager::CreateTradeItem(uint32 newId, int newQty, float itemQuality, bool transient)
{
    Debug3( LOG_TRADE, 0,"Creating new item id(%u) quantity(%d)\n", newId, newQty );

#ifdef DEBUG_WORKMANAGER
        CPrintf(CON_DEBUG, "creating item id=%u qty=%i quality=%f\n",
            newId, newQty, itemQuality);
#endif

    // Check to make sure it's not a trasnformation that just destroys items
    if (newId > 0)
    {
        // Create a new item including stack count and the calculated quality
        psItemStats* baseStats = CacheManager::GetSingleton().GetBasicItemStatsByID( newId );
        if (!baseStats)
        {
            Error2("CreateTradeItem() could not get base states for item #%iu", newId );
            return NULL;
        }

        // Make a perminent new item
        psItem* newItem = baseStats->InstantiateBasicItem( transient );
        if (!newItem)
        {
            Error3("CreateTradeItem() could not create item (%s) id #%u",
                baseStats->GetName(), newId );
            return NULL;
        }

        // Set stuff
        newItem->SetItemQuality(itemQuality);
        newItem->SetMaxItemQuality(itemQuality);            // Set the max quality the same as the inital quality.
        newItem->SetStackCount(newQty);
        newItem->SetDecayResistance(0.5);

        // Set current player ID creator mark
        newItem->SetCrafterID(worker->GetPID());

        // Set current player guild to creator mark
        if (owner->GetGuild())
            newItem->SetGuildID(owner->GetGuild()->id);
        else
            newItem->SetGuildID(0);

#ifdef DEBUG_WORKMANAGER
        CPrintf(CON_DEBUG, "done creating item crafterID=%s guildID=%i name=%s owner=%p\n",
                ShowID(worker->GetPID()), worker->GetGuildID(), owner->GetFirstName(), owner);
#endif

        return newItem;
    }
    return NULL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint functions
#define PSABS(x)    ((x) < 0 ? -(x) : (x))

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check hour of day
bool WorkManager::constraintTime(WorkManager* that,char* param)
{
    // Check paramater pointer
    if(!param)
        return false;

    // Get game hour in 24 hours cycle
    int curTime = psserver->GetWeatherManager()->GetGameTODHour();
    int targetTime = atoi( param );

    if ( curTime == targetTime )
    {
        return true;
    }
    return false;
}

#define FRIEND_RANGE 5
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check if players are near worker (including worker)
//  Note: Constraint distance is limited to proximiy list.
bool WorkManager::constraintFriends(WorkManager* that, char* param)
{
    // Check paramater pointer
    if(!param)
        return false;

    // Count proximity player objects
    csArray< gemObject *> *targetsInRange  = that->worker->GetObjectsInRange( FRIEND_RANGE );

    size_t count = (size_t)atoi(param);
    if ( targetsInRange->GetSize() == count )
    {
        return true;
    }
    return false;
}


#define MAXDISTANCE 1
#define MAXANGLE 0.2
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check location
bool WorkManager::constraintLocation(WorkManager* that, char* param)
{
    int ch = ',';
    char* pdest;

    // Get the current position of client
    csVector3 pos;
    float yrot;
    iSector* sect;
    that->worker->GetPosition( pos, yrot, sect );

    // Parse through the constraint parameters
    pdest = strchr( param, ch );
    *pdest = '\0';
    char* sector = param;

    // go to the next parameter
    param = pdest + 1;
    pdest = strchr( param, ch );
    *pdest = '\0';
    char* xloc = param;

    // go to the next parameter
    param = pdest + 1;
    pdest = strchr( param, ch );
    *pdest = '\0';
    char* yloc = param;

    // go to the next parameter
    param = pdest + 1;
    pdest = strchr( param, ch );
    *pdest = '\0';
    char* zloc = param;

    // go to the next parameter
    param = pdest + 1;
    pdest = strchr( param, ch );
    *pdest = '\0';
    char* yrotation = param;

    // Skip if no sector constraint name specified
    if ( strlen(sector) != 0 )
    {
        // Check sector
        if ( strcmp( sector, sect->QueryObject()->GetName()) != 0)
            return false;
    }

    // Skip if no X constraint co-ord specified
    if ( strlen(xloc) != 0 )
    {
        // Check X location
        float x = atof( xloc );
        if ( PSABS(pos.x - x) > MAXDISTANCE )
            return false;
    }

    // Skip if no Y constraint co-ord specified
    if ( strlen(yloc) != 0 )
    {
        // Check Y location
        float y = atof( yloc );
        if ( PSABS(pos.y - y) > MAXDISTANCE )
            return false;
    }

    // Skip if no Z constraint co-ord specified
    if ( strlen(zloc) != 0 )
    {
        // Check Z location
        float z = atof( zloc );
        if ( PSABS(pos.z - z) > MAXDISTANCE )
            return false;
    }

    // Skip if no Yrot constraint co-ord specified
    if ( strlen(yrotation) != 0 )
    {
        // Check Y rotation
        float r = atof( yrotation );
        if ( PSABS(yrot - r) > MAXANGLE )
            return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check client mode
bool WorkManager::constraintMode(WorkManager* that, char* param)
{
    // Check mode string pointer
    if ( !that->preworkModeString )
        return false;

    // Check constraint mode to mode before work started
    if ( strcmp( that->preworkModeString, param) != 0 )
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check client gender
bool WorkManager::constraintGender(WorkManager* that, char* param)
{
    // Get race info
    psRaceInfo* race = that->owner->GetCharacterData()->GetRaceInfo();

    // Check constraint gender to player gender
    if ( strcmp( race->GetGender(), param) != 0 )
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check client race
bool WorkManager::constraintRace(WorkManager* that, char* param)
{
    // Get race info
    psRaceInfo* race = that->owner->GetCharacterData()->GetRaceInfo();

    // Check constraint race to player race
    if ( strcmp( race->GetRace(), param) != 0 )
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This handles trade work transformation events
void WorkManager::HandleWorkEvent(psWorkGameEvent* workEvent)
{
#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "handling work event...\n");
#endif
    // Load vars
    if ( !LoadLocalVars(workEvent->client,workEvent->gemTarget) )
    {
        Error1("Could not load local variables in HandleWorkEvent().\n");
        if (secure) psserver->SendSystemInfo(clientNum,"Could not load local variables in HandleWorkEvent().");
        owner->SetTradeWork(NULL);
        worker->SetMode(PSCHARACTER_MODE_PEACE);
        return;
    }

    int transType = workEvent->GetTransformationType();
    if(transType != TRANSFORMTYPE_TARGET_TO_NPC)
    {
        // Get active transformation and then do it
        trans = workEvent->GetTransformation();
        if (!trans)
        {
            Error1("No valid transformation in HandleWorkEvent().\n");
            if (secure) psserver->SendSystemInfo(clientNum,"No valid transformation in HandleWorkEvent().");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }

        // Get active process
        process = workEvent->GetProcess();
        if (!process)
        {
            Error1("No valid process pointer for transaction.\n");
            if (secure) psserver->SendSystemInfo(clientNum,"No valid process pointer for transaction.\n");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }

    // Check for exploits before starting
    workItem = workEvent->GetWorkItem();
    if (transType == TRANSFORMTYPE_CONTAINER)
    {
        // Get event data
        if (!workItem)
        {
                Error1("No valid transformation item in HandleWorkEvent().\n");
                if (secure) psserver->SendSystemInfo(clientNum,"No valid transformation item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
        }

        // Check to see if player walked away from non-auto container
        gemObject* target = workItem->GetGemObject();
        if (worker->RangeTo(target, true) > RANGE_TO_USE)
        {
            psserver->SendSystemOK(clientNum,"You interrupted your work when you moved away.");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }

    // Clear out work item event pointer
    if (workItem)
    {
        workItem->SetTransformationEvent(NULL);
    }

    // Set and load
    currentQuality = workEvent->GetResultQuality();
    const int originalQty = workEvent->GetResultQuantity();
    psItem* transItem = workEvent->GetTranformationItem();

    uint32 result = 0;
    int resultQty = 0;
    int itemQty = 0;

    if(trans)
    {
        result = trans->GetResultId();
        resultQty = trans->GetResultQty();
        itemQty = trans->GetItemQty();
    }

    if (!transItem)
    {
        Error1("Bad transformation item in HandleWorkEvent().\n");
        if (secure) psserver->SendSystemInfo(clientNum,"Bad transformation item in HandleWorkEvent().");
        owner->SetTradeWork(NULL);
        worker->SetMode(PSCHARACTER_MODE_PEACE);
        return;
    }

    // Check for item being moved from hand slots - container movement is handled by slot manager
    if (transType == TRANSFORMTYPE_SLOT)
    {
        INVENTORY_SLOT_NUMBER slot = workEvent->GetTransformationSlot();
        psItem *oldItem = owner->GetCharacterData()->Inventory().GetInventoryItem(slot);
        if (oldItem != workEvent->GetTranformationItem() || (oldItem && oldItem->GetStackCount() != originalQty))
        {
            psserver->SendSystemOK(clientNum,"You interrupted your work when you moved item.");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }

    // Check for item being moved from targetted hand slots
    if (transType == TRANSFORMTYPE_TARGETSLOT)
    {
        INVENTORY_SLOT_NUMBER slot = workEvent->GetTransformationSlot();
        psItem *oldItem = gemTarget->GetCharacterData()->Inventory().GetInventoryItem(slot);
        if (oldItem != workEvent->GetTranformationItem() || (oldItem && oldItem->GetStackCount() != originalQty))
        {
            psserver->SendSystemOK(clientNum,"You interrupted your transform when your target moved item.");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }
    // Check and apply the skills to quality and practice points
    float startQuality = transItem->GetItemQuality();
    if ( process )
    {
        if ( !ApplySkills(workEvent->GetKFactor(), workEvent->GetTranformationItem()) && process->GetGarbageId() != 0 )
        {
            result = process->GetGarbageId();
            resultQty = process->GetGarbageQty();
        }
    }

    //  Check for 0 quantity transformations
    if ( itemQty == 0 )
    {
        resultQty = workEvent->GetResultQuantity();
        itemQty = resultQty;
    }

    // Handle all the different transformation types
    switch( transType )
    {
        case TRANSFORMTYPE_SLOT:
        {
            INVENTORY_SLOT_NUMBER slot = workEvent->GetTransformationSlot();
            psItem* newItem = TransformSlotItem(slot, result, resultQty, currentQuality );
            if (!newItem && (result != 0))
            {
                Error1("TransformSlotItem did not create new item in HandleWorkEvent().\n");
                if (secure) psserver->SendSystemInfo(clientNum,"TransformSlotItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_TARGETSLOT:
        {
            INVENTORY_SLOT_NUMBER slot = workEvent->GetTransformationSlot();
            psItem* newItem = TransformTargetSlotItem(slot, result, resultQty, currentQuality );
            if (!newItem && (result != 0))
            {
                Error1("TransformTargetSlotItem did not create new item in HandleWorkEvent().\n");
                if (secure) psserver->SendSystemInfo(clientNum,"TransformSlotItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_TARGET:
        {
            psItem* newItem = TransformTargetItem(workEvent->GetTranformationItem(), result, resultQty, currentQuality );
            if (!newItem && (result != 0))
            {
                Error1("TransformTargetItem did not create new item in HandleWorkEvent().\n");
                if (secure) psserver->SendSystemInfo(clientNum,"TransformSlotItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_TARGET_TO_NPC:
        {
            bool is_trap = workEvent->GetTranformationItem()->GetIsTrap();
            TransformTargetItemToNpc(workEvent->GetTranformationItem(), workEvent->client);

            if(is_trap)
            {
                psserver->SendSystemInfo(clientNum, "Trap constructed!");
            }

            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }

        case TRANSFORMTYPE_SELF_CONTAINER:
        {
            psItem* newItem = TransformSelfContainerItem(workEvent->GetTranformationItem(), result, resultQty, currentQuality );
            if (!newItem  && (result != 0))
            {
                Error1("TransformSelfContainerItem did not create new item in HandleWorkEvent().\n");
                if (secure) psserver->SendSystemInfo(clientNum,"TransformSelfContainerItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_AUTO_CONTAINER:
        case TRANSFORMTYPE_SLOT_CONTAINER:
        case TRANSFORMTYPE_CONTAINER:
        {
            psItem* newItem = TransformContainedItem(workEvent->GetTranformationItem(), result, resultQty, currentQuality );
            if (!newItem  && (result != 0))
            {
                Error1("TransformContainedItem did not create new item in HandleWorkEvent().\n");
                if (secure) psserver->SendSystemInfo(clientNum,"TransformContainedItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_UNKNOWN:
        default:
        {
            break;
        }
    }

    // Calculate and apply experience points
    if (result > 0)
    {
        unsigned int experiencePoints;
        {
            MathEnvironment env;
            env.Define("StartQuality", startQuality);
            env.Define("CurrentQuality", currentQuality);
            env.Define("Character", owner);
            calc_transform_exp->Evaluate(&env);
            experiencePoints = env.Lookup("Exp")->GetValue();
        }

        owner->GetCharacterData()->AddExperiencePointsNotify(experiencePoints);

        Debug2(LOG_TRADE, clientNum, "Giving experience points %i.\n",experiencePoints);
        if (secure) psserver->SendSystemInfo(clientNum,"Giving experience points %i.",experiencePoints);
    }

    // Let the user know the we have done something
    if (result <= 0 )
    {
        psItemStats* itemStats = CacheManager::GetSingleton().GetBasicItemStatsByID( trans->GetItemId() );
        if (itemStats)
        {
            psserver->SendSystemOK(clientNum," %i %s is gone.", itemQty, itemStats->GetName());
        }
        else
        {
            Error2("HandleWorkEvent() could not get item stats for item ID #%u.", trans->GetItemId());
            if (secure) psserver->SendSystemInfo(clientNum,"HandleWorkEvent() could not get item stats for item ID #%u.", trans->GetItemId());
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }
    else
    {
        psItemStats* resultStats = CacheManager::GetSingleton().GetBasicItemStatsByID( result );
        if (resultStats)
        {
            psserver->SendSystemOK(clientNum,"You made %i %s.", resultQty, resultStats->GetName() );
        }
        else
        {
            Error2("HandleWorkEvent() could not get result item stats for item ID #%u.", result);
            if (secure) psserver->SendSystemInfo(clientNum,"HandleWorkEvent() could not get result item stats for item ID #%u.", result);
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }

    // If there is something left lets see what other damage we can do
    if( (result > 0) )
    {
        // If it's an auto transformation keep going
        if( (transType == TRANSFORMTYPE_AUTO_CONTAINER) )
        {
            // Check to see if we have pattern
            if ( ValidateMind() )
            {
                //TODO: Maybe all those function like this which seems copy & paste should be put in an unique one?
                // Check if there is another transformation possible for the item just created
                unsigned int transMatch = AnyTransform( patternId, groupPatternId, result, resultQty );
                if ( (transMatch == TRANSFORM_MATCH ) || (transMatch == TRANSFORM_GARBAGE ) )
                {
                    // Set up event for new transformation with the same setting as the old
                    StartTransformationEvent(
                        workEvent->GetTransformationType(), workEvent->GetTransformationSlot(),
                        workEvent->GetResultQuantity(), workEvent->GetResultQuality(),
                        workEvent->GetTranformationItem());
                    if(transMatch == TRANSFORM_GARBAGE)
                        psserver->SendSystemError(clientNum,"You are not sure what is going to happen to %d %s.", workEvent->GetTranformationItem()->GetStackCount(), workEvent->GetTranformationItem()->GetName());
                    return;
                }
            }
        }

        // If item in any container clean it up
        if( (transType == TRANSFORMTYPE_AUTO_CONTAINER) ||
            (transType == TRANSFORMTYPE_CONTAINER) )
        {
            // If no transformations started go ahead and start a cleanup event
            psItem* newItem = workEvent->GetTranformationItem();
            if (newItem) {
                StartCleanupEvent(transType,workEvent->client, newItem, workEvent->client->GetActor());
            }
        }
    }

    // Make sure we clear out the old event and go to peace
    owner->SetTradeWork(NULL);
    worker->SetMode(PSCHARACTER_MODE_PEACE);
}

// Apply skills if any to quality and practice points
bool WorkManager::ApplySkills(float factor, psItem* transItem)
{
    // just return for processless transforms
    if (!process)
    {
        if (secure) psserver->SendSystemInfo(clientNum,"Processless transforms give no practice points or quality change.");
        return true;
    }

    // Check for a primary skill
    float startingQuality = currentQuality;
    if (secure) psserver->SendSystemInfo(clientNum,"Starting quality calculation with quality %f.", startingQuality);
    int priSkill = process->GetPrimarySkillId();
    if ( priSkill >= 0)
    {
        // Increase quality for crafted item based on if the starting quality was less then the normal quality
        float baseQuality = transItem->GetBaseStats()->GetQuality();
        if (currentQuality > baseQuality)
        {
            // Add the transfromation items base quality to the current quality as a crafting bonus
            if (secure) psserver->SendSystemInfo(clientNum,"Primary skill add crafting bonus of item base quality %f.", baseQuality);
            currentQuality = currentQuality + baseQuality;
        }
        else
        {
            // Double the current quality as a crafting bonus
            if (secure) psserver->SendSystemInfo(clientNum,"Primary skill double current quality.");
            currentQuality = currentQuality * 2;
        }

        // Get the players skill level using the transformations primary skill
        unsigned int basePriSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)priSkill).Current();
        unsigned int maxPriSkill = process->GetMaxPrimarySkill();

        // Get the quality factor for this primary skill
        //  and only use it if in range.
        // This value represents what percentage of the effect of skills should be
        //  applied to the quality calculation for this transformation
        float priQualFactor = (process->GetPrimaryQualFactor())/100.00;
        if ((priQualFactor > 0.00) && (priQualFactor < 1.00))
        {
            // For quality considerations cap the base skill
            //  at the max skill for this transformation.
            int capPriSkill = ( basePriSkill > maxPriSkill ) ? maxPriSkill : basePriSkill;

            // Calculate the lack of skill as a percentage of the capped skill over the skill range.
            // Since this is a lack of skill percentage subtract it from 1.
            int minPriSkill = process->GetMinPrimarySkill();
            float priSkillLessPercent = 0;
            if((maxPriSkill-minPriSkill)>0)
            {
                priSkillLessPercent = 1 - ((float)(capPriSkill-minPriSkill)/(float)(maxPriSkill-minPriSkill));
                if (secure) psserver->SendSystemInfo(clientNum,"Primary skill base level %d and max process level %d gives capped skill level %d.", basePriSkill, maxPriSkill, capPriSkill);
                if (secure) psserver->SendSystemInfo(clientNum,"Min process level %d gives skill percent %f.", minPriSkill, priSkillLessPercent);
            }

            // Calculate the effect of the quality factor for this skill by the skill level
            // Subtract it as a percentage from the current ingredient quality
            currentQuality -= startingQuality * priSkillLessPercent * priQualFactor;
            if (secure) psserver->SendSystemInfo(clientNum,"Current quality reduced to %f by starting quality %f times skill percent %f times process quality factor %f.", currentQuality, startingQuality, priSkillLessPercent, priQualFactor);
        }

        // Only give primary experience to those under the max
        if ( basePriSkill < maxPriSkill )
        {
            // Get some practice in
            int priPoints = process->GetPrimaryPracticePts();
            owner->GetCharacterData()->CalculateAddExperience((PSSKILL)priSkill, priPoints);
            if (secure) psserver->SendSystemInfo(clientNum,"Giving practice points %d to skill %d.",priPoints, priSkill);
        }

        // Apply the secondary skill if any
        int secSkill = process->GetSecondarySkillId();
        if ( secSkill >= 0)
        {
            // Increase quality for crafted item based on if the starting quality was less then the normal quality
            float baseQuality = transItem->GetBaseStats()->GetQuality();
            if (currentQuality > baseQuality)
            {
                // Add the transfromation items base quality to the current quality as a crafting bonus
                if (secure) psserver->SendSystemInfo(clientNum,"Secondary skill add crafting bonus of item base quality %f.", baseQuality);
                currentQuality = currentQuality + baseQuality;
            }
            else
            {
                // Double the current quality as a crafting bonus
                if (secure) psserver->SendSystemInfo(clientNum,"Secondary skill double current quality.");
                currentQuality = currentQuality * 2;
            }

            unsigned int baseSecSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)secSkill).Current();
            unsigned int maxSecSkill = process->GetMaxSecondarySkill();

            // Get the quality factor for this secmary skill
            //  and only use it if in range.
            float secQualFactor = (process->GetSecondaryQualFactor())/100.00;
            if ((secQualFactor > 0.00) && (secQualFactor < 1.00))
            {
                // For quality considerations cap the base skill
                //  at the max skill for this transformation.
                int capSecSkill = ( baseSecSkill > maxSecSkill ) ? maxSecSkill : baseSecSkill;

                // Calculate the lack of skill as a percentage of the capped skill over the skill range.
                // Since this is a lack of skill percentage subtract it from 1.
                int minSecSkill = process->GetMinSecondarySkill();
                float secSkillLessPercent = 0;
                if((maxSecSkill-minSecSkill) > 0)
                {
                    secSkillLessPercent = 1 - ((float)(capSecSkill-minSecSkill)/(float)(maxSecSkill-minSecSkill));
                    if (secure) psserver->SendSystemInfo(clientNum,"Secondary skill base level %d and max process level %d gives capped skill level %d.", baseSecSkill, maxSecSkill, capSecSkill);
                    if (secure) psserver->SendSystemInfo(clientNum,"Min process level %d gives skill percent %f.", minSecSkill, secSkillLessPercent);
                }

                // Calculate the effect of the quality factor for this skill by the skill level
                currentQuality -= startingQuality * secSkillLessPercent * secQualFactor;
                if (secure) psserver->SendSystemInfo(clientNum,"Current quality reduced to %f by starting quality %f times skill percent %f times process quality factor %f.", currentQuality, startingQuality, secSkillLessPercent, secQualFactor);
            }

            // Only give secondary experience to those under the max
            if ( baseSecSkill < maxSecSkill )
            {
                // Get some practice in
                int secPoints = process->GetSecondaryPracticePts();
                owner->GetCharacterData()->CalculateAddExperience((PSSKILL)secSkill, secPoints);
                if (secure) psserver->SendSystemInfo(clientNum,"Giving practice points %d to skill %d.",secPoints, secSkill);
            }
        }

#ifndef NO_RANDOM_QUALITY
        // Randomize the final quality results
        // We are using a logrithmic calculation so that normally there is little quality change
        //  except at the edges of the random distribution.
        // Use a pattern specific factor to determine the curve at the edges
        float roll = psserver->rng->Get();
        float expFactor = factor*log((1/roll)-1);
        currentQuality = currentQuality -((currentQuality*expFactor)/100);
        if (secure) psserver->SendSystemInfo(clientNum,"Applying random effect changes quality to %f using factor %f on roll %f.", currentQuality, expFactor, roll);
#endif
    }
    else
    {
        if (secure) psserver->SendSystemInfo(clientNum,"This transform gives no practice points or quality change.");
    }

    // Adjust the final quality with the transformation quality factor
    currentQuality = currentQuality * trans->GetItemQualityPeniltyPercent();
    if (secure) psserver->SendSystemInfo(clientNum,"Applying transformation penilty %f changes quality to %f.", trans->GetItemQualityPeniltyPercent(),currentQuality);

    // Check for range
    if ( currentQuality > 999.99)
    {
        currentQuality = 999.99F;
    }

    // Fail if it's worst then worse
    else if (currentQuality < 10.0)
    {
        currentQuality = 0;
        if (secure) psserver->SendSystemInfo(clientNum,"Failed quality check.  Creating garbage item.");
        return false;
    }
    return true;
}

void WorkManager::SendTransformError( uint32_t clientNum, unsigned int result, uint32 curItemId, int curItemQty )
{
    csString error("");

    // There is some hierachy to the error codes
    //  for example, bad quantity is only important if no other error is found
    if (result & TRANSFORM_UNKNOWN_WORKITEM)
    {
        psserver->SendSystemError(clientNum, "Nothing has been chosen to use.");
        return;
    }
    if (result & TRANSFORM_FAILED_CONSTRAINTS)
    {
        // Message is sent by constraint logic.
        // error = "The conditions are not right for this to work.";
        return;
    }
    if (result & TRANSFORM_UNKNOWN_PATTERN)
    {
        psserver->SendSystemError(clientNum, "You are unable to think about what to do with this.");
        return;
    }
    if (result & TRANSFORM_BAD_TRAINING)
    {
        psserver->SendSystemError(clientNum, "You need some more training before you can do this sort of work.");
        return;
    }
    if (result & TRANSFORM_BAD_SKILLS)
    {
        psserver->SendSystemError(clientNum, "This is a bit beyond your skills at this moment.");
        return;
    }
    if (result & TRANSFORM_OVER_SKILLED)
    {
        psserver->SendSystemError(clientNum, "You loose interest in these simple tasks and cannot complete the work.");
        return;
    }
    if (result & TRANSFORM_NO_STAMINA)
    {
        psserver->SendSystemError(clientNum, "You are either too tired or not focused enough to do this now.");
        return;
    }
    if (result & TRANSFORM_MISSING_EQUIPMENT)
    {
        psserver->SendSystemError(clientNum, "You do not have the correct tools for this sort of work.");
        return;
    }
    if (result & TRANSFORM_BAD_QUANTITY)
    {
        psserver->SendSystemError(clientNum, "This doesn't seem to be the right amount to do anything useful.");
        return;
    }
    if (result & TRANSFORM_BAD_USE)
    {
        psserver->SendSystemError(clientNum, "There is nothing in this container that you own.");
        return;
    }
    if (result & TRANSFORM_TOO_MANY_ITEMS)
    {
        psserver->SendSystemError(clientNum, "You can only work on one item at a time this way.");
        return;
    }
    if (result & TRANSFORM_BAD_COMBINATION)
    {
        psserver->SendSystemError(clientNum, "You do not have the right amount of the right items for what you have in mind.");
        return;
    }
    if (result & TRANSFORM_MISSING_ITEM)
    {
        psserver->SendSystemError(clientNum, "You are not sure what to do with this.");
        return;
    }
    if (result & TRANSFORM_UNKNOWN_ITEM)
    {
        psItemStats* curStats = CacheManager::GetSingleton().GetBasicItemStatsByID( curItemId );
        if(curStats)
        {
            psserver->SendSystemError(clientNum, "You are not sure what to do with %d %s.", curItemQty, curStats->GetName());
        }
        return;
    }
    if (result & TRANSFORM_GONE_WRONG)
    {
        psserver->SendSystemError(clientNum, "Something has obviously gone wrong.");
        return;
    }
    if (result & TRANSFORM_GARBAGE)
    {
        psItemStats* curStats = CacheManager::GetSingleton().GetBasicItemStatsByID( curItemId );
        if(curStats)
        {
            psserver->SendSystemError(clientNum, "You are not sure what to do with this.");
        }
        return;
    }
    Error4("SendTransformError() got unknown result type #%d from item ID #%u (%d).", result, curItemId, curItemQty);
    return;
}

//-----------------------------------------------------------------------------
// Cleaning up discarded items from public containers
//-----------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing cleanup work
void WorkManager::StopCleanupWork(Client* client, psItem* cleanItem)
{
    // Check for proper autoItem
    if ( !cleanItem )
    {
        Error1("StopCleanupWork does not have a good cleanItem pointer.");
        return;
    }

    // Stop the cleanup item's work event
    psWorkGameEvent* workEvent = cleanItem->GetTransformationEvent();
    if (workEvent)
    {
        cleanItem->SetTransformationEvent(NULL);
        workEvent->Interrupt();
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Start doing cleanup
void WorkManager::StartCleanupEvent(int transType, Client* client, psItem* item, gemActor* worker)
{
    if (!item)
    {
        Error1("No valid transformation item in StartCleanupEvent().\n");
        return;
    }

#ifdef DEBUG_WORKMANAGER
    // Let the server admin know
    CPrintf(CON_DEBUG, "Scheduled item id #%d to be cleaned up in %d seconds.\n", item->GetUID(), CLEANUP_DELAY);
#endif

    // Set event
    csVector3 pos(0,0,0);
    psWorkGameEvent* workEvent = new psWorkGameEvent(
        this, worker, CLEANUP_DELAY*1000, CLEANUP, pos, NULL, client );
    workEvent->SetTransformationType(transType);
    workEvent->SetTransformationItem(item);
    item->SetTransformationEvent(workEvent);
    psserver->GetEventManager()->Push(workEvent);

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Remove ownership of item making item free game.
void WorkManager::HandleCleanupEvent(psWorkGameEvent* workEvent)
{
#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "handling cleanup workEvent...\n");
#endif

    // Get event item
    psItem* cleanItem = workEvent->GetTranformationItem();
    if ( cleanItem )
    {
        cleanItem->SetGuardingCharacterID(0);
        cleanItem->Save(true);
        cleanItem->SetTransformationEvent(NULL);
        psserver->SendSystemOK(workEvent->client->GetClientNum(),"You probably should not leave %d %s here.", cleanItem->GetStackCount(), cleanItem->GetName());
    }
}

//-----------------------------------------------------------------------------
// Lock picking
//-----------------------------------------------------------------------------

void WorkManager::StartLockpick(Client* client,psItem* item)
{
    // Check if its a lock
    if(!item->GetIsLockable())
    {
        psserver->SendSystemInfo(client->GetClientNum(),"This object isn't lockable");
        return;
    }

    // Check if player has a key
    if (client->GetCharacterData()->Inventory().HaveKeyForLock(item->GetUID()))
    {
        bool locked = item->GetIsLocked();
        // Make sure the player knows he unlocked or locked his lock. Imagine guildhouse left unlocked on accident.
        if (locked)
        {
            psserver->SendSystemError(client->GetClientNum(), "You unlocked %s.", item->GetName());
        }
        else
        {
            psserver->SendSystemOK(client->GetClientNum(), "You locked %s.", item->GetName());
        }
        item->SetIsLocked(!locked);
        item->Save(false);
        return;
    }

    if (item->GetIsUnpickable())
    {
        psserver->SendSystemInfo(client->GetClientNum(),"This lock is impossible to %s by lockpicking.", item->GetIsLocked() ? "open" : "close");
        return;
    }

    if (!CheckStamina( client->GetCharacterData() ))
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You are too tired to lockpick");
        return;
    }

    client->GetCharacterData()->SetStaminaRegenerationWork(PSSKILL_LOCKPICKING);

    psserver->SendSystemInfo(client->GetClientNum(),"You started lockpicking %s",item->GetName());
    client->GetActor()->SetMode(PSCHARACTER_MODE_WORK);

    // Execute mathscript to get lockpicking time
    MathEnvironment env;
    env.Define("LockQuality", item->GetItemQuality());
    calc_lockpick_time->Evaluate(&env);
    MathVar *time = env.Lookup("Time");

    // Add new event
    csVector3 emptyV = csVector3(0,0,0);
    psWorkGameEvent *ev = new psWorkGameEvent(
        this,
        client->GetActor(),
        (int) time->GetValue(),
        LOCKPICKING,
        emptyV,
        0,
        client,
        item);
    psserver->GetEventManager()->Push(ev);
}

void WorkManager::LockpickComplete(psWorkGameEvent* workEvent)
{
    psCharacter* character = workEvent->client->GetCharacterData();
    PSSKILL skill = workEvent->object->GetLockpickSkill();

    // Check if not moved too far away from lock
    // First check if lock (object such as box) is in not inventory
    // For now, that is never the case, but I can imagine that changing
    if ( (PSCHARACTER_SLOT_NONE == workEvent->object->GetLocInParent()) &&
         (workEvent->client->GetActor()->RangeTo(workEvent->object->GetGemObject()) > RANGE_TO_USE) )
    {
        // Send denied message
        psserver->SendSystemInfo(workEvent->client->GetClientNum(),"You failed your lockpicking attempt, because you moved away.");
    }
    else
    {
        // Check if the user has the right skills
        int rank = 0;
        if (skill >= 0 && skill < PSSKILL_COUNT)
            rank = character->Skills().GetSkillRank(skill).Current();

        if (rank >= (int) workEvent->object->GetLockStrength())
        {
            bool locked = workEvent->object->GetIsLocked();
            psserver->SendSystemOK(workEvent->client->GetClientNum(), locked ? "You unlocked %s." : "You locked %s.", workEvent->object->GetName());
            workEvent->object->SetIsLocked(!locked);
            workEvent->object->Save(false);
        }
        else
        {
            // Send denied message
            psserver->SendSystemInfo(workEvent->client->GetClientNum(),"You failed your lockpicking attempt.");
        }
    }
    workEvent->client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
}


//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////
//  Primary work object
psWorkGameEvent::psWorkGameEvent(WorkManager* mgr,
                                 gemActor* worker,
                                 int delayticks,
                                 int cat,
                                 csVector3& pos,
                                 NaturalResource *natres,
                                 Client *c,
                                 psItem* object,
                                 float repairAmt)
                                 : psGameEvent(0,delayticks,"psWorkGameEvent"), effectID(0)
{
    workmanager = mgr;
    nr          = natres;
    client      = c;
    position    = pos;
    category    = cat;
    this->object = object;
    this->worker = worker;
    worker->RegisterCallback(this);
    repairAmount = repairAmt;
}


void psWorkGameEvent::Interrupt()
{
    // Setting the character mode ends up calling this function again, so we have to check for reentrancy here
    if (workmanager)
    {
        workmanager = NULL;

        // TODO: get tick count so far before killing event
        //  for partial trade work - store it in psItem
        //  leave work state alone so it can be restarted

        // Stop event from being executed when triggered.
        if(category == REPAIR && client->GetActor() && (client->GetActor()->GetMode() == PSCHARACTER_MODE_WORK))
        {
           client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);

            psItem *repairTarget = client->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
            if (repairTarget)
                repairTarget->SetInUse(false);
        }
    }
}


// Event trigger
void psWorkGameEvent::Trigger()
{
    if(workmanager && worker.IsValid())
    {
        if ( effectID != 0 )
        {
            psStopEffectMessage newmsg(effectID);
            newmsg.Multicast(multi,0,PROX_LIST_ANY_RANGE);
        }

        // Work done for now
        switch(category)
        {
            case(REPAIR):
            {
                workmanager->HandleRepairEvent(this);
                break;
            }
            case(MANUFACTURE):
            {
                workmanager->HandleWorkEvent(this);
                break;
            }
            case(PRODUCTION):
            {
                workmanager->HandleProductionEvent(this);
                break;
            }
            case(LOCKPICKING):
            {
                workmanager->LockpickComplete(this);
                break;
            }
            case(CLEANUP):
            {
                workmanager->HandleCleanupEvent(this);
                break;
            }
            default:
                Error1("Unknown transformation type!");
                break;
        }
    }
}

void psWorkGameEvent::DeleteObjectCallback(iDeleteNotificationObject * object)
{
    if(workmanager && worker.IsValid())
    {
        worker->UnregisterCallback(this);

        // Cleanup any autoitem associated with event
        if( transType == TRANSFORMTYPE_AUTO_CONTAINER )
        {
            if( item )
            {

#ifdef DEBUG_WORKMANAGER
CPrintf(CON_DEBUG, "Cleaning up item from auto-transform container...\n");
#endif
                // clear out event
                item->SetTransformationEvent(NULL);

            }
            Interrupt();
        }
        else
        {
            // Setting character to peace will interrupt work event
            worker->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
        }
    }
}

psWorkGameEvent::~psWorkGameEvent()
{
    if(worker.IsValid())
    {
        worker->UnregisterCallback(this);
        if (worker->GetTradeWork() == this)
            worker->SetTradeWork(NULL);
    }

    // For manufactuing events only
    if( category == MANUFACTURE )
    {
        // Remove any item reference to this  event
        psItem* transItem = GetTranformationItem();
        if( transItem && transItem->GetTransformationEvent() == this)
            transItem->SetTransformationEvent(NULL);

        // The garbase transformation is not cached and needs to be cleaned up
        if(transformation && !transformation->GetTransformationCacheFlag() )
        {
            delete transformation;
        }
    }
}
