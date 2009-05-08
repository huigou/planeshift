/*
 * psmainwidget.h - Author: Andrew Craig
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
// pawsmainwidget.h: interface for the pawsMainWidget class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PS_MAIN_WIDGET_HEADER
#define PS_MAIN_WIDGET_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/camera.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/clientmsghandler.h"

#include "paws/pawswidget.h"
#include "paws/psmousebinds.h"
#include "paws/pawsmainwidget.h"

#include "../pscelclient.h"

//=============================================================================
// Local Includes
//=============================================================================



class pawsChatWindow;


class psEntityType
{
public:
    psEntityType(const char *id,const char *label, int dflt, const char *commandsStr, const char *labelsStr);
    
    csString id;
    csString label;
    int usedCommand, dflt;
    csStringArray labels;
    csStringArray commands;
};

class psEntityTypes
{
public:
    psEntityTypes(iObjectRegistry* objReg);
    csString BuildDfltBehaviors();
    bool LoadConfigFromFile();
    bool SaveConfigToFile();
    
    csPDelArray<psEntityType> types;
    iObjectRegistry* objReg;
    csRef<iVFS> vfs;
};



/** The main or desktop widget.
 */
class psMainWidget : public pawsMainWidget, public psCmdBase
{
public:
    psMainWidget();
    virtual ~psMainWidget();

    bool OnKeyDown(int keyCode, int key, int modifiers );

    bool OnMouseDown( int button, int modifiers, int x, int y );
    bool OnMouseUp( int button, int modifiers, int x, int y );
    bool OnDoubleClick( int button, int modifiers, int x, int y );

    void LockPlayer() { locked = true; }
    bool IsPlayerLocked() { return locked; }
    void UnlockPlayer() { locked = false; }

    /** Find what entity the mouse is over.
     * @param mouseX The screen X position of the mouse.
     * @param mouseY The screen Y position of the mouse.
     * @return A pointer to the GEMObject found or NULL if no entity.
     */
    GEMClientObject* FindMouseOverObject( int mouseX, int mouseY );

    psEntityTypes * GetEntityTypes() { return &entTypes; }

    /// Handles server messages
    void HandleMessage( MsgEntry* message );

    /// Handles commands (unused for now)
    const char* HandleCommand(const char* cmd) { return NULL; };

	/**
	* Print a message on screen
	*
	* This function is called when a system message is received. To print a message on screen
	* fire a system message of type MSG_ERROR(Red), MSG_OK(Green), MSG_RESULT(Yellow) or MSG_ACK(Blue)
	*
	* @param text  The text to be printed on screen. Have to be at lest 2 chars or it will be ignored.
	* @param color The color to be used on the text.
	* @param ymod  Where on screen should the message be displayed. Default in center.
	*
	*/
	void PrintOnScreen(const char* text, int color, float ymod = 0.50f);

 public:
    void ClearFadingText();

    void DrawChildren();
    bool SetupMain();

    void DeleteChild(pawsWidget* txtBox); // Removes the fading text box from the array and the memory
    
private:

    pawsChatWindow *chatWindow;
    psCelClient* cel;

    /// If the player is forbidden to move, it will be stored here
    bool locked;

    psEntityTypes entTypes;
    
    pawsWidget* lastWidget;

    // Options to the on-screen messages
    csRef<iFont>    mesgFont;
    csRef<iFont>    mesgFirstFont;

    csArray<pawsWidget*> onscreen;
};

#endif


