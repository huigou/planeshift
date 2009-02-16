/*
 * psraceinfo.h
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



#ifndef __PSRACEINFO_H__
#define __PSRACEINFO_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "psitem.h"
#include "pscharacter.h"  // for gender 

enum PSRACEINFO_STAMINA
{
    PSRACEINFO_STAMINA_PHYSICAL_STILL = 0,
    PSRACEINFO_STAMINA_PHYSICAL_WALK,
    PSRACEINFO_STAMINA_MENTAL_STILL,
    PSRACEINFO_STAMINA_MENTAL_WALK
};

struct psRaceStartingLocation
{
    float x,y,z,yrot;
    const char* sector_name;
};

struct psRaceInfo
{
protected:
    unsigned short attributes[PSITEMSTATS_STAT_COUNT];
    
public:
    psRaceInfo();
    ~psRaceInfo();
    bool Load(iResultRow& row);

    bool LoadBaseSpeeds(iObjectRegistry *object_reg);
    
    unsigned int uid;
    unsigned int race;
    csString name;
    csString sex;

    PSCHARACTER_GENDER gender;
    
    const char *mesh_name;
    const char *base_texture_name;
    csVector3 size;
    int initialCP;
    int natural_armor_id;
    float runMinSpeed,runBaseSpeed,runMaxSpeed;
    float walkMinSpeed,walkBaseSpeed,walkMaxSpeed;
    
    float GetBaseAttribute(PSITEMSTATS_STAT attrib);
    
    csString helmGroup;         /// The name of the helm group race is in.
    
private:
    void  SetBaseAttribute(PSITEMSTATS_STAT attrib, float val);
public:

    float baseRegen[4];

    csArray<psRaceStartingLocation> startingLocations;

    void GetStartingLocation(float& x,float& y, float& z,float& rot,const char*& sectorname);
    void GetSize(csVector3& size)
    {
        size = this->size;
    };

    const char *GetHonorific();
    const char *GetObjectPronoun();
    const char *GetPossessive();

    csString ReadableRaceGender();

    const char *GetGender() { return sex; }
    const char *GetRace() { return name; }
};



#endif

