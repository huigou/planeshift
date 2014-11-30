/*
 * pawscontrolwindow.h - Author: Andrew Craig
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
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_CONTROL_WINDOW_HEADER
#define PAWS_CONTROL_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include <iutil/csinput.h>

class pawsControlledWindow;

/** Structure to relate windows with their buttons for lookup.
 * @remark Used by FindButton.
 */
struct WBName
{
    csString windowName;
    csString buttonName;
    int id;
};

/// Map windows to their pawsButtons on the control panel.
struct Icon
{
    pawsControlledWindow* window;
    pawsButton* theirButton;
    bool IsActive;
    bool IsOver;
    csString orgRes;
};


/** Window that drives the main interface.
 * This widget controls the behavior of the main toolbar and the interaction
 * with the various windows it controls.
 */
class pawsControlWindow : public pawsWidget
{
public:
    pawsControlWindow();
    virtual ~pawsControlWindow();
    pawsControlWindow(const pawsControlWindow& origin){}
    /** Toggle a window and it's children from hidden to visible or back
     * depending on the window state when called.
     */
    void Toggle();
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);
    bool OnChildMouseEnter(pawsWidget *child);
    bool OnChildMouseExit(pawsWidget *child);
    void Hide();
    void Show();

    /** Used for generic controlled window behavior when a window's button is
     * pressed.
     * Displays a window that is hidden when it's button is pressed. Hides a
     * window that is currently diplayed.
     * @param widgetStr: The window to handle.
     * @return TRUE if it makes it visible, returns FALSE if it made it
               invisible or error.
     */
    bool HandleWindow(csString widgetStr);

    /** Used for generic controlled window behavior when a window's  is called
     * Displays a window that is hidden when asked for by a command. Hides a
     * window that is currently diplayed.
     * @param widgetStr: The window to handle.
     * @return TRUE if the toggle was possible, returns FALSE if it was not found.
     */
    bool HandleWindowName(csString widgetStr);
    
    /** Used to show a window.
     * If the window was already shown nothing happens.
     * @param widgetStr: The window to show
     * @return FALSE in case of an error otherwise TRUE
     */
    bool showWindow(csString widgetStr);
    /** Used to hide a window.
     * If the window was already hidden nothing happens.
     * @param widgetStr: The window to show
     * @return FALSE in case of an error otherwise TRUE
     */
    bool hideWindow(csString widgetStr);
    /** Used to show a window from a command.
     * If the window was already shown nothing happens.
     * @param widgetStr: The window to hide (command form, translated to the actual window name by the function)
     * @return FALSE in case of an error otherwise TRUE
     */
    bool showWindowName(csString widgetStr);
    /** Used to hide a window from a command.
     * If the window was already hidden nothing happens.
     * @param widgetStr: The window to hide (command form, translated to the actual window name by the function)
     * @return FALSE in case of an error otherwise TRUE
     */
    bool hideWindowName(csString widgetStr);
    
    /** Used to set the position of windows from a command.
     *  @param widgetStr: The window to hide (command form, translated to the actual window name by the function)
     *  @param x: New x postion ofthe window
     *  @param y: New y postion ofthe window
     *  @return FALSE in case of an error otherwise TRUE
     */
    bool setWindowPositionName(csString widgetStr, int x, int y);
    
    /** Used to set the size of windows from a command.
     *  @param widgetStr: The window to resizehide (command form, translated to the actual window name by the function)
     *  @param width: New width of the window
     *  @param height: New height of the window
     *  @return FALSE in case of an error otherwise TRUE
     */
    bool setWindowSizeName(csString widgetStr, int width, int height);
    
    /** Used to get all names and alternative names of windows from a command.
     * @return a string containing all the window names
     */
    csString getWindowNames();
    
    /** Used to get the size and position of windows from a command.
     * @param widgetStr: The window that should return its info or "all" 
     * @return a string containing the requested info
     */
    csString getWindowInfo(csString widgetStr);

    /** When the quit button is pressed this method displays the yes/no dialog
     * box to confirm that a user really wants to quit.
     */
    void HandleQuit();

    /** Changes the icon IsActive flag to TRUE and displays the proper
     * background for it when a window is opened.
     * @param wnd: The window that opened.
     */
    void WindowOpen(pawsWidget* wnd);

    /**  Changes the icon IsActive flag to FALSE and displays the proper
     * background for it when a window is closed.
     * @param wnd: The window that closed.
     */
    void WindowClose(pawsWidget* wnd);

    /** When a new window registers make an entry for them in our list.
     * @param window The window to register.
     */
    void Register( pawsControlledWindow* window );

    /** Creates a new WBName structure to hold window and button status.
     * @param wndName: The name of the window to add.
     * @param btnName: The name of the button associated with the window.
     */
    void AddWindow(csString wndName, csString btnName);

    bool PostSetup();

    /** The find window and find button methods are used to match a button to
     * it's related window.
     * If you have the button name you can find the window.
     * @param btnName: The button name who's window you need.
     */
    pawsControlledWindow* FindWindowFromButton(csString btnName);

    /** The find window and find button methods are used to match a button to
     * it's related window.
     * If you have the window name you can find the button.
     * @param wndName: The window name who's button you need.
     */
    pawsButton* FindButtonFromWindow(csString wndName);

    /** Matchs the button name to the icon that represents it on the display.
     * @param btnName: The name to match.
     */
    Icon* GetIcon(csString btnName);

    /** Advances through the available styles when a users clicks the right
     * mouse button in the control window.
     */
    void NextStyle();

    bool OnMouseEnter();
    bool OnMouseExit();

    bool Contains( int x, int y );

private:
    /** helper function for commands to translate to widget names from a more readable form
     *  This takes the windows names and the alternative names from a file (standard: /planeshift/data/gui/windownames.xml)
     *  and then returns the "offical" window name for the given parameter
     *  @param widgetStr: The widget name to be translated
     *  @return The actual widget name used by the system or an empty string if this widget is unknown to this function
     */  
    csString translateWidgetName(csString widgetStr);
    struct WindowNames
    {
       csString name;
       csArray<csString> alternativeNames;
    }; // struct to contain the window names and their alternatives
    csArray<WindowNames> controlledWindows;  // array of all window names that are controllable by commands

    unsigned short int style;
    bool hidden;
    csArray<WBName> wbs;
    csPDelArray<Icon> buttons;

    pawsWidget* buttonUp;
    pawsWidget* buttonDown;

    Icon* QuitIcon;

    csRef<iKeyboardDriver> keyboard;

    bool alwaysResize;

    int orgw,orgh;
};

CREATE_PAWS_FACTORY( pawsControlWindow );

/** This is a window that is controlled by a button on the control bar.
  * Contains methods to register itself with the controller and to notify the
  * controller when it is hidden or shown.
  */
class pawsControlledWindow : public pawsWidget
{
public:

    pawsControlledWindow()
    {
        registered=false;
    }

    /** Called to register new window with the controller window.
     * Registers with the controller window and creates an entry for it.
     */
    virtual void Register()
    {
        controller = (pawsControlWindow*)PawsManager::GetSingleton().FindWidget("ControlWindow");
        controller->Register( this );
        registered = true;
    }

    /// When the window is opened let the controller window know about it.
    virtual void Show()
    {
        if (!registered)
            Register();

        pawsWidget::Show();
        controller->WindowOpen( this );
    }

    /// When the window is closed let the controller window know about it.
    virtual void Hide()
    {
        if (!registered)
            Register();

        pawsWidget::Hide();
        controller->WindowClose( this );
    }

private:
    pawsControlWindow* controller;
    bool registered;
};

#endif
