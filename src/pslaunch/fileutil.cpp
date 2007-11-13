/*
* fileutil.cpp by Matthias Braun <matze@braunis.de>
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

// All OS specific stuff should be in this file
#define CS_SYSDEF_PROVIDE_DIR
#include <psconfig.h>

#include <sys/stat.h>
#include <csutil/util.h>
#include <iutil/databuff.h> 
#include <csutil/csstring.h>

#include "fileutil.h"

FileUtil::FileUtil(csRef<iVFS> _vfs)
{
    vfs = _vfs;
}

FileUtil::~FileUtil()
{
}

FileStat* FileUtil::StatFile (const char* path)
{
    struct stat filestats;
    if (stat (path, &filestats) < 0)
        return NULL;

    FileStat* filestat = new FileStat;

#ifdef CS_PLATFORM_WIN32
    if (filestats.st_mode & _S_IFDIR)
        filestat->type = FileStat::TYPE_DIRECTORY;
    else
        filestat->type = FileStat::TYPE_FILE;
#else
    if (S_ISDIR (filestats.st_mode))
        filestat->type = FileStat::TYPE_DIRECTORY;
    else
        filestat->type = FileStat::TYPE_FILE;
#endif

    // XXX: Need support for symbolic links and executable
    filestat->link = false;
    filestat->executable = false;
    filestat->size = (uint32_t) filestats.st_size;
    filestat->readonly = !(filestats.st_mode & S_IWRITE);

    return filestat;
}

void FileUtil::RemoveFile (const char* filename)
{
    //If vfs deletefile fails, fall back on old variant
    if (!vfs->DeleteFile(filename))
    {
        int rc = remove(filename);
        if (rc < 0)
        {
            printf("Removal of '%s' failed.\n", filename);
        }
    }
    else
    {
        if (!vfs->Sync())
        {
            printf("Couldn't sync VFS!\n");
        }
    }
}

#ifdef CS_PLATFORM_UNIX
#include <sys/stat.h>

char* FileUtil::ConvertToSystemPath (const char* path)
{
    return csStrNew (path);
}

void FileUtil::MakeDirectory (const char* directory)
{
    // Append the /this/ path, since our assertion only works for relative paramaters
    if (vfs)
    {
        csRef<iDataBuffer> prefixpath = vfs->GetRealPath("/this/");
        csString destpath(prefixpath->GetData());
        destpath += directory;

        if (mkdir (destpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
            if(!vfs->Exists(csString("/this/") + directory))
                printf("Couldn't create directory '%s'.\n", (const char *)destpath);
    }
    else
        printf("Could not find VFS!!!");
}

#endif

#ifdef CS_PLATFORM_WIN32
#include <direct.h>

char* FileUtil::ConvertToSystemPath (const char* path)
{
    char* newpath = csStrNew(path);

    for (char*p = newpath; *p != 0; p++)
    {
        if (*p == '/')
            *p = '\\';
    }

    return newpath;
}

void FileUtil::MakeDirectory (const char* directory)
{
    char* path = ConvertToSystemPath (directory);
    int rc = mkdir(path);
    delete[] path;

    if (rc < 0)
        if(vfs->Exists(csString("/this/") + directory))
            printf("Couldn't create directory '%s'.", directory);
}

#endif

bool FileUtil::CopyFile(csString from, csString to, bool vfsPath, bool executable)
{
    csString n1;
    csString n2;

    csString file = to;
    if(vfsPath)
    {
        csRef<iDataBuffer> buff = vfs->GetRealPath(to);
        if(!buff)
        {
            printf("Couldn't get the real filename for %s!\n",to.GetData());
            return false;
        }

        file = buff->GetData();
    }

    FileStat* stat = StatFile(file);
    if(stat && stat->readonly)
    {
        printf("Won't write to %s, because it's readonly\n",file.GetData());
        return true; // Return true to bypass DLL checks and stuff
    }

    if (!vfsPath)
    {
        n1 = "/this/" + from;
        n2 = "/this/" + to;
    }
    else
    {
        n1= from;
        n2= to;
    }

    csRef<iDataBuffer> buffer = vfs->ReadFile(n1.GetData(),true);

    if (!buffer)
    {
        printf("Couldn't read file %s!\n",n1.GetData());
        return false;
    }

    if (!vfs->WriteFile(n2.GetData(), buffer->GetData(), buffer->GetSize() ) )
    {
        printf("Couldn't write to %s!\n", n2.GetData());
        return false;
    }

#ifdef CS_PLATFORM_UNIX
    // On unix type systems we might need to set permissions after copy.
    if(executable)
    {
        csString real(to);
        real.FindReplace("/this/", "./");
        if(chmod(real.GetData(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP) == -1)
            printf("Failed to set permissions on file %s.\n", real.GetData());
    }
#endif

    return true;
}
