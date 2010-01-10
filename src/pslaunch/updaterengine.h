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
#include "util/singleton.h"

/* To be incremented every time we want to make an update. */
#define UPDATER_VERSION 2.0

struct iConfigManager;
struct iVFS;

class InfoShare
{
private:
    /* Set to true if we want the GUI to exit. */
    volatile bool exitGUI;

    /* Set to true to cancel the updater. */
    volatile bool cancelUpdater;

    /* Set to true if we need to tell the GUI that an update is pending. */
    volatile bool updateNeeded;

    /* Set to true if an update is available but admin permissions are needed. */
    volatile bool updateAdminNeeded;

    /* If true, then it's okay to perform the update. */
    volatile bool performUpdate;

    /* Set to true to perform an integrity check. */
    volatile bool checkIntegrity;

    /* Set to true once we have checked for an update. */
    volatile bool updateChecked;

    /* Safety. */
    CS::Threading::Mutex mutex;

    /* Sync condition */
    CS::Threading::Condition synched;
    bool synching;

    /* Array to store console output. */
    csList<csString> consoleOut;
public:

    InfoShare()
    {
        exitGUI = false;
        performUpdate = false;
        updateNeeded = false;
        updateAdminNeeded = false;
        checkIntegrity = false;
        updateChecked = false;
        cancelUpdater = false;
        synching = false;
        mutex.Initialize();
    }

    inline void SetExitGUI(bool v) { exitGUI = v; }
    inline void SetUpdateNeeded(bool v) { updateNeeded = v; }
    inline void SetUpdateAdminNeeded(bool v) { updateAdminNeeded = v; }
    inline void SetPerformUpdate(bool v) { performUpdate = v; }
    inline void SetCheckIntegrity(bool v) { checkIntegrity = v; }
    inline void SetCancelUpdater(bool v) { cancelUpdater = v; }
    inline void SetUpdateChecked(bool v) { updateChecked = v; }
    inline void Sync()
    {
        CS::Threading::MutexScopedLock lock(mutex);
        if(!synching)
        {
            synching = true;
            synched.Wait(mutex);
        }
        else
        {
            synched.NotifyAll();
            synching = false;
        }
    }

    inline bool GetExitGUI() { return exitGUI; }
    inline bool GetUpdateNeeded() { return updateNeeded; }
    inline bool GetUpdateAdminNeeded() { return updateAdminNeeded; }
    inline bool GetPerformUpdate() { return performUpdate; }
    inline bool GetCheckIntegrity() { return checkIntegrity; }
    inline bool GetCancelUpdater() { return cancelUpdater; }
    inline bool GetUpdateChecked() { return updateChecked; }

    inline void EmptyConsole()
    {
        CS::Threading::MutexScopedLock lock(mutex);
        consoleOut.DeleteAll();
    }

    inline void ConsolePush(csString str)
    {
        CS::Threading::MutexScopedLock lock(mutex);
        consoleOut.PushBack(str);
    }

    inline csString ConsolePop()
    {
        CS::Threading::MutexScopedLock lock(mutex);
        csString ret = consoleOut.Front();
        consoleOut.PopFront();
        return ret;
    }

    inline bool ConsoleIsEmpty()
    {
        return consoleOut.IsEmpty();
    }
};

class UpdaterEngine : public CS::Threading::Runnable, public Singleton<UpdaterEngine>
{
private:
    iObjectRegistry* object_reg;

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
    void Init(csStringArray& args, iObjectRegistry* _object_reg, const char* _appName,
              InfoShare *infoshare);

    void CheckMD5s(iDocumentNode* md5sums, csString& baseurl, bool accepted = false);
    
public:
    UpdaterEngine(csStringArray& args, iObjectRegistry* _object_reg, const char* _appName);
    UpdaterEngine(csStringArray& args, iObjectRegistry* _object_reg, const char* _appName,
                  InfoShare *infoshare);
    ~UpdaterEngine();

    /* Return the config object */
    UpdaterConfig* GetConfig() const { return config; }

    /* Return VFS */
    csRef<iVFS> GetVFS() const { return vfs; }

    // Find the config node given an xml file name.
    csRef<iDocumentNode> GetRootNode(const char* fileName, csRef<iDocument>* document = 0);

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
    void CheckIntegrity(bool automatic = false);

    /* Switch updater mirror. */
    bool SwitchMirror();

    /* Check if a quit event has been triggered. */
    inline bool CheckQuit() { return infoShare->GetCancelUpdater(); }

    /* Print to console and save to array for GUI output. */
    void PrintOutput(const char* string, ...);

    /* Run updater thread. */
    void Run();
};

#endif // __UPDATERENGINE_H__
