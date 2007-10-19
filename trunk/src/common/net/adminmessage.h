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
#define ADMIN_ITEM_REQUEST_ALL   0x00000001
#define ADMIN_ITEM_REQUEST_TYPES 0x00000002
#define ADMIN_ITEM_CREATE        0x00000003

#define ADMIN_MALE_RACES              0x00000004
#define ADMIN_FEMALE_RACES            0x00000005
#define ADMIN_NEUTRAL_RACES           0x00000006
#define ADMIN_RACES_LIST              0x00000007

#define ADMIN_REQUEST_KNOWLEDGE_AREAS 0x00000008
#define ADMIN_CREATE_NEW_NPC          0x00000009
#define ADMIN_REQUEST_TRIGGERS        0x00000010
#define ADMIN_REQUEST_RESPONSES       0x00000011
#define ADMIN_DIALOG_CHANGE           0x00000012

PS_TYPEDEF_GROWING_ARRAY(stringList, psString*);
PS_TYPEDEF_GROWING_ARRAY(idList, int);
class psDialogManager;

/** \addtogroup Admin Common Admin Library 
 * \@{*/

 
 //! Main admin message cracker. 
 /*!
 * This class will either construct a message based on a command and an XML
 * string of data or will take an admin message and crack out the command and
 * XML data.
 */
class psAdminMessage : public psMessageCracker
{
public:

    //! Construct a new admin message.
    /*! 
     * Builds up an admin message using a command and data
     * @param clientnum The client to go to. 0 for server.
     * @param command One of the many admin commands available
     * @param data The data to go with this command.
     */
    psAdminMessage( uint32_t clientnum, 
                    uint32_t command, 
                    const char* data );

    
    //! Crack out the message into the commands and data. 
    /*! 
     * Will take a network message and break it out into it's command 
     * and command data.
     * @param mesgEntry The network message
     */
    psAdminMessage(  MsgEntry *mesgEntry );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param access_ptrs A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(AccessPointers * access_ptrs);


    /// The command that this message has. 
    int command;

    /// The command data
    csString data;
};


//--------------------------------------------------------------------------

//! A list of triggers
/*!
 * This is a helper class that can break/construct an XML string of triggers.
 * All triggers are stored in the stringList called triggers. 
 */
class psAdminTriggerList
{
public:
    //! Default constuctor
    psAdminTriggerList();

    //! Construct Trigger list from data
    psAdminTriggerList( csString &str );
    ~psAdminTriggerList();

    //! Add new trigger to the list.
    void AddTrigger(const char* name);

    //! Build the XML data for network use.
    const char* XML();

    /// List of trigger strings.
    stringList triggers;

    /// If used on the server side, stores the id of this trigger.
    idList id;

private:
    //! The string to go to the network.
    csString network;
};

//--------------------------------------------------------------------------
//! A list of responses
/*!
 * This helper class is used to construct the network data for a response to 
 * a dialog trigger. It should have a max of 5 responses since that is all the
 * database supports at the moment. 
 */ 
class psAdminResponseList
{
public:
    psAdminResponseList();
    //! Construct from data
    psAdminResponseList( csString& str );
    ~psAdminResponseList();

    //! Add the trigger this response is for. 
    void AddTrigger( const char* trigger );

    //! Add the area this response is for.
    void AddArea( const char* area );

    //! Insert a new response to the list
    void AddResponse( const char* response );

    //! Create the pronoun set for this dialog response.
    /*!
     * All 5 responses to the trigger must share the same pronoun set. 
     */
    void AddPronounSet( const char* him,
                        const char* her,
                        const char* it,
                        const char* them );

    const char* GetTrigger() { return trigger; }
    const char* GetArea()    { return area; }
    const char* GetPronounHim() { return him; }
    const char* GetPronounHer() { return her; }
    const char* GetPronounIt()  { return it; }
    const char* GetPronounThem(){ return them; }

    //! Construct the XML data to go out to the network. 
    const char* XML(); 

    /// Store the responses.
    stringList responses;

private:
    psString him;
    psString her;
    psString it;
    psString them;

    psString trigger;
    psString area;

    //! The string to go to the network.
    psString network;
};

//! A list of races
/*!
 * The helper class helps to send a list of races across the network. 
 * All races are stored in the vector array races. 
 */
class psAdminRaceList
{
public:
    //! Build A race list from data.
    psAdminRaceList(csString& data);
    psAdminRaceList();
   ~psAdminRaceList();

    //! Add a new race to the list.
    void AddRace(const char* race);
 
    //! Construct the XML data to go out to the network. 
    const char* XML();

   //! A vector array of races
   stringList races; 

private:
    //! The string to go to the network.
    csString network;
};


//--------------------------------------------------------------------------

//! A list of NPC dialog knowledge areas 
/*!
 * The helper class helps to send a list of knowledge areas across the 
network. 
 */
class psAdminKnowledgeAreaList
{
public:
    //! Constuct a knowledge area list from data.
    psAdminKnowledgeAreaList(csString& data);
    psAdminKnowledgeAreaList();
    ~psAdminKnowledgeAreaList();

    void AddArea(const char* data);

    //! Construct the string for this list that should go over the network.
    const char* XML();

    //! A vector of knowledge areas 
    stringList areas;

private:
    //! The string to go to the network.
    csString network;
};


//--------------------------------------------------------------------------

//! A complete set of triggers/responses for an NPC dialog. 
/*!
 *  This Helper Class basicly maintains a list of triggers and responses that
 *  a NPC has.  
 */
class psAdminNPCDialogSet
{
public:
    ~psAdminNPCDialogSet();

    //! Crack out the data.
    /*! This constucts many sets of dialog from the XML string.
     * @param data The XML string that has the data. 
     */
    void Crack ( const char* data );

    //! A dialog/response set
    struct dialog
    {
        //! List of triggers that the response is good for.
        psAdminTriggerList  triggers; 

        //! A ResponseSet that is good for 5 responses.
        psAdminResponseList responses;

        // If used on the server, stores the id of the response.
        int databaseResponseID;

        // Stores the prior response that this dialog set has. 
        int prior;
    };
    
   PS_TYPEDEF_GROWING_ARRAY(dialogVector, dialog*); 

   dialogVector dialogSet;
};


//--------------------------------------------------------------------------

//! A complete NPC specified in XML. 
/*!
 *  This Helper Class is used to get a NPC definition from an XML file that 
 *  can be easily sent across the network to create in the database. 
 */
class psAdminNPCData
{
public:
    psAdminNPCData(csString& data);
    psAdminNPCData();
    ~psAdminNPCData();

    void SetGender(char sex);
    void SetName(const char* name);
    void SetRace(const char* race);
    void SetPosition(const char* x, const char* y, const char* z,
                     const char* xrot, const char* yrot, const char* zrot,
                     const char* sector );

    void AddKnowledgeArea(const char* area);
    char* XML();

    const char* GetName();
    const char* GetRace();
    char GetGender();

    const char* GetHair();
    const char* GetEyes();
    const char* GetSkin();
    const char* GetClothes();

    csVector3 GetPosition();
    csVector3 GetRotation();
    const char* GetSector();

    stringList knowledgeAreas;
    psAdminNPCDialogSet dialog;

    psDialogManager* dialogManager;

private:
    csString name;
    csString race;
    csString sex;

    // used for char customization (those may change in the future)
    csString hair;
    csString eyes;
    csString skin;
    csString clothes;

    csVector3 pos;
    csVector3 rot;
    csString  sector;

    //! The string to go to the network.
    csString network;

};
/*\@}*/


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


