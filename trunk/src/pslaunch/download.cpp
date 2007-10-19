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

#include <psconfig.h>

#include <csutil/randomgen.h>

#include "download.h"

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

Downloader::Downloader(csRef<iVFS> _vfs, psUpdaterConfig* _config)
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
    startingMirrorID = random.Get((uint32)config->GetCurrentConfig()->GetMirrors()->GetSize());
    activeMirrorID = startingMirrorID;    
}

Downloader::~Downloader()
{
    delete[] curlerror;
    curl_easy_cleanup(curl);
}

void Downloader::SetProxy (const char* host, int port)
{
    curl_easy_setopt (curl, CURLOPT_PROXY, host);
    curl_easy_setopt (curl, CURLOPT_PROXYPORT, port);
}

bool Downloader::DownloadFile (const char* file, const char* dest)
{
    // Get active url, append file to get full path.
    Mirror* mirror = config->GetCurrentConfig()->GetMirrors()->Get(activeMirrorID);
    while(mirror)
    {
        csString url = mirror->GetBaseURL();
        url.AppendFmt(file);

        char* destpath;

        if (vfs)
        {
            csRef<iDataBuffer> prefixpath = vfs->GetRealPath("/this/");
            destpath = (char *)malloc(strlen(dest) + prefixpath->GetSize()+1); //not sure if the +1 is necessary, but paranoid.
            strncpy(destpath,prefixpath->GetData(),prefixpath->GetSize());
            strcat(destpath,dest);
        }
        else
        {
            printf("No VFS in object registry!?\n");
            return false;
        }

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

        CURLcode result = curl_easy_perform(curl);

        fclose (file);
        free(destpath);
        destpath = 0;

        long curlhttpcode;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &curlhttpcode);

        csString error;

        if (result != CURLE_OK)
        {
            if (result == CURLE_COULDNT_CONNECT || result == CURLE_COULDNT_RESOLVE_HOST)
                error = "Couldn't connect to mirror.\n";
            else
                error.Format("Error while downloading: %s, file %s.\n",curlerror,url.GetData());
        }

        // Tell the user that we failed
        if (curlhttpcode != 200 || error != "")
        {
            if(error == "")
                printf ("Server error %ld (%s).\n",curlhttpcode,url.GetData());
            else
                printf ("Server error: %s (%ld).\n",error.GetData(),curlhttpcode);

            // Try the next mirror.
            mirror = config->GetCurrentConfig()->GetMirrors()->Get(CycleActiveMirror());
            continue;
        }
        // Success!
        return true;
    }
    printf("There are no active mirrors! Please check the forums for more info and help!\n");
    return false;
}


uint Downloader::CycleActiveMirror()
{
    activeMirrorID++;
    // If we've reached the end, go back to the beginning of the list.
    if(activeMirrorID == config->GetCurrentConfig()->GetMirrors()->GetSize())
        activeMirrorID = 0;
    // If true, we've reached our start point. Break the loop.
    if(activeMirrorID == startingMirrorID)
        activeMirrorID = (uint32)config->GetCurrentConfig()->GetMirrors()->GetSize();

    return activeMirrorID;
}
