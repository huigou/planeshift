/*
 * psclientdr.h by Matze Braun <MatzeBraun@gmx.de>
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
#ifndef __PSCLIENTPERSIST_H__
#define __PSCLIENTPERSIST_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"

class psCelClient;
class ClientConnectionSet;
class MsgEntry;
class gemActor;
class MathScript;
class MathScriptVar;
class PaladinJr;
class EntityManager;
class psServerDR;

//#define USE_THREADED_DR

#ifdef USE_THREADED_DR
class DelayedDRManager : public CS::Threading::Runnable
{
public:
    DelayedDRManager(psServerDR* pDR);

    virtual void Run ();
    void Push(MsgEntry* msg, Client* c);
    void Stop();
private:
    csRefArray<MsgEntry>arr;
    csArray<Client*>arrClients;
    size_t start, end;
    CS::Threading::Mutex mutex;
    CS::Threading::RecursiveMutex mutexArray;
    CS::Threading::Condition datacondition;
    bool m_Close;
    psServerDR* serverdr;
};
#endif

class psServerDR : public MessageManager
{
public:    
    psServerDR();
    virtual ~psServerDR();

    bool Initialize(EntityManager* celbase,
                    ClientConnectionSet* clients);

    void SendPersist();

    virtual void HandleMessage(MsgEntry* me,Client *client);
    virtual void WorkOnMessage(MsgEntry* me,Client *client);

protected:

    /// If the entity was falling and stops falling, this is called.
    void HandleFallDamage(gemActor *actor,int clientnum, const csVector3& pos, iSector* sector);
    void ResetPos(gemActor* actor);

    MathScript *calc_damage;

    MathScriptVar *var_fall_height;
    MathScriptVar *var_fall_dmg;

    EntityManager       *entitymanager;
    ClientConnectionSet *clients;
    PaladinJr           *paladin;

#ifdef USE_THREADED_DR    
    csRef<DelayedDRManager> dm;
    csRef<Thread> dmThread;
#endif  

};

#endif

