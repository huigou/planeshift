/*
 * pawsscrollmenu.h - Author: Joe Lyon
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
// pawsscrollmenu.h: interface for the pawsScrollMenu class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_SCROLLMENU_HEADER
#define PAWS_SCROLLMENU_HEADER



#include "globals.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "pawsscrollmenu.h"


//=============================================================================
// Defines
//=============================================================================
#define BUTTON_PADDING          4
#define SHORTCUT_BUTTON_OFFSET  2000

#define ScrollMenuOptionDISABLED 0
#define ScrollMenuOptionENABLED  1
#define ScrollMenuOptionDYNAMIC  2

#include "paws/pawswidget.h"
#include "gui/pawsdndbutton.h"





/** A scrolling list of buttons, each with an icon and which accepts drag-n-drop.
 */
class pawsScrollMenu : public pawsWidget
{

public:


    pawsScrollMenu();
    virtual ~pawsScrollMenu();

    bool PostSetup();
    void OnResize();
    void OnResizeStop();
    int  CalcButtonSize(pawsDnDButton* target);
    void LayoutButtons();

    virtual bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* reporter);

    bool LoadArrays(csArray<csString> &name, csArray<csString> &icon, csArray<csString> &toolTip, csArray<csString> &actions, int baseIndex, pawsWidget* widget);
    bool LoadSingle(csString name, csString icon, csString toolTip, csString action, int Index, pawsWidget* widget, bool IsEnabled);

    bool RemoveByName(csString name);
    int  GetSize();
    int  GetButtonWidth();
    int  GetButtonHeight();
    int  GetButtonHolderWidth();
    int  GetButtonHolderHeight();
    int  GetTotalButtonWidth();

    virtual bool Setup(iDocumentNode* node);
    bool SelfPopulate(iDocumentNode* node);

    virtual void Draw();

    bool ScrollUp();
    bool ScrollDown();
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers);

    virtual bool IsEnabled() const;

    void SetEditLock(int mode);
    bool IsEditable() {return EditLockButton->GetState(); }
    void SetLeftScroll(int mode);
    void SetRightScroll(int mode);
    void setButtonWidth( int width );
    void setScrollIncrement( int incr );
    void setScrollProportion( float prop );


protected:

    /// Track to see if the button is down.
    bool down;

    /// Image to draw when button is pressed or when the mouse enters.
    csRef<iPawsImage> pressedImage;

    /// Image to draw when button is released or when the mouse exits.
    csRef<iPawsImage> releasedImage;

    csRef<iPawsImage> greyUpImage;
    csRef<iPawsImage> greyDownImage;

    /// Image to draw when button is released.
    csRef<iPawsImage> specialFlashImage;

    /// Check to see if this is a toggle button.
    bool toggle;

    /// Text shown in button
    csString buttonLabel;

    /// Keyboard equivalent of clicking on this button
    char keybinding;

    /// Widget to which event notifications are sent. If NULL, notifications go to parent
    pawsWidget* notify;

    /// Style -- right now only ShadowText supported
    int style;

    bool enabled;

    /// The state if the button is flashing, 0 is no flashing
    int flash;

    /// Type of flash (regular/special)
//    FLASH_STATE flashtype;

    /// Button can trigger sound effects with this
    csString sound_click;
    int upTextOffsetX;
    int upTextOffsetY;
    int downTextOffsetX;
    int downTextOffsetY;

    ///Used when restoring from highlight state
    int originalFontColour;

    /// Whether or not to change image on mouse enter/exit.
    bool changeOnMouseOver;


    int   buttonWidth,
          buttonHeight,
          scrollIncrement;
    float scrollProportion;
    bool  buttonWidthDynamic;

    pawsWidget*            ButtonHolder;
    csArray<pawsWidget*> Buttons;
    int                   buttonLocation;

    pawsButton*            LeftScrollButton;
    int                    LeftScrollMode;
    bool                   LeftScrollVisible;

    pawsButton*            RightScrollButton;
    int                    RightScrollMode;
    bool                   RightScrollVisible;

    pawsButton*            EditLockButton;
    int                    EditLockMode;               //enabled, disabled, (dynamic==>enabled)
    bool                   EditLock;                 //true = editing prevented, false = editing allowed

    pawsWidget*            callbackWidget;

};

//----------------------------------------------------------------------
CREATE_PAWS_FACTORY(pawsScrollMenu);

/** @} */

#endif
