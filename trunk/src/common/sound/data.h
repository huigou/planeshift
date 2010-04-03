/*
 * data.h
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

#ifndef _SOUND_DATA_H_
#define _SOUND_DATA_H_

/*
 * little struct that most important informations about a soundfile
 * name - name of the resource
 * filename - contains the filename
 * snddata - contains the SndData in a format readable for the soundsystem
 * loaded - if its loaded or not true/false
 */

struct sndfile
{
    csString            name;
    csString            filename;
    csRef<iSndSysData>  snddata;
    bool                loaded;
};

class SoundData
{
    public:
    
    /* Initializes Loader and VFS */
    bool Initialize(iObjectRegistry* objectReg);
    /* reads a xml file (soundlib.xml for example)
     * and fills our hash. Its possible to call it multiple times
     * but thats untested and not intended */
    bool LoadSoundLib (const char* filename, iObjectRegistry* objectReg);
    /*
     * Loads a soundfile out of the vfs
     */ 
    bool LoadSoundFile (const char *name, csRef<iSndSysData> &snddata);
    /*
     * Should unload a soundfile
     * TODO not implemented
     */
    void UnloadSoundFile (const char *file);
    
    private:
    /* Crystalspace soundloader */
    csRef<iSndSysLoader> sndloader;
    /* Hash of sndfiles */  
    csHash<sndfile *> soundfiles;
    /* vfs where were reading from */
    csRef<iVFS> vfs;
    /* fetches a sndfile out of our hash */
    sndfile *GetSound (const char *name);       
};

#endif /*_SOUND_DATA_H_*/
