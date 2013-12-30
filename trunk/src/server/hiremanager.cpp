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
#include <util/psdatabase.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "hiremanager.h"
#include "hiresession.h"
#include "entitymanager.h"
#include "bulkobjects/pscharacterloader.h"

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
    if (!Load())
    {
        Error1("Failed to load hire sessions");
        return false;
    }

    return true; // Server OK to continue to load.
}

bool HireManager::Load()
{
   Result result(db->Select("SELECT * FROM npc_hired_npcs"));
   if (!result.IsValid())
   {
       return false;
   }

   for (size_t i = 0; i < result.Count(); i++)
   {
       HireSession* session = new HireSession();
       if (!session->Load(result[i]))
       {
           delete session;
           return false;
       }
       
       hires.PushBack(session);

       Debug3(LOG_HIRE, session->GetOwnerPID().Unbox(), "Loaded hire session between owner %s and npc %s",
              ShowID(session->GetOwnerPID()),ShowID(session->GetHiredPID()));
   }
   return true;
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
        Error3("Failed to create npc for hire session for owner %s and hired_npc %s",
               ShowID(session->GetOwnerPID()), ShowID(session->GetHiredPID()))
        // TODO: What to do here!!
        // Delete session??
    }

    session->SetHiredNPC(hiredNPC);
    RemovePendingHire(owner);

    if (!session->Save(true)) // This is a new session.
    {
        Error3("Failed to save hire session for owner %s and hired_npc %s",
               ShowID(session->GetOwnerPID()), ShowID(session->GetHiredPID()))
    }
    
    return hiredNPC;
}

bool HireManager::ReleaseHire(gemActor* owner, gemNPC* hiredNPC)
{
    HireSession* session = GetSessionByPIDs(owner->GetPID(),hiredNPC->GetPID());
    if (!session || !owner || !hiredNPC)
    {
        return false;
    }

    if (!EntityManager::GetSingleton().DeleteActor(hiredNPC))
    {
        Error1("Failed to delete hired NPC!!!");
    }

    // Remove hire record from DB.
    session->Delete();

    hires.Delete(session);

    delete session;

    return true;
}

bool HireManager::AllowedToHire(gemActor* owner)
{
    // TODO: Include check to verify if actor satisfy the requirements to
    //       hire a NPC. This should be a math script.
    return true;
}

bool HireManager::AddHiredNPC(gemNPC* hiredNPC)
{
    Debug2(LOG_HIRE, hiredNPC->GetPID().Unbox(), "Checking hired for %s", ShowID(hiredNPC->GetPID()));

    HireSession* session = GetSessionByHirePID(hiredNPC->GetPID());
    if (!session)
    {
        return false; // This NPC was not among the hired NPCs.
    }
    Debug2(LOG_HIRE, hiredNPC->GetPID().Unbox(), "Found hired NPC %s", ShowID(hiredNPC->GetPID()));
    
    session->SetHiredNPC(hiredNPC);

    // Connect the owner of the hire to this NPC.
    hiredNPC->SetOwner(session->GetOwner());
    hiredNPC->GetCharacterData()->SetHired(true);

    return true;
}

bool HireManager::AddOwner(gemActor* owner)
{
    Debug2(LOG_HIRE, owner->GetPID().Unbox(), "Checking owner for %s", ShowID(owner->GetPID()));

    HireSession* session = GetSessionByOwnerPID(owner->GetPID());
    if (!session)
    {
        return false; // This actor was not among the owners.
    }
    Debug2(LOG_HIRE, owner->GetPID().Unbox(), "Found owner %s", ShowID(owner->GetPID()));
    
    session->SetOwner(owner);
    
    if (session->GetHiredNPC())
    {
        session->GetHiredNPC()->SetOwner(owner);
    }

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

        Debug2(LOG_HIRE, owner->GetPID().Unbox(), "New hire session created for %s", ShowID(owner->GetPID()));
    }
    
    return session;
}

HireSession* HireManager::GetPendingHire(gemActor* owner)
{
    return pendingHires.Get(owner->GetPID(),NULL);
}

HireSession* HireManager::GetSessionByHirePID(PID hiredPID)
{
    csList<HireSession*>::Iterator iter(hires);

    while (iter.HasNext())
    {
        HireSession* session = iter.Next();
        if (session->GetHiredPID() == hiredPID)
        {
            return session;
        }
    }
    return NULL;
}

HireSession* HireManager::GetSessionByOwnerPID(PID ownerPID)
{
    csList<HireSession*>::Iterator iter(hires);

    while (iter.HasNext())
    {
        HireSession* session = iter.Next();
        if (session->GetOwnerPID() == ownerPID)
        {
            return session;
        }
    }
    return NULL;
}

HireSession* HireManager::GetSessionByPIDs(PID ownerPID, PID hiredPID)
{
    csList<HireSession*>::Iterator iter(hires);

    while (iter.HasNext())
    {
        HireSession* session = iter.Next();
        if (session->GetOwnerPID() == ownerPID && session->GetHiredPID() == hiredPID)
        {
            return session;
        }
    }
    return NULL;
}

void HireManager::RemovePendingHire(gemActor* owner)
{
    pendingHires.DeleteAll(owner->GetPID());
}
