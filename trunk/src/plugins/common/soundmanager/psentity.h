/*
 * psentity.h
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Andrea Rizzi <88whacko@gmail.com>
 *           Saul Leite <leite@engineer.com>
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


#ifndef _PSENTITY_H_
#define _PSENTITY_H_

/**
 * This object represents a planeshift entity sound.
 *
 * @see psSoundSector psEntity main user.
 */ 

class psEntity
{
public:
    csString factoryName;      ///< name of this entity's factory
    csString meshName;         ///< name of this entity's mesh

    bool     active;           ///< is this psEntity active, used for caching purposes
    
    
    /**
     * Create an empty psEntity. The initial state is set to 1.
     */
    psEntity();

    /**
     * Destructor. When destroyed the sounds is not stopped and it is removed
     * by the SoundSystemManager when it is done.
     */
    ~psEntity();

    /**
     * Copy Constructor. It copies everthing but not the handle that is set to 0.
     *
     * @note the data of each possible state is copied by reference to save time
     * so any change on that data will affect also its clone.
     */
    psEntity(psEntity* const &entity);

    /**
     * Gets the probability to play a sound in the current state.
     * return the probability to play a sound.
     */
    float GetProbability() const;

    /**
     * Get the ID of the mesh associated to this entity.
     * @return the ID of the mesh.
     */
    uint GetMeshID() const;

    /**
     * Set the ID of the mesh associated to this entity.
     * @param id the new id of this entity.
     */
    void SetMeshID(uint id);

    /**
     * Define a new state and its parameters. If the same state has already been
     * defined the method do nothing and returns false.
     *
     * @param state the state id.
     * @param resource the sound to be played when in this state.
     * @param startResource the sound to be played when the entity change its
     * state into this one.
     * @param volume the volume at which this sound is played.
     * @param minRange minimum distance at which the sound is heard.
     * @param maxRange maximum distance at which the sound is heard
     * @param probability the probability that resource is played when in this
     * state.
     * @param timeOfDayStart time when this entity starts playing in hours.
     * @param timeOfDayEnd timewhen this entity stops playing in hours.
     * @param delayAfter how much time this entity waits before playing again
     * from the moment the sound ends in seconds.
     * @return true if the state has been defined, false if already exists.
     */
    bool DefineState(int state, const char* resource, const char* startResource,
        float volume, float minRange, float maxRange, float probability,
        int timeOfDayStart, int timeOfDayEnd, int delayAfter);

    /**
     * Decrease by the given interval the time that this entity have to wait
     * before play again.
     * @param interval the time elapsed in milliseconds.
     */
    void ReduceDelay(int interval);

    /**
     * Used to determine if this is a temporary entity associated to a specific mesh
     * object or if it is a factory/mesh entity that is not associated to any mesh.
     * @return true if this entity is temporary, false otherwise.
     */
    bool IsTemporary() const;

    /**
     * Check if the sound associated to this entity is still working.
     * @return true if it is still playing, false otherwise.
     */
    bool IsPlaying() const;

    /**
     * Checks if all condition for the sound to play are satisfied in
     * the current state.
     *
     * @param time <24 && >0 is resonable but can be any valid int.
     * @param range the distance to test.
     * @return true if the dalayed is done, the entity is not in an
     * undefined state, the given time is within this entity's time
     * window and if the distance is between the minimum and maximum
     * range. False otherwise.
     */
    bool IsReadyToPlay(int time, float range) const;

    /**
     * Check if time is within this entity's timewindow and if the distance
     * is between the minimum and maximum range in any defined state. This
     * control for only the current state is performed by IsReadyToPlay().
     * 
     * @param time <24 && >0 is resonable but can be any valid int.
     * @param range the distance to test.
     * @return true if conditions are satisfied for at least one defined
     * state, false otherwise.
     */
    bool CheckTimeAndRange(int time, float range) const;
    
    /**
     * Set the new state for the entity and play the start resource (if any)
     * with the given SoundControl. If it is already playing a sound, it is
     * stopped.
     *
     * If the given state is undefined for this entity the change of state
     * is not forced, the state does not change. On the other hand if the
     * change is forced the entity goes in a special "undefined state". In
     * this state psEntity cannot play anything.
     *
     * @param state the new state >= 0 for this entity. For negative value the
     * function is not defined.
     * @param ctrl the SoundControl for playing the starting sound.
     * @param entityPosition the position of the starting sound to play.
     * @param forceChange true to force the state change, false otherwise.
     */
    void SetState(int state, SoundControl* ctrl, csVector3 entityPostion, bool forceChange);

    /**
     * Play this entity sound.
     * You need to supply a SoundControl and the position for this sound.
     *
     * @param ctrl the SoundControl to control this sound.
     * @param entityposition position of this entity.
     * @return true if the sound is played, false if it is not or if this
     * entity is already playing a sound.
     */
    bool Play(SoundControl* &ctrl, csVector3 entityPosition);

private:
    struct StateParameters
    {
        csString resource;                      ///< resource name of the sound associated to the state
        csString startResource;                 ///< resource to be played when the entity state chage
        float volume;                           ///< volume of the sound
        float minRange;                         ///< minimum distance at which the sound is heard
        float maxRange;                         ///< maximum distance at which the sound is heard
        float probability;                      ///< how high is the probability that this entity makes this sound
        int timeOfDayStart;                     ///< time when this entity starts playing
        int timeOfDayEnd;                       ///< time when this entity stops.
        int delayAfter;                         ///< number of seconds till played again

        int references;                         ///< how many psEntity point to this StateParameters
    };

    csHash<StateParameters*, int> statePar;     ///< all the parameters associated to their state
    SoundHandle*                  handle;       ///< pointer to the SoundHandle if playing
    int                           state;        ///< current state of this entity. A negative value means that this entity is in an undefined state.
    int                           when;         ///< counter to keep track when it has been played - zero means i may play at any time (in ms)
    uint                          id;           ///< the id of the mesh object whose sound is controlled by this entity.

    float minMinRange;                         ///< minimum minRange between all the defined state of this entity
    float maxMaxRange;                         ///< maximum maxRange between all the defined state of this entity
    int minTimeOfDayStart;                     ///< minimum timeOfDayStart between all the defined state of this entity
    int maxTimeOfDayEnd;                       ///< maximum timeOfDayEnd between all the defined state of this entity

    /**
     * The Callback gets called if the SoundHandle is destroyed.
     * It sets handle to NULL.
     */
    static void StopCallback(void* object);
};

#endif /*_PSENTITY_H_*/
