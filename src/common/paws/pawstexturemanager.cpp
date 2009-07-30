/*
 * pawstexturemanager.cpp - Author: Andrew Craig
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <csutil/xmltiny.h>
#include <iutil/databuff.h>
#include <igraphic/imageio.h>
#include <igraphic/image.h>

#include <ivideo/txtmgr.h>
#include <ivideo/graph2d.h>
#include <csgeom/math.h>


#include "util/log.h"

#include "pawstexturemanager.h"
#include "pawsimagedrawable.h"
#include "pawsframedrawable.h"
#include "pawsmanager.h"

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsTextureManager::pawsTextureManager( iObjectRegistry* object )
{
    objectReg = object;

    vfs =  csQueryRegistry<iVFS > ( objectReg);
    xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
}

pawsTextureManager::~pawsTextureManager()
{
}
    
bool pawsTextureManager::LoadImageList( const char* listName )
{ 
    printf("Texture Manager parsing %s\n", listName);
    csRef<iDataBuffer> buff = vfs->ReadFile( listName );

    if ( !buff || !buff->GetSize() )
    {
        return false;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( buff );
    if ( error )
    {
        Error3("Error %s in %s", error, listName);
        return false;
    }
    
    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("XML error in %s", listName);
        return false;
    }

    csRef<iDocumentNode> topNode = root->GetNode("image_list");
    if(!topNode)
    {
        Error2("Missing tag <image_list> in %s", listName);
        return false;
    }   
    
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        if (elementList.Contains(node->GetAttributeValue("resource")))
            continue;

        iPawsImage *element = NULL;
        if (!strcmp(node->GetValue(), "image"))
            element = new pawsImageDrawable(node);
		else if (!strcmp(node->GetValue(), "frame"))
			element = new pawsFrameDrawable(node);
        else
            Error2("Illegal node name in imagelist.xml: %s", node->GetValue() );

        if (element)
            AddPawsImage(element);
    }

    return true;     
}

bool pawsTextureManager::AddImage(const char* resource)
{
    iPawsImage *element = NULL;
    element = new pawsImageDrawable(resource, resource);
    if(element)
    {
        AddPawsImage(element);
        return true;
    }
    return false;
}

void pawsTextureManager::Remove( const char* resource )
{
    elementList.DeleteAll(resource);
}

csPtr<iPawsImage> pawsTextureManager::GetPawsImage(const char * name)
{
    return csPtr<iPawsImage>(elementList.Get(name, 0));
}

void pawsTextureManager::AddPawsImage(iPawsImage * element)
{
    // printf("Adding pawsImage called %s\n", element->GetName() );
    elementList.Put(element->GetName(), element);
}

