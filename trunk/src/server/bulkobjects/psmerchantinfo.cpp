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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/psdatabase.h"

#include "../psserver.h"
#include "../cachemanager.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psmerchantinfo.h"



/**
 * A character is defined to be a merchant if there are
 * merchant item categories for the character.
 *
 * @return Return true if the character is a merchant.
 */
bool psMerchantInfo::Load(unsigned int characterid)
{
    bool isMerchant = false;
    
    Result merchantCategories(db->Select("SELECT * from merchant_item_categories where player_id=%u",characterid));
    if (merchantCategories.IsValid())
    {
        int i,count=merchantCategories.Count();

        for (i=0;i<count;i++)
        {
            psItemCategory * category = FindCategory(atoi(merchantCategories[i]["category_id"]));
            if (!category)
            {
                Error1("Error! Category could not be loaded. Skipping.\n");
                continue;
            }
            categories.Push(category);
            isMerchant = true;
        }
    }

    return isMerchant;
}

psItemCategory * psMerchantInfo::FindCategory(int id)
{
    return CacheManager::GetSingleton().GetItemCategoryByID(id);
}

psItemCategory * psMerchantInfo::FindCategory(const csString & name)
{
    return CacheManager::GetSingleton().GetItemCategoryByName(name);
}
