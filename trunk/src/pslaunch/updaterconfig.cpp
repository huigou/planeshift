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

#include <cssysdef.h>
#include <csutil/stringarray.h>
#include "updaterconfig.h"

#if defined(CS_PLATFORM_MACOSX)
#include <Carbon/Carbon.h>
#endif

iObjectRegistry* UpdaterConfig::object_reg = NULL;

UpdaterConfig::UpdaterConfig(csStringArray& args, iObjectRegistry* _object_reg, csRef<iVFS> _vfs)
{
    // Initialize the config manager.
    object_reg = _object_reg;
    vfs = _vfs;

    if (!csInitializer::SetupConfigManager(object_reg, CONFIG_FILENAME))
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, "updater2",
            "csInitializer::SetupConfigManager failed!\n"
            "Is your CRYSTAL environment variable set?");
        exit(1);
    }
    configManager = csQueryRegistry<iConfigManager> (object_reg);

    // Check if we're in the middle of a self update or doing an integrity check.
    selfUpdating = 0;
    checkIntegrity = false;
    switchMirror = false;
    csString last;
    for(uint i=1; i<=args.GetSize(); i++)
    {
        csString arg = args[args.GetSize()-i];
        if(arg.CompareNoCase("selfUpdateFirst"))
            selfUpdating = 1;
        else if(arg.CompareNoCase("selfUpdateSecond"))
            selfUpdating = 2;
        else if(arg.CompareNoCase("-repair") || arg.CompareNoCase("--repair"))
            checkIntegrity = true;
        else if(arg.CompareNoCase("-switch") || arg.CompareNoCase("--switch"))
        {
            switchMirror = true;
            checkIntegrity = true;
            newMirror = last;
        }
        else
        {
            last = arg;
        }
    }

    // Load config settings from cfg file.
    if(!vfs->Exists(CONFIG_FILENAME))
    {
        vfs->WriteFile(CONFIG_FILENAME, "", 0);
    }
    configFile = new csConfigFile(CONFIG_FILENAME, vfs);
    cleanUpdate = configFile->GetBool("Update.Clean", true);
    updatePlatform = configFile->GetBool("Update.Platform", true);
    repairInZip = configFile->GetBool("Update.RepairInZip");
    proxy.host = configFile->GetStr("Updater.Proxy.Host", "");
    proxy.port = configFile->GetInt("Updater.Proxy.Port", 0);

    // Init xml config objects.
    currentCon = new Config();
    newCon = new Config();
}

UpdaterConfig::~UpdaterConfig()
{
    delete currentCon;
    delete newCon;
    currentCon = NULL;
    newCon = NULL;
}

const char* Config::GetPlatform() const
{
    if(!UpdaterConfig::GetSingletonPtr()->UpdatePlatform())
    {
        return GetGeneric();
    }
#if defined(CS_PLATFORM_WIN32)
    return "win32";
#elif defined(CS_PLATFORM_MACOSX)
    long version;
    Gestalt(gestaltSystemVersion, &version);
    if(version >= 0x1040)
    {
        return "macosx";
    }
    return "macosx1039";
#elif defined(CS_PLATFORM_UNIX) && CS_PROCESSOR_SIZE == 64
    return "linux64";
#elif defined(CS_PLATFORM_UNIX) && CS_PROCESSOR_SIZE == 32
    return "linux32";
#endif

}

Config::Config()
{
    updaterVersionLatest = 0.0f;    
}

bool Config::Initialize(csRef<iDocumentNode> node)
{
    // Get activity state.
    active = node->GetAttributeValueAsBool("active", true);

    // Get Updater info.
    csRef<iDocumentNode> updaterNode = node->GetNode("updater");
    if(updaterNode)
    {
        updaterVersionLatest = updaterNode->GetAttributeValueAsFloat("version");
        
        csString md5 = "md5";
        csRef<iDocumentNode> md5Node;
        md5.Append(GetPlatform());
        updaterVersionLatestMD5 = updaterNode->GetAttributeValue(md5);
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
        mirrors.Empty();
        csRef<iDocumentNodeIterator> nodeItr = mirrorNode->GetNodes();
        while(nodeItr->HasNext())
        {
            csRef<iDocumentNode> mNode = nodeItr->Next();

            if (!strcmp(mNode->GetValue(),"mirror"))
            {
                csRef<Mirror> mirror;
                mirror.AttachNew(new Mirror);
                mirror->SetID(mNode->GetAttributeValueAsInt("id"));
                mirror->SetName(mNode->GetAttributeValue("name"));
                mirror->SetBaseURL(mNode->GetAttributeValue("url"));
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
        clientVersions.Empty();
        csRef<iDocumentNodeIterator> nodeItr = clientNode->GetNodes();
        while(nodeItr->HasNext())
        {
            csRef<iDocumentNode> cNode = nodeItr->Next();
            if(!strcmp(cNode->GetValue(),"version"))
            {
                csRef<ClientVersion> cVersion;
                cVersion.AttachNew(new ClientVersion());
                cVersion->SetName(cNode->GetAttributeValue("name"));
                csString md5 = "md5";
                cVersion->SetMD5Sum(cNode->GetAttributeValue(md5 + GetPlatform()));
                cVersion->SetGenericMD5Sum(cNode->GetAttributeValue(md5 + GetGeneric()));
                clientVersions.PushSmart(cVersion);
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
    if (x < mirrors.GetSize())
    {
        mirror = mirrors[x];
    }
    
    return mirror;
}
