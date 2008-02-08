/*
 * pawsskinwindow.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * Credits : Christian Svensson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
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
#include "globals.h"

#include <csutil/cfgfile.h>
#include <iutil/stringarray.h>
#include <iutil/databuff.h>
// PAWS INCLUDES
#include "pawsskinwindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawslistbox.h"
#include "paws/pawscombo.h"
#include "paws/pawsbutton.h"
#include "paws/pawscheckbox.h"
#include "paws/pawsimagedrawable.h"
#include "util/psxmlparser.h"

pawsSkinWindow::~pawsSkinWindow()
{
    vfs->Unmount("/skin",mountedPath);
    mountedPath.Empty();
}

bool pawsSkinWindow::PostSetup()
{
    vfs =  csQueryRegistry<iVFS> (PawsManager::GetSingleton().GetObjectRegistry());


    skins = (pawsComboBox*)FindWidget("skins");
    desc = (pawsMultiLineTextBox*)FindWidget("desc");
    preview = (pawsWidget*)FindWidget("preview");
    previewBtn = (pawsButton*)FindWidget("PreviewButton");
    previewBox = (pawsCheckBox*)FindWidget("PreviewBox");

    config = new csConfigFile("/planeshift/userdata/planeshift.cfg", vfs);

    // Fill the skins
    csConfigFile configPSC("/planeshift/psclient.cfg", vfs);
    skinPath = configPSC.GetStr("PlaneShift.GUI.Skin.Dir");
    csRef<iStringArray> files = vfs->FindFiles(skinPath);
    for (size_t i = 0; i < files->GetSize(); i++)
    {
        csString file = files->Get(i);
        file = file.Slice(skinPath.Length(),file.Length()-skinPath.Length());

        size_t dot = file.FindLast('.');
        csString ext = file.Slice(dot+1,3);
        if (ext == "zip")
        {
            skins->NewOption(file.Slice(0,dot));
        }
    }

    // Load the current skin
    csString skin = config->GetStr("PlaneShift.GUI.Skin.Selected");	
    if(!strcmp(skin,""))
        return true; // Stop here, but it's ok

    LoadSkin(skin);

    return true;       
}

void pawsSkinWindow::OnListAction( pawsListBox* widget, int status )
{
    LoadSkin(skins->GetSelectedRowString().GetData());
}

void pawsSkinWindow::LoadSkin(const char* name)
{
	// Create full path to skin.
    csString zip = skinPath + name + ".zip";

    // This .zip could be a file or a dir
    csString slash(CS_PATH_SEPARATOR);
    if ( vfs->Exists(zip + slash) )
        zip += slash;

    if ( !vfs->Exists(zip) )
    {
        printf("Current skin doesn't exist, skipping..\n");
        return;
    }

    // Make sure the skin is selected
    if(skins->GetSelectedRowString() != name)
    {
        skins->Select(name);
        return; // To prevent recursion
    }

    // Get the path
    csRef<iDataBuffer> real = vfs->GetRealPath(zip);
    
    // Mount the skin
    vfs->Unmount("/skin",mountedPath);
    vfs->Mount("/skin",real->GetData());
    mountedPath = real->GetData();

    // Parse XML
    csRef<iDocument> xml = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(),"/skin/skin.xml");
    if(!xml)
    {
        Error1("Parse error in /skin/skin.xml");
        return;
    }
    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error1("No XML root in /skin/skin.xml");
        return;
    }
    root = root->GetNode("xml");
    if(!root)
    {
        Error1("No <xml> tag in /skin/skin.xml");
        return;
    }
    csRef<iDocumentNode> skinInfo = root->GetNode("skin_information");
    if(!skinInfo)
    {
        Error1("No <skin_information> in /skin/skin.xml");
        return;
    }

    // Read XML
    csRef<iDocumentNode> nameNode           = skinInfo->GetNode("name");
    csRef<iDocumentNode> authorNode         = skinInfo->GetNode("author");
    csRef<iDocumentNode> descriptionNode    = skinInfo->GetNode("description");
    csRef<iDocumentNode> mountNode          = root->GetNode("mount_path");

    bool success = true;

    if(!mountNode)
    {
        printf("skin.xml is missing mount_path!\n");
        success = false;
    }

    if(!authorNode)
    {
        printf("skin.xml is missing author!\n");
        success = false;
    }

    if(!descriptionNode)
    {
        printf("skin.xml is missing description!\n");
        success = false;
    }

    if(!nameNode)
    {
        printf("skin.xml is missing name!\n");
        success = false;
    }

    if(!success)
        return;


    // Move data to variables
    csString skinname,author,description;
    skinname = nameNode->GetContentsValue();
    author = authorNode->GetContentsValue();
    description = descriptionNode->GetContentsValue();

    currentSkin = name;

    // Print the info
    csString info;
    info.Format("%s\nAuthor: %s\n%s",skinname.GetData(),author.GetData(),description.GetData());
    desc->SetText(info);

    // Reset the backgrounds
    preview->RemoveTitle();
    preview->SetBackground("Blue Background");
    previewBtn->SetBackground("Blue Background");
    previewBox->SetImages("radiooff","radioon");

    // Load the skin
    success = success && LoadResource("Examine Background","skintest_bg",mountNode->GetContentsValue());
    success = success && LoadResource("Standard Button","skintest_btn",mountNode->GetContentsValue());
    success = success && LoadResource("Blue Title","skintest_title",mountNode->GetContentsValue());
    success = success && LoadResource("radiooff","skintest_roff",mountNode->GetContentsValue());
    success = success && LoadResource("radioon","skintest_ron",mountNode->GetContentsValue());
    success = success && LoadResource("quit","quit",mountNode->GetContentsValue());

    if(!success)
    {
        PawsManager::GetSingleton().CreateWarningBox("Couldn't load skin! Check the console for detailed output");
        return;
    }

    preview->SetTitle("Skin preview","skintest_title","center","true");
    preview->SetMaxAlpha(1);
    preview->SetBackground("skintest_bg");

    previewBtn->SetMaxAlpha(1);
    previewBtn->SetBackground("skintest_btn");

    previewBox->SetImages("skintest_roff","skintest_ron");
}

bool pawsSkinWindow::LoadResource(const char* resource,const char* resname, const char* mountPath)
{
    // Remove the resource if it exists
    PawsManager::GetSingleton().GetTextureManager()->Remove(resname);

    // Open the image list
    csRef<iDocument> xml = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(),"/skin/imagelist.xml");
    if(!xml)
    {
        Error1("Parse error in /skin/imagelist.xml");
        return false;
    }
    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error1("No XML root in /skin/imagelist.xml");
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode("image_list");
    if(!topNode)
    {
        Error1("No <image_list> in /skin/imagelist.xml");
        return false;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    // Find the resource
    csString filename;
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        if ( strcmp( node->GetValue(), "image" ) == 0 )
        {
            if(!strcmp(node->GetAttributeValue( "resource" ),resource))
            {
                filename = node->GetAttributeValue( "file" );
                break;
            }
        }
    }

    if(!filename.Length())
    {
        printf("Couldn't locate resource %s in the current skin!\n",resource);
        return false;
    }


    // Skin uses /paws/skin which we can't use since the app is already using that
    // So we need to replace that with /skin which we can and do use
    filename.DeleteAt(0,strlen(mountPath));
    filename.Insert(0,"/skin/");

    if(!vfs->Exists(filename))
    {
        printf("Skin is missing the '%s' resource at file '%s'!\n",resource,filename.GetData());
        return false;
    }

    csRef<iPAWSDrawable> img = new pawsImageDrawable(filename.GetData(), resname, false, csRect(), 0, 0, 0, 0);
    PawsManager::GetSingleton().GetTextureManager()->AddDrawable(img);

    return true;
}

bool pawsSkinWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if(!widget->GetName())
        return false;

    if(!strcmp(widget->GetName(),"CancelButton"))
    {
        this->Hide();
        return true;
    }

    if(!strcmp(widget->GetName(),"OKButton"))
    {
     csString zip;
     zip = currentSkin;
     config->SetStr("PlaneShift.GUI.Skin.Selected", zip);
        config->Save();
        this->Hide();
        return true;
    }

    return false;
}

