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

#ifndef psSoundSector_H_
#define psSoundSector_H_

/**
 * work in progress - Looks like a mishap. I hope find a object where i
 * can merge this into
 */ 

class psSoundSector
{
    public:
    csString                name;               ///< name of this sector
    bool                    active;             ///< is this sector active?
    psMusic                *activeambient;      ///< active ambient music
    psMusic                *activemusic;        ///< active music
    csArray<psMusic*>       ambientarray;       ///< array of available ambients
    csArray<psMusic*>       musicarray;         ///< array of available musics
    csArray<psEmitter*>     emitterarray;       ///< array of emitters
    csArray<psEntity*>      entityarray;        ///< array of entitys

    csVector3               playerposition;     ///< current playerposition
    int                     timeofday;          ///< sector time of day
    
    csRandomGen                 rng;            ///< random number generator

    /**
     * Constructor
     * @param sector documentnode that contains all sector data
     */
    psSoundSector (csRef<iDocumentNode> sector);
    /**
     * Destructor.
     * Cleans all arrays
     */
    ~psSoundSector ();
    void AddAmbient (csRef<iDocumentNode> Node);
    void UpdateAmbient (int type, SoundControl* &ctrl);
    void DeleteAmbient (psMusic* &ambient);
    void AddMusic (csRef<iDocumentNode> Node);
    void UpdateMusic (bool loopToggle, int type, SoundControl* &ctrl);
    void DeleteMusic (psMusic* &music);
    void AddEmitter (csRef<iDocumentNode> Node);
    void UpdateEmitter (SoundControl* &ctrl);
    void DeleteEmitter (psEmitter* &emitter);
    void AddEntity (csRef<iDocumentNode> Node);
    void UpdateEntity (SoundControl* &ctrl);
    void DeleteEntity (psEntity* &entity);
    void Load (csRef<iDocumentNode> sector);
    void Reload (csRef<iDocumentNode> sector);
    void Delete ();
};

#endif /*psSoundSector_H_*/
