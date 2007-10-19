/*
 * pawsradio.h - Author: Andrew Craig
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
#ifndef PAWS_RADIOBUTTON_HEADER
#define PAWS_RADIOBUTTON_HEADER


#include "pawsbutton.h"

class pawsTextBox;

/** A combination widget that has a radio button and a text label.
    This should ALWAYS be a child of the pawsRadioButtonGroup in order 
    to control his brothers properly ( ie have only 1 in group active at 
    at time. )
    
    This widget is defined in an XML as:
         
    <widget name="nameHere" factory="pawsRadioButton" id="intIDNumber">
        <!-- The size of the entire widget including text and button. -->
        <frame x="75" y="5" width="70" height="30" />        
        
        <!-- The text label to use and it's position relative to the button -->
        <text string="Sell" position="right"/>
    </widget>
    
    Current supported positions are left/right.
    
    By default it uses the radioon/radiooff named images and automatically 
    assumes that their size is 16x16.  
*/
class pawsRadioButton: public pawsButton
{
public:
    pawsRadioButton();
    virtual ~pawsRadioButton();
    
    bool Setup( iDocumentNode* node );
    void SetState( bool state );
    bool GetState();
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
       
    pawsTextBox* GetTextBox() { return text; } 

    void SetRadioOffImage(const char &image){radioOff=image;};
    void SetRadioOnImage(const char &image){radioOn=image;};
    void SetRadioSize(int s){size=s;};

private:
    pawsButton* radioButton;
    pawsTextBox* text;

    csString radioOff;
    csString radioOn;
    int      size;
    
};
CREATE_PAWS_FACTORY( pawsRadioButton );



//---------------------------------------------------------------------------

/** This is a set of radio buttons and is used to control them.
 */
class pawsRadioButtonGroup : public pawsWidget
{
public:
    pawsRadioButtonGroup();
    virtual ~pawsRadioButtonGroup();

    bool Setup( iDocumentNode* node );

    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget );
    bool SetActive( const char* widgetName );
    csString GetActive();
    void TurnAllOff();

    csString GetRadioOnImage(){return radioOn;};
    csString GetRadioOffImage(){return radioOff;};
    int      GetRadioSize(){return size;};

private:

    csString radioOff;
    csString radioOn;
    int      size;
};
CREATE_PAWS_FACTORY( pawsRadioButtonGroup );





#endif 
