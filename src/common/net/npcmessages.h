/*
* npcmessages.h
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
#ifndef __NPCMESSAGES_H__
#define __NPCMESSAGES_H__

#include "net/message.h"
#include "net/messages.h"
#include <csutil/csstring.h>
#include <csutil/databuf.h>
#include <csgeom/vector3.h>
#include "util/psstring.h"

struct iSector;


/**
* The message sent from superclient to server on login.
*/
class psNPCAuthenticationMessage : public psAuthenticationMessage
{
public:

    /**
    * This function creates a PS Message struct given a userid and
    * password to send out. This would be used for outgoing, new message
    * creation when a superclient wants to log in.
    */
    psNPCAuthenticationMessage(uint32_t clientnum,
        const char *userid,
        const char *password);

    /**
    * This constructor receives a PS Message struct and cracks it apart
    * to provide more easily usable fields.  It is intended for use on
    * incoming messages.
    */
    psNPCAuthenticationMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    bool NetVersionOk();
};

/**
* The message sent from server to superclient after successful
* login.  It is an XML message of all the basic info needed for
* all npc's managed by this particular superclient based on the login.
*/
class psNPCListMessage : public psMessageCracker
{
public:

    psString xml;

    /// Create psMessageBytes struct for outbound use
    psNPCListMessage(uint32_t clientToken,int size);

    /// Crack incoming psMessageBytes struct for inbound use
    psNPCListMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
* The message sent from superclient to server after
* receiving all entities.  It activates the NPCs on the server.
*/
class psNPCReadyMessage : public psMessageCracker
{
public:
    /// Create psMessageBytes struct for outbound use
	psNPCReadyMessage();

    /// Crack incoming psMessageBytes struct for inbound use
	psNPCReadyMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
* The 2nd message sent from server to superclient after successful
* login.  It is a list of strings, each string with the name
* of a CS world map zip file to load.
*/
class psMapListMessage : public psMessageCracker
{
public:

    csArray<csString> map;

    /// Create psMessageBytes struct for outbound use
    psMapListMessage(uint32_t clientToken,csString& str);

    /// Crack incoming psMessageBytes struct for inbound use
    psMapListMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
* The 3rd message sent from server to superclient after successful
* login.  It is a list of races and its properties.
*/
class psNPCRaceListMessage : public psMessageCracker
{
public:

    typedef struct
    {
        csString name; // On average the name should not exceed 100 characters
        float    walkSpeed;
        float    runSpeed;
    } NPCRaceInfo_t;
    

    csArray<NPCRaceInfo_t> raceInfo;

    /// Create psMessageBytes struct for outbound use
    psNPCRaceListMessage(uint32_t clientToken,int count);

    /// Crack incoming psMessageBytes struct for inbound use
    psNPCRaceListMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Add Race Info to the message
     */
    void AddRace(csString& name, float walkSpeed, float runSpeed, bool last);

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
* The message sent from server to superclient after successful
* login.  It is an XML message of all the basic info needed for
* all npc's managed by this particular superclient based on the login.
*/
class psNPCCommandsMessage : public psMessageCracker
{
public:

    enum Flags
    {
        NONE        = 0,
        INVISIBLE   = 1 << 0,
        INVINCIBLE  = 1 << 1
    };

    enum PerceptionType
    {
        CMD_TERMINATOR, // cmds go from superclient to server
        CMD_DRDATA,
        CMD_ATTACK,
        CMD_SPAWN,
        CMD_TALK,
        CMD_VISIBILITY,
        CMD_PICKUP,
        CMD_EQUIP,
        CMD_DEQUIP,
        CMD_DIG,
        CMD_DROP,
        CMD_TRANSFER,
        CMD_RESURRECT,
        CMD_SEQUENCE,
        CMD_IMPERVIOUS,
        PCPT_TALK,   // perceptions go from server to superclient
        PCPT_ATTACK,
        PCPT_GROUPATTACK,
        PCPT_DMG,
        PCPT_SPELL,
        PCPT_DEATH,
        PCPT_LONGRANGEPLAYER,
        PCPT_SHORTRANGEPLAYER,
        PCPT_VERYSHORTRANGEPLAYER,
        PCPT_OWNER_CMD,
        PCPT_OWNER_ACTION,
        PCPT_INVENTORY,
        PCPT_FLAG,
        PCPT_NPCCMD,
        PCPT_TRANSFER
    };

    /// Create psMessageBytes struct for outbound use
    psNPCCommandsMessage(uint32_t clientToken,int size);

    /// Crack incoming psMessageBytes struct for inbound use
    psNPCCommandsMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();
    
     /**
     * Convert the message into human readable string.
     *
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
* The message sent from server to superclient every 2.5 seconds.
* This message is the positions (and sectors) of every person
* in the game.
*/
class psAllEntityPosMessage: public psMessageCracker
{
public:
    /// Hold the number of entity positions after the message is cracked
    int count;

    /// Create psMessageBytes struct for outbound use
    psAllEntityPosMessage() { count = 0; msg=NULL; }

    /// Crack incoming psMessageBytes struct for inbound use
    psAllEntityPosMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    /// Sets the max size of the buffer
    void SetLength(int size,int client);

    /// Add a new entity's position to the data buffer
    void Add(EID id, csVector3 & pos, iSector* & sector, InstanceID instance, csStringHashReversible* msgstrings);

    /// Get the next entity and position from the buffer
    EID Get(csVector3 & pos, iSector* & sector, InstanceID & instance, csStringHashReversible* msgstrings, iEngine* engine);
};

/**
* The message sent from server to superclient after successful
* NPC Creation.  
*/
class psNewNPCCreatedMessage : public psMessageCracker
{
public:
    PID new_npc_id, master_id, owner_id;

    /// Create psMessageBytes struct for outbound use
    psNewNPCCreatedMessage(uint32_t clientToken, PID new_npc_id, PID master_id, PID owner_id);

    /// Crack incoming psMessageBytes struct for inbound use
    psNewNPCCreatedMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
* The message sent from client to server to control
* the players pet.
*/
class psPETCommandMessage : public psMessageCracker
{
public:

    typedef enum 
    {
        CMD_FOLLOW,
        CMD_STAY,
        CMD_DISMISS,
        CMD_SUMMON,
        CMD_ATTACK,
        CMD_GUARD,
        CMD_ASSIST,
        CMD_STOPATTACK,
        CMD_NAME,
        CMD_TARGET
    } PetCommand_t;

    static const char *petCommandString[];

    int command;
    psString target;
    psString options;

    /// Create psMessageBytes struct for outbound use
    psPETCommandMessage(uint32_t clientToken, int cmd, const char * target, const char * options);

    /// Crack incoming psMessageBytes struct for inbound use
    psPETCommandMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
* Server command message is for using npcclient as a remote command
* and debugging console for psserver.
*/
class psServerCommandMessage : public psMessageCracker
{
public:
    
    psString command;

    /// Create psMessageBytes struct for outbound use
    psServerCommandMessage(uint32_t clientToken, const char * target);

    /// Crack incoming psMessageBytes struct for inbound use
    psServerCommandMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs) { return command; }
};


#endif

