/*
 * pawsmainwidget.cpp - Author: Andrew Craig
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
// pawsmainwidget.cpp: implementation of the pawsMainWidget class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/vfs.h>
#include <iutil/document.h>
#include <iutil/evdefs.h>
#include <iutil/object.h>
#include <csutil/xmltiny.h>
#include <csutil/event.h>
#include <csgeom/transfrm.h>
#include <iengine/camera.h>
#include <iengine/sector.h>
#include <iengine/mesh.h>
#include <ivideo/fontserv.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/strutil.h"

#include "paws/pawsmanager.h"
#include "paws/pawsprefmanager.h"

#include "effects/pseffectmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psmainwidget.h"
#include "chatwindow.h"
#include "inventorywindow.h"
#include "pawsinfowindow.h"
#include "pscamera.h"
#include "pscharcontrol.h"
#include "actionhandler.h"
#include "pawsgameboard.h"


#define DEFAULT_CONFIG_FILE_NAME "/planeshift/data/options/entityinter_def.xml"
#define CONFIG_FILE_NAME         "/planeshift/userdata/options/entityinter.xml"
#define FONT_SIZE                13  // Size for the font at 800x600


//////////////////////////////////////////////////////////////////////
//                  entity types
//////////////////////////////////////////////////////////////////////

psEntityType::psEntityType(const char *id, const char *label, int dflt, const char *commandsStr, const char *labelsStr)
{
    this->id = id;
    this->label = label;
    this->dflt = dflt;
    usedCommand = dflt;

    psString pslabelsStr(labelsStr);
    psString pscommandsStr(commandsStr);

    pslabelsStr.Split(this->labels);
    pscommandsStr.Split(this->commands);
}

psEntityTypes::psEntityTypes(iObjectRegistry* objReg)
{
    this->objReg = objReg;
    vfs =  csQueryRegistry<iVFS > ( objReg);
    
    types.Push(new psEntityType("items", "Items", 2,
               "use|combine|pickup|unlock|examine", 
               "Use|Combine|Pick up|Unlock|Examine"));
    types.Push(new psEntityType("players", "Players", 0,
               "playerdesc|exchange|attack", 
               "Description|Exchange|Attack"));
    types.Push(new psEntityType("merchants", "Merchants", 0,
               "buysell|give|playerdesc|attack", 
               "Buy/Sell|Give|Description|Attack"));
    types.Push(new psEntityType("aliveNPCs", "Alive NPCs", 3,
               "buysell|give|playerdesc|attack", 
               "Buy/Sell|Give|Description|Attack"));
    types.Push(new psEntityType("deadNPCs", "Dead NPCs", 0,
               "loot|playerdesc", 
               "Loot|Description"));
    types.Push(new psEntityType("petNPCs", "Pet NPCs", 0,
               "playerdesc", 
               "Description"));
    /*If you add a new psEntityType don't forget to add the widgets in 
    data/gui/configentityinter.xml!!!*/
}

bool psEntityTypes::LoadConfigFromFile()
{
    // Check if there have been created a custom file
    // else use the default file.
    csString fileName = CONFIG_FILE_NAME;
    if (!vfs->Exists(fileName))
    {
        fileName = DEFAULT_CONFIG_FILE_NAME;
    }

    csRef<iDocument> doc = ParseFile(objReg, fileName);
    if (!doc) 
    {
        Error2("Parse error in %s", fileName.GetData());
        return false;
    }
    csRef<iDocumentNode> rootNode = doc->GetRoot();
    if(!rootNode)
    {
        Error2("No XML root in %s", fileName.GetData());
        return false;
    }
    csRef<iDocumentNode> topNode = rootNode->GetNode("EntityInteraction");
    if(!topNode)
    {
        Error2("No <EntityInteraction> tag in %s", fileName.GetData());
        return false;
    }
    for (size_t i=0; i < types.GetSize(); i++)
    {
        csRef<iDocumentNode> entNode = topNode->GetNode(types[i]->id);
        if (entNode != NULL)
        {
            types[i]->usedCommand = entNode->GetAttributeValueAsInt("value");
        }
        else
        {
            types[i]->usedCommand = types[i]->dflt;
        }
    }
    return true;
}

bool psEntityTypes::SaveConfigToFile()
{
    csString xml;
    xml = "<EntityInteraction>\n";
    for (size_t i=0; i < types.GetSize(); i++)
        xml += csString().Format("\t<%s value='%i'/>\n", types[i]->id.GetData(), types[i]->usedCommand);
    xml += "</EntityInteraction>";
    return vfs->WriteFile(CONFIG_FILE_NAME, xml.GetData(), xml.Length());
}

csString psEntityTypes::BuildDfltBehaviors()
{
    csString dflt;
    for (size_t i=0; i < types.GetSize(); i++)
    {
        if (i > 0)
            dflt += "|";
        dflt += types[i]->commands[types[i]->usedCommand];
    }
    return dflt;
}

//////////////////////////////////////////////////////////////////////
//                  psMainWidget
//////////////////////////////////////////////////////////////////////

psMainWidget::psMainWidget() : psCmdBase(NULL, NULL, PawsManager::GetSingleton().GetObjectRegistry()),
                               entTypes(PawsManager::GetSingleton().GetObjectRegistry())
{
    cel = NULL;
    chatWindow = NULL;
    locked = false;
    entTypes.LoadConfigFromFile();
    lastWidget = 0;
}

psMainWidget::~psMainWidget()
{
    if(msgqueue)
        msgqueue->Unsubscribe(this, MSGTYPE_SYSTEM);
        
    ClearFadingText();
}

bool psMainWidget::OnDoubleClick( int button, int keyModifier, int x, int y )
{
    // Check to see if an entity was selected.
    pawsWidget* underlaying = WidgetAt( x, y );

    if ( psengine->GetPSCamera() && underlaying == this )
    {
        GEMClientObject* over = FindMouseOverObject( x, y );

        if ( psengine->GetMouseBinds()->CheckBind( "EntitySelect", button, keyModifier) )
        {
            if ( over )
            {
                psUserActionMessage action(0, over->GetEID(), "dfltBehavior", entTypes.BuildDfltBehaviors());
                action.SendMessage();
            }
        }
    }
    return false;
}

bool psMainWidget::OnKeyDown( int keyCode, int key, int modifiers )
{
 if (!locked)
 {
    if (pawsMainWidget::OnKeyDown(keyCode,key,modifiers))
    {
        return true;
    }
    
    // Check for tabing in and out of chat window.
    psCharController* charctrl = psengine->GetCharControl();
    if(!charctrl)
        return true;

    if ( charctrl->MatchTrigger("Toggle chat",psControl::KEYBOARD,keyCode,modifiers) )
    {
        if ( chatWindow == NULL )
        {
            chatWindow = (pawsChatWindow*)FindWidget("ChatWindow");           
        }
               
        if ( chatWindow )
        {
            if ( chatWindow->InputActive() )
            {
                if ( lastWidget )
                {
                    BringToTop(lastWidget);                
                    PawsManager::GetSingleton().SetCurrentFocusedWidget( lastWidget );
                }                    
            }                                
            else
            {
                lastWidget = PawsManager::GetSingleton().GetCurrentFocusedWidget();
                chatWindow->Show();
                BringToTop( chatWindow );
                PawsManager::GetSingleton().SetCurrentFocusedWidget( chatWindow->FindWidget("InputText") );
            }
        }
        return true;
    }
    
    if ( charctrl->MatchTrigger("Reply tell",psControl::KEYBOARD,keyCode,modifiers) )
    {
        if ( chatWindow == NULL )
        {
            chatWindow = (pawsChatWindow*)FindWidget("ChatWindow");
        }
        if (chatWindow && HasFocus())
        {
            chatWindow->Show();
            BringToTop( chatWindow );
            PawsManager::GetSingleton().SetCurrentFocusedWidget( chatWindow->FindWidget("InputText") );
            chatWindow->AutoReply();
        }
        return true;
    }
    if ( charctrl->MatchTrigger("Close",psControl::KEYBOARD,keyCode,modifiers) )
    {
        pawsWidget* widget = PawsManager::GetSingleton().GetCurrentFocusedWidget();
        if (widget != this)
        {
            //Keep checking for a parent untill the main widget is found
            for (pawsWidget* parent = widget->GetParent();parent != this;parent=widget->GetParent())
                widget = parent;
            widget->Close();
        }
        else
        {
            pawsControlWindow* ctrlWindow =  dynamic_cast<pawsControlWindow*>(FindWidget("ControlWindow"));
            if(!ctrlWindow)
                return false; // Should never happen, but no way to be sure.
            ctrlWindow->Toggle();
        }
        return true;
    }
    
    // Unlock mouse look if a matching key was pressed
    if (charctrl->MatchTrigger("Toggle MouseLook", psControl::KEYBOARD, keyCode, modifiers) || 
    	charctrl->MatchTrigger("MouseLook", psControl::KEYBOARD, keyCode, modifiers))
    {
        psengine->GetCharControl()->GetMovementManager()->MouseLookCanAct(true);
    }
    
    return false;
   
 }
 //If we return true, it will not parse the keycode to CharControl
 return true;
    
}

bool psMainWidget::OnMouseDown( int button, int keyModifier, int x, int y )
{
    pawsWidget* bar = FindWidget("ControlWindow");
    if (!bar)
        return false;// no gui loaded, so nothing here can be done.

    // ignore mouse clicks if character is dead
    if( psengine->GetCelClient() && psengine->GetCelClient()->GetMainPlayer()
     && !psengine->GetCelClient()->GetMainPlayer()->IsAlive() )
    {
        return false;
    }

    // Check to see if we are dropping a game piece
    pawsGameBoard *gameBoard = dynamic_cast<pawsGameBoard *>(FindWidget("GameBoardWindow"));
    if (gameBoard && gameBoard->IsDragging())
    {
        gameBoard->DropPiece();
    }

    // Close interact menu automatically
    pawsWidget* interactWindow = FindWidget("InteractWindow");
    if (interactWindow)
        interactWindow->Hide();
 	// KL: Not need here anymore. Now correctly handled in psmovement/HandleZoom
 	//     because Zoom in and Zoom out are now treated as psControl::NORMAL.
 	/*
    if (button == csmbWheelUp)
    {
       psengine->GetPSCamera()->MoveDistance(-1.0f);
       return true;
    }
    if (button == csmbWheelDown)
    {
       psengine->GetPSCamera()->MoveDistance(1.0f);
       return true;
    }       
	*/

    // Check to see if an entity was selected
    pawsWidget* underlaying = WidgetAt(x,y);

    if ( psengine->GetPSCamera() && underlaying == this )
    {
        GEMClientObject* over = FindMouseOverObject( x, y );

        // Unlock mouse look if a matching key was pressed
        const psControl* mouseLook = psengine->GetCharControl()->GetTrigger("MouseLook");
        if(mouseLook->button==(uint)button && mouseLook->mods==(uint)keyModifier)
        {
            psengine->GetCharControl()->GetMovementManager()->MouseLookCanAct(true);
        }

        const psControl* mouseLookToggle = psengine->GetCharControl()->GetTrigger("Toggle MouseLook");
        if(mouseLookToggle->button==(uint)button && mouseLookToggle->mods==(uint)keyModifier)
        {
            psengine->GetCharControl()->GetMovementManager()->MouseLookCanAct(true);
        }
        
        
        if (psengine->GetMouseBinds()->CheckBind("EntitySelect", button, keyModifier))
        {
            if ( over )
            {
                psengine->GetCharControl()->GetMovementManager()->SetMouseMove(false);
                psengine->GetCharManager()->SetTarget(over,"select"); 
            }
            else 
            {
                // Deselect current target
                psengine->GetCharManager()->SetTarget(NULL, "select");

                //// Check for Action Location
                //{
                //    int poly = 0;
                //    csVector3 pos;

                //    iMeshWrapper* mesh  = psengine->GetPSCamera()->FindMeshUnder2D( x, y, &pos, &poly );

                //    if (mesh)
                //    {
                //        iSector* sector = psengine->GetPSCamera()->GetICamera()->GetSector();
                //        const char* sectorname = sector->QueryObject()->GetName();
                //        const char* meshname = mesh->QueryObject()->GetName();
                //        psengine->GetActionHandler()->Query( "SELECT", sectorname, meshname, poly, pos);
                //    }
                //}
            }
        }

        // Check Context Menu
        if (psengine->GetMouseBinds()->CheckBind("ContextMenu", button, keyModifier))
        {
            if ( over )
            {
                psengine->GetCharManager()->SetTarget(over, "context");
            }
            else
            {
                // Deselect current target
                psengine->GetCharManager()->SetTarget(NULL, "select");

                // Check for Action Location
                {
                    int poly = 0;
                    csVector3 pos;

                    iMeshWrapper* mesh  = psengine->GetPSCamera()->FindMeshUnder2D( x, y, &pos, &poly );

                    if (mesh)
                    {
                        iSector* sector = psengine->GetPSCamera()->GetICamera()->GetCamera()->GetSector();
                        const char* sectorname = sector->QueryObject()->GetName();
                        const char* meshname = mesh->QueryObject()->GetName();

                        // See if it's worth quering
                        //bool meshRecorded = psengine->GetCelClient()->IsMeshSubjectToAction(sectorname,meshname);
                        //pawsWidget* action = PawsManager::GetSingleton().FindWidget("AddEditActionWindow");

                        //if((action && action->IsVisible()) || meshRecorded)
                            psengine->GetActionHandler()->Query( "SELECT", sectorname, meshname, poly, pos);
                    }
                }
            }
        }

    }
        
    return false;
}

bool psMainWidget::OnMouseUp( int button, int keyModifier, int x, int y )
{
    return false;
}


GEMClientObject* psMainWidget::FindMouseOverObject( int x, int y )
{
    GEMClientObject* target = NULL;

    if ( !cel )
    {
        cel = psengine->GetCelClient();
    }

    if (psengine->GetPSCamera() && psengine->GetPSCamera()->IsInitialized())
    {        
        iMeshWrapper* selectedMesh = psengine->GetPSCamera()->FindMeshUnder2D(x,y);        
        if ( selectedMesh && selectedMesh->QueryObject())        
        {
            target = cel->FindAttachedObject(selectedMesh->QueryObject());
        }
    }        
    
    return target;
}

bool psMainWidget::SetupMain()
{
    psCmdBase::Setup( psengine->GetMsgHandler(), psengine->GetCmdHandler());

    float fontsize;
    //fontsize = float(FONT_SIZE) / float(1024) * ;

    fontsize = PawsManager::GetSingleton().GetFontFactor() * float(FONT_SIZE);
    
    printf("Using fontsize %d for resolution %dx%d\n",(int)fontsize,screenFrame.Width(),screenFrame.Height());

    mesgFont       = graphics2D->GetFontServer()->LoadFont("/this/data/ttf/LiberationSans-Regular.ttf",(int)fontsize);
    mesgFirstFont  = graphics2D->GetFontServer()->LoadFont("/this/data/ttf/cupandtalon.ttf",(int)(fontsize*1.75));

    msgqueue->Subscribe(this, MSGTYPE_SYSTEM);

    return true;
}


void psMainWidget::HandleMessage( MsgEntry* message )
{
    if(message->GetType() != MSGTYPE_SYSTEM)
        return;

    psSystemMessage mesg(message);

    int color = -1;

    if(mesg.type == MSG_ERROR)
        color = graphics2D->FindRGB(255,0,0);
    else if(mesg.type == MSG_INFO_SERVER)
        color = graphics2D->FindRGB(255,0,0);
    else if(mesg.type == MSG_RESULT)
        color = graphics2D->FindRGB(255,255,0);
    else if(mesg.type == MSG_OK)
        color = graphics2D->FindRGB(0,255,0);
    else if(mesg.type == MSG_ACK)
        color = graphics2D->FindRGB(0,0,255);

    if(color != -1)
        PrintOnScreen(mesg.msgline,color);
}

void psMainWidget::DrawChildren()
{
    // Call the widget's draw functions first
    pawsMainWidget::DrawChildren();

    for(size_t i = 0; i < onscreen.GetSize();i++)
    {
        ClipToParent(false);
        onscreen[i]->Draw();
    }
}

void psMainWidget::DeleteChild(pawsWidget* txtBox)
{
    for(size_t i = 0; i < onscreen.GetSize();i++)
    {
        if(txtBox == onscreen[i])
        {
            pawsWidget* widget = onscreen[i];
            onscreen.DeleteIndex(i);

            delete widget;
            return;
        }
    }

    pawsMainWidget::DeleteChild(txtBox);
}

void psMainWidget::PrintOnScreen( const char* text, int color, float ymod )
{
    if (strlen(text) < 2)
    {
        Error2("Tried to print text with less than 2 chars in a Fading way (%s)",text);
        return;
    }

    // Create a fading textbox widget
    pawsFadingTextBox* txtBox = new pawsFadingTextBox;
    txtBox->SetParent(this);
    txtBox->SetText(text,mesgFirstFont,mesgFont,color); // Set options and enable
    txtBox->Show();

    onscreen.Push(txtBox); // Do like this to have full controll on the controlls :P

    // Set the right size
    int h=0,w=0;
    txtBox->GetSize(w,h);
    txtBox->Resize(w,h,0);

    int x,y;
    x = (ScreenFrame().Width() / 2) - (w / 2);

    y = (int)(ScreenFrame().Height() * ymod) - (h / 2);
    y = y - ScreenFrame().Height() / 8;

    txtBox->MoveTo(x,y);


    // Move the others down
    if (onscreen.GetSize() < 2)
        return;

    int j = 0;
    for (int i = (int)onscreen.GetSize()-2; i >= 0; i--)
    {
        j++;
        pawsFadingTextBox* widget = (pawsFadingTextBox*)onscreen[i];
        widget->MoveDelta(0,h);
        
        if (j > 2) // force fading if there is more than 2 elements
            widget->Fade();
    }
}

void psMainWidget::ClearFadingText()
{
    for (int i = (int)onscreen.GetSize()-1; i >= 0; i--)
    {
        pawsFadingTextBox* widget = (pawsFadingTextBox*)onscreen[i];
        widget->DeleteYourself();
    }
}

