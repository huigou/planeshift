/*
 * pawsSketchWindow.cpp - Author: Keith Fulton
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// PAWS INCLUDES
#include "pawsilluminationwindow.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "net/msghandler.h"
#include "util/log.h"
#include "util/psxmlparser.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsSketchWindow::pawsSketchWindow()
{
    selectedIndex = SIZET_NOT_FOUND;
    dirty     = false;
    editMode  = false;
    mouseDown = false;
}

pawsSketchWindow::~pawsSketchWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this,MSGTYPE_VIEW_SKETCH);
}

bool pawsSketchWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_VIEW_SKETCH);
    return true;
}

iGraphics2D *pawsSketchWindow::GetG2D()
{
    csRef<iGraphics2D> gfx = PawsManager::GetSingleton().GetGraphics2D();
    return gfx;
}

void pawsSketchWindow::HandleMessage( MsgEntry* me )
{   
    editMode      = false;
    stringPending = false;
    psSketchMessage msg( me );

    currentItemID = msg.ItemID;
    Notify4(LOG_PAWS,"Got Sketch for item %u: %s\nLimits:%s\n", msg.ItemID, msg.Sketch.GetData(), msg.limits.GetDataSafe());
    objlist.Empty();

    ParseSketch(msg.Sketch);
    dirty = false;
    ParseLimits(msg.limits);

    // if char does not have the right to edit (ie not the originator-artist)...
    if (!msg.rightToEdit)
        readOnly = true;

    if (!blackBox)
        blackBox = PawsManager::GetSingleton().GetTextureManager()->GetDrawable("blackbox");
    Show();
}

void pawsSketchWindow::Hide()
{
    if (dirty)
    {
        csString xml;

        xml = "<pages><page ";
		xml.AppendFmt("l=\"%d\" t=\"%d\" w=\"%d\" h=\"%d\">",
					  GetLogicalWidth(screenFrame.xmin),
					  GetLogicalHeight(screenFrame.ymin),
					  GetLogicalWidth(screenFrame.xmax-screenFrame.xmin),
					  GetLogicalHeight(screenFrame.ymax-screenFrame.ymin));

        // Add background and size stuff here

        for (size_t i=0; i<objlist.GetSize(); i++)
            objlist[i]->WriteXml(xml);

        xml += "</page></pages>";

        // printf("Saving sketch as: %s\n",xml.GetDataSafe());

        psSketchMessage sketch(0, currentItemID,0,"", xml, true);
        sketch.SendMessage();
    }
    pawsWidget::Hide();
}


void pawsSketchWindow::Draw()
{
    pawsWidget::Draw();

    // The close button X overrides the clip region so we have to set it back here
    ClipToParent();
    
    DrawSketch();
}
void pawsSketchWindow::DrawSketch()
{
    for (size_t i=0; i<objlist.GetSize(); i++)
        objlist[i]->Draw();
}

void pawsSketchWindow::DrawBlackBox(int x, int y)
{
    // Black box only appears in edit mode, but SketchObjects don't need to know that
    if (editMode)
        blackBox->Draw(x,y);
}

double pawsSketchWindow::CalcFunction(const char * functionName, const double * params)
{
	if (!strcasecmp(functionName,"ClickTextTool"))
	{
		OnKeyDown(0,'t',0);
		return 0.0;
	}
	else if (!strcasecmp(functionName,"ClickEditMode"))
	{
		OnKeyDown(CSKEY_F2,0,0);
		return 0.0;
	}
	else if (!strcasecmp(functionName,"ClickLineTool"))
	{
		OnKeyDown(0,'\\',0);
		return 0.0;
	}
	else if (!strcasecmp(functionName,"ClickIconTool"))
	{
		OnKeyDown(0,'+',0);
		return 0.0;
	}
	else if (!strcasecmp(functionName,"ClickPrevIcon"))
	{
		OnKeyDown(CSKEY_PGUP,0,0);
		return 0.0;
	}
	else if (!strcasecmp(functionName,"ClickNextIcon"))
	{
		OnKeyDown(CSKEY_PGDN,0,0);
		return 0.0;
	}
	else if (!strcasecmp(functionName,"ClickDeleteTool"))
	{
		OnKeyDown(CSKEY_DEL,0,0);
		return 0.0;
	}

	// else call parent version to inherit other functions
	return pawsWidget::CalcFunction(functionName,params);
}

bool pawsSketchWindow::OnKeyDown( int keyCode, int key, int modifiers )
{
    // printf("Keycode: %d, Key: %d\n", keyCode, key);

    if (keyCode == CSKEY_F2)
    {
        if (!readOnly)
        {
            if (selectedIndex != SIZET_NOT_FOUND && editMode)
            {
                objlist[selectedIndex]->Select(false); // turn off flashing
                selectedIndex = SIZET_NOT_FOUND;
            }
            editMode = editMode ? false : true;
			isResizable = editMode; // only resizable in edit mode
			dirty=true; // always set this for now to make sure resizes get saved.
        }
        else
        {
            psSystemMessage sysMsg( 0, MSG_ERROR, "You will only hurt the illustration if you try to edit it." );
            psengine->GetMsgHandler()->Publish(sysMsg.msg);
            editMode = false; // just to be safe
        }
        return true;
    }
    else if (editMode)
    {
        if (key == '+') // keycode is the kbd scan code, key is the ASCII
        {
            AddSketchIcon();
            return true;
        }
        else if (key == '\\')
        {
            AddSketchLine();
            return true;
        }
        else if (key == 't')
        {
            AddSketchText();
            return true;
        }
        switch (keyCode)
        {
            case CSKEY_DEL:     RemoveSelected();    
                                return true;
            case CSKEY_PGDN:    NextPrevIcon(1); // +1 is next
                                return true;
            case CSKEY_PGUP:    NextPrevIcon(-1); // +1 is next
                                return true;
            case CSKEY_DOWN:    MoveObject(0,1);
                                return true;
            case CSKEY_UP:      MoveObject(0,-1);
                                return true;
            case CSKEY_LEFT:    MoveObject(-1,0);
                                return true;
            case CSKEY_RIGHT:   MoveObject(1,0);
                                return true;
            default:
                break;
        }
    }

    return pawsWidget::OnKeyDown(keyCode,key,modifiers);
}

void pawsSketchWindow::AddSketchText()
{
    if ((int)objlist.GetSize() >= primCount)
    {
        psSystemMessage sysMsg( 0, MSG_ERROR, "Your sketch is too complex for you to add more." );
        psengine->GetMsgHandler()->Publish(sysMsg.msg);
        return;
    }

    if (!stringPending)
    {
        stringPending = true;

        // This window calls OnStringEntered when Ok is pressed.
        pawsStringPromptWindow::Create("New Text", csString(""),
            false, 220, 20, this, "AddText");            
    }
}

void pawsSketchWindow::OnStringEntered(const char *name,int param,const char *value)
{
    if (!value || !strlen(value))
        return;

    int x = (ScreenFrame().xmax + ScreenFrame().xmin)/2 - ScreenFrame().xmin;
    int y = (ScreenFrame().ymax + ScreenFrame().ymin)/2 - ScreenFrame().ymin;

    SketchText *text = new SketchText(x,y,value,this);
    objlist.Push(text);

    stringPending = false;
}

void pawsSketchWindow::AddSketchLine()
{
    if ((int)objlist.GetSize() >= primCount)
    {
        psSystemMessage sysMsg( 0, MSG_ERROR, "Your sketch is too complex for you to add more." );
        psengine->GetMsgHandler()->Publish(sysMsg.msg);
        return;
    }

    int x = (ScreenFrame().xmax + ScreenFrame().xmin)/2 - ScreenFrame().xmin;
    int y = (ScreenFrame().ymax + ScreenFrame().ymin)/2 - ScreenFrame().ymin;
    int x2 = x + 50;
    int y2 = y + 50;

    SketchLine *line = new SketchLine(x,y,x2,y2,this);
    objlist.Push(line);
}

void pawsSketchWindow::AddSketchIcon()
{
    if (iconList.GetSize() == 0)
        return;

    if ((int)objlist.GetSize() >= primCount)
    {
         psSystemMessage sysMsg( 0, MSG_ERROR, "Your sketch is too complex for you to add more." );
         psengine->GetMsgHandler()->Publish(sysMsg.msg);
         return;
    }

    // printf("Adding sketch icon now.\n");
    
    int x = (ScreenFrame().xmax + ScreenFrame().xmin)/2;
    int y = (ScreenFrame().ymax + ScreenFrame().ymin)/2;

    SketchIcon *icon = new SketchIcon(x,y,iconList[0],this);
    objlist.Push(icon);
}

void pawsSketchWindow::NextPrevIcon(int delta)
{
    if (selectedIndex == SIZET_NOT_FOUND)
        return;

    csString iconName = objlist[selectedIndex]->GetStr();

    // Find which icon is currently selected
    for (size_t whichIcon=0; whichIcon < iconList.GetSize(); whichIcon++)
    {
        if (iconName == iconList[whichIcon]) // found
        {
            whichIcon = (whichIcon + delta) % iconList.GetSize();
            objlist[selectedIndex]->SetStr(iconList[whichIcon]);
            break;
        }
    }
}

void pawsSketchWindow::MoveObject(int dx, int dy)
{
    if (selectedIndex == SIZET_NOT_FOUND)
        return;

    objlist[selectedIndex]->UpdatePosition(objlist[selectedIndex]->x + dx,
                                           objlist[selectedIndex]->y + dy);
}

void pawsSketchWindow::RemoveSelected()
{
    if (selectedIndex != SIZET_NOT_FOUND)
    {
        objlist.DeleteIndex(selectedIndex);
        selectedIndex = SIZET_NOT_FOUND;
    }
}

bool pawsSketchWindow::OnMouseDown( int button, int modifiers, int x, int y )
{
    if (!editMode)
		return pawsWidget::OnMouseDown(button, modifiers,x,y);

    mouseDown = true;

    for (size_t i=0; i<objlist.GetSize(); i++)
    {
        if (objlist[i]->IsHit(x,y))
        {
            // Unselect old object
            if (selectedIndex != SIZET_NOT_FOUND)
            {
                // printf("Deselect object %d.\n", selectedIndex);
                objlist[selectedIndex]->Select(false);
            }

            // Select new object
            selectedIndex = i;
            objlist[i]->Select(true);
            // printf("Select object %d.\n", selectedIndex);

            PawsManager::GetSingleton().GetMouse()->ChangeImage( objlist[i]->GetStr() );
            return true;
        }
    }

    if (selectedIndex != SIZET_NOT_FOUND)
    {
        // User clicked on nothing, so deselect whatever was selected
        objlist[selectedIndex]->Select(false);
        selectedIndex = SIZET_NOT_FOUND;
    }
	return pawsWidget::OnMouseDown(button, modifiers,x,y);
}

bool pawsSketchWindow::OnMouseUp( int button, int modifiers, int x, int y )
{
    if (!editMode)
		return pawsWidget::OnMouseUp(button, modifiers,x,y);

    mouseDown = false;

    if (selectedIndex != SIZET_NOT_FOUND)
    {
        objlist[selectedIndex]->UpdatePosition(GetLogicalWidth(x-ScreenFrame().xmin),GetLogicalHeight(y-ScreenFrame().ymin));
        PawsManager::GetSingleton().GetMouse()->ChangeImage("Skins Normal Mouse Pointer");
        dirty = true;
        return true;
    }
	return pawsWidget::OnMouseUp(button, modifiers,x,y);
}

bool pawsSketchWindow::ParseLimits(const char *xmlstr)
{
    csRef<iDocumentSystem>  xml;
    xml =  csQueryRegistry<iDocumentSystem> ( psengine->GetObjectRegistry() );
    if (!xml) 
        xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( xmlstr );
    if ( error )
    {
        Error2( "Error parsing Sketch string: %s", error );
        return false;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Sketch xml");
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode("limits");
    if(!topNode)
    {
        Error1("No <pages> tag in Sketch xml");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    iconList.Empty();
    primCount = 0;
    readOnly  = false;

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        csString type = node->GetValue();
        if (type=="ic")
        {
            iconList.Push(node->GetContentsValue());
            Notify2(LOG_ANY,"Got icon %s\n", node->GetContentsValue());
        }
        else if (type=="count")
        {
            primCount = node->GetContentsValueAsInt();
            Notify2(LOG_ANY,"Got max primitives count of %d.\n", primCount);
        }
        else if (type=="rdonly")
        {
            readOnly = true;
            Notify1(LOG_ANY,"Map is read-only for this player.\n");
        }
    }
    return true;
}

bool pawsSketchWindow::ParseSketch(const char *xmlstr)
{
    csRef<iDocumentSystem>  xml;
    xml =  csQueryRegistry<iDocumentSystem> ( psengine->GetObjectRegistry() );
    if (!xml) 
        xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( xmlstr );
    if ( error )
    {
        Error2( "Error parsing Sketch string: %s", error );
        return false;
    }
     csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Sketch xml");
        return false;
    }
   csRef<iDocumentNode> topNode = root->GetNode("pages");
    if(!topNode)
    {
        Error1("No <pages> tag in Sketch xml");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes("page");
    
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
		MoveTo(GetActualWidth(node->GetAttributeValueAsInt("l")),GetActualHeight(node->GetAttributeValueAsInt("t")));
		screenFrame.SetSize(GetActualWidth(node->GetAttributeValueAsInt("w")),GetActualHeight(node->GetAttributeValueAsInt("h")));
        
        defaultFrame.xmin = GetActualWidth(node->GetAttributeValueAsInt("l"));
        defaultFrame.ymin = GetActualHeight(node->GetAttributeValueAsInt("y"));
        int width  = GetActualWidth(node->GetAttributeValueAsInt("w"));
        int height = GetActualHeight(node->GetAttributeValueAsInt("h"));
        defaultFrame.SetSize(width, height);
                
        csRef<iDocumentNodeIterator> pagenodes = node->GetNodes();
        while (pagenodes->HasNext() )
        {
            SketchObject *obj=NULL;
            csRef<iDocumentNode> tmp = pagenodes->Next();
            csString type = tmp->GetValue();
            // Determine what type to create, and create it
            if (type == "ic") // icon
                obj = new SketchIcon;
            else if (type == "tx") // text
                obj = new SketchText;
            else if (type == "back") // background specifier
                ; // do nothing
            else if (type == "ln")
                obj = new SketchLine;
            else
            {
                Error2("Illegal sketch op <%s>.  Sketch cannot display.",type.GetDataSafe() );
                return false;
            }

            if (obj)
            {
                // Now load it
                if (!obj->Load(tmp,this))
                {
                    delete obj;
                    return false;
                }
                objlist.Push(obj);
            }
        }
        /// TODO: Support multiple pages here.
    }
    
    
    OnResize();
        
    return true;
}

pawsSketchWindow::SketchIcon::SketchIcon(int _x, int _y, const char *icon, pawsSketchWindow *parent)
{
    Init(_x,_y,icon,parent);
}

bool pawsSketchWindow::SketchIcon::Init(int _x, int _y, const char *icon, pawsSketchWindow *parent)
{
    x = _x;
    y = _y;
    str = icon;

    iconImage = PawsManager::GetSingleton().GetTextureManager()->GetDrawable(str);

    // printf("Icon %s at (%d,%d)\n",str.GetDataSafe(),x,y);

    if (parent)
        this->parent = parent;

    return true;
}

void pawsSketchWindow::SketchIcon::SetStr(const char *s)
{
    Init(x,y,s,NULL);
    // printf("Changed icon to %s.\n",s);
}

bool pawsSketchWindow::SketchIcon::Load(iDocumentNode *node, pawsSketchWindow *parent)
{
    if (!node->GetAttributeValue("i"))
        return false;

    return Init(node->GetAttributeValueAsInt("x"),
                node->GetAttributeValueAsInt("y"),
                node->GetAttributeValue("i"), // TODO: Convert to common_strings
                parent);
}

void pawsSketchWindow::SketchIcon::WriteXml(csString& xml)
{
    csString add;

    add.Format("<ic x=\"%d\" y=\"%d\" i=\"%s\"/>",x,y,str.GetDataSafe());
    xml += add;
}

void pawsSketchWindow::SketchIcon::Draw()
{
    if (!selected || frame > 15)
    {
        if (!parent->IsMouseDown() || !selected)
            iconImage->Draw(parent->GetActualWidth(x) + parent->ScreenFrame().xmin,
                            parent->GetActualHeight(y) + parent->ScreenFrame().ymin,
							parent->GetActualWidth(iconImage->GetWidth()),
							parent->GetActualHeight(iconImage->GetHeight()));
    }
    if (selected)
        frame = (frame > 29) ? 0 : frame+1;
}

bool pawsSketchWindow::SketchIcon::IsHit(int mouseX, int mouseY)
{
    csRect rect = parent->ScreenFrame();
    rect.xmin += parent->GetActualWidth(x);
    rect.ymin += parent->GetActualHeight(y);
    rect.xmax = rect.xmin + iconImage->GetWidth();
    rect.ymax = rect.ymin + iconImage->GetHeight();

    return rect.Contains(mouseX,mouseY);
}

bool pawsSketchWindow::SketchText::Load(iDocumentNode *node, pawsSketchWindow *parent)
{
    x = node->GetAttributeValueAsInt("x");
    y = node->GetAttributeValueAsInt("y");
    if (!node->GetAttributeValue("t"))
        return false;
    str = node->GetAttributeValue("t"); // TODO: Convert to common_strings
    // printf("Text %s at (%d,%d)\n",str.GetDataSafe(),x,y);

    this->parent = parent;
    return true;
}


void pawsSketchWindow::SketchText::WriteXml(csString& xml)
{
    csString txt = EscpXML(str);

    xml.AppendFmt("<tx x=\"%d\" y=\"%d\" t=\"%s\"/>", x, y, txt.GetData());
}


void pawsSketchWindow::SketchText::Draw()
{
    if (!selected || frame > 15)
    {
        parent->DrawWidgetText(str,
                               parent->GetActualWidth(x)+parent->ScreenFrame().xmin,
                               parent->GetActualHeight(y)+parent->ScreenFrame().ymin);
    }
    if (selected)
        frame = (frame > 29) ? 0 : frame+1;
}

bool pawsSketchWindow::SketchText::IsHit(int mouseX, int mouseY)
{
    csRect rect = parent->ScreenFrame();
    rect.xmin += parent->GetActualWidth(x);
    rect.ymin += parent->GetActualHeight(y);

    rect = parent->GetWidgetTextRect(str,rect.xmin, rect.ymin);
    return rect.Contains(mouseX,mouseY);
}


bool pawsSketchWindow::SketchLine::Load(iDocumentNode *node, pawsSketchWindow *parent)
{
    psString pts = node->GetAttributeValue("pts");
    psString num;

    pts.GetWordNumber(1,num);
    x   = atoi(num.GetDataSafe());
    pts.GetWordNumber(2,num);
    y   = atoi(num.GetDataSafe());
    pts.GetWordNumber(3,num);
    x2  = atoi(num.GetDataSafe());
    pts.GetWordNumber(4,num);
    y2  = atoi(num.GetDataSafe());

    printf("Line %d,%d to %d, %d\n",x,y,x2,y2);

    this->parent = parent;
    return true;
}

void pawsSketchWindow::SketchLine::WriteXml(csString& xml)
{
    csString add;

    add.Format("<ln pts=\"%d %d %d %d\"/>",x,y,x2,y2);
    xml += add;
}

void pawsSketchWindow::SketchLine::Draw()
{
    iGraphics2D *graphics2D = parent->GetG2D();

    int black = graphics2D->FindRGB( 0,0,0 );

    graphics2D->DrawLine( parent->GetActualWidth(x)+parent->ScreenFrame().xmin,
                          parent->GetActualHeight(y)+parent->ScreenFrame().ymin,
                          parent->GetActualWidth(x2)+parent->ScreenFrame().xmin,
                          parent->GetActualHeight(y2)+parent->ScreenFrame().ymin,
                          black );

    if (!selected || frame > 15)
    {
        parent->DrawBlackBox(parent->GetActualWidth(x)+parent->ScreenFrame().xmin-3,
                             parent->GetActualHeight(y)+parent->ScreenFrame().ymin-3);
        parent->DrawBlackBox(parent->GetActualWidth(x2)+parent->ScreenFrame().xmin-3,
                             parent->GetActualHeight(y2)+parent->ScreenFrame().ymin-3);
    }
    if (selected)
        frame = (frame > 29) ? 0 : frame+1;
}

bool pawsSketchWindow::SketchLine::IsHit(int mouseX, int mouseY)
{
    csRect rect = parent->ScreenFrame();
    mouseX -= rect.xmin;
    mouseY -= rect.ymin;
	
	mouseX = parent->GetLogicalWidth(mouseX);
	mouseY = parent->GetLogicalHeight(mouseY);

    dragMode = 0;

    if (abs(mouseX-x) < 3 &&
        abs(mouseY-y) < 3)
    {
        dragMode = 1;
        offsetX = mouseX - x;
        offsetY = mouseY - y;
        return true;
    }
    if (abs(mouseX-x2) < 3 &&
        abs(mouseY-y2) < 3)
    {
        dragMode = 2;
        offsetX = mouseX - x;
        offsetY = mouseY - y;
        return true;
    }

    rect.xmax = x2;
    rect.ymax = y2;
    rect.xmin = x;
    rect.ymin = y;

    rect.Normalize();

    if (!rect.Contains(mouseX,mouseY))
        return false;

    offsetX = mouseX - x;
    offsetY = mouseY - y;
    return true;
}

void pawsSketchWindow::SketchLine::UpdatePosition(int _x, int _y)
{
    int dx, dy;

    _x -= offsetX;  // This backs off the cursor from where it was
    _y -= offsetY;

    dx = _x - x;
    dy = _y - y;

    switch (dragMode)
    {
        case 0:
            x += dx;
            y += dy;
            x2 += dx;
            y2 += dy;
            break;
        case 1:
            x += dx;
            y += dy;
            break;
        case 2:
            x2 += dx;
            y2 += dy;
            break;
    }
    offsetX = 0;
    offsetY = 0;
}


