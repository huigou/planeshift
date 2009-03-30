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
#include <iutil/objreg.h>

#include "pseffectlight.h"

// used for generating a unique ID
static unsigned int genUniqueID = 0;

psLight::psLight(iObjectRegistry* object_reg)
{
    vclock = csQueryRegistry<iVirtualClock>(object_reg);
    movable = NULL;
    dim = true;
    sequencenum = 0;
    stepsPerSecond = 60;
    stepsPerCycle = 60;
    dimStrength = 100;
}

psLight::~psLight()
{
    light->QuerySceneNode()->SetParent(0);
    sector->GetLights()->Remove(light);
}

unsigned int psLight::AttachLight(csRef<iLight> newLight, csRef<iMeshWrapper> mw)
{
    light = newLight;
    movable = mw->GetMovable();
    baseColour = light->GetColor();

    light->QuerySceneNode()->SetParent(mw->QuerySceneNode());
    sector = movable->GetSectors()->Get(0);
    sector->AddLight(light);

    lastTime = vclock->GetCurrentTicks();

    return ++genUniqueID;
}

bool psLight::Update()
{
    if(movable.IsValid())
    {
        iSectorList* sectors = movable->GetSectors();
        if(sectors->GetCount() && sector != sectors->Get(0))
        {
            sector->GetLights()->Remove(light);
            sector = sectors->Get(0);
            sector->AddLight(light);
        }

        int advanceSteps = (vclock->GetCurrentTicks() - lastTime) / (1000.0f/stepsPerSecond);

        if(0 < advanceSteps)
        {
          lastTime = vclock->GetCurrentTicks();

          if(dim)
          {
            if(sequencenum < stepsPerCycle/2)
            {
              csColor n = light->GetColor();

              for(int i=0; sequencenum < stepsPerCycle/2 && i<advanceSteps; ++i)
              {
                n.red -= baseColour.red/dimStrength;
                n.green -= baseColour.green/dimStrength;
                n.blue -= baseColour.blue/dimStrength;
                ++sequencenum;
              }

              light->SetColor(n);
            }
            else
            {
              dim = false;
            }
          }
          else
          {
            if(sequencenum > 0)
            {
              csColor n = light->GetColor();

              for(int i=0; sequencenum > 0 && i<advanceSteps; ++i)
              {
                n.red += baseColour.red/dimStrength;
                n.green += baseColour.green/dimStrength;
                n.blue += baseColour.blue/dimStrength;
                --sequencenum;
              }

              light->SetColor(n);
            }
            else
            {
              dim = true;
            }
          }
        }

        return true;
    }

    return false;
}
