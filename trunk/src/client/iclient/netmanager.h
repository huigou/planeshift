/*
 * netmanager.h by Matze Braun <MatzeBraun@gmx.de>
 *
 * Copyright (C) 2001 PlaneShift Team (info@planeshift.it, 
 * http://www.planeshift.it)
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
#ifndef __I_NETMANAGER_H__
#define __I_NETMANAGER_H__

#include <csutil/scf.h>

SCF_VERSION(iNetManager, 0, 0, 1);

class MsgHandler;
class CmdHandler;

struct iNetManager : public iBase
{
    virtual bool Connect(const char* server, int port) = 0;
    virtual void Disconnect() = 0;
    virtual void SendDisconnect(bool final = true) = 0;
    virtual void Authenticate(const csString name, const csString pwd ) = 0;

    virtual MsgHandler* GetMsgHandler() = 0;
    virtual CmdHandler* GetCmdHandler() = 0;
    virtual const char* GetLastError() = 0;
    virtual const char* GetAuthMessage() = 0;

    virtual bool IsConnected() = 0;

    enum
    {
    EXISTING_PLAYER = 0,
    NEW_PLAYER = 1
    };
};

#endif

