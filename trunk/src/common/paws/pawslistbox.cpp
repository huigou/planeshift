/*
 * pawslistbox.cpp - Author: Andrew Craig
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
#include <ivideo/fontserv.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iutil/virtclk.h>
#include <iutil/event.h>
#include "pawslistbox.h"
#include "pawsmanager.h"
#include "pawstextbox.h"
#include "pawscrollbar.h"


listBoxSortingFunc pawsListBox::sort_sortFunc;
int pawsListBox::sort_sortColNum;
bool pawsListBox::sort_ascOrder;


/**
 * This widget is used to make titles of columns. It embedds real GUI widget like pawsTextBox.
 * It receives mouse clicks on this inner widget and changes sorting of listbox.
 */

class pawsListBoxTitle : public pawsWidget
{
public:
    pawsListBoxTitle(pawsListBox * listBox, int colNum, pawsWidget * innerWidget)
    {
        this->listBox = listBox;
        this->colNum  = colNum;

        AddChild(innerWidget);
    }

    bool OnMouseDown( int button, int modifiers, int x, int y )
    {
        if (button == csmbWheelUp || button == csmbWheelDown)
            return listBox->OnMouseDown( button, modifiers, x, y );
        else if (listBox->GetSortedColumn() == colNum)
            listBox->SetSortOrder( ! listBox->GetSortOrder() );
        else
            listBox->SetSortedColumn(colNum);

        return true;
    }

protected:

    pawsListBox * listBox;    // our mother listbox
    int colNum;               // the number of the column this widget is used in
};

//-----------------------------------------------------------------------------
//                            class pawsListBox
//-----------------------------------------------------------------------------

pawsListBox::pawsListBox()
{
    columnDef   = 0;
    totalColumns = 0;
    totalRows    = 0;
    selected     = -1;
    topRow       = 0;
    autoID       = false;
    notifyTarget = NULL;
    scrollBar    = NULL;
    horzscrollBar= NULL;
    titleRow     = NULL;
    xMod         = 0;

    usingTitleRow = true;

    sortColNum   = 0;
    ascOrder     = true;

    highlightImage = "Highlight";
    highlightAlpha = 128;

    arrowSize       = 16;
    arrowUp         = "Up Arrow";
    arrowDown       = "Down Arrow";
    scrollbarWidth  = 15;
    scrollbarHeightMod = 0;
    useBorder       = false;
    autoResize      = true;
    autoUpdateScroll= true;
    selectable      = true;
}


pawsListBox::~pawsListBox()
{
    delete[] columnDef;
}


void pawsListBox::Clear()
{
    for ( size_t x = 0; x < rows.GetSize(); x++ )
    {
        children.Delete( rows[x] );
        delete rows[x];
    }

    rows.DeleteAll();
    totalRows = 0;
    if (scrollBar) 
        scrollBar->Hide();
    if (horzscrollBar)
        horzscrollBar->Hide();
}


bool pawsListBox::Setup( iDocumentNode* node )
{
    csString sortOrder;

    csRef<iDocumentNode> columnsNode = node->GetNode( "columns" );

    columnHeight = columnsNode->GetAttributeValueAsInt( "height" );
    xmlbinding_row = columnsNode->GetAttributeValue("xmlbinding");

    const char *headings = columnsNode->GetAttributeValue("headings");
    if (!headings || !strcmp(headings,"no"))
        this->UseTitleRow(false);

    const char *aid = node->GetAttributeValue("autoid");
    autoID = (aid && !strcmp(aid,"yes"));

    // Get the definition of all the column types
    csRef<iDocumentNodeIterator> colIter = columnsNode->GetNodes( "column" );

    csArray<ColumnDef> colInfo;

    rowWidth = 0;
    while ( colIter->HasNext() )
    {
        csRef<iDocumentNode> colNode = colIter->Next();

        ColumnDef cInfo;

        cInfo.width      = GetActualWidth(colNode->GetAttributeValueAsInt("width"));
        cInfo.height     = GetActualHeight(columnHeight);
        cInfo.xmlbinding = colNode->GetAttributeValue("xmlbinding");
        
        cInfo.widgetNode = colNode->GetNode("widget");
        if (!cInfo.widgetNode)
        {
            csRef<iDocumentNodeIterator> it = colNode->GetNodes();
            while (it->HasNext())
            {
                csRef<iDocumentNode> n = it->Next();
                if (strncmp(n->GetValue(),"paws",4)==0) // must start with "paws"
                {
                    cInfo.widgetNode = n;
                    break;
                }
            }
        }
        CS_ASSERT_MSG("Did not find widget or paws node in column definition.",cInfo.widgetNode != NULL);

        rowWidth+= cInfo.width;

        colInfo.Push(cInfo);
    }

    // Copy from dynamic array into static size array.
    totalColumns = (int)colInfo.GetSize();
    if (columnDef)
    {
        delete [] columnDef;
    }
    columnDef = new ColumnDef[totalColumns];
    for (int i = 0; i < totalColumns; i++)
	{
        columnDef[i] = colInfo[i];
    }

    if (usingTitleRow)
        CreateTitleRow();

    SetSortedColumn(node->GetAttributeValueAsInt("sortBy"));
    SetSortingFunc(sortColNum, &textBoxSortFunc);
    csString selectableAttr = node->GetAttributeValue("selectable");
    selectable = selectableAttr != "0";

    sortOrder = node->GetAttributeValue("sortOrder");
    if (sortOrder.GetData() != NULL)
    {
        if (sortOrder == "desc")
            SetSortOrder(false);
        else
            SetSortOrder(true);
    }

    csRef<iDocumentNode> highlightImageNode = node->GetNode( "highlight" );
    if ( highlightImageNode )
    {
        csRef<iDocumentAttribute> attr;

        attr = highlightImageNode->GetAttribute("resource");

        if (attr)
            highlightImage = attr->GetValue();

        attr = highlightImageNode->GetAttribute("alpha");

        if ( attr )
            highlightAlpha = attr->GetValueAsInt();
    }

    csRef<iDocumentNode> scrollbarNode = node->GetNode( "scrollbar" );
    if ( scrollbarNode )
    {
        csRef<iDocumentAttribute> attr;

        attr = scrollbarNode->GetAttribute("arrowup");
        if (attr)
            arrowUp = attr->GetValue();

        attr = scrollbarNode->GetAttribute("arrowdown");
        if (attr)
            arrowDown = attr->GetValue();

        attr = scrollbarNode->GetAttribute("arrowsize");
        if (attr)
            arrowSize = attr->GetValueAsInt();

        attr = scrollbarNode->GetAttribute("width");
        if (attr)
            scrollbarWidth = attr->GetValueAsInt();

        attr = scrollbarNode->GetAttribute("heightmod");
        if (attr)
            scrollbarHeightMod = attr->GetValueAsInt();

        attr = scrollbarNode->GetAttribute("border");
        if (attr)
            useBorder = attr->GetValueAsBool();

        attr = scrollbarNode->GetAttribute("autoresize");
        if (attr)
            autoResize = attr->GetValueAsBool();
        else
            autoResize = true;
    }

    return true;
}


bool pawsListBox::PostSetup()
{

    scrollBar = new pawsScrollBar();
    scrollBar->SetParent( this );
    scrollBar->SetRelativeFrame( defaultFrame.Width() - GetActualWidth(scrollbarWidth),
                                 titleRow ? titleRow->DefaultFrame().ymax : 6,
                                 GetActualWidth(scrollbarWidth),
                                 defaultFrame.Height() - 6 - (titleRow ? titleRow->DefaultFrame().Height() : 6) -GetActualHeight(scrollbarHeightMod) );

    int attach = ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT;
    scrollBar->SetAttachFlags( attach );
    if (useBorder)
        scrollBar->UseBorder();
    scrollBar->PostSetup();
    scrollBar->SetTickValue( 1.0 );
    scrollBar->SetAlwaysOnTop(true);
    scrollBar->Hide();
    AddChild( scrollBar );

    // Add horizontal scollie
    horzscrollBar = new pawsScrollBar();
    horzscrollBar->SetParent( this );
    horzscrollBar->SetHorizontal(true);

    horzscrollBar->SetRelativeFrame(
        0, // Width to stand on -
        defaultFrame.Height() - GetActualWidth(scrollbarWidth), // Height to stand on |
        defaultFrame.Width() - 12, // Width
        GetActualWidth(scrollbarWidth) // Height
        );

    attach = ATTACH_BOTTOM | ATTACH_LEFT | ATTACH_RIGHT;
    horzscrollBar->SetAttachFlags( attach );
    if (useBorder)
        horzscrollBar->UseBorder();

    horzscrollBar->PostSetup();
    horzscrollBar->SetTickValue( 1.0 );
    horzscrollBar->SetAlwaysOnTop(true);
    horzscrollBar->Hide();
    AddChild( horzscrollBar );

    return true;
}


void pawsListBox::CreateTitleRow()
{
    titleRow = new pawsListBoxRow();
    titleRow->SetParent( this );

    titleRow->SetRelativeFrame( 0, 0,
                                ScreenFrame().Width() ,  GetActualHeight(columnHeight) );

    for ( int x = 0; x < totalColumns; x++ )
    {
        titleRow->AddTitleColumn( x, columnDef );
    }

    AddChild( titleRow );
}


pawsListBoxRow* pawsListBox::GetSelectedRow()
{
    if ( selected == -1 )
        return NULL;
    else
        return rows[selected];
}

int pawsListBox::GetSelectedRowNum()
{
    return selected;
}

pawsListBoxRow* pawsListBox::RemoveSelected()
{
    if ( selected == -1 )
        return NULL;
    else
    {
        pawsListBoxRow* zombie = rows[selected];
        Remove(zombie);
        selected = -1;
        return zombie;
    }
}


void pawsListBox::Remove( int id )
{
    for ( size_t x = 0; x < rows.GetSize(); x++ )
    {
        if ( rows[x]->GetID() == id )
        {
            pawsListBoxRow* zombie = rows[x];
            Remove(zombie);
            return;
        }
    }
}


void pawsListBox::Remove (pawsListBoxRow* rowToRemove)
{
    if (rowToRemove)
    {
        rows.Delete( rowToRemove );
        children.Delete( rowToRemove );
        totalRows--;
        CalculateDrawPositions();
    }
}


void pawsListBox::AddRow( pawsListBoxRow* row )
{
    // Can only add a valid row.
    if ( !row )
        return;


    row->SetParent( this );
    AddChild( row );

    row->SetRelativeFrame( 0 ,(totalRows+1)*GetActualHeight(columnHeight), rowWidth, GetActualHeight(columnHeight) );

    totalRows++;

    rows.Push( row );

    if ( scrollBar )
    {
        if(autoUpdateScroll)
            SetScrollBarMaxValue();

        scrollBar->SetCurrentValue( (float)topRow );
    }

    CalculateDrawPositions();
    row->SetBackground("Standard Background");
}

void pawsListBox::Resize()
{
    pawsWidget::Resize();
    if ( scrollBar )
    {
        scrollBar->Resize(); //TODO: why i need to call this manually?
        if(autoUpdateScroll)
            SetScrollBarMaxValue();

        scrollBar->SetCurrentValue( (float)topRow );
    }
    CalculateDrawPositions();
}

pawsListBoxRow* pawsListBox::NewRow(size_t position )
{
    pawsListBoxRow* newRow = new pawsListBoxRow();

    newRow->SetParent( this );
	// Here we automatically resize the row to be the width of the parent listbox, so the row can reference that when adding columns
    newRow->SetRelativeFrame( 0 ,(totalRows+1)*GetActualHeight(columnHeight), parent->GetActualWidth(), GetActualHeight(columnHeight));//GetActualHeight(columnHeight) );

    for ( int x = 0; x < totalColumns; x++ )
    {
        newRow->AddColumn( x, columnDef );
    }


    if ( position != (size_t)-1 )
    {
        children.Insert( position, newRow );
        rows.Insert( position, newRow );
    }
    else
    {
        AddChild( newRow );
        rows.Push( newRow );
    }

    totalRows++;

    if (autoID)
    {
        for ( int x = 0; x < totalColumns; x++ )
        {
            pawsWidget * widget = newRow->GetColumn( x );
            widget->SetID(id+totalRows*totalColumns+x);
        }
    }



    if ( scrollBar )
    {
        if(autoUpdateScroll)
            SetScrollBarMaxValue();

        scrollBar->SetCurrentValue( (float)topRow );
    }

    CalculateDrawPositions();
    newRow->SetBackground("Standard Background");
    newRow->SetBackgroundAlpha(255);
    
    return newRow;
}
pawsListBoxRow* pawsListBox::NewTextBoxRow( csList<csString> &rowEntry,size_t position )
{
    pawsListBoxRow* newRow = new pawsListBoxRow();

    newRow->SetParent( this );
	// Here we automatically resize the row to be the width of the parent listbox, so the row can reference that when adding columns
    newRow->SetRelativeFrame( 0 ,(totalRows+1)*GetActualHeight(columnHeight), parent->GetActualWidth(), GetActualHeight(columnHeight));//GetActualHeight(columnHeight) );

    for ( int x = 0; x < totalColumns; x++ )
    {
        newRow->AddColumn( x, columnDef );
        ((pawsTextBox*)newRow->GetColumn(x))->SetText(rowEntry.Front());
        rowEntry.PopFront();
    }


    if ( position != (size_t)-1 )
    {
        children.Insert( position, newRow );
        rows.Insert( position, newRow );
    }
    else
    {
        AddChild( newRow );
        rows.Push( newRow );
    }

    totalRows++;

    if (autoID)
    {
        for ( int x = 0; x < totalColumns; x++ )
        {
            pawsWidget * widget = newRow->GetColumn( x );
            widget->SetID(id+totalRows*totalColumns+x);
        }
    }



    if ( scrollBar )
    {
        if(autoUpdateScroll)
            SetScrollBarMaxValue();

        scrollBar->SetCurrentValue( (float)topRow );
    }

    CalculateDrawPositions();
    newRow->SetBackground("Standard Background");
    newRow->SetBackgroundAlpha(255);
    
    SortRows();
    return newRow;
}

void pawsListBox::SendOnListAction(int status)
{
    if (notifyTarget!=NULL)
        notifyTarget->OnListAction(this,status);
    else
        parent->OnListAction(this,status);
}


bool pawsListBox::Select( pawsListBoxRow* row, bool notify)
{
    if (!selectable)
        return false;

    selected = -1;
    // If no row assume no selection is wanted.
    if ( row == NULL )
    {
        for ( size_t x = 0; x < rows.GetSize(); x++ )
        {
            rows[x]->SetBackground("Standard Background");
            rows[x]->SetBackgroundAlpha(255);
        }
        return true;
    }

    for ( int x = 0; x < (int)rows.GetSize(); x++ )
    {
        if ( rows[x] == row )
        {
            selected = x;
            rows[x]->SetBackground(highlightImage);
            rows[x]->SetBackgroundAlpha(highlightAlpha);
        }
        else
        {
            rows[x]->SetBackground("Standard Background");
            rows[x]->SetBackgroundAlpha(255);
        }
    }

    if ( selected == -1 )
        return false;


    // Adjust the topRow of drawing if the selected row is not visible
    // -1 because of title row.
    int offset = 0;
    if ( usingTitleRow )
        offset = 1;
    int numberOfRows = screenFrame.Height() / columnDef[0].height - offset;

    if (selected<topRow)
        topRow=selected;
    else if (selected>=topRow+numberOfRows)
        topRow=(selected-numberOfRows)+1;

    CalculateDrawPositions();

    if ( scrollBar )
    {
       scrollBar->SetCurrentValue( (float)topRow );
    }

    if (notify)
        SendOnListAction(LISTBOX_HIGHLIGHTED);

    return true;
}

bool pawsListBox::SelectByIndex(int index, bool notify)
{
    if (!selectable)
        return false;

    selected = -1;
    if (rows.GetSize() <= index)
        return false;
    // If no row assume no selection is wanted.
    if ( index == -1 )
    {
        for ( size_t x = 0; x < rows.GetSize(); x++ )
        {
            rows[x]->SetBackground("Standard Background");
            rows[x]->SetBackgroundAlpha(255);
        }
        return true;
    }

    for ( int x = 0; x < (int)rows.GetSize(); x++ )
    {
        if ( x == index )
        {
            selected = x;
            rows[x]->SetBackground(highlightImage);
            rows[x]->SetBackgroundAlpha(highlightAlpha);
        }
        else
        {
            rows[x]->SetBackground("Standard Background");
            rows[x]->SetBackgroundAlpha(255);
        }
    }

    if ( selected == -1 )
        return false;


    // Adjust the topRow of drawing if the selected row is not visible
    // -1 because of title row.
    int offset = 0;
    if ( usingTitleRow )
        offset = 1;
    int numberOfRows = screenFrame.Height() / columnDef[0].height - offset;

    if (selected<topRow)
        topRow=selected;
    else if (selected>=topRow+numberOfRows)
        topRow=(selected-numberOfRows)+1;

    CalculateDrawPositions();

    if ( scrollBar )
    {
       scrollBar->SetCurrentValue( (float)topRow );
    }

    if (notify)
        SendOnListAction(LISTBOX_HIGHLIGHTED);

    return true;
}
void pawsListBox::SetScrollBarMaxValue()
{
    int horiz = 0;

    // If horz scroll bar is visible add an extra vertical scroll
    if ( horzscrollBar->IsVisible() )
    {
        horiz = 1;
    }
    scrollBar->SetMaxValue(
                            int(rows.GetSize()) + (usingTitleRow?1:0) + horiz +
                                -
                            screenFrame.Height() / columnDef[0].height
                          );

    // Find the longest row
    int longest = 0;
    for(size_t i = 0; i < rows.GetSize(); i++)
    {
        int rowWidth=0;
        for(size_t z = 0; z < rows[i]->GetTotalColumns();z++)
        {
            pawsTextBox* wdg = dynamic_cast <pawsTextBox*>(rows[i]->GetColumn(z));
            if(!wdg)
                continue;

            if(!wdg->IsVisible())
                continue;

            csString text = wdg->GetText();
            if(!text)
                continue;

            int width = 0;
            int height = 0; // Can be used for anything?

            wdg->GetFont()->GetDimensions(text,width,height);
            if (rowWidth+width > rows[i]->ScreenFrame().Width())
                width = rows[i]->ScreenFrame().Width() - rowWidth;

            rowWidth += width;

            if( autoResize && width > wdg->ScreenFrame().Width() )
            {
                wdg->SetRelativeFrameSize(width,wdg->ScreenFrame().Height());
            }
        }

        if(autoResize && rowWidth > rows[i]->ScreenFrame().Width())
        {
            rows[i]->SetRelativeFrameSize(rowWidth,rows[i]->ScreenFrame().Height());
        }

        // Compare
        if( rowWidth > longest )
            longest = rowWidth ;
    }

    // If everything has space, don't show scrollbar
    if(longest - GetActualWidth(rowWidth) <= 0)
    {
        horzscrollBar->Hide();
    }
    else
    {
        horzscrollBar->Show();
        horzscrollBar->SetMaxValue(longest - GetActualWidth(rowWidth) + scrollbarWidth);
    }
}

void pawsListBox::CalculateDrawPositions()
{
    // -1 because of title row.
    int offset = 0;
    if ( usingTitleRow )
        offset = 1;

     // Clamp toprow to existing rows
     if (topRow<0)
         topRow=0;
     if (topRow>totalRows)
         topRow=totalRows;


    size_t numberOfRows = screenFrame.Height() / columnDef[0].height - offset;

    size_t row = topRow;

    //Hide all rows till the one which will be drawn
    for ( size_t x = 0; x < row; x++ )
        rows[x]->Hide();

    //figure out which ones to draw and position them
    for ( size_t z = 0; z < numberOfRows; z++ )
    {
        if ( row < rows.GetSize() )
        {
            // +1 because of title row.
            int positionY = ((int)z+offset) * columnDef[0].height;
            int positionX = 0 + xMod;
            if (GetBorder())
            {
                positionY+=5;
                positionX+=5;
            }
            positionY+=margin;
            positionX+=margin;

            rows[row]->Show();
            rows[row]->SetRelativeFramePos( positionX, positionY );
            row++;
        }
        else
            break;
    }

    //hide the last rows we don't see
    for (size_t x = row; x < rows.GetSize(); x++ )
        rows[x]->Hide();

    if (scrollBar)
    {
        if (rows.GetSize() > numberOfRows)
            scrollBar->Show();
        else
            scrollBar->Hide();
    }
}


bool pawsListBox::OnScroll( int direction, pawsScrollBar* widget )
{
    if(widget == scrollBar)
    {
        topRow = (int) widget->GetCurrentValue();
        CalculateDrawPositions();
    }
    else if(widget == horzscrollBar)
    {
        xMod = -(int) widget->GetCurrentValue();
        CalculateDrawPositions();
    }

    return true;
}

bool pawsListBox::OnMouseDown(int button, int modifiers, int x, int y )
{
    if (button == csmbWheelUp)
    {
        if (scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() - LISTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if (button == csmbWheelDown)
    {
        if (scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() + LISTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if (!selectable)
    {
        return pawsWidget::OnMouseDown(button, modifiers, x, y);
    }

    return true;
}


pawsListBoxRow* pawsListBox::GetRow( size_t x )
{
    if ( x != (size_t)-1 && x < rows.GetSize() )
        return rows[x];
    else
        return 0;
}


void pawsListBox::UseTitleRow( bool yes )
{
    usingTitleRow = yes;
}


void pawsListBox::SetTotalColumns( int numCols )
{
    if ( columnDef )
        delete [] columnDef;

    columnDef = new ColumnDef[ numCols ];
    totalColumns = numCols;

    rowWidth = 0;
    columnHeight = 0;
}


void pawsListBox::SetColumnDef( int col, int width, int height, const char* widgetDesc )
{
    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

    csRef<iDocument> doc= xml->CreateDocument();
    doc->Parse( widgetDesc );

    columnDef[col].width = width;
    columnDef[col].height = GetActualHeight(height);
    columnDef[col].widgetNode = doc->GetRoot()->GetNode("widget");
    if(!columnDef[col].widgetNode)
    {
        Error1("Missing <widget> tag!");
        return;
    }
    rowWidth+=width;
    columnHeight = height;
}


bool pawsListBox::SelfPopulate( iDocumentNode *topNode )
{
    float oldScroll = scrollBar->GetCurrentValue();

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    int count = 0;
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        if ( strcmp( node->GetValue(), xmlbinding_row ) == 0 )
        {
            pawsListBoxRow *row = NewRow(count++);
            if (node->GetAttributeValueAsBool("heading") == true)
            {
                row->SetHeading(true);
            }
            int x;
            for (x=0; x<totalColumns; x++)
            {
                if (columnDef[x].xmlbinding.Length() > 0)
                {
                    pawsWidget *columnwidget = row->GetColumn(x);
                    csRef<iDocumentNode> column = node->GetNode(columnDef[x].xmlbinding);
                    if (!column)
                    {
                        Error3("Could not find xmlbinding '%s' in xml supplied to listbox '%s'.",
                               columnDef[x].xmlbinding.GetData(), name.GetData() );
                        return false;
                    }

                    // Set the XML binding so that a client can query the
                    // widget about how it was set if using SelfPopulate.
                    columnwidget->SetXMLBinding(columnDef[x].xmlbinding);

                    if (!columnwidget->SelfPopulate(column))
                    {
                        Error3("Widget %s in row %d could not self populate from xml node.\n",columnwidget->GetName(),count);
                        return false;
                    }
                }
            }
        }
    }

    scrollBar->SetCurrentValue(MIN(oldScroll, scrollBar->GetMaxValue()));
    return true;
}


bool pawsListBox::ConvertFromAutoID(int id, int &row, int &col)
{
    if (id >= this->id && id < this->id+totalColumns*(totalRows+1))
    {
        col = (id-this->id)%totalColumns;
        row = (id-this->id-col)/totalColumns - 1; // -1 is for the title row
        return true;
    }
    return false;
}


void pawsListBox::MoveSelectBar(bool direction)
{
    // move selectbar up or down
    if (direction)
    {
        if (selected>0)
        {
            selected--;
            if (GetSelectedRow()->IsHeading())
            {
                if (selected > 0)
                    selected--;
                else
                    selected++; // go back where we were if nothing is above the heading
            }

            Select(GetSelectedRow());
        }
    }
    else
    {
        if (selected<totalRows-1)
        {
            selected++;
            if (GetSelectedRow()->IsHeading())
            {
                if (selected < totalRows-1)
                    selected++;
                else
                    selected--; // go back where we were if nothing is above the heading
            }
            Select(GetSelectedRow());
        }
    }

    // -1 because of title row.
    int offset = 0;
    if ( usingTitleRow )
        offset = 1;

    int numberOfRows = screenFrame.Height() / columnDef[0].height - offset;


     // Adjust the topRow of drawing if the selected row is not visible
     if (selected<topRow)
         topRow=selected;
     else if (selected>=topRow+numberOfRows)
         topRow=(selected-numberOfRows)+1;

     CalculateDrawPositions();

}


bool pawsListBox::OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers)
{
    switch (keyChar)
    {
        case CSKEY_UP:
            MoveSelectBar(true);
            return true;
        break;
        case CSKEY_DOWN:
            MoveSelectBar(false);
            return true;
        break;
        case CSKEY_ENTER:
            SendOnListAction(LISTBOX_SELECTED);
            return true;
        break;
    }

    return pawsWidget::OnKeyDown(keyCode,keyChar,modifiers);
}

int pawsListBox::GetSortedColumn()
{
    return sortColNum;
}

void pawsListBox::SetSortedColumn(int colNum)
{
    if (colNum < -1  ||  colNum >= totalColumns)
        return;

    if (columnDef[colNum].sortFunc == NULL)
        return;

    if (sortColNum != -1)
        DeleteSortingArrow(sortColNum);

    sortColNum = colNum;
    ascOrder   = true;

    CreateSortingArrow(sortColNum);
    SetSortingArrow(sortColNum, ascOrder);
    SortRows();
}

bool pawsListBox::GetSortOrder()
{
    return ascOrder;
}

void pawsListBox::SetSortOrder(bool ascOrder)
{
    if (sortColNum == -1)
        return;

    this->ascOrder = ascOrder;
    SetSortingArrow(sortColNum, ascOrder);
    SortRows();
}

void pawsListBox::SetSortingFunc(int colNum, listBoxSortingFunc sortFunc)
{
    if (colNum < 0  ||  colNum >= totalColumns)
        return;

    columnDef[colNum].sortFunc = sortFunc;
    SortRows();
}

void pawsListBox::CreateSortingArrow(int colNum)
{
    pawsWidget * title = GetColumnTitle(colNum);
    if (title == NULL)
        return;
        
    pawsWidget * arrow = new pawsWidget();
    title->AddChild(arrow);
    arrow->SetRelativeFrame(title->ScreenFrame().Width()-arrowSize, 0, arrowSize, arrowSize);
    arrow->SetName("SortingArrow");
}

void pawsListBox::SetSortingArrow(int colNum, bool ascOrder)
{
    pawsWidget * title = GetColumnTitle(colNum);
    if (title == NULL)
        return;

    pawsWidget * arrow = title->FindWidget("SortingArrow");
    if (arrow != NULL)
    {
        if (ascOrder)
            arrow->SetBackground(arrowUp);
        else
            arrow->SetBackground(arrowDown);
    }
}

void pawsListBox::CheckSortingArrow(int colNum, bool ascOrder)
{
    pawsWidget * title = GetColumnTitle(colNum);
    if (title == NULL)
        return;

    pawsWidget * arrow = title->FindWidget("SortingArrow");
    if(arrow == NULL)
    {
        CreateSortingArrow(colNum);
        SetSortingArrow(colNum, ascOrder);
    }
}

void pawsListBox::DeleteSortingArrow(int colNum)
{
    pawsWidget * title = GetColumnTitle(colNum);
    if (title == NULL)
        return;

    pawsWidget * arrow = title->FindWidget("SortingArrow");
    if (arrow != NULL)
        title->DeleteChild(arrow);
}

pawsWidget * pawsListBox::GetColumnTitle(int colNum)
{
    if (titleRow == NULL)
        return NULL;

    return titleRow->FindWidget(colNum);
}

int pawsListBox::sort_cmp(const void * void_rowA, const void * void_rowB)
{
    pawsListBoxRow * rowA, * rowB;
    pawsWidget * cellA, * cellB;

    rowA  = *((pawsListBoxRow**) void_rowA);
    rowB  = *((pawsListBoxRow**) void_rowB);

    cellA = rowA -> GetColumn(sort_sortColNum);
    cellB = rowB -> GetColumn(sort_sortColNum);
    return sort_sortFunc(cellA, cellB) * (sort_ascOrder ? 1 : -1);
}

void pawsListBox::SortRows()
{
    pawsListBoxRow ** sortedRows;
    pawsListBoxRow * selectedrow = NULL; //stores the currently selected row before sorting
                                         //in order to be able to retrieve the correct position
    size_t i;

    if (sortColNum == -1)
        return;
    if (columnDef[sortColNum].sortFunc == NULL)
        return;

    sortedRows = new pawsListBoxRow*[rows.GetSize()];
    for ( i=0; i < rows.GetSize(); i++)
        sortedRows[i] = rows[i];

    if(selected != -1)
        selectedrow = sortedRows[selected]; //saves the corrispondence to the selected row

    sort_sortFunc    = columnDef[sortColNum].sortFunc;
    sort_sortColNum  = sortColNum;
    sort_ascOrder    = ascOrder;
    qsort(sortedRows, rows.GetSize(), sizeof (void*), sort_cmp);

    for ( i=0; i < rows.GetSize(); i++)
    {
        rows[i] = sortedRows[i];
        if(sortedRows[i] == selectedrow) //check if the current row is the one which was selected
            selected = (int) i; //if so update our selected variable
    }

    delete [] sortedRows;

    CheckSortingArrow(sortColNum, ascOrder); //check if the sorting arrow is there if not add it

    Select(GetSelectedRow()); //allows select to handle the new position and update gfx correctly

    CalculateDrawPositions();
}

/* Testcase for MoveRow (Place for example in pawsSkillWindow):

    printf("Moving 20 to 10...\n");
    skillList->MoveRow(20,10);
    printf("Moving 10 to 20...\n");
    skillList->MoveRow(10,20);
    printf("Moving 15 to 15...\n");
    skillList->MoveRow(15,15);
    printf("Clearing box..\n");
    skillList->Clear();
    printf("Testing moving with cleared box..\n");
    skillList->MoveRow(0,0);
    skillList->MoveRow(1,0);
    skillList->MoveRow(0,1);

    printf("Testing with 1 and 2 entites\n");
    skillList->NewRow();
    skillList->MoveRow(1,0);
    skillList->MoveRow(0,1);
    printf("2 entities\n");
    skillList->NewRow();
    skillList->MoveRow(2,1);
    skillList->MoveRow(1,2);
    skillList->MoveRow(2,0);
    skillList->MoveRow(2,2);

    printf("Selftest complete!\n");
*/
void pawsListBox::MoveRow(int rownr,int dest)
{
    pawsListBoxRow ** sortedRows;
    int i;

    if ((size_t)rownr >= rows.GetSize() || rownr < 0 || dest < 0 || (size_t)dest >= rows.GetSize() || dest == rownr )
        return;

    sortedRows = new pawsListBoxRow*[rows.GetSize()];


    bool reorder = false;
    for ( i=0;(uint)i < rows.GetSize(); i++)
    {
        if((i == dest && dest < rownr) || (i == rownr && dest > rownr))
            reorder = true;

        if((i == dest && dest > rownr) || (i == rownr && dest < rownr))
        {
            //printf("Row %d = %d\n",dest,rownr);
            sortedRows[dest] = rows[rownr];
            reorder = false;
            continue;
        }

        if(reorder)
        {
            if(dest < rownr)
            {
                //printf("Row %d = %d\n",i+1,i);
                sortedRows[i+1] = rows[i];
            }
            else
            {
                //printf("Row %d = %d\n",i,i+1);
                sortedRows[i] = rows[i+1];
            }
        }
        else
        {
            //printf("Row %d = %d\n",i,i);
            sortedRows[i] = rows[i];
        }
    }

    for ( i=0; (uint)i < rows.GetSize(); i++)
        rows[i] = sortedRows[i];
    delete [] sortedRows;
    CalculateDrawPositions();
}

const char *pawsListBox::GetSelectedText(size_t columnId)
{
	pawsListBoxRow *row = GetSelectedRow();
	if (!row)
		return NULL;

	pawsTextBox *box = (pawsTextBox *)GetSelectedRow()->GetColumn(columnId);
	if (!box)
		return NULL;

	return box->GetText();
}

//-----------------------------------------------------------------------------
//                            class pawsListBoxRow
//-----------------------------------------------------------------------------


pawsListBoxRow::pawsListBoxRow()
{
    isHeading = false;
}


bool pawsListBoxRow::OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers)
{
    return GetParent()->OnKeyDown(keyCode,keyChar,modifiers);
}


bool pawsListBoxRow::OnMouseDown( int button, int modifiers, int x, int y )
{
    // Heading rows are not clickable or selectable
    if (isHeading)
        return true;

    pawsListBox* parentBox = (pawsListBox*)parent;

    // mouse wheel
    if (button == csmbWheelUp || button == csmbWheelDown || !parentBox->IsSelectable())
    {
        return parentBox->OnMouseDown(button, modifiers, x, y);
    }
    return parentBox->Select( this );
}

bool pawsListBoxRow::OnDoubleClick(int button, int modifiers, int x, int y)
{
    // Heading rows are not clickable or selectable
    if (isHeading)
        return false;

    pawsListBox * parentBox = (pawsListBox *)parent;

    if (button != csmbWheelUp && button != csmbWheelDown)
    {
        parentBox->SendOnListAction(LISTBOX_SELECTED);
    }
    return true;
}

pawsWidget* pawsListBoxRow::GetColumn( size_t column )
{
    return columns[column];
}


void pawsListBoxRow::AddColumn( int column, ColumnDef* def )
{
    csString factory = def[column].widgetNode->GetAttributeValue("factory");
    if (factory.Length() == 0)
        factory = def[column].widgetNode->GetValue();

    pawsWidget* widget = PawsManager::GetSingleton().CreateWidget( factory );
    if (!widget)
    {
        Error3("Could not create column %d of listbox.  Factory specified was <%s>.\n",column,(const char *)factory);
        return;
    }

    widget->SetParent( this );
    widget->Load( def[column].widgetNode );

    int borderW = 0;
    int borderH = 0;
    if (parent->GetBorder())
    {
        borderW = 4;
        borderH = 4;
    }

    int offset = 0;
    for ( int x = 0; x < column; x++ )
        offset += children[x]->GetActualWidth();

	// Adjust this column width by the percentage growth of this column on the screen compared to original spec in file
	float w = this->screenFrame.Width();
	w /= GetLogicalWidth(screenFrame.Width());
	int myWidth = def[column].width * w;
    widget->SetRelativeFrame( offset+borderW+widget->DefaultFrame().xmin, borderH+widget->DefaultFrame().ymin,
                              GetActualWidth(myWidth), GetActualHeight(def[column].height) );

    columns.Push( widget );
    AddChild( widget );
}

void pawsListBoxRow::AddTitleColumn( int column, ColumnDef* def )
{
    // create the real title widget
    pawsTextBox *innerWidget = new pawsTextBox;

    innerWidget->SetName( def[column].widgetNode->GetAttributeValue("name") );
    innerWidget->SetText( def[column].widgetNode->GetAttributeValue("name") );

    csRef<iDocumentAttribute> atr = def[column].widgetNode->GetAttribute("visible");
    if (atr)
    {
        csString choice = csString( atr->GetValue() );
        if ( choice == "no"  )
        {
            innerWidget->Hide();
        }
    }

    // create the wrapper widget
    pawsListBoxTitle * title = new pawsListBoxTitle(dynamic_cast<pawsListBox*> (parent),
                                                    column, innerWidget);
    AddChild(title);

    int offset = 0;
    for ( int x = 0; x < column; x++ )
        offset+=GetActualWidth(def[x].width);

    title->SetRelativeFrame( offset-4, -4, GetActualWidth(def[column].width), GetActualHeight(def[column].height) );
    innerWidget->SetRelativeFrame(4, 4, GetActualWidth(def[column].width), GetActualHeight(def[column].height) );
    title->SetID(column);
}

void pawsListBoxRow::SetHeading(bool flag)
{
    isHeading = flag;

    for (size_t i=0; i<columns.GetSize(); i++)
    {
        columns[i]->SetFontStyle(flag ? FONT_STYLE_BOLD : DEFAULT_FONT_STYLE);
    }
}

size_t pawsListBox::GetRowCount()
{
    return rows.GetSize();
}

pawsTextBox * pawsListBox::GetTextCell(int rowNum, int colNum)
{
    pawsListBoxRow * row = GetRow(rowNum);
    if (row == NULL)
        return NULL;

    pawsWidget * cell = row->GetColumn(colNum);
    if (cell == NULL)
        return NULL;

    return dynamic_cast <pawsTextBox*> (cell);
}

csString pawsListBox::GetTextCellValue(int rowNum, int colNum)
{
    pawsTextBox * cell = GetTextCell(rowNum, colNum);
    if (cell != NULL)
        return cell->GetText();
    else
        return (const char*)NULL;
}

void pawsListBox::SetTextCellValue(int rowNum, int colNum, const csString & value)
{
    pawsTextBox * cell = GetTextCell(rowNum, colNum);
    if (cell != NULL)
        cell->SetText(value);
}





int textBoxSortFunc(pawsWidget * widgetA, pawsWidget * widgetB)
{
    pawsTextBox * textBoxA, * textBoxB;
    const char  * textA,    * textB;

    textBoxA = dynamic_cast <pawsTextBox*> (widgetA);
    textBoxB = dynamic_cast <pawsTextBox*> (widgetB);
    assert(textBoxA && textBoxB);
    textA = textBoxA->GetText();
    if (textA == NULL)
        textA = "";
    textB = textBoxB->GetText();
    if (textB == NULL)
        textB = "";
    return strcmp(textA, textB);
}
