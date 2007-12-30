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



class psNPCManagerTick : public psGameEvent
{
protected:
    NPCManager *npcmgr;

public:
    psNPCManagerTick(int offsetticks, NPCManager *c);
    virtual void Trigger();  // Abstract event processing function
};

#define NPC_TICK_INTERVAL 500  //msec

/**
 * This class is the relationship of Owner to Pet ( which includes Familiars ).
 */
class PetOwnerSession : public iDeleteObjectCallback
{

public:
    int ownerID; // Character ID of the owner
    unsigned int petID; // Character ID of the pet
    bool isActive;
    gemActor *owner;
    NPCManager *manager;
    // create time
    double elapsedTime;
    csString curDate;

    PetOwnerSession() 
    { 
        ownerID = 0;
        petID = 0;
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
            this->ownerID = owner->GetCharacterData()->GetCharacterID();
            this->owner = owner;
            this->owner->RegisterCallback( this );
        }
        
        if ( pet )
        {
            this->petID = pet->GetCharacterID();
        }
        
        if ( owner && pet )
        {
            CPrintf(CON_DEBUG,"Created PetSession ( %s, %d )\n", owner->GetName(), pet->GetCharacterID() );
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
                
                csString curTime ;
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
                curDate = "";
            }
        }

    };

    virtual ~PetOwnerSession() 
    {
        if ( owner ) 
            owner->UnregisterCallback( this );
    };
    
    // Renable Time tracking for returning players
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
                if ( pet && pet->GetCharacterData() && pet->GetCharacterData()->GetCharacterID() == petID )
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

    // used to verify the session should still be valid
    bool CheckSession()
    {
        double maxTime = GetMaxPetTime();
        csString thisDate;

        time_t curr=time(0);
        tm* gmtm = gmtime(&curr);

        thisDate.Format("%d-%02d-%02d",
                        gmtm->tm_year+1900,
                        gmtm->tm_mon+1,
                        gmtm->tm_mday);

        if ( !curDate.Compare( thisDate ) )
        {
            curDate = thisDate;
            elapsedTime = 0;
        }

        if ( elapsedTime >= maxTime && owner->GetClient()->GetSecurityLevel() < 29 )
        {
            CPrintf(CON_NOTIFY,"PetSession marked invalid ( %s, %d )\n", owner->GetName(), petID );

            this->isActive = false;
            return false;
        }

        return true;
    }

    // Uses a MathScript to calculate the maximum amount of time a Pet can remain in world.
    double GetMaxPetTime()
    {
        static MathScript *maxPetTime;
        double maxTime = 60 * 5 * 1000;

        if (!maxPetTime)
        {
            // Max Mana Script isn't loaded, so load it
            maxPetTime = psserver->GetMathScriptEngine()->FindScript("CalculateMaxPetTime");
            //CS_ASSERT(maxPetTime != NULL);
        }
        
        if ( maxPetTime )
        {
            MathScriptVar* actorvar  = maxPetTime->GetOrCreateVar("Actor");
            if ( owner )
            {
                actorvar->SetObject( this->owner->GetCharacterData() );
            
                maxPetTime->Execute();
                MathScriptVar* timeValue =  maxPetTime->GetVar("MaxTime");
                maxTime = timeValue->GetValue();
            }
        }

        return maxTime;
    }

    // The == and < operators for BinaryTree<> MUST be the same criteria!
    int operator==(PetOwnerSession& other) const
    {
        // use the familiar as the familiar can only have one owner
        return ( petID == other.petID );
    };

    int operator<(PetOwnerSession& other) const
    {
        // use the familiar as the familiar can only have one owner
        return ( petID < other.petID );
    };

};

NPCManager::NPCManager(ClientConnectionSet *pCCS,
                       psDatabase *db,
                       EventManager *evtmgr)
{
    clients      = pCCS;
    database     = db;
    eventmanager = evtmgr;

    psserver->GetEventManager()->Subscribe(this,MSGTYPE_NPCAUTHENT,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_NPCOMMANDLIST,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_NPC_COMMAND,REQUIRE_ANY_CLIENT);
    
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_DAMAGE_EVENT,NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_DEATH_EVENT,NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_PET_COMMAND,REQUIRE_ANY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_PET_SKILL,REQUIRE_ANY_CLIENT);

    PrepareMessage();

    psNPCManagerTick *tick = new psNPCManagerTick(NPC_TICK_INTERVAL,this);
    eventmanager->Push(tick);
}

NPCManager::~NPCManager()
{
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_NPCAUTHENT);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_NPCOMMANDLIST);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_NPC_COMMAND);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DAMAGE_EVENT);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DEATH_EVENT);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_PET_COMMAND);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_PET_SKILL);
    delete outbound;
}

void NPCManager::HandleMessage(MsgEntry *me,Client *client)
{
    switch ( me->GetType() )
    {
        case MSGTYPE_DAMAGE_EVENT:
        {
            HandleDamageEvent(me);
            break;
        }
        case MSGTYPE_DEATH_EVENT:
        {
            HandleDeathEvent(me);
            break;
        }
        case MSGTYPE_NPCAUTHENT:
        {
            HandleAuthentRequest(me);
            break;
        }
        case MSGTYPE_NPCOMMANDLIST:
        {
            HandleCommandList(me);
            break;
        }
        case MSGTYPE_PET_COMMAND:
        {
            HandlePetCommand(me);
            break;
        }
        case MSGTYPE_PET_SKILL:
        {
            HandlePetSkill(me);
            break;
        }
        case MSGTYPE_NPC_COMMAND:
        {
            HandleConsoleCommand(me);
            break;
        }
    }
}    

void NPCManager::HandleDamageEvent(MsgEntry *me)
{
    psDamageEvent evt(me);

    // NPC's need to know they were hit using a Perception
    if (evt.attacker!=NULL  &&  evt.target->GetNPCPtr()) // if npc damaged
        QueueDamagePerception(evt.attacker,evt.target->GetNPCPtr(),evt.damage);
}

void NPCManager::HandleDeathEvent(MsgEntry *me)
{
    Debug1(LOG_SUPERCLIENT, 0,"NPCManager handling Death Event\n");
    psDeathEvent evt(me);

    QueueDeathPerception(evt.deadActor);
}


void NPCManager::HandleConsoleCommand(MsgEntry *me)
{
    csString buffer;

    psServerCommandMessage msg(me);
    printf("Got command: %s\n", msg.command.GetDataSafe() );
    
    size_t i = msg.command.FindFirst(' ');
    csString word;
    msg.command.SubString(word,0,i);
    COMMAND *cmd = find_command(word);

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
    printf(buffer);
    printf("\n");

    psServerCommandMessage retn(me->clientnum, buffer);
    retn.SendMessage();
}


void NPCManager::HandleAuthentRequest(MsgEntry *me)
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
        //psserver->RemovePlayer (me->clientnum, "You are not running the correct version of Planeshift for this server.");
        Error2("Superclient '%s' is not running the correct version of Planeshift for this server.",
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
        Error1("No username specified.\n");
        return;   
    }
 
    msg.sUser.Downcase();
    msg.sUser.SetAt(0,toupper(msg.sUser.GetAt(0)));

    if (clients->Find(msg.sUser))  // username already logged in
    {
        // invalid
        // psserver->RemovePlayer(me->clientnum,"You are already logged on to this server. If you were disconnected, please wait 30 seconds and try again.");
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Already logged in.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);

        Error2("User '%s' authentication request rejected: User already logged in.\n",
            (const char *)msg.sUser);

        return;
    }

    // CHECK 4: Check to see if the login is correct.

    Error2("Check Superclient Login for: '%s'\n", (const char*)msg.sUser);
    psAccountInfo *acctinfo=CacheManager::GetSingleton().GetAccountInfoByUsername((const char *)msg.sUser);

    csString password = csMD5::Encode(msg.sPassword).HexString();
    
    if (acctinfo==NULL || strcmp(acctinfo->password,(const char *)password)) // authentication error
    {
        if (acctinfo==NULL)
        {
            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Username not found).\n",(const char *)msg.sUser);
        }
        else
            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Bad password).\n",(const char *)msg.sUser);
        delete acctinfo;
        return;
    }

    client->SetPlayerID(0);
    client->SetSecurityLevel(acctinfo->securitylevel);

    if (acctinfo->securitylevel!= 99)
    {
        // psserver->RemovePlayer(me->clientnum, "Login id does not have superclient security rights.");

        Error3("User '%s' authentication request rejected (security level was %d).\n",
            (const char *)msg.sUser, acctinfo->securitylevel );
        delete acctinfo;
        return;
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

    superclients.Push(PublishDestination(me->clientnum, client, 0, 0));

    psserver->GetAuthServer()->SendMsgStrings(me->clientnum);

    SendMapList(client);
    SendRaces(client);

    psserver->GetWeatherManager()->SendClientGameTime(me->clientnum);

    delete acctinfo;

    status.Format("%s, %u, Superclient logged in", (const char*) msg.sUser, me->clientnum);
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

        // NPC Client is now ready
        client->SetReady(true);
    }
    else
    {
        Bug2("Overran message buffer while sending NPC List to client %u.\n",client->GetClientNum());
    }
}

void NPCManager::HandleCommandList(MsgEntry *me)
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
                void *data = list.msg->GetData(&len);

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Error2("Received incomplete CMD_DRDATA from NPC client %u.\n",me->clientnum);
                    break;
                }
                
                psDRMessage drmsg(data,len,
                                  CacheManager::GetSingleton().GetMsgStrings(),
                                  EntityManager::GetSingleton().GetEngine());  // alternate method of cracking
                
                // copy the DR data into an iDataBuffer
                csRef<iDataBuffer> databuf = csPtr<iDataBuffer> (new csDataBuffer (len));
                memcpy(databuf->GetData(), data, len);
                // find the entity and Set the DR data for it
                gemNPC *actor = dynamic_cast<gemNPC*>(GEMSupervisor::GetSingleton().FindObject(drmsg.entityid));

                if (!actor)
                {
                    Error2("Illegal EID: %u from superclient!\n",drmsg.entityid);
                    
                }
                else if (!actor->IsAlive())
                {
                    Debug3(LOG_SUPERCLIENT, actor->GetPlayerID(),"Ignoring DR data for dead npc %s(EID: %u).\n", 
                           actor->GetName(),drmsg.entityid);
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
                        CPrintf(CON_DEBUG, "Received bad DR data from NPC %s(EID: %u PID: %u), killing NPC.\n", 
                                actor->GetName(),drmsg.entityid,actor->GetPlayerID());
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
                PS_ID attacker_id = list.msg->GetUInt32();
                PS_ID target_id = list.msg->GetUInt32();
                Debug3(LOG_SUPERCLIENT,attacker_id,"-->Got attack cmd for entity EID: %u to EID: %u\n",attacker_id, target_id);

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,attacker_id,"Received incomplete CMD_ATTACK from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC *attacker = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(attacker_id));
                if (attacker && attacker->IsAlive())
                {
                    gemObject *target   = (gemObject *)GEMSupervisor::GetSingleton().FindObject(target_id);
                    if (!target)
                    {
                        attacker->SetTarget(target);
                        if (attacker->GetCharacterData()->GetMode() == PSCHARACTER_MODE_COMBAT)
                            psserver->combatmanager->StopAttack(attacker);
                        if(target_id==0)
                        {
                            Debug2(LOG_SUPERCLIENT, attacker_id,"%s has stopped attacking.\n",attacker->GetName() );
                        }
                        else      // entity may have been removed since this msg was queued
                            Debug2(LOG_SUPERCLIENT, attacker_id, "Couldn't find entity %d to attack them.\n",target_id);
                    }
                    else if(attacker->GetTarget() != target)
                    {
                        attacker->SetTarget(target);
                        if (attacker->GetCharacterData()->GetMode() == PSCHARACTER_MODE_COMBAT)
                            psserver->combatmanager->StopAttack(attacker);

                        if ( !target->GetClient() || !target->GetActorPtr()->GetInvincibility() )
                        {
                            // NPCs only use 'Normal' stance for now.
                            psserver->combatmanager->AttackSomeone(attacker,target,attacker->GetCharacterData()->getStance("Normal"));
                            Debug3(LOG_SUPERCLIENT, attacker_id, "%s is now attacking %s.\n",attacker->GetName(),target->GetName() );
                        }
                        else
                        {
                            Debug3(LOG_SUPERCLIENT, attacker_id, "%s cannot attack GM [%s].\n",attacker->GetName(),target->GetName() );
                        }
                    }

                }
                break;
            }
            case psNPCCommandsMessage::CMD_SPAWN:
            {
                PS_ID spawner_id = list.msg->GetUInt32();
                PS_ID spawned_id = list.msg->GetUInt32();
                Debug3(LOG_SUPERCLIENT,spawner_id, "-->Got spawn cmd for entity EID: %u to EID: %u\n",spawner_id, spawned_id);

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
                PS_ID speaker_id = list.msg->GetUInt32();
                const char* text = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT,me->clientnum,"-->Got talk cmd: %s for entity EID: %u\n",text, speaker_id);

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
                PS_ID entity_id = list.msg->GetUInt32();
                bool status = list.msg->GetBool();
                Debug3(LOG_SUPERCLIENT,me->clientnum,"-->Got visibility cmd: %s for entity EID: %u\n",status?"yes":"no", entity_id);

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
                PS_ID entity_id = list.msg->GetUInt32();
                PS_ID item_id = list.msg->GetUInt32();
                int count = list.msg->GetInt16();
                Debug4(LOG_SUPERCLIENT,entity_id,"-->Got pickup cmd: Entity EID: %u to pickup %d of EID: %u\n",entity_id,count,item_id);

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Received incomplete CMD_PICKUP from NPC client %u.\n",me->clientnum);
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
                if (gItem) item = gItem->GetItem();
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
                PS_ID entity_id = list.msg->GetUInt32();
                csString item = list.msg->GetStr();
                csString slot = list.msg->GetStr();
                int count = list.msg->GetInt16();
                Debug6(LOG_SUPERCLIENT,entity_id,"-->Got equip cmd from %u: Entity EID: %u to equip %d %s in %s\n",
                       me->clientnum,entity_id,count,item.GetData(),slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Received incomplete CMD_EQUIP from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id,"Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)CacheManager::GetSingleton().slotNameHash.GetID( slot );
                if (slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Couldn't find slot %s.\n",slot.GetData());
                    break;
                }
                
                // Create the item
                psItemStats* baseStats = CacheManager::GetSingleton().GetBasicItemStatsByName(item);
                if (!baseStats)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Couldn't find base for item %s.\n",item.GetData());
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
                PS_ID entity_id = list.msg->GetUInt32();
                csString slot = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT,entity_id,"-->Got dequip cmd: Entity EID: %u to dequip from %s\n",
                       entity_id,slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Received incomplete CMD_DEQUIP from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id,"Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)CacheManager::GetSingleton().slotNameHash.GetID( slot );
                if (slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Couldn't find slot %s.\n",slot.GetData());
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
                PS_ID entity_id = list.msg->GetUInt32();
                csString resource = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT,entity_id,"-->Got dig cmd: Entity EID: %u to dig for %s\n",
                       entity_id,resource.GetData());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Received incomplete CMD_DIG from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id,"Couldn't find character data.\n");
                    break;
                }

                psserver->GetWorkManager()->HandleProduction(gEntity,"dig",resource);

                break;
            }
            case psNPCCommandsMessage::CMD_DROP:
            {
                PS_ID entity_id = list.msg->GetUInt32();
                csString slot = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT,entity_id,"-->Got drop cmd: Entity EID: %u to drop from %s\n",
                       entity_id,slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Received incomplete CMD_DROP from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id,"Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)CacheManager::GetSingleton().slotNameHash.GetID( slot );
                if (slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Couldn't find slot %s.\n",slot.GetData());
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
                PS_ID entity_id = list.msg->GetUInt32();
                csString item = list.msg->GetStr();
                int count = list.msg->GetInt8();
                csString target = list.msg->GetStr();

                Debug4(LOG_SUPERCLIENT,entity_id,"-->Got transfer cmd: Entity EID: %u to transfer %s to %s\n",
                       entity_id,item.GetDataSafe(),target.GetDataSafe());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,entity_id,"Received incomplete CMD_TRANSFER from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC *gEntity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));
                psCharacter* chardata = NULL;
                if (gEntity) chardata = gEntity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id,"Couldn't find character data.\n");
                    break;
                }

                psItemStats* itemstats = CacheManager::GetSingleton().GetBasicItemStatsByName(item);
                if (!itemstats)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id, "Invalid item name\n");
                    break;
                }

                size_t slot = chardata->Inventory().FindItemStatIndex(itemstats);
                if (slot == SIZET_NOT_FOUND)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id, "Item not found\n");
                    break;
                }
                

                psItem * transferItem = chardata->Inventory().RemoveItemIndex(slot,count);
                if (!transferItem)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id, "Item could not be removed\n");
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
                int playerID = list.msg->GetUInt32();
                float rot = list.msg->GetFloat();
                where.x = list.msg->GetFloat();
                where.y = list.msg->GetFloat();
                where.z = list.msg->GetFloat();
                csString sector = list.msg->GetStr();

                Debug7(LOG_SUPERCLIENT,playerID,"-->Got resurrect cmd: PID: %u Rot: %.2f Where: (%.2f,%.2f,%.2f) Sector: %s\n",
                       playerID,rot,where.x,where.y,where.z,sector.GetDataSafe());

                // Make sure we haven't run past the end of the buffer
                if (list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,playerID,"Received incomplete CMD_RESURRECT from NPC client %u.\n",me->clientnum);
                    break;
                }

                psserver->GetSpawnManager()->Respawn(-1,where,rot,sector,playerID);

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
                PS_ID entity_id = list.msg->GetUInt32();
                int impervious = list.msg->GetBool();
                
                gemNPC *entity = dynamic_cast<gemNPC *> (GEMSupervisor::GetSingleton().FindObject(entity_id));

                psCharacter* chardata = NULL;
                if (entity) chardata = entity->GetCharacterData();
                if (!chardata)
                {
                    Debug1(LOG_SUPERCLIENT,entity_id,"Couldn't find character data.\n");
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

void NPCManager::HandlePetCommand( MsgEntry * me )
{
    psPETCommandMessage msg( me );
    gemNPC *pet = NULL;
    psCharacter *chardata = NULL;
    csString firstName, lastName;
    csString prevFirstName, prevLastName;
    csString fullName, prevFullName;

    int familiarID = 0;
    
    Client* owner = clients->FindAny(me->clientnum);
    if (!owner)
    {
        Error2("invalid client object from psPETCommandMessage from client %u.\n",me->clientnum);
        return;
    }
    
    if ( !msg.valid )
    {
        Debug2(LOG_NET,me->clientnum,"Could not parse psPETCommandMessage from client %u.\n",me->clientnum);
        return;
    }

    WordArray words( msg.options );

    if ( msg.target.Length() != 0 )
    {
        size_t numPets = owner->GetNumPets();
        for( size_t i = 0; i < numPets; i++ )
        {
            pet = dynamic_cast <gemNPC*>( owner->GetPet( i ) );
            if ( pet && msg.target.CompareNoCase( pet->GetCharacterData()->GetCharName() ) )
                break;
            else
                pet = NULL;
        }
    }
    else
    {
        pet = dynamic_cast <gemNPC*>(owner->GetFamiliar());
    }

    if ( ( pet == NULL ) && ( msg.target.Length() != 0 ) )
    {
        psserver->SendSystemInfo( me->clientnum, "You do not have a pet named '%s'.", msg.target.Slice(0, msg.target.Length() - 1 ).GetData() );
        return;
    }

    switch ( msg.command )
    {
    case psPETCommandMessage::CMD_FOLLOW :
        if ( pet != NULL )
        {
            // If no target target owner
            if (!pet->GetTarget())
            {
                pet->SetTarget( owner->GetActor() );
            }
            QueueOwnerCmdFollowPerception( owner->GetActor(), pet );
        }
        break;
    case psPETCommandMessage::CMD_STAY :
        if ( pet != NULL )
        {
            QueueOwnerCmdStayPerception( owner->GetActor(), pet );
        }
        break;
    case psPETCommandMessage::CMD_DISMISS :

        if ( pet != NULL && pet->IsValid() )
        {
            PetOwnerSession key, *session = NULL;
            
            key.ownerID = 0;
            key.petID = pet->GetCharacterData()->GetCharacterID();
            
            session = OwnerPetList.Find( &key );
            // Check for an existing session
            if ( !session )
            {
                CPrintf(CON_NOTIFY,"Cannot locate PetSession for owner %s.\n",pet->GetName(),owner->GetName() );
            }
            else
            {
                session->isActive = false;
            }

            psserver->CharacterLoader.SaveCharacterData( pet->GetCharacterData(), pet, true );

            CPrintf(CON_NOTIFY,"NPCManager Removing familiar %s from owner %s.\n",pet->GetName(),owner->GetName() );
            owner->SetFamiliar( NULL );
            EntityManager::GetSingleton().RemoveActor( pet );
        }
        else
        {
            if ( owner->GetCharacterData()->familiar_id )
            {
                psserver->SendSystemInfo(me->clientnum,"Your pet has already returned to the netherworld.");
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum,"You have no familiar to command.");
            }
            return;
        }
            
            
        break;
    case psPETCommandMessage::CMD_SUMMON :

        // Attach to Familiar
        familiarID = owner->GetCharacterData()->familiar_id;
            
        if ( owner->GetFamiliar() )
        {
            psserver->SendSystemInfo(me->clientnum,"Your familiar has already been summoned.");
            return;
        }

        if ( familiarID > 0 )
        {
            PetOwnerSession key, *session = NULL;
            
            key.ownerID = 0;
            key.petID = familiarID;
            
            session = OwnerPetList.Find( &key );
            
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
                int instance;
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
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum,"You are too weak to call your familiar to you any more today.");
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
            if ( pet->GetTarget() != NULL )
            {
                Stance stance = pet->GetCharacterData()->getStance("Aggressive");
                if ( words.GetCount() != 0 )
                {
                    stance.stance_id = words.GetInt( 0 );
                }
                QueueOwnerCmdAttackPerception( owner->GetActor(), pet );
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum,"Your pet needs a target to attack.");
            }
        }
        break;
    case psPETCommandMessage::CMD_STOPATTACK :
        if ( pet != NULL )
        {
            QueueOwnerCmdStopAttackPerception( owner->GetActor(), pet );
        }
        break;
    case psPETCommandMessage::CMD_ASSIST :
        break;
    case psPETCommandMessage::CMD_GUARD :
        break;
    case psPETCommandMessage::CMD_NAME :
        // apply name change to pet.
        familiarID = owner->GetCharacterData()->familiar_id;
        if ( !familiarID ) 
        {
            psserver->SendSystemInfo(me->clientnum,"You have no familiar to command.");
            return;
        }
            
        if ( owner->GetFamiliar() )
        {
            pet = dynamic_cast <gemNPC*> (owner->GetFamiliar());
        }
        else
        {
            // TODO : Remove this code after fixing exploit
            psserver->SendSystemInfo( me->clientnum, "You must summon your pet to give it a new name." );
            return;

            EntityManager::GetSingleton().CreateNPC( familiarID );
            pet = GEMSupervisor::GetSingleton().FindNPCEntity( familiarID );
        }
        
        if (pet == NULL)
        {
            Error2("Error while finding Familiar NPC for Character '%s'\n", owner->GetCharacterData()->GetCharName());
            psserver->SendSystemError( me->clientnum, "Could not find Familiar GEM.");
            return;
        }
            
        if ( words.GetCount() == 0 )
        {
            psserver->SendSystemInfo( me->clientnum, "You must specify a new name for your pet." );
            return;
        }
            
        firstName = words.Get( 0 );
        firstName = NormalizeCharacterName( firstName );
        if ( !psCharCreationManager::FilterName( firstName ) )   
        {   
            psserver->SendSystemError( me->clientnum, "The name %s is invalid!", firstName.GetData() );   
            return;   
        }   
        if ( words.GetCount() > 1 )
        {
            lastName = words.GetTail( 1 );
            lastName = NormalizeCharacterName( lastName );
            if ( !psCharCreationManager::FilterName( lastName ) )   
            {   
                psserver->SendSystemError( me->clientnum, "The last name %s is invalid!", lastName.GetData() );   
                return;   
            }   
        }
        else
        {
            lastName = "";
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
            return;
        }

        if (!psCharCreationManager::IsUnique( firstName ))
        {
            psserver->SendSystemError( me->clientnum, "The name %s is not unique!", 
                                       firstName.GetDataSafe() );               
            return;
        }

        prevFullName = chardata->GetCharFullName();
        chardata->SetFullName( firstName, lastName );
        fullName = chardata->GetCharFullName();
            
        psServer::CharacterLoader.SaveCharacterData( chardata, pet, true );
            
        if ( owner->GetFamiliar() )
        {
            psUpdateObjectNameMessage newNameMsg(0,pet->GetEntity()->GetID(),pet->GetCharacterData()->GetCharFullName());
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
        break;
    case psPETCommandMessage::CMD_TARGET :
        if ( pet != NULL )
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
            
            firstName = NormalizeCharacterName( firstName );

            if (firstName == "Me")
            {
                firstName = owner->GetName();
            }
            lastName = NormalizeCharacterName( lastName );
            
            PS_ID target_id = psServer::CharacterLoader.FindCharacterID( firstName, false );
            
            if ( target_id )
            {
                gemObject *target = GEMSupervisor::GetSingleton().FindPlayerEntity( target_id );
                if ( target ) 
                {
                    pet->SetTarget( target );
                    psserver->SendSystemInfo( me->clientnum, "%s has successfully targeted %s." , pet->GetName(), firstName.GetData() );
                }
            }
            else
            {
                psserver->SendSystemInfo( me->clientnum, "Cannot find '%s' to target.", firstName.GetData() );
            }
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
    outbound->msg->Add( (uint32_t) speaker->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) target->GetEntity()->GetID() );
    outbound->msg->Add( (int16_t) faction );
    cmd_count++;
    Debug4(LOG_NPC, speaker->GetEntity()->GetID(),"Added perception: %s spoke to %s with %1.1f faction standing.\n",
        speaker->GetEntity()->GetName(),
        target->GetEntity()->GetName(),
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
        outbound->msg->Add( (uint32_t) target->GetEntity()->GetID() );
        csRef<PlayerGroup> g = attacker->GetGroup();
        outbound->msg->Add( (int8_t) g->GetMemberCount() );
        for (int i=0; i<(int)g->GetMemberCount(); i++)
        {
            outbound->msg->Add( (uint32_t) g->GetMember(i)->GetEntity()->GetID() );
            outbound->msg->Add( (int8_t) g->GetMember(i)->GetCharacterData()->GetSkills()->GetBestSkillSlot(true));
        }

        cmd_count++;
        Debug3(LOG_NPC, attacker->GetEntity()->GetID(),"Added perception: %s's group is attacking %s.\n",
                attacker->GetEntity()->GetName(),
                target->GetEntity()->GetName() );
    }
    else // lone gunman
    {
        outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_ATTACK);
        outbound->msg->Add( (uint32_t) target->GetEntity()->GetID() );
        outbound->msg->Add( (uint32_t) attacker->GetEntity()->GetID() );
        cmd_count++;
        Debug3(LOG_NPC, attacker->GetEntity()->GetID(),"Added perception: %s is attacking %s.\n",
                attacker->GetEntity()->GetName(),
                target->GetEntity()->GetName() );
    }
}

/**
 * Each instance of damage to an NPC is sent here.  The NPC is expected
 * to use this information on his hate list.
 */
void NPCManager::QueueDamagePerception(gemActor *attacker,gemNPC *target,float dmg)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_DMG);
    outbound->msg->Add( (uint32_t) attacker->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) target->GetEntity()->GetID() );
    outbound->msg->Add( (float) dmg );
    cmd_count++;
    Debug4(LOG_NPC, attacker->GetEntity()->GetID(),"Added perception: %s hit %s for %1.1f dmg.\n",
        attacker->GetEntity()->GetName(),
        target->GetEntity()->GetName(),
        dmg);
}

void NPCManager::QueueDeathPerception(gemObject *who)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_DEATH);
    outbound->msg->Add( (uint32_t) who->GetEntity()->GetID() );
    cmd_count++;
    Debug2(LOG_NPC, who->GetEntity()->GetID(),"Added perception: %s death.\n",who->GetName());
}

void NPCManager::QueueSpellPerception(gemActor *caster, gemObject *target,const char *spell_cat_name, 
                                      uint32_t spell_category, float severity)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_SPELL);
    outbound->msg->Add( (uint32_t) caster->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) target->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) spell_category );
    outbound->msg->Add( (int8_t) (severity * 10) );
    cmd_count++;
    Debug4(LOG_NPC, caster->GetEntity()->GetID(),"Added perception: %s cast a %s spell on %s.\n",caster->GetName(), spell_cat_name, target->GetName() );
}

void NPCManager::QueueEnemyPerception(psNPCCommandsMessage::PerceptionType type, 
                                      gemActor *npc, gemActor *player, 
                                      float relative_faction)
{
    outbound->msg->Add( (int8_t) type);
    outbound->msg->Add( (uint32_t) npc->GetEntity()->GetID() );   // Only entity IDs are passed to npcclient
    outbound->msg->Add( (uint32_t) player->GetEntity()->GetID() );
    outbound->msg->Add( (float) relative_faction);
    cmd_count++;
    Debug5(LOG_NPC, player->GetEntity()->GetID(),"Added perception: Entity %u within range of entity %u, type %d, faction %.0f.\n",player->GetEntity()->GetID(),npc->GetEntity()->GetID(),type,relative_faction );

    gemNPC *myNPC = dynamic_cast<gemNPC *>(npc);
    if (!myNPC)
        return;  // Illegal to not pass actual npc object to this function

    myNPC->ReactToPlayerApproach(type, player);
}

/**
 * The client /pet stay command cause the OwnerCmdStay perception to be sent to 
 * the superclient.
 */
void NPCManager::QueueOwnerCmdStayPerception(gemActor *owner, gemNPC *pet)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_OWNER_CMD );
    outbound->msg->Add( (uint32_t) psPETCommandMessage::CMD_STAY );
    outbound->msg->Add( (uint32_t) owner->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) pet->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) (pet->GetTarget() ? pet->GetTarget()->GetEntity()->GetID(): 0) );
    cmd_count++;
    Debug3(LOG_NPC, owner->GetEntity()->GetID(),"Added perception: %s has told %s to stay.\n",
        owner->GetEntity()->GetName(),
        pet->GetEntity()->GetName() );
}

/**
 * The client /pet follow command cause the OwnerCmdFollow perception to be sent to 
 * the superclient.
 */
void NPCManager::QueueOwnerCmdFollowPerception(gemActor *owner, gemNPC *pet)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_OWNER_CMD );
    outbound->msg->Add( (uint32_t) psPETCommandMessage::CMD_FOLLOW );
    outbound->msg->Add( (uint32_t) owner->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) pet->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) (pet->GetTarget() ? pet->GetTarget()->GetEntity()->GetID(): 0) );
    cmd_count++;
    Debug3(LOG_NPC, owner->GetEntity()->GetID(), "Added perception: %s has told %s to follow.\n",
        owner->GetEntity()->GetName(),
        pet->GetEntity()->GetName() );
}

/**
 * The client /pet attack command cause the OwnerCmdAttack perception to be sent to 
 * the superclient.
 */
void NPCManager::QueueOwnerCmdAttackPerception(gemActor *owner, gemNPC *pet)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_OWNER_CMD );
    outbound->msg->Add( (uint32_t) psPETCommandMessage::CMD_ATTACK );
    outbound->msg->Add( (uint32_t) owner->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) pet->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) (pet->GetTarget() ? pet->GetTarget()->GetEntity()->GetID(): 0) );
    cmd_count++;
    Debug3(LOG_NPC, owner->GetEntity()->GetID(),"Added perception: %s has told %s to attack.\n",
        owner->GetEntity()->GetName(),
        pet->GetEntity()->GetName() );
}

/**
 * The client /pet follow command cause the OwnerCmdStopAttack perception to be sent to 
 * the superclient.
 */
void NPCManager::QueueOwnerCmdStopAttackPerception(gemActor *owner, gemNPC *pet)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_OWNER_CMD );
    outbound->msg->Add( (uint32_t) psPETCommandMessage::CMD_STOPATTACK );
    outbound->msg->Add( (uint32_t) owner->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) pet->GetEntity()->GetID() );
    outbound->msg->Add( (uint32_t) (pet->GetTarget() ? pet->GetTarget()->GetEntity()->GetID(): 0) );
    cmd_count++;
    Debug3(LOG_NPC, owner->GetEntity()->GetID(),"Added perception: %s has told %s to stop atttacking.\n",
        owner->GetEntity()->GetName(),
        pet->GetEntity()->GetName() );
}

void NPCManager::QueueInventoryPerception(gemActor *owner, psItem * itemdata, bool inserted)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_INVENTORY );
    outbound->msg->Add( (uint32_t) owner->GetEntity()->GetID() );
    outbound->msg->Add( (char*) itemdata->GetName() );
    outbound->msg->Add( (bool) inserted );
    outbound->msg->Add( (int16_t) itemdata->GetStackCount() );
    cmd_count++;
    Debug7(LOG_NPC, owner->GetEntity()->GetID(),"Added perception: %s(EID: %u) has %s %d %s %s inventory.\n",
           owner->GetEntity()->GetName(),
           owner->GetEntity()->GetID(),
           (inserted?"added":"removed"),
           itemdata->GetStackCount(),
           itemdata->GetName(),
           (inserted?"to":"from") );
}

void NPCManager::QueueFlagPerception(gemActor *owner)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_FLAG );
    outbound->msg->Add( (uint32_t) owner->GetEntity()->GetID() );
    
    uint32_t flags = 0;
    
    if (!owner->GetVisibility())   flags |= psNPCCommandsMessage::INVISIBLE;
    if (owner->GetInvincibility()) flags |= psNPCCommandsMessage::INVINCIBLE;

    outbound->msg->Add( flags );
    cmd_count++;
    Debug4(LOG_NPC, owner->GetEntity()->GetID(),"Added perception: %s(EID: %u) flags 0x%X.\n",
           owner->GetEntity()->GetName(),
           owner->GetEntity()->GetID(),
           flags);
    
}

void NPCManager::QueueNPCCmdPerception(gemActor *owner, const csString& cmd)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_NPCCMD );
    outbound->msg->Add( (uint32_t) owner->GetEntity()->GetID() );
    outbound->msg->Add( cmd );

    cmd_count++;
    Debug4(LOG_NPC, owner->GetEntity()->GetID(),"Added perception: %s(EID: %u) npc cmd %s.\n",
           owner->GetEntity()->GetName(),
           owner->GetEntity()->GetID(),
           cmd.GetData());
    
}

void NPCManager::QueueTransferPerception(gemActor *owner, psItem * itemdata, csString target)
{
    outbound->msg->Add( (int8_t) psNPCCommandsMessage::PCPT_TRANSFER );
    outbound->msg->Add( (uint32_t) owner->GetEntity()->GetID() );
    outbound->msg->Add( (char*) itemdata->GetName() );
    outbound->msg->Add( (int8_t) itemdata->GetStackCount() );
    outbound->msg->Add( (char*) target.GetDataSafe() );
    cmd_count++;
    Debug6(LOG_NPC, owner->GetEntity()->GetID(),"Added perception: %s(EID: %u) has transfered %d %s to %s.\n",
           owner->GetEntity()->GetName(),
           owner->GetEntity()->GetID(),
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

void NPCManager::NewNPCNotify(int player_id,int master_id, int owner_id)
{
    Debug4(LOG_NPC, 0, "New NPC(PID: %d) with master(PID: %u) and owner(PID: %u) sent to superclients.\n",
           player_id, master_id, owner_id );
    
    psNewNPCCreatedMessage msg(0,player_id,master_id,owner_id);
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
        npc->GetCharacterData()->SetImperviousToAttack(npc->GetCharacterData()->GetImperviousToAttack() | TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later

}
 
void NPCManager::SetNPCOwner(gemNPC *npc,int owner_id)
{
    /*Debug5(LOG_NPC,*/ printf("Setting NPC %d to Owner %d sent to superclients.\n",npc->GetPlayerID(), owner_id );

    gemActor *owner = (gemActor *)GEMSupervisor::GetSingleton().FindObject( owner_id );
    if ( owner )
    {
        psNPCSetOwnerMessage msg( 0, npc->GetPlayerID(), owner_id );
        msg.Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
    }
}


PetOwnerSession *NPCManager::CreatePetOwnerSession( gemActor *owner, psCharacter* petData )
{
    if ( owner && petData )
    {
        PetOwnerSession *session = new PetOwnerSession( this, owner, petData );
        if ( session )
        {
            OwnerPetList.Insert( session ); 
            return session;
        }
    }
    return NULL;
}


void NPCManager::RemovePetOwnerSession( PetOwnerSession *session)
{
    if ( session )
    {
        OwnerPetList.Delete( session );
        delete session;
    }
}

void NPCManager::UpdatePetTime()
{
    PetOwnerSession *po;

	// Loop through all Sessions
    BinaryRBIterator< PetOwnerSession > loop( &OwnerPetList );
    for ( po = loop.First(); po; po = ++loop )	    // Increase Pet Time in Game by NPC_TICK_INTERVAL
    {  
        if ( po->isActive )
        {
            po->UpdateElapsedTime( NPC_TICK_INTERVAL );
            //if ( po->owner ) 
            //    CPrintf(CON_NOTIFY,"updated pet time ( %s, %d, %.2f )\n", po->owner->GetName(), po->petID, po->elapsedTime);
        }
	    // Check session for time up
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

    if (!(counter%5))
        npcmgr->UpdateWorldPositions();

    npcmgr->SendAllCommands();
    npcmgr->UpdatePetTime();
    counter++;

}

/*-----------------------------------------------------------------*/
/* Pet Skills Handler                                              */
/*-----------------------------------------------------------------*/
void NPCManager::HandlePetSkill( MsgEntry * me )
{
    psPetSkillMessage msg( me );
    if ( !msg.valid )
    {
        Debug2( LOG_NET, me->clientnum, "Received unparsable psPetSkillMessage from client %u.\n", me->clientnum );
        return;
    }
    Client* client = clients->FindAny( me->clientnum );

    if ( !client->GetFamiliar() )
        return;

    //    CPrintf(CON_DEBUG, "ProgressionManager::HandleSkill(%d,%s)\n",msg.command, (const char*)msg.commandData);
    switch ( msg.command )
    {
        case psPetSkillMessage::REQUEST:
        {
            // Send all petStats to seed client
            psCharacter *chr = client->GetFamiliar()->GetCharacterData();
            chr->SendStatDRMessage(me->clientnum, client->GetFamiliar()->GetEntity()->GetID(), DIRTY_VITAL_ALL);

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
                            (unsigned int)(chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_STRENGTH)),
                            (unsigned int)(chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_ENDURANCE)),
                            (unsigned int)(chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_AGILITY)),
                            (unsigned int)(chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_INTELLIGENCE)),
                            (unsigned int)(chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_WILL)),
                            (unsigned int)(chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_CHARISMA)),
                            (unsigned int)(chr->GetHP()),
                            (unsigned int)(chr->GetMana()),
                            (unsigned int)(chr->GetStamina(true)),
                            (unsigned int)(chr->GetStamina(false)),
                            (unsigned int)(chr->GetHitPointsMax()),
                            (unsigned int)(chr->GetManaMax()),
                            (unsigned int)(chr->GetStaminaMax(true)),
                            (unsigned int)(chr->GetStaminaMax(false)),
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
   //     case psPetSkillMessage::BUY_SKILL:
   //     {
   //         CPrintf(CON_DEBUG, "---------------Buying Skill-------------\n");
   //         csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

   //         CS_ASSERT( xml );
   //         
   //         csRef<iDocument> invList  = xml->CreateDocument();

   //         const char* error = invList->Parse( msg.commandData);
   //         if ( error )
   //         {
   //             Error2("Error in XML: %s", error );
   //             CPrintf(CON_DEBUG, "    Error in XML\n");
   //             return;
   //         }

   //         csRef<iDocumentNode> root = invList->GetRoot();
            //if(!root)
            //{
            //    Error1("No XML root");
            //    return;
            //}
   //         csRef<iDocumentNode> topNode = root->GetNode("B");
            //if(!topNode)
            //{
            //    Error1("No <B> tag");
            //    return;
            //}
   //         
   //         csString skillName = topNode->GetAttributeValue("NAME");
            //if(!skillName || (skillName==""))
            //{
            //    psserver->SendSystemError(client->GetClientNum(), "You don't have any skill selected.");
            //    return;
            //}

   //         psSkillInfo * info = CacheManager::GetSingleton().GetSkillByName(skillName);
   //         CPrintf(CON_DEBUG, "    Looking for: %s\n", (const char*)skillName);
   //         
   //         if (!info)
   //         {
   //             Error2("No skill with name %s found!",skillName.GetData());
   //             Error2("Full Data Sent from Client was: %s\n", msg.commandData.GetData() );
   //             CPrintf(CON_DEBUG, "    No Skill Found\n");
   //             return;
   //         }

   //         psCharacter * character = client->GetCharacterData();

   //         if (character->GetTrainer() == NULL)
   //         {
   //             psserver->SendSystemInfo(client->GetClientNum(),
   //                                      "Can't buy skills when not training!");
   //             return;
   //         }
   //         
   //         gemActor* actorTrainer = character->GetTrainer()->GetActor();
   //         if ( actorTrainer )
   //         {
   //             if ( character->GetActor()->RangeTo(actorTrainer) > RANGE_TO_SELECT )
   //             {
   //                 psserver->SendSystemInfo(client->GetClientNum(),
   //                                          "Need to get a bit closer to understand the training.");
   //                 return;                    
   //             }
   //         }

   //         
   //         
   //         CPrintf(CON_DEBUG, "    PP available: %d\n", character->GetProgressionPoints() );
   //         
   //         // Test for progression points
   //         if (character->GetProgressionPoints() <= 0)
   //         {
   //             psserver->SendSystemInfo(client->GetClientNum(),
   //                                      "You don't have any progression points!");
   //             return;
   //         }

   //         // Test for money
   //         
   //         if (info->price > character->Money())
   //         {
   //             psserver->SendSystemInfo(client->GetClientNum(),
   //                                      "You don't have the money to buy this skill!");
   //             return;
   //         }
   //         if ( !character->CanTrain( info->id ) )
   //         {
   //             psserver->SendSystemInfo(client->GetClientNum(),
   //                                      "You cannot train this skill any higher yet!");
   //             return;            
   //         }
   //         
   //         int current = character->GetSkills()->GetSkillRank((PSSKILL)info->id);
   //         float faction = actorTrainer->GetRelativeFaction(character->GetActor());
   //         if ( !character->GetTrainer()->GetTrainerInfo()->TrainingInSkill((PSSKILL)info->id, current, faction))
   //         {
   //             psserver->SendSystemInfo(client->GetClientNum(),
   //                                      "You cannot train this skill currently.");
   //             return;            
   //         }

   //         character->UseProgressionPoints(1);
   //         character->SetMoney(character->Money()-info->price);            
   //         character->Train(info->id,1);
      //      SendSkillList(client,true,info->id);
   //         
   //         psserver->SendSystemInfo(client->GetClientNum(), "You've received some %s training", skillName.GetData());
   //         
   //         break;
   //     }
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

    buff.Format("<L X=\"%i\" >", character->GetProgressionPoints());

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

        Skill * charSkill = character->GetSkills()->GetSkill( (PSSKILL)skillID );
        if (charSkill == NULL)
        {
            Error3("Can't find skill %d in character %i",skillID, character->characterid);
            return;
        }

     //   // If we are training, send skills that the trainer is providing education in only
     //   if  (
     //           !trainerInfo
     //               ||
     //           trainerInfo->TrainingInSkill((PSSKILL)skillID, character->GetSkills()->GetSkillRank((PSSKILL)skillID), faction)
     //        )
     //   {
     //       bool stat = info->id == PSSKILL_AGI ||
     //                   info->id == PSSKILL_CHA ||
     //                   info->id == PSSKILL_END ||
     //                   info->id == PSSKILL_INT ||
     //                   info->id == PSSKILL_WILL ||
     //                   info->id == PSSKILL_STR;

     //       if(!found) // Get the row id the client will use
     //       {
     //           realID++;
     //           if(info->id == focus)
     //               found = true;
     //       }
    
        csString escpxml = EscpXML(info->name);

            buff.AppendFmt("<SKILL NAME=\"%s\" R=\"%i\" Y=\"%i\" YC=\"%i\" Z=\"%i\" ZC=\"%i\" CAT=\"%d\"/>",
                              escpxml.GetData(), charSkill->rank,
                              charSkill->y, charSkill->yCost, charSkill->z, charSkill->zCost, info->category);
        //}
    }
    buff.Append("</L>");

    psCharacter* chr = client->GetFamiliar()->GetCharacterData();
    psPetSkillMessage newmsg(client->GetClientNum(),
                            psPetSkillMessage::SKILL_LIST,
                            buff,
                            (unsigned int)chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_STRENGTH),
                            (unsigned int)chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_ENDURANCE),
                            (unsigned int)chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_AGILITY),
                            (unsigned int)chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_INTELLIGENCE),
                            (unsigned int)chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_WILL),
                            (unsigned int)chr->GetAttributes()->GetStat(PSITEMSTATS_STAT_CHARISMA),
                            (unsigned int)chr->GetHP(),
                            (unsigned int)chr->GetMana(),
                            (unsigned int)chr->GetStamina(true),
                            (unsigned int)chr->GetStamina(false),
                            (unsigned int)chr->GetHitPointsMax(),
                            (unsigned int)chr->GetManaMax(),
                            (unsigned int)chr->GetStaminaMax(true),
                            (unsigned int)chr->GetStaminaMax(false),
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
