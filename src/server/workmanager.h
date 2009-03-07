/*
* workmanager.h
*
* Copyright (C) 2001-2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __WORKMANAGER_H__
#define __WORKMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/sysfunc.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"           // Message definitions
#include "net/msghandler.h"         // Network access

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class
#include "gem.h"

class psWorkGameEvent;
class WorkManager;
class psItem;
class Client;
struct CombinationConstruction;

// Define the work event types
#define MANUFACTURE 0                   // Digging, collecting, farming
#define PRODUCTION  1                   // Crafting
#define LOCKPICKING 2                   // Picking of locks
#define CLEANUP     3                   // Cleaning up of public containers
#define REPAIR      4                   // Repairing items takes time too

#define GARBAGE_PATTERNID   1           // Define for hard coded pattern IDs
#define CLEANUP_DELAY       600         // Seconds to wait before performing cleanup event
//#define CLEANUP_DELAY       10 // for testing only

/** Holds the possible return values for a test to see if an item is transformable.
    This is binary since each transform atempt could have multiple reasons for failure
 */
enum TradePatternMatch
{   
    TRANSFORM_MATCH                 = 0x0000,       ///< There was a transform that matched correctly
    TRANSFORM_UNKNOWN_PATTERN       = 0x0001,       ///< There was no pattern found
    TRANSFORM_UNKNOWN_ITEM          = 0x0002,       ///< Item is not used with any available transform.
    TRANSFORM_MISSING_ITEM          = 0x0004,       ///< No item found to use with work item.
    TRANSFORM_UNKNOWN_WORKITEM      = 0x0008,       ///< No work item container choosen.
    TRANSFORM_MISSING_EQUIPMENT     = 0x0010,       ///< Transform found but not correct equipment
    TRANSFORM_BAD_TRAINING          = 0x0020,       ///< Did not meet required skills.
    TRANSFORM_BAD_SKILLS            = 0x0040,       ///< Did not meet required skills.
    TRANSFORM_OVER_SKILLED          = 0x0080,       ///< Over skilled for this task.
    TRANSFORM_FAILED_CONSTRAINTS    = 0x0100,       ///< Failed one or more contstraints on the transform
    TRANSFORM_BAD_QUANTITY          = 0x0200,       ///< Item was ok for transform but quantity was not.         
    TRANSFORM_BAD_COMBINATION       = 0x0400,       ///< Combination items or ammounts not correct.
    TRANSFORM_TOO_MANY_ITEMS        = 0x0800,       ///< Tried to transform too many items.
    TRANSFORM_BAD_USE               = 0x1000,       ///< Use of item or ammounts not correct.
    TRANSFORM_NO_STAMINA            = 0x2000,       ///< Too tired to do trade skill work.
    TRANSFORM_GONE_WRONG            = 0x4000,       ///< Correct ingredients but wrong number or wrong process
    TRANSFORM_GARBAGE               = 0x8000        ///< There was a any item garbage transform that matched correctly
};

/** Holds the possible transformation types.
 */
enum TradeTransfomType
{   
    TRANSFORMTYPE_UNKNOWN=0,        ///< A unknown tranformation
    TRANSFORMTYPE_AUTO_CONTAINER,   ///< Transforming an item by putting it into auto-transform container
    TRANSFORMTYPE_SLOT,             ///< Transforming an item in an equipped slot
    TRANSFORMTYPE_CONTAINER,        ///< Transforming an item that is in a container
    TRANSFORMTYPE_SLOT_CONTAINER,   ///< Transforming an item that is in a container in an equipped slot
    TRANSFORMTYPE_TARGETSLOT,       ///< Transforming an item that is in a targetted actors equipped slot
    TRANSFORMTYPE_TARGET            ///< Transforming an item that is targeted
};

//-----------------------------------------------------------------------------

/** This class keeps natural resource concentrations across the world.
 */
struct NaturalResource
{
    int sector;                 ///< The id of the sector this resource is in.
    csVector3 loc;              ///< Centre point of resource location.
    float    radius;            ///< Radius around the centre where resource can be found.
    float    visible_radius;
    float    probability;       ///< Probability of finding resource on attempt.
    psSkillInfo *skill;         ///< Skill used to harvest resource. 
    int      skill_level;       ///< Skill level required to be able to harvest resource. 
    unsigned int item_cat_id;       
    float    item_quality;
    csString anim;                  ///< Name of animation to play while harvesting 
    int      anim_duration_seconds; ///< Length of time the animation should play.
    int      reward;
    csString reward_nickname;
    csString action;            ///< The action you need to take to get this resource.
};

//-----------------------------------------------------------------------------

struct constraint
{
    bool (*constraintFunction)(WorkManager* that, char* param);
    const char* name;
    const char* message;
};

//-----------------------------------------------------------------------------

/** This class handles all calculations around work, using statistics
*  and information from the pspccharacterinfo Prop Classes for both
*  the worker and the target.
*/
class WorkManager : public MessageManager
{
public:

    WorkManager();
    virtual ~WorkManager();

    virtual void HandleMessage(MsgEntry *me, Client *client)  { }

//-----------------------------------------------------------------------------
// Entry points
//-----------------------------------------------------------------------------

    /** Handles using an item for working.  This is called when an item 
      * is tagetted and the use command issued.
      *
      * @param client  The client that placed the item inside
      */
    void HandleUse(Client *client);

    /** Start Combine work
      * This gets called when the player tries to combine items in a container using /combine command.
      *
      * @param client  The client that placed the item inside
      */
    void HandleCombine(Client *client);
    
    /** Start a work event for this client.  This is called when an item is placed
      * in a container.  If the container is an auto-transform container it can transform items 
      * automatically ( ie placing items in automatically triggers the transformation process to start ).
      *
      * @param client  The client that placed the item inside
      * @param container The work item container that can transform an item.
      * @param autoItem The item that was placed inside and to be transformed.
      * @param count The stack count of the item placed in.
      */
    void StartAutoWork(Client *client, gemContainer* container, psItem *autoItem, int count);
    
    /** Checks to see if the progression script generated craft work can be done.
      *
      * @param client  The client for the actor that initiates that progressions script
      * @param target  Targetted item
      * @param client  Pattern name passed in progressions cript
      * @return False if there is a problem doing the craft.
      */
    bool StartScriptWork(Client* client, gemObject *target, csString pattern);

    /** Stop work event.  
     * This is called when a client removes an item from a container. 
     *  
     *  @param client The client that removed the item.
     *  @param item The item that was being transformed.
     */
    void StopWork(Client *client, psItem *item);

    /** @name Work Event Handlers
      *  These are the functions fired when a psWorkGameEvent is Triggered.
      */
    //@{     
    /** Handles a transformation/combination event. Basically the manufacturing events. 
      * 
      * @param event The work event that was in the queue to fire.
      */      
    void HandleWorkEvent(psWorkGameEvent* workEvent);
    
    /** Handles a cleanup event. Basically removing discarded items from public containers. 
      * 
      * @param event The work event that was in the queue to fire.
      */      
    void HandleCleanupEvent(psWorkGameEvent* workEvent);
    
    /** Handles a resource/harvesting event. Basically the production events. 
      * 
      * @param event The work event that was in the queue to fire.
      */          
    void HandleProductionEvent(psWorkGameEvent* workEvent);

    /** Handles a repair event, which occurs after a few seconds of repairing an item. 
    * 
    * @param event The work event that was in the queue to fire.
    */          
    void HandleRepairEvent(psWorkGameEvent* workEvent);
    void LockpickComplete(psWorkGameEvent* workEvent);   
    //@}
    
    
    /** @name Constraint Functions
    *  
    */
    //@{         
    static bool constraintTime(WorkManager* that, char* param);
    static bool constraintFriends(WorkManager* that,char* param);
    static bool constraintLocation(WorkManager* that,char* param);
    static bool constraintMode(WorkManager* that,char* param);
    static bool constraintGender(WorkManager* that,char* param);
    static bool constraintRace(WorkManager* that,char* param);
    //@}


    // Lockpicking
    void StartLockpick(Client* client,psItem* item);
    
   
    /** Sets up the internal structure of the work manager to handle a particular client.
      * This sets many variables of the workmanager to work with a single user at a time. 
      *
      * @param client The client that is the current one to use. 
      * @param target The object for which the client is targetting.
      *
      * @return False if there is a problem loading stuff.
      */
    bool LoadLocalVars(Client* client, gemObject *target=NULL);

    /* Send clear client view message to remove items from autocontainers.
      *
      * @param slotID The slot number to clear. 
      * @param containerID The container ID that has item that needs to be cleared. 
      *
      * @return False if there is a problem sending message.
      *
    bool SendClearUpdate( unsigned int slotID, unsigned int containerID ); */
    
    
    /// Handle production events from super clients
    void HandleProduction(gemActor *actor,const char *type,const char *reward);
    
protected:
    csPDelArray<NaturalResource> resources; ///< list of all natural resources in game.
    MathScript *calc_repair_time;           ///< This is the calculation for how long a repair takes.
    MathScript *calc_repair_result;         ///< This is the calculation for how many points of quality are added in a repair.
    MathScript *calc_mining_chance;         ///< This is the calculation for chance of successful mining.
    MathScript *calc_lockpick_time;         ///< This is the calculation for how long it takes to pick a lock.

    void HandleLockPick(MsgEntry* me,Client *client);
    void HandleWorkCommand(MsgEntry* me,Client *client);

    /** Stop auto work event.  
     * This is called when a client removes an item from any container 
     * before it has had a chance to transform the item. 
     *  
     *  @param client The client that removed the item.
     *  @param autoItem The item that was being transformed.
     */
    void StopAutoWork(Client *client, psItem *autoItem);

    /** Handles stopping the use of the item for working.  This is called when an item 
      * is tagetted and the /use command issued and it's already in use.
      *
      * @param client  The client that issues the /use command
      */
    void StopUseWork(Client *client);

    /** Checks to see if the item can be used for working.
      *
      * @param client  The client that issues the /use command
      */
    void StartUseWork(Client *client);
    
    /** Handles stopping the combining in the work container.  This is called when an item 
      * is tagetted and the /combine command issued and it's already in use.
      *
      * @param client  The client that issues the /combine command
      */
    void StopCombineWork(Client *client);

    /** Checks to see if the work container item can be used for combining.
      *
      * @param client  The client that issues the /combine command
      */
    void StartCombineWork(Client *client);

    /** Handles stopping the cleanup event for a particular item.  This is called when an item 
      * is removed from a container.
      *
      * @param client  The client that removes the item
      * @param cleanItem  The item that is removed
      */
    void StopCleanupWork(Client* client, psItem* cleanItem);

    /** Sends an error message to the client based on the trade pattern error. 
      * @see TradePatternMatch for the list of error conditions. 
      * @param clientNum the client to send the message to.
      * @param result The error code from the transformable test. 
      */
    void SendTransformError( uint32 clientNum, unsigned int result, uint32 curItemId = 0, int CurItemQty = 0 );
     
    /** Returns with the result ID and quantity of the combination
      *  if work item container has the correct items in the correct amounts
      * Note:  This assumes that the combination items array is sorted by
      *  resultId and then itemId
      *
      * @return resultID The item ID of the resulting item.
      * @return resultQty The stack quantity of the resulting item. 
      *
      * @return False if combination is not possible.
      */
    bool IsContainerCombinable(uint32 &resultId, int &resultQty);

    /** Returns with the result ID and quantity of the combination
      *  if player has the correct items in the correct amounts in hand
      * Note:  This assumes that the combination items array is sorted by
      *  resultId and then itemId
      *
      * @return resultID The item ID of the resulting item.
      * @return resultQty The stack quantity of the resulting item. 
      *
      * @return False if combination is not possible.
      */
    bool IsHandCombinable(uint32 &resultId, int &resultQty);

    /** Returns with the result ID and quantity of the combination
      *  if the item list matches every item in a valid combination
      *
      * @param itemArray The array of items that need to be checked.
      * @return resultID The item ID of the resulting item.
      * @return resultQty The stack quantity of the resulting item. 
      *
      * @return False if combination is not possible.
      */
    bool ValidateCombination(csArray<psItem*> itemArray, uint32 &resultId, int &resultQty);
    
    /** Returns with the result ID and quantity of the combination
      *  if the item list is in set of unique ingredients for that pattern
      *
      * @param itemArray The array of items that need to be checked.
      * @return resultID The item ID of the resulting item.
      * @return resultQty The stack quantity of the resulting item. 
      *
      * @return False if combination is not possible.
      */
    bool AnyCombination(csArray<psItem*> itemArray, uint32 &resultId, int &resultQty);

    /** Returns true then item array matchs combination array regardless of order
      *
      * @param itemArray The array of items that need to be checked.
      * @param current The combination structure that includes an array of items
      *
      * @return False if combination is not possible.
      */
    bool MatchCombinations(csArray<psItem*> itemArray, CombinationConstruction* current);

    /** Check to see if there is a possible trasnform available.
      * @param singlePatternID The current single pattern to use
      * @param groupPatternID The current group pattern to use
      * @param targetID the id of the item trying to transform.
      * @param targetQty The stack count of the item to transform.
      *
      * @return An indicator of pattern match status.
      */
    unsigned int AnyTransform(uint32 singlePatternId, uint32 groupPatternId, uint32 targetId, int targetQty);

    /** Check to see if there is a possible trasnform available.
      * @param patternId The current pattern to use
      * @param targetId the id of the item trying to transform.
      * @param targetQty The stack count of the item to transform.
      *
      * @return An indicator of pattern match status.
      */

    unsigned int IsTransformable(uint32 patternId, uint32 targetId, int targetQty);

    bool ScriptNoTarget();
    bool ScriptActor(gemActor* gemAct);
    bool ScriptItem(gemItem* gemItm);
    bool ScriptAction(gemActionLocation* gemAction);

    bool CombineWork();
    bool IsIngredient(uint32 patternId, uint32 groupPatternId, uint32 targetId);

    psItem* TransformContainedItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality);
    psItem* CombineContainedItem(uint32 newId, int newQty, float itemQuality, psItem* containerItem);
    psItem* TransformSlotItem(INVENTORY_SLOT_NUMBER slot, uint32 newId, int newQty, float itemQuality);
    psItem* TransformTargetSlotItem(INVENTORY_SLOT_NUMBER slot, uint32 newId, int newQty, float itemQuality);
    psItem* TransformTargetItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality);
//    bool TransformHandItem(uint32 newId, int newQty, float itemQuality);
    //bool SendItemUpdate( INVENTORY_SLOT_NUMBER slotID, psItem *newItem );

    void StartTransformationEvent(int transType, INVENTORY_SLOT_NUMBER transSlot, int resultQty, 
        float resultQuality, psItem* autoItem);
    void StartCleanupEvent(int transType, Client* client, psItem* item, gemActor* worker);

    bool ValidateTarget(Client* client);
    bool ValidateWork();
    bool ValidateMind();
    bool ValidateStamina(Client* client);
    bool IsOnHand(uint32 equipId);
    psItem* CreateTradeItem(uint32 newId, int newQty, float itemQuality, bool transient = false);
    bool ValidateTraining(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate);
    bool ValidateSkills(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate);
    bool ValidateNotOverSkilled(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate);
    bool ValidateConstraints(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate);
    int CalculateEventDuration(int pointQty);
    bool CheckStamina(psCharacter * owner) const;

    void Initialize();
    bool ApplySkills(float factor, psItem* transItem);
    void HandleRepair(Client *client, psWorkCmdMessage& msg);
    /// Handle production events from clients
    void HandleProduction(Client *client,const char *type,const char *reward);
    
    bool SameProductionPosition(gemActor *actor, const csVector3& startPos);
    NaturalResource *FindNearestResource(const char *reward,iSector *sector, csVector3& pos, const char *action);
        
private:

    csWeakRef<gemObject> worker;    ///< Current worker that the work manager is dealing with.
    
    uint32_t clientNum;             ///< Current client the work manager is dealing with.
    psItem* workItem;               ///< The current work item that is in use ( example, ore furnace )
    psItem* autoItem;               ///< The current item that is being transformed by auto-transformation container
    psCharacter *owner;             ///< The character pointer of the current character being used. 
    gemObject *gemTarget;           ///< The object being targeted by the player. 
    uint32 patternId;               ///< Current pattern ID
    uint32 groupPatternId;          ///< Current group pattern ID
    float patternKFactor;           ///< Pattern factor that's part of quality calculation
    float currentQuality;           ///< Current result item quality
    psTradeTransformations* trans;  ///< Current work transformation
    psTradeProcesses* process;      ///< Current work process
    const char* preworkModeString;  ///< Mode string prior to work
    bool secure;                    ///< Cleint reached required security level
};



//-----------------------------------------------------------------------------

// work event class
class psWorkGameEvent : public psGameEvent, public iDeleteObjectCallback
{
public:
    psWorkGameEvent(WorkManager* mgr,
                    gemActor* worker,
                    int delayticks, 
                    int cat,
                    csVector3& pos,
                    NaturalResource *natres=NULL,
                    Client *c=NULL,
                    psItem* object=NULL,
                    float repairAmount=0.0F);
    virtual ~psWorkGameEvent();

    void Interrupt();

    virtual void Trigger();  // Abstract event processing function

    virtual void DeleteObjectCallback(iDeleteNotificationObject * object);

    // Set the active trade transformation for the event.
    void SetTransformation(psTradeTransformations *t) { transformation = t; }

    // Return the active transformation, if any for this event.
    psTradeTransformations *GetTransformation() { return transformation; }

    // Set the active trade process for the event.
    void SetProcess(psTradeProcesses *p) { process = p; }

    // Return the active process, if any for this event.
    psTradeProcesses *GetProcess() { return process; }

    // result quantity is only used when transaction result is zero
    int GetResultQuantity() { return resultQuantity; }
    void SetResultQuantity(int newQuantity) { resultQuantity = newQuantity; }

    // result quality is calculated immediately before the event
    float GetResultQuality() { return resultQuality; }
    void SetResultQuality(float newQuality) { resultQuality = newQuality; }

    // pattern Kfactor is based on the current pattern
    float GetKFactor() { return KFactor; }
    void SetKFactor(float newFactor) { KFactor = newFactor; }

    // slot to perform the transformation 
    INVENTORY_SLOT_NUMBER GetTransformationSlot() { return transSlot; }
    void SetTransformationSlot(INVENTORY_SLOT_NUMBER curSlot) { transSlot = curSlot; }

    psItem* GetTranformationItem() { return item; }
    void SetTransformationItem(psItem* i) { item = i; }

    psItem* GetWorkItem() { return workItem; }
    void SetWorkItem(psItem* w) { workItem = w; }

    gemObject* GetTargetGem() { return gemTarget; }
    void SetTargetGem(gemObject* g) { gemTarget = g; }

    // transformation type
    int GetTransformationType() { return transType; }
    void SetTransformationType(int t) { transType = t; }
    
    WorkManager* workmanager;
    csWeakRef<gemObject> worker;
    NaturalResource* nr;
    Client* client;
    gemObject* gemTarget;
    int category;
    csVector3 position;
    psItem*    object;
    psTradeTransformations* transformation;
    psTradeProcesses* process;
    float repairAmount;
    
    uint32_t effectID;      // The id of the psEffect tied to event.
    csArray<PublishDestination> multi;
    
private:
    int resultQuantity;
    float resultQuality;
    float KFactor;
    INVENTORY_SLOT_NUMBER transSlot;
    csWeakRef<psItem> item;
    psItem* workItem;
    int transType;
};

#endif
