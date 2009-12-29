/*
 * pawstextbox.h - Author: Andrew Craig
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
// pawstextbox.h: interface for the pawsTextBox class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_TEXT_BOX_HEADER
#define PAWS_TEXT_BOX_HEADER

struct iVirtualClock;

#include "pawswidget.h"
#include <csutil/parray.h>
#include <ivideo/fontserv.h>
		
/** A basic text box widget.
 */
class pawsTextBox : public pawsWidget
{
public:
    enum pawsVertAdjust
    {
        vertTOP,
        vertBOTTOM,
        vertCENTRE
    };

    enum pawsHorizAdjust
    {
        horizLEFT,
        horizRIGHT,
        horizCENTRE
    };

    pawsTextBox();
    virtual ~pawsTextBox();

    bool Setup ( iDocumentNode* node );
    bool SelfPopulate( iDocumentNode *node);

    void Draw();
    
    void SetText( const char* text );
    void FormatText( const char *fmt, ... );
    const char* GetText() { return text; }
    void SetVertical(bool vertical);
    
    void Adjust (pawsHorizAdjust horiz, pawsVertAdjust vert);
    void HorizAdjust (pawsHorizAdjust horiz);
    void VertAdjust (pawsVertAdjust horiz);

    // Sets size of the widget to fit the text that it displays:
    void SetSizeByText(int padX,int padY);
    
    // Normal text boxes should not be focused.
    virtual bool OnGainFocus( bool notifyParent = true ) {return false;}

    int GetBorderStyle() { return BORDER_SUNKEN; }    

    void Grayed(bool g) { grayed = g; }

    virtual int GetFontColour();

    virtual void OnUpdateData(const char *dataname,PAWSData& data);

protected:

    /**
     * Calculates textX and textY attributes
     */
    void CalcTextPos();

    /**
     * Calculates size of text
     */
    void CalcTextSize(int & width, int & height);
    
    /**
     * Fills 'letterSizes' array. Can be called only when vertical==true
     */
    void CalcLetterSizes();

    /// The text for this box.
    csString text;

    /// The colour of text.
    int colour;

    bool grayed;
    
    pawsHorizAdjust horizAdjust;
    pawsVertAdjust  vertAdjust;
    
    bool vertical;
    
    int textX, textY;        // Position of text inside the widget (depends on adjustment)
    
    //These members are used only when vertical==true:
    int textWidth;           // Width of vertical column of letters (i.e. MAX(letter_width))
    psPoint *letterSizes;    // Size of each letter.
};

//--------------------------------------------------------------------------
CREATE_PAWS_FACTORY( pawsTextBox );



//--------------------------------------------------------------------------

#define MESSAGE_TEXTBOX_MOUSE_SCROLL_AMOUNT 3
/** This is a special type of text box that is used for messages.
 * This text box allows each 'message' to be stored as it's own line with
 * it's own colours.
 */
class pawsMessageTextBox : public pawsWidget
{
public:
	struct MessageSegment
	{
		int x;
		csString text;
		int colour;
		int size;
		
		MessageSegment() : x(0), text(""), colour(0), size(0) { }
	};
    struct MessageLine
    {
        csString text;
        int colour;
        int size;
        csArray<MessageSegment> segments;

        MessageLine()
        {
            text = "";
            colour = 0;
            size = 0;
        }
    };

    pawsMessageTextBox( );
    virtual ~pawsMessageTextBox();    
    virtual bool Setup( iDocumentNode* node );
    virtual bool Setup( void );

    /**
     * This function allows a widget to fill in its own contents from an xml node
     * supplied.  This is NOT the same as Setup, which is defining height, width,
     * styles, etc.  This is data.
     */
    bool SelfPopulate( iDocumentNode *node);

    virtual void Draw();
    
    /** Add a new message to this box.
     * @param data The new message to add. 
     * @param colour The colour this message should be. -1 means use the default
     *               colour that is available.
     */
    void AddMessage( const char* data, int colour = -1 );

    void AppendLastMessage( const char* data );

    virtual void OnResize();
    virtual void Resize(); 
    virtual bool OnScroll( int direction, pawsScrollBar* widget );
    virtual bool OnMouseDown( int button, int modifiers, int x, int y );
    
    void Clear();

    int GetBorderStyle() { return BORDER_SUNKEN; }

    /** Resets scrollbar to starting position (offset=0) */
    void ResetScroll();

    /** Sets the scrollbar to 100% (offset=max) */
    void FullScroll();

    virtual void OnUpdateData(const char *dataname,PAWSData& data);
    

protected:
    /// Renders an entire message and returns the total lines it took.
    int RenderMessage( const char* data, int lineStart, int colour );

    /// List of messages this box has. 
    csPDelArray<MessageLine> messages;
    
    //void AdjustMessages();
    
    void SplitMessage( const char* newText, int colour, int size, MessageLine*& msgLine, int& startPosition );

    /// Calculates value of the lineHeight attribute
    void CalcLineHeight();
    
    csPDelArray<MessageLine> adjusted;

    int lineHeight;
    size_t maxLines;

    int topLine;

    pawsScrollBar* scrollBar;
        
};

CREATE_PAWS_FACTORY( pawsMessageTextBox );

#define EDIT_TEXTBOX_MOUSE_SCROLL_AMOUNT 3
#define BLINK_TICKS  1000
/** An edit box widget/
 */
class pawsEditTextBox : public pawsWidget
{
public:
    pawsEditTextBox();
    virtual ~pawsEditTextBox();

    virtual void Draw();

    bool Setup( iDocumentNode* node );

    bool OnKeyDown( int code, int key, int modifiers );
    
    const char* GetText() { return text.GetDataSafe(); }

    void SetMultiline(bool multi);
    void SetPassword(bool pass); //displays astrices instead of text

    /** Change the text in the edit box.
     * @param text The text that will replace whatever is currently there.
     */
    void SetText( const char* text, bool publish = true );
    virtual void OnUpdateData(const char *dataname,PAWSData& data);
    
    // Sets size of the widget to fit the text that it displays:
    void SetSizeByText();

    void Clear();

    virtual bool OnMouseDown( int button, int modifiers, int x, int y );

    int GetBorderStyle() { return BORDER_SUNKEN; }    

    /**
     * This function allows a widget to fill in its own contents from an xml node
     * supplied.  This is NOT the same as Setup, which is defining height, width,
     * styles, etc.  This is data.
     */
    bool SelfPopulate( iDocumentNode *node);

    /**
     * Set & Get top line funcs
     */
    unsigned int GetTopLine() { return topLine; }
    void SetTopLine(unsigned int newTopLine) { topLine = newTopLine; }
    
    void SetCursorPosition(size_t pos) { cursorPosition = pos; }

    virtual const bool GetFocusOverridesControls() const { return true; }

protected:

    bool password;
    csRef<iVirtualClock> clock;

    /// Position of first character that we display 
    int start;

    /// The position of the cursor blink 
    size_t cursorPosition;
    unsigned int cursorLine;

    /// The current blink status
    bool blink;

    /// Keep track of ticks for flashing
    csTicks blinkTicks;

    /// Current input line
    csString text;

    bool multiLine;

    int lineHeight;
    size_t maxLines;

    // This value will always be one less than the number of used lines
    size_t usedLines;

    unsigned int topLine;

    pawsScrollBar* vScrollBar;
};

CREATE_PAWS_FACTORY( pawsEditTextBox );


//---------------------------------------------------------------------------------------


#define MULTILINE_TEXTBOX_MOUSE_SCROLL_AMOUNT 3
class pawsMultiLineTextBox : public pawsWidget
{
public:    
    pawsMultiLineTextBox();
    virtual ~pawsMultiLineTextBox();

    bool Setup ( iDocumentNode* node );

    void Draw();
      
    void SetText( const char* text );
    const char* GetText(){return text;}
    void Resize();
    bool OnScroll( int direction, pawsScrollBar* widget );
    virtual bool OnMouseDown( int button, int modifiers, int x, int y );
    
    // Normal text boxes should not be focused.
    virtual bool OnGainFocus( bool notifyParent = true ) {return false;}
    virtual void OnUpdateData(const char *dataname,PAWSData& data);
    
protected:

    void OrganizeText( const char* text );

    /// The text for this box.
    csString text;
    /// The text, broken into separate lines by OrganizeText()
    csArray<csString> lines;
    
    //where our scrollbar at?
    size_t startLine;
    //Number of lines that we can fit in the box
    size_t canDrawLines;
    bool usingScrollBar;
    int maxWidth;
    int maxHeight;
    pawsScrollBar* scrollBar;
};

//--------------------------------------------------------------------------
CREATE_PAWS_FACTORY( pawsMultiLineTextBox );

/* An edit box widget */
class pawsMultilineEditTextBox : public pawsWidget
{
public:
    pawsMultilineEditTextBox();
    virtual ~pawsMultilineEditTextBox();

    struct MessageLine
    {
        size_t lineLength; //Stores length of the line.
        size_t breakLine; //Stores real text position of where break occurs.
        int lineExtra; //Stores 1 if line ends with a \n, or 0 if it does not.
    };
    
    bool Setup( iDocumentNode* node );
    
    virtual void Draw();
    void DrawWidgetText(const char *text, size_t x, size_t y, int style, int fg);

    /** Change the text in the edit box.
     * @param text The text that will replace whatever is currently there.
     */
    void SetText( const char* text, bool publish = true );
    const char* GetText();
    void UpdateScrollBar();
    virtual void SetupScrollBar();

    bool OnScroll( int direction, pawsScrollBar* widget );
    virtual void OnResize();
    virtual bool OnMouseUp( int button, int modifiers, int x, int y );
    virtual bool OnMouseDown( int button, int modifiers, int x, int y );
    virtual void OnUpdateData(const char *dataname,PAWSData& data);
    virtual bool OnKeyDown( int code, int key, int modifiers );
    virtual void CalcMouseClick( int x, int y, size_t &cursorLine, size_t &cursorChar);
    
    virtual const bool GetFocusOverridesControls() const { return true; }

    void PushLineInfo( size_t lineLength, size_t lineBreak, int lineExtra);

    int GetBorderStyle() { return BORDER_SUNKEN; }    
    void LayoutText();

    /**
     * This function allows a widget to fill in its own contents from an xml node
     * supplied.  This is NOT the same as Setup, which is defining height, width,
     * styles, etc.  This is data.
     */
    bool SelfPopulate( iDocumentNode *node);

    /**
     * Set & Get top line funcs
     */
    size_t GetTopLine() { return topLine; }
    void SetTopLine(size_t newTopLine) { topLine = newTopLine; }
    size_t GetCursorPosition();
    size_t GetCursorPosition(size_t destLine, size_t destCursor);
    void SetCursorPosition(size_t pos) { cursorPosition = pos; }
    void Clear(){ SetText(""); }
    csString GetLine(size_t line);
    int GetLineWidth(int lineNumber);
    void GetLinePos(size_t lineNumber, size_t &start, size_t &end);
    void GetLineRelative(size_t pos, size_t &start, size_t &end);
    bool EndsWithNewLine(int lineNumber);
    void GetCursorLocation(size_t pos, size_t &destLine, size_t &destCursor);
    const char GetAt(size_t destLine, size_t destCursor);

protected:

    csRef<iVirtualClock> clock;

    // The position of the cursor blink 
    size_t cursorPosition;
    
    // The position of the cursor in an array (Used by draw method).
    size_t cursorLine;
    size_t cursorLoc;

    /// The current blink status
    bool blink;

    /// Keep track of ticks for flashing
    csTicks blinkTicks;

     /// Concatenated contents of lines, only valid after GetText()
    csString text;
    csPDelArray<MessageLine> lineInfo;
    int lineHeight;

    // Width of the vertical scroll bar
    int vScrollBarWidth;
    size_t canDrawLines;
    
    size_t topLine;
    pawsScrollBar* vScrollBar;
        
    void ReflowText();
    bool usingScrollBar;

    int maxWidth;
    int maxHeight;

    int yPos;
    csString tmp;
};

CREATE_PAWS_FACTORY( pawsMultilineEditTextBox );

/// pawsFadingTextBox - Used for mainly on screen messages, deletes itself upon 100% fade
#define FADE_TIME              1000 // Time for the onscreen message to fade away
#define MESG_TIME              3000 // Time for the onscreen message to be shown ( not including fading)

class pawsFadingTextBox : public pawsWidget
{
public:
    pawsFadingTextBox();
    virtual ~pawsFadingTextBox() {};

    bool Setup ( iDocumentNode* node ) { return true;} // Shouldn't be created in XML, only by code

    void Draw();

    void Fade(); // Force fading
    void SetText(const char* newtext, iFont* font1,iFont* font2, int color,int time=MESG_TIME,int fadetime=FADE_TIME);

    void GetSize(int &w, int &h);
private:
    void DrawBorderText(const char* text, iFont* font,int x);

    csString text;
    csString first;

    csRef<iFont> firstFont; // used for the first letter
    csRef<iFont> font;      // used for the rest

    int org_color;
    int color;
    int scolor;
    csTicks start;

    int ymod;

    int time,fadetime;
};

CREATE_PAWS_FACTORY( pawsFadingTextBox );

#endif 

