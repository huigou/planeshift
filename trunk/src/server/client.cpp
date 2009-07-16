/*
 * client.cpp - Author: Keith Fulton
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/util.h>


//=============================================================================
// Project Space Includes
//=============================================================================
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psguildinfo.h"

#include "util/psscf.h"
#include "util/consoleout.h"
#include "util/psdatabase.h"
#include "net/msghandler.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "usermanager.h"
#include "client.h"
#include "psserver.h"
#include "playergroup.h"
#include "globals.h"
#include "netmanager.h"
#include "combatmanager.h"
#include "events.h"
#include "advicemanager.h"
#include "entitymanager.h"
#include "cachemanager.h"
#include "adminmanager.h"

Client::Client ()
    : accumulatedLag(0), zombie(false), allowedToDisconnect(true), ready(false), mute(false),
      accountID(0), playerID(0), securityLevel(0), superclient(false),
      name(""), waypointEffectID(0), waypointIsDisplaying(false),
      pathEffectID(0), pathPath(NULL), pathIsDisplaying(false),
      locationEffectID(0), locationIsDisplaying(false),cheatMask(NO_CHEAT)
{
    actor           = 0;
    target          = 0;
    exchangeID      = 0;
    advisorPoints   = 0;
    lastInviteTime  = 0;
    spamPoints      = 0;
    clientnum       = 0;
    detectedCheatCount = 0;

    nextFloodHistoryIndex = 0;

    isAdvisor           = false;
    isFrozen            = false;
    lastInviteResult    = true;
    hasBeenWarned       = false;
    hasBeenPenalized    = false;
    valid               = false;

    // pets[0] is a special case for the players familiar.
    pets.Insert(0, PID(0));
}

Client::~Client()
{
}

bool Client::Initialize(LPSOCKADDR_IN addr, uint32_t clientnum)
{
    Client::addr=*addr;
    CS_ASSERT_MSG("Unexpected size for IP address structure!", sizeof(addr->sin_addr.s_addr) + sizeof(addr->sin_port) == 6);
    Client::clientnum=clientnum;
    Client::valid=true;

    outqueue = csPtr<NetPacketQueueRefCount>
      (new NetPacketQueueRefCount (MAXCLIENTQUEUESIZE));
    if (!outqueue)
        ERRORHALT("No Memory!");

    return true;
}

bool Client::Disconnect()
{
    // Make sure the advisor system knows this client is gone.
    if ( isAdvisor )
    {
        psserver->GetAdviceManager()->RemoveAdvisor( this->GetClientNum(), 0);
    }

    // take the player off his mount if he got one
    if(GetActor() && GetActor()->GetMount())
        GetActor()->SetMount(GetActor()->GetMount(), false);

    if (GetActor() && GetActor()->InGroup())
    {
        GetActor()->RemoveFromGroup();
    }

    // Only save if an account has been found for this client.
    if (accountID.IsValid())
    {
        SaveAccountData();
    }

    /*we have to clear the challenges else the other players will be stuck
     in challenge mode */
    if(GetDuelClientCount())
    {
        ClearAllDuelClients();
    }

    return true;
}

void Client::SetAllowedToDisconnect( bool allowed )
{
    allowedToDisconnect = allowed;
}

bool Client::AllowDisconnect()
{
    if(!GetActor() || !GetCharacterData())
        return true;

    if(GetActor()->GetInvincibility())
        return true;

    if(!zombie)
    {
        zombie = true;
        // max 3 minute timeout period
        zombietimeout = csGetTicks() + 3 * 60 * 1000;
    }
    else if(csGetTicks() > zombietimeout)
    {
        return true;
    }

    return allowedToDisconnect;
}

// Called from network thread, no access to server
// internal data should be made
bool Client::ZombieAllowDisconnect()
{
    if(csGetTicks() > zombietimeout)
    {
        return true;
    }

    return allowedToDisconnect;
}

void Client::SetTargetObject(gemObject* newobject, bool updateClientGUI)
{
    // We don't want to fire a target change event if the target hasn't changed.
    if (newobject == target)
        return;

    target = newobject;

    gemActor * myactor = GetActor();
    if (myactor)
    {
        psTargetChangeEvent targetevent( myactor, newobject );
        targetevent.FireEvent();
    }

    if (updateClientGUI)
    {
        psGUITargetUpdateMessage updateMessage(GetClientNum(), newobject->GetEID());
        updateMessage.SendMessage();
    }
}

void Client::SetFamiliar( gemActor *familiar )
{
    if ( familiar )
    {
        pets[0] = familiar->GetPID();
    }
    else
    {
        pets[0] = PID(0);
    }
}

gemActor* Client::GetFamiliar()
{
    if (pets[0].IsValid())
    {
        return GEMSupervisor::GetSingleton().FindNPCEntity(pets[0]);
    }
    return NULL;
}

void Client::AddPet( gemActor *pet )
{
    pets.Push(pet->GetPID());
}
void Client::RemovePet( size_t index )
{
    pets.DeleteIndex(index);
}

gemActor* Client::GetPet(size_t i)
{
    if (i >= pets.GetSize() || !pets[i].IsValid())
        return NULL;

    return GEMSupervisor::GetSingleton().FindNPCEntity(pets[i]);
}

size_t Client::GetNumPets()
{
    return pets.GetSize();
}

bool Client::IsMyPet(gemActor * other) const
{
    for (size_t i = 0; i < pets.GetSize(); i++)
    {
        if (GEMSupervisor::GetSingleton().FindNPCEntity( pets[i] ) == other)
        {
            return true;
        }
    }
    return false;
}

void Client::SetActor(gemActor* myactor)
{
    actor = myactor;
    if (actor == NULL)
    {
        allowedToDisconnect = true;
    }
}

psCharacter *Client::GetCharacterData()
{
    return (actor?actor->GetCharacterData():NULL);
}


bool Client::ValidateDistanceToTarget(float range)
{
    // Check if target is set
    if (!target) return false;

    return actor->IsNear(target,range);
}


int Client::GetTargetClientID()
{
    // Check if target is set
    if (!target) return -1;

    return target->GetClientID();
}

int Client::GetGuildID()
{
    psCharacter * mychar = GetCharacterData();
    if (mychar == NULL)
        return 0;

    psGuildInfo * guild = mychar->GetGuild();
    if (guild == NULL)
        return 0;

    return guild->id;
}

int Client::GetAllianceID()
{
    psCharacter * mychar = GetCharacterData();
    if (mychar == NULL)
        return 0;

    psGuildInfo * guild = mychar->GetGuild();
    if (guild == NULL)
        return 0;
    
    return guild->GetAllianceID();
}

unsigned int Client::GetAccountTotalOnlineTime()
{
    unsigned int totalTimeConnected = GetCharacterData()->GetOnlineTimeThisSession();
    Result result(db->Select("SELECT SUM(time_connected_sec) FROM characters WHERE account_id = %d", accountID.Unbox()));
    if (result.IsValid())
        totalTimeConnected += result[0].GetUInt32("SUM(time_connected_sec)");

    return totalTimeConnected;
}

void Client::AddDuelClient(int clientnum)
{
    if (!IsDuelClient(clientnum))
        duel_clients.Push(clientnum);
}

void Client::RemoveDuelClient(Client *client)
{
    if (actor)
        actor->RemoveAttackerHistory(client->GetActor());
    duel_clients.Delete(client->GetClientNum());
}

void Client::ClearAllDuelClients()
{
    for (int i = 0; i < GetDuelClientCount(); i++)
    {
        Client *duelClient = psserver->GetConnections()->Find(duel_clients[i]);
        if (duelClient)
        {
            // Also remove us from their list.
            duelClient->RemoveDuelClient(this);

            if (actor)
                actor->RemoveAttackerHistory(duelClient->GetActor());
        }
    }
    duel_clients.Empty();
}

int Client::GetDuelClientCount()
{
    return (int)duel_clients.GetSize();
}

int Client::GetDuelClient(int id)
{
    return duel_clients[id];
}

bool Client::IsDuelClient(int clientnum)
{
    return (duel_clients.Find(clientnum) != csArrayItemNotFound);
}

void Client::AnnounceToDuelClients(gemActor *attacker, const char *event)
{
    for (size_t i = 0; i < duel_clients.GetSize(); i++)
    {
        uint32 duelClientID = duel_clients[i];
        Client *duelClient = psserver->GetConnections()->Find(duelClientID);
        if (duelClient)
        {
            if (!attacker)
                psserver->SendSystemOK(duelClientID, "%s has %s %s!", GetName(), event, duelClient->GetName());
            else if (duelClientID == attacker->GetClientID())
                psserver->SendSystemOK(duelClientID, "You've %s %s!", event, GetName());
            else
                psserver->SendSystemOK(duelClientID, "%s has %s %s!", attacker->GetName(), event, GetName());
        }
    }
}

void Client::FloodControl(uint8_t chatType, const csString & newMessage, const csString & recipient)
{
    int matches = 0;

    floodHistory[nextFloodHistoryIndex] = FloodBuffRow(chatType, newMessage, recipient, csGetTicks());
    nextFloodHistoryIndex = (nextFloodHistoryIndex + 1) % floodMax;

    // Count occurances of this new message in the flood history.
    for (int i = 0; i < floodMax; i++)
    {
        if (csGetTicks() - floodHistory[i].ticks < floodForgiveTime && floodHistory[i].chatType == chatType && floodHistory[i].text == newMessage && floodHistory[i].recipient == recipient)
            matches++;
    }

    if (matches >= floodMax)
    {
        SetMute(true);
        psserver->SendSystemError(clientnum, "BAM! Muted.");
    }
    else if (matches >= floodWarn)
    {
        psserver->SendSystemError(clientnum, "Flood warning. Stop or you will be muted.");
    }
}

FloodBuffRow::FloodBuffRow(uint8_t chtType, csString txt, csString rcpt, unsigned int newticks)
{
    chatType = chtType;
    recipient = rcpt;
    text = txt;
    ticks = newticks;
}

bool Client::IsGM() const
{
    return GetSecurityLevel() >= GM_LEVEL_0;
}

bool Client::IsAllowedToAttack(gemObject * target, bool inform)
{
    csString tmp;
    const char *sMsg = NULL;

    if ( target == NULL )
    {
        sMsg = "The target selected is no more valid.";
        if (inform)
        {
            psserver->SendSystemError(clientnum, sMsg );
        }

        return false;
    }

    // Am I a GM?
    if (IsGM())
    {
        return true; /* Everything is attackable */
    }

    switch ( this->GetTargetType( target ) )
    {
        case TARGET_NONE:
            sMsg = "You must select a target to attack.";
            break;
        case TARGET_ITEM:
            sMsg = "You can't attack an inanimate object.";
            break;
        case TARGET_SELF:
            sMsg = "You cannot attack yourself.";
            break;
        case TARGET_FRIEND:
            sMsg = "You cannot attack %s.";
            break;
        case TARGET_FOE: /* Foe */
            {
                gemActor *foe = target->GetActorPtr();
                CS_ASSERT( foe != NULL ); // Since this is a foe it should have a actor.
                gemActor *attacker = GetActor();

                gemActor *lastAttacker=NULL;
                if (!foe->CanBeAttackedBy(attacker,&lastAttacker))
                {
                    if (lastAttacker)
                    {
                        tmp.Format("You must be grouped with %s to attack %s.",
                                   lastAttacker->GetName(), foe->GetName());
                    }
                    else
                    {
                        tmp.Format("You are not allowed to attack right now.");
                    }
                    sMsg = tmp.GetData();
                }
            }
            break;
    }

    if ( sMsg != NULL )
    {
        if(inform)
        {
            psserver->SendSystemError(clientnum, sMsg, target->GetName() );
        }

        return false;
    }
    return true;
}

int Client::GetTargetType(gemObject* target)
{
    if (!target)
    {
        return TARGET_NONE; /* No Target */
    }

    if (target->GetActorPtr() == NULL)
    {
        return TARGET_ITEM; /* Item */
    }

    if (!target->IsAlive())
    {
        return TARGET_DEAD;
    }

    if (GetActor() == target)
    {
        return TARGET_SELF; /* Self */
    }

    if (target->GetCharacterData()->impervious_to_attack)
    {
        return TARGET_FRIEND; /* Impervious NPC */
    }

    // Is target a NPC?
    Client* targetclient = psserver->GetNetManager()->GetAnyClient(target->GetClientID());
    if (!targetclient)
    {
        if (target->GetCharacterData()->IsPet())
        {
            /* Pet's target type depends on its owner's (enable when they can defend themselves)
            gemObject* owner = GEMSupervisor::GetSingleton().FindPlayerEntity( target->GetCharacterData()->GetOwnerID() );
            if ( !owner || !IsAllowedToAttack(owner,false) )
            */
                return TARGET_FRIEND;
        }

        return TARGET_FOE; /* Foe */
    }

    if (targetclient->GetActor()->GetInvincibility())
        return TARGET_FRIEND; /* Invincible GM */

    if (targetclient->GetActor()->attackable)
        return TARGET_FOE; /* attackable GM */

    // Challenged to a duel?
    if (IsDuelClient(target->GetClientID())
        || targetclient->IsDuelClient(clientnum))
    {
        return TARGET_FOE; /* Attackable player */
    }

    // In PvP region?
    csVector3 attackerpos, targetpos;
    float yrot;
    iSector* attackersector, *targetsector;
    GetActor()->GetPosition(attackerpos, yrot, attackersector);
    target->GetPosition(targetpos, yrot, targetsector);

    if (psserver->GetCombatManager()->InPVPRegion(attackerpos,attackersector)
        && psserver->GetCombatManager()->InPVPRegion(targetpos,targetsector))
    {
        return TARGET_FOE; /* Attackable player */
    }

    // Is this a player who has hit you and run out of a PVP area?
    for (size_t i=0; i< GetActor()->GetDamageHistoryCount(); i++)
    {
        const DamageHistory *dh = GetActor()->GetDamageHistory((int)i);
        // If the target has ever hit you, you can attack them back.  Logging out clears this.
        if (dh->attacker_ref.IsValid() && dh->attacker_ref->GetActorPtr() == target)
            return TARGET_FOE;
    }

    // Declared war?
    psGuildInfo* attackguild = GetActor()->GetGuild();
    psGuildInfo* targetguild = targetclient->GetActor()->GetGuild();
    if (attackguild && targetguild &&
        targetguild->IsGuildWarActive(attackguild))
    {
        return TARGET_FOE; /* Attackable player */
    }

    return TARGET_FRIEND; /* Friend */
}

static inline void TestTarget(csString& targetDesc, int32_t targetType,
                              enum TARGET_TYPES type, const char* desc)
{
    if (targetType & type)
    {
        if (targetDesc.Length() > 0)
        {
            targetDesc.Append((targetType > (type * 2)) ? ", " : ", or ");
        }
        targetDesc.Append(desc);
    }
}

void Client::GetTargetTypeName(int32_t targetType, csString& targetDesc) const
{
    targetDesc.Clear();
    TestTarget(targetDesc, targetType, TARGET_NONE, "the surrounding area");
    TestTarget(targetDesc, targetType, TARGET_ITEM, "items");
    TestTarget(targetDesc, targetType, TARGET_SELF, "yourself");
    TestTarget(targetDesc, targetType, TARGET_FRIEND, "living friends");
    TestTarget(targetDesc, targetType, TARGET_FOE, "living enemies");
    TestTarget(targetDesc, targetType, TARGET_DEAD, "the dead");
}

bool Client::IsAlive(void) const
{
    return actor ? actor->IsAlive() : true;
}

void Client::SaveAccountData()
{
    // First penalty after relogging should not be death
    if (spamPoints >= 2)
        spamPoints = 1;

    // Save to the db
    db->CommandPump("UPDATE accounts SET spam_points = '%d', advisor_points = '%d' WHERE id = '%d' LIMIT 1",
                    spamPoints, advisorPoints, accountID.Unbox());
}

uint32_t Client::WaypointGetEffectID()
{
    if (waypointEffectID == 0)
        waypointEffectID = CacheManager::GetSingleton().NextEffectUID();

    return waypointEffectID;
}

uint32_t Client::PathGetEffectID()
{
    if (pathEffectID == 0)
        pathEffectID = CacheManager::GetSingleton().NextEffectUID();

    return pathEffectID;
}

uint32_t Client::LocationGetEffectID()
{
    if (locationEffectID == 0)
        locationEffectID = CacheManager::GetSingleton().NextEffectUID();

    return locationEffectID;
}

void Client::SetAdvisorBan(bool ban)
{
    db->Command("UPDATE accounts SET advisor_ban = %d WHERE id = %d", (int) ban, accountID.Unbox());

    if (isAdvisor)
        psserver->GetAdviceManager()->RemoveAdvisor(clientnum, clientnum);

    psserver->SendSystemError(clientnum, "You have been %s from advising by a GM.", ban ? "banned" : "unbanned");
}

bool Client::IsAdvisorBanned()
{
    bool advisorBan = false;
    Result result(db->Select("SELECT advisor_ban FROM accounts WHERE id = %d", accountID.Unbox()));
    if (result.IsValid())
        advisorBan = result[0].GetUInt32("advisor_ban") != 0;

    return advisorBan;
}

void Client::SetCheatMask(CheatFlags mask,bool flag )
{
    if (flag)
        cheatMask |= mask;
    else
        cheatMask &= (~mask);
}

OrderedMessageChannel *Client::GetOrderedMessageChannel(msgtype mtype)
{
	OrderedMessageChannel *channel = orderedMessages.Get(mtype,NULL);

	if (!channel)
	{
		channel = new OrderedMessageChannel;
		orderedMessages.Put(mtype, channel);
	}
	return channel;
}

