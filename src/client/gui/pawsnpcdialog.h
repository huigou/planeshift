/*
 * Author: Christian Svensson
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_NPC_DIALOG
#define PAWS_NPC_DIALOG

#include "net/subscriber.h"
#include "pscelclient.h"

#include "paws/pawslistbox.h"
#include "paws/pawswidget.h"
#include "paws/pawsstringpromptwindow.h"

#define CONFIG_NPCDIALOG_FILE_NAME       "/planeshift/userdata/options/npcdialog.xml"
#define CONFIG_NPCDIALOG_FILE_NAME_DEF   "/planeshift/data/options/npcdialog_def.xml"


class pawsListBox;

/** This window shows the popup menu of available responses
 *  when talking to an NPC.
 */
class pawsNpcDialogWindow: public pawsWidget, public psClientNetSubscriber, public iOnStringEnteredAction
{
public:
    struct QuestInfo
    {
        csString    title;
        csString    text;
        csString    trig;
    };
    pawsNpcDialogWindow();

    /**
     * Aquire pointers to widgets and load settings for the npc dialog.
     */
    bool PostSetup();
    
    void HandleMessage(MsgEntry* me);

    void OnListAction(pawsListBox* widget, int status);

    void OnStringEntered(const char *name,int param,const char *value);

    /**
     * Loads the settings from the config files and sets the window
     * appropriately.
     * @return TRUE if loading succeded
     */
    bool LoadSetting();

    /**
     * Shows the window only if it's in bubbles mode, else does nothing.
     */
    void ShowIfBubbles();

    /**
     * empties all bubbles structures and hides them.
     */
    void CleanBubbles();

    /**
     * Hides all bubbles except freetext and bye.
     */
    void ShowOnlyFreeText();

    /**
     * Shows the window and applies some special handling to fix up the window
     * Behaviour and graphics correctly depending if we use the classic menu
     * or the bubble menu
     */
    virtual void Show();
    virtual void Hide();
    bool OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers);
    bool OnMouseDown(int button, int modifiers, int x , int y);
    /**
     * callback function called when a pawsButton (bubble) is clicked
     */
    bool OnButtonPressed(int button, int keyModifier, pawsWidget* widget);

    // This window should always stay on the background
    virtual void BringToTop(pawsWidget* widget) {};

    /**
     * @brief Load quest info from xmlbinding message
     *
     * @param xmlstr An xml string which contains quest info
     *
     */
    void LoadQuest(csString xmlstr);

    /**
     * @brief Display quests in bubbles.
     *
     * @param index From which index in questInfo array the quest info will be displayed in bubbles.
     *
     */
    void DisplayQuestBubbles(unsigned int index);

    /**
     * @brief Shows the npc chat window and hides the menu bubbles.
     */
    void ShowSpeechBubble();
    
    /**
     * @brief Display NPC's chat text
     *
     * @param inText Content that the NPC's chat text
     * @param actor The target NPC name
     */
    void NpcSays();//csString& inText, GEMClientActor *actor);

    /**
     * Handles timing to make bubbles disappear in the bubbles npc dialog mode.
     */
    virtual void Draw();

    /**
     * Has a real functionality only when using the bubbles npc dialog.
     * It will avoid drawing the background in order to make it transparent
     */
    virtual void DrawBackground();

    /**
     * Getter for useBubbles, which states if we use the bubbles based
     * npcdialog interface.
     * @return TRUE if we are using the bubbles based npc dialog interface
     *         FALSE if we are using the menu based npc dialog interface
     */
    bool GetUseBubbles()
    {
        return useBubbles;
    }
    /**
     * Sets if we have to use the bubbles based npc dialog interface (true)
     * or the classic menu based one (false).
     * @note This doesn't reconfigure the widgets for the new modality.
     */
    void SetUseBubbles(bool useBubblesNew)
    {
        useBubbles = useBubblesNew;
    }
    /**
     * Getter for npc messsage timeout.
     * The timeout determines how long a npc message is displayed.
     * Apparently this is just a scaling factor for shortening or
     * prolonging the display.
     */
    float GetNpcMsgTimeoutScale()
    {
        return npcMsgTimeoutScale;
    }
    /**
     * Setter for npc messsage timeout.
     * The timeout determines how long a npc message is displayed.
     * Apparently this is just a scaling factor for shortening or
     * prolonging the display.
     * @param timeoutScale is multiplied with the real factor.
     */
    void SetNpcMsgTimeoutScale(float timeoutScale)
    {
        if (timeoutScale < 0)
        {
            npcMsgTimeoutScale = 0.0;
        } else if (timeoutScale > npcMsgTimeoutScaleMax)
        {
            npcMsgTimeoutScale = npcMsgTimeoutScaleMax;
        } else
        {
            npcMsgTimeoutScale = timeoutScale;
        }
    }
    /**
     * Getter for npc messsage timeout.
     * The maximum timeout determines how long a npc message is
     * displayed at maximum.
     * @returns maximum scaling factor
     */
    float GetNpcMsgTimeoutScaleMax()
    {
        return npcMsgTimeoutScaleMax;
    }

    /**
     * Saves the setting of the menu used for later use
     */
    void SaveSetting();

    /**
     * Sets the widget appropriate for display depending on the type of
     * npc dialog menu used
     */
    void SetupWindowWidgets();


private:
    void AdjustForPromptWindow();
    /**
     * Handles the display of player text in chat
     */
    void DisplayTextInChat(const char *sayWhat);
    
    // values are loaded from configuration files
    bool useBubbles;                    ///< Stores which modality should be used for the npcdialog (bubbles/menus)
    float npcMsgTimeoutScale;           ///< stores how long an npc message is displayed (scaling factor only!)
    float npcMsgTimeoutScaleMax;        ///< stores the maximum time for how long an npc message is displayed  (scaling factor only!)

    // variables used to display npc dialogs
    csArray<QuestInfo> questInfo;       ///< Stores all the quest info and triggers parsed from xml binding.
    unsigned int    displayIndex;       ///< Index to display which quests
    int             cameraMode;         ///< Stores the camera mode
    int             loadOnce;           ///< Stores if bubbles has been loaded
    bool clickedOnResponseBubble;       ///< flag when player clicks on the response bubble 
    int questIDFree;                    ///< Keeps the value of the quest if the free text question was triggered.
    bool gotNewMenu;                    ///< keeps track of the incoming new menu message
    bool displaysNewMenu;               ///< set to true when displaying a new menu
    int triesNewMenu;                   ///< tries to fetch a new menu
    csTicks timeDelay;                  ///< stores the calculated time needed to read the last npc say (in ticks).
    csTicks ticks;                      ///< point in time when displaying a message started

    pawsListBox* responseList;
    pawsWidget* speechBubble;
    pawsEditTextBox* textBox;
    pawsButton* closeBubble;
    pawsButton* giveBubble;
    
    EID targetEID;                      ///< The eid of the current target used to hide the dialog if the actor is removed.
    
    csArray<csString> npcMsgQueue;        ///< list of text messages from a npc (sort of queue).
};


CREATE_PAWS_FACTORY(pawsNpcDialogWindow);
#endif
