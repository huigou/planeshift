/*
 * pawsconfigsound.cpp - Author: Christian Svensson
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// CS INCLUDES
#include <psconfig.h>
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/vfs.h>

// COMMON INCLUDES
#include "util/log.h"

// CLIENT INCLUDES
#include "iclient/isoundmngr.h"
#include "../globals.h"

// PAWS INCLUDES
#include "pawsconfigsound.h"
#include "paws/pawsmanager.h"
#include "paws/pawscrollbar.h"
#include "paws/pawscheckbox.h"

pawsConfigSound::pawsConfigSound()
{
    loaded= false;
}

bool pawsConfigSound::Initialize()
{
    if ( ! LoadFromFile("configsound.xml"))
        return false;

    return true;
}

bool pawsConfigSound::PostSetup()
{
    generalVol = (pawsScrollBar*)FindWidget("generalVol");
    if(!generalVol)
        return false;

    musicVol = (pawsScrollBar*)FindWidget("musicVol");
    if(!musicVol)
        return false;

    ambientVol = (pawsScrollBar*)FindWidget("ambientVol");
    if(!ambientVol)
        return false;

    actionsVol = (pawsScrollBar*)FindWidget("actionsVol");
    if(!actionsVol)
        return false;

    guiVol = (pawsScrollBar*)FindWidget("guiVol");
    if(!guiVol)
        return false;

    voicesVol = (pawsScrollBar*)FindWidget("voicesVol");
    if(!voicesVol)
        return false;

    voices = (pawsCheckBox*)FindWidget("voices");
    if(!voices)
        return false;

    gui = (pawsCheckBox*)FindWidget("gui");
    if(!gui)
        return false;

    ambient = (pawsCheckBox*)FindWidget("ambient");
    if(!ambient)
        return false;

    actions = (pawsCheckBox*)FindWidget("actions");
    if(!actions)
        return false;

    music = (pawsCheckBox*)FindWidget("music");
    if(!music)
        return false;

    muteOnFocusLoss = (pawsCheckBox*) FindWidget("muteOnFocusLoss");
    if (!muteOnFocusLoss)
        return false;

    loopBGM = (pawsCheckBox*) FindWidget("loopBGM");
    if (!loopBGM)
        return false;

    combatMusic = (pawsCheckBox*) FindWidget("combatMusic");
    if (!combatMusic)
        return false;

    generalVol->SetMaxValue(100);
    generalVol->SetTickValue(10);
    generalVol->EnableValueLimit(true);

    musicVol->SetMaxValue(100);
    musicVol->SetTickValue(10);
    musicVol->EnableValueLimit(true);

    ambientVol->SetMaxValue(100);
    ambientVol->SetTickValue(10);
    ambientVol->EnableValueLimit(true);

    actionsVol->SetMaxValue(100);
    actionsVol->SetTickValue(10);
    actionsVol->EnableValueLimit(true);

    guiVol->SetMaxValue(100);
    guiVol->SetTickValue(10);
    guiVol->EnableValueLimit(true);

    voicesVol->SetMaxValue(100);
    voicesVol->SetTickValue(10);
    voicesVol->EnableValueLimit(true);

    return true;
}
bool pawsConfigSound::LoadConfig()
{
    csRef<iSoundManager> pssnd = psengine->GetSoundManager(); // was iSoundManager*

    generalVol->SetCurrentValue(pssnd->GetVolume()*100,false);
    musicVol->SetCurrentValue(pssnd->GetMusicVolume()*100,false);
    ambientVol->SetCurrentValue(pssnd->GetAmbientVolume()*100,false);
    guiVol->SetCurrentValue(PawsManager::GetSingleton().GetVolume()*100,false);
    voicesVol->SetCurrentValue(pssnd->GetVoicesVolume()*100,false);
    actionsVol->SetCurrentValue(pssnd->GetActionsVolume()*100,false);

    ambient->SetState(pssnd->PlayingAmbient());
    actions->SetState(pssnd->PlayingActions());
    music->SetState(pssnd->PlayingMusic());
    gui->SetState(PawsManager::GetSingleton().PlayingSounds());
    voices->SetState(pssnd->PlayingVoices());

    muteOnFocusLoss->SetState(psengine->GetMuteSoundsOnFocusLoss());
    loopBGM->SetState(pssnd->LoopBGM());
    combatMusic->SetState(pssnd->PlayingCombatMusic());

    loaded= true;
    dirty = false;
    return true;
}

bool pawsConfigSound::SaveConfig()
{
    csString xml;
    xml = "<sound>\n";
    xml.AppendFmt("<ambient on=\"%s\" />\n",
                     ambient->GetState() ? "yes" : "no");
    xml.AppendFmt("<actions on=\"%s\" />\n",
                     actions->GetState() ? "yes" : "no");
    xml.AppendFmt("<music on=\"%s\" />\n",
                     music->GetState() ? "yes" : "no");
    xml.AppendFmt("<gui on=\"%s\" />\n",
                     gui->GetState() ? "yes" : "no");
    xml.AppendFmt("<voices on=\"%s\" />\n",
                     voices->GetState() ? "yes" : "no");
    xml.AppendFmt("<volume value=\"%d\" />\n",
                     int(generalVol->GetCurrentValue()));
    xml.AppendFmt("<musicvolume value=\"%d\" />\n",
                     int(musicVol->GetCurrentValue()));
    xml.AppendFmt("<ambientvolume value=\"%d\" />\n",
                     int(ambientVol->GetCurrentValue()));
    xml.AppendFmt("<actionsvolume value=\"%d\" />\n",
                     int(actionsVol->GetCurrentValue()));
    xml.AppendFmt("<guivolume value=\"%d\" />\n",
                     int(guiVol->GetCurrentValue()));
    xml.AppendFmt("<voicesvolume value=\"%d\" />\n",
                     int(voicesVol->GetCurrentValue()));
    xml.AppendFmt("<muteonfocusloss on=\"%s\" />\n",
                     muteOnFocusLoss->GetState() ? "yes" : "no");
    xml.AppendFmt("<loopbgm on=\"%s\" />\n",
                     loopBGM->GetState() ? "yes" : "no");
    xml.AppendFmt("<combatmusic on=\"%s\" />\n",
                     combatMusic->GetState() ? "yes" : "no");
    xml += "</sound>\n";

    dirty = false;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/sound.xml",
                                         xml,xml.Length());
}

void pawsConfigSound::SetDefault()
{
    psengine->LoadSoundSettings(true);
    LoadConfig();
}

bool pawsConfigSound::OnScroll(int scrollDir,pawsScrollBar* wdg)
{
    dirty = true;
    if(wdg == generalVol && loaded)
    {
        if(generalVol->GetCurrentValue() < 1)
            generalVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->SetVolume(generalVol->GetCurrentValue()/100);
        return true;
    }
    else if(wdg == musicVol && loaded)
    {
        if(musicVol->GetCurrentValue() < 1)
            musicVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->SetMusicVolume(musicVol->GetCurrentValue()/100);
        return true;
    }
    else if(wdg == ambientVol && loaded)
    {
        if(ambientVol->GetCurrentValue() < 1)
            ambientVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->SetAmbientVolume(ambientVol->GetCurrentValue()/100);
        return true;
    }
    else if(wdg == actionsVol && loaded)
    {
        if(actionsVol->GetCurrentValue() < 1)
            actionsVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->SetActionsVolume(actionsVol->GetCurrentValue()/100);
        return true;
    }
    else if(wdg == guiVol && loaded)
    {
        if(guiVol->GetCurrentValue() < 1)
            guiVol->SetCurrentValue(1,false);

        PawsManager::GetSingleton().SetVolume(guiVol->GetCurrentValue()/100);
        psengine->GetSoundManager()->SetGUIVolume(guiVol->GetCurrentValue()/100);
        return true;
    }
    else if(wdg == voicesVol && loaded)
    {
        if(voicesVol->GetCurrentValue() < 1)
            voicesVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->SetVoicesVolume(voicesVol->GetCurrentValue()/100);
        return true;
    }
    else
        return false;
}

bool pawsConfigSound::OnButtonPressed(int button, int mod, pawsWidget* wdg)
{
    dirty = true;
    if(wdg == ambient)
    {
        psengine->GetSoundManager()->ToggleAmbient(ambient->GetState());
        return true;
    }
    else if(wdg == actions)
    {
        psengine->GetSoundManager()->ToggleActions(actions->GetState());
        return true;
    }
    else if(wdg == music)
    {
        psengine->GetSoundManager()->ToggleMusic(music->GetState());
        return true;
    }
    else if(wdg == gui)
    {
        if(gui->GetState())
        {
            PawsManager::GetSingleton().ToggleSounds(true);
            psengine->GetSoundManager()->ToggleGUI(true);
        }
        else
        {
            PawsManager::GetSingleton().ToggleSounds(false);
            psengine->GetSoundManager()->ToggleGUI(false);
        }
        return true;
    }
    else if(wdg == voices)
    {
        psengine->GetSoundManager()->ToggleVoices(voices->GetState());
        return true;
    }
    else if(wdg == loopBGM)
    {
        psengine->GetSoundManager()->ToggleLoop(loopBGM->GetState());
        return true;
    }
    else if(wdg == combatMusic)
    {
        psengine->GetSoundManager()->ToggleCombatMusic(combatMusic->GetState());
        return true;
    }
    else if(wdg == muteOnFocusLoss)
    {
        psengine->SetMuteSoundsOnFocusLoss(muteOnFocusLoss->GetState());
        return true;
    }
    return false;
}

void pawsConfigSound::Show()
{
    oldambient = psengine->GetSoundManager()->PlayingAmbient();
    oldmusic = psengine->GetSoundManager()->PlayingMusic();
    oldactions = psengine->GetSoundManager()->PlayingActions();
    oldgui = PawsManager::GetSingleton().PlayingSounds();
    oldvoices = psengine->GetSoundManager()->PlayingVoices();

    oldvol = psengine->GetSoundManager()->GetVolume();
    oldmusicvol = psengine->GetSoundManager()->GetMusicVolume();
    oldambientvol = psengine->GetSoundManager()->GetAmbientVolume();
    oldactionsvol = psengine->GetSoundManager()->GetActionsVolume();
    oldguivol = PawsManager::GetSingleton().GetVolume();
    oldvoicesvol = psengine->GetSoundManager()->GetVoicesVolume();

    pawsWidget::Show();
}

void pawsConfigSound::Hide()
{
    if(dirty)
    {
        psengine->GetSoundManager()->ToggleAmbient(oldambient);
        psengine->GetSoundManager()->ToggleActions(oldactions);
        psengine->GetSoundManager()->ToggleMusic(oldmusic);
        PawsManager::GetSingleton().ToggleSounds(oldgui);
        psengine->GetSoundManager()->ToggleGUI(oldgui);
        psengine->GetSoundManager()->ToggleVoices(oldvoices);

        psengine->GetSoundManager()->SetVolume(oldvol);
        psengine->GetSoundManager()->SetMusicVolume(oldmusicvol);
        psengine->GetSoundManager()->SetAmbientVolume(oldambientvol);
        psengine->GetSoundManager()->SetActionsVolume(oldactionsvol);
        PawsManager::GetSingleton().SetVolume(oldguivol);
        psengine->GetSoundManager()->SetGUIVolume(oldguivol);
        psengine->GetSoundManager()->SetVoicesVolume(oldvoicesvol);
    }

    pawsWidget::Hide();
}

