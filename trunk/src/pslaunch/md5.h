/*
* md5.h by Matthias Braun <matze@braunis.de>
*
* Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __UPDATER_MD5_H__
#define __UPDATER_MD5_H__

/// This class holds the result of a md5 sum calculation
class MD5Sum
{
public:
    MD5Sum (csRef<iVFS> vfs);
    MD5Sum (char* buffer, size_t len);
    ~MD5Sum ();

    /// Read a file and calculate it's md5
    bool ReadFile (const char* filename);    
    /// Calculates a new md5 sum of a buffer
    void Calculate (char* buffer, size_t len);
    /// Set MD5 sum to value of the string
    void Set (const char* md5sum);
    /// Get MD5 sum as a string
    const char* Get() const;
    /// compare 2 md5 sums
    bool operator == (const MD5Sum& other) const;

private:
    csRef<iVFS> vfs;
    char* md5;
};

#endif
