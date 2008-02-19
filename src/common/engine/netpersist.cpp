/*
 * netpersist.cpp by Matze Braun <MatzeBraun@gmx.de>
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
 */
#include <psconfig.h>

#include <iutil/objreg.h>
#include <physicallayer/entity.h>
#include <physicallayer/persist.h>
#include <physicallayer/propclas.h>
#include <physicallayer/pl.h>
#include <propclass/inv.h>

#include "engine/netpersist.h"
#include "engine/celbase.h"
#include "net/msghandler.h"
#include "net/messages.h"
#include "util/psconst.h"

/*
 * DOES THIS STILL WORK?!
 */

// Define for debugging printfs.  You probably want this on for development, off for server.
//#define PSNETPERSISTDEBUG

#ifdef PSNETPERSISTDEBUG
psCelPersistMessage::psCelPersistMessage(uint32_t clientnum, uint16_t cmd, int prty,
    uint32_t id, csMemFile* memfile, const char *extra_str) : memfile(NULL)
{
    if (!extra_str)
    extra_str.Clear();

    size_t size = sizeof(cmd) + sizeof(id) + (extra_str!=NULL ? strlen(extra_str)+1 : 1)
    + sizeof(uint32_t) + (memfile ? memfile->GetSize() : 0);

    msg.AttachNew(new MsgEntry(size));

    msg->data->type = MSGTYPE_CELPERSIST;
    msg->clientnum  = clientnum;
    msg->priority   = prty;

    msg->Add(cmd);
    msg->Add(id);
    msg->Add(extra_str);

    if (memfile==NULL)
    {
        // Must always add the null data block; this makes the other side behave uniformly.
        msg->Add(NULL, 0);
    } 
    else
    {
        msg->Add(memfile->GetData(), memfile->GetSize());
    }
    if (msg->overrun)
        valid=false;
}

psCelPersistMessage::psCelPersistMessage(MsgEntry* message)
{   
    valid=true;

    cmd = message->GetInt16();
    id =  message->GetInt32();
    extra_str = message->GetStr();

    uint32_t datalength;
    void *dataptr = message->GetData(&datalength);
    // Make sure we haven't overrun
    if (message->overrun)
    {
        memfile=NULL;
        valid=false;
        return;
    }

    memfile = new csMemFile((const char *)dataptr, datalength);
}

psCelPersistMessage::~psCelPersistMessage()
{
}

#endif
