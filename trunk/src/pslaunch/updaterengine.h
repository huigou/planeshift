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
#include <csutil/list.h>


#include "download.h"
#include "util/fileutil.h"

/* To be incremented every time we want to make an update. */
#define UPDATER_VERSION 5

struct iConfigManager;
struct iVFS;

class InfoShare
{
private:
    /* Set to true if we want the GUI to exit. */
    bool exitGUI;

    /* Set to true if we need to tell the GUI that an update is pending. */
    bool updateNeeded;

    /* If true, then it's okay to perform the update. */
    bool performUpdate;

    /* Set to true to perform an integrity check. */
    bool checkIntegrity;

    /* Safety. */
    CS::Threading::Mutex mutex;

    /* Array to store console output. */
    csList<csString> consoleOut;
public:

    InfoShare()
    {
        exitGUI = false;
        performUpdate = true;
        updateNeeded = true;
        checkIntegrity = false;
        mutex.Initialize();
    }

    void SetExitGUI(bool v) { exitGUI = v; }
    void SetUpdateNeeded(bool v) { updateNeeded = v; }
    void SetPerformUpdate(bool v) { performUpdate = v; }
    void SetCheckIntegrity(bool v) { checkIntegrity = v; }

    bool GetExitGUI() { return exitGUI; }
    bool GetUpdateNeeded() { return updateNeeded; }
    bool GetPerformUpdate() { return performUpdate; }
    bool GetCheckIntegrity() { return checkIntegrity; }

    void ConsolePush(csString str)
    {
        mutex.Lock();
        consoleOut.PushBack(str);
        mutex.Unlock();
    }

    csString ConsolePop()
    {
        mutex.Lock();
        csString ret = consoleOut.Front();
        consoleOut.PopFront();
        mutex.Unlock();
        return ret;
    }

    bool ConsoleIsEmpty()
    {
        return consoleOut.IsEmpty();
    }
};

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

    /* Info shared with other threads. */
    InfoShare *infoShare;

    /* True if we're using a GUI. */
    bool hasGUI;

    /* Output console prints to file. */
    csRef<iFile> log;

    /* Function shared by ctors */
    void Init(const csArray<csString> args, iObjectRegistry* _object_reg, const char* _appName,
              InfoShare *infoshare);
    
public:
    UpdaterEngine(const csArray<csString> args, iObjectRegistry* _object_reg, const char* _appName);
    UpdaterEngine(const csArray<csString> args, iObjectRegistry* _object_reg, const char* _appName,
                  InfoShare *infoshare);
    ~UpdaterEngine();

    /* Return the config object */
    UpdaterConfig* GetConfig() const { return config; }

    /* Return VFS */
    csRef<iVFS> GetVFS() const { return vfs; }

    // Find the config node given an xml file name.
    csRef<iDocumentNode> GetRootNode(const char* fileName);

    /*
    * Starts and finishes a self update
    * Returns true if a restart is needed (MSVC compiled)
    */
    bool SelfUpdate(int selfUpdating);

    /* Starts a general update */
    void GeneralUpdate();

    /* Check for any updates */
    void CheckForUpdates();

    /* Check for updater/launcher updates */
    bool CheckUpdater();

    /* Check for 'general' updates */
    bool CheckGeneral();

    /* Check the integrity of the install */
    void CheckIntegrity();

    /* Print to console and save to array for GUI output. */
    void PrintOutput(const char* string, ...);
};

#endif // __UPDATERENGINE_H__
