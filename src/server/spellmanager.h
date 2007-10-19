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

#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>
#include "msgmanager.h"   // Subscriber class
#include "util/gameevent.h"
#include "bulkobjects/pscharacter.h"
#include "events.h"
#include "gem.h"

class psSpellCastGameEvent;
class psSpellAffectGameEvent;
class psGlyphGameEvent;
class ClientConnectionSet;
class EntityManager;
class psSpell;
class psGlyph;
struct ProgressionEvent;
class MathScript;
class MathScriptVar;



/**
 *  This class handles all calculations around spell
 */
class psSpellManager : public MessageManager
{
public:

    psSpellManager(ClientConnectionSet *clients,
                   iObjectRegistry * object_reg);
    virtual ~psSpellManager();
    virtual void HandleMessage(MsgEntry *me,Client *client);

    void EndPurifying(psCharacter * character, uint32 glyphUID);
    void SendGlyphs(Client * client);

    void HandleSpellCastEvent( psSpellCastGameEvent *event );
    void HandleSpellAffectEvent( psSpellAffectGameEvent *event );

protected:    
    void SaveSpell(Client * client, csString spellName);
    void Cast(Client * client, csString spellName, float kFactor);
    void CancelSpellCasting(gemActor * caster);
    void SendSpellBook(Client * client);
    void StartPurifying(Client * client, int statID);
    psSpell* FindSpell(Client * client, const glyphList_t & assembler);

    psSpell * FindSpell(csString& name);
    psSpell * FindSpell(int spellID);

    MathScript *researchSpellScript;
    MathScriptVar *varCaster;
    MathScriptVar *varSpell;
    MathScriptVar *varSuccess;

    void HandleAssembler(Client* client, MsgEntry* me);
    
    csRandomGen* randomgen;
    //    BinaryTree<psSpell> spells;
    ClientConnectionSet *clients;
    iObjectRegistry *object_reg;
};


/**
 */
class psSpellCastGameEvent : public psGameEvent, public iDeleteObjectCallback
{
 protected:
    psSpellManager *spellmanager;
    
 public:

    Client      *caster;     /// Entity who casting this spell
    gemObject   *target;     /// Entity who is target of this spell
    const psSpell     *spell;      /// The spell that is casted
    float        max_range;
    float        powerLevel;
    csTicks      duration;
    
    psSpellCastGameEvent(psSpellManager *mgr,
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

/**
 */
class psSpellAffectGameEvent : public psGameEvent, public iDeleteObjectCallback, public iDeathCallback
{
 protected:
    psSpellManager *spellmanager;
    
 public:

    Client      *caster;     /// Entity who casting this spell
    gemObject   *target;     /// Entity who is target of this spell
    const psSpell     *spell;      /// The spell that is casted
    float        min_range;
    float        max_range;
    bool         saved;
    float        powerLevel;
    csTicks      duration;
    bool         inverse;   // specifies if the progression script should run it's inverse or not.
    
    psSpellAffectGameEvent(psSpellManager *mgr,
                           const psSpell *spell,
                           Client  *caster, 
                           gemObject *target, 
                           csTicks progression_delay,
                           float max_range,
                           bool saved,
                           float powerLevel,
                           csTicks duration,
                           bool inverse = false);

    ~psSpellAffectGameEvent();
                         
    virtual void Trigger();  // Abstract event processing function
    virtual void DeleteObjectCallback(iDeleteNotificationObject * object);
    virtual void DeathCallback( iDeathNotificationObject * object );
};

#endif
