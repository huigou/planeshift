/*
* Author: Andrew Robberts
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

#include <psconfig.h>

#include <csutil/cscolor.h>
#include <iengine/light.h>
#include <iengine/halo.h>
#include <iengine/mesh.h>
#include <iengine/scenenode.h>
#include <iutil/object.h>

#include "pseffectlight.h"

// used for generating a unique ID
static unsigned int genUniqueID = 0;

psLight::psLight()
{
    movable = NULL;
}

psLight::~psLight()
{
}

unsigned int psLight::AttachLight(csRef<iLight> newLight, csRef<iMeshWrapper> mw)
{
    light = newLight;
    movable = mw->GetMovable();
    lightBasePos = light->GetMovable()->GetPosition();
    sector = movable->GetSectors()->Get(0);
    light->GetMovable()->SetSector(sector);
    light->GetMovable()->SetPosition(lightBasePos+movable->GetFullPosition());
    light->GetMovable()->UpdateMove();

    return ++genUniqueID;
}

bool psLight::Update()
{
    if(movable.IsValid())
    {
        iSectorList* sectors = movable->GetSectors();
        if(sectors->GetCount())
        {
            csString sectorname(sectors->Get(0)->QueryObject()->GetName());
            if(sectorname.Compare("SectorWhereWeKeepEntitiesResidingInUnloadedMaps"))
                return true;

            csVector3 newPos(movable->GetFullTransform().GetT2O()*lightBasePos);
            newPos += movable->GetFullPosition();

            if(light->GetMovable()->GetPosition() != newPos)
            {
                if(sectors && (!sector || sector != sectors->Get(0)))
                {
                    sector = sectors->Get(0);
                    light->GetMovable()->SetSector(sector);
                }
                light->GetMovable()->SetPosition(newPos);
                light->GetMovable()->UpdateMove();
            }
        }
        return true;
    }

    return false;
}
