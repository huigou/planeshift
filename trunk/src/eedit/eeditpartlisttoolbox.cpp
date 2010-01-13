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

EEditParticleListToolbox::EEditParticleListToolbox() : scfImplementationType(this)
{
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

static pawsListBoxRow* NewFloatRow (size_t& a, pawsListBox* box, const char* name, float value)
{
    pawsTextBox* col1, * col2, * col3;
    pawsListBoxRow* row = NewRow (a, box, &col1, &col2, &col3);
    if (!row)
	return 0;
    csString valueString;
    col1->SetText (name);
    col2->SetText ("F");
    valueString.Format ("%g", value);
    col3->SetText (valueString);
    return row;
}

static pawsListBoxRow* NewMinMaxRow (size_t& a, pawsListBox* box, const char* name, float value1, float value2)
{
    pawsTextBox* col1, * col2, * col3;
    pawsListBoxRow* row = NewRow (a, box, &col1, &col2, &col3);
    if (!row)
	return 0;
    csString valueString;
    col1->SetText (name);
    col2->SetText ("MM");
    valueString.Format ("%g , %g", value1, value2);
    col3->SetText (valueString);
    return row;
}

static pawsListBoxRow* NewVector3Row (size_t& a, pawsListBox* box, const char* name, const csVector3& v)
{
    pawsTextBox* col1, * col2, * col3;
    pawsListBoxRow* row = NewRow (a, box, &col1, &col2, &col3);
    if (!row)
	return 0;
    csString valueString;
    col1->SetText (name);
    col2->SetText ("V3");
    valueString.Format ("%g , %g, %g", v.x, v.y, v.z);
    col3->SetText (valueString);
    return row;
}

static pawsListBoxRow* NewChoicesRow (size_t& a, pawsListBox* box, const char* name, const char* currentChoice)
{
    pawsTextBox* col1, * col2, * col3;
    pawsListBoxRow* row = NewRow (a, box, &col1, &col2, &col3);
    if (!row)
	return 0;
    col1->SetText (name);
    col2->SetText ("*");
    col3->SetText (currentChoice);
    return row;
}

static pawsListBoxRow* NewBoolRow (size_t& a, pawsListBox* box, const char* name, bool v)
{
    pawsTextBox* col1, * col2, * col3;
    pawsListBoxRow* row = NewRow (a, box, &col1, &col2, &col3);
    if (!row)
	return 0;
    col1->SetText (name);
    col2->SetText ("B");
    col3->SetText (v ? "true" : "false");
    return row;
}

static pawsListBoxRow* NewLCRow (size_t& a, pawsListBox* box, const char* name,
	float endTTL, const csColor4& color)
{
    pawsTextBox* col1, * col2, * col3;
    pawsListBoxRow* row = NewRow (a, box, &col1, &col2, &col3);
    if (!row)
	return 0;
    col1->SetText (name);
    col2->SetText ("LC");
    csString valueString;
    valueString.Format("%g (%g , %g , %g , %g)\n", endTTL, color.red, color.green, color.blue, color.alpha);
    col3->SetText (valueString);
    return row;
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEffector* eff, const csString& name,
	float ttl, float r, float g, float b, float a)
{
    if (name.StartsWith("LinCol:"))
    {
    	csRef<iParticleBuiltinEffectorLinColor> lc = scfQueryInterface<iParticleBuiltinEffectorLinColor> (eff);
    	if (lc)
	{
	    size_t i;
	    csScanStr(name, "LinCol: %d", &i);
	    lc->SetColor(i,csColor4(r,g,b,a));
	}
    }
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEffector* eff, const csString& name, bool value)
{
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEffector* eff, const csString& name, const csString& value)
{
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEffector* eff, const csString& name, float value)
{
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEffector* eff, const csString& name,
	float value1, float value2)
{
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEffector* eff, const csString& name, const csVector3& v)
{
    if (name == "Acceleration")
    {
    	csRef<iParticleBuiltinEffectorForce> force = scfQueryInterface<iParticleBuiltinEffectorForce> (eff);
    	if (force)
	    force->SetAcceleration(v);
    }
    else if (name == "Force")
    {
    	csRef<iParticleBuiltinEffectorForce> force = scfQueryInterface<iParticleBuiltinEffectorForce> (eff);
    	if (force)
	    force->SetForce(v);
    }
    else if (name == "RandomAcc")
    {
    	csRef<iParticleBuiltinEffectorForce> force = scfQueryInterface<iParticleBuiltinEffectorForce> (eff);
    	if (force)
	    force->SetRandomAcceleration(v);
    }
}

void EEditParticleListToolbox::HideValues ()
{
    valueNumSpinBox->Hide();
    value2NumSpinBox->Hide();
    value3NumSpinBox->Hide();
    valueChoices->Hide();
    valueBool->Hide();
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
    parameterData.DeleteAll ();
}

void EEditParticleListToolbox::FillParmList(iParticleEffector* eff)
{
    ClearParmList();
    size_t a=0;

    csRef<iParticleBuiltinEffectorForce> force = scfQueryInterface<iParticleBuiltinEffectorForce> (eff);
    if (force)
    {
    	if (!NewVector3Row (a, parmList, "Acceleration", force->GetAcceleration())) return;
    	parameterData.Push(ParameterData("V3", -1000, 1000, .1));
    	if (!NewVector3Row (a, parmList, "Force", force->GetForce())) return;
    	parameterData.Push(ParameterData("V3", -1000, 1000, .1));
    	if (!NewVector3Row (a, parmList, "RandomAcc", force->GetRandomAcceleration())) return;
    	parameterData.Push(ParameterData("V3", -1000, 1000, .1));
    }

    csRef<iParticleBuiltinEffectorLinColor> lc = scfQueryInterface<iParticleBuiltinEffectorLinColor> (eff);
    if (lc)
    {
	for (size_t i = 0 ; i < lc->GetColorCount() ; i++)
	{
	    csString value;
	    csColor4 color;
	    float endTTL;
	    lc->GetColor(i,color,endTTL);
	    csString name = "LinCol:";
	    name += i;
	    if (!NewLCRow (a, parmList, name, endTTL, color)) return;
    	    parameterData.Push(ParameterData("LC", 0, 1));
	}
    }
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEmitter* emit, const csString& name,
	float ttl, float r, float g, float b, float a)
{
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEmitter* emit, const csString& name, bool v)
{
    if (name == "UniVel")
    {
        csRef<iParticleBuiltinEmitterBase> emitBase = scfQueryInterface<iParticleBuiltinEmitterBase> (emit);
	if (emitBase)
	    emitBase->SetUniformVelocity(v);
    }
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEmitter* emit, const csString& name, const csString& s)
{
    if (name == "PartPlace")
    {
        csRef<iParticleBuiltinEmitterBase> emitBase = scfQueryInterface<iParticleBuiltinEmitterBase> (emit);
	if (emitBase)
	{
	    if (s == "center")
		emitBase->SetParticlePlacement(CS_PARTICLE_BUILTIN_CENTER);
	    else if (s == "volume")
		emitBase->SetParticlePlacement(CS_PARTICLE_BUILTIN_VOLUME);
	    else if (s == "surface")
		emitBase->SetParticlePlacement(CS_PARTICLE_BUILTIN_SURFACE);
	}
    }
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEmitter* emit, const csString& name, float f)
{
    if (name == "StartTime")
	emit->SetStartTime(f);
    else if (name == "Duration")
	emit->SetDuration(f);
    else if (name == "EmitRate")
	emit->SetEmissionRate(f);
    else if (name == "CRadius")
    {
        csRef<iParticleBuiltinEmitterCylinder> cylinder = scfQueryInterface<iParticleBuiltinEmitterCylinder> (emit);
	if (cylinder)
	    cylinder->SetRadius (f);
    }
    else if (name == "SRadius")
    {
        csRef<iParticleBuiltinEmitterSphere> sphere = scfQueryInterface<iParticleBuiltinEmitterSphere> (emit);
	if (sphere)
	    sphere->SetRadius (f);
    }
    else if (name == "CAngle")
    {
        csRef<iParticleBuiltinEmitterCone> cone = scfQueryInterface<iParticleBuiltinEmitterCone> (emit);
        if (cone)
	    cone->SetConeAngle (f);
    }
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEmitter* emit, const csString& name, float f1, float f2)
{
    if (name == "TTL")
	emit->SetInitialTTL (f1, f2);
    else if (name == "Mass")
	emit->SetInitialMass (f1, f2);
}

void EEditParticleListToolbox::ChangeParticleValue(iParticleEmitter* emit, const csString& name, const csVector3& v)
{
    if (name == "Pos")
    {
        csRef<iParticleBuiltinEmitterBase> emitBase = scfQueryInterface<iParticleBuiltinEmitterBase> (emit);
	if (emitBase)
	    emitBase->SetPosition (v);
    }
    else if (name == "VelLinear")
    {
        csRef<iParticleBuiltinEmitterBase> emitBase = scfQueryInterface<iParticleBuiltinEmitterBase> (emit);
	if (emitBase)
	{
	    csVector3 linear, angular;
	    emitBase->GetInitialVelocity(linear, angular);
	    emitBase->SetInitialVelocity (v, angular);
	}
    }
    else if (name == "VelAngul")
    {
        csRef<iParticleBuiltinEmitterBase> emitBase = scfQueryInterface<iParticleBuiltinEmitterBase> (emit);
	if (emitBase)
	{
	    csVector3 linear, angular;
	    emitBase->GetInitialVelocity(linear, angular);
	    emitBase->SetInitialVelocity (linear, v);
	}
    }
    else if (name == "CExtent")
    {
        csRef<iParticleBuiltinEmitterCone> cone = scfQueryInterface<iParticleBuiltinEmitterCone> (emit);
	if (cone)
	    cone->SetExtent(v);
        csRef<iParticleBuiltinEmitterCylinder> cylinder = scfQueryInterface<iParticleBuiltinEmitterCylinder> (emit);
	if (cylinder)
	    cylinder->SetExtent(v);
    }
}

void EEditParticleListToolbox::FillParmList(iParticleEmitter* emit)
{
    ClearParmList();
    size_t a=0;

    if (!NewFloatRow (a, parmList, "StartTime", emit->GetStartTime())) return;
    parameterData.Push(ParameterData("F", 0, 200, .5));

    if (!NewFloatRow (a, parmList, "Duration", emit->GetDuration())) return;
    parameterData.Push(ParameterData("F", 0, 1000000000, 1));

    if (!NewFloatRow (a, parmList, "EmitRate", emit->GetEmissionRate())) return;
    parameterData.Push(ParameterData("F", 0, 100000, 1));

    float ttlmin, ttlmax;
    emit->GetInitialTTL (ttlmin, ttlmax);
    if (!NewMinMaxRow (a, parmList, "TTL", ttlmin, ttlmax)) return;
    parameterData.Push(ParameterData("MM", 0, 1000, .1));

    float massmin, massmax;
    emit->GetInitialMass (massmin, massmax);
    if (!NewMinMaxRow (a, parmList, "Mass", massmin, massmax)) return;
    parameterData.Push(ParameterData("MM", 0, 1000, .1));

    csRef<iParticleBuiltinEmitterBase> emitBase = scfQueryInterface<iParticleBuiltinEmitterBase> (emit);
    if (emitBase)
    {
	if (!NewVector3Row (a, parmList, "Pos", emitBase->GetPosition())) return;
    	parameterData.Push(ParameterData("V3", -1000, 1000, .1));

	csParticleBuiltinEmitterPlacement place = emitBase->GetParticlePlacement();
	csString p;
	switch (place)
	{
	    case CS_PARTICLE_BUILTIN_CENTER: p = "center"; break;
	    case CS_PARTICLE_BUILTIN_VOLUME: p = "volume"; break;
	    case CS_PARTICLE_BUILTIN_SURFACE: p = "surface"; break;
	    default: p = "?";
	}
	if (!NewChoicesRow (a, parmList, "PartPlace", p)) return;
	ParameterData pd = ParameterData("*");
	pd.choices.Push ("center");
	pd.choices.Push ("volume");
	pd.choices.Push ("surface");
    	parameterData.Push(pd);

	if (!NewBoolRow (a, parmList, "UniVel", emitBase->GetUniformVelocity())) return;
	pd = ParameterData("B");
	pd.text = "Uniform Velocity";
    	parameterData.Push(pd);

	csVector3 linear, angular;
	emitBase->GetInitialVelocity(linear, angular);
	if (!NewVector3Row (a, parmList, "VelLinear", linear)) return;
    	parameterData.Push(ParameterData("V3", -1000, 1000, .1));
	if (!NewVector3Row (a, parmList, "VelAngul", angular)) return;
    	parameterData.Push(ParameterData("V3", -1000, 1000, .1));
    }

    csRef<iParticleBuiltinEmitterSphere> sphere = scfQueryInterface<iParticleBuiltinEmitterSphere> (emit);
    if (sphere)
    {
	if (!NewFloatRow (a, parmList, "SRadius", sphere->GetRadius())) return;
	parameterData.Push(ParameterData("F", 0, 1000, .1));
    }

    csRef<iParticleBuiltinEmitterCone> cone = scfQueryInterface<iParticleBuiltinEmitterCone> (emit);
    if (cone)
    {
	if (!NewVector3Row (a, parmList, "CExtent", cone->GetExtent ())) return;
    	parameterData.Push(ParameterData("V3", -1000, 1000, .1));
	if (!NewFloatRow (a, parmList, "CAngle", cone->GetConeAngle())) return;
	parameterData.Push(ParameterData("F", 0, 3.1415926, .1));
    }

    csRef<iParticleBuiltinEmitterBox> box = scfQueryInterface<iParticleBuiltinEmitterBox> (emit);
    if (box)
    {
    }

    csRef<iParticleBuiltinEmitterCylinder> cylinder = scfQueryInterface<iParticleBuiltinEmitterCylinder> (emit);
    if (cylinder)
    {
	if (!NewFloatRow (a, parmList, "CRadius", cylinder->GetRadius())) return;
	parameterData.Push(ParameterData("F", 0, 1000, .1));
	if (!NewVector3Row (a, parmList, "CExtent", cylinder->GetExtent ())) return;
    	parameterData.Push(ParameterData("V3", -1000, 1000, .1));
    }

}

void EEditParticleListToolbox::FillEditList(const csString& partName)
{
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
	{
            partList->Select(row);
	    FillEditList(partName);
	}
    }
    partList->SetSortingFunc(0, SortTextBox);
    partList->SetSortedColumn(0);
    partList->SortRows();
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
    valueScroll1 = (pawsScrollBar *)FindWidget("value_scroll1");        CS_ASSERT(valueScroll1);
    valueScroll2 = (pawsScrollBar *)FindWidget("value_scroll2");        CS_ASSERT(valueScroll2);
    valueScroll3 = (pawsScrollBar *)FindWidget("value_scroll3");        CS_ASSERT(valueScroll3);
    valueScroll4 = (pawsScrollBar *)FindWidget("value_scroll4");        CS_ASSERT(valueScroll4);

    HideValues();

    return true;
}

void EEditParticleListToolbox::UpdateParticleValue()
{
    size_t num = parmList->GetSelectedRowNum();
    pawsListBoxRow * row = parmList->GetRow(num);
    pawsTextBox* colName = (pawsTextBox *)row->GetColumn(0);
    csString name = colName->GetText();
    pawsTextBox* colType = (pawsTextBox *)row->GetColumn(1);
    csString type = colType->GetText();
    pawsTextBox* colValue = (pawsTextBox *)row->GetColumn(2);
    if (type == "F")
    {
	float value = valueNumSpinBox->GetValue();
        num = editList->GetSelectedRowNum();
	if (num >= emitters.GetSize())
	    ChangeParticleValue (effectors[num-emitters.GetSize()], name, value);
	else
	    ChangeParticleValue (emitters[num], name, value);
	csString valueString;
	valueString.Format ("%g", value);
	colValue->SetText (valueString);
    }
    else if (type == "B")
    {
	bool value = valueBool->GetState();
        num = editList->GetSelectedRowNum();
	if (num >= emitters.GetSize())
	    ChangeParticleValue (effectors[num-emitters.GetSize()], name, value);
	else
	    ChangeParticleValue (emitters[num], name, value);
	colValue->SetText (value ? "true" : "false");
    }
    else if (type == "MM")
    {
	float value1 = valueNumSpinBox->GetValue();
	float value2 = value2NumSpinBox->GetValue();
        num = editList->GetSelectedRowNum();
	if (num >= emitters.GetSize())
	    ChangeParticleValue (effectors[num-emitters.GetSize()], name, value1, value2);
	else
	    ChangeParticleValue (emitters[num], name, value1, value2);
	csString valueString;
	valueString.Format ("%g , %g", value1, value2);
	colValue->SetText (valueString);
    }
    else if (type == "V3")
    {
	float value1 = valueNumSpinBox->GetValue();
	float value2 = value2NumSpinBox->GetValue();
	float value3 = value3NumSpinBox->GetValue();
	csVector3 v (value1, value2, value3);
        num = editList->GetSelectedRowNum();
	if (num >= emitters.GetSize())
	    ChangeParticleValue (effectors[num-emitters.GetSize()], name, v);
	else
	    ChangeParticleValue (emitters[num], name, v);
	csString valueString;
	valueString.Format ("%g , %g , %g", value1, value2, value3);
	colValue->SetText (valueString);
    }
    else if (type == "*")
    {
	csString value = valueChoices->GetSelectedRowString();
        num = editList->GetSelectedRowNum();
	if (num >= emitters.GetSize())
	    ChangeParticleValue (effectors[num-emitters.GetSize()], name, value);
	else
	    ChangeParticleValue (emitters[num], name, value);
	colValue->SetText (value);
    }
    else if (type == "LC")
    {
	float value = valueNumSpinBox->GetValue();
	float r = valueScroll1->GetCurrentValue();
	float g = valueScroll2->GetCurrentValue();
	float b = valueScroll3->GetCurrentValue();
	float a = valueScroll4->GetCurrentValue();

        num = editList->GetSelectedRowNum();
	if (num >= emitters.GetSize())
	    ChangeParticleValue (effectors[num-emitters.GetSize()], name, value, r, g, b, a);
	else
	    ChangeParticleValue (emitters[num], name, value, r, g, b, a);
	csString valueString;
	valueString.Format("%g (%g , %g , %g , %g)\n", value, r, g, b, a);
	colValue->SetText (valueString);
    }
}

bool EEditParticleListToolbox::OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget == valueBool->GetButton())
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
	FillEditList (name);
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
    printf ("OnChange\n"); fflush (stdout);
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
    }
}

void EEditParticleListToolbox::OnListAction(pawsListBox* selected, int status)
{
    if (selected == editList)
    {
        size_t num = editList->GetSelectedRowNum();
	if (num >= emitters.GetSize())
	    FillParmList (effectors[num-emitters.GetSize()]);
	else
	    FillParmList (emitters[num]);
    }
    else if (selected == valueChoices->GetChoiceList())
    {
	UpdateParticleValue();
    }
    else if (selected == parmList)
    {
	HideValues();

        size_t num = parmList->GetSelectedRowNum();
	pawsListBoxRow * row = parmList->GetRow(num);
	pawsTextBox* colType = (pawsTextBox *)row->GetColumn(1);
	csString type = colType->GetText();
	pawsTextBox* colValue = (pawsTextBox *)row->GetColumn(2);
	csString value = colValue->GetText();
	if (type == "F")
	{
	    ParameterData& pd = parameterData[num];
	    valueNumSpinBox->SetRange(pd.spinbox_min, pd.spinbox_max, pd.spinbox_step);
	    valueNumSpinBox->Show();
	    float f;
	    csScanStr(value, "%f", &f);
	    valueNumSpinBox->SetValue (f);
	}
	else if (type == "B")
	{
	    ParameterData& pd = parameterData[num];
	    valueBool->SetText (pd.text);
	    valueBool->Show();
	    bool f;
	    csScanStr(value, "%b", &f);
	    valueBool->SetState (f);
	}
	else if (type == "MM")
	{
	    ParameterData& pd = parameterData[num];
	    valueNumSpinBox->SetRange(pd.spinbox_min, pd.spinbox_max, pd.spinbox_step);
	    valueNumSpinBox->Show();
	    value2NumSpinBox->SetRange(pd.spinbox_min, pd.spinbox_max, pd.spinbox_step);
	    value2NumSpinBox->Show();
	    float f1, f2;
	    csScanStr(value, "%f , %f", &f1,&f2);
	    valueNumSpinBox->SetValue (f1);
	    value2NumSpinBox->SetValue (f2);
	}
	else if (type == "V3")
	{
	    ParameterData& pd = parameterData[num];
	    valueNumSpinBox->SetRange(pd.spinbox_min, pd.spinbox_max, pd.spinbox_step);
	    valueNumSpinBox->Show();
	    value2NumSpinBox->SetRange(pd.spinbox_min, pd.spinbox_max, pd.spinbox_step);
	    value2NumSpinBox->Show();
	    value3NumSpinBox->SetRange(pd.spinbox_min, pd.spinbox_max, pd.spinbox_step);
	    value3NumSpinBox->Show();
	    float f1, f2, f3;
	    csScanStr(value, "%f , %f , %f", &f1,&f2,&f3);
	    valueNumSpinBox->SetValue (f1);
	    value2NumSpinBox->SetValue (f2);
	    value3NumSpinBox->SetValue (f3);
	}
	else if (type == "*")
	{
	    ParameterData& pd = parameterData[num];
	    valueChoices->Clear();
	    for (size_t i = 0 ; i < pd.choices.GetSize() ; i++)
	        valueChoices->NewOption(pd.choices[i]);
	    valueChoices->Show();
	    valueChoices->Select(value);
	}
	else if (type == "LC")
	{
	    float f, r, g, b, a;
	    csScanStr(value, "%f (%f , %f , %f, %f)", &f, &r, &g, &b, &a);
	    valueNumSpinBox->SetRange(0, 1, .1);
	    valueScroll1->SetMinValue(0);
	    valueScroll1->SetMaxValue(1);
	    valueScroll2->SetMinValue(0);
	    valueScroll2->SetMaxValue(1);
	    valueScroll3->SetMinValue(0);
	    valueScroll3->SetMaxValue(1);
	    valueScroll4->SetMinValue(0);
	    valueScroll4->SetMaxValue(1);
	    valueNumSpinBox->SetValue (f);
	    valueScroll1->SetCurrentValue(r);
	    valueScroll2->SetCurrentValue(g);
	    valueScroll3->SetCurrentValue(b);
	    valueScroll4->SetCurrentValue(a);

	    //@@@ TTL can't be edited right now: valueNumSpinBox->Show();
	    valueScroll1->Show();
	    valueScroll2->Show();
	    valueScroll3->Show();
	    valueScroll4->Show();
	}
    }
}

