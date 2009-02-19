/*
* shortcutwindow.h - Author: Andrew Dai
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

#ifndef PAWS_SHORTCUT_WINDOW
#define PAWS_SHORTCUT_WINDOW 

// CS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "net/cmdbase.h"
#include "gui/pawscontrolwindow.h"
#include "gui/pawsconfigkeys.h"

#include "pscharcontrol.h"

class pawsMessageTextBox;
class pawsEditTextBox;
class pawsMultilineEditTextBox;
class pawsTextBox;
class pawsScrollBar;

#define NUM_SHORTCUTS    200

class pawsShortcutWindow : public pawsControlledWindow, public pawsFingeringReceiver
{
public:
    pawsShortcutWindow();

    virtual ~pawsShortcutWindow();

    virtual bool PostSetup();

    bool OnMouseDown( int button, int modifiers, int x, int y );
    // bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* reporter);
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);
    bool OnScroll( int direction, pawsScrollBar* widget );
    void OnResize();
    void StopResize();
    bool OnFingering(csString string, psControl::Device device, uint button, uint32 mods);

    /**
     * Execute a short cut script.
     * @param shortcutNum is the button number if local = true
     *                    else it is the command in range from
     *                    0 to MAX_SHORTCUT_SETS*10-1;
     */
    void ExecuteCommand(int shortcutNum, bool local);
    const csString& GetCommandName(int shortcutNum, bool local);
    csString GetTriggerText(int shortcutNum);

    void LoadDefaultCommands();
    
protected:
    /// System text for easy access
    pawsMessageTextBox* systemText;
    void LoadCommands(const char * fileName);
    void SaveCommands(void);
    CmdHandler *cmdsource;

    /** Calculates common size of shortcuts buttons */
    void CalcButtonSize();
    
    /** Calculates dimensions of shortcut button matrix */
    void CalcMatrixSize(size_t & matrixWidth, size_t & matrixHeight);
    
    /** Calculates how many rows of buttons do we need if there 
        is 'matrixWidth' number of buttons in each */
    size_t CalcTotalRowsNeeded(size_t matrixWidth);
    
    /** Creates matrix of shortcut buttons (first deletes old one) */
    void RebuildMatrix();
    
    /** Sets positions and sizes of buttons in the matrix */
    void LayoutMatrix();
    
    /** Sets texts and IDs of buttons inside matrix according to current scroll position */
    void UpdateMatrix();
    
    /** Sets window size that is ideal for current button matrix */
    void SetWindowSizeToFitMatrix();
    

    // Simple string arrays holding the commands and command name
    csString cmds[NUM_SHORTCUTS];
    csString names[NUM_SHORTCUTS];

    csRef<iVFS> vfs;
    csRef<iDocumentSystem> xml;

    // The widget that holds the command data during editing
    pawsMultilineEditTextBox* textBox;

    // The widget that holds the button label data during editing
    pawsEditTextBox* labelBox;

    // The widget that holds the shortcut lable data during editing
    pawsTextBox* shortcutText;

    pawsTextBox* title;

    // The button configuring widget
    pawsWidget* subWidget;

    // Current size of shortcut buttons
    int buttonWidth, buttonHeight;
    
    // The matrix of buttons of visible shortcuts
    csArray< csArray<pawsButton*> > matrix;
    
    int edit;

    pawsScrollBar* scrollBar;
};
CREATE_PAWS_FACTORY( pawsShortcutWindow );
#endif
