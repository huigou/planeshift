/*
 * pawsconfigactivemagic.cpp - Author: Joe Lyon
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
#include "pawsconfigactivemagic.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsactivemagicwindow.h"
#include "pawsscrollmenu.h"
#include "paws/pawsradio.h"


pawsConfigActiveMagic::pawsConfigActiveMagic() :
    ActiveMagicWindow(NULL),
    showEffects(NULL),
    autoResize(NULL),
    useImages(NULL),
    showWindow(NULL),
    buttonHeight(NULL),
    buttonWidthMode(NULL),
    buttonWidth(NULL),
//    leftScroll(NULL),
//    rightScroll(NULL),
    warnLevel(NULL),
    dangerLevel(NULL),
    flashLevel(NULL),
    textFont(NULL),
    textSize(NULL),
    textSpacing(NULL),
    ActiveMagic(NULL),
    MenuBar(NULL)

{
    loaded= false;
}

bool pawsConfigActiveMagic::Initialize()
{
    LoadFromFile("configactivemagic.xml");
    return true;
}

bool pawsConfigActiveMagic::PostSetup()
{

//get pointer to ActiveMagic window
    psMainWidget*   Main    = psengine->GetMainWidget();
    if( Main==NULL )
    {
        Error1( "pawsConfigActiveMagic::PostSetup unable to get psMainWidget\n");
        return false;
    }

    ActiveMagicWindow = (pawsActiveMagicWindow *)Main->FindWidget( "ActiveMagicWindow",true );
    if( ActiveMagicWindow==NULL )
    {
        Error1( "pawsConfigActiveMagic::PostSetup unable to get ActiveMagic window\n");
        return false;
    }

    MenuBar = (pawsScrollMenu*)(ActiveMagicWindow->FindWidget( "BuffBar",true ));
    if( MenuBar==NULL )
    {
        Error1( "pawsConfigActiveMagic::PostSetup unable to get MenuBar\n");
        return false;
    }


//get form widgets
    showEffects = (pawsRadioButtonGroup*)FindWidget("showEffects");
    if(!showEffects)
    {
        return false;
    }

    autoResize = (pawsCheckBox*)FindWidget("autoResize");
    if(!autoResize)
    {
        return false;
    }

    useImages = (pawsCheckBox*)FindWidget("useImages");
    if(!useImages)
    {
        return false;
    }

    showWindow = (pawsCheckBox*)FindWidget("showWindow");
    if(!showWindow)
    {
        return false;
    }

   buttonHeight = (pawsScrollBar*)FindWidget("buttonHeight");
    if(!buttonHeight)
    {
        return false;
    }
    buttonHeight->EnableValueLimit(true);
    buttonHeight->SetMinValue(8);
    buttonHeight->SetMaxValue(64);
    buttonHeight->SetCurrentValue(48,false);

    buttonWidthMode = (pawsRadioButtonGroup*)FindWidget("buttonWidthMode");
    if(!buttonWidthMode)
    {
        return false;
    }

    buttonWidth = (pawsScrollBar*)FindWidget("buttonWidth");
    if(!buttonWidth)
    {
        return false;
    }
    buttonWidth->EnableValueLimit(true);
    buttonWidth->SetMinValue(8);
    buttonWidth->SetMaxValue(512);
    buttonWidth->SetCurrentValue(48,false);

    warnLevel = (pawsScrollBar*)FindWidget("warnLevel");
    if(!warnLevel)
    {
        return false;
    }
    warnLevel->EnableValueLimit(true);
    warnLevel->SetMinValue(0);
    warnLevel->SetMaxValue(100);
    warnLevel->SetCurrentValue(100,false);

    warnSetting = (pawsTextBox*)FindWidget("warnSetting");
    if(!warnSetting)
    {
        return false;
    }

    warnMode = (pawsRadioButtonGroup*)FindWidget("warnMode");
    if(!warnMode)
    {
        return false;
    }
    warnMode->SetActive("warnModePercent");
    

    dangerLevel = (pawsScrollBar*)FindWidget("dangerLevel");
    if(!dangerLevel)
    {
        return false;
    }
    dangerLevel->EnableValueLimit(true);
    dangerLevel->SetMinValue(0);
    dangerLevel->SetMaxValue(100);
    dangerLevel->SetCurrentValue(100,false);

    dangerSetting = (pawsTextBox*)FindWidget("dangerSetting");
    if(!dangerSetting)
    {
        return false;
    }

    dangerMode = (pawsRadioButtonGroup*)FindWidget("dangerMode");
    if(!dangerMode)
    {
        return false;
    }
    dangerMode->SetActive("dangerModePercent");

    flashLevel = (pawsScrollBar*)FindWidget("flashLevel");
    if(!flashLevel)
    {
        return false;
    }
    flashLevel->EnableValueLimit(true);
    flashLevel->SetMinValue(0);
    flashLevel->SetMaxValue(100);
    flashLevel->SetCurrentValue(100,false);

    flashSetting = (pawsTextBox*)FindWidget("flashSetting");
    if(!flashSetting)
    {
        return false;
    }

    flashMode = (pawsRadioButtonGroup*)FindWidget("flashMode");
    if(!flashMode)
    {
        return false;
    }
    flashMode->SetActive("flashModePercent");

    textFontLabel = (pawsTextBox*)FindWidget("textFontLabel");
    textFont = (pawsComboBox*)FindWidget("textFont");
    if(!textFont)
    {
        return false;
    }
    csRef<iVFS>           vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    if( vfs )
    {
        csRef<iStringArray>   fileList( vfs->FindFiles( "/planeshift/data/ttf/*.ttf" )); 
        for (size_t i = 0; i < fileList->GetSize(); i++)
        {
            csString fileName ( fileList->Get (i));
            fileName.DeleteAt( 0, 21 ); // remove the leading path.
            fileName.ReplaceAll( ".ttf", "" );
                
            textFont->NewOption( fileName.GetData() );
        }
    }
    else
    {
        Error1( "pawsConfigActiveMagic::PostSetup unable to find vfs for font list" );
    }
    
    textSizeLabel = (pawsTextBox*)FindWidget("textSizeLabel");
    textSize = (pawsScrollBar*)FindWidget("textSize");
    if(!textSize)
    {
        return false;
    }
    textSize->EnableValueLimit(true);
    textSize->SetMinValue(6);
    textSize->SetCurrentValue(10,false);
    textSize->SetMaxValue(40);

    textSpacingLabel = (pawsTextBox*)FindWidget("textSpacingLabel");
    textSpacing = (pawsScrollBar*)FindWidget("textSpacing");
    if(!textSpacing)
    {
        return false;
    }
    textSpacing->EnableValueLimit(true);
    textSpacing->SetMinValue(0);
    textSpacing->SetCurrentValue(4,false);
    textSpacing->SetMaxValue(20);


    return true;
}

bool pawsConfigActiveMagic::LoadConfig()
{
    useImages->SetState( ActiveMagicWindow->GetUseImages() ); 
    if( useImages->GetState()==true )
    {
        textFontLabel->Hide();
        textFont->Hide();
        textSizeLabel->Hide();
        textSize->Hide();
        textSpacingLabel->Hide();
        textSpacing->Hide();
    }
    else
    {
        textFontLabel->Show();
        textFont->Show();
        textSizeLabel->Show();
        textSize->Show();
        textSpacingLabel->Show();
        textSpacing->Show();
    }

    autoResize->SetState( ActiveMagicWindow->GetAutoResize() ); 
    showEffects->SetActive( ActiveMagicWindow->GetShowEffects()?"itemAndSpell":"spellOnly" ); 
    showWindow->SetState( ActiveMagicWindow->IsVisible() ); 

    buttonHeight->SetCurrentValue( MenuBar->GetButtonHeight() );
    if( MenuBar->GetButtonWidth()==0 )
    {
        buttonWidthMode->SetActive( "buttonWidthAutomatic" );
        buttonWidth->SetCurrentValue( 0 );
        buttonWidth->Hide();
    }
    else
    {
        buttonWidthMode->SetActive( "buttonWidthManual" );
        buttonWidth->SetCurrentValue( MenuBar->GetButtonWidth() );
        buttonWidth->Show();
    }

    textSpacing->SetCurrentValue( MenuBar->GetButtonPaddingWidth() );

//    switch(  MenuBar->GetLeftScroll() )
//    {
//        case ScrollMenuOptionENABLED :
//        {
//            leftScroll->SetActive( "buttonScrollOn" );
//        }
//        break;
//
//        case ScrollMenuOptionDYNAMIC :
//        {
//            leftScroll->SetActive( "buttonScrollAuto" );
//        }
//        break;
//
//        case ScrollMenuOptionDISABLED :
//        {
//            leftScroll->SetActive( "buttonScrollOff" );
//        }
//        break;
//
//    }
//
//    switch(  MenuBar->GetRightScroll() )
//    {
//        case ScrollMenuOptionENABLED :
//        {
//            rightScroll->SetActive( "buttonScrollOn" );
//        }
//        break;
//
//        case ScrollMenuOptionDYNAMIC :
//        {
//            rightScroll->SetActive( "buttonScrollAuto" );
//        }
//        break;
//
//        case ScrollMenuOptionDISABLED :
//        {
//            rightScroll->SetActive( "buttonScrollOff" );
//        }
//        break;
//
//    }

    warnLevel->SetCurrentValue(ActiveMagicWindow->GetWarnLevel() );
    if( ActiveMagicWindow->GetWarnMode()==1 )
    {
        warnMode->SetActive( "warnModeSeconds" );
    }
    else 
    {
        warnMode->SetActive( "warnModePercent" );
    }
    UpdateWarnLevel();

    dangerLevel->SetCurrentValue(ActiveMagicWindow->GetDangerLevel() );
    UpdateDangerLevel();
    if( ActiveMagicWindow->GetDangerMode()==1 )
    {
        dangerMode->SetActive( "dangerModeSeconds" );
    }
    else 
    {
        dangerMode->SetActive( "dangerModePercent" );
    }

    flashLevel->SetCurrentValue(ActiveMagicWindow->GetFlashLevel() );
    UpdateFlashLevel();
    if( ActiveMagicWindow->GetFlashMode()==1 )
    {
        flashMode->SetActive( "flashModeSeconds" );
    }
    else 
    {
        flashMode->SetActive( "flashModePercent" );
    }

    textSize->SetCurrentValue(MenuBar->GetFontSize());

    csString tFontName=csString(((pawsActiveMagicWindow*)ActiveMagicWindow)->GetFontName());
    tFontName.DeleteAt(0,21);
    tFontName.ReplaceAll(".ttf","" );
    textFont->Select( tFontName.GetData() );

    loaded= true;
    dirty = false;

    return true;
}

bool pawsConfigActiveMagic::SaveConfig()
{
    csString xml;
    xml = "<activemagic>\n";
    xml.AppendFmt("<useImages on=\"%s\" />\n", useImages->GetState() ? "yes" : "no");
    xml.AppendFmt("<autoResize on=\"%s\" />\n", autoResize->GetState() ? "yes" : "no");
    xml.AppendFmt("<showEffects active=\"%s\" />\n", showEffects->GetActive().GetData());
    xml.AppendFmt("<showWindow on=\"%s\" />\n", showWindow->GetState() ? "yes" : "no");
    xml.AppendFmt("<buttonHeight value=\"%d\" />\n", int(buttonHeight->GetCurrentValue()));
    xml.AppendFmt("<buttonWidthMode active=\"%s\" />\n", buttonWidthMode->GetActive().GetData());
    xml.AppendFmt("<buttonWidth value=\"%d\" />\n", int(buttonWidth->GetCurrentValue()));
    xml.AppendFmt("<warnLevel value=\"%d\" />\n", int(warnLevel->GetCurrentValue()));
    xml.AppendFmt("<warnMode active=\"%s\" />\n", warnMode->GetActive().GetData());
    xml.AppendFmt("<dangerLevel value=\"%d\" />\n", int(dangerLevel->GetCurrentValue()));
    xml.AppendFmt("<dangerMode active=\"%s\" />\n", dangerMode->GetActive().GetData());
    xml.AppendFmt("<flashLevel value=\"%d\" />\n", int(flashLevel->GetCurrentValue()));
    xml.AppendFmt("<flashMode active=\"%s\" />\n", flashMode->GetActive().GetData());
    xml.AppendFmt("<textSize value=\"%d\" />\n", int(textSize->GetCurrentValue()));
    xml.AppendFmt("<textFont value=\"%s\" />\n", textFont->GetSelectedRowString().GetData());
    xml.AppendFmt("<textSpacing value=\"%d\" />\n", int(textSpacing->GetCurrentValue()));

    xml += "</activemagic>\n";

    dirty = false;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/configactivemagic.xml", xml,xml.Length());
}

void pawsConfigActiveMagic::SetDefault()
{
    LoadConfig();
}

bool pawsConfigActiveMagic::OnScroll(int /*scrollDir*/, pawsScrollBar* wdg)
{

    if(!loaded )
        return true;

    if(wdg == buttonWidth )
    {
        if(buttonWidth->GetCurrentValue() < buttonHeight->GetCurrentValue())
            buttonWidth->SetCurrentValue(buttonHeight->GetCurrentValue());
        else if( buttonWidth->GetCurrentValue() > buttonWidth->GetMaxValue() )
            buttonWidth->SetCurrentValue(buttonWidth->GetMaxValue());

        MenuBar->SetButtonWidth( buttonWidth->GetCurrentValue() );

    }
    else if(wdg == buttonHeight )
    {
        if(buttonHeight->GetCurrentValue() < 1)
            buttonHeight->SetCurrentValue(1,false);
        MenuBar->SetButtonHeight( buttonHeight->GetCurrentValue() );
    }
    else if(wdg == textSize)
    {
        if(textSize->GetCurrentValue() < 1)
            textSize->SetCurrentValue(1,false);
        PickText( textFont->GetSelectedRowString(),  textSize->GetCurrentValue() );
    }
    else if(wdg == textSpacing )
    {
        if( textSpacing->GetCurrentValue() < 1 )
            textSpacing->SetCurrentValue(1,false);
        MenuBar->SetButtonPaddingWidth(  textSpacing->GetCurrentValue() );
    }
    else if(wdg == warnLevel )
    {
        UpdateWarnLevel();
    }
    else if(wdg == dangerLevel )
    {
        UpdateDangerLevel();
    }
    else if(wdg == flashLevel )
    {
        UpdateFlashLevel();
    }

    SaveConfig();
    MenuBar->LayoutButtons();
    MenuBar->OnResize();
    ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();

    return true;
}

bool pawsConfigActiveMagic::OnButtonPressed(int /*button*/, int /*mod*/, pawsWidget* wdg)
{
    dirty = true;

    if( !wdg )
        return false;

    if( !loaded )
        return false;

    switch( wdg->GetID() )
    {
        case 1020 : //spell effects only
        {
            ActiveMagicWindow->SetShowEffects(false);
            if( ActiveMagicWindow->GetAutoResize() )
            {
                ActiveMagicWindow->AutoResize();
            }
            csString blankSpell;
            blankSpell="";
            psSpellCastMessage msg(blankSpell, psengine->GetKFactor()); //request the current Active Mgic list
            msg.SendMessage();
        }
        break;

        case 1021 : //Item and spell effects
        {
            ActiveMagicWindow->SetShowEffects(true);
            if( ActiveMagicWindow->GetAutoResize() )
            {
                ActiveMagicWindow->AutoResize();
            }
            csString blankSpell;
            blankSpell="";
            psSpellCastMessage msg(blankSpell, psengine->GetKFactor()); //request the current Active Mgic list
            msg.SendMessage();
        }
        break;

        case 1022 : // use icons (true) or text (false)?
        {
            ActiveMagicWindow->SetUseImages(useImages->GetState());
            if( useImages->GetState()==true )
            {
                textFontLabel->Hide();
                textFont->Hide();
                textSizeLabel->Hide();
                textSize->Hide();
                textSpacingLabel->Hide();
                textSpacing->Hide();
            }
            else
            {
                textFontLabel->Show();
                textFont->Show();
                textSizeLabel->Show();
                textSize->Show();
                textSpacingLabel->Show();
                textSpacing->Show();
            }
            if( ActiveMagicWindow->GetAutoResize() )
            {
                ActiveMagicWindow->AutoResize();
            }
            csString blankSpell;
            blankSpell="";
            psSpellCastMessage msg(blankSpell, psengine->GetKFactor()); //request the current Active Mgic list
            msg.SendMessage();
        }
        break;

        case 1023 : // auto- or manual sizing
        {
            ActiveMagicWindow->SetAutoResize(autoResize->GetState());
            if( ActiveMagicWindow->GetAutoResize() )
            {
                ActiveMagicWindow->AutoResize();
            }
        }
        break;

        case 1024 : // enable or disable the window
        {
            ActiveMagicWindow->SetShowWindow(showWindow->GetState());
            pawsWidget* widget = PawsManager::GetSingleton().FindWidget( "ActiveMagicWindow" );
            if( ActiveMagicWindow->GetShowWindow() )
            {
                widget->Show();
            }
            else
            {
                widget->Hide();
            }
        }
        break;

        case 1000 : //buttonWidthMode == automtic
        {
            MenuBar->SetButtonWidth( 0 );
            buttonWidth->Hide();
        }
        break;

        case 1001 : //buttonWidthMode == manual
        {
            MenuBar->SetButtonWidth( buttonWidth->GetCurrentValue() );
            buttonWidth->Show();
        }
        break;

        case 1002 : //warnMode == percent
        {
            //ActiveMagicWindow->SetWarnMode( 0 ); 
            UpdateWarnLevel();
        }
        break;

        case 1003 : //warnMode == seconds
        {
            //ActiveMagicWindow->SetWarnMode( 1 ); 
            UpdateWarnLevel();
        }
        break;

        case 1004 : //dangerMode == percent
        {
            //ActiveMagicWindow->SetDangerMode( 0 ); 
            UpdateDangerLevel();
        }
        break;

        case 1005 : //dangerMode == seconds
        {
            //ActiveMagicWindow->SetDangerMode( 1 ); 
            UpdateDangerLevel();
        }
        break;

        case 1006 : //flashMode == percent
        {
            UpdateFlashLevel();
        }
        break;

        case 1007 : //flashMode == seconds
        {
            UpdateFlashLevel();
        }
        break;

//        case 1004 :
//        {
//            MenuBar->SetLeftScroll(ScrollMenuOptionENABLED );
//        }
//        break;
//
//        case 1005 :
//        {
//            MenuBar->SetLeftScroll(ScrollMenuOptionDYNAMIC );
//        }
//        break;
//
//        case 1006 :
//        {
//            MenuBar->SetLeftScroll(ScrollMenuOptionDISABLED );
//        }
//        break;
//
//        case 1007 :
//        {
//            MenuBar->SetRightScroll(ScrollMenuOptionENABLED );
//        }
//        break;
//
//        case 1008 :
//        {
//            MenuBar->SetRightScroll(ScrollMenuOptionDYNAMIC );
//        }
//        break;
//        case 1009 :
//        {
//            MenuBar->SetRightScroll(ScrollMenuOptionDISABLED );
//        }
//        break;

        case 1014 :
        {
            MenuBar->EnableButtonBackground( ((pawsCheckBox*)wdg)->GetState() );
        }
        break;

        default :
        {
            Error2( "pawsConfigActiveMagic::OnButtonPressed got unrecognized widget with ID = %i\n", wdg->GetID() );
            return false;
        }
    }

    MenuBar->LayoutButtons();
    MenuBar->OnResize();
    ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
    SaveConfig();

    return true;
}

void pawsConfigActiveMagic::PickText( const char * fontName, int size )
{
    csString    fontPath( "/planeshift/data/ttf/");
    fontPath += fontName;
    fontPath += ".ttf";

    if( loaded )
    {
        ((pawsActiveMagicWindow*)ActiveMagicWindow)->SetFont( fontPath, size);
        MenuBar->SetFont( fontPath, size );
        MenuBar->SetButtonFont( fontPath, size );

        ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
        SaveConfig();
        MenuBar->LayoutButtons();
        MenuBar->OnResize();
    }

}

void pawsConfigActiveMagic::OnListAction(pawsListBox* selected, int status)
{
    PickText( textFont->GetSelectedRowString(),  textSize->GetCurrentValue() );
    MenuBar->LayoutButtons();
    MenuBar->OnResize();
    ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
    SaveConfig();
}

void pawsConfigActiveMagic::Show()
{
    pawsWidget::Show();
}

void pawsConfigActiveMagic::Hide()
{
    pawsWidget::Hide();
}

void pawsConfigActiveMagic::SetMainWindowVisible( bool status )
{
    if( loaded )
    {
        showWindow->SetState( status );
    }
}

void pawsConfigActiveMagic::UpdateWarnLevel( )
{
        csString temp;
        if( strcmp(warnMode->GetActive(),"warnModePercent")==0 )
        {
            float tLevel = warnLevel->GetCurrentValue();

            ActiveMagicWindow->SetWarnMode( 0 ); 
            if( tLevel<100 && tLevel>0  )
            {
                temp.Format("> %2.0f",tLevel );
                warnSetting->SetText( temp );

                //if warnlevel is > danger level in % mode, then increase danger level to match.
                if( tLevel>dangerLevel->GetCurrentValue() )
                {
                    dangerLevel->SetCurrentValue( tLevel );
                }
                ActiveMagicWindow->SetWarnLevel(  tLevel, false ); //convert 0-100 int to float for pawsProgressMeter
            }
            else
            {
                warnSetting->SetText( "Disabled" );
                ActiveMagicWindow->SetWarnLevel( 0, false );
            }
        }
        else
        {
            ActiveMagicWindow->SetWarnMode( 1 ); 
            if( warnLevel->GetCurrentValue()>0 )
            {
                temp.Format("< %2.1f", warnLevel->GetCurrentValue()/10 );
                warnSetting->SetText( temp );

                //if warn level is < danger level in seconds then decrease danger level to match
                if( warnLevel->GetCurrentValue()<dangerLevel->GetCurrentValue() )
                {
                    dangerLevel->SetCurrentValue( warnLevel->GetCurrentValue() );
                }
                ActiveMagicWindow->SetWarnLevel(  warnLevel->GetCurrentValue(), false ); //convert 0-100 int to float for pawsProgressMeter
            }
            else
            {
                warnSetting->SetText( "Disabled" );
                ActiveMagicWindow->SetWarnLevel( 0, false );
            }

        }
}

void pawsConfigActiveMagic::UpdateDangerLevel()
{
    csString temp;
    if( strcmp(dangerMode->GetActive(),"dangerModePercent")==0 )
    {
        float tLevel = dangerLevel->GetCurrentValue();

        ActiveMagicWindow->SetDangerMode( 0 ); 
        //if warn level > danger level in percent mode then increase danger level
        if( warnLevel->GetCurrentValue()>tLevel )
        {
            dangerLevel->SetCurrentValue( warnLevel->GetCurrentValue() );
        }

        if( tLevel<100 && tLevel>0 )
        {
            temp.Format("> %2.0f", tLevel );
            dangerSetting->SetText( temp );
            ActiveMagicWindow->SetDangerLevel(  tLevel, false );
        }
        else
        {
            dangerSetting->SetText( "Disabled" );
            ActiveMagicWindow->SetDangerLevel(  0, false ); //convert 0-100 int to float for pawsProgressMeter
        }

    }
    else
    {
        ActiveMagicWindow->SetDangerMode( 1 ); 
        //if warn level is < danger level in seconds then decrease danger level to match
        if( warnLevel->GetCurrentValue()<dangerLevel->GetCurrentValue() )
        {
            dangerLevel->SetCurrentValue( warnLevel->GetCurrentValue() );
        }

        if( dangerLevel->GetCurrentValue()>0 )
        {
            temp.Format("< %2.1f", dangerLevel->GetCurrentValue()/10 );
            dangerSetting->SetText( temp );
            ActiveMagicWindow->SetDangerLevel(  dangerLevel->GetCurrentValue(), false );
        }
        else
        {
            dangerSetting->SetText( "Disabled" );
            ActiveMagicWindow->SetDangerLevel(  0, false ); //convert 0-100 int to float for pawsProgressMeter
        }
    }
}

void pawsConfigActiveMagic::UpdateFlashLevel()
{
    csString temp;
    if( strcmp(flashMode->GetActive(),"flashModePercent")==0 )
    {
        float tLevel = flashLevel->GetCurrentValue();

        ActiveMagicWindow->SetFlashMode( 0 ); 
        if( tLevel<100 && tLevel>0 )
        {
            temp.Format("> %2.0f", tLevel );
            flashSetting->SetText( temp );
            ActiveMagicWindow->SetFlashLevel(  tLevel, false );
        }
        else
        {
            flashSetting->SetText( "Disabled" );
            ActiveMagicWindow->SetFlashLevel(  0, false ); //convert 0-100 int to float for pawsProgressMeter
        }
    }
    else
    {
        ActiveMagicWindow->SetFlashMode( 1 ); 
        if( flashLevel->GetCurrentValue()>0 )
        {
            temp.Format("< %2.1f", flashLevel->GetCurrentValue()/10 );
            flashSetting->SetText( temp );
            ActiveMagicWindow->SetFlashLevel(  flashLevel->GetCurrentValue(), false );
        }
        else
        {
            flashSetting->SetText( "Disabled" );
            ActiveMagicWindow->SetFlashLevel(  0, false ); //convert 0-100 int to float for pawsProgressMeter
        }
    }
}
