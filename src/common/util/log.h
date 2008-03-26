/*
* pslog.h -- Christophe Painchaud aka Atanor, DaSH <dash@ionblast.net>
*
* Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __PSUTIL_LOG_H__
#define __PSUTIL_LOG_H__

#include "util/singleton.h"
#include "ivaria/reporter.h"
#include <iutil/vfs.h>

struct iConfigManager;
struct iObjectRegistry;

enum LOG_TYPES
{
    LOG_ANY,
    LOG_WEATHER,
    LOG_SPAWN,
    LOG_CELPERSIST,
    LOG_PAWS,
    LOG_GROUP,
    LOG_CHEAT,
    LOG_LINMOVE,
    LOG_SPELLS,
    LOG_NEWCHAR,
    LOG_SUPERCLIENT,
    LOG_EXCHANGES,
    LOG_ADMIN,
    LOG_STARTUP,
    LOG_CHARACTER,
    LOG_CONNECTIONS,
    LOG_CHAT,
    LOG_NET,
    LOG_LOAD,
    LOG_NPC,
    LOG_TRADE,
    LOG_SOUND,
    LOG_COMBAT,
    LOG_SKILLXP,
    LOG_QUESTS,
    LOG_SCRIPT,
    LOG_MARRIAGE,
    LOG_MESSAGES,
    LOG_CACHE,
    LOG_PETS,
    LOG_USER,
    LOG_LOOT,
    LOG_DUELS,
    LOG_TRIBES,
    MAX_FLAGS
// NOTE: Remember to update the flagnames table in log.cpp when addning new entries
};

enum
{
    CSV_AUTHENT,
    CSV_EXCHANGES,
    CSV_PALADIN,
    CSV_STATUS,
    CSV_ADVICE,
    CSV_ECONOMY,
    CSV_STUCK,
    CSV_SQL,
    MAX_CSV
};

namespace pslog
{

extern iObjectRegistry* logger;
extern bool disp_flag[MAX_FLAGS];
 
bool DoLog(int severity, LOG_TYPES type, uint32 filter_id);
void LogMessage (const char* file, int line, const char* function,
             int severity, LOG_TYPES type, uint32 filter_id, const char* msg, ...) CS_GNUC_PRINTF (7, 8);
void Initialize(iObjectRegistry* object_reg);
void SetFlag(const char *name,bool flag, uint32 filter);
void DisplayFlags(const char *name=NULL);
bool GetValue(const char* name);
const char* GetName(int id);
const char* GetSettingName(int id);


// Check log macros
//
// Use this to prevent processing of preparation of args to the other macros:
// 
// if (DoLogDebug(LOG_NET))
// {
//    int arg = <Work that use much CPU> 
//    Debug1(LOG_NET,...,arg,...)
// } 
// 
#define DoLogDebug(type)              pslog::DoLog( CS_REPORTER_SEVERITY_DEBUG, type, 0)
#define DoLogDebug2(type,filter_id)   pslog::DoLog( CS_REPORTER_SEVERITY_DEBUG, type, filter_id) 
#define DoLogNotify(type)             pslog::DoLog( CS_REPORTER_SEVERITY_NOTIFY, type, 0)
#define DoLogError(type)              pslog::DoLog( CS_REPORTER_SEVERITY_ERROR, type, 0)
#define DoLogWarning(type)            pslog::DoLog( CS_REPORTER_SEVERITY_WARNING, type, 0) 
#define DoLogBug(type)                pslog::DoLog( CS_REPORTER_SEVERITY_BUG, type, 0) 
 
// Debug macros

#define Debug1(type, filter_id, a) \
    { if (DoLogDebug2(type,filter_id)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_DEBUG, type, filter_id, a); }}
#define Debug2(type, filter_id, a,b) \
    { if (DoLogDebug2(type,filter_id)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_DEBUG, type, filter_id, a, b); }}
#define Debug3(type, filter_id, a,b,c) \
    { if (DoLogDebug2(type,filter_id)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_DEBUG, type, filter_id, a, b, c); }}
#define Debug4(type, filter_id, a,b,c,d) \
    { if (DoLogDebug2(type,filter_id)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_DEBUG, type, filter_id, a, b, c, d); }}
#define Debug5(type, filter_id, a,b,c,d,e) \
    { if (DoLogDebug2(type,filter_id)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_DEBUG, type, filter_id, a, b, c, d, e); }}
#define Debug6(type, filter_id, a,b,c,d,e,f) \
    { if (DoLogDebug2(type,filter_id)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_DEBUG, type, filter_id, a, b, c, d, e, f); }}
#define Debug7(type, filter_id, a,b,c,d,e,f,g) \
    { if (DoLogDebug2(type,filter_id)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_DEBUG, type, filter_id, a, b, c, d, e, f, g); }}
#define Debug8(type, filter_id, a,b,c,d,e,f,g,h) \
    { if (DoLogDebug2(type,filter_id)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_DEBUG, type, filter_id, a, b, c, d, e, f, g, h); }}

#define Notify1(type, a) \
    { if (DoLogNotify(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_NOTIFY, type, 0, a); }}
#define Notify2(type, a,b) \
    { if (DoLogNotify(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_NOTIFY, type, 0, a, b); }}
#define Notify3(type, a,b,c) \
    { if (DoLogNotify(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_NOTIFY, type, 0, a, b, c); }}
#define Notify4(type, a,b,c,d) \
    { if (DoLogNotify(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_NOTIFY, type, 0, a, b, c, d); }}
#define Notify5(type, a,b,c,d,e) \
    { if (DoLogNotify(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_NOTIFY, type, 0, a, b, c, d, e); }}
#define Notify6(type, a,b,c,d,e,f) \
    { if (DoLogNotify(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_NOTIFY, type, 0, a, b, c, d, e, f); }}

#define Warning1(type, a) \
    { if (DoLogWarning(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_WARNING, type, 0, a); }}
#define Warning2(type, a,b) \
    { if (DoLogWarning(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_WARNING, type, 0, a, b); }}
#define Warning3(type, a,b,c) \
    { if (DoLogWarning(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_WARNING, type, 0, a, b, c); }}
#define Warning4(type, a,b,c,d) \
    { if (DoLogWarning(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_WARNING, type, 0, a, b, c, d); }}
#define Warning5(type, a,b,c,d,e) \
    { if (DoLogWarning(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_WARNING, type, 0, a, b, c, d, e); }}
#define Warning6(type, a,b,c,d,e,f) \
    { if (DoLogWarning(type)){ \
        pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_WARNING, type, 0, a, b, c, d, e, f); }}

#define Error1(a) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_ERROR, LOG_ANY, 0, a); }
#define Error2(a,b) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_ERROR, LOG_ANY, 0, a, b); }
#define Error3(a,b,c) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_ERROR, LOG_ANY, 0, a, b, c); }
#define Error4(a,b,c,d) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_ERROR, LOG_ANY, 0, a, b, c, d); }
#define Error5(a,b,c,d,e) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_ERROR, LOG_ANY, 0, a, b, c, d, e); }
#define Error6(a,b,c,d,e,f) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_ERROR, LOG_ANY, 0, a, b, c, d, e, f); }

#define Bug1(a) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_BUG, LOG_ANY, 0, a); }
#define Bug2(a,b) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_BUG, LOG_ANY, 0, a, b); }
#define Bug3(a,b,c) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_BUG, LOG_ANY, 0, a, b, c); }
#define Bug4(a,b,c,d) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_BUG, LOG_ANY, 0, a, b, c, d); }
#define Bug5(a,b,c,d,e) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_BUG, LOG_ANY, 0, a, b, c, d, e); }
#define Bug6(a,b,c,d,e,f) \
    { pslog::LogMessage (__FILE__, __LINE__, __FUNCTION__, CS_REPORTER_SEVERITY_BUG, LOG_ANY, 0, a, b, c, d, e, f); }

} // end of namespace pslog

// Used to create Comma Seperated Value files for general logging
// and takes advantage of ConfigManager and VFS which pslog cannot.
// This should be used only for day-to-day information needed in a
// consistent, readable format. Warnings and errors should go through pslog.
class LogCSV : public Singleton<LogCSV>
{
    csRef<iFile> csvFile[MAX_CSV];
    void StartLog(const char* logfile, iVFS* vfs, const char* header, size_t maxSize, csRef<iFile>& csvFile);

public:
    LogCSV(iConfigManager* configmanager, iVFS* vfs);
    void Write(int type, csString& text);
};

#endif
