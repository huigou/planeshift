/*
 * pawsLoadWindow.h - Author: Andrew Craig
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
#include <psconfig.h>

#include <iutil/objreg.h>

#include "../globals.h"

#include "paws/pawsmanager.h"
#include "pawsloading.h"
#include "paws/pawstextbox.h"

#include "net/cmdhandler.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "iclient/isoundmngr.h"
#include "pscharcontrol.h"


pawsLoadWindow::~pawsLoadWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this,MSGTYPE_MOTD);
}

void pawsLoadWindow::AddText( const char* newText )
{
    loadingText->AddMessage( newText );
}

void pawsLoadWindow::Clear()
{
    loadingText->Clear();
}

bool pawsLoadWindow::PostSetup()
{

    if ( !psengine->GetMsgHandler()->Subscribe(this,MSGTYPE_MOTD))
        return false;

    loadingText = (pawsMessageTextBox*)FindWidget("loadtext");
    
    if ( !loadingText ) 
        return false;
        
    return true;
}

void pawsLoadWindow::HandleMessage(MsgEntry *me)
{
    if ( me->GetType() == MSGTYPE_MOTD )
    {
        psMOTDMessage tipmsg(me);

        pawsMultiLineTextBox* tipBox = (pawsMultiLineTextBox*)FindWidget( "tip" );
        pawsMultiLineTextBox* motdBox = (pawsMultiLineTextBox*)FindWidget( "motd" );
        pawsMultiLineTextBox* guildmotdBox = (pawsMultiLineTextBox*)FindWidget( "guildmotd" );

        //Format the guild motd and motd
        csString motdMsg;
        csString guildmotdMsg;

        motdMsg.Format("Server MOTD: %s",tipmsg.motd.GetData());

        if (!tipmsg.guildmotd.IsEmpty())
            guildmotdMsg.Format("%s's MOTD: %s",tipmsg.guild.GetData(),tipmsg.guildmotd.GetData());

        //Set the text
        tipBox->SetText(tipmsg.tip.GetData());
        motdBox->SetText(motdMsg.GetData());
        guildmotdBox->SetText(guildmotdMsg.GetData());

    }
}

void pawsLoadWindow::Show()
{
    // Play some music
    if(psengine->GetSoundStatus())
    {
        psengine->GetSoundManager()->OverrideBGSong("connect");
    }
    PawsManager::GetSingleton().GetMouse()->Hide(true);
    pawsWidget::Show();
}

void pawsLoadWindow::Hide()
{
    // Play some music
    if (psengine->GetSoundStatus())
    {
        psengine->GetSoundManager()->StopOverrideBG();
    }

    if (!psengine->GetCharControl()->GetMovementManager()->MouseLook()) // do not show the mouse if it was hidden
    PawsManager::GetSingleton().GetMouse()->Hide(false);
    pawsWidget::Hide();
}

