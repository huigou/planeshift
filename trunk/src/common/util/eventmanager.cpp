/*
* eventmanager.cpp
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
#include <psconfig.h>

#include "gameevent.h"
#include "util/consoleout.h"

#include "eventmanager.h"

// Number of recent events to use when calculating moving average
#define EVENT_AVERAGETIME_COUNT 50

/*---------------------------------------------------------------------------*/

EventManager::EventManager(csRef<CS::Threading::Thread> _thread) {
    // Setting up the static pointer in psGameEvent. Used so
    // that an event can be fired without needing to look up 
    // the event manager first.
    lastTick = 0;
    psGameEvent::eventmanager = this;
    
    thread = _thread;
}

EventManager::EventManager()
{
    // Setting up the static pointer in psGameEvent. Used so
    // that an event can be fired without needing to look up 
    // the event manager first.
    lastTick = 0;
    psGameEvent::eventmanager = this;
}

EventManager::~EventManager()
{
    // Clean up the event queue
    while (eventqueue.Length())
    {
        delete eventqueue.DeleteMin();
    }
}


class ServerStarter : public CS::Threading::Runnable
{
public:
    csRef<EventManager> eventManager;
    csRef<CS::Threading::Thread> thread;
    NetBase* netBase;
    CS::Threading::Mutex doneMutex;
    CS::Threading::Condition initDone;

    int queuelen;
    
    bool success;
    
    ServerStarter(NetBase* _netBase, int _queuelen)
    {
        netBase = _netBase;
        queuelen = _queuelen;
    }

    void Run()
    {
        {
            CS::Threading::MutexScopedLock lock (doneMutex);
            // construct the netManager is its own thread to avoid wrong warnings of dynamic thread checking via valgrind
            eventManager.AttachNew(new EventManager(thread));
            if (!eventManager->Initialize(netBase, queuelen))
            {
                success = false;
                initDone.NotifyAll();
                return;
            }
            success = true;
            initDone.NotifyAll();
        }

        /* run the network loop */
        eventManager->Run();
    }
};



csRef<EventManager> EventManager::Create(NetBase* netBase, int queuelen)
{
    csRef<ServerStarter> serverStarter;
    serverStarter.AttachNew (new ServerStarter(netBase, queuelen));
    csRef<CS::Threading::Thread> thread;
    thread.AttachNew (new CS::Threading::Thread (serverStarter));
    serverStarter->thread = thread;
    // wait for initialization to be finished
    {
        CS::Threading::MutexScopedLock lock (serverStarter->doneMutex);
        thread->Start();   
        if (!thread->IsRunning()) {
            return NULL;        
        }
        serverStarter->initDone.Wait(serverStarter->doneMutex);
    }
    return serverStarter->eventManager;
}

void EventManager::Push(psGameEvent *event)
{
    CS::Threading::MutexScopedLock lock(mutex);

    // This inserts the event into the priority queue, sorted by timestamp
    eventqueue.Insert(event);

    /*check if events are inserted late*/
    if (event->triggerticks < lastTick)
    {
        // yelp and adjust lastTick to avoid wrong warning
        CPrintf(
                CON_DEBUG,
                "Event %d scheduled at %d is being inserted late. Last processed event was scheduled for %d.\n",
                event->id, event->triggerticks, lastTick);
        lastTick = event->triggerticks;
    }
}

// Process events at least every 250 tick
#define PROCESS_EVENT   250

csTicks EventManager::ProcessEventQueue()
{
    csTicks now = csGetTicks();

    static int lastid;

    psGameEvent *event = NULL;
    int count = 0;

    while (true)
    {

        {
            CS::Threading::MutexScopedLock lock(mutex);

            event = eventqueue.FindMin();

            if (!event || event->triggerticks > now)
            {
                // Empty event queue or not time for event yet
                break;
            }
            eventqueue.DeleteMin();

            /*check if events arrive in order*/
            if (event->triggerticks < lastTick)
            {
                /* this should not happen at all */
                CPrintf(
                        CON_DEBUG,
                        "Event %d scheduled at %d is being processed out of order at time %d! Last processed event was scheduled for %d.\n",
                        event->id, event->triggerticks, now, lastTick);
            }
            else
            {
                lastTick = event->triggerticks;
            }

        }
        

        csTicks start = csGetTicks();

        if (event->CheckTrigger())
        {
            event->Trigger();
        }

        csTicks timeTaken = csGetTicks() - start;

        if(timeTaken > 1000)
        {
            csString status;
            status.Format("Event type %s:%s has taken %u time to process\n", event->GetType(), 
                          event->ToString().GetDataSafe(),timeTaken);
            CPrintf(CON_WARNING, "%s\n", status.GetData());
            if(LogCSV::GetSingletonPtr())
                LogCSV::GetSingleton().Write(CSV_STATUS, status);
        }

        if (lastid == event->id)
        {
            CPrintf(CON_DEBUG, "Event %d is being processed more than once at time %d!\n",event->id,event->triggerticks);
        }
        lastid    = event->id;

        delete event;

        count++;
        if (count == 100)
        {
            CPrintf(CON_DEBUG, "Went through event loop 100 times in one timeslice.  This means we either have duplicate events, "
                "bugs in event generation or bugs in deleting events from the event tree.\n");
        }
    }

    if (event)
    {
        // We have a event so report when we would like to be
        // called again
        return MIN(event->triggerticks, PROCESS_EVENT + now);
    }

    return PROCESS_EVENT; // Process events at least every PROCESS_EVENT ticks
}



// This is the MAIN GAME thread. Every message and event are handled from
// this thread. This eliminate need for synchronization of access to
// game data.
void EventManager::Run ()
{
    csString status;
    csTicks eventtimes[EVENT_AVERAGETIME_COUNT];
    short index = 0;
    csTicks eventtimesTotal = 0;

    // Have we filled in all the entries in the array yet?
    bool filled = false;

    csRef<MsgEntry> msg = 0;

    csTicks nextEvent = csGetTicks() + PROCESS_EVENT;

    stop_network = false;

    while( !stop_network)
    {
        csTicks now = csGetTicks();
        int timeout = nextEvent - now;

        if (timeout > 0)
        {
            msg = queue->GetWait(timeout);
        }

        if (msg)
        {
            csTicks start = csGetTicks();

            Publish(msg);

            csTicks timeTaken = csGetTicks() - start;

            // Ignore messages that take no time to process.
            if(timeTaken)
            {
                if(!filled)
                    eventtimesTotal += timeTaken;
                else
                    eventtimesTotal = timeTaken + eventtimesTotal - eventtimes[index];

                eventtimes[index] = timeTaken;

                // Done this way to prevent a division operator
                if(filled && timeTaken > 500 && (timeTaken * EVENT_AVERAGETIME_COUNT > 2 * eventtimesTotal || eventtimesTotal > EVENT_AVERAGETIME_COUNT * 1000))
                {
                    status.Format("Message type %u has taken %u time to process, average time of events is %u", msg->GetType(), timeTaken, eventtimesTotal / EVENT_AVERAGETIME_COUNT);
                    CPrintf(CON_WARNING, "%s\n", status.GetData());
                    if(LogCSV::GetSingletonPtr())
                        LogCSV::GetSingleton().Write(CSV_STATUS, status);
                }

                index++;
                // Rollover
                if(index == EVENT_AVERAGETIME_COUNT)
                {
                    index = 0;
                    filled = true;
                }
            }
            // don't forget to release the packet
            msg = NULL;
        }
        else if (now >= nextEvent)
        {
            nextEvent = ProcessEventQueue();
        }
    }
    if(filled)
    {
        status.Format("Event manager shutdown, average time of events is %u", eventtimesTotal / EVENT_AVERAGETIME_COUNT);
        CPrintf(CON_CMDOUTPUT, "%s\n", status.GetData());
        if(LogCSV::GetSingletonPtr())
            LogCSV::GetSingleton().Write(CSV_STATUS, status);
    }
    CPrintf(CON_CMDOUTPUT, "Event thread stopped!\n");
}


class psDelayedMessageEvent : public psGameEvent
{
public:
    psDelayedMessageEvent(csTicks msecDelay, MsgEntry *me, EventManager *myparent)
        : psGameEvent(0,msecDelay,"psDelayedMessageEvent")
    {
        myMsg = me;
        myParent = myparent;
    }
    virtual void Trigger()
    {
        myParent->SendMessage(myMsg);
    }
    virtual csString ToString() const
    {
        csString str;
        str.Format("Delayed message of type : %d" + myMsg->GetType());
        return str;
    }
    

private:
    csRef<MsgEntry> myMsg;
    EventManager *myParent;
};


void EventManager::SendMessageDelayed(MsgEntry *me,csTicks msecDelay)
{
    psDelayedMessageEvent *msg = new psDelayedMessageEvent(msecDelay,me,this);
    Push(msg);
}

