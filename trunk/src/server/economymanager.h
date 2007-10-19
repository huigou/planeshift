/*
 * economymanager.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Christian Svensson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Creation Date: 11/Jan 2005
 * Description : This is the header file for the economymanager
 *               This manager handles all the exchange/trade things and calculates
 *               prices and keeps histories of transactions and so on
 *
 */

#ifndef ECONOMYMANAGER_HEADER
#define ECONOMYMANAGER_HEADER

#include "msgmanager.h"             // Parent class
#include "net/messages.h"           // Message definitions
#include "net/msghandler.h"         // Network access
#include "util/gameevent.h"

#include <csutil/sysfunc.h>

struct TransactionEntity
{
    int from;
    int to;

    csString item;
    int count;
    int quality;
    unsigned int price;

    bool selling;
    int stamp;
};

class EconomyManager : public MessageManager
{
public:
    EconomyManager();
    ~EconomyManager();

    void HandleMessage(MsgEntry *me,Client *client);

    void AddTransaction(TransactionEntity* trans,bool sell);

    TransactionEntity* GetTransaction(int id);
    unsigned int GetTotalTransactions();
    void ClearTransactions();
    void ScheduleDrop(csTicks ticks,bool loop);

protected:
    csPDelArray<TransactionEntity> history;
    
};

class psEconomyDrop : public psGameEvent
{
protected:
    EconomyManager* economy;
    csTicks eachTime;
    bool loop;
public:
    psEconomyDrop(EconomyManager* manager,csTicks ticks,bool loop);
    void Trigger();
};

#endif

