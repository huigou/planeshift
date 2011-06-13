/*
 * psslotmgr.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 */

#include <csutil/event.h>

#include <psconfig.h>
#include "psslotmgr.h"


//=============================================================================
// PlaneShift Includes
//=============================================================================
#include <iscenemanipulate.h>
#include "paws/pawsmainwidget.h"
#include "gui/pawsslot.h"
#include "pscamera.h"
#include "globals.h"


//=============================================================================
// Classes
//=============================================================================


psSlotManager::psSlotManager()
{
    isDragging = false;
    isPlacing = false;
    isRotating = false;

    // Initialize event shortcuts
    MouseMove = csevMouseMove (psengine->GetEventNameRegistry(), 0);
    MouseDown = csevMouseDown (psengine->GetEventNameRegistry(), 0);
    MouseUp = csevMouseUp (psengine->GetEventNameRegistry(), 0);
    KeyDown = csevKeyboardDown (psengine->GetEventNameRegistry());
    KeyUp = csevKeyboardUp (psengine->GetEventNameRegistry());
}


psSlotManager::~psSlotManager()
{
}


bool psSlotManager::HandleEvent( iEvent& ev )
{
    if(isDragging)
    {
        int button = csMouseEventHelper::GetButton(&ev);
        if(ev.Name == MouseMove)
        {
            if(isPlacing)
            {
                // Update item position.
                UpdateItem();
            }
        }
        else if(ev.Name == MouseDown)
        {
            if(button == 0) // Left
            {
                if(isPlacing)
                {
                    // Drop the item at the current position.
                    DropItem(!(csMouseEventHelper::GetModifiers(&ev) & CSMASK_SHIFT));
                    return true;
                }
                else
                {
                    PlaceItem();
                }
            }
            else if(button == 1) // right
            {
                if(!isRotating)
                {
                    basePoint = PawsManager::GetSingleton().GetMouse()->GetPosition();
                    isRotating = true;
                    if(csMouseEventHelper::GetModifiers(&ev) & CSMASK_SHIFT)
                    {
                        psengine->GetSceneManipulator()->SetRotation(PS_MANIPULATE_PITCH,PS_MANIPULATE_YAW);
                    }
                    else
                    {
                        psengine->GetSceneManipulator()->SetRotation(PS_MANIPULATE_ROLL,PS_MANIPULATE_NONE);
                    }
                    return true;
                }
            }
            else
            {
                CancelDrag();
            }
        }
        else if(ev.Name == MouseUp)
        {
            if(button == 1) // right
            {
                if(isRotating)
                {
                    //PawsManager::GetSingleton().GetMouse()->SetPosition(basePoint.x, basePoint.y);
                    psengine->GetG2D()->SetMousePosition(basePoint.x, basePoint.y);
                    psengine->GetSceneManipulator()->SetPosition(csVector2(basePoint.x, basePoint.y));
                    isRotating = false;
                    return true;
                }
            }
        }
        /*else if(ev.Name == KeyDown)
        {
        }
        else if(ev.Name == KeyUp)
        {
        }*/
    }

    return false;
}


void psSlotManager::CancelDrag()
{
    isDragging = false;

    if(isPlacing)
    {
        psengine->GetSceneManipulator()->RemoveSelected();
        if(hadInventory)
        {
            PawsManager::GetSingleton().GetMainWidget()->FindWidget("InventoryWindow")->Show();
        }
        isPlacing = false;
        isRotating = false;
        hadInventory = false;
    }

    pawsSlot* dragging = (pawsSlot*)PawsManager::GetSingleton().GetDragDropWidget();
    if ( !dragging )
        return;

    draggingSlot.slot->SetPurifyStatus(dragging->GetPurifyStatus());
    int oldStack =  draggingSlot.slot->StackCount();
    oldStack += draggingSlot.stackCount;

    csString res;
    if(dragging->Image())
        res = dragging->Image()->GetName();
    else
        res.Clear();

    draggingSlot.slot->PlaceItem(res, draggingSlot.meshFactName, draggingSlot.materialName, oldStack);
    draggingSlot.slot->SetToolTip(draggingSlot.toolTip);
    draggingSlot.slot->SetBartenderAction(draggingSlot.bartenderAction);
    PawsManager::GetSingleton().SetDragDropWidget( NULL );
}


void psSlotManager::OnNumberEntered(const char* /*name*/, int param, int count)
{
    if ( count == -1 )
        return;

    pawsSlot* parent = NULL;
    size_t i = (size_t)param;
    if (i < slotsInUse.GetSize())
    {
        // Get the slot ptr
        parent = slotsInUse[i];
        slotsInUse[i] = NULL;

        // Clean up the trailing NULLs  (can't just delete the index, as that would change other indicies)
        for (i=slotsInUse.GetSize()-1; i!=(size_t)-1; i--)
        {
            if (slotsInUse[i] == NULL)
                slotsInUse.DeleteIndex(i);
            else break;
        }
    }

    if (!parent)
        return;

    int purifyStatus = parent->GetPurifyStatus();
    int newStack = parent->StackCount() - count;

    pawsSlot* widget = new pawsSlot();
    widget->SetRelativeFrame( 0,0, parent->DefaultFrame().Width(), parent->DefaultFrame().Height() );

    if (parent->Image())
        widget->PlaceItem( parent->Image()->GetName(), parent->GetMeshFactName(), parent->GetMaterialName(), count );
    else
        widget->PlaceItem( NULL, parent->GetMeshFactName(), parent->GetMaterialName(), count );

    widget->SetPurifyStatus( purifyStatus );
    widget->SetBackgroundAlpha(0);
    widget->SetParent( NULL );
    widget->DrawStackCount(parent->IsDrawingStackCount());

    SetDragDetails(parent, count);
    parent->StackCount( newStack );
    isDragging = true;
    PawsManager::GetSingleton().SetDragDropWidget( widget );
}


void psSlotManager::SetDragDetails(pawsSlot* slot, int count)
{
    //printf("SetDragDetails: \n");
    draggingSlot.containerID     = slot->ContainerID();
    draggingSlot.slotID          = slot->ID();
    draggingSlot.stackCount      = count;
    draggingSlot.slot            = slot;
    draggingSlot.meshFactName    = slot->GetMeshFactName();
    draggingSlot.materialName    = slot->GetMaterialName();
    draggingSlot.toolTip         = slot->GetToolTip();
    draggingSlot.bartenderAction = slot->GetBartenderAction();
    //checks if we are dragging the whole thing or not
    draggingSlot.split           = slot->StackCount() != count;
}


void psSlotManager::PlaceItem()
{
    // Get WS position.
    psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();

    // Create mesh.
    outline = psengine->GetSceneManipulator()->CreateAndSelectMesh(draggingSlot.meshFactName,
        draggingSlot.materialName, psengine->GetPSCamera()->GetICamera()->GetCamera(), csVector2(p.x, p.y));

    // If created mesh is valid.
    if(outline)
    {
        // Hide the inventory so we can see where we're dropping.
        pawsWidget* inventory = PawsManager::GetSingleton().GetMainWidget()->FindWidget("InventoryWindow");
        hadInventory = inventory->IsVisible();
        inventory->Hide();

        // Get rid of icon.
        PawsManager::GetSingleton().SetDragDropWidget( NULL );

        // Mark placing.
        isPlacing = true;
    }
}


void psSlotManager::UpdateItem()
{
    // Get new position.
    psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();

    // If we're rotating then we use mouse movement to determine rotation.
    if(isRotating)
    {
        psengine->GetSceneManipulator()->RotateSelected(csVector2(p.x, p.y));
    }
    else
    {
        // Else we use it to determine item position.
        psengine->GetSceneManipulator()->TranslateSelected(false,
            psengine->GetPSCamera()->GetICamera()->GetCamera(), csVector2(p.x, p.y));
    }
}


void psSlotManager::DropItem(bool guard)
{
    // get final position and rotation
    psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();
    csVector3 pos;
    csVector3 rot;
    psengine->GetSceneManipulator()->GetPosition(pos, rot, csVector2(p.x, p.y));

    // Send drop message.
    psSlotMovementMsg msg( draggingSlot.containerID, draggingSlot.slotID,
      CONTAINER_WORLD, 0, draggingSlot.stackCount, &pos,
      &rot, guard);
    msg.SendMessage();

    // Remove outline mesh.
    psengine->GetSceneManipulator()->RemoveSelected();

    // Show inventory window again.
    if(hadInventory)
    {
        PawsManager::GetSingleton().GetMainWidget()->FindWidget("InventoryWindow")->Show();
    }

    // Reset flags.
    isDragging = false;
    isPlacing = false;
    isRotating = false;
    hadInventory = false;
}


void psSlotManager::Handle( pawsSlot* slot, bool grabOne, bool grabAll )
{
    //printf("In psSlotManager::Handle()\n");

    if ( !isDragging )
    {
        // Make sure other code isn't drag-and-dropping a different object.
        pawsWidget *dndWidget = PawsManager::GetSingleton().GetDragDropWidget();
        if (dndWidget)
            return;

        //printf("Starting a drag/drop action\n");

        int stackCount = slot->StackCount();
        if ( stackCount > 0 )
        {
            int tmpID = (int)slotsInUse.Push(slot);

            if ( stackCount == 1 || grabOne )
            {
                OnNumberEntered("StackCount",tmpID, 1);
            }
            else if ( grabAll )
            {
                OnNumberEntered("StackCount",tmpID, stackCount);
            }
            else // Ask for the number of items to grab
            {
                csString max;
                max.Format("Max %d", stackCount );

                pawsNumberPromptWindow::Create(max,
                                               -1, 1, stackCount, this, "StackCount", tmpID);
            }
        }
    }
    else
    {
        //do nothing if it's the same slot and we aren't dragging a split item
        if(slot == draggingSlot.slot && !draggingSlot.split)
        {
            CancelDrag();
            return;
        }
        //printf("Dropping Slot Here\n");
        //printf("Target Slot Information: \n");
        //printf("Bartender Slot: %d\n", slot->IsBartender());
        //printf("Sending slot movement message\n");
        if ( slot->IsBartender() )
        {
            //we cancel dragging because we aren't moving the original item but just taking
            //a reference to it. If it's among bartender slots we will handle them differently
            CancelDrag();
            slot->PlaceItem( draggingSlot.slot->ImageName(), "", "", draggingSlot.stackCount);
            slot->SetBartenderAction(draggingSlot.bartenderAction);
            slot->SetToolTip(draggingSlot.toolTip);
            //if the original slot was a bartender clear it as we are moving it to a new one
            if(draggingSlot.slot->IsBartender())
            {
                draggingSlot.slot->Clear();
            }
        }
        else
        {
            //printf("Slot->ID: %d\n", slot->ID() );
            //printf("Container: %d\n", slot->ContainerID() );
            //printf("DraggingSlot.ID %d\n", draggingSlot.slotID);
            

            if ( draggingSlot.containerID == CONTAINER_SPELL_BOOK )
            {
                // Stop dragging the spell around
                CancelDrag();

                // Set the image to this slot.
                slot->PlaceItem( draggingSlot.slot->ImageName(), "", "", draggingSlot.stackCount);
            }
            else
            {
                psSlotMovementMsg msg( draggingSlot.containerID, draggingSlot.slotID,
                               slot->ContainerID(), slot->ID(),
                               draggingSlot.stackCount );
                msg.SendMessage();

                // Reset widgets/objects/status.
                PawsManager::GetSingleton().SetDragDropWidget( NULL );
                isDragging = false;
                if(isPlacing)
                {
                    psengine->GetSceneManipulator()->RemoveSelected();
                    if(hadInventory)
                    {
                        PawsManager::GetSingleton().GetMainWidget()->FindWidget("InventoryWindow")->Show();
                    }
                    isPlacing = false;
                    isRotating = false;
                    hadInventory = false;
                }
            }
        }
    }
}
