/*
 * pssectorinfo.h
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



#ifndef __PSSECTORINFO_H__
#define __PSSECTORINFO_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================


/** Contains information about sectors from the server perspective.
*
*  Right now this class just includes the ID, name of a sector and rain parameters.
*  Future versions may include parameters for a bounding box, portals to other sectors, or other information.
*
*
*
*/
class psSectorInfo
{
public:
    psSectorInfo();
    ~psSectorInfo();

    unsigned int GetRandomRainGap();
    unsigned int GetRandomRainDuration();
    unsigned int GetRandomRainDrops();
    unsigned int GetRandomRainFadeIn();
    unsigned int GetRandomRainFadeOut();
    unsigned int GetRandomLightningGap();

    bool GetIsColliding() { return is_colliding; }
    bool GetIsNonTransient() { return is_non_transient; }
    
    unsigned int uid;
    csString  name;
    bool rain_enabled; // Will run automatic weather when true
    unsigned int rain_min_gap,rain_max_gap;
    unsigned int rain_min_duration,rain_max_duration;
    unsigned int rain_min_drops,rain_max_drops;
    unsigned int lightning_min_gap,lightning_max_gap;
    unsigned int rain_min_fade_in,rain_max_fade_in;
    unsigned int rain_min_fade_out,rain_max_fade_out;
    unsigned int current_rain_drops; // Drops
    
    bool is_raining;
    bool is_snowing;
    bool is_colliding;
    bool is_non_transient;

    // Fog
    unsigned int fog_density, fog_density_old;
    bool densitySaved;
    unsigned int fogFade;
    int r,g,b;

    float say_range;

    csString god_name;
};



#endif


