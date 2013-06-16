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
    buffList->SetLeftScroll( ScrollMenuOptionDYNAMIC );
    buffList->SetRightScroll( ScrollMenuOptionDYNAMIC );

    debuffList= (pawsScrollMenu*)FindWidget("DebuffBar");
    if(!debuffList)
        return false;
    debuffList->SetEditLock( ScrollMenuOptionDISABLED );
    debuffList->SetLeftScroll( ScrollMenuOptionDYNAMIC );
    debuffList->SetRightScroll( ScrollMenuOptionDYNAMIC );

    if(!LoadSetting())
        return false;

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_ACTIVEMAGIC);

    return true;
}

void pawsActiveMagicWindow::HandleMessage( MsgEntry* me )
{
    if(!configPopup)
        configPopup = (pawsConfigPopup*)PawsManager::GetSingleton().FindWidget("ConfigPopup");

    psGUIActiveMagicMessage incoming(me);
    csList<csString> rowEntry;
//    show = showWindow->GetState() ? false : true;
    show = true;
    if (!IsVisible() && psengine->loadstate == psEngine::LS_DONE && show)
        ShowBehind();

    switch ( incoming.command )
    {
        case psGUIActiveMagicMessage::Add:
        {
            rowEntry.PushBack(incoming.name);

            if( incoming.type == BUFF )
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
                buffList->Resize();
            }
            else
            {
                 csRef<iPawsImage> image = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(incoming.image);
                if (image)
                {
                    debuffList->LoadSingle( incoming.name, incoming.image, incoming.name, csString( "" ), 1, this, false );
                }
                else
                {
                    debuffList->LoadSingle( incoming.name, csString( "danger_01" ), incoming.name, csString( "" ), 1, this, false );
                }
                debuffList->Resize();
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
//            if (debuffCategories->GetRowCount() + buffCategories->GetRowCount() == 0)
            if( buffList->GetSize() < 1 && debuffList->GetSize() < 1 )
                Hide();

            break;
        }
    }
}

void pawsActiveMagicWindow::Close()
{
    Hide();
//    if (showWindow->GetState() == show) 
//    {
//        SaveSetting();
//        if (configPopup)
//        {
//            configPopup->showActiveMagicConfig->SetState(!showWindow->GetState());
//        }
//    }
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

//            if (nodeName == "showWindow")
//                showWindow->SetState(!option->GetAttributeValueAsBool("value"));
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
    //csRef<iDocumentNode> root,defaultRoot, activeMagicNode, activeMagicOptionsNode, showWindowNode;
    csRef<iDocumentNode> root,defaultRoot, activeMagicNode, activeMagicOptionsNode;

    root = doc->CreateRoot();

    activeMagicNode = root->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    activeMagicNode->SetValue("activemagic");

    activeMagicOptionsNode = activeMagicNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    activeMagicOptionsNode->SetValue("activemagicoptions");

    //showWindowNode = activeMagicOptionsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    //showWindowNode->SetValue("showWindow");
    //showWindowNode->SetAttributeAsInt("value", showWindow->GetState() ? 0 : 1);

    doc->Write(file);
}
