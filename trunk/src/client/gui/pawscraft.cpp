 /*
 * pawscraft.cpp - Author: Andrew Craig <acraig@planeshift.it> 
 *
 * Copyright (C) 2003-2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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


#include "paws/pawstree.h"
#include "paws/pawstextbox.h"
#include "net/clientmsghandler.h"
#include "pawscraft.h"

  
bool pawsCraftWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CRAFT_INFO);
    filterEditTextBox = dynamic_cast<pawsEditTextBox*>(FindWidget("Filter"));
    if ( !filterEditTextBox ) {
        Error1( "pawsCraftWindow::PostSetup failed to load widget 'Filter'");
        return false;
    }
   
    textBox = dynamic_cast<pawsMultiLineTextBox*>(FindWidget("HelpText"));
    if ( !textBox ) {
        Error1( "pawsCraftWindow::PostSetup failed to load widget 'HelpText'");
        return false;
    }
   
    // by default we set no filter
    filter="";

    return true;
}

void pawsCraftWindow::Show()
{
    // Get craft info for mind item
    psMsgCraftingInfo msg;
    msg.SendMessage();

    pawsWidget::Show();        
}   


void pawsCraftWindow::HandleMessage( MsgEntry* me )
{
//    textBox = dynamic_cast<pawsMultiLineTextBox*>(FindWidget("HelpText"));
    psMsgCraftingInfo mesg(me);
//    csString text(mesg.craftInfo);
//    text.ReplaceAll( "[[", "   With higher ");
//    text.ReplaceAll( "]]", " skill you could: " );
//    if (text)
//        textBox->SetText(text.GetData());
    // lets copy the text of the crafting info message
    craftText = mesg.craftInfo;
    // the folowing replacement should probably go to pawsCraftWindow::Format() as well
    craftText.ReplaceAll( "[[", "With higher ");
    craftText.ReplaceAll( "]]", " skill you could: " );
    // and format it for the output (apply filter)
    Format();

}

/*
void pawsCraftWindow::Add( const char* parent, const char* realParent, psMsgCraftingInfo::CraftingItem* item )
{
    csString name(parent);
    name.Append("$");
    name.Append(item->name);
    
    itemTree->InsertChildL( parent, name, item->name, "" );
    TreeNode* node = new TreeNode;
    node->name = name;
    node->count = item->count;
    node->equipment = item->requiredEquipment;
    node->workItem = item->requiredWorkItem;
    nodes.Push( node );
            
    for ( size_t z = 0; z <  item->subItems.GetSize(); z++ )
    {
        Add( name, item->name, item->subItems[z] );
    }        
}
*/

bool pawsCraftWindow::OnSelected(pawsWidget* /*widget*/)
{
/*
    pawsTreeNode* node = static_cast<pawsTreeNode*> (widget);
    
    for ( size_t z = 0; z < nodes.GetSize(); z++ )
    {
        if ( nodes[z]->name == node->GetName() )
        {
            csString text("");
            csString dummy;
            if ( nodes[z]->count > 0 )
            {
                dummy.Format("Count: %d\n", nodes[z]->count );
                text.Append(dummy);            
            }
            
            if ( nodes[z]->equipment.Length() > 0 )
            {
                dummy.Format("Required Equipment: %s\n", nodes[z]->equipment.GetData() );
                text.Append(dummy);
            }
                            
            if ( nodes[z]->workItem.Length() > 0 )
            {            
                dummy.Format("Required Work Item: %s\n", nodes[z]->workItem.GetData() );                
                text.Append(dummy);
            }                
            textBox->SetText( text );
        }
    }
    return false;        
*/       
    return true;
}

void pawsCraftWindow::Draw()
{
    // did the filter change since the last draw?
    if (filter != filterEditTextBox->GetText())
    {
       // well I guess we have to do some reformating then
       filter = filterEditTextBox->GetText();
       Format();
    }
    // lets not forget to draw the widget
    pawsWidget::Draw();
}

void pawsCraftWindow::Format()
{
    // first...if no filter is defined we just print out everything
    if (filter == "")
    {
       textBox->SetText(craftText.GetData());
       return;
    }
    // okay...lets see what lines the user actually wants to see
    csString outputText = "";
    // We don't really care about upper or lower case so just convert the filter to uppercase
    csString upcaseFilter = filter;
    upcaseFilter.Upcase();
    // now we split the whole crafting recipes text in individual lines and check if any of these lines contains the "filter"
    csString tmpLine;
    size_t oldNewLine = 0;
    size_t foundNewLine = craftText.Find("\n", oldNewLine);
    while (foundNewLine != (size_t)-1)
    {
       // we found a newline...so create a temporary string for this line
       craftText.SubString(tmpLine, oldNewLine, foundNewLine-oldNewLine);
       // and again...we only want upper case letters
       tmpLine.Upcase();
       // So, lets see if this line contains the "filter"
       if (tmpLine.Find(upcaseFilter) != (size_t)-1)
       {
           // we found it...so add the original line to the output text (reusing tmpLine as we don't need the uppercase version anymore)
           craftText.SubString(tmpLine, oldNewLine, foundNewLine-oldNewLine);
           outputText.Append(tmpLine);
       }
       // and lets go on with our search
       oldNewLine = foundNewLine;
       foundNewLine = craftText.Find("\n", oldNewLine+1);
    }
    // no more newlines found...so lets see if the remaining text fits the filter as well
    craftText.SubString(tmpLine, oldNewLine, craftText.Length()-oldNewLine);
    // yeah, yeah...uppercase
    tmpLine.Upcase();
    if (tmpLine.Find(upcaseFilter) != (size_t)-1)
    {
       // found it
       craftText.SubString(tmpLine, oldNewLine, craftText.Length()-oldNewLine);
       outputText.Append(tmpLine);
    }
    // last but not least...set the text to our found crafting recipes
    textBox->SetText(outputText.GetData());
}

