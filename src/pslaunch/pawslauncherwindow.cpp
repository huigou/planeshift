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

bool pawsLauncherWindow::PostSetup()
{
    configFile = new csConfigFile(LAUNCHER_CONFIG_FILENAME, psLaunchGUI->GetVFS());
    quit = (pawsButton*)FindWidget("Quit");
    launchClient = (pawsButton*)FindWidget("LaunchButton");
    settings = (pawsButton*)FindWidget("SettingsButton");

    // Get server news.
    serverNews = (pawsMultiLineTextBox*)FindWidget("ServerNews");
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

    // Setup update available window.
    updateAvailable = (pawsYesNoBox*)FindWidget("UpdateAvailable");
    updateAvailable->SetCallBack(HandleUpdateButton, updateAvailable, "An update to PlaneShift is available. Do you wish to update now?");

    return true;
}

bool pawsLauncherWindow::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget==(pawsWidget*)quit)
    {
        psLaunchGUI->Quit();
    }
    else if (widget==(pawsWidget*)launchClient)
    {
        psLaunchGUI->ExecClient(true);
        psLaunchGUI->Quit();
    }
    else if (widget==(pawsWidget*)settings)
    {
        // Show settings widgets.
    }

    return true;
}

void pawsLauncherWindow::HandleUpdateButton(bool yes, void *updatewindow)
{
    pawsYesNoBox* updateWindow = (pawsYesNoBox *)updatewindow;
    if(yes)
    {
        psLaunchGUI->PerformUpdate(true);
    }
    else
    {
        psLaunchGUI->PerformUpdate(false);
    }
    updateWindow->Hide();
}
