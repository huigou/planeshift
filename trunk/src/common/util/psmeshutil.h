/*
 * psmeshutil.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PSMESHUTIL_H
#define PSMESHUTIL_H

#include <iengine/mesh.h>

struct iObjectRegistry;
struct iEngine;
struct iLoader;
struct iVFS;
struct iTextureManager;
struct iGraphics3D;
struct iMaterialWrapper;
struct iRegion;

class psMeshUtil
{
public:
    psMeshUtil(iObjectRegistry *obj_reg);

    /** Returns an iMaterialWrapper to the given texture, creates one if one doesn't exist.
      *
      * @param name the name of the texture, as stored in CS
      * @param texture the vfs file path to the texture
      * @param region the region to add the effect to (leave 0 for none)
      * @return NULL on failure, the iMaterialWrapper of the texture on success
      */
    iMaterialWrapper* LoadMaterial(const char* name, const char* texture, iRegion* region=0 );

    /** Parse the string and change $F with factname, $P with part
      *
      * @param mesh The object to take data from
      * @param part The part to be inserted for $P
      * @param str The string to parse
      * @return Resulting string with replased variables.
      */
    csString ParseStrings(iMeshWrapper* mesh, const char *part, const char* str) const;

    /** Changes the material that is on a mesh.
      * Assumes that the mesh is infact a cal3d state mesh.
      *
      * @param mesh The main mesh wrapper
      * @param part The name of the sub mesh that we want to change the material on.
      * @param material The name of the new material that this part (submesh) should be.
      * @param texture The name of the texture to be used for material if material not found.
      * @return true if the material was placed on the mesh successfully.
      */
    bool ChangeMaterial(iMeshWrapper* mesh, const char* part, const char* material, const char* texture);

    /** Changes the materail on a submesh back to the default material.
      *
      * @param mesh The main mesh wrapper
      * @param part The name of the sub mesh that we want to revert back to using default.
      * @return true if the default material was placed back on the submesh.
      */
    bool DefaultMaterial(iMeshWrapper* mesh, const char* part);

    /** Change one of the submeshes to a new mesh.
     *
     * @param mesh The main mesh wrapper
     * @param newMeshName The name of the sub mesh that we want to be inserted.
     * @param oldMeshName The name of the sub mesh that we want to be replaces.
     * @return true if the mesh is changes.
     */
    bool ChangeMesh( iMeshWrapper* mesh, const char* partPattern, const char* newPart);
    
    /** Change one of the submeshes back to the default mesh
     *
     * @param mesh The main mesh wrapper
     * @param newMeshName The name of the sub mesh that we want to be inserted.
     * @param oldMeshName The name of the sub mesh that we want to be replaces.
     * @return true if the mesh is changes.
     */
    bool DefaultMesh( iMeshWrapper* mesh, const char* partPattern);
    
    /** Attach a mesh to another mesh in a particular socket on a different mesh.
      *
      * @param mesh The mesh to attach to.
      * @param socket The name of the socket to attach to.
      * @param meshName The name of the mesh that should be attached.
      * @return True if the mesh was attached properly.
      */
    bool Attach(iMeshWrapper* mesh, const char* socket, const char* meshName);

    /** Remove a mesh from a particular socket.
      *
      * @param mesh The parent mesh that has the child mesh attached.
      * @param socket The name of the socket to remove the mesh from.
      * @return True if the detach was successful.
      */
    bool Detach(iMeshWrapper* mesh, const char* socket);

private:
    
    csRef<iEngine>          engine;
    csRef<iLoader>          loader;
    csRef<iVFS>             vfs;
    csRef<iGraphics3D>      g3d;
    csRef<iTextureManager>  txtmgr;
};

#endif
