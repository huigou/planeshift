/*
* npcclient.cpp - author: Keith Fulton <keith@paqrat.com>
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
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/cmdline.h>
#include <iutil/object.h>
#include <csutil/csobject.h>
#include <ivaria/reporter.h>
#include <iutil/vfs.h>
#include <csutil/csstring.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>
#include <iengine/mesh.h>
#include <iengine/engine.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/serverconsole.h"
#include "util/psdatabase.h"
#include "util/eventmanager.h"
#include "util/location.h"
#include "util/waypoint.h"
#include "util/psstring.h"
#include "util/strutil.h"
#include "util/psutil.h"
#include "util/pspathnetwork.h"

#include "net/connection.h"
#include "net/clientmsghandler.h"

#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "npcclient.h"
#include "pathfind.h"
#include "networkmgr.h"
#include "npcbehave.h"
#include "npc.h"
#include "perceptions.h"
#include "pathfind.h"
#include "gem.h"
#include "tribe.h"
#include "status.h"

#ifndef INFINITY
#define INFINITY 999999999.0F
#endif

bool running;
extern iDataConnection *db;

class psNPCClientTick : public psGameEvent
{
protected:
    static psNPCClient *client;

public:
	psNPCClientTick(int offsetticks, psNPCClient *c);
    virtual void Trigger();  // Abstract event processing function
    virtual csString ToString() const;
};

psNPCClient::psNPCClient () : serverconsole(NULL)
{
    world        = NULL;
    eventmanager = NULL;
    running      = true;
    database     = NULL;
    network      = NULL;
    tick_counter = 0;
    current_long_range_perception_index = 0;
}

psNPCClient::~psNPCClient()
{
    running = false;
    delete network;
    delete serverconsole;
    delete database;
}

bool psNPCClient::Initialize(iObjectRegistry* object_reg,const char *_host, const char *_user, const char *_pass, int _port)
{
    objreg = object_reg;

    configmanager =  csQueryRegistry<iConfigManager> (object_reg);
    if (!configmanager)
    {
        CPrintf (CON_ERROR, "Couldn't find Configmanager!\n");
        return false;
    }

    engine =  csQueryRegistry<iEngine> (object_reg);
    if (!engine)
    {
        CPrintf (CON_ERROR, "Couldn't find Engine!\n");
        return false;
    }


    // Load the log settings
    LoadLogSettings();

    // Start Database

    database = new psDatabase(object_reg);

    csString db_host, db_user, db_pass, db_name;
	unsigned int db_port;

    db_host = configmanager->GetStr("PlaneShift.Database.npchost", "localhost");
    db_user = configmanager->GetStr("PlaneShift.Database.npcuserid", "planeshift");
    db_pass = configmanager->GetStr("PlaneShift.Database.npcpassword", "planeshift");
    db_name = configmanager->GetStr("PlaneShift.Database.npcname", "planeshift");
	db_port = configmanager->GetInt("PlaneShift.Database.npcport", 0);

    Debug4(LOG_STARTUP,0,COL_BLUE "Database Host: '%s' User: '%s' Databasename: '%s'\n" COL_NORMAL,
      (const char*) db_host, (const char*) db_user, (const char*) db_name);

	if (!database->Initialize(db_host, db_port, db_user, db_pass, db_name))
	{
        Error2("Could not create database or connect to it: %s\n", database->GetLastError());
        delete database;
        return false;
    }

    csString user,pass,host;
    int port;

    host = _host ? _host : configmanager->GetStr("PlaneShift.NPCClient.host", "localhost");
    user = _user ? _user : configmanager->GetStr("PlaneShift.NPCClient.userid", "superclient");
    pass = _pass ? _pass : configmanager->GetStr("PlaneShift.NPCClient.password", "planeshift");
    port = _port ? _port : configmanager->GetInt("PlaneShift.NPCClient.port", 13331);

    CPrintf (CON_DEBUG, "Initialize Network Thread...\n");
    connection = new psNetConnection(500); // 500 elements in queue
    if (!connection->Initialize(object_reg))
    {
        CPrintf (CON_ERROR, "Network thread initialization failed!\n");
        delete connection;
        return false;
    }
    if (!connection->Connect(host,port))
    {
        CPrintf(CON_ERROR, "Couldn't resolve hostname %s on port %d.\n",(const char *)host,port);
        exit(1);
    }

    eventmanager = new EventManager;
    msghandler   = eventmanager;
    psMessageCracker::msghandler = eventmanager;

    // Start up network
    if (!msghandler->Initialize(connection))
    {
        return false;                // Attach to incoming messages.
    }
    network = new NetworkManager(msghandler,connection, engine);

    vfs =  csQueryRegistry<iVFS> (object_reg);
    if (!vfs)
    {
        CPrintf(CON_ERROR, "could not open VFS\n");
        exit(1);
    }

    world = new psWorld();        
    if (!world)
    {
        CPrintf(CON_ERROR, "could not create world\n");
        exit(1);
    }
    world->Initialize( object_reg );

    csRef<iDocumentNode> root = GetRootNode("/this/data/npcbehave.xml");

    if (!root.IsValid() || !LoadNPCTypes(root))
    {
        CPrintf(CON_ERROR, "Couldn't load npcbehave.xml.\n");
        exit(1);
    }

    if (!LoadPathNetwork())
    {
        CPrintf(CON_ERROR, "Couldn't load the path network\n");
        exit(1);
    }
    
    if (!LoadTribes())
    {
        CPrintf(CON_ERROR, "Couldn't load the tribes table\n");
        exit(1);
    }

    if (!ReadNPCsFromDatabase())
    {
        CPrintf(CON_ERROR, "Couldn't load the npcs from db\n");
        exit(1);
    }

    if (!LoadLocations())
    {
        CPrintf(CON_ERROR, "Couldn't load the sc_locations table\n");
        exit(1);
    }    

    cdsys =  csQueryRegistry<iCollideSystem> (objreg);

    PFMaps = new psPFMaps(objreg);
    
    CPrintf(CON_DEBUG, "Connecting to Host: '%s' User: '%s' Password: '%s' Port %d...\n",
        (const char*) host, (const char*) user, (const char*) pass, port);
    // Starts the logon process
    network->Authenticate(host,port,user,pass);

    NPCStatus::Initialize (objreg);
        
    return true;
}

void psNPCClient::MainLoop ()
{
    csRef<iCommandLineParser> cmdline = 
         csQueryRegistry<iCommandLineParser> (objreg);

    if (cmdline)
    {
        const char* ofile = cmdline->GetOption ("output");
        if (ofile != NULL)
        {
            ConsoleOut::SetOutputFile (ofile, false);
        }
        else
        {
            const char* afile = cmdline->GetOption ("append");
            if (afile != NULL)
            {
                ConsoleOut::SetOutputFile (afile, true);
            }
        }
    }
    ConsoleOut::SetMaximumOutputClassStdout (CON_SPAM);
    ConsoleOut::SetMaximumOutputClassFile (CON_SPAM);

    // Start the server console (and handle -run=file).
    serverconsole = new ServerConsole(objreg, "psnpcclient", "NPC Client");
    serverconsole->SetCommandCatcher(this);

    // Enter the real main loop - handling events and messages.
    eventmanager->Run();

    // Save log settings
    SaveLogSettings();
}

void psNPCClient::Disconnect()
{
    network->Disconnect();
}

void psNPCClient::LoadLogSettings()
{
    int count=0;
    for (int i=0; i< MAX_FLAGS; i++)
    {
        if (pslog::GetName(i))
        {
            pslog::SetFlag(pslog::GetName(i),configmanager->GetBool(pslog::GetSettingName(i)),0);
            if(configmanager->GetBool(pslog::GetSettingName(i)))
                count++;
        }
    }
    if (count==0)
    {
        CPrintf(CON_CMDOUTPUT,"All LOGS are off.\n");
    }
}

void psNPCClient::SaveLogSettings()
{
    for (int i=0; i< MAX_FLAGS; i++)
    {
        if (pslog::GetName(i))
        {
            configmanager->SetBool(pslog::GetSettingName(i),pslog::GetValue(pslog::GetName(i)));            
        }
    } 
    
    configmanager->Save();
}


csRef<iDocumentNode> psNPCClient::GetRootNode(const char *xmlfile)
{
    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

    csRef<iDataBuffer> buff = vfs->ReadFile( xmlfile );

    if ( !buff || !buff->GetSize() )
    {
        return NULL;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( buff );
    if ( error )
    {
        Error3("%s in %s", error, xmlfile);
        return NULL;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", xmlfile);
        return NULL;
    }
    return root;
}

bool psNPCClient::LoadNPCTypes(iDocumentNode* root)
{
    csRef<iDocumentNode> topNode = root->GetNode("npctypes");
    if(!topNode)
    {
        Error1("No <npctypes> tag");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        // This is a NPC so load it.
        if ( strcmp( node->GetValue(), "npctype" ) == 0 )
        {
            NPCType *npctype = new NPCType(this, eventmanager);
            if (npctype->Load(node))
            {
                npctypes.Put(npctype->GetName(), npctype);
            }
            else
            {
                delete npctype;
                return false;
            }
        }
    }

    return true;
}

void psNPCClient::Add( gemNPCObject* object )
{
    EID eid = object->GetEID();

    all_gem_objects.Push( object );

    gemNPCItem * item = dynamic_cast<gemNPCItem*>(object);
    if (item)
    {
        all_gem_items.Push( item );
    }

    all_gem_objects_by_eid.Put(eid, object);
    if (object->GetPID().IsValid())
    {
        all_gem_objects_by_pid.Put( object->GetPID(), object);
    }
    
    Notify2(LOG_CELPERSIST,"Added gemNPCObject(%s)\n", ShowID(eid));
}

void psNPCClient::Remove ( gemNPCObject * object )
{
    NPC * npc = object->GetNPC();
    if (npc)
    {
        npc->Printf("Removing entity");
    }

    EID EID = object->GetEID();
    
    // Remove entity from all hated lists.
    for (size_t x=0; x<npcs.GetSize(); x++)
    {
        npcs[x]->RemoveFromHateList(EID);
    }

    all_gem_objects_by_eid.DeleteAll( EID );
    if (object->GetPID().IsValid())
    {
        all_gem_objects_by_pid.DeleteAll( object->GetPID() );
    }

    gemNPCItem * item = dynamic_cast<gemNPCItem*>(object);
    if (item)
    {
        size_t n = all_gem_items.Find ( item );
        if (n != csArrayItemNotFound)
        {
            all_gem_items.DeleteIndexFast( n );
        }
    }

    size_t n = all_gem_objects.Find( object );
    if (n != csArrayItemNotFound)
    {
        all_gem_objects.DeleteIndexFast( n );
    }

    delete object;

    Notify2(LOG_CELPERSIST,"removed gemNPCObject(%s)\n", ShowID(EID));
}

void psNPCClient::RemoveAll()
{
//    size_t i;
//    for (i=0; i<all_gem_objects.GetSize(); i++)
//    {
//        UnattachNPC(all_gem_objects[i]->GetEntity(),FindAttachedNPC(all_gem_objects[i]->GetEntity()));
//    }

    all_gem_objects.DeleteAll();
    all_gem_items.DeleteAll();
    all_gem_objects_by_eid.DeleteAll();
    all_gem_objects_by_pid.DeleteAll();    
}

gemNPCObject *psNPCClient::FindEntityID(EID EID)
{
    return all_gem_objects_by_eid.Get(EID, 0);
}

gemNPCObject *psNPCClient::FindCharacterID(PID PID)
{
    return all_gem_objects_by_pid.Get(PID, 0);
}

NPC* psNPCClient::ReadSingleNPC(PID char_id)
{
    Result result(db->Select("SELECT * FROM sc_npc_definitions WHERE char_id=%u", char_id.Unbox()));
    if (!result.IsValid() || !result.Count())
    {
        Error2("Error loading char_id %s.", ShowID(char_id));
        return NULL;
    }
    NPC *newnpc = new NPC(this, network, world, engine, cdsys);

    if (newnpc->Load(result[0],npctypes, eventmanager))
    {
    	newnpc->Tick();
        npcs.Push(newnpc);
        return newnpc;
    }
    else
    {
        delete newnpc;
        return NULL;
    }
}
bool psNPCClient::ReadNPCsFromDatabase()
{
    Result rs(db->Select("select * from sc_npc_definitions"));

    if (!rs.IsValid())
    {
        Error2("Could not load npcs from db: %s",db->GetLastError() );
        return false;
    }
    for (unsigned long i=0; i<rs.Count(); i++)
    {
        csString name = rs[i]["name"];
        // Familiars are not loaded until neeeded.
        if(name.StartsWith("FAMILIAR:"))
        {
            DeferredNPC defNPC;
            defNPC.id = rs[i].GetInt("char_id");
            defNPC.name = name;
            npcsDeferred.Push(defNPC);
            continue;
        }

        NPC *npc = new NPC(this, network, world, engine, cdsys);
        if (npc->Load(rs[i],npctypes, eventmanager))
        {
        	npc->Tick();
            npcs.Push(npc);

            CheckAttachTribes(npc);
        }
        else
        {
            delete npc;
            return false;
        }
    }
    return true;
}

bool psNPCClient::LoadPathNetwork()
{
    pathNetwork = new psPathNetwork();
    return pathNetwork->Load(engine,db,world);
}

bool psNPCClient::LoadLocations()
{
    Result rs(db->Select("select * from sc_location_type"));

    if (!rs.IsValid())
    {
        Error2("Could not load locations from db: %s",db->GetLastError() );
        return false;
    }
    for (int i=0; i<(int)rs.Count(); i++)
    {
        LocationType *loctype = new LocationType();

        if (loctype->Load(rs[i],engine,db))
        {
           loctypes.Insert(loctype);
           CPrintf(CON_DEBUG, "Added location type '%s'(%d)\n",loctype->name.GetDataSafe(),loctype->id);
        }
        else
        {
            Error2("Could not load location: %s",db->GetLastError() );            
            delete loctype;
            return false;
        }
        
    }

    return true;
}

bool psNPCClient::LoadTribes()
{
    Result rs(db->Select("SELECT t.*, s.name AS home_sector_name FROM tribes t, sectors s WHERE s.id = t.home_sector_id"));

    if (!rs.IsValid())
    {
        Error2("Could not load tribes from db: %s",db->GetLastError() );
        return false;
    }
    for (int i=0; i<(int)rs.Count(); i++)
    {
        psTribe *tribe = new psTribe;
        if (tribe->Load(rs[i]))
        {
            tribes.Push(tribe);
            { // Start Load Members scope
                Result rs2(db->Select("select * from tribe_members where tribe_id=%d",tribe->GetID()));
                if (!rs2.IsValid())
                {
                    Error2("Could not load tribe members from db: %s",db->GetLastError() );
                return false;
                }
                for (int j=0; j<(int)rs2.Count(); j++)
                {
                    tribe->LoadMember(rs2[j]);
                }
            } // End Load Memebers scope

            { // Start Load Memories scope
                Result rs2(db->Select("select m.*,s.name AS sector_name from sc_tribe_memories m, sectors s WHERE s.id = m.sector_id and tribe_id=%d",tribe->GetID()));
                if (!rs2.IsValid())
                {
                    Error2("Could not load tribe members from db: %s",db->GetLastError() );
                    return false;
                }
                for (int j=0; j<(int)rs2.Count(); j++)
                {
                    tribe->LoadMemory(rs2[j]);
                }
            } // End Load Memeories scope
            
            { // Start Load Resources scope
                Result rs2(db->Select("select * from sc_tribe_resources WHERE tribe_id=%d",tribe->GetID()));
                if (!rs2.IsValid())
                {
                    Error2("Could not load tribe resources from db: %s",db->GetLastError() );
                    return false;
                }
                for (int j=0; j<(int)rs2.Count(); j++)
                {
                    tribe->LoadResource(rs2[j]);
                }
            } // End Load Resources scope
            
        }
        else
        {
            delete tribe;
            return false;
        }
    }

    return true;
}

void psNPCClient::CheckAttachTribes( NPC* npc)
{
    // Check if this NPC is part of a tribe
    for (size_t j=0; j<tribes.GetSize(); j++)
    {
        // Check if npc is part of the tribe and if so attach the npc
        tribes[j]->CheckAttach(npc);
    }
}


void psNPCClient::AttachNPC( gemNPCActor* actor, uint8_t DRcounter)
{
    if (!actor) return;

    NPC *npc = NULL;

    // Check based on characterID
    npc = FindNPCByPID( actor->GetPID() );
    if ( !npc )
    {
        CPrintf(CON_NOTIFY,"NPC %s(%s) was not found in scripted npcs for this npcclient.\n",
                actor->GetName(), ShowID(actor->GetPID()));

        npc = ReadSingleNPC(actor->GetPID()); //try reloading the tables if we didn't find it
        if(!npc) //still not found. Try recovering missing data. The recovery process is only for pets, but teorically it can be expanded
        {        //to other npcs types
        
                Result result(db->Select("SELECT * FROM character_relationships WHERE related_id=%u", actor->GetPID().Unbox()));
                if (result.IsValid() && result.Count() && (csString)result[0][2] == "familiar") //just to be sure
                {
                    PID owner_id = PID(result[0].GetUInt32("character_id")); //good we have the owner
                    //now we need the char master npc
                    Result result2(db->Select("SELECT npc_master_id FROM characters WHERE id=%u",actor->GetPID().Unbox()));
                    
                    if (result.IsValid() && result.Count())
                    {
                        PID master_id = PID(result2[0].GetUInt32(0)); //gotcha :)
                        NPC *npc_int = FindNPCByPID(master_id);
                        if(npc_int)
                        {
                            npc_int->InsertCopy(actor->GetPID(),owner_id);
                            npc = ReadSingleNPC(actor->GetPID()); //should load now :P
                            CPrintf(CON_WARNING, "Recreated missing pet %s owned by %s of type %s\n", ShowID(actor->GetPID()), ShowID(owner_id), ShowID(master_id));
                        }
                    }

                }
                if(!npc) //last chance if false good bye
                    return;
        }
    }
    
    actor->AttachNPC(npc);

    if(DRcounter != (uint8_t) -1)
    {
        npc->SetDRCounter(DRcounter);
    }

    CheckAttachTribes(npc);

    npc->Printf("We are now managing NPC <%s, %s, %s>.\n", actor->GetName(), ShowID(actor->GetPID()), ShowID(actor->GetEID()));

    // Test if this actor is in a valid starting position.
    npc->CheckPosition();

    // Report final correct starting position.
    GetNetworkMgr()->QueueDRData(npc);
}



NPC *psNPCClient::FindNPCByPID(PID character_id)
{
    for (size_t x=0; x<npcs.GetSize(); x++)
    {
        if (npcs[x]->GetPID() == character_id)
            return npcs[x];
    }

    for (size_t j=0; j<npcsDeferred.GetSize(); j++)
    {
        if (npcsDeferred[j].id == character_id)
        {
            NPC * npc = ReadSingleNPC(character_id);
            if(npc)
            {
                npcsDeferred.DeleteIndexFast(j);
                return npc;
            }
        }
    }

    return NULL;
}

NPC *psNPCClient::FindNPC(EID EID)
{
    gemNPCObject * obj = all_gem_objects_by_eid.Get (EID, 0);
    if (!obj) return NULL;
    
    return obj->GetNPC();
}

psPath *psNPCClient::FindPath(const char *name)
{
    return pathNetwork->FindPath(name);
}

psPath *psNPCClient::FindPath(const Waypoint * wp1, const Waypoint * wp2, psPath::Direction & direction)
{
    return pathNetwork->FindPath(wp1,wp2,direction);
}

void psNPCClient::TriggerEvent(NPC *npc,Perception *pcpt,float max_range,
                               csVector3 *base_pos, iSector *base_sector)
{
    if (npc)
    {
        npc->TriggerEvent(pcpt);
    }
    else
    {
        for (size_t i=0; i<npcs.GetSize(); i++)
        {
            if (npcs[i]==NULL)  // one of our npcs is not active right now
                continue;

            if (max_range <= 0.0)  // broadcast perceptions
            {
                npcs[i]->TriggerEvent(pcpt);
            }
            else
            {
                if (npcs[i]->GetActor() == NULL) // Need an entity to find position
                    continue;

                iSector *sector;
                csVector3 pos;
                float yrot;
                psGameObject::GetPosition(npcs[i]->GetActor(),pos,yrot,sector);

                float dist = world->Distance(pos,sector,*base_pos,base_sector);
                
                if (dist <= max_range)
                {
                    npcs[i]->TriggerEvent(pcpt);
                }
            }
        }
    }
}

void psNPCClient::SetEntityPos(EID eid, csVector3& pos, iSector* sector, InstanceID instance)
{
    
    gemNPCObject *obj = FindEntityID(eid);
    if (obj)
    {
        if (obj->GetNPC())
        {
            // Skipp updating NPC
            return;
        }

        obj->SetPosition(pos,sector,&instance);
    }
    else
    {
        CPrintf(CON_DEBUG, "Entity %s not found!\n", ShowID(eid));
    }
}

bool psNPCClient::IsReady()
{
	return network->IsReady();
}

void psNPCClient::LoadCompleted()
{
    // Client is loaded.

    // This starts the NPC AI processing loop.
	psNPCClientTick *tick = new psNPCClientTick(255,this);
    tick->QueueEvent();

}

void psNPCClient::Tick()
{
    // Fire a new tick for the common AI processing loop
	psNPCClientTick *tick = new psNPCClientTick(250,this);
    tick->QueueEvent();

	tick_counter++;

	ScopedTimer st_tick(250, "tick for tick_counter %d.",tick_counter);
	
	csTicks when = csGetTicks();

	// Advance tribes
	for (size_t j=0; j<tribes.GetSize(); j++)
	{
		csTicks start = csGetTicks();             // When did we start

		tribes[j]->Advance(when,eventmanager);

		csTicks timeTaken = csGetTicks() - start; // How long did it take

		if (timeTaken > 250)                      // This took way to long time
		{
			CPrintf(CON_WARNING,"Used %u time to process tick for tribe: %s(ID: %u)\n",
					timeTaken,tribes[j]->GetName(),tribes[j]->GetID());
			ListTribes(tribes[j]->GetName());
		}
	}

	// Percept proximity items every 4th tick
	if (tick_counter % 4 == 0)
	{
		ScopedTimer st(200, "tick for percept proximity items");

		PerceptProximityItems();
	}
	// Send all queued npc commands to the server
	network->SendAllCommands(true); // Final
}




bool psNPCClient::LoadMap(const char* mapfile)
{
    static bool first = true;

    if (first)
    {
        if (!world->CreateMap(mapfile, mapfile, psWorld::LOAD_NOW ))
        {
            return false;
        }
        first = false;
    }
    else
    {        
        if (!world->NewRegion(mapfile, psWorld::LOAD_NOW))
        {
            return false;
        }
    }
    return true;
}

LocationType *psNPCClient::FindRegion(const char *regname)
{
    if (!regname)
        return NULL;

    LocationType key;
    key.name = regname;

    LocationType *found = loctypes.Find(&key);
    if (found && found->locs[0] && found->locs[0]->IsRegion())
    {
        return found;
    }
    return NULL;
}

LocationType *psNPCClient::FindLocation(const char *locname)
{
    if (!locname)
        return NULL;

    LocationType key;
    key.name = locname;

    LocationType *found = loctypes.Find(&key);
    return found;
}


NPCType *psNPCClient::FindNPCType(const char *npctype_name)
{

    return npctypes.Get(npctype_name, NULL);
}

void psNPCClient::AddRaceInfo(csString &name, float walkSpeed, float runSpeed)
{
    RaceInfo_t ri;
    ri.name = name;
    ri.walkSpeed = walkSpeed;
    ri.runSpeed = runSpeed;
    raceInfos.PutUnique(name,ri);
}

RaceInfo_t * psNPCClient::GetRaceInfo(const char *name)
{
    return raceInfos.GetElementPointer(name); 
}


float psNPCClient::GetWalkVelocity(csString &race)
{
    RaceInfo_t * ri = raceInfos.GetElementPointer(race);
    if (ri)
    {
        return ri->walkSpeed;
    }

    return 0.0;
}

float psNPCClient::GetRunVelocity(csString &race)
{
    RaceInfo_t * ri = raceInfos.GetElementPointer(race);
    if (ri)
    {
        return ri->runSpeed;
    }

    return 0.0;
}

Location *psNPCClient::FindLocation(const char *loctype, const char *name)
{
    LocationType key;
    key.name = loctype;

    LocationType *found = loctypes.Find(&key);
    if (found)
    {
        for (size_t i=0; i<found->locs.GetSize(); i++)
        {
            if (strcasecmp(found->locs[i]->name,name) == 0)
            {
                return found->locs[i];
            }
        }
    }
    return NULL;
}

Location *psNPCClient::FindNearestLocation(const char *loctype, csVector3& pos, iSector* sector, float range, float *found_range)
{
    LocationType key;
    key.name = loctype;

    LocationType *found = loctypes.Find(&key);
    if (found)
    {
        float min_range = range;    

        int   min_i = -1;

        for (size_t i=0; i<found->locs.GetSize(); i++)
        {
            float dist2 = world->Distance(pos,sector,found->locs[i]->pos,found->locs[i]->GetSector(engine));

            if (min_range < 0 || dist2 < min_range)
            {
                min_range = dist2;
                min_i = (int)i;
            }
        }
        if (min_i > -1)  // found closest one
        {
            if (found_range) *found_range = min_range;

            return found->locs[(size_t)min_i];
        }
    }
    return NULL;
}

Location *psNPCClient::FindRandomLocation(const char *loctype, csVector3& pos, iSector* sector, float range, float *found_range)
{
    LocationType key;
    key.name = loctype;

    csArray<Location*> nearby;
    csArray<float> dist;

    LocationType *found = loctypes.Find(&key);
    if (found)
    {
        for (size_t i=0; i<found->locs.GetSize(); i++)
        {
            float dist2 = world->Distance(pos,sector,found->locs[i]->pos,found->locs[i]->GetSector(engine));

            if (range < 0 || dist2 < range)
            {
                nearby.Push(found->locs[i]);
                dist.Push(dist2);
            }
        }

        if (nearby.GetSize()>0)  // found one or more closer than range
        {
            size_t pick = psGetRandom((uint32)nearby.GetSize());
            
            if (found_range) *found_range = sqrt(dist[pick]);

            return nearby[pick];
        }
    }
    return NULL;
}

Waypoint *psNPCClient::FindNearestWaypoint(csVector3& v,iSector *sector, float range, float * found_range)
{
    return pathNetwork->FindNearestWaypoint(v, sector, range, found_range);
}

Waypoint *psNPCClient::FindRandomWaypoint(csVector3& v,iSector *sector, float range, float * found_range)
{
    return pathNetwork->FindRandomWaypoint(v, sector, range, found_range);
}

Waypoint *psNPCClient::FindNearestWaypoint(const char* group, csVector3& v,iSector *sector, float range, float * found_range)
{
    int groupIndex = pathNetwork->FindWaypointGroup(group);
    if (groupIndex == -1)
    {
        return NULL;
    }
    return pathNetwork->FindNearestWaypoint(groupIndex, v, sector, range, found_range);
}

Waypoint *psNPCClient::FindRandomWaypoint(const char* group, csVector3& v,iSector *sector, float range, float * found_range)
{
    int groupIndex = pathNetwork->FindWaypointGroup(group);
    if (groupIndex == -1)
    {
        return NULL;
    }
    return pathNetwork->FindRandomWaypoint(groupIndex, v, sector, range, found_range);
}

Waypoint *psNPCClient::FindWaypoint(int id)
{
    return pathNetwork->FindWaypoint(id);
}


Waypoint *psNPCClient::FindWaypoint(const char * name)
{
    return pathNetwork->FindWaypoint(name);
}

csList<Waypoint*> psNPCClient::FindWaypointRoute(Waypoint * start, Waypoint * end)
{
    return pathNetwork->FindWaypointRoute(start,end);
}

void psNPCClient::ListAllNPCs(const char * pattern)
{
    CPrintf(CON_CMDOUTPUT, "%-7s %-5s %-30s %-6s %-6s %-20s %-20s %-4s %-3s %-8s\n", 
            "NPC ID", "EID", "Name", "Entity", "Status", "Brain","Behaviour","Step","Dbg","Disabled");
    if(strcmp(pattern, "summary"))
    {
        int disabled = 0;
        int alive = 0;
        int entity = 0;
        int behaviour = 0;
        int brain = 0;
        for (size_t i = 0; i < npcs.GetSize(); i++)
        {
            if(npcs[i]->IsAlive())
                alive++;
            if(npcs[i]->IsDisabled())
                disabled++;
            if(npcs[i]->GetActor())
                entity++;
            if(npcs[i]->GetCurrentBehavior())
                behaviour++;
            if(npcs[i]->GetBrain())
                brain++;
        }
        CPrintf(CON_CMDOUTPUT, "NPC summary for %d NPCs: %d disabled, %d alive, %d with entities, %d with current behaviour, %d with brain", 
                npcs.GetSize(), disabled, alive, entity, behaviour, brain);
    }
    for (size_t i = 0; i < npcs.GetSize(); i++)
    {
        if (!pattern || strstr(npcs[i]->GetName(),pattern))
        {
            CPrintf(CON_CMDOUTPUT, "%-7u %-5d %-30s %-6s %-6s %-20s %-20s %4d %-3s %-8s\n" ,
                    npcs[i]->GetPID().Unbox(),
                    npcs[i]->GetActor() ? npcs[i]->GetActor()->GetEID().Unbox() : 0,
                    npcs[i]->GetName(),
                    (npcs[i]->GetActor()?"Entity":"None  "),
                    (npcs[i]->IsAlive()?"Alive":"Dead"),
                    (npcs[i]->GetBrain()?npcs[i]->GetBrain()->GetName():"(None)"),
                    (npcs[i]->GetCurrentBehavior()?npcs[i]->GetCurrentBehavior()->GetName():"(None)"),
                    (npcs[i]->GetCurrentBehavior()?npcs[i]->GetCurrentBehavior()->GetCurrentStep():0),
                    (npcs[i]->IsDebugging()?"Yes":"No"),
                    (npcs[i]->IsDisabled()?"Disabled":"Aktive")
                    );
        }
    }
}

bool psNPCClient::DumpRace(const char *pattern)
{
    csHash<RaceInfo_t,csString>::GlobalIterator it(raceInfos.GetIterator());
    
    while (it.HasNext())
    {
        const RaceInfo_t & ri = it.Next();
        CPrintf(CON_CMDOUTPUT,"%.40s %.2f %.2f\n",ri.name.GetDataSafe(),ri.walkSpeed,ri.runSpeed);
    }

    return true;
}


bool psNPCClient::DumpNPC(const char *pattern)
{
    unsigned int id = atoi(pattern);
    for (size_t i=0; i<npcs.GetSize(); i++)
    {
        if (npcs[i]->GetPID() == id)
        {
            npcs[i]->Dump();
            return true;
        }
    }
    return false;
}

void psNPCClient::ListAllEntities(const char * pattern, bool onlyCharacters)
{
    if(onlyCharacters)
    {
        CPrintf(CON_CMDOUTPUT, "%-9s %-5s %-10s %-30s %-3s\n" ,"Player ID", "EID","Type","Name","Vis","Inv");
        for (size_t i=0; i < all_gem_objects.GetSize(); i++)
        {
            gemNPCActor * actor = dynamic_cast<gemNPCActor *> (all_gem_objects[i]);
            if(!actor)
                continue;

            if (!pattern || strstr(actor->GetName(),pattern))
            {
                CPrintf(CON_CMDOUTPUT, "%-9d %-5d %-10s %-30s %-3s %-3s\n",
                        actor->GetPID().Unbox(),
                        actor->GetEID().Unbox(),
                        actor->GetObjectType(),
                        actor->GetName(),
                        (actor->IsVisible()?"Yes":"No"),
                        (actor->IsInvincible()?"Yes":"No"));
            }
            
        }
        return;
    }

    CPrintf(CON_CMDOUTPUT, "%-5s %-10s %-30s %-3s %-3s %-4s Position\n","EID","Type","Name","Vis","Inv","Pick");
    for (size_t i=0; i < all_gem_objects.GetSize(); i++)
    {
        gemNPCObject * obj = all_gem_objects[i];
        csVector3 pos;
        float rot;
        iSector *sector;
        psGameObject::GetPosition(obj,pos,rot,sector);

        if (!pattern || strstr(obj->GetName(),pattern))
        {
            CPrintf(CON_CMDOUTPUT, "%5d %-10s %-30s %-3s %-3s %-4s %s %d\n",
                    obj->GetEID().Unbox(),
                    obj->GetObjectType(),
                    obj->GetName(),
                    (obj->IsVisible()?"Yes":"No"),
                    (obj->IsInvincible()?"Yes":"No"),
                    (obj->IsPickable()?"Yes":"No"),
                    toString(pos,sector).GetData(),
                    obj->GetInstance());
        }
    }
        
}

void psNPCClient::ListTribes(const char * pattern)
{
    CPrintf(CON_CMDOUTPUT, "%9s %-30s %-7s %-7s %7s %7s %7s %7s %-15s \n", "Tribe id", "Name", "MCount","NPCs","x","y","z","r","sector");
    for (size_t i = 0; i < tribes.GetSize(); i++)
    {
        if (!pattern || strstr(tribes[i]->GetName(),pattern))
        {
            csVector3 pos;
            iSector* sector;
            float radius;
            tribes[i]->GetHome(pos,radius,sector);
            CPrintf(CON_CMDOUTPUT, "%9d %-30s %-7d %-7d %7.1f %7.1f %7.1f %7.1f %-15s\n" ,
                    tribes[i]->GetID(),
                    tribes[i]->GetName(),
                    tribes[i]->GetMemberIDCount(),
                    tribes[i]->GetMemberCount(),
                    pos.x,pos.y,pos.z,radius,(sector?sector->QueryObject()->GetName():"(null)"));
            CPrintf(CON_CMDOUTPUT,"   ShouldGrow: %s\n",(tribes[i]->ShouldGrow()?"Yes":"No"));
            CPrintf(CON_CMDOUTPUT,"   CanGrow   : %s\n",(tribes[i]->CanGrow()?"Yes":"No"));
            CPrintf(CON_CMDOUTPUT,"Members:\n");
            CPrintf(CON_CMDOUTPUT, "%-6s %-6s %-30s %-6s %-6s %-15s %-15s %-20s %-20s\n", 
                    "NPC ID", "EID", "Name", "Entity", "Status", "Brain","Behaviour","Owner","Tribe");
            for (size_t j = 0; j < tribes[i]->GetMemberCount(); j++)
            {
                NPC * npc = tribes[i]->GetMember(j);
                CPrintf(CON_CMDOUTPUT, "%6u %6d %-30s %-6s %-6s %-15s %-15s %-20s %-20s\n" ,
                        npc->GetPID().Unbox(),
                        npc->GetActor() ? npc->GetActor()->GetEID().Unbox() : 0,
                        npc->GetName(),
                        (npc->GetActor()?"Entity":"None  "),
                        (npc->IsAlive()?"Alive":"Dead"),
                        (npc->GetBrain()?npc->GetBrain()->GetName():""),
                        (npc->GetCurrentBehavior()?npc->GetCurrentBehavior()->GetName():""),
                        npc->GetOwnerName(),
                        (npc->GetTribe()?npc->GetTribe()->GetName():"")
                        );
            }
            CPrintf(CON_CMDOUTPUT,"Resources:\n");
            CPrintf(CON_CMDOUTPUT,"%7s %-20s %7s\n","ID","Name","Amount");
            for (size_t r = 0; r < tribes[i]->GetResourceCount(); r++)
            {
                CPrintf(CON_CMDOUTPUT,"%7d %-20s %d\n",
                        tribes[i]->GetResource(r).id,
                        tribes[i]->GetResource(r).name.GetData(),
                        tribes[i]->GetResource(r).amount);
            }
            CPrintf(CON_CMDOUTPUT,"Memories:\n");
            CPrintf(CON_CMDOUTPUT,"%7s %-20s Position                Radius  %-20s  %-20s\n","ID","Name","Sector","Private to NPC");
            csList<psTribe::Memory*>::Iterator it = tribes[i]->GetMemoryIterator();
            while (it.HasNext())
            {
                psTribe::Memory* memory = it.Next();
                csString name;
                if (memory->npc)
                {
                    name.Format("%s(%u)", memory->npc->GetName(), memory->npc->GetPID().Unbox());
                }
                CPrintf(CON_CMDOUTPUT,"%7d %-20s %7.1f %7.1f %7.1f %7.1f %-20s %-20s\n",
                        memory->id,
                        memory->name.GetDataSafe(),
                        memory->pos.x,memory->pos.y,memory->pos.z,memory->radius,
                        (memory->sector?memory->sector->QueryObject()->GetName():""),
                        name.GetDataSafe());
            }
        }
    }
}

void psNPCClient::ListWaypoints(const char * pattern)
{
    pathNetwork->ListWaypoints(pattern);
}

void psNPCClient::ListPaths(const char * pattern)
{
    pathNetwork->ListPaths(pattern);
}

void psNPCClient::ListLocations(const char * pattern)
{
    BinaryRBIterator<LocationType> iter(&loctypes);
    LocationType *loc;

    CPrintf(CON_CMDOUTPUT, "%9s %9s %-30s %-10s %10s\n", "Type id", "Loc id", "Name", "Region","");
    for (loc = iter.First(); loc; loc = ++iter)
    {
        if (!pattern || strstr(loc->name.GetDataSafe(),pattern))
        {
            CPrintf(CON_CMDOUTPUT, "%9d %9s %-30s %-10s\n" ,
                    loc->id,"",loc->name.GetDataSafe(),"","");

            for (size_t i = 0; i < loc->locs.GetSize(); i++)
            {
                if (loc->locs[i]->IsRegion())
                {
                    CPrintf(CON_CMDOUTPUT, "%9s %9d %-30s %-10s\n" ,
                            "",loc->locs[i]->id,loc->locs[i]->name.GetDataSafe(),
                            (loc->locs[i]->IsRegion()?"True":"False"));
                    for (size_t j = 0; j < loc->locs[i]->locs.GetSize(); j++)
                    {
                        CPrintf(CON_CMDOUTPUT, "%9s %9s %-30s %-10s (%9.3f,%9.3f,%9.3f, %s) %9.3f\n" ,
                                "","","","",
                                loc->locs[i]->locs[j]->pos.x,loc->locs[i]->locs[j]->pos.y,loc->locs[i]->locs[j]->pos.z,
                                loc->locs[i]->locs[j]->sectorName.GetDataSafe(),
                                loc->locs[i]->locs[j]->radius);
                    }
                }
                else
                {
                    CPrintf(CON_CMDOUTPUT, "%9s %9d %-30s %-10s (%9.3f,%9.3f,%9.3f, %s) %9.3f  %9.3f\n" ,
                            "",loc->locs[i]->id,loc->locs[i]->name.GetDataSafe(),
                            (loc->locs[i]->IsRegion()?"True":"False"),
                            loc->locs[i]->pos.x,loc->locs[i]->pos.y,loc->locs[i]->pos.z,
                            loc->locs[i]->sectorName.GetDataSafe(),
                            loc->locs[i]->radius,loc->locs[i]->rot_angle);
                }
            }
        }
    }
}


void psNPCClient::HandleDeath(NPC *who)
{
    who->GetBrain()->Interrupt(who);
    who->SetAlive(false);
    if (who->GetTribe())
    {
        who->GetTribe()->HandleDeath(who);
    }
}

void psNPCClient::PerceptProximityItems()
{
    int size = (int)all_gem_items.GetSize();

    if (!size) return; // Nothing to do if no items


    
    for (size_t i=0; i<npcs.GetSize(); i++)
    {
        if (npcs[i]==NULL || npcs[i]->GetActor() == NULL) // Can't do anyting unless we have both
            continue;
        
        iSector *npc_sector;
        csVector3 npc_pos;
        float yrot; // Used later for items as well
        psGameObject::GetPosition(npcs[i]->GetActor(),npc_pos,yrot,npc_sector);
        
        //
        // Note: The follwing method could skeep checking a item for
        // an iteraction. This because fast delete is used to remove  
        // items from list and than a item on the end might be moved right
        // into the space just checked.
        //
        
        int size = (int)all_gem_items.GetSize();
        
        int check_count = 50; // We only check 50 items each time. This number has to be tuned
        if (check_count > size)
        {
            // If not more than check_count items don't check them more than once :)
            check_count = size;
        }
        
        while (check_count--)
        {
            current_long_range_perception_index++;
            if (current_long_range_perception_index >= (int)all_gem_items.GetSize())
            {
                current_long_range_perception_index = 0;
            }
            
            gemNPCItem * item = all_gem_items[current_long_range_perception_index];
            
            iSector *item_sector;
            csVector3 item_pos;
            psGameObject::GetPosition(item,item_pos,yrot,item_sector);
                         
            if (item && item->IsPickable())
            {
                float dist = world->Distance(npc_pos,npc_sector,item_pos,item_sector);
                
                if (dist <= LONG_RANGE_PERCEPTION)
                {
                    if (dist <= SHORT_RANGE_PERCEPTION)
                    {
                        if (dist <= PERSONAL_RANGE_PERCEPTION)
                        {
                            ItemPerception pcpt_nearby("item nearby", item);
                            TriggerEvent(npcs[i],&pcpt_nearby);
                            continue;
                        }
                        ItemPerception pcpt_adjacent("item adjacent", item);
                        TriggerEvent(npcs[i],&pcpt_adjacent);
                        continue;
                    }
                    ItemPerception pcpt_sensed("item sensed", item);
                    TriggerEvent(npcs[i],&pcpt_sensed);
                    continue;
                }
            }
        }
    }
}

void psNPCClient::PerceptProximityLocations()
{
    BinaryRBIterator<LocationType> iter(&loctypes);
    LocationType *loc;
    for (loc = iter.First(); loc; loc = ++iter)
    {
        for (size_t i = 0; i < loc->locs.GetSize(); i++)
        {
            LocationPerception pcpt_sensed("location sensed",loc->name, loc->locs[i], engine);  
      
            TriggerEvent(NULL, &pcpt_sensed, loc->locs[i]->radius + LONG_RANGE_PERCEPTION, 
                         &loc->locs[i]->pos, loc->locs[i]->GetSector(engine)); // Broadcast
        }
    }
}

void psNPCClient::UpdateTime(int minute, int hour, int day, int month, int year)
{
    gameHour = hour;
    gameMinute = minute;
    gameDay = day;
    gameMonth = month;
    gameYear = year;

    gameTimeUpdated = csGetTicks();

    TimePerception pcpt(gameHour,gameMinute,gameYear,gameMonth,gameDay);
    TriggerEvent(NULL, &pcpt); // Broadcast

    Notify6(LOG_WEATHER,"The time is now %d:%02d %d-%d-%d.",
            gameHour,gameMinute,gameYear,gameMonth,gameDay);
}

void psNPCClient::CatchCommand(const char *cmd)
{
    printf("Caught command: %s\n",cmd);
    network->SendConsoleCommand(cmd+1);
}

void psNPCClient::AttachObject( iObject* object, gemNPCObject* gobject )
{
    csRef<psNpcMeshAttach> attacher = csPtr<psNpcMeshAttach>(new psNpcMeshAttach(gobject));
    attacher->SetName( object->GetName() );
    csRef<iObject> attacher_obj(scfQueryInterface<iObject>(attacher));

    object->ObjAdd( attacher_obj );
}

void psNPCClient::UnattachObject( iObject* object, gemNPCObject* gobject )
{
    csRef<psNpcMeshAttach> attacher (CS::GetChildObject<psNpcMeshAttach>(object));
    if (attacher)
    {
        if (attacher->GetObject() == gobject)
        {
            csRef<iObject> attacher_obj(scfQueryInterface<iObject>(attacher));
            object->ObjRemove(attacher_obj);
        }
    }
}

gemNPCObject* psNPCClient::FindAttachedObject( iObject* object )
{
    gemNPCObject* found = 0;

    csRef<psNpcMeshAttach> attacher(CS::GetChildObject<psNpcMeshAttach>(object));
    if ( attacher )
    {
        found = attacher->GetObject();
    }

    return found;
}

csArray<gemNPCObject*> psNPCClient::FindNearbyEntities( iSector* sector, const csVector3& pos, float radius, bool doInvisible )
{
    csArray<gemNPCObject*> list;
    
    csRef<iMeshWrapperIterator> obj_it =  engine->GetNearbyMeshes( sector, pos, radius );
    while (obj_it->HasNext())
    {
        iMeshWrapper* m = obj_it->Next();
        if (!doInvisible)
        {
            bool invisible = m->GetFlags().Check(CS_ENTITY_INVISIBLE);
            if (invisible)
                continue;
        }

        gemNPCObject* object = FindAttachedObject(m->QueryObject());

        if (object)
        {
            list.Push( object );
        }
    }

    return list;
}


/*------------------------------------------------------------------*/

psNPCClient* psNPCClientTick::client = NULL;

psNPCClientTick::psNPCClientTick(int offsetticks, psNPCClient *c)
: psGameEvent(0,offsetticks,"psNPCClientTick")
{
    client = c;
}

void psNPCClientTick::Trigger()
{
    if (running)
        client->Tick();
}

csString psNPCClientTick::ToString() const
{
    return "NPC Client tick";
}
