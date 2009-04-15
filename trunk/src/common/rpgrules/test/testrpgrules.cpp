/*
 *  testrpgrules.cpp - Author: Keith Fulton
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include "testrpgrules.h"

CS_IMPLEMENT_APPLICATION

TestRPGRules::TestRPGRules(iObjectRegistry* object_reg) : object_reg(object_reg)
{
    vfs = csQueryRegistry<iVFS>(object_reg);
    log = vfs->Open("/this/testrpgrules.log", VFS_FILE_WRITE);
}

TestRPGRules::~TestRPGRules()
{
}

void TestRPGRules::PrintHelp()
{
    printf("This application checks for duplicate meshfact and texture inclusions in art files.\n\n");

    printf("Options:\n");
    printf("-path The vfs path to directory to search in. Defaults to /this/ccheck/\n\n");
    printf("Usage: ccheck(.exe) -path /this/path/to/directory/\n");
}

int TestRPGRules::Run()
{
    PrintOutput("RPG Rules Library Unit Test Application.\n\n");
	return 0;
}


void TestRPGRules::PrintOutput(const char* string, ...)
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
											   CS_REQUEST_END);

    TestRPGRules* test = new TestRPGRules(object_reg);
    int ret = test->Run();
    delete test;

    return ret;
}
