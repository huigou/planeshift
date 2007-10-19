/*
* md5.cpp by Matthias Braun <matze@braunis.de>
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
#include <psconfig.h>

#include <string.h>

#include <csutil/snprintf.h>
#include <csutil/csmd5.h>
#include <csutil/util.h>
#include <iutil/vfs.h>
#include <csutil/databuf.h>

#include "md5.h"
#include "updaterconfig.h"

MD5Sum::MD5Sum (csRef<iVFS> _vfs)
: md5(NULL) 
{
    vfs = _vfs;
}

MD5Sum::MD5Sum (char* buffer, size_t len)
{
    Calculate (buffer, len);
}

MD5Sum::~MD5Sum ()
{
}

void MD5Sum::Calculate (char* buffer, size_t len)
{
    if (!md5)
        md5 = new char[33];

    csMD5::Digest digest = csMD5::Encode (buffer, (int) len);

    cs_snprintf (md5, 33,
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        digest.data[0], digest.data[1], digest.data[2], digest.data[3],
        digest.data[4], digest.data[5], digest.data[6], digest.data[7],
        digest.data[8], digest.data[9], digest.data[10], digest.data[11],
        digest.data[12], digest.data[13], digest.data[14], digest.data[15]);
}

void MD5Sum::Set (const char* md5sum)
{
    if(strlen (md5sum) != 32)
    {
        printf("Tried to set md5 sum with wrong len.\n");
        PS_PAUSEEXIT(1);
    }

    delete[] md5;
    md5 = csStrNew(md5sum);
}

const char* MD5Sum::Get() const
{
    return md5;
}

bool MD5Sum::ReadFile (const char* filename)
{
    csRef<iDataBuffer> buffer = vfs->ReadFile(filename,true);

    if (!buffer)
    {
        printf("No buffer!\n");
        return false;
    }

    Calculate (buffer->GetData(), buffer->GetSize());
    return true;
}

bool MD5Sum::operator == (const MD5Sum& other) const
{
    if (!md5 || !other.md5)
        return false;

    return strcmp(md5, other.md5) == 0;
}
