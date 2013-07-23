/*
* shortcutwindow.h - Author: Andrew Dai
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Revision Author: Joe Lyon
* Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_SHORTCUT_WINDOW
#define PAWS_SHORTCUT_WINDOW 

//=============================================================================
// Library Includes
//=============================================================================
#include "gui/pawscontrolwindow.h"
#include "gui/pawsconfigkeys.h"

#include "net/message.h"
#include "paws/pawsprogressbar.h"
#include "pawsscrollmenu.h"

// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "../globals.h"
#include "clientvitals.h"

// PAWS INCLUDES
#include "pawsinfowindow.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsprogressbar.h"
#include "paws/pawscrollbar.h"
#include "paws/pawsbutton.h"
#include "gui/pawscontrolwindow.h"
#include "pawsslot.h"



//=============================================================================
// Forward Declarations
//=============================================================================
class pawsChatWindow;
class pawsMessageTextBox;
class pawsEditTextBox;
class pawsMultilineEditTextBox;
class pawsTextBox;
class pawsScrollBar;

//=============================================================================
// Defines
//=============================================================================
#define NUM_SHORTCUTS    256

//=============================================================================
// Classes 
//=============================================================================

/**
 * 
 */
class pawsShortcutWindow : public pawsControlledWindow, public pawsFingeringReceiver, public psClientNetSubscriber
{
public:
    pawsShortcutWindow();

    virtual ~pawsShortcutWindow();

    virtual bool Setup(iDocumentNode *node);
    virtual bool PostSetup();

    bool OnMouseDown( int button, int modifiers, int x, int y );
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* reporter);
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);
    void OnResize();
    bool OnFingering(csString string, psControl::Device device, uint button, uint32 mods);

    /**
     * Execute a short cut script.
     *
     * @param shortcutNum is the button ordinal number 
     */
    void ExecuteCommand(int shortcutNum );
    
    /**
     * Get the short cut script.
     *
     * @param shortcutNum is the button ordinal number 
     */
    const csString& GetCommandName(int shortcutNum );

    /**
     * Get the text of a buttons assigned shortcut key 
     *
     * @param shortcutNum is the button ordinal number
     */
    csString GetTriggerText(int shortcutNum);

    /**
     * Load the commands, icon names and shortcut text keys
    **/
    void LoadDefaultCommands();
    void LoadCommandsFile();
    void LoadQuickbarFile();
    
    void ResetEditWindow();

    void Show();

protected:
    /// chat window for easy access
    pawsChatWindow* chatWindow;
    void LoadCommands(const char * fileName);
    void SaveCommands(void);
    CmdHandler *cmdsource;

    csArray<csString> cmds;
    csArray<csString> names;
    csArray<csString> toolTips;
    csArray<csString> icon;

    csRef<iVFS> vfs;
    csRef<iDocumentSystem> xml;

    // The widget that holds the command data during editing
    pawsMultilineEditTextBox* textBox;

    // The widget that holds the button label data during editing
    pawsEditTextBox* labelBox;

    // The widget that holds the shortcut lable data during editing
    pawsTextBox* shortcutText;

    pawsTextBox* title;

    // Widget used to configure the shortcuts
    pawsWidget* subWidget;

    pawsScrollMenu* iconPalette;
    pawsDnDButton*  iconDisplay;
    int             iconDisplayID;

    csString buttonBackgroundImage;

    int edit;
    pawsWidget *editedButton;

    virtual void HandleMessage(MsgEntry *msg);

private:
    pawsProgressBar *main_hp;
    pawsProgressBar *main_mana;
    pawsProgressBar *phys_stamina;
    pawsProgressBar *ment_stamina;
    pawsScrollMenu  *MenuBar;


    csArray<csString>    allIcons;
    csArray<csString>    allNames; //not populated at this time...

    size_t            position;
    size_t            buttonWidth;
    float             scrollSize;
};


CREATE_PAWS_FACTORY( pawsShortcutWindow );
#endif
