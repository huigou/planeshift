/*
 * pawsconfighpandmana.h - Author: Joe Lyon
 *
 * Copyright (C) 2014 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_CONFIG_HPANDMANA_HEADER
#define PAWS_CONFIG_HPANDMANA_HEADER


// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawscombo.h"
#include "pawsconfigwindow.h"
#include "util/psxmlparser.h"
#include "shortcutwindow.h"
#include "pawsinfowindow.h"
#include "pawsskillwindow.h"


class pawsCheckBox;
class pawsRadioButtonGroup;


/*
 * class pawsConfigHPandMana is options screen for configuration of HPandMana Bar
 */
class pawsConfigHPandMana : public pawsConfigSectionWindow
{
public:
    pawsConfigHPandMana();

    //from pawsWidget:
    virtual bool PostSetup();
    virtual bool OnScroll(int, pawsScrollBar*);

    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

    void PickText( const char * fontName, int size );

    void UpdateHPWarnLevel( );
    void UpdateHPDangerLevel( );
    void UpdateHPFlashLevel( );
    void UpdateManaWarnLevel( );
    void UpdateManaDangerLevel( );
    void UpdateManaFlashLevel( );

    bool LoadUserSharedPrefs();

protected:

    pawsWidget*            ShortcutMenu;
    pawsScrollMenu*        MenuBar;
    pawsInfoWindow*        InfoWindow;
    pawsSkillWindow*       SkillWindow;

    pawsScrollBar*         HPWarnLevel;
    pawsTextBox*           HPWarnSetting;

    pawsScrollBar*         HPDangerLevel;
    pawsTextBox*           HPDangerSetting;

    pawsScrollBar*         HPFlashLevel;
    pawsTextBox*           HPFlashSetting;

    pawsScrollBar*         ManaWarnLevel;
    pawsTextBox*           ManaWarnSetting;

    pawsScrollBar*         ManaDangerLevel;
    pawsTextBox*           ManaDangerSetting;

    pawsScrollBar*         ManaFlashLevel;
    pawsTextBox*           ManaFlashSetting;


    bool loaded;

};


CREATE_PAWS_FACTORY(pawsConfigHPandMana);


#endif

