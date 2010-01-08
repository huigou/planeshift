/*
 * pawstextbox.cpp - Author: Andrew Craig
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
// pawstextbox.cpp: implementation of the pawsTextBox class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>
#include <ctype.h>
#include <ivideo/fontserv.h>
#include <iutil/virtclk.h>
#include <iutil/evdefs.h>
#include <iutil/event.h>

#include "pawstextbox.h"
#include "pawsmanager.h"
#include "pawsprefmanager.h"
#include "pawscrollbar.h"
#include "util/log.h"
#include "util/strutil.h"
#include "util/psstring.h"

#define BORDER_SIZE            2 // For pawsFadingTextBox


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsTextBox::pawsTextBox() : textX(0), textY(0)
{
    horizAdjust = horizLEFT;
    vertAdjust  = vertCENTRE;
    
    vertical = false;
    grayed = false;
    letterSizes = NULL;
}

pawsTextBox::~pawsTextBox()
{
    if (letterSizes != NULL)
        delete [] letterSizes;
}

bool pawsTextBox::Setup( iDocumentNode* node )
{    
    csRef<iDocumentAttribute> vertAttribute = node->GetAttribute("vertical");
    if ( vertAttribute )
        SetVertical(strcmp(vertAttribute->GetValue(), "yes") == 0);

    csRef<iDocumentNode> textNode = node->GetNode( "text" );        
    
    if ( textNode )
    {
        csRef<iDocumentAttribute> fontAdjustAttribute;
        fontAdjustAttribute = textNode->GetAttribute("horizAdjust");
        if ( fontAdjustAttribute )
        {
            csString fontAdjust = fontAdjustAttribute->GetValue();            
            if (fontAdjust == "CENTRE")
                HorizAdjust(horizCENTRE);
            else if (fontAdjust == "RIGHT")
                HorizAdjust(horizRIGHT);
            else
                HorizAdjust(horizLEFT);
        }            

        fontAdjustAttribute = textNode->GetAttribute("vertAdjust");
        if ( fontAdjustAttribute )
        {
            csString fontAdjust = fontAdjustAttribute->GetValue();            
            if (fontAdjust == "CENTRE")
                VertAdjust(vertCENTRE);
            else if (fontAdjust == "BOTTOM")
                VertAdjust(vertBOTTOM);
            else
                VertAdjust(vertTOP);
        } 
		else
			VertAdjust(vertCENTRE);  // default is centered vertically unless specified otherwise
        
        csRef<iDocumentAttribute> textAttribute = textNode->GetAttribute("string");
        if ( textAttribute )
        {
            SetText(PawsManager::GetSingleton().Translate(textAttribute->GetValue()));
        }
    }
        
    return true;
}

void pawsTextBox::CalcTextPos()
{
    int width, height;

	if (!screenFrame.Height())
		SetSizeByText(0,0);

    if (horizAdjust==horizRIGHT  ||  horizAdjust==horizCENTRE  ||  vertAdjust==vertBOTTOM  ||  vertAdjust==vertCENTRE)
        CalcTextSize(width, height);

    switch (horizAdjust)
    {
        case horizLEFT:
            textX = 0;
            break;
        case horizRIGHT:
            textX = (screenFrame.Width()-margin*2) - width;
            break;
        case horizCENTRE:
            textX = ((screenFrame.Width()-margin*2) - width)  / 2;
            break;
    }

    switch (vertAdjust)
    {
        case vertTOP:
            textY = 0;
            break;
        case vertBOTTOM:
            textY = (screenFrame.Height()-margin*2) - height;
            break;
        case vertCENTRE:
            textY = ((screenFrame.Height()-margin*2) - height) / 2;
            break;
    }
}

void pawsTextBox::CalcLetterSizes()
{
    char letterStr[2];
    unsigned int i;
    
    assert(vertical);
    
    if (letterSizes != NULL)
        delete [] letterSizes;
    
    letterSizes = new psPoint [text.Length()];
    textWidth = 0;
    letterStr[1] = '\0'; // Make sure the string is terminated
                         // pos 0 will be filled in later.
    
    for (i=0; i < text.Length(); i++)
    {
        letterStr[0] = text.GetAt(i);

        GetFont()->GetDimensions(letterStr, letterSizes[i].x, letterSizes[i].y);

        textWidth = MAX(textWidth, letterSizes[i].x);
    }
}

void pawsTextBox::SetVertical(bool vertical)
{
    this->vertical = vertical;
    
    if (vertical)
        CalcLetterSizes();
    else if (letterSizes != NULL)
    {
        delete [] letterSizes;
        letterSizes = NULL;
    }
    CalcTextPos();
}

void pawsTextBox::Adjust(pawsHorizAdjust horiz, pawsVertAdjust vert)
{
    horizAdjust  = horiz;
    vertAdjust   = vert;
    CalcTextPos();
}

void pawsTextBox::HorizAdjust(pawsHorizAdjust horiz)
{
    horizAdjust  = horiz;
    CalcTextPos();
}

void pawsTextBox::VertAdjust(pawsVertAdjust vert)
{
    vertAdjust   = vert;
    CalcTextPos();
}

void pawsTextBox::CalcTextSize(int& width, int& height)
{
    if (vertical)
    {
        unsigned int i;
        
        width = 0;
        height = 0;

        for (i=0; i < text.Length(); i++)
        {
            width   = MAX(width, letterSizes[i].x);
            height  += letterSizes[i].y;
        }
    }
    else if (text.GetData() != NULL)
    {
        GetFont()->GetDimensions( (const char*)text, width, height );
        //width+=5;
        //height+=5;
    }        
    else
    {
        GetFont()->GetDimensions("Sample", width, height);  // Example text with full height caps and descenders
    }
}

bool pawsTextBox::SelfPopulate( iDocumentNode *node)
{
    if (node->GetAttributeValue("text"))
    {
        SetText (node->GetAttributeValue("text"));
        return true;
    }
	else if (node->GetContentsValue())
	{
		SetText (node->GetContentsValue());
		return true;
	}
    else
        return false;
}

void pawsTextBox::FormatText( const char *fmt, ... )
{
    char text[1024];
    va_list args;

    va_start(args, fmt);
    cs_vsnprintf(text,sizeof(text),fmt,args);
    va_end(args);
    
    SetText( (const char*)text );
}    


void pawsTextBox::SetText( const char* newText )
{
    // Make sure any \r characters are replaced by printable \n ones.
    psString str(newText);
    str.ReplaceAll("\r", "\n");

    text.Replace(str.GetData());

    if (vertical)
        CalcLetterSizes();
    CalcTextPos();
}

void pawsTextBox::SetSizeByText(int padX, int padY)
{
    int width, height;
 
    // These padding parameters used to be hardcoded to 5,5 in all cases inside CalcTextSize.
    CalcTextSize(width, height);
    SetRelativeFrameSize(width+padX, height+padY);
    textX = 0;
    textY = 0;
}

int pawsTextBox::GetFontColour()
{
    if (grayed)
        return graphics2D->FindRGB( 150,150,100 );
    return pawsWidget::GetFontColour();
}

void pawsTextBox::Draw()
{
    pawsWidget::Draw();

    if (vertical)
    {
        char letterStr[2];
        int letterY;
        
        letterStr[1] = 0;
        letterY = textY;
        for (unsigned int i = 0; i < text.Length(); i++)
        {
            letterStr[0] = text.GetAt(i);
            DrawWidgetText( letterStr , 
                            screenFrame.xmin + margin + textX + (textWidth-letterSizes[i].x)/2,
                            screenFrame.ymin + margin + letterY );

            letterY += letterSizes[i].y;
        }
    }
    else
    {
		DrawWidgetText( (const char*)text, 
						screenFrame.xmin + margin + textX,
						screenFrame.ymin + margin + textY );   
    }
}

void pawsTextBox::OnUpdateData(const char *dataname,PAWSData& value)
{
    // This is called automatically whenever subscribed data is published.
    if (subscription_format == "percent")
    {
        float f = value.GetFloat();
        char buff[10];
        sprintf(buff,"%.01f%%",f*100);
        SetText(buff);
    }
    else
        SetText( value.GetStr());
}



//----------------------------------------------------------------------------------------

pawsMessageTextBox::pawsMessageTextBox()
{
    topLine = 0;
    maxLines = 0;
    CalcLineHeight();
    scrollBar = NULL;
}

pawsMessageTextBox::~pawsMessageTextBox()
{
    Clear();
}

void pawsMessageTextBox::Clear()
{
    messages.Empty();
    adjusted.Empty();
    topLine = 0;
    if (scrollBar)
        scrollBar->Hide();
}

void pawsMessageTextBox::CalcLineHeight()
{
    int dummy;
    GetFont()->GetMaxSize( dummy, lineHeight );
    lineHeight -=2;
}

bool pawsMessageTextBox::Setup( iDocumentNode* node )
{
    csRef<iDocumentNode> scrollBarNode = node->GetNode( "pawsScrollBar" );
    if ( scrollBarNode )
    {
        CalcLineHeight();

        // Create the optional scroll bar here as well but hidden.
        scrollBar = new pawsScrollBar;
        //scrollBar->Setup( scrollBarNode );
        scrollBar->SetParent( this );

        csRef<iDocumentAttribute> widthAttribute = scrollBarNode->GetAttribute("width");
        int scrollBarWidth = 25;
        if(widthAttribute){
            scrollBarWidth = widthAttribute->GetValueAsInt();
        }

        scrollBar->SetRelativeFrame( defaultFrame.Width() - scrollBarWidth, 6, scrollBarWidth, defaultFrame.Height() - 12 ); 
        
        int attach = ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT;
        scrollBar->SetAttachFlags( attach );
        scrollBar->PostSetup();
        AddChild( scrollBar );
        OnResize();
        return true;    
    } 
    else 
    {
        return Setup();
    }
}

bool pawsMessageTextBox::Setup()
{
    CalcLineHeight();

    // Create the optional scroll bar here as well but hidden.
    scrollBar = new pawsScrollBar;
    scrollBar->SetParent( this );
    scrollBar->SetRelativeFrame( defaultFrame.Width() - 24, 6, 24, defaultFrame.Height() - 12 ); 
    int attach = ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT;
    scrollBar->SetAttachFlags( attach );
    scrollBar->PostSetup();
    scrollBar->SetTickValue( 1.0 );
    scrollBar->SetMaxValue(0.0);
    AddChild( scrollBar );

    OnResize();

    return true;
}

void pawsMessageTextBox::Resize()
{
    pawsWidget::Resize();

    CalcLineHeight();

    adjusted.Empty();    

    for ( size_t x = 0; x < messages.GetSize(); x++ )    
    {        
        MessageLine* dummy = NULL;
        int dummyX = -1;
    	if(messages[x]->segments.IsEmpty())
    		SplitMessage( messages[x]->text, messages[x]->colour, messages[x]->size, dummy, dummyX ); 
    	else
    	{
    		MessageLine* msgLine = NULL;
    		int offsetX = 0;
    		for(size_t i = 0; i < messages[x]->segments.GetSize(); i++)
    			SplitMessage( messages[x]->segments[i].text, messages[x]->segments[i].colour, messages[x]->segments[i].size, msgLine, offsetX);
    	}
    } 
}

void pawsMessageTextBox::OnResize()
{
    int borderOffSize = 4;
    if ( border )
        borderOffSize = 8;

    int rest = (screenFrame.Height()-borderOffSize) % lineHeight;
    if (screenFrame.Height() >= borderOffSize+rest)
        maxLines = (screenFrame.Height()-borderOffSize-rest) / lineHeight;
    else
        maxLines = 0;

    // Re adjust the top line value 
    topLine = (int)adjusted.GetSize() - (int)maxLines;
    if ( topLine < 0 )
        topLine = 0;

    if (scrollBar)
    {
        scrollBar->SetMaxValue( maxLines > adjusted.GetSize() ? 0 : (float)(adjusted.GetSize()-maxLines) );
        scrollBar->SetCurrentValue( (float)topLine );

        if ( maxLines < adjusted.GetSize() )
        {
            scrollBar->Show();
            scrollBar->RecalcScreenPositions();
        }
        else
            scrollBar->Hide();
    }
}


void pawsMessageTextBox::Draw()
{
    pawsWidget::Draw();

    ClipToParent(false);

    int yPos = 0;
    
    if ( topLine < 0 )
        topLine = 0;

    for ( size_t x = topLine; x < (topLine+maxLines); x++ )
    {
        if ( x < adjusted.GetSize() )
        {
        	if(adjusted[x]->segments.IsEmpty())
        	{
				// Draw shadow
				graphics2D->Write(  GetFont(),
									screenFrame.xmin + 1,
									screenFrame.ymin + yPos*lineHeight + 1,
									0,
									-1,
									(const char*)adjusted[x]->text );
				// Draw actual text
				graphics2D->Write(  GetFont(),
									screenFrame.xmin,
									screenFrame.ymin + yPos*lineHeight,
									adjusted[x]->colour,
									-1,
									(const char*)adjusted[x]->text );
        	}
        	else
        	{
        		for(size_t i = 0; i < adjusted[x]->segments.GetSize(); i++)
        		{
					// Draw shadow
					graphics2D->Write(  GetFont(),
										screenFrame.xmin + adjusted[x]->segments[i].x + 1,
										screenFrame.ymin + yPos*lineHeight + 1,
										0,
										-1,
										(const char*)adjusted[x]->segments[i].text );
					// Draw actual text
					graphics2D->Write(  GetFont(),
										screenFrame.xmin + adjusted[x]->segments[i].x,
										screenFrame.ymin + yPos*lineHeight,
										adjusted[x]->segments[i].colour,
										-1,
										(const char*)adjusted[x]->segments[i].text );
        		}
        	}
           
            yPos++;                                                                            
        }
    }
}


bool pawsMessageTextBox::SelfPopulate( iDocumentNode *node)
{
    if (node->GetAttributeValue("text"))
    {
        Clear();
        AddMessage(node->GetAttributeValue("text"));
        return true;
    }
    return false;
}


void pawsMessageTextBox::AddMessage( const char* data, int msgColour )
{
	// Notify parent of activity
	OnChange(this);
    // Extract \n out of the data and print a newline for each.
    csString message = data;
    csArray<csString> cutMessages;

    size_t pos = message.Length();
    if(pos == 0)
    {
      cutMessages.Push("");
    }

    while(pos > 0)
    {
        size_t last = message.FindLast("\n");
        if(last == (size_t)-1)
        {
            cutMessages.Push(message);
            break;
        }
        if(last == 0)
        {
            cutMessages.Push(message.Slice(1, pos));
            break;   
        }
        if(last != message.Length()-1)
        {
            cutMessages.Push(message.Slice(last+1, pos));
            cutMessages.Push("");
        }
        cutMessages.Push("");
        message.Truncate(last);
        pos = last;
    }

    while(!cutMessages.IsEmpty())
    {
        csString messageText = cutMessages.Pop();
        bool onBottom = false;
        int oldTopLine = topLine;

        if ( topLine == (int)scrollBar->GetMaxValue() )
            onBottom = true;                        
        if ( (size_t)scrollBar->GetMaxValue() < maxLines )
            onBottom = true;    

        if ( topLine < 0 )
            topLine = 0;                 

        // Add it to the main message buffer.
        MessageLine * msg = new MessageLine;

        // Find font info embedded in the data.
        int colour = msgColour;
        int size = 0;

        if(msgColour == -1)
        {
            colour = GetFontColour();
        }
        
    	size_t textStart = 0;
    	size_t textEnd = messageText.Length();
    	int x = 0;
    	MessageLine* msgLine = NULL;
        msg->size = 0;
        msg->colour = msgColour;
        msg->text = messageText;

        // Empty line is a special case here
        if(messageText.Length() == 0)
        {
        	SplitMessage(messageText, colour, size, msgLine, x);
        }

    	while(textStart < messageText.Length())
    	{
			size_t pos = messageText.FindFirst(ESCAPECODE, textStart);
			if(pos == (size_t) - 1)
				textEnd = messageText.Length();
			else
				textEnd = pos;
			
			if(textStart == 0 && pos == (size_t) - 1)
			{
				int dummyX = -1;
				SplitMessage( messageText, colour, size, msgLine, dummyX);
				break;
			}
			else if(textEnd > textStart)
			{
				csString subText = messageText.Slice(textStart, textEnd - textStart);
				SplitMessage( subText, colour, size, msgLine, x);   
				
				MessageSegment newSegment;
				newSegment.text = subText;
				newSegment.colour = colour;
				newSegment.size = size;
				msg->segments.Push(newSegment);
			}
			textStart = textEnd;
			
			if(pos != (size_t)-1 && pos + LENGTHCODE <= messageText.Length())
			{
				int r, g, b;
				if (!psColours::ParseColour(messageText.GetData() + pos, r, g, b, size))
				{
					// Not a colour code so skip
					textStart++;
					continue;
				}
				if(r == 0 && g == 0 && b == 0)
					colour = msgColour;
				else
					colour = graphics2D->FindRGB(r, g, b);
				messageText.DeleteAt(pos, LENGTHCODE);
			}
    	}  
        messages.Push(msg);  
        if (scrollBar)
        {
            if ( adjusted.GetSize() > maxLines )
                scrollBar->ShowBehind();
            else
                scrollBar->Hide();
        }


        scrollBar->SetMaxValue( maxLines > adjusted.GetSize() ? 0 : (float)(adjusted.GetSize()-maxLines) );

        topLine = (int)adjusted.GetSize() - (int)maxLines;      
        if ( topLine < 0 )
            topLine = 0;                 


        if ( !onBottom )            
        {
            topLine = oldTopLine;
            scrollBar->SetCurrentValue( float(topLine) );
        }
        else
        {
            scrollBar->SetCurrentValue( float(topLine) );
        }
    }
}

void pawsMessageTextBox::AppendLastMessage(const char* data)
{
    if(messages.IsEmpty())
    {
      AddMessage(data);
      return;
    }

    MessageLine* line = messages.Get(messages.GetSize()-1);
    line->text.Append(data);
    line = adjusted.Get(adjusted.GetSize()-1);
    line->text.Append(data);

    // Trim \n from the end and add a new line.
    while(line->text.FindLast("\n") == line->text.Length()-1)
    {
      line->text.Truncate(line->text.Length()-1);
      AddMessage("");
    }
}

void pawsMessageTextBox::OnUpdateData(const char *dataname,PAWSData& value)
{
    // This is called automatically whenever subscribed data is published.
    if (overwrite_subscription)
        Clear();

    if(value.type == PAWS_DATA_INT_STR)
    	AddMessage(value.GetStr(), value.GetInt());
    else
    	AddMessage( value.GetStr() );
}

void pawsMessageTextBox::ResetScroll()
{
    if (scrollBar)
        scrollBar->SetCurrentValue(0);
}

void pawsMessageTextBox::FullScroll()
{
    if(scrollBar)
        scrollBar->SetCurrentValue(scrollBar->GetMaxValue());

}

void pawsMessageTextBox::SplitMessage(const char* newText, int colour,
        int size, MessageLine*& msgLine, int& startPosition)
{
    csString stringBuffer(newText);

    if (stringBuffer.IsEmpty())
    {
        return;
    }

    while (!stringBuffer.IsEmpty())
    {
        int offSet = INITOFFSET;
        int width = -1;
        int height = -1;
        /// See how many characters can be drawn on a single line.
        if (startPosition != -1)
        {
            GetFont()->GetDimensions(stringBuffer.GetData(), width, height);
            offSet += startPosition;
        }
        int canDrawLength = GetFont()->GetLength(stringBuffer.GetData(),
                screenFrame.Width() - offSet);

        /// If it can fit the entire string then return.
        if (canDrawLength == stringBuffer.Length())
        {
            if (!msgLine)
            {
                WriteMessageLine(msgLine,stringBuffer,colour);
            }
            if (startPosition != -1)
            {
                WriteMessageSegment(msgLine,stringBuffer,colour,startPosition);
                startPosition += width;
            }
            break;
        }
        else
        {
            csString processedString = FindStringThatFits(stringBuffer, canDrawLength);            
            stringBuffer.DeleteAt(0, processedString.Length());
            
            if (!msgLine)
            {
                WriteMessageLine(msgLine,processedString,colour);
            }
            if (startPosition != -1)
            {
                WriteMessageSegment(msgLine,processedString,colour,startPosition);
                startPosition = 0;
            }
            // Next time use a new line!
            msgLine = NULL;
        }
    }
}

void pawsMessageTextBox::WriteMessageLine(MessageLine*& msgLine, csString text, int colour)
{
    msgLine = new MessageLine;
    msgLine->size = text.Length();
    msgLine->text = text;
    msgLine->colour = colour;
    adjusted.Push(msgLine);
}

void pawsMessageTextBox::WriteMessageSegment(MessageLine*& msgLine, csString text, int colour, int startPosition)
{
    MessageSegment seg;
    seg.size = text.Length();
    seg.text = text;
    seg.colour = colour;
    seg.x = startPosition;
    msgLine->segments.Push(seg);
}

csString pawsMessageTextBox::FindStringThatFits(csString stringBuffer, int canDrawLength)
{
    int breakPoint = stringBuffer.FindLast(' ', canDrawLength - 1);
    int spaceAfterBreak = stringBuffer.FindFirst(' ', breakPoint + 1);
    int wordLength = spaceAfterBreak - breakPoint;
    csString wordAfterBreak;

    if (spaceAfterBreak == -1)
    {
        stringBuffer.SubString(wordAfterBreak, breakPoint + 1);
    }
    else
    {
        stringBuffer.SubString(wordAfterBreak, breakPoint + 1, wordLength);
    }

    int width = 0;
    int height = 0;
    GetFont()->GetDimensions(wordAfterBreak.GetData(), width, height);

    csString processedString;
    if (width <= screenFrame.Width() - INITOFFSET)
    {
        processedString.Append(stringBuffer, breakPoint + 1);
    }
    else
    {
        processedString.Append(stringBuffer, canDrawLength);
    }
    return processedString;
}

bool pawsMessageTextBox::OnScroll( int direction, pawsScrollBar* widget )
{   
    topLine = (int)widget->GetCurrentValue();       
    return true;
}

bool pawsMessageTextBox::OnMouseDown( int button, int modifiers, int x, int y )
{
    if (button == csmbWheelUp)
    {
        if (scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() - MESSAGE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if (button == csmbWheelDown)
    {
        if (scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() + MESSAGE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }

    return pawsWidget::OnMouseDown( button, modifiers, x ,y);
}
    


//---------------------------------------------------------------------------------


pawsEditTextBox::pawsEditTextBox()
{
    start = 0;
    blink = true;
    cursorPosition = 0;
    password = 0;

    clock =  csQueryRegistry<iVirtualClock > ( PawsManager::GetSingleton().GetObjectRegistry());

    blinkTicks = clock->GetCurrentTicks();

    // Find the line height;
    int dummy;
    GetFont()->GetMaxSize( dummy, lineHeight );
    lineHeight -=2;
}

pawsEditTextBox::~pawsEditTextBox()
{
}

void pawsEditTextBox::SetSizeByText()
{
    int width, height;
    
    if (GetFont()!=NULL && text.GetData()!=NULL )
    {       
        GetFont()->GetDimensions( text.GetData(), width, height );
        SetSize(width+margin*2, height+margin*2);
    }
}

void pawsEditTextBox::SetPassword( bool pass )
{
    password = pass;
}

bool pawsEditTextBox::Setup( iDocumentNode* node )
{
    csRef<iDocumentNode> textNode = node->GetNode( "text" );        
    if (textNode)
    {
        csRef<iDocumentAttribute> stringAttribute = textNode->GetAttribute("string");
        if ( stringAttribute )
        {
            SetText( PawsManager::GetSingleton().Translate(stringAttribute->GetValue()) );
        }
    }
    // Find the line height
    int dummy;
    GetFont()->GetMaxSize( dummy, lineHeight );
    lineHeight -=2;
            
    return true;
}

bool pawsEditTextBox::SelfPopulate( iDocumentNode *node)
{
    if (node->GetAttributeValue("text"))
    {
        SetText(node->GetAttributeValue("text"));
        return true;
    }
    else
        return false;
}

void pawsEditTextBox::SetText( const char* newText, bool publish )
{
    if (publish && subscribedVar)
        PawsManager::GetSingleton().Publish(subscribedVar, newText);

    text.Replace( newText );
    
    if (newText)
        cursorPosition = strlen( newText );
    else
        cursorPosition = 0;
}

void pawsEditTextBox::OnUpdateData(const char *dataname,PAWSData& value)
{
    // This is called automatically whenever subscribed data is published.
    SetText( value.GetStr(), false );
}

void pawsEditTextBox::Draw()
{
    if ( clock->GetCurrentTicks() - blinkTicks > BLINK_TICKS )
    {
        blink = !blink;
        blinkTicks = clock->GetCurrentTicks();
    }
    
    pawsWidget::Draw();

    ClipToParent(false);

    if (cursorPosition>text.Length())
        cursorPosition=text.Length();
  
    if(start>(int)text.Length())
        start=0;               
        
    if ( text.Length() > 0 )
    {
        // Get the number of characters to draw
        int maxChars;
        maxChars = GetFont()->GetLength( text.GetData() + start, screenFrame.Width()-margin*2);

        if ( (int)cursorPosition > start + maxChars )
            start = (int)cursorPosition - maxChars;
        if ( start < 0 )
            start = 0;

        // Make the text the correct length
        csString tmp ( text.GetData() + start );
        tmp.Truncate( maxChars );
        if (password) //show astrices instead of text
             for (unsigned int i=0;i<tmp.Length();i++)
                 tmp.SetAt(i,'*');

        // Get size of text
        int textWidth;
        int textHeight;

        GetFont()->GetDimensions( tmp.GetData(), textWidth, textHeight );

        // Get the center
        int textCenterX = 4;
        int textCenterY = (screenFrame.Height()-(margin*2)) / 2  - textHeight / 2;


        DrawWidgetText( tmp.GetData(),
                        screenFrame.xmin + textCenterX + margin,
                        screenFrame.ymin + textCenterY + margin);

        if ( blink && hasFocus )
        {
            int cursor = (int)cursorPosition - start;
            tmp.Truncate( cursor );

            // Figure out where to put the cursor.
            int width, height;
            GetFont()->GetDimensions( tmp, width, height );

            graphics2D->DrawLine( (float)(screenFrame.xmin + margin + textCenterX + width + 1),
                (float)(screenFrame.ymin + margin + textCenterY),
                (float)(screenFrame.xmin + margin + textCenterX + width + 1),
                (float)(screenFrame.ymin + margin + textCenterY+height),
                GetFontColour() );
        }
    } // End if stored.Length() > 0
    else if ( blink && hasFocus )
    {
        graphics2D->DrawLine( (float)screenFrame.xmin + margin + 5,
            (float)screenFrame.ymin + margin + 4,
            (float)screenFrame.xmin + margin + 5,
            (float)screenFrame.ymax - (margin + 4),
            GetFontColour() );
    }
}


bool pawsEditTextBox::OnKeyDown( int code, int key, int modifiers )
{   
    bool changed = false;
    blink = true;

    switch ( key )
    {
    case CSKEY_DEL:
    {
        if (cursorPosition >= text.Length())
            break;
        if ( cursorPosition != (size_t)-1 )
        {
            if ( cursorPosition - start < 5 ) 
                start = (int)cursorPosition - 5;

            if ( start < 0 ) 
                start = 0;

            if ( text.Length() > 1 )
            {
                csString tmp(text.GetData());
                tmp.Truncate(cursorPosition);
                tmp.Append(text.GetData() + cursorPosition + 1);
                text.Replace(tmp);
            } 
            else
            {
                text.Clear();
            }
        } //endif cursor position > 0
        changed = true; 
        break;        
    }
    
    
    case CSKEY_BACKSPACE:
        if (cursorPosition > text.Length())
            cursorPosition = text.Length();
        if ( cursorPosition > 0 )
        {
            cursorPosition--;
            if ( cursorPosition - start < 5 ) 
                start = (int)cursorPosition - 5;

            if ( start < 0 ) 
                start = 0;

            if ( text.Length() > 1 )
            {
                if ( cursorPosition == text.Length() )
                {
                    text.Truncate( text.Length() - 1 );
                }
                else
                {
                    csString tmp( text.GetData() );
                    tmp.Truncate( cursorPosition );
                    tmp.Append( text.GetData() + cursorPosition + 1 );
                    text.Replace( tmp );
                }
            } 
            else
            {
                text.Clear();
            }
        }
        changed = true;
        break;
        // End case CSKEY_BACKSPACE

    case CSKEY_LEFT:
        if (cursorPosition > text.Length())
            cursorPosition = text.Length();
        if ( cursorPosition > 0 )
            cursorPosition--;
        if ( cursorPosition - start < 5 )
            start = (int)cursorPosition - 5;
        if ( start < 0 )
            start = 0;

        break;
    case CSKEY_RIGHT:
        if (cursorPosition > text.Length())
            cursorPosition = text.Length();
        if ( cursorPosition < text.Length() )
        {
            cursorPosition++;
        }
        break;
    case CSKEY_END:
        cursorPosition=1024;
        break;
    case CSKEY_HOME:
        cursorPosition=0;
        start=0;
        break;
    default:
        if ( !isprint((unsigned char)key))
        {
            break;
        }            

        if ( key == CSKEY_ALT || key == CSKEY_CTRL )
        {
            break;
        }
         
        if ( cursorPosition >= text.Length() )
        {
            text.Append( (char)key );
        }
        else
        {
            text.Insert( cursorPosition, (char)key );
        }
        cursorPosition++;

        changed = true;
    }
    
    if (changed)
    {
        if (subscribedVar)
            PawsManager::GetSingleton().Publish(subscribedVar, text);
        parent->OnChange(this);
    }

    if (!isprint((unsigned char)key))
        pawsWidget::OnKeyDown( code, key, modifiers );

    return true;
}

void pawsEditTextBox::Clear()
{
    start = 0;
    cursorPosition = 0;

    text.Clear();
}

bool pawsEditTextBox::OnMouseDown( int button, int modifiers, int x, int y )
{
    x -= (screenFrame.xmin+margin+4); // Adjust x to be relative to the text box
    int textWidth; 
    int boxWidth = (screenFrame.xmax + margin) - (screenFrame.xmin + margin);
    int dummy;

    //Force the cursor to blink
    blink = true;

    // Basic comparisons to see if it was clicked after all the text or before all the text or neither
    if (x <= 0 || text.Length() == 0)
    {
        start=0;
        cursorPosition=0;
        return true;
    }

    GetFont()->GetDimensions(text.GetData(),textWidth,dummy);
    if (x >= textWidth)
    {
    	cursorPosition = text.Length();
    	return true;
    }
    
    if(textWidth > boxWidth)
    {
    	x += textWidth - boxWidth;
    }

    // Find exact letter mouse was clicked on
    int xlast;
    int xlast2=0;
    const char *c;
    char temp[2];
    temp[1]='\0';
    c=temp;

    for (unsigned int i=0;i<text.Length();i++)
    {
        xlast=xlast2;
        temp[0]=text.GetAt(i);
        GetFont()->GetDimensions(c, textWidth, dummy);
        xlast2+=textWidth;
        if (x >= xlast && x < xlast2)
        {
            cursorPosition=i;
            break;
        }
    }
    return pawsWidget::OnMouseDown( button, modifiers, x ,y);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
pawsMultiLineTextBox::pawsMultiLineTextBox()
{
    scrollBar = 0;
    startLine = 0;
    canDrawLines = 0;
    maxHeight = 0;
    maxWidth = 0;
}

pawsMultiLineTextBox::~pawsMultiLineTextBox()
{

}

bool pawsMultiLineTextBox::Setup( iDocumentNode* node )
{    
    csRef<iDocumentNode> textNode = node->GetNode( "text" );        
    
    if ( textNode )
    {        
        csRef<iDocumentAttribute> textAttribute = textNode->GetAttribute("string");
        if ( textAttribute )
        {            
            SetText( textAttribute->GetValue() );
        }
    }        

    return true;
}

void pawsMultiLineTextBox::Resize()
{
    pawsWidget::Resize();
    SetText( text );
}

void pawsMultiLineTextBox::OrganizeText( const char* newText )
{       
    csString text(newText);

    // Check if we end with \n
    // Chomp if we do
    if (text.Length() > 0)
    {
        if (text.GetAt(text.Length()-1) == '\n')
        {
            text = text.Slice(0,text.Length()-1);
        }

    }
    
    startLine = 0;

    GetFont()->GetMaxSize( maxWidth, maxHeight );

    canDrawLines = (screenFrame.Height()-(margin*2)) / maxHeight;

    char* dummy = new char[text.Length() + 1];
    char* head = dummy;

    if (text.Length () > 0)
        strcpy( dummy, text );
    else
        dummy[0] = 0;

    int offSet = margin*2;
    if ( usingScrollBar ) 
        offSet += 36;


    while ( dummy )
    {
        /// See how many characters can be drawn on a single line.                
        int canDrawLength =  GetFont()->GetLength( dummy, screenFrame.Width()-offSet );

        // Check for linebreaks
        const char* pos=strchr(dummy,'\n');
        csString loopStr(dummy);
        if (pos)
        {
            while(pos)
            {
                csString temp(loopStr.GetData());
                csString temp2(loopStr.GetData());

                size_t pos1=temp.FindFirst('\n');

                temp= temp.Slice(0,pos1);
                temp2= temp2.Slice(pos1+1,(temp2.Length() - pos1-1));

                OrganizeText(temp.GetData());

                if (temp2.Length())
                {
                    loopStr = temp2;
                    pos=strchr(loopStr.GetData(),'\n');
                    if (!pos)
                    {
                        OrganizeText(temp2.GetData());
                    }
                }  
                else
                    break;
            }
            break;
        }

        /// If it can fit the entire string then return.
        if ( canDrawLength == (int)strlen( dummy ) )
        {
            lines.Push( dummy );
            break;
        }
        // We have to push in a new line to the lines bit.
        else
        {
            // Find out the nearest white space to break the line. 
            int index = canDrawLength;

            while ( index > 0 && dummy[index] != ' ' )
            {
                index--;
            }

            if (index == 0)
                index = canDrawLength;

            // Index now points to the whitespace line so we can break it here.

            csString test;
            test.Append( dummy, index+1 );
            dummy+=index+1;        
            lines.Push( test );            
        }
    }

    delete [] head;    
}


void pawsMultiLineTextBox::SetText( const char* newText )
{
    lines.Empty();
    
    psString str(newText);      
    size_t pos = str.FindSubString("\r");
    while(pos != (size_t)-1)
    {            
        str = str.Slice(0,pos) + "\n" + str.Slice(pos+2,str.Length()-pos-2);
        pos = str.FindSubString("\r");            
    }
            
    
    usingScrollBar = false;
    if ( scrollBar ) scrollBar->Hide();
    
    
    OrganizeText( str.GetData() );
                
    if ( canDrawLines >= lines.GetSize() )
    {
        canDrawLines = lines.GetSize();
        if(scrollBar) //if there is a scrollbar we must setup it correctly else we will scroll in the void
        {
            scrollBar->SetMaxValue(0);
            scrollBar->SetCurrentValue(0);
        }
    }
    else
    {
        usingScrollBar = true;
        lines.Empty();    
        OrganizeText( str.GetData() );        
        if ( !scrollBar )
        {
            // Create the optional scroll bar here as well but hidden.
            scrollBar = new pawsScrollBar();
            scrollBar->SetParent( this );
            scrollBar->SetRelativeFrame( defaultFrame.Width() - 40, 6, 24, defaultFrame.Height() - 12 ); 
            scrollBar->SetAttachFlags(ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT);
            scrollBar->PostSetup();
            scrollBar->SetTickValue( 1.0 );
            AddChild( scrollBar );
            pawsWidget::Resize();
        }
        
        scrollBar->Show();
        scrollBar->SetMaxValue(lines.GetSize() - canDrawLines );
        scrollBar->SetCurrentValue(0);
    }    
    startLine = 0;    
    text.Replace( str.GetData() );    
}

void pawsMultiLineTextBox::OnUpdateData(const char *dataname,PAWSData& value)
{
    // This is called automatically whenever subscribed data is published.
    SetText( value.GetStr() );
}


void pawsMultiLineTextBox::Draw()
{
    pawsWidget::Draw();
    pawsWidget::ClipToParent(false);
    
    int drawX = screenFrame.xmin+margin;
    int drawY = screenFrame.ymin+margin;
    
    if (!maxHeight && GetFont())
        GetFont()->GetMaxSize( maxWidth, maxHeight );

    if (!canDrawLines && maxHeight)
        canDrawLines = (screenFrame.Height()-(margin*2)) / maxHeight;

    for (size_t x = startLine; x < (startLine+canDrawLines); x++ )
    {
        if ( x >= lines.GetSize() )
            return;

        DrawWidgetText( (const char*)lines[x], drawX, drawY);

        drawY+=maxHeight;
    }

}

bool pawsMultiLineTextBox::OnScroll( int direction, pawsScrollBar* widget )
{
    startLine = (int)widget->GetCurrentValue();       
    return true;
}

bool pawsMultiLineTextBox::OnMouseDown( int button, int modifiers, int x, int y )
{
    if (button == csmbWheelUp)
    {
        if (scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() - MULTILINE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if (button == csmbWheelDown)
    {
        if (scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() + MULTILINE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    return pawsWidget::OnMouseDown( button, modifiers, x ,y);
}

pawsFadingTextBox::pawsFadingTextBox()
{
    start = 0;
    color = scolor = 0;
    ymod = 0;
    firstFont = NULL;
    font = NULL;
}

void pawsFadingTextBox::SetText(const char* newtext, iFont* font1,iFont* font2, int color,int time,int fadetime)
{
    // Seperate the first and the rest
    text = newtext;
    first = "XXX";

    first.SetAt(0,text.GetAt(0));
    first.SetAt(1,0);
    text = text.Slice(1,text.Length() -1);
    
    // Add the other stuff
    firstFont = font1;
    font = font2;
    start = csGetTicks(); //Start the effect

    this->org_color = color;
    this->time = time;
    this->fadetime = fadetime;

}

void pawsFadingTextBox::GetSize(int &w, int &h)
{
    int iw=0,ih=0;
    int dummy=0;
    // Get the sizes
    firstFont->GetDimensions(first,iw,ih,dummy);
    font->GetDimensions(text,w,h,dummy);

    if(iw > w) // Stretch the size to have space for the first letter
        w = iw;
    if(ih > h)
        h = ih;

    w += BORDER_SIZE*2; // Add the border pixels
    h += BORDER_SIZE*2; // Add the border pixels
}

void pawsFadingTextBox::Draw()
{
    if(!text || text.Length() == 0)
        return;

    // Calculate fading
    int alpha = 0;
    if(csGetTicks() > start + time)
    {
        int elapsed = csGetTicks() - (start + time);

        float multi = float(elapsed) / float(fadetime);
        alpha = int(multi * float(255));
    }

    int r=0,g=0,b=0,a=0;
    graphics2D->GetRGB(org_color,r,g,b,a);
    a -= alpha; // Apply alpha

    if(a < 1)
    {
        // Destroy the widget
        DeleteYourself();
        return;
    }

    int newymod = alpha / 2;

    MoveDelta(0,newymod - ymod); // Move downwards
    ymod = newymod;

    // Prepare to paint the text
    color  = graphics2D->FindRGB(r,g,b,a); // Color on text
    scolor = graphics2D->FindRGB(0,0,0,a); // Color on "shadow"

    int iw=0; // Width on the first letter
    int dummy=-1; // Used as return value for 'desc' and the height

    firstFont->GetDimensions(first,iw,dummy,dummy);

    // Draw
    //ClipToParent();

    DrawBorderText(first,firstFont,0);
    DrawBorderText(text,font,iw);
}

void pawsFadingTextBox::Fade()
{
    if(start < csGetTicks() - time) // Already fading?
        return;

    start = csGetTicks() - time;
}

void pawsFadingTextBox::DrawBorderText(const char* text, iFont* font,int x)
{
    int y = 0;
    x += screenFrame.xmin;
    y += screenFrame.ymin;

    graphics2D->Write(font,x+BORDER_SIZE,y+BORDER_SIZE,scolor,-1,text); // Right down
    graphics2D->Write(font,x-BORDER_SIZE,y+BORDER_SIZE,scolor,-1,text); // Left down
    graphics2D->Write(font,x+BORDER_SIZE,y-BORDER_SIZE,scolor,-1,text); // Right up
    graphics2D->Write(font,x-BORDER_SIZE,y-BORDER_SIZE,scolor,-1,text); // Left up

    graphics2D->Write(font,x,y+BORDER_SIZE,scolor,-1,text); // Down
    graphics2D->Write(font,x,y-BORDER_SIZE,scolor,-1,text); // Up
    graphics2D->Write(font,x+BORDER_SIZE,y,scolor,-1,text); // Right
    graphics2D->Write(font,x-BORDER_SIZE,y,scolor,-1,text); // Left

    // Draw the letter
    graphics2D->Write(font,x,y,color,-1,text);
}
