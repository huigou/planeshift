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

#include "globals.h"
#include "pawslauncherwindow.h"

using namespace std;

pawsLauncherWindow::pawsLauncherWindow()
{
}

bool pawsLauncherWindow::PostSetup()
{
    configFile.AttachNew(new csConfigFile(LAUNCHER_CONFIG_FILENAME, psLaunchGUI->GetVFS()));

    launcherMain = FindWidget("LauncherMain");
    launcherSettings = FindWidget("LauncherSettings");
    launcherUpdater = FindWidget("LauncherUpdater");

    launcherMain->OnGainFocus();

    // Get server news.
    UpdateNews();

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

bool pawsLauncherWindow::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    int ID = widget->GetID();

    if(ID == QUIT_BUTTON)
    {
        psLaunchGUI->Quit();
    }
    else if(ID == LAUNCH_BUTTON)
    {
        psLaunchGUI->ExecClient(true);
        psLaunchGUI->Quit();
    }
    else if(ID == SETTINGS_BUTTON)
    {
        launcherMain->Hide();
        launcherSettings->Show();
        launcherSettings->OnGainFocus();
    }
    else if(ID == REPAIR_BUTTON)
    {
        launcherMain->Hide();
        launcherUpdater->Show();
        launcherUpdater->OnGainFocus();
        psLaunchGUI->PerformRepair();
    }
    else if(ID == UPDATER_YES_BUTTON)
    {
        launcherUpdater->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
    }
    else if(ID == UPDATER_NO_BUTTON)
    {
        launcherUpdater->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
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
    }

    return true;
}

void pawsLauncherWindow::HandleUpdateButton(bool choice, void *updatewindow)
{
    pawsWidget* updateWindow = (pawsWidget*)updatewindow;
    psLaunchGUI->PerformUpdate(choice);
    updateWindow->Hide();
}
