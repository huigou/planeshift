/*
 * pawscontrolwindow.cpp - Author: Andrew Craig
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

#include <psconfig.h>

// CS INCLUDES
#include <csgeom/vector3.h>
#include <csutil/xmltiny.h>
#include <iutil/vfs.h>
#include <iutil/objreg.h>

// COMMON INCLUDES

// CLIENT INCLUDES
#include "globals.h"
#include "util/psxmlparser.h"
#include <iutil/cfgmgr.h>
#include <iutil/csinput.h>

// PAWS INCLUDES
#include "pawscontrolwindow.h"
#include "pawsconfigwindow.h"
#include "psmainwidget.h"
#include "paws/pawstextbox.h"
#include "paws/pawsbutton.h"
#include "paws/pawsmanager.h"
#include "paws/pawsyesnobox.h"
#include "util/localization.h"

#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"

//////////////////////////////////////////////////////////////////////
// GUI BUTTONS NOTE: ONLY THOSE WHO NEEDS SPECIAL TREATMENT SHOULD BE
// PLACED HERE!
//////////////////////////////////////////////////////////////////////
#define CONTROL_MINIUP    10
#define CONTROL_MINIDOWN  20
#define CONTROL_QUIT      100

#define IMAGE_SIZE 52
#define ICONS 14
//////////////////////////////////////////////////////////////////////

// consts
const char* WINDOWNAMESFILE ="/planeshift/data/gui/windownames.xml";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsControlWindow::pawsControlWindow()
{
    hidden = false;
    style = 0;
    alwaysResize = true;
}

pawsControlWindow::~pawsControlWindow()
{
    visible = true; // Force visibility on next load
    if (hidden) Toggle(); // Unhide so we get the right sizes saved

    csRef<iConfigManager> file = psengine->GetConfig();
    file->SetInt("PlaneShift.GUI.ControlWindow.CurrentStyle", style);
    file->Save();
}

bool pawsControlWindow::PostSetup()
{
    SetAlwaysOnTop(true);

    AddWindow( "InventoryWindow" ,   "InventoryButton" );
    AddWindow( "ConfigWindow" ,      "OptionsButton" );
    AddWindow( "SpellBookWindow" ,   "SpellBookButton" );
    AddWindow( "AttackBookWindow" ,  "AttackButton" );
    AddWindow( "InfoWindow" ,        "InfoButton" );
    AddWindow( "HelpWindow" ,        "HelpButton" );
    AddWindow( "ShortcutMenu" ,    "ShortcutButton" );
    AddWindow( "BuddyWindow" ,       "BuddyButton" );
    AddWindow( "GroupWindow" ,       "GroupButton" );
    AddWindow( "PetitionWindow" ,    "PetitionButton" );
    AddWindow( "ChatWindow" ,        "ChatButton" );
    AddWindow( "SkillWindow" ,       "SkillsButton" );
    AddWindow( "QuestNotebook" ,     "QuestButton" );
    AddWindow( "GuildWindow" ,       "GuildButton" );
    AddWindow( "ActiveMagicWindow" ,       "ActiveMagicButton" );

    keyboard = csQueryRegistry<iKeyboardDriver> (PawsManager::GetSingleton().GetObjectRegistry());

    //The quit button is a bit special
    //We need to manualy register it
    QuitIcon = new Icon;
    QuitIcon->window = NULL;
    QuitIcon->theirButton = (pawsButton*)FindWidget("QuitButton");
    QuitIcon->orgRes = QuitIcon->theirButton->GetBackground();
    QuitIcon->IsActive = false;
    QuitIcon->IsOver = false;
    buttons.Push(QuitIcon);

    csRef<iConfigManager> file = psengine->GetConfig();
    int loadStyle = file->GetInt("PlaneShift.GUI.ControlWindow.CurrentStyle", 1);
    for (int i=0; i < loadStyle; i++)
        NextStyle(); // Switch to saved style

    buttonUp = FindWidget("ShowButtonUp");
    buttonDown = FindWidget("ShowButtonDown");
    
    //get a list of all controlled windows and their alternative names
    // first we clean the old array, who knows what was in there before or if this is the second time PostSetup runs
    controlledWindows.Empty();
    // then we crate a XML document reader...  
    csRef<iDocumentSystem> xml = psengine->GetXMLParser ();
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    if (!vfs)
    {
       // well, if we got here the question is probably how we got this far at all.
       Error1("interresting...couldn't load the virtual file system plugin. Something is definitively fishy");
       return false;
    }
    // ...and read the file with the names and alternative names with it...
    csRef<iDataBuffer> buf(vfs->ReadFile (WINDOWNAMESFILE));
    if (!buf || !buf->GetSize ())
    {
       Error2("Error reading windows name file %s", WINDOWNAMESFILE);
        return false;
    }
    // ..just to hand it over to the XML parser
    const char* error = doc->Parse( buf );
    if ( error )
    {
        Error2("Error loading windows names file: %s", error);
        return false;
    }
    // now the real work starts...making sense out of the xml file and storing it for later use
    csRef<iDocumentNode> rootNode = doc->GetRoot();
    if(!rootNode)
    {
        Error2("No XML root in %s", WINDOWNAMESFILE);
        return false;
    }
    csRef<iDocumentNodeIterator> windowsIt = rootNode->GetNodes();
    // iterate through all nodes
    while(windowsIt->HasNext())
    {
       csRef<iDocumentNode> baseNode = windowsIt->Next();
       csString baseNodeName (baseNode->GetValue());
       if (baseNodeName == "windows")
       {
           csRef<iDocumentNodeIterator> winIt = baseNode->GetNodes();
           while(winIt->HasNext())
           {
               csRef<iDocumentNode> curNode = winIt->Next();
               // check if it's a <window> node
               csString nodeName (curNode->GetValue());
               if (nodeName == "window")
               {
                   // found a <windows> node, lets store it's content
                   WindowNames windowName;
                   windowName.name = curNode->GetAttributeValue("name");
                   if(!windowName.name.Length())
                   {
                       // no name given for this window, not good
                       Error2("Missing window name in  %s", WINDOWNAMESFILE);
                       return false;
                   }
                   // now we also store the alternative names of this window
                   csRef<iDocumentNodeIterator> altNamesIt = curNode->GetNodes();
                   while (altNamesIt->HasNext())
                   {
                       csRef<iDocumentNode> altNameNode = altNamesIt->Next();
                       csString nodeAltName (altNameNode->GetValue());
                       // check if we found a <alternativeName> node
                       if (nodeAltName == "alternativeName")
                       {
                             csString altName = altNameNode->GetAttributeValue("name");
                             if(!altName.Length())
                             {                      
                                 // we got a alternative name node but no "name" attribute...bad
                                 Error2("Missing alternative window name in  %s", WINDOWNAMESFILE);
                                 return false;
                             }
                             // for alt names we only want lowercase versions
                             altName.Downcase();
                             // store all found alternative names
                             windowName.alternativeNames.Push(altName);
                       }
                   }
                   // last but no least we save the struct for later use
                   controlledWindows.Push(windowName);
               }
           }
       }    
    }    
    return true;
}

//show/hide buttons
void pawsControlWindow::Toggle()
{

    static int oldW = 0, oldH = 0;
    static int oldMinW = 0, oldMinH = 0;

    for (size_t z = 0; z < children.GetSize(); z++ )
    {
        if (hidden) children[z]->Show();
        else children[z]->Hide();
    }

    if (hidden)
    {
        buttonUp->Show();
        buttonDown->Hide();
        this->SetForceSize(oldW,oldH);
        this->SetMinSize(oldMinW,oldMinH);
    }
    else
    {
        oldW = this->GetScreenFrame().Width();
        oldH = this->GetScreenFrame().Height();
        this->GetMinSize(oldMinW,oldMinH);

        int w = buttonUp->GetScreenFrame().Width();
        int h = buttonUp->GetScreenFrame().Height();

        this->SetForceSize(w,h);
        this->SetMinSize(w,h);
        buttonUp->Hide();
        buttonDown->Show();
        buttonDown->SetRelativeFramePos(0,0);
    }

    hidden = !hidden;
}

bool pawsControlWindow::OnMouseEnter()
{
    if(keyboard->GetKeyState(CSKEY_SHIFT) && !alwaysResize)
    {
        SetResizeShow(true);
        pawsWidget::OnMouseEnter();
        return true;
    }
    else
        return pawsWidget::OnMouseEnter();
}

bool pawsControlWindow::OnMouseExit()
{
    if(!alwaysResize)
        SetResizeShow(false);

    pawsWidget::OnMouseExit();
    return true;
}

bool pawsControlWindow::OnButtonReleased(int mouseButton, int /*keyModifier*/, pawsWidget* reporter)
{
    if(reporter->GetID() == CONTROL_MINIDOWN || reporter->GetID() == CONTROL_MINIUP)
    {
        if (mouseButton == csmbRight)
        { //If the user clicked on the right mouse button, switch to another style
            if ( !hidden ) //don't allow to change styles while the toolbar is closed
                NextStyle();
            return true;
        }
        else if (mouseButton == csmbMiddle)
        {
            Debug3(LOG_PAWS, 0, "Resetting toolbar to orginal size (%d,%d)", orgw, orgh);
            SetRelativeFrameSize(orgw,orgh);
            return true;
        }
    }

    switch ( reporter->GetID() )
    {
        case CONTROL_MINIUP:
        {
            if ( !hidden )
                Toggle();
            return true;
        }

        case CONTROL_MINIDOWN:
        {
            if ( hidden )
                Toggle();
            return true;
        }

        case CONTROL_QUIT:
        {
            HandleQuit();

            QuitIcon->IsActive = true;
            csString bg(QuitIcon->orgRes);
            bg += "_active";
            QuitIcon->theirButton->SetBackground(bg.GetData());
            return true;
        }
        ////////////////////////////////////////////////////////////////////
        //Special cases that need more than the standard to open correctly
        ////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////
        // These two are from the confirm window buttons for quiting.
        ////////////////////////////////////////////////////////////////////
        case CONFIRM_YES:
        {
            psengine->QuitClient();
            return true;
        }
        case CONFIRM_NO:
        {
            PawsManager::GetSingleton().SetModalWidget( 0 );
            reporter->GetParent()->Hide();

            QuitIcon->IsActive = false;
            QuitIcon->theirButton->SetBackground(QuitIcon->orgRes.GetData());
            return true;
        }
        ////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////
        //Default handle for windows
        ////////////////////////////////////////////////////////////////////
        default:
        {
            pawsWidget* wdg = FindWidget(reporter->GetID());
            if (!wdg)
                return false;

            pawsControlledWindow* wnd = FindWindowFromButton(wdg->GetName());
            if (!wnd)
                return false;

            HandleWindow(wnd->GetName());
            return true;
        }
    }
    return false;
}

//Returns true if it made it visible, returns false if it made it invisible or error
bool pawsControlWindow::HandleWindow(csString widgetStr)
{
    pawsWidget* widget = PawsManager::GetSingleton().FindWidget(widgetStr.GetData());
    if (!widget)
    {
        Error2("%s isn't loaded", widgetStr.GetData());
        return false;
    }

    if ( widget->IsVisible() )
    {
        widget->Hide();
        if ( this->Includes(PawsManager::GetSingleton().GetCurrentFocusedWidget()) )
            PawsManager::GetSingleton().SetCurrentFocusedWidget(NULL);  // Don't leave focus on us after hiding
        return false;
    }
    else
    {
        widget->Show();
        return true;
    }
}

bool pawsControlWindow::HandleWindowName(csString widgetStr)
{
    // first some special cases
    csString widget = widgetStr;
    widget.Downcase();
    if(widget == "quit")
    {
        HandleQuit();
        return true;
    }
    if(widget == "buy")
    {
        psengine->GetCmdHandler()->Execute("/buy");
        return true;
    }
    // now the general window handling
    // first we translate the given name to the real window name
    widget = translateWidgetName(widgetStr);
    // if the translation was successful we "handle" the window
    if(widget != "")
    {
        HandleWindow(widget);
        return true;
    }
    // otherwise we return with an error
    else return false;
}
 
bool pawsControlWindow::showWindow(csString widgetStr)
{
    // lets see if we can find the specified window
    pawsWidget* widget = PawsManager::GetSingleton().FindWidget(widgetStr.GetData());
    if (!widget)
    {
        // doesn't look like it, log and return with an error
        Error2("%s isn't loaded", widgetStr.GetData());
        return false;
    }
    // if the window was not visible before we show it now.
    if ( !widget->IsVisible() )
    {
       widget->Show();
    }
    return true;
}
 
bool pawsControlWindow::hideWindow(csString widgetStr)
{
    // lets see if we can find the specified window
    pawsWidget* widget = PawsManager::GetSingleton().FindWidget(widgetStr.GetData());
    if (!widget)
    {
        // doesn't look like it, log and return with an error
        Error2("%s isn't loaded", widgetStr.GetData());
        return false;
    }
    // if the window was not visible before we show it now.
    if ( widget->IsVisible() )
    {
       widget->Hide();
       // Don't leave focus on us after hiding
        if ( this->Includes(PawsManager::GetSingleton().GetCurrentFocusedWidget()) )
       {
            PawsManager::GetSingleton().SetCurrentFocusedWidget(NULL);
       }
    }
    return true;
}

bool pawsControlWindow::showWindowName(csString widgetStr)
{
    // special window handling
    csString widget = widgetStr;
    widget.Downcase();
    if(widget == "quit")
    {
        HandleQuit();
        return true;
    }
    if(widget == "buy")
    {
        psengine->GetCmdHandler()->Execute("/buy");
        return true;
    }
    // and the "normal" windows
    widget = translateWidgetName(widgetStr);
    if(widget != "")
    {
       // if the window was found show it
        return showWindow(widget);
    }
    else return false;
}

bool pawsControlWindow::hideWindowName(csString widgetStr)
{
    // special window handling
    csString widget = widgetStr;
    widget.Downcase();
    if(widget == "quit")
    {
        // well, hiding of quit is not supported (and doesn't really make sense)
        return false;
    }
    if(widget == "buy")
    {
       // hiding of /buy isn't supported either at the moment
        return false;
    }
    // translate the given name and hide the window if found
    widget = translateWidgetName(widgetStr);
    if(widget != "")
    {
        return hideWindow(widget);
    }
    else return false;
}

bool pawsControlWindow::setWindowPositionName(csString widgetStr, int x, int y)
{
    csString widget = translateWidgetName(widgetStr);
    
    // set the new position of the window
    if(widget != "")
    {
       pawsWidget* w = PawsManager::GetSingleton().FindWidget(widget.GetData());
       if (!w)
       {
           Error2("%s isn't loaded", widget.GetData());
           return false;
       }
       w->SetRelativeFramePos(x,y);
       w->Resize();
           // needed to prevent some drawing errors...PAWS calls resize for parent widget but not the childern.
       return true;
    }
    return false;
}

bool pawsControlWindow::setWindowSizeName(csString widgetStr, int width, int height)
{
    csString widget = translateWidgetName(widgetStr);
    
    // set the new size of the window
    if(widget != "")
    {
       pawsWidget* w = PawsManager::GetSingleton().FindWidget(widget.GetData());
       if (!w)
       {
           Error2("%s isn't loaded", widget.GetData());
           return false;
       }
       // first check if the window is resizeable at all, if not do nothing
       if (w->IsResizable())
       {
           // then we check for the allowed minimum size
           int minWidth, minHeight;
           w->GetMinSize(minWidth, minHeight);
           // if the given size is smaller than that set it to the minimum size
           if (width < minWidth)
           {
               width = minWidth;
           }
           if (height < minHeight)
           {
               height = minHeight;
           }
           // and last but not least set the size
           w->SetSize(width,height);
           // needed to prevent some drawing errors...PAWS calls resize for all child widgets but not the parent itself.
           w->Resize();
       }
       return true;
    }
    return false;
}

csString pawsControlWindow::getWindowNames()
{
    csString returnNames = "";
    for (unsigned int i=0; i < controlledWindows.GetSize(); i++)
    {
       // add the real window name
       returnNames.Append(controlledWindows[i].name);
       for (unsigned int j=0; j<controlledWindows[i].alternativeNames.GetSize(); j++)
       {
           // add every found alternative name
           returnNames.Append(" | ").Append(controlledWindows[i].alternativeNames[j]);
       }
       // add a newline after every window type
       returnNames.Append("\n");
    }
    return returnNames;
}

csString pawsControlWindow::getWindowInfo(csString widgetStr)
{
    csString returnPositions = "";
    // check if all positions are requested or only a specific window
    csString window = widgetStr;
    window.Downcase();
    if (window == "all")
    {
        // so we want the positiosn of all windows
       for (unsigned int i = 0; i < controlledWindows.GetSize(); i++)
       {
           pawsWidget* widget = PawsManager::GetSingleton().FindWidget(controlledWindows[i].name.GetData());
           // error checking...this should only fail if someone messed up the definition file WINDOWNAMESFILE
           if (!widget)
           {
               // window could not be found
               Error2("%s isn't loaded", widgetStr.GetData());
               csString errMsg = "Could not find window ";
               errMsg.Append(controlledWindows[i].name);
               return errMsg;
           }
           else
           {
               // get the postion and add it to the string
               csRect rect = widget->GetScreenFrame();
               returnPositions.Append(controlledWindows[i].name).Append(" x:").Append(rect.xmin).Append("|").Append(rect.xmin-graphics2D->GetWidth())
                              .Append(" y:").Append(rect.ymin).Append("|").Append(rect.ymin-graphics2D->GetHeight())
                              .Append(" width:").Append(rect.Width()).Append(" height:").Append(rect.Height()).Append("\n");
           }
       }
    }
    else
    {
       // specific window
       window = translateWidgetName(widgetStr);
       if( window == "" )
       {
           // window could not be found
           Error2("%s Could not be found.", widgetStr.GetData());
           csString errMsg = "Could not find window ";
           errMsg.Append(widgetStr);
           return errMsg;
       }

       pawsWidget* widget = PawsManager::GetSingleton().FindWidget(window.GetData());
       if (!widget)
       {
           // window could not be found
           Error2("%s Could not be found.", widgetStr.GetData());
           csString errMsg = "Could not find window ";
           errMsg.Append(window);
           return errMsg;
       }
       else
       {
           // put the position in the string
           csRect rect = widget->GetScreenFrame();
//           returnPositions.Append(window).Append(" x:").Append(rect.xmin).Append("|").Append(rect.xmin-graphics2D->GetWidth())
//                              .Append(" y:").Append(rect.ymin).Append("|").Append(rect.ymin-graphics2D->GetHeight())
//                              .Append(" width:").Append(rect.Width()).Append(" height:").Append(rect.Height());
           returnPositions.Format( "%s  x:%d | %d  y:%d | %d  width:%d  height:%d",
               window.GetData(),
               rect.xmin,
               rect.xmin-graphics2D->GetWidth(),
               rect.ymin,
               rect.ymin-graphics2D->GetHeight(),
               rect.Width(),
               rect.Height() );
       }
    }
    return returnPositions;
}

csString pawsControlWindow::translateWidgetName(csString widgetStr)
{
    // first we convert to lowercase to be case-insensitive
    widgetStr.Downcase();
    csString compareName;
    // lets check all names and alternative names if the requested window can be found
    for (unsigned int i=0; i < controlledWindows.GetSize(); i++)
    {
       //  convert to lowercase
       compareName = controlledWindows[i].name;
       compareName.Downcase();
       if (widgetStr == compareName)
       {
           // real window name was used, return it
           return controlledWindows[i].name;
       }
       // now we check the alternative names of this window..these should be all lowercase already from ::PostSetup()
       for (unsigned int j=0; j<controlledWindows[i].alternativeNames.GetSize(); j++)
       {
           if (widgetStr == controlledWindows[i].alternativeNames[j])
           {
               // return the real window name
               return controlledWindows[i].name;
           }
       }
    }
    // no window with that name found
    return "";
}

void pawsControlWindow::HandleQuit()
{
    PawsManager::GetSingleton().CreateYesNoBox( "\nDo you really wish to leave Yliakum?", this );
}

bool pawsControlWindow::OnChildMouseEnter(pawsWidget *child)
{
    pawsButton* btn = (pawsButton*)child;
    Icon* icon = GetIcon(child->GetName());

    if (!icon)
        return false;

    csString bg(icon->orgRes); //Get the icon and then get the orignal resource bg
    bg += "_over";

    btn->SetBackground(bg.GetData());
    return true;
}

bool pawsControlWindow::OnChildMouseExit(pawsWidget *child)
{
    pawsButton* btn = (pawsButton*)child;
    Icon* icon = GetIcon(child->GetName());
    if (!icon)
        return false;

    csString bg(icon->orgRes);
    btn->SetBackground(bg.GetData());

    if (GetIcon(child->GetName())->IsActive)
    {
        bg = GetIcon(child->GetName())->orgRes;
        bg += "_active";
        btn->SetBackground(bg.GetData());
    }

    return true;
}

// When a new window registers make a new entry for them in our list.
void pawsControlWindow::Register( pawsControlledWindow* window )
{
    Icon * icon = new Icon;
    icon->window = window;
    icon->theirButton = FindButtonFromWindow( window->GetName() );
    if( icon->theirButton==NULL )
    {
        Error2("pawsControlWindow::Register couldn't find window %s!", window->GetName() );
        delete icon;
        return;
    }
    icon->orgRes = icon->theirButton->GetBackground();
    icon->IsActive = false;
    icon->IsOver = false;

    buttons.Push( icon );
}

void pawsControlWindow::AddWindow(csString wndName, csString btnName)
{
    WBName newName;
    pawsButton* btn = (pawsButton*)FindWidget(btnName.GetData());

    if (!btn)
    {
        Error2("Couldn't find button %s!", btnName.GetData());
        return;
    }

    newName.buttonName = btnName;
    newName.windowName = wndName;
    newName.id = btn->GetID();
    wbs.Push(newName);
}

pawsControlledWindow* pawsControlWindow::FindWindowFromButton(csString btnName)
{
    for (size_t x = 0; x < wbs.GetSize(); x++ )
    {
        if ( wbs[x].buttonName == btnName )
        {
            pawsWidget* wdg = PawsManager::GetSingleton().FindWidget(wbs[x].windowName.GetData());
            if (!wdg)
            {
                Error2("Found %s but FindWidget returned null!", btnName.GetData());
                return NULL;
            }

            return (pawsControlledWindow*)wdg;
        }
    }
    return NULL;
}


pawsButton* pawsControlWindow::FindButtonFromWindow(csString wndName)
{
    for (size_t x = 0; x < wbs.GetSize(); x++ )
    {
        if ( wbs[x].windowName == wndName )
        {
            pawsWidget* wdg = FindWidget(wbs[x].buttonName.GetData());
            if (!wdg)
            {
                Error2("Couldn't find window %s!", wndName.GetData());
                return NULL;
            }
            return (pawsButton*)wdg;
        }
    }
    return NULL;
}

Icon* pawsControlWindow::GetIcon(csString btnName)
{
    for (size_t x = 0; x < buttons.GetSize(); x++ )
    {
        if ( !strcmp(buttons[x]->theirButton->GetName(), btnName.GetData()) )
        {
            return buttons[x];
        }
    }
    return NULL;
}

//This is called everytime a window that has a button opens
void pawsControlWindow::WindowOpen(pawsWidget* wnd)
{
    pawsButton* btn = FindButtonFromWindow(wnd->GetName());
    Icon* icon = GetIcon(btn->GetName());

    icon->IsActive = true;

    csString bg(icon->orgRes);
    bg += "_active";

    btn->SetBackground(bg.GetData());
}

//This is called everytime a window that has a button closes
void pawsControlWindow::WindowClose(pawsWidget* wnd)
{
    pawsButton* btn = FindButtonFromWindow(wnd->GetName());
    Icon* icon = GetIcon(btn->GetName());
    icon->IsActive = false;
    btn->SetBackground(icon->orgRes.GetData());
}

void pawsControlWindow::NextStyle()
{
    // We load it each time to make the user able to change the style without the need to
    // restart the client
    //Increase style
    style++;
    csString filename;
    filename = PawsManager::GetSingleton().GetLocalization()->FindLocalizedFile("control_styles.xml");
    if (!psengine->GetVFS()->Exists(filename.GetData()))
    {
        Error2( "Could not find XML: %s", filename.GetData());
        return;
    }

    csRef<iDocument> doc = ParseFile(psengine->GetObjectRegistry(),filename.GetData());
    if (!doc)
    {
        Error2("Error parsing file %s", filename.GetData());
        return;
    }

    csString topNodestr("style");
    topNodestr += style;

    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("No XML root in %s", filename.GetData());
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode(topNodestr.GetData());

    //If "style"+style doesn't exist, jump to start
    if (!topNode)
    {
        if (style == 1) //No styles at all :(
            return;

        style=0;
        NextStyle();
        return;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    //Loop through the XML
    int bwidth=52, bheight=52, mwidth=16, mheight=16;
    float xscale=1.0f, yscale=1.0f;

    // get the current width and height of the buttons (should be the same for all)
    pawsButton* btn = (pawsButton*)FindWidget("QuitButton");
    int boldwidth = btn->ClipRect().Width();
    int boldheight = btn->ClipRect().Height();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        if (!strcmp(node->GetValue(),"button"))
        {
            int x = GetActualWidth(node->GetAttributeValueAsInt("x"));
            int y = GetActualHeight(node->GetAttributeValueAsInt("y"));
            csString name = node->GetAttributeValue("name");

            pawsWidget* wdg = FindWidget(name.GetData());
            if (!wdg)
            {
                Error2("No such widget as %s!", name.GetData());
                continue;
            }
            wdg->SetSize(bwidth, bheight);
            wdg->SetRelativeFramePos(x, y);
        }
        else if (!strcmp(node->GetValue(),"buttons"))
        {
            bwidth = GetActualWidth(node->GetAttributeValueAsInt("width"));
            bheight = GetActualHeight(node->GetAttributeValueAsInt("height"));
            mwidth = GetActualWidth(node->GetAttributeValueAsInt("hide_width"));
            mheight = GetActualHeight(node->GetAttributeValueAsInt("hide_height"));

            // get the multiplier for the button and the widget size
            if (boldwidth != 0 && boldheight != 0)
            {
                xscale = float(boldwidth) / float(bwidth);
                yscale = float(boldheight) / float(bheight);
            }
        }
        else if (!strcmp(node->GetValue(), "bar"))
        {
            orgw = GetActualWidth(node->GetAttributeValueAsInt("width"));
            orgh = GetActualWidth(node->GetAttributeValueAsInt("height"));
            SetRelativeFrameSize(orgw, orgh);
            min_width = GetActualWidth(node->GetAttributeValueAsInt("min_w"));
            min_height = GetActualWidth(node->GetAttributeValueAsInt("min_h"));

            alwaysResize = node->GetAttributeValueAsBool("always_resize", true);
            SetResizeShow(alwaysResize);
        }
    }

    /* Resize the widget according to the proportions of the previous style.
     *
     * When the new styles dimensions exceeds its max it will be downsized to fit.
     * See: pawsWidget::Resize(int deltaX, int deltaY, int flags)
     */
    Resize( int(xscale*orgw-orgw), int(yscale*orgh-orgh), RESIZE_RIGHT | RESIZE_BOTTOM );

    // Set the minimize buttons size
    pawsButton* minUp = (pawsButton*)FindWidget("ShowButtonUp");
    minUp->SetSize(mwidth,mheight);

    pawsButton* minDown = (pawsButton*)FindWidget("ShowButtonDown");
    minDown->SetSize(mwidth,mheight);
}

void pawsControlWindow::Hide()
{
    if (!hidden)
        Toggle();
}

void pawsControlWindow::Show()
{
    if (hidden)
        Toggle();

    pawsWidget::Show();
}

bool pawsControlWindow::Contains( int x, int y )
{
    if(!pawsWidget::Contains(x, y))
        return false;

    // Special case of a resize border
    if(keyboard->GetKeyState(CSKEY_SHIFT) || (ResizeFlags(x,y) && showResize) || alwaysResize)
        return true;

    // Typical case, this window is transparent
    for (size_t z = 0; z < children.GetSize(); z++ )
    {
        if (children[z]->IsVisible() )
        {
            if ( children[z]->Contains(x,y) )
                return true;
        }
    }
    return false;
}
