/*
 * pawsactivemagicwindow.cpp
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

// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/strutil.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawsactivemagicwindow.h"
#include "pawsconfigactivemagic.h"
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigpopup.h"

#define BUFF_CATEGORY_PREFIX    "+"
#define DEBUFF_CATEGORY_PREFIX  "-"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsActiveMagicWindow::pawsActiveMagicWindow() :
    useImages(true),
    useTimers(true),
    autoResize(true),
    showEffects(false),
    show(true),
    warnLow(false),
    dangerLow(false),
    flashLow(false),
    warnLevel(0.80),
    dangerLevel(0.90),
    flashLevel(0.95),
    warnMode(0),
    dangerMode(0),
    flashMode(0),
    buffList(NULL),
    lastIndex(0),
    configPopup(NULL)
{
    vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());

    OnResize(); //get orientation set correctly
}

void pawsActiveMagicWindow::OnResize()
{
    if(GetScreenFrame().Width() > GetScreenFrame().Height())
    {
        Orientation = ScrollMenuOptionHORIZONTAL;
    }
    else
    {
        Orientation = ScrollMenuOptionVERTICAL;
    }
    if(buffList!=NULL)
    {
        buffList->SetOrientation(Orientation);
    }
}

bool pawsActiveMagicWindow::PostSetup()
{
    pawsWidget::PostSetup();

    buffList  = (pawsScrollMenu*)FindWidget("BuffBar");
    if(!buffList)
        return false;
    buffList->SetEditLock(ScrollMenuOptionDISABLED);

    if(autoResize)
    {
        buffList->SetLeftScroll(ScrollMenuOptionDISABLED);
        buffList->SetRightScroll(ScrollMenuOptionDISABLED);
    }
    else
    {
        buffList->SetLeftScroll(ScrollMenuOptionDYNAMIC);
        buffList->SetRightScroll(ScrollMenuOptionDYNAMIC);
    }

    showWindow              = (pawsCheckBox*)FindWidget("ShowActiveMagicWindow");
    if(!showWindow)
        return false;

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_ACTIVEMAGIC);

    // If no active magic, hide the window.
    if(buffList->GetSize() < 1 && showWindow->GetState() == false)
    {
        show=false;
        showWindow->Hide();
    }
    else
    {
        show=true;
        showWindow->Show();
    }

    LoadSetting();

    csString blankSpell;
    blankSpell="";
    psSpellCastMessage msg(blankSpell, psengine->GetKFactor()); //request the current Active Mgic list
    msg.SendMessage();

    return true;
}

bool pawsActiveMagicWindow::Setup(iDocumentNode* node)
{

    if(node->GetAttributeValue("name") && strcmp("ActiveMagicWindow", node->GetAttributeValue("name"))==0)
    {
        csRef<iDocumentAttributeIterator> attiter = node->GetAttributes();
        csRef<iDocumentAttribute> subnode;

        while(attiter->HasNext())
        {

            subnode = attiter->Next();
            if(strcmp("useImages", subnode->GetName())==0)
            {
                if(strcmp("false", subnode->GetValue())==0)
                {
                    useImages=false;
                }
            }
            else if(strcmp("autoResize", subnode->GetName())==0)
            {
                if(strcmp("false", subnode->GetValue())==0)
                {
                    autoResize=false;
                }
            }
            else if(strcmp("showEffects", subnode->GetName())==0)
            {
                if(strcmp("true", subnode->GetValue())==0)
                {
                    showEffects=true;
                }
            }
        }
    }

    return true;
}


void pawsActiveMagicWindow::HandleMessage(MsgEntry* me)
{
    if(!configPopup)
        configPopup = (pawsConfigPopup*)PawsManager::GetSingleton().FindWidget("ConfigPopup");

    psGUIActiveMagicMessage incoming(me);
    if( !incoming.valid )
        return;

    // Use signed comparison to handle sequence number wrap around.
    if( (int)incoming.index - (int)lastIndex < 0 )
    {
        return;
    }


    csList<csString> rowEntry;
    show = showWindow->GetState() ? false : true;
    if(!IsVisible() && psengine->loadstate == psEngine::LS_DONE && show)
        ShowBehind();

    size_t    numSpells=incoming.name.GetSize();

    buffList->Clear();

        for( size_t i=0; i<numSpells; i++ )
        {

            pawsDnDButton* newButton = NULL;

            if(incoming.duration[i]==0 && showEffects==false)
            {  //if showEffects is false, don't show anything with a constant effect.
    	        continue;
            }
    	    rowEntry.PushBack(incoming.name[i]);
    
	    if( incoming.image[i] == "none" )
	    {
	        //do not show any icon or text in activemagic window - this suppresses display of things like potions or etc.
	        continue;
	    }

            if(useImages)
            {
                csRef<iPawsImage> image;
                if(incoming.image[i].Length() >0)
                {
                    image = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(incoming.image[i]);
                }
                if(image)
                {
                    newButton = buffList->LoadSingle(incoming.name[i], incoming.image[i], incoming.name[i], csString(""), 0, this, false);
                }
                else
                {
                    if( incoming.type[i]==BUFF )
                    {
                        newButton = buffList->LoadSingle(incoming.name[i], csString("/planeshift/materials/crystal_ball_icon.dds"), incoming.name[i], csString(""), 0, this, false);
                    }
                    else
                    {
                        newButton = buffList->LoadSingle(incoming.name[i], csString("danger_01"), incoming.name[i], csString(""), -1, this, false);
                    }
                }
            }
            else
            {
                newButton = buffList->LoadSingle(incoming.name[i], csString(""), incoming.name[i], csString(""), 0, this, false);
            }

            if( newButton && useTimers )
            {
                //received 3 times from the server:
                //    registeredTime = when the server thinks the spell was cast.
                //    serverTime     = when the server sent the active magic update
                //    duration       = how many ticks the spell will last.
                //
                //plus we get the current client time.
                if( warnMode==0 )
                    newButton->SetWarnLevel(warnLevel/100, warnLow );
                else
                {
                    float ratio = warnLevel*100;
                    if( ratio > incoming.duration[i])
                    {
                        ratio = 1.0;
                    }
                    else
                    {
                        ratio = 1-(ratio/(float)incoming.duration[i]);
                    }
                    newButton->SetWarnLevel( ratio, warnLow );
                }
                if( dangerMode==0 )
                    newButton->SetDangerLevel(dangerLevel/100, dangerLow );
                else
                {
                    float ratio = dangerLevel*100;
                    if( ratio > incoming.duration[i])
                    {
                        ratio = 1.0;
                    }
                    else
                    {
                        ratio = 1-(ratio/(float)incoming.duration[i]);
                    }
                    newButton->SetDangerLevel( ratio, dangerLow );
                }
                if( flashMode==0 )
                    newButton->SetFlashLevel(flashLevel/100, flashLow );
                else
                {
                    float ratio = flashLevel*100;
                    if( ratio > incoming.duration[i])
                    {
                        ratio = 1.0;
                    }
                    else
                    {
                        ratio = 1-(ratio/(float)incoming.duration[i]);
                    }
                    newButton->SetFlashLevel( ratio, flashLow );
                }
                newButton->Start( incoming.registrationTime[i], incoming.serverTime, incoming.duration[i] );
            }
            else
            {

            }
        }
        if(autoResize)
        {
            AutoResize();
        }
        else
        {
            buffList->Resize();
        }
}

void pawsActiveMagicWindow::AutoResize()
{
    if(!buffList)
    {
        return;
    }
    if (buffList->GetSize()<=0)
    {
    	return;
    }
    //what is the starting position ?
    int leftEdge   = GetScreenFrame().xmin,
        rightEdge  = GetScreenFrame().xmax,
        topEdge    = GetScreenFrame().ymin,
        bottomEdge = GetScreenFrame().ymax;

    int newLeftEdge   = 0,
        newRightEdge  = 0,
        newTopEdge    = 0,
        newBottomEdge = 0,
        newHeight     = 0,
        newWidth      = 0,
        tOrientation  = Orientation;
    // Screen boundaries
    int rightScreenEdge = psengine->GetG2D()->GetWidth(),
        bottomScreenEdge = psengine->GetG2D()->GetHeight();

    // Let's keep these values for now
    int verticalScrollWidth    = 10,
    	horizontalScrollHeight = 0;

    int buttonWidth  = buffList->GetButtonWidth(),
    	buttonHeight = buffList->GetButtonHeight(),
        buttonWidestWidth = buffList->GetWidestWidth();

    // Calculate icons surface area. Forget it, only width can vary.
    int totalButtonWidth = buffList->GetTotalButtonWidth();
    // Current amount of buttons
    int buttonsCount = buffList->GetSize();
	// Calculate total amount of rows which would fit the current window size
	int maxTotalRows = int( (bottomScreenEdge - 0 - horizontalScrollHeight) / buttonHeight);
	// Calculate total amount of columns would fit the current window size
	int maxTotalColumns = int(
			(rightScreenEdge - 0 - verticalScrollWidth) / buttonWidestWidth);
	int currentRows = int(
					(bottomEdge - topEdge - horizontalScrollHeight) / buttonHeight);
	int currentColumns = int(
			(rightEdge - leftEdge - verticalScrollWidth) / buttonWidestWidth);
	// Division by zero protection
	currentRows    = (currentRows<=0    ? 1 : currentRows);
	currentColumns = (currentColumns<=0 ? 1 : currentColumns);
	if (Orientation == ScrollMenuOptionHORIZONTAL)
	{
		// Horizontal orientation: try to keep the height value the same and increase the width
		// Calculate required columns amount
		currentColumns = int((buttonsCount - 1) / currentRows) + 1;

		// Trying to fit horizontally
		if (maxTotalColumns < currentColumns)
		{
			currentColumns = maxTotalColumns;
			currentRows = int (buttonsCount / currentColumns);
		}
		// Trying to fit vertically
		if (maxTotalRows < currentRows)
		{
			currentRows = maxTotalRows;
		}
	}
	else
	{
		// Vertical orientation: try to keep the width value the same and increase the height
		// Calculate required rows amount
		currentRows = int ((buttonsCount - 1) / currentColumns) +1;
		// Trying to fit vertically
		if (maxTotalRows < currentRows)
		{
			currentRows = maxTotalRows;
			currentColumns = int (buttonsCount / currentRows);
		}
		// Trying to fit horizontally
		if (maxTotalColumns < currentColumns)
		{
			currentColumns = maxTotalColumns;
		}
	}
	// Calculate new height
	newHeight = currentRows * buttonHeight + BUTTON_PADDING;
	newHeight += horizontalScrollHeight;
	// Calculate new width to fit all available buttons. Widest button width * columns amount
	newWidth = (buttonWidestWidth * currentColumns) + BUTTON_PADDING;
	newWidth += verticalScrollWidth;

	// Calculate top left corner coordinates
	// Align by bottom right corner
	newLeftEdge = rightScreenEdge - newWidth;
	newTopEdge = bottomScreenEdge - newHeight;
	// Return to previous position if it still fits individually for each coordinate
	if (newLeftEdge > leftEdge)
	{
		newLeftEdge = (leftEdge <= 0 ? 0 : leftEdge);
	}
	if (newTopEdge > topEdge)
	{
		newTopEdge = (topEdge <= 0 ? 0 : topEdge);
	}

	// Calculate bottom right corner coordinates
	newRightEdge = newLeftEdge + newWidth;
	newBottomEdge = newTopEdge + newHeight;

    SetRelativeFrame( newLeftEdge, newTopEdge, newWidth, newHeight ); 

    //SetRelativeFrame calls OnResize which will recalculate Orientation, but we don't want
    //autoResize to change orientation. Restore it here.
    Orientation=tOrientation; 
    buffList->SetOrientation(Orientation);

    buffList->SetRelativeFrame( 0, 0, newWidth-verticalScrollWidth, newHeight-horizontalScrollHeight );

}

void pawsActiveMagicWindow::Close()
{
    Hide();
}

void pawsActiveMagicWindow::Show()
{
    pawsConfigActiveMagic* configWindow;

    configWindow = (pawsConfigActiveMagic*)PawsManager::GetSingleton().FindWidget("ConfigActiveMagic");
    if( configWindow ) {
        configWindow->SetMainWindowVisible( true );
    }

    pawsControlledWindow::Show();
}

void pawsActiveMagicWindow::Hide()
{
    pawsConfigActiveMagic* configWindow;

    configWindow = (pawsConfigActiveMagic*)PawsManager::GetSingleton().FindWidget("ConfigActiveMagic");
    if( configWindow ) {
        configWindow->SetMainWindowVisible( false );
    }

    pawsControlledWindow::Hide();
}

bool pawsActiveMagicWindow::LoadSetting()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root,
                         mainNode,
                         optionNode,
                         optionNode2;
    csString fileName;
    fileName = "/planeshift/userdata/options/configactivemagic.xml";

    if(!vfs->Exists(fileName))
    {
       return true; //no saved config to load. Use default values.
    }

    doc = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(), fileName);
    if(doc == NULL)
    {
        Error2("pawsActiveMagicWindow::LoadUserPrefs Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if(root == NULL)
    {
        Error2("pawsActiveMagicWindow::LoadUserPrefs : %s has no XML root",fileName.GetData());
        return false;
    }
    mainNode = root->GetNode("activemagic");
    if(mainNode == NULL)
    {
        Error2("pawsActiveMagicWindow::LoadUserPrefs %s has no <activemagic> tag",fileName.GetData());
        return false;
    }

    optionNode = mainNode->GetNode("useImages");
    if(optionNode != NULL)
    {
        if( strcasecmp( "yes",  optionNode->GetAttributeValue("on") )==0 )
        {
            SetUseImages( true );
        }
        else
        {
            SetUseImages( false );
        }
    }

    optionNode = mainNode->GetNode("autoResize");
    if(optionNode != NULL)
    {
        if( strcasecmp( "yes",  optionNode->GetAttributeValue("on") )==0 )
        {
            SetAutoResize( true );
        }
        else
        {
            SetAutoResize( false );
        }
    }

    optionNode = mainNode->GetNode("showEffects");
    if(optionNode != NULL)
    {
        if( strcasecmp( "itemAndSpell",  optionNode->GetAttributeValue("active") )==0 )
        {
            SetShowEffects( true );
        }
        else
        {
            SetShowEffects( false );
        }
    }

    optionNode = mainNode->GetNode("showWindow");
    if(optionNode != NULL)
    {
        if( strcasecmp( "yes",  optionNode->GetAttributeValue("on") )==0 )
        {
            SetShowWindow( true );
        }
        else
        {
            SetShowWindow( false );
        }
    }

    optionNode = mainNode->GetNode("buttonHeight");
    if(optionNode != NULL)
        buffList->SetButtonHeight( optionNode->GetAttributeValueAsInt("value", true));
    else
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve buttonHeight node");
    }

    optionNode = mainNode->GetNode("buttonWidthMode");
    if(optionNode != NULL)
    {
        if( strcasecmp( "buttonWidthAutomatic", optionNode->GetAttributeValue("active") )==0 )
        {
            buffList->SetButtonWidth( 0 );
        }
        else
        {
            optionNode = mainNode->GetNode("buttonWidth");
            if(optionNode != NULL)
                buffList->SetButtonWidth( optionNode->GetAttributeValueAsInt("value", true));
            else
            {
                Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve buttonWidth");
                return false;
            }
        }
    }
    else
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve buttonWidthMode");
    }

    optionNode = mainNode->GetNode("textSize");
    if(optionNode == NULL)
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve textSize node");
    }

    optionNode2 = mainNode->GetNode("textFont");
    if(optionNode2 == NULL)
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve textFont node");
    }

    if( optionNode != NULL && optionNode2 != NULL )
    {
        fontName = csString( optionNode2->GetAttributeValue("value") );

        csString    fontPath( "/planeshift/data/ttf/");
        fontPath += fontName.GetData();
        fontPath += ".ttf"; 
        SetFont( fontPath, optionNode->GetAttributeValueAsInt("value", true) );
        buffList->SetFont( fontPath, optionNode->GetAttributeValueAsInt("value", true) );
    }
    //else use default.

    optionNode = mainNode->GetNode("textSpacing");
    if(optionNode != NULL)
        buffList->SetButtonPaddingWidth( optionNode->GetAttributeValueAsInt("value", true));
    else
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve textSpacing node");
    }

    optionNode = mainNode->GetNode("warnLevel");
    if(optionNode != NULL)
    {
        warnLevel=optionNode->GetAttributeValueAsFloat("value", true);
    }
//    else
//    {
//        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve warnLevel node");
//        warnLevel = 100;
//    }
    optionNode = mainNode->GetNode("warnMode");
    if(optionNode != NULL)
    {
        if( strcasecmp( "warnModeSeconds",  optionNode->GetAttributeValue("active") )==0 )
        {
            warnMode=1;
        }
        else
        {
            warnMode=0;
        }
    }
//    else
//    {
//        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve warnMode node");
//        warnMode=0;
//    }

    optionNode = mainNode->GetNode("dangerLevel");
    if(optionNode != NULL)
        dangerLevel = optionNode->GetAttributeValueAsFloat("value", true);
//    else
//    {
//        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve dangerLevel node");
//        dangerLevel = 100;
//    }
    optionNode = mainNode->GetNode("dangerMode");
    if(optionNode != NULL)
    {
        if( strcasecmp( "dangerModeSeconds",  optionNode->GetAttributeValue("active") )==0 )
        {
            dangerMode=1;
        }
        else
        {
            dangerMode=0;
        }
    }
//    else
//    {
//        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve dangerMode node");
//        dangerMode=0;
//    }

    optionNode = mainNode->GetNode("flashLevel");
    if(optionNode != NULL)
        flashLevel = optionNode->GetAttributeValueAsFloat("value", true);
//    else
//    {
//        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve flashLevel node");
//        flashLevel=100;
//    }
    optionNode = mainNode->GetNode("flashMode");
    if(optionNode != NULL)
    {
        if( strcasecmp( "flashModeSeconds",  optionNode->GetAttributeValue("active") )==0 )
        {
            flashMode=1;
        }
        else
        {
            flashMode=0;
        }
    }
//    else
//    {
//        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve FlashMode node");
//        flashMode=0;
//    }


    return true;
}

void pawsActiveMagicWindow::SaveSetting()
{
}

void pawsActiveMagicWindow::SetShowEffects( bool setting ) 
{
    showEffects=setting;
}
bool pawsActiveMagicWindow::GetShowEffects()
{
    return showEffects;
}

void pawsActiveMagicWindow::SetUseImages( bool setting ) 
{
    useImages=setting;
}
bool pawsActiveMagicWindow::GetUseImages()
{
    return useImages;
}


void pawsActiveMagicWindow::SetAutoResize( bool setting ) 
{
    autoResize=setting;
}
bool pawsActiveMagicWindow::GetAutoResize()
{
    return autoResize;
}

void pawsActiveMagicWindow::SetShowWindow( bool setting ) 
{
    show=setting;
    showWindow->SetState(setting);
}
bool pawsActiveMagicWindow::GetShowWindow()
{
    return show;
}

int pawsActiveMagicWindow::GetWarnMode()
{
    return warnMode;
}
void pawsActiveMagicWindow::SetWarnMode(int i)
{
    warnMode=i;
}
float pawsActiveMagicWindow::GetWarnLevel()
{
    return warnLevel;
}
void pawsActiveMagicWindow::SetWarnLevel( float val, bool low )
{
    warnLevel = val;
    warnLow   = low;
}

int pawsActiveMagicWindow::GetDangerMode()
{
    return dangerMode;
}
void pawsActiveMagicWindow::SetDangerMode(int i)
{
    dangerMode=i;
}
float pawsActiveMagicWindow::GetDangerLevel()
{
    return dangerLevel;
}
void pawsActiveMagicWindow::SetDangerLevel( float val, bool low )
{
    dangerLevel =val;
    dangerLow   =low;
}

int pawsActiveMagicWindow::GetFlashMode()
{
    return flashMode;
}
void pawsActiveMagicWindow::SetFlashMode(int i)
{
    flashMode=i;
}
float pawsActiveMagicWindow::GetFlashLevel()
{
    return flashLevel;
}
void pawsActiveMagicWindow::SetFlashLevel( float val, bool low )
{
    flashLevel = val;
    flashLow   = low;
}

