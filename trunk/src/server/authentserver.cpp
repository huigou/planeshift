/*
 * AuthentServer.cpp by Keith Fulton <keith@paqrat.com>
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
#include <iutil/cfgmgr.h>
#include <csutil/csmd5.h>


//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psaccountinfo.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/pscharacterlist.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/pscharacter.h"

#include "net/messages.h"
#include "net/msghandler.h"

#include "util/psdatabase.h"
#include "util/log.h"
#include "util/eventmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "authentserver.h"
#include "adminmanager.h"
#include "weathermanager.h"
#include "client.h"
#include "playergroup.h"
#include "gem.h"
#include "clients.h"
#include "psserver.h"
#include "events.h"
#include "cachemanager.h"
#include "globals.h"
#include "icachedobject.h"
#include "advicemanager.h"
#include "commandmanager.h"

class CachedAuthMessage : public iCachedObject
{
public: 
    psAuthApprovedMessage *msg;

    CachedAuthMessage(psAuthApprovedMessage *message) { msg = message; }
    ~CachedAuthMessage() { delete msg; }

    // iCachedObject Functions below
    virtual void ProcessCacheTimeout() {};          /// required for iCachedObject but not used here
    virtual void *RecoverObject() { return this; }  /// Turn iCachedObject ptr into psAccountInfo
    virtual void DeleteSelf()     { delete this; }  /// Delete must come from inside object to handle operator::delete overrides.
};


AuthenticationServer::AuthenticationServer(ClientConnectionSet *pCCS,
                                               UserManager *usermgr,
                                               GuildManager *gm)
{
    clients      = pCCS;
    usermanager  = usermgr;
    guildmanager = gm;
    msgstringsmessage = NULL;

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<AuthenticationServer>(this,&AuthenticationServer::HandlePreAuthent),MSGTYPE_PREAUTHENTICATE,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<AuthenticationServer>(this,&AuthenticationServer::HandleAuthent),MSGTYPE_AUTHENTICATE,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<AuthenticationServer>(this,&AuthenticationServer::HandleDisconnect),MSGTYPE_DISCONNECT,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<AuthenticationServer>(this,&AuthenticationServer::HandleAuthCharacter),MSGTYPE_AUTHCHARACTER,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<AuthenticationServer>(this,&AuthenticationServer::HandleStatusUpdate),MSGTYPE_CLIENTSTATUS,REQUIRE_ANY_CLIENT);
}

AuthenticationServer::~AuthenticationServer()
{
    if (psserver->GetEventManager())
    {
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_PREAUTHENTICATE);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_AUTHENTICATE);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DISCONNECT);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_AUTHCHARACTER);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_CLIENTSTATUS);
    }
    if (msgstringsmessage)
        delete msgstringsmessage;
}


void AuthenticationServer::HandleAuthCharacter( MsgEntry* me, Client *client )
{
    psCharacterPickerMessage charpick( me );
    if (!charpick.valid)
    {
        Debug1(LOG_NET,me->clientnum,"Mangled psCharacterPickerMessage received.\n");
        return;
    } 

    psCharacterList *charlist = psserver->CharacterLoader.LoadCharacterList( client->GetAccountID());
    
    if (!charlist)
    {
        Error1("Could not load Character List for account! Rejecting client!\n");
        psserver->RemovePlayer( me->clientnum, "Could not load the list of characters for your account.  Please contact a PS Admin for help.");        
        return;
    } 

    int i;
    for (i=0;i<MAX_CHARACTERS_IN_LIST;i++)
    {
        if (charlist->GetEntryValid(i))
        {
            // Trim out whitespaces from name
            charpick.characterName.Trim();
            csString listName( charlist->GetCharacterFullName(i) );
            listName.Trim();
                                            
            if ( charpick.characterName == listName )
            {
                 client->SetPID(charlist->GetCharacterID(i));
                 // Set client name in code to just firstname as other code depends on it
                 client->SetName(charlist->GetCharacterName(i));
                 psCharacterApprovedMessage out( me->clientnum );
                 out.SendMessage();
                 break;
            }                     
        }
    }

    // cache will auto-delete this ptr if it times out
    CacheManager::GetSingleton().AddToCache(charlist, CacheManager::GetSingleton().MakeCacheName("list", client->GetAccountID().Unbox()),120);
}


bool AuthenticationServer::CheckAuthenticationPreCondition(int clientnum, bool netversionok, const char * sUser)
{
    /**
     * CHECK 1: Is Network protokol compatible?
     */
    if (!netversionok)
    {
        psserver->RemovePlayer (clientnum, "You are not running the correct version of PlaneShift. Please launch the updater.");
        return false;
    }

    /**
     * CHECK 2: Server has loaded a map
     */

    // check if server is already ready (ie level loaded)
    if (!psserver->IsReady())
    {
        if (psserver->HasBeenReady())
        {
            // Locked
            psserver->RemovePlayer(clientnum,"The server is up but about to shutdown. Please try again in 2 minutes.");

            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected: Server does not accept connections anymore.", sUser );
        }
        else
        {
            // Not ready yet
            psserver->RemovePlayer(clientnum,"The server is up but not fully ready to go yet. Please try again in a few minutes.");

            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected: Server has not loaded a world map.\n", sUser );
        }
        
        return false;
    }

    return true;
}


void AuthenticationServer::HandlePreAuthent(MsgEntry *me, Client *notused)
{
    psPreAuthenticationMessage msg(me);
    if (!msg.valid)
    {
        Debug1(LOG_NET,me->clientnum,"Mangled psPreAuthenticationMessage received.\n");
        return;
    }

    if (!CheckAuthenticationPreCondition(me->clientnum,msg.NetVersionOk(),"pre"))
        return;
    
    psPreAuthApprovedMessage reply(me->clientnum);
    reply.SendMessage();
}

void AuthenticationServer::HandleAuthent(MsgEntry *me, Client *notused)
{
    csTicks start = csGetTicks();

    psAuthenticationMessage msg(me); // This cracks message into members.

    if (!msg.valid)
    {
        Debug1(LOG_NET,me->clientnum,"Mangled psAuthenticationMessage received.\n");
        return;
    }

    if (!CheckAuthenticationPreCondition(me->clientnum, msg.NetVersionOk(),msg.sUser))
        return;

    csString status;
    status.Format("%s, %u, Received Authentication Message", (const char *) msg.sUser, me->clientnum);
    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);

    if ( msg.sUser.Length() == 0 || msg.sPassword.Length() == 0)
    {
        psserver->RemovePlayer(me->clientnum,"No username or password entered");

        Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected: No username or password.\n",
                (const char *)msg.sUser);            
        return;                
    }
    
    // Check if login was correct
    Notify2(LOG_CONNECTIONS,"Check Login for: '%s'\n", (const char*)msg.sUser);
    psAccountInfo *acctinfo=CacheManager::GetSingleton().GetAccountInfoByUsername((const char *)msg.sUser);

    if ( !acctinfo )
    {
        // invalid
        psserver->RemovePlayer(me->clientnum,"Incorrect password or username.");

        Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected: No account found with that name.\n",
                (const char *)msg.sUser);            
        return;                
    }

    // Add account to cache to optimize repeated login attempts
    CacheManager::GetSingleton().AddToCache(acctinfo,msg.sUser,120);
    
    // Check if password was correct
    csString passwordhashandclientnum (acctinfo->password);
    passwordhashandclientnum.Append(":");
    passwordhashandclientnum.Append(me->clientnum);
    
    csString encoded_hash = csMD5::Encode(passwordhashandclientnum).HexString();
    if (strcmp( encoded_hash.GetData() , msg.sPassword.GetData())) // authentication error
    {
        psserver->RemovePlayer(me->clientnum, "Incorrect password or username.");
        Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Bad password).",(const char *)msg.sUser);
        // No delete necessary because AddToCache will auto-delete
        // delete acctinfo;
        return;
    }
    
    /**
     * Check if the client is already logged in
     */
    Client* existingClient = clients->FindAccount(acctinfo->accountid, me->clientnum);
    if (existingClient)  // account already logged in
    {
        // invalid authent message from a different client
        csString reason;
        if(existingClient->IsZombie())
        {
            reason.Format("Your character(%s) was still in combat or casting a spell when you disconnected. "
                          "This connection is being overridden by a new login.", existingClient->GetName());
        }
        else
        {
            reason.Format("You are already logged on to this server as %s. "
                          "This connection is being overridden by a new login..", existingClient->GetName());
        }

        psserver->RemovePlayer(existingClient->GetClientNum(), reason);
        Notify2(LOG_CONNECTIONS,"User '%s' authentication request overrides an existing logged in user.\n",
            (const char *)msg.sUser);

        // No delete necessary because AddToCache will auto-delete
        // delete acctinfo;
    }


    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time authenticating account ID %u, After password check", 
            csGetTicks() - start, acctinfo->accountid);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }


    Client *client = clients->FindAny(me->clientnum);
    if (!client)
    {
        Bug2("Couldn't find client %d?!?",me->clientnum);
        // No delete necessary because AddToCache will auto-delete
        // delete acctinfo;
        return;
    }

    client->SetName(msg.sUser);
    client->SetAccountID( acctinfo->accountid );
    

    // Check to see if the client is banned
    time_t now = time(0);
    BanEntry* ban = banmanager.GetBanByAccount(acctinfo->accountid);
    if (ban == NULL)
    {
        // Account not banned; try IP range
        ban = banmanager.GetBanByIPRange(client->GetIPRange());
        // 2 day IP ban limit removed
        //if (ban && ban->end && now > ban->start + IP_RANGE_BAN_TIME)
        //{  
        //    // Only ban by IP range for the first 2 days
        //    ban = NULL;
        //}
    }
    if (ban)
    {
        if (now > ban->end)  // Time served
        {
            banmanager.RemoveBan(acctinfo->accountid);
        }
        else  // Notify and block
        {
            tm* timeinfo = gmtime(&(ban->end));
            csString banmsg;
            banmsg.Format("You are banned until %d-%d-%d %d:%d GMT.  Reason: %s",
                          timeinfo->tm_year+1900,
                          timeinfo->tm_mon+1,
                          timeinfo->tm_mday,
                          timeinfo->tm_hour,
                          timeinfo->tm_min,
                          ban->reason.GetData() );
    
            psserver->RemovePlayer(me->clientnum, banmsg);
    
            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Banned).",(const char *)msg.sUser);
            // No delete necessary because AddToCache will auto-delete
            // delete acctinfo;
            return;
        }
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time authenticating account ID %u, After ban check", 
            csGetTicks() - start, acctinfo->accountid);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    /** Check to see if there are any players on that account.  All accounts should have
    *    at least one player in this function.
    */
    psCharacterList *charlist = psserver->CharacterLoader.LoadCharacterList(acctinfo->accountid);

    if (!charlist)
    {
        Error2("Could not load Character List for account! Rejecting client %s!\n",(const char *)msg.sUser);
        psserver->RemovePlayer( me->clientnum, "Could not load the list of characters for your account.  Please contact a PS Admin for help.");
        delete acctinfo;
        return;
    }

    // cache will auto-delete this ptr if it times out
    CacheManager::GetSingleton().AddToCache(charlist, CacheManager::GetSingleton().MakeCacheName("list", client->GetAccountID().Unbox()),120);

    
     /**
     * CHECK 6: Connection limit
     * 
     * We check against number of concurrent connections, but players with
     * security rank of GameMaster or higher are not subject to this limit.
     */
    if (psserver->IsFull(clients->Count(),client)) 
    {
        // invalid
        psserver->RemovePlayer(me->clientnum, "The server is full right now.  Please try again in a few minutes.");

        Notify2(LOG_CONNECTIONS, "User '%s' authentication request rejected: Too many connections.\n", (const char *)msg.sUser );
        // No delete necessary because AddToCache will auto-delete
        // delete acctinfo;
        status = "User limit hit!";
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
        return;
    }

    Notify3(LOG_CONNECTIONS,"User '%s' (%d) added to active client list\n",(const char*) msg.sUser, me->clientnum);

    // Get the struct to refresh
    // Update last login ip and time
    char addr[20];
    client->GetIPAddress(addr);
    acctinfo->lastloginip = addr;

    tm* gmtm = gmtime(&now);
    csString timeStr;
    timeStr.Format("%d-%02d-%02d %02d:%02d:%02d",
        gmtm->tm_year+1900,
        gmtm->tm_mon+1,
        gmtm->tm_mday,
        gmtm->tm_hour,
        gmtm->tm_min,
        gmtm->tm_sec);

    acctinfo->lastlogintime = timeStr;
    CacheManager::GetSingleton().UpdateAccountInfo(acctinfo);

    iCachedObject *obj = CacheManager::GetSingleton().RemoveFromCache(CacheManager::GetSingleton().MakeCacheName("auth",acctinfo->accountid));
    CachedAuthMessage *cam;

    if (!obj)
    {
        // Send approval message
        psAuthApprovedMessage *message = new psAuthApprovedMessage(me->clientnum,client->GetPID(), charlist->GetValidCount() );    

        if(csGetTicks() - start > 500)
        {
            csString status;
            status.Format("Warning: Spent %u time authenticating account ID %u, After approval", 
                csGetTicks() - start, acctinfo->accountid);
            psserver->GetLogCSV()->Write(CSV_STATUS, status);
        }

        // Send out the character list to the auth'd player    
        for (int i=0; i<MAX_CHARACTERS_IN_LIST; i++)
        {
            if (charlist->GetEntryValid(i))
            {
                // Quick load the characters to get enough info to send to the client
                psCharacter* character = psserver->CharacterLoader.QuickLoadCharacterData( charlist->GetCharacterID(i), false );
                if (character == NULL)
                {
                    Error2("QuickLoadCharacterData failed for character '%s'", charlist->GetCharacterName(i));
                    continue;
                }

                Notify3(LOG_CHARACTER, "Sending %s to client %d\n", character->name.GetData(), me->clientnum );
                character->AppendCharacterSelectData(*message);

                delete character;
            }
        }
        message->ConstructMsg();
        cam = new CachedAuthMessage(message);
    }
    else
    {
        // recover underlying object
        cam = (CachedAuthMessage *)obj->RecoverObject();
        // update client id since new connection here
        cam->msg->msg->clientnum = me->clientnum;
    }
    // Send auth approved and char list in one message now
    cam->msg->SendMessage();
    CacheManager::GetSingleton().AddToCache(cam, CacheManager::GetSingleton().MakeCacheName("auth",acctinfo->accountid), 10);

    SendMsgStrings(me->clientnum); 
    
    client->SetSpamPoints(acctinfo->spamPoints);
    client->SetAdvisorPoints(acctinfo->advisorPoints);
    client->SetSecurityLevel(acctinfo->securitylevel);

    if (acctinfo->securitylevel >= GM_TESTER)
    {
        psserver->GetAdminManager()->Admin(me->clientnum, client);
    }
    
    if (CacheManager::GetSingletonPtr()->GetCommandManager()->Validate(client->GetSecurityLevel(), "default advisor"))
        psserver->GetAdviceManager()->AddAdvisor(client);

    psserver->GetWeatherManager()->SendClientGameTime(me->clientnum);

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time authenticating account ID %u, After load", 
            csGetTicks() - start, acctinfo->accountid);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    status.Format("%s - %s, %u, Logged in", addr, (const char*) msg.sUser, me->clientnum);
    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);
}

void AuthenticationServer::HandleDisconnect(MsgEntry* me,Client *client)
{
    psDisconnectMessage mesg(me);

    // Check if this client is allowed to disconnect or if the
    // zombie state should be set
    if(!client->AllowDisconnect())
        return;

    if(mesg.msgReason == "!") // Not a final disconnect
    {
        psserver->RemovePlayer(me->clientnum,"!");
    }
    else
    {
        psserver->RemovePlayer(me->clientnum,"Your client has disconnected. If you are seeing this message a connection error has likely occurred.");
    }
    
}

void AuthenticationServer::SendDisconnect(Client* client, const char *reason)
{
    if (client->GetActor())
    {
        psDisconnectMessage msg(client->GetClientNum(), client->GetActor()->GetEID(), reason);
        if (msg.valid)
        {
            msg.msg->priority = PRIORITY_LOW;
            psserver->GetEventManager()->Broadcast(msg.msg, NetBase::BC_FINALPACKET);
        }
        else
        {
            Bug2("Could not create a valid psDisconnectMessage for client %u.\n",client->GetClientNum());
        }
    }
    else
    {
        psDisconnectMessage msg(client->GetClientNum(), 0, reason);
        if (msg.valid)
        {
            psserver->GetEventManager()->Broadcast(msg.msg, NetBase::BC_FINALPACKET);
        }
    }
}

void AuthenticationServer::SendMsgStrings(int cnum)
{
    // send message strings hash table to client
    if (!msgstringsmessage)
        msgstringsmessage = new psMsgStringsMessage(cnum, CacheManager::GetSingleton().GetMsgStrings());
    else
        msgstringsmessage->msg->clientnum = cnum;
    if (!msgstringsmessage->valid)
    {
        Bug1("Could not form a valid psMsgStringsMessage from Message Strings!\n");
        delete msgstringsmessage;
        msgstringsmessage=NULL;
    }
    else
        msgstringsmessage->SendMessage();
}

void AuthenticationServer::HandleStatusUpdate(MsgEntry* me, Client* client)
{
    psClientStatusMessage msg(me);
    // printf("Got ClientStatus message!\n");

    // We ignore !ready messages because of abuse potential.
    if (msg.ready)
    {
        psConnectEvent evt(client->GetClientNum());
        evt.FireEvent();
        client->SetReady(msg.ready);
    }
}

BanManager::BanManager()
{
    Result result(db->Select("SELECT * FROM bans"));
    if (!result.IsValid())
        return;

    time_t now = time(0);
    for (unsigned int i=0; i<result.Count(); i++)
    {
        AccountID account = AccountID(result[i].GetUInt32("account"));
        time_t end = result[i].GetUInt32("end");

        if (now > end)  // Time served
        {
            db->Command("DELETE FROM bans WHERE account='%u'", account.Unbox());
            continue;
        }

        BanEntry* newentry = new BanEntry;
        newentry->account = account;
        newentry->end = end;
        newentry->start = result[i].GetUInt32("start");
        newentry->ipRange = result[i]["ip_range"];
        newentry->reason = result[i]["reason"];
        newentry->banIP = result[i].GetUInt32("ban_ip") != 0;
        
        // If account ban, add to list
        if (newentry->account.IsValid())
            banList_IDHash.Put(newentry->account,newentry);

        // If IP range ban, add to list
        if ( newentry->ipRange.Length() && newentry->banIP /*(!end || now < newentry->start + IP_RANGE_BAN_TIME)*/ )
            banList_IPRList.Push(newentry);
    }
}

BanManager::~BanManager()
{
    csHash<BanEntry*, AccountID>::GlobalIterator it(banList_IDHash.GetIterator());
    while (it.HasNext())
    {
        BanEntry* entry = it.Next();
        delete entry;
    }
}

bool BanManager::RemoveBan(AccountID account)
{
    BanEntry* ban = GetBanByAccount(account);
    if (!ban)
        return false;  // Not banned

    db->Command("DELETE FROM bans WHERE account='%u'", account.Unbox());
    banList_IDHash.Delete(account,ban);
    banList_IPRList.Delete(ban);
    delete ban;
    return true;
}

bool BanManager::AddBan(AccountID account, csString ipRange, time_t duration, csString reason, bool banIP)
{
    if (GetBanByAccount(account))
        return false;  // Already banned

    time_t now = time(0);

    BanEntry* newentry = new BanEntry;
    newentry->account = account;
    newentry->start = now;
    newentry->end = now + duration;
    newentry->ipRange = ipRange;
    newentry->reason = reason;
    newentry->banIP = banIP;

    csString escaped;
    db->Escape(escaped, reason.GetData());
    int ret = db->Command("INSERT INTO bans "
                          "(account,ip_range,start,end,reason,ban_ip) "
                          "VALUES ('%u','%s','%u','%u','%s','%u')",
                          newentry->account.Unbox(),
                          newentry->ipRange.GetData(),
                          newentry->start,
                          newentry->end,
                          escaped.GetData(),
                          newentry->banIP );
    if (ret == -1)
    {
        delete newentry;
        return false;
    }

    if (newentry->account.IsValid())
        banList_IDHash.Put(newentry->account,newentry);

    if (newentry->ipRange.Length() && newentry->banIP)
        banList_IPRList.Push(newentry);

    return true;
}

BanEntry* BanManager::GetBanByAccount(AccountID account)
{
    return banList_IDHash.Get(account,NULL);
}

BanEntry* BanManager::GetBanByIPRange(csString IPRange)
{
    for (size_t i=0; i<banList_IPRList.GetSize(); i++)
        if ( IPRange.StartsWith(banList_IPRList[i]->ipRange) )
            return banList_IPRList[i];
    return NULL;
}
