/*
 * vitals.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PS_VITAL_HEADER
#define PS_VITAL_HEADER

#include <csutil/array.h>

#define HP_REGEN_RATE  0.2F
#define MANA_REGEN_RATE  0.2F


/** The vitals that the client is concerned about. Used as params into 
  * the vitals manager.
  */
enum PS_VITALS
{
    VITAL_HITPOINTS,
    VITAL_MANA,
    VITAL_PYSSTAMINA,
    VITAL_MENSTAMINA,
    VITAL_COUNT   // Count used to build a array of vitals, not a legal vital name
};

/** Used by the server to tell which fields are incoming */
enum PS_DIRTY_VITALS
{
    DIRTY_VITAL_HP              = 0x0001,
    DIRTY_VITAL_HP_MAX          = 0x0002,
    DIRTY_VITAL_HP_RATE         = 0x0004,
    DIRTY_VITAL_MANA            = 0x0008,
    DIRTY_VITAL_MANA_MAX        = 0x0010,
    DIRTY_VITAL_MANA_RATE       = 0x0020,
    DIRTY_VITAL_PYSSTAMINA      = 0x0040,
    DIRTY_VITAL_PYSSTAMINA_MAX  = 0x0080,
    DIRTY_VITAL_PYSSTAMINA_RATE = 0x0100,
    DIRTY_VITAL_MENSTAMINA      = 0x0200,
    DIRTY_VITAL_MENSTAMINA_MAX  = 0x0400,
    DIRTY_VITAL_MENSTAMINA_RATE = 0x0800,
    DIRTY_VITAL_EXPERIENCE      = 0x1000,
    DIRTY_VITAL_PROGRESSION     = 0x2000,
    DIRTY_VITAL_ALL = DIRTY_VITAL_HP | DIRTY_VITAL_HP_MAX | DIRTY_VITAL_HP_RATE |
                      DIRTY_VITAL_MANA | DIRTY_VITAL_MANA_MAX |DIRTY_VITAL_MANA_RATE |
                      DIRTY_VITAL_PYSSTAMINA | DIRTY_VITAL_PYSSTAMINA_MAX | DIRTY_VITAL_PYSSTAMINA_RATE | 
                      DIRTY_VITAL_MENSTAMINA | DIRTY_VITAL_MENSTAMINA_MAX | DIRTY_VITAL_MENSTAMINA_RATE |
                      DIRTY_VITAL_EXPERIENCE |
                      DIRTY_VITAL_PROGRESSION
};

//----------------------------------------------------------------------------


/** A character vital stat. These are things such as the current HP or MANA. Can 
  * be any number of different type of vital statistic.
  */
struct psCharVital
{
    float value;
    float drRate;
    float max;
	float maxModifier;
};



//----------------------------------------------------------------------------

/** Manages a set of Vitals and does the predictions and updates on them 
  *   when new data comes from the server.
  */
class psVitalManager
{
public:
    psVitalManager();
    ~psVitalManager();
    
    void ResetVitals();
    void SetOrigVitals();

    /** Get the value of a particular Vital.
     */
    float GetValue( int vital );
    
    /** Get players experience points. */
    int GetExp() { return experiencePoints; }
    
    /** Gets a players current progression points.*/
    int GetPP()  { return progressionPoints; }
    

    /** Get the current Hitpoint value. */   
    float GetHP() { return vitals[VITAL_HITPOINTS].value; }
    
    /** Get the current Mana value. */
    float GetMana() { return vitals[VITAL_MANA].value; }
    
    /** Get the current stamina value. */
    float GetStamina(bool pys);
    
    /** Get a reference to a particular vital. 
      * @param vital @see PS_VITALS
      * @return The vital reference. 
      */
    psCharVital & GetVital( int vital );
    
protected:
    /// Used by the above Predict function to determine new predicted values. 
    csTicks lastDRUpdate;        

    /** A list of player Vital. */   
    psCharVital vitals[VITAL_COUNT]; 
    psCharVital orig_vitals[VITAL_COUNT]; 
    
    /// Players current experience points
    int experiencePoints;
    
    /// Players progression Points
    int progressionPoints;
};
#endif
