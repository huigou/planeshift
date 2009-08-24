/*
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef PS_GEM_HEADER
#define PS_GEM_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/mesh.h>
#include <csutil/csobject.h>

//=============================================================================
// Project Library Includes
//=============================================================================
#include "net/messages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "npcclient.h"

class gemNPCActor;
class gemNPCObject;
class npcMesh;


//-----------------------------------------------------------------------------

/** Helper class to attach a PlaneShift npc gem object to a particular mesh.
  */
class psNpcMeshAttach : public scfImplementationExt1<psNpcMeshAttach,
                                                           csObject,
                                                           scfFakeInterface<psNpcMeshAttach> >
{
public:
    SCF_INTERFACE(psNpcMeshAttach, 0, 0, 1);

    /** Setup this helper with the object we want to attach with.
     * @param object  The npcObject we want to attach to a mesh.
     */
    psNpcMeshAttach(gemNPCObject* object);

    /** Get the npc Object that the mesh has attached.
     */
    gemNPCObject* GetObject() { return object; }

private:
    gemNPCObject* object;          ///< The object that is attached to a iMeshWrapper object. 
};

//-----------------------------------------------------------------------------



class gemNPCObject : public CS::Utility::WeakReferenced
{
public:
    gemNPCObject(psNPCClient* npcclient, EID id);
    virtual ~gemNPCObject();
    
    bool InitMesh(const char *factname,const char *filename,
                  const csVector3& pos,const float rotangle, const char* sector );
    
    iMeshWrapper *GetMeshWrapper();
    void Move(const csVector3& pos, float rotangle, const char* room);
    void Move(const csVector3& pos, float rotangle, const char* room, InstanceID instance);
    
    EID GetEID() { return eid; }
    npcMesh* pcmesh;   
    
    int GetType() { return type; }
    
    const char* GetName() { return name.GetDataSafe(); }
    virtual PID GetPID() { return PID(0); }

    virtual const char* GetObjectType(){ return "Object"; }
    virtual gemNPCActor *GetActorPtr() { return NULL; }

    virtual bool IsPickable() { return false; }
    virtual bool IsVisible() { return visible; }
    virtual bool IsInvisible() { return !visible; }
    virtual void SetVisible(bool vis) { visible = vis; }

    virtual bool IsInvincible() { return invincible; }
    virtual void SetInvincible(bool inv) { invincible = inv; }

    virtual NPC *GetNPC() { return NULL; }

    virtual void SetPosition(csVector3& pos, iSector* sector = NULL, InstanceID* instance = NULL);
    virtual void SetInstance( InstanceID instance ) { this->instance = instance; }
    virtual InstanceID GetInstance(){ return instance; };

protected:
    psNPCClient *npcclient;
    
    static csRef<iMeshFactoryWrapper> nullfact;

    csString name;
    EID  eid;
    int  type;
    bool visible;
    bool invincible;
    InstanceID  instance;
    
    csRef<iEngine> engine;
};


class gemNPCActor : public gemNPCObject
{
public:

    gemNPCActor( psNPCClient* npcclient, psPersistActor& mesg);
    virtual ~gemNPCActor();
    
    psLinearMovement* pcmove;
    
    virtual PID GetPID() { return playerID; }
    virtual EID GetOwnerEID() { return ownerEID; }

    csString& GetRace() { return race; };

    virtual const char* GetObjectType(){ return "Actor"; }    
    virtual gemNPCActor *GetActorPtr() { return this; }

    virtual void AttachNPC(NPC * newNPC);
    virtual NPC *GetNPC() { return npc; }

protected:

    bool InitLinMove(const csVector3& pos,float angle, const char* sector,
                     csVector3 top, csVector3 bottom, csVector3 offset);
                     
    bool InitCharData(const char* textures, const char* equipment);        
    
    PID playerID;
    EID ownerEID;
    csString race;
    
    NPC *npc;
};


class gemNPCItem : public gemNPCObject
{
public:
    enum Flags
    {
        NONE           = 0,
        NOPICKUP       = 1 << 0
    };
    
    gemNPCItem( psNPCClient* npcclient, psPersistItem& mesg);
    virtual ~gemNPCItem();
    
    virtual const char* GetObjectType(){ return "Item"; }

    virtual bool IsPickable();

protected:
    int flags;
};

#endif
