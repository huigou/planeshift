/*
* download.cpp
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <cssysdef.h>

#include <csutil/randomgen.h>

#include "download.h"

static int ProgressCallback(int progress, int finalSize)
{
    static int lastSize = 0;
    
    if(lastSize == 0)
    {
        if(finalSize > 102400)
        {
            printf("\n0%% ");
            if(progress == 0)
                progress++;
            lastSize = progress;
        }
    }
    else if(finalSize/progress < 4 && lastSize < finalSize/4)
    {
        printf(" 25%% ");
        lastSize = progress;
    }
    else if(finalSize/progress < 2 && lastSize < finalSize/2)
    {
        printf(" 50%% ");
        lastSize = progress;
    }
    else if((float)finalSize/(float)progress < 1.34 && (float)lastSize < (float)finalSize/1.34)
    {
        printf(" 75%% ");
        lastSize = progress;
    }
    else if(progress == finalSize)
    {
        printf(" 100%%\n");
        lastSize = 0;
    }
    else if((progress-lastSize) > (finalSize/20) && progress < finalSize - (finalSize/20))
    {
        printf("-");
        lastSize = progress;
    }

    fflush(stdout);
    
    return 0;
}

Downloader::Downloader(csRef<iVFS> _vfs, UpdaterConfig* _config)
{
    vfs = _vfs;
    config = _config;
    csRandomGen random = csRandomGen();
    startingMirrorID = random.Get((uint32)config->GetCurrentConfig()->GetMirrors().GetSize());
    activeMirrorID = startingMirrorID;    
}

Downloader::Downloader(csRef<iVFS> _vfs)
{
    vfs = _vfs;
}

Downloader::~Downloader()
{
}

void Downloader::SetProxy(const char *host, int port)
{
}

bool Downloader::DownloadFile(const char *file, const char *dest, bool URL, bool silent)
{
    // Get active url, append file to get full path.
    Mirror* mirror;
    if(URL)
    {
        mirror = new Mirror;
        mirror->SetBaseURL(file);
    }
    else
    {
        mirror = config->GetCurrentConfig()->GetMirrors().Get(activeMirrorID);
    }
    
    while(mirror)
    {
        csString url = mirror->GetBaseURL();
        
        if(!URL)
        {
            url.AppendFmt(file);
        }

        csString destpath = dest;

        if (vfs)
        {
            csRef<iDataBuffer> path = vfs->GetRealPath("/this/" + destpath);
            destpath = path->GetData();
        }
        else
        {
            printf("No VFS in object registry!?\n");
            return false;
        }

        nsHTTPConn *conn = new nsHTTPConn(url.GetData());
        int result = conn->Open();
        result = conn->ResumeOrGet(ProgressCallback, destpath.GetData());
        int httpCode = conn->GetResponseCode();
        conn->Close();
        delete conn;
        conn = NULL;

        csString error;

        if (result != nsSocket::OK && !silent)
        {
            if (result == nsSocket::E_INVALID_HOST)
                error.Format("Couldn't connect to mirror %s. \n", url.GetData());
            else
                error.Format("Error while downloading file: %s.\n", url.GetData());
        }

        // Tell the user that we failed
        if(httpCode != 200 || !error.IsEmpty())
        {
            if(!silent)
            {
                if(error.IsEmpty())
                    printf ("Server error %i (%s).\n", httpCode, url.GetData());
                else
                    printf ("Server error: %s (%i).\n", error.GetData(), httpCode);
            }

            if(!URL)
            {
                // Try the next mirror.
                mirror = config->GetCurrentConfig()->GetMirror(CycleActiveMirror());
                continue;
            }
            break;
        }
        
        // Success!
        if(URL)
        {
            delete mirror;
            mirror = NULL;
        }
        return true;
    }

    if(URL)
    {
        delete mirror;
        mirror = NULL;
    }
    else
        printf("There are no active mirrors! Please check the forums for more info and help!\n");

    return false;
}

uint Downloader::CycleActiveMirror()
{
    activeMirrorID++;
    // If we've reached the end, go back to the beginning of the list.
    if(activeMirrorID == config->GetCurrentConfig()->GetMirrors().GetSize())
        activeMirrorID = 0;
    // If true, we've reached our start point. Break the loop.
    if(activeMirrorID == startingMirrorID)
        activeMirrorID = (uint32)config->GetCurrentConfig()->GetMirrors().GetSize();

    return activeMirrorID;
}
