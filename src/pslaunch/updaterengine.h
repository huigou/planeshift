/*
* updaterengine.h - Author: Mike Gist
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __UPDATERENGINE_H__
#define __UPDATERENGINE_H__

#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <iutil/cfgmgr.h>
#include <csutil/csstring.h>
#include <iutil/document.h>
#include <csutil/threading/thread.h>


#include "download.h"
#include "util/fileutil.h"

/* To be incremented every commit */
#define UPDATER_VERSION 2

struct iConfigManager;
struct iVFS;

class UpdaterEngine
{
private:
    static iObjectRegistry* object_reg;

    /* VFS to manage our zips */
    csRef<iVFS> vfs;

    /* File utilities */
    FileUtil* fileUtil;

    /* Downloader */
    Downloader* downloader;

    /* Config file; check for proxy and clean update. */
    UpdaterConfig* config;

    /* Real name of the app (e.g. updater, pslaunch, etc.) */
    csString appName;

    /* XML doc; reading from xml files in the update zip. */
    csRef<iDocument> configdoc;

    /* Array to store console output. */
    csArray<csString> *consoleOut;

    /* Set to true if we want the GUI to exit. */
    bool *exitGUI;

    /* Set to true if we need to tell the GUI that an update is pending. */
    bool *updateNeeded;

    /* If true, then it's okay to perform the update. */
    bool *performUpdate;

    /* True if we're using a GUI. */
    bool hasGUI;

    /* Output console prints to file. */
    FILE* log;
    
    CS::Threading::Mutex *mutex;

    /* Function shared by ctors */
    void Init(const csArray<csString> args, iObjectRegistry* _object_reg, const char* _appName,
         bool *_performUpdate, bool *_exitGui, bool *_updateNeeded, csArray<csString> *_consoleOut,
         CS::Threading::Mutex *_mutex);
    
public:
    UpdaterEngine(const csArray<csString> args, iObjectRegistry* _object_reg, const char* _appName);
    UpdaterEngine(const csArray<csString> args, iObjectRegistry* _object_reg, const char* _appName,
                  bool *_performUpdate, bool *_exitGui, bool *_updateNeeded, csArray<csString> *_consoleOut,
                  CS::Threading::Mutex *_mutex);
    ~UpdaterEngine();

    /* Return the config object */
    UpdaterConfig* GetConfig() const { return config; }

    /* Return VFS */
    csRef<iVFS> GetVFS() const { return vfs; }

    // Find the config node given an xml file name.
    csRef<iDocumentNode> GetRootNode(csString fileName);

    /*
    * Starts and finishes a self update
    * Returns true if a restart is needed (MSVC compiled)
    */
    bool selfUpdate(int selfUpdating);

    /* Starts a general update */
    void generalUpdate();

    /* Check for any updates */
    void checkForUpdates();

    /* Check for updater/launcher updates */
    bool checkUpdater();

    /* Check for 'general' updates */
    bool checkGeneral();

    /* Print to console and save to array for GUI output. */
    void printOutput(const char* string, ...);
};

#endif // __UPDATERENGINE_H__
