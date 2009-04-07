
/*
 * pawscombobox.cpp - Author: Andrew Craig
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
#include <string.h>
#include "pawscombo.h"
#include "pawsbutton.h"
#include "pawstextbox.h"
#include "pawslistbox.h"
#include "pawsmanager.h"

#define SHOW_LIST 100

pawsComboBox::pawsComboBox()
{
    oldHeight = 0;
    oldWidth = 0;
    closed = true;  
    fliptotop = false;
    
    rows = 0;
    rowHeight = 0;
}

pawsComboBox::~pawsComboBox()
{

}

bool pawsComboBox::Setup( iDocumentNode* node ) 
{ 
    csRef<iDocumentNode> listNode = node->GetNode( "listbox" );
    if(!listNode.IsValid())
    {
        return false;
    }
    
    rows      = listNode->GetAttributeValueAsInt("rows");
    rowHeight = listNode->GetAttributeValueAsInt("height");
    text      = listNode->GetAttributeValue("text");
    listalpha = listNode->GetAttributeValueAsInt("alpha");
    fliptotop = listNode->GetAttributeValueAsBool("fliptotop");
    useScrollBar = listNode->GetAttributeValueAsBool("useScrollBar", true);
    
    return true;
}

bool pawsComboBox::PostSetup() 
{
    bool ok = false;

    ///////////////////////////////////////////////////////////////////////
    // Create the drop arrow button that will cause the list box to drop.
    ///////////////////////////////////////////////////////////////////////
    arrow = new pawsButton;
    AddChild( arrow );

    // Puts the button at the edge of the text box widget
    arrow->SetRelativeFrame( defaultFrame.Width()-16, 4, 16, 16 );
    arrow->SetUpImage( "Down Arrow" );
    arrow->SetDownImage( "Down Arrow" );
    arrow->SetID( SHOW_LIST );
    
    ok = arrow->PostSetup();    
    
    
    ///////////////////////////////////////////////////////////////////////
    // Create the textbox that has the current selected choice
    ///////////////////////////////////////////////////////////////////////    
    itemChoice = new pawsTextBox;
    AddChild( itemChoice );

    // Puts the button at the edge of the text box widget
    itemChoice->SetRelativeFrame( 0 , 4, defaultFrame.Width()-16, defaultFrame.Height() );
    ok = ok && itemChoice->PostSetup();

    itemChoice->SetText(text);
    itemChoice->SetID( SHOW_LIST );
    
    
    ///////////////////////////////////////////////////////////////////////
    // Create the drop down list box
    ///////////////////////////////////////////////////////////////////////   
    listChoice = new pawsListBox;
    AddChild( listChoice );

    if (fliptotop)
    {
        listChoice->SetRelativeFrame( 0 , 0, defaultFrame.Width(), rows*GetActualHeight(rowHeight)+15);
    }
    else
    {
        listChoice->SetRelativeFrame( 0 , defaultFrame.Height(), defaultFrame.Width(), rows*GetActualHeight(rowHeight)+15);
    }

    listChoice->Hide();
    listChoice->UseTitleRow( false ); 
    listChoice->SetID( id );
    listChoice->SetBackground("Standard Background");
    listChoice->SetBackgroundAlpha(listalpha);
    listChoice->UseBorder("line");
    listChoice->SetAlwaysOnTop(true);
    listChoice->SetName("ComboListBox");
    
    csString widgetDef("<widget name=\"Text\" factory=\"pawsTextBox\" ></widget>");
    listChoice->SetTotalColumns( 1 );

    if(useScrollBar)
    {
        ok = ok && listChoice->PostSetup();
        listChoice->SetColumnDef( 0, defaultFrame.Width()-32, rowHeight, widgetDef );
    }
    else
    {
        listChoice->SetColumnDef( 0, defaultFrame.Width()-10, rowHeight, widgetDef );
    }

    return ok;
}

pawsListBoxRow* pawsComboBox::NewOption(const csString & text)
{
    pawsListBoxRow * row;
        
    row = NewOption();
    if (row)
    {
        pawsTextBox * cell=(pawsTextBox*)row->GetColumn(0);
        if (cell != NULL)
            cell->SetText(text);
    }

    return row;        
}

bool pawsComboBox::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    switch ( widget->GetID() )
    {
        case SHOW_LIST:
        {
            if ( closed )
            {
                arrow->SetUpImage( "Up Arrow" );
                arrow->SetDownImage( "Up Arrow" );
                oldHeight = ScreenFrame().Height();    
                oldWidth  = ScreenFrame().Width();    
                SetSize( ScreenFrame().Width(), defaultFrame.Height()+rows*GetActualHeight(rowHeight)+15 );
                if (fliptotop)
                {
                    MoveDelta(0,-1*(rows*GetActualHeight(rowHeight)+15));
                    itemChoice->MoveDelta(0,rows*GetActualHeight(rowHeight)+15);
                    arrow->MoveDelta(0,rows*GetActualHeight(rowHeight)+15);
                }
                                
                listChoice->Show();
                closed = false;
            }
            else
            {
                arrow->SetUpImage( "Down Arrow" );
                arrow->SetDownImage( "Down Arrow" );
                SetSize( oldWidth, oldHeight );
                if (fliptotop)
                {
                    MoveDelta(0,rows*GetActualHeight(rowHeight)+15);
                    itemChoice->MoveDelta(0,-1*(rows*GetActualHeight(rowHeight)+15));
                    arrow->MoveDelta(0,-1*(rows*GetActualHeight(rowHeight)+15));
                }
                listChoice->Hide();
                closed = true;
            }
            return true;
        }
    }    
    
    return false;
}

csString pawsComboBox::GetSelectedRowString()
{
    pawsListBoxRow* row = listChoice->GetSelectedRow();
    if (row == NULL) return "";
    
    pawsTextBox* thing = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
    if (thing == NULL) return "";
    
    return thing->GetText();
}

void pawsComboBox::OnListAction( pawsListBox* widget, int status )
{
    itemChoice->SetText(GetSelectedRowString());
    
    if (!closed)
    {
        arrow->SetUpImage( "Down Arrow" );
        arrow->SetDownImage( "Down Arrow" );
        SetSize( oldWidth, oldHeight );
        if (fliptotop)
        {
            MoveDelta(0,rows*GetActualHeight(rowHeight)+15);
            itemChoice->MoveDelta(0,-1*(rows*GetActualHeight(rowHeight)+15));
            arrow->MoveDelta(0,-1*(rows*GetActualHeight(rowHeight)+15));
        }
        listChoice->Hide();
        closed = true;
    }
    
    parent->OnListAction( widget, status );
}

int pawsComboBox::GetSelectedRowNum()
{
    return listChoice->GetSelectedRowNum();
}

pawsListBoxRow* pawsComboBox::Select(int optionNum)
{
    pawsListBoxRow * row = listChoice->GetRow(optionNum);
    
    listChoice->Select(row);   

    if ( row == NULL )
        itemChoice->SetText( initalText );
    else
        itemChoice->SetText( GetSelectedRowString() );
    
    if (!closed)
    {
        arrow->SetUpImage( "Down Arrow" );
        arrow->SetDownImage( "Down Arrow" );
        SetSize( oldWidth, oldHeight );
        if (fliptotop)
        {
            MoveDelta(0,rows*GetActualHeight(rowHeight)+15);
            itemChoice->MoveDelta(0,-1*(rows*GetActualHeight(rowHeight)+15));
            arrow->MoveDelta(0,-1*(rows*GetActualHeight(rowHeight)+15));
        }
        listChoice->Hide();
        closed = true;
    }
    return row;
}
pawsListBoxRow* pawsComboBox::Select(const char * text)
{
    pawsListBoxRow *row;
    for (unsigned int i = 0; i < listChoice->GetRowCount(); i++ )
    {
        row = listChoice->GetRow(i);
        pawsTextBox* thing = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
        if ( thing )
        {
            if ( !csStrCaseCmp(thing->GetText(), text) )
            {
                return Select( i );
            }
        }
    }
    Select( -1 );
    return NULL;
}


pawsListBoxRow* pawsComboBox::NewOption()
{
    return listChoice->NewRow((size_t)-1);
}

int pawsComboBox::GetRowCount()
{
    return listChoice->GetRowCount();
}

bool pawsComboBox::Clear()
{
    if (!listChoice)
        return false;
    
    listChoice->Clear();
    itemChoice->SetText("");

    return true;
}
