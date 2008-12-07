/*
 * Author: Andrew Robberts
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

#include <psconfig.h>
#include "eeditrendertoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"

EEditRenderToolbox::EEditRenderToolbox() : scfImplementationType(this)
{
}

EEditRenderToolbox::~EEditRenderToolbox()
{
}

void EEditRenderToolbox::Update(unsigned int elapsed)
{
}

size_t EEditRenderToolbox::GetType() const
{
    return T_RENDER;
}

const char * EEditRenderToolbox::GetName() const
{
    return "Render";
}

bool EEditRenderToolbox::PostSetup()
{
    renderButton = (pawsButton *) FindWidget("render");     CS_ASSERT(renderButton);
    loadButton   = (pawsButton *) FindWidget("load");       CS_ASSERT(loadButton);
    pauseButton  = (pawsButton *) FindWidget("pause");      CS_ASSERT(pauseButton);
    cancelButton = (pawsButton *) FindWidget("stop");       CS_ASSERT(cancelButton);
    return true;
}

bool EEditRenderToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget == renderButton)
    {
        editApp->RenderCurrentEffect();
        return true;
    }
    else if (widget == loadButton)
    {
        editApp->ReloadCurrentEffect();
        return true;
    }
    else if (widget == pauseButton)
    {
        if (editApp->TogglePauseEffect())
            pauseButton->SetText("Resume");
        else
            pauseButton->SetText("Pause");
    }
    else if (widget == cancelButton)
    {
        editApp->CancelEffect();
        return true;
    }
    return false;
}

