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

#include <iutil/object.h>
#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <imesh/object.h>
#include <imesh/particles.h>

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

void EEditParticleListToolbox::FillList(iEngine* engine)
{
    EEditParticleListToolbox::engine = engine;

    if (!partList)
        return;

    partList->Clear();
    
    size_t a=0;
    iMeshFactoryList* factlist = engine->GetMeshFactories ();
    for (size_t i = 0 ; i < factlist->GetCount (); i++)
    {
        iMeshFactoryWrapper* fact = factlist->Get (i);
	csRef<iParticleSystemFactory> pfact = scfQueryInterface<iParticleSystemFactory> (
		fact->GetMeshObjectFactory());
	if (!pfact)
	    continue;

        partList->NewRow(a);
        pawsListBoxRow * row = partList->GetRow(a);
        if (!row)
            return;

        pawsTextBox * col = (pawsTextBox *)row->GetColumn(0);
        if (!col)
            return;
        
        csString partName = fact->QueryObject()->GetName();
        col->SetText(partName);
        if (editApp->GetCurrParticleSystemName() == partName)
            partList->Select(row);

        ++a;
    }
    partList->SetSortingFunc(0, SortTextBox);
    partList->SetSortedColumn(0);
    partList->SortRows();
}

#if 0
void EEditParticleListToolbox::SelectEffect(const csString & effectName)
{
    for (size_t a=0; a<partList->GetRowCount(); ++a)
    {
        if (partList->GetTextCellValue(a, 0) == effectName)
            partList->GetTextCell(a, 0)->SetColour(0x00ffff);
        else
            partList->GetTextCell(a, 0)->SetColour(0xffffff);
    }
}
#endif

bool EEditParticleListToolbox::PostSetup()
{
    partList = (pawsListBox *)FindWidget("part_list");                  CS_ASSERT(partList);
    openPartButton = (pawsButton *)FindWidget("load_part_button");      CS_ASSERT(openPartButton);
    refreshButton = (pawsButton *)FindWidget("refresh_button");         CS_ASSERT(refreshButton);

    return true;
}

bool EEditParticleListToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget == openPartButton)
    {
        editApp->CreateParticleSystem (partList->GetTextCellValue(partList->GetSelectedRowNum(), 0));
        return true;
    }
    else if (widget == refreshButton)
    {
	FillList(engine);
        return true;
    }
    return false;
}

void EEditParticleListToolbox::OnListAction(pawsListBox* selected, int status)
{
    //if (status == LISTBOX_SELECTED && selected == partList)
        //editApp->SetCurrEffect(partList->GetTextCellValue(partList->GetSelectedRowNum(), 0));
}
