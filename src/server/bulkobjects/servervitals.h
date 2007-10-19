/*
 * servervitals.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef SERVER_VITALS_HEADER
#define SERVER_VITALS_HEADER

#include "psstdint.h"
#include "rpgrules/vitals.h"
#include "util/psconst.h"
#include "../playergroup.h"

class MsgEntry;
class psCharacter;

/** Server side of the character vitals manager.  Does a lot more accessing
  * of the data to set particular things.  Also does construction of data to 
  * send to a client.
  */
class psServerVitals : public psVitalManager
{
private:
    ///  @see  PS_DIRTY_VITALS
    unsigned int statsDirty;
    unsigned char version;    
    psCharacter * character;  // the char whose vitals we manage
        
public:
    psServerVitals(psCharacter * character);
    
     /** Handles new Vital data construction for the server.
      */ 
    bool SendStatDRMessage(uint32_t clientnum, PS_ID eid, int flags, csRef<PlayerGroup> group = NULL);
    
    bool Update( csTicks now );
    
    void SetExp( int exp );
    void SetPP( int pp );    
    
    /** Adjust a field in a vital statistic.
    *   @param vitalName One of the enums @see PS_VITALS
    *   @param dirtyFlag What became dirty in this adjustment. @see  PS_DIRTY_VITAL.
    *
    *   @return The new value of the field in the vital stat.
    */        
    psCharVital& DirtyVital( int vitalName, int dirtyFlag );            
};


#endif

