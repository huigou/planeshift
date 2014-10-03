/*
 * scripting.h - by Kenneth Graunke <kenneth@whitecape.org>
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef SCRIPTING_HEADER
#define SCRIPTING_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/parray.h>
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"

/**
 * \addtogroup xmlscripting
 * @{ */

/** @file scripting.h
 * This is PlaneShift's XML based scripting system.
 * A RelaxNG schema is in data/ProgressionSchema.rng, both defining the
 * language and allowing one to check the validity of scripts.
 *
 * Scripts have two modes: imperative and applied.
 * - Imperative operations are one-shot, and can do basically anything
 *   one would like.
 * - Applied operations describe the effect of magic that lasts for a time,
 *   but which eventually will wear off.  Thus, every applied operation must
 *   have a well defined inverse.  These become ActiveSpells.
 *
 * See scripting.cpp for the implementation of all the various operations.
 */

// forward declarations
class MathEnvironment;
class MathExpression;
class ActiveSpell;
class EntityManager;
class CacheManager;

/**
 * Events that can trigger scripts, i.e. \<on type="attack"\>
 */
enum SCRIPT_TRIGGER
{
    ATTACK,     ///< triggered \<on type="attack"\>
    DEFENSE,    ///< triggered \<on type="defense"\>
    MOVE,       ///< triggered \<on type="move"\>
    NEARLYDEAD  ///< triggered \<on type="nearlydead"\>
};

class ImperativeOp; // forward declaration of private script operation

/** @short ProgressionScript is the imperative script container.
 *
 * An imperative script is a one-shot script.
 */
class ProgressionScript
{
public:
    /** @short default destructor
     * deletes all internally stored script operation object
     */
    ~ProgressionScript();
    /** @short create a progressionscript from a string containing a xml script
     * 
     * Basically this function creates a xml script from the given string and then
     * passes it on to the other Create function.
     * 
     * @param entitymanager of the psserver
     * @param cachemanager of the psserver
     * @param name of the progressionscript ?
     * @param script const char* string containing the script to parse and store
     */
    static ProgressionScript* Create(EntityManager* entitymanager,CacheManager* cachemanager, const char* name, const char* script);
    /** @short create a progressionscript from a xml script
     * 
     * Parses the supplied xml script and creates and stores an object representation of the script.
     * 
     * @param entitymanager of the psserver
     * @param cachemanager of the psserver
     * @param name of the progressionscript ?
     * @param top xml iDocumentNode containing the script to parse and store
     */
    static ProgressionScript* Create(EntityManager* entitymanager,CacheManager* cachemanager, const char* name, iDocumentNode* top);

    /** @short fetch the name of this progressionscript object
     * @return the name of the ProgressionScript
     */
    const csString &GetName()
    {
        return name;
    }
    /** Run is executing the internally stored script.
     * 
     * @param env is a container for passing variables to the script
     */
    void Run(MathEnvironment* env);

protected:
    /** @short internal constructor for Create().
     * Is constructor only available inside the class.
     * 
     * @param name of the script object (identifier ?)
     */
    ProgressionScript(const char* name) : name(name) { }
    
    csString name;                      ///< name of the script object (identifier?)
    csArray<ImperativeOp*> ops;         ///< script operation objects, warning: may have objects that contain further arrays of operations
};

class AppliedOp; // forward declaration of private script operation

/** @short ApplicativeScript is the applied script container.
 *
 * An applied script is a reversible script which is applied 
 * and after some time the applied effect is removed.
 * Therefore the operations of the script are computed and 
 * then stored in an ActiveSpell object. 
 * So every operation within the ApplicativeScript must be
 * reversible.
 */
class ApplicativeScript
{
public:
    /**
     * @short default destructore
     */
    ~ApplicativeScript();
    /** create a ApplicativeScript from a string containing a xml script
     * 
     * Basically this function creates a xml script from the given string and then
     * passes it on to the other Create function.
     * 
     * @param entitymanager of the psserver
     * @param cachemanager of the psserver
     * @param script const char* string containing the script to parse and store
     */
    static ApplicativeScript* Create(EntityManager* entitymanager, CacheManager* cachemanager, const char* script);
     /** \short create a progressionscript from a xml script
     * 
     * Parses the supplied xml script and creates and stores an object representation of the script.
     * 
     * @param entitymanager of the psserver
     * @param cachemanager of the psserver
     * @param top xml iDocumentNode containing the script to parse and store
     */
    static ApplicativeScript* Create(EntityManager* entitymanager, CacheManager* cachemanager, iDocumentNode* top);
     /** \short create a progressionscript from a xml script
     * 
     * Parses the supplied xml script and creates and stores an object representation of the script.
     * 
     * @param entitymanager of the psserver
     * @param cachemanager of the psserver
     * @param top xml iDocumentNode containing the script to parse and store
     * @param type of the spell (currently only buff or debuff)
     * @param name of the spell that is created
     * @param duration of the spell, may be a formula
     */
    static ApplicativeScript* Create(EntityManager* entitymanager, CacheManager* cachemanager, iDocumentNode* top, SPELL_TYPE type, const char* name, const char* duration);

    /** @short execute the script
     * Executes the script in the context of the supplied MathEnvironment.
     * A cancel event can be registered as well.
     * @param env is a MathEnvironment containing variables and objects necessary for the script to run
     * @param registerCancelEvent if true a cancel event is registered, otherwise not
     */
    ActiveSpell* Apply(MathEnvironment* env, bool registerCancelEvent = true);
    /***
     * retrieve the description of the Applicativescripts commands
     * @return csString containing a description of the scripts effect
     */
    const csString &GetDescription();

    void SetImage( csString tImage ) { image=tImage; }

protected:
    /**
     * @short internally used constructor
     */
    ApplicativeScript();        

    SPELL_TYPE type;            ///< spell type...buff, debuff, etc.
    csString aim;               ///< name of the MathScript var to aim at
    csString name;              ///< the name of the spell
    csString description;       ///< textual representation of the effect
    csString image;             ///< graphical representation of the effect
    MathExpression* duration;   ///< an embedded MathExpression
    csPDelArray<AppliedOp> ops; ///< all the sub-operations
};

#if 0
// Eventually, we'll want to be able to target items in the inventory,
// and specific points on the ground, as well as entities in the world.
class Target
{
public:
    Target(psItem*  item)   : type(TARGET_PSITEM),    item(item) { }
    Target(gemObject*  obj) : type(TARGET_GEMOBJECT), obj(obj)   { }
    //Target(Location loc) : loc(loc), type(TARGET_LOCATION) { }

protected:
    enum Type
    {
        TARGET_GEMOBJECT,
        TARGET_PSITEM,
        TARGET_LOCATION
    };
    Type type;

    union
    {
        psItem*    item;
        gemObject* obj;
        //Location loc;
    };
};
#endif

/**
 * @} */

#endif
