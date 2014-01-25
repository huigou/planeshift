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
#include "shortcutwindow.h"
#include "pawsscrollmenu.h"
#include "paws/pawsradio.h"

pawsConfigActiveMagic::pawsConfigActiveMagic() :
    showEffects(NULL),
    autoResize(NULL),
    useImages(NULL)
{
    loaded= false;
}

bool pawsConfigActiveMagic::Initialize()
{
    if ( ! LoadFromFile("configactivemagic.xml"))
        return false;

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

    return true;
}

bool pawsConfigActiveMagic::LoadConfig()
{
    useImages->SetState( ActiveMagicWindow->useImages ); 
    autoResize->SetState( ActiveMagicWindow->autoResize ); 
    showEffects->SetActive( ActiveMagicWindow->showEffects?"itemAndSpell":"spellOnly" ); 
    autoResize->SetState( ActiveMagicWindow->showWindow ); 

    loaded= true;
    dirty = false;

    return true;
}

bool pawsConfigActiveMagic::SaveConfig()
{
    csString xml;
    xml = "<activemagic>\n";
    xml.AppendFmt("<useImages on=\"%s\" />\n",
                     useImages->GetState() ? "yes" : "no");
    xml.AppendFmt("<autoResize on=\"%s\" />\n",
                     autoResize->GetState() ? "yes" : "no");
    xml.AppendFmt("<showEffects active=\"%s\" />\n",
                     showEffects->GetActive().GetData());
    xml.AppendFmt("<showWindow on=\"%s\" />\n",
                     showWindow->GetState() ? "yes" : "no");

    xml += "</activemagic>\n";

    dirty = false;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/configactivemagic.xml",
                                         xml,xml.Length());
}

void pawsConfigActiveMagic::SetDefault()
{
    LoadConfig();
}

bool pawsConfigActiveMagic::OnScroll(int /*scrollDir*/, pawsScrollBar* wdg)
{
    
    if( loaded )
        SaveConfig();

    return true;
}

bool pawsConfigActiveMagic::OnButtonPressed(int /*button*/, int /*mod*/, pawsWidget* wdg)
{
    dirty = true;

    switch( wdg->GetID() )
    {
        case 1000 : //spell effects only
        {
            ActiveMagicWindow->showEffects=0;
            if( ActiveMagicWindow->autoResize )
            {
                ActiveMagicWindow->AutoResize();
            }
        }
        break;

        case 1001 : //Item and spell effects
        {
            ActiveMagicWindow->showEffects=1;
            if( ActiveMagicWindow->autoResize )
            {
                ActiveMagicWindow->AutoResize();
            }
        }
        break;

        case 1002 : // use icons (true) or text (false)?
        {
            ActiveMagicWindow->useImages=useImages->GetState();
            if( ActiveMagicWindow->autoResize )
            {
                ActiveMagicWindow->AutoResize();
            }
        }
        break;

        case 1003 : // auto- or manual sizing
        {
            ActiveMagicWindow->autoResize=autoResize->GetState();
            if( ActiveMagicWindow->autoResize )
            {
                ActiveMagicWindow->AutoResize();
            }
        }
        break;

        case 1004 : // enable or disable the window
        {
            ActiveMagicWindow->show=showWindow->GetState();
            pawsWidget* widget = PawsManager::GetSingleton().FindWidget( "ActiveMagicWindow" );
            if( ActiveMagicWindow->show )
            {
                widget->Show();
            }
            else
            {
                widget->Hide();
            }
        }
        break;

        default :
        {
            Error2( "pawsConfigActiveMagic::OnButtonPressed got unrecognized widget with ID = %i\n", wdg->GetID() );
            return false;
        }
    }

    SaveConfig();    

    return true;
}

void pawsConfigActiveMagic::PickText( int index, int size )
{
    SaveConfig();    
}

void pawsConfigActiveMagic::OnListAction(pawsListBox* selected, int status)
{
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

