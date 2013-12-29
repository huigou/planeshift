/*
 * hiresession.cpp  creator <andersr@pvv.org>
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
#include "hiresession.h"
#include "gem.h"

HireSession::HireSession(gemActor* owner)
{
    ownerPID = owner->GetCharacterData()->GetPID();
    this->owner = owner;
}

HireSession::~HireSession()
{
}

void HireSession::SetHireType(const csString& name, const csString& npcType)
{
    hireTypeName = name;
    hireTypeNPCType = npcType;
}

const csString& HireSession::GetHireTypeName() const
{
    return hireTypeName;
}

const csString& HireSession::GetHireTypeNPCType() const
{
    return hireTypeNPCType;
}

void HireSession::SetMasterPID(PID masterPID)
{
    this->masterPID = masterPID;
}

PID HireSession::GetMasterPID() const
{
    return masterPID;
}

void HireSession::SetHiredNPC(gemNPC* hiredNPC)
{
    hiredPID = hiredNPC->GetPID();
    this->hiredNPC = hiredNPC;
}


bool HireSession::VerifyPendingHireConfigured()
{
    return (masterPID.IsValid() && !hireTypeName.IsEmpty() && !hireTypeNPCType.IsEmpty());
}

