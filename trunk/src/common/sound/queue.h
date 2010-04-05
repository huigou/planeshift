/*
 * queue.h
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

#ifndef _SOUND_QUEUE_H_
#define _SOUND_QUEUE_H_

class SoundQueueItem
{
    public:
    csString        filename;
    SoundHandle     *handle;

    SoundQueueItem ();
    ~SoundQueueItem ();
};


class SoundQueue
{
    private:
    csArray<SoundQueueItem*>    queue;              /* the queue itself */
    float                       volume;             /* main volume for this queue */
    SoundControl               *sndCtrl;            /* soundcontrol this queue will use */

    public:
    SoundQueue (SoundControl* &ctrl, float vol);    /* constructor */
    ~SoundQueue ();                                 /* destructor */
    void AddItem (const char *filename);            /* add a item to the queue */
    void Work ();                                   /* check if there is something todo */
    void Purge ();                                  /* stop and purge that queue */
};

#endif /*_SOUND_QUEUE_H_*/
