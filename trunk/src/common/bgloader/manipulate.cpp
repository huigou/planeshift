/*
 *  loader.cpp - Author: Mike Gist
 *
 * Copyright (C) 2008-10 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cssysdef.h>
#include <cstool/collider.h>
#include <cstool/enginetools.h>
#include <cstool/vfsdirchange.h>
#include <csutil/scanstr.h>
#include <csutil/scfstringarray.h>
#include <iengine/camera.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <imap/services.h>
#include <imesh/object.h>
#include <iutil/cfgmgr.h>
#include <iutil/document.h>
#include <iutil/object.h>
#include <iutil/plugin.h>
#include <ivaria/collider.h>
#include <ivaria/engseq.h>
#include <ivideo/graph2d.h>
#include <ivideo/material.h>

#include "util/psconst.h"
#include "loader.h"

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
    iMeshWrapper* BgLoader::CreateAndSelectMesh(const char* factName, iCamera* camera, const csVector2& pos)
    {
        // Check that requested mesh is valid.
        csRef<MeshFact> meshfact = meshfacts.Get(mfStringSet.Request(factName), csRef<MeshFact>());
        if(!meshfact.IsValid())
            return 0;

        // Get WS position.
        csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);

        // If there's no hit then we can't create.
        if(result.mesh == 0)
            return 0;

        // Load meshfactory.
        while(!LoadMeshFact(meshfact));
        csRef<iMeshFactoryWrapper> factory = scfQueryInterface<iMeshFactoryWrapper>(meshfact->status->GetResultRefPtr());

        // Update stored position.
        previousPosition = pos;

        // Create new mesh.
        if(selectedMesh && resetHitbeam)
        {
            selectedMesh->GetFlags().Reset(CS_ENTITY_NOHITBEAM);
        }
        resetHitbeam = false;
        selectedMesh = factory->CreateMeshWrapper();
        selectedMesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
        selectedMesh->GetMovable()->SetPosition(camera->GetSector(), result.isect);
        selectedMesh->GetMovable()->UpdateMove();

        return selectedMesh;
    }

    iMeshWrapper* BgLoader::SelectMesh(iCamera* camera, const csVector2& pos)
    {
        // Get WS position.
        csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);

        // Reset flags.
        if(selectedMesh && resetHitbeam)
        {
            selectedMesh->GetFlags().Reset(CS_ENTITY_NOHITBEAM);
        }

        // Get new selected mesh.
        selectedMesh = result.mesh;
        if(selectedMesh)
        {
            resetHitbeam = !selectedMesh->GetFlags().Check(CS_ENTITY_NOHITBEAM);
            selectedMesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
        }

        // Update stored position.
        previousPosition = pos;

        return selectedMesh;
    }

    bool BgLoader::TranslateSelected(bool vertical, iCamera* camera, const csVector2& pos)
    {
        if(selectedMesh.IsValid())
        {
            if(vertical)
            {
                float d = 5 * float(previousPosition.y - pos.y) / g2d->GetWidth();
                csVector3 position = selectedMesh->GetMovable()->GetPosition();
                selectedMesh->GetMovable()->SetPosition(position + csVector3(0.0f, d, 0.0f));
                previousPosition = pos;
            }
            else
            {
                // Get WS position.
                csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);
                if(result.mesh)
                {
                    selectedMesh->GetMovable()->SetPosition(result.isect);
                    return true;
                }
            }
        }

        return false;
    }

    void BgLoader::RotateSelected(const csVector2& pos)
    {
        if(selectedMesh.IsValid())
        {
            float d = 6 * PI * ((float)previousPosition.x - pos.x) / g2d->GetWidth();
            csYRotMatrix3 rotation(d);

            selectedMesh->GetMovable()->GetTransform().SetO2T(origRotation*rotation);
        }
    }

    void BgLoader::RemoveSelected()
    {
        if(selectedMesh.IsValid())
        {
            selectedMesh->GetMovable()->SetSector(0);
            selectedMesh.Invalidate();
        }
    }
}
CS_PLUGIN_NAMESPACE_END(bgLoader)
