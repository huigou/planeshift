/*
 * pawsscrollmenu.cpp - Author: Joe Lyon
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
// pawsscrollmenu.cpp: implementation of the pawsScrollMenu class.
//
//////////////////////////////////////////////////////////////////////

#include <ivideo/fontserv.h>
#include <iutil/evdefs.h>
#include <iutil/plugin.h>

#include <isoundmngr.h>

#include "paws/pawsmainwidget.h"
#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "gui/pawsdndbutton.h"
#include "pawsscrollmenu.h"
#include "paws/pawstexturemanager.h"
#include "paws/pawsprefmanager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsScrollMenu::pawsScrollMenu()
    : enabled(true) /*, upTextOffsetX(0), upTextOffsetY(0), downTextOffsetX(0), downTextOffsetY(0) */
{
}

pawsScrollMenu::~pawsScrollMenu()
{
}


bool pawsScrollMenu::Setup(iDocumentNode* node)
{
    return true;
}

bool pawsScrollMenu::PostSetup()
{
    size_t i;

    buttonHeight = screenFrame.Height();
    buttonWidth = buttonHeight;

    EditLockButton = new pawsButton;
    AddChild(EditLockButton);
    EditLockButton->SetSound("gui.shortcut");
    EditLockButton->SetID(101);
    EditLockButton->SetToolTip("Prevent/Allow Drag and Drop");
    EditLockButton->SetToggle(true);
    EditLockButton->SetUpImage("lockicon");
    EditLockButton->SetDownImage("unlockicon");
    EditLockButton->SetAlwaysOnTop(true);
    SetEditLock( ScrollMenuOptionENABLED );

    LeftScrollButton = new pawsButton;
    AddChild(LeftScrollButton);
    LeftScrollButton->SetSound("gui.shortcut");
    LeftScrollButton->SetID(102);
    LeftScrollButton->SetToolTip("Scroll left in shortcuts list");
    LeftScrollButton->SetBackgroundAlpha(0);
    LeftScrollButton->SetBackground("Left Arrow");
    LeftScrollButton->SetAlwaysOnTop(true);
    SetLeftScroll( ScrollMenuOptionDYNAMIC );

    ButtonHolder = new pawsWidget;
    AddChild(ButtonHolder);
    ButtonHolder->SetName("ButtonHolder");

    RightScrollButton = new pawsButton;
    AddChild(RightScrollButton);
    RightScrollButton->SetSound("gui.shortcut");
    RightScrollButton->SetID(103);
    RightScrollButton->SetToolTip("Scroll right in shortcuts list");
    RightScrollButton->SetBackground("Right Arrow");
    RightScrollButton->SetBackgroundAlpha(0);
    RightScrollButton->SetAlwaysOnTop(true);
    SetRightScroll( ScrollMenuOptionDYNAMIC );

    buttonLocation=BUTTON_PADDING;

    return true;
}

void pawsScrollMenu::OnResize()
{
    scrollIncrement = ((int)((screenFrame.Width()/buttonWidth)/2))*buttonWidth;

    if(EditLockButton)
    {
        if(EditLockMode>0)
        {
            EditLockButton->Show(); //allows for dynamic enable/disable
            EditLockButton->SetRelativeFrame(0, buttonHeight/4, buttonWidth/2, buttonHeight/2);
        }
        else
        {
            EditLockButton->Hide();
        }
    }
/*    if(LeftScrollButton)
    {
        if( LeftScrollVisible )
        {
            LeftScrollButton->SetRelativeFrame((EditLockMode>0?buttonWidth/2:0), buttonHeight/4, buttonWidth/2, buttonHeight/2);
        }
    } */

    if(ButtonHolder)
    {
        //resize the button holder to fit the new parent window size
        int edgeSpace = buttonWidth/2 + BUTTON_PADDING \
                        + (EditLockMode>0?buttonWidth/2:0) +BUTTON_PADDING \
                        + buttonWidth/2 + BUTTON_PADDING;

        ButtonHolder->SetRelativeFrame((buttonWidth/2)*(EditLockMode>0?2:1)+2*BUTTON_PADDING, 0, GetScreenFrame().Width()-edgeSpace,  GetScreenFrame().Height());

        //if we have buttons then size them properly, show those within the buttonHolder visible area and hide the rest
        if(Buttons.GetSize() >0)
        {

            LayoutButtons();

            // if the last button is not within the visible area (could happen due to removals) shift right
            if(Buttons[ Buttons.GetSize()-1 ]->GetScreenFrame().xmax < 0)   //if the last button is off the left side of the viewing area then move them all.
            {
                int distance;

                //distance = abs(Buttons[ Buttons.GetSize()-1 ]->GetScreenFrame().xmax) + GetScreenFrame().xmax;
                distance =  GetScreenFrame().xmax;
                buttonLocation+=distance;
                LayoutButtons();
            }
        }
    }

    if(LeftScrollButton)
    {
        if( LeftScrollVisible )
        {
            LeftScrollButton->SetRelativeFrame((EditLockMode>0?buttonWidth/2:0), buttonHeight/4, buttonWidth/2, buttonHeight/2);
        }
    }
    if(RightScrollButton)
    {
        if( RightScrollVisible )
        {
            RightScrollButton->SetRelativeFrame(GetScreenFrame().Width()-(buttonWidth/2), buttonWidth/4, buttonWidth/2, buttonHeight/2);
        }
    }

    pawsWidget::OnResize();
}

void pawsScrollMenu::LayoutButtons()
{
    if(Buttons.GetSize()>0)
    {
        int buttonCol = buttonLocation==0?BUTTON_PADDING:buttonLocation,
            buttonRow = 1;

        for(int i=0; i<Buttons.GetSize(); i++)
        {
            if(!Buttons[i])
            {
                printf("pawsScrollMenu::OnResize - ERROR Button[ %i ] is null\n", i);
                continue;
            }
            //perform layout
            int buttonSize = CalcButtonSize((pawsDnDButton*)Buttons[i]);

            if( buttonCol+buttonSize > ButtonHolder->GetScreenFrame().Width())
            {
                if((buttonRow)*buttonHeight < ButtonHolder->GetScreenFrame().Height())  //there's enough vertical space for another row of buttons ...
                {
                    buttonCol = BUTTON_PADDING;
                    buttonRow++;
                }
            }
            Buttons[i]->SetRelativeFrame(buttonCol, 4+((buttonRow-1)*buttonHeight), buttonSize-8, buttonWidth-8);
            buttonCol += buttonSize;

            //Hide/Show...this is necessary because text on hidden widgets is *not* hidden as it should be.
            if(Buttons[i]->GetScreenFrame().xmax <= ButtonHolder->GetScreenFrame().xmin ||  Buttons[i]->GetScreenFrame().xmin >= ButtonHolder->GetScreenFrame().xmax || Buttons[i]->GetScreenFrame().ymax > ButtonHolder->GetScreenFrame().ymax)
            {
                Buttons[i]->Hide();
            }
            else
            {
                Buttons[i]->Show();
            }
        }
        if( LeftScrollMode==ScrollMenuOptionDYNAMIC )
        {
            if( Buttons[0]->GetScreenFrame().xmax <= ButtonHolder->GetScreenFrame().xmin )
            {
                LeftScrollVisible=true;
                LeftScrollButton->Show();
            }
            else 
            {
                LeftScrollVisible=false;
                LeftScrollButton->Hide();
            }
        }
        else if( LeftScrollMode==ScrollMenuOptionENABLED )
        {
                LeftScrollVisible=true;
                LeftScrollButton->Show();
        }
        else
        {
                LeftScrollVisible=true;
                LeftScrollButton->Show();
        }
        if( RightScrollMode==ScrollMenuOptionDYNAMIC )
        {
            if( (Buttons[Buttons.GetSize()-1]->GetScreenFrame().xmin >= ButtonHolder->GetScreenFrame().xmax || Buttons[Buttons.GetSize()-1]->GetScreenFrame().ymin >= ButtonHolder->GetScreenFrame().ymax ) )
            {
                RightScrollVisible=true;
                RightScrollButton->Show();
            }
            else 
            {
                RightScrollVisible=false;
                RightScrollButton->Hide();
            }
        }
        else if( RightScrollMode==ScrollMenuOptionENABLED )
        {
             RightScrollVisible=true;
             RightScrollButton->Show();
        }
        else //RightScrollMode==ScrollMenuOptionDISABLED
        {
             RightScrollVisible=false;
             RightScrollButton->Hide();
        }
    }
    else
    {
        LeftScrollVisible=false;
        LeftScrollButton->Hide();
        RightScrollVisible=false;
        RightScrollButton->Hide();
    }
}

void pawsScrollMenu::OnResizeStop()
{
}

bool pawsScrollMenu::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    return true;
}

bool pawsScrollMenu::OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* widget)
{
    switch(widget->GetID())
    {
        case 101:    //Edit Lock Button - prevents editing & drag-n-drop
        {
            for(int i=0; i<Buttons.GetSize(); i++)
            {
                ((pawsDnDButton*)Buttons[i])->SetDnDLock(EditLockButton->GetState());
            }
            return true;
        }
        case 102:    //left scroll button
        {
            int moveDist;

            if(Buttons.GetSize()>0)
            {
                if(Buttons[0]->GetScreenFrame().xmin + scrollIncrement > ButtonHolder->GetScreenFrame().xmin)
                {
                    moveDist = (ButtonHolder->GetScreenFrame().xmin-Buttons[0]->GetScreenFrame().xmin)+BUTTON_PADDING;   //xmin should be negative...
                }
                else
                {
                    moveDist = scrollIncrement;
                }
                buttonLocation+=moveDist;

                OnResize();
                return true;
            }
            return false;
        }
        case 103:    //right scroll button
        {
            int moveDist;

            if(Buttons.GetSize()>0)   //if there's a least one button
            {
                if(Buttons[Buttons.GetSize()-1]->GetScreenFrame().xmax-scrollIncrement < ButtonHolder->GetScreenFrame().xmax && Buttons[Buttons.GetSize()-1]->GetScreenFrame().ymax <  ButtonHolder->GetScreenFrame().ymax)
                {
                    moveDist = (((int)((Buttons[Buttons.GetSize()-1]->GetScreenFrame().xmax-ButtonHolder->GetScreenFrame().xmax)/buttonWidth))*buttonWidth);
                }
                else
                {
                    moveDist = scrollIncrement;
                }
                if(moveDist<buttonWidth)
                {
                    moveDist=0;
                }
                buttonLocation-=moveDist;

                OnResize();
                return true;
            }
            return false;
        }
    }            // switch( ... )

    callbackWidget->OnButtonReleased(mouseButton, keyModifier, widget);
    return true;
}

bool pawsScrollMenu::LoadArrays(csArray<csString> &name, csArray<csString> &icon, csArray<csString> &toolTip, csArray<csString> &action, int baseIndex, pawsWidget* widget)
{
    int innerSize = 0;

    if(widget)
    {
        callbackWidget = widget;
    }
    else
    {
        callbackWidget = parent;
    }

    if(!name.IsEmpty())
    {
        innerSize = name.GetSize();
    }
    if(!icon.IsEmpty())
    {
        innerSize = innerSize<icon.GetSize()?icon.GetSize():innerSize;
    }
    if(!toolTip.IsEmpty())
    {
        innerSize = innerSize<toolTip.GetSize()?toolTip.GetSize():innerSize;
    }

    for(int i=0; i<innerSize; i++)
    {
        int buttonPos;

        pawsDnDButton* button;
        button = new pawsDnDButton;
        ButtonHolder->AddChild(button);
        Buttons.Push(button);

        button->SetSound("gui.shortcut");
        button->SetBackground("Scaling Button");
        button->ClipToParent(true);
        button->SetIndexBase(baseIndex);
        button->SetID(baseIndex + i);
        button->SetImageNameCallback(&icon);
        button->SetNameCallback(&name);
        button->SetActionCallback(&action);

        if(!icon.IsEmpty() && icon[i])
        {
            button->SetMaskingImage(icon[i]);
            button->SetText("");
        }
        else if(!name.IsEmpty() && name[i])
        {
            // if we want to display text in buttons without images...
            button->SetText(name[i].GetData());
        }
        if(!toolTip.IsEmpty() && toolTip[i])
        {
            button->SetToolTip(toolTip[i]);
        }
    }
    return true;
}

bool pawsScrollMenu::LoadSingle(csString name, csString icon, csString toolTip, csString action, int Index, pawsWidget* widget, bool IsEnabled)
{
    int buttonPos;

    pawsDnDButton* button;
    button = new pawsDnDButton;
    ButtonHolder->AddChild(button);
    Buttons.Push(button);

    button->SetSound("gui.shortcut");
    button->SetBackground("Scaling Button");
    button->ClipToParent(true);
    button->SetID(Index);
    button->SetName(name);
    button->SetEnabled( IsEnabled );

    if(!icon.IsEmpty())
    {
        button->SetMaskingImage(icon);
        button->SetText("");
    }
    else if(!name.IsEmpty())
    {
        // if we want to display text in buttons without images...
        button->SetText(name.GetData());
    }
    if(!toolTip.IsEmpty())
    {
        button->SetToolTip(toolTip);
    }
    return true;
}

bool pawsScrollMenu::RemoveByName(csString name)
{
    pawsWidget*    match = NULL;

    match = ButtonHolder->FindWidget(name.GetData(), true);

    if(match!=NULL)
    {
        int preRemovalSize = ButtonHolder->GetChildrenCount(),
            postRemovalSize = 0;
        ButtonHolder->RemoveChild(match);
        postRemovalSize = ButtonHolder->GetChildrenCount();

        for(int i=0; i<Buttons.GetSize(); i++)
        {
            if(Buttons[i]==match)
            {
                Buttons.DeleteIndex(i);
                break;				//NOTE: if there are multiple entries with this name, we only want to delete the first!
            }
        }
    }
    return true;
}

int pawsScrollMenu::GetSize()
{
    return ButtonHolder->GetChildrenCount();
}


int pawsScrollMenu::CalcButtonSize(pawsDnDButton* target)
{
    if(!target)
    {
        return buttonWidth;
    }
    if(target->GetMaskingImage())
    {
        return buttonWidth;
    }
    else if(target->GetText() && strlen(target->GetText())>0)
    {
        // if we want to display text in buttons without images...
        int paddingWidth,
            textWidth,
            height,
            finalWidth;

        target->GetFont()->GetDimensions(target->GetText(), textWidth, height);
        target->GetFont()->GetDimensions("MM", paddingWidth, height);
        finalWidth = ((int)((paddingWidth+textWidth)/buttonWidth)+1)*buttonWidth;

        return finalWidth;
    }
    else //empty button
    {
        return buttonWidth;
    }
}

bool pawsScrollMenu::SelfPopulate(iDocumentNode* node)
{
    return true;
}


void pawsScrollMenu::Draw()
{
    pawsWidget::Draw();
}

bool pawsScrollMenu::OnMouseEnter()
{
    return true;
}

bool pawsScrollMenu::OnMouseExit()
{
    return true;
}

bool pawsScrollMenu::OnMouseDown(int button, int modifiers, int x, int y)
{
    if( Buttons.GetSize() > 0 )
    {
        if( Buttons[ Buttons.GetSize()-1 ]->GetScreenFrame().xmax <x || Buttons[ Buttons.GetSize()-1 ]->GetScreenFrame().ymax <y )
        {
           parent->OnMouseDown( button, modifiers, GetScreenFrame().xmin + x, GetScreenFrame().ymin + y );
        }
    }
    else
    {
       parent->OnMouseDown( button, modifiers, GetScreenFrame().xmin + x, GetScreenFrame().ymin + y );
    }
    return false;
}

bool pawsScrollMenu::CheckKeyHandled(int keyCode)
{
    return false;
}

bool pawsScrollMenu::OnMouseUp(int button, int modifiers, int x, int y)
{
    return false;
}

bool pawsScrollMenu::OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers)
{
    return pawsWidget::OnKeyDown(keyCode, key, modifiers);
}

void pawsScrollMenu::SetNotify(pawsWidget* widget)
{
}

void pawsScrollMenu::SetEnabled(bool enabled)
{
}

bool pawsScrollMenu::IsEnabled() const
{
    return true;
}

void pawsScrollMenu::SetLeftScroll(int mode)
{
    LeftScrollMode = mode;
    if( LeftScrollMode == ScrollMenuOptionENABLED )
    {
        LeftScrollVisible=true;
    }
    else
    {
        LeftScrollVisible=false;
    }
}

void pawsScrollMenu::SetRightScroll(int mode)
{
    RightScrollMode = mode;
    if( RightScrollMode == ScrollMenuOptionENABLED )
    {
        RightScrollVisible=true;
    }
    else
    {
        RightScrollVisible=false;
    }
}
