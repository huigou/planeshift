/*
 * ClientCacheManager.cpp
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <imap/loader.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psxmlparser.h"

#include "engine/materialmanager.h"
#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "clientcachemanager.h"
#include "globals.h"

ClientCacheManager::ClientCacheManager()
{
}

ClientCacheManager::~ClientCacheManager()
{
}

void ClientCacheManager::LoadNewFactory(const char* filename)
{
    csString file = filename;

    // Check if the file exists
    if (!psengine->GetVFS()->Exists(filename))
    {
        Error2("Couldn't find file %s",filename);
        return;
    }

    csRef<iDocument> doc = ParseFile(psengine->GetObjectRegistry(),filename);
    if (!doc)
    {
        Error2("Couldn't parse file %s",filename);
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("The file(%s) doesn't have a root",filename);
        return;
    }

    csRef<iDocumentNode> meshNode;
    csRef<iDocumentNode> libNode = root->GetNode("library");
    if (libNode)
        meshNode = libNode->GetNode("meshfact");
    else
        meshNode = root->GetNode("meshfact");
    if (!meshNode)
    {
        Error2("The file(%s) doesn't have a meshfact or library node", filename);
        return;
    }

    FactoryIndexEntry* indexEntry = new FactoryIndexEntry;
    indexEntry->factory = NULL;

    // If we have a mesh name, see if it's already loaded
    const char* name = meshNode->GetAttributeValue("name");
    indexEntry->factname = name;
    if (name)
    {
        iMeshFactoryWrapper* meshW = psengine->GetEngine()->GetMeshFactories()->FindByName(name);
        if (meshW) 
            indexEntry->factory = meshW;
    }

    if(file.Find(".cal3d") != (size_t)-1)
    {
        if(!(psengine->GetGFXFeatures() & useNormalMaps))
        {
            csRef<iDocumentNode> texNode = libNode->GetNode("textures");
            csRef<iDocumentNodeIterator> textures = texNode->GetNodes();
            while(textures->HasNext())
            {
                csRef<iDocumentNode> texture = textures->Next();
                csRef<iDocumentNode> classNode = texture->GetNode("class");
                if(classNode)
                {
                    csString texClass(classNode->GetContentsValue());
                    if(texClass.Compare("normalmap"))
                    {
                        texNode->RemoveNode(texture);
                    }
                }
            }
            csRef<iDocumentNode> matNode = libNode->GetNode("materials");
            csRef<iDocumentNodeIterator> materials = matNode->GetNodes();
            while(materials->HasNext())
            {
                csRef<iDocumentNode> material = materials->Next();
                material->RemoveNodes(material->GetNodes("shader"));
                material->RemoveNodes(material->GetNodes("shadervar"));
            }
        }
    }

    if (indexEntry->factory == NULL)
    {
        iBase* result = NULL;
        psengine->GetLoader()->Load (root, result);
        iMeshFactoryWrapper* meshW = psengine->GetEngine()->GetMeshFactories()->FindByName(name);
        indexEntry->factory = meshW;
    }

    indexEntry->filename = filename;

    factIndex.Push(indexEntry);
}

FactoryIndexEntry* ClientCacheManager::GetFactoryEntry(const char* filename)
{
    for (size_t i = 0;i < factIndex.GetSize(); i++)
    {
        FactoryIndexEntry* factory = factIndex.Get(i);
        if (!factory)
            continue;

        if (factory->filename == filename)
            return factory;
    }
    
    LoadNewFactory(filename);
    for (size_t i = 0;i < factIndex.GetSize(); i++)
    {
        FactoryIndexEntry* factory = factIndex.Get(i);
        if (!factory)
            continue;

        if (factory->filename == filename)
            return factory;
    }
    return NULL;
}
