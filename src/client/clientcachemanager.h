/*
 * ClientCacheManager.h
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

#ifndef CLIENT_CACHE_MANAGER_HEADER
#define CLIENT_CACHE_MANAGER_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/hash.h>
#include <iutil/threadmanager.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "globals.h"

struct iCollection;

/** Holds details on a mesh factory. */
struct FactoryIndexEntry : public csRefCount
{
    csString filename;
    csString factname;
    csRef<iMeshFactoryWrapper> factory;
    csRef<iThreadReturn> result;
};

//-----------------------------------------------------------------------------

/** Used to cache in the model files at startup.
  * This is used to load all the model factories at startup so all loading is 
  * in one place and doesn't cause big pauses in the middle of the game 
  * session.
  */
class ClientCacheManager
{
public:
    ClientCacheManager();
    ~ClientCacheManager();
    
    /** Checks the cache list for a particular factory.
      * @param filename  The model file that we want to get.
      * @return The structure for this file if found. NULL otherwise. 
      */
    FactoryIndexEntry* GetFactoryEntry(const char* filename);

private:

    /** Loads a new model factory. 
     * @param filename The VFS file name of the model file to load.
     * @param old Use the old functionallity (compatibility).
     */
    FactoryIndexEntry* LoadNewFactory(const char* filename);

    csRef<iCollection> cache;
    csRef<iStringSet> stringset;
    csHash<csRef<FactoryIndexEntry>, csStringID> factIndex;
};

#endif
