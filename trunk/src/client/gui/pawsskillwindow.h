/*
 * pawsskillwindow.h
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

#ifndef PAWS_SKILL_WINDOW_HEADER
#define PAWS_SKILL_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "net/cmdbase.h"
#include "util/skillcache.h"
#include "gui/pawscontrolwindow.h"

class pawsTextBox;
class pawsListBox;
class pawsListBoxRow;
class pawsMultiLineTextBox;
class MsgHandler;
class pawsProgressBar;
class pawsObjectView;
class psCharAppearance;

/** Describes a skill description inside the GUI system.
 */
class psSkillDescription 
{
public:		
    psSkillDescription()
    {
        category = 0;
        description.Clear();
    }

    psSkillDescription(int cat, const char *str)
    {
        category = cat;
        description = str;
    }

    void Update(int cat, const char *str)
    {
        category = cat;
        description = str;
    }

    int GetCategory() const
    {
        return category;
    }

    const char *GetDescription() const
    {
        return (const char *)description;
    }

private:
    int category;
    csString description;
};

//-----------------------------------------------------------------------------

/** This handles all the details about how the skill window.
 */
class pawsSkillWindow : public pawsControlledWindow, public psClientNetSubscriber
{
public:
    pawsSkillWindow();
    virtual ~pawsSkillWindow();

    bool PostSetup();
    virtual void HandleMessage(MsgEntry *msg);
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    void OnListAction( pawsListBox* listbox, int status );

    virtual void Draw();
    virtual void Show();
    virtual void Hide();
    virtual void Close();
    
protected:

    bool SetupDoll();

    void BuySkill();
    
    void HandleSkillList(psSkillCache *skills, int selectedNameId = -1, int *rowIdx = NULL);
    void HandleSkillDescription( csString& description );
        
    void SelectSkill(int skill, int cat);

    /* This method is used for making the tab button blinking. It is used while training:
       a trainer can train skills in a certain category, so with the flashing of the tab 
       (the category) the player knows that a certain skill is available to be trained*/
    void FlashTabButton(const char* buttonName);

    /* This handles the skill list for each category */
    void HandleSkillCategory(pawsListBox* tabNameSkillList, const char* indWidget,
                             const char* tabName, psSkillCacheItem* skillInfo, int &idx);

    pawsListBox *statsSkillList, *combatSkillList, *magicSkillList, *jobsSkillList, *variousSkillList;
	pawsListBox *factionList;

    pawsMultiLineTextBox *combatSkillDescription, *magicSkillDescription, *jobsSkillDescription;
    pawsMultiLineTextBox *variousSkillDescription, *statsSkillDescription;
    pawsProgressBar *hpBar, *manaBar, *pysStaminaBar, *menStaminaBar, *experienceBar;
    pawsTextBox *hpFrac, *manaFrac, *pysStaminaFrac, *menStaminaFrac, *experiencePerc;

    csArray<pawsListBoxRow*> unsortedSkills; // Array keeping the server order of the skills

    bool filter, train, foundSelected;
    int x; //Stores topnode "X" information

    csString skillString;
    csString selectedSkill;
    
    int hitpointsMax, manaMax, physStaminaMax, menStaminaMax;

    int currentTab, previousTab; //Used for storing which is the current tab and the previous one
    
    csRef<iDocumentSystem> xml;
    csRef<MsgHandler> msgHandler;

    /* Local copy of skills and stats */
    psSkillCache skillCache;
    /* Local copy of skill and stat descriptions */
    csHash<psSkillDescription *> skillDescriptions;

	void HandleFactionMsg(MsgEntry* me);

	struct FactionRating
	{
		csString name;
		int      rating;
	};

	csPDelArray<FactionRating>   factions;		/// The factions by name

	/// Flag if we have sent our initial request for faction information. Only sent
	/// once and everything else is an update. 
	bool factRequest;	
														
    
    psCharAppearance* charApp;
};

CREATE_PAWS_FACTORY( pawsSkillWindow );

//-----------------------------------------------------------------------------

/** pawsSkillIndicator is a widget that graphically displays skill status */

class pawsSkillIndicator : public pawsWidget
{
public:
    pawsSkillIndicator();

    void Draw();
    void Set(int x, int rank, int y, int yCost, int z, int zCost);
protected:
    /* Calculates relative (to widget) horizontal coordinate of a point on the skill bar */
    int GetRelCoord(int pt);
    
    void DrawSkillProgressBar(int x, int y, int width, int height, 
                              int start_r, int start_g, int start_b);

    /* Skill status: */
    int x;
    int rank, y, yCost, z, zCost;

    iGraphics2D * g2d;
};

CREATE_PAWS_FACTORY( pawsSkillIndicator );

#endif 
