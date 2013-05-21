/*
 * pawsdndbutton.cpp - Author: Joe Lyon
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
// pawsdndbutton.cpp: implementation of the pawsDnDButton class.
//
//////////////////////////////////////////////////////////////////////

#include <csutil/event.h>

#include <psconfig.h>
#include <ivideo/fontserv.h>
#include <iutil/evdefs.h>
#include <iutil/plugin.h>

#include <isoundmngr.h>

#include <iscenemanipulate.h>
#include "globals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "gui/pawsdndbutton.h"
#include "paws/pawstexturemanager.h"
#include "paws/pawsprefmanager.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsDnDButton::pawsDnDButton()
{
    down = false;
    notify = NULL;
    toggle = false;
    flash = 0;
    flashtype = FLASH_REGULAR;
    keybinding = 0;
    changeOnMouseOver = false;
    originalFontColour = -1;
    factory = "pawsDnDButton";
    dragDrop = false;
}

pawsDnDButton::pawsDnDButton(const pawsDnDButton &pb)
{
    factory = "pawsDnDButton";
}
bool pawsDnDButton::Setup(iDocumentNode* node)
{
    // Check for toggle
    toggle = node->GetAttributeValueAsBool("toggle", true);

    // Check for keyboard shortcut for this button
    const char* key = node->GetAttributeValue("key");
    if(key)
    {
        if(!strcasecmp(key,"Enter"))
            keybinding = 10;
        else
            keybinding = *key;
    }
    // Check for sound to be associated to Buttondown
    csRef<iDocumentAttribute> soundAttribute = node->GetAttribute("sound");
    if(soundAttribute)
    {
        csString soundName = node->GetAttributeValue("sound");
        SetSound(soundName);
    }
    else
    {
        csString name2;

        csRef<iDocumentNode> buttonLabelNode = node->GetNode("label");
        if(buttonLabelNode)
            name2 = buttonLabelNode->GetAttributeValue("text");

        name2.Downcase();

        if(name2 == "ok")
            SetSound("gui.ok");
        else if(name2 == "quit")
            SetSound("gui.quit");
        else if(name2 == "cancel")
            SetSound("gui.cancel");
        else
            SetSound("sound.standardButtonClick");
    }

    // Check for notify widget
    csRef<iDocumentAttribute> notifyAttribute = node->GetAttribute("notify");
    if(notifyAttribute)
        notify = PawsManager::GetSingleton().FindWidget(notifyAttribute->GetValue());

    // Check for mouse over
    changeOnMouseOver = node->GetAttributeValueAsBool("changeonmouseover", false);

    // Get the down button image name.
    csRef<iDocumentNode> buttonDownImage = node->GetNode("buttondown");
    if(buttonDownImage)
    {
        csString downImageName = buttonDownImage->GetAttributeValue("resource");
        SetDownImage(downImageName);
        downTextOffsetX = buttonDownImage->GetAttributeValueAsInt("textoffsetx");
        downTextOffsetY = buttonDownImage->GetAttributeValueAsInt("textoffsety");
    }

    // Get the up button image name.
    csRef<iDocumentNode> buttonUpImage = node->GetNode("buttonup");
    if(buttonUpImage)
    {
        csString upImageName = buttonUpImage->GetAttributeValue("resource");
        SetUpImage(upImageName);
        upTextOffsetX = buttonUpImage->GetAttributeValueAsInt("textoffsetx");
        upTextOffsetY = buttonUpImage->GetAttributeValueAsInt("textoffsety");
    }

    // Get the down button image name.
    csRef<iDocumentNode> buttonGreyDownImage = node->GetNode("buttongraydown");
    if(buttonGreyDownImage)
    {
        csString greyDownImageName = buttonGreyDownImage->GetAttributeValue("resource");
        SetGreyUpImage(greyDownImageName);
    }

    // Get the up button image name.
    csRef<iDocumentNode> buttonGreyUpImage = node->GetNode("buttongrayup");
    if(buttonGreyUpImage)
    {
        csString greyUpImageName = buttonGreyDownImage->GetAttributeValue("resource");
        SetGreyUpImage(greyUpImageName);
    }

    // Get the "on char name flash" button image name.
    csRef<iDocumentNode> buttonSpecialImage = node->GetNode("buttonspecial");
    if(buttonSpecialImage)
    {
        csString onSpecialImageName = buttonSpecialImage->GetAttributeValue("resource");
        SetOnSpecialImage(onSpecialImageName);
    }


    // Get the button label
    csRef<iDocumentNode> buttonLabelNode = node->GetNode("label");
    if(buttonLabelNode)
    {
        buttonLabel = PawsManager::GetSingleton().Translate(buttonLabelNode->GetAttributeValue("text"));
    }

    originalFontColour = GetFontColour();

    return true;
}

bool pawsDnDButton::SelfPopulate(iDocumentNode* node)
{
    if(node->GetAttributeValue("text"))
    {
        SetText(node->GetAttributeValue("text"));
    }

    if(node->GetAttributeValue("down"))
    {
        SetState(strcmp(node->GetAttributeValue("down"),"true")==0);
    }
    SetDnDLock(true);

    return true;
}


pawsDnDButton::~pawsDnDButton()
{
}

void pawsDnDButton::Draw()
{
    pawsWidget::Draw();
    int drawAlpha = -1;
    if(parent && parent->GetMaxAlpha() >= 0)
    {
        fadeVal = parent->GetFadeVal();
        alpha = parent->GetMaxAlpha();
        alphaMin = parent->GetMinAlpha();
        drawAlpha = (int)(alphaMin + (alpha-alphaMin) * fadeVal * 0.010);
    }
    if(down)
    {
        if(!enabled && greyDownImage)
            greyDownImage->Draw(screenFrame, drawAlpha);
        else if(pressedImage)
            pressedImage->Draw(screenFrame, drawAlpha);
    }
    else if(flash==0)
    {
        if(!enabled && greyUpImage)
            greyUpImage->Draw(screenFrame, drawAlpha);
        else if(releasedImage)
            releasedImage->Draw(screenFrame, drawAlpha);
    }
    else // Flash the button if it's not depressed.
    {
        if(flashtype == FLASH_HIGHLIGHT)
        {
            SetColour(graphics2D->FindRGB(255,0,0));
            if(releasedImage)
                releasedImage->Draw(screenFrame, drawAlpha);
        }
        else
        {
            if(flash <= 10)
            {
                flash++;
                switch(flashtype)
                {
                    case FLASH_REGULAR:
                        if(pressedImage)
                            pressedImage->Draw(screenFrame);
                        break;
                    case FLASH_SPECIAL:
                        if(specialFlashImage)
                            specialFlashImage->Draw(screenFrame);
                        break;
                    default:
                        // Unexpected flash
                        Error1("Unknown flash type!");
                }
            }
            else
            {
                if(flash == 30)
                    flash = 1;
                else flash++;
                if(releasedImage) releasedImage->Draw(screenFrame, drawAlpha);
            }
        }
    }
    if(!(buttonLabel.IsEmpty()))
    {
        int drawX=0;
        int drawY=0;
        int width=0;
        int height=0;

        GetFont()->GetDimensions(buttonLabel , width, height);

        int midX = screenFrame.Width() / 2;
        int midY = screenFrame.Height() / 2;

        drawX = screenFrame.xmin + midX - width/2;
        drawY = screenFrame.ymin + midY - height/2;
        drawY -= 2; // correction

        if(down)
            DrawWidgetText(buttonLabel, drawX + downTextOffsetX, drawY + downTextOffsetY);
        else
            DrawWidgetText(buttonLabel, drawX + upTextOffsetX, drawY + upTextOffsetY);
    }
}

bool pawsDnDButton::OnMouseDown(int button, int modifiers, int x, int y)
{
    bool empty;

    if(GetMaskingImage())
    {
        empty = false;
    }
    else if(GetText() && (*GetText())!=0)
    {
        empty = false;
    }
    else
    {
        empty = true;
    }


    if(psengine->GetSlotManager()->IsDragging())
    {
        if(GetDnDLock())
        {
            if(!empty)
            {
                psengine->GetSlotManager()->CancelDrag();

                //if CTRL-ALT-lmb click then clear the button
                if((modifiers & CSMASK_CTRL) && (modifiers & CSMASK_ALT))
                {
                    //Clear();
                    return true;
                }
                //if CTRL-lmb then start drag
                else if(!(modifiers & CSMASK_CTRL))
                {
                }
            }
            else
            {
                if(!mgr)
                {
                    mgr = psengine->GetSlotManager();
                }
                if(!mgr)
                {
                    return false;
                }
                mgr->Handle(this);

                SetID(-GetID());	// communicates up the predecessor icons that this is a DnD action.
            }
        }
        else
        {
            psengine->GetSlotManager()->CancelDrag();
        }
    }
    else
    {
        if(GetID() < 0)
        {
            SetID(abs(GetID()));	// this is no longer a DnD action
        }
        pawsButton::OnMouseDown(button, modifiers, x, y);
    }

    return false;
}

bool pawsDnDButton::CheckKeyHandled(int keyCode)
{
    if(keybinding && keyCode == keybinding)
    {
        OnMouseUp(-1,0,0,0);
        return true;
    }
    return false;
}

bool pawsDnDButton::OnMouseUp(int button, int modifiers, int x, int y)
{
    if(!enabled)
    {
        return false;
    }

    if(!toggle)
        SetState(false, false);

    if(button != -1)   // triggered by keyboard
    {
        // Check to make sure mouse is still in this button
        if(!Contains(x,y))
        {
            Debug1(LOG_PAWS, 0, "Not still in button so not pressed.");
            return true;
        }
    }

    if(notify != NULL)
    {
        notify->OnButtonReleased(button, modifiers, this);
    }
    else if(parent)
    {
        return parent->OnButtonReleased(button, modifiers, this);
    }

    return false;
}

csRef<iPawsImage> pawsDnDButton::GetMaskingImage()
{
    return maskImage;
}

void pawsDnDButton::PlaceItem(const char* imageName, const char* Name, const char* toolTip, const char* action)
{
    SetMaskingImage(imageName);
    this->toolTip = csString(toolTip);
    this->action = csString(action);
    if(ImageNameCallback)
    {
        ImageNameCallback->Get(id-indexBase).Replace(imageName);
    }
    if(NameCallback)
    {
        if(*Name!=0)
        {
            NameCallback->Get(id-indexBase).Replace(Name);
        }
        else
        {
            NameCallback->Get(id-indexBase).Replace(toolTip);
        }
    }
    if(ActionCallback)
    {
        ActionCallback->Get(id-indexBase).Replace(action);
    }
}



void pawsDnDButton::SetMaskingImage(const char* image)
{
    maskingImageName = csString(image);
    pawsWidget::SetMaskingImage(image);
    return;
}