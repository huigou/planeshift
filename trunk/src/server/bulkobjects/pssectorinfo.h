/*
 * pssectorinfo.h
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
 */



#ifndef __PSSECTORINFO_H__
#define __PSSECTORINFO_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csgeom/vector3.h>
#include "util/scriptvar.h"

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================


/** Contains information about sectors from the server perspective.
*
*  Right now this class just includes the ID, name of a sector and rain parameters.
*  Future versions may include parameters for a bounding box, portals to other sectors, or other information.
*
*
*
*/
class psSectorInfo : public iScriptableVar
{
public:
    psSectorInfo();
    ~psSectorInfo();

    unsigned int GetRandomRainGap();
    unsigned int GetRandomRainDuration();
    unsigned int GetRandomRainDrops();
    unsigned int GetRandomRainFadeIn();
    unsigned int GetRandomRainFadeOut();
    unsigned int GetRandomLightningGap();

    /// This is used by the math scripting engine to get various values.
    double GetProperty(const char *ptr);
    double CalcFunction(const char * functionName, const double * params);
    const char *ToString() { return name.GetDataSafe(); }

    bool GetIsColliding() { return is_colliding; }
    bool GetIsNonTransient() { return is_non_transient; }
    /** Checks if this is set as a teleporting sector.
     *  A teleporting sector is a sector which teleports the player
     *  to defined sector (in teleportingSector/Cords/Rot) or spawn if not
     *  defined.
     *  @return TRUE if this is a teleporting sector.
     */
    bool GetIsTeleporting() { return is_teleporting; }
    /** Checks if a teleporting sector will trigger the death penalty when used.
     *  @note this can be TRUE even if the previous isn't but it won't do anything.
     *  @return TRUE if this is a sector with death penalty.
     */
    bool GetHasPenalty() { return has_penalty; }
    
    /** Checks if a teleporting sector will trigger the death penalty when used.
     *  @note this can be TRUE even if the previous isn't but it won't do anything.
     *  @return TRUE if this is a sector with death penalty.
     */
    bool GetDeathRestoreHP() { return deathRestoreHP; }
    
        /** Checks if a teleporting sector will trigger the death penalty when used.
     *  @note this can be TRUE even if the previous isn't but it won't do anything.
     *  @return TRUE if this is a sector with death penalty.
     */
    bool GetDeathRestoreMana() { return deathRestoreMana; }
    
    /** Gets the sector name we will teleport to when entering this sector.
     *  @return A csString containing the sector name.
     */
    csString GetTeleportingSector() { return teleportingSector; }
    /** Gets the sector cordinates we will teleport to when entering this sector.
     *  @return A csVector3 containing the sector cordinates.
     */
    csVector3 GetTeleportingCord() { return teleportingCords; }
    /** Gets the rotation we will teleport to when entering this sector.
     *  @return A float containing the rotation.
     */
    float GetTeleportingRot() { return teleportingRot; }

    /** Gets the sector name we will teleport to when dieing in this sector.
     *  @return csString A csString containing the sector name.
     */
    csString GetDeathSector() { return deathSector; }
    /** Gets the sector cordinates we will teleport to when dieing in this sector.
     *  @return A csVector3 containing the sector cordinates.
     */
    csVector3 GetDeathCord() { return deathCords; }
    /** Gets the rotation we will teleport to when dieing in this sector.
     *  @return A float containing the rotation.
     */
    float GetDeathRot() { return deathRot; }
    
    unsigned int uid;
    csString  name;
    bool rain_enabled; // Will run automatic weather when true
    unsigned int rain_min_gap,rain_max_gap;
    unsigned int rain_min_duration,rain_max_duration;
    unsigned int rain_min_drops,rain_max_drops;
    unsigned int lightning_min_gap,lightning_max_gap;
    unsigned int rain_min_fade_in,rain_max_fade_in;
    unsigned int rain_min_fade_out,rain_max_fade_out;
    unsigned int current_rain_drops; // Drops
    
    bool is_raining;
    bool is_snowing;
    bool is_colliding;
    bool is_non_transient;
    
    /// Sets if this sector will restore mana when the player dies in it.
    bool deathRestoreMana;
    /// Sets if this sector will restore hp when the player dies in it.
    bool deathRestoreHP;

    /// This sector will immediately teleport the player somewhere else when entered if true.
    bool is_teleporting;
    /// This sector will apply the death penalty if it's a teleporting sector
    bool has_penalty;
    /// The destination sector when this sector is a teleporting sector. Note if empty it will be spawn.
    csString teleportingSector;
    /// The destination cordinates when this sector is a teleporting sector
    csVector3 teleportingCords;
    /// The destination rotation when this sector is a teleporting sector
    float teleportingRot;
    ///The sector where to teleport on death from this sector. Note if empty it will be the default one.
    csString deathSector;
    ///the destination cordinates where to teleport on death from this sector
    csVector3 deathCords;
    /// The destination rotation where to teleport on death from this sector
    float deathRot;
    
    

    // Fog
    unsigned int fog_density, fog_density_old;
    bool densitySaved;
    unsigned int fogFade;
    int r,g,b;

    float say_range;

    csString god_name;
};



#endif


