/*
 * pawsstringpromptwindow.cpp - Author: Ondrej Hurt
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


// CS INCLUDES
#include <psconfig.h>

// COMMON INCLUDES
#include "util/log.h"

// PAWS INCLUDES
#include "pawsstringpromptwindow.h"
#include "pawstextbox.h"
#include "pawsbutton.h"
#include "pawsmanager.h"
#include "pawsmainwidget.h"


pawsStringPromptWindow::pawsStringPromptWindow()
{
    action = NULL;
}

void pawsStringPromptWindow::Initialize(const csString & label, const csString & string, bool multiline, int width, int height, iOnStringEnteredAction *action, const char *name,int param)
{
    multiLine = multiline;
    if (multiLine)
    {
        inputWidget = new pawsMultilineEditTextBox();
        pawsMultilineEditTextBox *editBox = dynamic_cast<pawsMultilineEditTextBox*> (inputWidget);
        editBox->SetRelativeFrameSize(width, height);
        editBox->SetText(string);
    }
    else
    {
        inputWidget = new pawsEditTextBox();
        pawsEditTextBox *editBox = dynamic_cast<pawsEditTextBox*> (inputWidget);
        editBox->SetRelativeFrameSize(width, height);
        editBox->SetText(string);
    }

    AddChild(inputWidget);
    inputWidget->SetName("stringPromptEntry");

    SetLabel(label);
    PawsManager::GetSingleton().SetCurrentFocusedWidget(inputWidget);
    LayoutWindow();
    //inputWidget->SetBackground("Entry Field Background");
    //inputWidget->SetBackgroundAlpha(0);
    inputWidget->UseBorder("line");
 
    this->action = action;
    this->name   = name;
    this->param  = param;
}

bool pawsStringPromptWindow::OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (action==NULL)
        return false;

    if (widget == okButton)
    {
        CloseWindow();
        return true;
    }
    else if (widget == cancelButton)
    {
        CloseWindow((const char*)NULL);
        return true;
    }
    return false;
}

void pawsStringPromptWindow::CloseWindow()
{
    if (inputWidget != NULL)
    {
        if (multiLine)
            CloseWindow(dynamic_cast<pawsMultilineEditTextBox*> (inputWidget)->GetText());
        else
            CloseWindow(dynamic_cast<pawsEditTextBox*> (inputWidget)->GetText());
    }
}

void pawsStringPromptWindow::CloseWindow(const csString & text)
{
    action->OnStringEntered(name,param,text);
    action = NULL;
    parent->DeleteChild(this);       // destructs itself 
}

bool pawsStringPromptWindow::OnKeyDown( int code, int key, int modifiers )
{
    if ( key==CSKEY_ENTER )
    {
        CloseWindow();
        return true;
    }
    return false;
}



pawsStringPromptWindow * pawsStringPromptWindow::Create(
            const csString & label, const csString & string, 
            bool multiline, int width, int height, 
            iOnStringEnteredAction * action, const char *name,int param, 
            bool modal )
{
    pawsStringPromptWindow * w = new pawsStringPromptWindow();
    PawsManager::GetSingleton().GetMainWidget()->AddChild(w);
    w->PostSetup();
    
    if ( modal )
        PawsManager::GetSingleton().SetModalWidget(w);
    else
    {
        PawsManager::GetSingleton().SetCurrentFocusedWidget(w);
        w->BringToTop(w);
    }
    
    w->Initialize(label, string, multiline, width, height, action, name,param);
    w->MoveTo( (PawsManager::GetSingleton().GetGraphics2D()->GetWidth() - w->GetActualWidth(width) ) / 2,
                       (PawsManager::GetSingleton().GetGraphics2D()->GetHeight() - w->GetActualHeight(height))/2 );
    w->SetMovable(true);
   
    return w;
}
