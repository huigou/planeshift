/*
* updaterconfig.h - Author: Mike Gist
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

#ifndef __UPDATERCONFIG_H__
#define __UPDATERCONFIG_H__

#include <csutil/csstring.h>
#include <cstool/initapp.h>
#include <csutil/array.h>
#include <csutil/parray.h>
#include <csutil/cfgmgr.h>
#include <csutil/cfgfile.h>
#include <iutil/document.h>

#define CONFIG_FILENAME "/this/pslaunch.cfg"
#define UPDATERINFO_FILENAME "/this/updaterinfo.xml"
#define UPDATERINFO_OLD_FILENAME "/this/updaterinfo.xml.bak"

#ifndef PS_PAUSEEXIT
#define PS_PAUSEEXIT(x) exit(x)
#endif

class UpdaterConfig;

class Mirror
{
public:
    /* Return mirror ID */
    unsigned int GetID() { return id; }

    /* Return mirror name */
    csString GetName() { return name; }

    /* Return mirror URL */
    csString GetBaseURL() { return baseURL; }

    /* Set mirror URL */
    void SetBaseURL(csString url) { baseURL = url; }

protected:
    friend class Config;
    /* Mirror ID */
    uint id;

    /* Mirror name */
    csString name;

    /* URL of the mirror (including update dir) */
    csString baseURL;
};

class ClientVersion
{
public:
    /* Get client update file name */
    csString GetName() { return name; }

    /* Get client update file md5sum */
    csString GetMD5Sum() { return md5sum; }
protected:
    friend class Config;
    /* Client update file name */
    csString name;

    /* md5sum of the client update file */
    csString md5sum;
};

struct Proxy
{
    /* Hostname of the proxy */
    csString host;
    /* Port */
    uint port;
};

class Config
{
public:
    Mirror* GetMirror(uint mirrorNumber);

    /* Get mirrors. */
    csPDelArray<Mirror>* GetMirrors();

    /* Get clientVersions list. */
    csPDelArray<ClientVersion>* GetClientVersions();

    /* Init a xml config file. */
    bool Initialize(csRef<iDocumentNode> node);

    /* Get our platform string. */
    const char* GetPlatform();

    /* Get latest updater version */
    int GetUpdaterVersionLatest() { return updaterVersionLatest; }

    /* Get latest updater version md5sum */
    const char* GetUpdaterVersionLatestMD5() { return updaterVersionLatestMD5; }


private:
    /* Latest updater version */
    int updaterVersionLatest;

    /* Latest version md5sum */
    const char* updaterVersionLatestMD5;

    /* List of mirrors */
    csPDelArray<Mirror> mirrors;

    /* List of client versions */
    csPDelArray<ClientVersion> clientVersions;
};

class UpdaterConfig
{
public:
    UpdaterConfig(const csArray<csString> args, iObjectRegistry* _object_reg, csRef<iVFS> _vfs);
    ~UpdaterConfig();

    /* Returns true if the updater is self updating */
    int IsSelfUpdating() const { return selfUpdating; }

    /* Returns the proxy struct */
    Proxy GetProxy() { return proxy; }

    /* Returns the current/old config from updaterinfo.xml */
    Config* GetCurrentConfig() { return currentCon; }

    /* Returns the new/downloaded config from updaterinfo.xml */
    Config* GetNewConfig() { return newCon; }

    /* Returns true if the last update was successful */
    bool WasCleanUpdate() { return cleanUpdate; }

    /* Returns the configfile for the app */
    csRef<iConfigFile> GetConfigFile() { return configFile; }


private:
    /* Holds stage of self updating. */
    int selfUpdating;

    /* Holds whether or not the last update was successful */
    bool cleanUpdate;

    /* VFS, Object Registry */
    csRef<iVFS> vfs;
    static iObjectRegistry* object_reg;

    /* Config Manager */
    csRef<iConfigManager> configManager;

    /* Config File */
    csRef<iConfigFile> configFile;

    /* Proxy server */
    Proxy proxy;

    /* Current Config */
    Config* currentCon;

    /* New Config */
    Config* newCon;
};

#endif // __UPDATERCONFIG_H__
