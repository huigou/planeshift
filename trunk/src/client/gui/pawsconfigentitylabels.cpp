/*
 * pawsconfigentitylabels.cpp - Author: Ondrej Hurt
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// CS INCLUDES
#include <psconfig.h>
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>

// COMMON INCLUDES
#include "util/log.h"
#include "globals.h"

// CLIENT INCLUDES
#include "entitylabels.h"

// PAWS INCLUDES
#include "pawsconfigentitylabels.h"
#include "paws/pawsmanager.h"
#include "paws/pawsradio.h"


bool pawsConfigEntityLabels::Initialize()
{
    if ( ! LoadFromFile("configentitylabels.xml"))
        return false;
       
    csRef<psCelClient> celclient = psengine->GetCelClient();
    assert(celclient);
    
    entityLabels = celclient->GetEntityLabels();
    assert(entityLabels);
    return true;
}

bool pawsConfigEntityLabels::PostSetup()
{
    visCreaturesRadioGroup = dynamic_cast<pawsRadioButtonGroup*> (FindWidget("visibilityCreatures"));
    if (visCreaturesRadioGroup == NULL)
        return false;
    
    visItemsRadioGroup = dynamic_cast<pawsRadioButtonGroup*> (FindWidget("visibilityItems"));
    if(visItemsRadioGroup == NULL)
    	return false;
    
    contentRadioGroup = dynamic_cast<pawsRadioButtonGroup*> (FindWidget("content"));
    if (contentRadioGroup == NULL)
        return false;
    
    return true;
}

bool pawsConfigEntityLabels::LoadConfig()
{
    psEntityLabelVisib visCreatures;
    psEntityLabelVisib visItems;
    bool showGuild;

    entityLabels->GetConfiguration(visCreatures, visItems, showGuild);
    
    switch (visCreatures)
    {
        case LABEL_ALWAYS:
            visCreaturesRadioGroup->SetActive("always");
            break;
        case LABEL_ONMOUSE:
        	visCreaturesRadioGroup->SetActive("mouse");
            break;
        case LABEL_NEVER:
        	visCreaturesRadioGroup->SetActive("never");
            break;
    }

    switch (visItems)
    {
        case LABEL_ALWAYS:
            visItemsRadioGroup->SetActive("always");
            break;
        case LABEL_ONMOUSE:
        	visItemsRadioGroup->SetActive("mouse");
            break;
        case LABEL_NEVER:
        	visItemsRadioGroup->SetActive("never");
            break;
    }
    
    if (showGuild)
        contentRadioGroup->SetActive("guild");
    else
        contentRadioGroup->SetActive("name");
        
    dirty = false;
    return true;
}

bool pawsConfigEntityLabels::SaveConfig()
{
    psEntityLabelVisib visCreatures;
    psEntityLabelVisib visItems;
    bool showGuild;
    csString activeVisib;

    activeVisib = visCreaturesRadioGroup->GetActive();
    if (activeVisib == "always")
        visCreatures = LABEL_ALWAYS;
    else if (activeVisib == "mouse")
        visCreatures = LABEL_ONMOUSE;
    else // if (activeVisib == "never")
        visCreatures = LABEL_NEVER;

    activeVisib = visItemsRadioGroup->GetActive();
    if (activeVisib == "always")
        visItems = LABEL_ALWAYS;
    else if (activeVisib == "mouse")
        visItems = LABEL_ONMOUSE;
    else // if (activeVisib == "never")
        visItems = LABEL_NEVER;
    
    if (contentRadioGroup->GetActive() == "guild")
        showGuild = true;
    else
        showGuild = false;


    entityLabels->Configure(visCreatures, visItems, showGuild);
    entityLabels->SaveToFile();
    dirty = false;
    return true;
}

void pawsConfigEntityLabels::SetDefault()
{
    visCreaturesRadioGroup->SetActive("mouse");
    visItemsRadioGroup->SetActive("mouse");
    contentRadioGroup->SetActive("guild");
    dirty = true;
}

bool pawsConfigEntityLabels::OnChange(pawsWidget * widget)
{
    if ((widget == visCreaturesRadioGroup) || (widget == visItemsRadioGroup) || (widget == contentRadioGroup))
        dirty = true;

    return true;
}
