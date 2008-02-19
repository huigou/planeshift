/*
* genqueue.h by Matze Braun <MatzeBraun@gmx.de>
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

#ifndef __GENQUEUE_H__
#define __GENQUEUE_H__

#include <csutil/ref.h>
#include <csutil/threading/mutex.h>
#include <csutil/threading/condition.h>

#include "util/pserror.h"

/**
* A queue of smart pointers with locking facilties
* for multi-threading. The objects in the queue must
* implement reference counting.
*/
template <class queuetype>
class GenericRefQueue
{
public:
    GenericRefQueue<queuetype>::GenericRefQueue(unsigned int maxsize = 500)
    {
        /* we make the buffer 1 typ bigger, so we can avoid one check and one
        variable when testing if buffer is full */
        maxsize++;
        qbuffer = new queuetype* [maxsize];
        if (!qbuffer)
            ERRORHALT ("No Memory");
        qstart = qend = 0;
        qsize = maxsize;
    }

    GenericRefQueue<queuetype>::~GenericRefQueue()
    {
        delete []qbuffer;
    }

    /** This adds a message to the queue */
    bool GenericRefQueue<queuetype>::Add(queuetype* msg)
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

            msg->SetPending(true);

            msg->IncRef(); 

            Interrupt();
        }
        return true;
    }

    /**
    * This gets the next message from the queue, it is then removed from
    * the queue. Note: It returns a pointer to the message, so a null
    * pointer indicates an error
    */
    csPtr<queuetype> GenericRefQueue<queuetype>::Get()
    {
        CS::Threading::RecursiveMutexScopedLock lock(mutex);

        // check if queue is empty
        if (qstart == qend)
            return 0;

        // removes Message from queue
        csRef<queuetype> ptr = *(qbuffer + qstart);
        qstart = (qstart + 1) % qsize;

        ptr->SetPending(false);

        ptr->DecRef();

        return csPtr<queuetype>(ptr);
    }

    /** like above, but waits for the next message, if the queue is empty */
    csPtr<queuetype> GenericRefQueue<queuetype>::GetWait(csTicks timeout)
    {
        // is there's a message in the queue left just return it
        CS::Threading::RecursiveMutexScopedLock lock(mutex);
        csRef<queuetype> temp = Get();
        if (temp)
        {
            return csPtr<queuetype> (temp);
        }

        // Wait release mutex before waiting so that it is possible to
        // add new messages.
        if (!datacondition.Wait(mutex, timeout))
        {
            // Timed out waiting for new message
            return 0;
        }

        // check if queue is empty
        if (qstart == qend)
            return 0;

        // removes Message from queue
        csRef<queuetype> ptr = *(qbuffer + qstart);
        qstart = (qstart + 1) % qsize;

        ptr->SetPending(false);

        ptr->DecRef();

        return csPtr<queuetype>(ptr);
    }

    /**
    * This function interrupt the queue if it is waiting.
    */
    void GenericRefQueue<queuetype>::Interrupt()
    {
        datacondition.NotifyOne();
    }

protected:
    queuetype **qbuffer;
    unsigned int qstart, qend;
    unsigned int qsize;
    CS::Threading::RecursiveMutex mutex;
    CS::Threading::Condition datacondition;
};

#endif
