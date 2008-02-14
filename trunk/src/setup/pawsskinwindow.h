/*
 * pawsskinwindow.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * Credits : Christian Svensson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef PAWS_SKIN_WINDOW
#define PAWS_SKIN_WINDOW

#include <iutil/vfs.h>
#include <csutil/ref.h>
#include "paws/pawswidget.h"

class pawsComboBox;
class pawsButton;
class pawsMultiLineTextBox;
class pawsCheckBox;

class pawsSkinWindow : public pawsWidget
{
public:
    ~pawsSkinWindow();
    
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    void OnListAction( pawsListBox* widget, int status );
    bool PostSetup();

    // Used to load a resource from the new skin
    bool LoadResource(const char* resource,const char* resname,const char* mountPath);

    void LoadSkin(const char* name);

    void SetConfig(csRef<iConfigFile> cfg) { config = cfg; }

private:

    csString mountedPath;
    csString currentSkin;
    csString skinPath;

    csRef<iVFS> vfs;
    csRef<iConfigFile> config;

    pawsComboBox* skins;
    pawsMultiLineTextBox* desc;
    pawsWidget* preview; // The preview widget
    pawsButton* previewBtn; // The preview button
    pawsCheckBox* previewBox; // The preview checkbox
};

CREATE_PAWS_FACTORY( pawsSkinWindow );

#endif


