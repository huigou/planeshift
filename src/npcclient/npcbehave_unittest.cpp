/*
 * npcbehave_unittest.cpp by Andrew Dai
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/databuf.h>
#include <csutil/objreg.h>
#include <ctype.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>


#include "npcbehave.h"
#include "npcclient.h"
#include "npc.h"
// This requires googletest to be installed
#include <gtest/gtest.h>

// This should be compiled as a standalone app.
CS_IMPLEMENT_APPLICATION

psNPCClient* npcclient;

class BehaviorSetTest : public testing::Test {
protected:
    BehaviorSetTest() : behavior("test_behavior") {
        if (iSCF::SCF == 0)
            scfInitialize(0);
        objreg = new csObjectRegistry();
        
        csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
        csRef<iDocument> doc = xml->CreateDocument();
        
        csString testNpctypeXml = "<npctypes>"
                                  "<npctype name='type' vel='3'>"
                                  "<behavior name='goChase' completion_decay='100' growth='0' initial='10'>"
                                  "<chase type='target' anim='walk' />"
                                  "</behavior>"
                                  "</npctype>"
                                  "</npctypes>";
        
        doc->Parse(testNpctypeXml);
        csRef<iDocumentNode> root = doc->GetRoot();
        
        
        npcclient = ::npcclient = new psNPCClient;
        
        // This loads in the NPC client wide NPCtypes.
        npcclient->LoadNPCTypes(root);
        
        //psPersistActor persistMsg(1, 1, 1, true, "npc", "npcs", "stonebm", "stonebm", "stoneb", 0, "helm", csVector3(1, 1, 1), csVector3(1, 1, 1), csVector3(), "tex", "equipment", 1, EID(1), npcclient->GetNetworkMgr()->GetMsgStrings(), npcclient->GetEngine(), 
        //gemNPCActor(npcclient, persistMsg);
        
        npc = new NPC(npcclient);
        
        behaviorset.Add(&behavior);

        npc->Load("npc", PID(1), npcclient->FindNPCType("type"), "region", 5, false);
    }
    
    ~BehaviorSetTest() {
        delete npc;
        delete npcclient;
        delete objreg;
    }
    
    iObjectRegistry* objreg;
    BehaviorSet behaviorset;
    Behavior behavior;
    Reaction reaction;
    NPC* npc;
    psNPCClient* npcclient;
    csHash<NPCType*, const char*> npctypes;
    EventManager eventmgr;
};

TEST_F(BehaviorSetTest, Add) {
    behaviorset.Add(&behavior);
    
}

TEST_F(BehaviorSetTest, Find) {   
    EXPECT_EQ(&behavior, behaviorset.Find("test_behavior"));
}

TEST_F(BehaviorSetTest, Advance) {
    behaviorset.Advance(1000, npc, &eventmgr);
}
