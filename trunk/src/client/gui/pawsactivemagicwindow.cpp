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
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigpopup.h"

#define BUFF_CATEGORY_PREFIX    "+"
#define DEBUFF_CATEGORY_PREFIX  "-"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool pawsActiveMagicWindow::PostSetup()
{
    pawsWidget::PostSetup();

    configPopup = 0;

    buffList  = (pawsScrollMenu*)FindWidget("BuffBar");
    if(!buffList)
        return false;
    buffList->SetEditLock( ScrollMenuOptionDISABLED );
    if( autoResize )
    {
        buffList->SetLeftScroll( ScrollMenuOptionDISABLED );
        buffList->SetRightScroll( ScrollMenuOptionDISABLED );
    }
    else
    {
        buffList->SetLeftScroll( ScrollMenuOptionDYNAMIC );
        buffList->SetRightScroll( ScrollMenuOptionDYNAMIC );
    }

    debuffList= (pawsScrollMenu*)FindWidget("DebuffBar");
    if(!debuffList)
        return false;
    debuffList->SetEditLock( ScrollMenuOptionDISABLED );
    if( autoResize )
    {
        debuffList->SetLeftScroll( ScrollMenuOptionDISABLED );
        debuffList->SetRightScroll( ScrollMenuOptionDISABLED );
    }
    else
    {
        debuffList->SetLeftScroll( ScrollMenuOptionDYNAMIC );
        debuffList->SetRightScroll( ScrollMenuOptionDYNAMIC );
    }

    showWindow              = (pawsCheckBox*)FindWidget("ShowActiveMagicWindow");
    if(!showWindow)
        return false;
    showWindow->Hide();

    if(!LoadSetting())
        return false;

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_ACTIVEMAGIC);

    return true;
}

bool pawsActiveMagicWindow::Setup(iDocumentNode *node)
{
    useImages  = true;
    autoResize = true;
    showItemEffects = true;

    if( node->GetAttributeValue("name") && strcmp( "ActiveMagicWindow", node->GetAttributeValue("name"))==0 )
    {
        csRef<iDocumentAttributeIterator> attiter = node->GetAttributes();
        csRef<iDocumentAttribute> subnode;

        while ( attiter->HasNext() )
        {

            subnode = attiter->Next();
            if( strcmp( "useImages", subnode->GetName() )==0 )
            {
                if( strcmp( "false", subnode->GetValue() )==0 )
                {
                    useImages=false;
                }
            }
            else if( strcmp( "autoResize", subnode->GetName() )==0 )
            {
                if( strcmp( "false", subnode->GetValue() )==0 )
                {
                    autoResize=false;
                }
            }
            else if( strcmp( "showItemEffects", subnode->GetName() )==0 )
            {
                if( strcmp( "false", subnode->GetValue() )==0 )
                {
                    showItemEffects=false;
                }
            }
        }
    }

    return true;
}


void pawsActiveMagicWindow::HandleMessage( MsgEntry* me )
{
    if(!configPopup)
        configPopup = (pawsConfigPopup*)PawsManager::GetSingleton().FindWidget("ConfigPopup");

    psGUIActiveMagicMessage incoming(me);
    csList<csString> rowEntry;
    show = showWindow->GetState() ? false : true;
    if (!IsVisible() && psengine->loadstate == psEngine::LS_DONE && show)
        ShowBehind();

    switch ( incoming.command )
    {
        case psGUIActiveMagicMessage::Add:
        {
            rowEntry.PushBack(incoming.name);

            if( incoming.type == BUFF )
            {
                if( useImages )
                {
                    csRef<iPawsImage> image;

                    if( incoming.image.Length() >0 )
                    {
                         image = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(incoming.image);
                    }
                    else
                    {
                        image = NULL;
                    }
                    if (image)
                    {
                        buffList->LoadSingle( incoming.name, incoming.image, incoming.name, csString( "" ), 1, this, false );

                    }
                    else
                    {
                        buffList->LoadSingle( incoming.name, csString( "/planeshift/materials/crystal_ball_icon.dds" ), incoming.name, csString( "" ), 1, this, false );
                    }
                }
                else
                {
                    buffList->LoadSingle( incoming.name, csString( "" ), incoming.name, csString( "" ), 1, this, false );
                }
                buffList->SetEditLock( ScrollMenuOptionDISABLED );
                buffList->Resize();
            }
            else // incoming.type == DEBUFF
            {
                if( useImages )
                {
                    csRef<iPawsImage> image;

                    if( incoming.image.Length() >0 )
                    {
                         image = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(incoming.image);
                    }
                    else
                    {
                        image = NULL;
                    }
                    if (image)
                    {
                        debuffList->LoadSingle( incoming.name, incoming.image, incoming.name, csString( "" ), 1, this, false );
                    }
                    else
                    {
                        debuffList->LoadSingle( incoming.name, csString( "danger_01" ), incoming.name, csString( "" ), 1, this, false );
                    }
                }
                else
                {
                    debuffList->LoadSingle( incoming.name, csString( "" ), incoming.name, csString( "" ), 1, this, false );
                }
                debuffList->SetEditLock( ScrollMenuOptionDISABLED );
                debuffList->Resize();
            }

            if( autoResize )
            {
                int    windowPadding = GetScreenFrame().Width() - buffList->GetScreenFrame().Width();    // horizontal space between window & lists
                if( GetScreenFrame().Width() > GetScreenFrame().Height())  //horizontal case
                {
                    int newWidth = buffList->GetTotalButtonWidth(),
                        t = 0;

                    //size to the buff/debuff bar with the greatest icon width
                    if( newWidth < (t=debuffList->GetTotalButtonWidth()) )
                    {
                        newWidth = t;
                    }
                    if( newWidth > buffList->GetButtonHolderWidth() ) //buffList & debuffList should always be the same size...
                    { //need to resize
                        int  buttonPadding = buffList->GetScreenFrame().Width() - buffList->GetButtonHolderWidth(); //get space within scrollbar reserved for scrolling & lock buttons

                        SetSize( newWidth+windowPadding+buttonPadding,  GetScreenFrame().Height() );
                        if( GetScreenFrame().xmax > psengine->GetG2D()->GetWidth() ) //sticking off the edge of the screen
                        {
                            MoveDelta( -(GetScreenFrame().xmax - psengine->GetG2D()->GetWidth()), 0 );
                        }
                    }
                }
                else //vertical case
                {
                    int vertPadding = GetScreenFrame().Height() - buffList->GetScreenFrame().Height() - debuffList->GetScreenFrame().Height(),
                    newHeight = buffList->GetSize();    //how many buttons
                    if( newHeight < debuffList->GetSize() )
                    {
                        newHeight = debuffList->GetSize();
                    }
                    newHeight *= (buffList->GetButtonHeight()+BUTTON_PADDING);  //convert number of buttons to number of pixels
                    
                    if( (newHeight*2)+vertPadding >  GetScreenFrame().Height() )
                    { //need to resize
                        SetSize( GetScreenFrame().Width(), (newHeight*2)+vertPadding );
                        //buffList->SetSize( buffList->GetScreenFrame().Width(), newHeight);
                        //debuffList->SetSize( debuffList->GetScreenFrame().Width(), newHeight );

                        if( GetScreenFrame().ymax > psengine->GetG2D()->GetHeight() ) //sticking off the bottom of the screen
                        {
                            MoveDelta( 0, -(GetScreenFrame().ymax - psengine->GetG2D()->GetHeight()) );
                        }
                    }
                }
            }
            break;
        }
        case psGUIActiveMagicMessage::Remove:
        {
            if( incoming.type == BUFF )
            {
                buffList->RemoveByName( incoming.name );
                buffList->Resize();
            }
            else
            {
                debuffList->RemoveByName( incoming.name );
                debuffList->Resize();
            }

            // If no active magic, hide the window.
            if( buffList->GetSize() < 1 && debuffList->GetSize() < 1 )
                Hide();

            break;
        }
    }
}

void pawsActiveMagicWindow::Close()
{
    Hide();
}

bool pawsActiveMagicWindow::LoadSetting()
{    
    csRef<iDocument> doc;
    csRef<iDocumentNode> root,activeMagicNode, activeMagicOptionsNode;
    csString option;

    doc = ParseFile(psengine->GetObjectRegistry(), CONFIG_ACTIVEMAGIC_FILE_NAME);
    if (doc == NULL)
    {
        //load the default configuration file in case the user one fails (missing or damaged)
        doc = ParseFile(psengine->GetObjectRegistry(), CONFIG_ACTIVEMAGIC_FILE_NAME_DEF);    
        if (doc == NULL)
        {
            Error2("Failed to parse file %s", CONFIG_ACTIVEMAGIC_FILE_NAME_DEF);
            return false;
        }    
    }
   
    root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("activemagic_def.xml or activemagic.xml has no XML root");
        return false;
    }
    
    activeMagicNode = root->GetNode("activemagic");
    if (activeMagicNode == NULL)
    {
        Error1("activemagic_def.xml or activemagic.xml has no <activemagic> tag");
        return false;
    }

    // Load options for Active Magic Window
    activeMagicOptionsNode = activeMagicNode->GetNode("activemagicoptions");
    if (activeMagicOptionsNode != NULL)
    {
        csRef<iDocumentNodeIterator> oNodes = activeMagicOptionsNode->GetNodes();
        while(oNodes->HasNext())
        {
            csRef<iDocumentNode> option = oNodes->Next();
            csString nodeName (option->GetValue());

            if (nodeName == "showWindow")
                showWindow->SetState(!option->GetAttributeValueAsBool("value"));
        }
    }

    return true;
}

void pawsActiveMagicWindow::SaveSetting()
{
    csRef<iFile> file;
    file = psengine->GetVFS()->Open(CONFIG_ACTIVEMAGIC_FILE_NAME,VFS_FILE_WRITE);

    csRef<iDocumentSystem> docsys = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());

    csRef<iDocument> doc = docsys->CreateDocument();        
    csRef<iDocumentNode> root,defaultRoot, activeMagicNode, activeMagicOptionsNode, showWindowNode;
    //csRef<iDocumentNode> root,defaultRoot, activeMagicNode, activeMagicOptionsNode;

    root = doc->CreateRoot();

    activeMagicNode = root->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    activeMagicNode->SetValue("activemagic");

    activeMagicOptionsNode = activeMagicNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    activeMagicOptionsNode->SetValue("activemagicoptions");

    doc->Write(file);
}
