/*
 * chatwindow.h - Author: Andrew Craig
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

// chatwindow.h: interface for the pawsChatWindow class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_CHAT_WINDOW
#define PAWS_CHAT_WINDOW

// CS INCLUDES
#include <csutil/array.h>
#include <csutil/redblacktree.h>
#include <csutil/hashr.h>

#include "net/cmdbase.h"
//#include "util/psstring.h"
#include "util/stringarray.h"

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawsmenu.h"
#include "gui/pawsignore.h"
#include "gui/pawscontrolwindow.h"

#define CONFIG_CHAT_FILE_NAME       "/planeshift/userdata/options/chat.xml"
#define CONFIG_CHAT_FILE_NAME_DEF   "/planeshift/data/options/chat_def.xml"

enum E_CHAT_LOG {
    CHAT_LOG_ALL    = 0,
    CHAT_LOG_SYSTEM,
    CHAT_NLOG
};

enum CHAT_COMBAT_FILTERS {
    COMBAT_SUCCEEDED = 1,
    COMBAT_BLOCKED   = 2,
    COMBAT_DODGED    = 4,
    COMBAT_MISSED    = 8,
    COMBAT_FAILED    = 16,
    COMBAT_STANCE    = 32,
    COMBAT_TOTAL_AMOUNT = 6
};

extern const char *logWidgetName[];
extern const char *logFileName[];

class pawsMessageTextBox;
class pawsEditTextBox;
class pawsChatHistory;
class pawsTabWindow;
struct iSoundManager;

// Struct for returning and setting settings
struct ChatSettings
{
    /// Player Color.
    int playerColor;
    int chatColor;
    int systemColor;
    int adminColor;
    int npcColor;
    int tellColor;
    int guildColor;
    int shoutColor;
    int channelColor;
    int gmColor;
    int yourColor;
    int groupColor;
    int auctionColor;
    int helpColor;
    bool joindefaultchannel;
    bool looseFocusOnSend;
    bool logChannel[CHAT_NLOG];
    bool enableBadWordsFilterIncoming, enableBadWordsFilterOutgoing;
    bool echoScreenInSystem;
    bool mainBrackets; //If it's true brackets like [guild] [tell] will be put in main tab.
    bool yourColorMix; //If it's true the yourColor will be mixed with the Color of the destination 
                       //Example: if you send a tell the color of your text will be (yourColor+tellColor)/2
    csArray<csString> badWords;
    csArray<csString> goodWords;
    int selectTabStyle;
    int vicinityFilters; // Flags int
    int meFilters; // Flags int
};


/** Main Chat window for PlaneShift.
 * This class will handle most of the communication that goes on between
 * players and system messages from the server. This class also tracks the
 * ignore list on the client.
 */
class pawsChatWindow  : public pawsControlledWindow, public psCmdBase
{
public:
    pawsChatWindow();
    virtual ~pawsChatWindow();

    /** For the iNetSubsciber interface.
     */
    void HandleMessage( MsgEntry* message );

    /** For the iCommandSubscriber interface.
    */
    const char* HandleCommand( const char* cmd );

    bool OnMenuAction(pawsWidget * widget, const pawsMenuAction & action);
    bool PostSetup();


    bool OnMouseDown( int button, int modifiers, int x , int y );

    bool OnKeyDown( int keyCode, int key, int modifiers );

    /** Check to see if the text input box is active.
     *  @return true if the text box is active.
     */
    bool InputActive();

    virtual bool OnButtonPressed( int button, int keyModifier, pawsWidget* widget );
    void SwitchChannel(const csString &name, pawsWidget* widget = NULL);

    void PerformAction( const char* action );

    //Clear history
    void Clear();

    void AutoReply(void);

    //Sets the away message
    void SetAway(const char* text);

    virtual void OnLostFocus();
    virtual void Show();

    csString GetBracket(int type); ///generates a bracket like [tell] starting from the msgtype

    /// Function that handles output of various tabbed windows.
    void ChatOutput(const char* data, int colour = -1, int type = CHAT_SYSTEM, bool flashEnabled = true, bool hasCharName = false, int hotkeyChannel = 0);

    void AddAutoCompleteName(const char *name);
    void RemoveAutoCompleteName(const char *name);

    ChatSettings& GetSettings() {return settings;}
    void SetSettings(ChatSettings& newSets);

    void SaveChatSettings();
    void LoadChatSettings();
    
    void RefreshCommandList();

    pawsIgnoreWindow*  GetIgnoredList() { return IgnoredList; }
    
    /// Remove bad words from the output string
    void BadWordsFilter(csString& s);
    
    /// mixes two colours and returns their mixed colour
    int mixColours(int colour1, int colour2);
    
    // Leaves the channel and removes the hotkey association
    bool LeaveChannel(int hotkeyChannel);
    
    // Joins the channel and adds the hotkey when the server accepts the join.
    void JoinChannel(csString name);
    
protected:

    void HandleSystemMessage( MsgEntry* message );

    /// Sends the contents of the input text to the server.
    void SendChatLine(csString& inputText);

    /// Subscribe the player commands.
    void SubscribeCommands();

    /// Tab completion
    void TabCompleteCommand(const char *cmd);
    void TabCompleteName(const char *cmd);

    void DetermineChatTabAndSelect(int chattype);

    pawsIgnoreWindow*  IgnoredList;
    csString           replyList[4];
    int replyCount;

    csString awayText;

    /// list of names to auto-complete
    csArray<csString> autoCompleteNames;

    /// list of last commands
    csRedBlackTree<psString> commandList;
    csArray<csString> systemTriggers;
    csArray<csString> chatTriggers;

    /// Input box for quick access
    pawsEditTextBox* inputText;

    pawsTabWindow* tabs;

    /// Decides if we have a player name
    bool havePlayerName;

    /// Player name, with no upercase
    csString noCasePlayerName;
    csString noCasePlayerForename;

    /// Chat history
    pawsChatHistory* chatHistory;
    
    /// Current line, stored for when scrolling back in the chat history
    csString currLine;

    /// Chat can trigger sound effects with this
    iSoundManager *soundmgr;

    ChatSettings settings;

    csRef<iFile> logFile[CHAT_NLOG];
    
    void LogMessage(enum E_CHAT_LOG channel, const char* message, int type = CHAT_SAY);

    void CreateSettingNode(iDocumentNode* mNode,int color,const char* name);

    // Replay recent message history on load
    void ReplayMessages();
    
    // Subscribed channel name to server channel ID reversible mapping
    csHashReversible<uint32_t, csString> channelIDs;
    // Hotkeys for server channel IDs
    csArray<uint16_t> channels;
};

//--------------------------------------------------------------------------

CREATE_PAWS_FACTORY( pawsChatWindow );


//--------------------------------------------------------------------------
/** This stores the text the player has entered into their edit window.  It
 * keeps all the commands that the player has entered in sequential order.
 * This can be normal chatting or different /commands.
 */
class pawsChatHistory
{
public:
    pawsChatHistory();
    ~pawsChatHistory();

    /** Insert a new string that the player has entered.
     *  @param str The new string to add to the history.
     */
    void Insert(const char *str);

    /** Get a string that is in the history.
      * @param n The index of the string to get.
      * @return The string if the value of n is in range. NULL otherwise.
      */
    csString* GetCommand(int n);

    /// return the next (temporally) command from history
    csString* GetNext();

    /// return the prev (temporally) command from history
    csString* GetPrev();

    void SetGetLoc(unsigned int pos) { getLoc = pos; }

private:
    /// The current position we are in the buffer list compared to the end.
    unsigned int getLoc;

    /// Array of strings to store our history
    csPDelArray<csString> buffer;
};

#endif
