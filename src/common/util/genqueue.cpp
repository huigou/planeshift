/*
 * genqueue.cpp by Matze Braun <MatzeBraun@gmx.de>
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
#ifndef __GENQUEUE_CPP__
#define __GENQUEUE_CPP__

#ifndef __GENQUEUE_H__
#   include <psconfig.h>
#   include "util/genqueue.h"
#endif

#include "util/pserror.h"

template <class queuetyp>
GenericQueue<queuetyp>::GenericQueue (unsigned int maxsize)
{
    /* we make the buffer 1 typ bigger, so we can avoid one check and one
       variable when testing if buffer is full */
    maxsize++;
    qbuffer = new queuetyp* [maxsize];
    if (!qbuffer)
    ERRORHALT ("No Memory");
    qstart = qend = 0;
    qsize = maxsize;
}

template <class queuetyp>
GenericQueue<queuetyp>::GenericQueue(GenericQueue& other)
{
    CS::Threading::RecursiveMutexScopedLock lock(other.mutex);
    
    qsize = other.qsize;
    qbuffer = new queuetyp* [qsize];
    memcpy (qbuffer, other.qbuffer, qsize * sizeof(queuetyp*));
    if (!qbuffer)
    ERRORHALT ("No Memory");
    qstart = other.qstart;
    qend = other.qend;
}

template <class queuetyp>
GenericQueue<queuetyp>::~GenericQueue()
{
    delete []qbuffer;
}

template <class queuetyp>
bool GenericQueue<queuetyp>::Add (queuetyp* msg)
{
    unsigned int tqend;

    CS::Threading::RecursiveMutexScopedLock lock(mutex);

    if (!msg->GetPending())
    {
        tqend = (qend + 1) % qsize;
        // check if queue is full
        if (tqend == qstart)
        {
            return false;
        }
          
        // add Message to queue
        qbuffer[qend]=msg;
        qend=tqend;

        msg->IncRef();
        msg->SetPending(true);

        Interrupt();
    }
    return true;
}

template <class queuetyp>
csPtr<queuetyp> GenericQueue<queuetyp>::Get ()
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);

    // check if queue is empty
    if (IsEmpty())
        return 0;
    
    // removes Message from queue
    queuetyp* ptr = *(qbuffer + qstart);
    qstart = (qstart + 1) % qsize;
    
    ptr->SetPending(false);
    return ptr;
}

template <class queuetyp>
csPtr<queuetyp> GenericQueue<queuetyp>::GetWait(csTicks timeout)
{
    // is there's a message in the queue left just return it
    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    csRef<queuetyp> temp = Get();
    if (temp)
    {
        return csPtr<queuetyp> (temp);
    }
    
    // Wait release mutex before waiting so that it is possible to
    // add new messages.
    if (!datacondition.Wait(mutex, timeout))
    {
        // Timed out waiting for new message
        return 0;
    }
    
    // check if queue is empty
    if (IsEmpty())
        return 0;
    
    // removes Message from queue
    queuetyp* ptr = *(qbuffer + qstart);
    qstart = (qstart + 1) % qsize;
    
    ptr->SetPending(false);
    
    return ptr;
}

template <class queuetyp>
void GenericQueue<queuetyp>::Interrupt()
{
    datacondition.NotifyOne();
}

#endif
