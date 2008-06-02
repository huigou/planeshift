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
#include <csutil/cfgmgr.h>
#include <csutil/cfgfile.h>
#include <iutil/document.h>

#define CONFIG_FILENAME "/this/psupdater.cfg"
#define UPDATERINFO_FILENAME "/this/updaterinfo.xml"
#define UPDATERINFO_OLD_FILENAME "/this/updaterinfo.xml.bak"

class Mirror : public csRefCount
{
public:
    Mirror() {}
    ~Mirror() {}

    /* Return mirror ID */
    unsigned int GetID() const { return id; }

    /* Return mirror name */
    const char* GetName() const { return name; }

    /* Return mirror URL */
    const char* GetBaseURL() const { return baseURL; }

    /* Set mirror URL */
    void SetBaseURL(csString url) { baseURL = url; }

    void SetID(uint id) { this->id = id; }
    void SetName(const char* name) { this->name = name; }
    void SetBaseURL(const char* baseURL) { this->baseURL = baseURL; }

protected:
    /* Mirror ID */
    uint id;

    /* Mirror name */
    csString name;

    /* URL of the mirror (including update dir) */
    csString baseURL;
};

class ClientVersion : public csRefCount
{
public:
    ClientVersion() {}
    ~ClientVersion() {}

    /* Get client update file name */
    const char* GetName() const { return name; }

    /* Get client update file md5sum */
    const char* GetMD5Sum() const { return md5sum; }

    void SetName(const char* name) { this->name = name; }
    void SetMD5Sum(const char* md5sum) { this->md5sum = md5sum; }

private:
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

/** Holds an updater configuration file.
 */
class Config
{
public:
    Config();

    Mirror* GetMirror(uint mirrorNumber);

    /* Get mirrors. */
    csRefArray<Mirror>& GetMirrors() { return mirrors; }
    /* Get clientVersions list. */
    csRefArray<ClientVersion>& GetClientVersions() { return clientVersions; }

    /* Init a xml config file. */
    bool Initialize(csRef<iDocumentNode> node);

    /* Get our platform string. */
    const char* GetPlatform() const;

    /* Get latest updater version */
    float GetUpdaterVersionLatest() const { return updaterVersionLatest; }

    /* Get latest updater version md5sum */
    const char* GetUpdaterVersionLatestMD5() const { return updaterVersionLatestMD5; }


private:
    /* Latest updater version */
    float updaterVersionLatest;

    /* Latest version md5sum */
    const char* updaterVersionLatestMD5;

    /* List of mirrors */
    csRefArray<Mirror> mirrors;

    /* List of client versions */
    csRefArray<ClientVersion> clientVersions;
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
    Config* GetCurrentConfig() const { return currentCon; }

    /* Returns the new/downloaded config from updaterinfo.xml */
    Config* GetNewConfig() const { return newCon; }

    /* Returns true if the last update was successful */
    bool WasCleanUpdate() const { return cleanUpdate; }

    /* Returns true if we want the updater to update executable files. */
    bool UpdateExecs() const { return updateExecs; }

    /* Returns the configfile for the app */
    csRef<iConfigFile> GetConfigFile() const { return configFile; }


private:
    /* Holds stage of self updating. */
    int selfUpdating;

    /* Holds whether or not the last update was successful. */
    bool cleanUpdate;

    /* True if we want the updater to update executable files. */
    bool updateExecs;

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
