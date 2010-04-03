/*
 * data.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com> 
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#include <crystalspace.h>
#include <iutil/objreg.h>

#include "util/log.h"
#include "data.h"

/*
 * SndData is a set of functions to help us load and unload sounds
 * simple interface 
 */

/*
 * Initializes a loader and vfs
 * if one of both fails it will return false
 */

bool
SoundData::
Initialize(iObjectRegistry* objectReg)
{
    sndloader = csQueryRegistry<iSndSysLoader> (objectReg);
  
    if (!sndloader)
    {
        Error1("Failed to locate Sound loader!");
        return false;
    }
   

    if (!(vfs = csQueryRegistry<iVFS> (objectReg)))
    {
        Error1("psSndSourceMngr: Could not initialize. Cannot find iVFS");
        return false;
    }
    
    return true;
}

/*
 * Loads a soundfile into a given buffer
 * it checks if we have the resource and loads it if its not already loaded
 */
 
bool
SoundData::
LoadSoundFile (const char *name, csRef<iSndSysData> &snddata)
{
    sndfile                     *snd;
    csRef<iDataBuffer>          soundbuf; 
  
    if ((snd = GetSound(name)) != NULL)
    {
        /* Sound exists in "known or loaded state
         * get the sound with GetSound
         * check if its already loaded ..
         * return true if it is
         * if not load it and mark it loaded
         */
        if (snd->loaded == true)
        {
            snddata = snd->snddata;
            return true;
        }
    }
    else
    {
        /*
         * give it a chance
         * maybe this is a dynamic file
         */
        snd             = new sndfile;
        snd->name       = csString(name);
        snd->filename   = csString(name);
        snd->loaded = false;
        /*
         * This sndfile wont be deleted!
         * thats intended!
         */
    }

    /* load the sounddata into a buffer */
    if (!(soundbuf = vfs->ReadFile (snd->filename)))
    {
        Error2("Can't load file '%s'!", name); /* FIXME */
        return false;
    }
    
    /* extract sound from data */
    if (!(snddata = sndloader->LoadSound (soundbuf)))
    {
        Error2("Can't load sound '%s'!", name);
        return false;
    }

    snd->loaded = true;
    snd->snddata = snddata;
  
    return true;
}

/*
 *  FIXME
 * it should unload sounds which are no longer needed
 */

void
SoundData::
UnloadSoundFile (const char *file)
{
    return;
}

/*
 * Returns the reuqests sound if it exists
 * NULL if it doesnt
 */

sndfile *
SoundData::
GetSound (const char *name)
{
    sndfile *snd;

    if (!(snd = soundfiles.Get(csHashCompute(name), NULL)))
    {
        return NULL;
    }
    return snd;
}

/*
 * loads soundlib.xml get all the names and filenames
 * store them in the hash "soundfiles"
 * 
 * i think we could load all the sounds right now but i fear that this
 * could eat a lot of memory
 * reload should be possible but im too lazy right now
 * 
 */

bool
SoundData::
LoadSoundLib (const char* filename, iObjectRegistry* objectReg)
{
    csRef<iDocumentSystem>          xml;    /* try get existing Document System or create one*/
    csRef<iDataBuffer>              buff;   /* buffer for reading the xml */
    csRef<iDocument>                doc;    /* document created out of the xml */
    const char*                     error;  /* to store error msgs */
    csRef<iDocumentNode>            root;   /* document root */
    csRef<iDocumentNode>            topNode; /* topnode to work with */
    csRef<iDocumentNodeIterator>    iter; /* node iterator */
    csRef<iDocumentNode>            node;   /* yet another node .... */
    sndfile                         *snd;   /* new soundfiles */
    
    if ( !(xml = csQueryRegistry<iDocumentSystem> (objectReg)))
      xml = csPtr<iDocumentSystem> (new csTinyDocumentSystem);

    buff = vfs->ReadFile(filename);

    if ( !buff || !buff->GetSize())
    {
        return false;
    }

    doc = xml->CreateDocument();

    error = doc->Parse(buff);
    if (error)
    {
        Error3("Parsing file %s gave error %s", filename, error);
        return false;
    }
    
    
    if( !(root = doc->GetRoot()))
    {
        Error1("No XML root in soundlib.xml");
        return false;
    }

    
    if( !(topNode = root->GetNode("Sounds")))
    {
        Error1("No <sounds> tag in soundlib.xml");
        return false;
    }
    
    iter = topNode->GetNodes();

    while (iter->HasNext())
    {
        node = iter->Next();

        if (node->GetType() != CS_NODE_ELEMENT)
            continue;

        if (strcmp(node->GetValue(), "Sound") == 0)
        {
            snd = new sndfile;
            snd->name = node->GetAttributeValue("name");
            snd->filename = node->GetAttributeValue("file");
            snd->loaded = false;
            soundfiles.Put(csHashCompute((const char*) snd->name), snd);
       }
    }
    return true;
}
