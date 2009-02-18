/*
 * pawsconfigsound.h - Author: Christian Svensson
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_CONFIG_SOUND_HEADER
#define PAWS_CONFIG_SOUND_HEADER

// CS INCLUDES
#include <csutil/array.h>
#include <iutil/document.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"
#include "util/psxmlparser.h"

class pawsCheckBox;
class pawsScrollBar;

class pawsRadioButtonGroup;

/**
 * class pawsConfigPvP is options screen for configuration of PvP
 */
class pawsConfigSound : public pawsConfigSectionWindow
{
public:
    pawsConfigSound();

    //from pawsWidget:
    virtual bool PostSetup();
    virtual bool OnScroll(int,pawsScrollBar*);
    virtual bool OnButtonPressed(int, int, pawsWidget*);
    virtual void Show();
    virtual void Hide();

    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

protected:

    pawsScrollBar* generalVol;
    pawsScrollBar* musicVol;
    pawsScrollBar* ambientVol;
    pawsScrollBar* actionsVol;
    pawsScrollBar* guiVol;
    pawsScrollBar* voicesVol;

    pawsCheckBox* ambient;
    pawsCheckBox* actions;
    pawsCheckBox* music;
    pawsCheckBox* gui;
    pawsCheckBox* voices;

    pawsCheckBox* muteOnFocusLoss;
    pawsCheckBox* loopBGM;
    pawsCheckBox* combatMusic;

    bool loaded;

    bool oldambient,oldactions,oldmusic,oldgui,oldvoices;
    float oldvol,oldmusicvol,oldambientvol,oldactionsvol,oldguivol,oldvoicesvol;
};


CREATE_PAWS_FACTORY(pawsConfigSound)


#endif



