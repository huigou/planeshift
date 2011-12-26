/*
* pawsnpcdialog.cpp - Author: Christian Svensson
*
* Copyright (C) 2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iutil/objreg.h>

#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"

#include "../globals.h"
#include "paws/pawslistbox.h"
#include "paws/pawsborder.h"
#include "gui/pawscontrolwindow.h"
#include "pscelclient.h"
#include "pscamera.h"

#include "pawsnpcdialog.h"
#include "psclientchar.h"


/**
 * TODO:
 * -fix handling of free text and questions
 * -fix the graphics of the chat bubbles generically
 * -add support for numkey/enter action on the list version
 */

pawsNpcDialogWindow::pawsNpcDialogWindow()
{
    responseList = NULL;
    useBubbles = true;
    ticks = 0;
    cameraMode = 0;
}

bool pawsNpcDialogWindow::PostSetup()
{
	psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_DIALOG_MENU );
                
    responseList = (pawsListBox*)FindWidget("ResponseList");

    //loads the options regarding this window
    if(!LoadSetting())
    {
        //setup the window with defaults.
        SetupWindowWidgets();
    }
    return true;
}

void pawsNpcDialogWindow::Draw()
{
    if(useBubbles && ticks != 0 && csGetTicks()-ticks > 5000)
    {//let this dialog invisible after 5 secs when npc finishes his speech
        SetVisibility(false);
        FindWidget("SpeckBubble")->SetVisibility(false);
        ticks = 0;
    }
    pawsWidget::Draw();
}

void pawsNpcDialogWindow::DrawBackground()
{
    if(!useBubbles)
    {
        pawsWidget::DrawBackground();
    }

}

bool pawsNpcDialogWindow::OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers )
{
    //manage when the window has bubbles enabled
    if(useBubbles)
    {
        pawsEditTextBox * textBox = dynamic_cast<pawsEditTextBox*>(FindWidget("InputText"));

        //check if the text entry box has focus else just let the basic
        //pawswidget implementation handle it
        if(!textBox || !textBox->HasFocus())
        {
            return pawsWidget::OnKeyDown(keyCode, key, modifiers);
        }
        switch ( key )
        {
            case CSKEY_ENTER:
            {
               
                csString text = textBox->GetText();
                csString answer = "";
                //
                for (size_t i = 0 ; i < questInfo.GetSize(); i++)
                {
                    csString tmp = questInfo[i].text;
                    tmp.DeleteAt(0,2);
                    tmp = tmp.Trim();
                    if(text == tmp)
                    {
                        answer = questInfo[i].trig;
                        break;
                    }
                }

                if(answer != "")
                {
                    printf("%s\n", answer.GetData());
                    csString cmd;
                    if (answer.GetAt(0) == '=') // prompt window signal
                    {
                        pawsStringPromptWindow::Create(csString(answer.GetData()+1),
                            csString(""),
                            false, 320, 30, this, answer.GetData()+1 );
                    }
                    else
                    {
                        if (answer.GetAt(0) != '<')
                        {
                            cmd.Format("/tellnpc %s", answer.GetData() );
                            psengine->GetCmdHandler()->Publish(cmd);
                        }
                        else
                        {
                            psSimpleStringMessage gift(0,MSGTYPE_EXCHANGE_AUTOGIVE,answer);
                            gift.SendMessage();
                        }
                        DisplayTextBubbles(text.GetData());
                    }
                }

                //Hide();
                textBox->Clear();
                if(answer == "")
                    psengine->GetPSCamera()->SetCameraMode(cameraMode);
                break;
            }
       
        }
        BringToTop(textBox);
    }
    else
    {
        return pawsWidget::OnKeyDown(keyCode, key, modifiers);
    }
    return true;
}

bool pawsNpcDialogWindow::OnButtonPressed( int button, int keyModifier, pawsWidget* widget )
{
    if(useBubbles)
    {
        if(widget)
        {//process the left/right arrow clicking event
            csString name = widget->GetName();
            if(name.StartsWith("Bubble"))
            {
                int entryNumber = widget->GetID()-100; //subtract 100 as ids start with 100
                csString cmd;
                cmd.Format("/tellnpc %s", questInfo.Get(displayIndex+entryNumber).trig.GetData());
                psengine->GetCmdHandler()->Publish(cmd);
                DisplayTextBubbles(questInfo.Get(displayIndex+entryNumber).text.GetData());
            }
            else
            {
                if(name == "LeftArrow")
                {
                    if(displayIndex >= 3) displayIndex -= 3;
                    else displayIndex = 0;
                }
                else if(name == "RightArrow")
                {
                    if(displayIndex < questInfo.GetSize() - 3) displayIndex += 3;
                    else displayIndex = questInfo.GetSize() - 3;
                }
                
                DisplayQuest(displayIndex);
                
                pawsWidget * pw1 = FindWidget("LeftArrow");
                pawsWidget * pw2 = FindWidget("RightArrow");

                pw1->SetVisibility(true);
                pw2->SetVisibility(true);

                if(displayIndex < 3) pw1->SetVisibility(false);
                if(displayIndex >= questInfo.GetSize() - 3) pw2->SetVisibility(false);
            }
        }
        pawsWidget *textBox = FindWidget("InputText");
        if(textBox)
            PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);
        return true;
    }
    
    return OnButtonPressed(button, keyModifier, widget);
}

bool pawsNpcDialogWindow::OnMouseDown( int button, int modifiers, int x , int y )
{
    //first let pawsWidget handle the event
    bool result = pawsWidget::OnMouseDown( button, modifiers, x, y );
    //if we use bubbles set the textinput as automatically focused so
    //it's possible to write on it directly.
    if(useBubbles)
    {
        //check which widget was pressed
        //mainWidget->WidgetAt(data.x, data.y);
        pawsWidget *textBox = FindWidget("InputText");
        //Don't attempt anything if the textbox is missing. Else
        //the main widget will be focused.
        if(textBox)
        {
            PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);
        }
        return true;
    }
    return result;
}

void pawsNpcDialogWindow::DisplayQuest(unsigned int index)
{
    unsigned int c = 1;
    csString bname = "";
    for (unsigned int i = index; i < questInfo.GetSize() && c < 3; i++, c++)
    {//set visible bubbles
        bname = "Bubble";
        bname.Append(c);
        pawsWidget * pw = FindWidget(bname);
        pawsTextBox * qn = dynamic_cast<pawsTextBox*>(pw->FindWidget("QuestName"));
        pawsMultiLineTextBox * qt = dynamic_cast<pawsMultiLineTextBox *>(pw->FindWidget("QuestText"));

        qn->SetText(questInfo[i].title);
        qt->SetText(questInfo[i].text);
        pw->SetVisibility(true);
    }
    for (;c <= 3 ; c++)
    {//set invisible bubbles
        bname = "Bubble";
        bname.Append(c);
        pawsWidget * pw = FindWidget(bname);
        pw->SetVisibility(false);
    }
}

void pawsNpcDialogWindow::LoadQuest(csString xmlstr)
{
    questInfo.DeleteAll();
    displayIndex = 0;

    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

    csRef<iDocument> doc= xml->CreateDocument();
    const char *error = doc->Parse( xmlstr );
    if (error)
    {
        Error2("%s\n", error);
        return;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    csRef<iDocumentNode> topNode = root->GetNode(xmlbinding!=""?xmlbinding:name);
    
    csRef<iDocumentNode> options = topNode->GetNode("options");

    csRef<iDocumentNodeIterator> it  = options->GetNodes("row");
    
    while(it != 0 && it->HasNext())
    {
        csRef<iDocumentNode> cur = it->Next();
        if(cur->GetAttributeValueAsInt("heading"))
        {
            QuestInfo qi;
            csRef<iDocumentNode> txt = cur->GetNode("text");
            qi.title = txt->GetContentsValue();
            questInfo.Push(qi);
        }
        else
        {
            csRef<iDocumentNode> txt = cur->GetNode("text");
            csRef<iDocumentNode> trg = cur->GetNode("trig");
            QuestInfo& qi = questInfo[questInfo.GetSize()-1];
            qi.text = txt->GetContentsValue();
            qi.trig = trg->GetContentsValue();
        }
    }
    displayIndex = 0;

    pawsWidget * pw1 = FindWidget("LeftArrow");
    pawsWidget * pw2 = FindWidget("RightArrow");

    pw2->SetVisibility(true);
    pw1->SetVisibility(false);

    if(displayIndex >= questInfo.GetSize() - 3) pw2->SetVisibility(false);
}

void pawsNpcDialogWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status == LISTBOX_HIGHLIGHTED)
    {
		pawsTextBox *fld = dynamic_cast<pawsTextBox *>(widget->GetSelectedRow()->FindWidgetXMLBinding("text"));
		Debug2(LOG_QUESTS, 0, "Pressed: %s\n",fld->GetText() );
    }
	else if (status == LISTBOX_SELECTED)
	{
        //if no row is selected
        if(!widget->GetSelectedRow())
            return;

		pawsTextBox *fld  = dynamic_cast<pawsTextBox *>(widget->GetSelectedRow()->FindWidgetXMLBinding("text"));
		Debug2(LOG_QUESTS, 0,"Player chose '%s'.\n", fld->GetText() );
		pawsTextBox *trig = dynamic_cast<pawsTextBox *>(widget->GetSelectedRow()->FindWidgetXMLBinding("trig"));
		Debug2(LOG_QUESTS, 0,"Player says '%s'.\n", trig->GetText() );

        csString trigger(trig->GetText());

        // Send the server the original trigger
    	csString cmd;
        if (trigger.GetAt(0) == '=') // prompt window signal
		{
			pawsStringPromptWindow::Create(csString(trigger.GetData()+1),
				                           csString(""),
				                           false, 320, 30, this, trigger.GetData()+1 );
		}
		else
		{
			if (trigger.GetAt(0) != '<')
			{
	    		cmd.Format("/tellnpc %s", trigger.GetData() );
				psengine->GetCmdHandler()->Publish(cmd);
			}
			else
			{
				psSimpleStringMessage gift(0,MSGTYPE_EXCHANGE_AUTOGIVE,trigger);
				gift.SendMessage();
			}
			DisplayTextBubbles(fld->GetText());
		}
		Hide();
	}
}

void pawsNpcDialogWindow::DisplayTextBubbles(const char *sayWhat)
{
	// Now send the chat window and chat bubbles the nice menu text
	csString text(sayWhat);
	size_t dot = text.FindFirst('.'); // Take out the numbering to display
	if (dot != SIZET_NOT_FOUND)
	{
		text.DeleteAt(0,dot+1);
	}
	csString cmd;
	cmd.Format("/tellnpcinternal %s", text.GetData() );
	psengine->GetCmdHandler()->Publish(cmd);
	responseList->Clear();
}

void pawsNpcDialogWindow::HandleMessage( MsgEntry* me )
{
    if ( me->GetType() == MSGTYPE_DIALOG_MENU )
    {
        psDialogMenuMessage mesg(me);

		Debug2(LOG_QUESTS, 0,"Got psDialogMenuMessage: %s\n", mesg.xml.GetDataSafe() );
		responseList->Clear();

		SelfPopulateXML(mesg.xml);

        if(useBubbles)
        {
            LoadQuest(mesg.xml);
            DisplayQuest(displayIndex);

            FindWidget("FreeBubble")->SetVisibility(true);
        }

		AdjustForPromptWindow();

		Show();
    }
}

void pawsNpcDialogWindow::NpcSays(csArray<csString>& lines,GEMClientActor *actor)
{
    //this is used only when using the chat bubbles interface
    if(!useBubbles)
    {
        return;
    }
    //display npc response
    if(actor && psengine->GetCharManager()->GetTarget() == actor)
    {
        csString all = "";
        for (size_t i = 0 ; i < lines.GetSize() ; i++)
        {
            all += lines[i];
        }
        dynamic_cast<pawsMultiLineTextBox*>(FindWidget("SpeckBubble")->FindWidget("BubbleText"))->SetText(all);
        FindWidget("SpeckBubble")->SetVisibility(true);
        FindWidget("Bubble1")->SetVisibility(false);
        FindWidget("Bubble2")->SetVisibility(false);
        FindWidget("Bubble3")->SetVisibility(false);
        FindWidget("LeftArrow")->SetVisibility(false);
        FindWidget("RightArrow")->SetVisibility(false);
        FindWidget("FreeBubble")->SetVisibility(false);

        ticks = csGetTicks();

        Show();//show the npc dialog
    }

    
}


void pawsNpcDialogWindow::AdjustForPromptWindow()
{
    csString str;

    for (size_t i=0; i<responseList->GetRowCount(); i++)
    {
        str = responseList->GetTextCellValue(i,0);
        size_t where = str.Find("?=");
        if (where != SIZET_NOT_FOUND) // we have a prompt choice
        {
            pawsTextBox *hidden = (pawsTextBox *)responseList->GetRow(i)->GetColumn(1);
            if (where != SIZET_NOT_FOUND)
            {
                str.DeleteAt(where,1); // take out the ?
                // Save the question prompt, starting with the =, in the hidden column
                hidden->SetText(str.GetData() + where);
                str.DeleteAt(where,1); // take out the =

                // now change the visible menu choice to something better
                pawsTextBox *prompt = (pawsTextBox *)responseList->GetRow(i)->GetColumn(0);

                csString menuPrompt(str);
                menuPrompt.Insert(where,"<Answer ");
                menuPrompt.Append('>');
                prompt->SetText(menuPrompt);
            }
        }
    }

    for (size_t i = 0 ; i < questInfo.GetSize(); i++)
    {
        QuestInfo & qi = questInfo[i];
        str = qi.text;
        size_t where = str.Find("?=");
        if(where != SIZET_NOT_FOUND)
        {
            str.DeleteAt(where,1);
            qi.trig = csString(str.GetData() + where);
            str.DeleteAt(where,1);
            str.Insert(where,"I know the answer to : ");
            qi.text = str;
        }
    }
}

void pawsNpcDialogWindow::OnStringEntered(const char* name, int /*param*/, const char* value)
{
    //The user cancelled the operation. So show again the last window and do nothing else.
    if(value == NULL) 
    { 
        Show(); 
        return; 
    }

	Debug3(LOG_QUESTS, 0,"Got name=%s, value=%s\n", name, value);

	csString cmd;
	cmd.Format("/tellnpc %s", value );
	psengine->GetCmdHandler()->Publish(cmd);
	DisplayTextBubbles(value);
}

void pawsNpcDialogWindow::SetupWindowWidgets()
{
    pawsWidget *lists = FindWidget("Lists");
    pawsWidget *bubbles = FindWidget("Bubbles");
    if(useBubbles)
    {
        if(border)  border->Hide();
        //if(close_widget) close_widget->Hide();
        if(lists)   lists->Hide();
        if(bubbles) bubbles->Show();
        defaultFrame = bubbles->DefaultFrame();
        Resize();
        SetSize(defaultFrame.Width(), defaultFrame.Height());
        MoveTo(0,0);
    }
    else
    {
        if(border)  border->Show();
        if(close_widget) close_widget->Show();
        if(lists)   lists->Show();
        if(bubbles) bubbles->Hide();
        defaultFrame = lists->DefaultFrame();
        Resize();
    }
}

void pawsNpcDialogWindow::Show()
{
    pawsWidget::Show();
    SetupWindowWidgets();

    //split apart just for visiblity ordering in source
    if(useBubbles)
    {
        GEMClientObject* cobj = psengine->GetCharManager()->GetTarget();
        if(cobj)
        {//let the camera focus upon the target npc
            csRef<psCelClient> celclient = psengine->GetCelClient();
            GEMClientObject * mobj = celclient->GetMainPlayer();
            csVector3 p1 = cobj->GetPosition();
            csVector3 p2 = mobj->GetPosition();
            csVector3 direction = p1 - p2;
            if (direction.x == 0.0f)
                direction.x = 0.00001f;
            float nyaw = atan2(-direction.x, -direction.z);

            csVector3 pos;
            float yrot,rot;
            iSector * isect;
            dynamic_cast<GEMClientActor *>(mobj)->GetLastPosition(p1,yrot,isect);
            dynamic_cast<GEMClientActor *>(mobj)->SetPosition(p1,nyaw,isect);
            dynamic_cast<GEMClientActor *>(cobj)->GetLastPosition(p2,rot,isect);
            dynamic_cast<GEMClientActor *>(cobj)->SetPosition(p2,nyaw+3.1415f,isect);
            cameraMode = psengine->GetPSCamera()->GetCameraMode();//get the camera's current mode
            psengine->GetPSCamera()->SetCameraMode(0);//set the camera to the first person mode
        }

    }   

    
}

bool pawsNpcDialogWindow::LoadSetting()
{    
    csRef<iDocument> doc;
    csRef<iDocumentNode> root,npcDialogNode, npcDialogOptionsNode;
    csString option;

    doc = ParseFile(psengine->GetObjectRegistry(), CONFIG_NPCDIALOG_FILE_NAME);
    if(doc == NULL)
    {
        //load the default configuration file in case the user one fails (missing or damaged)
        doc = ParseFile(psengine->GetObjectRegistry(), CONFIG_NPCDIALOG_FILE_NAME_DEF);    
        if(doc == NULL)
        {
            Error2("Failed to parse file %s", CONFIG_NPCDIALOG_FILE_NAME_DEF);
            return false;
        }    
    }
   
    root = doc->GetRoot();
    if(root == NULL)
    {
        Error1("npcdialog_def.xml or npcdialog.xml has no XML root");
        return false;
    }
    
    npcDialogNode = root->GetNode("npcdialog");
    if(npcDialogNode == NULL)
    {
        Error1("npcdialog_def.xml or npcdialog.xml has no <npcdialog> tag");
        return false;
    }

    // Load options for the npc dialog
    npcDialogOptionsNode = npcDialogNode->GetNode("npcdialogoptions");
    if(npcDialogOptionsNode != NULL)
    {
        csRef<iDocumentNodeIterator> oNodes = npcDialogOptionsNode->GetNodes();
        while(oNodes->HasNext())
        {
            csRef<iDocumentNode> option = oNodes->Next();
            csString nodeName (option->GetValue());

            if(nodeName == "usenpcdialog")
            {    //showWindow->SetState(!option->GetAttributeValueAsBool("value"));
                useBubbles = option->GetAttributeValueAsBool("value");
            }
        }
    }

    //setup the window with the new settings
    SetupWindowWidgets();

    return true;
}

void pawsNpcDialogWindow::SaveSetting()
{
    csRef<iFile> file;
    file = psengine->GetVFS()->Open(CONFIG_NPCDIALOG_FILE_NAME,VFS_FILE_WRITE);

    csRef<iDocumentSystem> docsys = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());

    csRef<iDocument> doc = docsys->CreateDocument();        
    csRef<iDocumentNode> root,defaultRoot, npcDialogNode, npcDialogOptionsNode, useNpcDialogNode;

    root = doc->CreateRoot();

    npcDialogNode = root->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    npcDialogNode->SetValue("npcdialog");

    npcDialogOptionsNode = npcDialogNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    npcDialogOptionsNode->SetValue("npcdialogoptions");

    useNpcDialogNode = npcDialogOptionsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    useNpcDialogNode->SetValue("usenpcdialog");
    useNpcDialogNode->SetAttributeAsInt("value", useBubbles? 1 : 0);

    doc->Write(file);
}
