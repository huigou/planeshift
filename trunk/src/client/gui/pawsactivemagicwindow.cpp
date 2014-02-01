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
    autoResize(true),
    showEffects(false),
    show(true),
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
        buffList->OnResize();
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
        buffList->AutoResize();
        AutoResize();
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

    if( numSpells==0 )
    {
        Hide();
    }
    else
    {
        for( size_t i=0; i<numSpells; i++ )
        {
            if(incoming.duration[i]==0 && showEffects==false)
            {
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
                    buffList->LoadSingle(incoming.name[i], incoming.image[i], incoming.name[i], csString(""), 0, this, false);
                }
                else
                {
                    if( incoming.type[i]==BUFF )
                    {
                        buffList->LoadSingle(incoming.name[i], csString("/planeshift/materials/crystal_ball_icon.dds"), incoming.name[i], csString(""), 0, this, false);
                    }
                    else
                    {
                        buffList->LoadSingle(incoming.name[i], csString("danger_01"), incoming.name[i], csString(""), -1, this, false);
                    }
                }
            }
            else
            {
                buffList->LoadSingle(incoming.name[i], csString(""), incoming.name[i], csString(""), 0, this, false);
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
}

void pawsActiveMagicWindow::AutoResize()
{
    int buffSize = 0,
        t = 0;

    if(!buffList)
    {
        return;
    }

// if width>screenwidth, width=screenwidth
// if height>screenheight, height=screenheight
// if left edge < 0, left edge = 0
// if top edge < 0, top edge = 0
// if left edge + width > screenwidth,
//        left edge = screenwidth - width
// if top edge + height > screenheight, top edge  = screenheight - height

    if(Orientation == ScrollMenuOptionHORIZONTAL)
    {
        buffSize = buffList->AutoResize();
        if(buffSize == 0)
        {
            buffSize = buffList->GetButtonWidth();
        }

        SetSize(buffSize+buffList->GetButtonWidth(), GetScreenFrame().Height());
        if(GetScreenFrame().xmax > psengine->GetG2D()->GetWidth())   //sticking off the edge of the screen
        {
            //t = GetScreenFrame().xmax - psengine->GetG2D()->GetWidth();
            t = buffList->GetTotalButtonWidth();
            if(GetScreenFrame().xmin - t < 0)
            {
                t = -GetScreenFrame().xmin; //should put the window at the left ede of the screen
            }
            else
            {
                t = -(GetScreenFrame().xmax - psengine->GetG2D()->GetWidth());
            }
            MoveDelta(t, 0);
        }
    }
    else
    {
        buffSize = buffList->AutoResize();
        if(buffSize == 0)
        {
            buffSize = buffList->GetButtonHeight();
        }

        SetSize(GetScreenFrame().Width(), buffSize+buffList->GetButtonHeight());

        if(GetScreenFrame().ymax > psengine->GetG2D()->GetHeight())   //sticking off the bottom of the screen
        {
            MoveDelta(0, -(GetScreenFrame().ymax - psengine->GetG2D()->GetHeight()));
        }
    }
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
    csRef<iDocumentNode> root,mainNode, optionNode;
    csString fileName;
    fileName = "/planeshift/userdata/options/configactivemagic.xml";

    if(!vfs->Exists(fileName))
    {
       return false; //no saved config to load.
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
