//
// Delimited String class
//
// KWF : 12/29/2000
//


#ifndef DELIMITEDSTRING_Z
#define DELIMITEDSTRING_Z

#include "psstring.h"

class psDelimitedString : public psString
{
protected:
    psString delimiter;

    void FindParm(int iWhich, int& iStart, int& iStop);

public:
    psDelimitedString(const char *pStr,const char *pDelimiter)
        : psString(pStr)
    {
        delimiter = pDelimiter;
    };
    psDelimitedString(psString& str,const char *pDelimiter)
        : psString(str)
    {
        delimiter = pDelimiter;
    };

    psString& operator=(const char *pStr)
    {
        return *(psString *)this = pStr;
    };

    void SetParm(int iWhich,const char *pNewParm);
    void GetParm(int iWhich,psString& stParm);
};

#endif
