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

#include "download.h"
#include "fileutil.h"

/* To be incremented every commit */
#define UPDATER_VERSION 1

struct iConfigManager;
struct iVFS;

class psUpdaterEngine
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
    psUpdaterConfig* config;

    /* Real name of the app (e.g. updater, pslaunch, etc.) */
    csString appName;

    /* XML doc; reading from xml files in the update zip. */
    csRef<iDocument> configdoc;
public:
    psUpdaterEngine(const csArray<csString> args, iObjectRegistry* _object_reg, const char* _appName);
    ~psUpdaterEngine();

    /* Return the config object */
    psUpdaterConfig* GetConfig() { return config; }

    /* Return VFS */
    csRef<iVFS> GetVFS() { return vfs; }

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
};

#endif // __UPDATERENGINE_H__
