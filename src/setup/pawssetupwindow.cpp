/*
 * pawssetupwindow.cpp - Author: Ian Donderwinkel
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
//////////////////////////////////////////////////////////////////////


#include <psconfig.h>
#include "globals.h"

#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <iutil/vfs.h>
#include <imap/loader.h>
#include <ivaria/conout.h>

#include <iengine/engine.h>
#include <iengine/region.h>
#include <iutil/cache.h>
#include <csutil/cfgfile.h>
#include <csutil/cspmeter.h>
#include <ivaria/sequence.h>
#include <iutil/stringarray.h>
#include <iutil/databuff.h>
// PAWS INCLUDES
#include "pawssetupwindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawsradio.h"
#include "paws/pawscheckbox.h"
#include "paws/pawstextbox.h"
#include "paws/pawscrollbar.h"
#include "paws/pawsprogressbar.h"
#include "paws/pawslistbox.h"

#define PS_QUERY_PLUGIN(myref,intf, str)                          \
myref =  csQueryRegistry<intf> (object_reg);                      \
if (!myref) {                                                     \
    printf ("No " str " plugin!");\
    return false;\
}

void RefreshDraw()
{
    PawsManager::GetSingleton().GetGraphics3D()->BeginDraw (CSDRAW_2DGRAPHICS);
    PawsManager::GetSingleton().Draw (); 
    PawsManager::GetSingleton().GetGraphics3D()->FinishDraw ();
    PawsManager::GetSingleton().GetGraphics2D()->Print (0);
}

pawsSetupWindow::pawsSetupWindow()
{
    AdvancedMode(false);
    customWidth  = 1280;
    customHeight = 960;
}

bool pawsSetupWindow::PostSetup()
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (PawsManager::GetSingleton().GetObjectRegistry());
   
    config = new csConfigFile("/planeshift/userdata/planeshift.cfg", vfs);

    rbgResolution= (pawsRadioButtonGroup*)FindWidget("resbox");
    rbgDepth     = (pawsRadioButtonGroup*)FindWidget("depthbox");
    rbgVBO       = (pawsRadioButtonGroup*)FindWidget("vbobox");

    cbSound         = (pawsCheckBox*)FindWidget("sound");
    cbFullScreen    = (pawsCheckBox*)FindWidget("fullscreen");
    cbAllMaps       = (pawsCheckBox*)FindWidget("allmaps");
    cbMultiQuality  = (pawsCheckBox*)FindWidget("ms_quality");
    cbKeepMaps      = (pawsCheckBox*)FindWidget("keepmaps");
    cbPreloadModels = (pawsCheckBox*)FindWidget("preloadmodels");
 
    btnOK        = (pawsButton*)FindWidget("OKButton");
    btnCancel    = (pawsButton*)FindWidget("CancelButton");
    btnLaunch    = (pawsButton*)FindWidget("LaunchButton");

    edtCustomWidth   = (pawsEditTextBox*)FindWidget("screenwidth");
    edtCustomHeight  = (pawsEditTextBox*)FindWidget("screenheight");
    edtStencil       = (pawsEditTextBox*)FindWidget("stencil");

    lblStreamBuffer = (pawsTextBox*)FindWidget("streambufferlbl");
    lblAnisotropy   = (pawsTextBox*)FindWidget("at");
    lblMultiSampling= (pawsTextBox*)FindWidget("ms");
    lblTextureSample= (pawsTextBox*)FindWidget("texdownc");
    lblRelightTarget= (pawsTextBox*)FindWidget("relightTarget");
    lblFontScale    = (pawsTextBox*)FindWidget("fontscalepct");

    scbAnisotropy   = (pawsScrollBar*)FindWidget("ani");
    scbMultiSampling= (pawsScrollBar*)FindWidget("multi");
    scbTextureSample= (pawsScrollBar*)FindWidget("texdown");
    scbFontScale    = (pawsScrollBar*)FindWidget("fontscale");

    LoadSettings();
    return true;
}

bool pawsSetupWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (widget==btnOK)
    {
        SaveSettings();
        setupApp->Quit();
    }
    else if (widget==btnCancel)
    {
        setupApp->Quit();
    }
    else if (widget==btnLaunch)
    {
        SaveSettings();
    #ifdef CS_PLATFORM_WIN32
        // Lanuch PlaneShift in the win way
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );

        // Start the second updater process. 
        CreateProcess( NULL,    // No module name (use command line). 
            "psclient.exe",     // Command line.
            NULL,               // Process handle not inheritable. 
            NULL,               // Thread handle not inheritable. 
            FALSE,              // Set handle inheritance to FALSE. 
            CREATE_NEW_CONSOLE, // Give the process it's own console
            NULL,               // Use parent's environment block. 
            NULL,               // Use parent's starting directory. 
            &si,                // Pointer to STARTUPINFO structure.
            &pi );              // Pointer to PROCESS_INFORMATION structure.

    #elif defined(CS_PLATFORM_UNIX)
        // *NIX have a quite easier way to do that :)
        system("./psclient &");
    #elif defined(CS_PLATFORM_MACOSX) //is this the right define?
        //system("open ./psclient.app &");
    #else
    #error "Bad platform for launch"
    #endif
        setupApp->Quit();
    }
    else if(widget == cbAllMaps && cbAllMaps->GetState())
    {
        PawsManager::GetSingleton().CreateWarningBox("Warning! While this will eliminate all loading while ingame, this will demand much more RAM and the first loading will take longer to load");
    }
    else if(widget == cbKeepMaps && cbKeepMaps->GetState())
    {
        PawsManager::GetSingleton().CreateWarningBox("This experimental mode will only load zones as you enter them, but will keep them loaded. It does not unload zones yet.");
    }
    else if(widget == cbPreloadModels && cbPreloadModels->GetState())
    {
        PawsManager::GetSingleton().CreateWarningBox("This mode forces a preload of all models at client startup. You get higher memory usage and a longer client load, but a slightly smoother experience once you are ingame.");
    }
    else if(!strcmp(widget->GetName(),"Skins"))
    {
        pawsWidget* skin = PawsManager::GetSingleton().FindWidget("skin");
        if(skin)
        {
            skin->SetFade(false);
            skin->SetMaxAlpha(1);
            skin->Show();
        }
    }

    return false;
}


void pawsSetupWindow::LoadSettings()
{
    // video settings
    int      width  = config->GetInt("Video.ScreenWidth");
    int      height = config->GetInt("Video.ScreenHeight");
    
    csString res;
    res.Format("%ix%i", width, height);

    if (!rbgResolution->SetActive(res))
    {
        rbgResolution->SetActive("rescustom");
        customWidth  = width;
        customHeight = height;
    }

    res.Format("%i", customWidth);
    edtCustomWidth->SetText(res);
    res.Format("%i", customHeight);
    edtCustomHeight->SetText(res);

    cbFullScreen->SetState(config->GetBool("Video.Fullscreen"));
    rbgDepth->SetActive(config->GetStr("Video.ScreenDepth"));
    
    // Stencil
    int stencil = config->GetInt("Video.OpenGL.StencilThreshold",50);
    csString stenstr;
    stenstr.Format("%d",stencil);
    edtStencil->SetText(stenstr);

    // VBO
    if(!config->KeyExists("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object"))
    {
        rbgVBO->SetActive("def");
    }
    else
    {
        bool value = config->GetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object",true);
        if(value)
            rbgVBO->SetActive("on");
        else
            rbgVBO->SetActive("off");
    }

    // sound
    cbSound->SetState(config->KeyExists("System.PlugIns.iSndSysRenderer"));

    // all maps
    cbAllMaps->SetState(config->GetBool("Planeshift.Client.Loading.AllMaps",false));
    // keep maps
    cbKeepMaps->SetState(config->GetBool("Planeshift.Client.Loading.KeepMaps",false));
    // preload models
    cbPreloadModels->SetState(config->GetBool("PlaneShift.Client.Loading.PreloadModels",false));

    // Multisampling quality
    cbMultiQuality->SetState(config->GetBool("Video.OpenGL.MultisampleFavorQuality",true));

    // Anisotropy
    float aniso = config->GetFloat("Video.OpenGL.TextureFilterAnisotropy",1.0f);
    scbAnisotropy->SetMinValue(1.0f);
    scbAnisotropy->SetMaxValue(16.0f);
    scbAnisotropy->SetTickValue(0.5f);
    scbAnisotropy->EnableValueLimit(true);
    scbAnisotropy->SetCurrentValue(aniso,true);

    // Multisampling
    int multi = config->GetInt("Video.OpenGL.MultiSamples",0);
    scbMultiSampling->SetMaxValue(8.0f);
    scbMultiSampling->SetTickValue(0.0f); // handled extra in onscroll
    scbMultiSampling->SetCurrentValue((float)multi,true);

    // Texture downsampling
    int samp = config->GetInt("Video.OpenGL.TextureDownsample",0);
    scbTextureSample->SetMaxValue(4.0f);
    scbTextureSample->SetTickValue(1.0f); // handled extra in onscroll
    scbTextureSample->EnableValueLimit(true);
    scbTextureSample->SetCurrentValue((float)samp,true);

    // Font scaling
    int fontFactor = config->GetInt("Font.ScalePercent",100);
    scbFontScale->SetMinValue(50.0f);
    scbFontScale->SetMaxValue(150.0f);
    scbFontScale->SetTickValue(5.0f); // handled extra in onscroll
    scbFontScale->EnableValueLimit(true);
    scbFontScale->SetCurrentValue((float)fontFactor,true);
}

bool pawsSetupWindow::OnScroll(int dir,pawsScrollBar* widget)
{
    if(widget == scbAnisotropy)
    {
        // Easy, just update the label
        csString lbl;
        lbl.Format("%.1f",widget->GetCurrentValue());
        lblAnisotropy->SetText(lbl);
    }
    else if(widget == scbMultiSampling)
    {
        // Harder, needs to be 1,2,4,8,16,32
        int value = (int)widget->GetCurrentValue();
        int pow = 0;

        while(value > 0)
        {
            value = value >> 1;
            pow++;
        }
        pow--;

        if(pow >= 0)
            value = 1 << pow;
        else
            value = 0;

        // Note: direction seems to be reversed while horz
        if(value == 0 && dir == SCROLL_DOWN)
            value = 1;
        else if(dir != SCROLL_SET)
        {
            if(dir == SCROLL_UP)
                value = value >> 1;
            else
                value = value << 1;
        }

        if(value > (int)widget->GetMaxValue())
        {
            value = (int)widget->GetMaxValue();
        }

        widget->SetCurrentValue(value,false);

        // Update label
        csString lbl;
        lbl.Format("%d",value);
        lblMultiSampling->SetText(lbl);
    }
    else if(widget == scbTextureSample)
    {
        int pow = (int)widget->GetCurrentValue();
        int value = 1 << pow;

        csString lbl;
        lbl.Format("1/%d",value);
        lblTextureSample->SetText(lbl);
    }
    else if(widget == scbFontScale)
    {
        int value = (int)widget->GetCurrentValue();

        csString lbl;
        lbl.Format("%d%%",value);
        lblFontScale->SetText(lbl);
    }
    else
        return false; // Unkown widget

    return true;
}

void pawsSetupWindow::SaveSettings()
{
    config->SetBool("Video.Fullscreen",cbFullScreen->GetState());
    config->SetStr("Video.ScreenDepth",rbgDepth->GetActive());

    config->SetInt("Video.OpenGL.StencilThreshold",atoi(edtStencil->GetText()));

    // VBO
    int value;
    csString vbo = rbgVBO->GetActive();
    if(vbo == "def")
        value = 2;
    else if(vbo == "on")
        value = 1;
    else
        value = 0;

    if(value==2)
        config->DeleteKey("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object");
    else
        config->SetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object",(value ? true : false));

    // resolution
    int width, height;
    csString res = rbgResolution->GetActive();
    if (res=="800x600")
    {
        width=800;
        height=600;
    }
    else if (res=="1024x768")
    {
        width=1024;
        height=768;
    }
    else
    {
        width=atoi(edtCustomWidth->GetText());
        height=atoi(edtCustomHeight->GetText());
    }       
    config->SetInt("Video.ScreenWidth",width);
    config->SetInt("Video.ScreenHeight",height);

    // sound enabled/disabled
    if (cbSound->GetState())
    {
        config->SetStr("System.PlugIns.iSndSysRenderer","crystalspace.sndsys.renderer.software");
    }
    else
    {
        config->DeleteKey("System.PlugIns.iSndSysRenderer");
    }

    // all maps
    config->SetBool("Planeshift.Client.Loading.AllMaps",cbAllMaps->GetState());
    // keep maps
    config->SetBool("Planeshift.Client.Loading.KeepMaps",cbKeepMaps->GetState());
    // preload models
    config->SetBool("PlaneShift.Client.Loading.PreloadModels",cbPreloadModels->GetState());

    config->SetBool ("Video.OpenGL.MultisampleFavorQuality"   ,cbMultiQuality->GetState());
    config->SetInt  ("Video.OpenGL.MultiSamples"              ,(int)scbMultiSampling->GetCurrentValue());
    config->SetFloat("Video.OpenGL.TextureFilterAnisotropy"   ,scbAnisotropy->GetCurrentValue());
    config->SetInt  ("Video.OpenGL.TextureDownsample"         ,(int)scbTextureSample->GetCurrentValue());

    config->SetInt("Font.ScalePercent", (int)scbFontScale->GetCurrentValue() );

    config->Save();
}
