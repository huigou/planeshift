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

    chatSound = (pawsCheckBox*) FindWidget("chatSound");
    if (!chatSound)
        return false;

    soundLocation = (pawsComboBox*) FindWidget("soundLocation");
    soundLocation->NewOption("Player");
    soundLocation->NewOption("Camera");
    
    generalVol->SetMaxValue(200);
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
    generalVol->SetCurrentValue(psengine->GetSoundManager()->mainSndCtrl->GetVolume()*100,false);
    musicVol->SetCurrentValue(psengine->GetSoundManager()->musicSndCtrl->GetVolume()*100,false);
    ambientVol->SetCurrentValue(psengine->GetSoundManager()->ambientSndCtrl->GetVolume()*100,false);
    guiVol->SetCurrentValue(psengine->GetSoundManager()->guiSndCtrl->GetVolume()*100,false);
    voicesVol->SetCurrentValue(psengine->GetSoundManager()->voiceSndCtrl->GetVolume()*100,false);
    actionsVol->SetCurrentValue(psengine->GetSoundManager()->actionSndCtrl->GetVolume()*100,false);

    ambient->SetState(psengine->GetSoundManager()->ambientSndCtrl->GetToggle());
    actions->SetState(psengine->GetSoundManager()->actionSndCtrl->GetToggle());
    music->SetState(psengine->GetSoundManager()->musicSndCtrl->GetToggle());
    gui->SetState(psengine->GetSoundManager()->guiSndCtrl->GetToggle());
    voices->SetState(psengine->GetSoundManager()->voiceSndCtrl->GetToggle());

    muteOnFocusLoss->SetState(psengine->GetMuteSoundsOnFocusLoss());
    loopBGM->SetState(psengine->GetSoundManager()->loopBGM.GetToggle());
    combatMusic->SetState(psengine->GetSoundManager()->combatMusic.GetToggle());
    chatSound->SetState(psengine->GetSoundManager()->chatToggle.GetToggle());
    
    if (psengine->GetSoundManager()->listenerOnCamera.GetToggle() == true)
    {
        soundLocation->Select("Camera");
    }
    else
    {
        soundLocation->Select("Player");
    }

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
    xml.AppendFmt("<chatsound on=\"%s\" />\n",
                     chatSound->GetState() ? "yes" : "no");
               
    if (csStrCaseCmp(soundLocation->GetSelectedRowString(), "Camera") == 0)
    {
        xml.AppendFmt("<usecamerapos on=\"%s\" />\n", "yes");
    }
    else
    {
        xml.AppendFmt("<usecamerapos on=\"%s\" />\n", "no");
    }
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

        psengine->GetSoundManager()->mainSndCtrl->SetVolume(generalVol->GetCurrentValue()/100);
    }
    else if(wdg == musicVol && loaded)
    {
        if(musicVol->GetCurrentValue() < 1)
            musicVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->musicSndCtrl->SetVolume(musicVol->GetCurrentValue()/100);
    }
    else if(wdg == ambientVol && loaded)
    {
        if(ambientVol->GetCurrentValue() < 1)
            ambientVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->ambientSndCtrl->SetVolume(ambientVol->GetCurrentValue()/100);
    }
    else if(wdg == actionsVol && loaded)
    {
        if(actionsVol->GetCurrentValue() < 1)
            actionsVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->actionSndCtrl->SetVolume(actionsVol->GetCurrentValue()/100);
    }
    else if(wdg == guiVol && loaded)
    {
        if(guiVol->GetCurrentValue() < 1)
            guiVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->guiSndCtrl->SetVolume(guiVol->GetCurrentValue()/100);
    }
    else if(wdg == voicesVol && loaded)
    {
        if(voicesVol->GetCurrentValue() < 1)
            voicesVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->voiceSndCtrl->SetVolume(voicesVol->GetCurrentValue()/100);
    }
    else
    {
        return false;
    }
    
    SaveConfig();
    return true;
}

bool pawsConfigSound::OnButtonPressed(int button, int mod, pawsWidget* wdg)
{
    dirty = true;

    if(wdg == ambient)
    {
        psengine->GetSoundManager()->ambientSndCtrl->SetToggle(ambient->GetState());
    }
    else if(wdg == actions)
    {
        psengine->GetSoundManager()->actionSndCtrl->SetToggle(actions->GetState());
    }
    else if(wdg == music)
    {
        psengine->GetSoundManager()->musicSndCtrl->SetToggle(music->GetState());
    }
    else if(wdg == gui)
    {
        psengine->GetSoundManager()->guiSndCtrl->SetToggle(gui->GetState());
    }
    else if(wdg == voices)
    {
        psengine->GetSoundManager()->SetVoiceToggle(voices->GetState());
    }
    else if(wdg == loopBGM)
    {
		psengine->GetSoundManager()->loopBGM.SetToggle(loopBGM->GetState());
    }
    else if(wdg == combatMusic)
    {
        psengine->GetSoundManager()->combatMusic.SetToggle(combatMusic->GetState());
    }
    else if(wdg == muteOnFocusLoss)
    {
        psengine->SetMuteSoundsOnFocusLoss(muteOnFocusLoss->GetState());
    }
    else if(wdg == chatSound)
    {
        psengine->GetSoundManager()->chatToggle.SetToggle(chatSound->GetState());
    }
    else
    {
        return false;
    }

    SaveConfig();    
    return true;
}

void pawsConfigSound::OnListAction(pawsListBox* selected, int status)
{
    pawsComboBox* soundlocation = (pawsComboBox*)FindWidget("soundLocation");
    csString _selected = soundlocation->GetSelectedRowString();
   
    if (_selected.Compare("Camera"))
    {
        psengine->GetSoundManager()->listenerOnCamera.SetToggle(true);
    }
    else
    {
        psengine->GetSoundManager()->listenerOnCamera.SetToggle(false);
    }
    SaveConfig();
}


void pawsConfigSound::Show()
{
    oldambient = psengine->GetSoundManager()->ambientSndCtrl->GetToggle();
    oldmusic = psengine->GetSoundManager()->musicSndCtrl->GetToggle();
    oldactions = psengine->GetSoundManager()->actionSndCtrl->GetToggle();
    oldgui = psengine->GetSoundManager()->guiSndCtrl->GetToggle();
    oldvoices = psengine->GetSoundManager()->voiceSndCtrl->GetToggle();
    oldchatsound = psengine->GetSoundManager()->chatToggle.GetToggle();
    oldlisteneroncamerapos = psengine->GetSoundManager()->listenerOnCamera.GetToggle();

    oldvol = psengine->GetSoundManager()->mainSndCtrl->GetVolume();
    oldmusicvol = psengine->GetSoundManager()->musicSndCtrl->GetVolume();
    oldambientvol = psengine->GetSoundManager()->ambientSndCtrl->GetVolume();
    oldactionsvol = psengine->GetSoundManager()->actionSndCtrl->GetVolume();
    oldguivol = psengine->GetSoundManager()->guiSndCtrl->GetVolume();
    oldvoicesvol = psengine->GetSoundManager()->voiceSndCtrl->GetVolume();

    pawsWidget::Show();
}

void pawsConfigSound::Hide()
{
    if(dirty)
    {
        psengine->GetSoundManager()->ambientSndCtrl->SetToggle(oldambient);
        psengine->GetSoundManager()->actionSndCtrl->SetToggle(oldactions);
        psengine->GetSoundManager()->musicSndCtrl->SetToggle(oldmusic);
        psengine->GetSoundManager()->guiSndCtrl->SetToggle(oldgui);
        psengine->GetSoundManager()->SetVoiceToggle(oldvoices);
        psengine->GetSoundManager()->chatToggle.SetToggle(oldchatsound);
        psengine->GetSoundManager()->listenerOnCamera.SetToggle(oldlisteneroncamerapos);

        psengine->GetSoundManager()->mainSndCtrl->SetVolume(oldvol);
        psengine->GetSoundManager()->musicSndCtrl->SetVolume(oldmusicvol);
        psengine->GetSoundManager()->ambientSndCtrl->SetVolume(oldambientvol);
        psengine->GetSoundManager()->actionSndCtrl->SetVolume(oldactionsvol);
        psengine->GetSoundManager()->guiSndCtrl->SetVolume(oldguivol);
        psengine->GetSoundManager()->voiceSndCtrl->SetVolume(oldvoicesvol);
    }

    pawsWidget::Hide();
}

