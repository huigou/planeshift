/*
* ispellchecker.h, Author: Fabian Stock (AiwendilH@googlemail.com)
*
* Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef _ISPELLCHECKER_H_
#define _ISPELLCHECKER_H_

// crystalSpace includes
#include <csutil/scf.h>
#include <csutil/scf_implementation.h>

struct iSpellChecker : public virtual iBase
{
    SCF_INTERFACE(iSpellChecker, 1, 0, 0);
    
    /** adds a word to the personal dictionary
     */
    virtual void addWord(csString newWord) = 0;
    /** get an array with words of the personal dictionary
     */
    virtual const csArray<csString>& getPersonalDict() = 0;
    /** clears the whole personal dictionary
     */
    virtual void clearPersonalDict() = 0;
    /** are any dictionaries loaded?     
     */    
    virtual bool hasDicts() = 0;
    /** is the given word in one of the dictionaries?
     */
    virtual bool correct(csString wordToCheck) = 0;
};

#endif // _ISPELLCHECKER_H_