#include <csutil/hash.h>
#include <csutil/set.h>
#include "net/messages.h"        // Message definitions
#include "net/msghandler.h"         // Network access
#include "msgmanager.h"             // Parent class
#include "gem.h"

class IntroductionManager : public MessageManager
{
public:
    IntroductionManager();
    ~IntroductionManager();

    bool Introduce(unsigned int charid, unsigned int targetcharid);
    bool UnIntroduce(unsigned int charid, unsigned int targetcharid);
    bool IsIntroduced(unsigned int charid, unsigned int targetcharid);

    bool LoadCharIntroductions(unsigned int charid);
    bool UnloadCharIntroductions(unsigned int charid);

    virtual void HandleMessage(MsgEntry *pMsg,Client *client);

protected:
    csHash< csSet<unsigned int>* > introMap;
};

