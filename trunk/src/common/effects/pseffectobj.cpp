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

#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iengine/camera.h>
#include <iengine/scenenode.h>
#include <cstool/csview.h>
#include <imesh/particles.h>
#include <csutil/flags.h>

#include "effects/pseffectobj.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/log.h"
#include "util/pscssetup.h"

// keep track of the interpolation types for each key frame
const int lerpTypes[psEffectObjKeyFrame::KA_COUNT] = { 
                            psEffectObjKeyFrame::IT_NONE,  /* BLANK */
                            psEffectObjKeyFrame::IT_LERP,  /* SCALE */
                            psEffectObjKeyFrame::IT_LERP,  /* TOP SCALE */
                            psEffectObjKeyFrame::IT_LERP,  /* POS_X */
                            psEffectObjKeyFrame::IT_LERP,  /* POS_Y */
                            psEffectObjKeyFrame::IT_LERP,  /* POS_Z */
                            psEffectObjKeyFrame::IT_LERP,  /* ROT_X */
                            psEffectObjKeyFrame::IT_LERP,  /* ROT_Y */
                            psEffectObjKeyFrame::IT_LERP,  /* ROT_Z */
                            psEffectObjKeyFrame::IT_LERP,  /* SPIN_X */
                            psEffectObjKeyFrame::IT_LERP,  /* SPIN_Y */
                            psEffectObjKeyFrame::IT_LERP,  /* SPIN_Z */
                            psEffectObjKeyFrame::IT_FLOOR, /* CELL */
                            psEffectObjKeyFrame::IT_LERP,  /* COLOUR_R */
                            psEffectObjKeyFrame::IT_LERP,  /* COLOUR_G */
                            psEffectObjKeyFrame::IT_LERP,  /* COLOUR_B */
                            psEffectObjKeyFrame::IT_LERP,  /* ALPHA */
                            psEffectObjKeyFrame::IT_LERP,  /* HEIGHT */
                            psEffectObjKeyFrame::IT_LERP,  /* PADDING */
                            psEffectObjKeyFrame::IT_FLOOR, /* ANIMATE */
    };

psEffectObjKeyFrame::psEffectObjKeyFrame() : specAction(KA_COUNT)
{
    specAction.Clear();
}

psEffectObjKeyFrame::psEffectObjKeyFrame(iDocumentNode *node, const psEffectObjKeyFrame *prevKeyFrame) : specAction(KA_COUNT)
{
    specAction.Clear();
    
    csRef<iDocumentNodeIterator> xmlbinds;
    
    time = atof(node->GetAttributeValue("time"));

    xmlbinds = node->GetNodes("action");
    csRef<iDocumentNode> keyNode;
    csRef<iDocumentAttribute> attr;
    
    if (!prevKeyFrame)
    {
        // this is the first frame, so pretend that everything has been set (if it hasn't, it will use the defaults)
        for (int a=0; a<KA_COUNT; ++a)
            specAction.SetBit(a);
    }

    // set default values, if these are wrong then they'll be replaced on the second (lerp) pass
    actions[KA_SCALE] = 1.0f;
    actions[KA_TOPSCALE] = 1.0f;
    actions[KA_POS_X] = 0;
    actions[KA_POS_Y] = 0;
    actions[KA_POS_Z] = 0;
    actions[KA_ROT_X] = 0.0f;
    actions[KA_ROT_Y] = 0.0f;
    actions[KA_ROT_Z] = 0.0f;
    actions[KA_SPIN_X] = 0;
    actions[KA_SPIN_Y] = 0;
    actions[KA_SPIN_Z] = 0;
    actions[KA_CELL] = 0;
    actions[KA_COLOUR_R] = 1;
    actions[KA_COLOUR_G] = 1;
    actions[KA_COLOUR_B] = 1;
    actions[KA_ALPHA] = 1.0f;
    actions[KA_HEIGHT] = 1.0f;
    actions[KA_PADDING] = 0.1f;
    actions[KA_ANIMATE] = 1;

    while (xmlbinds->HasNext())
    {
        keyNode = xmlbinds->Next();
        csString action = keyNode->GetAttributeValue("name");
        action.Downcase();
        
        if (action == "scale")
        {
            specAction.SetBit(KA_SCALE);
            actions[KA_SCALE] = keyNode->GetAttributeValueAsFloat("value");
            if (actions[KA_SCALE] == 0.0f)
                actions[KA_SCALE] = 0.001f;
        }
        else if (action == "volume")
        {
            specAction.SetBit(KA_SCALE);
            actions[KA_SCALE] = keyNode->GetAttributeValueAsFloat("value");
        }
        else if (action == "topscale")
        {
            specAction.SetBit(KA_TOPSCALE);
            actions[KA_TOPSCALE] = keyNode->GetAttributeValueAsFloat("value");
            if (actions[KA_TOPSCALE] == 0.0f)
                actions[KA_TOPSCALE] = 0.001f;
        }
        else if (action == "position")
        {
            attr = keyNode->GetAttribute("x");
            if (attr)
            {
                specAction.SetBit(KA_POS_X);
                actions[KA_POS_X] = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("y");
            if (attr)
            {
                specAction.SetBit(KA_POS_Y);
                actions[KA_POS_Y] = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("z");
            if (attr)
            {
                specAction.SetBit(KA_POS_Z);
                actions[KA_POS_Z] = attr->GetValueAsFloat();
            }
        }
        else if (action == "rotate")
        {
            attr = keyNode->GetAttribute("x");
            if (attr)
            {
                specAction.SetBit(KA_ROT_X);
                actions[KA_ROT_X] = attr->GetValueAsFloat() * PI / 180.0f;
            }
            attr = keyNode->GetAttribute("y");
            if (attr)
            {
                specAction.SetBit(KA_ROT_Y);
                actions[KA_ROT_Y] = attr->GetValueAsFloat() * PI / 180.0f;
            }
            attr = keyNode->GetAttribute("z");
            if (attr)
            {
                specAction.SetBit(KA_ROT_Z);
                actions[KA_ROT_Z] = attr->GetValueAsFloat() * PI / 180.0f;
            }
        }
        else if (action == "spin")
        {
            attr = keyNode->GetAttribute("x");
            if (attr)
            {
                specAction.SetBit(KA_SPIN_X);
                actions[KA_SPIN_X] = attr->GetValueAsFloat() * PI / 180.0f;
            }
            attr = keyNode->GetAttribute("y");
            if (attr)
            {
                specAction.SetBit(KA_SPIN_Y);
                actions[KA_SPIN_Y] = attr->GetValueAsFloat() * PI / 180.0f;
            }
            attr = keyNode->GetAttribute("z");
            if (attr)
            {
                specAction.SetBit(KA_SPIN_Z);
                actions[KA_SPIN_Z] = attr->GetValueAsFloat() * PI / 180.0f;
            }
        }
        else if (action == "cell")
        {
            specAction.SetBit(KA_CELL);
            actions[KA_CELL] = keyNode->GetAttributeValueAsInt("value") + 0.1f;
        }
        else if (action == "colour")
        {
            attr = keyNode->GetAttribute("r");
            if (attr)
            {
                specAction.SetBit(KA_COLOUR_R);
                actions[KA_COLOUR_R] = attr->GetValueAsFloat() / 255.0f;
            }
            attr = keyNode->GetAttribute("g");
            if (attr)
            {
                specAction.SetBit(KA_COLOUR_G);
                actions[KA_COLOUR_G] = attr->GetValueAsFloat() / 255.0f;
            }
            attr = keyNode->GetAttribute("b");
            if (attr)
            {
                specAction.SetBit(KA_COLOUR_B);
                actions[KA_COLOUR_B] = attr->GetValueAsFloat() / 255.0f;
            }
            attr = keyNode->GetAttribute("a");
            if (attr)
            {
                specAction.SetBit(KA_ALPHA);
                actions[KA_ALPHA] = attr->GetValueAsFloat() / 255.0f;
            }
        }
        else if (action == "alpha")
        {
            specAction.SetBit(KA_ALPHA);
            actions[KA_ALPHA] = keyNode->GetAttributeValueAsFloat("value") / 255.0f;
        }
        else if (action == "height")
        {
            specAction.SetBit(KA_HEIGHT);
            actions[KA_HEIGHT] = keyNode->GetAttributeValueAsFloat("value");
            if (actions[KA_HEIGHT] == 0.0f)
                actions[KA_HEIGHT] = 0.001f;
        }
        else if (action == "padding")
        {
            specAction.SetBit(KA_PADDING);
            actions[KA_PADDING] = keyNode->GetAttributeValueAsFloat("value");
        }
        else if (action == "animate")
        {
            specAction.SetBit(KA_ANIMATE);
            csString val = keyNode->GetAttributeValue("value");
            val.Downcase();
            if (val == "yes" || val == "true")
                actions[KA_ANIMATE] = 1;
            else
                actions[KA_ANIMATE] = -1;
        }
    }
}

psEffectObjKeyFrame::~psEffectObjKeyFrame()
{
}

psEffectObjKeyFrameGroup::psEffectObjKeyFrameGroup()
{
}

psEffectObjKeyFrameGroup::~psEffectObjKeyFrameGroup()
{
    keyFrames.DeleteAll();
}

psEffectObj::psEffectObj(iView *parentView, psEffect2DRenderer * renderer2d)
			: renderer2d(renderer2d)
{
    engine =  csQueryRegistry<iEngine> (psCSSetup::object_reg);
    stringSet = csQueryRegistryTagInterface<iShaderVarStringSet>(psCSSetup::object_reg,
      "crystalspace.shader.variablenameset");
    globalStringSet = csQueryRegistryTagInterface<iStringSet> 
            (psCSSetup::object_reg, "crystalspace.shared.stringset");
    view = parentView;

    killTime = -1;
    isAlive = true;
    birth = 0;
    
    zFunc = CS_ZBUF_TEST;
    
    anchor = 0;

    scale = 1.0f;
    aspect = 1.0f;

    effectsCollection = engine->GetCollection("effects");
    keyFrames.AttachNew(new psEffectObjKeyFrameGroup);

    baseScale = 1.0f;
}

psEffectObj::~psEffectObj()
{
    if (mesh)
    {
        if (anchorMesh && isAlive)
            mesh->QuerySceneNode()->SetParent(0);
        
        engine->RemoveObject(mesh);
    }
    // note that we don't delete the mesh factory since that is shared
    // between all instances of this effect object and will be cleaned up by
    // CS's smart pointer system
}

bool psEffectObj::Load(iDocumentNode *node, iLoaderContext* ldr_context)
{
    
    csRef<iDocumentNode> dataNode;
    
    // birth
    dataNode = node->GetNode("birth");
    if (dataNode)
    {
        birth = dataNode->GetContentsValueAsFloat();
        if (birth > 0)
            isAlive = false;
    }
    
    // death
    dataNode = node->GetNode("death");
    if (dataNode)
    {
        killTime = dataNode->GetContentsValueAsInt();
    }
    else
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Effect obj %s is missing a <death> tag.  If you want an infinite effect use <death>none</death>\n", name.GetData());
        return false; // effect is invalid without a death tag
    }
    
    // anchor
    dataNode = node->GetNode("attach");
    if (dataNode)
        SetAnchorName(dataNode->GetContentsValue());

    // render priority
    dataNode = node->GetNode("priority");
    if (dataNode)
        priority = engine->GetRenderPriority(dataNode->GetContentsValue());
    else
        priority = engine->GetAlphaRenderPriority();

    // z func
    if (node->GetNode("znone"))
        zFunc = CS_ZBUF_NONE;
    else if (node->GetNode("zfill"))
        zFunc = CS_ZBUF_FILL;
    else if (node->GetNode("zuse"))
        zFunc = CS_ZBUF_USE;
    else if (node->GetNode("ztest"))
        zFunc = CS_ZBUF_TEST;

    // mix mode
    dataNode = node->GetNode("mixmode");
    if (dataNode)
    {
        csString mix = dataNode->GetContentsValue();
        mix.Downcase();
        if (mix == "copy")
            mixmode = CS_FX_COPY;
        else if (mix == "mult")
            mixmode = CS_FX_MULTIPLY;
        else if (mix == "mult2")
            mixmode = CS_FX_MULTIPLY2;
        else if (mix == "alpha")
            mixmode = CS_FX_ALPHA;
        else if (mix == "transparent")
            mixmode = CS_FX_TRANSPARENT;
        else if (mix == "destalphaadd")
            mixmode = CS_FX_DESTALPHAADD;
        else if (mix == "srcalphaadd")
            mixmode = CS_FX_SRCALPHAADD;
        else if (mix == "premultalpha")
            mixmode = CS_FX_PREMULTALPHA;
        else
            mixmode = CS_FX_ADD;
    }
    else
        mixmode = CS_FX_ADD;
    
    // direction
    dataNode = node->GetNode("dir");
    if (dataNode)
    {
        csString dirName = dataNode->GetContentsValue();
        dirName.Downcase();
        if (dirName == "target")
            dir = DT_TARGET;
        else if (dirName == "origin")
            dir = DT_ORIGIN;
        else if (dirName == "totarget")
            dir = DT_TO_TARGET;
        else if (dirName == "camera")
            dir = DT_CAMERA;
        else if (dirName == "billboard")
            dir = DT_BILLBOARD;
        else
            dir = DT_NONE;
    }
    else
        dir = DT_NONE;

    // KEYFRAMES
    csRef<iDocumentNodeIterator> xmlbinds;
    xmlbinds = node->GetNodes("keyFrame");
    csRef<iDocumentNode> keyNode;

    animLength = 0;
    psEffectObjKeyFrame *prevKeyFrame = 0;
    while (xmlbinds->HasNext())
    {
        keyNode = xmlbinds->Next();
        psEffectObjKeyFrame * keyFrame = new psEffectObjKeyFrame(keyNode, prevKeyFrame);
        if (keyFrame->time > animLength)
            animLength = keyFrame->time;
        prevKeyFrame = keyFrame;
        keyFrames->Push(keyFrame);
    }
    animLength += 10;
    // TODO: sort keyframes by time here

    // linearly interpolate values where an action wasn't specified
    FillInLerps();

    return true;
}

bool psEffectObj::Render(const csVector3 &up)
{
    return false;
}

bool psEffectObj::SetScaling(float scale, float aspect)
{
    this->scale = scale;
    this->aspect = aspect;
    return true;
}

bool psEffectObj::Update(csTicks elapsed)
{
    if (!anchor || !anchor->IsReady() || !anchorMesh->GetMovable()->GetSectors()->GetCount()) // wait for anchor to be ready
        return true;

    const static csMatrix3 UP_FIX(1,0,0,   0,0,1,  0,1,0);
        
    life += (float)elapsed;
    while (life > animLength && killTime <= 0)
        life -= animLength;

    if (life >= birth && !isAlive)
    {
        isAlive = true;
//        if (anchorMesh)
//            anchorMesh->GetChildren()->Add(mesh);
    }

    if (isAlive && anchorMesh && anchor->IsReady())
    {
        mesh->GetMovable()->SetSector(anchorMesh->GetMovable()->GetSectors()->Get(0));
        mesh->GetMovable()->SetPosition(anchorMesh->GetMovable()->GetFullPosition());
        if (dir == DT_NONE)
            matBase = anchorMesh->GetMovable()->GetFullTransform().GetT2O();
    }

	const static csMatrix3 billboardFix = csXRotMatrix3(-3.14f/2.0f);
    if (keyFrames->GetSize() == 0)
    {
        if (dir == DT_CAMERA)
        {
            csVector3 camDir = -view->GetCamera()->GetTransform().GetO2TTranslation() 
                             + anchorMesh->GetMovable()->GetFullPosition();
            csReversibleTransform rt;
            rt.LookAt(camDir, csVector3(0.f,1.f,0.f));
            matBase = rt.GetT2O() * UP_FIX;
        }
        else if (dir == DT_BILLBOARD)
        {
            matBase = view->GetCamera()->GetTransform().GetT2O() * billboardFix;
        }
		if (scale != 1.0f)
			mesh->GetMovable()->SetTransform(matBase * (1.0f / scale));
		else
			mesh->GetMovable()->SetTransform(matBase);
		baseScale = scale;

        mesh->GetMovable()->UpdateMove();
    }
    else
    {
        currKeyFrame = FindKeyFrameByTime(life);
        nextKeyFrame = currKeyFrame + 1;
        if (nextKeyFrame >= keyFrames->GetSize())
            nextKeyFrame = 0;
       
        // grab and lerp values
        
        csVector3 lerpRot = LERP_VEC_KEY(KA_ROT);
        csVector3 lerpSpin = LERP_VEC_KEY(KA_SPIN);
        csVector3 objOffset = LERP_VEC_KEY(KA_POS);
        
        // calculate rotation from lerped values
        csMatrix3 matRot = csZRotMatrix3(lerpRot.z) * csYRotMatrix3(lerpRot.y) * csXRotMatrix3(lerpRot.x);
        if (dir != DT_CAMERA && dir != DT_BILLBOARD)
            matRot = matRot * matBase;

        // calculate new position
        csVector3 newPos = matRot * csVector3(-objOffset.x, objOffset.y, -objOffset.z);
        
        csMatrix3 matTransform;
        if (dir == DT_CAMERA)
        {
            csVector3 camDir = -view->GetCamera()->GetTransform().GetO2TTranslation() 
                             + anchorMesh->GetMovable()->GetFullPosition() + newPos;
            csReversibleTransform rt;
            rt.LookAt(camDir, csVector3(sinf(lerpSpin.y),cosf(lerpSpin.y),0.f));
            matBase = rt.GetT2O() * UP_FIX;
            
            newPos = rt.GetT2O() * newPos;
            // rotate and spin should have no effect on the transform when we want it to face the camera
            matTransform = matBase;
        }
        else if (dir == DT_BILLBOARD)
        {
            matBase = view->GetCamera()->GetTransform().GetT2O() * billboardFix;
            matTransform = matBase;
        }
        else
        {
            matTransform = matRot;
            matTransform *= csZRotMatrix3(lerpSpin.z) * csYRotMatrix3(lerpSpin.y) * csXRotMatrix3(lerpSpin.x);
        }

        // SCALE
        baseScale = (LERP_KEY(KA_SCALE) * scale);
        matTransform *= (1.0f / baseScale);

        // set new transform
        mesh->GetMovable()->SetTransform(matTransform);
    
        // set new position
        mesh->GetMovable()->SetPosition(anchorMesh->GetMovable()->GetFullPosition() + newPos);
        mesh->GetMovable()->UpdateMove();
    }
    
    if (killTime <= 0)
        return true; // this effect obj will last forever until some divine intervention

    // age the effect obj
    killTime -= (int)elapsed;
    if (killTime <= 0)
        return false;
    
    return true;
}

void psEffectObj::CloneBase(psEffectObj *newObj) const
{
    newObj->name = name;
    newObj->materialName = materialName;
    newObj->killTime = killTime;
    newObj->life = 0;
    newObj->animLength = animLength;
    newObj->anchorName = anchorName;
    newObj->meshFact = meshFact;
    newObj->objUp = objUp;
    newObj->matBase = matBase;
    newObj->matUp = matUp;
    newObj->birth = birth;
    newObj->isAlive = isAlive;
    newObj->zFunc = zFunc;
    newObj->dir = dir;
    newObj->priority = priority;
    newObj->mixmode = mixmode;

    // the group of keyFrames remains static (don't clone)
    newObj->keyFrames = keyFrames;
}

psEffectObj *psEffectObj::Clone() const
{
    psEffectObj *newObj = new psEffectObj(view, renderer2d);
    CloneBase(newObj);

    return newObj; 
}

bool psEffectObj::AttachToAnchor(psEffectAnchor * newAnchor)
{
    if (mesh && newAnchor->GetMesh())
    {
        anchor = newAnchor;
        anchorMesh = anchor->GetMesh();

        /*
        if (isAlive)
            anchorMesh->GetChildren()->Add(mesh);
        anchorMesh->GetMovable()->UpdateMove();
        mesh->GetMovable()->SetPosition(csVector3(0,0,0));
        mesh->GetMovable()->UpdateMove();
        */
        return true;
    }
    return false;
}

size_t psEffectObj::FindKeyFrameByTime(float time) const
{
    for ( size_t a = keyFrames->GetSize(); a-- > 0; )
    {
        if (keyFrames->Get(a)->time < time)
            return a;
    }
    return 0;
}

bool psEffectObj::FindNextKeyFrameWithAction(size_t startFrame, size_t action, size_t &index) const
{
    for (size_t a = startFrame; a < keyFrames->GetSize(); a++)
    {
        if (keyFrames->Get(a)->specAction.IsBitSet(action))
        {
            index = a;
            return true;
        }
    }
    return false;
}

void psEffectObj::FillInLerps()
{
    // this code is crap, but doing it this way allows everything else to be decently nice, clean, and efficient
    size_t a,b,nextIndex;

    for (size_t k=0; k<psEffectObjKeyFrame::KA_COUNT; ++k)
    {
        a = 0;
        while (FindNextKeyFrameWithAction(a+1, k, nextIndex))
        {
            for (b=a+1; b<nextIndex; ++b)
            {
                switch(lerpTypes[k])
                {
                    case psEffectObjKeyFrame::IT_FLOOR:
                        keyFrames->Get(b)->actions[k] = keyFrames->Get(a)->actions[k];
                        break;
                    case psEffectObjKeyFrame::IT_CEILING:
                        keyFrames->Get(b)->actions[k] = keyFrames->Get(nextIndex)->actions[k];
                        break;
                    case psEffectObjKeyFrame::IT_LERP:
                        float lerpVal = lerp(keyFrames->Get(a)->actions[k], keyFrames->Get(nextIndex)->actions[k], 
                                             keyFrames->Get(a)->time,
                                             keyFrames->Get(nextIndex)->time,
                                             keyFrames->Get(b)->time);
                        keyFrames->Get(b)->actions[k] = lerpVal;
                        break;
                }
            }
            a = nextIndex;
        }
        
        // no matter what the interpolation type (as long as we have one), just clamp the end
        if (lerpTypes[k] != psEffectObjKeyFrame::IT_NONE)
        {
            for (b = a + 1; b < keyFrames->GetSize(); ++b)
                keyFrames->Get(b)->actions[k] = keyFrames->Get(a)->actions[k];
        }
    }
}

csMatrix3 psEffectObj::BuildRotMatrix(const csVector3 &up) const
{
    csVector3 forward = csVector3(0, 0, 1);
    csVector3 right = up % forward;
    return csMatrix3(right.x, right.y, right.z, up.x, up.y, up.z, forward.x, forward.y, forward.z);
}

void psEffectObj::Show(bool value)
{
    if(mesh)
    {
        if(value)
            mesh->GetFlags().Reset(CS_ENTITY_INVISIBLEMESH);
        else
            mesh->GetFlags().Set(CS_ENTITY_INVISIBLEMESH);
    }
}
