/*
 * pawscheckbox.h
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
#ifndef PAWS_CHECKBOX_HEADER
#define PAWS_CHECKBOX_HEADER


#include "pawsbutton.h"

class pawsTextBox;

/** A combination widget that has a check box and a text label.
    
    This widget is defined in an XML as:
         
    <widget name="nameHere" factory="pawsCheckBox" id="intIDNumber">
        <!-- The size of the entire widget including text and button. -->
        <frame x="75" y="5" width="70" height="30" />        
        
        <!-- The text label to use and it's position relative to the button -->
        <text string="Sell" position="right"/>
    </widget>
    
    Current supported positions are left/right.
    
    By default it uses the checkon/checkoff named images and automatically 
    assumes that their size is 16x16.  
*/
class pawsCheckBox: public pawsWidget
{
public:
    pawsCheckBox();
    virtual ~pawsCheckBox();
    
    virtual bool Setup( iDocumentNode* node );
    bool SelfPopulate( iDocumentNode *node);
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

    virtual void SetState( bool state );
    virtual bool GetState();

    virtual void OnUpdateData(const char *dataname,PAWSData& data);

    virtual void SetImages(const char* up, const char* down);
        
	virtual double GetProperty(const char * ptr);
	virtual double CalcFunction(const char * functionName, const double * params);
	virtual void SetProperty(const char * ptr, double value);

private:
    pawsButton* checkBox;
    pawsTextBox* text;
	int textOffsetX;
	int textOffsetY;

    csString checkBoxOff;
    csString checkBoxOn;
	csString checkBoxGrey;

    int checkBoxSize;  
    
};
CREATE_PAWS_FACTORY( pawsCheckBox );

#endif 
