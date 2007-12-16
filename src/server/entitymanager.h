/*
 * entitymanager.h
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __EntityManager_H__
#define __EntityManager_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "engine/celbase.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"
#include "commandmanager.h"

class ClientConnectionSet;
class psServerDR;
class psServer;
class psDatabase;
class psWorld;
class psItem;
class GEMSupervisor;
class UserManager;
class gemObject;
class gemNPC;
class psActionLocation;
class psCharacter;
class psSectorInfo;

struct iPcLinearMovement;
struct iPcCharacterData;
struct iPcProximityList;


struct psAffinityAttribute
{
public:
    csString Attribute;
    csString Category;
};

struct psFamiliarType
{
public:
    PS_ID Id;
    csString Name;
    csString Type;
    csString Lifecycle;
    csString AttackTool;
    csString AttackType;
    csString MagicalAffinity;
};



/// Manages CEL entities on the server
class EntityManager : public CelBase, public MessageManager, public Singleton<EntityManager>
{
public:
    EntityManager();
    virtual ~EntityManager();

    bool Initialize(iObjectRegistry* object_reg,
                    ClientConnectionSet* clients,
                    UserManager *usermanager);

    bool LoadMap (const char* mapname);    

    iSector* FindSector(const char *name);
    
    bool CreatePlayer(Client* client);
    bool DeletePlayer(Client* client);

    PS_ID CopyNPCFromDatabase(int master_id, float x, float y, float z, float angle, const csString & sector, int instance, const char *firstName = NULL, const char *lastName = NULL);
    PS_ID CreateNPC(int NPCID, bool updateProxList = true);
    PS_ID CreateNPC(psCharacter *chardata, bool updateProxList = true);
    PS_ID CreateNPC(psCharacter *chardata, int instance, csVector3 pos, iSector* sector, float yrot, bool updateProxList = true);

    gemNPC *CreateFamiliar(gemActor *owner);
    gemNPC *CreatePet( Client* client, int familiarid );
    gemNPC *CloneNPC( psCharacter *chardata );

    bool CreateActionLocation(psActionLocation *instance, bool transient);

    gemObject *CreateItem(psItem *& iteminstance, bool transient);
    gemObject *MoveItemToWorld(psItem       *keyItem,
                               int           instance,
                               psSectorInfo *sectorinfo,
                               float         loc_x,
                               float         loc_y,
                               float         loc_z,
                               float         loc_yrot,
                               psCharacter  *owner,
                               bool          transient);

    virtual void HandleMessage(MsgEntry* me,Client *client);

    bool RemoveActor(gemObject *actor);

    csPtr<iCelEntity> CreateEntity( const char* propclass );

    void SetReady(bool flag);
    bool IsReady() { return ready; }
    bool HasBeenReady() { return hasBeenReady; }
    GEMSupervisor *GetGEM() { return gem; }
    iEngine *GetEngine() { return engine; }

    ClientConnectionSet *GetClients() { return clients; };
    psWorld* GetWorld() { return gameWorld; }

    void SendMovementInfo(int cnum);

    void Teleport( gemObject *subject, gemObject *dest);

protected:
    csHash<psAffinityAttribute *> affinityAttributeList;
    csHash<psFamiliarType*, PS_ID> familiarTypeList;

    bool CreateRoom (const char* name, const char* mapfile);
    csPtr<iCelEntity> CreateActor(const char* factname, 
                            const csVector3& pos, 
                            float angle,
                            iSector* sector, 
                            int playerID,
                            int objectID,
                            int clientID);        

    bool InitMesh(iPcMesh *pcmesh,const char* factname,const csVector3& pos,const iSector* sector);
    bool InitLinMove(iPcMesh *pcmesh,iPcLinearMovement *pcmove, const csVector3& pos,float angle,const iSector* sector);
    bool SamePos(gemObject * actor, iSector * sector, const csVector3 & point);

    void HandleUserAction(MsgEntry* me);    
    
    void HandleWorld( MsgEntry* me );
    void HandleActor( MsgEntry* me );
    void HandleAllRequest( MsgEntry* me );
    
    bool SendActorList(Client *client);

    void loadFamiliarTypes();
    void loadFamiliarAffinityAttributes();
    PS_ID getMasterFamiliarID( psCharacter *charData );
    int calculateFamiliarAffinity(  psCharacter * chardata, size_t type, size_t lifecycle, size_t attacktool, size_t attacktype  );


    bool ready;
    bool hasBeenReady;
    psServerDR* serverdr;
    ClientConnectionSet* clients;
    psDatabase* database;
    UserManager* usermanager;
    GEMSupervisor* gem;
    iEngine *engine;    
    psWorld* gameWorld;
    
    psMovementInfoMessage* moveinfomsg;
};

#endif

