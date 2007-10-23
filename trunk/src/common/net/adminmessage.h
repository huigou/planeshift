/*
 * adminmessage.h
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
 * Helper classes for cracking Admin data
 */
#ifndef ADMIN_MESSAGE_H
#define ADMIN_MESSAGE_H

#include <csgeom/vector3.h>

#include "net/message.h"
#include "net/messages.h"
#include <util/growarray.h>
#include "util/psxmlparser.h"

#define ADMIN_ACCESS             0x00000000

PS_TYPEDEF_GROWING_ARRAY(stringList, psString*);
PS_TYPEDEF_GROWING_ARRAY(idList, int);
class psDialogManager;

class psTriggerBlock;
class psAttitudeBlock;
class psSpecialResponse;
class psResponse;

PS_TYPEDEF_GROWING_ARRAY( triggerBlockList, psTriggerBlock* );
PS_TYPEDEF_GROWING_ARRAY( specialResponseList, psSpecialResponse* );
PS_TYPEDEF_GROWING_ARRAY( attitudeList, psAttitudeBlock* );

//! This is used to handle <specificknowledge> and <specialresponse> tags.
class psDialogManager
{
public:
    //! Add a trigger to the list of triggers.
    void AddTrigger( psTriggerBlock* trigger );

    //! Add a special response to the internal list.
    void AddSpecialResponse( psSpecialResponse* response );

    //! A debuging function that prints all the trigger data. 
    void PrintInfo();

    //! A list of all the triggers.
    /*!
     * This is a simple list of the all the trigger data that needs
     * to be created. It is of type psTriggerBlock.
     */
    triggerBlockList triggers;


    //! A list of all the special responses.
    /*!
     * This is a simple list of the all the special response data that needs
     * to be created. It is of type psSpecialResponse.
     */
    specialResponseList special;

    //! Maps a internal ID to a database ID.
    /*! 
     * When responses are loaded from a file they are given internal IDs.
     * When they are added to the database they are given a proper database
     * id.  This function returns that database ID based on the internal one
     * passed in.
     */
    int GetPriorID( int internalID );

    //! Keep track of the database id's of the triggers.
    idList triggerIDs;

    //! Keep track of the database id's of responses.
    idList responseIDs;

    csString    area;

    bool WriteToDatabase();

    int  InsertResponse( csString& response );
    int  InsertResponseSet( psResponse &response );
    int  InsertTrigger( const char* trigger, const char* area, int maxAttitude, 
                        int minAttitude, int responseID, int priorID, int questID );
};
   
//--------------------------------------------------------------------------

//! A special NPC response.
/*!
 * This holds the data related to a special response. These are usually
 * fall back or error responses for NPCs.  The format of the tag is:
 *            <specialresponse reactionMax="max" 
 *                             reactionMin="min" 
 *                             type="type"
 *                             value="expression"/>
 */
class psSpecialResponse
{
public: 
    //! Create the response based on the given tag.
    psSpecialResponse( csRef<iDocumentNode> responseNode, int questID );

    //! The trigger name.
    csString type;

    //! The response given
    csString response;

    //! The min attitude required for this response.
    int attMin;

    //! The max attitude required for this response.
    int attMax;

    int questID;
};


//--------------------------------------------------------------------------

//! A set of trigger data. 
/*!
 * This manages all the data that is related to a trigger.  It consists of 
 * a list of phrases and a list of attitudes.  For each attitude and phrase
 * a new trigger will be generated.  So 3 phrases and 2 attitudes will 
 * generate 6 triggers. 
 */
class psTriggerBlock
{
public:
    psTriggerBlock( psDialogManager* mgr )
    {
        manager = mgr;
        priorResponseID = 0;
        questID = -1;
    }

    //! A list of phrases for this trigger. 
    /*! This allows us to create many triggers to map to many responses.
     */
    stringList phraseList;

    //! The list of attitudes. 
    /*! This contain the responses based on an attitude range.
     */
    attitudeList attitudes;
   
    psTriggerBlock() { priorResponseID = 0; } 

    //! This is a recursive function that creates triggers.
    void Create( csRef<iDocumentNode> triggerNode, int questID );
 
    // This sets the prior responseID for this trigger data. 
    void SetPrior( int id ) { priorResponseID = id; }

    // Holds the prior responseID.
    int priorResponseID;

    // Holds the real databaseID of this trigger.
    int databaseID;

    // The knowledge area of this trigger.
    csString area;

    //  Main mangaer.
    psDialogManager* manager;

    // questID of trigger
    int questID;
};


//--------------------------------------------------------------------------

//! A simple response 
class psResponse
{
public:
    //! The response lists.
    /*! Each response set can have up to 5 different phrases that are 
     * randomly picked from.
     */
    stringList responses;

    csString script;
    csString pronoun_him,
             pronoun_her,
             pronoun_it,
             pronoun_them;

    //! The internal ID given by the dialog manager.
    int givenID;

    //! The actual databaseID.
    int databaseID;

    //! The questID of the response
    int questID;
};


//--------------------------------------------------------------------------

//! This is an attitude class data that has the responses.
/*! The attitude block is a set of responses based on a particular attitude
 *  range.  This class keeps track of that data.
 */
class psAttitudeBlock
{
public:
    //! Used to generate the internal id's
    static int responseID;

    psAttitudeBlock( psDialogManager * mgr) 
    { 
        manager = mgr;
        responseSet.givenID = ++responseID; 
    }

    //! The response set for this attitude.
    psResponse responseSet;

    //! Populate the data for this class.
    void Create( csRef<iDocumentNode> attitudeNode, int questID );

    //! The max attitude that these responses are for.
    int maxAttitude;

    //! The min NPC attitude that these responses are for.
    int minAttitude;

    //! Main dialog manager.
    psDialogManager* manager;
};







#endif


