/*
 * uploaddump.cpp by Joe Lyon
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>

#include "updaterconfig.h"
#include "updaterengine.h"

#include "globals.h"
#include "psengine.h"
#include "util/pscssetup.h"
#include "libcurl_wrapper.h"

///////////////////////////////////////////////////////////////////////////////////////
static google_breakpad::LibcurlWrapper* http_layer;
#ifdef WIN32
typedef std::wstring BpString;
#else
typedef std::string BpString;
#endif

std::map<BpString, BpString> parameters;
BpString report_code;



void UploadDump( const char * file, const char *uploadargs )
{
fprintf( stderr, "UploadDump sending file %s with args \"%s\"\n", file, uploadargs );


    http_layer = new google_breakpad::LibcurlWrapper();

    if(!http_layer->Init())
    {
                printf("Unable to start correctly libcurl!\n");
                return;
    }
        // Don't use GoogleCrashdumpUploader as it doesn't allow custom parameters.
        if (http_layer->AddFile(file, "upload_file_minidump")) {
                         http_layer->SendRequest("http://194.116.72.94/crash-reports/submit",
                                                        parameters,
                                                        &report_code);
    }
        else
        {
                        printf("Could not add minidump file.\n");
                        return;
    }




    delete http_layer;
    http_layer = NULL;

fprintf( stderr, "\n----------upload completed----------\n" );
  return;
}
