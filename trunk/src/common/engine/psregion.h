/*
 * psregion.h
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef __PSREGION_H__
#define __PSREGION_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================

struct iCollection;
struct iDocumentNode;
struct iDocumentSystem;
struct iEngine;
struct iLoader;
struct iObjectRegistry;
struct iVFS;

// Feature defines.
#define useLowShaders 0x01
#define useMediumShaders 0x03
#define useHighShaders 0x07
#define useMeshGen 0x08
#define useAll (useHighShaders | useMeshGen)

/**
* Replacement class for iPcRegion from CEL.
* This is more versatile for dynamic loading
* and unloading of multiple maps than iPcRegion.
*/
class psRegion
{
public:
    /**
    * Creates an entry representing a single region
    * but does not load it.
    */
    psRegion(iObjectRegistry *obj_reg, const char *file, uint gfxFeatures = useAll);

    /**
    * Dtor unloads region if loaded
    */
    ~psRegion();

    /**
    * Loads the file "world" from /planeshift/art/world/<name>.zip
    * into the region called <name>.
    */
    bool Load(bool loadMeshes = true);

    /**
    * Unloads the region from the CS engine, deleting all objects
    * in that region.
    */
    void Unload();

    bool IsLoaded()
    { return loaded; }

    iCollection *GetCollection() { return collection; }

    /**
    * Sets up the collision detection blockers for all meshes in
    * the region.
    */
    void SetupWorldColliders(iEngine *engine);
    void SetupWorldCollidersCD(iEngine *engine, iCollection *cd_col);

    /**
    * The retain flag is used by psWorld owner to manage the list
    * of loaded and unloaded regions efficiently.  See psWorld class
    * def for details on how this is done.
    */
    void SetNeededFlag(bool flag)
    { isNeeded = flag; }
    bool IsNeeded()
    { return isNeeded; }

    const char *GetName()
    { return regionName; }

    /// Cleans the given file and removes all meshes, lights, etc. not needed on the server.
    csRef<iDocumentNode> Clean(csRef<iDocumentNode> worldNode);

    /**
    * Filters the world file to remove features which have been marked
    * as disabled by the user (post proc effects for example).
    */
    csRef<iDocumentNode> Filter(csRef<iDocumentNode> worldNode, bool using3D);

private:
   /**
    * True if we need to filter the world file.
    */
    bool needToFilter;

    // Graphics features we want to use.
    uint gfxFeatures;

    iObjectRegistry* object_reg;
    iCollection* collection;
    csString regionName;
    csString worlddir;
    csString colldetworlddir;
    csString worldfile;
    bool loaded;
    bool isNeeded;

    csRef<iEngine> engine;
    csRef<iVFS> vfs;
    csRef<iDocumentSystem> xml;
    csRef<iLoader> loader;
};

#endif // __PSREGION_H__
