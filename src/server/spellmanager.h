/*
 * spellmanager.h by Anders Reggestad <andersr@pvv.org>
 *
 * Copyright (C) 2001-2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef __SPELLMANAGER_H__
#define __SPELLMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/gameevent.h"
#include "bulkobjects/pscharacter.h"

//=============================================================================
// Server includes
//=============================================================================
#include "msgmanager.h"   // Subscriber class
#include "events.h"
#include "gem.h"

//=============================================================================
// Forward Declarations
//=============================================================================

class psSpellCastGameEvent;
class psSpellAffectGameEvent;
class psGlyphGameEvent;
class ClientConnectionSet;
class EntityManager;
class psSpell;
class psGlyph;
class ProgressionEvent;
class MathScript;
class MathScriptVar;



/** Manager class that handles loading/searching/casting spells. 
  * This class also manages a number of psSpell Events.  
  */
class SpellManager : public MessageManager
{
public:

    SpellManager(ClientConnectionSet *clients,
                   iObjectRegistry * object_reg);
    virtual ~SpellManager();
    
    /** Handles a network message.  
      * This is a factory for the different types of message that the manager handles.
      *
      * @param me The message entry incoming.
      * @param client The client that this message came from.
      */
    virtual void HandleMessage(MsgEntry *me,Client *client) { }

    /** Purifying on a glyph has been complete.  
      *  This will send out a network message to the client and update it's inventory 
      *  with the new purified glyph.
      *
      *  @param character The character this is for.
      *  @param glyphUID  The unique ID for this item instance of the glyph.
      */
    void EndPurifying(psCharacter * character, uint32 glyphUID);
    
    /** Sends out the glyphs to a client.  
      * Builds and sends psRequestGlyphsMessage for the client.
      *
      * @param client  The client that will be sent it's current glyphs.
      */
    void SendGlyphs(MsgEntry *notused, Client * client);

    /** Handles a cast event object.
      * @param event  The event that needs to be handled.
      */
    void HandleSpellCastEvent( psSpellCastGameEvent *event );

    /** Handles a spell effect event.  
      * @param event The spell affect event that needs to be handled.
      */    
    void HandleSpellAffectEvent( psSpellAffectGameEvent *event );

protected:    
    /** Save a spell to the database for when a player has researched it.
      *
      * @param client The client that this is for.
      * @param spellName The name of the spell to save for that player.
      */      
    void SaveSpell(Client * client, csString spellName);
    
    /** Case a particular spell.
      * 
      * @param client The client that is casting the spell.
      * @param spellName The name of the spell to cast.
      * @param kFactor The power factor that the spell is cast with.
      */
    void Cast(MsgEntry *me, Client * client);

    void HandleCancelSpell(MsgEntry* notused, Client* client);

    /** Send the player's spell book.
      * 
      * @param client The client that will be sent the spell book.
      */
    void SendSpellBook(MsgEntry *notused, Client * client);
    
    /** Start to purify a glyph.
      * This will also send out notifications to the client about the start of operation.
      *
      * @param client The client that this data is for.
      * @param The stat ID of the glyph that the player wants to purify.
      */  
    void StartPurifying(MsgEntry *me, Client * client);
    
    /** Find a spell in the assorted glyphs.
      * This checks ths list of glyphs and see if it matches any 
      * known spell. This is for when players are researching spells.
      * 
      * @param client The client this data is for.
      * @param assembler A list of glyphs to check for spell match.
      *
      * @return A spell if a match found, NULL otherwise.
      */
    psSpell* FindSpell(Client * client, const glyphList_t & assembler);

    /** Find a spell based on name.
      *
      * @param name The name of the spell to find.
      * 
      * @return a psSpell object that matches the name or NULL if no match found.
      */    
    psSpell * FindSpell(csString& name);
    
    /** Find a spell based on id.
      *
      * @param id The id of the spell to find in the spells table.
      * 
      * @return a psSpell object that matches the id or NULL if no match found.
      */        
    psSpell * FindSpell(int spellID);

    /** Handles a command when player tries to research.
      * 
      * @param client The client this is for.
      * @param me The message from that client.
      */
    void HandleAssembler(MsgEntry* me,Client* client);
    
    MathScript *researchSpellScript;                
    MathScriptVar *varCaster;
    MathScriptVar *varSpell;
    MathScriptVar *varSuccess;
     
    csRandomGen* randomgen;
    ClientConnectionSet *clients;
    iObjectRegistry *object_reg;
};

//-----------------------------------------------------------------------------

/** A spell event.
  * These are fired off when spells are cast.
 */
class psSpellCastGameEvent : public psGameEvent, public iDeleteObjectCallback
{
 protected:
    SpellManager *spellmanager;
    
 public:

    Client      *caster;        ///< Entity who casting this spell
    gemObject   *target;        ///< Entity who is target of this spell
    const psSpell     *spell;   ///< The spell that is casted
    
    float        max_range;     
    float        powerLevel;
    csTicks      duration;
    
    psSpellCastGameEvent(SpellManager *mgr,
                     const psSpell *spell,
                     Client  *caster,
                     gemObject *target,
                     csTicks castingDuration,
                     float max_range,
                     float powerLevel,
                     csTicks duration);

    ~psSpellCastGameEvent();
                         
    void Interrupt();

    virtual void Trigger();  // Abstract event processing function
    virtual void DeleteObjectCallback(iDeleteNotificationObject * object);
};

//-----------------------------------------------------------------------------

/** A spell event.
  * These are fired off when spells are cast.
 */
class psSpellAffectGameEvent : public psGameEvent, public iDeleteObjectCallback, public iDeathCallback
{
 protected:
    SpellManager *spellmanager;
    
 public:

    Client      *caster;        ///< Entity who casting this spell
    gemObject   *target;        ///< Entity who is target of this spell
    const psSpell     *spell;   ///< The spell that is casted
    float        min_range;
    float        max_range;
    bool         saved;
    float        powerLevel;
    csTicks      duration;
    
    psSpellAffectGameEvent(SpellManager *mgr,
                           const psSpell *spell,
                           Client  *caster, 
                           gemObject *target, 
                           csTicks progression_delay,
                           float max_range,
                           bool saved,
                           float powerLevel,
                           csTicks duration);

    ~psSpellAffectGameEvent();
                         
    virtual void Trigger();  // Abstract event processing function
    virtual void DeleteObjectCallback(iDeleteNotificationObject * object);
    virtual void DeathCallback( iDeathNotificationObject * object );
};

#endif
