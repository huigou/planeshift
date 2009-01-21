/*
* usermanager.cpp - Author: Keith Fulton
*
* Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <ctype.h>
#include <string.h>
#include <memory.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iengine/engine.h>
#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/serverconsole.h"
#include "util/pserror.h"
#include "util/psconst.h"
#include "util/strutil.h"
#include "util/log.h"
#include "util/eventmanager.h"

#include "engine/psworld.h"

#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psactionlocationinfo.h"

#include "rpgrules/factions.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "usermanager.h"
#include "client.h"
#include "clients.h"
#include "events.h"
#include "gem.h"
#include "netmanager.h"
#include "entitymanager.h"
#include "marriagemanager.h"
#include "combatmanager.h"
#include "invitemanager.h"
#include "adminmanager.h"
#include "commandmanager.h"
#include "psserver.h"
#include "psserverchar.h"
#include "cachemanager.h"
#include "playergroup.h"
#include "progressionmanager.h"
#include "netmanager.h"
#include "advicemanager.h"
#include "actionmanager.h"
#include "chatmanager.h"
#include "gmeventmanager.h"
#include "bankmanager.h"
#include "globals.h"

#define RANGE_TO_CHALLENGE 50

//TODO: Implement or reuse adminmanager standard targetting also here

class psUserStatRegeneration : public psGameEvent
{
protected:
    UserManager * usermanager;

public:
    psUserStatRegeneration(UserManager *mgr,csTicks ticks);

    virtual void Trigger();  // Abstract event processing function
};

/** A structure to hold the clients that are pending on duel challenges.
*/
class PendingDuelInvite : public PendingInvite
{
public:

    PendingDuelInvite(Client *inviter,
        Client *invitee,
        const char *question)
        : PendingInvite( inviter, invitee, true,
        question,"Accept","Decline",
        "You have challenged %s to a duel.",
        "%s has challenged you to a duel.",
        "%s has accepted your challenge.",
        "You have accepted %s's challenge.",
        "%s has declined your challenge.",
        "You have declined %s's challenge.", psQuestionMessage::duelConfirm)
    {
    }

    virtual ~PendingDuelInvite() {}

    void HandleAnswer(const csString & answer);
};

/***********************************************************************/

UserManager::UserManager(ClientConnectionSet *cs)
{
    clients       = cs;

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<UserManager>(this,&UserManager::HandleUserCommand),MSGTYPE_USERCMD,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<UserManager>(this,&UserManager::HandleMOTDRequest),MSGTYPE_MOTDREQUEST,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<UserManager>(this,&UserManager::HandleCharDetailsRequest),MSGTYPE_CHARDETAILSREQUEST,REQUIRE_READY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<UserManager>(this,&UserManager::HandleCharDescUpdate),MSGTYPE_CHARDESCUPDATE,REQUIRE_READY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<UserManager>(this,&UserManager::HandleTargetEvent),MSGTYPE_TARGET_EVENT,NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<UserManager>(this,&UserManager::HandleEntranceMessage),MSGTYPE_ENTRANCE,REQUIRE_READY_CLIENT);
}

UserManager::~UserManager()
{
    if (psserver->GetEventManager())
    {
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_USERCMD);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_MOTDREQUEST);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_CHARDETAILSREQUEST);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_CHARDESCUPDATE);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_TARGET_EVENT);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_ENTRANCE);
    }
}

void UserManager::HandleMOTDRequest(MsgEntry *me,Client *client)
{
    //Sends MOTD and tip

    unsigned int guildID =0;
    //If data isn't loaded, load from db
    if (!client->GetCharacterData())
    {
        // FIXME: We really shouldn't be loading this from the DB.
        Result result(db->Select("SELECT guild_member_of FROM characters WHERE id = '%u'", client->GetPID().Unbox()));
        if (result.Count() > 0)
            guildID = result[0].GetUInt32(0);
    }
    else
    {
        psGuildInfo* playerGuild = client->GetCharacterData()->GetGuild();

        if (playerGuild)
            guildID = playerGuild->id;
        else
            guildID =0;
    }

    csString tip;
    if (CacheManager::GetSingleton().GetTipLength() > 0)
        CacheManager::GetSingleton().GetTipByID(psserver->GetRandom(CacheManager::GetSingleton().GetTipLength()), tip );

    csString motdMsg(psserver->GetMOTD());

    csString guildMotd("");
    csString guildName("");
    psGuildInfo * guild = CacheManager::GetSingleton().FindGuild(guildID);

    if (guild)
    {
        guildMotd=guild->GetMOTD();
        guildName=guild->GetName();
    }

    psMOTDMessage motd(me->clientnum,tip,motdMsg,guildMotd,guildName);
    motd.SendMessage();
    return;
}

void UserManager::HandleUserCommand(MsgEntry *me,Client *client)
{
    if (client->IsFrozen()) //disable most commands
        return;

    psUserCmdMessage msg(me);

    Debug3(LOG_USER, client->GetClientNum(),"Received user command: %s from %s\n",
        me->bytes->payload, (const char *)client->GetName());

    // We don't check for validity for emotes, they always are.
    if (!msg.valid && !CheckForEmote(msg.command, false, me, client))
    {
        psserver->SendSystemError(me->clientnum,"Command not supported by server yet.");
        return;
    }

    if (msg.command == "/who")
    {
        Who(msg,client,me->clientnum);
    }
    else if (msg.command == "/buddy")
    {
        Buddy(msg,client,me->clientnum);
    }
    else if (msg.command == "/roll")
    {
        RollDice(msg,client,me->clientnum);
    }
    else if (msg.command == "/pos")
    {
        ReportPosition(msg,client,me->clientnum);
    }
    /*else if (msg.command == "/spawn")
    {
    MoveToSpawnPos(msg,client,me->clientnum);*
    }*/
    else if (msg.command == "/unstick")
    {
        HandleUnstick(msg,client,me->clientnum);
    }
    else if (msg.command == "/attack")
    {
        HandleAttack(msg,client,me->clientnum);
    }
    else if (msg.command == "/stopattack")
    {
        psserver->combatmanager->StopAttack(client->GetActor());
    }
    else if ( msg.command == "/admin" )
    {
        psserver->GetAdminManager()->Admin(me->clientnum, client);
    }
    else if ( msg.command == "/loot" )
    {
        HandleLoot(client);
    }
    else if ( msg.command == "/quests" )
    {
        HandleQuests(client);
        HandleGMEvents(client);
    }
    else if ( msg.command == "/train" )
    {
        HandleTraining(client);
    }
    else if ( msg.command == "/sit" )
    {
        if (client->GetActor()->GetMode() == PSCHARACTER_MODE_PEACE && client->GetActor()->AtRest() && !client->GetActor()->IsFalling())
        {
            client->GetActor()->SetMode(PSCHARACTER_MODE_SIT);
            Emote("%s takes a seat.", "%s takes a seat by %s.", "sit", me, client);
        }
    }
    else if ( msg.command == "/stand" )
    {
        if (client->GetActor()->GetMode() == PSCHARACTER_MODE_SIT)
        {
            client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
            psUserActionMessage anim(me->clientnum, client->GetActor()->GetEID(), "stand up");
            anim.Multicast( client->GetActor()->GetMulticastClients(),0,PROX_LIST_ANY_RANGE );
            Emote("%s stands up.", "%s stands up.", "stand", me, client);
        }
        else if (client->GetActor()->GetMode() == PSCHARACTER_MODE_OVERWEIGHT)
        {
            psserver->SendSystemError(client->GetClientNum(), "You can't stand up because you're overloaded!");
        }
    }
    else if (msg.command == "/starttrading")
    {
        client->GetCharacterData()->SetTradingStopped(false);
        psserver->SendSystemInfo(me->clientnum,"You can trade now.");
    }
    else if (msg.command == "/stoptrading")
    {
        client->GetCharacterData()->SetTradingStopped(true);
        psserver->SendSystemInfo(me->clientnum,"You are busy and can't trade.");
    }
    else if ( msg.command == "/assist" )
    {
        Assist( msg, client, me->clientnum );
    }
    else if (msg.command == "/tip")
    {
        GiveTip(me->clientnum);
    }
    else if (msg.command == "/motd")
    {
        GiveMOTD(me->clientnum);
    }
    else if (msg.command == "/challenge")
    {
        ChallengeToDuel(msg,client);
    }
    else if (msg.command == "/yield")
    {
        YieldDuel(client);
    }
    else if (msg.command == "/npcmenu")
    {
        ShowNpcMenu(client);
    }
    else if ( msg.command == "/die" )
    {
        gemActor* actor = client->GetActor();
        if (!actor)
            return;
        actor->Kill(actor);
    }
    else if (msg.command == "/marriage")
    {
        if (msg.action == "propose")
        {
            if ( msg.player.IsEmpty() || msg.text.IsEmpty() )
            {
                psserver->SendSystemError( client->GetClientNum(), "Usage: /marriage propose [first name] [message]" );
                return;
            }

            // Send propose message
            psserver->GetMarriageManager()->Propose(client,  msg.player,  msg.text);
        }
        else if (msg.action == "divorce")
        {
            if ( msg.text.IsEmpty() )
            {
                psserver->SendSystemError( client->GetClientNum(), "Usage: /marriage divorce [message]" );
                return;
            }

            // Send divorce prompt
            psserver->GetMarriageManager()->ContemplateDivorce(client,  msg.text);
        }
        else
        {
            psserver->SendSystemError( client->GetClientNum(), "Usage: /marriage [propose|divorce]" );
        }
    }
    else if(CheckForEmote(msg.command, true, me, client))
    {
        return;
    }
    else if (msg.command == "/bank")
    {
        HandleBanking(client, msg.action);
    }
    else if (msg.command == "/pickup")
    {
        HandlePickup(client, msg.target);
    }
    else if (msg.command == "/guard")
    {
        HandleGuard(client, msg.target, msg.action);
    }
    else
    {
        psserver->SendSystemError(me->clientnum,"Command not supported by server yet.");
    }
}

csArray<csString> UserManager::DecodeCommandArea(Client *client, csString target)
{
    csArray<csString> result;

    if (!CacheManager::GetSingleton().GetCommandManager()->Validate(client->GetSecurityLevel(), "command area"))
    {
        psserver->SendSystemError(client->GetClientNum(),
                "You are not allowed to use area", target.GetData());
        return result;
    }

    csArray<csString> splitTarget = psSplit(target, ':');
    size_t splitSize = splitTarget.GetSize();

    if (splitSize < 3 || splitSize > 4)
    {
        psserver->SendSystemError(client->GetClientNum(),
                "Try /$CMD area:item:range[:name]");
        return result;
    }

    csString itemName = splitTarget[1];
    csString* nameFilter = splitSize > 3 ? &splitTarget[3] : 0;

    const int range = atoi(splitTarget[2].GetData());
    if (range <= 0)
    {
        psserver->SendSystemError(client->GetClientNum(),
                "You must specify a positive integer for the area: range.");
        return result;
    }

    bool allNames = true;
    if (nameFilter && (*nameFilter!="all"))
        allNames = false;

    int mode;
    if (itemName == "players")
        mode = 0;
    else if (itemName == "actors")
        mode = 1;
    else if (itemName == "items")
        mode = 2;
    else if (itemName == "npcs")
        mode = 3;
    else if (itemName == "entities")
        mode = 4;
    else
    {
        psserver->SendSystemError(client->GetClientNum(),
                "item must be players|actors|items|npcs|entities");
        return result;
    }

    gemActor* self = client->GetActor();
    if (!self)
    {
        psserver->SendSystemError(client->GetClientNum(), "You do not exist...");
        return result;
    }

    csVector3 pos;
    iSector* sector;
    self->GetPosition(pos, sector);

    GEMSupervisor* gem = GEMSupervisor::GetSingletonPtr();
    if (!gem)
        return result;

    csArray<gemObject*> nearlist = gem->FindNearbyEntities(sector, pos,
            range);
    size_t count = nearlist.GetSize();
    csArray<csString *> results;

    for (size_t i=0; i<count; i++)
    {
        gemObject *nearobj = nearlist[i];
        if (!nearobj)
            continue;

        if (nearobj->GetInstance() != self->GetInstance())
            continue;

        if (!allNames)
        {
            csString nearobjName = nearobj->GetName();
            if (!nearobjName.StartsWith(*nameFilter->GetData(), true))
                continue;
        }

        csString newTarget;

        switch (mode)
        {
        case 0: // Target players
        {
            if (nearobj->GetClientID())
            {
                newTarget.Format("pid:%d", nearobj->GetPID().Unbox());
                break;
            }
            else
                continue;
        }
        case 1: // Target actors
        {
            if (nearobj->GetPID().IsValid())
            {
                newTarget.Format("pid:%d", nearobj->GetPID().Unbox());
                break;
            }
            else
                continue;
        }
        case 2: // Target items
        {
            if (nearobj->GetItem())
            {
                newTarget.Format("eid:%u", nearobj->GetEID().Unbox());
                break;
            }
            else
                continue;
        }
        case 3: // Target NPCs
        {
            if (nearobj->GetNPCPtr())
            {
                newTarget.Format("pid:%u", nearobj->GetPID().Unbox());
                break;
            }
            else
                continue;
        }
        case 4: // Target everything
        {
            newTarget.Format("eid:%u", nearobj->GetEID().Unbox());
            break;
        }
        }

        // Run this once for every target in range  (each one will be verified and logged seperately)
        result.Push(newTarget);
    }
    if (result.IsEmpty())
    {
        psserver->SendSystemError(client->GetClientNum(),
                "Nothing of specified type in range.");
    }
    return result;
}

bool UserManager::CheckForEmote(csString command, bool execute, MsgEntry *me, Client *client)
{
    for(unsigned int i=0;  i < emoteList.GetSize(); i++)
    {
        if( command == emoteList[i].command )
        {
            if(execute)
            {
                Emote(emoteList[i].general, emoteList[i].specific, emoteList[i].anim, me, client);
            }
            return true;
        }
    }
    return false;
}

void UserManager::Emote(csString general, csString specific, csString animation, MsgEntry *me, Client *client)
{
    if (client->GetTargetObject() && (client->GetTargetObject() != client->GetActor()))
    {
        psSystemMessage newmsg(me->clientnum, MSG_INFO_BASE, specific, client->GetActor()->GetName(), client->GetTargetObject()->GetName());
        newmsg.Multicast(client->GetActor()->GetMulticastClients(), 0, CHAT_SAY_RANGE);
    }
    else
    {
        psSystemMessage newmsg(me->clientnum, MSG_INFO_BASE, general, client->GetActor()->GetName());
        newmsg.Multicast(client->GetActor()->GetMulticastClients(), 0, CHAT_SAY_RANGE);
    }

    if (animation != "noanim")
    {
        psUserActionMessage anim(me->clientnum, client->GetActor()->GetEID(), animation);
        anim.Multicast(client->GetActor()->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
    }
}

bool UserManager::LoadEmotes(const char *xmlfile, iVFS *vfs)
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);

    csRef<iDataBuffer> buff = vfs->ReadFile( xmlfile );

    if ( !buff || !buff->GetSize() )
    {
        return false;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( buff );
    if ( error )
    {
        Error3("%s in %s", error, xmlfile);
        return false;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", xmlfile);
        return false;
    }

    csRef<iDocumentNode> topNode = root->GetNode("emotes");
    if(!topNode)
    {
        Error2("No <emotes> tag in %s", xmlfile);
        return false;
    }
    csRef<iDocumentNodeIterator> emoteIter = topNode->GetNodes("emote");

    while(emoteIter->HasNext())
    {
        csRef<iDocumentNode> emoteNode = emoteIter->Next();

        EMOTE emote;
        emote.command = emoteNode->GetAttributeValue("command");
        emote.general = emoteNode->GetAttributeValue("general");
        emote.specific = emoteNode->GetAttributeValue("specific");
        emote.anim = emoteNode->GetAttributeValue("anim");

        emoteList.Push(emote);
    }

    return true;
}

void UserManager::HandleCharDetailsRequest(MsgEntry *me,Client *client)
{
    psCharacterDetailsRequestMessage msg(me);

    gemActor *myactor;
    if (!msg.isMe)
    {
        gemObject *target = client->GetTargetObject();
        if (!target)    return;

        myactor = target->GetActorPtr();
        if (!myactor) return;
    }
    else
    {
        myactor = client->GetActor();
        if (!myactor) return;
    }


    psCharacter* charData = myactor->GetCharacterData();
    if (!charData) return;


    SendCharacterDescription(client, charData, false, msg.isSimple, msg.requestor);
}

csString intToStr(int f)
{
    csString s;
    s.Format("%i", f);
    return s;
}

csString fmtStatLine(const char *const label, unsigned int value, unsigned int buffed)
{
    csString s;
    if (value != buffed)
        s.Format("%s: %u (%u)\n", label, value, buffed);
    else
        s.Format("%s: %u\n", label, value);
    return s;
}

const enum PSITEMSTATS_STAT skilltostat[6] = {PSITEMSTATS_STAT_AGILITY,
    PSITEMSTATS_STAT_CHARISMA,
    PSITEMSTATS_STAT_ENDURANCE,
    PSITEMSTATS_STAT_INTELLIGENCE,
    PSITEMSTATS_STAT_STRENGTH,
    PSITEMSTATS_STAT_WILL};

void UserManager::SendCharacterDescription(Client * client, psCharacter * charData, bool full, bool simple, const csString & requestor)
{
    StatSet & playerAttr = client->GetCharacterData()->Stats();
    csString meshName = charData->GetActor()->GetMesh();

    bool isSelf = (charData->GetPID() == client->GetCharacterData()->GetPID());

    csString charName = charData->GetCharFullName();
    csString raceName;
    PSCHARACTER_GENDER gender;
    psRaceInfo* charrinfo = CacheManager::GetSingleton().GetRaceInfoByMeshName(meshName);
    if (charrinfo != NULL)
    {
        raceName = charrinfo->GetRace();
        gender = charrinfo->gender;
    }
    else
    {
        raceName = "being";
        gender = PSCHARACTER_GENDER_NONE;
    }
    csString desc          = charData->GetDescription();

    csArray<psCharacterDetailsMessage::NetworkDetailSkill> skills;

    if ( !simple && CacheManager::GetSingleton().GetCommandManager()->Validate(client->GetSecurityLevel(), "view stats") )
        full = true;  // GMs can view the stats list

    //send creation info only if the player is requesting  his info
    csString creationinfo = ( isSelf || full )? charData->GetCreationInfo() : "";
    csString desc_ooc     = charData->GetOOCDescription();

    if (full)
    {
        desc += "\n\n";
        desc += "HP: "+intToStr(int(charData->GetHP()))+" Max HP: "+intToStr(int(charData->GetHitPointsMax()))+"\n";
        SkillSet & sks = charData->Skills();
        StatSet & sts = charData->Stats();

        for (int skill = 0; skill < PSSKILL_COUNT; skill++)
        {
            psSkillInfo *skinfo;
            skinfo = CacheManager::GetSingleton().GetSkillByID((PSSKILL)skill);
            if (skinfo != NULL)
            {
                psCharacterDetailsMessage::NetworkDetailSkill s;

                s.category = skinfo->category;
                if (PSSKILL_AGI <= skill && skill <= PSSKILL_WILL)
                {
                    // handle stats specially in order to pick up buffs/debuffs
                    s.text = fmtStatLine(skinfo->name,
                                         sts.GetStat(skilltostat[skill - PSSKILL_AGI], false),
                                         sts.GetStat(skilltostat[skill - PSSKILL_AGI], true));
                }
                else
                {
                    s.text = fmtStatLine(skinfo->name,
                                         sks.GetSkillRank(static_cast<PSSKILL>(skill), false),
                                         sks.GetSkillRank(static_cast<PSSKILL>(skill), true));
                }
                skills.Push(s);
            }
        }

        csHash<FactionStanding*, int>::GlobalIterator iter(
            charData->GetActor()->GetFactions()->GetStandings().GetIterator());
        while(iter.HasNext())
        {
            FactionStanding* standing = iter.Next();
            psCharacterDetailsMessage::NetworkDetailSkill s;

            s.category = 5; // faction
            s.text.Format("%s: %d\n", standing->faction->name.GetDataSafe(),
                standing->score);
            skills.Push(s);
        }
    }

    // No fancy things if simple description is requested
    if ( !simple )
    {
        // Don't guess strength if we can't attack the character or if he's
        //  dead or if we are viewing our own description
        if ( !charData->impervious_to_attack && (charData->GetMode() != PSCHARACTER_MODE_DEAD) && !isSelf )
        {
            if (playerAttr.GetStat(PSITEMSTATS_STAT_INTELLIGENCE) < 50)
                desc.AppendFmt( "\n\nYou try to evaluate the strength of %s, but you have no clue.", charName.GetData() );
            else
            {
                // Character's Strength assessment code below.
                static const char* const StrengthGuessPhrases[] =
                { "won't require any effort to defeat",
                "is noticeably weaker than you",
                "won't pose much of a challenge",
                "is not quite as strong as you",
                "is about as strong as you",
                "is somewhat stronger than you",
                "will pose a challenge to defeat",
                "is significantly more powerful than you",
                "may be impossible to defeat" };

                bool smart = (playerAttr.GetStat(PSITEMSTATS_STAT_INTELLIGENCE) >= 100);

                int CharsLvl      = charData->GetCharLevel();
                int PlayersLvl    = client->GetCharacterData()->GetCharLevel();
                int LvlDifference = PlayersLvl - CharsLvl;
                int Phrase        = 0;

                if ( LvlDifference >= 50 )
                    Phrase = 0;
                else if ( LvlDifference > 30 )
                    Phrase = 1;
                else if ( LvlDifference > 15 && smart)
                    Phrase = 2;
                else if ( LvlDifference > 5 )
                    Phrase = 3;
                else if ( LvlDifference >= -5 )
                    Phrase = 4;
                else if ( LvlDifference >= -15 )
                    Phrase = 5;
                else if ( LvlDifference >= -30 && smart)
                    Phrase = 6;
                else if ( LvlDifference > -50 )
                    Phrase = 7;
                else
                    Phrase = 8;

                // Enable for Debugging only
                // desc+="\n CharsLvl: "; desc+=CharsLvl; desc+=" | YourLvl: "; desc+=PlayersLvl; desc+="\n";

                desc.AppendFmt( "\n\nYou evaluate that %s %s.", charName.GetData(), StrengthGuessPhrases[Phrase] );
            }
        }

        // Show spouse name if character is married
        if ( charData->GetIsMarried() && !isSelf )
            desc.AppendFmt( "\n\nMarried to: %s", charData->GetSpouseName() );


        // Show owner name if character is a pet
        if (charData->IsPet() && charData->GetOwnerID().IsValid())
        {
            gemActor *owner = GEMSupervisor::GetSingleton().FindPlayerEntity(charData->GetOwnerID());
            if (owner)
                desc.AppendFmt( "\n\nA pet owned by: %s", owner->GetName() );
        }
    }

    if (!(charData->IsNPC() || charData->IsPet()) && client->GetSecurityLevel() < GM_LEVEL_0 &&
        !client->GetCharacterData()->Knows(charData) && !isSelf)
    {
        charName = "[Unknown]";
    }

    // Finally send the details message
    psCharacterDetailsMessage detailmsg(client->GetClientNum(), charName,
        (short unsigned int)gender, raceName,
        desc, skills, desc_ooc, creationinfo, requestor );
    detailmsg.SendMessage();
}

void UserManager::HandleCharDescUpdate(MsgEntry *me,Client *client)
{
    psCharacterDescriptionUpdateMessage descUpdate(me);
    psCharacter* charData = client->GetCharacterData();
    if (!charData)
        return;

    if(descUpdate.oocdesc)
        charData->SetOOCDescription(descUpdate.newValue);
    else
        charData->SetDescription(descUpdate.newValue);

    Debug3(LOG_USER, client->GetClientNum(), "Character description updated for %s (%s)\n", charData->GetCharFullName(), ShowID(client->GetAccountID()));
}

void UserManager::HandleTargetEvent(MsgEntry *me, Client *notused)
{
    psTargetChangeEvent targetevent(me);

    Client *targeter = NULL;
    Client *targeted = NULL;
    if (targetevent.character)
    {
        Debug2(LOG_USER, targetevent.character->GetClientID(),"UserManager handling target event for %s\n", targetevent.character->GetName());
        targeter = clients->Find( targetevent.character->GetClientID() );
        if (!targeter)
            return;
    }
    if (targetevent.target)
        targeted = clients->Find( targetevent.target->GetClientID() );

    psserver->combatmanager->StopAttack(targeter->GetActor());

    if(!targeted
        && dynamic_cast<gemNPC*>(targetevent.target)
        && targetevent.character->GetMode() == PSCHARACTER_MODE_COMBAT) // NPC?
    {
        if (targeter->IsAllowedToAttack(targetevent.target))
            SwitchAttackTarget( targeter, targeted );

        return;
    }
    else if (!targeted)
        return;

    if (targeted->IsReady() && targeter->IsReady() )
    {
        if (targetevent.character->GetMode() == PSCHARACTER_MODE_COMBAT)
            SwitchAttackTarget( targeter, targeted );
    }
    else
    {
        psserver->SendSystemError(targeter->GetClientNum(),"Target is not ready yet");
    }
}

void UserManager::HandleEntranceMessage( MsgEntry* me, Client *client )
{
    bool secure = (client->GetSecurityLevel() > GM_LEVEL_0 ) ? true : false;
    psEntranceMessage mesg(me);
    psActionLocation *action = psserver->GetActionManager()->FindAction( mesg.entranceID );
    if ( !action )
    {
        if (secure)
            psserver->SendSystemInfo(client->GetClientNum(), "No item/action: %s", ShowID(mesg.entranceID));
        Error2("No item/action: %s", ShowID(mesg.entranceID));
        return;
    }

    // Check range
    csWeakRef<gemObject> gem = client->GetActor();
    csWeakRef<gemObject> gemAction = action->GetGemObject();
    if (gem.IsValid() && gemAction.IsValid() && gem->RangeTo(gemAction, false) > RANGE_TO_SELECT)
    {
        psserver->SendSystemError(client->GetClientNum(), "You are no longer in range to do this.");
        return;
    }

    // Check for entrance
    if ( !action->IsEntrance() )
    {
        if (secure) psserver->SendSystemInfo(client->GetClientNum(),"No <Entrance> tag in action response");
        Error1("No <Entrance> tag in action response");
        return;
    }

    // Check for a lock
    uint32 instanceID = action->GetInstanceID();
    if ( instanceID  != 0 )
    {

        // find lock to to test if locked
        gemItem* realItem = GEMSupervisor::GetSingleton().FindItemEntity( instanceID );
        if (!realItem)
        {
            if (secure) psserver->SendSystemInfo(client->GetClientNum(),"Invalid instance ID %u in action location %s", instanceID, action->name.GetDataSafe());
            Error3("Invalid instance ID %u in action location %s", instanceID, action->name.GetDataSafe());
            return;
        }

        // get real item
        psItem* item = realItem->GetItem();
        if ( !item )
        {
            if (secure) psserver->SendSystemInfo(client->GetClientNum(),"Invalid ItemID in Action Location Response.\n");
            Error1("Invalid ItemID in Action Location Response.\n");
            return;
        }

        // Check if locked
        if(item->GetIsLocked())
        {
            if (!client->GetCharacterData()->Inventory().HaveKeyForLock(item->GetUID()))
            {
                psserver->SendSystemInfo(client->GetClientNum(),"Entrance is locked.");
                return;
            }
        }
    }

    // Get entrance attributes
    csVector3 pos = action->GetEntrancePosition();
    float rot = action->GetEntranceRotation();
    csString sectorName = action->GetEntranceSector();

    // Check for different entrance types
    csString entranceType = action->GetEntranceType();

    // Send player to main game instance
    if (entranceType == "Prime")
    {
        if (secure) psserver->SendSystemInfo(client->GetClientNum(),"Teleporting to sector %s", sectorName.GetData());
        Teleport( client, pos.x, pos.y, pos.z, 0, rot, sectorName );
    }

    // Send player to unique instance
    else if (entranceType == "ActionID")
    {
        InstanceID instance = action->id;
        if (secure) psserver->SendSystemInfo(client->GetClientNum(),"Teleporting to sector %s", sectorName.GetData());
        Teleport( client, pos.x, pos.y, pos.z, instance, rot, sectorName );
    }

    // Send player back to starting point
    else if (entranceType == "ExitActionID")
    {
        // Use current player instance to get entrance action location
        psActionLocation *retAction = psserver->GetActionManager()->FindActionByID( client->GetActor()->GetInstance() );
        if ( !retAction )
        {
            if (secure) psserver->SendSystemInfo(client->GetClientNum(),"No exit item/action : %i", client->GetActor()->GetInstance() );
            Error2( "No exit item/action : %i", client->GetActor()->GetInstance() );
            return;
        }

        // Check for return entrance
        if ( !retAction->IsReturn() )
        {
            if (secure) psserver->SendSystemInfo(client->GetClientNum(),"No <Return tag in action response %s",retAction->response.GetData());
            Error2("No <Return tag in action response %s",retAction->response.GetData());
            return;
        }

        // Process return attributes and spin us around 180 deg for exit
        csVector3 retPos = retAction->GetReturnPosition();
        float retRot = retAction->GetReturnRotation() + (3.141592654/2);
        csString retSectorName = retAction->GetReturnSector();

        // Send player back to return point
        if (secure) psserver->SendSystemInfo(client->GetClientNum(),"Teleporting to sector %s", sectorName.GetData());
        Teleport( client, retPos.x, retPos.y, retPos.z, 0, retRot, retSectorName );
    }
    else
    {
        if (secure) psserver->SendSystemInfo(client->GetClientNum(),"Unknown type in entrance action location");
        Error1("Unknown type in entrance action location");
    }
}


void UserManager::Who(psUserCmdMessage& msg, Client* client, int clientnum)
{
    csString message((size_t) 1024);
    csString headerMsg("Players Currently Online");

    if (!msg.filter.IsEmpty())
    {
        size_t pos = msg.filter.FindFirst('%');
        while (pos != (size_t) -1)
        {
            msg.filter.DeleteAt(pos);
            pos = msg.filter.FindFirst('%');
        }

        StrToLowerCase(msg.filter);

        headerMsg.Append(" (Applying Filter: '*");
        headerMsg.Append(msg.filter);
        headerMsg.Append("*')");
    }

    message.Append(headerMsg);
    headerMsg.Append("\n-------------------------------------");

    unsigned count = 0;

    if (msg.filter.StartsWith("area:"))
    {
        csArray<csString> filters = DecodeCommandArea(client, msg.filter);
        csArray<csString>::Iterator it(filters.GetIterator());
        while (it.HasNext())
        {
            csString pid = it.Next();
            gemObject *object = psserver->GetAdminManager()->FindObjectByString(pid,client->GetActor());
            Client* thisclient = object->GetClient();
            if (thisclient)
            {
                if (!WhoProcessClient(thisclient, client->GetGuildID(), &message, msg.filter,
                        false, &count))
                    break;
            }
        }
    }
    else
    {
        // Guild rank, guild and title should come from acraig's player prop class.
        ClientIterator i(*clients);
        while (i.HasNext())
        {
            Client *curr = i.Next();
            if (!WhoProcessClient(curr, client->GetGuildID(), &message, msg.filter, true,
                    &count))
                break;
        }
    }

    // Could be about to overflow by now, so check.
    csString temp;
    temp.Format("%u shown from %zu players online\n", count, clients->Count());

    message.Append('\n');
    message.Append(temp);

    psSystemMessageSafe newmsg(clientnum, MSG_WHO, message);
    newmsg.SendMessage();
}

bool UserManager::WhoProcessClient(Client *curr, int guildId, csString* message, csString filter, bool check, unsigned * count)
{
    if (curr->IsSuperClient() || !curr->GetActor())
        return true;

    csString temp((size_t) 1024);
    csString playerName(curr->GetName());
    csString guildTitle;
    csString guildName;
    csString format("%s"); // Player name.

    psGuildInfo* guild = curr->GetActor()->GetGuild();
    if (guild != NULL)
    {
        if (guild->id && (!guild->IsSecret() || guild->id
                == guildId))
        {
            psGuildLevel* level = curr->GetActor()->GetGuildLevel();
            if (level)
            {
                format.Append(", %s in %s"); // Guild level title.
                guildTitle = level->title;
            }
            else
            {
                format.Append(", %s");
            }
            guildName = guild->name;
        }
    }
    temp.Format(format.GetData(), curr->GetName(), guildTitle.GetData(),
            guildName.GetData());

    if (check)
    {
        csString lower(temp);
        StrToLowerCase(lower);
        if (!filter.IsEmpty() && lower.FindStr(filter) == (size_t) -1)
            return true;
    }

    /* if message + new text + 50 spare bytes for the footer exceed the maximum package size do not append */
    if (temp.Length() + message->Length() + 50 > MAXPACKETSIZE)
        return false;
    else
        message->Append('\n');
    message->Append(temp);

    (*count)++;
    return true;
}

void UserManager::StrToLowerCase(csString& str)
{
    for (register size_t i = 0; i < str.Length(); ++i)
        str[i] = tolower(str[i]);
}

void UserManager::Buddy(psUserCmdMessage& msg,Client *client,int clientnum)
{
    bool onoff = false;  ///< Holds if the player is forcing add (true) or remove
    bool toggle = false; ///< Holds if the player didn't force add or remove and so we are going to toggle

    psCharacter *chardata=client->GetCharacterData();

    if (chardata==NULL)
    {
        Error3("Client for account '%s' attempted to manage buddy '%s' but has no character data!",client->GetName(),msg.player.GetData());
        return;
    }

    if (msg.player.Length() == 0) //if no imput was provided other than the command send the buddylist
    {
        BuddyList(client,clientnum, UserManager::ALL_PLAYERS);
        return;
    }

    msg.player = NormalizeCharacterName(msg.player);

    PID selfid = chardata->GetPID();

    if (msg.action == "add")         //The player provided an add so add the buddy
        onoff = true;
    else if (msg.action == "remove") //The player provided a remove so remove the buddy
        onoff = false;
    else                              //The player didn't provide either so toggle the buddy
        toggle = true;

    bool excludeNPCs = true;
    PID buddyid = psServer::CharacterLoader.FindCharacterID(msg.player.GetData(), excludeNPCs);

    if (!buddyid.IsValid()) //Check if the buddy was found
    {
        psserver->SendSystemError(clientnum,"Could not add buddy: Character '%s' not found.", msg.player.GetData());
        return;
    }

    //If the player used add or didn't provide arguments and the buddy is missing from the list add it
    if((onoff && !toggle)|| (toggle && !chardata->IsBuddy(buddyid)))
    {
        if ( !chardata->AddBuddy( buddyid, msg.player ) )
        {
            psserver->SendSystemError(clientnum,"%s could not be added to buddy list.",(const char *)msg.player);
            return;
        }

        Client* buddyClient = clients->FindPlayer( buddyid );
        if ( buddyClient && buddyClient->IsReady() )
        {
            buddyClient->GetCharacterData()->BuddyOf( selfid );
        }

        if (!psserver->AddBuddy(selfid,buddyid))
        {
            psserver->SendSystemError(clientnum,"%s is already on your buddy list.",(const char *)msg.player);
            return;
        }

        psserver->SendSystemInfo(clientnum,"%s has been added to your buddy list.",(const char *)msg.player);
    }
    else //If the player used remove or the buddy was found in the player list remove it from there
    {
        chardata->RemoveBuddy( buddyid );
        Client* buddyClient = clients->FindPlayer( buddyid );
        if ( buddyClient )
        {
            psCharacter* buddyChar = buddyClient->GetCharacterData();
            if (buddyChar)
                buddyChar->NotBuddyOf( selfid );
        }

        if (!psserver->RemoveBuddy(selfid,buddyid))
        {
            psserver->SendSystemError(clientnum,"%s is not on your buddy list.",(const char *)msg.player);
            return;
        }

        psserver->SendSystemInfo(clientnum,"%s has been removed from your buddy list.",(const char *)msg.player);
    }

    BuddyList( client, clientnum, true );
}

void UserManager::BuddyList(Client *client,int clientnum,bool filter)
{
    psCharacter *chardata=client->GetCharacterData();
    if (chardata==NULL)
    {
        Error2("Client for account '%s' attempted to display buddy list but has no character data!",client->GetName());
        return;
    }

    int totalBuddies = (int)chardata->buddyList.GetSize(); //psBuddyListMsg should have as parameter a size_t. This is temporary.


    psBuddyListMsg mesg( clientnum, totalBuddies );
    for ( int i = 0; i < totalBuddies; i++ )
    {
        mesg.AddBuddy( i, chardata->buddyList[i].name, (clients->Find(chardata->buddyList[i].name)? true : false) );
    }

    mesg.Build();
    mesg.SendMessage();
}


void UserManager::NotifyBuddies(Client * client, bool logged_in)
{
    csString name (client->GetName());

    for (size_t i=0; i< client->GetCharacterData()->buddyOfList.GetSize(); i++)
    {
        Client *buddy = clients->FindPlayer( client->GetCharacterData()->buddyOfList[i] );  // name of player buddy

        if (buddy)  // is buddy online at the moment?  if so let him know buddy just logged on
        {
            psBuddyStatus status( buddy->GetClientNum(),  name , logged_in );
            status.SendMessage();

            if (logged_in)
            {
                psserver->SendSystemInfo(buddy->GetClientNum(),"%s just joined PlaneShift",name.GetData());
            }
            else
            {
                psserver->SendSystemInfo(buddy->GetClientNum(),"%s has quit",name.GetData());
            }
        }
    }
}

void UserManager::NotifyGuildBuddies(Client * client, bool logged_in)
{
    csString name (client->GetName());
    PID char_id = client->GetCharacterData()->GetPID();
    psGuildInfo * charGuild = client->GetCharacterData()->GetGuild();
    if(charGuild)
    {
        for(size_t i = 0; i < charGuild->members.GetSize(); i++)
        {
            psCharacter *notifiedmember = charGuild->members[i]->actor;
            gemActor *notifiedactor = notifiedmember? notifiedmember->GetActor() : NULL;

            if(notifiedactor && notifiedmember && (charGuild->members[i]->char_id != char_id))
            {
                if(notifiedmember->IsGettingGuildNotifications())
                {
                    csString text;
                    if (logged_in)
                        text.Format("/me just joined PlaneShift");
                    else
                        text.Format("/me has quit");

                    psChatMessage guildmsg(notifiedactor->GetClientID(),name.GetData(),0,text,CHAT_GUILD, false);
                    guildmsg.SendMessage();
                }
            }
        }
    }
}



void UserManager::RollDice(psUserCmdMessage& msg,Client *client,int clientnum)
{
    int total=0;

    if (msg.dice > 100)
        msg.dice = 100;
    if (msg.sides > 10000)
        msg.sides = 10000;
        if (msg.dtarget > msg.sides)
                msg.dtarget = msg.sides;

    if (msg.dice < 1)
        msg.dice = 1;
    if (msg.sides < 1)
        msg.sides = 1;
        if (msg.dtarget < 0)
                msg.dtarget = 0;

    for (int i = 0; i<msg.dice; i++)
    {
        // must use msg.sides instead of msg.sides-1 because rand never actually
        // returns max val, and int truncation never results in max val as a result
                if (msg.dtarget)
                        total += ((psserver->rng->Get(msg.sides) + 1 >= (uint)msg.dtarget)? 1: 0);
                else
                        total = total + psserver->rng->Get(msg.sides) + 1;
    }
        if (msg.dtarget)
        {
                if (msg.dice > 1)
                {
                        psSystemMessage newmsg(clientnum,MSG_INFO_BASE,
                                "Player %s has rolled %d %d-sided dice and had %d of them come up %d or greater.",
                                client->GetName(),
                                msg.dice,
                                msg.sides,
                                total,
                                msg.dtarget);

                        newmsg.Multicast(client->GetActor()->GetMulticastClients(),0, 10);
                }
                else
                {
                        psSystemMessage newmsg(clientnum,MSG_INFO_BASE,((total)?
                                "Player %s has rolled a %d-sided dice and had it come up %d or greater.":
                                "Player %s has rolled a %d-sided dice and had it come up less than %d."),
                                client->GetName(),
                                msg.sides,
                                msg.dtarget);

                        newmsg.Multicast(client->GetActor()->GetMulticastClients(),0, 10);
                }
        }
        else
        {
                if (msg.dice > 1)
                {
                        psSystemMessage newmsg(clientnum,MSG_INFO_BASE,
                                "Player %s has rolled %d %d-sided dice for a %d.",
                                client->GetName(),
                                msg.dice,
                                msg.sides,
                                total);

                        newmsg.Multicast(client->GetActor()->GetMulticastClients(),0, 10);
                }
                else
                {
                        psSystemMessage newmsg(clientnum,MSG_INFO_BASE,
                                "Player %s has rolled a %d-sided die for a %d.",
                                client->GetName(),
                                msg.sides,
                                total);

                        newmsg.Multicast(client->GetActor()->GetMulticastClients(),0, 10);
                }
        }
}


void UserManager::ReportPosition(psUserCmdMessage& msg,Client *client,int clientnum)
{

    if (msg.player.StartsWith("area:"))
    {
        csArray<csString> filters = DecodeCommandArea(client, msg.player);
        csArray<csString>::Iterator it(filters.GetIterator());
        while (it.HasNext()) {
            msg.player = it.Next();
            ReportPosition(msg, client, clientnum);
        }
        return;
    }

    gemObject *object = NULL;
    bool self = true;

    bool extras = CacheManager::GetSingleton().GetCommandManager()->Validate(client->GetSecurityLevel(), "pos extras");

    // Allow GMs to get other players' and entities' locations
    if (extras && msg.player.Length())
    {
        self = false;

        if (msg.player == "target")
        {
            object = client->GetTargetObject();
            if (!object)
            {
                psserver->SendSystemError(client->GetClientNum(), "You must have a target selected.");
                return;
            }
        }
        else
        {
            Client* c = psserver->GetAdminManager()->FindPlayerClient(msg.player);
            if (c) object = (gemObject*)c->GetActor();
            if (!object) object = psserver->GetAdminManager()->FindObjectByString(msg.player,client->GetActor());
        }
    }
    else
        object = client->GetActor();

    if (object)
    {
        csVector3 pos;
        iSector* sector = 0;
        float angle;

        object->GetPosition(pos, angle, sector);
        InstanceID instance = object->GetInstance();

        csString sector_name = (sector) ? sector->QueryObject()->GetName() : "(null)";

        csString name,range;
        if (self){
            name = "Your";
            range.Clear();
        }
        else
        {
            float dist = 0.0;
            csVector3 char_pos;
            float char_angle;
            iSector* char_sector;
            client->GetActor()->GetPosition(char_pos,char_angle,char_sector);
            dist = EntityManager::GetSingleton().GetWorld()->Distance(char_pos,char_sector,pos,sector);
            name.Format("%s's",object->GetName());
            range.Format(", range: %.2f",dist);
        }

        // Report extra info to GMs (players will use skills to determine correct direction)
        if (extras)
        {
            // Get the region this sector belongs to
            csString region_name = (sector) ? sector->QueryObject()->GetObjectParent()->GetName() : "(null)";
            csString idtxt; //holds the pid data if present
            if(object->GetActorPtr()) //check if it's an actor else just don't add the pid: entry
                idtxt.Format(", pid: %u",object->GetPID().Unbox()); //adds the pid: entry to the end of the output

            int degrees = (int)(angle*180.0/PI);
            psserver->SendSystemInfo(clientnum,
                "%s current position is region: %s %1.2f %1.2f %1.2f %d angle: %d, sector : %s%s%s",
                name.GetData(), region_name.GetData(), pos.x, pos.y, pos.z, instance, degrees,
                sector_name.GetData(), range.GetData(), idtxt.GetDataSafe());
        }
        else
        {
            psserver->SendSystemInfo(clientnum,"%s current position is %s %1.2f %1.2f %1.2f, instance: %d",
                name.GetData(), sector_name.GetData(), pos.x, pos.y, pos.z, instance);
        }
    }
}

void UserManager::HandleUnstick(psUserCmdMessage& msg, Client *client,
                                int clientnum)
{
    gemActor *actor = client->GetActor();
    if (!actor)
        return;

    StopAllCombat(client);
    LogStuck(client);

    if (actor->MoveToValidPos())
    {
        csVector3 pos;
        iSector* sector;

        actor->GetPosition(pos, sector);
        psserver->SendSystemInfo(clientnum,
            "Moving back to valid position in sector %s...",
            sector->QueryObject()->GetName());
    }
    else
    {
        int timeRemaining = (UNSTICK_TIME - int(csGetTicks() - actor->GetFallStartTime())) / 1000;
        if (actor->IsFalling() && timeRemaining > 0)
            psserver->SendSystemError(clientnum, "You cannot /unstick yet - please wait %d %s and try again.", timeRemaining, timeRemaining == 1 ? "second" : "seconds");
        else
            psserver->SendSystemError(clientnum, "You cannot /unstick at this time.");
    }
}

void UserManager::LogStuck(Client* client)
{
    csVector3 pos;
    float yrot;
    iSector* sector;

    if (!client || !client->GetActor())
        return;

    client->GetActor()->GetPosition(pos, yrot, sector);
    psRaceInfo* race = client->GetActor()->GetCharacterData()->GetRaceInfo();

    csString buffer;
    buffer.Format("%s, %s, %d, %s, %.3f, %.3f, %.3f, %.3f", client->GetName(),
        race->name.GetDataSafe(), race->gender,
        sector->QueryObject()->GetName(), pos.x, pos.y, pos.z, yrot);
    psserver->GetLogCSV()->Write(CSV_STUCK, buffer);
}

void UserManager::StopAllCombat(Client *client)
{
    if (!client || !client->GetActor())
        return;

    if (client->GetDuelClientCount() > 0)
        YieldDuel(client);
    else
        psserver->combatmanager->StopAttack(client->GetActor());
}

void UserManager::HandleAttack(psUserCmdMessage& msg,Client *client,int clientnum)
{
    Attack(client->GetCharacterData()->getStance(msg.stance), client, clientnum);
}

void UserManager::Attack(Stance stance, Client *client,int clientnum)
{
    if (!client->IsAlive() || client->IsFrozen())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot fight now.");
        return;
    }

    gemObject *target = client->GetTargetObject();
    if ( ! target )
    {
        psserver->SendSystemError(clientnum,"You do not have a target selected.");
        return;
    }
    if (target->GetItem() || strcmp(target->GetObjectType(), "ActionLocation") == 0 )
    {
        psserver->SendSystemError(clientnum,"You cannot attack %s.", (const char*)target->GetName() );
        return;
    }
    if (!target->IsAlive())
    {
        psserver->SendSystemError(clientnum,"%s is already dead.", (const char*)target->GetName() );
        return;
    }

    if (client->IsAllowedToAttack(target))
    {
        psserver->combatmanager->AttackSomeone(client->GetActor(), target, stance );
    }
}

void UserManager::Assist( psUserCmdMessage& msg, Client* client, int clientnum )
{
    Client* targetClient = NULL;

    if (!client->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot assist anybody now.");
        return;
    }

    // If the player doesn't provide an argument, use the players current
    // target instead.
    if ( msg.player.IsEmpty() )
    {
        int currentTarget = client->GetTargetClientID();
        if ( currentTarget == -1 )
        {
            psserver->SendSystemInfo( clientnum, "You have no target selected.");
            return;
        }

        if ( currentTarget == 0 )
        {
            psserver->SendSystemInfo( clientnum, "You can assist other players only.");
            return;
        }

        targetClient = clients->Find( currentTarget );
        if ( targetClient == NULL )
        {
            psserver->SendSystemInfo( clientnum, "Internal error - client not found.");
            return;
        }
    }
    else
    {
        csString playerName = NormalizeCharacterName(msg.player);
        targetClient = clients->Find( playerName );

        if ( !targetClient )
        {
            psserver->SendSystemInfo( clientnum,"Specified player is not online." );
            return;
        }
    }

    if ( targetClient == client )
    {
        psserver->SendSystemInfo( clientnum,"You cannot assist yourself." );
        return;
    }

    if ( !client->GetActor()->IsNear( targetClient->GetActor(), ASSIST_MAX_DIST ) )
    {
        psserver->SendSystemInfo(clientnum,"Specified player is too far away." );
        return;
    }

    // bug PS#2402 - Command /assist can be used to determine enemy's target.
    // Add check for actors in the same guild.
    if ( (client->GetGuildID() == 0) || (client->GetGuildID() != targetClient->GetGuildID()) )
    {
        // Add check for grouped actors
        int nGID = client->GetActor()->GetGroupID();
        if ( (nGID == 0) || (nGID != targetClient->GetActor()->GetGroupID()) )
        {
        	psserver->SendSystemInfo( clientnum,"You can only assist members of your guild or players you're grouped with." );
        	return;
        }
    }

    gemObject* targetObject = targetClient->GetTargetObject();
    if(!targetObject)
    {
        psserver->SendSystemInfo(clientnum,
            "Specified player has no target selected." );
        return;
    }

    client->SetTargetObject( targetObject, true );
}

void UserManager::UserStatRegeneration()
{
    GEMSupervisor::GetSingleton().UpdateAllStats();

    // Push a new event
    psUserStatRegeneration* event;
    nextUserStatRegeneration += 1000;
    event = new psUserStatRegeneration(this,nextUserStatRegeneration);
    psserver->GetEventManager()->Push(event);
}

void UserManager::Ready()
{
    nextUserStatRegeneration = csGetTicks();
    UserStatRegeneration();
}

/**
* Check target dead
* Check target lootable by this client
* Return lootable items list if present
*/
void UserManager::HandleLoot(Client *client)
{
    uint32_t clientnum = client->GetClientNum();

    if (!client->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot loot now.");
        return;
    }

    gemObject *target = client->GetTargetObject();
    if (!target)
    {
        psserver->SendSystemError(clientnum,"You don't have a target selected.");
        return;
    }

    gemNPC *npc = target->GetNPCPtr();
    if(!npc)
    {
        gemActor *actor = target->GetActorPtr();
        if(actor && actor->GetClient())
        {
            if (clientnum == actor->GetClient()->GetClientNum())
            {
                psserver->SendSystemError(clientnum, "You can't loot yourself.");
                return;
            }

            if (actor->IsAlive())
            {
                psserver->SendSystemError(clientnum, "You can't loot a person that is alive.");
                return;
            }
        }
        psserver->SendSystemError(clientnum, "You can only loot NPCs.");
        return;
    }


    if (target->IsAlive())
    {
        psserver->SendSystemError(clientnum, "You can't loot a creature that is alive.");
        return;
    }

    // Check target lootable by this client
    if (!npc->IsLootableClient(client->GetClientNum()))
    {
        Debug2(LOG_USER, client->GetClientNum(),"Client %d tried to loot mob that wasn't theirs.\n",client->GetClientNum() );
        psserver->SendSystemError(client->GetClientNum(),"You are not allowed to loot %s.",target->GetName() );
        return;
    }

    // Check to make sure loot is in range.
    if (client->GetActor()->RangeTo(target) > RANGE_TO_LOOT)
    {
        psserver->SendSystemError(client->GetClientNum(), "Too far away to loot %s", target->GetName() );         return;
    }


    // Return lootable items list if present
    psCharacter *chr = target->GetCharacterData();
    if (chr)
    {
        // Send items to looting player
        int money = chr->GetLootMoney();
        psLootMessage loot;
        size_t count = chr->GetLootItems(loot,
            target->GetEID(),
            client->GetClientNum() );
        if (count)
        {
            Debug3(LOG_LOOT, client->GetClientNum(), "Sending %zu loot items to %s.\n", count, client->GetActor()->GetName());
            loot.SendMessage();
        }
        else if(!count && !money)
        {
            Debug1(LOG_LOOT, client->GetClientNum(),"Mob doesn't have loot.\n");
            psserver->SendSystemError(client->GetClientNum(),"%s has nothing to be looted.",target->GetName() );
        }

        // Split up money among LootableClients in group.
        // These are those clients who were close enough when the NPC was killed.

        if (money)
        {
            Debug2(LOG_LOOT, client->GetClientNum(),"Splitting up %d money.\n", money);
            csRef<PlayerGroup> group = client->GetActor()->GetGroup();
            int remainder,each;
            if (group)
            {
                // Locate the group members who are in range to loot the trias
                csArray<gemActor*> closegroupmembers;
                const csArray<int>& lootable_clients = npc->GetLootableClients();
                size_t membercount = group->GetMemberCount();
                for (size_t i = 0 ; i < membercount ; i++)
                {
                    gemActor* currmember = group->GetMember(i);
                    if (!currmember)
                    {
                        continue;
                    }
                    //Copy all lootable clients.
                    //TODO: a direct interface to gemNPC::lootable_clients would be better
                    if (lootable_clients.Contains(currmember->GetClientID()) != csArrayItemNotFound)
                    {
                        closegroupmembers.Push(currmember);
                    }
                }

                remainder = money % (int) closegroupmembers.GetSize();
                each      = money / (int) closegroupmembers.GetSize();
                psMoney eachmoney = psMoney(each).Normalized();
                psMoney remmoney  = psMoney(remainder).Normalized();
                csString remstr   = remmoney.ToUserString();
                csString eachstr  = eachmoney.ToUserString();

                if (each)
                {
                    psSystemMessage loot(client->GetClientNum(),MSG_LOOT,"Everyone nearby has looted %s.",eachstr.GetData());
                    client->GetActor()->SendGroupMessage(loot.msg);

                    // Send a personal message to each group member about the money
                    for (size_t i = 0 ; i < membercount ; i++)
                    {
                        gemActor* currmember = group->GetMember(i);
                        if (closegroupmembers.Contains(currmember) != csArrayItemNotFound)
                        {
                            //Normal loot is not shown in yellow. Until that happens, do not do it for few coins
                            //psserver->SendSystemResult(currmember->GetClient()->GetClientNum(), "You have looted %s.", eachstr.GetData());
                            psserver->SendSystemInfo(currmember->GetClient()->GetClientNum(), "You have looted %s.", eachstr.GetData());

                            psLootEvent evt(
                                            chr->GetPID(),
                                            currmember->GetCharacterData()->GetPID(),
                                            0,
                                            0,
                                            0,
                                            eachmoney.GetTotal()
                                            );
                            evt.FireEvent();
                        }
                        else
                        {
                            // Something less intrusive for players who were too far away
                            psserver->SendSystemInfo(currmember->GetClient()->GetClientNum(), "You were too far away to loot.");
                        }


                    }

                    for(unsigned int i=0; i< closegroupmembers.GetSize(); i++)
                    {
                        closegroupmembers[i]->GetCharacterData()->AdjustMoney(eachmoney, false);
                        psserver->GetCharManager()->SendPlayerMoney(closegroupmembers[i]->GetClient());
                    }

                }
                if(remainder)
                {
                    psSystemMessage loot2(client->GetClientNum(),MSG_LOOT,"You have looted an extra %s.",remstr.GetData() );
                    loot2.SendMessage();

                    client->GetCharacterData()->AdjustMoney(remmoney, false);
                    psserver->GetCharManager()->SendPlayerMoney(client);
                    psLootEvent evt(
                                    chr->GetPID(),
                                    client->GetCharacterData()->GetPID(),
                                    0,
                                    0,
                                    0,
                                    remmoney.GetTotal()
                                    );
                    evt.FireEvent();
                }
            }
            else
            {
                psMoney m = psMoney(money).Normalized();
                csString mstr = m.ToUserString();

                psSystemMessage loot(client->GetClientNum(),MSG_LOOT,"You have looted %s.",mstr.GetData() );
                loot.SendMessage();

                client->GetCharacterData()->AdjustMoney(m, false);
                psserver->GetCharManager()->SendPlayerMoney(client);

                psLootEvent evt(
                               chr->GetPID(),
                               client->GetCharacterData()->GetPID(),
                               0,
                               0,
                               0,
                               m.GetTotal()
                               );
                evt.FireEvent();
            }
        }
        else
            Debug1(LOG_LOOT, client->GetClientNum(),"Mob has no money to loot.\n");
    }
}

void UserManager::HandleQuests(Client *client)
{
    psQuestListMessage quests;
    size_t count = client->GetActor()->GetCharacterData()->GetAssignedQuests(quests,client->GetClientNum() );

    if (count)
    {
        Debug3(LOG_QUESTS, client->GetClientNum(), "Sending %zu quests to player %s.\n", count, client->GetName());
        quests.SendMessage();
    }
    else
    {
        Debug1(LOG_QUESTS, client->GetClientNum(),"Client has no quests yet.\n");
    }
}

void UserManager::HandleGMEvents(Client* client)
{

    psGMEventListMessage gmEvents;
    size_t count = client->GetActor()->GetCharacterData()->GetAssignedGMEvents(gmEvents, client->GetClientNum());

    if (count)
    {
        Debug3(LOG_QUESTS, client->GetClientNum(),
            "Sending %zu events to player %s.\n",
            count, client->GetName() );
        gmEvents.SendMessage();
    }
    else
    {
        Debug1(LOG_QUESTS,
            client->GetClientNum(),
            "Client is not participating in a GM-Event.\n");
    }
}

void UserManager::HandleTraining(Client *client)
{
    if (!client->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot train your skills now");
        return;
    }

    // Check target is a Trainer
    gemObject *target = client->GetTargetObject();
    if (!target || !target->GetActorPtr())
    {
        psserver->SendSystemInfo(client->GetClientNum(),
            "No target selected for training!");
        return;
    }

    // Check range
    if (client->GetActor()->RangeTo(target) > RANGE_TO_SELECT)
    {
        psserver->SendSystemInfo(client->GetClientNum(),
            "You are not in range to train with %s.",target->GetCharacterData()->GetCharName());
        return;
    }

    if (!target->IsAlive())
    {
        psserver->SendSystemInfo(client->GetClientNum(),
            "Can't train with a dead trainer!");
        return;
    }

    if (client->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE)
    {
        csString err;
        err.Format("You can't train while %s.", client->GetCharacterData()->GetModeStr());
        psserver->SendSystemInfo(client->GetClientNum(), err);
        return;
    }

    psCharacter * trainer = target->GetCharacterData();
    if (!trainer->IsTrainer())
    {
        psserver->SendSystemInfo(client->GetClientNum(),
            "%s isn't a trainer.",target->GetCharacterData()->GetCharName());
        return;
    }

    psserver->GetProgressionManager()->StartTraining(client,trainer);
}

void UserManager::HandleBanking(Client *client, csString accountType)
{
    // Check if target is a banker.
    gemObject *target = client->GetTargetObject();
    if (!target || !target->GetActorPtr() || !target->GetActorPtr()->GetCharacterData()->IsBanker())
    {
        psserver->SendSystemError(client->GetClientNum(), "Your target must be a banker!");
        return;
    }

    // Check range
    if (client->GetActor()->RangeTo(target) > RANGE_TO_SELECT)
    {
        psserver->SendSystemError(client->GetClientNum(),
            "You are not within range to interact with %s.",target->GetCharacterData()->GetCharName());
        return;
    }

    // Check that the banker is alive!
    if (!target->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(), "You can't interact with a dead banker!");
        return;
    }

    // Make sure that we're not busy doing something else.
    if (client->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE)
    {
        csString err;
        err.Format("You can't access your bank account while %s.", client->GetCharacterData()->GetModeStr());
        psserver->SendSystemError(client->GetClientNum(), err);
        return;
    }

    // Check which account we're trying to access.
    if (accountType.CompareNoCase("personal"))
    {
        // Open personal bank.
        psserver->GetBankManager()->GetSingleton().StartBanking(client, false);
        return;
    }
    else if (accountType.CompareNoCase("guild"))
    {
        // Open guild bank.
        psserver->GetBankManager()->GetSingleton().StartBanking(client, true);
        return;
    }
    else
    {
        psserver->SendSystemError( client->GetClientNum(), "Usage: /bank [personal|guild]" );
        return;
    }
}

void UserManager::HandlePickup(Client *client, csString target)
{
    if (target.StartsWith("area:"))
    {
        csArray<csString> filters = DecodeCommandArea(client, target);
        csArray<csString>::Iterator it(filters.GetIterator());
        while (it.HasNext()) {
            HandlePickup(client, it.Next());
        }
        return;
    }
    else if (target.StartsWith("eid:", true))
    {
        gemObject* object= NULL;
        GEMSupervisor *gem = GEMSupervisor::GetSingletonPtr();
        csString eid_str = target.Slice(4);
        EID eID = EID(strtoul(eid_str.GetDataSafe(), NULL, 10));
        if (eID.IsValid())
        {
            object = gem->FindObject(eID);
            if (object && object->GetItem())
            {
                object->SendBehaviorMessage("pickup", client->GetActor() );
            }
            else
            {
                psserver->SendSystemError(client->GetClientNum(),
                                "Item not found.", target.GetData());
            }
        }
    }
    else
        psserver->SendSystemError(client->GetClientNum(),
                "You can't pickup %s", target.GetData());
}

void UserManager::HandleGuard(Client *client, csString target, csString action)
{
    if (target.StartsWith("area:"))
    {
        csArray<csString> filters = DecodeCommandArea(client, target);
        csArray<csString>::Iterator it(filters.GetIterator());
        while (it.HasNext()) {
            HandleGuard(client, it.Next(), action);
        }
        return;
    }
    else if (target.StartsWith("eid:", true))
    {
        gemObject* object= NULL;
        bool onoff = false;
        bool toggle = false;
        GEMSupervisor *gem = GEMSupervisor::GetSingletonPtr();
        csString eid_str = target.Slice(4);
        EID eID = EID(strtoul(eid_str.GetDataSafe(), NULL, 10));
        if (eID.IsValid())
        {
            object = gem->FindObject(eID);
            if (action == "on")         //The player provided an on so set ignore
                onoff = true;
            else if (action == "off") //The player provided an off so unset ignore
                onoff = false;
            else                           //The player didn't provide anything so toggle the option
                toggle = true;

            if (object && object->GetItem())
            {
                if(onoff || (toggle && object->GetItem()->GetGuardingCharacterID() == 0))
                {
                    if(client->GetSecurityLevel() > 22) //GM2
                    {
                        object->GetItem()->SetGuardingCharacterID(client->GetPID());
                        psserver->SendSystemError(client->GetClientNum(), "You have guarded %s", object->GetName());
                    }
                    else
                    {
                        psserver->SendSystemError(client->GetClientNum(), "You can't guard %s", object->GetName());
                    }
                }
                else
                {
                    if(object->GetItem()->GetGuardingCharacterID() == client->GetPID() || client->GetSecurityLevel() > 22)
                    {
                        object->GetItem()->SetGuardingCharacterID(0);
                        psserver->SendSystemError(client->GetClientNum(), "You have unguarded %s", object->GetName());
                    }
                    else
                    {
                        psserver->SendSystemError(client->GetClientNum(), "You can't unguard %s", object->GetName());
                    }
                }
            }
            else
            {
                psserver->SendSystemError(client->GetClientNum(),
                                "Item not found.", target.GetData());
            }
        }
    }
    else if(client->GetTargetObject())
    {
        gemObject* object= client->GetTargetObject();
        HandleGuard(client, object->GetEID().Show(), action);
    }
    else
        psserver->SendSystemError(client->GetClientNum(),
                "You can't change guard status of %s", target.GetData());
}

void UserManager::GiveTip(int id)
{
    unsigned int max=CacheManager::GetSingleton().GetTipLength();
    unsigned int rnd = psserver->rng->Get(max);

    csString tip;
    CacheManager::GetSingleton().GetTipByID(rnd, tip);
    psserver->SendSystemInfo(id,tip.GetData());
}

void UserManager::GiveMOTD(int id)
{
    psserver->SendSystemInfo(id,psserver->GetMOTD());
}


void UserManager::ShowNpcMenu(Client *client)
{
    gemNPC *npc = dynamic_cast<gemNPC*> ( client->GetTargetObject() );
    if (npc)
    {
        npc->ShowPopupMenu(client);
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "You must select an NPC first.");
    }
}

void UserManager::ChallengeToDuel(psUserCmdMessage& msg,Client *client)
{
    int clientnum = client->GetClientNum();

    if (!client->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot challenge opponents now");
        return;
    }

    //don't allow frozen clients to challenge
    if(client->IsFrozen())
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You can't challenge opponents while being frozen by a GM");
        return;
    }


    // Check target dead
    gemObject *target = client->GetTargetObject();
    if (!target)
    {
        psserver->SendSystemError(clientnum,"You don't have another player targeted.");
        return;
    }

    Client * targetClient = psserver->GetNetManager()->GetClient(target->GetClientID());
    if (!targetClient)
    {
        psserver->SendSystemError(clientnum, "You can challenge other players only");
        return;
    }

    // Distance
    if(client->GetActor()->RangeTo(target) > RANGE_TO_CHALLENGE)
    {
        psserver->SendSystemError(clientnum,target->GetName() + csString(" is too far away"));
        return;
    }

    if (!target->IsAlive())
    {
        psserver->SendSystemError(clientnum, "You can't challenge a dead person");
        return;
    }

    if(targetClient->IsFrozen())
    {
        psserver->SendSystemInfo(client->GetClientNum(), "% was frozen by a GM and cannot be challenged", target->GetName());
        return;
    }

    if (targetClient == client)
    {
        psserver->SendSystemError(clientnum, "You can't challenge yourself.");
        return;
    }

    // Check for pre-existing duel with this person
    if (client->IsDuelClient(target->GetClientID()))
    {
        psserver->SendSystemError(clientnum, "You have already agreed on this duel !");
        return;
    }

    // Challenge
    csString question;
    question.Format("%s has challenged you to a duel!  Click on Accept to allow the duel or Reject to ignore the challenge.",
        client->GetName() );
    PendingDuelInvite *invite = new PendingDuelInvite(client,
        targetClient,
        question);
    psserver->questionmanager->SendQuestion(invite);
}

void UserManager::AcceptDuel(PendingDuelInvite *invite)
{
    Client * inviteeClient = clients->Find(invite->clientnum);
    Client * inviterClient = clients->Find(invite->inviterClientNum);

    if (inviteeClient!=NULL  &&  inviterClient!=NULL)
    {
        inviteeClient->AddDuelClient( invite->inviterClientNum );
        inviterClient->AddDuelClient( invite->clientnum );

        // Target eachother and update their GUIs
        inviteeClient->SetTargetObject(inviterClient->GetActor(),true);
        inviterClient->SetTargetObject(inviteeClient->GetActor(),true);
    }
}

void PendingDuelInvite::HandleAnswer(const csString & answer)
{
    Client * client = psserver->GetConnections()->Find(clientnum);
    if (!client  ||  client->IsDuelClient(inviterClientNum))
        return;

    PendingInvite::HandleAnswer(answer);
    if (answer == "yes")
        psserver->usermanager->AcceptDuel(this);
}

void UserManager::YieldDuel(Client *client)
{
    if (client->GetActor()->GetMode() == PSCHARACTER_MODE_DEFEATED || !client->GetActor()->IsAlive())
        return;

    bool canYield = false;
    for (int i = 0; i < client->GetDuelClientCount(); i++)
    {
        Client *duelClient = psserver->GetConnections()->Find(client->GetDuelClient(i));
        if (duelClient && duelClient->GetActor() && duelClient->GetActor()->IsAlive() && duelClient->GetActor()->GetMode() != PSCHARACTER_MODE_DEFEATED)
        {
            canYield = true;
            if (duelClient->GetTargetObject() == client->GetActor())
                psserver->combatmanager->StopAttack(duelClient->GetActor());
            psserver->SendSystemOK(client->GetClientNum(), "You've yielded to %s!", duelClient->GetName());
            psserver->SendSystemOK(duelClient->GetClientNum(), "%s has yielded!", client->GetName());
        }
    }

    if (!canYield)
    {
        psserver->SendSystemOK(client->GetClientNum(), "You have no opponents to yield to!");
        return;
    }

    psSpareDefeatedEvent *evt = new psSpareDefeatedEvent(client->GetActor());
    psserver->GetEventManager()->Push(evt);
    client->GetActor()->SetMode(PSCHARACTER_MODE_DEFEATED);
}

void UserManager::SwitchAttackTarget(Client *targeter, Client *targeted )
{
    // If we switch targets while in combat, start attacking the new
    // target, unless we no longer have a target.
    if (targeted)
        Attack(targeter->GetCharacterData()->GetCombatStance(), targeter, targeter->GetClientNum());
    else
        psserver->combatmanager->StopAttack(targeter->GetActor());
}

void UserManager::Teleport( Client *client, float x, float y, float z, InstanceID instance, float rot, const char* sectorname )
{
    csVector3 pos( x,y,z );
    csRef<iEngine> engine = csQueryRegistry<iEngine> (psserver->GetObjectReg());
    iSector * sector = engine->GetSectors()->FindByName(sectorname);
    if ( !sector )
    {
        Bug2("Sector %s is not found!", sectorname );
        return;
    }
    if ( !client->GetActor() )
    {
        Bug1("Actor for client not found!" );
        return;
    }

    client->GetActor()->SetInstance(instance);
    client->GetActor()->Move( pos, rot, sector );
    client->GetActor()->SetPosition( pos, rot, sector );
    client->GetActor()->UpdateProxList(true);  // true=force update
    client->GetActor()->MulticastDRUpdate();
}




/*---------------------------------------------------------------------*/

psUserStatRegeneration::psUserStatRegeneration(UserManager *mgr, csTicks ticks)
: psGameEvent(ticks,0,"psUserStatRegeneration")
{
    usermanager = mgr;
}

void psUserStatRegeneration::Trigger()
{
    usermanager->UserStatRegeneration();
}
