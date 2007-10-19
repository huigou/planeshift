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
    if ( ! LoadFromFile("data/gui/configentitylabels.xml"))
        return false;
       
    csRef<psCelClient> celclient = psengine->GetCelClient();
    assert(celclient);
    
    entityLabels = celclient->GetEntityLabels();
    assert(entityLabels);
    return true;
}

bool pawsConfigEntityLabels::PostSetup()
{
    visibilityRadioGroup = dynamic_cast<pawsRadioButtonGroup*> (FindWidget("visibility"));
    if (visibilityRadioGroup == NULL)
        return false;
    contentRadioGroup = dynamic_cast<pawsRadioButtonGroup*> (FindWidget("content"));
    if (contentRadioGroup == NULL)
        return false;
    return true;
}

bool pawsConfigEntityLabels::LoadConfig()
{
    psEntityLabelVisib visibility;
    bool showGuild;

    entityLabels->GetConfiguration(visibility, showGuild);
    switch (visibility)
    {
        case LABEL_ALWAYS:
            visibilityRadioGroup->SetActive("always");
            break;
        case LABEL_ONMOUSE:
            visibilityRadioGroup->SetActive("mouse");
            break;
        case LABEL_NEVER:
            visibilityRadioGroup->SetActive("never");
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
    psEntityLabelVisib visibility;
    bool showGuild;
    csString activeVisib;

    activeVisib = visibilityRadioGroup->GetActive();
    if (activeVisib == "always")
        visibility = LABEL_ALWAYS;
    else if (activeVisib == "mouse")
        visibility = LABEL_ONMOUSE;
    else // if (activeVisib == "never")
        visibility = LABEL_NEVER;

    if (contentRadioGroup->GetActive() == "guild")
        showGuild = true;
    else
        showGuild = false;


    entityLabels->Configure(visibility, showGuild);
    entityLabels->SaveToFile();
    dirty = false;
    return true;
}

void pawsConfigEntityLabels::SetDefault()
{
    visibilityRadioGroup->SetActive("always");
    contentRadioGroup->SetActive("guild");
    dirty = true;
}

bool pawsConfigEntityLabels::OnChange(pawsWidget * widget)
{
    if ((widget == visibilityRadioGroup) || (widget == contentRadioGroup))
        dirty = true;

    return true;
}
