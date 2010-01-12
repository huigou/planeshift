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

#include <psconfig.h>

// Only support breakpad for win32 for now.
#if defined(WIN32) && !defined(CS_DEBUG)
#define USE_BREAKPAD
#endif

#ifdef USE_BREAKPAD
#ifdef WIN32
#include "client/windows/handler/exception_handler.h"
#include "client/windows/sender/crash_report_sender.h"
#elif !defined(CS_PLATFORM_MACOSX) && defined(CS_PLATFORM_UNIX) // Mac uses a separate lib
#include "client/linux/handler/exception_handler.h"
#include "common/linux/libcurl_wrapper.h"
#endif

#include <map>
#include <string>
#include "globals.h"
#include "psengine.h"
#include "pscelclient.h"

#ifdef WIN32
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
#ifdef WIN32
		crash_sender = new CrashReportSender(L"");
#else
		http_layer = new LibcurlWrapper();
#endif
		// Set up parameters
	    /* Get data associated with "psclient.exe": */
#ifdef WIN32
	    result = _stat64( "psclient.exe", &buf );
#else
	    result = stat64( "psclient", &buf );
#endif
	    if(result == 0)
	    {
	        PS_CHAR* time;
#ifdef WIN32
	        time = _wctime64(&buf.st_mtime);
	        parameters[L"exe_time"] = time;
#else
	        time = ctime64(&buf.st_mtime);
	        parameters["exe_time"] = time;
#endif
	    }
	    // Reserve space for player name, gfx card info etc because we can't use the heap
	    // when handling the crash.
#ifdef WIN32
		parameters[L"player_name"] = "";
		parameters[L"player_name"].reserve(256);
		mbstowcs(paramBuffer, CS_PLATFORM_NAME, 511);
		parameters[L"platform"] = paramBuffer;
		mbstowcs(paramBuffer, CS_PROCESSOR_NAME, 511);
		parameters[L"processor"] = paramBuffer;
		mbstowcs(paramBuffer, CS_COMPILER_NAME, 511);
		parameters[L"compiler"] = paramBuffer;
		parameters[L"renderer"] = "";
		parameters[L"renderer"].reserve(256);
		parameters[L"hw_version"] = "";
		parameters[L"hw_version"].reserve(256);
		parameters[L"ProductName"] = L"PlaneShift";
		parameters[L"Version"] = L"0.5";
#else
		parameters["player_name"] = "";
		parameters["player_name"].reserve(256);
		parameters["platform"] = CS_PLATFORM_NAME;
		parameters["processor"] = CS_PROCESSOR_NAME;
		parameters["compiler"] = CS_COMPILER_NAME;
		parameters["renderer"] = "";
		parameters["renderer"].reserve(256);
		parameters["hw_version"] = "";
		parameters["hw_version"].reserve(256);
		parameters["ProductName"] = "PlaneShift";
		parameters["Version"] = "0.5";
#endif
		report_code.reserve(512);

	}
	~BreakPadWrapper() {
		delete crash_handler;
		crash_handler = NULL;
		delete crash_sender;
		crash_sender = NULL;
	}
	
#ifdef WIN32
	static CrashReportSender* crash_sender;
#else
	static LibcurlWrapper* http_layer;
#endif
#ifdef WIN32
	std::map<std::wstring, std::wstring> parameters;
	std::wstring report_code;
#else
	std::map<std::string, std::string> parameters;
	std::string report_code;
#endif // WIN32
	
private:
	static ExceptionHandler* crash_handler;

};

ExceptionHandler* BreakPadWrapper::crash_handler = NULL;
CrashReportSender* BreakPadWrapper::crash_sender = NULL;

// At global scope to ensure we hook in as early as possible.
BreakPadWrapper wrapper;

// This function should not modify the heap!
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
	
    if(
        psengine && 
        psengine->GetCelClient() && 
        psengine->GetCelClient()->GetMainPlayer() && 
        psengine->GetCelClient()->GetMainPlayer()->GetName()
        )
    {
#ifdef WIN32
    	mbstowcs(paramBuffer, psengine->GetCelClient()->GetMainPlayer()->GetName(), 511);
    	wrapper.parameters[L"player_name"] = paramBuffer;
#else
    	wrapper.parameters["player_name"] = paramBuffer;
#endif
    }
#ifdef WIN32
    mbstowcs(paramBuffer, hwRenderer, 511);
    wrapper.parameters[L"renderer"] = paramBuffer;
    mbstowcs(paramBuffer, hwVersion, 511);
    wrapper.parameters[L"hw_version"] = paramBuffer;
#else
    wrapper.parameters["renderer"] = hwRenderer;
    wrapper.parameters["hw_version"] = hwVersion;
#endif

	
	printf("Attempting to upload crash report.\n");

	ReportResult reportResult = RESULT_FAILED;
	
#ifdef WIN32
	reportResult = BreakPadWrapper::crash_sender->SendCrashReport(crash_post_url,
			wrapper.parameters,
			path_file,
			&report_code);
#elif defined(CS_PLATFORM_UNIX)
	// Don't use GoogleCrashdumpUploader as it doesn't allow custom parameters.
	if (!wrapper.http_layer->AddFile(path_file,
								"upload_file_minidump")) {
		bool result = http_layer_->SendRequest(crash_post_url,
										  wrapper.parameters,
										  &report_code);
		if (result)
			reportResult = RESULT_SUCCEEDED;
		else
			reportResult = RESULT_FAILED;
	}
	else
		reportResult = RESULT_FAILED;
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


#endif // USE_BREAKPAD

