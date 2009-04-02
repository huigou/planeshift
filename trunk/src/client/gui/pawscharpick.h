#ifndef PAWS_CHAR_PICKER_HEADER
#define PAWS_CHAR_PICKER_HEADER

#include "paws/pawsstringpromptwindow.h"
#include "paws/pawswidget.h"
#include "net/message.h"
#include "net/msghandler.h"

#include "psengine.h"

class pawsObjectView;
class psCharAppearance;

#define MAX_CHARS 10

struct Model
{
    csString fileName;
    csString factName;
    csString race;
    csString traits;
    csString equipment;
};
    
/** Where players can pick which character ( from the account ) that they to 
 *  play as.
 *  
 *  From this window you can also select to create a new character and enter
 *  that phase. This is connected to the network and listens for MSGTYPE_CHARACTERDATA
 *  type messages.
 */
class pawsCharacterPickerWindow: public pawsWidget, public psClientNetSubscriber, public iOnStringEnteredAction, public DelayedLoader
{
public:
    pawsCharacterPickerWindow();
    ~pawsCharacterPickerWindow();
    
    bool PostSetup();
    void HandleMessage( MsgEntry* me );
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    void Show();

    void SelectCharacter(int character, pawsWidget* widget);
    void SelectCharacter(int character);   

    // This is used ONLY by psClientDR to let the user join if we got the MsgStrings we need
    void ReceivedStrings();
    
    void Draw();
    
    void StoreHashedPassword(csString passwordHash) {passHash = passwordHash;}

    void CheckLoadStatus();
private:

    /// Creates the character creation screens.
    void SetupCharacterCreationScreens();
  
    /// Disconnects the client from server and returns to login window
    void ReturnToLoginWindow();

    /// Tracks if the character creations screens have been loaded or not.
    bool characterCreationScreens;
    
    virtual void OnStringEntered(const char *name, int param,const char *value);

    csRef<MsgHandler> msgHandler;
    
    int charactersFound;
    int selectedCharacter;

    bool connecting;
    bool gotStrings;
    int lastResend;

    pawsObjectView* view;
    Model models[MAX_CHARS];

    csString passHash;
    psCharAppearance* charApp;
    bool loaded;
};

CREATE_PAWS_FACTORY( pawsCharacterPickerWindow );


#endif

