/*
* updaterconfig.cpp - Author: Mike Gist
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

#include <psconfig.h>

#include "updaterconfig.h"

iObjectRegistry* psUpdaterConfig::object_reg = NULL;

psUpdaterConfig::psUpdaterConfig(csArray<csString> args, iObjectRegistry* _object_reg, csRef<iVFS> _vfs)
{
    // Initialize the config manager.
    object_reg = _object_reg;
    vfs = _vfs;

    if (!csInitializer::SetupConfigManager(object_reg, CONFIG_FILENAME))
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, "updater2",
            "csInitializer::SetupConfigManager failed!\n"
            "Is your CRYSTAL environment variable set?");
        PS_PAUSEEXIT(1);
    }
    configManager = csQueryRegistry<iConfigManager> (object_reg);

    // Check if we're in the middle of a self update, and the iteration.
    selfUpdating = 0;
    for(uint i=0; i<args.GetSize(); i++)
    {
        csString arg = args.Pop();
        if(arg.Compare("selfUpdateFirst"))
            selfUpdating = 1;
        else if(arg.Compare("selfUpdateSecond"))
            selfUpdating = 2;
    }

    // Load config settings from cfg file.
    configFile = new csConfigFile(CONFIG_FILENAME, vfs);
    cleanUpdate = configFile->GetBool("Update.Clean", true);
    proxy.host = configFile->GetStr("Updater.Proxy.Host", "");
    proxy.port = configFile->GetInt("Updater.Proxy.Port", 0);

    // Init xml config objects.
    currentCon = new Config;
    newCon = new Config;
}

psUpdaterConfig::~psUpdaterConfig()
{
    delete currentCon;
    delete newCon;
    currentCon = NULL;
    newCon = NULL;
}

const char* Config::GetPlatform()
{
#if defined(CS_PLATFORM_WIN32)
    return "win32";
#elif defined(CS_PLATFORM_MACOSX)
    return "macosx";
#elif defined(CS_PLATFORM_UNIX) && CS_PROCESSOR_SIZE == 64
    return "linux64";
#elif defined(CS_PLATFORM_UNIX) && CS_PROCESSOR_SIZE == 32
    return "linux32";
#endif

}

bool Config::Initialize(csRef<iDocumentNode> node)
{
    // Get Updater info.
    csRef<iDocumentNode> updaterNode = node->GetNode("updater");
    if(updaterNode)
    {
        csRef<iDocumentNode> versionNode;
        versionNode = updaterNode->GetNode("version");
        if(versionNode)
        {
            updaterVersionLatest = versionNode->GetContentsValueAsInt();
        }
        csString md5 = "md5";
        csRef<iDocumentNode> md5Node;
        md5Node = updaterNode->GetNode(md5.Append(GetPlatform()));
        if(md5Node)
        {
            updaterVersionLatestMD5 = md5Node->GetContentsValue();
        }
    }
    else
    {
        printf("Unable to load updater node!\n");
        return false;
    }

    // Get mirrors.
    csRef<iDocumentNode> mirrorNode = node->GetNode("mirrors");
    if (mirrorNode)
    {
        csRef<iDocumentNodeIterator> nodeItr = mirrorNode->GetNodes();
        while(nodeItr->HasNext())
        {
            csRef<iDocumentNode> mNode = nodeItr->Next();

            if (!strcmp(mNode->GetValue(),"mirror"))
            {
                Mirror* mirror = new Mirror;
                mirror->id               = mNode->GetAttributeValueAsInt("id");
                mirror->name             = mNode->GetAttributeValue("name");
                mirror->baseURL          = mNode->GetAttributeValue("url");
                mirrors.Push(mirror);
            }
        }
    }
    else
    {
        printf("Unable to load mirrors!\n");
        return false;
    }

    // Get client versions.
    csRef<iDocumentNode> clientNode = node->GetNode("client");
    if(clientNode)
    {
        csRef<iDocumentNodeIterator> nodeItr = clientNode->GetNodes();
        while(nodeItr->HasNext())
        {
            csRef<iDocumentNode> cNode = nodeItr->Next();
            if(!strcmp(cNode->GetValue(),"version"))
            {
                ClientVersion* cVersion = new ClientVersion;
                cVersion->name = cNode->GetAttributeValue("name");
                csString md5 = "md5";
                cVersion->md5sum = cNode->GetAttributeValue(md5.Append(GetPlatform()));
                clientVersions.Push(cVersion);
            }
        }
    }
    else
    {
        printf("Unable to load client info!\n");
        return false;
    }

    // Successfully Loaded!
    return true;
}

Mirror* Config::GetMirror(uint x)
{
    Mirror * mirror = NULL;
    if ( x >=0 && x < mirrors.GetSize() )
    {
        mirror = mirrors[x];
    }
    
    return mirror;
}

csPDelArray<Mirror>* Config::GetMirrors()
{
    csPDelArray<Mirror>* a;
    a = &mirrors;
    return a;
}

csPDelArray<ClientVersion>* Config::GetClientVersions()
{
    csPDelArray<ClientVersion>* a;
    a = &clientVersions;
    return a;
}
