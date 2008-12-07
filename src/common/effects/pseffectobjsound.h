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

#ifndef PS_EFFECT_OBJ_SOUND_HEADER
#define PS_EFFECT_OBJ_SOUND_HEADER

#include "pseffectobj.h"
#include "iclient/isoundmngr.h"

struct iSoundManager;
struct iSoundHandle;
struct iSoundSource;
class psEffect2DRenderer;

class psEffectObjSound : public psEffectObj
{
public:

    psEffectObjSound(iView * parentView, psEffect2DRenderer * renderer2d);
    ~psEffectObjSound();

    // inheritted function overloads
    bool Load(iDocumentNode *node, iLoaderContext* ldr_context);
    bool Render(const csVector3 &up);
    bool Update(csTicks elapsed);
    bool AttachToAnchor(psEffectAnchor * newAnchor);
    psEffectObj *Clone() const;


private:

    /** performs the post setup (after the effect obj has been loaded).
     *  Things like create mesh factory, etc are initialized here.
     */
    bool PostSetup();

    csString soundName;

    float minDistSquared;
    float maxDistSquared;

    float volumeMultiplier;
    bool loop;
    
    csRef<iSoundManager>        soundmanager;
    csRef<SOUND_DATA_TYPE>      sndData;
    csRef<SOUND_STREAM_TYPE>    sndStream;
    csRef<SOUND_SOURCE_TYPE>    sndSource;
    csRef<SOUND_SOURCE3D_TYPE>  sndSource3d;
};

#endif
