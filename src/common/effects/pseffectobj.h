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

#ifndef PS_EFFECT_OBJ_HEADER
#define PS_EFFECT_OBJ_HEADER

#include <csgfx/shadervar.h>
#include <csutil/csstring.h>
#include <csutil/array.h>
#include <csutil/bitarray.h>
#include <csutil/parray.h>
#include <csutil/refcount.h>
#include <csutil/leakguard.h>
#include <iutil/virtclk.h>
#include <imesh/object.h>
#include <csgeom/matrix3.h>
#include <csgeom/vector3.h>


struct iDocumentNode;
struct iEngine;
struct iView;
struct iMeshFactoryWrapper;
struct iSector;
struct iCollection;
class psEffect2DRenderer;

class psEffectAnchor;

/**
 * Stores data for a specific effect object keyframe
 */
class psEffectObjKeyFrame
{
public:
    psEffectObjKeyFrame();
    psEffectObjKeyFrame(iDocumentNode *node, const psEffectObjKeyFrame *prevKeyFrame);
    ~psEffectObjKeyFrame();

    /// this is the time of the keyframe animation (in seconds)
    float time;

    enum INTERP_TYPE
    {
        IT_NONE = 0,
        IT_FLOOR,
        IT_CEILING,
        IT_LERP,
        
        IT_COUNT
    };
    
    enum KEY_ACTION
    {
        KA_SCALE = 1,
        KA_TOPSCALE,
        KA_POS_X,
        KA_POS_Y,
        KA_POS_Z,
        KA_ROT_X,
        KA_ROT_Y,
        KA_ROT_Z,
        KA_SPIN_X,
        KA_SPIN_Y,
        KA_SPIN_Z,
        KA_CELL,
        KA_COLOUR_R, KA_COLOUR_X = KA_COLOUR_R,
        KA_COLOUR_G, KA_COLOUR_Y = KA_COLOUR_G,
        KA_COLOUR_B, KA_COLOUR_Z = KA_COLOUR_B,
        KA_ALPHA,
        KA_HEIGHT,
        KA_PADDING,
        KA_ANIMATE,

        KA_COUNT
    };

    float actions[KA_COUNT];

    /// keep track of which actions were specified for which 
    csBitArray specAction;
};

/**
 * Effect objects KeyFrame group.
 */
class psEffectObjKeyFrameGroup : public csRefCount
{
private:
    csPDelArray<psEffectObjKeyFrame> keyFrames;
    
public:
    psEffectObjKeyFrameGroup();
    ~psEffectObjKeyFrameGroup();

    /** returns the number of keyframes in the group.
     *   @return the keyframe count
     */
    size_t GetSize() const
    { return keyFrames.GetSize(); }

    /** returns the keyframe at the given index.
     *   @param idx the index of the keyframe to grab
     *   @return the keyframe
     */
    psEffectObjKeyFrame * Get(size_t idx) const
    { return keyFrames[idx]; }

    /** returns the keyframe at the given index.
     *   @param idx the index of the keyframe to grab
     *   @return the keyframe
     */
    psEffectObjKeyFrame * operator [] (size_t idx) const
    { return keyFrames[idx]; }
    
    /** pushes a keyframe onto the group.
     *   @param keyFrame the keyframe to push
     */
    void Push(psEffectObjKeyFrame * keyFrame)
    { keyFrames.Push(keyFrame); }

    /** deletes the keyframe at the given index.
     *   @param idx the index of the keyframe to delete
     */
    void DeleteIndex(size_t idx)
    { keyFrames.DeleteIndex(idx); }

    /** deletes all of the keyframes in this group.
     */
    void DeleteAll()
    { keyFrames.DeleteAll(); }
};

/**
 * An effect is not much more than a collection of effect objects.
 * Effect objects aren't much more than a set of initial attributes
 * and a collection of key frames to dictate behaviour
 */
class psEffectObj
{
public:
    psEffectObj(iView * parentView, psEffect2DRenderer * renderer2d);
    virtual ~psEffectObj();
    
    /** loads the effect object from an xml node
     *   @param node the xml node containing the effect object, must be valid
     *   @return true on success, false otherwise
     */
    virtual bool Load(iDocumentNode *node, iLoaderContext* ldr_context);

    /** renders the effect
     *   @param up the base up vector of the effect obj
     *   @return true on success
     */
    virtual bool Render(const csVector3 &up);

    /** If the obj supports it, sets the scaling parameters.
     */
    virtual bool SetScaling(float scale, float aspect);

    /** Updates the spell effect -- called every frame.
     *   @param elapsed the ticks elapsed since last update
     *   @return false if the obj is useless and can be removed
     */
    virtual bool Update(csTicks elapsed);

    /** Convenience function to clone the base member variables
     *   @param newObj reference to the new object that will contain the cloned variables
     */
    virtual void CloneBase(psEffectObj *newObj) const;

    /** Clones the effect object.  This will almost always be overloaded.
     */
    virtual psEffectObj *Clone() const;

    /** Attaches this mesh to the given effect anchor.
     *   @param newAnchor The effect anchor to attach this mesh to.
     *   @return true If it attached properly, false otherwise.
     */
    virtual bool AttachToAnchor(psEffectAnchor * newAnchor);

    /** Shows or hides an object
     *   @param show or hide (true = show, false = hide)
     */
    virtual void Show(bool value);

    /** gets the time left that the effect obj has to live
     *   @return the current kill time
     */
    int GetKillTime() const { return killTime; }

    /** sets the new time left that the effect obj has to live
     *   @param newKillTime the new kill time
     */
    void SetKillTime(int newKillTime) { killTime = newKillTime; }

    /** Sets the base rotation matrix of the effect obj
     *   @param rot the base rotation matrix of the angle
     */
    void SetRotBase(const csMatrix3 & newRotBase)
    {
        if (dir == DT_TO_TARGET)
            matBase = newRotBase;
    }
    
    /** Sets the position rotation of the effect obj.
     *   @param newPosTransf the new position rotation.
     */
    void SetPosition(const csMatrix3 & newPosTransf)
    {
        if (dir == DT_ORIGIN)
            matBase = newPosTransf;
    }

    /** Sets the target rotation of the effect obj.
     *   @param newTargetTransf the new target rotation.
     */
    void SetTarget(const csMatrix3 & newTargetTransf)
    {
        if (dir == DT_TARGET)
            matBase = newTargetTransf;
    }

    /** Sets the name of the anchor that this effect obj is attached to.
     *   @param anchor the new name of the anchor that this obj is attached to
     */
    void SetAnchorName(const csString & anchor) { anchorName = anchor; }

    /** Gets the name of the anchor that this effect obj is attached to.
     *   @return The name of the anchor that this effect obj is attached to.
     */
    const csString & GetAnchorName() const { return anchorName; }
   
    /** Accessor function to get the animation length of this effect obj.
     *   @return the animation length of this effect obj
     */
    float GetAnimLength() const { return animLength; }

    /** Gets the name of this effect obj.
     *   @return the name of this effect obj.
     */
    csString GetName() const { return name; }
    
    /** returns the number of keyframes in this obj.
     *   @return the keyFrame count
     */
    size_t GetKeyFrameCount() const
    { return keyFrames->GetSize(); }

    /** returns the keyframe at the given index.
     *   @param idx the index of the keyframe to grab
     *   @return the keyframe at the given index
     */
    psEffectObjKeyFrame * GetKeyFrame(size_t idx) const
    { return keyFrames->Get(idx); }

    enum DIR_TYPE
    {
        DT_NONE = 0,
        DT_ORIGIN,
        DT_TARGET,
        DT_TO_TARGET,
        DT_CAMERA,
        DT_BILLBOARD,
        
        DT_COUNT
    };

protected:

    /** finds the index of the keyFrame at the specified time
     *   @param time the time to lookup
     *   @return the index of the keyFrame at the specified time
     */
    size_t FindKeyFrameByTime(float time) const;

    /** finds the next key frame where the specific action is specified
     *   @param startFrame the first frame to start looking
     *   @param action the action to look for
     *   @param index a container to store the index of the found key frame
     *   @return true if it found one, false otherwise
     */
    bool FindNextKeyFrameWithAction(size_t startFrame, size_t action, size_t &index) const;

    /** linearly interpolates keyFrame values for actions that weren't specified in a certain key frame
     */
    void FillInLerps();

    /** builds a rotation matrix given an up vector (yaw is assumed to be 0)
     *   @param up the up (unit) vector
     *   @return a rotation matrix representing the up vector
     */
    csMatrix3 BuildRotMatrix(const csVector3 &up) const;

    csRef<iShaderVarStringSet> stringSet;

    csString name;
    csString materialName;

    int killTime;
    float life;
    float animLength;
    
    // the effect anchor that this obj is attached to
    csString anchorName;
    csRef<iMeshWrapper> anchorMesh;
    psEffectAnchor * anchor;

    csVector3 target;

    csVector3 objUp;
    csMatrix3 matBase;
    csMatrix3 matUp;
    
    csRef<iMeshFactoryWrapper> meshFact;
    csRef<iMeshWrapper> mesh;
    
    float birth;
    bool isAlive;
    float baseScale;

    psEffect2DRenderer * renderer2d;

    csZBufMode zFunc;
    long priority;
    unsigned int mixmode;

    // direction
    int dir;
    
    // used for the update loop
    size_t currKeyFrame;
    size_t nextKeyFrame;

    //csArray<psEffectObjKeyFrame> keyFrames;
    csRef<psEffectObjKeyFrameGroup> keyFrames;
    
    // CS references
    csRef<iEngine> engine;
    csRef<iView> view;
    csRef<iStringSet> globalStringSet;
    
    /// region to store the CS objects
    csRef<iCollection> effectsCollection;

    float scale;
    float aspect;

    inline float lerp(float f1, float f2, float t1, float t2, float t)
    {
        if (t2 == t1)
            return f1;
        
        return f1 + (f2-f1)*(t-t1)/(t2-t1);
    }
    
    inline csVector3 lerpVec(const csVector3 &v1, const csVector3 &v2, float t1, float t2, float t)
    {
        if (t2 == t1)
            return v1;
        
        return v1 + (v2-v1)*(t-t1)/(t2-t1);
    }
};

#define LERP_KEY(action) \
    lerp(keyFrames->Get(currKeyFrame)->actions[psEffectObjKeyFrame::action], \
         keyFrames->Get(nextKeyFrame)->actions[psEffectObjKeyFrame::action], \
         keyFrames->Get(currKeyFrame)->time, \
         keyFrames->Get(nextKeyFrame)->time, \
         life)

#define LERP_VEC_KEY(action) \
    lerpVec( \
        csVector3(keyFrames->Get(currKeyFrame)->actions[psEffectObjKeyFrame::action##_X], \
                  keyFrames->Get(currKeyFrame)->actions[psEffectObjKeyFrame::action##_Y], \
                  keyFrames->Get(currKeyFrame)->actions[psEffectObjKeyFrame::action##_Z]), \
        csVector3(keyFrames->Get(nextKeyFrame)->actions[psEffectObjKeyFrame::action##_X], \
                  keyFrames->Get(nextKeyFrame)->actions[psEffectObjKeyFrame::action##_Y], \
                  keyFrames->Get(nextKeyFrame)->actions[psEffectObjKeyFrame::action##_Z]), \
        keyFrames->Get(currKeyFrame)->time, \
        keyFrames->Get(nextKeyFrame)->time, \
        life)



#endif
