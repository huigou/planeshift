/*
 * pawssetupwindow.h - Author: Ian Donderwinkel
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
//////////////////////////////////////////////////////////////////////
#ifndef PAWS_SETUP_WINDOW
#define PAWS_SETUP_WINDOW

#include "paws/pawswidget.h"
#include <iutil/vfs.h>
#include <imap/loader.h>
#include <ivaria/conout.h>
#include <ivaria/pmeter.h>
#include <iengine/engine.h>

class pawsRadioButtonGroup;
class pawsCheckBox;
class pawsButton;
class pawsEditTextBox;
class pawsTextBox;
class pawsScrollBar;
class pawsProgressBar;

class pawsSetupWindow : public pawsWidget
{
public:
    pawsSetupWindow();
    
    bool PostSetup();
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    bool OnScroll(int dir,pawsScrollBar* widget);

    void AdvancedMode(bool v) { advanced = v;}

private:
    pawsRadioButtonGroup*   rbgRenderer;
    pawsRadioButtonGroup*   rbgResolution;
    pawsRadioButtonGroup*   rbgDepth;
    pawsRadioButtonGroup*   rbgVBO;

    pawsCheckBox*           cbSound;
    pawsCheckBox*           cbFullScreen;
    pawsCheckBox*           cbAllMaps;
    pawsCheckBox*           cbKeepMaps;
    pawsCheckBox*           cbMultiQuality;
    pawsCheckBox*           cbPreloadModels;

    pawsButton*             btnOK;
    pawsButton*             btnCancel;
    pawsButton*             btnLaunch;

    pawsEditTextBox*        edtCustomWidth;
    pawsEditTextBox*        edtCustomHeight;
    pawsEditTextBox*        edtStencil;

    pawsTextBox*            lblStreamBuffer;
    pawsTextBox*            lblMultiSampling;
    pawsTextBox*            lblAnisotropy;
    pawsTextBox*            lblTextureSample;
    pawsTextBox*            lblRelightTarget;
    pawsTextBox*            lblFontScale;

    pawsScrollBar*          scbMultiSampling;
    pawsScrollBar*          scbAnisotropy;
    pawsScrollBar*          scbTextureSample;
    pawsScrollBar*          scbFontScale;

    csRef<iConfigFile> config;

    int customHeight;
    int customWidth;

    bool advanced;

    void LoadSettings();
    void LoadResolutionSettings();
    void LoadDepthSettings();
    void LoadRendererSettings();

    void SaveSettings();

    csRef<iLoader> loader;
    csRef<iEngine> engine;
    csRef<iVFS>    vfs;
};

CREATE_PAWS_FACTORY( pawsSetupWindow );

#endif
