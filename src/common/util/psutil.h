/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef PSUTIL_H
#define PSUTIL_H

#include <cstypes.h>
#include <csutil/csstring.h>

struct psPoint
{
    int x;
    int y;

    psPoint ():x(0), y(0) { }

    /// Constructor: initialize the object with given values
    psPoint (int iX, int iY):x(iX), y(iY) { }
};


class ScopedTimer
{
    csTicks start,limit;
    csString comment;
 public:
    ScopedTimer(csTicks limit, const char * format, ... );
    ~ScopedTimer();
};

/** Returns a random number.
 *
 * @return Returns a random number between 0.0 and 1.0.
 */
float psGetRandom();

/** Returns a random number with a limit.
 *
 * @return Returns a random number between 0 and limit.
 */
uint32 psGetRandom(uint32 limit);



#endif
