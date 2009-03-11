/*
* psactionlocationinfo.h
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
* Description : Container for the action_locations db table.
*
*/
#ifndef __PSACTIONLOCATION_H__
#define __PSACTIONLOCATION_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/parray.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/poolallocator.h"
#include "util/psconst.h"

#include "../iserver/idal.h"

//=============================================================================
// Local Includes
//=============================================================================

class CacheManager;
class gemActionLocation;
class gemItem;
struct iDocumentNode;
class MathExpression;

/**
 * This huge class stores all the properties of any object
 * a player can have in the game.  All stats, bonuses, maluses,
 * magic stat alterations, combat properties, spell effects,
 * and so forth are all stored here.
 */
class psActionLocation
{
public:
    psActionLocation ();
    ~psActionLocation ();

    bool Load( iResultRow& row );
    bool Load( csRef<iDocumentNode> root );
    bool Save( );
    bool Delete( );
    csString ToXML() const;

    int IsMatch( psActionLocation *compare );

    void SetGemObject( gemActionLocation *gemAction );
    gemActionLocation *GetGemObject( void );

    gemItem * GetRealItem();

    void GetLocationInWorld(const char **sectorname, float &loc_x, float &loc_y, float &loc_z, float &loc_yrot);
    //void SetLocationInWorld(psSectorInfo *sectorinfo,float loc_x,float loc_y,float loc_z,float loc_yrot);
    void Send( int clientnum);

    // Returns the result of the XML parsing of the action lcoation reponse string and
    //  the setting of the action location member variables
    bool ParseResponse();

    /// Returns instance ID of referenced in action location response string
    ///  This is either a container ID or a lock ID
    InstanceID GetInstanceID() const { return instanceID; }
    void SetInstanceID(InstanceID newID) { instanceID = newID; }

    /// Returns the enter script in entrance action location response string
    MathExpression *GetEnterScript() const { return enterScript; }

    /// Returns the entrance type in entrance action location response string
    csString GetEntranceType() const { return entranceType; }
    void SetEntranceType(csString newType) { entranceType = newType; }

    /// Returns true if this action location is a minigame board
    bool IsGameBoard() const { return isGameBoard; }

    /// Returns true if this action location is an entrance
    bool IsEntrance() const { return isEntrance; }
    void SetIsEntrance(bool flag) { isEntrance = flag; }

    /// Returns true if this action location is an lockable entrance
    bool IsLockable() const { return isLockable; }
    void SetIsLockable(bool flag) { isLockable = flag; }

    /// Returns true if this action location is a container
    bool IsContainer() const { return isContainer; }

    /// Returns true if this action location has return tag
    bool IsReturn() const { return isReturn; }

    /// Returns true if this action location is actaive
    bool IsActive() const { return isActive; }
    void SetActive(bool flag) { isActive = flag; }

    /// Returns action location description 
    csString GetDescription() const { return description; }
    void SetDescription(csString newDescription) { description =  newDescription; }

    /// Returns or sets entrance location memebers
    csVector3 GetEntrancePosition() const { return entrancePosition; }
    void SetEntrancePosition(csVector3 newPosition) { entrancePosition = newPosition; }
    float GetEntranceRotation() const { return entranceRot; }
    void SetEntranceRotation(float newRot) { entranceRot = newRot; }
    csString GetEntranceSector() const { return entranceSector; }
    void SetEntranceSector(csString newSector) { entranceSector = newSector; }

    /// Returns or sets return location memebers
    csVector3 GetReturnPosition() const { return returnPosition; }
    void SetReturnPosition(csVector3 newPosition) { returnPosition = newPosition; }
    float GetReturnRotation() const { return returnRot; }
    void SetReturnRotation(float newRot) { returnRot = newRot; }
    csString GetReturnSector() const { return returnSector; }
    void SetReturnSector(csString newSector) { returnSector = newSector; }

    /// Sets
    void SetName(csString newname) { name = newname; }
    void SetSectorName(csString newsector) { sectorname = newsector; }
    void SetMeshName(csString newmeshname) { meshname = newmeshname; }
    void SetTriggerType(csString newtrigger) { triggertype = newtrigger; }
    void SetResponseType(csString newresponsetype) { responsetype = newresponsetype; }
    void SetResponse(csString newresponse) { response = newresponse; }
    void SetPosition(csVector3 newposition) { position = newposition; }
    void SetRadius(float newradius) { radius = newradius; }

    ///  The new operator is overriden to call PoolAllocator template functions
    void *operator new(size_t);
    ///  The delete operator is overriden to call PoolAllocator template functions
    void operator delete(void *);

    uint32 id;
    size_t master_id;

    csString name;
    csString sectorname; // Sector Where item is located
    csString meshname; // Mesh name
    csString polygon; // Required if multiple mesh of same name in area
    csVector3 position; // x,y,z coordinates required for entrances
    float radius;
    csString triggertype;
    csString responsetype;
    csString response;

    gemActionLocation *gemAction;

private:
    /// Static reference to the pool for all psItem objects
    static PoolAllocator<psActionLocation> actionpool;

    /// Sets up member variables for different response strings.
    ///  You can add code here when new response string XML is needed or parse it dynamically
    ///  @param action The action location to which you want to setup
    ///  @param xxxxxxxxNode The action location response XML node
    void SetupEntrance(csRef<iDocumentNode> entranceNode);
    void SetupReturn(csRef<iDocumentNode> returnNode);
    void SetupContainer(csRef<iDocumentNode> containerNode);
    void SetupGameboard(csRef<iDocumentNode> boardNode);
    void SetupDescription(csRef<iDocumentNode> descriptionNode);

    /// Flag indicating that this action location is a container
    bool isContainer;

    /// Flag indicating that this action location is a minigame
    bool isGameBoard;

    /// Flag indicating that this action location is an entrance
    bool isEntrance;

    /// Flag indicating that this action location is lockable
    bool isLockable;

    // Flag indicator that location will display menu
    bool isActive;

    // Flag indicator that location has return tag
    bool isReturn;

    ///  This is either a container ID or a lock ID
    InstanceID instanceID;

    /// String containing the entrance type
    csString entranceType;

    // Possition stuff for entrances
    csVector3 entrancePosition;
    float entranceRot;
    csString entranceSector;

    /// String containing the entrance script
    ///   Script return value of 0 indicates no entry
    MathExpression *enterScript;

    // Possition stuff for returns
    csVector3 returnPosition;
    float returnRot;
    csString returnSector;

    /// String containing response description
    csString description;


    //DB Helper Functions
    unsigned int Insert      ( const char *table, const char **fieldnames, psStringArray& fieldvalues );
    bool         UpdateByKey ( const char *table, const char *idname, const char *idvalue, const char **fieldnames, psStringArray& fieldvalues );
    bool         DeleteByKey ( const char *table, const char *idname, const char *idvalue );
};



#endif

