/*
 * psslotmgr.h
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

#ifndef PS_SLOT_MANAGER
#define PS_SLOT_MANAGER

#include <csutil/array.h>

#include "paws/pawsnumberpromptwindow.h"

class pawsSlot;
class MsgHandler;
struct iMeshWrapper;

//------------------------------------------------------------------------------

class psSlotManager : public iOnNumberEnteredAction
{
private:
    struct DraggingSlot
    {       
        int containerID;
        int slotID;
        int stackCount;
        pawsSlot* slot;
        csString meshFactName;
        int parentID;
    } draggingSlot;
  
    bool isDragging;
    bool isPlacing;
    bool isRotating;

    psPoint basePoint;

    csArray<pawsSlot*> slotsInUse;

    /// Shortcut for event mouse move
    csEventID MouseMove;

    /// Shortcut for event mouse down
    csEventID MouseDown;

    void PlaceItem();
    void UpdateItem();
    bool DropItem();

    csString draggedMesh;
    csRef<iMeshWrapper> outline;

public:
    psSlotManager();
    virtual ~psSlotManager();

    bool HandleEvent( iEvent& ev );
    
    void Handle( pawsSlot* slot, bool grabOne = false, bool grabAll = false );
    void SetDragDetails( pawsSlot* slot, int count );
    
    bool IsDragging() { return isDragging; }
    
    /** Set the state of the drag flag. */
    void SetDragFlag( bool flag ) { isDragging = flag; }
    
    int HoldingContainerID() { return draggingSlot.containerID; }
    int HoldingSlotID() { return draggingSlot.slotID; }

    void CancelDrag();
    
    void OnNumberEntered(const char *name,int param,int value);
};


#endif

