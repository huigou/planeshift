/*
 * pawsslot.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "pawsslot.h"
#include "psslotmgr.h"
#include "globals.h"
#include "pscelclient.h"
#include "paws/pawstextbox.h"
#include "net/msghandler.h"

pawsSlot::pawsSlot()
{
    emptyOnZeroCount = true;
    empty = true;
    containerID = -100;
    slotID = -1;

    SetRelativeFrame( 0,0, GetActualWidth(48), GetActualHeight(48) );

    purifySign = new pawsWidget();
    AddChild(purifySign);
    purifySign->SetRelativeFrame(GetActualWidth(5), GetActualHeight(5), GetActualWidth(37), GetActualHeight(37));
    purifySign->Hide();
    SetPurifyStatus(0);

    stackCountLabel = (pawsTextBox*)PawsManager::GetSingleton().CreateWidget( "pawsTextBox" );
    AddChild( stackCountLabel );
    stackCountLabel->SetAlwaysOnTop(true);
    stackCountLabel->SetRelativeFrame( 8, 8, 40, 20 );
    stackCountLabel->SetFont(NULL,8);
    stackCountLabel->SetColour( graphics2D->FindRGB(255,255,255) );
    stackCountLabel->Hide();
    dragDrop = true;
    StackCount(0); 

    drawStackCount = true;
}


pawsSlot::~pawsSlot()
{
}

bool pawsSlot::Setup( iDocumentNode* node )
{
    csRef<iDocumentNode> ident = node->GetNode( "ident" );
    if ( ident )
    {
        containerID = ident->GetAttributeValueAsInt("container");        
        slotID = ident->GetAttributeValueAsInt("id");            
    }        
    
    mgr = psengine->GetSlotManager();
           
    return true;        
}

bool pawsSlot::OnMouseDown( int button, int modifiers, int x, int y )
{
    if ( !psengine->GetCelClient()->GetMainPlayer()->IsAlive() )
        return true;

    if ( !empty && psengine->GetMouseBinds()->CheckBind("ContextMenu",button,modifiers) )
    {
        psViewItemDescription out(containerID, slotID);
        psengine->GetMsgHandler()->SendMessage( out.msg );
        return true;
    }
    else if ( dragDrop && (!empty || psengine->GetSlotManager()->IsDragging()) )
    {
        // Grab one item if shift key are used. Grab everything in the slot 
        // if either middle mouse button or ctrl key are used
        mgr->Handle( this, modifiers==1, button==2 || modifiers==2 );
        return true;
    }
    else if ( parent )
        return parent->OnButtonPressed(button, modifiers, this);
    else
        return pawsWidget::OnMouseDown(button, modifiers, x, y );
}

void pawsSlot::SetToolTip( const char* text )
{
    pawsWidget::SetToolTip( text );
    stackCountLabel->SetToolTip( text );
}

void pawsSlot::StackCount( int newCount )
{
    if (emptyOnZeroCount  &&  newCount==0)
        Clear();
    else
    {
        stackCount = newCount;
        char buffer[10] = "";
        if (newCount > 0)
            sprintf( buffer, "%d", newCount );
        stackCountLabel->SetText( buffer );
    }
}

void pawsSlot::PlaceItem( const char* imageName, int count )
{
    empty = false;

    image = PawsManager::GetSingleton().GetTextureManager()->GetDrawable(imageName);

    if (drawStackCount)
        stackCountLabel->Show();
       
    stackCount = count;
    StackCount( count );
}


void pawsSlot::Draw()
{
    if (!drawStackCount && stackCountLabel->IsVisible())
        stackCountLabel->Hide();

    pawsWidget::Draw();
    ClipToParent();    
    csRect frame = screenFrame;

    if (!empty)
    {
        if (image)
            image->Draw( frame );
        //Need to redraw the count
        if (drawStackCount)
            stackCountLabel->Draw();
    }
              
    graphics2D->SetClipRect( 0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
}    

void pawsSlot::Clear()
{
    //printf("In pawsSlot::Clear()\n");
    empty = true;
    stackCount = 0;
    stackCountLabel->Hide();
    SetPurifyStatus(0);
    SetToolTip("");
}    

const char *pawsSlot::ImageName()
{
    if (image)
        return image->GetName();
    return NULL;
}

int pawsSlot::GetPurifyStatus()
{
    return purifyStatus;
}

void pawsSlot::SetPurifyStatus(int status)
{
    purifyStatus = status;

    switch (status)
    {
        case 0:
            purifySign->Hide();
            break;
        case 1:
            purifySign->Show();
            purifySign->SetBackground("GlyphSlotPurifying");	
            break;
        case 2:
            purifySign->Show();
            purifySign->SetBackground("GlyphSlotPurified");
			break;
    }
}

void pawsSlot::DrawStackCount(bool value)
{
    drawStackCount = value;

    if (value)
        stackCountLabel->Show();
    else
        stackCountLabel->Hide();
}

bool pawsSlot::SelfPopulate( iDocumentNode *node)
{
    if (node->GetAttributeValue("icon"))
    {
        this->DrawStackCount(false);
        this->SetDrag(false);
        PlaceItem(node->GetAttributeValue("icon"),1);
    }
    
    return true;
}

void pawsSlot::OnUpdateData(const char *dataname,PAWSData& value)
{
    csString sig(dataname);
    
    if (sig.StartsWith("sigClear"))
    {
        // Clear out the pub-sub cache for this specific slot to avoid stale
        // stuff coming back to life when a slot subscribes again.
        if (!slotName.IsEmpty())
            PawsManager::GetSingleton().Publish(slotName, "");
            
        Clear();
        SetDefaultToolTip();                        
    }
    else if (value.IsData())
    {
        psString data(value.GetStr());

        if (data.IsEmpty())
        {
            Clear();
            SetDefaultToolTip();
            return;
        }

        psString icon;
        psString count;
        psString name;
        psString status;

        data.GetWordNumber(1,icon);       
        data.GetWordNumber(2,count);
        data.GetWordNumber(3,status);        
        data.GetSubString( name, icon.Length()+count.Length()+status.Length()+3, data.Length());
        
        PlaceItem( icon, atoi(  count.GetData() ) );        
        SetToolTip( name );
        SetPurifyStatus( atoi(status.GetData())  );
    }   

}

void pawsSlot::ScalePurifyStatus()
{
	csRect rect = this->ClipRect();
	int width = rect.Width()- GetActualWidth(11);
	int height = rect.Height() - GetActualHeight(11);
	purifySign->SetRelativeFrameSize(width, height);

}

