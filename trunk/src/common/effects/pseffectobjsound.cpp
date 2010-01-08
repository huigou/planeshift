/*
* Author: Andrew Robberts
*
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <iengine/camera.h>
#include <cstool/csview.h>
#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>

#include <imap/loader.h>

#include <isndsys/ss_renderer.h>
#include <isndsys/ss_source.h>
#include <isndsys/ss_stream.h>
#include <isndsys/ss_listener.h>

#include "effects/pseffectobjsound.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/pscssetup.h"
#include "util/log.h"

#include <iclient/isoundmngr.h>

psEffectObjSound::psEffectObjSound(iView *parentView, psEffect2DRenderer * renderer2d)
    : psEffectObj(parentView, renderer2d),soundmanager(NULL)
{
}

psEffectObjSound::~psEffectObjSound()
{
  if (soundmanager.IsValid() && soundmanager->GetSoundSystem())
  {
    if (sndSource.IsValid())
    {
         soundmanager->GetSoundSystem()->RemoveSource(sndSource);
    }
    if (sndStream.IsValid())
    {
        soundmanager->GetSoundSystem()->RemoveStream(sndStream);
    }
  }
}

bool psEffectObjSound::Load(iDocumentNode *node, iLoaderContext* ldr_context)
{
    if (!psEffectObj::Load(node, ldr_context))
        return false;

    // get the attributes
    name.Clear();
    soundName.Clear();
    minDistSquared = 25.0f;
    maxDistSquared = 100000.0f;
    loop = true;
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while (attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();
        if (attrName == "name")
            name = attr->GetValue();
        else if (attrName == "resource")
            soundName = attr->GetValue();
        else if (attrName == "loop")
            loop = attr->GetValueAsBool();
    }

    if (!loop && killTime <= 0)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Can't have a looping sound effect without a death.\n");
        return false;
    }

    csRef<iDocumentNode> dataNode;

    // min distance
    dataNode = node->GetNode("mindist");
    if (dataNode)
    {
        minDistSquared = dataNode->GetContentsValueAsFloat(); 
        minDistSquared *= minDistSquared;
    }

    // max distance
    dataNode = node->GetNode("maxdist");
    if (dataNode)
    {
        maxDistSquared = dataNode->GetContentsValueAsFloat(); 
        maxDistSquared *= maxDistSquared;
    }

    if (name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }

    return PostSetup();
}

bool psEffectObjSound::Render(const csVector3 &up)
{
    static unsigned long nextUniqueID = 0;
    csString effectID = "effect_sound_";
    effectID += nextUniqueID++;

    if (soundmanager->PlayingActions())
    {
        if (!sndStream.IsValid())
        {
            sndStream = soundmanager->GetSoundSystem()->CreateStream(sndData, CS_SND3D_RELATIVE);
            if (!sndStream.IsValid())
            {
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Could not create stream for sound: %s\n", soundName.GetData());
                return false;
            }
        }
  
        if (!sndSource.IsValid())
        {
            if (!soundmanager->GetSoundSystem())
            {
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Could not create %s effect obj sound source. Sound system not found.\n", name.GetData());
                return false;
            }

            sndSource = soundmanager->GetSoundSystem()->CreateSource(sndStream);
            if (!sndSource.IsValid())
            {
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Could not create %s effect obj sound source.\n", name.GetData());
                return false;
            }
            sndSource3d= scfQueryInterface<SOUND_SOURCE3D_TYPE> (sndSource);
            if (!sndSource3d.IsValid())
            {
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Could not create %s effect obj sound 3D source.\n", name.GetData());
                return false;
            }
        }

        // Setup looping on the stream
        if (loop)
            sndStream->SetLoopState(CS_SNDSYS_STREAM_LOOP);
        else
            sndStream->SetLoopState(CS_SNDSYS_STREAM_DONTLOOP);

        volumeMultiplier = soundmanager->GetActionsVolume();

        sndSource->SetVolume(volumeMultiplier);
        sndSource3d->SetPosition(csVector3(0,0,0));

        sndSource3d->SetMinimumDistance(sqrt(minDistSquared));
        sndSource3d->SetMaximumDistance(sqrt(maxDistSquared));

        isAlive = false;

        return true;
    }
    return false;
}

bool psEffectObjSound::AttachToAnchor(psEffectAnchor * newAnchor)
{
    if ( newAnchor && newAnchor->GetMesh() )
        anchorMesh = newAnchor->GetMesh();
    anchor = newAnchor;
    return true;
}

bool psEffectObjSound::Update(csTicks elapsed)
{
    if (!sndSource) //If there was no sound why update?
        return true;
    if (!anchor || !anchor->IsReady()) // wait for anchor to be ready
        return true;

    life += (float)elapsed;
    while (life > animLength && killTime <= 0)
        life -= animLength;

    if (life >= birth && !isAlive)
    {
        isAlive = true;
        sndStream->ResetPosition();
        sndStream->Unpause();
    }

    csVector3 soundPos = anchorMesh->GetMovable()->GetPosition();

    if (keyFrames->GetSize() > 0)
    {
        currKeyFrame = FindKeyFrameByTime(life);
        nextKeyFrame = currKeyFrame + 1;
        if (nextKeyFrame >= keyFrames->GetSize())
            nextKeyFrame = 0;

        // position
        soundPos += LERP_VEC_KEY(KA_POS);

        // volume
        sndSource->SetVolume(LERP_KEY(KA_SCALE) * volumeMultiplier);
    }

    sndSource3d->SetPosition(view->GetCamera()->GetTransform().Other2This(soundPos));

    if (killTime <= 0)
        return true;

    killTime -= (int)elapsed;
    if (killTime <= 0)
        return false;

    return true;
}

psEffectObj *psEffectObjSound::Clone() const
{
    psEffectObjSound *newObj = new psEffectObjSound(view, renderer2d);
    CloneBase(newObj);

    // simp mesh specific
    newObj->soundName = soundName;

    newObj->minDistSquared = minDistSquared;
    newObj->maxDistSquared = maxDistSquared;

    newObj->loop = loop;

    newObj->soundmanager = soundmanager;
    newObj->sndData = sndData;
//    newObj->sndStream = sndStream;
//    newObj->sndSource = sndSource;

    return newObj;
}

bool psEffectObjSound::PostSetup()
{
    soundmanager =  csQueryRegistry<iSoundManager> (psCSSetup::object_reg);
    if ( soundmanager )
    {
        if ( !soundmanager->Setup() )
            return false;
    }
    else
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "No sound manager to load sound: %s\n", soundName.GetData());
        return false;
    }

    if (!sndData.IsValid())
    {
      sndData = soundmanager->GetSoundResource(soundName);
      if (!sndData.IsValid())
      {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Could not load sound: %s\n", soundName.GetData());
        return false;
      }
    }

    return true;
}
