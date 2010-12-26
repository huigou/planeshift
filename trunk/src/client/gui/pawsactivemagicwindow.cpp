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

    buffCategories        = (pawsListBox*)FindWidget("BuffCategories");
    if (!buffCategories)
        return false;
    debuffCategories      = (pawsListBox*)FindWidget("DebuffCategories");
    if (!debuffCategories)
        return false;
    showWindow              = (pawsCheckBox*)FindWidget("ShowActiveMagicWindow");
    if(!showWindow)
        return false;
    if(!LoadSetting())
        return false;

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_ACTIVEMAGIC);

    // do something here....
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

    pawsListBox *list = incoming.type == BUFF ? buffCategories : debuffCategories;
    switch ( incoming.command )
    {
        case psGUIActiveMagicMessage::Add:
        {
            rowEntry.PushBack(incoming.name);
            list->NewTextBoxRow(rowEntry);
            break;
        }
        case psGUIActiveMagicMessage::Remove:
        {
            for (size_t i = 0; i < list->GetRowCount(); i++)
            {
                pawsListBoxRow *row = list->GetRow(i);
                pawsTextBox *name = dynamic_cast<pawsTextBox*>(row->GetColumn(0));
                if (incoming.name == name->GetText())
                    list->Remove(row);
            }

            // If no active magic, hide the window.
            if (debuffCategories->GetRowCount() + buffCategories->GetRowCount() == 0)
                Hide();

            break;
        }
    }
}

void pawsActiveMagicWindow::Close()
{
    Hide();
    if (showWindow->GetState() == show) 
    {
        SaveSetting();
        if (configPopup)
        {
            configPopup->showActiveMagicConfig->SetState(!showWindow->GetState());
        }
    }
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

    root = doc->CreateRoot();

    activeMagicNode = root->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    activeMagicNode->SetValue("activemagic");

    activeMagicOptionsNode = activeMagicNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    activeMagicOptionsNode->SetValue("activemagicoptions");

    showWindowNode = activeMagicOptionsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    showWindowNode->SetValue("showWindow");
    showWindowNode->SetAttributeAsInt("value", showWindow->GetState() ? 0 : 1);

    doc->Write(file);
}
