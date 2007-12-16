/*
 * msgmanager.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __MSGMANAGER_H__
#define __MSGMANAGER_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/subscriber.h"         // Subscriber class

//=============================================================================
// Local Includes
//=============================================================================


/// These flags define the tests that are centrally done 
/// before subclasses get the message.
#define NO_VALIDATION        0x00
#define REQUIRE_ANY_CLIENT   0x01
#define REQUIRE_READY_CLIENT 0x02
#define REQUIRE_ALIVE        0x04
#define REQUIRE_TARGET       0x08
#define REQUIRE_TARGETACTOR  0x10
#define REQUIRE_TARGETNPC    0x20

class Client;
class MsgEntry;

class MessageManager : public iNetSubscriber
{
public:
    virtual ~MessageManager() {}

    virtual bool Verify(MsgEntry *pMsg,unsigned int flags,Client*& client);

    /** Finds Client* of character with given name. */
    Client * FindPlayerClient(const char *name);
};

#endif
