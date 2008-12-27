/*
 *  ccheck.cpp - Author: Mike Gist
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cstool/initapp.h>
#include <csutil/cmdhelp.h>
#include <csutil/stringarray.h>

#include <iutil/cmdline.h>
#include <iutil/document.h>
#include <iutil/stringarray.h>

#include "ccheck.h"

CS_IMPLEMENT_APPLICATION

CCheck::CCheck(iObjectRegistry* object_reg) : object_reg(object_reg)
{
    docsys = csQueryRegistry<iDocumentSystem>(object_reg);
    vfs = csQueryRegistry<iVFS>(object_reg);
    strings = csQueryRegistryTagInterface<iStringSet>(object_reg, "crystalspace.shared.stringset");
    log = vfs->Open("/this/ccheck.log", VFS_FILE_WRITE);
}

CCheck::~CCheck()
{
}

void CCheck::PrintHelp()
{
    printf("This application checks for duplicate meshfact and texture inclusions in art files.\n\n");

    printf("Options:\n");
    printf("-path The vfs path to directory to search in. Defaults to /this/ccheck/\n\n");
    printf("Usage: ccheck(.exe) -path /this/path/to/directory/\n");
}

void CCheck::Run()
{
    PrintOutput("Art Conflict Checker.\n\n");

    csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser>(object_reg);
    if (csCommandLineHelper::CheckHelp (object_reg))
    {
        PrintHelp();
        return;
    }

    csString rootpath = cmdline->GetOption("path");
    if(rootpath.IsEmpty())
    {
        rootpath = "/this/ccheck/";
    }

    csRef<iStringArray> files = vfs->FindFiles(rootpath);
    for(size_t i=0; i<files->GetSize(); i++)
    {
        csRef<iDataBuffer> filePath = vfs->GetRealPath(files->Get(i));
        
        if(csString(files->Get(i)).Length() > 4 &&
            csString(files->Get(i)).Slice(csString(files->Get(i)).Length()-4).Compare(".zip"))
        {
            csString fileName(files->Get(i));
            fileName = fileName.Slice(fileName.FindLast('/')+1);

            vfs->Mount("/ccheck/", filePath->GetData());
            csRef<iStringArray> zipfiles = vfs->FindFiles("/ccheck/");
            for(size_t j=0; j<zipfiles->GetSize(); j++)
            {
                csString zipfile = zipfiles->Get(j);
                if(zipfile.GetAt(zipfile.Length()-1) != '/')
                    ParseFile(zipfile, fileName);
            }
            vfs->Unmount("/ccheck/", filePath->GetData());
        }
        else
        {
            ParseFile(filePath->GetData(), filePath->GetData());
        }
    }

    PrintOutput("\nConflicted meshfacts:\n");

    for(size_t i=0; i<meshfacts.GetSize(); i++)
    {
        csArray<csString> conflicted = cmeshfacts.GetAll(meshfacts[i]);
        for(size_t j=0; 1<conflicted.GetSize() && j<conflicted.GetSize(); j++)
        {
            if(j == 0)
                PrintOutput("\nMeshFact '%s' conflicts in: ", meshfacts[i].GetData());

            PrintOutput("%s ", conflicted[j].GetData());
        }
    }

    PrintOutput("\n\nConflicted textures:\n");

    for(size_t i=0; i<textures.GetSize(); i++)
    {
        csArray<csString> conflicted = ctextures.GetAll(textures[i]);
        for(size_t j=0; 1<conflicted.GetSize() && j<conflicted.GetSize(); j++)
        {
            if(j == 0)
                PrintOutput("\nTexture '%s' conflicts in: ", textures[i].GetData());

            PrintOutput("%s ", conflicted[j].GetData());
        }
    }    
}

void CCheck::ParseFile(const char* filePath, const char* fileName)
{
    csRef<iDataBuffer> buf = vfs->ReadFile(filePath);
    csRef<iDocument> doc = docsys->CreateDocument();

    doc->Parse(buf, true);

    if(!doc->GetRoot().IsValid())
    {
        csString fp(filePath);
        fp = fp.Slice(fp.FindLast('/')+1);
        textures.PushSmart(fp);
        ctextures.Put(fp, fileName);
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot()->GetNode("library");
    if(!root.IsValid())
    {
        root = doc->GetRoot()->GetNode("world");
    }

    if(!root.IsValid())
    {
        return;
    }

    csRef<iDocumentNodeIterator> itr = root->GetNodes("meshfact");
    while(itr->HasNext())
    {
        csRef<iDocumentNode> node = itr->Next();
        meshfacts.PushSmart(node->GetAttributeValue("name"));
        cmeshfacts.Put(node->GetAttributeValue("name"), fileName);
    }
}

void CCheck::PrintOutput(const char* string, ...)
{
    csString outputString;
    va_list args;
    va_start (args, string);
    outputString.FormatV (string, args);
    va_end (args);

    printf("%s", outputString.GetData());

    if(log.IsValid())
    {
        log->Write(outputString.GetData(), outputString.Length());
    }    
}

int main(int argc, char** argv)
{
    iObjectRegistry* object_reg = csInitializer::CreateEnvironment(argc, argv);
    if(!object_reg)
    {
        printf("Object Reg failed to Init!\n");
        return 1;
    }

    csInitializer::RequestPlugins (object_reg, CS_REQUEST_VFS,
	CS_REQUEST_PLUGIN("crystalspace.documentsystem.multiplexer", iDocumentSystem),
	CS_REQUEST_END);

    CCheck* ccheck = new CCheck(object_reg);
    ccheck->Run();
    delete ccheck;

    return 0;
}
