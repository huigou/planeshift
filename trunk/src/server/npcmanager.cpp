/*
* npcmanager.cpp by Keith Fulton <keith@paqrat.com>
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
#include <ctype.h>
#include <csutil/csmd5.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================

#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/npcmessages.h"

#include "util/eventmanager.h"
#include "util/serverconsole.h"
#include "util/psdatabase.h"
#include "util/log.h"
#include "util/psconst.h"
#include "util/strutil.h"
#include "util/psxmlparser.h"
#include "util/mathscript.h"
#include "util/serverconsole.h"
#include "util/command.h"

#include "engine/psworld.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psaccountinfo.h"
#include "bulkobjects/servervitals.h"
#include "bulkobjects/psraceinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "globals.h"
#include "events.h"
#include "psserver.h"
#include "cachemanager.h"
#include "playergroup.h"
#include "gem.h"
#include "combatmanager.h"
#include "authentserver.h"
#include "entitymanager.h"
#include "spawnmanager.h"
#include "clients.h"                // Client, and ClientConnectionSet classes
#include "client.h"                 // Client, and ClientConnectionSet classes
#include "psserverchar.h"
#include "creationmanager.h"
#include "npcmanager.h"
#include "workmanager.h"
#include "weathermanager.h"
#include "adminmanager.h"



class psNPCManagerTick : public psGameEvent
{
protected:
    NPCManager *npcmgr;

public:
    psNPCManagerTick(int offsetticks, NPCManager *c);
    virtual void Trigger();  ///< Abstract event processing function
};

const int NPC_TICK_INTERVAL = 300;  //msec

/**
 * This class is the relationship of Owner to Pet ( which includes Familiars ).
 */
class PetOwnerSession : public iDeleteObjectCallback
{

public:
    PID ownerID; ///< Character ID of the owner
    PID petID; ///< Character ID of the pet
    bool isActive;
    gemActor *owner;
    NPCManager *manager;
    double elapsedTime; ///< create time
    csString curDate;
    csString curTime;

    PetOwnerSession()
    {
        manager = NULL;
        owner = NULL;
        elapsedTime = 0.0f;
        isActive = false;
    };

    PetOwnerSession( NPCManager *mgr, gemActor *owner, psCharacter* pet)
    {
        manager = mgr;

        if ( owner )
        {
            this->ownerID = owner->GetCharacterData()->GetPID();
            this->owner = owner;
            this->owner->RegisterCallback( this );
        }

        if ( pet )
        {
            this->petID = pet->GetPID();
        }

        if ( owner && pet )
        {
            CPrintf(CON_DEBUG, "Created PetSession (%s, %s)\n", owner->GetName(), ShowID(pet->GetPID()));
        }

        elapsedTime = 0.0f;
        isActive = true;

        // Get pet characterdata
        if ( pet )
        {
            csString last_login = pet->GetLastLoginTime();
            if ( last_login.Length() > 0 )
            {
                curDate = last_login.Truncate( 10 ); // YYYY-MM-DD
                CPrintf(CON_DEBUG,"Last Logged Date : %s\n", curDate.GetData() );

                pet->GetLastLoginTime().SubString(curTime, 11 );
                CPrintf(CON_DEBUG,"Last Logged time : %s\n", curTime.GetData() );

                int hours = 0, mins = 0, secs = 0;

                hours = atoi( curTime.Slice( 0, 2 ) ); //hours
                mins = atoi( curTime.Slice( 3, 2 ) );
                secs = atoi( curTime.Slice( 6, 2 ) );

                elapsedTime = ( ( ( ( hours * 60 ) + mins  ) * 60 ) + secs ) * 1000;
            }
            else
            {
                curDate.Clear();
            }
        }

    };

    virtual ~PetOwnerSession()
    {
        if ( owner )
            owner->UnregisterCallback( this );
    };

    /// Renable Time tracking for returning players
    void Reconnect( gemActor *owner )
    {
        if ( owner )
        {
            if ( this->owner )
            {
                owner->UnregisterCallback( this );
            }

            this->owner = owner;
            this->owner->RegisterCallback( this );
        }
    };

    /// Disable time tracking for disconnected clients
    virtual void DeleteObjectCallback(iDeleteNotificationObject * object)
    {
        gemActor *sender = (gemActor *)object;

        if ( sender && sender == owner )
        {
            if ( owner )
            {
                owner->UnregisterCallback( this );
                owner = NULL;
            }
        }
    };

    /// Update time in game for this session
    void UpdateElapsedTime( float elapsed )
    {
        gemActor *pet = NULL;

        if ( owner && isActive)
        {
            this->elapsedTime += elapsed;

            //Update Session status using new elpasedTime values
            CheckSession();

            size_t numPets = owner->GetClient()->GetNumPets();
            // Check to make sure this pet is still summoned
            for( size_t i = 0; i < numPets; i++ )
            {
                pet = owner->GetClient()->GetPet( i );
                if ( pet && pet->GetCharacterData() && pet->GetCharacterData()->GetPID() == petID )
                {
                    break;
                }
                else
                {
                    pet = NULL;
                }
            }

            if ( pet )
            {
                int hour = 0, min = 0, sec = 0;

                sec = (long) ( elapsedTime / 1000 ) % ( 60 ) ;
                min = (long) ( elapsedTime / ( 1000 * 60 ) )  % ( 60 );
                hour = (long) ( elapsedTime / ( 1000 * 60 * 60 ) )  % ( 24 );

                csString strLastLogin;
                strLastLogin.Format("%s %02d:%02d:%02d",
                            this->curDate.GetData(),
                            hour, min, sec);

                pet->GetCharacterData()->SetLastLoginTime( strLastLogin, false );

                if ( !isActive ) // past Session time
                {
                    psserver->CharacterLoader.SaveCharacterData( pet->GetCharacterData(), pet, true );

                    CPrintf(CON_NOTIFY,"NPCManager Removing familiar %s from owner %s.\n",pet->GetName(),pet->GetName() );
                    owner->GetClient()->SetFamiliar( NULL );
                    EntityManager::GetSingleton().RemoveActor( pet );
                    psserver->SendSystemInfo( owner->GetClientID(), "You feel your power to maintain your pet wane." );
                }
            }
        }

    };

    /// used to verify the session should still be valid
    bool CheckSession()
    {
        double maxTime = GetMaxPetTime();

        time_t old;
        time(&old);
        tm* tm_old = localtime(&old);

        int year;
        sscanf(curDate, "%d-%d-%d", &year, &tm_old->tm_mon, &tm_old->tm_mday);
        year = year - 1900;
        tm_old->tm_year = year;

        sscanf(curTime, "%d:%d:%d", &tm_old->tm_hour, &tm_old->tm_min, &tm_old->tm_sec);
        old = mktime(tm_old);

        time_t curr;
        time(&curr);
        tm* tm_curr = localtime(&curr);

        int secsdiff = difftime(curr, old);
        if (secsdiff >= 240*60)
        {
            year = tm_curr->tm_year+1900;
            curDate.Format("%d-%d-%d", year, tm_curr->tm_mon, tm_curr->tm_mday);
            curTime.Format("%d:%d:%d", tm_curr->tm_hour, tm_curr->tm_min, tm_curr->tm_sec);
            elapsedTime = 0;
        }

        if ( elapsedTime >= maxTime && owner->GetClient()->GetSecurityLevel() < GM_LEVEL_9 )
        {
            CPrintf(CON_NOTIFY,"PetSession marked invalid (%s, %s)\n", owner->GetName(), ShowID(petID));

            this->isActive = false;
            return false;
        }

        return true;
    }

    /// Uses a MathScript to calculate the maximum amount of time a Pet can remain in world.
    double GetMaxPetTime()
    {
        static MathScript *maxPetTime;
        double maxTime = 60 * 5 * 1000;

        if (!maxPetTime)
            maxPetTime = psserver->GetMathScriptEngine()->FindScript("CalculateMaxPetTime");

        if (maxPetTime && owner)
        {
            MathEnvironment env;
            env.Define("Actor", owner->GetCharacterData());
            env.Define("Skill", owner->GetCharacterData()->Skills().GetSkillRank(PSSKILL_EMPATHY).Current());
            maxPetTime->Evaluate(&env);
            MathVar *timeValue = env.Lookup("MaxTime");
            maxTime = timeValue->GetValue();
        }

        return maxTime;
    }
};

NPCManager::NPCManager(ClientConnectionSet *pCCS,
                       psDatabase *db,
                       EventManager *evtmgr)
{
    clients      = pCCS;
    database     = db;
    eventmanager = evtmgr;

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<NPCManager>(this,&NPCManager::HandleAuthentRequest),MSGTYPE_NPCAUTHENT,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<NPCManager>(this,&NPCManager::HandleCommandList)   ,MSGTYPE_NPCOMMANDLIST,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<NPCManager>(this,&NPCManager::HandleConsoleCommand),MSGTYPE_NPC_COMMAND,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<NPCManager>(this,&NPCManager::HandleNPCReady)   ,MSGTYPE_NPCREADY, REQUIRE_ANY_CLIENT);

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<NPCManager>(this,&NPCManager::HandleDamageEvent),MSGTYPE_DAMAGE_EVENT,NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<NPCManager>(this,&NPCManager::HandleDeathEvent) ,MSGTYPE_DEATH_EVENT,NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<NPCManager>(this,&NPCManager::HandlePetCommand) ,MSGTYPE_PET_COMMAND,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<NPCManager>(this,&NPCManager::HandlePetSkill)   ,MSGTYPE_PET_SKILL,REQUIRE_ANY_CLIENT);

    PrepareMessage();

    psNPCManagerTick *tick = new psNPCManagerTick(NPC_TICK_INTERVAL,this);
    eventmanager->Push(tick);

    petRangeScript = psserver->GetMathScriptEngine()->FindScript("CalculateMaxPetRange");
    petReactScript = psserver->GetMathScriptEngine()->FindScript("CalculatePetReact");;
}

bool NPCManager::Initialize()
{
    if (!petRangeScript)
    {
        Error1("No CalculateMaxPetRange script!");
        return false;
    }

    if (!petReactScript)
    {
        Error1("No CalculatePetReact script!");
        return false;
    }

    return true;
}


NPCManager::~NPCManager()
{
	csHash<PetOwnerSession*, PID>::GlobalIterator iter(OwnerPetList.GetIterator());
	while(iter.HasNext())
		delete iter.Next();
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_NPCAUTHENT);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_NPCOMMANDLIST);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_NPCREADY);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_NPC_COMMAND);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DAMAGE_EVENT);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DEATH_EVENT);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_PET_COMMAND);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_PET_SKILL);
    delete outbound;
}

void NPCManager::HandleNPCReady(MsgEntry *me,Client *client)
{
	GEMSupervisor::GetSingleton().ActivateNPCs(client->GetAccountID());
	// NPC Client is now ready so add onto superclients list
	client->SetReady(true);
	superclients.Push(PublishDestination(client->GetClientNum(), client, 0, 0));
}

void NPCManager::HandleDamageEvent(MsgEntry *me,Client *client)
{
    psDamageEvent evt(me);

    // NPC's need to know they were hit using a Perception
    if (evt.attacker!=NULL  &&  dynamic_cast<gemNPC*>(evt.target)) // if npc damaged
    {
        QueueDamagePerception(evt.attacker, dynamic_cast<gemNPC*>(evt.target), evt.damage);
    }
}

void NPCManager::HandleDeathEvent(MsgEntry *me,Client *client)
{
    Debug1(LOG_SUPERCLIENT, 0,"NPCManager handling Death Event\n");
    psDeathEvent evt(me);

    QueueDeathPerception(evt.deadActor);
}


void NPCManager::HandleConsoleCommand(MsgEntry *me,Client *client)
{
    csString buffer;

    psServerCommandMessage msg(me);
    printf("Got command: %s\n", msg.command.GetDataSafe() );

    size_t i = msg.command.FindFirst(' ');
    csString word;
    msg.command.SubString(word,0,i);
    const COMMAND *cmd = find_command(word.GetDataSafe());

    if (cmd && cmd->allowRemote)
    {
        int ret = execute_line(msg.command, &buffer);
        if (ret == -1)
            buffer = "Error executing command on the server.";
    }
    else
    {
        buffer = cmd ? "That command is not allowed to be executed remotely"
                     : "No command by that name.  Please try again.";
    }
    printf("%s\n", buffer.GetData());

    psServerCommandMessage retn(me->clientnum, buffer);
    retn.SendMessage();
}


void NPCManager::HandleAuthentRequest(MsgEntry *me,Client *notused)
{
    Client* client = clients->FindAny(me->clientnum);
    if (!client)
    {
        Error1("NPC Manager got authentication message from already connected client!");
        return;
    }

    psNPCAuthenticationMessage msg(me);

    csString status;
    status.Format("%s, %u, Received NPC Authentication Message", (const char *) msg.sUser, me->clientnum);
    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);

    // CHECK 1: Networking versions match
    if (!msg.NetVersionOk())
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Version mismatch.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
        //psserver->RemovePlayer (me->clientnum, "You are not running the correct version of PlaneShift for this server.");
        Error2("Superclient '%s' is not running the correct version of PlaneShift for this server.",
            (const char *)msg.sUser);
        return;
    }

    // CHECK 2: Is the server ready yet to accept connections
    if (!psserver->IsReady())
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Server not ready");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);

        if (psserver->HasBeenReady())
        {
            // Locked
            Error2("Superclient '%s' authentication request rejected: Server is up but about to shutdown.\n",
                (const char *)msg.sUser );
        }
        else
        {
            // Not ready
            // psserver->RemovePlayer(me->clientnum,"The server is up but not fully ready to go yet. Please try again in a few minutes.");

            Error2("Superclient '%s' authentication request rejected: Server not ready.\n",
                (const char *)msg.sUser );
        }
        return;
    }

    // CHECK 3: Is the client is already logged in?
    if (msg.sUser.Length() == 0)
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Empty username.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
        Error1("No username specified.\n");
        return;
    }

    msg.sUser.Downcase();
    msg.sUser.SetAt(0,toupper(msg.sUser.GetAt(0)));



    // CHECK 4: Check to see if the login is correct.

    Notify2(LOG_SUPERCLIENT, "Check Superclient Login for: '%s'\n", (const char*)msg.sUser);
    psAccountInfo *acctinfo=CacheManager::GetSingleton().GetAccountInfoByUsername((const char *)msg.sUser);


    csString password = csMD5::Encode(msg.sPassword).HexString();

    if (acctinfo==NULL || strcmp(acctinfo->password,(const char *)password)) // authentication error
    {
        if (acctinfo==NULL)
        {
            psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Username not found.");
            psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Username not found).\n",(const char *)msg.sUser);
        }
        else
        {
            psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Bad password.");
            psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Bad password).\n",(const char *)msg.sUser);
        }
        delete acctinfo;
        return;
    }

    client->SetPID(0);
    client->SetSecurityLevel(acctinfo->securitylevel);

    if (acctinfo->securitylevel!= 99)
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Wrong security level.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
        // psserver->RemovePlayer(me->clientnum, "Login id does not have superclient security rights.");

        Error3("User '%s' authentication request rejected (security level was %d).\n",
            (const char *)msg.sUser, acctinfo->securitylevel );
        delete acctinfo;
        return;
    }
    

    Client* existingClient = clients->FindAccount(acctinfo->accountid, client->GetClientNum());
    if(existingClient)// username already logged in
    {
        // invalid
        // psserver->RemovePlayer(me->clientnum,"You are already logged on to this server. If you were disconnected, please wait 30 seconds and try again.");
    	// Diconnect existing superclient

        Error2("Superclient '%s' already logged in. Logging out existing client\n",
               (const char *)msg.sUser);
        psserver->RemovePlayer(existingClient->GetClientNum(),"Existing connection overridden by new login.");
    }
    
    // *********Successful superclient login here!**********

    time_t curtime = time(NULL);
    struct tm *gmttime;
    gmttime = gmtime (&curtime);
    csString buf(asctime(gmttime));
    buf.Truncate(buf.Length()-1);

    Debug3(LOG_SUPERCLIENT,0,"Superclient '%s' connected at %s.\n",(const char *)msg.sUser, (const char *)buf);

    client->SetName(msg.sUser);
    client->SetAccountID( acctinfo->accountid );
    client->SetSuperClient( true );

    char addr[30];
    client->GetIPAddress(addr);
    //TODO:    database->UpdateLoginDate(cid,addr);

    psserver->GetAuthServer()->SendMsgStrings(me->clientnum, false);

    SendMapList(client);
    SendRaces(client);

    psserver->GetWeatherManager()->SendClientGameTime(me->clientnum);

    delete acctinfo;

    status.Format("%s, %u, %s, Superclient logged in", (const char*) msg.sUser, me->clientnum, addr);
    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);
}

void NPCManager::Disconnect(Client *client)
{
    // Deactivate all the NPCs that are managed by this client
    GEMSupervisor::GetSingleton().StopAllNPCs(client->GetAccountID() );

    // Disconnect the superclient
    for (size_t i=0; i< superclients.GetSize(); i++)
    {
        PublishDestination &pd = superclients[i];
        if ((Client *)pd.object == client)
        {
            superclients.DeleteIndex(i);
            Debug1(LOG_SUPERCLIENT, 0,"Deleted superclient from NPCManager.\n");
            return;
        }
    }
    CPrintf(CON_DEBUG, "Attempted to delete unknown superclient.\n");
}

void NPCManager::SendMapList(Client *client)
{
    psWorld *psworld = EntityManager::GetSingleton().GetWorld();

    csString regions;
    psworld->GetAllRegionNames(regions);

    psMapListMessage list(client->GetClientNum(),regions);
    list.SendMessage();
}

void NPCManager::SendRaces(Client *client)
{
    uint32_t count = (uint32_t)CacheManager::GetSingleton().GetRaceInfoCount();

    psNPCRaceListMessage newmsg(client->GetClientNum(),count);
    for (uint32_t i=0; i < count; i++)
    {
        psRaceInfo * ri = CacheManager::GetSingleton().GetRaceInfoByIndex(i);
        newmsg.AddRace(ri->name,ri->walkBaseSpeed,ri->runBaseSpeed, i == (count-1));
    }
    newmsg.SendMessage();
}


void NPCManager::SendNPCList(Client *client)
{
    int count = GEMSupervisor::GetSingleton().CountManagedNPCs(client->GetAccountID());

    // Note, building the internal message outside the msg ctor is very bad
    // but I am doing this to avoid sending database result sets to the msg ctor.
    psNPCListMessage newmsg(client->GetClientNum(),count * 2 * sizeof(uint32_t) + sizeof(uint32_t));

    newmsg.msg->Add( (uint32_t)count );

    GEMSupervisor::GetSingleton().FillNPCList(newmsg.msg,client->GetAccountID());

    if (!newmsg.msg->overrun)
    {
        newmsg.SendMessage();
    }
    else
    {
        Bug2("Overran message buffer while sending NPC List to client %u.\n",client->GetClientNum());
    }
}

void NPCManager::HandleCommandList(MsgEntry *me,Client *client)
{
    psNPCCommandsMessage list(me);

    if (!list.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Could not parse psNPCCommandsMessage from client %u.\n",me->clientnum);
        return;
    }

    csTicks begin = csGetTicks();
    int count[24] = {0};
    int times[24] = {0};

    int cmd = list.msg->GetInt8();
    while (cmd != list.CMD_TERMINATOR && !list.msg->overrun)
    {
        csTicks cmdBegin = csGetTicks();
        count[cmd]++;
        switch(cmd)
        {
            case psNPCCommandsMessage::CMD_DRDATA:
            {
                // extract the data
                uint32_t len = 0;
				void *data = list.msg->GetBufferPointerUnsafe(len);

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Error2("Received incomplete CMD_DRDATA from NPC client %u.\n",me->clientnum);
                    break;
                }

                psDRMessage drmsg(data,len,
                                  CacheManager::GetSingleton().GetMsgStrings(), 0,
                                  EntityManager::GetSingleton().GetEngine());  // alternate method of cracking

                // copy the DR data into an iDataBuffer
                csRef<iDataBuffer> databuf = csPtr<iDataBuffer> (new csDataBuffer (len));
                memcpy(databuf->GetData(), data, len);
                // find the entity and Set the DR data for it
                gemNPC *actor = dynamic_cast<gemNPC*>(GEMSupervisor::GetSingleton().FindObject(drmsg.entityid));

                if (!actor)
                {
                    Error2("Illegal %s from superclient!\n", ShowID(drmsg.entityid));

                }
                else if (!actor->IsAlive())
                {
                    Debug3(LOG_SUPERCLIENT, actor->GetPID().Unbox(), "Ignoring DR data for dead npc %s(%s).\n",
                           actor->GetName(), ShowID(drmsg.entityid));
                }
                else
                {
                    // Go ahead and update the server version
                    actor->SetDRData(drmsg);
                    // Now multicast to other clients
                    actor->UpdateProxList();

                    actor->MulticastDRUpdate();
                    if(drmsg.vel.y < -20 || drmsg.pos.y < -1000)                   //NPC has fallen down
                    {
                        // First print out what happend
                        CPrintf(CON_DEBUG, "Received bad DR data from NPC %s(%s %s), killing NPC.\n",
                                actor->GetName(), ShowID(drmsg.entityid), ShowID(actor->GetPID()));
                        csVector3 pos;
                        float yrot;
                        iSector*sector;
                        actor->GetPosition(pos,yrot,sector);
                        CPrintf(CON_DEBUG, "Pos: %s\n",toString(pos,sector).GetData());

                        // Than kill the NPC
                        actor->Kill(NULL);
                        break;
                    }
                }
                break;
            }
            case psNPCCommandsMessage::CMD_ATTACK:
            {
                EID attacker_id = EID(list.msg->GetUInt32());
                EID target_id = EID(list.msg->GetUInt32());
                Debug3(LOG_SUPERCLIENT, attacker_id.Unbox(), "-->Got attack cmd for entity %s to %s\n", ShowID(attacker_id), ShowID(target_id));

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, attacker_id.Unbox(), "Received incomplete CMD_ATTACK from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC *attacker = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(attacker_id));
                if (attacker && attacker->IsAlive())
                {
                    gemObject *target   = (gemObject *)GEMSupervisor::GetSingleton().FindObject(target_id);
                    if (!target)
                    {
                        attacker->SetTarget(target);
                        if (attacker->GetMode() == PSCHARACTER_MODE_COMBAT)
                        {
                            psserver->combatmanager->StopAttack(attacker);
                        }

                        if(target_id==0)
                        {
                            Debug2(LOG_SUPERCLIENT, attacker_id.Unbox(), "%s has stopped attacking.\n", attacker->GetName());
                        }
                        else      // entity may have been removed since this msg was queued
                        {
                            Debug2(LOG_SUPERCLIENT, attacker_id.Unbox(), "Couldn't find entity %s to attack.\n", ShowID(target_id));
                        }
                    }
                    else
                    {
                        attacker->SetTarget(target);
                        if (attacker->GetMode() == PSCHARACTER_MODE_COMBAT)
                        {
                            psserver->combatmanager->StopAttack(attacker);
                        }

                        if ( !target->GetClient() || !dynamic_cast<gemActor*>(target)->GetInvincibility() )
                        {
                            // NPCs only use 'Normal' stance for now.
                            psserver->combatmanager->AttackSomeone(attacker,target,CombatManager::GetStance("Normal"));
                            Debug3(LOG_SUPERCLIENT, attacker_id.Unbox(), "%s is now attacking %s.\n", attacker->GetName(), target->GetName());
                        }
                        else
                        {
                            Debug3(LOG_SUPERCLIENT, attacker_id.Unbox(), "%s cannot attack GM [%s].\n", attacker->GetName(), target->GetName());
                        }
                    }

                }
                else
                {
                    Debug2(LOG_SUPERCLIENT, attacker_id.Unbox(), "No entity %s or not alive", ShowID(attacker_id));
                }
                break;
            }
            case psNPCCommandsMessage::CMD_SPAWN:
            {
                EID spawner_id = EID(list.msg->GetUInt32());
                EID spawned_id = EID(list.msg->GetUInt32());
                Debug3(LOG_SUPERCLIENT, spawner_id.Unbox(), "-->Got spawn cmd for entity %s to %s\n", ShowID(spawner_id), ShowID(spawned_id));

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_SPAWN from NPC client %u.\n",me->clientnum);
                    break;
                }
                gemNPC *spawner = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(spawner_id));

                if (spawner)
                {
                    EntityManager::GetSingleton().CloneNPC(spawner->GetCharacterData());
                }
                else
                {
                    Error1("NPC Client try to clone non existing npc");
                }
                break;
            }
            case psNPCCommandsMessage::CMD_TALK:
            {
                EID speaker_id = EID(list.msg->GetUInt32());
                const char* text = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT,me->clientnum,"-->Got talk cmd: %s for entity %s\n", text, ShowID(speaker_id));

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_TALK from NPC client %u.\n",me->clientnum);
                    break;
                }
                gemNPC *speaker = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(speaker_id));
                csTicks timeDelay=0;
                speaker->Say(text,NULL,false,timeDelay);
                break;
            }
            case psNPCCommandsMessage::CMD_VISIBILITY:
            {
                EID entity_id = list.msg->GetUInt32();
                bool status = list.msg->GetBool();
                Debug3(LOG_SUPERCLIENT, me->clientnum, "-->Got visibility cmd: %s for entity %s\n", status ? "yes" : "no", ShowID(entity_id));

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_VISIBILITY from NPC client %u.\n",me->clientnum);
                    break;
                }
                gemNPC *entity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                entity->SetVisibility(status);
                break;
            }
            case psNPCCommandsMessage::CMD_PICKUP:
            {
                EID entity_id = list.msg->GetUInt32();
                EID item_id = list.msg->GetUInt32();
                int count = list.msg->GetInt16();
                Debug4(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got pickup cmd: Entity %s to pickup %d of %s\n", ShowID(entity_id), count, ShowID(item_id));

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_PICKUP from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT,0,"Couldn't find character data.\n");
                    break;
                }

                gemItem *gItem = dynamic_cast<gemItem *> (GEMSupervisor::GetSingleton().FindObject(item_id));
                psItem *item = NULL;
                if (gItem) item = gItem->GetItemData();
                if (!item)
                {
                    Debug1(LOG_SUPERCLIENT,0,"Couldn't find item data.\n");
                    break;
                }

                // If the entity is in range of the item AND the item may be picked up, and check for dead user
                if ( gEntity->IsAlive() && (gItem->RangeTo( gEntity ) < RANGE_TO_SELECT) && gItem->IsPickable())
                {

                    // TODO: Include support for splitting of a stack
                    //       into count items.

                    // Cache values from item, because item might be deleted in Add
                    csString qname = item->GetQuantityName();

                    if (chardata && chardata->Inventory().Add(item))
                    {
                        EntityManager::GetSingleton().RemoveActor(gItem);  // Destroy this
                    } else
                    {
                        // TODO: Handle of pickup of partial stacks.
                    }
                }

                break;
            }
            case psNPCCommandsMessage::CMD_EQUIP:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                csString item = list.msg->GetStr();
                csString slot = list.msg->GetStr();
                int count = list.msg->GetInt16();
                Debug6(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got equip cmd from %u: Entity %s to equip %d %s in %s\n",
                       me->clientnum, ShowID(entity_id), count, item.GetData(), slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_EQUIP from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)CacheManager::GetSingleton().slotNameHash.GetID( slot );
                if (slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find slot %s.\n", slot.GetData());
                    break;
                }

                // Create the item
                psItemStats* baseStats = CacheManager::GetSingleton().GetBasicItemStatsByName(item);
                if (!baseStats)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find base for item %s.\n", item.GetData());
                    break;
                }
                printf("Searching for item_stats %p (%s)\n", baseStats, baseStats->GetName() );

                // See if the char already has this item
                size_t index = chardata->Inventory().FindItemStatIndex(baseStats);
                if (index != SIZET_NOT_FOUND)
                {
                    printf("Equipping existing %s on character %s.\n",baseStats->GetName(), chardata->GetCharName() );

                    psItem *existingItem = chardata->Inventory().GetInventoryIndexItem(index);
                    if (!chardata->Inventory().EquipItem(existingItem, (INVENTORY_SLOT_NUMBER) slotID))
                        Error3("Could not equip %s in slot %u for npc, but it is in inventory.\n",existingItem->GetName(),slotID);
                }
                else
                {
                    // Make a permanent new item
                    psItem* newItem = baseStats->InstantiateBasicItem( true );
                    if (!newItem)
                    {
                        Debug2(LOG_SUPERCLIENT,0,"Couldn't create item %s.\n",item.GetData());
                        break;
                    }
                    newItem->SetStackCount(count);

                    if (chardata->Inventory().Add(newItem,false,false))  // Item must be in inv before equipping it
                    {
                        if (!chardata->Inventory().EquipItem(newItem, (INVENTORY_SLOT_NUMBER) slotID))
                            Error3("Could not equip %s in slot %u for npc, but it is in inventory.\n",newItem->GetName(),slotID);
                    }
                    else
                    {
                        Error2("Adding new item %s to inventory failed.", item.GetData());
                        delete newItem;
                    }
                }
                break;
            }
            case psNPCCommandsMessage::CMD_DEQUIP:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                csString slot = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got dequip cmd: Entity %s to dequip from %s\n",
                       ShowID(entity_id), slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_DEQUIP from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)CacheManager::GetSingleton().slotNameHash.GetID( slot );
                if (slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find slot %s.\n", slot.GetData());
                    break;
                }

                printf("Removing item in slot %d from %s.\n",slotID,chardata->GetCharName() );

                psItem * oldItem = chardata->Inventory().RemoveItem(NULL,(INVENTORY_SLOT_NUMBER)slotID);
                if (oldItem)
                    delete oldItem;

                break;
            }
            case psNPCCommandsMessage::CMD_DIG:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                csString resource = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got dig cmd: Entity %s to dig for %s\n",
                       ShowID(entity_id), resource.GetData());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_DIG from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                psserver->GetWorkManager()->HandleProduction(gEntity,"dig",resource);

                break;
            }
            case psNPCCommandsMessage::CMD_DROP:
            {
                EID entity_id = list.msg->GetUInt32();
                csString slot = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got drop cmd: Entity %s to drop from %s\n",
                       ShowID(entity_id), slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_DROP from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)CacheManager::GetSingleton().slotNameHash.GetID( slot );
                if (slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find slot %s.\n", slot.GetData());
                    break;
                }

                psItem * oldItem = chardata->Inventory().GetInventoryItem((INVENTORY_SLOT_NUMBER)slotID);
                if (oldItem)
                {
                    oldItem = chardata->Inventory().RemoveItemID(oldItem->GetUID());
                    chardata->DropItem(oldItem);
                }

                break;
            }

            case psNPCCommandsMessage::CMD_TRANSFER:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                csString item = list.msg->GetStr();
                int count = list.msg->GetInt8();
                csString target = list.msg->GetStr();

                Debug4(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got transfer cmd: Entity %s to transfer %s to %s\n",
                       ShowID(entity_id), item.GetDataSafe(), target.GetDataSafe());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_TRANSFER from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                psItemStats* itemstats = CacheManager::GetSingleton().GetBasicItemStatsByName(item);
                if (!itemstats)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(),  "Invalid item name\n");
                    break;
                }

                size_t slot = chardata->Inventory().FindItemStatIndex(itemstats);
                if (slot == SIZET_NOT_FOUND)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(),  "Item not found\n");
                    break;
                }


                psItem * transferItem = chardata->Inventory().RemoveItemIndex(slot,count);
                if (!transferItem)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(),  "Item could not be removed\n");
                    break;
                }

                count = transferItem->GetStackCount();

                // TODO: Check the target, for now assume tribe. Tribe dosn't held items in server so delete them and notify npcclient
                QueueTransferPerception(gEntity,transferItem,target);
                delete transferItem;

                break;
            }
            case psNPCCommandsMessage::CMD_RESURRECT:
            {
                csVector3 where;
                PID playerID = PID(list.msg->GetUInt32());
                float rot = list.msg->GetFloat();
                where.x = list.msg->GetFloat();
                where.y = list.msg->GetFloat();
                where.z = list.msg->GetFloat();
                csString sector = list.msg->GetStr();

                Debug7(LOG_SUPERCLIENT, playerID.Unbox(), "-->Got resurrect cmd: %s Rot: %.2f Where: (%.2f,%.2f,%.2f) Sector: %s\n",
                       ShowID(playerID), rot, where.x, where.y, where.z, sector.GetDataSafe());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, playerID.Unbox(), "Received incomplete CMD_RESURRECT from NPC client %u.\n", me->clientnum);
                    break;
                }

                psserver->GetSpawnManager()->Respawn(INSTANCE_ALL,where,rot,sector,playerID);

                break;
            }

            case psNPCCommandsMessage::CMD_SEQUENCE:
            {
                csString name = list.msg->GetStr();
                int cmd = list.msg->GetUInt8();
                int count = list.msg->GetInt32();

                psSequenceMessage msg(0,name,cmd,count);
                psserver->GetEventManager()->Broadcast(msg.msg,NetBase::BC_EVERYONE);

                break;
            }

            case psNPCCommandsMessage::CMD_IMPERVIOUS:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                int impervious = list.msg->GetBool();

                gemNPC *entity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));

                psCharacter* chardata = NULL;
                if (entity) chardata = entity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                if (impervious)
                {
                    chardata->SetImperviousToAttack(chardata->GetImperviousToAttack() | TEMPORARILY_IMPERVIOUS);
                }
                else
                {
                    chardata->SetImperviousToAttack(chardata->GetImperviousToAttack() & ~TEMPORARILY_IMPERVIOUS);
                }

                break;
            }

        }
        times[cmd] += csGetTicks() - cmdBegin;
        cmd = list.msg->GetInt8();
    }
    begin = csGetTicks() - begin;
    if(begin > 500)
    {
        int total = 0;
        for(int i = 0;i < 24;i++)
            total += count[i];

        csString status;
        status.Format("NPCManager::HandleCommandList() took %d time. %d commands. Counts: ", begin, total);

        for(int i = 0;i < 24;i++)
            if(times[i] > 100)
                status.AppendFmt("%d: %d# x%d", i, count[i], times[i]);

        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if (list.msg->overrun)
    {
        Debug2(LOG_NET,me->clientnum,"Received unterminated or unparsable psNPCCommandsMessage from client %u.\n",me->clientnum);
    }
}

void NPCManager::AddEntity(gemObject *obj)
{
    obj->Send(0,false,true);
}


void NPCManager::RemoveEntity(MsgEntry *me)
{
    psserver->GetEventManager()->Multicast(me, superclients, 0, PROX_LIST_ANY_RANGE);
}

void NPCManager::UpdateWorldPositions()
{
    if (superclients.GetSize())
    {
        GEMSupervisor::GetSingleton().UpdateAllDR();

        psAllEntityPosMessage msg;
        GEMSupervisor::GetSingleton().GetAllEntityPos(msg);

        msg.Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
    }
}

bool NPCManager::CanPetHearYou(int clientnum, Client *owner, gemNPC *pet, const char *type)
{
    //TODO: Add a range check

    MathEnvironment env;
    env.Define("Skill", owner->GetCharacterData()->GetSkillRank(PSSKILL_EMPATHY).Current());
    petRangeScript->Evaluate(&env);
    MathVar *varMaxRange = env.Lookup("MaxRange");
    float max_range = varMaxRange->GetValue();

    if (DoLogDebug(LOG_NPC))
    {
        CPrintf(CON_DEBUG, "Variables for CalculateMaxPetRange:\n");
        env.DumpAllVars();
    }

    if (pet->GetInstance() != owner->GetActor()->GetInstance() ||
        owner->GetActor()->RangeTo(pet, false) >= max_range )
    {
        psserver->SendSystemInfo(clientnum, "Your %s is too far away to hear you", type);
        return false;
    }

    return true;
}

bool NPCManager::WillPetReact(int clientnum, Client * owner, gemNPC * pet, const char * type, int level)
{
    MathEnvironment env;
    env.Define("Skill", owner->GetCharacterData()->GetSkillRank(PSSKILL_EMPATHY).Current());
    env.Define("Level", level);
    petReactScript->Evaluate(&env);
    MathVar *varReact = env.Lookup("React");
    float react = varReact->GetValue();

    if (DoLogDebug(LOG_NPC))
    {
        CPrintf(CON_DEBUG, "Variables for CalculatePetReact:\n");
        env.DumpAllVars();
    }

    if ( react > 0.0)
    {
        return true;
    }

    psserver->SendSystemInfo(clientnum, "Your %s does not react to your command", type);
    return false;
}

void NPCManager::HandlePetCommand(MsgEntry * me,Client *client)
{
    psPETCommandMessage msg( me );
    gemNPC *pet = NULL;
    psCharacter *chardata = NULL;
    csString firstName, lastName;
    csString prevFirstName, prevLastName;
    csString fullName, prevFullName;
    const char * typeStr = "familiar";

    Client* owner = clients->FindAny(me->clientnum);
    if (!owner)
    {
        Error2("invalid client object from psPETCommandMessage from client %u.\n",me->clientnum);
        return;
    }

    PID familiarID = owner->GetCharacterData()->familiar_id;

    if ( !msg.valid )
    {
        Debug2(LOG_NET,me->clientnum,"Could not parse psPETCommandMessage from client %u.\n",me->clientnum);
        return;
    }

    WordArray words( msg.options );

    // Operator did give a name, lets see if we find the named pet
    if ( msg.target.Length() != 0 )
    {
        size_t numPets = owner->GetNumPets();
        for( size_t i = 0; i < numPets; i++ )
        {
            pet = dynamic_cast <gemNPC*>( owner->GetPet( i ) );
            if ( pet && msg.target.CompareNoCase( pet->GetCharacterData()->GetCharName() ) )
            {
                if (i)
                {
                    typeStr = "pet";
                }
                break;
            }
            else
            {
                pet = NULL;
            }
        }

        if ( !pet )
        {
            psserver->SendSystemInfo( me->clientnum, "You do not have a pet named '%s'.", msg.target.Slice(0, msg.target.Length() - 1 ).GetData() );
            return;
        }
    }
    else
    {
        if (!familiarID.IsValid())
        {
            psserver->SendSystemInfo(me->clientnum,"You have no familiar to command.");
            return;
        }
        else
        {
            pet = dynamic_cast <gemNPC*>(owner->GetFamiliar());
        }
    }


    switch ( msg.command )
    {
    case psPETCommandMessage::CMD_FOLLOW :
        if ( pet != NULL )
        {
            if (CanPetHearYou(me->clientnum, owner, pet, typeStr) && WillPetReact(me->clientnum, owner, pet, typeStr, 1))
            {
                // If no target target owner
                if (!pet->GetTarget())
                {
                    pet->SetTarget( owner->GetActor() );
                }
                QueueOwnerCmdPerception( owner->GetActor(), pet, psPETCommandMessage::CMD_FOLLOW );
                owner->GetCharacterData()->Skills().AddSkillPractice(PSSKILL_EMPATHY, 1);
            }
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "You have no %s to command", typeStr);
            return;
        }
        break;
    case psPETCommandMessage::CMD_STAY :
        if ( pet != NULL )
        {
            if (CanPetHearYou(me->clientnum, owner, pet, typeStr) && WillPetReact(me->clientnum, owner, pet, typeStr, 1))
            {
                QueueOwnerCmdPerception( owner->GetActor(), pet, psPETCommandMessage::CMD_STAY );
                owner->GetCharacterData()->Skills().AddSkillPractice(PSSKILL_EMPATHY, 1);
            }
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "You have no %s to command", typeStr);
            return;
        }
        break;
    case psPETCommandMessage::CMD_DISMISS :

        if ( pet != NULL && pet->IsValid() )
        {
            PetOwnerSession *session = NULL;

            session = OwnerPetList.Get( pet->GetCharacterData()->GetPID(), NULL );
            // Check for an existing session
            if ( !session )
            {
                CPrintf(CON_NOTIFY, "Cannot locate PetSession for owner %s.\n", pet->GetName(), owner->GetName() );
            }
            else
            {
                session->isActive = false;
            }

            psserver->CharacterLoader.SaveCharacterData( pet->GetCharacterData(), pet, true );

            CPrintf(CON_NOTIFY, "NPCManager Removing familiar %s from owner %s.\n", pet->GetName(), owner->GetName() );
            owner->SetFamiliar( NULL );
            EntityManager::GetSingleton().RemoveActor( pet );
        }
        else
        {
            if (owner->GetCharacterData()->familiar_id.IsValid())
            {
                psserver->SendSystemInfo(me->clientnum, "Your pet has already returned to the netherworld.");
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum, "You have no familiar to command.");
            }
            return;
        }


        break;
    case psPETCommandMessage::CMD_SUMMON :

        // Attach to Familiar
        familiarID = owner->GetCharacterData()->familiar_id;

        if ( owner->GetFamiliar() )
        {
            psserver->SendSystemInfo(me->clientnum, "Your familiar has already been summoned.");
            return;
        }

        if (!owner->GetCharacterData()->CanSummonFamiliar())
        {
            psserver->SendSystemInfo(me->clientnum, "You need to equip the item your familiar is bound to.");
            return;
        }

        if (familiarID.IsValid())
        {
            PetOwnerSession *session = NULL;

            session = OwnerPetList.Get( familiarID, NULL );

            psCharacter *petdata = psserver->CharacterLoader.LoadCharacterData( familiarID, false );
            // Check for an existing session
            if ( !session )
            {
                // Create session if one doesn't exist
                session = CreatePetOwnerSession( owner->GetActor(), petdata );
            }

            if ( !session )
            {
                Error2("Error while creating pet session for Character '%s'\n", owner->GetCharacterData()->GetCharName());
                return;
            }

            if ( session->owner != owner->GetActor() )
            {
                session->Reconnect( owner->GetActor() );
            }

            // Check time in game for pet
            if ( session->CheckSession() )
            {
                session->isActive = true; // re-enable time tracking on pet.

                iSector * targetSector;
                csVector3 targetPoint;
                float yRot = 0.0;
                InstanceID instance;
                owner->GetActor()->GetPosition(targetPoint,yRot,targetSector);
                instance = owner->GetActor()->GetInstance();
                psSectorInfo* sectorInfo = CacheManager::GetSingleton().GetSectorInfoByName(targetSector->QueryObject()->GetName());
                petdata->SetLocationInWorld(instance,sectorInfo,targetPoint.x,targetPoint.y,targetPoint.z,yRot);

                EntityManager::GetSingleton().CreateNPC( petdata );
                pet = GEMSupervisor::GetSingleton().FindNPCEntity( familiarID );
                if (pet == NULL)
                {
                    Error2("Error while creating Familiar NPC for Character '%s'\n", owner->GetCharacterData()->GetCharName());
                    psserver->SendSystemError( owner->GetClientNum(), "Could not find Familiar GEM and set its location.");
                    return; // If all we didn't do was load the familiar
                }
                if (!pet->IsValid())
                {
                    Error2("No valid Familiar NPC for Character '%s'\n", owner->GetCharacterData()->GetCharName());
                    psserver->SendSystemError( owner->GetClientNum(), "Could not find valid Familiar");
                    return; // If all we didn't do was load the familiar
                }

                owner->SetFamiliar( pet );
                // Send OwnerActionLogon Perception
                pet->SetOwner( owner->GetActor() );
                owner->GetCharacterData()->Skills().AddSkillPractice(PSSKILL_EMPATHY, 1);
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum,"The power of the ring of familiar is currently depleted, it will take more time to summon a pet again.");
            }
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum,"You have no familiar to command.");
            return;
        }

        break;
    case psPETCommandMessage::CMD_ATTACK :
        if ( pet != NULL )
        {
            if (CanPetHearYou(me->clientnum, owner, pet, typeStr) && WillPetReact(me->clientnum, owner, pet, typeStr, 4))
            {
                gemActor *lastAttacker = NULL;
                gemObject * trg = pet->GetTarget();
                if ( trg != NULL )
                {
                    gemActor * targetActor = dynamic_cast<gemActor*>(trg);
                    /* We check if the owner can attack the other entity in order to not allow players
                     * to override permissions and at the same time allowing pet<->player, pet<->pet
                     * when in pvp. We allow gm to do anything they want (can attack everything including
                     * their own pet just like how it happens with players
                     */
                    if( targetActor == NULL || !owner->IsAllowedToAttack(trg, false) ||
                      ( trg == pet && !owner->IsGM()) )

                    {
                        psserver->SendSystemInfo(me->clientnum,"Your familiar refuses.");
                    }
                    else if (targetActor && !targetActor->CanBeAttackedBy(pet,&lastAttacker))
                    {
                        csString tmp;
                        if (lastAttacker)
                        {
                            tmp.Format("You must be grouped with %s for your pet to attack %s.",
                                       lastAttacker->GetName(), trg->GetName());
                        }
                        else
                        {
                            tmp.Format("Your pet are not allowed to attack right now.");
                        }
                        psserver->SendSystemInfo(me->clientnum,tmp.GetDataSafe());
                    }
                    else
                    {
                        Stance stance = CombatManager::GetStance("Aggressive");
                        if ( words.GetCount() != 0 )
                        {
                            stance.stance_id = words.GetInt( 0 );
                        }
                        QueueOwnerCmdPerception( owner->GetActor(), pet, psPETCommandMessage::CMD_ATTACK );
                        owner->GetCharacterData()->Skills().AddSkillPractice(PSSKILL_EMPATHY, 1);
                    }
                }
                else
                {
                    psserver->SendSystemInfo(me->clientnum, "Your %s needs a target to attack.", typeStr);
                }
            }
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "You have no %s to command", typeStr);
            return;
        }
        break;
    case psPETCommandMessage::CMD_STOPATTACK :
        if ( pet != NULL )
        {
            if (CanPetHearYou(me->clientnum, owner, pet, typeStr) && WillPetReact(me->clientnum, owner, pet, typeStr, 4))
            {
                QueueOwnerCmdPerception( owner->GetActor(), pet, psPETCommandMessage::CMD_STOPATTACK );
                owner->GetCharacterData()->Skills().AddSkillPractice(PSSKILL_EMPATHY, 1);
            }
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "You have no %s to command", typeStr);
            return;
        }
        break;
    case psPETCommandMessage::CMD_ASSIST :
        if ( pet != NULL )
        {
            if (CanPetHearYou(me->clientnum, owner, pet, typeStr) && WillPetReact(me->clientnum, owner, pet, typeStr, 3) )
            {
                QueueOwnerCmdPerception( owner->GetActor(), pet, psPETCommandMessage::CMD_ASSIST );
            }
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "You have no %s to command",typeStr);
            return;
        }
        break;
    case psPETCommandMessage::CMD_GUARD :
        if ( pet != NULL )
        {
            if (CanPetHearYou(me->clientnum, owner, pet, typeStr) && WillPetReact(me->clientnum, owner, pet, typeStr, 2))
            {
                QueueOwnerCmdPerception( owner->GetActor(), pet, psPETCommandMessage::CMD_GUARD );
            }
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "You have no %s to command",typeStr);
            return;
        }
        break;
    case psPETCommandMessage::CMD_NAME :
        if ( pet != NULL )
        {

            if (!CanPetHearYou(me->clientnum, owner, pet, typeStr))
            {
                return;
            }

            if ( words.GetCount() == 0 )
            {
                psserver->SendSystemInfo( me->clientnum, "You must specify a new name for your pet." );
                return;
            }

            firstName = words.Get( 0 );
            if (firstName.Length() > MAX_PLAYER_NAME_LENGTH)
            {
                psserver->SendSystemError( me->clientnum, "First name is too long!");
                return;
            }
            firstName = NormalizeCharacterName( firstName );
            if ( !CharCreationManager::FilterName( firstName ) )
            {
                psserver->SendSystemError( me->clientnum, "The name %s is invalid!", firstName.GetData() );
                return;
            }
            if ( words.GetCount() > 1 )
            {
                lastName = words.GetTail( 1 );
                if (lastName.Length() > MAX_PLAYER_NAME_LENGTH)
                {
                    psserver->SendSystemError( me->clientnum, "Last name is too long!");
                    return;
                }

                lastName = NormalizeCharacterName( lastName );
                if ( !CharCreationManager::FilterName( lastName ) )
                {
                    psserver->SendSystemError( me->clientnum, "The last name %s is invalid!", lastName.GetData() );
                    return;
                }
            }
            else
            {
                lastName.Clear();
            }

            if (psserver->GetCharManager()->IsBanned(firstName))
            {
                psserver->SendSystemError( me->clientnum, "The name %s is invalid!", firstName.GetData() );
                return;
            }

            if (psserver->GetCharManager()->IsBanned(lastName))
            {
                psserver->SendSystemError( me->clientnum, "The last name %s is invalid!", lastName.GetData() );
                return;
            }

            chardata = pet->GetCharacterData();
            prevFirstName = chardata->GetCharName();
            prevLastName = chardata->GetCharLastName();
            if ( firstName == prevFirstName && lastName == prevLastName )
            {
                // no changes needed
                psserver->SendSystemError( me->clientnum, "Your %s is already known with that name!", typeStr );
                return;
            }

            if (firstName != prevFirstName && !CharCreationManager::IsUnique( firstName ))
            {
                psserver->SendSystemError( me->clientnum, "The name %s is not unique!",
                                           firstName.GetDataSafe() );
                return;
            }

            prevFullName = chardata->GetCharFullName();
            chardata->SetFullName( firstName, lastName );
            fullName = chardata->GetCharFullName();
            pet->SetName(fullName);

            psServer::CharacterLoader.SaveCharacterData( chardata, pet, true );

            if ( owner->GetFamiliar() )
            {
                psUpdateObjectNameMessage newNameMsg(0,pet->GetEID(),pet->GetCharacterData()->GetCharFullName());
                psserver->GetEventManager()->Broadcast(newNameMsg.msg,NetBase::BC_EVERYONE);
            }
            else
            {
                EntityManager::GetSingleton().RemoveActor( pet );
            }

            psserver->SendSystemInfo( me->clientnum,
                                      "Your pet %s is now known as %s",
                                      prevFullName.GetData(),
                                      fullName.GetData());
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum,"You have no %s to command",typeStr);
            return;
        }
        break;
    case psPETCommandMessage::CMD_TARGET :
        if ( pet != NULL )
        {
            if (CanPetHearYou(me->clientnum, owner, pet, typeStr))
            {
                if ( words.GetCount() == 0 )
                {
                    psserver->SendSystemInfo( me->clientnum, "You must specify a name for your pet to target." );
                    return;
                }

                firstName = words.Get( 0 );
                if ( words.GetCount() > 1 )
                {
                    lastName = words.GetTail( 1 );
                }
                gemObject *target = psserver->GetAdminManager()->FindObjectByString(firstName,owner->GetActor());

                firstName = NormalizeCharacterName( firstName );

                if (firstName == "Me")
                {
                    firstName = owner->GetName();
                }
                lastName = NormalizeCharacterName( lastName );

                if ( target )
                {
                    pet->SetTarget( target );
                    psserver->SendSystemInfo( me->clientnum, "%s has successfully targeted %s." , pet->GetName(), target->GetName() );
                }
                else
                {
                    psserver->SendSystemInfo( me->clientnum, "Cannot find '%s' to target.", firstName.GetData() );
                }
            }
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum,"You have no %s to command",typeStr);
            return;
        }
        break;
    }
}


void NPCManager::PrepareMessage()
{
    outbound = new psNPCCommandsMessage(0,15000);
    cmd_count = 0;
}

/**
 * Talking sends the speaker, the target of the speech, and
 * the worst faction score between the two to the superclient.
 */
void NPCManager::QueueTalkPerception(gemActor *speaker,gemNPC *target)
{
    float faction = target->GetRelativeFaction(speaker);
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_TALK);
    outbound->msg->Add(speaker->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add( (int16_t) faction );
    cmd_count++;
    Debug4(LOG_NPC, speaker->GetEID().Unbox(), "Added perception: %s spoke to %s with %1.1f faction standing.\n",
        speaker->GetName(),
        target->GetName(),
        faction);
}

/**
 * The initial attack perception sends group info to the superclient
 * so that the npc can populate his hate list.  Then future damage
 * perceptions can influence this hate list to help the mob decide
 * who to attack back.  The superclient can also use this perception
 * to "cheat" and not wait for first damage to attack back--although
 * we are currently not using it that way.
 */
void NPCManager::QueueAttackPerception(gemActor *attacker,gemNPC *target)
{
    if (attacker->InGroup())
    {
        outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_GROUPATTACK);
        outbound->msg->Add(target->GetEID().Unbox());
        csRef<PlayerGroup> g = attacker->GetGroup();
        outbound->msg->Add( (int8_t) g->GetMemberCount() );
        for (int i=0; i<(int)g->GetMemberCount(); i++)
        {
            outbound->msg->Add(g->GetMember(i)->GetEID().Unbox());
            outbound->msg->Add( (int8_t) g->GetMember(i)->GetCharacterData()->Skills().GetBestSkillSlot(true));
        }

        cmd_count++;
        Debug3(LOG_NPC, attacker->GetEID().Unbox(), "Added perception: %s's group is attacking %s.\n",
                attacker->GetName(),
                target->GetName() );
    }
    else // lone gunman
    {
        outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_ATTACK);
        outbound->msg->Add(target->GetEID().Unbox());
        outbound->msg->Add(attacker->GetEID().Unbox());
        cmd_count++;
        Debug3(LOG_NPC, attacker->GetEID().Unbox(), "Added perception: %s is attacking %s.\n",
                attacker->GetName(),
                target->GetName() );
    }
}

/**
 * Each instance of damage to an NPC is sent here.  The NPC is expected
 * to use this information on his hate list.
 */
void NPCManager::QueueDamagePerception(gemActor *attacker,gemNPC *target,float dmg)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_DMG);
    outbound->msg->Add(attacker->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add( (float) dmg );
    cmd_count++;
    Debug4(LOG_NPC, attacker->GetEID().Unbox(), "Added perception: %s hit %s for %1.1f dmg.\n",
        attacker->GetName(),
        target->GetName(),
        dmg);
}

void NPCManager::QueueDeathPerception(gemObject *who)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_DEATH);
    outbound->msg->Add(who->GetEID().Unbox());
    cmd_count++;
    Debug2(LOG_NPC, who->GetEID().Unbox(), "Added perception: %s death.\n", who->GetName());
}

void NPCManager::QueueSpellPerception(gemActor *caster, gemObject *target,const char *spell_cat_name,
                                      uint32_t spell_category, float severity)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_SPELL);
    outbound->msg->Add(caster->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add( (uint32_t) spell_category );
    outbound->msg->Add( (int8_t) (severity * 10) );
    cmd_count++;
    Debug4(LOG_NPC, caster->GetEID().Unbox(), "Added perception: %s cast a %s spell on %s.\n", caster->GetName(), spell_cat_name, target->GetName() );
}

void NPCManager::QueueEnemyPerception(psNPCCommandsMessage::PerceptionType type,
                                      gemActor *npc, gemActor *player,
                                      float relative_faction)
{
    outbound->msg->Add( (int8_t) type);
    outbound->msg->Add(npc->GetEID().Unbox());   // Only entity IDs are passed to npcclient
    outbound->msg->Add(player->GetEID().Unbox());
    outbound->msg->Add( (float) relative_faction);
    cmd_count++;
    Debug5(LOG_NPC, player->GetEID().Unbox(), "Added perception: Entity %s within range of entity %s, type %d, faction %.0f.\n", ShowID(player->GetEID()), ShowID(npc->GetEID()), type, relative_faction);

    gemNPC *myNPC = dynamic_cast<gemNPC *>(npc);
    if (!myNPC)
        return;  // Illegal to not pass actual npc object to this function

    myNPC->ReactToPlayerApproach(type, player);
}

/**
 * The client /pet stay command cause the OwnerCmd perception to be sent to
 * the superclient.
 */
void NPCManager::QueueOwnerCmdPerception(gemActor *owner, gemNPC *pet, psPETCommandMessage::PetCommand_t command)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_OWNER_CMD );
    outbound->msg->Add( (uint32_t) command );
    outbound->msg->Add(owner->GetEID().Unbox());
    outbound->msg->Add(pet->GetEID().Unbox());
    outbound->msg->Add((uint32_t) (pet->GetTarget() ? pet->GetTarget()->GetEID().Unbox() : 0));
    cmd_count++;
    Debug4(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s has told %s to %d.\n",
        owner->GetName(),
        pet->GetName(), (int)command);
}

void NPCManager::QueueInventoryPerception(gemActor *owner, psItem * itemdata, bool inserted)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_INVENTORY );
    outbound->msg->Add(owner->GetEID().Unbox());
    outbound->msg->Add( (char*) itemdata->GetName() );
    outbound->msg->Add( (bool) inserted );
    outbound->msg->Add( (int16_t) itemdata->GetStackCount() );
    cmd_count++;
    Debug7(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s(%s) has %s %d %s %s inventory.\n",
           owner->GetName(),
           ShowID(owner->GetEID()),
           (inserted?"added":"removed"),
           itemdata->GetStackCount(),
           itemdata->GetName(),
           (inserted?"to":"from") );
}

void NPCManager::QueueFlagPerception(gemActor *owner)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_FLAG );
    outbound->msg->Add(owner->GetEID().Unbox());

    uint32_t flags = 0;

    if (!owner->GetVisibility())   flags |= psNPCCommandsMessage::INVISIBLE;
    if (owner->GetInvincibility()) flags |= psNPCCommandsMessage::INVINCIBLE;

    outbound->msg->Add( flags );
    cmd_count++;
    Debug4(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s(%s) flags 0x%X.\n",
           owner->GetName(),
           ShowID(owner->GetEID()),
           flags);

}

void NPCManager::QueueNPCCmdPerception(gemActor *owner, const csString& cmd)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_NPCCMD );
    outbound->msg->Add(owner->GetEID().Unbox());
    outbound->msg->Add( cmd );

    cmd_count++;
    Debug4(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s(%s) npc cmd %s.\n",
           owner->GetName(),
           ShowID(owner->GetEID()),
           cmd.GetData());

}

void NPCManager::QueueTransferPerception(gemActor *owner, psItem * itemdata, csString target)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_TRANSFER );
    outbound->msg->Add(owner->GetEID().Unbox());
    outbound->msg->Add( (char*) itemdata->GetName() );
    outbound->msg->Add( (int8_t) itemdata->GetStackCount() );
    outbound->msg->Add( (char*) target.GetDataSafe() );
    cmd_count++;
    Debug6(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s(%s) has transfered %d %s to %s.\n",
           owner->GetName(),
           ShowID(owner->GetEID()),
           itemdata->GetStackCount(),
           itemdata->GetName(),
           target.GetDataSafe() );
}

void NPCManager::SendAllCommands()
{
    if (cmd_count)
    {
        outbound->msg->Add( (int8_t) psNPCCommandsMessage::CMD_TERMINATOR);
        if (outbound->msg->overrun)
        {
            Bug1("Failed to terminate and send psNPCCommandsMessage.  Would have overrun buffer.\n");
        }
        else
        {
            outbound->msg->ClipToCurrentSize();

            // CPrintf(CON_DEBUG, "Sending %d bytes to superclients...\n",outbound->msg->data->GetTotalSize() );

            outbound->Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
        }
        delete outbound;
        outbound=NULL;
        PrepareMessage();
    }

    psNPCManagerTick *tick = new psNPCManagerTick(NPC_TICK_INTERVAL,this);
    eventmanager->Push(tick);
}

void NPCManager::NewNPCNotify(PID player_id, PID master_id, PID owner_id)
{
    Debug4(LOG_NPC, 0, "New NPC(%s) with master(%s) and owner(%s) sent to superclients.\n",
           ShowID(player_id), ShowID(master_id), ShowID(owner_id));

    psNewNPCCreatedMessage msg(0, player_id, master_id, owner_id);
    msg.Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
}

void NPCManager::ControlNPC( gemNPC* npc )
{
    Client* superclient = clients->FindAccount( npc->GetSuperclientID() );
    if(superclient)
    {
        npc->Send(superclient->GetClientNum(), false, true );
        npc->GetCharacterData()->SetImperviousToAttack(npc->GetCharacterData()->GetImperviousToAttack() & ~TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later
    }
    else
    {
        npc->GetCharacterData()->SetImperviousToAttack(npc->GetCharacterData()->GetImperviousToAttack() | TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later
    }
}

PetOwnerSession *NPCManager::CreatePetOwnerSession( gemActor *owner, psCharacter* petData )
{
    if ( owner && petData )
    {
        PetOwnerSession *session = new PetOwnerSession( this, owner, petData );
        if ( session )
        {
            OwnerPetList.Put( session->petID, session );
            return session;
        }
    }
    return NULL;
}


void NPCManager::RemovePetOwnerSession( PetOwnerSession *session)
{
    if ( session )
    {
        OwnerPetList.DeleteAll( session->petID );
        delete session;
    }
}

void NPCManager::UpdatePetTime()
{
    PetOwnerSession *po;

    // Loop through all Sessions
    csHash< PetOwnerSession*, PID >::GlobalIterator loop( OwnerPetList.GetIterator() );
    while(loop.HasNext())      // Increase Pet Time in Game by NPC_TICK_INTERVAL
    {
    	po = loop.Next();
        if ( po->isActive )
        {
            po->UpdateElapsedTime( NPC_TICK_INTERVAL );
        }
    }
}

/*------------------------------------------------------------------*/

psNPCManagerTick::psNPCManagerTick(int offsetticks, NPCManager *c)
: psGameEvent(0,offsetticks,"psNPCManagerTick")
{
    npcmgr = c;
}

void psNPCManagerTick::Trigger()
{
    static int counter=0;

    if (!(counter%3))
        npcmgr->UpdateWorldPositions();

    npcmgr->SendAllCommands();
    npcmgr->UpdatePetTime();
    counter++;

}

/*-----------------------------------------------------------------*/
/* Pet Skills Handler                                              */
/*-----------------------------------------------------------------*/
void NPCManager::HandlePetSkill(MsgEntry * me,Client *client)
{
    psPetSkillMessage msg( me );
    if ( !msg.valid )
    {
        Debug2( LOG_NET, me->clientnum, "Received unparsable psPetSkillMessage from client %u.\n", me->clientnum );
        return;
    }
    // Client* client = clients->FindAny( me->clientnum );

    if ( !client->GetFamiliar() )
        return;

    //    CPrintf(CON_DEBUG, "ProgressionManager::HandleSkill(%d,%s)\n",msg.command, (const char*)msg.commandData);
    switch ( msg.command )
    {
        case psPetSkillMessage::REQUEST:
        {
            // Send all petStats to seed client
            psCharacter *chr = client->GetFamiliar()->GetCharacterData();
            chr->SendStatDRMessage(me->clientnum, client->GetFamiliar()->GetEID(), DIRTY_VITAL_ALL);

            SendPetSkillList( client );
            break;
        }
        case psPetSkillMessage::SKILL_SELECTED:
        {
            csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

            CS_ASSERT( xml );

            csRef<iDocument> invList  = xml->CreateDocument();

            const char* error = invList->Parse( msg.commandData );
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

            csString skillName = topNode->GetAttributeValue( "NAME" );

            psSkillInfo *info = CacheManager::GetSingleton().GetSkillByName( skillName );

            csString buff;
            if (info)
            {
                csString escpxml = EscpXML( info->description );
                buff.Format( "<DESCRIPTION DESC=\"%s\" CAT=\"%d\"/>", escpxml.GetData(), info->category);
            }
            else
            {
                buff.Format( "<DESCRIPTION DESC=\"\" CAT=\"%d\"/>", info->category );
            }

            psCharacter* chr = client->GetFamiliar()->GetCharacterData();
            psPetSkillMessage newmsg(client->GetClientNum(),
                            psPetSkillMessage::DESCRIPTION,
                            buff,
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
                            PSSKILL_NONE);

            if (newmsg.valid)
                eventmanager->SendMessage(newmsg.msg);
            else
            {
                Bug2("Could not create valid psPetSkillMessage for client %u.\n",client->GetClientNum());
            }
            break;
        }

        case psPetSkillMessage::QUIT:
        {
            //client->GetCharacterData()->SetTrainer(NULL);
            break;
        }
    }
}

void NPCManager::SendPetSkillList(Client * client, bool forceOpen, PSSKILL focus )
{
    psString buff;
    psCharacter * character = client->GetFamiliar()->GetCharacterData();
    psCharacter * trainer = character->GetTrainer();
    psTrainerInfo * trainerInfo = NULL;
    float faction = 0.0;

    buff.Format("<L X=\"%u\" >", character->GetProgressionPoints());

    if (trainer)
    {
        trainerInfo = trainer->GetTrainerInfo();
        faction = trainer->GetActor()->GetRelativeFaction(character->GetActor());
    }

    int realID = -1;
    bool found = false;

    for (int skillID = 0; skillID < (int)PSSKILL_COUNT; skillID++)
    {
        psSkillInfo * info = CacheManager::GetSingleton().GetSkillByID(skillID);
        if (!info)
        {
            Error2("Can't find skill %d",skillID);
            return;
        }

        Skill & charSkill = character->Skills().Get((PSSKILL) skillID);

        csString escpxml = EscpXML(info->name);

            buff.AppendFmt("<SKILL NAME=\"%s\" R=\"%i\" Y=\"%i\" YC=\"%i\" Z=\"%i\" ZC=\"%i\" CAT=\"%d\"/>",
                              escpxml.GetData(), charSkill.rank.Base(),
                              charSkill.y, charSkill.yCost, charSkill.z, charSkill.zCost, info->category);
    }
    buff.Append("</L>");

    psCharacter* chr = client->GetFamiliar()->GetCharacterData();
    psPetSkillMessage newmsg(client->GetClientNum(),
                            psPetSkillMessage::SKILL_LIST,
                            buff,
                            (unsigned int)chr->Stats()[PSITEMSTATS_STAT_STRENGTH].Current(),
                            (unsigned int)chr->Stats()[PSITEMSTATS_STAT_ENDURANCE].Current(),
                            (unsigned int)chr->Stats()[PSITEMSTATS_STAT_AGILITY].Current(),
                            (unsigned int)chr->Stats()[PSITEMSTATS_STAT_INTELLIGENCE].Current(),
                            (unsigned int)chr->Stats()[PSITEMSTATS_STAT_WILL].Current(),
                            (unsigned int)chr->Stats()[PSITEMSTATS_STAT_CHARISMA].Current(),
                            (unsigned int)chr->GetHP(),
                            (unsigned int)chr->GetMana(),
                            (unsigned int)chr->GetStamina(true),
                            (unsigned int)chr->GetStamina(false),
                            (unsigned int)chr->GetMaxHP().Current(),
                            (unsigned int)chr->GetMaxMana().Current(),
                            (unsigned int)chr->GetMaxPStamina().Current(),
                            (unsigned int)chr->GetMaxMStamina().Current(),
                            forceOpen,
                            found?realID:-1
                            );

    CPrintf(CON_DEBUG, "Sending psPetSkillMessage w/ stats to %d, Valid: ",int(client->GetClientNum()));
    if (newmsg.valid)
    {
        CPrintf(CON_DEBUG, "Yes\n");
        eventmanager->SendMessage(newmsg.msg);

    }
    else
    {
        CPrintf(CON_DEBUG, "No\n");
        Bug2("Could not create valid psPetSkillMessage for client %u.\n",client->GetClientNum());
    }
}
