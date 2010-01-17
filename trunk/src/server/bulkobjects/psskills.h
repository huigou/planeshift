/*
 * psskills.h
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

#ifndef __PSSKILLS_H__
#define __PSSKILLS_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/poolallocator.h"

#include "rpgrules/psmoney.h"

//=============================================================================
// Local Includes
//=============================================================================

#define MAX_SKILL 5000
#define MAX_STAT  5000

enum PSSKILL {
    PSSKILL_NONE            =   -1,
    PSSKILL_SWORD           =   0,
    PSSKILL_KNIVES          =   1,
    PSSKILL_AXE             =   2,
    PSSKILL_HAMMER          =   3,
    PSSKILL_MARTIALARTS     =   4,
    PSSKILL_POLEARM         =   5,
    PSSKILL_RANGED          =   6,
    PSSKILL_LIGHTARMOR      =   7,
    PSSKILL_MEDIUMARMOR     =   8,
    PSSKILL_HEAVYARMOR      =   9,
    PSSKILL_SHIELD          =   10,
    PSSKILL_CRYSTALWAY      =   11,
    PSSKILL_AZUREWAY        =   12,
    PSSKILL_BLUEWAY         =   13,
    PSSKILL_REDWAY          =   14,
    PSSKILL_BROWNWAY        =   15,
    PSSKILL_DARKWAY         =   16,
    PSSKILL_ASSASSIN        =   17,
    PSSKILL_BACKSTAB        =   18,
    PSSKILL_CLIMB           =   19,
    PSSKILL_FINDTRAPS       =   20,
    PSSKILL_HIDEINSHADOWS   =   21,
    PSSKILL_LOCKPICKING     =   22,
    PSSKILL_PICKPOCKETS     =   23,
    PSSKILL_SETTRAPS        =   24,
    PSSKILL_BODYDEVELOPMENT =   25,
    PSSKILL_RIDING          =   26,
    PSSKILL_SWIMMING        =   27,
    PSSKILL_ALCHEMY         =   28,
    PSSKILL_ANTIMAGIC       =   29,
    PSSKILL_REPAIRARMOR     =   30,
    PSSKILL_EMPATHY         =   31,
    PSSKILL_HERBAL          =   32,
    PSSKILL_REPAIRWEAPONS   =   33,
    PSSKILL_ARGAN           =   34,
    PSSKILL_ESTERIA         =   35,
    PSSKILL_LAHAR           =   36,
    PSSKILL_MINING          =   37,
    PSSKILL_BAKING          =   38,
    PSSKILL_COOKING         =   39,
    PSSKILL_BLACKSMITH      =   40,
    PSSKILL_KNIFEMAKING     =   41,
    PSSKILL_SWORDMAKING     =   42,
    PSSKILL_AXEMAKING       =   43,
    PSSKILL_MACEMAKING      =   44,
    PSSKILL_SHIELDMAKING    =   45,
    PSSKILL_AGI             =   46,
    PSSKILL_CHA             =   47,
    PSSKILL_END             =   48,
    PSSKILL_INT             =   49,
    PSSKILL_STR             =   50,
    PSSKILL_WILL            =   51,
    PSSKILL_MUSICINST       =   52,
    PSSKILL_BREWING         =   53,
    PSSKILL_MASONRY         =   54,
    PSSKILL_PAINTING        =   55,
    PSSKILL_TAILORING       =   56,
    PSSKILL_GLASSBLOWING    =   57,
    PSSKILL_FISHING         =   58,
    PSSKILL_BOWMAKING       =   59,
    PSSKILL_LEATHERWORKING  =   60,
    PSSKILL_GEMCUTTING      =   61,
    PSSKILL_POTTERY         =   62,
    PSSKILL_METALLURGY      =   63,
    PSSKILL_ILLUSTRATION    =   64,
    PSSKILL_ARMORMAKING     =   65,
    PSSKILL_TOOLMAKING      =   66,
    PSSKILL_REPAIRTOOLS     =   67,
    PSSKILL_COUNT           =   68
};
//These flags define the possible skills categories
enum PSSKILLS_CATEGORY
{
    PSSKILLS_CATEGORY_STATS = 0,//Intelligence, Charisma, etc.
    PSSKILLS_CATEGORY_COMBAT,
    PSSKILLS_CATEGORY_MAGIC,
    PSSKILLS_CATEGORY_JOBS,        //Crafting, mining, etc.
    PSSKILLS_CATEGORY_VARIOUS,
    PSSKILLS_CATEGORY_FACTIONS
};

class psSkillInfo
{
public:
    psSkillInfo();
    ~psSkillInfo();

    ///  The new operator is overriden to call PoolAllocator template functions
//    void *operator new(size_t);
    ///  The delete operator is overriden to call PoolAllocator template functions
//    void operator delete(void *);
    
    PSSKILL id;
    csString name;
    csString description;
    int practice_factor;
    int mental_factor;
    psMoney price;
    PSSKILLS_CATEGORY category;
    /// The base cost that this skill requires. Used to calculate costs for next rank.
    int baseCost;

private:
    /// Static reference to the pool for all psSkillInfo objects
//    static PoolAllocator<psSkillInfo> skillinfopool;    

};



#endif


