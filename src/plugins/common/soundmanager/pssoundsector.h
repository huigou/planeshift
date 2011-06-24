/*
 * pssoundsector.h
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
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

#ifndef _PSSOUNDSECTOR_H_
#define _PSSOUNDSECTOR_H_


/**
 * work in progress - Looks like a mishap. I hope find a object where i
 * can merge this into
 */ 
class psSoundSector
{
public:
    csString                     name;               ///< name of this sector
    bool                         active;             ///< is this sector active?
    psMusic*                     activeambient;      ///< active ambient music
    psMusic*                     activemusic;        ///< active music
    csArray<psMusic*>            ambientarray;       ///< array of available ambients
    csArray<psMusic*>            musicarray;         ///< array of available musics
    csArray<psEmitter*>          emitterarray;       ///< array of emitters
    csHash<psEntity*, csString>  factories;          ///< hash of factory entities (i.e. with meshName empty)
    csHash<psEntity*, csString>  meshes;             ///< hash of mesh entities
    csHash<psEntity*, uint>      tempEntities;       ///< hash of all the temporary psEntites (i.e. associated to a specific mesh)

    csVector3                    playerposition;     ///< current playerposition
    int                          timeofday;          ///< sector time of day

    csRandomGen                  rng;                ///< random number generator

    /**
     * Create an empty psSoundSector with no musics, ambients, emitters and entities.
     * @param name the psSoundSector's name
     * @param objReg the object registry
     */
    psSoundSector(const char* name, iObjectRegistry* objReg);

    /**
     * Constructor
     * @param sector documentnode that contains all sector data
     */
    psSoundSector(csRef<iDocumentNode> sector, iObjectRegistry* objReg);

    /**
     * Destructor.
     * Cleans all arrays
     */
    ~psSoundSector();
    void AddAmbient(csRef<iDocumentNode> Node);
    void UpdateAmbient(int type, SoundControl* &ctrl);
    void DeleteAmbient(psMusic* &ambient);
    void AddMusic(csRef<iDocumentNode> Node);
    void UpdateMusic(bool loopToggle, int type, SoundControl* &ctrl);
    void DeleteMusic(psMusic* &music);
    void AddEmitter(csRef<iDocumentNode> Node);
    void UpdateEmitter(SoundControl* &ctrl);
    void DeleteEmitter(psEmitter* &emitter);
    void AddEntity(csRef<iDocumentNode> Node);
    void UpdateEntity(SoundControl* &ctrl, psSoundSector* commonSector);
    void DeleteEntity(psEntity* &entity);

    /**
     * Sets the new state for the entity associated to the given mesh and
     * plays the start resource (if defined). If it is already playing a
     * sound, it is stopped.
     *
     * @param state the new state >= 0 for the entity. For negative value
     * the function is not defined.
     * @param ctrl the sound control used to play the start resource.
     * @param mesh the mesh associated to the entity.
     * @param forceChange if it is false the entity does not change its
     * state if the new one is not defined. If it is true the entity stops
     * play any sound until a new valid state is defined.
     */
    void SetEntityState(int state, SoundControl* ctrl, iMeshWrapper* mesh, bool forceChange);
    void Load(csRef<iDocumentNode> sector);
    void Reload(csRef<iDocumentNode> sector);
    void Delete();

private:
    csRef<iObjectRegistry> objectReg;

    void UpdateEntityValues(SoundControl* &ctrl, psEntity* entity, iMeshWrapper* mesh);
};

#endif /*_PSSOUNDSECTOR_H_*/
