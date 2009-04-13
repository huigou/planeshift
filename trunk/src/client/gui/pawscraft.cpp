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
//    itemTree = NULL;
    msgHandler = psengine->GetMsgHandler();
    if ( !msgHandler ) return false;

    if ( !msgHandler->Subscribe( this, MSGTYPE_CRAFT_INFO ) )
        return false;
        
    return true;    
}

void pawsCraftWindow::Show()
{
    pawsWidget::Show();        

    // Get craft info for mind item
    psMsgCraftingInfo msg;
    msg.SendMessage();
}   


void pawsCraftWindow::HandleMessage( MsgEntry* me )
{
    textBox = dynamic_cast<pawsMultiLineTextBox*>(FindWidget("HelpText"));
    psMsgCraftingInfo mesg(me);
    csString text(mesg.craftInfo);
    if (text)
        textBox->SetText(text.GetData());

/*
    if ( itemTree )
        delete itemTree;
        
    itemTree = (pawsSimpleTree*) (PawsManager::GetSingleton().CreateWidget("pawsSimpleTree"));
    if (itemTree == NULL)
    {
        Error1("Could not create widget pawsSimpleTree");
        return; 
    }
        
    AddChild(itemTree);
    itemTree->SetRelativeFrame(37,29,GetActualWidth(198),GetActualHeight(224));
    itemTree->SetNotify(this);
    itemTree->SetAttachFlags(ATTACH_TOP | ATTACH_BOTTOM | ATTACH_LEFT);
    itemTree->SetScrollBars(false, true);
    itemTree->Resize();
    itemTree->SetDefaultColor(psengine->GetG2D()->FindRGB(255,255,255));
        

    itemTree->InsertChildL("", "Root", "", "");

    // Change the title of the window that of the pattern.
    csString title("Crafting: ");
    title.Append( mesg.pattern );    
    SetTitle( title );      
         
    for ( size_t z = 0; z <  mesg.items.GetSize(); z++ )
    {
        Add( "Root", "Root", mesg.items[z] );
    }         
    
    pawsTreeNode * child = itemTree->GetRoot()->GetFirstChild();
    while (child != NULL)
    {
        child->CollapseAll();
        child = child->GetNextSibling();
    }   
*/    
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

bool pawsCraftWindow::OnSelected(pawsWidget *widget)
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
