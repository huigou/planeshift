/*
* pawsconfigtooltips.h - Author: Neeno Pelero
*
* Copyright (C) 2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2 of the License)
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
*/

#ifndef PAWS_CONFIG_TOOLTIPS
#define PAWS_CONFIG_TOOLTIPS

//==============================================================================
// CS INCLUDES
//==============================================================================
#include <csutil/array.h>
#include <iutil/document.h>

//==============================================================================
// PAWS INCLUDES
//==============================================================================
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"
#include "paws/pawscolorpromptwindow.h"


class pawsCheckBox;

class pawsConfigTooltips : public pawsConfigSectionWindow, public iOnColorEnteredAction
{
public:
    pawsConfigTooltips(void); /// Constructor

    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

    /// from iOnColorEnteredAction (set color to param-identified label)
    virtual void OnColorEntered(const char *name,int param,int color);

    virtual bool PostSetup(); /// see pawsWidget

    /// from pawsConfigSectionWindow
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

    void SaveSetting();         /// Saves the configuration file
    bool LoadDefaults();         /// Loads the default values

private:
    pawsCheckBox* EnableTooltips; /// Checkbox for toggling the tooltips on/off
    pawsCheckBox* EnableBgColor;  /// Checkbox for toggling the tooltip background on/off

    pawsTextBox* TooltipBgColor;        /// Textboxes for showing the colors
    pawsTextBox* TooltipFontColor;
    pawsTextBox* TooltipShadowColor;

    pawsWidget* widget;

    void RefreshColorFrames();               /// Refreshes the color frames

    pawsColorPromptWindow * colorPicker;    /// pointer to colorPicker window

    int defTooltipsColors[3];  /// array of default colors
    int defToolTipEnable, defToolTipEnableBgColor;

};

CREATE_PAWS_FACTORY(pawsConfigTooltips);

#endif