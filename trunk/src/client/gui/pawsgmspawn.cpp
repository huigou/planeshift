/*
 * pawsgmspawn.cpp - Author: Christian Svensson
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iutil/objreg.h>

#include "engine/loader.h"
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"

#include "../globals.h"
#include "paws/pawslistbox.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawsobjectview.h"
#include "paws/pawstextbox.h"
#include "paws/pawstree.h"
#include "paws/pawscheckbox.h"
#include "pawsgmspawn.h"

pawsGMSpawnWindow::pawsGMSpawnWindow()
{
    itemTree = NULL;
    itemName = NULL;
    itemCount = NULL;
    objView = NULL;
    cbForce = NULL;
    cbLockable = NULL;
    cbLocked = NULL;
    cbPickupable = NULL;
    cbCollidable = NULL;
    lockSkill = NULL;
    lockStr = NULL;
}                 

pawsGMSpawnWindow::~pawsGMSpawnWindow()
{
    psengine->UnregisterDelayedLoader(this);
}

bool pawsGMSpawnWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_GMSPAWNITEMS );
    psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_GMSPAWNTYPES );
    loaded = true;
                
    itemName = (pawsTextBox*)FindWidget("ItemName");
    itemCount= (pawsEditTextBox*)FindWidget("Count");
    objView  = (pawsObjectView*)FindWidget("ItemView");
    itemImage = FindWidget("ItemImage");
    cbForce  = (pawsCheckBox*)FindWidget("Force");
    cbLockable = (pawsCheckBox*)FindWidget("Lockable");
    cbLocked = (pawsCheckBox*)FindWidget("Locked");
    cbPickupable = (pawsCheckBox*)FindWidget("Pickupable");
    cbCollidable = (pawsCheckBox*)FindWidget("Collidable");
    lockSkill = (pawsEditTextBox*)FindWidget("LockSkill");
    lockStr = (pawsEditTextBox*)FindWidget("LockStr");
    factname = (pawsTextBox*)FindWidget("meshfactname");
    meshname = (pawsTextBox*)FindWidget("meshname");
    imagename = (pawsTextBox*)FindWidget("imagename");

    // creates tree:
    itemTree = new pawsSimpleTree;
    if (itemTree == NULL)
    {
        Error1("Could not create widget pawsSimpleTree");
        return false;
    }
    
    AddChild(itemTree);    
    itemTree->SetDefaultColor( graphics2D->FindRGB( 0,255,0 ) );
    itemTree->SetRelativeFrame(0,0,GetActualWidth(250),GetActualHeight(500));
    itemTree->SetNotify(this);
    itemTree->SetAttachFlags(ATTACH_TOP | ATTACH_BOTTOM | ATTACH_LEFT);
    itemTree->SetScrollBars(false, true);
    itemTree->UseBorder("line");
    itemTree->Resize();

    itemTree->InsertChildL("", "TypesRoot", "", "");

    pawsTreeNode * child = itemTree->GetRoot()->GetFirstChild();
    while (child != NULL)
    {
        child->CollapseAll();
        child = child->GetNextSibling();
    }
    
    itemTree->SetNotify(this);

    return true;
}

void pawsGMSpawnWindow::HandleMessage(MsgEntry* me)
{
    if(me->GetType() == MSGTYPE_GMSPAWNITEMS) 
    {
        psGMSpawnItems msg(me);
        for(size_t i = 0;i < msg.items.GetSize(); i++)
        {
            itemTree->InsertChildL(msg.type, msg.items.Get(i).name, msg.items.Get(i).name, "");

            Item item;
            item.name       = msg.items.Get(i).name;
            item.mesh       = msg.items.Get(i).mesh;
            item.icon       = msg.items.Get(i).icon;
            item.category   = msg.type;
            items.Push(item);
        }

    }
    else if(me->GetType() == MSGTYPE_GMSPAWNTYPES) 
    {
        Show();
        psGMSpawnTypes msg(me);
        for(size_t i = 0;i < msg.types.GetSize();i++)
        {
            itemTree->InsertChildL("TypesRoot", msg.types.Get(i), msg.types.Get(i), "");
        }
    }
}

bool pawsGMSpawnWindow::OnSelected(pawsWidget* widget)
{
    pawsTreeNode* node = (pawsTreeNode*)widget;
    if(node->GetFirstChild() != NULL)
        return true;

    if(strcmp(node->GetParent()->GetName(),"TypesRoot"))
    {
        Item item;
        for(size_t i = 0;i < items.GetSize();i++)
        {
            if(items.Get(i).name == node->GetName())
            {
                item = items.Get(i);
                break;
            }
        }

        if ( !item.mesh )
        {
            objView->Clear();
        }
        else 
        {
            psString fact_name(item.mesh);
            fact_name.ReplaceAllSubString("$H", "stonebm");
            item.mesh = fact_name;
            factName = item.mesh;

            csRef<iMeshFactoryWrapper> factory = Loader::GetSingleton().LoadFactory(factName);
            if(!factory)
            {
                loaded = false;
                CheckLoadStatus();
            }
            else
            {
                objView->View(factory);
            }
        }
        
        // set stuff
        itemName->SetText(item.name);
        itemCount->SetText("1");
        itemCount->Show();
        itemImage->Show();
        itemImage->SetBackground(item.icon);
        if (itemImage->GetBackground() != item.icon) // if setting the background failed...hide it
            itemImage->Hide();
        itemName->SetText(item.name);
        pawsWidget* spawnBtn = FindWidget("Spawn");
        spawnBtn->Show();
        cbLockable->SetState(false);
        cbLocked->SetState(false);
        cbPickupable->SetState(true);
        cbCollidable->SetState(false);
        lockSkill->SetText("Lockpicking");
        lockStr->SetText("5");
        imagename->SetText(item.icon);
        factname->SetText(item.mesh);
        meshname->SetText(item.name);

        cbLockable->Show();
        cbLocked->Show();
        cbPickupable->Show();
        cbCollidable->Show();
        lockStr->Hide();
        lockSkill->Hide();
        imagename->Show();
        meshname->Show();
        factname->Show();

        currentItem = item.name;
        return true;
    }

    psGMSpawnItems msg(widget->GetName());
    msg.SendMessage();

    return true;
}

void pawsGMSpawnWindow::CheckLoadStatus()
{
    if(!loaded)
    {
        csRef<iMeshFactoryWrapper> factory = Loader::GetSingleton().LoadFactory(factName);
        if(factory.IsValid())
        {
            psengine->UnregisterDelayedLoader(this);
            objView->View(factory);
            loaded = true;
        }
        else
        {
            psengine->RegisterDelayedLoader(this);
        }
    }
}

bool pawsGMSpawnWindow::OnButtonPressed(int button,int keyModifier,pawsWidget* widget)
{
    if(widget == cbLockable)
    {
        if(cbLockable->GetState())
        {
            lockSkill->SetText("Lockpicking");
            lockStr->SetText("5");

            lockStr->Show();
            lockSkill->Show();
        }
        else
        {
            lockStr->Hide();
            lockSkill->Hide();
        }
    }
    else if(widget && !strcmp(widget->GetName(),"Spawn"))
    {
        if(currentItem.IsEmpty())
            return true;

        psGMSpawnItem msg(
            currentItem,
            atol(itemCount->GetText()),
            cbLockable->GetState(),
            cbLocked->GetState(),
            lockSkill->GetText(),
            atoi(lockStr->GetText()),
            cbPickupable->GetState(),
            cbCollidable->GetState()
            );
        
        // Send spawn message
        psengine->GetMsgHandler()->SendMessage(msg.msg);
    }
    else if(widget->GetID() == 2001) // Rotate
    {
        iMeshWrapper* mesh = objView->GetObject();
        if(mesh)
        {
            static bool toggle = true;

            if(toggle)
                objView->Rotate(10,0.05f);
            else
                objView->Rotate(-1,0);
            
            toggle = !toggle;
        }
    }
    else if(widget->GetID() == 2002) // Up)
    {
        csVector3 pos = objView->GetCameraPosModifier();
        pos.y += 0.1f;
        objView->SetCameraPosModifier(pos);
    }
    else if(widget->GetID() == 2003) // Down
    {
        csVector3 pos = objView->GetCameraPosModifier();
        pos.y -= 0.1f;
        objView->SetCameraPosModifier(pos);
    }

    return true;
}

void pawsGMSpawnWindow::Show()
{
    itemTree->Clear();
    pawsWidget::Show();
}
