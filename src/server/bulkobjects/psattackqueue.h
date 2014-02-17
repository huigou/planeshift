 /*
 * psattackqueue.cpp              creator hartra344@gmail.com
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef psAttackQueue_HEADER
#define psAttackQueue_HEADER


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <csutil/csstring.h>
#include <csutil/list.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <idal.h>
#include "psattack.h"

/** A queue to hold attacks in order of execution
 *
 * The main function of this class is to hold attacks to be executed using the combat system.
 * it will function by being a wrapper for a csList holding psAttack items.
 *
 */
class psAttackQueue : public csRefCount
{
public:

    /* Constructor
     *
     */
    psAttackQueue();

    /*
     *pushes attack into the last slot of the queue
     *@param attack the attack to push into the last slot of the queue
     */
    bool Push(psAttack* attack, psCharacter* character);

    /*
     * deletes the first slot in the queue
     */
    bool PopDelete();

    /*
     * Returns the first slot in the queue
     */
    psAttack* Pop();

    /*
     *Dumps all items in the queue
     */
    void Purge();

    /*
     * Calculate current total time of the queued elements
     */
    float GetTotalQueueTime();

    /*
     * Returns th list of special attacks in the queue
     */
    csList< csRef<psAttack> > getAttackList();

    /*
     * Returns the number of special attacks in the queue.
     */
    int getAttackListCount();

    /*
     * returns the maximum number of elements in the queue
     */
    int getMax()
    {
        return DEFAULT_ATTACKQUEUE_SIZE;
    }


private:
    csList< csRef<psAttack> > attackList; ///< the list of attacks in the queue
    size_t max; ///later max number of elements will be influenced based whether the player is in combat or not.

};
#endif
