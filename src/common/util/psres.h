#ifndef __PSRES_H__
#define __PSRES_H__

#include <csutil/scf.h>
#include <csutil/csstring.h>

class psTemplateResMngr;

class psTemplateRes
{
public:
    psTemplateRes();
    virtual ~psTemplateRes();

    void Init (psTemplateResMngr* mngr, const char* name);

    virtual void IncRef();
    virtual void DecRef();

    virtual void AddRefOwner( void** ref_owner ) {}
    virtual void RemoveRefOwner( void** ref_owner ) {}

    virtual int GetRefCount() { return refcount; }
    virtual void* QueryInterface(scfInterfaceID, int) { return NULL; }    

    const char* GetName() { return (const char*) name; }
    
protected:
    psTemplateResMngr* mngr;
    int refcount;
    csString name;
};

#endif

