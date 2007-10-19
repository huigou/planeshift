/*
 * pawsradio.cpp - Author: Andrew Craig
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

#include "pawsradio.h"
#include "pawsmanager.h"
#include "pawstexturemanager.h"
#include "pawstextbox.h"

//---------------------------------------------------------------------------------
pawsRadioButton::pawsRadioButton()
{
    down=false;
    radioOff ="radiooff";
    radioOn  ="radioon ";
    size = 16;
}

void pawsRadioButton::SetState( bool state )
{    
    radioButton->SetState( state );
}

pawsRadioButton::~pawsRadioButton()
{
}

bool pawsRadioButton::Setup( iDocumentNode* node )
{
    csRef<iDocumentNode> textNode = node->GetNode( "text" );        
    
    if ( !textNode )
    {
        Error2("%s XML is defined incorrectly. No <text /> tag found", name.GetData());
        return false;
    }
    
    csString pos(textNode->GetAttributeValue("position"));

    pawsRadioButtonGroup* rbg = dynamic_cast<pawsRadioButtonGroup*>(parent);

    // if this radio button is part of a radio group
    if (rbg!=NULL)
    {
        // get the way the off and up radio buttons should look from there
        radioOff = rbg->GetRadioOffImage();
        radioOn  = rbg->GetRadioOnImage();
        size     = rbg->GetRadioSize();
    }

    csRef<iDocumentNode> radioNode = node->GetNode( "radio" );

    if ( radioNode )
    {
        csRef<iDocumentAttribute> attr;

        attr = radioNode->GetAttribute("off");
        if (attr)
            radioOff = attr->GetValue();

        attr = radioNode->GetAttribute("on");
        if (attr)
            radioOn  = attr->GetValue();

        attr = radioNode->GetAttribute("size");
        if (attr)
            size     = attr->GetValueAsInt();
    }


    
    ///////////////////////////////////////////////////////////////////////
    // Create the radio button
    ///////////////////////////////////////////////////////////////////////
    radioButton = (pawsButton*)PawsManager::GetSingleton().CreateWidget("pawsButton");
    AddChild( radioButton );

    csRect buttonRect;
    csRect textRect;
    
    if ( pos == "left" )    
    {
        buttonRect = csRect(  defaultFrame.Width()-size, 4, 
                              defaultFrame.Width(), size+4 );
                              
        textRect   = csRect(  0, 4, defaultFrame.Width() - size, defaultFrame.Height() );
    }
    
    if ( pos == "right" )    
    {
        buttonRect = csRect(  4, 4, 
                              size+4, size+4 );
                              
        textRect   = csRect(  size+6, 4, defaultFrame.Width()-size, defaultFrame.Height() );
    }
    
    if ( pos == "underneath" )
    {
        buttonRect = csRect( defaultFrame.Width() / 2 - size /2, 4, defaultFrame.Width()/2 + size/2, 4+size );
        textRect = csRect( 0,size+6, defaultFrame.Width() , defaultFrame.Height()-(size+6) );
    }
                
    if ( pos == "above" )
    {
        buttonRect = csRect( defaultFrame.Width() / 2 - size /2, 22, defaultFrame.Width()/2 + size/2, size+22 );
        textRect = csRect( 0,4, defaultFrame.Width() , size+4 );
    }

    radioButton->SetRelativeFrame( buttonRect.xmin, buttonRect.ymin, 
                                   buttonRect.Width(), buttonRect.Height() );
    radioButton->SetUpImage( radioOff );
    radioButton->SetDownImage( radioOn );
    radioButton->SetState( false );
    radioButton->SetToggle(true);
    radioButton->PostSetup();
    radioButton->SetID( id );
     
    
    ///////////////////////////////////////////////////////////////////////
    // Create the textbox that has the current selected choice
    ///////////////////////////////////////////////////////////////////////    
    csString str(textNode->GetAttributeValue("string"));
    
    text = (pawsTextBox*)PawsManager::GetSingleton().CreateWidget("pawsTextBox");
    AddChild( text );

    // Puts the button at the edge of the text box widget
    text->SetRelativeFrame( textRect.xmin, textRect.ymin,
                            textRect.Width(), textRect.Height() );
    text->PostSetup();
    
    text->SetText(str);
    text->SetID( id );
    
    return true;               
}


bool pawsRadioButton::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if ( parent ) 
        return parent->OnButtonPressed( mouseButton, keyModifier, this );

    return false;
}

bool pawsRadioButton::GetState()
{
    if (radioButton != NULL)
        return radioButton->GetState();
    else
        return false;
}


//--------------------------------------------------------------
//  START OF pawsRadioButtonGroup
//--------------------------------------------------------------

pawsRadioButtonGroup::pawsRadioButtonGroup()
{
    radioOff = "radiooff";
    radioOn  = "radioon";
    size     = 16;
}


pawsRadioButtonGroup::~pawsRadioButtonGroup()
{
}

bool pawsRadioButtonGroup::SetActive( const char* widgetName )
{
    pawsWidget* widget = FindWidget( widgetName );
    if (widget==NULL)
        return false;

    for ( size_t x=0; x<children.GetSize(); x++ )
    {
        csString factory = csString(children[x]->GetType());
        if ( factory == "pawsRadioButton" )
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];

            if ( radButton == widget )
                radButton->SetState( true );
            else
                radButton->SetState( false );
        }
    }
    return true;
}

void pawsRadioButtonGroup::TurnAllOff()
{
    for ( size_t x=0; x<children.GetSize(); x++ )
    {
        csString factory = csString(children[x]->GetType());
        if ( factory == "pawsRadioButton" )
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];
            radButton->SetState( false );
        }
    }
}

bool pawsRadioButtonGroup::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{    
    for ( size_t x=0; x<children.GetSize(); x++ )
    {
        csString factory = csString(children[x]->GetType());       
        
        if ( factory == "pawsRadioButton" )
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];
            
            if ( radButton == widget )            
                radButton->SetState( true );                            
            else
                radButton->SetState( false );
        }
    }

    if ( parent )
        parent->OnChange(this);

    if ( parent ) 
        return parent->OnButtonPressed( mouseButton, keyModifier, widget );
    
    return false;
}

csString pawsRadioButtonGroup::GetActive()
{
    for ( size_t x=0; x<children.GetSize(); x++ )
    {
        csString factory = csString(children[x]->GetType());
        if ( factory == "pawsRadioButton" )
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];
            if (radButton->GetState())
                return radButton->GetName();
        }
    }
    return "";
}


bool pawsRadioButtonGroup::Setup( iDocumentNode* node )
{
    csRef<iDocumentNode> radioNode = node->GetNode( "radio" );

    if ( radioNode )
    {
        csRef<iDocumentAttribute> attr;

        attr = radioNode->GetAttribute("off");
        if (attr)
            radioOff = attr->GetValue();

        attr = radioNode->GetAttribute("on");
        if (attr)
            radioOn  = attr->GetValue();

        attr = radioNode->GetAttribute("size");
        if (attr)
            size     = attr->GetValueAsInt();
    }
    
    return true;               
}


