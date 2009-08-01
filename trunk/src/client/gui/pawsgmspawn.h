/*
 * pawsgmspawn.h - Author: Christian Svensson
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_GMSPAWN_HEADER
#define PAWS_GMSPAWN_HEADER

#include "paws/pawswidget.h"
#include "psengine.h"

class pawsSimpleTree;
class pawsObjectView;
class pawsTextBox;
class pawsEditTextBox;
class pawsCheckBox;

class pawsGMSpawnWindow : public pawsWidget, public psClientNetSubscriber, public DelayedLoader
{
public:
    pawsGMSpawnWindow();
    virtual ~pawsGMSpawnWindow();

    bool PostSetup(); 
    void Show();
    void HandleMessage(MsgEntry* me);
    bool OnSelected(pawsWidget* widget);

    bool OnButtonPressed(int button,int keyModifier,pawsWidget* widget);

    bool CheckLoadStatus();

private:
    pawsSimpleTree*                 itemTree;
    pawsObjectView*                 objView;
    pawsWidget*                     itemImage;
    pawsTextBox*                    itemName;
    pawsCheckBox*                   cbForce;
    pawsCheckBox*                   cbLockable;
    pawsCheckBox*                   cbLocked;
    pawsCheckBox*                   cbPickupable;
    pawsCheckBox*                   cbCollidable;
    pawsEditTextBox*                itemCount;
    pawsEditTextBox*                lockSkill;
    pawsEditTextBox*                lockStr;
    pawsTextBox*                    factname;
    pawsTextBox*                    meshname;
    pawsTextBox*                    imagename;

    struct Item
    {
        csString name;
        csString category;
        csString mesh;
        csString icon;
    };

    csArray<Item>   items;

    csString currentItem;
    bool loaded;
    csString factName;
};


CREATE_PAWS_FACTORY( pawsGMSpawnWindow );
#endif
