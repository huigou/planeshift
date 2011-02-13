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
    psengine->GetMsgHandler()->Subscribe(this,MSGTYPE_MOTD);

    loadingText = (pawsMessageTextBox*)FindWidget("loadtext");

    dot = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage("Dot");
    
    if(!loadingText) 
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

        //Format the guild motd
        csString guildmotdMsg;

        if (!tipmsg.guildmotd.IsEmpty())
            guildmotdMsg.Format("%s's MOTD: %s",tipmsg.guild.GetData(),tipmsg.guildmotd.GetData());

        //Set the text
        tipBox->SetText(tipmsg.tip.GetData());
        motdBox->SetText(tipmsg.motd.GetData());
        guildmotdBox->SetText(guildmotdMsg.GetData());

    }
}

void pawsLoadWindow::Show()
{
    PawsManager::GetSingleton().GetMouse()->Hide(true);
    pawsWidget::Show();
}

void pawsLoadWindow::Hide()
{
    if (!psengine->GetCharControl()->GetMovementManager()->MouseLook()) // do not show the mouse if it was hidden
    PawsManager::GetSingleton().GetMouse()->Hide(false);
    pawsWidget::Hide();
    renderAnim = false;
}

void pawsLoadWindow::Draw()
{
    if(DrawWindow())
    {
        if(renderAnim)
            DrawAnim();

        DrawForeground();
    }
}

void pawsLoadWindow::InitAnim(csVector2 start, csVector2 dest, csTicks delay)
{
    //if we lack the picture for the anim we don't render it
    if(!dot)
    {
        Error1("Couldn't find the picture to be used for the movement anim. Animation Aborted.");
        return;
    }

    float length;
    renderAnim = true;
    startFrom = 0;
    lastPos = start;
    destination = dest;
    positions.DeleteAll();
    iter = 0;
    
    csVector2 direction = dest - start;
    length = direction.Norm();

    numberDot = (int)ceil(length / 40);
    
    positions.SetSize(numberDot);
    delayBetDot = (delay * 1000) / numberDot; 
}

void pawsLoadWindow::DrawAnim()
{       
    if(startFrom + delayBetDot <= csGetTicks())
    {
        if(iter == 0 || iter + 1 == numberDot) //First and last dot shall not have noise
        {
            positions[iter] = (iter == 0) ? lastPos : destination; //lastPos has the start position of the first dot
            lastPos = positions[iter];
            startFrom = csGetTicks();
            ++iter;
        }
        else
        {
            diffVector.x = psengine->GetRandom() - 0.5f;
            diffVector.y = psengine->GetRandom() - 0.5f;

            csVector2 direction = destination - lastPos;
            direction.Normalize();
            direction += diffVector;
            vel = direction.Unit() * 40;    

            positions[iter] = lastPos + vel;
            lastPos = positions[iter];
            startFrom = csGetTicks();
            ++iter;
        }
    } 
    
    for(size_t i = 0; i < positions.GetSize(); ++i)
    { 
        dot->Draw(positions[i].x, positions[i].y);
    } 
}
