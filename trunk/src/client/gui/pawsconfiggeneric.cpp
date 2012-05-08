/*
 * pawsconfiggeneric.cpp - Author: Luca Pancallo
 *
 * Copyright (C) 2001-2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//CS INCLUDES
#include <psconfig.h>

//PAWS INCLUDES
#include "pawsconfiggeneric.h"
#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"

//CLIENT INCLUDES
#include "../globals.h"


pawsConfigGeneric::pawsConfigGeneric()
{
	text = NULL;
}

bool pawsConfigGeneric::PostSetup()
{
	text = (pawsMultiLineTextBox*)FindWidget("config generic text");
	if (!text) {
		Error1("Could not locate description text widget!");
		return false;
	}
	
	drawFrame();
	
	return true;
}

void pawsConfigGeneric::drawFrame()
{
	//Clear the frame by hiding all widgets
	text->Hide();
    text->Show();
}

void pawsConfigGeneric::setTextName(csString* textName)
{
    // show translated text
    text->SetText(PawsManager::GetSingleton().Translate(*textName));

}

bool pawsConfigGeneric::Initialize()
{       
    if ( ! LoadFromFile("configgeneric.xml")) {
        return false;
    }
    return true;
}

bool pawsConfigGeneric::LoadConfig()
{
	return true;
}

bool pawsConfigGeneric::SaveConfig()
{
    return true;
}

void pawsConfigGeneric::SetDefault()
{
	LoadConfig();
    SaveConfig();
}

void pawsConfigGeneric::Show()
{
	pawsWidget::Show();
}

void pawsConfigGeneric::OnListAction(pawsListBox* /*selected*/, int /*status*/)
{
    dirty = true;
}

