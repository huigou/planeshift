/*
* dummysndmngr.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
*
* Copyright (C) 2001-2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <iutil/plugin.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "dummysndmngr.h"
#include "dummysndctrl.h"


SCF_IMPLEMENT_FACTORY(DummySoundManager)


DummySoundManager::DummySoundManager(iBase* parent): scfImplementationType(this, parent)
{
    defaultSndCtrl = new DummySoundControl(0, iSoundControl::NORMAL);
    return;
}

DummySoundManager::~DummySoundManager()
{
    delete defaultSndCtrl;
    return;
}

bool DummySoundManager::Initialize(iObjectRegistry* objReg)
{
    return true;
}

bool DummySoundManager::InitializeSectors()
{
    return true;
}

void DummySoundManager::LoadActiveSector(const char* sector)
{
    return;
}

void DummySoundManager::ReloadSectors()
{
    return;
}

void DummySoundManager::UnloadActiveSector()
{
    return;
}

iSoundControl* DummySoundManager::AddSndCtrl(int ctrlID, int type)
{
    return defaultSndCtrl;
}

void DummySoundManager::RemoveSndCtrl(iSoundControl* sndCtrl)
{
    return;
}

iSoundControl* DummySoundManager::GetSndCtrl(int ctrlID)
{
    return defaultSndCtrl;
}

iSoundControl* DummySoundManager::GetMainSndCtrl()
{
    return defaultSndCtrl;
}

bool DummySoundManager::AddSndQueue(int queueID, iSoundControl* sndCtrl)
{
    return true;
}

void DummySoundManager::RemoveSndQueue(int queueID)
{
    return;
}

bool DummySoundManager::PushQueueItem(int queueID, const char* fileName)
{
    return true;
}

void DummySoundManager::SetCombatStance(int newCombatStance)
{
    combat = newCombatStance;
}

int DummySoundManager::GetCombatStance() const
{
    return combat;
}

void DummySoundManager::SetPlayerMovement(csVector3 playerPos, csVector3 playerVelocity)
{
    position = playerPos;
}


csVector3 DummySoundManager::GetPosition() const
{
    return position;
}

void DummySoundManager::SetTimeOfDay(int newTimeOfDay)
{
    time = newTimeOfDay;
}

int DummySoundManager::GetTimeOfDay() const
{
    return time;
}

void DummySoundManager::SetWeather(int newWeather)
{
    weather = newWeather;
}

int DummySoundManager::GetWeather() const
{
    return weather;
}

void DummySoundManager::SetEntityState(int state, iMeshWrapper* mesh, bool forceChange)
{
    return;
}

void DummySoundManager::SetLoopBGMToggle(bool toggle)
{
    loopToggle = toggle;
}

bool DummySoundManager::IsLoopBGMToggleOn()
{
    return loopToggle;
}

void DummySoundManager::SetCombatMusicToggle(bool toggle)
{
    combatToggle = toggle;
}

bool DummySoundManager::IsCombatMusicToggleOn()
{
    return combatToggle;
}

void DummySoundManager::SetListenerOnCameraToggle(bool toggle)
{
    listenerToggle = toggle;
}

bool DummySoundManager::IsListenerOnCameraToggleOn()
{
    return listenerToggle;
}

void DummySoundManager::SetChatToggle(bool toggle)
{
    chatToggle = toggle;
}

bool DummySoundManager::IsChatToggleOn()
{
    return chatToggle;
}

bool DummySoundManager::IsSoundValid(uint soundID) const
{
    return false;
}

uint DummySoundManager::PlaySound(const char* fileName, bool loop, iSoundControl* &ctrl)
{
    return 1;
}

uint DummySoundManager::PlaySound(const char* fileName, bool loop, iSoundControl* &ctrl, csVector3 pos, csVector3 dir, float minDist, float maxDist)
{
    return 1;
}

uint DummySoundManager::PlaySong(csRef<iDocument> musicalSheet, const char* instrument, float errorRate,
        iSoundControl* ctrl, csVector3 pos, csVector3 dir)
{
    return 1;
}

bool DummySoundManager::StopSound(uint soundID)
{
    return true;
}

bool DummySoundManager::SetSoundSource(uint soundID, csVector3 position)
{
    return true;
}

void DummySoundManager::Update()
{
    return;
}

void DummySoundManager::UpdateListener(iView* view)
{
    return;
}
