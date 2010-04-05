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
 * i still have to make this doxygen compatible .. please dont mock me
 * but i need to get this story out of my head and into this file
 *
 * How SoundData works: (Birth till Death)
 *
 * Birth:
 *
 * On Initialization it aquires references to vfs and a loader.
 * It returns false if it fails and no further checking is done.
 *
 * On succes the very first thing that may happen is that LoadSoundLib is called.
 * It fills libsoundfiles with the data it found in soundlib.xml
 * Even if not called we can still process requests for filenames within our vfs.
 *
 * We assume LoadSoundLib has been called and the library is filled.
 *
 * Now Someone calls LoadSoundFile to load a named resource or file
 * the one that calls expects a pointer to the SoundFile he requested
 * depending on how succesfull this is LoadSoundFile will fill it in and return
 * true or false.
 *
 * GetSound is called and GetSound will search if theres a cached
 * SoundFile or if theres a unloaded resource in our soundlib.
 * if its cached then it will return that one. Thats best case
 * because LoadSoundFile can return a pointer to the cached data.
 * If its in our soundlib, then GetSound will make a copy and put that into our cache
 * (using PutSound) on succes it will return a Pointer to a valid SoundFile
 * on failure it will return NULL.
 *
 * GetSound may return NULL. In that case LoadSoundfile creates a new SoundFile
 * (object) with the given name as filename to check if we got a (dynamic)
 * file as name.
 * That happens if we are playing voicefiles. Those are downloaded from the server
 * and thus do not exist in our libraray. PutSound is used to add it to the cache.
 *
 * However theres always a valid SoundFile (object) for our loader
 * Now LoadSoundFile tries to load that file (finally).
 * There are two cases:
 * 1) file exists and is loadable (succes .. return true)
 * 2) file doesnt exist or ISNT loadable (failure .. return false)
 *
 * case 1) means that loaded (a bool) is set to true and snddata valid
 * case 2) loaded remains false and snddata is invalid / NULL
 *
 * LoadSoundFile has returned and the caller
 * might now do whatever he wanted todo with that snddata.
 *
 * Death:
 *
 * SoundData has a Update method that checks if there are expired SoundFiles
 * theres a hardcoded caching time of 300 seconds. After 300 seconds it will
 * check if there are still references on the snddata our SoundFile provides.
 * If theres only one then its the SoundFile object itself.
 * That means we go ahead and delete that object using DeleteSound.
 *
 * Now sometimes it isnt that easy ;) Maybe thats a looping background sound
 * in that case our RefCount is at last higher then one.
 * We set lasttouch to current ticks .. and check again .. in 300 seconds ;)
 *
 * Anyway UnloadSound is a public method and thus someone may call it for any
 * reason. Be aware!
 * It will crash your program if you unload data which is still in use ;)
 *
 * TODO: constructor, deconstructor, ReloadSoundLIb, UnloadSoundLib
 */

/*
 * Class that contains the most important informations about a soundfile
 * name - name of the resource
 * filename - contains the filename
 * snddata - contains the SndData in a format readable for the soundsystem
 * loaded - if its loaded or not true/false
 */

// FIXME i should be an option ;)
#define SOUNDFILE_CACHETIME     300000  ///<- number of milliseconds a file remains cached

class SoundFile
{
    public:
    csString            name;           ///< name of this file/resource MUST be unique
    csString            filename;       ///< filename in our vfs (maybe not unique)
    csRef<iSndSysData>  snddata;        ///< data in suitable format
    bool                loaded;         ///< true if snddata is loaded, false if not
    csTicks             lasttouch;      ///< last time when this SoundFile was used/touched

    SoundFile(const char *newname, const char *newfilename);
    // allows us to copy/clone a SoundFile
    SoundFile (SoundFile* const &copythat);
    ~SoundFile();
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
    // Unloads a SoundFile
    void UnloadSoundFile (const char *name);
    // checks if there are sounds to unload and unload them
    void Update ();

    private:
    /* Crystalspace soundloader */
    csRef<iSndSysLoader> sndloader;
    /* Hash of loaded SoundFiles */
    csHash<SoundFile *> soundfiles;
    /* Hash of Resources soundlib.xml provides */
    csHash<SoundFile *> libsoundfiles;

    /* vfs where were reading from */
    csRef<iVFS> vfs;
    // fetches a sndfile out of our hash
    SoundFile *GetSound (const char *name);
    // puts a SoundFile into our hash
    void PutSound (SoundFile* &sound);
    // delete a SoundFile and deletes it from our hash
    void DeleteSound (SoundFile* &sound);
};

#endif /*_SOUND_DATA_H_*/
