/*
 * mdump.h
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

#ifndef MDUMP_H
#define MDUMP_H


/* Planeshift enums for minidump types.
 *  Redefined and translated so the caller doesn't have to find/include the real dbghelp.h header
 */
typedef enum _PS_MINIDUMP_TYPE {
    PSMiniDumpNormal         = 0x0000,
    PSMiniDumpWithDataSegs   = 0x0001,
    PSMiniDumpWithFullMemory = 0x0002,
    PSMiniDumpWithHandleData = 0x0004,
    PSMiniDumpFilterMemory   = 0x0008,
    PSMiniDumpScanMemory     = 0x0010
} PS_MINIDUMP_TYPE;

typedef enum {
    PSCrashActionOff = 0,
    PSCrashActionPrompt,
    PSCrashActionAlways,
    PSCrashActionIgnore
} PS_CRASHACTION_TYPE;


class MiniDumper
{
private:
  static int DumpType; // really a MINIDUMP_TYPE enum, but stored as an int
  static PS_CRASHACTION_TYPE CrashAction;

  static LONG WINAPI TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo );

public:
  MiniDumper();

  void SetDumpType(PS_MINIDUMP_TYPE type);
  PS_MINIDUMP_TYPE GetDumpType();

  void SetCrashAction(PS_CRASHACTION_TYPE action);
  PS_CRASHACTION_TYPE GetCrashAction();

  const char *GetDumpTypeString();
};


#endif // #ifndef MDUMP_H

