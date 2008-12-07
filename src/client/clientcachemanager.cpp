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
#include <iengine/collection.h>
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
    cache = psengine->GetEngine()->CreateCollection("psclientcache");
    stringset = csQueryRegistryTagInterface<iStringSet>(psengine->GetObjectRegistry(),
                                                       "crystalspace.shared.stringset");
}

ClientCacheManager::~ClientCacheManager()
{
    psengine->GetEngine()->RemoveCollection(cache);
}

FactoryIndexEntry* ClientCacheManager::LoadNewFactory(const char* filename)
{
    csString file = filename;

    // Check if the file exists
    if (!psengine->GetVFS()->Exists(filename))
    {
        Error2("Couldn't find file %s",filename);
        return NULL;
    }

    csRef<iDocument> doc = ParseFile(psengine->GetObjectRegistry(),filename);
    if (!doc)
    {
        Error2("Couldn't parse file %s",filename);
        return NULL;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("The file(%s) doesn't have a root",filename);
        return NULL;
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
        return NULL;
    }

    csRef<FactoryIndexEntry> indexEntry;
    indexEntry.AttachNew(new FactoryIndexEntry);
    indexEntry->factname = meshNode->GetAttributeValue("name");

    if(!indexEntry->factname)
    {
        Error2("The mesh with file %s doesn't have a factory name. This is very bad!", filename);
        return NULL;
    }

    indexEntry->filename = filename;
    indexEntry->factory = NULL;
    factIndex.Put(stringset->Request(filename), indexEntry);

    // Check if it's already loaded.
    iMeshFactoryWrapper* meshW = psengine->GetEngine()->GetMeshFactories()->FindByName(indexEntry->factname);
    if (meshW)
    {
        indexEntry->factory = meshW;
        cache->Add(meshW->QueryObject());
    }

    if (!indexEntry->factory.IsValid())
    {
        if(file.Find(".cal3d") != (size_t)-1)
        {
            if(!(psengine->GetGFXFeatures() & useAdvancedShaders))
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
                    material->RemoveNodes(material->GetNodes("shadervar"));
                    csRef<iDocumentNodeIterator> shaders = material->GetNodes("shader");
                    while(shaders->HasNext())
                    {
                        csRef<iDocumentNode> shader = shaders->Next();
                        if(strcmp("colorize", shader->GetContentsValue()) != 0)
                        {
                            material->RemoveNode(shader);
                        }
                    }
                }
            }
        }

        indexEntry->result = psengine->GetLoader()->LoadNode(root, cache);
    }

    return indexEntry;
}

FactoryIndexEntry* ClientCacheManager::GetFactoryEntry(const char* filename)
{
    // Search for a factory entry with the filename we've passed.
    FactoryIndexEntry* indexEntry = factIndex.Get(stringset->Request(filename), NULL);

    bool checked = false;
    while(true)
    {
        // If the factory entry exists...
        if (indexEntry)
        {
            CS_ASSERT_MSG("Factory index != filename", indexEntry->filename == filename);

            // If it's loaded then we can return it.
            if(indexEntry->factory)
            {
                // Disable decals on all movable meshes. Make more specific if/when we need this and it works.
                indexEntry->factory->GetFlags().Set(CS_ENTITY_NODECAL);
                return indexEntry;
            }
            else if(indexEntry->result->IsFinished())
            {
                if(indexEntry->result->WasSuccessful())
                {
                    indexEntry->factory = cache->FindMeshFactory(indexEntry->factname);
                }
                else
                {
                    // Something bad happened (probably a data problem).
                    csString msg;
                    msg.Format("Factory Entry %s failed to load!", filename);
                    CS_ASSERT_MSG(msg.GetData(), false);
                }
            }
            else
            {
                // Try again later.
                return NULL;
            }
        }

        if(checked)
        {
            // Try again later.
            return NULL;
        }

        // No such factory entry exists.. so create a new one.
        indexEntry = LoadNewFactory(filename);
        checked = true;
    }

    // It looks like something bad happened. This should be caught before now.. so bail out screaming!
    csString msg;
    msg.Format("Factory Entry %s failed to load!", filename);
    CS_ASSERT_MSG(msg.GetData(), false);
    return NULL;
}
