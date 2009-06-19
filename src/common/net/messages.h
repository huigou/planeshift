/*
 * messages.h
 *
 * Copyright (C) 2001-2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "net/netbase.h"
#include "net/message.h"
#include <csutil/csstring.h>
#include <csutil/array.h>
#include <csutil/stringarray.h>
#include <csutil/databuf.h>
#include <csutil/csmd5.h>
#include "util/psscf.h"
#include <csgeom/vector3.h>
#include "rpgrules/psmoney.h"
#include "util/psconst.h"
#include "util/skillcache.h"

// Jorrit: hack for mingw.
#ifdef SendMessage
#undef SendMessage
#endif

struct iSpriteCal3DState;
struct iEngine;
class psLinearMovement;

// This holds the version number of the network code, remember to increase
// this each time you do an update which breaks compatibility
#define PS_NETVERSION   0x009F
// Remember to bump the version in pscssetup.h, as well.


// NPC Networking version is separate so we don't have to break compatibility
// with clients to enhance the superclients.  Made it a large number to ensure
// no inadvertent overlaps.
#define PS_NPCNETVERSION 0x1013

enum Slot_Containers
{
    CONTAINER_INVENTORY_BULK      = -1,
    CONTAINER_INVENTORY_EQUIPMENT = -2,
    CONTAINER_EXCHANGE_OFFERING  = -3,
    CONTAINER_EXCHANGE_RECEIVING  = -4,

    CONTAINER_INVENTORY_MONEY = -5,
    CONTAINER_OFFERING_MONEY = -6,
    CONTAINER_RECEIVING_MONEY = -7,

    CONTAINER_WORLD = -8,
    CONTAINER_GEM_OBJECT = -9
};

struct iSector;

enum MSG_TYPES
{
    MSGTYPE_PING = 1,
    MSGTYPE_AUTHENTICATE,
    MSGTYPE_PREAUTHENTICATE,
    MSGTYPE_PREAUTHAPPROVED,
    MSGTYPE_AUTHAPPROVED,
    MSGTYPE_AUTHREJECTED,
    MSGTYPE_DISCONNECT,
    MSGTYPE_CHAT,
    MSGTYPE_CHANNEL_JOIN,
    MSGTYPE_CHANNEL_JOINED,
    MSGTYPE_CHANNEL_LEAVE,
    MSGTYPE_GUILDCMD,
    MSGTYPE_USERCMD,
    MSGTYPE_SYSTEM,
    MSGTYPE_CHARREJECT,
    MSGTYPE_DEAD_RECKONING,
    MSGTYPE_FORCE_POSITION,
    MSGTYPE_CELPERSIST,
    MSGTYPE_CONFIRMQUESTION,
    MSGTYPE_USERACTION,
    MSGTYPE_ADMINCMD,
    MSGTYPE_GUIINTERACT,
    MSGTYPE_GUIINVENTORY,
    MSGTYPE_VIEW_ITEM,
    MSGTYPE_VIEW_CONTAINER,
    MSGTYPE_VIEW_SKETCH,
    MSGTYPE_VIEW_ACTION_LOCATION,
    MSGTYPE_READ_BOOK,
    MSGTYPE_WRITE_BOOK,
    MSGTYPE_UPDATE_ITEM,
    MSGTYPE_MODE,
    MSGTYPE_WEATHER,
    MSGTYPE_NEWSECTOR,
    MSGTYPE_GUIGUILD,
    MSGTYPE_EQUIPMENT,
    MSGTYPE_GUIEXCHANGE,
    MSGTYPE_EXCHANGE_REQUEST,
    MSGTYPE_EXCHANGE_ADD_ITEM,
    MSGTYPE_EXCHANGE_REMOVE_ITEM,
    MSGTYPE_EXCHANGE_ACCEPT,
    MSGTYPE_EXCHANGE_STATUS,
    MSGTYPE_EXCHANGE_END,
    MSGTYPE_EXCHANGE_AUTOGIVE,
    MSGTYPE_EXCHANGE_MONEY,
    MSGTYPE_GUIMERCHANT,
    MSGTYPE_GROUPCMD,
    MSGTYPE_GUIGROUP,
    MSGTYPE_STATDRUPDATE,
    MSGTYPE_SPELL_BOOK,
    MSGTYPE_GLYPH_REQUEST,
    MSGTYPE_GLYPH_ASSEMBLE,
    MSGTYPE_PURIFY_GLYPH,
    MSGTYPE_SPELL_CAST,
    MSGTYPE_SPELL_CANCEL,
    MSGTYPE_EFFECT,
    MSGTYPE_EFFECT_STOP,
    MSGTYPE_NPCAUTHENT,
    MSGTYPE_NPCLIST,
    MSGTYPE_GUITARGETUPDATE,
    MSGTYPE_MAPLIST,
    MSGTYPE_NPCOMMANDLIST,
    MSGTYPE_NPCREADY,
    MSGTYPE_ALLENTITYPOS,
    MSGTYPE_NEW_NPC,
    MSGTYPE_PETITION,
    MSGTYPE_MSGSTRINGS,
    MSGTYPE_CHARACTERDATA,
    MSGTYPE_AUTHCHARACTER,
    MSGTYPE_AUTHCHARACTERAPPROVED,
    MSGTYPE_CHAR_CREATE_CP,
    MSGTYPE_COMBATEVENT,
    MSGTYPE_LOOT,
    MSGTYPE_LOOTITEM,
    MSGTYPE_LOOTREMOVE,
    MSGTYPE_GUISKILL,
    MSGTYPE_OVERRIDEACTION,
    MSGTYPE_QUESTLIST,
    MSGTYPE_QUESTINFO,
    MSGTYPE_GMGUI,
    MSGTYPE_WORKCMD,
    MSGTYPE_BUDDY_LIST,
    MSGTYPE_BUDDY_STATUS,
    MSGTYPE_MOTD,
    MSGTYPE_MOTDREQUEST,
    MSGTYPE_QUESTION,
    MSGTYPE_QUESTIONRESPONSE,
    MSGTYPE_SLOT_MOVEMENT,
    MSGTYPE_QUESTIONCANCEL,
    MSGTYPE_GUILDMOTDSET,
    MSGTYPE_PLAYSOUND,
    MSGTYPE_CHARACTERDETAILS,
    MSGTYPE_CHARDETAILSREQUEST,
    MSGTYPE_CHARDESCUPDATE,
    MSGTYPE_FACTION_INFO,
    MSGTYPE_QUESTREWARD,
    MSGTYPE_NAMECHANGE,
    MSGTYPE_GUILDCHANGE,
    MSGTYPE_LOCKPICK,
    MSGTYPE_GMSPAWNITEMS,
    MSGTYPE_GMSPAWNTYPES,
    MSGTYPE_GMSPAWNITEM,
    MSGTYPE_ADVICE,
    MSGTYPE_ACTIVEMAGIC,
    MSGTYPE_GROUPCHANGE,
    MSGTYPE_MAPACTION,
    MSGTYPE_CLIENTSTATUS,
    MSGTYPE_TUTORIAL,
    MSGTYPE_BANKING,
    MSGTYPE_CMDDROP,

    // Movement
    MSGTYPE_REQUESTMOVEMENTS,
    MSGTYPE_MOVEINFO,
    MSGTYPE_MOVEMOD,
    MSGTYPE_MOVELOCK,

    // Char creation messages under this
    MSGTYPE_CHAR_DELETE,
    MSGTYPE_CHAR_CREATE_PARENTS,
    MSGTYPE_CHAR_CREATE_CHILDHOOD,
    MSGTYPE_CHAR_CREATE_LIFEEVENTS,
    MSGTYPE_CHAR_CREATE_UPLOAD,
    MSGTYPE_CHAR_CREATE_VERIFY,
    MSGTYPE_CHAR_CREATE_NAME,
    MSGTYPE_PERSIST_WORLD_REQUEST,
    MSGTYPE_PERSIST_WORLD,
    MSGTYPE_PERSIST_ACTOR_REQUEST,
    MSGTYPE_PERSIST_ACTOR,
    MSGTYPE_PERSIST_ITEM,
    MSGTYPE_PERSIST_ACTIONLOCATION,
    MSGTYPE_PERSIST_ALL,
    MSGTYPE_REMOVE_OBJECT,
    MSGTYPE_CHANGE_TRAIT,

    // Internal Server Events here
    MSGTYPE_DAMAGE_EVENT,
    MSGTYPE_DEATH_EVENT,
    MSGTYPE_TARGET_EVENT,
    MSGTYPE_ZPOINT_EVENT,
    MSGTYPE_BUY_EVENT,
    MSGTYPE_SELL_EVENT,
    MSGTYPE_PICKUP_EVENT,
    MSGTYPE_DROP_EVENT,
    MSGTYPE_LOOT_EVENT,
    MSGTYPE_CONNECT_EVENT,
    MSGTYPE_MOVEMENT_EVENT,
    MSGTYPE_GENERIC_EVENT, // catchall for many Tutorial Events

    // Sound Events here
    MSGTYPE_SOUND_EVENT,

    // Char creation message
    MSGTYPE_CHAR_CREATE_TRAITS,
    MSGTYPE_STATS,

    // Pet Related Messages
    MSGTYPE_PET_COMMAND,
    MSGTYPE_PET_SKILL,

    MSGTYPE_CRAFT_INFO,

    MSGTYPE_PETITION_REQUEST,
    MSGTYPE_HEART_BEAT,
    MSGTYPE_NPC_COMMAND,

    // Minigame messages
    MSGTYPE_MINIGAME_STARTSTOP,
    MSGTYPE_MINIGAME_BOARD,
    MSGTYPE_MINIGAME_UPDATE,

    // Entance message type
    MSGTYPE_ENTRANCE,

    // GM Event message
    MSGTYPE_GMEVENT_LIST,
    MSGTYPE_GMEVENT_INFO,

    MSGTYPE_SEQUENCE,
    MSGTYPE_NPCRACELIST,

    MSGTYPE_INTRODUCTION,

	MSGTYPE_CACHEFILE,
    MSGTYPE_DIALOG_MENU,
    MSGTYPE_SIMPLE_STRING,
	MSGTYPE_ORDEREDTEST
};

class psMessageCracker;


// Types of system messages
#define MSG_ERROR               0x00000000 // Used for stuff that failed (By default, OnScreen Red)
#define MSG_INFO                0x00010000
#define MSG_INFO_SERVER         0x00010001
#define MSG_RESULT              0x00010002 // Used for things that the user might be interessted in (By default, OnScreen Yellow)
#define MSG_OK                  0x00010003 // Used for confimation that the action was accepted (By default, OnScreen Green)
#define MSG_WHO                 0x00010004 // Used for the message with /who content
#define MSG_ACK                 0x00010005 // Used for feedback localy (By default, OnScreen Blue)
#define MSG_INFO_BASE           0x00010006 // System messages that are shown on the "Main" tab
#define MSG_COMBAT              0x00020000
#define MSG_COMBAT_DODGE        0x00020001
#define MSG_COMBAT_BLOCK        0x00020002
#define MSG_COMBAT_HITYOU       0x00020003
#define MSG_COMBAT_HITOTHER     0x00020004
#define MSG_COMBAT_YOURHIT      0x00020005
#define MSG_COMBAT_OTHERHIT     0x00020006
#define MSG_COMBAT_MISS         0x00020007
#define MSG_COMBAT_OWN_DEATH    0x00020008
#define MSG_COMBAT_DEATH        0x00020009
#define MSG_COMBAT_VICTORY      0x0002000a
#define MSG_COMBAT_STANCE       0x0002000b
#define MSG_COMBAT_NEARLY_DEAD  0x0002000c
#define MSG_LOOT                0x00030000
#define MSG_SEC                 0x00300000
#define SEC_LEVEL0              0x00300000
#define SEC_LEVEL1              0x00300001
#define SEC_LEVEL2              0x00300002
#define SEC_LEVEL3              0x00300003
#define SEC_LEVEL4              0x00300004
#define MSG_PURCHASE            0x00400000

#define TOP_SHORT_INT_VAL       65535

class PublishVector;
class MsgHandler;


/**
 * All net messages inherit from this class.
 * The constructors either take the parameters required to build the
 * net message, or take the net message and pull out all the parameters.
 */
class psMessageCracker
{
public:
    static MsgHandler *msghandler;

    /**
     * Struct used by ToString to distribute a number of access pointers.
     * Collect all in one struct instead of creating multiple arguments that
     * would be hard to maintain.
     */
    struct AccessPointers
    {
        csStringHashReversible* msgstrings;
        iEngine *engine;
    };


    csRef<MsgEntry> msg;
    bool valid;

    psMessageCracker()
    : msg(NULL),valid(true)
    {}

    virtual ~psMessageCracker() {}

    /**
     * @brief Sends the message to the client/server.
     */
    void SendMessage();

    /**
     * @brief Multicasts the message to all current connections.
     */
    void Multicast(csArray<PublishDestination>& multi, int except, float range);

    /**
     * @brief Publishes the message to the local program.
     */
    void FireEvent();

    /**
     * @brief Gets the name of the message type
     *
     * Used by the message factory to get the name of the message type.
     * This should NOT be overloaded manually.
     * Use the PSF_DECLARE_MSG_FACTORY macro.
     *
     */
    virtual csString GetMessageTypeName() const = 0;

    /**
     * @brief Converts the message into human readable string.
     *
     * Used when a message is Logged.
     * Every message should implement a version of this function that print
     * all data that was decoded when the message was initiated from a NetEntry.
     * See psDRMessage::ToString for an example.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs) = 0;
    //{ return "Not implemented"; }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// PSF MESSAGE FACTORY stuff

typedef psMessageCracker* (*psfMsgFactoryFunc)(MsgEntry* me,psMessageCracker::AccessPointers * access_ptrs);

csString GetMsgTypeName(int msgType);
csString GetDecodedMessage(MsgEntry* me, csStringHashReversible* msgstrings, iEngine *engine, bool filterhex);

void psfRegisterMsgFactoryFunction(psfMsgFactoryFunc factoryfunc, int msgtype, const char* msgtypename);
psMessageCracker* psfCreateMsg(int msgtype,
                               MsgEntry* me,
                               psMessageCracker::AccessPointers * access_ptrs);
csString psfMsgTypeName(int msgType);
int psfMsgType(const char * msgTypeName);

#define PSF_DECLARE_MSG_FACTORY()                                 \
    virtual csString GetMessageTypeName() const;                  \
    static psMessageCracker* CreateMessage(MsgEntry * me,         \
               psMessageCracker::AccessPointers * access_ptrs)

#define PSF_IMPLEMENT_MSG_FACTORY_REGISTER(Class,MsgType)         \
    class Class##_StaticInit                                      \
    {                                                             \
    public:                                                       \
        Class##_StaticInit()                                      \
        {                                                         \
            psfRegisterMsgFactoryFunction (Class::CreateMessage,  \
                                           MsgType,#MsgType);     \
        }                                                         \
    } Class##_static_init__;


#define PSF_IMPLEMENT_MSG_FACTORY_CREATE(Class)                   \
    psMessageCracker* Class::CreateMessage(MsgEntry * me,         \
           psMessageCracker::AccessPointers * access_ptrs)        \
    {                                                             \
        return (psMessageCracker*)new Class(me);                  \
    }

#define PSF_IMPLEMENT_MSG_FACTORY_TYPENAME(Class,MsgType)         \
    csString Class::GetMessageTypeName() const                    \
    {                                                             \
        return #MsgType;                                          \
    }

#define PSF_IMPLEMENT_MSG_FACTORY(Class,MsgType)                  \
    PSF_IMPLEMENT_MSG_FACTORY_REGISTER(Class,MsgType)             \
    PSF_IMPLEMENT_MSG_FACTORY_TYPENAME(Class,MsgType)             \
    PSF_IMPLEMENT_MSG_FACTORY_CREATE(Class)

#define PSF_IMPLEMENT_MSG_FACTORY_CREATE2(Class)                  \
    psMessageCracker* Class::CreateMessage(MsgEntry * me,         \
           psMessageCracker::AccessPointers * a_p)                \
    {                                                             \
        return (psMessageCracker*)new Class(me,                   \
                                            a_p->msgstrings,      \
                                            a_p->engine);         \
    }

#define PSF_IMPLEMENT_MSG_FACTORY2(Class,MsgType)                 \
    PSF_IMPLEMENT_MSG_FACTORY_REGISTER(Class,MsgType)             \
    PSF_IMPLEMENT_MSG_FACTORY_TYPENAME(Class,MsgType)             \
    PSF_IMPLEMENT_MSG_FACTORY_CREATE2(Class)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** The message sent to the player being proposed for marriage */
class psMarriageMsgPropose : public psMessageCracker
{
public:
    psMarriageMsgPropose( const char* charName, const char* proposeMessage,
        uint32_t clientNum = 0 );
    psMarriageMsgPropose( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString charName;      /// Name of the character that we have to Propose
    csString proposeMsg;  /// Message from player to the character being Proposed
};

/** The message sent when someone divorces someone */
class psMarriageMsgDivorce : public psMessageCracker
{
public:
    psMarriageMsgDivorce( const char* divorceMessage, uint32_t clientNum = 0 );
    psMarriageMsgDivorce( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString divorceMsg;  /// Divorcing Message from player to spouse
};

/** The message is sent when someone requests marriage details of a player */
class psMarriageMsgDivorceConfirm : public psMessageCracker
{
public:
    psMarriageMsgDivorceConfirm( uint32_t clientNum = 0 );
    psMarriageMsgDivorceConfirm( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};


/**
 * The message sent from client to server to request a char deletion.
 * It deletes the selected char in charpickup screen after login.
 */
class psCharDeleteMessage : public psMessageCracker
{
public:
    psCharDeleteMessage( const char* charNameToDel, uint32_t clientNum );
    psCharDeleteMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString charName;
};

/**
 * The message sent from client to server before login.
 * It is just a request for a random number from the server
 * for the client to encrypt against
 */
class psPreAuthenticationMessage : public psMessageCracker
{
public:
    uint32_t  netversion;
    /**
     * @brief Creates a message for requesting auth from server
     */
    psPreAuthenticationMessage(uint32_t clientnum,uint32_t version=PS_NETVERSION);
    /**
     * This constructor receives a PS Message struct and cracks it apart
     * to provide more easily usable fields.  It is intended for use on
     * incoming messages.
     */
    psPreAuthenticationMessage(MsgEntry *message);


    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    bool NetVersionOk();
};

/**
 * The message sent from client to server on login.
 */
class psAuthenticationMessage : public psMessageCracker
{
public:
    uint32_t  netversion;
    csString  sAddr;
    csString  sUser,sPassword;

    /**
     * This function creates a PS Message struct given a userid and
     * password to send out. This would be used for outgoing, new message
     * creation when a user wants to log in.
     */
    psAuthenticationMessage(uint32_t clientnum,const char *userid,
        const char *password, uint32_t version=PS_NETVERSION);

    /**
     * This constructor receives a PS Message struct and cracks it apart
     * to provide more easily usable fields.  It is intended for use on
     * incoming messages.
     */
    psAuthenticationMessage(MsgEntry *message);


    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    bool NetVersionOk();
};

class psCharacterPickerMessage : public psMessageCracker
{
public:
    psCharacterPickerMessage( const char* character );
    psCharacterPickerMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    /// The name of the character that the account user wants to use.
    csString characterName;
};

class psCharacterApprovedMessage : public psMessageCracker
{
public:
    psCharacterApprovedMessage(uint32_t clientnum);
    psCharacterApprovedMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
 * Message sent from server to client that holds random number (clientnum).
 */
class psPreAuthApprovedMessage : public psMessageCracker
{
public:

    uint32_t  ClientNum;
    /** Create psMessageBytes struct for outbound use */
    psPreAuthApprovedMessage(uint32_t clientnum);

    /** Crack incoming psMessageBytes struct for inbound use */
    psPreAuthApprovedMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

};

/**
 * Message sent from server to client if login was valid.
 */
class psAuthApprovedMessage : public psMessageCracker
{
public:
    /** This must be returned by the client in all future messages to validate
     * sender.
     */
    uint32_t msgClientValidToken;
    /** This is the ID which must be requested to instantiate the client
     * player.
     */
    PID msgPlayerID;

    /// The number of characters for this account
    uint8_t msgNumOfChars;

    /** Stores initial data, but does not construct the outbound message. This allows
     *  a varying number of characters to be added via AddCharacter. Call ConstructMsg
     *  to create the actual message.
     */
    psAuthApprovedMessage(uint32_t clientnum, PID playerID, uint8_t numCharacters);

    /** Crack incoming psMessageBytes struct for inbound use */
    psAuthApprovedMessage(MsgEntry *message);

    /// Add another character definition to the buffer
    void AddCharacter(const char *fullname, const char *race,
                      const char *mesh, const char *traits, const char *equipment);

    /// Get the next character definition from the MsgEntry buffer
    void GetCharacter(MsgEntry *message,csString& fullname, csString& race,
                      csString& mesh, csString& traits,csString& equipment);

    /// Build the message
    void ConstructMsg();

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

private:
    csStringArray contents;
};


/**
 * Message sent from server to client if login was not valid.
 */
class psAuthRejectedMessage : public psMessageCracker
{
public:
    /** This is something like "account not valid" or "password not valid */
    csString msgReason;

    /// Create psMessageBytes struct for outbound use
    psAuthRejectedMessage(uint32_t clientToken,const char *reason);

    /// Crack incoming psMessageBytes struct for inbound use
    psAuthRejectedMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

};

enum {
    CHAT_SYSTEM,
    CHAT_COMBAT,
    CHAT_SAY,
    CHAT_TELL,
    CHAT_GROUP,
    CHAT_GUILD,
    CHAT_AUCTION,
    CHAT_SHOUT,
    CHAT_CHANNEL,
    CHAT_TELLSELF,
    CHAT_REPORT,
    CHAT_ADVISOR,
    CHAT_ADVICE,
    CHAT_ADVICE_LIST,
    CHAT_SERVER_TELL,      ///< this tell came from the server, not from another player
    CHAT_GM,
    CHAT_SERVER_INFO,
    CHAT_NPC,
    CHAT_NPCINTERNAL,
    CHAT_SYSTEM_BASE,      ///< System messages that are also shown on the "Main" tab
    CHAT_PET_ACTION,
    CHAT_NPC_ME,
    CHAT_NPC_MY,
    CHAT_NPC_NARRATE,
    CHAT_END
};

/**
 * Message sent with chat info.
 */
class psChatMessage : public psMessageCracker
{
public:
    /// type of message this is
    uint8_t  iChatType;

    /** name of person this chat message comes from */
    csString  sPerson;

    /**
     * Name of the other person involved in this chat message (used only with some chat types)
     *
     * This string is included in the net message only when needed and currently only the CHAT_ADVISOR
     * message type uses it. Modify both constructors if you want to use it with other chat message
     * types.
     */
    csString sOther;

    /** the text the message contains */
    csString  sText;

    /** is the text supposed to be translated by psLocalization on target client ? */
    bool translate;

    /** Keeps the eid of the originator client for chat bubbles */
    EID actor;
    
    uint16_t channelID;

    /** This function creates a PS Message struct given a chat text to send
     * out. This would be used for outgoing, new message creation
     */
    psChatMessage(uint32_t cnum, EID actorid, const char *person, const char * other, const char *chatMessage,
          uint8_t type, bool translate, uint16_t channelID = 0);

    /** This constructor receives a PS Message struct and cracks it apart
     * to provide more easily usable fields.  It is intended for use on
     * incoming messages.
     */
    psChatMessage(MsgEntry *message);


    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    /** Translate type code into words.  Could be multilingual in future. */
    const char *GetTypeText();
};

/**
 * Message from a client for a request to join a chat channel
 */
class psChannelJoinMessage : public psMessageCracker
{
public:
	csString channel;
	psChannelJoinMessage(const char* name);
	psChannelJoinMessage(MsgEntry *message);
	
	PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
 * Message from the server with a channel id
 */
class psChannelJoinedMessage : public psMessageCracker
{
public:
	uint16_t id;
	csString channel;
	psChannelJoinedMessage(uint32_t clientnum, const char* name, uint16_t id);
	psChannelJoinedMessage(MsgEntry *message);
	
	PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
 * Message from a client for a request to leave a chat channel
 */
class psChannelLeaveMessage : public psMessageCracker
{
public:
	uint16_t chanID;
	psChannelLeaveMessage(uint16_t id);
	psChannelLeaveMessage(MsgEntry *message);
	
	PSF_DECLARE_MSG_FACTORY();
	

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
 * Messages with system information sent to user.
 */
class psSystemMessage : public psMessageCracker
{
protected:
    psSystemMessage(){}
public:
    csString msgline;
    uint32_t type;

    psSystemMessage(uint32_t clientnum, uint32_t msgtype, const char *fmt, ... );
    psSystemMessage(uint32_t clientnum, uint32_t msgtype, const char *fmt, va_list args );
    psSystemMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
 * Messages with system information sent to user. This version is safe
 * for messages with user-input
 */
class psSystemMessageSafe : public psSystemMessage
{
public:
    psSystemMessageSafe(uint32_t clientnum, uint32_t msgtype,
            const char *text );
};

/**
 * Contains data for one petition
 */
struct psPetitionInfo
{
    int id;                 ///< unique id of the petition
    csString petition;      ///< petition text
    csString status;        ///< status of the petition
    csString assignedgm;    ///< the currently assigned GM

    int escalation;         ///< the escalation level of the petition
    csString player;        ///< the player
    csString created;       ///< date and time of creation
    csString resolution;    ///< resolution of a closed petition

    bool online;            ///< is the player online?
};

enum {
    PETITION_LIST = 0,      ///< Server is returning a list of petitions
    PETITION_CANCEL = 1,    ///< Server sends back result of cancel petition
    PETITION_CLOSE = 2,     ///< Server sends back result of close petition
    PETITION_CHANGE = 3,    ///< Server sends back result of change petition
    PETITION_ASSIGN = 4,    ///< Server sends back result of assignation of petition
    PETITION_DEASSIGN = 5,  ///< Server sends back result of deassignation of petition
    PETITION_ESCALATE = 6,  ///< Server sends back result of escalation of petition
    PETITION_DESCALATE = 7, ///< Server sends back result of descalation of petition
    PETITION_DIRTY = 8      ///< Server informs client that their petition list is dirty
};

/**
 * Messages sent to user with petition information
 */
class psPetitionMessage : public psMessageCracker
{
public:
    csArray<psPetitionInfo> petitions;
    bool success;
    csString error;
    int msgType;
    bool isGM;

    psPetitionMessage() { msgType = PETITION_LIST; }
    psPetitionMessage(uint32_t clientnum, csArray<psPetitionInfo> *petitionArray, const char* errMsg,
                bool succeed = true, int type = PETITION_LIST, bool gm = false);
    psPetitionMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
 * @brief Messages sent by the user to the server with requests for
 * petition information
 *
 * This class was added in case we need to add more petition
 * requests to the server, currently the recognized requests are:
 *
 *  - query    : return list of user petitions or all petitions if GM
 *  - cancel   : cancel the specified petition
 *  - change   : change the specified petition
 *  - close    : closes the specified petition
 *  - assign   : assign the GM to work on the specified petition
 *  - escalate : escalates the specified petition
 */
class psPetitionRequestMessage: public psMessageCracker
{
public:
    bool isGM;
    csString request;
    csString desc;
    int id;

    psPetitionRequestMessage(bool gm, const char* requestCmd, int petitionID = -1, const char* petDesc = "");
    psPetitionRequestMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

/**
 * Messages that are sent to the GM GUI windowManager
 */
class psGMGuiMessage : public psMessageCracker
{
public:

    struct PlayerInfo
    {
      csString name;
      csString lastName;
      int gender;
      csString guild;
      csString sector;
    };

    int gmSettings;
    csArray<PlayerInfo> players;
    int type;

    enum {
        TYPE_QUERYPLAYERLIST,
        TYPE_PLAYERLIST,
        TYPE_GETGMSETTINGS
    };

    psGMGuiMessage(uint32_t clientnum, int gmSets);
    psGMGuiMessage(uint32_t clientnum, csArray<PlayerInfo> *playerArray, int type);
    psGMGuiMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------

/**
 * Guild commands sent from client to server.
 */
class psGuildCmdMessage : public psMessageCracker
{
public:
    csString command, guildname, player, levelname, accept, secret, web_page,motd, alliancename;
    int level;


    psGuildCmdMessage(const char *cmd);
    psGuildCmdMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------

/** GUI Guild Message
 *
 * This message is used to manage the player guild window. The
 * client psGuildWindow and the server psSpellManger will deal with
 * this messages.
 */
class psGUIGuildMessage : public psMessageCracker
{
public:
    enum Command { SUBSCRIBE_GUILD_DATA,       ///< Client asks server to send guild data (GUILD_DATA,LEVELS_DATA,MEMBERS_DATA msgs)
                                               ///<     and also to send it when it changes
                   UNSUBSCRIBE_GUILD_DATA,     ///< Client asks server not to send guild data updates any more
                   SET_ONLINE,                 ///< Cliens asks server to change 'onlineOnly' attribute of its subscription
                                               ///<     server will reply with new MEMBERS_DATA message

                   GUILD_DATA,                 ///< Server sends client basic data about guild
                   LEVEL_DATA,                 ///< Server sends client data about guild levels
                   MEMBER_DATA,                ///< Server sends client data about guild members
                   ALLIANCE_DATA,              ///< Server sends client data about guild alliance

                   CLOSE_WINDOW,               ///< server tells client to close GuildWindow

                   NOT_IN_GUILD,               ///< Server tells client that player is not in a guild
                                               ///<        so the guild data cannot be sent

                   SET_LEVEL_RIGHT,            ///< Clients changes right for guild level

                   SET_MEMBER_POINTS,
                   SET_MEMBER_PUBLIC_NOTES,
                   SET_MEMBER_PRIVATE_NOTES,

                   SET_GUILD_NOTIFICATION      ///< Clients asks server to change the guild member login/logout notification setting
                };

    /** @brief Constuct a new equipment message to go on the network.
     *
     * This will build any of the GUI exchange message needed in
     * the guild window.
     *
     * @param clientNum   Client destination.
     * @param command     One of OPEN.
     * @param commandData XML string with command data
     *
     */
    psGUIGuildMessage( uint32_t command,
                       csString commandData);

    psGUIGuildMessage( uint32_t clientNum,
                       uint32_t command,
                       csString commandData);

    /// Crack this message off the network.
    psGUIGuildMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint32_t command;
    csString commandData;
};

//--------------------------------------------------------------------------

/**
 * Group commands
 */
class psGroupCmdMessage : public psMessageCracker
{
public:
    csString command,player,accept;

    psGroupCmdMessage(const char *cmd);
    psGroupCmdMessage(uint32_t clientnum,const char *cmd);
    psGroupCmdMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------

/**
 * User commands.
 * Those are processed by UserManager
 */
class psUserCmdMessage : public psMessageCracker
{
public:
    csString command,player,filter,action,text,target;
    int dice,sides,dtarget;
    int level;
    csString stance;

    psUserCmdMessage(const char *cmd);
    psUserCmdMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------

/**
 * Work commands.
 * Those are processed by WorkManager
 */
class psWorkCmdMessage : public psMessageCracker
{
public:
    csString command;
    csString player;
    csString filter;

    csString repairSlotName;        // The name of the slot to repair.

    psWorkCmdMessage(const char *cmd);
    psWorkCmdMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------

/**
 * Admin commands.
 * Those are processed by AdminManager
 */
class psAdminCmdMessage : public psMessageCracker
{
public:
    csString cmd;

    psAdminCmdMessage(const char *cmd);
    psAdminCmdMessage(const char *cmd, uint32_t client = 0);

    psAdminCmdMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------

class psDisconnectMessage : public psMessageCracker
{
public:
    EID actor;
    csString msgReason;

    psDisconnectMessage(uint32_t clientnum, EID actorid, const char *reason);
    psDisconnectMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------

class psUserActionMessage : public psMessageCracker
{
public:
    EID target;
    csString action;
    csString dfltBehaviors;

    psUserActionMessage(uint32_t clientnum, EID target, const char *action, const char *dfltBehaviors="");
    psUserActionMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//---------------------------------------------------------------------------

/// Sends messages to the client interaction window
class psGUIInteractMessage : public psMessageCracker
{
public:
    psGUIInteractMessage(uint32_t clientnum, uint32_t options);
    psGUIInteractMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    enum guiOptions
    {
        PICKUP      = 0x0000001,
        EXAMINE     = 0x0000002,
        UNLOCK      = 0x0000004,
        LOOT        = 0x0000008,
        BUYSELL     = 0x0000010,
        GIVE        = 0x0000020,
        CLOSE       = 0x0000040,
        USE         = 0x0000080,
        PLAYERDESC  = 0x0000100,
        ATTACK      = 0x0000200,
        COMBINE     = 0x0000400,
        EXCHANGE    = 0x0000800,
        TRAIN       = 0x0001000,
        NPCTALK     = 0x0002000,
        // Pet Commands
        VIEWSTATS   = 0x0004000,
        DISMISS     = 0x0008000,
        // Marriage
        MARRIAGE    = 0x0010000,
        DIVORCE     = 0x0020000,
        // Stuff
        PLAYGAME    = 0x0040000,
        ENTER       = 0x0080000,
        LOCK        = 0x0100000,
        ENTERLOCKED = 0x0200000,
        BANK        = 0x0400000,
        INTRODUCE   = 0x0800000,
        CONSTRUCT   = 0x1000000,
    };

public:
    /// Holds the options that the window should display.
    uint32_t options;
};

//--------------------------------------------------------------------------

/**
 * Messages that are sent to/from the ActionManager
 */
class psMapActionMessage : public psMessageCracker
{
public:

    uint32_t command;
    enum commands
    {
        QUERY,
        NOT_HANDLED,
        SAVE,
        LIST,
        LIST_QUERY,
        DELETE_ACTION,
        RELOAD_CACHE
    };

    csStringFast<1024> actionXML;

    // ctor
    psMapActionMessage( uint32_t clientnum, uint32_t cmd, const char *xml );

    // cracker
    psMapActionMessage( MsgEntry *message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//---------------------------------------------------------------------------

/// Sends messages to the client indicating player combat stance
class psModeMessage : public psMessageCracker
{
public:
    psModeMessage(uint32_t clientnum, EID actorID, uint8_t mode, uint32_t value);
    psModeMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    enum playerMode
    {
        PEACE = 1,
        COMBAT,
        SPELL_CASTING,
        WORK,
        DEAD,
        SIT,
        OVERWEIGHT,
        EXHAUSTED,
        DEFEATED,
        STATUE
    };

public:
    EID actorID;
    uint8_t mode;
    uint32_t value; //< stance if COMBAT, duration if SPELL_CASTING, ...
};

//---------------------------------------------------------------------------

/** Sends messages to the client informing of server-side movement lockouts.
 *  If the client ignores this, the server will just drop the invalid DR messages.
 */
class psMoveLockMessage : public psMessageCracker
{
public:
    psMoveLockMessage(uint32_t clientnum, bool locked);
    psMoveLockMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

public:
    bool locked;
};

//---------------------------------------------------------------------------

/// Sends messages to the client indicating rain and time of day
class psWeatherMessage : public psMessageCracker
{
public:
    enum playerMode
    {
        DAYNIGHT = 1,
        WEATHER  = 2
    };

    enum Weather
    {
        RAIN = 4,
        SNOW = 8,
        FOG = 16,
        LIGHTNING = 32
    };

    struct NetWeatherInfo
    {
        bool has_downfall;
        bool downfall_is_snow;
        bool has_fog;
        bool has_lightning;
        csString sector;
        int downfall_drops; // 0 = no downfall
        int downfall_fade;
        int fog_density;    // 0 = no fog
        int fog_fade;
        int r,g,b;          // For fog
    };

    psWeatherMessage(uint32_t client, psWeatherMessage::NetWeatherInfo info , uint clientnum = 0);
    psWeatherMessage(uint32_t client, int minute, int hour, int day, int month, int year );
    psWeatherMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

public:
    /// Holds the options that the window should display.
    int type;

    /// For Day/Night events
    uint minute;
    uint hour;
    uint day;
    uint month;
    uint year;

    NetWeatherInfo weather;
};


//---------------------------------------------------------------------------

/** This message class deals with the inventory messages.
  * This deals with all the requests/incomming information that the
  * player may need for the inventory screen. When the message is cracked
  * open it fills some public data members with data from the message based
  * on what the command is, so you should be aware of which data is valid for which
  * commands.
  *
  * When building the message the usual flow is:
  * - Create the message with the right command
  * - Use functions to add data to the message buffer.
  *
  * When reading the message the usual flow is:
  * - Crack message.
  * - Read valid data members based on command.
  */
class psGUIInventoryMessage : public psMessageCracker
{
public:
    enum commands
    {
        LIST,               ///< This is a list of items.
        REQUEST,            ///< This is a request for entire inventory.
        UPDATE_REQUEST,     ///< This is a request for inventory updates.
        UPDATE_LIST         ///< This is a list-update of items.
    };

    // One of the commands as defined above.
    uint8_t command;

    /** Creates a message with a command and max size.
     * The default message constructed is a request for inventory which
     * should normally be only done on the client.
     * @param command The type that this inventory message is. Should be one of the
     *                 defined enums. Default is request for inventory.
     * @param size The max size of this message.  Can be cliped when sent on the network.
     */
    psGUIInventoryMessage(uint8_t command = REQUEST, uint32_t size=0);


    /** Create a new message that will be used for an inventory list.
     * This is usually done by the server to create a message to be sent
     * to the client about the players inventory. It needs to know totalItems
     * so it can write that to the message first.
     *
     * @param command Should be LIST.
     * @param totalItems The total items or item stacks.
     * @param totalEmptiedSlots The total number of slots been emptied.
     * @param maxWeight The max weight the player can carry.
     * @param cache_version Inventory cache version we're gonna build in this message.
     */
    psGUIInventoryMessage( uint32_t clientnum,
                           uint8_t command,
                           uint32_t totalItems,
                           uint32_t totalEmptiedSlots,
                           float maxWeight,
						   uint32_t cache_version,
                           size_t msgsize);


    /** Crack open the message.
     * This is a switch statement that fills in particular data members
     * depending on the command that is given.
     */
    psGUIInventoryMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);


    /// Add an item to the output message buffer
    void AddItem(   const char* name,
                    int containerID,
                    int slot,
                    int stackcount,
                    float weight,
                    uint32 size,
                    const char* icon,
                    int purifyStatus );

    /// Add a newly emptied slot to output message buffer.
    void AddEmptySlot( int containerID, int slotID );

    /// Add money to the output message buffer
    void AddMoney( const psMoney & money);

    // Used in a move message.

    /// A small struct to hold item info after read out of message.
    struct ItemDescription
    {
        csString name;
        int slot;
        float weight;
        uint32 size;
        int stackcount;
        csString iconImage;
        int container;
        int purifyStatus;
    };

    // Item list
    csArray<ItemDescription> items;
    size_t totalItems;
    size_t totalEmptiedSlots;
    float maxWeight;    ///< The total max weight the player can carry.
    psMoney money;
	uint32 version;		/// cache version (PS#2691)
};

//---------------------------------------------------------------------------

/// Sends messages to the client indicating that a sector portal has been crossed
class psNewSectorMessage : public psMessageCracker
{
public:
    psNewSectorMessage(const csString & oldSector, const csString & newSector, csVector3 pos);
    psNewSectorMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

public:
    /// Holds the options that the window should display.
    csString oldSector, newSector;
    csVector3 pos;
};

//---------------------------------------------------------------------------

/// Sends messages to the server to indicate what to loot
class psLootItemMessage : public psMessageCracker
{
public:
    enum
    {
        LOOT_SELF,
        LOOT_ROLL
    };
    psLootItemMessage(int client, EID entity, int item, int action);
    psLootItemMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    EID entity;
    int lootitem;
    int lootaction;
};

//---------------------------------------------------------------------------

/// Sends messages to the client listing the available loot on a mob.
class psLootMessage : public psMessageCracker
{
public:
    psLootMessage();
    psLootMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    void Populate(EID entity, csString & lootstr, int cnum);

    EID entity_id;
    csString lootxml;
};

//---------------------------------------------------------------------------

/// Sends messages to the client listing the assigned quests for the player.
class psQuestListMessage : public psMessageCracker
{
public:
    psQuestListMessage();
    psQuestListMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    void Populate(csString& queststr, int cnum);

    csString questxml;
};

//---------------------------------------------------------------------------

/// Sends messages to the client listing the assigned quests for the player.
class psQuestInfoMessage : public psMessageCracker
{
public:
    enum
    {
        CMD_QUERY,
        CMD_INFO,
        CMD_DISCARD
    };
    psQuestInfoMessage(int cnum, int cmd, int id, const char *name,const char *info);
    psQuestInfoMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int command,id;
    csString xml;
};

//---------------------------------------------------------------------------

/// Indicates that an OverrideAction is being done and not a loop as covered in DR msgs.
class psOverrideActionMessage : public psMessageCracker
{
public:
    psOverrideActionMessage(int client, EID entity, const char *action, int duration = 0);
    psOverrideActionMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    EID entity_id;
    csString action;
    int duration;
};

//---------------------------------------------------------------------------


//--------------------------------------------------------------------------

/** General Equipment Message
 *
 * This message is used to tell other players about visual changes
 * in equipment. The inventory screen may generate these messages but
 * it is the client/server char managers that will deal with these
 *  messages.
 */
class psEquipmentMessage : public psMessageCracker
{
public:
    enum Command { EQUIP, DEEQUIP };

    /** @brief Constuct a new equipment message to go on the network.
     *
     * This will build an equipment message for any player that has
     * changed their equipment.  Should only be generated by the server.
     *
     * @param clientNum Client destination.
     * @param actorid   The entity that has changed ( target of this message )
     * @param type One of EQUIP or DEEQUIP
     * @param slot The slot that has changed
     * @param meshName The name of the mesh to attach to slot
     * @param part The name of the submesh that the equipment should go on.
     * @param texture The name of the new texture to go onto a part
     *
     * If the type is DEEQUIP the meshName is ignored and sent as ""
     */
    psEquipmentMessage( uint32_t clientNum,
                        EID actorid,
                        uint8_t type,
                        int slot,
                        csString& mesh,
                        csString& part,
                        csString& texture,
                        csString& partMesh);


    /// Crack this message off the network.
    psEquipmentMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint8_t type;
    int player;
    csString mesh;
    int slot;
    csString part;
    csString texture;
    csString partMesh; /* Set if the body part given by pattern given in part
                          should be replaced with this */
};

//--------------------------------------------------------------------------

/** GUI Exchange Message
 *
 * This message is used to manage the player exchange window. The
 * client/server exchange manger will deal with these messages.
 */
//--------------------------------------------------------------------------

/** GUI Merchant Message
 *
 * This message is used to manage the player merchant window. The
 * client/server NPC manger will deal with these messages.
 */
class psGUIMerchantMessage : public psMessageCracker
{
public:
    enum Command { REQUEST,
                   MERCHANT,
                   CATEGORIES,
                   CATEGORY,
                   MONEY,
                   ITEMS,
                   BUY,
                   SELL,
                   VIEW,
                   CANCEL};

    /** @brief Constuct a new equipment message to go on the network.
     *
     * This will build any of the GUI exchange message needed in
     * a player item exchange.
     *
     * @param clientNum   Client destination.
     * @param command     One of  REQUEST, EXCHANGE, INVENTORY, OFFERING or RECEIVING
     * @param commandData XML string with command data
     *
     */
    psGUIMerchantMessage( uint32_t clientNum,
                          uint8_t command,
                          csString commandData);

    psGUIMerchantMessage( uint8_t command,
                          csString commandData);

    /// Crack this message off the network.
    psGUIMerchantMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint8_t command;
    csString commandData;
};

//--------------------------------------------------------------------------

/** GUI Group Message
 *
 * This message is used to manage the player group window. The
 * client/server NPC manger will deal with these messages.
 */
class psGUIGroupMessage : public psMessageCracker
{
public:
    enum Command { GROUP,
                   MEMBERS,
                   LEAVE};

    /** @brief Constucts a new equipment message to go on the network.
     *
     * This will build any of the GUI exchange message needed in
     * a player item exchange.
     *
     * @param clientNum   Client destination.
     * @param command     One of  REQUEST, EXCHANGE, INVENTORY, OFFERING or RECEIVING
     * @param commandData XML string with command data
     *
     */
    psGUIGroupMessage( uint32_t clientNum,
                       uint8_t command,
                       csString commandData);

    psGUIGroupMessage( uint8_t command,
                       csString commandData);

    /// Crack this message off the network.
    psGUIGroupMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint8_t command;
    csString commandData;
};

//--------------------------------------------------------------------------
class psSpellCancelMessage : public psMessageCracker
{
public:
    psSpellCancelMessage()
    {
        msg.AttachNew(new MsgEntry());
        msg->SetType(MSGTYPE_SPELL_CANCEL);
        msg->clientnum = 0;
    }
    psSpellCancelMessage( MsgEntry * message ){};

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------
class psSpellBookMessage : public psMessageCracker
{
public:
    struct NetworkSpell
    {
        csString name;
        csString description;
        csString way;
        int realm;
        csString glyphs[4];
    };

    psSpellBookMessage();
    psSpellBookMessage( uint32_t client );
    psSpellBookMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    void AddSpell(const csString& name, const csString& description, const csString& way, int realm, const csString& glyph0, const csString& glyph1, const csString& glyph2, const csString& glyph3);
    void Construct();

    csArray<NetworkSpell> spells;

private:
    uint32_t size;
    uint32_t client;
};

//--------------------------------------------------------------------------

class psPurifyGlyphMessage : public psMessageCracker
{
public:
    psPurifyGlyphMessage( uint32_t glyphID );
    psPurifyGlyphMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint32_t glyph;
};

//--------------------------------------------------------------------------

/** Spell Cast Message
  *
 **/
class psSpellCastMessage : public psMessageCracker
{
public:
    psSpellCastMessage( csString &spellName, float kFactor );
    psSpellCastMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString spell;
    float kFactor;
};


//--------------------------------------------------------------------------
class psGlyphAssembleMessage : public psMessageCracker
{
public:
    psGlyphAssembleMessage() {}

    psGlyphAssembleMessage( int slot0, int slot1, int slot2, int slot3, bool info = false );
    psGlyphAssembleMessage( uint32_t clientNum,
                            csString spellName, csString image, csString description );
    psGlyphAssembleMessage( MsgEntry* me );
    void FromClient( MsgEntry* me );
    void FromServer( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int glyphs[4];
    bool info;
    csString description;
    csString name;
    csString image;

private:
    bool msgFromServer;
};



class psRequestGlyphsMessage : public psMessageCracker
{
public:
    struct NetworkGlyph
    {
        csString name;
        csString image;
        uint32_t purifiedStatus;
        uint32_t way;
        uint32_t statID;
    };


    psRequestGlyphsMessage( uint32_t client = 0 );
    psRequestGlyphsMessage( MsgEntry* me );
    virtual ~psRequestGlyphsMessage();

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    void AddGlyph( csString name, csString image, int purifiedStatus,
                   int way, int statID );

    void Construct();

    csArray<NetworkGlyph> glyphs;
private:
    size_t size;
    uint32_t client;

};


//--------------------------------------------------------------------------
 class psStopEffectMessage : public psMessageCracker
 {
 public:
    psStopEffectMessage( uint32_t clientNum, uint32_t uid )
    {
        msg.AttachNew(new MsgEntry(sizeof(uint32_t)));

        msg->SetType(MSGTYPE_EFFECT_STOP);
        msg->clientnum = clientNum;
        msg->Add(uid);
        valid = !(msg->overrun);
    }

    psStopEffectMessage( uint32_t uid )
    {
        msg.AttachNew(new MsgEntry(sizeof(uint32_t)));

        msg->SetType(MSGTYPE_EFFECT_STOP);
        msg->clientnum = 0;
        msg->Add(uid);
        valid = !(msg->overrun);
    }

    psStopEffectMessage(MsgEntry* message)
    {
        if (!message)
            return;

        uid = message->GetUInt32();
    }

    PSF_DECLARE_MSG_FACTORY();
    csString ToString(AccessPointers * access_ptrs)
    {
        csString msgtext;
        msgtext.AppendFmt("Effect ID: %d", uid);
        return msgtext;
    }

    uint32_t uid;
 };


/** Effect Message
 *
 * This message is used to manage any effect the server wants to send to the
 * clients.  psClientCharManager handles this clientside
 */
 class psEffectMessage : public psMessageCracker
{
public:

    /**  @brief Constructs a new message that will tell the client to render an effect
     *   @param clientNum the client to send the effect message to
     *   @param effectName the name of the effect to render
     *   @param effectOffset the offset position from the anchor point
     *   @param anchorID the ID of the entity to anchor the effect to (0 for absolute anchor)
     *   @param targetID the ID of the entity that will be the target of the effect (0 for a target the same as the anchor)
     *   @param uid Optional ID that server can use to stop a particular Effect.
     */
    psEffectMessage(uint32_t clientNum, const csString &effectName,
                    const csVector3 &effectOffset, EID anchorID,
                    EID targetID, uint32_t uid);

    /**  @brief Constructs a new message that will tell the client to render a spell effect - not just a normal effect
     *   @param clientNum the client to send the effect message to
     *   @param effectName the name of the effect to render
     *   @param effectOffset the offset position from the anchor point
     *   @param anchorID the ID of the entity to anchor the effect to (0 for absolute anchor)
     *   @param targetID the ID of the entity that will be the target of the effect (0 for a target the same as the anchor)
     *   @param duration the duration of the effect
     *   @param uid Optional ID that server can use to stop a particular Effect.
     */
    psEffectMessage(uint32_t clientNum, const csString &effectName,
                    const csVector3 &effectOffset, EID anchorID,
                    EID targetID, uint32_t duration, uint32_t uid);

    /**  @brief Constructs a new message that will tell the client to render a text effect - not just a normal effect
     *   @param clientNum the client to send the effect message to
     *   @param effectName the name of the effect to render
     *   @param effectOffset the offset position from the anchor point
     *   @param anchorID the ID of the entity to anchor the effect to (0 for absolute anchor)
     *   @param targetID the ID of the entity that will be the target of the effect (0 for a target the same as the anchor)
     *   @param effectText (this was documented incorrectly)
     *   @param uid Optional ID that server can use to stop a particular Effect.
     */
    psEffectMessage(uint32_t clientNum, const csString &effectName,
                    const csVector3 &effectOffset, EID anchorID,
                    EID targetID, csString &effectText, uint32_t uid);

    /**  @brief Translates a generic message to a psEffectMessage
     *   @param message the generic message to translate
     */
    psEffectMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString  name;
    csVector3 offset;
    EID       anchorID;
    EID       targetID;
    uint32_t  duration;
    uint32_t  uid;
    csString effectText;
};

//--------------------------------------------------------------------------

/** GUI Target Update
 * This class is used to notify the client that data it is showing in the GUI
 * target window has changed serverside, and should be refreshed.
 */
class psGUITargetUpdateMessage : public psMessageCracker
{
public:
    /** @brief Constucts a new GUI Target Update message to go on the network.
     *
     * This will build any of the GUI target update messages.
     *
     * @param clientNum   Client destination.
     * @param targetName  Name of the new target to display.
     */
    psGUITargetUpdateMessage(uint32_t client_num, EID target_id);
    psGUITargetUpdateMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint32_t clientNum;
    EID targetID;
};


/**
 * Message sent from server to client containing the message strings hash table.
 */
class psMsgStringsMessage : public psMessageCracker
{
public:
    /** Message strings hash table pointer (null for outbound)
     * This hash table will be allocated during message cracking,
     * and \b must \b be \b deleted manually.
     */
    csStringHashReversible* msgstrings;

    /** Create psMessageBytes struct for outbound use */
    psMsgStringsMessage(uint32_t clientnum, csStringHashReversible *strings);

    /** Crack incoming psMessageBytes struct for inbound use */
    psMsgStringsMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

private:
    uint32_t nstrings;
};

#if 0
/**
 * Message sent from server to client containing character description
 */
class psCharacterDataMessage : public psMessageCracker
{
public:
    csString fullname;
    csString race_name;
    csString mesh_name;
    csString traits;
    csString equipment;

    /** Create psMessageBytes struct for outbound use */
    psCharacterDataMessage(uint32_t clientnum,
                           csString fullname,
                           csString race_name,
                           csString mesh_name,
                           csString traits,
                           csString equipment);

    /** Crack incoming psMessageBytes struct for inbound use */
    psCharacterDataMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};
#endif

/**
 * Messages sent from server to client containing each detailed
 * combat event.
 */
class psCombatEventMessage : public psMessageCracker
{
public:
    int        event_type;
    EID        attacker_id;
    EID        target_id;
    int        target_location; // Where on the target the attack hit/miss
    float      damage;
    int        attack_anim;
    int        defense_anim;
    enum
    {
        COMBAT_DODGE,
        COMBAT_BLOCK,
        COMBAT_DAMAGE,
        COMBAT_MISS,
        COMBAT_OUTOFRANGE,
        COMBAT_DEATH,
        COMBAT_DAMAGE_NEARLY_DEAD       // This is equal to COMBAT_DAMAGE plus the target is nearly dead
    };

    /** Create psMessageBytes struct for outbound use */
    psCombatEventMessage(uint32_t clientnum,
                         int event_type,
                         EID attacker,
                         EID target,
                         int target_location,
                         float damage,
                         int attack_anim,
                         int defense_anim);

    void SetClientNum(int cnum);

    /** Crack incoming psMessageBytes struct for inbound use */
    psCombatEventMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//--------------------------------------------------------------------------

/** Sound Events Message
 *
 * This message is used to trigger sound events. The
 * client psSoundManager and various server managers will deal with
 * this messages. Specific sound events with additional data should inherit from this message.
 */

class psSoundEventMessage : public psMessageCracker
{
public:
    uint32_t type;
    /** Create psMessageBytes struct for outbound use */
    psSoundEventMessage(uint32_t clientnum, uint32_t type);

    /** Crack incoming psMessageBytes struct for inbound use */
    psSoundEventMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};
//--------------------------------------------------------------------------

class psStatDRMessage : public psMessageCracker
{
public:
    psStatDRMessage(uint32_t clientnum, EID eid, csArray<float> fVitals, csArray<uint32_t> uiVitals, uint8_t version, int flags);

    /** Send a request to the server for a full stat update.  */
    psStatDRMessage();

    /** Crack open the message from the server. */
    psStatDRMessage(MsgEntry* me);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    bool useful;
    EID entityid;
    uint32_t statsDirty;
    uint8_t counter;

    float hp,hp_rate,mana,mana_rate,pstam,pstam_rate;
    float mstam,mstam_rate,exp,prog;
};

//--------------------------------------------------------------------------

/** This message is sent to the client to let it know the basic stats of the player.
 */
class psStatsMessage : public psMessageCracker
{
public:
    /** @brief Sends a message to the client.
     *  @param client The active client number to send to.
     *  @param maxHP The newly calculated maximum HP for this client's character.
     *  @param maxMana The newly calculated maximum mana for this client's character.
     *  @param maxWeight The maximum weight that this character can carry.
     *  @param maxCapacity The maximum capacity of items that this character can carry.
     */
    psStatsMessage( uint32_t client, float maxHP, float maxMana, float maxWeight, float maxCapacity );

    /** Crack open the message from the server. */
    psStatsMessage( MsgEntry* me );

    /** Send a request to the server for the stats */
    psStatsMessage();

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    float hp;
    float mana;
    float weight;
    float capacity;
};




//--------------------------------------------------------------------------

/** GUI Skill Message
 *
 * This message is used to manage the player skill window. The
 * client psSkillWindow and server psProgressionManager will deal with
 * this messages.
 */
class psGUISkillMessage : public psMessageCracker
{
public:
    enum Command { REQUEST,
                   BUY_SKILL,
                   SKILL_LIST,
                   SKILL_SELECTED,
                   DESCRIPTION,
                   QUIT};

    /** @brief Constucts a new equipment message to go on the network.
     *
     * This will build any of the GUI exchange message needed in
     * a skill window.
     *
     * @param clientNum   Client destination.
     * @param command     One of REQUEST,BUY_SKILL, SKILL_LIST
     *                             SKILL_SELECTED, DESCRIPTION, QUIT
     * @param commandData XML string with command data
     *
     */
    psGUISkillMessage( uint8_t command,
                       csString commandData);

    psGUISkillMessage( uint32_t clientNum,
                        uint8_t command,
                        csString commandData,
                        psSkillCache *skills,
                        uint32_t str,
                        uint32_t end,
                        uint32_t agi,
                        uint32_t inl,
                        uint32_t wil,
                        uint32_t chr,
                        uint32_t hp,
                        uint32_t man,
                        uint32_t physSta,
                        uint32_t menSta,
                        uint32_t hpMax,
                        uint32_t manMax,
                        uint32_t physStaMax,
                        uint32_t menStaMax,
                        bool open,
                        int32_t focus,
                        int32_t selSkillCat,
                        bool isTraining);

    /// Crack this message off the network.
    psGUISkillMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint8_t command;
    csString commandData;
    psSkillCache skillCache;

    unsigned int strength;
    unsigned int endurance;
    unsigned int agility;
    unsigned int intelligence;
    unsigned int will;
    unsigned int charisma;
    unsigned int hitpoints;
    unsigned int mana;
    unsigned int physStamina;
    unsigned int menStamina;

    unsigned int hitpointsMax;
    unsigned int manaMax;
    unsigned int physStaminaMax;
    unsigned int menStaminaMax;
    bool openWindow;
    int32_t focusSkill;
    int32_t skillCat;
    bool trainingWindow; //Are we training or not?

private:
    bool includeStats;
};


//--------------------------------------------------------------------------

/** GUI Banking Message
 *
 * This message is used to manage the player banking window. On the
 * client psBankingWindow and on the server BankManager will deal with
 * these messages.
 */
class psGUIBankingMessage : public psMessageCracker
{
public:
    enum Command { WITHDRAWFUNDS,
                   DEPOSITFUNDS,
                   EXCHANGECOINS,
                   VIEWBANK };

    psGUIBankingMessage(uint32_t clientNum,
                        uint8_t command,
                        bool guild,
                        int circlesBanked,
                        int octasBanked,
                        int hexasBanked,
                        int triasBanked,
                        int circles,
                        int octas,
                        int hexas,
                        int trias,
                        int maxCircles,
                        int maxOctas,
                        int maxHexas,
                        int maxTrias,
                        float exchangeFee,
                        bool forceOpen);

    psGUIBankingMessage(uint8_t command,
                        bool guild,
                        int circles,
                        int octas,
                        int hexas,
                        int trias);

    psGUIBankingMessage(uint8_t command,
                        bool guild,
                        int coins,
                        int coin);

    /// Crack this message off the network.
    psGUIBankingMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint8_t command;
    bool guild;
    int circlesBanked;
    int octasBanked;
    int hexasBanked;
    int triasBanked;
    int circles;
    int octas;
    int hexas;
    int trias;
    int maxCircles;
    int maxOctas;
    int maxHexas;
    int maxTrias;
    int coins;
    int coin;
    float exchangeFee;
    bool openWindow;

private:
    bool sendingFull;
    bool sendingExchange;
};

//--------------------------------------------------------------------------

/** Pet Skill Message
 *
 * This message is used to manage the Pet skill window. The
 * client psPetStatWindow and server ??? will deal with
 * this messages.
 */
class psPetSkillMessage : public psMessageCracker
{
public:
    enum Command { REQUEST,
                   BUY_SKILL,
                   SKILL_LIST,
                   SKILL_SELECTED,
                   DESCRIPTION,
                   QUIT };

    /** @brief Constucts a new equipment message to go on the network.
     *
     * This will build any of the GUI exchange message needed in
     * a skill window.
     *
     * @param clientNum   Client destination.
     * @param command     One of REQUEST,BUY_SKILL, SKILL_LIST
     *                             SKILL_SELECTED, DESCRIPTION, QUIT
     * @param commandData XML string with command data
     *
     */
    psPetSkillMessage( uint8_t command,
                       csString commandData);

    psPetSkillMessage( uint32_t clientNum,
                        uint8_t command,
                        csString commandData,
                        uint32_t str,
                        uint32_t end,
                        uint32_t agi,
                        uint32_t inl,
                        uint32_t wil,
                        uint32_t chr,
                        uint32_t hp,
                        uint32_t man,
                        uint32_t physSta,
                        uint32_t menSta,
                        uint32_t hpMax,
                        uint32_t manMax,
                        uint32_t physStaMax,
                        uint32_t menStaMax,
                        bool open,
                        int32_t focus);

    /// Crack this message off the network.
    psPetSkillMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint8_t command;
    csString commandData;

    unsigned int strength;
    unsigned int endurance;
    unsigned int agility;
    unsigned int intelligence;
    unsigned int will;
    unsigned int charisma;
    unsigned int hitpoints;
    unsigned int mana;
    unsigned int physStamina;
    unsigned int menStamina;

    unsigned int hitpointsMax;
    unsigned int manaMax;
    unsigned int physStaminaMax;
    unsigned int menStaminaMax;
    bool openWindow;
    int32_t focusSkill;

private:
    bool includeStats;
};

//-----------------------------------------------------------------------------

class psDRMessage : public psMessageCracker
{
protected:
    void WriteDRInfo(uint32_t client, EID mappedid,
                    bool on_ground, uint8_t mode, uint8_t counter,
                    const csVector3& pos, float yrot, iSector *sector,
                    csString sectorName, const csVector3& vel, csVector3& worldVel,
                    float ang_vel, csStringHashReversible* msgstrings, bool donewriting=true);
    void ReadDRInfo( MsgEntry* me, csStringHashReversible* msgstrings, iEngine *engine);
    void CreateMsgEntry(uint32_t client, csStringHashReversible* msgstrings, iSector *sector, csString sectorName);

    /// Flags indicating what components are packed in this message
    enum DRDataFlags
    {
        NOT_MOVING      = 0,
        ACTOR_MODE      = 1 << 0,
        ANG_VELOCITY    = 1 << 1,
        X_VELOCITY      = 1 << 2,
        Y_VELOCITY      = 1 << 3,
        Z_VELOCITY      = 1 << 4,
        X_WORLDVELOCITY = 1 << 5,
        Y_WORLDVELOCITY = 1 << 6,
        Z_WORLDVELOCITY = 1 << 7,
        ALL_DATA        = ~0
    };

    enum { ON_GOUND = 128 }; ///< Use last bit of mode to indicate on/off ground

    static uint8_t GetDataFlags(const csVector3& v, const csVector3& wv, float yrv, uint8_t mode);

public:
    uint8_t counter;        ///< sequence checker byte
    bool on_ground;         ///< Helps determine whether gravity applies
    uint8_t mode;           ///< Current character mode
    csVector3 pos,          ///< Position vector
              vel,          ///< Body Velocity vector
              worldVel;     ///< World velocity vector
    float yrot;             ///< Rotation around Y-axis in radians
    iSector *sector;        ///< Ptr to sector for mesh
    csString sectorName;    ///< Name of the sector
    float ang_vel;          ///< Angular velocity of Yrot member changing
    EID entityid;           ///< The mapped id of the entity in question

    psDRMessage() { }
    psDRMessage(uint32_t client, EID mappedid, uint8_t counter,
                csStringHashReversible* msgstrings, psLinearMovement *linmove, uint8_t mode=0);
    psDRMessage(uint32_t client, EID mappedid,
                bool on_ground, uint8_t mode, uint8_t counter,
                const csVector3& pos, float yrot, iSector *sector, csString sectorName,
                const csVector3& vel, csVector3& worldVel, float ang_vel,
                csStringHashReversible* msgstrings);
    psDRMessage(void *data,int size, csStringHashReversible* msgstrings, iEngine *engine);
    psDRMessage( MsgEntry* me, csStringHashReversible* msgstrings, iEngine *engine);

    /// Returns true if this message is newer than the passed DR sequence value
    bool IsNewerThan(uint8_t oldCounter);

    PSF_DECLARE_MSG_FACTORY();

    void operator=(psDRMessage& other);

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//-----------------------------------------------------------------------------

class psForcePositionMessage : public psMessageCracker
{
public:
    csVector3 pos;       ///< Position vector
    float yrot;          ///< Rotation around Y-axis in radians
    iSector *sector;     ///< Ptr to sector for mesh
    csString sectorName; ///< Name of the sector

    psForcePositionMessage() { }
    psForcePositionMessage(uint32_t client, uint8_t sequence,
                           const csVector3& pos, float yRot, iSector *sector,
                           csStringHashReversible *msgstrings);
    psForcePositionMessage(MsgEntry *me, csStringHashReversible *msgstrings, iEngine *engine);

    PSF_DECLARE_MSG_FACTORY();

    void operator=(psForcePositionMessage& other);

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//-----------------------------------------------------------------------------

class psPersistWorldRequest : public psMessageCracker
{
public:
    psPersistWorldRequest();
    psPersistWorldRequest( MsgEntry * message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

class psRequestAllObjects : public psMessageCracker
{
public:
    psRequestAllObjects();
    psRequestAllObjects( MsgEntry * message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

class psPersistWorld : public psMessageCracker
{
public:
    psPersistWorld( uint32_t clientNum, const char* sectorName);
    psPersistWorld( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString sector;

};

class psPersistActorRequest : public psMessageCracker
{
public:
    psPersistActorRequest()
    {
        msg.AttachNew(new MsgEntry());
        msg->SetType(MSGTYPE_PERSIST_ACTOR_REQUEST);
        msg->clientnum  = 0;
    }

    psPersistActorRequest( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

};

class psPersistActor : public psDRMessage
{
public:
    enum Flags
    {
        NONE            = 0,
        INVISIBLE       = 1 << 0, // Used to inform super client. Clients will not get this
                                  // since actor is removed when invisible.
        INVINCIBLE      = 1 << 1, // Used to inform super client.
        NPC             = 1 << 2, // Set for NPCs
        NAMEKNOWN       = 1 << 3  // Used to tell the client if he knows this actor
    };

    psPersistActor( uint32_t clientnum,
                    int type,
                    int masqueradeType,
                    bool control,
                    const char* name,
                    const char* guildName,
                    const char* factname,
                    const char* raceName,
                    unsigned short int gender,
                    const char* helmGroup,
                    csVector3 collTop, csVector3 collBottom, csVector3 collOffset,
                    const char* texParts,
                    const char* equipmentParts,
                    uint8_t counter,
                    EID mappedid, csStringHashReversible* msgstrings, psLinearMovement *linmove,
                    uint8_t movementMode,
                    uint8_t serverMode,
                    PID playerID = 0, uint32_t groupID = 0, EID ownerEID = 0,
                    uint32_t flags = NONE );

    psPersistActor( MsgEntry* me, csStringHashReversible* msgstrings, iEngine *engine );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    /**
     * Used to insert playerid into the message buffer after creation.
     */
    void SetPlayerID(PID playerID);

    /**
     * Used to insert instance into the message buffer after creation.
     */
    void SetInstance(InstanceID instance);

    csString name;
    csString guild;
    csString factname;
    csString race;
    unsigned short int gender;
    csString helmGroup;                 // Used for helm groupings.
    csVector3 top, bottom, offset;
    csString texParts;
    csString equipment;
    int type;
    int masqueradeType;
    uint8_t serverMode;
    PID playerID;
    uint32_t groupID;
    EID ownerEID;
    bool control;
    uint32_t flags;
    InstanceID instance;

    int posPlayerID; // Remember the position the playerID in the generated message
    int posInstance; // Remember the position of the instance field in the generated message
};


class psPersistItem : public psMessageCracker
{
public:
    enum Flags
    {
        NONE            = 0,
        NOPICKUP        = 1 << 0,
        COLLIDE         = 1 << 1
    };

    psPersistItem(  uint32_t clientnum,
                    EID id,
                    int type,
                    const char* name,
                    const char* factname,
                    const char* sector,
                    csVector3 pos,
                    float xRot,
                    float yRot,
                    float zRot,
                    uint32_t flags
                 );

    psPersistItem( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString name;
    csString factname;
    csString sector;
    csVector3 pos;
    float xRot;
    float yRot;
    float zRot;
    EID eid;
    uint32_t type;
    uint32_t flags;
};

class psPersistActionLocation : public psMessageCracker
{
public:
    psPersistActionLocation( uint32_t clientNum,
                                EID eid,
                                int type,
                                const char* name,
                                const char* sector,
                                const char* mesh
                               );

    psPersistActionLocation( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString name;
    csString sector;
    csString mesh;
    EID eid;
    uint32_t type;
};

class psRemoveObject : public psMessageCracker
{
public:
    psRemoveObject(uint32_t clientNum, EID objectEID);
    psRemoveObject( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    EID objectEID;
};



class psBuddyListMsg : public psMessageCracker
{
public:
    struct BuddyData
    {
        csString name;
        bool online;
    };

    psBuddyListMsg( uint32_t clientNum, int totalBuddies );
    psBuddyListMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    void AddBuddy( int num, const char* name, bool onlineStatus );
    void Build();

    csArray<BuddyData> buddies;
};



class psBuddyStatus : public psMessageCracker
{
public:
    psBuddyStatus( uint32_t clientNum, csString& buddyName, bool online )
    {
        msg.AttachNew(new MsgEntry( buddyName.Length()+1 + sizeof(bool) ));

        msg->SetType(MSGTYPE_BUDDY_STATUS);
        msg->clientnum = clientNum;

        msg->Add( buddyName );
        msg->Add( online );
    }

    psBuddyStatus( MsgEntry* me )
    {
        buddy        = me->GetStr();
        onlineStatus = me->GetBool();
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString buddy;
    bool onlineStatus;
};

class psMOTDMessage : public psMessageCracker
{
public:
    psMOTDMessage(uint32_t clientNum, const csString& tipMsg, const csString& motdMsg, const csString& guildMsg, const csString& guild)
    {
        msg.AttachNew(new MsgEntry( tipMsg.Length()+1 + motdMsg.Length()+1 + guildMsg.Length()+1 + guild.Length() +1 ));

        msg->SetType(MSGTYPE_MOTD);
        msg->clientnum = clientNum;

        msg->Add( tipMsg );
        msg->Add( motdMsg );
        msg->Add( guildMsg );
        msg->Add( guild );
    }

    psMOTDMessage( MsgEntry* me )
    {
        tip        = me->GetStr();
        motd       = me->GetStr();
        guildmotd  = me->GetStr();
        guild      = me->GetStr();
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString tip;
    csString motd;
    csString guildmotd;
    csString guild;
};

class psMOTDRequestMessage : public psMessageCracker
{
public:
    psMOTDRequestMessage()
    {
        msg.AttachNew(new MsgEntry());
        msg->SetType(MSGTYPE_MOTDREQUEST);
        msg->clientnum  = 0;
    }
    psMOTDRequestMessage( MsgEntry* me ) {}


    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

class psQuestionResponseMsg : public psMessageCracker
{
public:
    uint32_t questionID;
    csString answer;

    psQuestionResponseMsg(int clientnum,uint32_t questionID,const csString & answer)
    {
        msg.AttachNew(new MsgEntry( sizeof(questionID)+answer.Length()+1 ));
        msg->SetType(MSGTYPE_QUESTIONRESPONSE);
        msg->clientnum  = clientnum;
        msg->Add(questionID);
        msg->Add(answer);
    }
    psQuestionResponseMsg(MsgEntry *me)
    {
        questionID = me->GetUInt32();
        answer     = me->GetStr();
        valid = true;
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

class psQuestionMessage : public psMessageCracker
{
public:
    enum questionType_t
    {
        generalConfirm,
        secretGuildNotify,
        duelConfirm
    };

    uint32_t questionID;
    csString question;     // some string - its format depends on 'type'
    questionType_t type;

    psQuestionMessage(int clientnum,
                      uint32_t questionID,
                      const char *question,
                      questionType_t type)
    {
        msg.AttachNew(new MsgEntry( sizeof(questionID) +strlen(question)+1 +2 ));
        msg->SetType(MSGTYPE_QUESTION);
        msg->clientnum  = clientnum;

        msg->Add(questionID);
        msg->Add(question);
        msg->Add((uint16_t)type);
        valid=!(msg->overrun);
    }
    psQuestionMessage(MsgEntry *me)
    {
        questionID  =       me->GetUInt32();
        question    =       me->GetStr();
        type        =       (questionType_t)me->GetInt16();
        valid = true;
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

class psAdviceMessage : public psMessageCracker
{
public:
    csString sCommand;
    csString sTarget;
    csString sMessage;

    psAdviceMessage( int clientNum, const char *command, const char *target, const char *message )
    {
        size_t msgSize = 0;

        if ( message ) msgSize = strlen( message );

        msg.AttachNew(new MsgEntry( strlen( command ) + strlen( target ) + msgSize + 3));
        msg->SetType(MSGTYPE_ADVICE);
        msg->clientnum  = clientNum;
        msg->Add( command );
        msg->Add( target );
        msg->Add( message?message:"" );
        valid = !(msg->overrun);
    };

    psAdviceMessage( MsgEntry *me )
    {
        sCommand = me->GetStr();
        sTarget  = me->GetStr();
        sMessage = me->GetStr();
        valid = true;
    };

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

////////////////////////////////////////////////////////////////////////////////
/** GUI Active Magic Message
 *
 * This message is used to manage the active spells window. The
 * client psActiveSpellWindow and server psProgressionManager will deal with
 * this messages.
 */
class psGUIActiveMagicMessage : public psMessageCracker
{
public:
    enum commandType { Add, Remove };

    psGUIActiveMagicMessage(uint32_t clientNum,
                            commandType cmd,
                            SPELL_TYPE type,
                            const csString & name)
    {
        msg.AttachNew(new MsgEntry( sizeof(bool) + +sizeof(uint8_t) + sizeof(int32_t) + name.Length() + 1));
        msg->SetType(MSGTYPE_ACTIVEMAGIC);
        msg->clientnum = clientNum;
        msg->Add((uint8_t)cmd);
        msg->Add((uint8_t)type);
        msg->Add(name);
        valid = !(msg->overrun);
    }

    /// Crack this message off the network.
    psGUIActiveMagicMessage( MsgEntry* message )
    {
        command = (commandType) message->GetUInt8();
        type = (SPELL_TYPE) message->GetUInt8();
        name = message->GetStr();
        valid = true;
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    commandType command;
    SPELL_TYPE type;
    csString name;
};

//-----------------------------------------------------------------------------
class psSlotMovementMsg : public psMessageCracker
{
public:
    psSlotMovementMsg( int fromContainerID,
                       int fromSlotID,
                       int toContainerID,
                       int toSlotID,
                       int stackCount,
                       csVector3 *pt3d=NULL,
                       bool guarded=true,
                       bool inplace=true)
    {
        msg.AttachNew(new MsgEntry( sizeof( int32_t ) * 5 + 3 * sizeof(float) + 2 * sizeof(bool) ));

        msg->SetType(MSGTYPE_SLOT_MOVEMENT);
        msg->clientnum  = 0;

        msg->Add( (int32_t) fromContainerID );
        msg->Add( (int32_t) fromSlotID );
        msg->Add( (int32_t) toContainerID );
        msg->Add( (int32_t) toSlotID );
        msg->Add( (int32_t) stackCount );
        if (pt3d != NULL)
            msg->Add( *pt3d );
        else
        {
            csVector3 v = 0;
            msg->Add(v);  // add dummy zeroes if not specified.
        }
        msg->Add( guarded );
        msg->Add( inplace );

    }

    psSlotMovementMsg( MsgEntry* me )
    {
        fromContainer = me->GetInt32();
        fromSlot      = me->GetInt32();
        toContainer   = me->GetInt32();
        toSlot        = me->GetInt32();
        stackCount    = me->GetInt32();
        posWorld      = me->GetVector();
        guarded       = me->GetBool();
        inplace       = me->GetBool();

    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int fromContainer;
    int fromSlot;
    int toContainer;
    int toSlot;
    int stackCount;
    csVector3 posWorld;
    bool guarded;
    bool inplace;
};

class psCmdDropMessage : public psMessageCracker
{
public:
    psCmdDropMessage( int quantity, csString &itemName, bool container, bool guarded, bool inplace);
    psCmdDropMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int quantity;
    csString itemName;

    bool container;
    bool guarded;
    bool inplace;
};

class psQuestionCancelMessage : public psMessageCracker
{
public:
    uint32_t questionID;

    psQuestionCancelMessage(int clientnum, uint32_t id)
    {
        msg.AttachNew(new MsgEntry( sizeof(id) ));
        msg->SetType(MSGTYPE_QUESTIONCANCEL);
        msg->clientnum  = clientnum;

        msg->Add(id);
    }
    psQuestionCancelMessage(MsgEntry *me)
    {
        questionID = me->GetUInt32();
        valid = true;
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

class psGuildMOTDSetMessage : public psMessageCracker
{
public:
    psGuildMOTDSetMessage( csString& guildMsg,csString& guild)
    {
        msg.AttachNew(new MsgEntry( guildMsg.Length()+1 + guild.Length() +1 ));

        msg->SetType(MSGTYPE_GUILDMOTDSET);
        msg->clientnum = 0;
        msg->Add( guildMsg );
        msg->Add( guild );
    }

    psGuildMOTDSetMessage( MsgEntry* me )
    {
        guildmotd  = me->GetStr();
        if (!guildmotd.Length())
            guildmotd = "(No message of the day.)";
        guild      = me->GetStr();
        if (!guild.Length())
            valid = false;
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString guildmotd;
    csString guild;
};

class psCharacterDetailsMessage : public psMessageCracker
{
public:
    struct NetworkDetailSkill
    {
        int category;
        csString text;
    };

    psCharacterDetailsMessage( int clientnum, const csString& name2s,unsigned short int gender2s,const csString& race2s,
                               const csString& desc2s, const csArray<NetworkDetailSkill>& skills2s, const csString& desc_ooc, const csString& creationinfo, const csString& requestor);
    psCharacterDetailsMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString name;
    unsigned short int gender;
    csString race;
    csString desc;
    csString desc_ooc;
    csString creationinfo;
    csArray<NetworkDetailSkill> skills;

    csString requestor;  // Identifies which part of system initiated this
};

class psCharacterDetailsRequestMessage : public psMessageCracker
{
public:
    psCharacterDetailsRequestMessage( bool myself, bool simple, const csString & requestor)
    {
        //If myself = true, the server sends the information about the player
        //If myself = false, the server sends the information about the target

        msg.AttachNew(new MsgEntry( sizeof(myself) + sizeof(simple) + requestor.Length() + 1 ));
        msg->SetType(MSGTYPE_CHARDETAILSREQUEST);
        msg->clientnum  = 0;
        msg->Add(myself);
        msg->Add(simple);
        msg->Add(requestor.GetData());
    }

    psCharacterDetailsRequestMessage(MsgEntry *me)
    {
        isMe       =  me->GetBool();
        isSimple   =  me->GetBool();
        requestor  =  me->GetStr();
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    bool isMe;
    bool isSimple;          /// Weather simple description or full is requested
    csString requestor;     /// Identifies which part of system initiated this
};

enum DESCTYPE
{
    DESC_IC = 1, ///< Normal character description
    DESC_OOC,    ///< Out of Character description
    DESC_CC      ///< Character life additional data provided by players
};

class psCharacterDescriptionUpdateMessage : public psMessageCracker
{
public:
    psCharacterDescriptionUpdateMessage(csString& newValue, DESCTYPE desctype)
    {

        msg.AttachNew(new MsgEntry( newValue.Length() +1 + sizeof(uint8_t) ));
        msg->SetType(MSGTYPE_CHARDESCUPDATE);

        msg->clientnum  = 0;
        msg->Add(newValue);
        msg->Add( (uint8_t)desctype);
    }

    psCharacterDescriptionUpdateMessage(MsgEntry *me)
    {
        newValue    =  me->GetStr();
        desctype    =  (DESCTYPE) me->GetUInt8();
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString newValue;
    DESCTYPE desctype;
};

//------------------------------------------------------------------------------

class psViewActionLocationMessage : public psMessageCracker
{
public:
    psViewActionLocationMessage(uint32_t clientnum, const char* actname, const char* desc);
    psViewActionLocationMessage(MsgEntry* me);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    const char *name;
    const char *description;
};

//------------------------------------------------------------------------------

/** General Message for sending information about an item to a client.
  * This class can take single items or containers and will send all the data to
  * the client.  The client will then pop up the correct window when it gets one
  * of these messages.  The item screen for a single item or the container screen
  * if the item is a container.
  *
  * When sending an item to a client you create this message by giving the name and
  * description of the item ( all items have this )
  *
  * If it is a container you need to set the container ID and call AddContents() for
  * each item in the container.  After that you need to call ConstructMsg() to build the
  * network message.
  */
class psViewItemDescription : public psMessageCracker
{
public:
    /** @brief Requests to the server for an item Description.
      *
      * @param containerID What container this item is from. Can be one of the special ones
      *                    like CONTAINER_INVENTORY. If the item is in the world or not in
      *                    a container then it is the gem id of the item.
      * @param slotID Where this item is in the container.
      */
    psViewItemDescription(int containerID, int slotID);

    /** @brief Construct a message to go out to a client.
      *
      * If icContainer is false then the message is constructed right away.  If it is true
      * then the data is stored and the message is not constructed (because size is not known ).
      *
      * @param to The desitination client.
      * @param itemName The name of the item requested.
      * @param description The description of the requested item.
      * @param icon The 2D gui image to draw for this item.
      * @param isContainer True if this item is a container.
      */
    psViewItemDescription( uint32_t to, const char *itemName, const char *Description, const char *icon,
                           uint32_t stackCount, bool isContainer = false );

    /** Crack out the details from the message.
      * This will look at the packet and figure out if it is a single item or a container.
      * If it is a container it will populate it's internal array of data.
      */
    psViewItemDescription( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    /// The name of the item or container.
    const char *itemName;

    /// The description of this item or container.
    const char *itemDescription;

    /// The 2D graphic GUI image.
    const char *itemIcon;

    /// Stack count of the item.
    uint32_t stackCount;

    /// The container ID for this item.
    int containerID;

    /// Where this item is in the container.
    int slotID;

    /// Running count of message size. Used for constructing the outgoing message.
    int msgSize;

    /// True if this item is a container and has contents.
    bool hasContents;

    /// The destination client for this message.
    int to;

    /// Add a item to this message ( assumes the base item is a container )
    void AddContents( const char *name, const char *icon, int purifyStatus, int slot, int stack );

    /// Build the message ( assumes base item is a container ).
    void ConstructMsg();

    struct ContainerContents
    {
        csString name;
        csString icon;
        int slotID;
        int stackCount;
        int purifyStatus;
    };

    csArray<ContainerContents> contents;

private:
    enum
    {
        REQUEST,
        DESCR
    };
    int format;
};



class psViewItemUpdate : public psMessageCracker
{
public:
    /** @brief Constructs a message to go out to a client.
      *
      * @param to The desitination client.
      * @param containerID The destination container's entity ID (it's always a world container).
      * @param slotID The slot in the container where to make the update.
      * @param clearSlot Boolean that indicates if the update is to clear out the slot.
      * @param itemName The name of the item requested.
      * @param icon The 2D gui image to draw for this item.
      * @param stackCount The number of items in the stack.
      * @param ownerEID The GEM entity ID of the owner
      */
    psViewItemUpdate(uint32_t to, EID containerID, uint32_t slotID, bool clearSlot, const char *itemName, const char *icon, uint32_t stackCount, EID ownerID);

    /// Crack out the details from the message.
    psViewItemUpdate( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

//    /// Running count of message size. Used for constructing the outgoing message.
//    int msgSize;

    /// indicates if the slot should be cleared
    bool clearSlot;

    /// The destination client for this message.
    int to;

    /// The EID for the container we're updating.
    EID containerID;

    /// Item info
    csString name;
    csString icon;
    int slotID;
    int stackCount;
    EID ownerID;
};


class psWriteBookMessage : public psMessageCracker
{
public:
    /**
     * 'Request to Write' message, from client to server.
     *
     */
    psWriteBookMessage( int slotID, int containerID);
    /**
     * 'Request to Save' message, from client to server
     */
    psWriteBookMessage( int slotID, int containerID, csString& title, csString& content);
    /**
     * Response from server, if success is false then content is the error message
     */
    psWriteBookMessage( uint32_t clientNum, csString& title, csString& content, bool success,  int slotID, int containerID);
    /**
     * Response to Save from server to client.
     */
    psWriteBookMessage( uint32_t clientNum, csString& title, bool success);

    psWriteBookMessage( MsgEntry *me);

    PSF_DECLARE_MSG_FACTORY();

    virtual csString ToString(AccessPointers * access_ptrs);

    uint8_t messagetype;
    csString title, content;
    int slotID;
    int containerID;
    bool success;

    enum {
       REQUEST,
       RESPONSE,
       SAVE,
       SAVERESPONSE
    };
};

class psReadBookTextMessage : public psMessageCracker
{
public:
   psReadBookTextMessage(uint32_t clientNum, csString& itemName, csString& bookText, bool canWrite, int slotID, int containerID);

    /** Crack out the details from the message.
      * This will look at the packet and figure out if it is a single item or a container.
      * If it is a container it will populate it's internal array of data.
      */
    psReadBookTextMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString name;
    csString text;
    //whether or not to display the 'edit' button
    bool canWrite;
    //to identify which item this is later
    int slotID;
    int containerID;
};

//--------------------------------------------------------------------------

class psQuestRewardMessage : public psMessageCracker
{
public:
    enum
    {
        offerRewards,
        selectReward
    };

    psQuestRewardMessage(uint32_t clientnum, csString& newValue, uint8_t type)
    {
        msg.AttachNew(new MsgEntry( newValue.Length() + 1 + sizeof(uint8_t) ));
        msg->SetType(MSGTYPE_QUESTREWARD);
        msg->clientnum  = clientnum;
        msg->Add(newValue);
        msg->Add(type);
    }

    psQuestRewardMessage(MsgEntry *me)
    {
        newValue    = me->GetStr();
        msgType     = me->GetUInt8();
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int msgType;
    csString newValue;
};

//--------------------------------------------------------------------------

class psExchangeMoneyMsg : public psMessageCracker
{
public:
    psExchangeMoneyMsg( uint32_t client, int container,
                        int trias, int hexas, int circles,int octas );

    psExchangeMoneyMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int container;

    int trias;
    int octas;
    int hexas;
    int circles;
};

/** A request to start an exchange with your current target
  */
class psExchangeRequestMsg : public psMessageCracker
{
public:
    /** Request to start an exchange.
      * This is created on the client and is for the server.
      */
    psExchangeRequestMsg( bool withPlayer );

    /// From the server to the client.
    psExchangeRequestMsg( uint32_t client, csString& name, bool withPlayer );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);


    psExchangeRequestMsg( MsgEntry* me );

    csString player;
    bool withPlayer;
};


/** Notification of an item added to an exchange.  This should only be from
  * server to client.  It is a message to update the Exchange window.
  */
class psExchangeAddItemMsg : public psMessageCracker
{
public:
    psExchangeAddItemMsg(   uint32_t clientNum,
                            const csString& name,
                            int containerID,
                            int slot,
                            int stackcount,
                            const csString& icon );

    psExchangeAddItemMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString name;
    int container;
    int slot;
    int stackCount;
    csString icon;
};

//------------------------------------------------------------------------------

/** Notification of an item removed from an exchange.  This should only be from
  * server to client.  It is a message to update the Exchange window.
  */
class psExchangeRemoveItemMsg : public psMessageCracker
{
public:
    psExchangeRemoveItemMsg( uint32_t client, int container, int slot, int newStack );
    psExchangeRemoveItemMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int container;          // CONTAINER_EXCHANGE_*
    int slot;
    int newStackCount;
};

//------------------------------------------------------------------------------

class psExchangeAcceptMsg : public psMessageCracker
{
public:
    psExchangeAcceptMsg( uint32_t client = 0);
    psExchangeAcceptMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//------------------------------------------------------------------------------

class psExchangeStatusMsg : public psMessageCracker
{
public:
    psExchangeStatusMsg( uint32_t client, bool playerAccept, bool otherPlayerAccept );
    psExchangeStatusMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    bool playerAccept;
    bool otherAccept;
};
//------------------------------------------------------------------------------


class psExchangeEndMsg : public psMessageCracker
{
public:
    psExchangeEndMsg( uint32_t client = 0 );
    psExchangeEndMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};


//------------------------------------------------------------------------------

class psUpdateObjectNameMessage : public psMessageCracker
{
public:
    psUpdateObjectNameMessage(uint32_t client, EID eid, const char* newName);
    psUpdateObjectNameMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    EID objectID;
    csString newObjName;
};

class psUpdatePlayerGuildMessage : public psMessageCracker
{
public:
    psUpdatePlayerGuildMessage(uint32_t client, int total,  const char* newGuild);
    psUpdatePlayerGuildMessage(uint32_t client, EID entity, const char* newGuild); // shortcut for only 1 entity
    psUpdatePlayerGuildMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    void AddPlayer(EID id); // Adds an object

    csArray<uint32_t> objectID; // Array with objects
    csString newGuildName;
};


class psUpdatePlayerGroupMessage : public psMessageCracker
{
public:
    psUpdatePlayerGroupMessage(int clientnum, EID objectID, uint32_t groupID);
    psUpdatePlayerGroupMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    EID objectID;
    uint32_t groupID;
};

//------------------------------------------------------------------------------

/** Used to check to see if a name chosen is a valid name to be picked from.
 */
class psNameCheckMessage : public psMessageCracker
{
public:
    psNameCheckMessage(){}
    psNameCheckMessage( const char* name );
    psNameCheckMessage( uint32_t client, bool accepted, const char* reason );
    psNameCheckMessage( MsgEntry* me );

    void FromClient( MsgEntry* me );
    void FromServer( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString firstName;
    csString lastName;
    csString reason;
    bool accepted;

private:
    bool msgFromServer;
};

//-----------------------------------------------------------------------------

#define PINGFLAG_REQUESTFLAGS    0x0001
#define PINGFLAG_READY           0x0002
#define PINGFLAG_HASBEENREADY    0x0004
#define PINGFLAG_SERVERFULL      0x0008

class psPingMsg : public psMessageCracker
{
public:
    psPingMsg( uint32_t client, uint32_t id, uint8_t flags );
    psPingMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    uint32_t id;
    uint8_t flags;
};

//-----------------------------------------------------------------------------

class psHeartBeatMsg : public psMessageCracker
{
public:
    psHeartBeatMsg( uint32_t client );
    psHeartBeatMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//-----------------------------------------------------------------------------

class psLockpickMessage : public psMessageCracker
{
public:
    psLockpickMessage( const char* password ); // Password is for future use
    psLockpickMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString password;
};

//-----------------------------------------------------------------------------

class psGMSpawnItems : public psMessageCracker
{
public:
    psGMSpawnItems( uint32_t client,const char* type,unsigned int size );
    psGMSpawnItems( const char* type);
    psGMSpawnItems( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    bool request;

    struct Item
    {
        csString name;
        csString mesh;
        csString icon;
    };

    csArray<Item> items;

    csString type;
};

class psGMSpawnTypes : public psMessageCracker
{
public:
    psGMSpawnTypes( uint32_t client,unsigned int size );
    psGMSpawnTypes( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csArray<csString> types;
};

class psGMSpawnItem : public psMessageCracker
{
public:
    psGMSpawnItem(
        const char* item,
        unsigned int count,
        bool lockable,
        bool locked,
        const char* lskill,
        int lstr,
        bool pickupable,
    bool collidable,
    bool random = false,
    float quality = 0.0f);

    psGMSpawnItem( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString item;
    unsigned int count;
    bool lockable,locked,pickupable,collidable;

    csString lskill;
    int lstr;
    bool random;
    float quality;
};

class psLootRemoveMessage : public psMessageCracker
{
public:
    psLootRemoveMessage( uint32_t client,int item );
    psLootRemoveMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int id;
};

class psCharCreateTraitsMessage : public psMessageCracker
{
public:
    psCharCreateTraitsMessage( uint32_t client, csString& string)
    {
        msg.AttachNew(new MsgEntry( string.Length()+1 ));

        msg->SetType(MSGTYPE_CHAR_CREATE_TRAITS);
        msg->clientnum = client;
        msg->Add( string );
    }

    psCharCreateTraitsMessage( csString& string)
    {
        msg.AttachNew(new MsgEntry( string.Length()+1 ));

        msg->SetType(MSGTYPE_CHAR_CREATE_TRAITS);
        msg->clientnum = 0;
        msg->Add( string );
    }

    psCharCreateTraitsMessage( MsgEntry* me )
    {
        string  = me->GetStr();
        if (!string.Length())
            valid = false;
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    const csString& GetString() const
    {
        return string;
    }

private:
    csString string;
};

class psClientStatusMessage : public psMessageCracker
{
public:
    psClientStatusMessage(bool ready);
    psClientStatusMessage(MsgEntry* me);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    bool ready;

    enum ClientStatus
    {
        READY = 0x1
    };
};

class psMoveModMsg : public psMessageCracker
{
public:
    enum ModType
    {
        NONE = 0,   ///< Reset modifiers
        ADDITION,   ///< Add this to movements
        MULTIPLIER, ///< Multiply this with movements
        CONSTANT,   ///< Add this to velocity until told otherwise
        PUSH        ///< Execute this movement once
    };

    psMoveModMsg(uint32_t client, ModType type, const csVector3& move, float Yrot);
    psMoveModMsg(MsgEntry* me);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    ModType type;
    csVector3 movementMod;
    float rotationMod;
};

class psMsgRequestMovement : public psMessageCracker
{
public:
    psMsgRequestMovement();
    psMsgRequestMovement(MsgEntry* me);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

class psMovementInfoMessage : public psMessageCracker
{
public:
    psMovementInfoMessage(size_t modes, size_t moves);
    psMovementInfoMessage(MsgEntry* me);

    PSF_DECLARE_MSG_FACTORY();

    void AddMode(uint32 id, const char* name, csVector3 move_mod, csVector3 rotate_mod, const char* idle_anim);
    void AddMove(uint32 id, const char* name, csVector3 base_move, csVector3 base_rotate);

    void GetMode(uint32 &id, const char* &name, csVector3 &move_mod, csVector3 &rotate_mod, const char* &idle_anim);
    void GetMove(uint32 &id, const char* &name, csVector3 &base_move, csVector3 &base_rotate);

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    // Count of each object
    size_t modes, moves;
};

//-----------------------------------------------------------------------------

/** Defines all the info about crafting for an item.
 */

//-----------------------------------------------------------------------------

class psMsgCraftingInfo : public psMessageCracker
{
public:
    psMsgCraftingInfo()
    {
        msg.AttachNew(new MsgEntry( 1 ));
        msg->SetType(MSGTYPE_CRAFT_INFO);
    }

    psMsgCraftingInfo( uint32_t client, csString craftinfo )
    {
        msg.AttachNew(new MsgEntry( craftinfo.Length()+1 ));

        msg->SetType(MSGTYPE_CRAFT_INFO);
        msg->clientnum = client;
        msg->Add( craftinfo.GetData() );
    }

    psMsgCraftingInfo( MsgEntry* me )
    {
        craftInfo = me->GetStr();
        if (!craftInfo.Length())
            valid = false;
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString craftInfo;
};

//-----------------------------------------------------------------------------

/** Message to change a character's trait.
  * Used to change things like hair colour.
  */
class psTraitChangeMessage : public psMessageCracker
{
public:
    psTraitChangeMessage(uint32_t client, EID targetID, csString & string)
    {
        msg.AttachNew(new MsgEntry( string.Length()+1 + sizeof(uint32_t) ));

        msg->SetType(MSGTYPE_CHANGE_TRAIT);
        msg->clientnum = client;
        msg->Add(targetID.Unbox());
        msg->Add( string );
    }

    psTraitChangeMessage( MsgEntry* me )
    {
        target = EID(me->GetUInt32());
        string  = me->GetStr();
        if (!string.Length())
            valid = false;
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    EID target;
    csString string;
};

//-----------------------------------------------------------------------------

/**
 *  Class to send instructions from server to client on a tutorial event.
 */
class psTutorialMessage : public psMessageCracker
{
public:
    psTutorialMessage( uint32_t client, uint32_t which, const char *instructions)
    {
        msg.AttachNew(new MsgEntry( sizeof(uint32_t) + strlen(instructions)+1 ));

        msg->SetType(MSGTYPE_TUTORIAL);
        msg->clientnum = client;
        msg->Add( which );
        msg->Add( instructions );
    }

    psTutorialMessage( MsgEntry* me )
    {
        whichMessage = me->GetUInt32();
        instrs = me->GetStr();
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int whichMessage;
    csString instrs;
};

/**
 *  Class to send client directions on how to render a Sketch.
 */
class psSketchMessage : public psMessageCracker
{
public:
    psSketchMessage( uint32_t client, uint32_t itemID, uint8_t flags, const char *limitxml,const char *sketch_def, bool rightToEditFlag, const char *sketch_name)
    {
        msg.AttachNew(new MsgEntry( sizeof(uint32_t)+1+strlen(limitxml)+1+strlen(sketch_def)+1+sizeof(bool)+strlen(sketch_name)+1 ));

        msg->SetType(MSGTYPE_VIEW_SKETCH);
        msg->clientnum = client;
        msg->Add(itemID);
        msg->Add(flags);
        msg->Add(limitxml);
        msg->Add( sketch_def );
        msg->Add(rightToEditFlag);
        msg->Add(sketch_name);
    }

    psSketchMessage( MsgEntry* me )
    {
        ItemID = me->GetUInt32();
        Flags  = me->GetUInt8();
        limits = me->GetStr();
        Sketch = me->GetStr();
        rightToEdit = me->GetBool();
        name = me->GetStr();
    }

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs)
    {
        return csString("not implemented");
    }

    uint32_t ItemID;
    uint8_t  Flags;
    csString Sketch;
    csString limits;
    bool rightToEdit;
    csString name;
};

/**
 * Message from the client to start a minigame
 */
class psMGStartStopMessage : public psMessageCracker
{

    public:
        psMGStartStopMessage(uint32_t client, bool start);

        psMGStartStopMessage(MsgEntry *me);

        PSF_DECLARE_MSG_FACTORY();

        /**
         * @brief Converts the message into human readable string.
         *
         * @param access_ptrs A struct to a number of access pointers
         * @return A human readable string for the message
         */
        virtual csString ToString(AccessPointers *access_ptrs);

        /// Indicates that this is a request to start the game
        bool msgStart;
};

/**
 * Message from the server to setup/update the minigame board on the client side.
 */
class psMGBoardMessage : public psMessageCracker
{

    public:
        psMGBoardMessage(uint32_t client, uint8_t counter,
                         uint32_t gameID, uint16_t options, int8_t cols, int8_t rows, uint8_t *layout,
                         uint8_t numOfPieces, uint8_t *pieces);

        psMGBoardMessage(MsgEntry *me);

        PSF_DECLARE_MSG_FACTORY();

        /**
         * @brief Converts the message into human readable string.
         *
         * @param access_ptrs A struct to a number of access pointers
         * @return A human readable string for the message
         */
        virtual csString ToString(AccessPointers *access_ptrs);

        /// Returns true if this message recent compared to the passed sequence value
        bool IsNewerThan(uint8_t oldCounter);

        /// Message counter for versioning
        uint8_t msgCounter;

        /// Game ID (from action location ID).
        uint32_t msgGameID;

        /// Game options
        uint16_t msgOptions;

        /// Number of columns
        int8_t msgCols;

        /// Number of rows
        int8_t msgRows;

        /**
         * Array with the game board layout.
         *
         * Every byte in this array defines two game tiles.
         * 0 - Empty
         * 1..14 - Game pieces
         * 15 - Disabled
         */
        uint8_t *msgLayout;

        /// Number of available pieces.
        uint8_t msgNumOfPieces;

        /// Available pieces (both white and black).
        uint8_t *msgPieces;

};

/**
 * Message from the client with a minigame board update.
 */
class psMGUpdateMessage : public psMessageCracker
{

    public:
        psMGUpdateMessage(uint32_t client, uint8_t counter,
                          uint32_t gameID, uint8_t numUpdates, uint8_t *updates);

        psMGUpdateMessage(MsgEntry *me);

        PSF_DECLARE_MSG_FACTORY();

        /**
         * @brief Converts the message into human readable string.
         *
         * @param access_ptrs A struct to a number of access pointers
         * @return A human readable string for the message
         */
        virtual csString ToString(AccessPointers *access_ptrs);

        /// Returns true if this message is newer than the passed sequence value
        bool IsNewerThan(uint8_t oldCounter);

        /// Message counter for versioning
        uint8_t msgCounter;

        /// Game ID (from the action location ID)
        uint32_t msgGameID;

        /// Number of updates in this message
        uint8_t msgNumUpdates;

        /**
         * Array with updates for the game board.
         *
         * Updates are packed into two sequential bytes. The first byte defines the column and row,
         * the second byte defines the game tile.
         */
        uint8_t *msgUpdates;
};

/**
 * Message from the server to handle entrances into and out of map instances.
 */
class psEntranceMessage : public psMessageCracker
{

    public:
        /**
         * @brief Constructor for message for entering.
         *
         * @param gem entity ID
         */
        psEntranceMessage(EID entranceID);

        psEntranceMessage( MsgEntry* me );

        PSF_DECLARE_MSG_FACTORY();

        /**
         * @brief Converts the message into human readable string.
         *
         * @param access_ptrs A struct to a number of access pointers
         * @return A human readable string for the message
         */
        virtual csString ToString(AccessPointers *access_ptrs);

        /// The gem entity ID of the entrance object
        EID entranceID;
};

//--------------------------------------------------------------------------

/** GM Event List Message.
 *
 * This message is used to manage the GM-Event page in the Quest window.
 * The client requests the data. The Server will send the appropriate Event
 * description. If it is from the GM running the event, the involved players
 * are sent too.
 */
class psGMEventListMessage : public psMessageCracker
{
public:

    psGMEventListMessage();
    psGMEventListMessage(MsgEntry *me);

    PSF_DECLARE_MSG_FACTORY();

    void Populate(csString& gmeventStr, int clientnum);

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString gmEventsXML;
};

//---------------------------------------------------------------------------

/// Sends messages to the client listing the assigned events for the player.
class psGMEventInfoMessage : public psMessageCracker
{
public:
    enum
    {
        CMD_QUERY,   ///< command to request the list of events
        CMD_INFO,    ///< command to get informations on the specific event (description, evaluatable status)
        CMD_DISCARD, ///< command to discard an event
        CMD_EVAL     ///< command to send an evaluation of the event
    };
    psGMEventInfoMessage(int cnum, int cmd, int id, const char *name,const char *info, bool Evaluatable = false);
    psGMEventInfoMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int command,id;
    bool Evaluatable; ///< Stores if an event can be evaluated
    csString xml;     ///< Stores an xml string (various uses)
};



/** Faction Message to get faction info from the server.  This is used to
 *   either get the full faction information or any updated faction info.
 */
class psFactionMessage : public psMessageCracker
{
public:
    // Structure to hold faction name/faction value pairs.
    struct FactionPair
    {
        csString faction;       // Name of the faction.
        int      rating;        // Rating with that faction.
    };

    /// The different commands that this message can be.
    enum
    {
        MSG_FULL_LIST,          // Data is the full faction list.
        MSG_UPDATE              // Data is an update faction list.
    };

   /** @brief Construct the faction message.
     * @param cnum The client destination for the message.
     * @param cmd  What the data inside will be. [ MSG_FULL_LIST | MSG_UPDATE ]
     */
    psFactionMessage(int cnum, int cmd);

    /** @brief Adds a faction to the list to deliver.
     *  @param factionName The name of the faction.
     *  @param rating The rating with that faction.
     */
    void AddFaction( csString factionName, int rating );

    /// Build the messasge to prepare for it to be sent.
    void BuildMsg();

    /// Parse incoming messages.
    psFactionMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();
    virtual csString ToString(AccessPointers * access_ptrs);

    int cmd;                                /// The command type of message.
    int client;                             /// Destination client.
    csPDelArray<FactionPair> factionInfo;       /// Faction details.
};

//---------------------------------------------------------------------------

/// Sends messages to the client to control sequences
class psSequenceMessage : public psMessageCracker
{
public:
    enum // Should use same values as in the NPC client SequenceOperation
    {
        CMD_START = 1,
        CMD_STOP = 2,
        CMD_LOOP = 3
    };
    psSequenceMessage(int cnum, const char * name, int cmd, int count);
    psSequenceMessage(MsgEntry* message);

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    csString name;
    int command,count;
};


/**  Sound Message
*
* This message is used to tell the client to play a sound. The
* client psSoundManager and various server managers will deal with
* this messages. Specific sound events with additional data should
* inherit from this message.
*/
class psPlaySoundMessage : public psMessageCracker
{
public:
    csString sound;
    /** Create psMessageBytes struct for outbound use */
    psPlaySoundMessage(uint32_t clientnum, csString snd);

    /** Crack incoming psMessageBytes struct for inbound use */
    psPlaySoundMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();
    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};

//-----------------------------------------------------------------------------

/** The message sent from client to server to request a cp value for
 *  creation.
 */
class psCharCreateCPMessage : public psMessageCracker
{
public:
    psCharCreateCPMessage(  uint32_t client, int32_t rID, int32_t CPVal );
    psCharCreateCPMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

    int32_t raceID;
    int32_t CPValue;
};

//-----------------------------------------------------------------------------

/** The message sent from client to server to request a new introduction
 */

class psCharIntroduction : public psMessageCracker
{
public:
    psCharIntroduction( );
    psCharIntroduction( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);
};


/**
 *  Class to send a possibly cached file to the client.
 */
class psCachedFileMessage : public psMessageCracker
{

public:
	csString hash;
	csRef<iDataBuffer> databuf;

	psCachedFileMessage( uint32_t client, uint8_t sequence, const char *pathname, iDataBuffer *contents);
    psCachedFileMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs)
    {
        return csString("not implemented");
    }

};

//-----------------------------------------------------------------------------

/** The message sent from server to client when a menu of possible responses is available
 */

class psDialogMenuMessage : public psMessageCracker
{
public:
	csString xml;

    psDialogMenuMessage();
    psDialogMenuMessage( MsgEntry* message );

    PSF_DECLARE_MSG_FACTORY();


	void BuildMsg(int clientnum);

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);

	struct DialogResponse
	{
		uint32_t id;
		csString menuText;
		csString triggerText;
		uint32_t flags;
	};

	void AddResponse( uint32_t id, const csString& menuText, const csString& triggerText, 
		              const csString& playerName, const csString& playerRace, const csString& honorific,
									  const csString& possessive, uint32_t flags = 0x00 );

	csArray<DialogResponse> responses;
};

/**
 *  Class to send a single arbitrary string to the client or server.
 */
class psSimpleStringMessage : public psMessageCracker
{

public:
	csString str;

	psSimpleStringMessage( uint32_t client,MSG_TYPES type, const char *string);
    psSimpleStringMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * @brief Converts the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs)
    {
        return csString("not implemented");
    }

};

/**
*  Class to implement sequential delivery of net messages
*/
class psOrderedMessage : public psMessageCracker
{

public:
	int value;

	psOrderedMessage( uint32_t client, int valueToSend, int sequenceNumber);
	psOrderedMessage( MsgEntry* me );

	PSF_DECLARE_MSG_FACTORY();

	/**
	* @brief Converts the message into human readable string.
	*
	* @param access_ptrs A struct to a number of access pointers.
	* @return Return a human readable string for the message.
	*/
	virtual csString ToString(AccessPointers * access_ptrs)
	{
		return csString("not implemented");
	}

};


#endif
