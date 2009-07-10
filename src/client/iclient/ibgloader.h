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

#include <iutil/threadmanager.h>

class csColor4;
class csVector3;
struct iObjectRegistry;
struct iThreadedLoader;

/**
 * Interface to the background loader plugin.
 */
struct iBgLoader : public virtual iBase
{
  SCF_INTERFACE(iBgLoader, 1, 3, 0);

 /**
  * Sets key settings and performs a shader parse.
  * @param gfxFeatures Mask of available graphics features.
  * @param loadRange The maximum range within which the loader should check for objects.
  */
  virtual void Setup(uint gfxFeatures, float loadRange) = 0;

 /**
  * Start loading a material into the engine. Returns 0 if the material is not yet loaded.
  * @param failed Pass a boolean to be able to manually handle a failed load.
  */
  virtual csPtr<iMaterialWrapper> LoadMaterial(const char* name, bool* failed = NULL) = 0;

 /**
  * Start loading a mesh factory into the engine. Returns 0 if the factory is not yet loaded.
  * @param failed Pass a boolean to be able to manually handle a failed load.
  */
  virtual csPtr<iMeshFactoryWrapper> LoadFactory(const char* name, bool* failed = NULL) = 0;

 /**
  * Pass a data file to be cached. This method will parse your data and add it to it's
  * internal world representation. You may then request that these objects are loaded.
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
  * @param waiting Set as 'true' if you wish the loader to not return until all objects are loaded.
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
  */
  virtual void SetLoadRange(float r) = 0;

 /**
  * Request to know whether the current world position stored by the loader is valid.
  * Returns false until the first call of UpdatePosition().
  */
  virtual bool HasValidPosition() const = 0;

 /**
  * Request to know whether you are currently positioned in a water body.
  * @param sector The sector that you are checking.
  * @param pos The world space position that you are checking.
  * @param colour Will contain the colour of the water that you are positioned in.
  */
  virtual bool InWaterArea(const char* sector, csVector3* pos, csColor4** colour) const = 0;

 /**
  * Returns an array of the available shaders for a given type.
  * @param usageType The type of shader you wish to have.
  * E.g. 'default_alpha' to get an array of all default world alpha shaders.
  */
  virtual csPtr<iStringArray> GetShaderName(const char* usageType) const = 0;
};

#endif // __IBGLOADER_H__
