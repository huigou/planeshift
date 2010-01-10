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

#ifdef WIN32
// Only support breakpad for win32 for now.
#define USE_BREAKPAD
typedef wchar_t PS_CHAR;
#define PS_PATH_MAX 4096 // Real limit is 32k but that won't fit on the stack.
#define PS_PATH_SEP L"\\"
#define PS_STRNCAT wcsncat
#define PS_STRCAT wcscat
#define PS_STRLEN wcslen
const PS_CHAR* crash_post_url = L"http://planeshift.ezpcusa.com/crash_reporting/submit";
#define DUMP_EXTENSION L".dmp"
#else
typedef char PS_CHAR;
#define PS_PATH_MAX PATH_MAX
#define PS_PATH_SEP "/"
#define PS_STRNCAT strncat
#define PS_STRCAT strcat
#define PS_STRLEN strlen
const PS_CHAR* crash_post_url = "http://planeshift.ezpcusa.com/crash_reporting/submit";
#define DUMP_EXTENSION ".dmp"
#endif

#ifdef CS_DEBUG
#undef USE_BREAKPAD
#endif

#ifdef USE_BREAKPAD
using namespace google_breakpad;

bool UploadDump(const PS_CHAR* dump_path,
                     const PS_CHAR* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded);

// Initialise the crash dumper.
class BreakPadWrapper
{
public:
	BreakPadWrapper() {
		PS_CHAR* tempPath;
#ifdef WIN32
		int pathLen = GetTempPathW(0, NULL);
		tempPath = new PS_CHAR[pathLen];
		GetTempPathW(pathLen, tempPath);
#else
		tempPath = "/tmp/";
#endif
		
		crash_handler = new ExceptionHandler(tempPath,
				NULL,
				UploadDump,
				NULL,
#ifdef WIN32
				ExceptionHandler::HANDLER_ALL
#else
				true
#endif
				);
		crash_sender = new CrashReportSender(L"");
	}
	
#ifdef WIN32
	static CrashReportSender* crash_sender;
#endif
private:
	static ExceptionHandler* crash_handler;

};

ExceptionHandler* BreakPadWrapper::crash_handler = NULL;
CrashReportSender* BreakPadWrapper::crash_sender = NULL;

bool UploadDump(const PS_CHAR* dump_path,
                     const PS_CHAR* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded) 
{
	PS_CHAR path_file[PS_PATH_MAX];
	PS_CHAR* p_path_end = path_file + PS_PATH_MAX;
	PS_CHAR* p_path = path_file;

	PS_STRNCAT(path_file, dump_path, PS_PATH_MAX);
	p_path += PS_STRLEN(path_file);
	PS_STRCAT(path_file, PS_PATH_SEP);
	p_path += PS_STRLEN(PS_PATH_SEP);
	PS_STRNCAT(path_file, minidump_id, p_path_end - p_path);
	p_path += PS_STRLEN(minidump_id);
	PS_STRNCAT(path_file, DUMP_EXTENSION, p_path_end - p_path);


#ifdef WIN32
    char crashMsg[512];
    sprintf(crashMsg, "Something unexpected happened in PlaneShift!\nA crash report containing only information strictly necessary to resolve this crash in the future will automatically be sent to the developers.");
    ::MessageBoxA( NULL, crashMsg, "PlaneShift", MB_OK + MB_ICONERROR );
#endif

    // Add the date of compiled file to check version
    struct __stat64 buf;
    int result;
    PS_CHAR paramBuffer[512];

#ifdef WIN32
	std::map<std::wstring, std::wstring> parameters;
    /* Get data associated with "psclient.exe": */
    result = _stat64( "psclient.exe", &buf );
    if(result == 0)
    {
        PS_CHAR* time = _wctime64(&buf.st_mtime);
        parameters[L"exe_time"] = time;
    }
    
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
#endif

	std::wstring report_code;
	printf("Attempting to upload crash report.\n");

	ReportResult reportResult = RESULT_FAILED;
#ifdef WIN32
	reportResult = BreakPadWrapper::crash_sender->SendCrashReport(crash_post_url,
			parameters,
			path_file,
			&report_code);
#endif
	if(reportResult == RESULT_SUCCEEDED && !report_code.empty())
	{
		printf("Upload successful.");
#ifdef WIN32
		if(!report_code.empty())
			::MessageBoxW( NULL, report_code.c_str(), L"Report upload response", MB_OK );
		if(succeeded)
			::MessageBoxA( NULL, "Report uploaded successfully. Thanks for your help.", "PlaneShift", MB_OK );
#endif
		return succeeded;
	}
	else if(reportResult == RESULT_FAILED)
	{
		printf("Report upload failed: Could not reach server.");
#ifdef WIN32
		::MessageBoxA( NULL, "Report upload failed: Could not reach server.", "PlaneShift", MB_OK + MB_ICONERROR );
#endif
		return false;
	}
	else
	{
		printf("Report upload failed: Unknown reason.");
#ifdef WIN32
		::MessageBoxA( NULL, "Report upload failed: Unknown reason.", "PlaneShift", MB_OK + MB_ICONERROR );
#endif
		return false;
	}
}

// At global scope to ensure we hook in as early as possible.
BreakPadWrapper wrapper;
#endif // USE_BREAKPAD

