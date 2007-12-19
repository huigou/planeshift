/*
 * pssetup.h
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef _PSSETUP_H__
#define _PSSETUP_H__

#include <csutil/sysfunc.h>
#include <csutil/csstring.h>
#include <csutil/ref.h>

#include "util/genericevent.h"

struct iEngine;
struct iLoader;
struct iVFS;
struct iEvent;
struct iEventQueue;
struct iConfigManager;
struct iTextureManager;
struct iVirtualClock;
struct iGraphics3D;
struct iGraphics2D;

class PawsManager;
class pawsMainWidget;

class psSetupApp
{
public:

    static const char* CONFIG_FILENAME;
    static const char* APP_NAME;
    static const char* WINDOW_CAPTION;

    /** Constructor
     */
    psSetupApp(iObjectRegistry *obj_reg);

    /** Destructor
     */
    ~psSetupApp();

    /** Reports a severe error in the application
     *   @param msg the error message
     */
    void SevereError(const char* msg);

    /** Initializes some CS specific stuff, fills most of this classes global variables, and inits eedit specifics
     *   @return true on success, false otherwise
     */
    bool Init();

    /** Loads all the paws widgets (windows)
     *   @return true if all succeeded to load, false if any had an error
     */
    bool LoadWidgets();

    /** handles an event from the event handler
     *   @param Event the event to handle
     */
    bool HandleEvent (iEvent &ev);

    /**
     * Quit the application
     */
    void Quit();

    iObjectRegistry* GetObjReg() {return object_reg;}

private:

    /** Registers the custom PAWS widgets created in this application
     */
    void RegisterFactories();

    /// Declare our event handler
    DeclareGenericEventHandler(EventHandler,psSetupApp,"planeshift.setup");
    csRef<EventHandler> event_handler;

    iObjectRegistry*        object_reg;
    csRef<iEngine>          engine;
    csRef<iVFS>             vfs;
    csRef<iEventQueue>      queue;
    csRef<iConfigManager>   cfgmgr;
    csRef<iGraphics3D>      g3d;
    csRef<iGraphics2D>      g2d;
    csRef<iVirtualClock>    vc;

    // PAWS
    PawsManager*    paws;
    pawsMainWidget* mainWidget;

    /// keeps track of whether the window is visible or not
    bool drawScreen;

    /// Limits the frame rate either by sleeping
    void FrameLimit();

    /// Time when the last frame was drawn.
    csTicks elapsed;

};

#endif
