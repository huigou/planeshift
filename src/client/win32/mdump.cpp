/*
 * mdump.cpp
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// Code to generate a minidump upon exception.

#include <psconfig.h>

//We're using a namespace to protect the rest of the PS code
#include <iostream>
#include <tchar.h>
#include <assert.h>
#include <windows.h>
#include "..\pscelclient.h"
#include "..\globals.h"

#if _MSC_VER < 1300
#define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"            // must be XP version of file
#else
// VC7: ships with updated headers
#include "dbghelp.h"
#endif


// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
                                    );

#include "mdump.h"

int MiniDumper::DumpType;
PS_CRASHACTION_TYPE MiniDumper::CrashAction;


MiniDumper::MiniDumper()
{
    // if this assert fires then you have two instances of MiniDumper
    // which is not allowed
    // Taken out
  
  // Default to a normal dump including only stack, segment, and load information
  DumpType=(int)MiniDumpNormal; 
  // Default to prompting wether to save a dump on crash
  CrashAction=PSCrashActionPrompt;

    ::SetUnhandledExceptionFilter( TopLevelFilter );
}

void MiniDumper::SetDumpType(PS_MINIDUMP_TYPE type)
{
  switch (type)
  {
    case PSMiniDumpWithDataSegs:
      DumpType=(int)MiniDumpWithDataSegs;
      break;
    case PSMiniDumpWithFullMemory:
      DumpType=(int)MiniDumpWithFullMemory;
      break;
    case PSMiniDumpWithHandleData:
      DumpType=(int)MiniDumpWithHandleData;
      break;
    case PSMiniDumpFilterMemory:
      DumpType=(int)MiniDumpFilterMemory;
      break;
    case PSMiniDumpScanMemory:
      DumpType=(int)MiniDumpScanMemory;
      break;
    default:
      DumpType=(int)MiniDumpNormal;
      break;
  }
}

PS_MINIDUMP_TYPE MiniDumper::GetDumpType()
{
  switch ((MINIDUMP_TYPE)DumpType)
  {
    case MiniDumpWithDataSegs:
      return PSMiniDumpWithDataSegs;
    case MiniDumpWithFullMemory:
      return PSMiniDumpWithFullMemory;
    case MiniDumpWithHandleData:
      return PSMiniDumpWithHandleData;
    case MiniDumpFilterMemory:
      return PSMiniDumpFilterMemory;
    case MiniDumpScanMemory:
      return PSMiniDumpScanMemory;
    default:
      return PSMiniDumpNormal;
  }
}

void MiniDumper::SetCrashAction(PS_CRASHACTION_TYPE action)
{
  CrashAction=action;
}

PS_CRASHACTION_TYPE MiniDumper::GetCrashAction()
{
  return CrashAction;
}


const char *MiniDumper::GetDumpTypeString()
{
  static char dumpstring[256];

  switch (CrashAction)
  {
    case PSCrashActionOff:
      strcpy(dumpstring,"Crash dumps are OFF");
      return dumpstring;
    case PSCrashActionAlways:
      strcpy(dumpstring,"Crash dumps will ALWAYS be generated.  ");
      break;
    default:
      strcpy(dumpstring,"You will be PROMPTED before a crash dump is generated.  ");
      break;
  }

  switch ((MINIDUMP_TYPE)DumpType)
  {
    case MiniDumpWithDataSegs:
      strcat(dumpstring,"Format is Detailed (Include data segments associated with modules at load time.  This includes global and static members but NOT the entire heap)");
      break;
    case MiniDumpWithFullMemory:
      strcat(dumpstring,"Format is Full (Include ALL process memory)");
      break;
    case MiniDumpWithHandleData:
      strcat(dumpstring,"Format is NTHandles (Like 'Normal' but include system handle information at the time of the crash - NT/2K/XP only)");
      break;
    case MiniDumpFilterMemory:
      strcat(dumpstring,"Format is Filter (Like 'Normal' but strip the stack and backtrace data down to only what's necessary for the raw function trace)");
      break;
    case MiniDumpScanMemory:
      strcat(dumpstring,"Format is Scan (Like 'Normal' but scan stack and backtrace data for module references and mark accordingly)");
      break;
    default:
      strcat(dumpstring,"Format is Normal (Stack and Backtrace information only)");
      break;
  }
  return dumpstring;
}


LONG MiniDumper::TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
    // See if a crash dump should be generated at all
    if (CrashAction == PSCrashActionOff)
        return EXCEPTION_CONTINUE_SEARCH;
    else if (CrashAction == PSCrashActionIgnore)
        return EXCEPTION_EXECUTE_HANDLER;

    LONG retval = EXCEPTION_CONTINUE_SEARCH;

    // firstly see if dbghelp.dll is around and has the function we need
    // look next to the EXE first, as the one in System32 might be old 
    // (e.g. Windows 2000)
    HMODULE hDll = NULL;
    char szDbgHelpPath[_MAX_PATH];

    if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
    {
        char *pSlash = _tcsrchr( szDbgHelpPath, '\\' );
        if (pSlash)
        {
            _tcscpy( pSlash+1, "DBGHELP.DLL" );
            hDll = ::LoadLibrary( szDbgHelpPath );
        }
    }

    if (hDll==NULL)
    {
        // load any version we can
        hDll = ::LoadLibrary( "DBGHELP.DLL" );
    }

    LPCTSTR szResult = NULL;

    if (hDll)
    {
    char exceptionNum[30];
        MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
        if (pDump)
        {
      const char* exceptionType;
      switch(pExceptionInfo->ExceptionRecord->ExceptionCode)
      {
      case EXCEPTION_ACCESS_VIOLATION:
        exceptionType = "EXCEPTION_ACCESS_VIOLATION";
        break;
      case EXCEPTION_DATATYPE_MISALIGNMENT:
        exceptionType = "EXCEPTION_DATATYPE_MISALIGNMENT";
        break;
      case EXCEPTION_BREAKPOINT:
        exceptionType = "EXCEPTION_BREAKPOINT";
        break;
      case EXCEPTION_SINGLE_STEP:
        exceptionType = "EXCEPTION_SINGLE_STEP";
        break;
      case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        exceptionType = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        break;
      case EXCEPTION_FLT_DENORMAL_OPERAND:
        exceptionType = "EXCEPTION_FLT_DENORMAL_OPERAND";
        break;
      case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        exceptionType = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        break;
      case EXCEPTION_FLT_INEXACT_RESULT:
        exceptionType = "EXCEPTION_FLT_INEXACT_RESULT";
        break;
      case EXCEPTION_FLT_INVALID_OPERATION:
        exceptionType = "EXCEPTION_FLT_INVALID_OPERATION";
        break;
      case EXCEPTION_FLT_OVERFLOW:
        exceptionType = "EXCEPTION_FLT_OVERFLOW";
        break;
      case EXCEPTION_FLT_STACK_CHECK:
        exceptionType = "EXCEPTION_FLT_STACK_CHECK";
        break;
      case EXCEPTION_FLT_UNDERFLOW:
        exceptionType = "EXCEPTION_FLT_UNDERFLOW";
        break;
      case EXCEPTION_INT_DIVIDE_BY_ZERO:
        exceptionType = "EXCEPTION_INT_DIVIDE_BY_ZERO";
        break;
      case EXCEPTION_INT_OVERFLOW:
        exceptionType = "EXCEPTION_INT_OVERFLOW";
        break;
      case EXCEPTION_PRIV_INSTRUCTION:
        exceptionType = "EXCEPTION_PRIV_INSTRUCTION";
        break;
      case EXCEPTION_IN_PAGE_ERROR:
        exceptionType = "EXCEPTION_IN_PAGE_ERROR";
        break;
      case EXCEPTION_ILLEGAL_INSTRUCTION:
        exceptionType = "EXCEPTION_ILLEGAL_INSTRUCTION";
        break;
      case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        exceptionType = "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        break;
      case EXCEPTION_STACK_OVERFLOW:
        exceptionType = "EXCEPTION_STACK_OVERFLOW";
        break;
      case EXCEPTION_INVALID_DISPOSITION:
        exceptionType = "EXCEPTION_INVALID_DISPOSITION";
        break;
      case EXCEPTION_GUARD_PAGE:
        exceptionType = "EXCEPTION_GUARD_PAGE";
        break;
      case EXCEPTION_INVALID_HANDLE:
        exceptionType = "EXCEPTION_INVALID_HANDLE";
        break;
      default:
        sprintf(exceptionNum, "Unknown code %X", pExceptionInfo->ExceptionRecord->ExceptionCode);
        exceptionType = exceptionNum;
      }



            char crashMsg[512];
            sprintf(crashMsg, "Something unexpected happened in PlaneShift!\nDetails: %s at %p\n\nWould you like to save a diagnostic file?",
                exceptionType, pExceptionInfo->ExceptionRecord->ExceptionAddress);

            // ask the user if they want to save a dump file unless the crash action is to always generate a dump
            if ( CrashAction == PSCrashActionAlways ||
                (::MessageBox( NULL, crashMsg, "PlaneShift", MB_YESNO + MB_ICONERROR )==IDYES))
            {
                char szDumpPath[_MAX_PATH];
                char szScratch [_MAX_PATH];

                // work out a good place for the dump file
                if (!GetModuleFileName( NULL, szDumpPath, _MAX_PATH ))
                {
                    if (!GetTempPath( _MAX_PATH, szDumpPath ))
                        _tcscpy( szDumpPath, "c:\\temp\\" );
                }
                else
                {
                    char *pSlash = _tcsrchr( szDumpPath, '\\' );
                    *(pSlash + 1) = '\0';
                }

                // Format the message
                _tcscat( szDumpPath, "PlaneShift");
                _tcscat( szDumpPath, "_");

                // Has to be failsafe
                if(
                    psengine && 
                    psengine->GetCelClient() && 
                    psengine->GetCelClient()->GetMainPlayer() && 
                    psengine->GetCelClient()->GetMainPlayer()->GetName()
                    )
                {
                    _tcscat( szDumpPath, psengine->GetCelClient()->GetMainPlayer()->GetName());
                    _tcscat( szDumpPath, "_");
                }

                // Add the date of compiled file to check version

                struct __stat64 buf;
                int result;

                /* Get data associated with "psclient.exe": */
                result = _stat64( "psclient.exe", &buf );
                if(result == 0)
                {
                    _tcscat( szDumpPath, "Version_");
                    char* time = _ctime64(&buf.st_mtime);
                    strncat(szDumpPath, time + 4, 6);
                    strncat(szDumpPath, time + 19, 5);
                }
                else
                {
                    //Add the time
                    time_t curr=time(0);
                    tm* localtm = localtime(&curr);

                    sprintf(szScratch,"_%d%02d%02d-%02d%02d",localtm->tm_year+1900,localtm->tm_mon+1,localtm->tm_mday,localtm->tm_hour,localtm->tm_min);
                    _tcscat( szDumpPath, szScratch);
                }
                sprintf(szScratch, "_Addr%p", pExceptionInfo->ExceptionRecord->ExceptionAddress);
                _tcscat(szDumpPath,szScratch);

                // Convert
                _tcscat( szDumpPath, ".dmp" );


                // create the file
                HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL );

                if (hFile!=INVALID_HANDLE_VALUE)
                {
                    _MINIDUMP_EXCEPTION_INFORMATION ExInfo;

                    ExInfo.ThreadId = ::GetCurrentThreadId();
                    ExInfo.ExceptionPointers = pExceptionInfo;
                    ExInfo.ClientPointers = NULL;

                    // write the dump
                    BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, (MINIDUMP_TYPE)DumpType, &ExInfo, NULL, NULL );
                    if (bOK)
                    {
                        sprintf( szScratch, "Saved dump file to '%s'.\nPlease consult the PlaneShift forums for more details.\nIf you have concerns about privacy, see http://watson.microsoft.com/dw/1033/dcp.asp", szDumpPath );
                        szResult = szScratch;
                        retval = EXCEPTION_EXECUTE_HANDLER;
                    }
                    else
                    {
                        sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, GetLastError() );
                        szResult = szScratch;
                    }
                    ::CloseHandle(hFile);
                }
                else
                {
                    sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, GetLastError() );
                    szResult = szScratch;
                }
            }
        }
        else
        {
            szResult = "DBGHELP.DLL too old";
        }
    }
    else
    {
        szResult = "DBGHELP.DLL not found";
    }

    if (szResult)
        ::MessageBox( NULL, szResult, "PlaneShift", MB_OK + MB_ICONINFORMATION );

    return retval;
}
