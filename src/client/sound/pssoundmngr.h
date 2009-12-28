/*
 * pssoundmngr.h --- Saul Leite <leite@engineer.com>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 * The main manager of sounds and sound resource handler.
 * Classes in this file:
 *          psSoundManager
 *          psSoundManager::psSndSourceMngr
 *          psSoundHandle
 */
#ifndef PS_SOUND_MANAGER_H
#define PS_SOUND_MANAGER_H

// CS includes
#include <csutil/sysfunc.h>
#include <csutil/ref.h>
#include <csutil/parray.h>
#include <csutil/stringarray.h>
#include <iutil/comp.h>
#include "csutil/hash.h"
#include "isndsys/ss_structs.h"
#include "isndsys/ss_data.h"
#include "isndsys/ss_stream.h"
#include "isndsys/ss_source.h"
#include "isndsys/ss_renderer.h"

// PS includes
#include "iclient/isoundmngr.h"
#include "util/psresmngr.h"
#include "util/genericevent.h"

struct iEngine;
struct iObjectRegistry;

class psSoundHandle;
class psMapSoundSystem;
class psSoundObject;
class psSectorSoundManager;
struct ps3DFactorySound;

struct psSoundFileInfo;

enum SoundEvent
{
    SOUND_EVENT_WEATHER = 1
};

/*  Notes on class structure:
 *
 *  psSoundManager is the top level interface from PlaneShift.
 *  psMapSoundSystem handles only sounds dealing with maps and sectors.  It contains a list of active Songs, Ambients and Emitters
 *    and performs the high level logic for updates related to all active sounds.
 *  psSectorSoundManager handles sound logic for a single sector.  It contains logic for working with sounds for an entire sector.
 *  psSoundObject represents a single sound source - 3D or not.
 *
 *  psSectorSoundManager is called on to setup sounds when entering a new sector, and called to test if sounds that are already playing
 *    exist in the new sector as well.  psMapSoundSystem controls the actual list of active sounds (playing sounds for songs/ambience and
 *    playable-if-in-range sounds for emitters).  Time and position based updates pass through psMapSoundSystem directly to each active
 *    sound and do not pass through the sector manager, so emitters must all be added to the active list when the sector is entered.
 */



/** The main PlaneShift Sound Manager.
 * Handles the details for loading/playing sounds and music.
 */
class psSoundManager : public iSndSysRendererCallback, public scfImplementation2<psSoundManager, iSoundManager, iComponent>
{

public:
    psSoundManager(iBase* iParent);
    virtual ~psSoundManager();
    virtual bool Setup();

    virtual bool Initialize(iObjectRegistry* object_reg);

    /** Called when a player enters into a new sector.
      * The time/weather are required to figure out the best sounds to play.
      *
      * @param sector The name of the sector crossed into.
      * @param timeOfDay The current time of day
      * @param weather The current weather conditions.
      * @param position The players current position ( used for 3d sound emitters ).
      */
    virtual void EnterSector( const char* sector, int timeOfDay, int weather, csVector3& position );


    virtual void SetVolume(float vol);
    virtual void SetMusicVolume(float vol);
    virtual void SetAmbientVolume(float vol);
    virtual void SetActionsVolume(float vol);
    virtual void SetGUIVolume(float vol);
    virtual void SetVoicesVolume(float vol);

    virtual float GetVolume();
    virtual float GetMusicVolume();
    virtual float GetAmbientVolume();
    virtual float GetActionsVolume();
    virtual float GetGUIVolume();
    virtual float GetVoicesVolume();

    // Use these to play sounds. Choose the right method for your intended purpose. This will assure correct volume control
    virtual csRef<iSndSysSource> StartMusicSound(const char* name,bool loop = iSoundManager::NO_LOOP);
    virtual csRef<iSndSysSource> StartAmbientSound(const char* name,bool loop = iSoundManager::NO_LOOP);
    virtual csRef<iSndSysSource> StartActionsSound(const char* name,bool loop = iSoundManager::NO_LOOP);
    virtual csRef<iSndSysSource> StartGUISound(const char* name,bool loop = iSoundManager::NO_LOOP);
    virtual csRef<iSndSysSource> StartVoiceSound(const char* name,bool loop = iSoundManager::NO_LOOP);

    // This one should not be used. I'm leaving it in only because... I'm using it inside the class :-).
    // And you can insist on controlling volume.
    virtual csRef<iSndSysSource> StartSound(const char* name,float volume,bool loop = iSoundManager::NO_LOOP);

    /** Change the current song to use as the background
     * @param name The resource name of the sound file
     * @param loop Set to true if the song should loop.
     * @param fadeTime This is how long a crossfade between the existing BG
     * song and the new one should be.
     *
     * @return True if the BG song was changed.
     */
    virtual bool OverrideBGSong(const char* name, bool loop, float fadeTime );
    virtual void StopOverrideBG();

    /** Fades all the sounds in that sector */
    void FadeSectorSounds( Fade_Direction dir );

    virtual void ToggleMusic(bool toggle);
    virtual void ToggleAmbient(bool toggle);
    virtual void ToggleActions(bool toggle);
    virtual void ToggleGUI(bool toggle);
    virtual void ToggleVoices(bool toggle);
    virtual void ToggleLoop(bool toggle);
    virtual void ToggleCombatMusic(bool toggle);

    virtual bool PlayingMusic() {return musicEnabled;}
    virtual bool PlayingAmbient() {return ambientEnabled;}
    virtual bool PlayingActions() {return actionsEnabled;}
    virtual bool PlayingGUI() {return guiEnabled;}
    virtual bool PlayingVoices() {return voicesEnabled;}
    virtual bool LoopBGM() {return loopBGM;}
    virtual bool PlayingCombatMusic() {return combatMusicEnabled;}

    /** Update the sound system with the new time of day */
    virtual void ChangeTimeOfDay( int newTime );

    /** Change the mode if the player is fighting */
    virtual void ChangeMusicMode(bool combat) {musicCombat = combat;}

    /**Return name of the background music that is overriding */
    virtual const char* GetSongName() {return overSongName;}

    /** Retrieves a iSoundHandle for a given resource name.
      * If the resource is not loaded and cannot be loaded an invalid csRef<> is returned.
      * (ref.IsValid() == false).
      *
      * @param name The resource name of the sound file.
      *
     */
    virtual csRef<iSndSysData> GetSoundResource(const char *name);

    /// Retrieves a pointer to the sound renderer - the main interface of the sound system
    virtual csRef<iSndSysRenderer> GetSoundSystem() { return soundSystem; }

    bool HandleEvent(iEvent &event);

    virtual void HandleSoundType(int type);


    /** Update the 3D sound list.
      * @param The current player position.
      */
    void Update( csVector3 pos );


    /** Update the listner.  This is to update the sound renderer to indicate
      * that the player ( ie the listener ) has changed positions. This in turn
      * will adjust the volume of any 3D sounds that are playing.
      *
      * @param view The current view that the camera has.  This could be done using
      *             the player position as well but I am taking this as an example from
      *             walktest ( please don't hurt me ).
      */
    void Update( iView* view );

    void StartMapSoundSystem();
    iEngine* GetEngine() { return engine; }

    /** Update the current weather. This will trigger WEATHER sounds in
      * the current sector
      * @param weather New weather from the WeatherSound enum (weather.h)
      * @param sector The sector to update
      */
    void UpdateWeather(int weather,const char* sector);
    void UpdateWeather(int weather);


	/* This is the iSndSysRendererCallback interface */
	void StreamAddNotification(iSndSysStream *pStream);
	void StreamRemoveNotification(iSndSysStream *pStream);
	void SourceAddNotification(iSndSysSource *pSource);
	void SourceRemoveNotification(iSndSysSource *pSource);



private:

    csRef<iConfigManager>   cfgmgr;
	/// This array holds the currently playing voice file, plus any in queue, in order.
	csStringArray voicingQueue;

	/// This reference holds the current source, so the callback can know when to play the next voice
	/// while other sounds are playing.
	csRef<iSndSysSource> currentVoiceSource;
    
	bool soundEnabled;
    bool musicEnabled;
    bool ambientEnabled;
    bool actionsEnabled;
    bool guiEnabled;
    bool voicesEnabled;
    bool loopBGM;
    bool combatMusicEnabled;

    float musicVolume;
    float ambientVolume;
    float actionsVolume;
    float guiVolume;
    float voicesVolume;

    bool musicCombat; //This is used for determining if the character is fighting (true), or not (false).

    const char* overSongName;

    /// The time of the last sound emitter update check.
    csTicks lastTicks;

    /// Declare our event handler
    DeclareGenericEventHandler(EventHandler,psSoundManager,"planeshift.sound");
    csRef<EventHandler> eventHandler;

    /// This handles the resource details of loading sounds
    class psSndSourceMngr : public psTemplateResMngr
    {
    public:
        psSndSourceMngr(psSoundManager* parent);
        virtual ~psSndSourceMngr();

        bool Initialize();

        bool LoadSoundLib(const char* fname);
        csRef<iSndSysData> LoadSound(const char* fname);
        csRef<psSoundHandle> CreateSound (const char* name);

        /// Check to see if the two resources are pointing to the same file.
        bool CheckAlreadyPlaying(psSoundHandle* oldResource,
                                 const char* newResource);


    public:
        csPtr<psTemplateRes> LoadResource (const char* name);
        csHash<psSoundFileInfo *> sndfiles;
        psSoundManager* parent;
        csRef<iSndSysLoader> sndloader;
        csRef<iVFS> vfs;
    } sndmngr;
    friend class psSndSourceMngr;

public:
   csRef<iSndSysRenderer> soundSystem;

private:
    csRef<iSndSysSource> backsound;
    csRef<psSoundHandle> backhandle;
    csRef<iEngine> engine;
    csRef<iObjectRegistry> object_reg;
    csString currentSectorName;
    csRef<iSndSysSource> oldBackSound;
    csRef<psSoundHandle> oldBackHandle;

    // Current sound managers for sectors.
    psSectorSoundManager* currentSoundSector;
    psSectorSoundManager* lastSoundSector;
    psSectorSoundManager* lastlastSoundSector;//This is just for safety.

    psSoundObject* backgroundSong;

    /// How many ticks are needed before a 0.01 change is made in volume.
    float volumeTick;

    /// True if the sound manager is currently crossfading.
    bool performCrossFade;

    /// The last tick count recored.
    csTicks oldTick;

    /// The current volume of the BG song fading out.
    float fadeOutVolume;

    /// The current volume of the BG song fading in.
    float fadeInVolume;

    // Current meshes already registered as emitters.
    csSet<iMeshWrapper*>  registered;

    psMapSoundSystem* mapSoundSystem;

    /// Set to true once the "permanent" settings of the listener have been set
    bool ListenerInitialized;

    // path to the soundlib xml file
    csString soundLib;
};


//-----------------------------------------------------------------------------

/// A basic sound handle
class psSoundHandle : public psTemplateRes
{
public:
    psSoundHandle(iSndSysData* ndata) : snddata(ndata)
    {
    }
    ~psSoundHandle()
    {
    }

    csRef<iSndSysData> operator() () { return snddata; }

    csRef<iSndSysData> snddata;
};

/// A basic stream handle
class psSndStreamHandle
{
protected:
    csRef<iSndSysStream> soundStream;
    csRef<iSndSysSource> soundSource;

public:
    psSndStreamHandle(csRef<iSndSysStream> s) : soundStream(s) {}

    bool Start(iSndSysRenderer* renderer, bool loop, size_t loopStart, size_t loopEnd);
    void Stop(iSndSysRenderer* renderer);

    void SetVolume(float volume);

    csRef<SOUND_SOURCE3D_TYPE> GetSource3D();

    bool IsValid() { return soundSource.IsValid(); }
    
    size_t GetPlayPos() { return soundStream->GetPosition(); }
    size_t SetPlayPos(size_t position) { soundStream->SetPosition(position); }
};

//-----------------------------------------------------------------------------


/** A Sound Object.
 * This basically handles all types of map sounds that are in the game from
 * background to ambient to 3d sound emitters.
 */
class psSoundObject
{
public:
    psSoundObject(csRef<iSndSysStream> soundData,
                  psMapSoundSystem* mapSystem,
                  float maxVol, float minVol, int fadeDelay = 0,
                  int timeOfDay = 0, int timeOfDayRange = 0, int weatherCondition = 0, size_t loopStart = 0,
                  size_t loopEnd = 0, bool looping = true,
                  psSectorSoundManager* sector = NULL,
                  int connectWith = 0);

    psSoundObject(psSoundObject* other, csRef<iSndSysStream> soundData);

    ~psSoundObject();

    /** Start to fade this sound.
      * @param dir  FADE_UP or FADE_DOWN.
      */
    void StartFade( Fade_Direction dir );

   /** Update this sound.
     * This is normally for fading sounds to update their volumes.
     */
    void Update();

    /** Attaches this sound object to a static ( ie non moving mesh ) defined in
      * the map world file.
      *
      * @param meshName The name of the mesh in the world file.
      */
    void AttachToMesh( const char* meshName ) { attachedMesh = meshName; }

    csString& GetMesh() { return attachedMesh; }

    /** Update a sound for a position.
     * This is normally for 3d sounds who's volume is based on player position.
     * Currently this is only for emitters.
     *  @param position The players position.
     */
    void Update( csVector3& position );

    /** For 3D sounds to set a new position.
     * @param pos The position of the sound emitter in the world.
     */
    void SetPosition( csVector3& pos ) { position = pos; }

    /** For 3D sounds sets the max and min range for the volume adjusts.
     */
    void SetRange( float maxRange, float minRange );

    /** Starts a 3D sound.
      * @param position The current players position to the sound.
      */
    void Start3DSound( csVector3 &position );
    void StartSound();
    void Stop() { stream.SetVolume(0.0f); isPlaying = false; }

    bool HasNoTime() { return (timeOfDay == -1); }
    bool MatchTime( int time );
    bool HasNoWeather() { return (weatherCondition == -1); }
    bool MatchWeather( int weather ) { return weatherCondition == weather; }
    
    

    float Volume() { return currentVolume * ambientVolume;}
    void SetVolume(float vol);
    //TODO: check if there is no other kind of data that can be equal between two objects.
    bool Same( const psSoundObject *other )
    {
        return (this->resourceName == other->resourceName);
    }

    /** Returns true if this sound is triggered and should not
      * be auto started
      */
    bool Triggered();

    bool IsPlaying() { return isPlaying; }

    void SetResource( const char* resource ) { resourceName = resource; }
    csString& GetName() { return resourceName; }

    void Notify(int event);

    /** Returns true if this sound is looping **/
    bool IsLooping() {return loop;}

    /** Set the value of loop **/
    void SetLooping(bool looping) {loop = looping;}
    
    size_t GetPlayPos() { return stream.GetPlayPos(); }
    size_t SetPlayPos(size_t position) { stream.SetPlayPos(position); }

protected:

    void UpdateWeather(int weather);

    psSndStreamHandle stream;
    csRef<SOUND_SOURCE3D_TYPE> soundSource3D;

    csString resourceName;
    csString attachedMesh;
    bool isPlaying;

    float maxVol;
    float minVol;
    float currentVolume;
    float ambientVolume;

    csTicks startTime;          ///< Use to track start of fade.
    bool fadeComplete;

    Fade_Direction fadeDir;
    csTicks fadeDelay;          ///< Time this sound should complete it's fade.
    int timeOfDay;
    int timeOfDayRange;         ///< The range from timeOfDay.
    int weatherCondition;
    psSectorSoundManager* connectedSector;
    psMapSoundSystem*     mapSystem;

    int connectedWith; ///< What we are connected with (SoundEvent)

    bool threeDee;              ///< Track if this is a 3D sound object
    csVector3 position;
    float rangeToStart;
    float minRange;
    float rangeConstant;        ///< Used to calculate 3D sound volume.


    bool loop;              ///< indicates if this soundobject loops
    size_t loopStart;       ///< indicates the start of the loop sequence
    size_t loopEnd;         ///< indicates the end of the loop sequence
};

//-----------------------------------------------------------------------------

class psSectorSoundManager
{
public:
    psSectorSoundManager( csString& sectorName, iEngine* engine, psMapSoundSystem *mapSS);
    ~psSectorSoundManager();
    void Music ( bool toggle ) { music = toggle; }
    void Sounds( bool toggle ) { sounds = toggle; }

    //These two methods are used for determining if the sector has music/sound enable or not.
    bool HasMusic (){return music;}
    bool HasSounds(){return sounds;}

    void NewBackground( psSoundObject* song );
    void NewAmbient( psSoundObject* sound );
    void New3DSound( psSoundObject* sound );
    void New3DMeshSound( psSoundObject* sound );
    void New3DFactorySound( psSoundObject* sound, const char* factory, float probability );

    void Init3DFactorySounds();

    /** Change the sound time in the sector and see if new music is needed.
     * @param newTime The new time of day.
     */
    void ChangeTime( int newTime );
    
    /** Sets a new Background Song.
     * 
     * @param song: the song to set as background song for this sector
     */
    void SetBGSong(psSoundObject* song);
    void Enter( psSectorSoundManager* enterFrom, int timeOfDay, int weather, csVector3& position );

    void StartBackground();
    void StopBackground();
    void StartSounds(int weather);
    void StopSounds();

    bool CheckSong( psSoundObject* bgSound );
    bool CheckAmbient( psSoundObject* ambient );

    void Fade( Fade_Direction dir );

    /** Update the current weather.
      * @param weather New weather from the WeatherSound enum (weather.h)
      */
    void UpdateWeather(int weather);

    /** Appends the soundobject to a list of objects
      * and calls Notify on it when that event occurs
      * @param obj The sound object
      * @param obj Event to subscribe to (SoundEvent)
      */
    void ConnectObject(psSoundObject* obj,int to);
    void DisconnectObject(psSoundObject* obj,int from);

    const char* GetSector() {return sector;}
    int GetWeather() { return weather;}

private:
    csRef<iEngine> engine;
    csString sector;
    psMapSoundSystem *mapsoundsystem;

    csPDelArray<psSoundObject> songs;
    csPDelArray<psSoundObject> ambient;
    csPDelArray<psSoundObject> emitters;

    csArray<psSoundObject*>    weatherNotify; // Objects to call when we get weather event

    psSoundObject* mainBG;

    // These are 3d sound emitters that are attached to a mesh but have not yet
    // had their positions set.  When the map is loaded it will get their positions
    // and move them from this list into the emitters array.
    csArray<psSoundObject*> unAssignedEmitters;
    csArray<ps3DFactorySound*> unAssignedEmitterFactories;

    int weather;
    bool music;
    bool sounds;
};

//-----------------------------------------------------------------------------

class psMapSoundSystem
{
public:
    psMapSoundSystem( psSoundManager* manager, iObjectRegistry* object );
    ~psMapSoundSystem();
    bool Initialize();

    void EnableMusic( bool enable );
    void EnableSounds( bool enable );
    csRef<iObjectRegistry> objectReg;
    csHash<psSectorSoundManager*, csString> sectors;
    csArray<psSectorSoundManager*> pendingSectors;
    psSoundManager* sndmngr;


    void RegisterActiveSong(psSoundObject *song);
    void RemoveActiveSong(psSoundObject *song);
    void SetMusicVolumes(float volume);
    psSoundObject *FindSameActiveSong(psSoundObject *other);

    void RegisterActiveAmbient(psSoundObject *ambient);
    void RemoveActiveAmbient(psSoundObject *ambient);
    void SetAmbientVolumes(float volume);
    psSoundObject *FindSameActiveAmbient(psSoundObject *other);

    void RegisterActiveEmitter(psSoundObject *emitter);
    void RemoveActiveEmitter(psSoundObject *emitter);
    void SetEmitterVolumes(float volume);
    psSoundObject *FindSameActiveEmitter(psSoundObject *other);

    void RemoveActiveAnyAudio(psSoundObject *soundobject);
    void EnterSector(psSectorSoundManager *enterTo);

    void Update();
    void Update( csVector3 & pos );

    psSectorSoundManager* GetSoundSectorByName(const char* name);
    psSectorSoundManager* GetPendingSoundSector(const char* name);
    psSectorSoundManager* GetOrCreateSector(const char* name); // Creates a pending sector if it's not found
    int TriggerStringToInt(const char* str);
    size_t GetRandomNumber(size_t max) { randomGen.Get(max); }


private:
    /// Lists of playing songs, ambience sounds and emitters
    csArray<psSoundObject *> active_songs;
    csArray<psSoundObject *> active_ambient;
    csArray<psSoundObject *> active_emitters;
    csRandomGen randomGen;

};

struct ps3DFactorySound
{
    ps3DFactorySound(psSoundObject* s, const char* f, float p)
        : sound(s), meshfactname(f), probability(p) {}
    ~ps3DFactorySound() { delete sound; }

    psSoundObject* sound;
    csString meshfactname;
    float probability;
};


#endif // PS_SOUND_MANAGER_H



