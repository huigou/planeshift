//
// Database Abstraction Layer
// Keith Fulton <keith@paqrat.com>
// 02/14/02
//

#ifndef __DAL_H__
#define __DAL_H__

#include "iserver/idal.h"

#include <csutil/scf.h>
#include <csutil/threading/thread.h>

#include "iutil/comp.h"

#include "net/netbase.h"  // Make sure that winsock is included.

#include <mysql.h>

#include <csutil/csstring.h>
#include "util/stringarray.h"
#include "util/dbprofile.h"

using namespace CS::Threading;

struct iObjectRegistry;

class DelayedQueryManager : public CS::Threading::Runnable
{
public:
    DelayedQueryManager(const char *host, unsigned int port, const char *database,
                              const char *user, const char *pwd);

    virtual void Run ();
    void Push(csString query);
    void Stop();
private:
    MYSQL* m_conn;
    csArray<csString> arr;
    size_t start, end;
    CS::Threading::RecursiveMutex mutex;
    CS::Threading::RecursiveMutex mutexArray;
    CS::Threading::Condition datacondition;
    bool m_Close;
};

class psMysqlConnection : public iComponent, public iDataConnection
{
protected:
    MYSQL mydb; // Mysql connection
    MYSQL *conn; // Points to mydb after a successfull connection to the db 
    csString lastquery;
    iObjectRegistry *objectReg;
    psDBProfiles profs;
    csString profileDump;

public:

    SCF_DECLARE_IBASE;

    psMysqlConnection(iBase *iParent);
    virtual ~psMysqlConnection();

    bool Initialize (iObjectRegistry *objectreg);
    bool Initialize(const char *host, unsigned int port, const char *database, 
                    const char *user, const char *pwd);
    virtual bool Close();

    int IsValid(void);

    /** Escapes a string to safely insert it into the database.
     *  @param to Where the resulting escaped string will be placed.
     *  @param from The string that we want to escape.
     */
    void Escape(csString& to, const char *from);
    
    iResultSet *Select(const char *sql,...);
    int SelectSingleNumber(const char *sql, ...);
    unsigned long Command(const char *sql,...);
    unsigned long CommandPump(const char *sql,...);

    uint64 GenericInsertWithID(const char *table,const char **fieldnames,psStringArray& fieldvalues);
    bool GenericUpdateWithID(const char *table,const char *idfield,const char *id,const char **fieldnames,psStringArray& fieldvalues);

    const char *GetLastError(void);
    const char *GetLastQuery(void)
    {
        return lastquery;
    };
    uint64 GetLastInsertID();
    
    const char *uint64tostring(uint64 value,csString& recv);

    virtual const char* DumpProfile();
    virtual void ResetProfile();

    csRef<DelayedQueryManager> dqm;
    csRef<Thread> dqmThread;
};


class psResultRow : public iResultRow
{
protected:
    MYSQL_ROW rr;
    MYSQL_RES *rs;
    int max;
    int last_index;

public:
    psResultRow()
    {
        rr = NULL;
        rs = NULL;
    last_index=0;
    };

    void SetMaxFields(int fields)  {    max = fields;   };
    void SetResultSet(long resultsettoken);

    int Fetch(int row);

    const char *operator[](int whichfield);
    const char *operator[](const char *fieldname);

    int GetInt(int whichfield);
    int GetInt(const char *fieldname);

    unsigned long GetUInt32(int whichfield);
    unsigned long GetUInt32(const char *fieldname);

    float GetFloat(int whichfield);
    float GetFloat(const char *fieldname);

    uint64 GetUInt64(int whichfield);
    uint64 GetUInt64(const char *fieldname);

    uint64 stringtouint64(const char *stringbuf);
    const char *uint64tostring(uint64 value,char *stringbuf,int buflen);
};

class psResultSet : public iResultSet
{
protected:
    MYSQL_RES *rs;
    unsigned long rows, fields, current;
    psResultRow  row;

public:
    psResultSet(MYSQL *conn);
    virtual ~psResultSet();

    void Release(void) { delete this; };

    iResultRow& operator[](unsigned long whichrow);

    unsigned long Count(void) { return rows; };
};

#endif

