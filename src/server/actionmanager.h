/*
* actionmanager.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*                  Michael Cummings <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : server manager for clickable map object actions
*
*/
#ifndef __ACTIONMANAGER_H__
#define __ACTIONMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/gameevent.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class


class psDatabase;
class SpawnManager;
class psServer;
class psDatabase;
class ClientConnectionSet;
class psActionLocation;
class ActionManager;
class psSectorInfo;
class Client;

/** Time out event on interacting with an action item.
 */
class psActionTimeoutGameEvent : public psGameEvent
{
public:    
    psActionTimeoutGameEvent(ActionManager *mgr,
                     const psActionLocation *actionLocation,
                     size_t  client);

    ~psActionTimeoutGameEvent();
    
    /** Abstract event processing function.
    */
    virtual void Trigger();  
    virtual bool IsValid() { return valid; }
    
protected:
    ActionManager *actionmanager;
    bool valid;
    
    size_t                       client;     
    const  psActionLocation      *info;         
};


//----------------------------------------------------------------------------

/** Handles the map interaction system.
  * Used to populate/update/change current action locations.
  */
class ActionManager : public MessageManager
{
public:

    ActionManager( psDatabase *db );

    virtual ~ActionManager();

    /** Loads cache from action_location table in db
      * @param sectorinfo The sector to repopulate. Null means all sectors.
      */
    bool RepopulateActionLocations(psSectorInfo *sectorinfo = 0);

    /** Loads cache with given action location
      * @param action The action location to which you want to load
      */
    bool CacheActionLocation(psActionLocation* action);

    /** Delegates MsgEntry to correct MessageHandler
      *
      * @param pMsg            The MsgEntry to crack.
      * @param client      The client that sent the message.
      */
    void HandleMessage( MsgEntry *pMsg, Client *client );

    /** Processes psMapActionMessages
      *
      * @param msg            The message to process
      * @param client      The client that sent the message.
      */
    void HandleMapAction( MsgEntry *msg, Client *client );

    void RemoveActiveTrigger( size_t clientnum, const psActionLocation *actionLocation );

    /** Finds an ActionLocation from it's CEL Entity ID
      *
      * @param id The id of the cel entity to find.
      */
    psActionLocation *FindAction(EID id);

    /** Finds an ActionLocation from the action ID
      *
      * @param id The id of the action location.
      */
    psActionLocation *FindActionByID( uint32 id );

    /** Finds an inactive entrance action location in the specified target sector map
      *
      * @param entranceSector The entrance teleport target sector string to qualify the search.
      */
    psActionLocation* FindAvailableEntrances( csString entranceSector );

protected:

    // Message Handlers

    /** Handles Query messages from client.
    *
    * @param xml xml containing query parameters.
    * @param client      The client that sent the message.
    */
    void HandleQueryMessage( csString xml, Client *client );
    void LoadXML( csRef<iDocumentNode> topNode );
    bool HandleSelectQuery( csRef<iDocumentNode> topNode, Client *client );
    bool HandleProximityQuery( csRef<iDocumentNode> topNode, Client *client );
    bool ProcessMatches( csArray<psActionLocation *> matches, Client *client );

    /** Handles Save messages from client.
    *
    * @param xml xml containing query parameters.
    * @param client      The client that sent the message.
    */
    void HandleSaveMessage( csString xml, Client *client );

    /** Handles List messages from client.
    *
    * @param xml xml containing query parameters.
    * @param client      The client that sent the message.
    */
    void HandleListMessage( csString xml, Client *client );

    /** Handles Delete messages from client.
    *
    * @param xml xml containing query parameters.
    * @param client      The client that sent the message.
    */
    void HandleDeleteMessage( csString xml, Client *client );

    /** Handles Reload messages from client.
    *
    * @param client      The client that sent the message.
    */
    void HandleReloadMessage( Client *client);

    // Operation Handlers

    /** Handles Examine Operation for a action location.
    *
    * @param action The action that is to be performed.
    * @param client      The client that sent the message.
    */
    void HandleExamineOperation( psActionLocation* action, Client *client );
    void HandleScriptOperation ( psActionLocation* action, Client *client );

    // Current action location data
    csString triggerType;
    csString sectorName;
    csString meshName;
    csVector3 position;

    // ActionItems
    //psActionItemInfo *GetActionItemByID(unsigned int id);
    //psActionItemInfo *GetActionItemByName(const char *name);

    psDatabase              *database;
    csHash<psActionLocation *> actionLocationList;
    csHash<psActionLocation *> actionLocation_by_name;
    csHash<psActionLocation *> actionLocation_by_sector;
    csHash<psActionLocation *, uint32> actionLocation_by_id;
    csHash<psActionLocation *> activeTriggers;

};

#endif
