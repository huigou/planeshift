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


#include <psconfig.h>


#include "psslotmgr.h"
#include "pscamera.h"
#include "globals.h"

#include "gui/pawsslot.h"

psSlotManager::psSlotManager()
{
    isDragging = false;
}

psSlotManager::~psSlotManager()
{
}

void psSlotManager::CancelDrag()
{
    isDragging = false;
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

    draggingSlot.slot->PlaceItem(res,oldStack);    
    PawsManager::GetSingleton().SetDragDropWidget( NULL );
}

void psSlotManager::OnNumberEntered(const char *name,int param,int count)
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
        widget->PlaceItem( parent->Image()->GetName(), count );
    else        
        widget->PlaceItem( NULL, count );
        
    parent->StackCount( newStack );
    widget->SetPurifyStatus( purifyStatus );
    widget->SetBackgroundAlpha(0);
    widget->SetParent( NULL );
           
    SetDragDetails( parent, count );
    isDragging = true;
    PawsManager::GetSingleton().SetDragDropWidget( widget );
}

void psSlotManager::SetDragDetails( pawsSlot* slot, int count ) 
{ 
    draggingSlot.containerID    = slot->ContainerID();
    draggingSlot.slotID         = slot->ID();
    draggingSlot.stackCount     = count;
    draggingSlot.slot           = slot;
    //printf("Start dragging\n  containerID=%d\n  slotID=%d\n",slot->ContainerID(), slot->ID());
}

void psSlotManager::DropItem()
{
    //printf("In psSlotManager::DropItem()\n");

    psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();
    csVector3 pt3d;
    iMeshWrapper *mesh = psengine->GetPSCamera()->Get3DPointFrom2D(p.x, p.y, &pt3d);
    if (!mesh)  // weird!
    {
        pt3d.Set(0,0,0);
    }
    // This p.x < ... stuff seems crazy...
    psSlotMovementMsg msg( draggingSlot.containerID, draggingSlot.slotID,
                           CONTAINER_WORLD, p.x < PawsManager::GetSingleton().GetGraphics2D()->GetWidth() / 2,
                           draggingSlot.stackCount, &pt3d);
    PawsManager::GetSingleton().SetDragDropWidget( NULL );
    msg.SendMessage();                               
    isDragging = false;                           
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
        //printf("Sending slot movement message\n");
        psSlotMovementMsg msg( draggingSlot.containerID, draggingSlot.slotID,
                               slot->ContainerID(), slot->ID(),
                               draggingSlot.stackCount );

        PawsManager::GetSingleton().SetDragDropWidget( NULL );
        msg.SendMessage();                               
        isDragging = false;
    }    
}

