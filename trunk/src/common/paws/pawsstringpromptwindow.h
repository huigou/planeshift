/*
 * pawsstringpromptwindow.h - Author: Ondrej Hurt
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

#ifndef PAWS_STRING_PROMPT_WINDOW_HEADER
#define PAWS_STRING_PROMPT_WINDOW_HEADER

#include <csutil/list.h>
#include <iutil/document.h>
#include "pawspromptwindow.h"

class pawsButton;

/**
 * This interface defines the callback used by pawsStringPromptWindow
 * to notify another window of a supplied answer.
 */
class iOnStringEnteredAction
{
public:
    /**
     * When the pawsStringPromptWindow is created, a ptr to a class
     * which implements this function is provided, and a "name" string
     * is provided, so that a single window can use 1 callback for
     * many fields.
     */
    virtual void OnStringEntered(const char *name, int param,const char *value) = 0;
    virtual ~iOnStringEnteredAction() {};
};

/** 
 * pawsStringPromptWindow is window that lets the user enter string.
 */
class pawsStringPromptWindow : public pawsPromptWindow
{
public:
    pawsStringPromptWindow();
    
    //from pawsWidget:
    bool OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget );
    virtual bool OnKeyDown( utf32_char keyCode, utf32_char keyChar, int modifiers );
    
    static pawsStringPromptWindow * Create( 
        const csString & label, 
        const csString & string, bool multiline, int width, int height, 
        iOnStringEnteredAction * action,const char *name,int param = 0, 
        bool modal = false );

protected:
    void Initialize(const csString & label, const csString & string, bool multiline, int width, int height, iOnStringEnteredAction *action, const char *name, int param=0);
    
    /** Executes action with 'text' as parameter and destroys window */
    void CloseWindow(const csString & text);
    
    /** Executes action with text entered by user as parameter and destroys window */
    void CloseWindow();
    
    bool multiLine;
    iOnStringEnteredAction * action;
    csString name;
    int param;
};

CREATE_PAWS_FACTORY(pawsStringPromptWindow)


#endif
