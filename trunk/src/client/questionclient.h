/*
 * questionclient.h - Author: Ondrej Hurt
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef __QUESTION_CLIENT_H__
#define __QUESTION_CLIENT_H__

#include <csutil/hash.h>
#include "net/cmdbase.h"

class MsgHandler;
struct iObjectRegistry;
class pawsYesNoBox;

/** psQuestion is superclass of all question types.*/
class psQuestion
{
public:
    psQuestion(uint32_t questionID) { this->questionID = questionID; }
    virtual ~psQuestion() {};
    
    /** Cancels answering of this question, for example closes some PAWS window 
      * Question deletes itself. */
    virtual void Cancel() = 0;
protected:
    uint32_t questionID;
};


/**
  * The psQuestionClient class manages answering to various questions sent from server
  * to user. It keeps list of the questions that have not been answered yet.
 */
class psQuestionClient : public psClientNetSubscriber
{
public:
    psQuestionClient(MsgHandler* mh, iObjectRegistry* obj);
    virtual ~psQuestionClient();

    // iNetSubscriber interface
    virtual void HandleMessage(MsgEntry *msg);
    
    /** Delete question from list of pending questions */
    void DeleteQuestion(uint32_t questionID);

    /** Sends response to given question to server */
    void SendResponseToQuestion(uint32_t questionID, const csString & answer);
    
protected:
    
    /** Handlers of different question types: */
    void HandleConfirm(uint32_t questionID, const csString & question);
    void HandleDuel(uint32_t questionID, const csString & question);
    void HandleSecretGuildNotify(uint32_t questionID, const csString & question);
    
    MsgHandler* messageHandler;
    
    csHash<psQuestion*> questions;
};

#endif

