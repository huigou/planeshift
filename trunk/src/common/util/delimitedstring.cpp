//
// DelimitedString implementation
//
// KWF : 12/29/2000
//

#include <psconfig.h>

#include "delimitedstring.h"



void psDelimitedString::FindParm(int iWhich, int& iStart, int& iStop)
{
#ifdef DEBUG
    assert(iWhich>0);
#endif
    iStop  = -1;
    iStart = 0;

    while (iWhich>0)
    {
        iStop  = FindSubString(delimiter,iStart);

        if (iStop == -1)
            iStop = (int)Length();

        iWhich--;

        if (iWhich)
            iStart = iStop + (int)delimiter.Length();

        if (iStart > (int) Length())
        {
            iStart = -1;
            iStop  = -1;
            return;
        }
    }
}

void psDelimitedString::SetParm(int iWhich,const char *pNewParm)
{
    int iStart,iStop;

    FindParm(iWhich,iStart,iStop);

    if (iStart != -1)
    {
        DeleteAt(iStart,iStop-iStart);
        Insert(iStart,pNewParm);
    }
}

void psDelimitedString::GetParm(int iWhich,psString& stParm)
{
    int iStart,iStop;

    FindParm(iWhich,iStart,iStop);

    if (iStop != -1)  // successfully found parm
    {
        GetSubString(stParm,iStart,iStop);
    }
    else
        stParm = "";
}

