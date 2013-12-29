/*
 * hiremanager.h  creator <andersr@pvv.org>
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <psconfig.h>
//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================
 
//====================================================================================
// Local Includes
//====================================================================================
#include "hiremanager.h"
#include "hiresession.h"
#include "entitymanager.h"


HireManager::HireManager()
{
    
}


HireManager::~HireManager()
{
    // Free all allocated sessions
    while (!hires.IsEmpty())
    {
        HireSession* session = hires.Last();
        delete session;
        hires.PopBack();
    }
}

bool HireManager::Initialize()
{

    return true; // Server OK to continue to load.
}

bool HireManager::StartHire(gemActor* owner)
{
    HireSession* session = NULL;

    // Check if a pending hire exists allready for this owner.
    session = GetPendingHire(owner);
    if (session)
    {
        return false; // Can only handle one pending hire at a time
    }
    
    // Create a new session
    session = CreateHireSession(owner);

    return (session != NULL); // True if a session was created
}

bool HireManager::SetHireType(gemActor* owner, const csString& name, const csString& npcType)
{
    HireSession* session = GetPendingHire(owner);
    if (!session)
    {
        // Indirect create a new session if no session is found.
        session = CreateHireSession(owner);
        if (!session)
        {
            return false; // Actor was not allowed to create a new session.
        }
    }

    // Update session object
    session->SetHireType(name, npcType);

    return true;
}

bool HireManager::SetHireMasterPID(gemActor* owner, PID masterPID)
{
    HireSession* session = GetPendingHire(owner);
    if (!session)
    {
        // Indirect create a new session if no session is found.
        session = CreateHireSession(owner);
        if (!session)
        {
            return false; // Actor was not allowed to create a new session.
        }
    }

    // Update session object
    session->SetMasterPID(masterPID);

    return true;
}

gemActor* HireManager::ConfirmHire(gemActor* owner)
{
    HireSession* session = GetPendingHire(owner);
    if (!session)
    {
        return NULL; // No session to confirm found.
    }

    // Are all the prerequesites configured?
    if (!session->VerifyPendingHireConfigured())
    {
        return NULL;
    }

    //Spawn hired NPC
    gemNPC* hiredNPC = EntityManager::GetSingleton().CreateHiredNPC(owner,
                                                                    session->GetMasterPID(),
                                                                    session->GetHireTypeName());
    if (!hiredNPC)
    {
        // TODO: What to do here!!
        // Delete session??
    }

    session->SetHiredNPC(hiredNPC);
    RemovePendingHire(owner);
    
    return hiredNPC;
}

bool HireManager::ReleaseHire(gemActor* owner, gemNPC* hiredNPC)
{
    
    return false;
}

bool HireManager::AllowedToHire(gemActor* owner)
{
    // TODO: Include check to verify if actor satisfy the requirements to
    //       hire a NPC. This should be a math script.
    return true;
}

HireSession* HireManager::CreateHireSession(gemActor* owner)
{
    HireSession* session = NULL;
    
    if (AllowedToHire(owner))
    {
        // Create a new session
        session = new HireSession(owner);
    
        // Store the session in the manager
        hires.PushBack(session);
        pendingHires.PutUnique(owner->GetPID(),session);
    }
    
    return session;
}

HireSession* HireManager::GetPendingHire(gemActor* owner)
{
    return pendingHires.Get(owner->GetPID(),NULL);
}

void HireManager::RemovePendingHire(gemActor* owner)
{
    pendingHires.DeleteAll(owner->GetPID());
}
