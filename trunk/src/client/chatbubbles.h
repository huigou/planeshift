/*
 * Author: Andrew Robberts
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

#ifndef CHAT_BUBBLES_HEADER
#define CHAT_BUBBLES_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <cstypes.h>
#include <csutil/ref.h>
#include <csutil/array.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/subscriber.h"

#include "effects/pseffectobjtextable.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psengine.h"

struct BubbleChatType
{
    int                 chatType;           // the chat type this settings will apply to
    psEffectTextRow     textSettings;       // the settings
    char                effectPrefix[64];   // the prefix of the effect name to apply, effects of name <prefix>longphrase, <prefix>normal, and <prefix>shortphrase should exist
};


class psChatBubbles : public iNetSubscriber
{
private:
    psEngine *      psengine;
    MsgHandler *    msgHandler;

    csArray<BubbleChatType> chatTypes;

    size_t bubbleMaxLineLen;            // maximum number of characters per line
    size_t bubbleShortPhraseCharCount;  // messages with fewer than this many characters get small bubble
    size_t bubbleLongPhraseLineCount;   // messages with more than this many lines get large bubble

public:
    psChatBubbles();
    virtual ~psChatBubbles();

    bool Initialize(psEngine * engine);

    bool Load(const char * filename);

    // implemented iNetSubscriber messages
    virtual bool Verify(MsgEntry * msg, unsigned int flags, Client *& client);
    virtual void HandleMessage(MsgEntry * msg, Client * client);
    
};

#endif // CHAT_BUBBLES_HEADER
