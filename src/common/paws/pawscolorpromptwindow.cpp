/*
 * pawsnumberpromptwindow.cpp - Author: Ondrej Hurt
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
#include <ctype.h>

// COMMON INCLUDES
#include "util/log.h"

// PAWS INCLUDES
#include "pawscolorpromptwindow.h"
#include "pawstextbox.h"
#include "pawsbutton.h"
#include "pawscrollbar.h"
#include "pawsmanager.h"
#include "pawsmainwidget.h"

class pawsColorInput : public pawsWidget
{
public:
    pawsColorInput()
    {
        SetRelativeFrameSize(140, 80);

        // we have three scroll bars, one for Red, Green and Blue and a button used for "Preview".
        // The preview works following: As soon as the user scrolls, the buttons background colour
        // changes to the set colour.
        scrollBarR = new pawsScrollBar();
        scrollBarR->SetHorizontal(true);
        scrollBarR->SetRelativeFrame(1, 1, 110, 20);
        scrollBarR->SetTickValue(1);
        scrollBarR->PostSetup();

        scrollBarG = new pawsScrollBar();
        scrollBarG->SetHorizontal(true);
        scrollBarG->SetRelativeFrame(1, 31, 110, 20);
        scrollBarG->SetTickValue(1);
        scrollBarG->PostSetup();

        scrollBarB = new pawsScrollBar();
        scrollBarB->SetHorizontal(true);
        scrollBarB->SetRelativeFrame(1, 61, 110, 20);
        scrollBarB->SetTickValue(1);
        scrollBarB->PostSetup();

        buttonPreview = new pawsButton();
        buttonPreview->SetRelativeFrame(115, 35, 20, 20);

        AddChild(scrollBarR);
        AddChild(scrollBarG);
        AddChild(scrollBarB);
        AddChild(buttonPreview);
    }

    pawsScrollBar * scrollBarR;
    pawsScrollBar * scrollBarG;
    pawsScrollBar * scrollBarB;
    pawsButton * buttonPreview;
};

pawsColorPromptWindow::pawsColorPromptWindow()
{
    scrollBarR     = NULL;
    scrollBarG     = NULL;
    scrollBarB     = NULL;
    action         = NULL;

    SetBoundaries(0, 0);

    SetSpacing(5);
}

bool pawsColorPromptWindow::PostSetup()
{
    pawsPromptWindow::PostSetup();

    pawsColorInput * colorInput = new pawsColorInput();
    AddChild(colorInput);
    inputWidget = colorInput;

    scrollBarR = colorInput->scrollBarR;
    scrollBarG = colorInput->scrollBarG;
    scrollBarB = colorInput->scrollBarB;
    buttonPreview = colorInput->buttonPreview;
    return true;
}

bool pawsColorPromptWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (action == NULL)
        return false;

    if (widget == okButton)
    {
        int r, g, b, color;
        r = (int)scrollBarR->GetCurrentValue();
        g = (int)scrollBarG->GetCurrentValue();
        b = (int)scrollBarB->GetCurrentValue();
        //color = r<<16^g<<8^b;
        // the right way to get the colour (should work with all modes)
        color = graphics2D->FindRGB(r, g, b);
        ColorWasEntered(color);
        return true;
    }
    else if (widget == cancelButton)
    {
        if (action != NULL)
        {
            action->OnColorEntered("Cancel",0,-1);
            action = NULL;
        }
        parent->DeleteChild(this);
        return true;
    }
    return false;
}

bool pawsColorPromptWindow::OnKeyDown( int code, int key, int modifiers )
{
    if ( key==CSKEY_ENTER )
    {
        int r, g, b, color;
        r = (int)scrollBarR->GetCurrentValue();
        g = (int)scrollBarG->GetCurrentValue();
        b = (int)scrollBarB->GetCurrentValue();
        //color = r<<16^g<<8^b;
        // the right way to get the colour (should work with all modes)
        color = graphics2D->FindRGB(r, g, b);
        ColorWasEntered(color);
        return true;
    }
    return false;
}

void pawsColorPromptWindow::SetBoundaries(int minColor, int maxColor)
{
    // restrict the RGB range
    csString colorStr;

    this->minColor = minColor;
    this->maxColor = maxColor;

    if (scrollBarR != NULL)
    {
        scrollBarR->SetMinValue(minColor);
        scrollBarR->SetMaxValue(maxColor);
    }
    if (scrollBarG != NULL)
    {
        scrollBarG->SetMinValue(minColor);
        scrollBarG->SetMaxValue(maxColor);
    }
    if (scrollBarB != NULL)
    {
        scrollBarB->SetMinValue(minColor);
        scrollBarB->SetMaxValue(maxColor);
    }

    if (label != NULL)
    {
        if (!strcmp(label->GetText(),""))
        {
            colorStr.Format("%d",maxColor);
            label->SetText("Max "+colorStr);
        }
    }
}

bool pawsColorPromptWindow::OnScroll( int scrollDirection, pawsScrollBar* widget )
{
    int r,g,b;
    r = (int)scrollBarR->GetCurrentValue();
    g = (int)scrollBarG->GetCurrentValue();
    b = (int)scrollBarB->GetCurrentValue();
    //color = r<<16^g<<8^b;
    buttonPreview->SetBackgroundColor(r, g, b);
    return true;
}

void pawsColorPromptWindow::ColorWasEntered(int color)
{
    if (action != NULL)
    {
        action->OnColorEntered(name,param,color);
        action = NULL;
    }
    parent->DeleteChild(this);
}

void pawsColorPromptWindow::Initialize(const csString & labelStr, int color, int minColor, int maxColor,
                                        iOnColorEnteredAction * action,const char *name, int param)
{
    SetLabel(labelStr);

    if (label != NULL)
        label->SetText(labelStr);

    SetBoundaries(minColor, maxColor);
    this->action = action;
    this->name   = name;
    this->param  = param;

    csRef<iGraphics2D> g2d = PawsManager::GetSingleton().GetGraphics2D();
    if (color != -1)
    {
        // the colorStr was used for an input text box which doesn't exist anymore.
        //csString colorStr;
        //colorStr.Format("%X",color);
        int r, g, b;
        g2d->GetRGB(color, r, g, b);
        scrollBarR->SetCurrentValue(r);
        scrollBarG->SetCurrentValue(g);
        scrollBarB->SetCurrentValue(b);
    }
    else
    {
        scrollBarR->SetCurrentValue(minColor);
        scrollBarG->SetCurrentValue(minColor);
        scrollBarB->SetCurrentValue(minColor);
    }
    PawsManager::GetSingleton().SetCurrentFocusedWidget(scrollBarR);
}


pawsColorPromptWindow * pawsColorPromptWindow::Create(const csString & label,
                                                        int color, int minColor, int maxColor,
                                                        iOnColorEnteredAction * action,const char *name, int param)
{
    pawsColorPromptWindow * w = new pawsColorPromptWindow();
    PawsManager::GetSingleton().GetMainWidget()->AddChild(w);
    w->PostSetup();
    w->UseBorder();


    w->SetBackground("Scaling Window Background");
    PawsManager::GetSingleton().SetCurrentFocusedWidget(w);
        w->BringToTop(w);

    w->Initialize(label, color, minColor, maxColor, action,name,param);

    w->SetMovable(true);
    return w;
}
