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

#ifdef USE_DELAY_QUERY
#define THREADED_BUFFER_SIZE 300
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
    csString arr[THREADED_BUFFER_SIZE];
    size_t start, end;
    CS::Threading::Mutex mutex;
    CS::Threading::RecursiveMutex mutexArray;
    CS::Threading::Condition datacondition;
    bool m_Close;
    psDBProfiles profs;
    csString m_host;
    unsigned int m_port;
    csString m_db;
    csString m_user;
    csString m_pwd;
};
#endif

class psMysqlConnection : public iComponent, public iDataConnection
{
protected:
    MYSQL mydb; // Mysql connection
    MYSQL *conn; // Points to mydb after a successfull connection to the db 
    csString lastquery;
    iObjectRegistry *objectReg;
    psDBProfiles profs;
    csString profileDump;
    LogCSV* logcsv;

public:

    SCF_DECLARE_IBASE;

    psMysqlConnection(iBase *iParent);
    virtual ~psMysqlConnection();

    bool Initialize (iObjectRegistry *objectreg);
    bool Initialize(const char *host, unsigned int port, const char *database, 
                    const char *user, const char *pwd, LogCSV* logcsv);
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
    bool GenericUpdateWithID(const char *table,const char *idfield,const char *id,psStringArray& fields);

    const char *GetLastError(void);
    const char *GetLastQuery(void)
    {
        return lastquery;
    };
    uint64 GetLastInsertID();
    
    const char *uint64tostring(uint64 value,csString& recv);

    virtual const char* DumpProfile();
    virtual void ResetProfile();
    
    iRecord* NewPreparedStatement(const char* table, const char* idfield, unsigned int count);

#ifdef USE_DELAY_QUERY    
    csRef<DelayedQueryManager> dqm;
    csRef<Thread> dqmThread;
#endif    
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
    void SetResultSet(void* resultsettoken);

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

class dbRecord : public iRecord
{
    const char* table;
    const char* idfield;
    
    MYSQL* conn;
    
    psStringArray command;
    bool prepared;
    
    MYSQL_BIND* bind;
    MYSQL_STMT* stmt;
    
    unsigned int index;
    unsigned int count;
    
    // This is a holding structure to ensure the data is available as long as the binding needs it.
    typedef struct{
        int iValue;
        // On *nix and Windows, uint32 is equivalent to unsigned int
        // this is important for SQL
        unsigned int uiValue;
        unsigned short usValue;
        float fValue;
        csString string;
        unsigned long length;
    } dataType;
    
    dataType* temp;
    
    void AddToStatement(const char* fname)
    {
        if(!prepared)
            command.FormatPush("%s = ?", fname);
    }
    
public:
    dbRecord(MYSQL* db, const char* Table, const char* Idfield, unsigned int count)
    {
        conn = db;
        table = Table;
        idfield = Idfield;
        bind = new MYSQL_BIND[count];
        memset(bind, 0, sizeof(MYSQL_BIND) * count);
        temp = new dataType[count];
        index = 0;
        this->count = count;
        
        stmt = (MYSQL_STMT*) mysql_stmt_init(conn);
        prepared = false;
    }
    
    ~dbRecord()
    {
        mysql_stmt_close(stmt);
        delete[] bind;
        delete[] temp;
    }
    
    void Reset()
    {
        index = 0;
        memset(bind, 0, sizeof(MYSQL_BIND) * count);
    }

    
    void AddField(const char* fname, float fValue)
    {
        //command.FormatPush("%s='%1.2f'", fname, fValue);
        AddToStatement(fname);
        temp[index].fValue = fValue;
        bind[index].buffer_type = MYSQL_TYPE_FLOAT;
        bind[index].buffer = (void *)&(temp[index].fValue);
        index++;
    }
    
    void AddField(const char* fname, int iValue)
    {
        //command.FormatPush("%s='%u'", fname, iValue);
        AddToStatement(fname);
        temp[index].iValue = iValue;
        bind[index].buffer_type = MYSQL_TYPE_LONG;
        bind[index].buffer = (void *)&(temp[index].iValue);
        index++;
    }
    
    void AddField(const char* fname, unsigned int uiValue)
    {
        //command.FormatPush("%s='%u'", fname, iValue);
        AddToStatement(fname);
        temp[index].uiValue = uiValue;
        bind[index].buffer_type = MYSQL_TYPE_LONG;
        bind[index].is_unsigned = true;
        bind[index].buffer = (void *)&(temp[index].uiValue);
        index++;
    }
    
    void AddField(const char* fname, unsigned short usValue)
    {
        //csString escape;
        //db->Escape(escape, sValue);
        //command.FormatPush("%s='%s'", fname, escape.GetData());
        AddToStatement(fname);
        temp[index].usValue = usValue;
        bind[index].buffer_type = MYSQL_TYPE_SHORT;
        bind[index].is_unsigned = true;
        bind[index].buffer = &(temp[index].usValue);
        index++;
    }

    void AddField(const char* fname, const char* sValue)
    {
        //csString escape;
        //db->Escape(escape, sValue);
        //command.FormatPush("%s='%s'", fname, escape.GetData());
        AddToStatement(fname);
        temp[index].string = sValue;
        temp[index].length = temp[index].string.Length();
        
        bind[index].buffer_type = MYSQL_TYPE_STRING;
        bind[index].buffer = const_cast<char *> (temp[index].string.GetData());
       
        bind[index].buffer_length = temp[index].length;
        bind[index].length = &(temp[index].length);
        index++;
    }
    
    bool Prepare()
    {
        csString statement;
        
        // count - 1 fields to update
        statement.Format("UPDATE %s SET ", table);
        for (unsigned int i=0;i<(count-1);i++)
        {
            if (i>0)
                statement.Append(",");
            statement.Append(command[i]);
        }
        statement.Append(" where ");
        statement.Append(idfield);
        // field count is the idfield
        statement.Append("= ?");
        
        prepared = (mysql_stmt_prepare(stmt, statement, statement.Length()) == 0);
        
        return prepared;
    }
    
    bool Execute(uint32 uid)
    {
        temp[index].uiValue = uid;
        bind[index].buffer_type = MYSQL_TYPE_LONG;
        bind[index].buffer = &(temp[index].uiValue);
        bind[index].is_unsigned = true;
        index++;
        CS_ASSERT(index == count);
        
        if(!prepared)
            Prepare();
        
        CS_ASSERT(count == mysql_stmt_param_count(stmt));
        
        if(mysql_stmt_bind_param(stmt, bind) != 0)
            return false;
        
        return (mysql_stmt_execute(stmt) == 0);
    }
};

#endif

