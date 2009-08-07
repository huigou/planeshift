/*
 * pawspromptwindow.cpp - Author: Ondrej Hurt
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
#include "pawspromptwindow.h"
#include "pawsmanager.h"
#include "pawsbutton.h"
#include "pawstextbox.h"


#define DEFAULT_SPACING 10        //distance between inner widgets and border


pawsPromptWindow::pawsPromptWindow()
{
    label          = NULL;
    okButton       = NULL;
    cancelButton   = NULL;
    inputWidget    = NULL;
    spacing        = DEFAULT_SPACING;
}

bool pawsPromptWindow::PostSetup()
{
    SetBackground("Scaling Widget Background");
    UseBorder("line");
    
    label = new pawsTextBox();
    AddChild(label);
    
    okButton = new pawsButton();
    AddChild(okButton);
    okButton->SetUpImage("Scaling Button Up");
    okButton->SetDownImage("Scaling Button Down");
    okButton->SetRelativeFrameSize(80, 25);
    okButton->SetText(PawsManager::GetSingleton().Translate("OK"));
    okButton->SetSound("gui.ok");
    
    cancelButton = new pawsButton();
    AddChild(cancelButton);
    cancelButton->SetUpImage("Scaling Button Up");
    cancelButton->SetDownImage("Scaling Button Down");
    cancelButton->SetRelativeFrameSize(80, 25);
    cancelButton->SetText(PawsManager::GetSingleton().Translate("Cancel"));
    cancelButton->SetSound("gui.cancel");

    SetBackgroundAlpha(0);
    return true;
}

void pawsPromptWindow::SetLabel(const csString & label)
{
    this->label->SetText(label);
    this->label->SetSizeByText(5,5);
    LayoutWindow();
}

void pawsPromptWindow::SetSpacing(int spacing)
{
    this->spacing = spacing;
}

void pawsPromptWindow::LayoutWindow()
{
    int windowWidth, windowHeight;
    
    assert(label && okButton && cancelButton && inputWidget);
    
    windowWidth = MAX(
                        2*spacing + label->DefaultFrame().Width(),
                        MAX(
                             4*spacing + inputWidget->DefaultFrame().Width(),
                             3*spacing + okButton->DefaultFrame().Width() + cancelButton->DefaultFrame().Width()
                           )
                     );
    windowHeight = 4*spacing + label->DefaultFrame().Height() + inputWidget->DefaultFrame().Height() 
                             + okButton->DefaultFrame().Height();
    SetRelativeFrameSize(windowWidth, windowHeight);

    label->SetRelativeFramePos((windowWidth-label->DefaultFrame().Width()) / 2,
                               spacing);

    inputWidget->SetRelativeFramePos((windowWidth-inputWidget->DefaultFrame().Width()) / 2,
                                     label->DefaultFrame().ymax + spacing);

    okButton->SetRelativeFramePos((windowWidth-okButton->DefaultFrame().Width()-cancelButton->DefaultFrame().Width()-spacing) / 2,
                                  inputWidget->DefaultFrame().ymax + spacing);

    cancelButton->SetRelativeFramePos(okButton->DefaultFrame().xmax + spacing,
                                      okButton->DefaultFrame().ymin);

    SetAppropriatePos();
}

void pawsPromptWindow::SetAppropriatePos()
{
    psPoint mouse;
    int x, y;
    int width, height;
    
    width   =  screenFrame.Width();
    height  =  screenFrame.Height();
    
    mouse = PawsManager::GetSingleton().GetMouse()->GetPosition();
    x = mouse.x - width  / 2;
    y = mouse.y - height / 2;
    MoveTo(x, y);
    MakeFullyVisible();
}
