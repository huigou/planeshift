/*
 * psstring.h
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
 *
 * A slightly improved string over csString.  Adds a couple of functions
 * that will be needed by the XML parser.
 */
#ifndef PS_STRING_H
#define PS_STRING_H

#include <csutil/csstring.h>

#define XML_CASE_INSENSITIVE true
#define XML_CASE_SENSITIVE   false

class csStringArray;
class MathEnvironment;

class psString : public csString
{
public:
    psString() {}
    psString(const char* str) : csString(str) {}
    psString(const csStringBase& str) : csString(str) {}
    psString(const csString& str) : csString(str) {}

    int FindSubString(const char *sub, size_t start=0, bool caseInsense=XML_CASE_SENSITIVE,bool wholeWord=false) const;
    int FindSubStringReverse(psString& sub, size_t start, bool caseInsense=XML_CASE_SENSITIVE);
    void GetSubString(psString& str, size_t from, size_t to);
    bool FindNumber(unsigned int & pos, unsigned int & end);
    bool FindString(const char *border, unsigned int & pos, unsigned int & end);

    /** Escapes special xml characters.*/
    const char* EscapeXML();

    enum
    {
        NO_PUNCT=0,
        INCLUDE_PUNCT=1
    };

    void GetWord(size_t pos,psString &buff,bool wantPunct=INCLUDE_PUNCT);
    void GetWordNumber(int which,psString& buff);
    void GetLine(size_t start,csString& line);

    /// Perform string interpolation, i.e. replacing ${...} with the appropriate variable.
    void Interpolate(const MathEnvironment *env);
    bool ReplaceSubString(const char* what, const char* with);
    int ReplaceAllSubString(const char* what, const char* with, bool wholeWord=false);

    bool operator==(psString& other)
    { return strcmp(GetData(),other.GetData())==0; }

    bool operator<(psString& other)
    { return strcmp(GetData(),other.GetData())<0;  }

    bool operator==(const char* other)
    { return strcmp(GetData() ? GetData() : "",other ? other : "") == 0;}

    int PartialEquals(psString& other)
    { return strncmp(GetData(),other.GetData(),other.Length()); }

    size_t FindCommonLength(psString& other);
    
    bool IsVowel(size_t pos); /// Check if a character is a vowel
    void Plural(); /// Turn the last word of the string into an English plural

    void Split(csStringArray& result, char delim='|');
};

#endif

