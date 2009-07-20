/*
 * pawsmanager.h - Author: Andrew Craig
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
 *
 * pawsmanager.h: interface for the PawsManager class.
 *
 *----------------------------------------------------------------------------
 */

#ifndef PAWS_MANAGER_HEADER
#define PAWS_MANAGER_HEADER

#include <csutil/ref.h>
#include <csutil/parray.h>
#include <csutil/hash.h>
#include <csutil/csstring.h>
#include <csgeom/vector3.h>

#include <iutil/vfs.h>
#include <iutil/document.h>
#include "isndsys/ss_structs.h"
#include "isndsys/ss_data.h"
#include "isndsys/ss_loader.h"
#include "isndsys/ss_stream.h"
// Until someone reworks the sound system, there's no point in spamming
// everyone with a million iSndSysSourceSoftware3D deprecation warnings.
#include <csutil/deprecated_warn_off.h>
#include "isndsys/ss_source.h"
#include <csutil/deprecated_warn_on.h>
#include "isndsys/ss_renderer.h"

#include "util/mathscript.h"
#include "util/log.h"

#include "pawsmouse.h"
#include "psmousebinds.h"
#include "pawsstyles.h"

struct iObjectRegistry;
struct iGraphics2D;
struct iGraphics3D;
struct iEvent;
class psLocalization;

class pawsWidget;
class pawsWidgetFactory;

class pawsMainWidget;
class pawsTextureManager;
class pawsPrefManager;
class PawsSoundHandle;

struct iPAWSSubscriber;
struct PAWSData;
struct PAWSSubscription;

typedef csHash<PAWSSubscription*,csString> PAWSSubscriptionsHash;

/**
 * Main PlaneShift Window manager.
 */
class PawsManager : public Singleton<PawsManager>
{
public:

    PawsManager(iObjectRegistry* objectReg, const char* skin, const char* skinBase = NULL,
                const char* pawsConfigFile = "/planeshift/userdata/planeshift.cfg", uint _gfxFeatures = useAll);

    virtual ~PawsManager();

    /// Establish main widget.
    void SetMainWidget(pawsMainWidget *widg);

    /** @brief Process mouse and keyboard events.
     *  @param event iEvent to process.
     */
    bool HandleEvent( iEvent& event );

    /** Draw the main widget and the mouse last.
     */
    void Draw();

    /// Returns the 2D renderer.
    iGraphics2D* GetGraphics2D() { return graphics2D; }

    /// Returns the 3D renderer.
    iGraphics3D* GetGraphics3D() { return graphics3D; }

    /// Returns the object registry.
    iObjectRegistry* GetObjectRegistry() { return objectReg; }

    /// Get the event name registry.
    iEventNameRegistry* GetEventNameRegistry() { return nameRegistry; }

    /// Returns the texture manager.
    pawsTextureManager* GetTextureManager() { return textureManager; }

    inline uint GetGFXFeatures() { return gfxFeatures; }

    /// Loads a skin and loades unregistered resources
    bool LoadSkinDefinition(const char* zip);

    /** @brief Add a new factory to the list that the manager knows about.
     *
     *  Each widget type must have it's own factory so the manager can
     *  build it based on it's name.
     *  @param factory The widget factory to add to the list.
     */
    void RegisterWidgetFactory( pawsWidgetFactory* factory );

    /** @brief Loads a widget definition file.
     *
     *  This loads a widget from XML and adds it to the main widget.
     *
     *  @param widgetFile The standard path of the widget to load.
     *  @see psLocalization::FindLocalizedFile()
     *  @return True if the widget was loaded properly.
     */
    bool LoadWidget( const char* widgetFile );

    /** @brief Loads a widget from given XML node.
     *  @return NULL on failure.
     */
    pawsWidget *LoadWidget(iDocumentNode *widgetNode);

    /** @brief Loads widgets from a definition file without assigning a parent.
     *
     *  This loads multiple widgets from XML and places pointers to the
     *  loaded widgets into the array loadedWidgets.
     *
     *  @param widgetFile The standard path of the widget to load.
     *  @param loadedWidgets An array which will be cleared and then filled
     *  with any widgets loaded.
     *  @return False if an error occured.  Note that some widgets may be
     *  loaded even if an error occurs.
     *  @see psLocalization::FindLocalizedFile()
     */
    bool LoadChildWidgets( const char* widgetFile, csArray<pawsWidget *> &loadedWidgets );


    /** @brief Create a new widget.
     *
     *  This creates a new widget based on the factory that is passed in.
     *  @param factoryName The name of the factory that is used to create a
     *  widget.
     *  @return A new instance of a widget if the factory was found. NULL
     *  if the widget could not be found.
     */
    pawsWidget* CreateWidget( const char* factoryName );

    /// Returns the widget that is focused.
    pawsWidget* GetCurrentFocusedWidget() { return currentFocusedWidget; }

    /// Returns true if the current focused widget needs to override all controls
    bool GetFocusOverridesControls() { return focusOverridesControls; }

    /** @brief Give this widget focus.
     *  @param widget The widget to focus.
     */
    void SetCurrentFocusedWidget ( pawsWidget* widget );

    /** @brief Make this widget modal.
     *  @param widget The modal widget.
     */
    void SetModalWidget( pawsWidget* widget );

    /** pawsWidget destructor calls this so PawsManager can NULLify all
     * its links to the widget.
     */
    void OnWidgetDeleted(pawsWidget * widget);

    /** Remove focus and mouseover effect from widget if widget is hidden */
    void OnWidgetHidden(pawsWidget * widget);

    /** @brief Let the window manager know that a widget is being moved.
     *  @param moving The widget that is currently moving.
     */
    void MovingWidget( pawsWidget* moving );

    /** @brief Let the manager know that a widget is being resized.
     *  @param widget The widget that is being resized.
     *  @param flags The resize flags that this widget should be resized with.
     */
    void ResizingWidget( pawsWidget* widget, int flags );

    /// Returns the prefrence manager.
    pawsPrefManager* GetPrefs() { return prefs; }

    /// Returns the mouse.
    pawsMouse* GetMouse() { return mouse; }

    /// Returns the resized image.
    csRef<iPawsImage> GetResizeImage() { return resizeImg; }

    /** Locate a widget by name.
     * @param name The name of the widget.
     */
    pawsWidget* FindWidget( const char* name, bool complain=true );


    /// Returns the main widget.
    pawsMainWidget * GetMainWidget() { return mainWidget; }

    /// Returns the psLocalization object:
    psLocalization * GetLocalization() { return localization; }

    /// A shortcut - translation without need to call GetLocalization().
    csString Translate(const csString & orig);

    /** @brief Gets the widget that is being drag'n'dropped over screen with the mouse.
     *
     *  @return The widget being dragged.
     *  @remarks Ownership of the widget goes to psPawsManager. This means that
     *  this widget must not be already owned (e.g. be child of another widget).
     *  Parameter can be NULL.
     */
    pawsWidget * GetDragDropWidget();

    /** @brief Sets the widget that is being drag'n'dropped over screen with the mouse.
     *
     * @param dragDropWidget The widget to drag.
     * @remarks Ownership of the widget goes to psPawsManager. This means that
     * this widget must not be already owned (e.g. be child of another widget).
     */
    void SetDragDropWidget(pawsWidget * dragDropWidget);

    /** @brief Gets the factor the font should be adjusted by for proper fontsize based on resolution.
     *  @return the factor for the fontsize.
     */
    float GetFontFactor() { return fontFactor; };

    /** @brief Creates a warning box with the supplied text.
     *  @param message The warning.
     *  @param notify The widget which recevies event notifications ( i.e. Button Pressed ).
     *  @param modal If the widet should be a modal one or not.
     */
    void CreateWarningBox( const char* message, pawsWidget* notify = NULL, bool modal = true );

    /** @brief Creates a YesNo box with the supplied text.
     *  @param message The warning.
     *  @param notify The Widget which recives event notifications (i.e. Button Pressed).
     +
     */
    void CreateYesNoBox( const char* message, pawsWidget* notify = NULL, bool modal = true );

    /** @brief Applies PAWS style to XML node.
     *  @see pawsstyles.h
     */
    bool ApplyStyle(const char * name, iDocumentNode * target);

    /*                          Sound Functions
    ------------------------------------------------------------------------*/

    /** @brief Registers a pre-processed sound in the list of available sounds.
     *
      * This allows an application to handle its own sound loading prior to
      * initializing anyPaws widgets that use sounds.  The widgets may then
      * reference the registered sounds by any name provided here.
      *
      * @return FALSE if registration was not possible (conflict of name or
      * memory allocation error).
     */
    bool RegisterSound(const char *name, csRef<iSndSysData> sounddata);

    /** @brief Loads a sound given a filename and an optional 'registered' name.
      *
      * If the registered name is specified, it is used for all operations
      * except loading the sound data from the VFS.  Otherwise the filename
      * is used for all operations. The name is first checked against the list
      * of previously registered or loaded sounds. If no match is found, a
      * file with the given filename is loaded. If no file is found or the
      * file cannot be processed into a sound handle the function fails and
      * returns an invalid csRef<>  (return.IsValid() == false).
     */
    csRef<iSndSysData> LoadSound(const char *filename, const char *registeredname=NULL);

    /// Plays a sound given the iSndSysData returned from LoadSound.
    bool PlaySound(csRef<iSndSysData> sound);

    /// Sets the volume for PAWS sound playback.
    void SetVolume(float vol)  { volume = vol; }

    /// Returns the current sound volume.
    float GetVolume() { return volume; }

    /// Returns the pawsConfig data.
    const char * GetConfigFile() { return pawsConfig.GetData(); }

    /// Turns sound on and off by setting useSounds.
    void ToggleSounds(bool value);

    /// Returns play status.
    bool PlayingSounds() { return useSounds; }

    /*                       Subcription Functions
    ------------------------------------------------------------------------*/

    /// Unsubscribe the given subscriber.
    void UnSubscribe(iPAWSSubscriber *listener);

    /// Subscribe to a named piece of data, so updates are received automatically.
    void Subscribe(const char *dataname,iPAWSSubscriber *listener);

    /// Announce a change in a named element to all subscribers.
    void Publish(const csString & dataname,PAWSData& data);

    /// Publish a string to all subscribers.
    void Publish(const csString & dataname,const char *datavalue);

    /// Publish a boolean value to all subscribers.
    void Publish(const csString & dataname,bool  datavalue);

    /// Publish an int to all subscribers.
    void Publish(const csString & dataname,int   datavalue);

    /// Publish an unsigned int to all subscribers.
    void Publish(const csString & dataname,unsigned int   datavalue);

    /// Publish a float to all subscribers.
    void Publish(const csString & dataname,float datavalue);
    
    /// Publish a coloured string to all subscribers.
    void Publish(const csString & dataname, const char *datavalue, int color);

    /// Publish nothing to all subscribers. (Used for one-time named signals.)
    void Publish(const csString & dataname);

    /// Return a list of all subscribers.
    csArray<iPAWSSubscriber*> ListSubscribers(const char *dataname);

    ///Get if sound is available or not (psclient.cfg)
    bool GetSoundStatus() { return soundStatus;}

    ///Set if sound is available or not (psclient.cfg)
    void SetSoundStatus(bool sound){ soundStatus = sound; }

    MathEnvironment & ExtraScriptVars() { return extraScriptVars; }

    csString &getVFSPathToSkin() { return vfsPathToSkin; }

protected:
    MathEnvironment extraScriptVars;

    psPoint MouseLocation( iEvent &ev );

    /** Sets position of the dragDropWidget (if there is one) to the position
     * of the mouse cursor.
     */
    void PositionDragDropWidget();

    /// VFS Mount directory mapping to the specified skin zip file
    csString vfsPathToSkin;

    /// Localized file object registry.
    psLocalization * localization;

    /// The preference/default manager.
    pawsPrefManager * prefs;

    /// The last widget that the mouse clicked on. ( Hence the focused one ).
    pawsWidget* currentFocusedWidget;

    /// Does the currentFocusedWidget take focus from the control system?
    bool focusOverridesControls;

    /// The last widget that the mouse moved over. ( For alpha fade effects ).
    pawsWidget* mouseoverWidget;

    /// The actual widget that was last faded.
    pawsWidget* lastfadeWidget;

    /// The time mouse has been over the last widget.
    csTicks timeOver;

    /// The time mouse has been over a  widget before showing the tooltip.
    int tipDelay;

    /// Current widget that is being moved.
    pawsWidget* movingWidget;

    /// Current modal widget.
    pawsWidget* modalWidget;

    /// Holds the widget that is being resized.
    pawsWidget* resizingWidget;

    /// The resize flags.
    int resizingFlags;

    /// Helper function to load standard factories.
    void RegisterFactories();

    /** @brief Process mouse movement events.
     *
     *  Determines if a widget is moving or being resized. If there is no modal
     *  widget it operates on the topmost widget at the event coordinates.
     *
     *  @param event iEvent to process.
     *  @return TRUE if it moves, resizes or fades in a widget.
     */
    bool HandleMouseMove( iEvent& event );

     /** @brief Process mouse down events.
      *
      *  Determines the widget at event coordinates.
      *  Calls OnMouseDown() on the currentFocusedWidget or widget at the event
      *  coordinates.
      *  @param event iEvent to process.
      *  @return TRUE if event stops moving or resizing.
      */
    bool HandleMouseDown( iEvent& event );


    /** @brief Process mouse double click events.
      *
      * Calls OnDoubleClick on the currentFocusedWidget or widget at the event
      * coordinates.
      *
      * @param event iEvent to process.
      * @return Results of OnDoubleClick or FALSE.
      */
    bool HandleDoubleClick( iEvent& event );

     /** @brief Process mouse up events.
      *
      *  Stops moving or resizing and turns off the corresponding flag.
      *  Calls OnMouseUp on the currentFocusedWidget or widget at the event
      *  coordinates.
      *
      *  @param event iEvent to process.
      *  @return Results of OnMouseUp or FALSE.
      */
    bool HandleMouseUp( iEvent& event );

    /** @brief Process key down events.
     *
     * If a widget has focus it extracts the event keycode, key and modifiers
     * and calls OnKeyDown on the focused widget.
     *
     * @param event iEvent to process.
     * @return TRUE if a widget has focus AND the current event type is key up
     * while hadKeyDown was set OR if OnKeyDown returns TRUE when called on
     * the focused widget.
     * @remarks Sets hadKeyDown to result of OnKeyDown call.
     */
    bool HandleKeyDown( iEvent& event );

    /** The object registry.
     */
    iObjectRegistry* objectReg;

    /** Pointer to the Crystal Space iGraphics2D renderer used to display
     * 2D graphics.
     */
    csRef<iGraphics2D> graphics2D;

    /** Pointer to the Crystal Space iGraphics3D renderer used to display
     * 3D graphics.
     */
    csRef<iGraphics3D> graphics3D;

    /** For event parsing.
     */
    csRef<iEventNameRegistry> nameRegistry;

    /// The main handler widget.
    pawsMainWidget* mainWidget;

    /// The texture manager.
    pawsTextureManager* textureManager;

    /// The mouse pointer.
    pawsMouse* mouse;

    /// Resized image.
    csRef<iPawsImage> resizeImg;

    /// An array of pointers to available factories.
    csPDelArray<pawsWidgetFactory> factories;

    /// Pointer to the Crystal Space iVFS file system.
    csRef<iVFS> vfs;

    /// Pointer to the Crystal Space iDocumentSystem.
    csRef<iDocumentSystem>  xml;

    /// Flag for key down function.
    bool hadKeyDown;

    /**
     * The widget that is drag'n'dropped across the screen by the mouse.
     * If it is not NULL, it is drawn instead of mouse on its position.
     * This widget has no parent widget (same as with pawsMainWidget)
     */
    pawsWidget * dragDropWidget;

    /// The font resizing factor for all widgets
    float fontFactor;

    // Feature defines.
    enum gfxFeatures
    {
        useLowestShaders = 0x1,
        useLowShaders = 0x2,
        useMediumShaders = 0x4,
        useHighShaders = 0x8,
        useHighestShaders = 0x10,
        useShadows = 0x20,
        useMeshGen = 0x40,
        useAll = (useHighShaders | useShadows | useMeshGen)
    };

    /// Graphics features we want to use.
    uint gfxFeatures;


    /*                      Sound Member Variables
    ------------------------------------------------------------------------*/

    /** A hash of sounds currently available to Paws components.
     * Hashed based on the registered name.
     */
    csHash<PawsSoundHandle *> sounds;

    /// Pointer to the Crystal Space iSndSysLoader used to load sounds.
    csRef<iSndSysLoader> soundloader;

    /// Pointer to the Crystal Space iSndSysRenderer used to play sounds.
    csRef<iSndSysRenderer> soundrenderer;

    /// Destructor helper for sound hash.
    void ReleaseAllSounds();

    /** @brief Parses given file and returns the <widget_description> tag of it
     *  @return NULL on failure.
     */
    csRef<iDocumentNode> ParseWidgetFile( const char* widgetFile );

    /// The config file to store stuff like window positions.
    csString pawsConfig;

    /// This is the internal var for sounds.
    bool useSounds;

    /// The volume for PAWS sound playback.
    float volume;

    /// PAWS style definitions.
    pawsStyles * styles;

    /// Table of subscriptions.
    PAWSSubscriptionsHash subscriptions;

    /// Is sound on or off?
    bool soundStatus;

    /*                      Shortcuts for events
    ------------------------------------------------------------------------*/
    /// Shortcut for event mouse move
    csEventID MouseMove;
    /// Shortcut for event mouse down
    csEventID MouseDown;
    /// Shortcut for event mouse double click
    csEventID MouseDoubleClick;
    /// Shortcut for event mouse up
    csEventID MouseUp;
    /// Shortcut for event key down
    csEventID KeyboardDown;
    /// Shortcut for event key up
    csEventID KeyboardUp;

};

/// Stores the relationship between iSndSysData * and sound filename.
class PawsSoundHandle
{
public:
    PawsSoundHandle(const char *soundname,csRef<iSndSysData> sounddata) : name (soundname), snddata (sounddata) {};
    ~PawsSoundHandle() {};

public:
    csString name;
    csRef<iSndSysData> snddata;
};

/// Data types for pub/sub.
enum PAWSDATATYPE
{
    PAWS_DATA_UNKNOWN,
    PAWS_DATA_STR,
    PAWS_DATA_BOOL,
    PAWS_DATA_INT,
    PAWS_DATA_UINT,
    PAWS_DATA_FLOAT,
    PAWS_DATA_INT_STR
};


struct PAWSData
{
    static csString temp_buffer;

    PAWSDATATYPE type;
    union
    {
        int intval;
        unsigned int uintval;
        float floatval;
        bool boolval;
    };
    csString str;  //csString cannot be inside union

    PAWSData() { type=PAWS_DATA_UNKNOWN; }
    PAWSData& operator=(PAWSData& other)
    {
        type = other.type;
        intval = other.intval;
        str = other.str;
        return *this;
    }

    bool IsData() { return type != PAWS_DATA_UNKNOWN; }

    const char *GetStr();
    float GetFloat();
    int GetInt();
    unsigned int GetUInt();
    bool GetBool();
};


struct iPAWSSubscriber
{
    virtual void OnUpdateData(const char *name,PAWSData& data) = 0;
    virtual void NewSubscription(const char *name) = 0;
    virtual ~iPAWSSubscriber() {};
};

struct PAWSSubscription
{
    PAWSData lastKnownValue;
    iPAWSSubscriber *subscriber;
};

#endif



