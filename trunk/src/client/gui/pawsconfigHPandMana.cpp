/*
 * pawsconfighpandmana.cpp - Author: Joe Lyon
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iutil/stringarray.h>
#include <iutil/vfs.h>


// COMMON INCLUDES
#include "util/log.h"
#include "psmainwidget.h"

// CLIENT INCLUDES
#include "../globals.h"
#include "util/psxmlparser.h"


// PAWS INCLUDES
#include "pawsconfigHPandMana.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "shortcutwindow.h"
#include "pawsinfowindow.h"
#include "pawsscrollmenu.h"
#include "paws/pawsradio.h"

pawsConfigHPandMana::pawsConfigHPandMana() :
    ShortcutMenu(NULL),
    MenuBar(NULL),
    InfoWindow(NULL),
    HPWarnLevel(NULL),
    HPWarnSetting(NULL),
    HPDangerLevel(NULL),
    HPDangerSetting(NULL),
    HPFlashLevel(NULL),
    HPFlashSetting(NULL),
    ManaWarnLevel(NULL),
    ManaWarnSetting(NULL),
    ManaDangerLevel(NULL),
    ManaDangerSetting(NULL),
    ManaFlashLevel(NULL),
    ManaFlashSetting(NULL)
{
    loaded= false;
}

bool pawsConfigHPandMana::Initialize()
{
    LoadFromFile("confighpandmana.xml");
    return true;
}

bool pawsConfigHPandMana::PostSetup()
{
//get pointers to Shortcut Menu and its Menu Bar
    psMainWidget*   Main    = psengine->GetMainWidget();
    if( Main==NULL )
    {
        Error1( "pawsConfigHPandMana::PostSetup unable to get psMainWidget\n");
        return false;
    }

    ShortcutMenu = Main->FindWidget( "ShortcutMenu",true );
    if( ShortcutMenu==NULL )
    {
        Error1( "pawsConfigHPandMana::PostSetup unable to get ShortcutMenu\n");
        return false;
    }

    MenuBar = (pawsScrollMenu*)(ShortcutMenu->FindWidget( "MenuBar",true ));
    if( MenuBar==NULL )
    {
        Error1( "pawsConfigHPandMana::PostSetup unable to get MenuBar\n");
        return false;
    }

//get pointer to Info window
    InfoWindow = (pawsInfoWindow *)Main->FindWidget( "InfoWindow", true );
    if( InfoWindow==NULL )
    {
        Error1( "pawsConfigHPandMana::PostSetup unable to get InfoWindow\n");
        return false;
    }


//get form widgets

    HPWarnLevel = (pawsScrollBar*)FindWidget("HPWarnLevel");
    if(!HPWarnLevel)
    {
        return false;
    }
    HPWarnLevel->EnableValueLimit(true);
    HPWarnLevel->SetMinValue(0);
    HPWarnLevel->SetMaxValue(100);
    HPWarnLevel->SetCurrentValue(0,false);

    HPWarnSetting = (pawsTextBox*)FindWidget("HPWarnSetting");
    if(!HPWarnSetting)
    {
        return false;
    }


    HPDangerLevel = (pawsScrollBar*)FindWidget("HPDangerLevel");
    if(!HPDangerLevel)
    {
        return false;
    }
    HPDangerLevel->EnableValueLimit(true);
    HPDangerLevel->SetMinValue(0);
    HPDangerLevel->SetMaxValue(100);
    HPDangerLevel->SetCurrentValue(0,false);

    HPDangerSetting = (pawsTextBox*)FindWidget("HPDangerSetting");
    if(!HPDangerSetting)
    {
        return false;
    }


    HPFlashLevel = (pawsScrollBar*)FindWidget("HPFlashLevel");
    if(!HPFlashLevel)
    {
        return false;
    }
    HPFlashLevel->EnableValueLimit(true);
    HPFlashLevel->SetMinValue(0);
    HPFlashLevel->SetMaxValue(100);
    HPFlashLevel->SetCurrentValue(0,false);

    HPFlashSetting = (pawsTextBox*)FindWidget("HPFlashSetting");
    if(!HPFlashSetting)
    {
        return false;
    }


    ManaWarnLevel = (pawsScrollBar*)FindWidget("ManaWarnLevel");
    if(!ManaWarnLevel)
    {
        return false;
    }
    ManaWarnLevel->EnableValueLimit(true);
    ManaWarnLevel->SetMinValue(0);
    ManaWarnLevel->SetMaxValue(100);
    ManaWarnLevel->SetCurrentValue(0,false);

    ManaWarnSetting = (pawsTextBox*)FindWidget("ManaWarnSetting");
    if(!ManaWarnSetting)
    {
        return false;
    }


    ManaDangerLevel = (pawsScrollBar*)FindWidget("ManaDangerLevel");
    if(!ManaDangerLevel)
    {
        return false;
    }
    ManaDangerLevel->EnableValueLimit(true);
    ManaDangerLevel->SetMinValue(0);
    ManaDangerLevel->SetMaxValue(100);
    ManaDangerLevel->SetCurrentValue(0,false);

    ManaDangerSetting = (pawsTextBox*)FindWidget("ManaDangerSetting");
    if(!ManaDangerSetting)
    {
        return false;
    }


    ManaFlashLevel = (pawsScrollBar*)FindWidget("ManaFlashLevel");
    if(!ManaFlashLevel)
    {
        return false;
    }
    ManaFlashLevel->EnableValueLimit(true);
    ManaFlashLevel->SetMinValue(0);
    ManaFlashLevel->SetMaxValue(100);
    ManaFlashLevel->SetCurrentValue(0,false);

    ManaFlashSetting = (pawsTextBox*)FindWidget("ManaFlashSetting");
    if(!ManaFlashSetting)
    {
        return false;
    }

    loaded=true;
    return true;
}

bool pawsConfigHPandMana::LoadConfig()
{

    HPWarnLevel->SetCurrentValue( ((pawsShortcutWindow*)ShortcutMenu)->GetHPWarnLevel() );
    UpdateHPWarnLevel();

    HPDangerLevel->SetCurrentValue( ((pawsShortcutWindow*)ShortcutMenu)->GetHPDangerLevel() );
    UpdateHPDangerLevel();

    HPFlashLevel->SetCurrentValue( ((pawsShortcutWindow*)ShortcutMenu)->GetHPFlashLevel() );
    UpdateHPFlashLevel();

    ManaWarnLevel->SetCurrentValue( ((pawsShortcutWindow*)ShortcutMenu)->GetManaWarnLevel() );
    UpdateManaWarnLevel();

    ManaDangerLevel->SetCurrentValue( ((pawsShortcutWindow*)ShortcutMenu)->GetManaDangerLevel() );
    UpdateManaDangerLevel();

    ManaFlashLevel->SetCurrentValue( ((pawsShortcutWindow*)ShortcutMenu)->GetManaFlashLevel() );
    UpdateManaFlashLevel();


    loaded= true;
    dirty = false;
    return true;
}

bool pawsConfigHPandMana::SaveConfig()
{

    csString xml;
    xml = "<hpandmana>\n";
    xml.AppendFmt("<HPWarnLevel value=\"%d\" />\n",
                     int(HPWarnLevel->GetCurrentValue()));
    xml.AppendFmt("<HPDangerLevel value=\"%d\" />\n",
                     int(HPDangerLevel->GetCurrentValue()));
    xml.AppendFmt("<HPFlashLevel value=\"%d\" />\n",
                     int(HPFlashLevel->GetCurrentValue()));
    xml.AppendFmt("<ManaWarnLevel value=\"%d\" />\n",
                     int(ManaWarnLevel->GetCurrentValue()));
    xml.AppendFmt("<ManaDangerLevel value=\"%d\" />\n",
                     int(ManaDangerLevel->GetCurrentValue()));
    xml.AppendFmt("<ManaFlashLevel value=\"%d\" />\n",
                     int(ManaFlashLevel->GetCurrentValue()));
    xml += "</hpandmana>\n";

    dirty = false;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/confighpandmana.xml",
                                         xml,xml.Length());
}

void pawsConfigHPandMana::UpdateHPWarnLevel( )
{
    if( !loaded ) return;
    csString temp;
    float tLevel = HPWarnLevel->GetCurrentValue();

    if( tLevel<100 && tLevel>=1.0  )
    {
        temp.Format("> %2.0f%%",tLevel );
        HPWarnSetting->SetText( temp );

        //if warnlevel is > danger level in % mode, then increase danger level to match.
        if( tLevel>HPDangerLevel->GetCurrentValue() )
        {
            ((pawsShortcutWindow*)(ShortcutMenu))->SetHPDangerLevel(  HPDangerLevel->GetCurrentValue()/100 );
        }
        ((pawsShortcutWindow*)(ShortcutMenu))->SetHPWarnLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
        InfoWindow->SetHPWarnLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
    }
    else
    {
        HPWarnSetting->SetText( "Disabled" );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetHPWarnLevel( 0 );
        InfoWindow->SetHPWarnLevel( 0 );
    }
}

void pawsConfigHPandMana::UpdateHPDangerLevel( )
{
    if( !loaded ) return;
    csString temp;
    float tLevel = HPDangerLevel->GetCurrentValue();

    if( tLevel<100 && tLevel>=1.0  )
    {
        temp.Format("> %2.0f%%",tLevel );
        HPDangerSetting->SetText( temp );

        //if warnlevel is > danger level in % mode, then increase danger level to match.
        if( HPWarnLevel->GetCurrentValue()>tLevel )
        {
            tLevel=HPWarnLevel->GetCurrentValue();
        }
        ((pawsShortcutWindow*)(ShortcutMenu))->SetHPDangerLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
        InfoWindow->SetHPDangerLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
    }
    else
    {
        HPDangerSetting->SetText( "Disabled" );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetHPDangerLevel( 0 );
        InfoWindow->SetHPDangerLevel( 0 );
    }
}

void pawsConfigHPandMana::UpdateHPFlashLevel( )
{
    if( !loaded ) return;
    csString temp;
    float tLevel = HPFlashLevel->GetCurrentValue();

    if( tLevel<100 && tLevel>=1.0  )
    {
        temp.Format("> %2.0f%%",tLevel );
        HPFlashSetting->SetText( temp );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetHPFlashLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
        InfoWindow->SetHPFlashLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
    }
    else
    {
        HPFlashSetting->SetText( "Disabled" );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetHPFlashLevel( 0 );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetHPOn( true );
        InfoWindow->SetHPFlashLevel( 0 );
    }
}

void pawsConfigHPandMana::UpdateManaWarnLevel( )
{
    if( !loaded ) return;
    csString temp;
    float tLevel = ManaWarnLevel->GetCurrentValue();

    if( tLevel<100 && tLevel>=1.0  )
    {
        temp.Format("> %2.0f%%",tLevel );
        ManaWarnSetting->SetText( temp );

        //if warnlevel is > danger level in % mode, then increase danger level to match.
        if( tLevel>ManaDangerLevel->GetCurrentValue() )
        {
            ((pawsShortcutWindow*)(ShortcutMenu))->SetManaDangerLevel(  ManaDangerLevel->GetCurrentValue()/100 );
        }
        ((pawsShortcutWindow*)(ShortcutMenu))->SetManaWarnLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
        InfoWindow->SetManaWarnLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
    }
    else
    {
        ManaWarnSetting->SetText( "Disabled" );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetManaWarnLevel( 0 );
        InfoWindow->SetManaWarnLevel( 0 );
    }
}

void pawsConfigHPandMana::UpdateManaDangerLevel( )
{
    if( !loaded ) return;
    csString temp;
    float tLevel = ManaDangerLevel->GetCurrentValue();

    if( tLevel<100 && tLevel>=1.0  )
    {
        temp.Format("> %2.0f%%",tLevel );
        ManaDangerSetting->SetText( temp );

        //if warnlevel is > danger level in % mode, then increase danger level to match.
        if( ManaWarnLevel->GetCurrentValue()>tLevel )
        {
            tLevel=ManaWarnLevel->GetCurrentValue();
        }
        ((pawsShortcutWindow*)(ShortcutMenu))->SetManaDangerLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
        InfoWindow->SetManaDangerLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
    }
    else
    {
        ManaDangerSetting->SetText( "Disabled" );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetManaDangerLevel( 0 );
        InfoWindow->SetManaDangerLevel( 0 );
    }
}

void pawsConfigHPandMana::UpdateManaFlashLevel( )
{
    if( !loaded ) return;
    csString temp;
    float tLevel = ManaFlashLevel->GetCurrentValue();

    if( tLevel<100 && tLevel>=1.0  )
    {
        temp.Format("> %2.0f%%",tLevel );
        ManaFlashSetting->SetText( temp );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetManaFlashLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
        InfoWindow->SetManaFlashLevel(  tLevel/100 ); //convert 0-100 int to float for pawsProgressMeter
    }
    else
    {
        ManaFlashSetting->SetText( "Disabled" );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetManaFlashLevel( 0 );
        ((pawsShortcutWindow*)(ShortcutMenu))->SetManaOn( true );
        InfoWindow->SetManaFlashLevel( 0 );
    }
}


void pawsConfigHPandMana::SetDefault()
{
    LoadConfig();
}

bool pawsConfigHPandMana::OnScroll(int /*scrollDir*/, pawsScrollBar* wdg)
{
    if( !loaded ) return false;

    if(wdg == HPWarnLevel )
    {
        if( HPWarnLevel->GetCurrentValue()>0 )
        {
            if( HPWarnLevel->GetCurrentValue()<HPDangerLevel->GetCurrentValue() )
            {
                HPDangerLevel->SetCurrentValue( HPWarnLevel->GetCurrentValue() );
            }
            ((pawsShortcutWindow*)(ShortcutMenu))->SetHPWarnLevel(  HPWarnLevel->GetCurrentValue()/100 );
            InfoWindow->SetHPWarnLevel(  HPWarnLevel->GetCurrentValue()/100 );
            UpdateHPWarnLevel();
        }
    }
    else if(wdg == HPDangerLevel )
    {
        if( HPDangerLevel->GetCurrentValue()>0 )
        {
            if( HPDangerLevel->GetCurrentValue()>HPWarnLevel->GetCurrentValue() )
            {
                HPDangerLevel->SetCurrentValue( HPWarnLevel->GetCurrentValue() );
            }
            ((pawsShortcutWindow*)(ShortcutMenu))->SetHPDangerLevel(  HPDangerLevel->GetCurrentValue()/100 );
            InfoWindow->SetHPDangerLevel(  HPDangerLevel->GetCurrentValue()/100 );
            UpdateHPDangerLevel();
        }
    }
    else if(wdg == HPFlashLevel )
    {
        if( HPFlashLevel->GetCurrentValue()>0 )
        {
            ((pawsShortcutWindow*)(ShortcutMenu))->SetHPFlashLevel(  HPFlashLevel->GetCurrentValue()/100 );
            InfoWindow->SetHPFlashLevel(  HPFlashLevel->GetCurrentValue()/100 );
        }
        UpdateHPFlashLevel();
    }
    else if(wdg == ManaWarnLevel )
    {
        if( ManaWarnLevel->GetCurrentValue()>0 )
        {
            if( ManaWarnLevel->GetCurrentValue()<ManaDangerLevel->GetCurrentValue() )
            {
                ManaDangerLevel->SetCurrentValue( ManaWarnLevel->GetCurrentValue() );
            }
            ((pawsShortcutWindow*)(ShortcutMenu))->SetManaWarnLevel(  ManaWarnLevel->GetCurrentValue()/100 );
            InfoWindow->SetManaWarnLevel(  ManaWarnLevel->GetCurrentValue()/100 );
            UpdateManaWarnLevel();
        }
    }
    else if(wdg == ManaDangerLevel )
    {
        if( ManaDangerLevel->GetCurrentValue()>0 )
        {
            if( ManaDangerLevel->GetCurrentValue()>ManaWarnLevel->GetCurrentValue() )
            {
                ManaDangerLevel->SetCurrentValue( ManaWarnLevel->GetCurrentValue() );
            }
            ((pawsShortcutWindow*)(ShortcutMenu))->SetManaDangerLevel(  ManaDangerLevel->GetCurrentValue()/100 );
            InfoWindow->SetManaDangerLevel(  ManaDangerLevel->GetCurrentValue()/100 );
            UpdateManaDangerLevel();
        }
    }
    else if(wdg == ManaFlashLevel )
    {
        if( ManaFlashLevel->GetCurrentValue()>0 )
        {
            ((pawsShortcutWindow*)(ShortcutMenu))->SetManaFlashLevel(  ManaFlashLevel->GetCurrentValue()/100 );
            InfoWindow->SetManaFlashLevel(  ManaFlashLevel->GetCurrentValue()/100 );
        }
        UpdateManaFlashLevel();
    }
    
    if( loaded )
        SaveConfig();
    ((pawsShortcutWindow*)ShortcutMenu)->Draw();
    InfoWindow->Draw();
    return true;
}

bool pawsConfigHPandMana::OnButtonPressed(int /*button*/, int /*mod*/, pawsWidget* wdg)
{

    return true;
}

void pawsConfigHPandMana::Show()
{
    pawsWidget::Show();
}

void pawsConfigHPandMana::Hide()
{
    if(dirty)
    {
    }

    pawsWidget::Hide();
}


