/*
* actionmanager.cpp
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings <cummings.michael@gmail.com>
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

#include <psconfig.h>
//=============================================================================
// CrystalSpace Includes
//=============================================================================
#include <csutil/xmltiny.h>
#include <csgeom/math3d.h>
#include <iutil/object.h>
#include <iengine/campos.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/eventmanager.h"

#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pssectorinfo.h"

#include "net/msghandler.h"
#include "net/messages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "globals.h"
#include "actionmanager.h"
#include "gem.h"
#include "clients.h"
#include "events.h"
#include "cachemanager.h"
#include "netmanager.h"
#include "npcmanager.h"
#include "psserverchar.h"
#include "entitymanager.h"
#include "progressionmanager.h"
#include "adminmanager.h"
#include "minigamemanager.h"


psActionTimeoutGameEvent::psActionTimeoutGameEvent( ActionManager *mgr, const psActionLocation * actionLocation, size_t clientnum)
: psGameEvent(0, 1000, "psActionTimeoutGameEvent")
{
    valid = false;
    if ( mgr ) 
    {            
        actionmanager =  mgr;
        info = actionLocation;
        client = clientnum;

        valid = true;
    }        
}

psActionTimeoutGameEvent::~psActionTimeoutGameEvent()
{
    valid = false;
    actionmanager = NULL;
    info = NULL;
    client = 0;
}

void psActionTimeoutGameEvent::Trigger()
{
    psSectorInfo *sector;
    float pos_x, pos_y, pos_z, yrot;
    InstanceID instance;
    Client *clientPtr = psserver->GetNetManager()->GetClient( (int)client );   
    if ( clientPtr && info )
    {
        clientPtr->GetCharacterData()->GetLocationInWorld(instance, sector, pos_x, pos_y, pos_z, yrot );
        csVector3 clientPos(pos_x, pos_y, pos_z );

        if ( csSquaredDist::PointPoint( clientPos, info->position ) > ( info->radius * info->radius ) )
        {
            actionmanager->RemoveActiveTrigger( client, info );
            valid = false;
        }
        else
        {
            // still in range create new event
            psActionTimeoutGameEvent *newevent = new psActionTimeoutGameEvent( actionmanager, info, client );
            psserver->GetEventManager()->Push( newevent );
        }
    }
}

//----------------------------------------------------------------------------

ActionManager::ActionManager(  psDatabase *db)
{
    database = db;

    // Action Messages from client that need handling
    psserver->GetEventManager()->Subscribe( this,new NetMessageCallback<ActionManager>(this,&ActionManager::HandleMapAction), MSGTYPE_MAPACTION, REQUIRE_READY_CLIENT );
}


ActionManager::~ActionManager()
{
    // Unsubscribe from Messages
    psserver->GetEventManager()->Unsubscribe( this, MSGTYPE_MAPACTION );
    
    csHash<psActionLocation *>::GlobalIterator it (actionLocationList.GetIterator ());
    while ( it.HasNext () )
    {
        psActionLocation* actionLocation = it.Next ();
        delete actionLocation;
    }

    database = NULL;
}

void ActionManager::HandleMessage( MsgEntry *me, Client *client )
{
    // here for backwards compatibility.  no longer used
}

void ActionManager::HandleMapAction(MsgEntry *me, Client *client )
{
    psMapActionMessage msg( me );
    
    if ( !msg.valid ) 
        return;
    
    switch ( msg.command )
    {
        case psMapActionMessage::QUERY :
            HandleQueryMessage( msg.actionXML, client );
            break;
        case psMapActionMessage::SAVE :
            HandleSaveMessage( msg.actionXML, client );
            break;
        case psMapActionMessage::LIST_QUERY :
            HandleListMessage( msg.actionXML, client );
            break;
        case psMapActionMessage::DELETE_ACTION:
            HandleDeleteMessage( msg.actionXML, client );
            break;
        case psMapActionMessage::RELOAD_CACHE:
            HandleReloadMessage( client );
            break;
    }
}

void ActionManager::HandleQueryMessage( csString xml, Client *client )
{
    csString  triggerType;
    bool handled = false;

    // Search for matching locations
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, topNode, node;   
    
    if ((doc = ParseString( xml )) && ((root = doc->GetRoot())) && ((topNode = root->GetNode("location"))) 
        && (node = topNode->GetNode("triggertype"))) 
    {        
        triggerType = node->GetContentsValue();

        LoadXML(topNode);

        if ( triggerType.CompareNoCase( "SELECT" )    )   
        {
            handled = HandleSelectQuery   ( topNode, client );
        }
        
        if ( triggerType.CompareNoCase( "PROXIMITY" ) )
        {   
            handled = HandleProximityQuery( topNode, client );
        }            

        // evk: Sending to clients with security level 30+ only.
        if ( !handled && client->GetSecurityLevel() >= GM_DEVELOPER )
        {
            // Set target for later use
            client->SetMesh(meshName);
            psserver->SendSystemError(client->GetClientNum(),"You have targeted %s.",meshName.GetData());

            // Send unhandled message to client
            psMapActionMessage msg(  client->GetClientNum(), psMapActionMessage::NOT_HANDLED, xml );
            if ( msg.valid ) 
            {
                msg.SendMessage();
            }                
        }
    }
    else
    {
        Error4("Player (%s)(%s) tried to send a bogus XML string as an action message\nString was %s", 
        client->GetActor()->GetCharacterData()->GetCharName(), ShowID(client->GetActor()->GetPID()), xml.GetData());
    }
}

void ActionManager::LoadXML( csRef<iDocumentNode> topNode )
{
    csRef<iDocumentNode> node;

    // trigger type
    node = topNode->GetNode( "triggertype" );
    if ( node )
    {
        triggerType = node->GetContentsValue();
    }

    // sector
    node = topNode->GetNode( "sector" );
    if ( node )
    {
        sectorName = node->GetContentsValue();
    }

    // mesh
    node = topNode->GetNode( "mesh" );
    if ( node )
    {
        meshName = node->GetContentsValue();
    }

    csRef<iDocumentNode> posNode = topNode->GetNode( "position" );
    if ( topNode )
    {
        float posx=0.0f, posy=0.0f, posz=0.0f;

        node = posNode->GetNode( "x" );
        if ( node ) posx = node->GetContentsValueAsFloat();

        node = posNode->GetNode( "y" );
        if ( node ) posy = node->GetContentsValueAsFloat();

        node = posNode->GetNode( "z" );
        if ( node ) posz = node->GetContentsValueAsFloat();

        position = csVector3( posx, posy, posz );
    }
}    

    
bool ActionManager::HandleSelectQuery( csRef<iDocumentNode> topNode, Client *client )
{
    bool handled = false;

    psActionLocation *search = new psActionLocation();
    search->Load( topNode );

    // Search for matches
    csArray<psActionLocation *> matchMesh;  // matches on sector + mesh
    csArray<psActionLocation *> matchPoly;  // matches on sector + mesh + poly or point w/in radius
    csArray<psActionLocation *> matchPoint; // matches on sector + mesh + poly + point w/in radius
    csArray<psActionLocation *> matches;    
    psActionLocation* actionLocation;

    csHash<psActionLocation *>::Iterator iter ( actionLocationList.GetIterator( csHashCompute( triggerType + sectorName + meshName ) ) );
    while ( iter.HasNext() )
    {
        actionLocation = iter.Next ();

        switch ( actionLocation->IsMatch( search ) ) 
        {
        case 0: // No Match
            break;
        case 1: // Match on Sector + Mesh
            matchMesh.Push( actionLocation );
            break;
        case 2: // Match on Poly or Point
            matchPoly.Push( actionLocation );
            break;
        case 3: // Match on Poly and Point
            matchPoint.Push( actionLocation );
            break;
        }
    }

    // Use correct Set of Matches
    if ( matchPoint.GetSize() != 0 )
        matchPoint.TransferTo( matches );
    else if ( matchPoly.GetSize() != 0 )
        matchPoly.TransferTo( matches );
    else if ( matchMesh.GetSize() != 0 )
        matchMesh.TransferTo( matches );
    
    // ProcessMatches
    handled = ProcessMatches( matches, client );

    // cleanup
    delete search;

    return handled;
}

bool ActionManager::HandleProximityQuery( csRef<iDocumentNode> topNode, Client *client )
{
    bool handled = false;
    
    csArray<psActionLocation *> matches;    
    psActionLocation* actionLocation;
    csHash<psActionLocation *>::Iterator iter ( actionLocation_by_sector.GetIterator( csHashCompute( sectorName ) ) );
    while (iter.HasNext ())
    {
        actionLocation = iter.Next ();
        
        if ( actionLocation->triggertype == "PROXIMITY" )
        {
            csString key("");
            size_t id= actionLocation->id; 
            key.AppendFmt("%zu", id);
            size_t clientnum=client->GetClientNum();
            key.AppendFmt( "%zu", clientnum );
            csHash<psActionLocation *>::Iterator active ( activeTriggers.GetIterator( csHashCompute( key ) ) );

            if ( ( actionLocation->sectorname == sectorName ) &&
                 ( csSquaredDist::PointPoint( actionLocation->position, position ) < ( actionLocation->radius * actionLocation->radius ) ) )
            { // Found match
                if ( !active.HasNext() )
                {
                    matches.Push( actionLocation );
                }                    
            }
        }
    }

    // ProcessMatches
    handled = ProcessMatches( matches, client );

    csArray<psActionLocation *>::Iterator results( matches.GetIterator() );
    while ( results.HasNext() )
    {
        actionLocation = results.Next();
        csString key("");
        size_t id=actionLocation->id;
        size_t clientnum=client->GetClientNum();
        key.AppendFmt("%zu",id );
        key.AppendFmt( "%zu", clientnum );
        activeTriggers.Put( csHashCompute( key ), actionLocation );
        psActionTimeoutGameEvent *event = new psActionTimeoutGameEvent( this, actionLocation, client->GetClientNum() );
        psserver->GetEventManager()->Push( event );
    }
    return handled;
}

psActionLocation *ActionManager::FindAction(EID id)
{
    psActionLocation* actionLocation;
    csHash<psActionLocation *>::GlobalIterator iter ( actionLocationList.GetIterator() );

    while ( iter.HasNext() )
    {
        actionLocation = iter.Next();

        if (actionLocation->GetGemObject()->GetEID() == id)
        {
            return actionLocation;
        }
    }
    return NULL;
}

psActionLocation *ActionManager::FindActionByID( uint32 id )
{
    return actionLocation_by_id.Get(id, NULL);
}

psActionLocation* ActionManager::FindAvailableEntrances( csString entranceSector )
{
    psActionLocation* actionLocation;
    csHash<psActionLocation *>::GlobalIterator iter ( actionLocationList.GetIterator() );

    while ( iter.HasNext() )
    {
        actionLocation = iter.Next();
        if ( actionLocation->IsEntrance() && !actionLocation->IsActive() )
        {
            if( actionLocation->GetEntranceSector() == entranceSector )
            {
                return actionLocation;
            }
        }
    }
    return NULL;
}

bool ActionManager::ProcessMatches( csArray<psActionLocation *> matches, Client* client )
{
    bool handled = false;
    psActionLocation* actionLocation;

    // Call correct OperationHandler
    csArray<psActionLocation *>::Iterator results( matches.GetIterator() );
    while ( results.HasNext() )
    {
        actionLocation = results.Next();

        if( actionLocation->IsActive() )
        {
            if ( actionLocation->responsetype == "EXAMINE" )
            {
                HandleExamineOperation( actionLocation, client );
                handled = true;
            }

            if ( actionLocation->responsetype == "SCRIPT" )
            {
                HandleScriptOperation( actionLocation, client );
                handled = true;
            }
        }
    }

    return handled;
}

void ActionManager::HandleListMessage( csString xml, Client *client )
{
    if ( client->GetSecurityLevel() <= GM_LEVEL_9)
    {
        psserver->SendSystemError(client->GetClientNum(), "Access is denied. Only Admin level 9 can manage Actions.");
        return;
    }

    csString  sectorName;

    // Search for matching locations
    csRef<iDocument> doc ;    
    csRef<iDocumentNode> root, topNode, node;

    if ((doc = ParseString( xml )) && ((root = doc->GetRoot())) && ((topNode = root->GetNode("location"))) 
        && (node = topNode->GetNode("sector"))) // sector
    {
        sectorName = node->GetContentsValue();
        
        // Call proper operation
        csString responseXML("");
        responseXML.Append( "<locations>" );

        csHash<psActionLocation *>::Iterator iter ( actionLocation_by_sector.GetIterator( csHashCompute( sectorName ) ) );
        while (iter.HasNext ())
        {
            psActionLocation* actionLocation = iter.Next ();

            responseXML.Append( actionLocation->ToXML() );
        }
        responseXML.Append( "</locations>" );

        psMapActionMessage msg( client->GetClientNum(), psMapActionMessage::LIST, responseXML );
        if ( msg.valid ) 
            msg.SendMessage();
    }
    else
    {
        Error4("Player (%s)(%s) tried to send a bogus XML string as an action message\nString was %s", 
        client->GetActor()->GetCharacterData()->GetCharName(), ShowID(client->GetActor()->GetPID()), xml.GetData());
    }

}

void ActionManager::HandleSaveMessage( csString xml, Client *client )
{
    if ( client->GetSecurityLevel() <= GM_LEVEL_9)
    {
        psserver->SendSystemError(client->GetClientNum(), "Access is denied. Only Admin level 9 can manage Actions.");
        return;
    }

    // Search for matching locations
    csRef<iDocument> doc;    
    csRef<iDocumentNode> root, topNode;
    if ((doc = ParseString( xml )) && ((root = doc->GetRoot())) && ((topNode = root->GetNode("location"))))// sector
    {
        csRef<iDocumentNode> node;
        psActionLocation *action = NULL;

        node = topNode->GetNode( "id" );
        if ( !node ) return;
        csString id( node->GetContentsValue() );
    
        node = topNode->GetNode( "name" );
        if ( !node ) return;
        csString name( node->GetContentsValue() );

        node = topNode->GetNode( "sector" );
        if ( !node ) return;
        csString sectorName( node->GetContentsValue() );

        node = topNode->GetNode( "mesh" );
        if ( !node ) return;
        csString meshName( node->GetContentsValue() );

        if ( id.Length() == 0 )
        { // Add New Location
            action = new psActionLocation();
            CS_ASSERT( action != NULL );
        }
        else
        { // Update existing location
            psActionLocation *current;

            // Find Elememt
            csHash<psActionLocation *>::Iterator iter ( actionLocation_by_sector.GetIterator( csHashCompute( sectorName ) ) );
            while ( iter.HasNext() )
            {
                current = iter.Next();
                CS_ASSERT( current != NULL );

                if ( current->id == (size_t)atoi( id.GetData() ) )
                {
                    action = current;
                }
            }

            // Remove from Cache
            actionLocationList.Delete( csHashCompute( action->triggertype + action->sectorname + action->meshname ), action );
            actionLocation_by_name.Delete( csHashCompute( action->name ), action );
            actionLocation_by_sector.Delete( csHashCompute( action->sectorname ), action );
            actionLocation_by_id.Delete( action->id, action );
        }

        if ( action != NULL )
        {
            // Update DB
            if ( action->Load( topNode ) )
            {
                action->Save();

                //Update Cache
                if ( !CacheActionLocation(action) )
                {
                    Error2("Failed to populate action : \"%s\"", action->name.GetData());
                    delete action;
                }
            }
            else
            {
                Error2("Failed to load action : \"%s\"", action->name.GetData());
                delete action;
            }
        }

        csString xmlMsg;
        csString escpxml = EscpXML(sectorName);
        xmlMsg.Format("<location><sector>%s</sector></location>", escpxml.GetData() );
        HandleListMessage(xmlMsg, client );
    }
    else
    {
        Error4("Player (%s)(%s) tried to send a bogus XML string as an action message\nString was %s", 
        client->GetActor()->GetCharacterData()->GetCharName(), ShowID(client->GetActor()->GetPID()), xml.GetData());
    }
}

void ActionManager::HandleDeleteMessage( csString xml, Client *client )
{
    if ( client->GetSecurityLevel() <= GM_LEVEL_9)
    {
        psserver->SendSystemError(client->GetClientNum(), "Access is denied. Only Admin level 9 can manage Actions.");
        return;
    }

    // Search for matching locations
    csRef<iDocument>     doc;    
    csRef<iDocumentNode> root, topNode, node;

    if ((doc = ParseString( xml )) && ((root = doc->GetRoot())) && ((topNode = root->GetNode("location"))) 
        && (node = topNode->GetNode("id")))     
    {
        csString id( node->GetContentsValue() );

        // Find Elememt
        psActionLocation *current, *actionLocation = NULL;
        csHash<psActionLocation *>::GlobalIterator iter ( actionLocationList.GetIterator() );
        while ( iter.HasNext() )
        {
            current = iter.Next();
            CS_ASSERT( current != NULL );

            if ( current->id == (size_t)atoi( id.GetData() ) )
            {
                actionLocation = current;
            }
        }

        // No Match
        if ( !actionLocation ) return;

        csString sectorName = actionLocation->sectorname;

        // Update DB
        if ( actionLocation->Delete() )
        {
            // Remove from Cache
            actionLocationList.Delete( csHashCompute( actionLocation->triggertype + actionLocation->sectorname + actionLocation->meshname ), actionLocation );
            actionLocation_by_name.Delete( csHashCompute( actionLocation->name ), actionLocation );
            actionLocation_by_sector.Delete( csHashCompute( actionLocation->sectorname ), actionLocation );
            actionLocation_by_id.Delete( actionLocation->id, actionLocation );

            if(actionLocation->gemAction != NULL)
                delete actionLocation->gemAction;
            delete actionLocation;
        }

        csString xmlMsg;
        csString escpxml = EscpXML(sectorName);
        xmlMsg.Format("<location><sector>%s</sector></location>", escpxml.GetData() );
        HandleListMessage(xmlMsg, client );        
    }
    else
    {
        Error4("Player (%s)(%s) tried to send a bogus XML string as an action message\nString was %s", 
        client->GetActor()->GetCharacterData()->GetCharName(), ShowID(client->GetActor()->GetPID()), xml.GetData());
    }

    
}

void ActionManager::HandleReloadMessage(Client * client)
{
    if ( client->GetSecurityLevel() <= GM_LEVEL_9)
    {
        psserver->SendSystemError(client->GetClientNum(), "Access is denied. Only Admin level 9 can manage Actions.");
        return;
    }

    csHash<psActionLocation *>::GlobalIterator it (actionLocationList.GetIterator ());
    while (it.HasNext ())
    {
        psActionLocation* actionLocation = it.Next ();
        if(actionLocation->gemAction != NULL)
            delete actionLocation->gemAction;
        delete actionLocation;
    }

    // reset game sessions
    psserver->GetMiniGameManager()->ResetAllGameSessions();

    actionLocationList.DeleteAll();
    actionLocation_by_sector.DeleteAll();
    actionLocation_by_name.DeleteAll();
    actionLocation_by_id.DeleteAll();

    RepopulateActionLocations();
}

void ActionManager::HandleExamineOperation( psActionLocation* action, Client *client )
{
    // Assume you are allowed in unless there is a script that returns false
    bool allowedEntry = true;

    // Create Entity on Client
    action->Send( client->GetClientNum() );

    // Set as the target
    client->SetTargetObject( action->GetGemObject(), true );

    // Find the real item if there exist
    gemItem* realItem = action->GetRealItem();

    // If no enter check is specified, do nothing
    if (action->GetEnterScript())
    {
        MathEnvironment env;
        env.Define("Actor", client->GetActor());

        allowedEntry = action->GetEnterScript()->Evaluate(&env) != 0.0;
    }

    // Invoke Interaction menu
    uint32_t options = psGUIInteractMessage::EXAMINE;

    // Is action location an entrance
    if (action->IsEntrance())
    {
        // If there is a real item associated with the entrance must be a lock
        if (realItem)
        {
            // Is there a security lock
            if (realItem->IsSecurityLocked())
            {
                options |= psGUIInteractMessage::ENTERLOCKED;
            }

            // Is there a lock
            if (realItem->IsLockable())
            {
                // Is it locked
                if(realItem && realItem->IsLocked())
                    options |= psGUIInteractMessage::UNLOCK;
                else
                    options |= psGUIInteractMessage::LOCK;
            }
        }
        // Otherwise walk right in if script allowed
        else if (allowedEntry)
        {
            options |= psGUIInteractMessage::ENTER;
        }
    }

    // Or a game board
    else if (action->IsGameBoard())
    {
        options |= psGUIInteractMessage::PLAYGAME;
    }
    // Everything else
    else if (realItem)
    {
        // Container items can combine
        if(realItem->IsContainer()) 
            options |= psGUIInteractMessage::COMBINE;
        if(realItem->IsConstructible())
            options |= psGUIInteractMessage::CONSTRUCT;
        // All other action location items can be used
        options |= psGUIInteractMessage::USE;
    }
    options |= psGUIInteractMessage::CLOSE;

    psGUIInteractMessage interactMsg( client->GetClientNum(), options );
    interactMsg.SendMessage();
}



void ActionManager::HandleScriptOperation( psActionLocation* action, Client *client )
{
    // if no event is specified, do nothing
    if ( action->response.Length() > 0)
    {
        ProgressionScript *script = psserver->GetProgressionManager()->FindScript(action->response);
        if (!script)
        {
            Error2("Failed to find progression script \"%s\"", action->response.GetData());
            return;
        }

        // Find the real item if there exist
        gemItem* realItem = action->GetRealItem();

        MathEnvironment env;
        env.Define("Actor", client->GetActor());
        if (realItem)
            env.Define("Item", realItem->GetItem());
        script->Run(&env);
    }
}

void ActionManager::RemoveActiveTrigger( size_t clientnum, const psActionLocation *actionLocation )
{
    csString key("");
    size_t id= actionLocation->id;
    key.AppendFmt("%zu", id);
    key.AppendFmt("%zu",  clientnum );

    csHash<psActionLocation *>::Iterator active ( activeTriggers.GetIterator( csHashCompute( key ) ) );

    while ( active.HasNext() )
        activeTriggers.Delete( csHashCompute( key ), active.Next() );
}

bool ActionManager::RepopulateActionLocations(psSectorInfo *sectorinfo)
{
    unsigned int currentrow;
    psActionLocation* newaction;
    csString query;
    
    if ( sectorinfo )
        query.Format("SELECT al.*, master.triggertype master_triggertype, master.responsetype master_responsetype, master.response master_response FROM action_locations al LEFT OUTER JOIN action_locations master ON al.master_id = master.id WHERE al.sectorname='%s'", sectorinfo->name.GetData());
    else
        query = "SELECT al.*, master.triggertype master_triggertype, master.responsetype master_responsetype, master.response master_response FROM action_locations al LEFT OUTER JOIN action_locations master ON al.master_id = master.id";
    
    Result result( db->Select(query) );

    if (!result.IsValid())
        return false;

    for ( currentrow = 0; currentrow < result.Count(); currentrow++)
    {
        newaction = new psActionLocation();
        CS_ASSERT(newaction != NULL);

        if ( newaction->Load( result[currentrow] ) )
        {
                if ( !CacheActionLocation(newaction) )
                {
                    Error2("Failed to populate action : \"%s\"", newaction->name.GetData());
                    delete newaction;
                }
        }
        else
        {
            Error2("Failed to find load action : \"%s\"", newaction->name.GetData());
            delete newaction;
        }
    }
    return true;
}


bool ActionManager::CacheActionLocation(psActionLocation* action)
{

    // Parse the action response string
    if ( !action->ParseResponse() )
    {
        Error2("Failed to parse response string for action : \"%s\"", action->name.GetData());
        return false;
    }

    // Create location and add to hash lists
    if ( EntityManager::GetSingleton().CreateActionLocation( action, false ) )
    {
        actionLocationList.Put( csHashCompute( action->triggertype + action->sectorname + action->meshname ), action);
        actionLocation_by_name.Put( csHashCompute( action->name ), action);
        actionLocation_by_sector.Put( csHashCompute( action->sectorname ), action);
        actionLocation_by_id.Put( action->id, action);
    }
    else
    {
        Error2("Failed to create action : \"%s\"", action->name.GetData());
        return false;
    }
    return true;
}

