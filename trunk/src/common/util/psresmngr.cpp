#ifndef __PSRESMNGR_CPP__
#define __PSRESMNGR_CPP__

#include <psconfig.h>

#include "psresmngr.h"
#include "util/consoleout.h"

psTemplateResMngr::psTemplateResMngr()
{
    p_resources=new psTemplateResourceHash;
}

psTemplateResMngr::~psTemplateResMngr()
{
    if (p_resources)
    {
        psTemplateResourceHash::GlobalIterator i = p_resources->GetIterator();
        bool head=false;

        psTemplateRes* res;
        while ( i.HasNext() )
        {
            res = (psTemplateRes*) i.Next();
        if (res->GetRefCount() != 1)
        {
            if (!head) {
            CPrintf (CON_ERROR, "***ERROR Resource Manager goes, but the following"
            "resources haven't been freed:\n");
            head=true;
            }
                CPrintf (CON_ERROR, "'%s'\n", res->GetName());
        }
        res->DecRef ();
        }
        delete p_resources;
    }
}

csRef<psTemplateRes> psTemplateResMngr::CreateResource(const char* name)
{
    if (!p_resources)
        return NULL;

    // already a resource with that name there?
    psTemplateResourceHash::Iterator i = p_resources->GetIterator (name);
    psTemplateRes* res;
    while ( i.HasNext() )
    {
        res = (psTemplateRes*) i.Next();
    if (!strcmp(res->GetName(), name))
        return res;
    }

    // we have to create it...
    csRef <psTemplateRes> newres = LoadResource(name);
    if (!newres)
    {
    CPrintf (CON_ERROR, "Couldn't create Resource '%s'\n", name);
    return NULL;
    }

    newres->Init(this, name);
    newres->IncRef();
    p_resources->Put(name, newres);

    return newres;
}

void psTemplateResMngr::UnregisterResource (psTemplateRes* )
{
    // just do nothing at the moment
}

/*  Since it's not safe to perform a delete while an iterator exists
 *  we create a new hash, copy the resources that stay into it,
 *  decref the resources that dont.  Destroy the old hashmap and keep
 *  the new one.
 *
 */

void psTemplateResMngr::Clean()
{
    if (p_resources)
    {
        psTemplateResourceHash::GlobalIterator i = p_resources->GetIterator ();
        psTemplateResourceHash *p_newresources=new psTemplateResourceHash;

        psTemplateRes* res;
        while ( i.HasNext() )
        {
            res = (psTemplateRes*) i.Next();
        if (res->GetRefCount() != 1)
        {
                // Add to the new hash map
                p_newresources->Put(res->GetName(), res);
        }
            else
            {
                // Destroy this resource
                res->DecRef ();
            }
        }
        delete p_resources;
        p_resources=p_newresources;
    }
}

#endif

