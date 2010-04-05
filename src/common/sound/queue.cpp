/*
 * queue.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com> 
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#include "sound.h"

extern SoundSystemManager  *SndSysMgr;

SoundQueueItem::SoundQueueItem ()
{
    handle      = NULL;
}
SoundQueueItem::~SoundQueueItem ()
{
}

SoundQueue::SoundQueue (SoundControl* &ctrl, float vol)
{
    sndCtrl = ctrl;
    volume = vol;
}

SoundQueue::~SoundQueue ()
{
    // purge on deconstruction
    Purge();
}

/*
 * adds an item to the queue
 * name must be a !unique! filename which exists in our vfs
 */

void
SoundQueue::AddItem (const char *filename)
{
    // NOTE: its a pointer because i need a new Instance of SoundQueueItem
    SoundQueueItem      *newItem;
    
    newItem              = new SoundQueueItem;
    newItem->filename    = csString(filename);   

    queue.Push(newItem);
}

/*
 * it will work on the queue
 * playing one item at a time
 * 
 * Old way to solve this problem was via callback
 * i still like the idea of getting a callback
 * but i want to register my own function for a callback
 * and not a generic one which has to handle *all* callbacks
 * TODO
 */

void
SoundQueue::Work ()
{
    SoundQueueItem  *item;
    
    for (size_t i = 0; i < queue.GetSize(); i++)
    {
        item = queue[i];
        
        if (item->handle == NULL)   
        {
            /* item will be played */
            SndSysMgr->Play2DSound (item->filename, false, 0, 0,
                                    volume, sndCtrl,
                                    item->handle);
            item->handle->SetAutoRemove(false);
            return;
        }
        else if (item->handle->sndstream 
                 ->GetPauseState () != CS_SNDSYS_STREAM_PAUSED)
        {
            /* item is still playing */
            return;
        }
        else if (item->handle->sndstream 
                 ->GetPauseState () == CS_SNDSYS_STREAM_PAUSED)
        {
            /* item is paused. remove it and play the next one */
            item->handle->SetAutoRemove(true);
            queue.Delete(item);
            delete item;
            i--;
        }
    }
}

/*
 * stop and all sounds this queue is playing
 * and purge the queue
 */

void
SoundQueue::Purge ()
{
    for (size_t i = 0; i < queue.GetSize(); i++)
    {
        if (queue[i]->handle != NULL)
        {
            queue[i]->handle->SetAutoRemove(true);
            queue[i]->handle->sndstream->Pause();  
        }

        queue.Delete(queue[i]);
        delete (queue[i]);
        i--;
    }
}
