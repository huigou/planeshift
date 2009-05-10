/*
 * pssectorinfo.cpp
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
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pssectorinfo.h"

psSectorInfo::psSectorInfo()
{
    uid = 0;
    rain_enabled = false;
    is_raining = false;
    is_snowing = false;
    current_rain_drops = 0;
    fog_density = 0;
    fog_density_old = 0;
    densitySaved = false;
    fogFade = 0;
    say_range = 0;
    god_name.Clear();
}

psSectorInfo::~psSectorInfo()
{
}

unsigned int psSectorInfo::GetRandomRainGap()
{
    return psserver->GetRandom(rain_max_gap-rain_min_gap) + rain_min_gap;
}

unsigned int psSectorInfo::GetRandomRainDuration()
{
    return psserver->GetRandom(rain_max_duration-rain_min_duration) + rain_min_duration;
}

unsigned int psSectorInfo::GetRandomRainDrops()
{
    return psserver->GetRandom(rain_max_drops-rain_min_drops) + rain_min_drops;
}

unsigned int psSectorInfo::GetRandomRainFadeIn()
{
    return psserver->GetRandom(rain_max_fade_in-rain_min_fade_in) + rain_min_fade_in;
}

unsigned int psSectorInfo::GetRandomRainFadeOut()
{
    return psserver->GetRandom(rain_max_fade_out-rain_min_fade_out) + rain_min_fade_out;
}

unsigned int psSectorInfo::GetRandomLightningGap()
{
    return psserver->GetRandom(lightning_max_gap - lightning_min_gap) + lightning_min_gap;
}



