/*
 * crashreport.cpp by Andrew Dai
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
// Code for wrapping around Google Breakpad.

#ifdef WIN32
#include "client/windows/handler/exception_handler.h"
#include "client/windows/sender/crash_report_sender.h"
#elif defined(CS_PLATFORM_MACOSX)
// TODO
#else
// TODO
#endif

#include <map>
#include <string>
#include <psconfig.h>
#include "globals.h"
#include "psengine.h"
#include "pscelclient.h"

const wchar_t* crash_post_url = L"http://planeshift.ezpcusa.com/crash_reporting/upload.py";

// Only support breakpad for win32 for now.
#ifdef WIN32
#define USE_BREAKPAD
#endif

#ifdef USE_BREAKPAD
bool UploadDump(const wchar_t* dump_path,
                     const wchar_t* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded);

// Initialise the crash dumper.
class BreakPadWrapper
{
public:
	BreakPadWrapper() {
		wchar_t* tempPath;
#ifdef WIN32
		int pathLen = GetTempPathW(0, NULL);
		tempPath = new wchar_t[pathLen];
		GetTempPathW(pathLen, tempPath);
#else
		tempPath = "/tmp/";
#endif
		
		crash_handler = new google_breakpad::ExceptionHandler(tempPath,
				NULL,
				UploadDump,
				NULL,
#ifdef WIN32
				google_breakpad::ExceptionHandler::HANDLER_ALL
#else
				true
#endif
				);
		crash_sender = new google_breakpad::CrashReportSender(L"");
	}
	
#ifdef WIN32
	static google_breakpad::CrashReportSender* crash_sender;
#endif
private:
	static google_breakpad::ExceptionHandler* crash_handler;

};

google_breakpad::ExceptionHandler* BreakPadWrapper::crash_handler = NULL;
google_breakpad::CrashReportSender* BreakPadWrapper::crash_sender = NULL;

bool UploadDump(const wchar_t* dump_path,
                     const wchar_t* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded) 
{
#ifdef WIN32
    char crashMsg[512];
    sprintf(crashMsg, "Something unexpected happened in PlaneShift!\nA crash report containing only information strictly necessary to resolve this crash in the future will automatically be sent to the developers.");
    ::MessageBoxA( NULL, crashMsg, "PlaneShift", MB_OK + MB_ICONERROR );
#endif
	std::map<std::wstring, std::wstring> parameters;
    // Add the date of compiled file to check version
    struct __stat64 buf;
    int result;
    wchar_t paramBuffer[512];

#ifdef WIN32
    /* Get data associated with "psclient.exe": */
    result = _stat64( "psclient.exe", &buf );
    if(result == 0)
    {
        wchar_t* time = _wctime64(&buf.st_mtime);
        parameters[L"exe_time"] = time;
    }
#endif
    
    if(
        psengine && 
        psengine->GetCelClient() && 
        psengine->GetCelClient()->GetMainPlayer() && 
        psengine->GetCelClient()->GetMainPlayer()->GetName()
        )
    {
    	mbstowcs(paramBuffer, psengine->GetCelClient()->GetMainPlayer()->GetName(), 511);
    	parameters[L"player_name"] = paramBuffer;
    }
    mbstowcs(paramBuffer, CS_PLATFORM_NAME, 511);
    parameters[L"platform"] = paramBuffer;
    mbstowcs(paramBuffer, CS_PROCESSOR_NAME, 511);
    parameters[L"processor"] = paramBuffer;
    mbstowcs(paramBuffer, CS_COMPILER_NAME, 511);
    parameters[L"compiler"] = paramBuffer;
    mbstowcs(paramBuffer, hwRenderer, 511);
    parameters[L"renderer"] = paramBuffer;
    mbstowcs(paramBuffer, hwVersion, 511);
    parameters[L"hw_version"] = paramBuffer;

	std::wstring report_code;
	printf("Attempting to upload crash report.");
#ifdef WIN32
	BreakPadWrapper::crash_sender->SendCrashReport(crash_post_url,
			parameters,
			dump_path,
			&report_code);
#endif
	if(report_code.empty())
	{
		printf("Upload successful.");
		return succeeded;
	}
	else
		return false;
}

// At global scope to ensure we hook in as early as possible.
BreakPadWrapper wrapper;
#endif // USE_BREAKPAD
