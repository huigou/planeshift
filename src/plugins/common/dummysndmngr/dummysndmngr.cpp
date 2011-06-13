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

void DummySoundManager::SetPosition(csVector3 playerPos)
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

void DummySoundManager::PlaySound(const char* fileName, bool loop, iSoundControl* &ctrl)
{
    return;
}

void DummySoundManager::PlaySound(const char* fileName, bool loop, iSoundControl* &ctrl, csVector3 pos, csVector3 dir, float minDist, float maxDist)
{
    return;
}

bool DummySoundManager::StopSound(const char* fileName)
{
    return true;
}

bool DummySoundManager::SetSoundSource(const char* fileName, csVector3 position)
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
