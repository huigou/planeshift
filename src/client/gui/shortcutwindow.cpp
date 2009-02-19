/*
* shortcutwindow.cpp - Author: Andrew Dai
*
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "globals.h"

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/xmltiny.h>
#include <csutil/inputdef.h>
#include <iutil/evdefs.h>
#include <ivideo/fontserv.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "paws/pawswidget.h"
#include "paws/pawsborder.h"
#include "paws/pawsmanager.h"
#include "paws/pawsprefmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawscrollbar.h"

#include "net/cmdhandler.h"

#include "gui/pawsconfigkeys.h"
#include "gui/pawscontrolwindow.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "shortcutwindow.h"
#include "pscelclient.h"

#define COMMAND_FILE         "/planeshift/userdata/options/shortcutcommands.xml"
#define DEFAULT_COMMAND_FILE "/planeshift/data/options/shortcutcommands_def.xml"

#define BUTTON_PADDING       2
#define BUTTON_SPACING       8
#define WINDOW_PADDING       5
#define SCROLLBAR_SIZE       12

#define DONE_BUTTON          1100
#define CANCEL_BUTTON        1101
#define SETKEY_BUTTON        1102
#define CLEAR_BUTTON         1103

pawsShortcutWindow::pawsShortcutWindow()
{
    vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    xml = psengine->GetXMLParser ();

    // Check if there have been created a custom file
    // else use the default file.
    csString fileName = COMMAND_FILE;
    if (!vfs->Exists(fileName))
    {
        fileName = DEFAULT_COMMAND_FILE;
    }

    LoadCommands(fileName);

    cmdsource = psengine->GetCmdHandler();
    systemText = (pawsMessageTextBox*)PawsManager::GetSingleton().FindWidget("SystemText");
    subWidget = NULL;
    shortcutText = NULL;
    textBox = NULL;
    labelBox = NULL;
}

pawsShortcutWindow::~pawsShortcutWindow()
{
    SaveCommands();
}

void pawsShortcutWindow::CalcButtonSize()
{
    int maxWidth, maxHeight;
    int width, height;
    int shortcutNum;

    maxWidth = maxHeight = 0;
    
    for (shortcutNum = 0; shortcutNum < NUM_SHORTCUTS; shortcutNum++)
    {
        if (names[shortcutNum].GetData() != NULL)
        {
            GetFont()->GetDimensions( names[shortcutNum], width, height );
        }            
        maxWidth   = MAX(width,  maxWidth);
        maxHeight  = MAX(height, maxHeight);
    }
    
    buttonWidth   = maxWidth  + 2*BUTTON_PADDING + 8;
    buttonHeight  = maxHeight + 2*BUTTON_PADDING + 2;
}

size_t pawsShortcutWindow::CalcTotalRowsNeeded(size_t matrixWidth)
{
    return (int)ceil(NUM_SHORTCUTS / (float)matrixWidth);
}

void pawsShortcutWindow::CalcMatrixSize(size_t & matrixWidth, size_t & matrixHeight)
{
    CalcButtonSize();
    matrixWidth   = (screenFrame.Width()   - 2*WINDOW_PADDING - SCROLLBAR_SIZE/2) / (buttonWidth  + BUTTON_SPACING);
    matrixHeight  = (screenFrame.Height()  - 2*WINDOW_PADDING)                  / (buttonHeight + BUTTON_SPACING);

    matrixWidth   = MAX(matrixWidth,  1);
    matrixHeight  = MIN(MAX(matrixHeight, 1), CalcTotalRowsNeeded(matrixWidth));
}

void pawsShortcutWindow::RebuildMatrix()
{
    size_t matrixWidth, matrixHeight;
    pawsButton * button;
    size_t i, k;

    // delete old matrix
    for (i=0; i < matrix.GetSize(); i++)
    {
        for (k=0; k < matrix[i].GetSize(); k++)
        {
            DeleteChild(matrix[i][k]);
        }
    }            
    matrix.DeleteAll();
    
    CalcMatrixSize(matrixWidth, matrixHeight);
    
    // create new matrix
    matrix.SetSize(matrixWidth);
    for (i=0; i < matrix.GetSize(); i++)
    {
        matrix[i].SetSize(matrixHeight);
        for (k=0; k < matrix[i].GetSize(); k++)
        {
            button = new pawsButton;
            button->SetSound("gui.shortcut");
            AddChild(button);
            button->SetBackground("Scaling Button Up");
            matrix[i][k] = button;
        }
    }

    UpdateMatrix();
    LayoutMatrix();
    
    CS_ASSERT(matrix.GetSize()>0);
    if (scrollBar != NULL)
    {    
        scrollBar->SetMaxValue(ceil(    NUM_SHORTCUTS / (float)(matrixWidth*matrixHeight)    )   - 1   );
    }        
}

void  pawsShortcutWindow::LayoutMatrix()
{
    pawsButton * button;
    size_t i, k;
    
    for (i=0; i < matrix.GetSize(); i++)
    {
        for (k=0; k < matrix[i].GetSize(); k++)
        {
            button = matrix[i][k];
            button->SetRelativeFrame(WINDOW_PADDING + (int)i * (buttonWidth  + BUTTON_SPACING),
                                     WINDOW_PADDING + (int)k * (buttonHeight + BUTTON_SPACING),
                                     buttonWidth, buttonHeight);
        }
    }
}

void  pawsShortcutWindow::UpdateMatrix()
{
    pawsButton * button;
    size_t shortcutNum;
    int scrollBarPos;
    size_t i, k;

    if (scrollBar != NULL)
    {
        scrollBarPos = (int)scrollBar->GetCurrentValue();
    }        
    else
    {
        scrollBarPos = 0;
    }        

    for (i=0; i < matrix.GetSize(); i++)
    {
        for (k=0; k < matrix[i].GetSize(); k++)
        {
            button = matrix[i][k];
            shortcutNum = i + k * matrix.GetSize() + scrollBarPos*matrix.GetSize()*matrix[0].GetSize();
            if (shortcutNum < NUM_SHORTCUTS)
            {
                button->Show();
                button->SetText(names[shortcutNum]);
                button->SetID(2000 + (int)shortcutNum);
            }
            else
            {
                button->Hide();
            }                
        }
    }        
}

bool pawsShortcutWindow::PostSetup()
{
    // Create the scroll bar
    scrollBar = new pawsScrollBar;
    AddChild( scrollBar );
    scrollBar->SetHorizontal(false);
    int attach = ATTACH_BOTTOM | ATTACH_RIGHT | ATTACH_TOP;
    scrollBar->SetAttachFlags( attach );
    scrollBar->SetRelativeFrame( defaultFrame.Width()-SCROLLBAR_SIZE, 0, SCROLLBAR_SIZE, defaultFrame.Height()-8);
    scrollBar->PostSetup();
    scrollBar->Resize();
    scrollBar->SetTickValue( 1.0 );

    RebuildMatrix();

    return true;
}

void pawsShortcutWindow::OnResize()
{
    size_t newMatrixWidth, newMatrixHeight;

    if (matrix.GetSize() == 0)
    {
        return;
    }        
    
    // If the matrix size that is appropriate for current window size differs from actual matrix size,
    // we will adjust the matrix to it
    CalcMatrixSize(newMatrixWidth, newMatrixHeight);
    if (matrix.GetSize() != newMatrixWidth   ||   matrix[0].GetSize() != newMatrixHeight)
    {
        RebuildMatrix();
    }                    
}

bool pawsShortcutWindow::OnMouseDown( int button, int modifiers, int x, int y )
{
    if ( button == csmbWheelUp )
    {
        scrollBar->ScrollUp();
        return true;
    }
    else if ( button == csmbWheelDown )
    {
        scrollBar->ScrollDown();
        return true;
    }
    else
    {
        return pawsControlledWindow::OnMouseDown(button, modifiers, x, y);
    }
}

// bool pawsShortcutWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
bool pawsShortcutWindow::OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (!subWidget)
        subWidget = PawsManager::GetSingleton().FindWidget("ShortcutEdit");

    if (!labelBox)
        labelBox = dynamic_cast <pawsEditTextBox*> (subWidget->FindWidget("LabelBox"));

    if (!textBox)
        textBox = dynamic_cast <pawsMultilineEditTextBox*> (subWidget->FindWidget("CommandBox"));

    if (!shortcutText)
        shortcutText = dynamic_cast <pawsTextBox*> (subWidget->FindWidget("ShortcutText"));


    // These should not be NULL
    CS_ASSERT(subWidget); CS_ASSERT(labelBox); CS_ASSERT(textBox); CS_ASSERT(shortcutText);



    BringToTop(this);

    // Finished Configuring command button
    switch ( widget->GetID() )
    {
        case DONE_BUTTON:
        {
            if (!labelBox->GetText() || *(labelBox->GetText()) == '\0')
            {
                labelBox->Clear();
                textBox->Clear();
            }
            // Otherwise save the label and command as it is
            names[edit] = labelBox->GetText();
            cmds[edit] = textBox->GetText();
        
            CalcButtonSize();
            UpdateMatrix();
            LayoutMatrix();
            SetWindowSizeToFitMatrix();

            PawsManager::GetSingleton().SetModalWidget(NULL);
            PawsManager::GetSingleton().SetCurrentFocusedWidget(this);
            subWidget->Hide();

            pawsWidget * configKeyAsWidget = PawsManager::GetSingleton().FindWidget("ConfigKeys");
        
            pawsConfigKeys * configKey = dynamic_cast<pawsConfigKeys*>
                (configKeyAsWidget);
                
            if (configKey)
            {
                configKey->UpdateNicks();
            }                

            SaveCommands();
            return true;
        }
        case CLEAR_BUTTON:
        {
            labelBox->Clear();
            textBox->Clear();
            return true;
        }
        case CANCEL_BUTTON:
        {
            PawsManager::GetSingleton().SetModalWidget(NULL);
            PawsManager::GetSingleton().SetCurrentFocusedWidget(this);
            subWidget->Hide();
            return true;
        }
        case SETKEY_BUTTON:
        {
            pawsWidget * fingWndAsWidget;

            fingWndAsWidget = PawsManager::GetSingleton().FindWidget("FingeringWindow");
            if (fingWndAsWidget == NULL)
            {
                Error1("Could not find widget FingeringWindow");
                return false;
            }
            pawsFingeringWindow * fingWnd = dynamic_cast<pawsFingeringWindow *>(fingWndAsWidget);
            if (fingWnd == NULL)
            {
                Error1("FingeringWindow is not pawsFingeringWindow");
                return false;
            }
            fingWnd->ShowDialog(this, labelBox->GetText());
            
            return true;
        }
    }            // switch( ... )

    // Execute clicked on button
    if ( mouseButton == csmbLeft && !(keyModifier & CSMASK_CTRL))
        ExecuteCommand( widget->GetID() - 2000, false );
    // Configure the button that was clicked on
    else if ( mouseButton == csmbRight || (mouseButton == csmbLeft && (keyModifier & CSMASK_CTRL)) )
    {
        edit = widget->GetID() - 2000;
        if ( edit < 0 || edit >= NUM_SHORTCUTS )
            return false;

        if (!subWidget || !labelBox || !textBox || !shortcutText)
            return false;

        if ( names[edit] && names[edit].Length() )
            labelBox->SetText( names[edit].GetData() );
        else
            labelBox->Clear();

        if ( cmds[edit] && cmds[edit].Length() )
        {
            textBox->SetText( cmds[edit].GetData() );
            shortcutText->SetText( GetTriggerText(edit) );
        }
        else
        {
            textBox->Clear();
            shortcutText->SetText("");
        }

        subWidget->Show();
        PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);
    }
    else
    {
        return false;
    }
    return true;
}

csString pawsShortcutWindow::GetTriggerText(int shortcutNum)
{
    psCharController* manager = psengine->GetCharControl();
    csString str;
    str.Format("Shortcut %d",shortcutNum+1);
            
    const psControl* ctrl = manager->GetTrigger( str );
    if (!ctrl)
    {
        printf("Unimplemented action '%s'!\n",str.GetData());
        return "";
    }

    return ctrl->ToString();
}

bool pawsShortcutWindow::OnScroll( int direction, pawsScrollBar* widget )
{    
    UpdateMatrix();
    return true;
}

void pawsShortcutWindow::SetWindowSizeToFitMatrix()
{
    CS_ASSERT(matrix.GetSize() > 0);
    CS_ASSERT(matrix[0].GetSize() > 0);

    int width  = 2*WINDOW_PADDING + (int)matrix.GetSize()    * (buttonWidth  + BUTTON_SPACING) + SCROLLBAR_SIZE/2 +1;
    int height = 2*WINDOW_PADDING + (int)matrix[0].GetSize() * (buttonHeight + BUTTON_SPACING) + 1;

    if (width > screenFrame.Width())
        width = screenFrame.Width();
    if (height > screenFrame.Height())
        height = screenFrame.Height();

    SetSize(width, height);
}

void pawsShortcutWindow::StopResize()
{
    SetWindowSizeToFitMatrix();
}


void pawsShortcutWindow::LoadDefaultCommands()
{
    LoadCommands(DEFAULT_COMMAND_FILE);
    UpdateMatrix();
}

void pawsShortcutWindow::LoadCommands(const char * fileName)
{
    int number;
    // Read button commands
    csRef<iDocument> doc = xml->CreateDocument();
    
    csRef<iDataBuffer> buf (vfs->ReadFile (fileName));
    if (!buf || !buf->GetSize ())
    {
        return ;
    }
    const char* error = doc->Parse( buf );
    if ( error )
    {
        printf("Error loading shortcut window commands: %s\n", error);
        return ;
    }

    csRef<iDocumentNodeIterator> iter = doc->GetRoot()->GetNode("shortcuts")->GetNodes();

    bool zerobased = false;

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> child = iter->Next();

        if ( child->GetType() != CS_NODE_ELEMENT )
            continue;
        sscanf(child->GetValue(), "shortcut%d", &number);
        if(number == 0)
        {
            zerobased = true;
        }
        if(!zerobased)
        {
            number--;
        }
        if (number < 0 || number >= NUM_SHORTCUTS)
            continue;
        names[number] = child->GetAttributeValue("name");
        cmds[number] = child->GetContentsValue();
    }
    
    // the 0th entry must exist to correctly make the matrix
    if (names[0].IsEmpty())
        names[0].Clear();
    if (cmds[0].IsEmpty())
        cmds[0].Clear();
}

void pawsShortcutWindow::SaveCommands(void)
{
    bool found = false;
    int i;
    for (i = 0;i < NUM_SHORTCUTS;i++)
    {
        if (cmds[i].IsEmpty())
            continue;

        found = true;
        break;
    }
    if (!found) // Don't save if no commands have been defined
        return ;

    // Save the commands with their labels
    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot ();
    csRef<iDocumentNode> parentMain = root->CreateNodeBefore(CS_NODE_ELEMENT);
    parentMain->SetValue("shortcuts");
    csRef<iDocumentNode> parent;

    csRef<iDocumentNode> text;
    csString temp;
    for (i=0;i < NUM_SHORTCUTS; i++)
    {
        if (cmds[i].IsEmpty())
            continue;
        parent = parentMain->CreateNodeBefore (CS_NODE_ELEMENT);
        temp.Format("shortcut%d", i + 1);
        parent->SetValue(temp);

        if (names[i].IsEmpty())
        {
            temp.Format("%d", i);
            parent->SetAttribute("name", temp);
        }
        else
        {
            parent->SetAttribute("name", names[i].GetData());
        }
        text = parent->CreateNodeBefore(CS_NODE_TEXT);
        text->SetValue(cmds[i].GetData());
    }
    doc->Write(vfs, COMMAND_FILE);
}

        
void pawsShortcutWindow::ExecuteCommand(int shortcutNum, bool local)
{
    if (local)
    {
        shortcutNum = shortcutNum + (int)scrollBar->GetCurrentValue()*(int)matrix.GetSize();
    }

    if (shortcutNum < 0 || shortcutNum >= NUM_SHORTCUTS)
    {
        return;
    }
    
    const char* current = cmds[shortcutNum].GetData();
    if (current)
    {
        const char* pos;
        const char* next = current - 1;
        csString command;
        
        while (next && *(next + 1))
        {
            // Move command pointer to start of next command
            pos = next + 1;
            command = pos;
            // Execute next command delimited by a line feed
            if (*pos)
            {
                // Find location of next command
                next = strchr(pos, '\n');
                if (next)
                    command.Truncate(next - pos);
            }

            if(command.GetAt(0) == '#') //it's a comment skip it
            {
                continue;
            }
 
            if(command.FindFirst("$target") != command.Length() - 1)
            {
                GEMClientObject *object= psengine->GetCharManager()->GetTarget();
                if(object)
                {
                    csString name = object->GetName(); //grab name of target
                    size_t space = name.FindFirst(" ");
                    name = name.Slice(0,space);
                    command.ReplaceAll("$target",name); // actually replace target
                }
            }
            if(command.FindFirst("$guild") != command.Length() - 1)
            {
                GEMClientActor *object= dynamic_cast<GEMClientActor*>(psengine->GetCharManager()->GetTarget());
                if(object)
                {
                    csString name = object->GetGuildName(); //grab guild name of target
                    command.ReplaceAll("$guild",name); // actually replace target
                }
            }
            if(command.FindFirst("$race") != command.Length() - 1)
            {
                GEMClientActor *object= dynamic_cast<GEMClientActor*>(psengine->GetCharManager()->GetTarget());
                if(object)
                {
                    csString name = object->race; //grab race name of target
                    command.ReplaceAll("$race",name); // actually replace target
                }
            }
            if(command.FindFirst("$sir") != command.Length() - 1)
            {
                GEMClientActor *object= dynamic_cast<GEMClientActor*>(psengine->GetCharManager()->GetTarget());
                if(object)
                {
                    csString name = "Dear";
                    switch (object->gender)
                    {
                    case PSCHARACTER_GENDER_NONE:
                        name = "Gemma";
                        break;
                    case PSCHARACTER_GENDER_FEMALE:
                        name = "Lady";
                        break;
                    case PSCHARACTER_GENDER_MALE:
                        name = "Sir";
                        break;
                    }
                    command.ReplaceAll("$sir",name); // actually replace target
                }
            }
            const char* errorMessage = cmdsource->Publish( command );
            if ( errorMessage )
                systemText->AddMessage( errorMessage );
            
        }
        
    }
}

const csString& pawsShortcutWindow::GetCommandName(int shortcutNum, bool local)
{
    if (local)
    {
        shortcutNum = shortcutNum + (int)scrollBar->GetCurrentValue()*(int)matrix.GetSize();
    }

    if (shortcutNum < 0 || shortcutNum >= NUM_SHORTCUTS)
    {
        static csString error("Out of range");
        return error;
    }

    if (!names[shortcutNum])
    {
        static csString unused("Unused");
        return unused;
    }
    
    return names[shortcutNum];
}

bool pawsShortcutWindow::OnFingering(csString string, psControl::Device device, uint button, uint32 mods)
{
    pawsFingeringWindow* fingWnd = dynamic_cast<pawsFingeringWindow*>(PawsManager::GetSingleton().FindWidget("FingeringWindow"));
    if (fingWnd == NULL || !fingWnd->IsVisible())
        return true;

    csString editedCmd;
    editedCmd.Format("Shortcut %d",edit+1);

    bool changed = false;

    if (string == NO_BIND)  // Removing trigger
    {
        psengine->GetCharControl()->RemapTrigger(editedCmd,psControl::NONE,0,0);
        changed = true;
    }
    else  // Changing trigger
    {
        changed = psengine->GetCharControl()->RemapTrigger(editedCmd,device,button,mods);
    }

    if (changed)
    {
        shortcutText->SetText(GetTriggerText(edit));
        return true;  // Hide fingering window
    }
    else  // Map already exists
    {
        const psControl* other = psengine->GetCharControl()->GetMappedTrigger(device,button,mods);
        CS_ASSERT(other);
        fingWnd->SetCollisionInfo(other->name);
        return false;
    }
}
