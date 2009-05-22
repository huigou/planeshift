/*
 * progressionmanager.cpp
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
#include <stdlib.h>
#include "globals.h"

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iutil/object.h>
#include <csutil/scfstr.h>
#include <iengine/engine.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/skillcache.h"
#include "util/eventmanager.h"
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"
#include "util/psxmlparser.h"

#include "rpgrules/factions.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/pstrainerinfo.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/pstrait.h"

#include "net/messages.h"
#include "net/npcmessages.h"

#include "engine/psworld.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "clients.h"
#include "psserver.h"
#include "events.h"
#include "psserverchar.h"
#include "playergroup.h"
#include "gem.h"
#include "progressionmanager.h"
#include "entitymanager.h"
#include "cachemanager.h"
#include "spellmanager.h"
#include "weathermanager.h"
#include "actionmanager.h"
#include "workmanager.h"
#include "npcmanager.h"
#include "usermanager.h"
#include "introductionmanager.h"
#include "adminmanager.h"

ProgressionManager::ProgressionManager(ClientConnectionSet *ccs)
{
    clients      = ccs;

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<ProgressionManager>(this,&ProgressionManager::HandleSkill)      ,MSGTYPE_GUISKILL,    REQUIRE_READY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<ProgressionManager>(this,&ProgressionManager::HandleDeathEvent) ,MSGTYPE_DEATH_EVENT, NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<ProgressionManager>(this,&ProgressionManager::HandleZPointEvent),MSGTYPE_ZPOINT_EVENT,NO_VALIDATION);
}


ProgressionManager::~ProgressionManager()
{
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_GUISKILL);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DEATH_EVENT);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_ZPOINT_EVENT);
}

bool ProgressionManager::Initialize()
{
    Result result_affinitycategories(db->Select("SELECT * from char_create_affinity"));

    if ( result_affinitycategories.IsValid() )
    {
        for ( unsigned int x = 0; x < result_affinitycategories.Count(); x++ )
        {
            affinitycategories.Put( csString( result_affinitycategories[(unsigned long)x]["category"]).Downcase() , csString( result_affinitycategories[(unsigned long)x]["attribute"]).Downcase() );
        }
    }

    return true;
}


void ProgressionManager::HandleZPointEvent(MsgEntry *me, Client *client)
{
    psZPointsGainedEvent evt(me);

    csString string;
    string.Format("You've gained some practice points in %s.", evt.skillName.GetData() );
    if ( evt.rankUp )
    {
        string.Append(" You've also ranked up!");
    }
    psserver->SendSystemInfo(evt.actor->GetClientID(), string);

    SendSkillList( client, false );
}


void ProgressionManager::HandleDeathEvent(MsgEntry *me, Client *notused)
{
    Debug1(LOG_COMBAT, me->clientnum,"Progression Manager handling Death Event\n");
    psDeathEvent evt(me);

    // Only award experience if the dead actor is a NPC and not a pet, or a GM with the givekillexp flag.
    if (evt.killer && ((evt.deadActor->GetClientID()==0 && !evt.deadActor->GetCharacterData()->IsPet()) || evt.deadActor->givekillexp))
    {
        ProgressionScript *script = FindScript("kill");
        MathEnvironment env;
        env.Define("Killer",    evt.killer);
        env.Define("DeadActor", evt.deadActor);
        env.Define("KillExp",   evt.deadActor->GetCharacterData()->GetKillExperience());
        script->Run(&env);
    }
}


void ProgressionManager::HandleSkill(MsgEntry *me, Client * client)
{
    psGUISkillMessage msg(me);
    if (!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Received unparsable psGUISkillMessage from client %u.\n",me->clientnum);
        return;
    }

    //    CPrintf(CON_DEBUG, "ProgressionManager::HandleSkill(%d,%s)\n",msg.command, (const char*)msg.commandData);
    switch ( msg.command )
    {
        case psGUISkillMessage::REQUEST:
        {
            // Clear the current skill cache
            psCharacter * character = client->GetCharacterData();
            if (character)
                character->GetSkillCache()->clear();

            SendSkillList(client,false);
            break;
        }
        case psGUISkillMessage::SKILL_SELECTED:
        {
            csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

            CS_ASSERT( xml );

            csRef<iDocument> invList  = xml->CreateDocument();

            const char* error = invList->Parse( msg.commandData);
            if ( error )
            {
                Error2("Error in XML: %s", error );
                return;
            }

            csRef<iDocumentNode> root = invList->GetRoot();
            if(!root)
            {
                Error1("No XML root");
                return;
            }
            csRef<iDocumentNode> topNode = root->GetNode("S");
            if(!topNode)
            {
                Error1("No <S> tag");
                return;
            }

            csString skillName = topNode->GetAttributeValue("NAME");

            psSkillInfo * info = CacheManager::GetSingleton().GetSkillByName(skillName);
            Faction * faction = CacheManager::GetSingleton().GetFactionByName(skillName);
            csString buff;
            csString description;
            int cathegory;
            if (info)
            {
                description = EscpXML(info->description).GetData();
                cathegory = info->category;
            }
            else if (faction)
            {
                description = faction->description;
                cathegory = PSSKILLS_CATEGORY_FACTIONS;
            }
            else
            {
                description = "";
                cathegory = PSSKILLS_CATEGORY_VARIOUS;
            }
            buff.Format("<DESCRIPTION NAME=\"%s\" DESC=\"%s\" CAT=\"%d\"/>",
                        EscpXML(skillName).GetData(), description.GetData(),
                        cathegory);

            psCharacter* chr = client->GetCharacterData();
            psGUISkillMessage newmsg(client->GetClientNum(),
                            psGUISkillMessage::DESCRIPTION,
                            buff,
                            NULL,
                            (unsigned int)(chr->Stats()[PSITEMSTATS_STAT_STRENGTH].Current()),
                            (unsigned int)(chr->Stats()[PSITEMSTATS_STAT_ENDURANCE].Current()),
                            (unsigned int)(chr->Stats()[PSITEMSTATS_STAT_AGILITY].Current()),
                            (unsigned int)(chr->Stats()[PSITEMSTATS_STAT_INTELLIGENCE].Current()),
                            (unsigned int)(chr->Stats()[PSITEMSTATS_STAT_WILL].Current()),
                            (unsigned int)(chr->Stats()[PSITEMSTATS_STAT_CHARISMA].Current()),
                            (unsigned int)(chr->GetHP()),
                            (unsigned int)(chr->GetMana()),
                            (unsigned int)(chr->GetStamina(true)),
                            (unsigned int)(chr->GetStamina(false)),
                            (unsigned int)(chr->GetMaxHP().Current()),
                            (unsigned int)(chr->GetMaxMana().Current()),
                            (unsigned int)(chr->GetMaxPStamina().Current()),
                            (unsigned int)(chr->GetMaxMStamina().Current()),
                            true,
                            PSSKILL_NONE,
                            -1,
                            false);

            if (newmsg.valid)
                SendMessage(newmsg.msg);
            else
            {
                Bug2("Could not create valid psGUISkillMessage for client %u.\n",client->GetClientNum());
            }
            break;
        }
        case psGUISkillMessage::BUY_SKILL:
        {
            Debug1(LOG_SKILLXP, client->GetClientNum(),"---------------Buying Skill-------------\n");
            csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

            CS_ASSERT( xml );

            csRef<iDocument> invList  = xml->CreateDocument();

            const char* error = invList->Parse( msg.commandData);
            if ( error )
            {
                Error2("Error in XML: %s", error );
                return;
            }

            csRef<iDocumentNode> root = invList->GetRoot();
            if(!root)
            {
                Error1("No XML root");
                return;
            }
            csRef<iDocumentNode> topNode = root->GetNode("B");
            if(!topNode)
            {
                Error1("No <B> tag");
                return;
            }

            csString skillName = topNode->GetAttributeValue("NAME");
            uint skillAmount = topNode->GetAttributeValueAsInt("AMOUNT");

            psSkillInfo * info = CacheManager::GetSingleton().GetSkillByName(skillName);
            Debug2(LOG_SKILLXP, client->GetClientNum(),"    Looking for: %s\n", (const char*)skillName);

            if (!info)
            {
                Error2("No skill with name %s found!",skillName.GetData());
                Error2("Full Data Sent from Client was: %s\n", msg.commandData.GetData() );
                return;
            }

            psCharacter * character = client->GetCharacterData();

            if (character->GetTrainer() == NULL)
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "Can't buy skills when not training!");
                return;
            }

            gemActor* actorTrainer = character->GetTrainer()->GetActor();
            if ( actorTrainer )
            {
                if ( character->GetActor()->RangeTo(actorTrainer, false) > RANGE_TO_SELECT )
                {
                    psserver->SendSystemInfo(client->GetClientNum(),
                                             "Need to get a bit closer to understand the training.");
                    return;
                }
            }

            if (client->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE)
            {
                csString err;
                err.Format("You can't train while %s.", client->GetActor()->GetModeStr());
                psserver->SendSystemError(client->GetClientNum(), err);
                return;
            }

            Debug2(LOG_SKILLXP, client->GetClientNum(),"    PP available: %u\n", character->GetProgressionPoints() );

            // Test for progression points
            if (character->GetProgressionPoints() < skillAmount)
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You don't have enough progression points!");
                return;
            }

            // Test for money

            if ((info->price * skillAmount) > character->Money())
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You don't have the money to buy this skill!");
                return;
            }
            if ( !character->CanTrain( info->id ) )
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You cannot train this skill any higher yet!");
                return;
            }

            unsigned int current = character->Skills().GetSkillRank((PSSKILL) info->id).Base();
            float faction = actorTrainer->GetRelativeFaction(character->GetActor());
            if ( !character->GetTrainer()->GetTrainerInfo()->TrainingInSkill((PSSKILL)info->id, current, faction))
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You cannot train this skill currently.");
                return;
            }

            character->UseProgressionPoints(skillAmount);
            character->SetMoney(character->Money()-(info->price * skillAmount));
            character->Train(info->id,skillAmount);
            SendSkillList(client,true,info->id);
            psserver->GetCharManager()->UpdateItemViews(client->GetClientNum());
            psserver->SendSystemInfo(client->GetClientNum(), "You've received some %s training", skillName.GetData());

            break;
        }
        case psGUISkillMessage::QUIT:
        {
            client->GetCharacterData()->SetTrainer(NULL);
            client->GetCharacterData()->GetSkillCache()->clear();
            break;
        }
    }
}

void ProgressionManager::SendSkillList(Client * client, bool forceOpen, PSSKILL focus, bool isTraining )
{
    psCharacter * character = client->GetCharacterData();
    psCharacter * trainer = character->GetTrainer();
    psTrainerInfo * trainerInfo = NULL;
    float faction = 0.0;
    int selectedSkillCat = -1; //This is used for storing the category of the selected skill
    int selectedSkillNameId = -1; // Name ID value of the selected skill

    // Get the current skill cache
    psSkillCache *skills = character->GetSkillCache();

    skills->setProgressionPoints(character->GetProgressionPoints());

    if (trainer)
    {
        trainerInfo = trainer->GetTrainerInfo();
        faction = trainer->GetActor()->GetRelativeFaction(character->GetActor());
    }


    for (int skillID = 0; skillID < (int)PSSKILL_COUNT; skillID++)
    {
        psSkillInfo * info = CacheManager::GetSingleton().GetSkillByID(skillID);
        if (!info)
        {
            Error2("Can't find skill %d",skillID);
            return;
        }

        Skill & charSkill = character->Skills().Get((PSSKILL) skillID);

        // If we are training, send skills that the trainer is providing education in only
        if  (
                !trainerInfo
                    ||
                trainerInfo->TrainingInSkill((PSSKILL) skillID, character->Skills().GetSkillRank((PSSKILL) skillID).Base(), faction)
             )
        {
            bool stat = info->id == PSSKILL_AGI ||
                        info->id == PSSKILL_CHA ||
                        info->id == PSSKILL_END ||
                        info->id == PSSKILL_INT ||
                        info->id == PSSKILL_WILL ||
                        info->id == PSSKILL_STR;

            /* Get the ID value for the skill name string and find the skill
               in the cache. If it can't be found, skip this skill.
            */
            unsigned int skillNameId =
                    CacheManager::GetSingleton().FindCommonStringID(info->name);

            if (skillNameId == 0)
            {
                Error2("Can't find skill name \"%s\" in common strings", info->name.GetData());
                continue;
            }

            psSkillCacheItem *item = skills->getItemBySkillId(skillID);

            if (info->id == focus)
            {
                selectedSkillNameId = (int)skillNameId;
                selectedSkillCat=info->category;
            }

            unsigned int actualStat;

            if (info->category != 0)
                actualStat = character->Skills().GetSkillRank((PSSKILL) skillID).Current();
            else
            {
                if(info->name=="Strength")
                {
                    actualStat = character->Stats()[PSITEMSTATS_STAT_STRENGTH].Current();
                }
                else if(info->name== "Endurance")
                {
                    actualStat = character->Stats()[PSITEMSTATS_STAT_ENDURANCE].Current();
                }
                else if(info->name== "Agility")
                {
                    actualStat = character->Stats()[PSITEMSTATS_STAT_AGILITY].Current();
                }
                else if(info->name== "Intelligence")
                {
                    actualStat = character->Stats()[PSITEMSTATS_STAT_INTELLIGENCE].Current();
                }
                else if(info->name== "Will")
                {
                    actualStat = character->Stats()[PSITEMSTATS_STAT_WILL].Current();
                }
                else
                {
                    actualStat = character->Stats()[PSITEMSTATS_STAT_CHARISMA].Current();
                }

            }

            if (item)
            {
                item->update(charSkill.rank.Base(), actualStat,
                             charSkill.y, charSkill.yCost,
                             charSkill.z, charSkill.zCost);
            } else
            {
                item = new psSkillCacheItem(skillID, skillNameId,
                                            charSkill.rank.Base(), actualStat,
                                            charSkill.y, charSkill.yCost,
                                            charSkill.z, charSkill.zCost,
                                            info->category, stat);
                skills->addItem(skillID, item);
            }
        } else if (trainerInfo)
        {
            // We are training, but this skill is not available for training
            psSkillCacheItem *item = skills->getItemBySkillId(skillID);
            if (item)
                item->setRemoved(true);
        }
    }

    bool training= false;
    if (isTraining)
        training= true;

    psGUISkillMessage newmsg(client->GetClientNum(),
                            psGUISkillMessage::SKILL_LIST,
                            "",
                            skills,
                            (unsigned int)character->Stats()[PSITEMSTATS_STAT_STRENGTH].Current(),
                            (unsigned int)character->Stats()[PSITEMSTATS_STAT_ENDURANCE].Current(),
                            (unsigned int)character->Stats()[PSITEMSTATS_STAT_AGILITY].Current(),
                            (unsigned int)character->Stats()[PSITEMSTATS_STAT_INTELLIGENCE].Current(),
                            (unsigned int)character->Stats()[PSITEMSTATS_STAT_WILL].Current(),
                            (unsigned int)character->Stats()[PSITEMSTATS_STAT_CHARISMA].Current(),
                            (unsigned int)character->GetHP(),
                            (unsigned int)character->GetMana(),
                            (unsigned int)character->GetStamina(true),
                            (unsigned int)character->GetStamina(false),
                            (unsigned int)character->GetMaxHP().Current(),
                            (unsigned int)character->GetMaxMana().Current(),
                            (unsigned int)character->GetMaxPStamina().Current(),
                            (unsigned int)character->GetMaxMStamina().Current(),
                            forceOpen,
                            selectedSkillNameId,
                            selectedSkillCat,
                            training //If we are training the client must know it
                            );

    Debug2(LOG_SKILLXP, client->GetClientNum(),"Sending psGUISkillMessage w/ stats to %d, Valid: ",int(client->GetClientNum()));
    if (newmsg.valid)
    {
        Debug1(LOG_SKILLXP, client->GetClientNum(),"Yes\n");
        SendMessage(newmsg.msg);

    }
    else
    {
        Debug1(LOG_SKILLXP, client->GetClientNum(),"No\n");
        Bug2("Could not create valid psGUISkillMessage for client %u.\n",client->GetClientNum());
    }
}

void ProgressionManager::StartTraining(Client * client, psCharacter * trainer)
{
    client->GetCharacterData()->SetTrainer(trainer);
    SendSkillList(client, true, PSSKILL_NONE, true);
}

ProgressionScript *ProgressionManager::FindScript(char const *name)
{
    return CacheManager::GetSingleton().GetProgressionScript(name);
}

void ProgressionManager::QueueEvent(psGameEvent *event)
{
    psserver->GetEventManager()->Push(event);
}

void ProgressionManager::SendMessage(MsgEntry *me)
{
    psserver->GetEventManager()->SendMessage(me);
}

void ProgressionManager::Broadcast(MsgEntry *me)
{
    psserver->GetEventManager()->Broadcast(me, NetBase::BC_EVERYONE);
}

