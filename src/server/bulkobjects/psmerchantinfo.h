/*
 * psmerchantinfo.h
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
#ifndef __PSMERCHANTINFO_H__
#define __PSMERCHANTINFO_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/array.h>
#include <csutil/refcount.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================

struct psItemCategory
{
    /// Unique identifier for the category
    unsigned int id;
    /// Human readable name of category
    csString     name;
    /// Item_stats id of item required to do a repair on this category.
    unsigned int repair_tool_stat_id;
    /// Flag to tell us whether the repair tool is consumed in the repair or not.  (Kit or Tool)
    bool         repair_tool_consumed;
    /// ID of skill which is used to calculate result of repair
    int          repair_skill_id;
};



class psMerchantInfo : public csRefCount
{
public:
    bool Load(unsigned int characterid);
    csArray<psItemCategory*> categories;
    
    psItemCategory * FindCategory(int id);
    psItemCategory * FindCategory(const csString & name);
};

#endif
