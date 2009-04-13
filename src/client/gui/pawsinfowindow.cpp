/*
 * pawsinfowindow.cpp - Author: Andrew Craig
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
// pawsinfowindow.cpp: implementation of the pawsInfoWindow class.
//
//////////////////////////////////////////////////////////////////////

#include <psconfig.h>

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "../globals.h"
#include "clientvitals.h"

// PAWS INCLUDES
#include "pawsinfowindow.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsprogressbar.h"
#include "paws/pawscrollbar.h"
#include "paws/pawsbutton.h"
#include "gui/pawscontrolwindow.h"


pawsInfoWindow::~pawsInfoWindow()
{
    msgHandler->Unsubscribe(this,MSGTYPE_MODE);
}


void pawsInfoWindow::Show()
{
    psStatDRMessage msg;
    psengine->GetMsgHandler()->SendMessage( msg.msg );
    pawsControlledWindow::Show();
}


void pawsInfoWindow::Draw()
{
    pawsWidget::Draw();
}

bool pawsInfoWindow::PostSetup()
{
    pawsControlledWindow::PostSetup();

    msgHandler = psengine->GetMsgHandler();
    if ( !msgHandler->Subscribe(this,MSGTYPE_MODE) )
        return false;

    targetName = (pawsTextBox*)FindWidget( "Targeted" );
    if ( !targetName )
        return false;

    main_hp      = (pawsProgressBar*)FindWidget( "My HP" );
    main_mana    = (pawsProgressBar*)FindWidget( "My Mana" );
    main_stamina[0] = (pawsProgressBar*)FindWidget( "My PysStamina" );
    main_stamina[1] = (pawsProgressBar*)FindWidget( "My MenStamina" );
    if ( !main_hp || !main_mana || !main_stamina[0] || !main_stamina[1] )
        return false;

    target_hp    = (pawsProgressBar*)FindWidget( "Target HP" );
    if ( !target_hp )
        return false;

    main_hp->SetTotalValue(1);
    main_mana->SetTotalValue(1);
    main_stamina[0]->SetTotalValue(1);
    main_stamina[1]->SetTotalValue(1);
    target_hp->SetTotalValue(1);

    kFactor = (pawsScrollBar*)FindWidget( "KFactor" );
    kFactorPct = (pawsTextBox*)FindWidget( "KFactor Pct" );
    if ( !kFactor || !kFactorPct )
        return false;
    kFactor->SetMaxValue(100.0);
    kFactor->SetCurrentValue(0.0);
    kFactor->SetTickValue(10.0);
    kFactor->EnableValueLimit(true);

    // without this the buttons are not displayed at start
    SetStanceHighlight(0);
    pawsWidget* wdg = FindWidget(600);
    csString bg = wdg->GetBackground();
    wdg->SetBackground(bg);

    return true;
}

void pawsInfoWindow::HandleMessage( MsgEntry* me )
{
    switch ( me->GetType() )
    {
        case MSGTYPE_MODE:
        {
            psModeMessage msg(me);
            // We only want to deal with our own stance changes here...
            if (msg.actorID == psengine->GetCelClient()->GetMainPlayer()->GetEID())
            {
                if (msg.mode != psModeMessage::COMBAT && selectedstance)
                    SetStanceHighlight(0);
                else if (msg.mode == psModeMessage::COMBAT && msg.value != selectedstance)
                    SetStanceHighlight(msg.value);
            }
            break;
        }
    }
}

bool pawsInfoWindow::OnScroll( int direction, pawsScrollBar* widget )
{
    csString cmd;

    Notify4(LOG_PAWS,"Scrolling widget %d dir %d to value %.1f\n",
           widget->GetID(), direction,widget->GetCurrentValue());

    if (widget == kFactor)
    {
        float value = kFactor->GetCurrentValue();
        csString tmp;
        tmp.Format("%.0f%%",value);
        kFactorPct->SetText(tmp);
        psengine->SetKFactor(value);
    }
    return true;
}

bool pawsInfoWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* reporter )
{
    csString cmd;

    if ( reporter->GetID() == 600 )
    {
        cmd = "/stopattack";
        SetStanceHighlight(0);
    }
    else
    {
        csString stance = stanceConvert(reporter->GetID());
        cmd.Format("/attack %s", stance.GetData() );
        //SetStanceHighlight(value);
    }

    psengine->GetCmdHandler()->Execute(cmd);
    return true;
}

csString pawsInfoWindow::stanceConvert(const uint ID)
{
    const uint enumID = ID/100;
    switch(enumID)
    {
    case BLOODY:
        return "Bloody";
        break;
    case AGGRESSIVE:
        return "Aggressive";
        break;
    case DEFENSIVE:
        return "Defensive";
        break;
    case FULLYDEFENSIVE:
        return "FullyDefensive";
        break;
    default: // Default stance;
        return "Normal";
        break;
    }
}


void pawsInfoWindow::SetStanceHighlight(int stance)
{
    selectedstance = stance;

    for (int i = 1; i < 6; i++)
    {
        pawsWidget* wdg = FindWidget(i*100);
        csString bg = wdg->GetBackground();
        if (bg.Slice(bg.Length()-6,6) == "Active")
        {
            // Remove Active from the bg and apply it
            bg = bg.Slice(0,bg.Length()-6);
            wdg->SetBackground(bg);
        }
        else
            wdg->SetBackground(bg);
    }

    if (stance > 0 && stance < 6)
    {
        // Set stance active
        pawsWidget* wdg = FindWidget(stance*100);
        csString bg = wdg->GetBackground();
        if (bg.Slice(bg.Length()-6,6) != "Active")
            bg += "Active";

        wdg->SetBackground(bg);
    }
}
