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

/* Implementation of a generic queue (alias fifo)
 * This is a generic queue implementation using templates
 * with some additional locking facilties for multithreading
 */
#ifndef __GENQUEUE_H__
#define __GENQUEUE_H__

#include <csutil/ref.h>
#include <csutil/threading/mutex.h>
#include <csutil/threading/condition.h>

/**
 * A queue class with some locking facilties for multi threading
 * You can only put classes in here that are derived from RefCountBase
 */
template <class queuetyp>
class GenericQueue
{
public:
    GenericQueue(unsigned int maxsize=500);
    GenericQueue(GenericQueue& other);
    ~GenericQueue();

    /** This adds a message to the queue */
    bool Add (queuetyp* msg);

    /**
     * This gets the next message from the queue, it is then removed from
     * the queue. Note: It returns a pointer to the message, so a null
     * pointer indicates an error
     */
    csPtr<queuetyp> Get();

    /** like above, but waits for the next message, if the queue is empty */
    csPtr<queuetyp> GetWait(csTicks timeout = 0);

    /**
     * This function interrupt the queue if it is waiting.
     */
    void Interrupt();
    
protected:
    /** Note: marked as deprecated
     * return true if queue is empty. Attention: Don't assume this function
     * is theadsafe, It may be possible that the queue isn't full/empty
     * anymore directly after your call.
     */
    bool IsEmpty() 
    {
        return qstart == qend;
    }
    
    /** Note: marked as deprecated
     * return true, if queue is full (max size). Attention: Don't assume this
     * function is threadsafe. It may be possible that the queue isn't
     * full/notfull anymore directly after your call.
     */
    bool IsFull()
    {
    return ((qend + 1) %  qsize) == qstart;
    }
    
protected:
    queuetyp **qbuffer;
    unsigned int qstart, qend;
    unsigned int qsize;
    CS::Threading::RecursiveMutex mutex;
    CS::Threading::Condition datacondition;
};

/* We have to include template code each time the class is used :( */
#include "genqueue.cpp"

#endif
