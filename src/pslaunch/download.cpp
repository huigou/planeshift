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
#include "updaterconfig.h"
#include "updaterengine.h"

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

int ProgressCallback(void *clientp, double finalSize, double dlnow, double ultotal, double ulnow)
{
    static int lastSize = 0;
    double progress = dlnow / finalSize;
    
    // Don't output anything if there's been no progress.
    if(progress == 0)
        return 0;

    if(lastSize == 0)
    {
        if(finalSize > 102400)
        {
            UpdaterEngine::GetSingletonPtr()->PrintOutput("\n0%% ");
            lastSize = progress;
        }
    }
    else if(finalSize/progress < 4 && lastSize < finalSize/4)
    {
        UpdaterEngine::GetSingletonPtr()->PrintOutput(" 25%% ");
        lastSize = progress;
    }
    else if(finalSize/progress < 2 && lastSize < finalSize/2)
    {
        UpdaterEngine::GetSingletonPtr()->PrintOutput(" 50%% ");
        lastSize = progress;
    }
    else if((float)finalSize/(float)progress < 1.34 && (float)lastSize < (float)finalSize/1.34)
    {
        UpdaterEngine::GetSingletonPtr()->PrintOutput(" 75%% ");
        lastSize = progress;
    }
    else if(progress == finalSize)
    {
        UpdaterEngine::GetSingletonPtr()->PrintOutput(" 100%%\n\n");
        lastSize = 0;
    }
    else if((progress-lastSize) > (finalSize/20) && progress < finalSize - (finalSize/20))
    {
        UpdaterEngine::GetSingletonPtr()->PrintOutput("-");
        lastSize = progress;
    }

    fflush(stdout);
    
    return UpdaterEngine::GetSingletonPtr()->CheckQuit() ? 1 : 0;
}

Downloader::Downloader(csRef<iVFS> _vfs, UpdaterConfig* _config)
{
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curlerror = new char[CURL_ERROR_SIZE];
    curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, curlerror);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

    vfs = _vfs;
    config = _config;
    csRandomGen random = csRandomGen();
    startingMirrorID = random.Get((uint32)config->GetCurrentConfig()->GetMirrors().GetSize());
    activeMirrorID = startingMirrorID;    
}

Downloader::Downloader(csRef<iVFS> _vfs)
{
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curlerror = new char[CURL_ERROR_SIZE];
    curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, curlerror);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    vfs = _vfs;
}

Downloader::~Downloader()
{
    delete[] curlerror;
    curl_easy_cleanup(curl);
}

void Downloader::SetProxy(const char *host, int port)
{
}

bool Downloader::DownloadFile(const char *file, const char *dest, bool URL, bool silent, uint retries, bool vfsPath)
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
            url.Append(file);
        }

        csString destpath = dest;

        if (vfs)
        {
            csRef<iDataBuffer> path;
            if(vfsPath)
            {
                path = vfs->GetRealPath(destpath);
            }
            else
            {
                path = vfs->GetRealPath("/this/" + destpath);
            }

            destpath = path->GetData();
        }
        else
        {
            printf("No VFS in object registry!?\n");
            return false;
        }

        long curlhttpcode = 200;
        csString error;

        for(uint i=0; i<=retries; i++)
        {
        FILE* file;
        file = fopen (destpath, "wb");

        if (!file)
        {
            printf("Couldn't write to file! (%s).\n", dest);
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.GetData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &ProgressCallback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

        CURLcode result = curl_easy_perform(curl);

        fclose (file);

        curl_easy_getinfo (curl, CURLINFO_HTTP_CODE, &curlhttpcode);

            if (result != CURLE_OK)
            {
                if(!silent)
                {
                    if (result == CURLE_COULDNT_CONNECT || result == CURLE_COULDNT_RESOLVE_HOST)
                        error.Format("Couldn't connect to mirror %s\n", url.GetData());
                    else
                        error.Format("Error %d while downloading file: %s\n", curlerror, url.GetData());
                }
            }
            else
            {
                break;
            }
	}

        // Tell the user that we failed
        if(curlhttpcode != 200 || !error.IsEmpty())
        {
            if(!silent)
            {
                if(error.IsEmpty())
                    printf ("Server error %i (%s)\n", curlhttpcode, url.GetData());
                else
                    printf ("Server error: %s (%i)\n", error.GetData(), curlhttpcode);
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
        printf("\nThere are no active mirrors! Please check the forums for more info and help!\n");

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
