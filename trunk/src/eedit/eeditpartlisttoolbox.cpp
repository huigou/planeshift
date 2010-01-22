/*
 * Author: Jorrit Tyberghein
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "eeditpartlisttoolbox.h"
#include "eeditglobals.h"

#include <csutil/scanstr.h>
#include <csutil/plugmgr.h>
#include <csutil/xmltiny.h>
#include <iutil/object.h>
#include <iutil/document.h>
#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <imesh/object.h>
#include <imesh/particles.h>
#include <imap/writer.h>

#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawsbutton.h"

//---------------------------------------------------------------------------------------

class ParticleParameterRow
{
protected:
    csString name;

public:
    ParticleParameterRow(const char* name) : name (name) { }
    virtual ~ParticleParameterRow() { }

    virtual csString GetRowName() { return name; }
    virtual csString GetRowDescription() = 0;

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb) = 0;
    virtual void FillParticleEditor(EEditParticleListToolbox* tb) = 0;
};

class ParticleParameterFloatRow : public ParticleParameterRow
{
private:
    float Min, Max, Step;

public:
    ParticleParameterFloatRow(const char* name, float Min=0, float Max=1, float Step=.1)
	: ParticleParameterRow(name), Min(Min), Max(Max), Step(Step) { }

    virtual float GetValue() = 0;
    virtual void SetValue(float v) = 0;

    virtual csString GetRowDescription()
    {
    	csString valueString;
	valueString.Format("%g", GetValue());
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float value = tb->valueNumSpinBox->GetValue();
	SetValue(value);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(Min, Max, Step);
	tb->valueNumSpinBox->Show();
	tb->valueNumSpinBox->SetValue (GetValue());
    }
};

class PPEmitFloat : public ParticleParameterFloatRow
{
protected:
    csRef<iParticleEmitter> emit;
public:
    PPEmitFloat (iParticleEmitter* emit, const char* name, float Min, float Max, float Step)
	: ParticleParameterFloatRow(name, Min, Max, Step), emit(emit) { }
};

class PPStartTime : public PPEmitFloat
{
public:
    PPStartTime (iParticleEmitter* emit) : PPEmitFloat(emit, "StartTime", 0, 200, .5) { }
    virtual float GetValue() { return emit->GetStartTime(); }
    virtual void SetValue(float v) { emit->SetStartTime(v); }
};

class PPDuration : public PPEmitFloat
{
public:
    PPDuration (iParticleEmitter* emit) : PPEmitFloat(emit, "Duration", 0, 1000000000, 1) { }
    virtual float GetValue() { return emit->GetDuration(); }
    virtual void SetValue(float v) { emit->SetDuration(v); }
};

class PPEmitRate : public PPEmitFloat
{
public:
    PPEmitRate (iParticleEmitter* emit) : PPEmitFloat(emit, "EmitRate", 0, 100000, 1) { }
    virtual float GetValue() { return emit->GetEmissionRate(); }
    virtual void SetValue(float v) { emit->SetEmissionRate(v); }
};

class PPSphereRadius : public ParticleParameterFloatRow
{
protected:
    csRef<iParticleBuiltinEmitterSphere> sphere;
public:
    PPSphereRadius (iParticleBuiltinEmitterSphere* sphere)
	: ParticleParameterFloatRow("SRadius", 0, 1000, .1), sphere(sphere) { }
    virtual float GetValue() { return sphere->GetRadius(); }
    virtual void SetValue(float v) { sphere->SetRadius(v); }
};

class PPConeAngle : public ParticleParameterFloatRow
{
protected:
    csRef<iParticleBuiltinEmitterCone> cone;
public:
    PPConeAngle (iParticleBuiltinEmitterCone* cone)
	: ParticleParameterFloatRow("CAngle", 0, 3.1415926, .1), cone(cone) { }
    virtual float GetValue() { return cone->GetConeAngle(); }
    virtual void SetValue(float v) { cone->SetConeAngle(v); }
};

class PPCylinderRadius : public ParticleParameterFloatRow
{
protected:
    csRef<iParticleBuiltinEmitterCylinder> cylinder;
public:
    PPCylinderRadius (iParticleBuiltinEmitterCylinder* cylinder)
	: ParticleParameterFloatRow("CRadius", 0, 1000, .1), cylinder(cylinder) { }
    virtual float GetValue() { return cylinder->GetRadius(); }
    virtual void SetValue(float v) { cylinder->SetRadius(v); }
};

class ParticleParameterMinMaxRow : public ParticleParameterRow
{
private:
    float Min, Max, Step;

public:
    ParticleParameterMinMaxRow(const char* name, float Min=0, float Max=1, float Step=.1)
	: ParticleParameterRow(name), Min(Min), Max(Max), Step(Step) { }

    virtual float GetMinValue() = 0;
    virtual void SetMinValue(float v) = 0;
    virtual float GetMaxValue() = 0;
    virtual void SetMaxValue(float v) = 0;

    virtual csString GetRowDescription()
    {
    	csString valueString;
	valueString.Format ("%g , %g", GetMinValue(), GetMaxValue());
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float value1 = tb->valueNumSpinBox->GetValue();
	float value2 = tb->value2NumSpinBox->GetValue();
	SetMinValue(value1);
	SetMaxValue(value2);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(Min, Max, Step);
	tb->value2NumSpinBox->SetRange(Min, Max, Step);
	tb->valueNumSpinBox->Show();
	tb->value2NumSpinBox->Show();
	tb->valueNumSpinBox->SetValue(GetMinValue());
	tb->value2NumSpinBox->SetValue(GetMaxValue());
    }
};

class PPTTL : public ParticleParameterMinMaxRow
{
protected:
    csRef<iParticleEmitter> emit;
public:
    PPTTL (iParticleEmitter* emit) : ParticleParameterMinMaxRow("TTL", 0, 1000, .1), emit(emit) { }
    virtual float GetMinValue()
    {
	float ttlmin, ttlmax;
	emit->GetInitialTTL (ttlmin, ttlmax);
	return ttlmin;
    }
    virtual void SetMinValue(float v)
    {
	float ttlmin, ttlmax;
	emit->GetInitialTTL (ttlmin, ttlmax);
	emit->SetInitialTTL (v, ttlmax);
    }
    virtual float GetMaxValue()
    {
	float ttlmin, ttlmax;
	emit->GetInitialTTL (ttlmin, ttlmax);
	return ttlmax;
    }
    virtual void SetMaxValue(float v)
    {
	float ttlmin, ttlmax;
	emit->GetInitialTTL (ttlmin, ttlmax);
	emit->SetInitialTTL (ttlmin, v);
    }
};

class PPInitialMass : public ParticleParameterMinMaxRow
{
protected:
    csRef<iParticleEmitter> emit;
public:
    PPInitialMass (iParticleEmitter* emit) : ParticleParameterMinMaxRow("Mass", 0, 1000, .1), emit(emit) { }
    virtual float GetMinValue()
    {
	float massmin, massmax;
	emit->GetInitialMass (massmin, massmax);
	return massmin;
    }
    virtual void SetMinValue(float v)
    {
	float massmin, massmax;
	emit->GetInitialMass (massmin, massmax);
	emit->SetInitialMass (v, massmax);
    }
    virtual float GetMaxValue()
    {
	float massmin, massmax;
	emit->GetInitialMass (massmin, massmax);
	return massmax;
    }
    virtual void SetMaxValue(float v)
    {
	float massmin, massmax;
	emit->GetInitialMass (massmin, massmax);
	emit->SetInitialMass (massmin, v);
    }
};

class ParticleParameterVector3Row : public ParticleParameterRow
{
private:
    float Min, Max, Step;

public:
    ParticleParameterVector3Row(const char* name, float Min=0, float Max=1, float Step=.1)
	: ParticleParameterRow(name), Min(Min), Max(Max), Step(Step) { }

    virtual csVector3 GetVector() = 0;
    virtual void SetVector(const csVector3& v) = 0;

    virtual csString GetRowDescription()
    {
    	csString valueString;
	const csVector3& v = GetVector();
	valueString.Format ("%g , %g , %g", v.x, v.y, v.z);
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float x = tb->valueNumSpinBox->GetValue();
	float y = tb->value2NumSpinBox->GetValue();
	float z = tb->value3NumSpinBox->GetValue();
	SetVector(csVector3(x,y,z));
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(Min, Max, Step);
	tb->value2NumSpinBox->SetRange(Min, Max, Step);
	tb->value3NumSpinBox->SetRange(Min, Max, Step);
	tb->valueNumSpinBox->Show();
	tb->value2NumSpinBox->Show();
	tb->value3NumSpinBox->Show();
	csVector3 v = GetVector();
	tb->valueNumSpinBox->SetValue(v.x);
	tb->value2NumSpinBox->SetValue(v.y);
	tb->value3NumSpinBox->SetValue(v.z);
    }
};

class ParticleParameterBoxVector3Row : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEmitterBox> box;
public:
    ParticleParameterBoxVector3Row (const char* name, iParticleBuiltinEmitterBox* box)
	: ParticleParameterVector3Row (name, -100000, 100000, .1), box(box) { }
};

class PPBoxDir1 : public ParticleParameterBoxVector3Row
{
public:
    PPBoxDir1 (iParticleBuiltinEmitterBox* box)
	: ParticleParameterBoxVector3Row ("Dir1", box) { }
    virtual csVector3 GetVector()
    {
	const csOBB& obb = box->GetBox();
	return obb.GetMatrix().Row1();
    }
    virtual void SetVector(const csVector3& v)
    {
	const csOBB& obb = box->GetBox();
	csOBB newObb (obb);
	newObb.GetMatrix().m11 = v.x;
	newObb.GetMatrix().m12 = v.y;
	newObb.GetMatrix().m13 = v.z;
	box->SetBox (newObb);
    }
};

class PPBoxDir2 : public ParticleParameterBoxVector3Row
{
public:
    PPBoxDir2 (iParticleBuiltinEmitterBox* box)
	: ParticleParameterBoxVector3Row ("Dir2", box) { }
    virtual csVector3 GetVector()
    {
	const csOBB& obb = box->GetBox();
	return obb.GetMatrix().Row2();
    }
    virtual void SetVector(const csVector3& v)
    {
	const csOBB& obb = box->GetBox();
	csOBB newObb (obb);
	newObb.GetMatrix().m21 = v.x;
	newObb.GetMatrix().m22 = v.y;
	newObb.GetMatrix().m23 = v.z;
	box->SetBox (newObb);
    }
};

class PPBoxDir3 : public ParticleParameterBoxVector3Row
{
public:
    PPBoxDir3 (iParticleBuiltinEmitterBox* box)
	: ParticleParameterBoxVector3Row ("Dir3", box) { }
    virtual csVector3 GetVector()
    {
	const csOBB& obb = box->GetBox();
	return obb.GetMatrix().Row3();
    }
    virtual void SetVector(const csVector3& v)
    {
	const csOBB& obb = box->GetBox();
	csOBB newObb (obb);
	newObb.GetMatrix().m31 = v.x;
	newObb.GetMatrix().m32 = v.y;
	newObb.GetMatrix().m33 = v.z;
	box->SetBox (newObb);
    }
};

class PPBoxTopLeft : public ParticleParameterBoxVector3Row
{
public:
    PPBoxTopLeft (iParticleBuiltinEmitterBox* box)
	: ParticleParameterBoxVector3Row ("TopLeft", box) { }
    virtual csVector3 GetVector()
    {
	const csOBB& obb = box->GetBox();
	return obb.Min();
    }
    virtual void SetVector(const csVector3& v)
    {
	const csOBB& obb = box->GetBox();
	csOBB newObb (obb);
	newObb.Set (v, obb.Max());
	box->SetBox (newObb);
    }
};

class PPBoxBotRight : public ParticleParameterBoxVector3Row
{
public:
    PPBoxBotRight (iParticleBuiltinEmitterBox* box)
	: ParticleParameterBoxVector3Row ("BotRight", box) { }
    virtual csVector3 GetVector()
    {
	const csOBB& obb = box->GetBox();
	return obb.Max();
    }
    virtual void SetVector(const csVector3& v)
    {
	const csOBB& obb = box->GetBox();
	csOBB newObb (obb);
	newObb.Set (obb.Min(), v);
	box->SetBox (newObb);
    }
};

class PPAcceleration : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEffectorForce> force;
public:
    PPAcceleration (iParticleBuiltinEffectorForce* force)
	: ParticleParameterVector3Row ("Acceleration", -1000, 1000, .1), force(force) { }
    virtual csVector3 GetVector() { return force->GetAcceleration(); }
    virtual void SetVector(const csVector3& v) { force->SetAcceleration(v); }
};

class PPForce : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEffectorForce> force;
public:
    PPForce (iParticleBuiltinEffectorForce* force)
	: ParticleParameterVector3Row ("Force", -1000, 1000, .1), force(force) { }
    virtual csVector3 GetVector() { return force->GetForce(); }
    virtual void SetVector(const csVector3& v) { force->SetForce(v); }
};

class PPRandomAcceleration : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEffectorForce> force;
public:
    PPRandomAcceleration (iParticleBuiltinEffectorForce* force)
	: ParticleParameterVector3Row ("RandomAcc", -1000, 1000, .1), force(force) { }
    virtual csVector3 GetVector() { return force->GetRandomAcceleration(); }
    virtual void SetVector(const csVector3& v) { force->SetRandomAcceleration(v); }
};

class PPEmitPos : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEmitterBase> emitBase;
public:
    PPEmitPos (iParticleBuiltinEmitterBase* emitBase)
	: ParticleParameterVector3Row ("Pos", -1000, 1000, .1), emitBase(emitBase) { }
    virtual csVector3 GetVector() { return emitBase->GetPosition(); }
    virtual void SetVector(const csVector3& v) { emitBase->SetPosition(v); }
};

class PPEmitLinVel : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEmitterBase> emitBase;
public:
    PPEmitLinVel (iParticleBuiltinEmitterBase* emitBase)
	: ParticleParameterVector3Row ("VelLinear", -1000, 1000, .1), emitBase(emitBase) { }
    virtual csVector3 GetVector()
    {
	csVector3 linear, angular;
	emitBase->GetInitialVelocity(linear, angular);
	return linear;
    }
    virtual void SetVector(const csVector3& v)
    {
	csVector3 linear, angular;
	emitBase->GetInitialVelocity(linear, angular);
	emitBase->SetInitialVelocity(v, angular);
    }
};

class PPEmitAngVel : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEmitterBase> emitBase;
public:
    PPEmitAngVel (iParticleBuiltinEmitterBase* emitBase)
	: ParticleParameterVector3Row ("VelAngul", -1000, 1000, .1), emitBase(emitBase) { }
    virtual csVector3 GetVector()
    {
	csVector3 linear, angular;
	emitBase->GetInitialVelocity(linear, angular);
	return angular;
    }
    virtual void SetVector(const csVector3& v)
    {
	csVector3 linear, angular;
	emitBase->GetInitialVelocity(linear, angular);
	emitBase->SetInitialVelocity(linear, v);
    }
};

class PPConeExtent : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEmitterCone> cone;
public:
    PPConeExtent (iParticleBuiltinEmitterCone* cone)
	: ParticleParameterVector3Row ("CExtent", -1000, 1000, .1), cone(cone) { }
    virtual csVector3 GetVector() { return cone->GetExtent(); }
    virtual void SetVector(const csVector3& v) { cone->SetExtent(v); }
};

class PPCylinderExtent : public ParticleParameterVector3Row
{
protected:
    csRef<iParticleBuiltinEmitterCylinder> cylinder;
public:
    PPCylinderExtent (iParticleBuiltinEmitterCylinder* cylinder)
	: ParticleParameterVector3Row ("CExtent", -1000, 1000, .1), cylinder(cylinder) { }
    virtual csVector3 GetVector() { return cylinder->GetExtent(); }
    virtual void SetVector(const csVector3& v) { cylinder->SetExtent(v); }
};

#if 0
class PPLinColCount : public ParticleParameterRow
{
private:
    csRef<iParticleBuiltinEffectorLinColor> lin;

public:
    PPLinColCount(iParticleBuiltinEffectorLinColor* lin)
	: ParticleParameterRow("LinCol: #"), lin(lin) { }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	valueString.Format("0");
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	//float value = tb->valueNumSpinBox->GetValue(); @@@ TTL not yet supported.
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
    }
};
#endif

class PPLinCol : public ParticleParameterRow
{
private:
    csRef<iParticleBuiltinEffectorLinColor> lin;
    size_t index;

public:
    PPLinCol(iParticleBuiltinEffectorLinColor* lin, size_t index)
	: ParticleParameterRow("LinCol: "), lin(lin), index(index)
    {
	name += index;
    }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	csColor4 color;
	float ttl;
	lin->GetColor(index,color,ttl);
	valueString.Format("%g (%g , %g , %g , %g)", ttl, color.red, color.green, color.blue, color.alpha);
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float value = tb->valueNumSpinBox->GetValue();
	float r = tb->valueScroll1->GetCurrentValue();
	float g = tb->valueScroll2->GetCurrentValue();
	float b = tb->valueScroll3->GetCurrentValue();
	float a = tb->valueScroll4->GetCurrentValue();
	lin->SetColor(index,csColor4(r,g,b,a));
	lin->SetEndTTL(index,value);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	csColor4 color;
	float ttl;
	lin->GetColor(index,color,ttl);
	tb->valueNumSpinBox->SetRange(0, 1000000, .1);
	tb->valueScroll1->SetMinValue(0);
	tb->valueScroll1->SetMaxValue(1);
	tb->valueScroll2->SetMinValue(0);
	tb->valueScroll2->SetMaxValue(1);
	tb->valueScroll3->SetMinValue(0);
	tb->valueScroll3->SetMaxValue(1);
	tb->valueScroll4->SetMinValue(0);
	tb->valueScroll4->SetMaxValue(1);
	tb->valueNumSpinBox->SetValue (ttl);
	tb->valueScroll1->SetCurrentValue(color.red, false, false);
	tb->valueScroll2->SetCurrentValue(color.green, false, false);
	tb->valueScroll3->SetCurrentValue(color.blue, false, false);
	tb->valueScroll4->SetCurrentValue(color.alpha, false, false);

	tb->valueNumSpinBox->Show();
	tb->valueScroll1->Show();
	tb->valueScroll2->Show();
	tb->valueScroll3->Show();
	tb->valueScroll4->Show();
    }
};

class ParticleParameterChoicesRow : public ParticleParameterRow
{
private:
    csStringArray choices;

public:
    ParticleParameterChoicesRow(const char* name) : ParticleParameterRow(name) { }
    void AddChoice (const char* choice)
    {
	choices.Push(choice);
    }

    virtual csString GetCurrentChoice() = 0;
    virtual void SetCurrentChoice(const csString& str) = 0;

    virtual csString GetRowDescription()
    {
    	return GetCurrentChoice();
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	csString value = tb->valueChoices->GetSelectedRowString();
	SetCurrentChoice(value);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueChoices->Clear();
	for (size_t i = 0 ; i < choices.GetSize() ; i++)
	    tb->valueChoices->NewOption(choices[i]);
	tb->valueChoices->Show();
	tb->valueChoices->Select(GetCurrentChoice());
    }
};

class PPPartPlacement : public ParticleParameterChoicesRow
{
protected:
    csRef<iParticleBuiltinEmitterBase> emitBase;
public:
    PPPartPlacement (iParticleBuiltinEmitterBase* emitBase)
	: ParticleParameterChoicesRow ("PartPlace"), emitBase(emitBase)
    {
	AddChoice("center");
	AddChoice("volume");
	AddChoice("surface");
    }
    virtual csString GetCurrentChoice()
    {
	csParticleBuiltinEmitterPlacement place = emitBase->GetParticlePlacement();
	switch (place)
	{
	    case CS_PARTICLE_BUILTIN_CENTER: return "center";
	    case CS_PARTICLE_BUILTIN_VOLUME: return "volume";
	    case CS_PARTICLE_BUILTIN_SURFACE: return "surface";
	    default: return "?";
	}
    }
    virtual void SetCurrentChoice(const csString& str)
    {
	csParticleBuiltinEmitterPlacement p = CS_PARTICLE_BUILTIN_CENTER;
	if (str == "center") p = CS_PARTICLE_BUILTIN_CENTER;
	if (str == "volume") p = CS_PARTICLE_BUILTIN_VOLUME;
	if (str == "surface") p = CS_PARTICLE_BUILTIN_SURFACE;
	emitBase->SetParticlePlacement (p);
    }
};

class PPVFType : public ParticleParameterChoicesRow
{
protected:
    csRef<iParticleBuiltinEffectorVelocityField> vfield;

public:
    PPVFType (iParticleBuiltinEffectorVelocityField* vfield)
	: ParticleParameterChoicesRow ("VFType"), vfield(vfield)
    {
	AddChoice("spiral");
	AddChoice("radialpoint");
    }
    virtual csString GetCurrentChoice()
    {
	csParticleBuiltinEffectorVFType place = vfield->GetType();
	switch (place)
	{
	    case CS_PARTICLE_BUILTIN_SPIRAL: return "spiral";
	    case CS_PARTICLE_BUILTIN_RADIALPOINT: return "radialpoint";
	    default: return "?";
	}
    }
    virtual void SetCurrentChoice(const csString& str)
    {
	csParticleBuiltinEffectorVFType p = CS_PARTICLE_BUILTIN_SPIRAL;
	if (str == "spiral") p = CS_PARTICLE_BUILTIN_SPIRAL;
	if (str == "radialpoint") p = CS_PARTICLE_BUILTIN_RADIALPOINT;
	vfield->SetType(p);
    }
};

class PPVFieldFPar : public ParticleParameterRow
{
private:
    csRef<iParticleBuiltinEffectorVelocityField> vfield;
    size_t index;

public:
    PPVFieldFPar(iParticleBuiltinEffectorVelocityField* vfield, size_t index)
	: ParticleParameterRow("FPar "), vfield(vfield), index(index)
    {
	name += index;
    }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	valueString.Format("%g", vfield->GetFParameter(index));
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float value = tb->valueNumSpinBox->GetValue();
	vfield->SetFParameter(index, value);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(-100000, 1000000, .1);
	tb->valueNumSpinBox->Show();
	tb->valueNumSpinBox->SetValue (vfield->GetFParameter(index));
    }
};

class PPVFieldVPar : public ParticleParameterRow
{
private:
    csRef<iParticleBuiltinEffectorVelocityField> vfield;
    size_t index;

public:
    PPVFieldVPar(iParticleBuiltinEffectorVelocityField* vfield, size_t index)
	: ParticleParameterRow("VPar "), vfield(vfield), index(index)
    {
	name += index;
    }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	csVector3 v = vfield->GetVParameter(index);
	valueString.Format("%g , %g , %g", v.x, v.y, v.z);
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float x = tb->valueNumSpinBox->GetValue();
	float y = tb->value2NumSpinBox->GetValue();
	float z = tb->value3NumSpinBox->GetValue();
	vfield->SetVParameter(index, csVector3(x,y,z));
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	csVector3 v = vfield->GetVParameter(index);
	tb->valueNumSpinBox->SetRange(-100000, 1000000, .1);
	tb->valueNumSpinBox->Show();
	tb->valueNumSpinBox->SetValue (v.x);
	tb->value2NumSpinBox->SetRange(-100000, 1000000, .1);
	tb->value2NumSpinBox->Show();
	tb->value2NumSpinBox->SetValue (v.y);
	tb->value3NumSpinBox->SetRange(-100000, 1000000, .1);
	tb->value3NumSpinBox->Show();
	tb->value3NumSpinBox->SetValue (v.z);
    }
};

class ParticleParameterBoolRow : public ParticleParameterRow
{
private:
    csString text;

public:
    ParticleParameterBoolRow(const char* name, const char* text) : ParticleParameterRow(name), text(text) { }

    virtual bool GetValue() = 0;
    virtual void SetValue(bool v) = 0;

    virtual csString GetRowDescription()
    {
	if (GetValue())
	    return "true";
	else
	    return "false";
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	bool value = tb->valueBool->GetState();
	SetValue(value);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueBool->SetText (text);
	tb->valueBool->Show();
	tb->valueBool->SetState (GetValue());
    }
};

class PPUniformVelocity : public ParticleParameterBoolRow
{
protected:
    csRef<iParticleBuiltinEmitterBase> emitBase;
public:
    PPUniformVelocity (iParticleBuiltinEmitterBase* emitBase)
	: ParticleParameterBoolRow("UniVel", "Uniform Velocity"), emitBase(emitBase) { }
    virtual bool GetValue() { return emitBase->GetUniformVelocity(); }
    virtual void SetValue(bool v) { emitBase->SetUniformVelocity(v); }
};

class PPLinMask : public ParticleParameterRow
{
private:
    csRef<iParticleBuiltinEffectorLinear> lin;

public:
    PPLinMask(iParticleBuiltinEffectorLinear* lin) : ParticleParameterRow("LinMask"), lin(lin)
    {
    }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	int mask = lin->GetMask();
    	if (mask & CS_PARTICLE_MASK_MASS) valueString += "mass ";
    	if (mask & CS_PARTICLE_MASK_LINEARVELOCITY) valueString += "lin ";
    	if (mask & CS_PARTICLE_MASK_ANGULARVELOCITY) valueString += "ang ";
    	if (mask & CS_PARTICLE_MASK_COLOR) valueString += "col ";
    	if (mask & CS_PARTICLE_MASK_PARTICLESIZE) valueString += "size ";
    	return valueString;
    }
    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	bool mass = tb->valueBool->GetState();
	bool linv = tb->valueBool2->GetState();
	bool angv = tb->valueBool3->GetState();
	bool col  = tb->valueBool4->GetState();
	bool size = tb->valueBool5->GetState();
	int mask = 0;
	if (mass) mask |= CS_PARTICLE_MASK_MASS;
	if (linv) mask |= CS_PARTICLE_MASK_LINEARVELOCITY;
	if (angv) mask |= CS_PARTICLE_MASK_ANGULARVELOCITY;
	if (col)  mask |= CS_PARTICLE_MASK_COLOR;
	if (size) mask |= CS_PARTICLE_MASK_PARTICLESIZE;
	lin->SetMask(mask);
	tb->RefreshParmList();
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	int mask = lin->GetMask();
	tb->valueBool->SetText ("Mass");
	tb->valueBool->SetState (mask & CS_PARTICLE_MASK_MASS);
	tb->valueBool->Show();
	tb->valueBool2->SetText ("Linear Velocity");
	tb->valueBool2->SetState (mask & CS_PARTICLE_MASK_LINEARVELOCITY);
	tb->valueBool2->Show();
	tb->valueBool3->SetText ("Angular Velocity");
	tb->valueBool3->SetState (mask & CS_PARTICLE_MASK_ANGULARVELOCITY);
	tb->valueBool3->Show();
	tb->valueBool4->SetText ("Color");
	tb->valueBool4->SetState (mask & CS_PARTICLE_MASK_COLOR);
	tb->valueBool4->Show();
	tb->valueBool5->SetText ("Particle Size");
	tb->valueBool5->SetState (mask & CS_PARTICLE_MASK_PARTICLESIZE);
	tb->valueBool5->Show();
    }
};

class ParticleParameterLinParRow : public ParticleParameterRow
{
protected:
    csRef<iParticleBuiltinEffectorLinear> lin;
    size_t index;

public:
    ParticleParameterLinParRow(const char* name, iParticleBuiltinEffectorLinear* lin, size_t index)
	: ParticleParameterRow(name), lin(lin), index(index)
    {
	name += index;
    }
};

class PPLinParMass : public ParticleParameterLinParRow
{
public:
    PPLinParMass(iParticleBuiltinEffectorLinear* lin, size_t index)
	: ParticleParameterLinParRow("Mass ", lin, index) { }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	valueString.Format("%g", par.mass);
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float value = tb->valueNumSpinBox->GetValue();
	csParticleParameterSet par = lin->GetParameterSet(index);
	par.mass = value;
	lin->SetParameterSet(index, par);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(0, 1000000, .1);
	tb->valueNumSpinBox->Show();
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	tb->valueNumSpinBox->SetValue (par.mass);
    }
};

class PPLinParTTL : public ParticleParameterLinParRow
{
public:
    PPLinParTTL(iParticleBuiltinEffectorLinear* lin, size_t index)
	: ParticleParameterLinParRow("TTL ", lin, index) { }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	valueString.Format("%g", lin->GetEndTTL(index));
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float value = tb->valueNumSpinBox->GetValue();
	lin->SetEndTTL(index, value);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(0, 1000000, .1);
	tb->valueNumSpinBox->Show();
	tb->valueNumSpinBox->SetValue (lin->GetEndTTL(index));
    }
};

class PPLinParLinVel : public ParticleParameterLinParRow
{
public:
    PPLinParLinVel(iParticleBuiltinEffectorLinear* lin, size_t index)
	: ParticleParameterLinParRow("LinVel ", lin, index) { }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	valueString.Format("%g , %g , %g", par.linearVelocity.x, par.linearVelocity.y, par.linearVelocity.z);
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float x = tb->valueNumSpinBox->GetValue();
	float y = tb->value2NumSpinBox->GetValue();
	float z = tb->value3NumSpinBox->GetValue();
	csParticleParameterSet par = lin->GetParameterSet(index);
	par.linearVelocity.Set(x,y,z);
	lin->SetParameterSet(index, par);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(-100000, 1000000, .1);
	tb->valueNumSpinBox->Show();
	tb->value2NumSpinBox->SetRange(-100000, 1000000, .1);
	tb->value2NumSpinBox->Show();
	tb->value3NumSpinBox->SetRange(-100000, 1000000, .1);
	tb->value3NumSpinBox->Show();
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	tb->valueNumSpinBox->SetValue (par.linearVelocity.x);
	tb->value2NumSpinBox->SetValue (par.linearVelocity.y);
	tb->value3NumSpinBox->SetValue (par.linearVelocity.z);
    }
};

class PPLinParAngVel : public ParticleParameterLinParRow
{
public:
    PPLinParAngVel(iParticleBuiltinEffectorLinear* lin, size_t index)
	: ParticleParameterLinParRow("AngVel ", lin, index) { }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	valueString.Format("%g , %g , %g", par.angularVelocity.x, par.angularVelocity.y, par.angularVelocity.z);
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float x = tb->valueNumSpinBox->GetValue();
	float y = tb->value2NumSpinBox->GetValue();
	float z = tb->value3NumSpinBox->GetValue();
	csParticleParameterSet par = lin->GetParameterSet(index);
	par.angularVelocity.Set(x,y,z);
	lin->SetParameterSet(index, par);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(-100000, 1000000, .1);
	tb->valueNumSpinBox->Show();
	tb->value2NumSpinBox->SetRange(-100000, 1000000, .1);
	tb->value2NumSpinBox->Show();
	tb->value3NumSpinBox->SetRange(-100000, 1000000, .1);
	tb->value3NumSpinBox->Show();
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	tb->valueNumSpinBox->SetValue (par.angularVelocity.x);
	tb->value2NumSpinBox->SetValue (par.angularVelocity.y);
	tb->value3NumSpinBox->SetValue (par.angularVelocity.z);
    }
};

class PPLinParColor : public ParticleParameterLinParRow
{
public:
    PPLinParColor(iParticleBuiltinEffectorLinear* lin, size_t index)
	: ParticleParameterLinParRow("Color ", lin, index) { }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	valueString.Format("%g , %g , %g , %g", par.color.red, par.color.green, par.color.blue, par.color.alpha);
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float r = tb->valueScroll1->GetCurrentValue();
	float g = tb->valueScroll2->GetCurrentValue();
	float b = tb->valueScroll3->GetCurrentValue();
	float a = tb->valueScroll4->GetCurrentValue();
	csParticleParameterSet par = lin->GetParameterSet(index);
	par.color.Set(r,g,b,a);
	lin->SetParameterSet(index, par);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueScroll1->SetMinValue(0);
	tb->valueScroll1->SetMaxValue(1);
	tb->valueScroll2->SetMinValue(0);
	tb->valueScroll2->SetMaxValue(1);
	tb->valueScroll3->SetMinValue(0);
	tb->valueScroll3->SetMaxValue(1);
	tb->valueScroll4->SetMinValue(0);
	tb->valueScroll4->SetMaxValue(1);

	const csParticleParameterSet& par = lin->GetParameterSet(index);

	tb->valueScroll1->SetCurrentValue(par.color.red, false, false);
	tb->valueScroll2->SetCurrentValue(par.color.green, false, false);
	tb->valueScroll3->SetCurrentValue(par.color.blue, false, false);
	tb->valueScroll4->SetCurrentValue(par.color.alpha, false, false);

	tb->valueScroll1->Show();
	tb->valueScroll2->Show();
	tb->valueScroll3->Show();
	tb->valueScroll4->Show();
    }
};

class PPLinParPartSize : public ParticleParameterLinParRow
{
public:
    PPLinParPartSize(iParticleBuiltinEffectorLinear* lin, size_t index)
	: ParticleParameterLinParRow("Size ", lin, index) { }

    virtual csString GetRowDescription()
    {
    	csString valueString;
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	valueString.Format("%g , %g", par.particleSize.x, par.particleSize.y);
    	return valueString;
    }

    virtual void UpdateParticleValue(EEditParticleListToolbox* tb)
    {
	float x = tb->valueNumSpinBox->GetValue();
	float y = tb->value2NumSpinBox->GetValue();
	csParticleParameterSet par = lin->GetParameterSet(index);
	par.particleSize.Set(x,y);
	lin->SetParameterSet(index, par);
    }
    virtual void FillParticleEditor(EEditParticleListToolbox* tb)
    {
	tb->valueNumSpinBox->SetRange(-100000, 1000000, .1);
	tb->valueNumSpinBox->Show();
	tb->value2NumSpinBox->SetRange(-100000, 1000000, .1);
	tb->value2NumSpinBox->Show();
	const csParticleParameterSet& par = lin->GetParameterSet(index);
	tb->valueNumSpinBox->SetValue (par.particleSize.x);
	tb->value2NumSpinBox->SetValue (par.particleSize.y);
    }
};

//---------------------------------------------------------------------------------------

EEditParticleListToolbox::EEditParticleListToolbox() : scfImplementationType(this)
{
    updatingParticleValue = 0;
}

EEditParticleListToolbox::~EEditParticleListToolbox()
{
}

void EEditParticleListToolbox::Update(unsigned int elapsed)
{
}

size_t EEditParticleListToolbox::GetType() const
{
    return T_PARTICLES;
}

const char * EEditParticleListToolbox::GetName() const
{
    return "Particle Systems";
}
    
int EEditParticleListToolbox::SortTextBox(pawsWidget * widgetA, pawsWidget * widgetB)
{
    pawsTextBox * textBoxA, * textBoxB;
    const char  * textA,    * textB;
    
    textBoxA = dynamic_cast <pawsTextBox*> (widgetA);
    textBoxB = dynamic_cast <pawsTextBox*> (widgetB);
    assert(textBoxA && textBoxB);
    textA = textBoxA->GetText();
    if (textA == NULL)
        textA = "";
    textB = textBoxB->GetText();
    if (textB == NULL)
        textB = "";
    
    return strcmp(textA, textB);
}

static pawsListBoxRow* NewRow (size_t& a, pawsListBox* box, pawsTextBox** col1, pawsTextBox** col2 = 0, pawsTextBox** col3 = 0)
{
    box->NewRow(a);
    pawsListBoxRow * row = box->GetRow(a);
    if (!row) return 0;

    *col1 = (pawsTextBox *)row->GetColumn(0);
    if (!*col1) return 0;

    if (col2)
    {
        *col2 = (pawsTextBox *)row->GetColumn(1);
        if (!*col2) return 0;
    }

    if (col3)
    {
        *col3 = (pawsTextBox *)row->GetColumn(2);
        if (!*col3) return 0;
    }

    ++a;
    return row;
}

static pawsListBoxRow* NewParameterRow (size_t& a, pawsListBox* box, EEditParticleListToolbox* tb,
	ParticleParameterRow* prow)
{
    pawsTextBox* col1, * col2;
    pawsListBoxRow* row = NewRow (a, box, &col1, &col2);
    if (!row)
	return 0;
    col1->SetText (prow->GetRowName());
    col2->SetText (prow->GetRowDescription());
    tb->parameterRows.Push(prow);
    return row;
}

void EEditParticleListToolbox::HideValues ()
{
    valueNumSpinBox->Hide();
    value2NumSpinBox->Hide();
    value3NumSpinBox->Hide();
    valueChoices->Hide();
    valueBool->Hide();
    valueBool2->Hide();
    valueBool3->Hide();
    valueBool4->Hide();
    valueBool5->Hide();
    valueScroll1->Hide();
    valueScroll2->Hide();
    valueScroll3->Hide();
    valueScroll4->Hide();
}

void EEditParticleListToolbox::ClearParmList ()
{
    parmList->Clear();
    parmList->Select (0);
    HideValues();
    parameterRows.DeleteAll ();
}

void EEditParticleListToolbox::RefreshParmList()
{
    updatingParticleValue++;
    size_t num = editList->GetSelectedRowNum();
    if (num >= emitters.GetSize())
	FillParmList (effectors[num-emitters.GetSize()]);
    else
	FillParmList (emitters[num]);
    updatingParticleValue--;
}

void EEditParticleListToolbox::FillParmList(iParticleEffector* eff)
{
    ClearParmList();
    size_t a=0;

    csRef<iParticleBuiltinEffectorForce> force = scfQueryInterface<iParticleBuiltinEffectorForce> (eff);
    if (force)
    {
	if (!NewParameterRow (a, parmList, this, new PPAcceleration(force))) return;
	if (!NewParameterRow (a, parmList, this, new PPForce(force))) return;
	if (!NewParameterRow (a, parmList, this, new PPRandomAcceleration(force))) return;
    }

    csRef<iParticleBuiltinEffectorLinColor> lc = scfQueryInterface<iParticleBuiltinEffectorLinColor> (eff);
    if (lc)
    {
	for (size_t i = 0 ; i < lc->GetColorCount() ; i++)
	    if (!NewParameterRow (a, parmList, this, new PPLinCol(lc,i))) return;
    }

    csRef<iParticleBuiltinEffectorLinear> lin = scfQueryInterface<iParticleBuiltinEffectorLinear> (eff);
    if (lin)
    {
	if (!NewParameterRow (a, parmList, this, new PPLinMask(lin))) return;
	for (size_t i = 0 ; i < lin->GetParameterSetCount() ; i++)
	    if (!NewParameterRow (a, parmList, this, new PPLinParTTL(lin,i))) return;
	int mask = lin->GetMask();
	if (mask & CS_PARTICLE_MASK_MASS)
	    for (size_t i = 0 ; i < lin->GetParameterSetCount() ; i++)
	    	if (!NewParameterRow (a, parmList, this, new PPLinParMass(lin,i))) return;
	if (mask & CS_PARTICLE_MASK_LINEARVELOCITY)
	    for (size_t i = 0 ; i < lin->GetParameterSetCount() ; i++)
	    	if (!NewParameterRow (a, parmList, this, new PPLinParLinVel(lin,i))) return;
	if (mask & CS_PARTICLE_MASK_ANGULARVELOCITY)
	    for (size_t i = 0 ; i < lin->GetParameterSetCount() ; i++)
	    	if (!NewParameterRow (a, parmList, this, new PPLinParAngVel(lin,i))) return;
	if (mask & CS_PARTICLE_MASK_COLOR)
	    for (size_t i = 0 ; i < lin->GetParameterSetCount() ; i++)
	    	if (!NewParameterRow (a, parmList, this, new PPLinParColor(lin,i))) return;
	if (mask & CS_PARTICLE_MASK_PARTICLESIZE)
	    for (size_t i = 0 ; i < lin->GetParameterSetCount() ; i++)
	    	if (!NewParameterRow (a, parmList, this, new PPLinParPartSize(lin,i))) return;
    }
    csRef<iParticleBuiltinEffectorVelocityField> vfield = scfQueryInterface<iParticleBuiltinEffectorVelocityField> (eff);
    if (vfield)
    {
	if (!NewParameterRow (a, parmList, this, new PPVFType(vfield))) return;
	for (size_t i = 0 ; i < vfield->GetFParameterCount() ; i++)
	    if (!NewParameterRow (a, parmList, this, new PPVFieldFPar(vfield,i))) return;
	for (size_t i = 0 ; i < vfield->GetVParameterCount() ; i++)
	    if (!NewParameterRow (a, parmList, this, new PPVFieldVPar(vfield,i))) return;
    }
}

void EEditParticleListToolbox::FillParmList(iParticleEmitter* emit)
{
    ClearParmList();
    size_t a=0;

    if (!NewParameterRow (a, parmList, this, new PPStartTime(emit))) return;
    if (!NewParameterRow (a, parmList, this, new PPDuration(emit))) return;
    if (!NewParameterRow (a, parmList, this, new PPEmitRate(emit))) return;

    if (!NewParameterRow (a, parmList, this, new PPTTL(emit))) return;
    if (!NewParameterRow (a, parmList, this, new PPInitialMass(emit))) return;

    csRef<iParticleBuiltinEmitterBase> emitBase = scfQueryInterface<iParticleBuiltinEmitterBase> (emit);
    if (emitBase)
    {
	if (!NewParameterRow (a, parmList, this, new PPEmitPos(emitBase))) return;
	if (!NewParameterRow (a, parmList, this, new PPPartPlacement(emitBase))) return;
	if (!NewParameterRow (a, parmList, this, new PPUniformVelocity(emitBase))) return;
	if (!NewParameterRow (a, parmList, this, new PPEmitLinVel(emitBase))) return;
	if (!NewParameterRow (a, parmList, this, new PPEmitAngVel(emitBase))) return;
    }

    csRef<iParticleBuiltinEmitterSphere> sphere = scfQueryInterface<iParticleBuiltinEmitterSphere> (emit);
    if (sphere)
    {
	if (!NewParameterRow (a, parmList, this, new PPSphereRadius(sphere))) return;
    }

    csRef<iParticleBuiltinEmitterCone> cone = scfQueryInterface<iParticleBuiltinEmitterCone> (emit);
    if (cone)
    {
	if (!NewParameterRow (a, parmList, this, new PPConeExtent(cone))) return;
	if (!NewParameterRow (a, parmList, this, new PPConeAngle(cone))) return;
    }

    csRef<iParticleBuiltinEmitterBox> box = scfQueryInterface<iParticleBuiltinEmitterBox> (emit);
    if (box)
    {
	if (!NewParameterRow (a, parmList, this, new PPBoxDir1(box))) return;
	if (!NewParameterRow (a, parmList, this, new PPBoxDir2(box))) return;
	if (!NewParameterRow (a, parmList, this, new PPBoxDir3(box))) return;
	if (!NewParameterRow (a, parmList, this, new PPBoxTopLeft(box))) return;
	if (!NewParameterRow (a, parmList, this, new PPBoxBotRight(box))) return;
    }

    csRef<iParticleBuiltinEmitterCylinder> cylinder = scfQueryInterface<iParticleBuiltinEmitterCylinder> (emit);
    if (cylinder)
    {
	if (!NewParameterRow (a, parmList, this, new PPCylinderRadius(cylinder))) return;
	if (!NewParameterRow (a, parmList, this, new PPCylinderExtent(cylinder))) return;
    }

}

void EEditParticleListToolbox::RefreshEditList()
{
    csString partName = editApp->GetCurrParticleSystemName();
    if (partName.IsEmpty()) return;

    editList->Clear();
    editList->Select (0);
    ClearParmList();

    emitters.DeleteAll();
    effectors.DeleteAll();

    iMeshFactoryWrapper* fact = engine->FindMeshFactory (partName);
    if (!fact) return;
    csRef<iParticleSystemFactory> pfact = scfQueryInterface<iParticleSystemFactory> (fact->GetMeshObjectFactory());
    if (!pfact) return;
    csRef<iParticleSystemBase> base = scfQueryInterface<iParticleSystemBase> (pfact);

    size_t a=0;
    for (size_t i = 0 ; i < base->GetEmitterCount(); i++)
    {
        iParticleEmitter* emit = base->GetEmitter (i);

	pawsTextBox* col1, * col2;
	pawsListBoxRow* row = NewRow (a, editList, &col1, &col2);
	if (!row)
	    return;

        col1->SetText("Emit");

	emitters.Push(emit);

	bool found = false;
	csRef<iParticleBuiltinEmitterSphere> sphere = scfQueryInterface<iParticleBuiltinEmitterSphere> (emit);
	if (sphere)
	{
	    csString desc;
	    desc.Format ("Sphere(%g)", sphere->GetRadius ());
            col2->SetText(desc);
	    found = true;
	}
	csRef<iParticleBuiltinEmitterCone> cone = scfQueryInterface<iParticleBuiltinEmitterCone> (emit);
	if (cone)
	{
	    csString desc;
	    const csVector3& ext = cone->GetExtent ();
	    desc.Format ("Cone(%g,%g,%g)", ext.x, ext.y, ext.z);
            col2->SetText(desc);
	    found = true;
	}
	csRef<iParticleBuiltinEmitterBox> box = scfQueryInterface<iParticleBuiltinEmitterBox> (emit);
	if (box)
	{
	    csString desc;
	    desc.Format ("Box()");
            col2->SetText(desc);
	    found = true;
	}
	csRef<iParticleBuiltinEmitterCylinder> cylinder = scfQueryInterface<iParticleBuiltinEmitterCylinder> (emit);
	if (cylinder)
	{
	    csString desc;
	    desc.Format ("Cylinder(%g)", cylinder->GetRadius ());
            col2->SetText(desc);
	    found = true;
	}
	if (!found)
	{
	    col2->SetText("Unknown(?)");
	}
    }
    for (size_t i = 0 ; i < base->GetEffectorCount(); i++)
    {
        iParticleEffector* eff = base->GetEffector (i);

	pawsTextBox* col1, * col2;
	if (!NewRow (a, editList, &col1, &col2))
	    return;

        col1->SetText("Effect");

	effectors.Push(eff);

	bool found = false;
	csRef<iParticleBuiltinEffectorForce> force = scfQueryInterface<iParticleBuiltinEffectorForce> (eff);
	if (force)
	{
	    csString desc;
	    const csVector3& f = force->GetForce ();
	    desc.Format ("Force(%g,%g,%g)", f.x, f.y, f.z);
            col2->SetText(desc);
	    found = true;
	}
	csRef<iParticleBuiltinEffectorLinColor> lc = scfQueryInterface<iParticleBuiltinEffectorLinColor> (eff);
	if (lc)
	{
	    csString desc;
	    desc.Format ("LinCol(%d)", lc->GetColorCount());
            col2->SetText(desc);
	    found = true;
	}
	csRef<iParticleBuiltinEffectorVelocityField> vf = scfQueryInterface<iParticleBuiltinEffectorVelocityField> (eff);
	if (vf)
	{
	    csString desc;
	    csParticleBuiltinEffectorVFType type = vf->GetType ();
	    desc.Format ("VelFld(%s)", type==CS_PARTICLE_BUILTIN_SPIRAL ? "spiral" : "radpnt");
            col2->SetText(desc);
	    found = true;
	}
	csRef<iParticleBuiltinEffectorLinear> lin = scfQueryInterface<iParticleBuiltinEffectorLinear> (eff);
	if (lin)
	{
	    csString desc;
	    desc.Format ("Linear()");
            col2->SetText(desc);
	    found = true;
	}
	if (!found)
	{
	    col2->SetText("Unknown(?)");
	}
    }

}

void EEditParticleListToolbox::FillList(iEngine* engine)
{
    EEditParticleListToolbox::engine = engine;

    if (!partList)
        return;

    partList->Clear();

    size_t a=0;
    iMeshFactoryList* factlist = engine->GetMeshFactories ();
    for (size_t i = 0 ; i < (size_t)factlist->GetCount (); i++)
    {
        iMeshFactoryWrapper* fact = factlist->Get (i);
	csRef<iParticleSystemFactory> pfact = scfQueryInterface<iParticleSystemFactory> (
		fact->GetMeshObjectFactory());
	if (!pfact)
	    continue;

	pawsTextBox* col;
	pawsListBoxRow* row = NewRow (a, partList, &col);
	if (!row)
	    return;

        csString partName = fact->QueryObject()->GetName();
        col->SetText(partName);
        if (editApp->GetCurrParticleSystemName() == partName)
            partList->Select(row);
    }
    partList->SetSortingFunc(0, SortTextBox);
    partList->SetSortedColumn(0);
    partList->SortRows();
    RefreshEditList();
}

void EEditParticleListToolbox::SaveParticleSystem (const csString& name)
{
    iMeshFactoryWrapper* fact = engine->FindMeshFactory (name);
    if (!fact) return;
    csRef<iParticleSystemFactory> pfact = scfQueryInterface<iParticleSystemFactory> (fact->GetMeshObjectFactory());
    if (!pfact) return;

    csRef<iPluginManager> plugmgr = csQueryRegistry<iPluginManager> (editApp->GetObjectRegistry());
    csRef<iSaverPlugin> saver = csLoadPlugin<iSaverPlugin> (plugmgr, "crystalspace.mesh.saver.factory.particles");

    csRef<iDocumentSystem> xml;
    xml.AttachNew (new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot();
    saver->WriteDown (pfact, root, 0);
    doc->Write(editApp->GetVFS(), "/this/particlesystem.xml");

}

bool EEditParticleListToolbox::PostSetup()
{
    partList = (pawsListBox *)FindWidget("part_list");                  CS_ASSERT(partList);
    editList = (pawsListBox *)FindWidget("edit_list");                  CS_ASSERT(editList);
    parmList = (pawsListBox *)FindWidget("parm_list");                  CS_ASSERT(parmList);
    openPartButton = (pawsButton *)FindWidget("load_part_button");      CS_ASSERT(openPartButton);
    refreshButton = (pawsButton *)FindWidget("refresh_button");         CS_ASSERT(refreshButton);
    saveButton = (pawsButton *)FindWidget("save_part_button");          CS_ASSERT(saveButton);
    valueNumSpinBox = (pawsSpinBox *)FindWidget("value_num");           CS_ASSERT(valueNumSpinBox);
    value2NumSpinBox = (pawsSpinBox *)FindWidget("value2_num");         CS_ASSERT(value2NumSpinBox);
    value3NumSpinBox = (pawsSpinBox *)FindWidget("value3_num");         CS_ASSERT(value3NumSpinBox);
    valueChoices = (pawsComboBox *)FindWidget("value_choices");         CS_ASSERT(valueChoices);
    valueBool = (pawsCheckBox *)FindWidget("value_bool");               CS_ASSERT(valueBool);
    valueBool2 = (pawsCheckBox *)FindWidget("value_bool2");             CS_ASSERT(valueBool2);
    valueBool3 = (pawsCheckBox *)FindWidget("value_bool3");             CS_ASSERT(valueBool3);
    valueBool4 = (pawsCheckBox *)FindWidget("value_bool4");             CS_ASSERT(valueBool4);
    valueBool5 = (pawsCheckBox *)FindWidget("value_bool5");             CS_ASSERT(valueBool5);
    valueScroll1 = (pawsScrollBar *)FindWidget("value_scroll1");        CS_ASSERT(valueScroll1);
    valueScroll2 = (pawsScrollBar *)FindWidget("value_scroll2");        CS_ASSERT(valueScroll2);
    valueScroll3 = (pawsScrollBar *)FindWidget("value_scroll3");        CS_ASSERT(valueScroll3);
    valueScroll4 = (pawsScrollBar *)FindWidget("value_scroll4");        CS_ASSERT(valueScroll4);

    HideValues();

    return true;
}

void EEditParticleListToolbox::UpdateParticleValue()
{
    if (updatingParticleValue) return;
    updatingParticleValue++;
    size_t num = parmList->GetSelectedRowNum();
    ParticleParameterRow* prow = parameterRows[num];
    prow->UpdateParticleValue(this);

    // Select and fetch the prow again because UpdateParticleValue() may
    // have forced a refresh of the parameter list.
    pawsListBoxRow * row = parmList->GetRow(num);
    parmList->Select(row);
    prow = parameterRows[num];
    pawsTextBox* colValue = (pawsTextBox *)row->GetColumn(1);
    colValue->SetText(prow->GetRowDescription());
    updatingParticleValue--;
}

bool EEditParticleListToolbox::OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (updatingParticleValue) return false;
    if (widget == valueBool->GetButton()
	    || widget == valueBool2->GetButton()
	    || widget == valueBool3->GetButton()
	    || widget == valueBool4->GetButton()
	    || widget == valueBool5->GetButton()
	    )
    {
	UpdateParticleValue();
	return true;
    }
    return false;
}

bool EEditParticleListToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget == openPartButton)
    {
        csString name = partList->GetTextCellValue(partList->GetSelectedRowNum(), 0);
        editApp->CreateParticleSystem (name);
	RefreshEditList();
        return true;
    }
    else if (widget == refreshButton)
    {
	FillList(engine);
        return true;
    }
    else if (widget == saveButton)
    {
        csString name = partList->GetTextCellValue(partList->GetSelectedRowNum(), 0);
	if (name != "")
	    SaveParticleSystem (name);
        return true;
    }
    return false;
}

bool EEditParticleListToolbox::OnChange(pawsWidget* widget)
{
    if (widget == valueNumSpinBox || widget == value2NumSpinBox || widget == value3NumSpinBox)
    {
	UpdateParticleValue();
	return true;
    }
    return false;
}

bool EEditParticleListToolbox::OnScroll(int dir, pawsScrollBar* widget)
{
    if (widget == valueScroll1 || widget == valueScroll2 || widget == valueScroll3 || widget == valueScroll4)
    {
	UpdateParticleValue();
	return true;
    }
    return false;
}

void EEditParticleListToolbox::OnListAction(pawsListBox* selected, int status)
{
    if (selected == editList)
    {
	RefreshParmList();
    }
    else if (selected == valueChoices->GetChoiceList())
    {
	UpdateParticleValue();
    }
    else if (selected == parmList)
    {
	HideValues();

        size_t num = parmList->GetSelectedRowNum();
	ParticleParameterRow* prow = parameterRows[num];
	updatingParticleValue++;
	prow->FillParticleEditor(this);
	updatingParticleValue--;
    }
}

