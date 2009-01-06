/*
 * chatmanager.h - Author: Matthias Braun <MatzeBraun@gmx.de>
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
 * This implements a simple chatmanager which resend every chat packet to each
 * client...
 *
 */

#ifndef __CHATMANAGER_H__
#define __CHATMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "util/gameevent.h"

#include "net/messages.h"            // Chat Message definitions

//=============================================================================
// Local Space Includes
//=============================================================================
#include "msgmanager.h"             // parent class

class Client;
class ClientConnectionSet;
class psServer;
class psEndChatLoggingEvent;
class NpcResponse;
class psGuildInfo;
class gemNPC;
class gemActor;


#define CHAT_SAY_RANGE 10

struct CachedData
{
	csString key;
	csString alternate;
	csRef<iDataBuffer> data;

	CachedData(iDataBuffer *buffer, const char *n, const char *alt)
	{
		data = buffer;
		key = n;
		alternate = alt;
	}
};

class ChatManager : public MessageManager
{
public:

    ChatManager();
    virtual ~ChatManager();

    virtual void HandleMessage(MsgEntry *pMsg,Client *client) { }
    void HandleChatMessage (MsgEntry *me, Client *client);
    void HandleCacheMessage(MsgEntry *me, Client *client);

    void SendNotice(psChatMessage& msg);

    NpcResponse *CheckNPCEvent(Client *client,csString& trigger,gemNPC * &target);

    void SendGuild(const csString & sender, psGuildInfo * guild, psChatMessage& msg);

protected:
	csPDelArray<CachedData> audioFileCache;

    void SendTell(psChatMessage& msg, const char* who,Client *from,Client *target);
    void SendSay(uint32_t clientNum, gemActor* actor, psChatMessage& msg, const char* who);
    void SendGuild(Client * client, psChatMessage& msg);
    void SendGroup(Client * client, psChatMessage& msg);
    void SendShout(Client * client, psChatMessage& msg);

    NpcResponse *CheckNPCResponse(psChatMessage& msg,Client *client,gemNPC * &target);

	/// Starts the process of sending the specified file to the client
	void SendAudioFileHash(Client *client, const char *voiceFile);
	/// Sends the actual file to the client if needed
	void SendAudioFile(Client *client, const char *voiceFile);
	
    /// If this returns true, all went well.  If it returns false, the client was muted
    bool FloodControl(csString& newMessage, Client *client);
};


class psEndChatLoggingEvent : public psGameEvent
{
public:
    uint32_t clientnum;

    psEndChatLoggingEvent(uint32_t clientNum, const int delayTicks);

    virtual void Trigger();

};

#endif

