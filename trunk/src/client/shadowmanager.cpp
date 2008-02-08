/*
* shadowmanager.cpp - Author: Andrew Robberts
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// CS INCLUDES
#include <psconfig.h>

// CEL INCLUDES
#include <iengine/mesh.h>
#include <csgeom/box.h>
#include <imesh/objmodel.h>
#include <iutil/cfgmgr.h>

// PS INCLUDES
#include "globals.h"
#include "shadowmanager.h"
#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"
#include "effects/pseffectobj.h"
#include "effects/pseffectobjtext.h"

bool psShadowManager::WithinRange(GEMClientObject * object) const
{
    if (shadowRange <= 0.000001f)
        return true;

    GEMClientObject * mainPlayer = psengine->GetCelClient()->GetMainPlayer();
    if (!mainPlayer)
        return true;

    csVector3 diff = object->Mesh()->GetMovable()->GetPosition() - 
        psengine->GetCelClient()->GetMainPlayer()->Mesh()->GetMovable()->GetPosition();
    return (diff.SquaredNorm() <= shadowRange*shadowRange);
}

psShadowManager::psShadowManager()
{
    cfgmgr = psengine->GetConfig();
    shadowRange = cfgmgr->GetFloat("PlaneShift.Visuals.ShadowRange", -1.0f);

    RecreateAllShadows();
}

psShadowManager::~psShadowManager()
{

}

void psShadowManager::CreateShadow(GEMClientObject * object)
{
    if (!object)
        return;

    // don't create one if the object already has a shadow
    if (object->GetShadow())
        return;

    iMeshWrapper* mesh = object->Mesh();
    if (!mesh || !mesh->GetMeshObject())
        return;

    if (!WithinRange(object))
        return;

    // calculate a suitable size for this shadow
    const csBox3& boundBox = mesh->GetMeshObject()->GetObjectModel()->GetObjectBoundingBox();
    float scale = (boundBox.Max(0) + boundBox.Max(2)) * 0.75f;
    if (scale < 0.35f)
        scale = 0.35f;

    psEffectManager* effectMgr = psengine->GetEffectManager();

    // Create the effect
    unsigned int id = effectMgr->RenderEffect("shadow", csVector3(0,0,0), mesh);
    psEffect* effect = effectMgr->FindEffect(id);
    if (!effect)
        return;

    effect->SetScaling(scale, 1.0f);
    object->SetShadow(effect);
}

void psShadowManager::RemoveShadow(GEMClientObject * object)
{
    if (!object)
        return;

    psEffect * shadow = object->GetShadow();
    if (!shadow)
        return;

    psengine->GetEffectManager()->DeleteEffect(shadow->GetUniqueID());
    object->SetShadow(0);
}

void psShadowManager::RecreateAllShadows()
{
    const csPDelArray<GEMClientObject>& entities = psengine->GetCelClient()->GetEntities();
    size_t len = entities.GetSize();
    for (size_t a=0; a<len; ++a)
        CreateShadow(entities[a]);
}

void psShadowManager::RemoveAllShadows(GEMClientObject * object)
{
    const csPDelArray<GEMClientObject>& entities = psengine->GetCelClient()->GetEntities();
    size_t len = entities.GetSize();
    for (size_t a=0; a<len; ++a)
        RemoveShadow(entities[a]);
}

float psShadowManager::GetShadowRange() const
{
    return shadowRange;
}

void psShadowManager::SetShadowRange(float shadowRange)
{
    this->shadowRange = shadowRange;
    cfgmgr->SetFloat("PlaneShift.Visuals.ShadowRange", shadowRange);
}

void psShadowManager::UpdateShadows()
{
    const csPDelArray<GEMClientObject>& entities = psengine->GetCelClient()->GetEntities();
    size_t len = entities.GetSize();
    for (size_t a=0; a<len; ++a)
    {
        if (!WithinRange(entities[a]))
            RemoveShadow(entities[a]);
        else
            CreateShadow(entities[a]);         
    }
}

