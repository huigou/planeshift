/*
 * ibgloader.h - Author: Mike Gist
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef __IBGLOADER_H__
#define __IBGLOADER_H__

#include <csgeom/vector3.h>
#include <csutil/csstring.h>
#include <iutil/threadmanager.h>

class csColor4;
struct iMaterialWrapper;
struct iMeshFactoryWrapper;
struct iObjectRegistry;
struct iThreadedLoader;

/**
 * Structure holding start position data.
 */
struct StartPosition : public csRefCount
{
    csString zone;
    csString sector;
    csVector3 position;
};

/**
 * Interface to the background loader plugin.
 */
struct iBgLoader : public virtual iBase
{
  SCF_INTERFACE(iBgLoader, 1, 6, 1);

 /**
  * Start loading a material into the engine. Returns 0 if the material is not yet loaded.
  * @param name name of the material to load.
  * @param failed Pass a boolean to be able to manually handle a failed load.
  * @param wait specify whether to wait for the texture to be loaded.
  * @return pointer to the material.
  * @remark a successfull call(e.g. return pointer != 0) increases the usage count of the material.
  * call FreeMaterial in order to release your usage.
  */
  virtual iMaterialWrapper* LoadMaterial(const char* name, bool* failed = NULL, bool wait = false) = 0;

 /**
  * Start loading a mesh factory into the engine. Returns 0 if the factory is not yet loaded.
  * @param name name of the factory to load.
  * @param failed Pass a boolean to be able to manually handle a failed load.
  * @param wait specify whether to wait until the factory is full loaded.
  * @return pointer to the factory.
  * @remark a successfull call(e.g. return pointer != 0) increases the usage count of the factory.
  * call FreeFactory in order to release your usage.
  */
  virtual csPtr<iMeshFactoryWrapper> LoadFactory(const char* name, bool* failed = NULL, bool wait = false) = 0;

 /**
  * free your instance of a material.
  * @param name name of the material to free.
  * @return true upon success, false otherwise
  */
  virtual bool FreeMaterial(const char * name) = 0;

 /**
  * free your instance of a mesh factory.
  * @param name name of the factory to free.
  * @return true upon success, false otherwise
  */
  virtual bool FreeFactory(const char * name) = 0;

 /**
  * Clone a mesh factory.
  * @param name The name of the mesh factory to clone.
  * @param newName The name of the new cloned mesh factory.
  * @param load Begin loading the cloned mesh factory.
  * @param failed Pass a boolean to be able to manually handle a failed clone.
  */
  virtual void CloneFactory(const char* name, const char* newName, bool* failed = NULL) = 0;

 /**
  * Pass a data file to be cached. This method will parse your data and add it to it's
  * internal world representation. You may then request that these objects are loaded.
  * @param path path to the file to be cached.
  * @param recursive Mark true if this is a recursive call (no vfs chdir needed).
  * If you don't know, set this to false.
  * This call will be dispatched to a thread, so it will return immediately.
  * You should wait for parsing to finish before calling UpdatePosition().
  */
  THREADED_INTERFACE2(PrecacheData, const char* path, bool recursive);

 /**
  * Update your position in the world.
  * Calling this will trigger per-object checks and initiate (un)loading if the object
  * is within a given threshold (loadRange).
  * @param pos Your world space position.
  * @param sectorName The name of the sector that you are currently in.
  * @param force Forces the checks to be done (normally they won't if you e.g. haven't moved).
  */
  virtual void UpdatePosition(const csVector3& pos, const char* sectorName, bool force) = 0;

 /**
  * Call this function to finalise a number of loading objects.
  * Useful when you are waiting for a load to finish (load into the world, teleport),
  * but want to continue rendering while you wait.
  * Will return after processing a number of objects.
  * @param waiting Set as 'true' if you're waiting for the load to finish.
  * This will make it process more before returning (lower overhead).
  * Note that you should process a frame after each call, as spawned threads
  * may depend on the main thread to handle requests.
  */
  virtual void ContinueLoading(bool waiting) = 0;

 /**
  * Returns a pointer to the Crystal Space threaded loader.
  */
  virtual iThreadedLoader* GetLoader() = 0;

 /**
  * Returns the number of objects currently loading.
  */
  virtual size_t GetLoadingCount() = 0;

 /**
  * Returns a pointer to the object registry.
  */
  virtual iObjectRegistry* GetObjectRegistry() const = 0;

 /**
  * Update the load range initially passed to the loader in Setup().
  * @param r range to set.
  */
  virtual void SetLoadRange(float r) = 0;

 /**
  * Request to know whether the current world position stored by the loader is valid.
  * @return false until the first call of UpdatePosition().
  */
  virtual bool HasValidPosition() const = 0;

 /**
  * Request to know whether you are currently positioned in a water body.
  * @param sector The sector that you are checking.
  * @param pos The world space position that you are checking.
  * @param colour Will contain the colour of the water that you are positioned in.
  * @return false until you are located in a water body.
  */
  virtual bool InWaterArea(const char* sector, csVector3* pos, csColor4** colour) = 0;

 /**
  * Get a list of shaders available for a given type.
  * @param usageType The type of shader you wish to have.
  * E.g. 'default_alpha' to get an array of all default world alpha shaders.
  * @return pointer to an array holding the names of the requested shaders.
  */
  virtual csPtr<iStringArray> GetShaderName(const char* usageType) const = 0;

 /**
  * Request start positions in the world.
  * @return pointer to an array holding all known starting positions.
  */
  virtual csRefArray<StartPosition>* GetStartPositions() = 0;

 /**
  * Load zones given by name.
  * @param regions pointer to an array holding the region names. if NULL, all regions are loaded.
  * @param loadMeshes specify whether or not to load meshes.
  * @param priority specify whether the regions shall be marked high priority.
  * @see LoadPriorityZones.
  * @return true upon success, false otherwise.
  */
  virtual bool LoadZones(iStringArray* regions = 0, bool loadMeshes = true, bool priority = false) = 0;

 /**
  * Load high priority zones given by name.
  * Unlike normal zones those don't get unloaded
  * unless a new set of high priority zones is given.
  * @param regions pointer to an array holding the region names.
  * @return true upon success, false otherwise.
  * @see LoadZones.
  */
  virtual bool LoadPriorityZones(iStringArray* regions) = 0;
};

#endif // __IBGLOADER_H__
