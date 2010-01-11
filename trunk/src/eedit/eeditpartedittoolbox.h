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

#ifndef EEDIT_PARTEDIT_TOOLBOX_HEADER
#define EEDIT_PARTEDIT_TOOLBOX_HEADER

#include "eedittoolbox.h"
#include "paws/pawswidget.h"
#include <imesh/particles.h>

class pawsButton;
class pawsListBox;

/** This allows you to open/edit particle systems.
 */
class EEditParticleEditToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditParticleEditToolbox>
{
public:
    EEditParticleEditToolbox();
    virtual ~EEditParticleEditToolbox();

    /** Fills the particle system with the names of all particle systems in the engine.
     */
    void FillList(iParticleSystemFactory* partsysfact);

    /** Selects the given effect and highlights it in the list.
     *   @param effectName the name of the effect to select.
     */
    //void SelectEffect(const csString & effectName);

    // inheritted from EEditToolbox
    virtual void Update(unsigned int elapsed);
    virtual size_t GetType() const;
    virtual const char * GetName() const;

    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual void OnListAction(pawsListBox* selected, int status);
    
private:
    // used by pawsListBox to sort the listbox
    static int SortTextBox(pawsWidget * widgetA, pawsWidget * widgetB);

    csRef<iParticleSystemFactory> partsysfact;
    pawsListBox * partList;
    pawsButton  * openPartButton;
    pawsButton  * refreshButton;
};

CREATE_PAWS_FACTORY(EEditParticleEditToolbox);

#endif 
