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

    //set defaults
    buttonHeight = screenFrame.Height();
    buttonWidth = buttonHeight;
    buttonWidthDynamic = true;
    scrollIncrement = 0;
    scrollProportion = 0.5;

    EditLockButton = new pawsButton;
    AddChild(EditLockButton);
    EditLockButton->SetSound("gui.shortcut");
    EditLockButton->SetID(101);
    EditLockButton->SetToolTip("Prevent/Allow Editing and Drag/Drop");
    EditLockButton->SetToggle(true);
    EditLockButton->SetUpImage("lockicon");
    EditLockButton->SetDownImage("unlockicon");
    EditLockButton->SetAlwaysOnTop(true);
    SetEditLock( ScrollMenuOptionENABLED );

    LeftScrollButton = new pawsButton;
    AddChild(LeftScrollButton);
    LeftScrollButton->SetSound("gui.shortcut");
    LeftScrollButton->SetID(102);
    LeftScrollButton->SetBackgroundAlpha(0);
    LeftScrollButton->SetAlwaysOnTop(true);
    SetLeftScroll( ScrollMenuOptionDYNAMIC );

    ButtonHolder = new pawsWidget;
    AddChild(ButtonHolder);
    ButtonHolder->SetName("ButtonHolder");

    RightScrollButton = new pawsButton;
    AddChild(RightScrollButton);
    RightScrollButton->SetSound("gui.shortcut");
    RightScrollButton->SetID(103);
    RightScrollButton->SetBackgroundAlpha(0);
    RightScrollButton->SetAlwaysOnTop(true);
    SetRightScroll( ScrollMenuOptionDYNAMIC );

    buttonLocation=BUTTON_PADDING;

    return true;
}

void pawsScrollMenu::setButtonWidth( int width )
{
    if( width>0 )
    {
        buttonWidth = width;
        buttonWidthDynamic = false;
    }
    else
    {
        buttonWidth = 0;
        buttonWidthDynamic = true;
    }
}

void pawsScrollMenu::setScrollIncrement( int incr )
{
    scrollIncrement = incr;
    scrollProportion = 0.0f;
}

void pawsScrollMenu::setScrollProportion( float prop )
{
    scrollProportion = prop;
    scrollIncrement  = 0;
}

void pawsScrollMenu::OnResize()
{
    if(EditLockButton)
    {
        if(EditLockMode>0)
        {
            EditLockButton->Show(); //allows for dynamic enable/disable
            EditLockButton->SetRelativeFrame(0, buttonHeight/4, buttonHeight/2, buttonHeight/2);
        }
        else
        {
            EditLockButton->Hide();
        }
    }
    if(ButtonHolder)
    {
        //resize the button holder to fit the new parent window size
        int edgeSpace = 0,
            topSpace = 0,
            leftEdge = 0;

        if( screenFrame.Width() > screenFrame.Height() ) //horizontal case
        {
            leftEdge =  (LeftScrollMode>ScrollMenuOptionDISABLED?buttonHeight/2 + BUTTON_PADDING:0) \
                        + (EditLockMode>0?buttonHeight/2:0) +BUTTON_PADDING;
            edgeSpace = leftEdge + (RightScrollMode>ScrollMenuOptionDISABLED?buttonHeight/2 + BUTTON_PADDING:0);
            topSpace = 0;
        }
        else //vertical case
        {
            edgeSpace = 0;
            topSpace  = (LeftScrollMode>ScrollMenuOptionDISABLED?buttonHeight/2:0)+(RightScrollMode>ScrollMenuOptionDISABLED?buttonHeight/2:0);
        }

        ButtonHolder->SetRelativeFrame(leftEdge, topSpace, GetScreenFrame().Width()-edgeSpace,  GetScreenFrame().Height()-2*topSpace );

        //if we have buttons then size them properly, show those within the buttonHolder visible area and hide the rest
        if(Buttons.GetSize() >0)
        {

            LayoutButtons();

            // if the last button is not within the visible area (could happen due to removals) shift right
            if(Buttons[ Buttons.GetSize()-1 ]->GetScreenFrame().xmax < 0)   //if the last button is off the left side of the viewing area then move them all.
            {
                int distance;

                distance =  GetScreenFrame().xmax;
                buttonLocation+=distance;
                LayoutButtons();
            }
        }
    }

    if(LeftScrollButton )
    {
        if( LeftScrollVisible && LeftScrollMode>ScrollMenuOptionDISABLED)
        {
            if( screenFrame.Width() > screenFrame.Height() )
            {
                LeftScrollButton->SetBackground("Left Arrow");
                LeftScrollButton->SetToolTip("Scroll left in shortcuts list");
                LeftScrollButton->SetRelativeFrame((EditLockMode>0?buttonHeight/2:0), buttonHeight/4, buttonHeight/2, buttonHeight/2);
            }
            else
            {
                LeftScrollButton->SetBackground("Up Arrow");
                LeftScrollButton->SetToolTip("Scroll up in shortcuts list");
                LeftScrollButton->SetRelativeFrame(buttonHeight, buttonHeight/4, buttonHeight/2, buttonHeight/2);
            }
        }
    }
    if(RightScrollButton)
    {
        if( RightScrollVisible && RightScrollMode>ScrollMenuOptionDISABLED )
        {
            if( screenFrame.Width() > screenFrame.Height() )
            {
                RightScrollButton->SetRelativeFrame(GetScreenFrame().Width()-(buttonHeight/2), buttonHeight/4, buttonHeight/2, buttonHeight/2);
                RightScrollButton->SetToolTip("Scroll right in shortcuts list");
                RightScrollButton->SetBackground("Right Arrow");
            }
            else
            {
                RightScrollButton->SetBackground("Down Arrow");
                RightScrollButton->SetToolTip("Scroll down in shortcuts list");
                RightScrollButton->SetRelativeFrame(buttonHeight, GetScreenFrame().Height()-(buttonHeight/2), buttonHeight/2, buttonHeight/2);
            }
        }
    }

    pawsWidget::OnResize();
}

void pawsScrollMenu::LayoutButtons()
{
    int buttonSize;
    if(Buttons.GetSize()>0)
    {
        int buttonCol = buttonLocation==0?BUTTON_PADDING:buttonLocation,
            buttonRow = 1;

        for(int i=0; i<Buttons.GetSize(); i++)
        {
            if(!Buttons[i])            {
                printf("pawsScrollMenu::OnResize - ERROR Button[ %i ] is null\n", i);
                continue;
            }
            //perform layout
            buttonSize = CalcButtonSize((pawsDnDButton*)Buttons[i]);

            if( buttonCol+buttonSize > ButtonHolder->GetScreenFrame().Width())
            {
                if((buttonRow)*buttonHeight < ButtonHolder->GetScreenFrame().Height())  //there's enough vertical space for another row of buttons ...
                {
                    buttonCol = BUTTON_PADDING;
                    buttonRow++;
                }
            }
            Buttons[i]->SetRelativeFrame(buttonCol, 4+((buttonRow-1)*buttonHeight), buttonSize-8, buttonHeight-8);
            buttonCol += buttonSize;

            //Hide/Show...this is necessary because text on hidden widgets is *not* hidden as it should be, and masking images aren't clipped properly.
            if(Buttons[i]->GetScreenFrame().xmin >=ButtonHolder->GetScreenFrame().xmin && ButtonHolder->GetScreenFrame().Width()<=buttonSize )
            { //special case where a button is wider than the display area; try to show as much as you can...this may be ugly
                Buttons[i]->Show();
            }
            else if(Buttons[i]->GetScreenFrame().xmax <= ButtonHolder->GetScreenFrame().xmin ||  Buttons[i]->GetScreenFrame().xmax > ButtonHolder->GetScreenFrame().xmax || Buttons[i]->GetScreenFrame().ymax >= ButtonHolder->GetScreenFrame().ymax)
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
            if( Buttons[0]->GetScreenFrame().xmax < ButtonHolder->GetScreenFrame().xmin )
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
        else //LeftScrollMode==ScrollMenuOptionDISABLED
        {
                LeftScrollVisible=false;
                LeftScrollButton->Hide();
        }
        if( RightScrollMode==ScrollMenuOptionDYNAMIC )
        {
            if( (Buttons[Buttons.GetSize()-1]->GetScreenFrame().xmax > ButtonHolder->GetScreenFrame().xmax || Buttons[Buttons.GetSize()-1]->GetScreenFrame().ymax > ButtonHolder->GetScreenFrame().ymax ) )
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

bool pawsScrollMenu::ScrollUp()
{
    int moveDist;

    if(Buttons.GetSize()>0)
    {
	//else
	{
	    if( scrollIncrement == 0 && scrollProportion == 0.0f )
	    {
		scrollProportion = 0.5f;
	    }
	    if( scrollProportion > 0 )
	    {
		float sprop;

		if( screenFrame.Width() > screenFrame.Height() )
		{
		    sprop = (float)ButtonHolder->GetScreenFrame().Width()/(float)buttonWidth;
		}
		else
		{
		    sprop = ((float)ButtonHolder->GetScreenFrame().Height()/(float)buttonHeight)*((float)ButtonHolder->GetScreenFrame().Width()/(float)buttonWidth);
		}
		sprop = sprop*scrollProportion;
		scrollIncrement = (int)sprop;
		if( scrollIncrement<1 )
		{
		    scrollIncrement = 1;
		}
		scrollIncrement = scrollIncrement*buttonWidth;
	    }
	    moveDist = scrollIncrement;
	}
	if(Buttons[0]->GetScreenFrame().xmin + scrollIncrement > ButtonHolder->GetScreenFrame().xmin)
	//if(Buttons[0]->GetScreenFrame().xmin > ButtonHolder->GetScreenFrame().xmin)
	{
	    moveDist = (ButtonHolder->GetScreenFrame().xmin-Buttons[0]->GetScreenFrame().xmin)+BUTTON_PADDING;   //xmin should be negative...
	}
	buttonLocation+=moveDist;

	OnResize();
	return true;
    }
    return false;
}

bool pawsScrollMenu::ScrollDown()
{
    int moveDist;

    if(Buttons.GetSize()>0)   //if there's a least one button
    {
	    if( scrollIncrement == 0 && scrollProportion == 0.0f )
	    {
		scrollProportion = 0.5f;
	    }
	    if( scrollProportion > 0 )
	    {
		float sprop;

		if( screenFrame.Width() > screenFrame.Height() )
		{
		    sprop = (float)ButtonHolder->GetScreenFrame().Width()/(float)buttonWidth;
		}
		else
		{
		    sprop = (((float)ButtonHolder->GetScreenFrame().Height()/(float)buttonHeight)-1)*((float)ButtonHolder->GetScreenFrame().Width()/(float)buttonWidth);
		}
		sprop = sprop*scrollProportion;
		scrollIncrement = (int)sprop;
		if( scrollIncrement<1 )
		{
		    scrollIncrement = 1;
		}
		scrollIncrement = scrollIncrement*buttonWidth;
	    }
	if( scrollIncrement<buttonWidth )
	{
	    scrollIncrement = buttonWidth;
	}
	moveDist = scrollIncrement;
	if(Buttons[Buttons.GetSize()-1]->GetScreenFrame().xmax-scrollIncrement < ButtonHolder->GetScreenFrame().xmax && Buttons[Buttons.GetSize()-1]->GetScreenFrame().ymax <  ButtonHolder->GetScreenFrame().ymax)
	{
	    moveDist = (((int)((Buttons[Buttons.GetSize()-1]->GetScreenFrame().xmax-ButtonHolder->GetScreenFrame().xmax)/buttonWidth))*buttonWidth);
	}
	buttonLocation-=moveDist;

	OnResize();
	return true;
    }
    return false;
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
        case 102:    //left or up scroll button
        {
            ScrollUp();
            return true;
        }
        case 103:    //right or down scroll button
        {
            ScrollDown();
            return true;
        }
    }            // switch( ... )

    if( callbackWidget )
    {
        callbackWidget->OnButtonReleased(mouseButton, keyModifier, widget);
    }
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
        if(!action.IsEmpty() && action[i])
        {
            button->SetAction(action[i]);
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

int pawsScrollMenu::GetButtonWidth()
{
    return buttonWidth;
}

int pawsScrollMenu::GetButtonHeight()
{
    return buttonHeight;
}

int  pawsScrollMenu::GetButtonHolderWidth()
{
    return ButtonHolder->GetScreenFrame().Width();
}

int  pawsScrollMenu::GetButtonHolderHeight()
{
    return ButtonHolder->GetScreenFrame().Height();
}

int pawsScrollMenu::GetTotalButtonWidth()
{  
    int total=0;

    for( int i=0; i< Buttons.GetSize(); i++ )
    {
        if( Buttons[i] )
            total+= CalcButtonSize((pawsDnDButton*)Buttons[i]);
    }
    return total;
}

int pawsScrollMenu::CalcButtonSize(pawsDnDButton* target)
{
    if(!target)
    {
        return buttonWidth;
    }
    if( buttonWidthDynamic == false && buttonWidth>0 )
    {
        return buttonWidth;
    }
    if( buttonWidth==0 )
    {
        buttonWidth=buttonHeight;
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

bool pawsScrollMenu::OnMouseDown(int button, int modifiers, int x, int y)
{
    if (button == csmbWheelUp || button == csmbHWheelLeft)
    {
        ScrollUp();
        return true;
    }
    else if(button == csmbWheelDown || button == csmbHWheelRight)
    {
        ScrollDown();
        return true;
    }
    parent->OnMouseDown( button, modifiers, GetScreenFrame().xmin + x, GetScreenFrame().ymin + y );
    return true;
}

bool pawsScrollMenu::OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers)
{
    return pawsWidget::OnKeyDown(keyCode, key, modifiers);
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
