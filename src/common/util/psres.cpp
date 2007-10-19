#ifndef __PSRES_CPP__
#define __PSRES_CPP__

#include <psconfig.h>

#include "psres.h"
#include "psresmngr.h"

psTemplateRes::psTemplateRes()
{
}

void psTemplateRes::Init (psTemplateResMngr* nmngr, const char* nname)
{
    mngr = nmngr;
    refcount = 1;
    name = nname;
}

psTemplateRes::~psTemplateRes()
{
}

void psTemplateRes::IncRef()
{
    refcount++;
}

void psTemplateRes::DecRef()
{
    CS_ASSERT(refcount>=0);
    refcount--;
    if (refcount<=1)
    {
    if (refcount==0)
    {
      delete this;
    }
    else
      mngr->UnregisterResource(this);
    }
}

#endif

