/*
* pawslauncherwindow.cpp - Author: Mike Gist
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <fstream>

#include "paws/pawsbutton.h"
#include "paws/pawscombo.h"
#include "paws/pawswidget.h"
#include "paws/pawstextbox.h"
#include "paws/pawsyesnobox.h"

#include "globals.h"
#include "pawslauncherwindow.h"

using namespace std;

pawsLauncherWindow::pawsLauncherWindow()
{
}

bool pawsLauncherWindow::PostSetup()
{
    configFile.AttachNew(new csConfigFile(LAUNCHER_CONFIG_FILENAME, psLaunchGUI->GetVFS()));
    configUser.AttachNew(new csConfigFile("/planeshift/userdata/planeshift.cfg", psLaunchGUI->GetVFS()));

    launcherMain = FindWidget("LauncherMain");
    launcherSettings = FindWidget("LauncherSettings");
    launcherUpdater = FindWidget("LauncherUpdater");

    launcherMain->OnGainFocus();

    // Load game settings.
    LoadSettings();

    // Get server news.
    newsUpdater.AttachNew(new Thread(new NewsUpdater(this), true));

    // Setup update available window.
    updateAvailable = (pawsYesNoBox*)FindWidget("UpdateAvailable");
    updateAvailable->SetCallBack(HandleUpdateButton, updateAvailable, "An update to PlaneShift is available. Do you wish to update now?");

    return true;
}

void pawsLauncherWindow::UpdateNews()
{
    pawsMultiLineTextBox* serverNews = (pawsMultiLineTextBox*)FindWidget("ServerNews");
    psLaunchGUI->GetDownloader()->DownloadFile(configFile->GetStr("Launcher.News.URL", ""), "servernews", true, false);
    
    ifstream newsFile("servernews", ifstream::in);
    csString buffer;
    while(newsFile.good())
    {
        buffer.Append((char)newsFile.get());
    }
    buffer.Truncate(buffer.Length()-1);
    serverNews->SetText(buffer.GetDataSafe());
    newsFile.close();
    psLaunchGUI->GetFileUtil()->RemoveFile("servernews");
}

pawsButton* pawsLauncherWindow::FindButton(WidgetID id)
{
    return static_cast<pawsButton*>(FindWidget(id));
}

bool pawsLauncherWindow::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    int ID = widget->GetID();

    if(ID == QUIT_BUTTON)
    {
        psLaunchGUI->Quit();
    }
    else if(ID == PLAY_BUTTON)
    {
        psLaunchGUI->ExecClient(true);
        psLaunchGUI->Quit();
    }
    else if(ID == SETTINGS_BUTTON)
    {
        launcherMain->Hide();
        launcherSettings->Show();
        launcherSettings->OnGainFocus();
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(true, false);
    }
    else if(ID == REPAIR_BUTTON)
    {
        psLaunchGUI->PerformRepair();
        pawsMessageTextBox* output = (pawsMessageTextBox*)FindWidget("UpdaterOutput");
        output->Clear();
        launcherMain->Hide();
        launcherUpdater->Show();
        launcherUpdater->OnGainFocus();
    }
    else if(ID == UPDATER_YES_BUTTON)
    {
        widget->Hide();
        FindWidget("UpdaterNoButton")->Hide();
        FindWidget("UpdaterCancelButton")->Show();
        psLaunchGUI->PerformUpdate(true);
    }
    else if(ID == UPDATER_NO_BUTTON)
    {
        FindWidget("UpdaterYesButton")->Hide();
        widget->Hide();
        launcherUpdater->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
        psLaunchGUI->PerformUpdate(false);
    }
    else if(ID == UPDATER_OK_BUTTON)
    {
        FindWidget("UpdaterNoButton")->Hide();
        FindWidget("UpdaterYesButton")->Hide();
        widget->Hide();
        launcherUpdater->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
    }
    else if(ID == UPDATER_CANCEL_BUTTON)
    {
        widget->Hide();
        launcherUpdater->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
        psLaunchGUI->CancelUpdater();
    }
    else if(ID == SETTINGS_CANCEL_BUTTON)
    {
        launcherSettings->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
    }
    else if(ID == SETTINGS_OK_BUTTON)
    {
        launcherSettings->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
        SaveSettings();
    }
    else if(ID == SETTINGS_AUDIO_BUTTON)
    {
        FindWidget("SettingsAudio")->Show();
        FindWidget("SettingsControls")->Hide();
        FindWidget("SettingsGeneral")->Hide();
        FindWidget("SettingsGraphics")->Hide();
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(false, false);
        launcherSettings->OnGainFocus();
    }
    else if(ID == SETTINGS_CONTROLS_BUTTON)
    {
        FindWidget("SettingsAudio")->Hide();
        FindWidget("SettingsControls")->Show();
        FindWidget("SettingsGeneral")->Hide();
        FindWidget("SettingsGraphics")->Hide();
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(false, false);
        launcherSettings->OnGainFocus();
    }
    else if(ID == SETTINGS_GENERAL_BUTTON)
    {
        FindWidget("SettingsAudio")->Hide();
        FindWidget("SettingsControls")->Hide();
        FindWidget("SettingsGeneral")->Show();
        FindWidget("SettingsGraphics")->Hide();
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(false, false);
        launcherSettings->OnGainFocus();
    }
    else if(ID == SETTINGS_GRAPHICS_BUTTON)
    {
        FindWidget("SettingsAudio")->Hide();
        FindWidget("SettingsControls")->Hide();
        FindWidget("SettingsGeneral")->Hide();
        FindWidget("SettingsGraphics")->Show();
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(false, false);
        launcherSettings->OnGainFocus();
    }

    return true;
}

void pawsLauncherWindow::HandleUpdateButton(bool choice, void *updatewindow)
{
    pawsWidget* updateWindow = (pawsWidget*)updatewindow;
    psLaunchGUI->PerformUpdate(choice);
    updateWindow->Hide();
}

void pawsLauncherWindow::LoadSettings()
{
    csConfigFile configPSC("/planeshift/psclient.cfg", psLaunchGUI->GetVFS());

    pawsComboBox* graphicsPreset = (pawsComboBox*)FindWidget("GraphicsPreset");
    graphicsPreset->NewOption("Highest");
    graphicsPreset->NewOption("High");
    graphicsPreset->NewOption("Low");
    graphicsPreset->NewOption("Lowest");
    graphicsPreset->NewOption("Custom");

    csString preset = configUser->GetStr("PlaneShift.Graphics.Preset");
    if(preset.Compare(""))
    {
        preset = configPSC.GetStr("PlaneShift.Graphics.Preset");
    }
    graphicsPreset->Select(preset);
}

void pawsLauncherWindow::SaveSettings()
{
    pawsComboBox* graphicsPreset = (pawsComboBox*)FindWidget("GraphicsPreset");
    configUser->SetStr("PlaneShift.Graphics.Preset", graphicsPreset->GetSelectedRowString());

    switch(graphicsPreset->GetSelectedRowNum())
    {
    case HIGHEST:
        {
            break;
        }
    case HIGH:
        {
            break;
        }
    case LOW:
        {
            break;
        }
    case LOWEST:
        {
            break;
        }
    case CUSTOM:
        {
            break;
        }
    };

    configUser->Save();
}
