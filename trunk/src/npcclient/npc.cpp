/*
* npc.cpp by Keith Fulton <keith@paqrat.com>
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
#include <csutil/csstring.h>
#include <iutil/document.h>
#include <iutil/vfs.h>
#include <iutil/object.h>
#include <csgeom/vector3.h>
#include <iengine/movable.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "engine/linmove.h"
#include "engine/psworld.h"

#include "util/log.h"
#include "util/psdatabase.h"
#include "util/location.h"
#include "util/strutil.h"
#include "util/waypoint.h"

#include "net/clientmsghandler.h"
#include "net/npcmessages.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "networkmgr.h"
#include "npc.h"
#include "npcclient.h"
#include "gem.h"
#include "npcmesh.h"
#include "tribe.h"

extern iDataConnection *db;

NPC::NPC(psNPCClient* npcclient, NetworkManager* networkmanager, psWorld* world, iEngine* engine, iCollideSystem* cdsys): checked(false), hatelist(npcclient, engine, world), tick(NULL)
{ 
    brain=NULL; 
    pid=0;
    last_update=0; 
    npcActor=NULL;
    movable=NULL;
    DRcounter=0;

    // Set up the locates
    activeLocate = new Locate();
    activeLocate->sector=NULL;
    activeLocate->angle=0.0;
    activeLocate->wp = NULL;
    activeLocate->radius = 0.0;
    // Store the active locate in the stored locates structure.
    storedLocates.PutUnique("Active", activeLocate);

    ang_vel=vel=999; 
    walkVelocity=runVelocity=0.0; // Will be cached
    region=NULL; 
    insideRegion = true; // We assume that we start inside the region, if it exists
    insideTribeHome = true; // We assume that we start inside tribe home.
    last_perception=NULL; 
    debugging=0; 
    alive=false; 
    tribe=NULL;
    raceInfo=NULL;
    checkedSector=NULL;
    checked = false;
    checkedResult = false;
    disabled = false;
    fallCounter = 0;
    owner_id = 0;
    this->npcclient = npcclient;
    this->networkmanager = networkmanager;
    this->world = world;
    this->cdsys = cdsys;

    for (int i = 0; i < 10; i++)
    {
        debugLog.Push(csString(""));
    }
    nextDebugLogEntry = 0;
}

NPC::~NPC()
{
    if (brain)
    {
        delete brain; 
        brain = NULL;
    }

    if (last_perception)
    {
        delete last_perception;
        last_perception = NULL;
    }

    // Cleare some cached values 
    region = NULL;
    activeLocate->sector = NULL;
    activeLocate->wp = NULL;
    raceInfo = NULL;
}

void NPC::Tick()
{
    // Ensure NPC only has one tick at a time.
    CS_ASSERT(tick == NULL);

    if(npcclient->IsReady())
    {
        ScopedTimer st(200, this); // Calls the ScopedTimerCallback on timeout

        Advance(csGetTicks());  // Abstract event processing function
    }

    tick = new psNPCTick(NPC_BRAIN_TICK, this);
    tick->QueueEvent();
}


void NPC::ScopedTimerCallback(const ScopedTimer* timer)
{
    CPrintf(CON_WARNING,"Used %u time to process tick for npc: %s(%s)\n",
            timer->TimeUsed(), GetName(), ShowID(GetEID()));
    Dump();
}

csString NPC::Info()
{
    csString reply;
    // Position and instance
    {
        csVector3 pos;
        iSector* sector;
        float rot;
        InstanceID instance = INSTANCE_ALL;

        if (npcActor)
        {
            psGameObject::GetPosition(npcActor,pos,rot,sector);
            instance = npcActor->GetInstance();
        }
	reply.AppendFmt("Pos: %s Rot: %.2f Inst: %d ",
                        npcActor?toString(pos,sector).GetDataSafe():"(none)", rot, instance);
    }
    reply.AppendFmt("DR Counter: %d ", DRcounter);
    reply.AppendFmt("%s",disabled?"Disabled ":"");

    reply.AppendFmt("Active locate( Pos: %s Angle: %.2f Radius: %.2f WP: %s ) ",
		    toString(activeLocate->pos,activeLocate->sector).GetDataSafe(),
		    activeLocate->angle,activeLocate->radius,
		    activeLocate->wp?activeLocate->wp->GetName():"(None)");
    reply.AppendFmt("Spawn pos: %s ", toString(spawnPosition,spawnSector).GetDataSafe());
    if (GetOwner())
    {
        reply.AppendFmt("Owner: %s ",GetOwnerName());
    }
    if (GetRegion())
    {
        reply.AppendFmt("Region( Name: %s Inside: %s ) ", GetRegion()->GetName(), insideRegion?"Yes":"No");
    }
    if (GetTribe())
    {
	reply.AppendFmt("Tribe( Name: %s Type: %s Inside home: %s ) ",
			GetTribe()->GetName(),GetTribeMemberType().GetDataSafe(),insideTribeHome?"Yes":"No");
    }
    if (GetTarget())
    {
        reply.AppendFmt("Target: %s ",GetTarget()->GetName());
    }
    reply.AppendFmt("Last perception: '%s' ",last_perception?last_perception->GetName(this).GetDataSafe():"(None)");
    reply.AppendFmt("Fall counter: %d ", GetFallCounter());
    reply.AppendFmt("Brain: %s ",GetBrain()->GetName());
    reply.AppendFmt("Behaviors: %s",GetBrain()->InfoBehaviors(this).GetDataSafe());

    return reply;
}

void NPC::Dump()
{
    DumpState();
    CPrintf(CON_CMDOUTPUT,"\n");
    
    DumpBehaviorList();
    CPrintf(CON_CMDOUTPUT,"\n");

    DumpReactionList();
    CPrintf(CON_CMDOUTPUT,"\n");

    DumpHateList();            
    CPrintf(CON_CMDOUTPUT,"\n");

    DumpDebugLog();
    CPrintf(CON_CMDOUTPUT,"\n");

    // Dump it's memory buffer
    CPrintf(CON_CMDOUTPUT, "Buffers:\n");
    int index = 0;
    BufferHash::GlobalIterator iter = npcBuffer.GetIterator();
    while (iter.HasNext())
    {
        csString bufferName;
        csString value = iter.Next(bufferName);
        CPrintf(CON_CMDOUTPUT, "String buffer(%d): %s = %s\n",index++,bufferName.GetDataSafe(),value.GetDataSafe());
    }
    CPrintf(CON_CMDOUTPUT, "Memory buffer:\n");
    if(bufferMemory.sector)
    {
        CPrintf(CON_CMDOUTPUT, "Name: %s\n", bufferMemory.name.GetData());
        CPrintf(CON_CMDOUTPUT, "Pos: x:%f y:%f z:%f\n",
                                bufferMemory.pos[0],
                                bufferMemory.pos[1],
                                bufferMemory.pos[2]);
        CPrintf(CON_CMDOUTPUT, "Has Sector:\n");
        if(bufferMemory.GetSector())
            CPrintf(CON_CMDOUTPUT, "Yes\n");
        else
            CPrintf(CON_CMDOUTPUT, "No\n");
    }
    else
    {
        CPrintf(CON_CMDOUTPUT, "Empty\n");
    }
}

EID NPC::GetEID()
{
    return (npcActor ? npcActor->GetEID() : 0);
}

psLinearMovement* NPC::GetLinMove()
{
    if (npcActor)
    {
        return npcActor->pcmove;
    }
    return NULL;
}

void NPC::Load(const char* name, PID pid, NPCType* type, const char* region_name, int debugging, bool disabled, EventManager* eventmanager)
{
    this->name = name;
    this->pid = pid;
    this->type = type->GetName();
    this->region_name = region_name;
    this->debugging = debugging;
    this->disabled = disabled;
    this->brain = new NPCType(*type, eventmanager);
}

void NPC::SetBrain(NPCType *type, EventManager* eventmanager)
{
    delete this->brain;
    this->type = type->GetName();
    this->brain = new NPCType(*type, eventmanager);

}

bool NPC::Load(iResultRow& row, csHash<NPCType*, const char*>& npctypes, EventManager* eventmanager, PID usePID)
{
    name = row["name"];
    if(usePID.IsValid())
    {
        pid = usePID;
    }
    else
    {
        pid   = row.GetInt("char_id");
        if ( pid == 0 )
        {
            Error1("NPC has no id attribute. Error in XML");
            return false;
        }
    }

    type = row["npctype"];
    if ( type.Length() == 0 )
    {
        Error1("NPC has no type attribute. Error in XML");
        return false;
    }

    region_name = row["region"]; // optional

    NPCType* t = npctypes.Get(type, NULL);
    if (!t)
    {
        Error2("NPC type '%s' is not found. Error in XML",(const char *)type);
        return false;
    }

    if (row.GetFloat("ang_vel_override") != 0.0)
    {
        ang_vel = row.GetFloat("ang_vel_override");
    }

    if (row.GetFloat("move_vel_override") != 0.0)
    {
        vel = row.GetFloat("move_vel_override");
    }

    const char *d = row["console_debug"];
    if (d && *d=='Y')
    {
        debugging = 5;
    }
    else
    {
        debugging = 0;
    }

    const char *e = row["disabled"];
    if (e && (*e=='Y' || *e=='y'))
    {
        disabled = true;
    }
    else
    {
        disabled = false;
    }

    brain = new NPCType(*t, eventmanager); // deep copy constructor

    return true; // success
}

bool NPC::InsertCopy(PID use_char_id, PID ownerPID)
{
    int r = db->Command("insert into sc_npc_definitions "
                        "(name, char_id, npctype, region, ang_vel_override, move_vel_override, console_debug, char_id_owner) values "
                        "('%s', %d,      '%s',    '%s',     %f,               %f, '%c',%d)",
                        name.GetData(), use_char_id.Unbox(), type.GetData(), region_name.GetData(), ang_vel, vel, IsDebugging() ? 'Y' : 'N', ownerPID.Unbox());

    if (r!=1)
    {
        Error3("Error in InsertCopy: %s->%s",db->GetLastQuery(),db->GetLastError() );
    }
    else
    {
        Debug2(LOG_NEWCHAR, use_char_id.Unbox(), "Inserted %s", db->GetLastQuery());
    }
    return (r==1);
}

void NPC::SetActor(gemNPCActor * actor)
{
    npcActor = actor;

    // Initialize active location to a known ok value
    if (npcActor)
    {
        
        iSector *sector;
        psGameObject::GetPosition(actor,activeLocate->pos,activeLocate->angle,sector);
        movable = actor->pcmesh->GetMesh()->GetMovable();
    }
    else
    {
        movable = NULL;
    }
}

void NPC::SetAlive( bool a )
{
    alive = a;
}

void NPC::Advance(csTicks when)
{
    if (last_update && !disabled)
    {
        brain->Advance(when-last_update, this);
    }

    last_update = when;
}

void NPC::ResumeScript(Behavior *which)
{
    if (!disabled)
    {
        brain->ResumeScript(this, which);
    }
}

void NPC::TriggerEvent(Perception *pcpt, float maxRange,
                       csVector3 *basePos, iSector *baseSector, bool sameSector)
{
    if (disabled)
    {
        Printf(15,"Disabled so rejecting perception #s"
               ,pcpt->ToString(this).GetData() );
        return;
    }

    if (maxRange > 0.0)
    {
        // This is a range based perception
        gemNPCActor* me = GetActor();
        if (!me)
        {
            Printf(15,"Can't do a ranged based check without an actor");
            return;
        }

        csVector3 pos;
        iSector*  sector;
        psGameObject::GetPosition(me, pos, sector);

        if (sameSector && sector != baseSector)
        {
            return;
        }

        float distance = world->Distance(pos, sector, *basePos, baseSector);

        if (distance > maxRange)
        {
            Printf(15,"The distance %.2f is outside range %.2f of perception %s",
                   distance, maxRange, pcpt->ToString(this).GetData() );
            return;
        }
    }
    
    Printf(10,"Got event %s",pcpt->ToString(this).GetData() );
    brain->FirePerception(this, pcpt);
}

void NPC::TriggerEvent(const char* pcpt)
{
    Perception perc(pcpt);
    TriggerEvent(&perc, -1, NULL, NULL, false);
}

void NPC::SetLastPerception(Perception *pcpt)
{
    if (last_perception)
        delete last_perception;
    last_perception = pcpt;
}

gemNPCActor* NPC::GetMostHated(float range, bool includeInvisible, bool includeInvincible, float* hate)
{
    iSector *sector=NULL;
    csVector3 pos;
    psGameObject::GetPosition(GetActor(), pos, sector);

    return GetMostHated(pos, sector, range, GetRegion(),
                        includeInvisible, includeInvincible, hate);
}


gemNPCActor* NPC::GetMostHated( csVector3& pos, iSector *sector, float range, LocationType * region, bool includeInvisible, bool includeInvincible, float* hate)
{
    gemNPCActor* hated = hatelist.GetMostHated(pos, sector, range, region,
                                               includeInvisible, includeInvincible, hate);

    if (hated)
    {
        Printf(5, "Found most hated: %s(%s)", hated->GetName(), ShowID(hated->GetEID()));
        
    }
    else
    {
        Printf(5,"Found no hated entity");
    }

    return hated;
}

void NPC::AddToHateList(gemNPCActor *attacker, float delta)
{
    Printf("Adding %1.2f to hatelist score for %s(%s).",
           delta, attacker->GetName(), ShowID(attacker->GetEID()));
    hatelist.AddHate(attacker->GetEID(),delta);
    if(IsDebugging(5))
    {
        DumpHateList();
    }
}


float NPC::GetEntityHate(gemNPCActor *ent)
{
    return hatelist.GetHate(ent->GetEID());
}

void NPC::RemoveFromHateList(EID who)
{
    if (hatelist.Remove(who))
    {
        Printf("Removed %s from hate list.", ShowID(who));
    }
}

void NPC::SetLocate(const csString& destination, const NPC::Locate& locate )
{
    // Find or create locate
    Locate* current = storedLocates.Get(destination,NULL);
    if (!current)
    {
        current = new Locate;
        storedLocates.PutUnique(destination,current);
    }
    
    // Copy content
    *current = locate;
}

void NPC::GetActiveLocate(csVector3& pos, iSector*& sector, float& rot)
{
    pos=activeLocate->pos;
    sector = activeLocate->sector;
    rot=activeLocate->angle;
}

void NPC::GetActiveLocate(Waypoint*& wp)
{
    wp = activeLocate->wp;
}

float NPC::GetActiveLocateRadius() const
{
    return activeLocate->radius;
}

bool NPC::CopyLocate(csString source, csString destination)
{
    Printf(5,"Copy locate from %s to %s",source.GetDataSafe(),destination.GetDataSafe());

    Locate* sourceLocate = storedLocates.Get(source,NULL);
    if (!sourceLocate)
    {
        Printf(5,"Failed to copy, no source found!");
        return false;
    }

    Locate* destinationLocation = storedLocates.Get(destination,NULL);
    if (!destinationLocation)
    {
        destinationLocation = new Locate;
        storedLocates.PutUnique(destination,destinationLocation);
    }
    
    destinationLocation->pos    = sourceLocate->pos;
    destinationLocation->sector = sourceLocate->sector;
    destinationLocation->angle  = sourceLocate->angle;
    destinationLocation->wp     = sourceLocate->wp;
    destinationLocation->radius = sourceLocate->radius;

    return true;
}



float NPC::GetWalkVelocity()
{
    // Cache value if not looked up before
    if (walkVelocity == 0.0)
    {
        walkVelocity = npcclient->GetWalkVelocity(npcActor->GetRace());
    }

    return walkVelocity;
}

float NPC::GetRunVelocity()
{
    // Cache value if not looked up before
    if (runVelocity == 0.0)
    {
        runVelocity = npcclient->GetRunVelocity(npcActor->GetRace());
    }

    return runVelocity;
}

LocationType *NPC::GetRegion()
{
    if (region)
    {
        return region;
    }
    else
    {
        region = npcclient->FindRegion(region_name);
        return region;
    }
}

void NPC::Disable( bool disable)
{
    disabled = disabled;

    // Stop the movement
    
    // Set Vel to zero again
    if(GetActor())
    {
        GetLinMove()->SetVelocity( csVector3(0,0,0) );
        GetLinMove()->SetAngularVelocity( 0 );
    

        //now persist
        networkmanager->QueueDRData(this);
        networkmanager->QueueImperviousCommand(GetActor(),true);
    }
}

void NPC::DumpState()
{
    csVector3 loc;
    iSector* sector;
    float rot;
    InstanceID instance = INSTANCE_ALL;

    if (npcActor)
    {
        psGameObject::GetPosition(npcActor,loc,rot,sector);
        instance = npcActor->GetInstance();
    }

    CPrintf(CON_CMDOUTPUT, "States for %s (%s)\n", name.GetData(), ShowID(pid));
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");
    CPrintf(CON_CMDOUTPUT, "Position:            %s\n",npcActor?toString(loc,sector).GetDataSafe():"(none)");
    CPrintf(CON_CMDOUTPUT, "Rotation:            %.2f\n",rot);
    CPrintf(CON_CMDOUTPUT, "Instance:            %d\n",instance);
    CPrintf(CON_CMDOUTPUT, "Debugging:           %d\n",debugging);
    CPrintf(CON_CMDOUTPUT, "DR Counter:          %d\n",DRcounter);
    CPrintf(CON_CMDOUTPUT, "Alive:               %s\n",alive?"True":"False");
    CPrintf(CON_CMDOUTPUT, "Disabled:            %s\n",disabled?"True":"False");
    CPrintf(CON_CMDOUTPUT, "Checked:             %s\n",checked?"True":"False");
    CPrintf(CON_CMDOUTPUT, "Spawn position:      %s\n",toString(spawnPosition,spawnSector).GetDataSafe());
    CPrintf(CON_CMDOUTPUT, "Ang vel:             %.2f\n",ang_vel);
    CPrintf(CON_CMDOUTPUT, "Vel:                 %.2f\n",vel);
    CPrintf(CON_CMDOUTPUT, "Walk velocity:       %.2f\n",walkVelocity);
    CPrintf(CON_CMDOUTPUT, "Run velocity:        %.2f\n",runVelocity);
    CPrintf(CON_CMDOUTPUT, "Owner:               %s\n",GetOwnerName());
    CPrintf(CON_CMDOUTPUT, "Race:                %s\n",GetRaceInfo()?GetRaceInfo()->GetName():"(None)");
    CPrintf(CON_CMDOUTPUT, "Region:              %s\n",GetRegion()?GetRegion()->GetName():"(None)");
    CPrintf(CON_CMDOUTPUT, "Inside region:       %s\n",insideRegion?"Yes":"No");
    CPrintf(CON_CMDOUTPUT, "Tribe:               %s\n",GetTribe()?GetTribe()->GetName():"(None)");
    CPrintf(CON_CMDOUTPUT, "TribeMemberType:     %s\n",GetTribeMemberType().GetDataSafe());
    CPrintf(CON_CMDOUTPUT, "Inside tribe home:   %s\n",insideTribeHome?"Yes":"No");
    CPrintf(CON_CMDOUTPUT, "Target:              %s\n",GetTarget()?GetTarget()->GetName():"");
    CPrintf(CON_CMDOUTPUT, "Last perception:     %s\n",last_perception?last_perception->GetName(this).GetDataSafe():"(None)");
    CPrintf(CON_CMDOUTPUT, "Fall counter:        %d\n", GetFallCounter());
    CPrintf(CON_CMDOUTPUT, "Brain:               %s\n\n", brain->GetName());

    CPrintf(CON_CMDOUTPUT, "Locates for %s (%s)\n", name.GetData(), ShowID(pid));
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");
    LocateHash::GlobalIterator iter = storedLocates.GetIterator();
    while (iter.HasNext())
    {
        csString name;
        Locate* value = iter.Next(name);
        CPrintf(CON_CMDOUTPUT, "%-15s Position: %s\n",name.GetDataSafe(),toString(activeLocate->pos,activeLocate->sector).GetDataSafe());
        CPrintf(CON_CMDOUTPUT, "%-15s Angle:    %.2f\n","",activeLocate->angle);
        CPrintf(CON_CMDOUTPUT, "%-15s Radius:   %.2f\n","",activeLocate->radius);
        CPrintf(CON_CMDOUTPUT, "%-15s WP:       %s\n","",activeLocate->wp?activeLocate->wp->GetName():"");
        
    }
    
}


void NPC::DumpBehaviorList()
{
    CPrintf(CON_CMDOUTPUT, "Behaviors for %s (%s)\n", name.GetData(), ShowID(pid));
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");
    
    brain->DumpBehaviorList(this);
}

void NPC::DumpReactionList()
{
    CPrintf(CON_CMDOUTPUT, "Reactions for %s (%s)\n", name.GetData(), ShowID(pid));
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");
    
    brain->DumpReactionList(this);
}

void NPC::DumpHateList()
{
    iSector *sector=NULL;
    csVector3 pos;

    CPrintf(CON_CMDOUTPUT, "Hate list for %s (%s)\n", name.GetData(), ShowID(pid));
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");

    if (GetActor())
    {
        psGameObject::GetPosition(GetActor(),pos,sector);
        hatelist.DumpHateList(pos,sector);
    }
}

void NPC::DumpDebugLog()
{
    CPrintf(CON_CMDOUTPUT, "Debug log for %s (%s)\n", name.GetData(), ShowID(pid));
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");
    for (size_t i = 0; i < debugLog.GetSize(); i++)
    {
        CPrintf(CON_CMDOUTPUT,"%2d %s\n",i,debugLog[(nextDebugLogEntry+i)%debugLog.GetSize()].GetDataSafe());
    }
    
}


void NPC::ClearState()
{
    Printf(5,"ClearState");
    brain->ClearState(this);
    last_perception = NULL;
    hatelist.Clear();
    SetAlive(false);
    // Enable position check next time npc is attached
    checked = false;
    disabled = false;
}

///TODO: The next 3 functions are exactly the same except a line in the code, maybe figure out a better way to handle this?

gemNPCActor* NPC::GetNearestActor(float range, csVector3 &destPosition, iSector* &destSector, float &destRange)
{
    csVector3 loc;
    iSector*  sector;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCActor*> nearlist = npcclient->FindNearbyActors(sector, loc, range);
    if (nearlist.GetSize() > 0)
    {
        gemNPCActor* nearestEnt = NULL;
        csVector3    nearestLoc;
        iSector*     nearestSector = NULL;

        float nearestRange=range;

        for (size_t i=0; i<nearlist.GetSize(); i++)
        {
            gemNPCActor *ent = nearlist[i];

            // Filter own NPC actor
            if(ent == GetActor())
                continue;

            csVector3 loc2;
            iSector *sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = world->Distance(loc, sector, loc2, sector2);
            if (dist < nearestRange)
            {
                nearestRange  = dist;
                nearestEnt    = ent;
                nearestLoc    = loc2;
                nearestSector = sector2;
            }
        }
        if (nearestEnt)
        {
            destPosition = nearestLoc;
            destSector   = nearestSector;
            destRange    = nearestRange;
            return nearestEnt;
        }
    }
    return NULL;
}

gemNPCActor* NPC::GetNearestNPC(float range, csVector3 &destPosition, iSector* &destSector, float &destRange)
{
    csVector3 loc;
    iSector*  sector;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCActor*> nearlist = npcclient->FindNearbyActors(sector, loc, range);
    if (nearlist.GetSize() > 0)
    {
        gemNPCActor* nearestEnt = NULL;
        csVector3    nearestLoc;
        iSector*     nearestSector = NULL;

        float nearestRange=range;

        for (size_t i=0; i<nearlist.GetSize(); i++)
        {
            gemNPCActor *ent = nearlist[i];

            // Filter own NPC actor, and all players
            if(ent == GetActor() || !ent->GetNPC())
                continue;

            csVector3 loc2;
            iSector *sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = world->Distance(loc, sector, loc2, sector2);
            if (dist < nearestRange)
            {
                nearestRange  = dist;
                nearestEnt    = ent;
                nearestLoc    = loc2;
                nearestSector = sector2;
            }
        }
        if (nearestEnt)
        {
            destPosition = nearestLoc;
            destSector   = nearestSector;
            destRange    = nearestRange;
            return nearestEnt;
        }
    }
    return NULL;
}

gemNPCActor* NPC::GetNearestPlayer(float range, csVector3 &destPosition, iSector* &destSector, float &destRange)
{
    csVector3 loc;
    iSector*  sector;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCActor*> nearlist = npcclient->FindNearbyActors(sector, loc, range);
    if (nearlist.GetSize() > 0)
    {
        gemNPCActor* nearestEnt = NULL;
        csVector3    nearestLoc;
        iSector*     nearestSector = NULL;

        float nearestRange=range;

        for (size_t i=0; i<nearlist.GetSize(); i++)
        {
            gemNPCActor *ent = nearlist[i];

            // Filter own NPC actor, and all NPCs
            if(ent == GetActor() || ent->GetNPC())
                continue;

            csVector3 loc2;
            iSector *sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = world->Distance(loc, sector, loc2, sector2);
            if (dist < nearestRange)
            {
                nearestRange  = dist;
                nearestEnt    = ent;
                nearestLoc    = loc2;
                nearestSector = sector2;
            }
        }
        if (nearestEnt)
        {
            destPosition = nearestLoc;
            destSector   = nearestSector;
            destRange    = nearestRange;
            return nearestEnt;
        }
    }
    return NULL;
}


gemNPCActor* NPC::GetNearestVisibleFriend(float range)
{
    csVector3     loc;
    iSector*      sector;
    float         min_range;
    gemNPCObject* friendEnt = NULL;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCObject*> nearlist = npcclient->FindNearbyEntities(sector,loc,range);
    if (nearlist.GetSize() > 0)
    {
        min_range=range;
        for (size_t i=0; i<nearlist.GetSize(); i++)
        {
            gemNPCObject *ent = nearlist[i];
            NPC* npcFriend = ent->GetNPC();

            if (!npcFriend || npcFriend == this)
                continue;

            csVector3 loc2, isect;
            iSector *sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = (loc2 - loc).Norm();
            if(min_range < dist)
                continue;

            // Is this friend visible?
            csIntersectingTriangle closest_tri;
            iMeshWrapper* sel = 0;
            
            dist = csColliderHelper::TraceBeam (npcclient->GetCollDetSys(), sector,
                loc + csVector3(0, 0.6f, 0), loc2 + csVector3(0, 0.6f, 0), true, closest_tri, isect, &sel);
            // Not visible
            if (dist > 0)
                continue;

            min_range = (loc2 - loc).Norm();
            friendEnt = ent;
        }
    }
    return (gemNPCActor*)friendEnt;
}

gemNPCActor* NPC::GetNearestDeadActor(float range)
{
    csVector3    loc;
    iSector*     sector;
    float        min_range;
    gemNPCActor* nearEnt = NULL;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCObject*> nearlist = npcclient->FindNearbyEntities(sector,loc,range);
    if (nearlist.GetSize() > 0)
    {
        min_range=range;
        for (size_t i=0; i<nearlist.GetSize(); i++)
        {
            // Check if this is an Actor
            gemNPCActor *ent = dynamic_cast<gemNPCActor*>(nearlist[i]);
            if (!ent)
            {
                continue; // No actor
            }
            
            // Check if this is an NPC
            if (ent->GetNPC())
            {
                continue; // This is and NPC
            }
            
            if (ent->IsAlive())
            {
                continue;
            }

            csVector3 loc2, isect;
            iSector *sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = (loc2 - loc).Norm();
            if(min_range < dist)
                continue;

            min_range = (loc2 - loc).Norm();
            nearEnt = ent;
        }
    }
    return nearEnt;
}

void NPC::Printf(const char *msg,...)
{
    va_list args;
	if(!IsDebugging())
		return;

    va_start(args, msg);
    VPrintf(5,msg,args);
    va_end(args);
}

void NPC::Printf(int debug, const char *msg,...)
{
    char str[1024];
    va_list args;

	if(!IsDebugging())
		return;

    va_start(args, msg);
    vsprintf(str, msg, args);
    va_end(args);

    // Add string to the internal log buffer
    debugLog[nextDebugLogEntry] = str;
    nextDebugLogEntry = (nextDebugLogEntry+1)%debugLog.GetSize();

    if (!IsDebugging(debug))
        return;

    CPrintf(CON_CMDOUTPUT, "%s (%s)> %s\n", GetName(), ShowID(pid), str);
}

void NPC::VPrintf(int debug, const char *msg, va_list args)
{
    char str[1024];
   
	if(!IsDebugging())
		return; 

    vsprintf(str, msg, args);

    // Add string to the internal log buffer
    debugLog[nextDebugLogEntry] = str;
    nextDebugLogEntry = (nextDebugLogEntry+1)%debugLog.GetSize();

    if (!IsDebugging(debug))
        return;

    CPrintf(CON_CMDOUTPUT, "%s (%s)> %s\n", GetName(), ShowID(pid), str);
}

gemNPCObject* NPC::GetTarget()
{
    // If something is targeted, use it.
    if (target_id != 0)
    {
        // Check if visible
        gemNPCObject * obj = npcclient->FindEntityID(target_id);
        if (obj && obj->IsInvisible())
        {
            Printf(15, "GetTarget returning nothing, target is invisible");
            return NULL;
        }
        
        return obj;
    }
    else  // if not, try the last perception entity
    {
        if (GetLastPerception())
        {
            gemNPCObject* target = NULL;
            gemNPCObject* entity = GetLastPerception()->GetTarget();
            if (entity)
            {
                target = npcclient->FindEntityID(entity->GetEID());
            }
            Printf(16,"GetTarget returning last perception entity: %s",target ? target->GetName() : "None specified");
            return target;
        }
    }
    return NULL;
}

void NPC::SetTarget(gemNPCObject *t)
{
    if (t == NULL)
    {
        Printf(10,"Clearing target");
        target_id = EID(0);
    }
    else
    {
        Printf(10,"Setting target to: %s (%s)",t->GetName(),ShowID(t->GetEID()));
        target_id = t->GetEID();
    }
}


gemNPCObject *NPC::GetOwner()
{
    if (owner_id.IsValid())
    {
        return npcclient->FindEntityID( owner_id );
    }        
    return NULL;
}

const char * NPC::GetOwnerName()
{
    if (owner_id.IsValid())
    {
        gemNPCObject *obj = npcclient->FindEntityID( owner_id );
        if (obj)
        {
            return obj->GetName();
        }
    }        

    return "";        
}

void NPC::SetOwner(EID owner_EID)
{
    if (owner_EID.IsValid())
        owner_id = owner_EID;
}

void NPC::SetTribe(Tribe * new_tribe)
{
    tribe = new_tribe;
}

Tribe * NPC::GetTribe()
{
    return tribe;
}

void  NPC::SetTribeMemberType( const char* tribeMemberType )
{
    this->tribeMemberType = tribeMemberType;
}

const csString&  NPC::GetTribeMemberType() const
{
    return tribeMemberType;
}

RaceInfo_t* NPC::GetRaceInfo()
{
    if (!raceInfo && npcActor)
    {
        raceInfo = npcclient->GetRaceInfo(npcActor->GetRace());
    }
    
    return raceInfo;
}



void NPC::CheckPosition()
{
    // We only need to check the position once
    
    npcMesh *pcmesh =  GetActor()->pcmesh;

    if(checked)
    {
        if(checkedPos == pcmesh->GetMesh()->GetMovable()->GetPosition() && checkedSector == pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0))
        {
            SetAlive(checkedResult);
            CPrintf(CON_NOTIFY,"Extrapolation skipped, result of: %s\n", checkedResult ? "Alive" : "Dead");
            return;
        }
    }
    if(!alive || GetLinMove()->IsPath())
        return;

    // Give the npc a jump start to make sure gravity will be applied.
    csVector3 startVel(0.0f,1.0f,0.0f);
    csVector3 vel;

    GetLinMove()->AddVelocity(startVel);
    GetLinMove()->SetOnGround(false);
    csVector3 pos(pcmesh->GetMesh()->GetMovable()->GetPosition());
    iSector* sector = pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0);
    // See what happens in the next 10 seconds
    int count = 100;
    while (count--)
    {
        GetLinMove()->ExtrapolatePosition(0.1f);
        vel = GetLinMove()->GetVelocity();
        // Bad starting position - npc is falling at high speed, server should automatically kill it
        if(vel.y < -50)
        {
            CPrintf(CON_ERROR,"Got bad starting location %s, killing %s (%s/%s).\n",
                    toString(pos,sector).GetDataSafe(), name.GetData(), ShowID(pid), ShowID(GetActor()->GetEID()));
            Disable();
            break;
        }
    }


    if(vel == startVel)
    {
        // Collision detection is not being applied!
        GetLinMove()->SetVelocity(csVector3(0.0f, 0.0f, 0.0f));
        psGameObject::SetPosition(GetActor(), pos);
    }
    checked = true;
    checkedPos = pos;
    checkedSector = pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0);
    checkedResult = alive;
}

void NPC::StoreSpawnPosition()
{
    psGameObject::GetPosition(GetActor(), spawnPosition, spawnSector );
}

const csVector3& NPC::GetSpawnPosition() const
{
    return spawnPosition;
}

iSector* NPC::GetSpawnSector() const
{
    return spawnSector;
}

void NPC::SetBufferMemory(Tribe::Memory* memory)
{
    // Just copy data
    bufferMemory.name       = memory->name;
    bufferMemory.pos        = memory->pos;
    bufferMemory.sector     = memory->sector;
    bufferMemory.sectorName = memory->sectorName;
    bufferMemory.radius     = memory->radius;
}

void NPC::SetBuildingSpot(Tribe::Asset* buildingSpot)
{
    this->buildingSpot = buildingSpot;
}

    
/** Get the stored building spot for this NPC
 */
Tribe::Asset* NPC::GetBuildingSpot()
{
    return buildingSpot;
}



csString NPC::GetBuffer(const csString& bufferName)
{
    csString value = npcBuffer.Get(bufferName,"");

    Printf(6,"Get Buffer(%s) return: '%s'",bufferName.GetDataSafe(),value.GetDataSafe());

    return value;
}

void NPC::SetBuffer(const csString& bufferName, const csString& value)
{
    Printf(6,"Set Buffer(%s,%s)",bufferName.GetDataSafe(),value.GetDataSafe());

    npcBuffer.PutUnique(bufferName,value);
}

void NPC::ReplaceBuffers(csString& result)
{
    int index = 0;
    BufferHash::GlobalIterator iter = npcBuffer.GetIterator();
    while (iter.HasNext())
    {
        csString bufferName;
        csString value = iter.Next(bufferName);
        csString replace("$NBUFFER[");
        replace += bufferName;
        replace += "]";
        
        result.ReplaceAll(replace,value);
    }
}



//-----------------------------------------------------------------------------

void HateList::AddHate(EID entity_id, float delta)
{
    HateListEntry *h = hatelist.Get(entity_id, 0);
    if (!h)
    {
        h = new HateListEntry;
        h->entity_id   = entity_id;
        h->hate_amount = delta;
        hatelist.Put(entity_id,h);
    }
    else
    {
        h->hate_amount += delta;
    }
}

gemNPCActor *HateList::GetMostHated(csVector3& pos, iSector *sector, float range, LocationType * region, bool includeInvisible, bool includeInvincible, float* hate)
{
    gemNPCObject *mostHated = NULL;
    float mostHateAmount=0.0;

    csArray<gemNPCObject*> list = npcclient->FindNearbyEntities(sector,pos,range);
    for (size_t i=0; i<list.GetSize(); i++)
    {
        HateListEntry *h = hatelist.Get( list[i]->GetEID(),0 );
        if (h)
        {
            gemNPCObject * obj = npcclient->FindEntityID(list[i]->GetEID());

            if (!obj) continue;

            // Skipp if invisible or invincible
            if (obj->IsInvisible() && !includeInvisible) continue;
            if (obj->IsInvincible() && !includeInvincible) continue;
            
            if (!mostHated || h->hate_amount > mostHateAmount)
            {
                csVector3 objPos;
                iSector* objSector;
                psGameObject::GetPosition(obj, objPos, objSector);
                
                // Don't include if a region is defined and obj not within region.
                if (region && !region->CheckWithinBounds(engine,objPos,objSector)) 
                {
                    continue;
                }
                
                mostHated = list[i];
                mostHateAmount = h->hate_amount;
            }
        }
    }
    if (hate)
    {
        *hate = mostHateAmount;
    }
    return (gemNPCActor*)mostHated;
}

bool HateList::Remove(EID entity_id)
{
    return hatelist.DeleteAll(entity_id);
}

void HateList::Clear()
{
    hatelist.DeleteAll();
}

float HateList::GetHate(EID ent)
{
    HateListEntry *h = hatelist.Get(ent, 0);
    if (h)
        return h->hate_amount;
    else
        return 0;
}

void HateList::DumpHateList(const csVector3& myPos, iSector *mySector)
{
    csHash<HateListEntry*, EID>::GlobalIterator iter = hatelist.GetIterator();

    CPrintf(CON_CMDOUTPUT, "%6s %5s %-40s %5s %s\n",
            "Entity","Hated","Pos","Range","Flags");

    while (iter.HasNext())
    {
        HateListEntry *h = (HateListEntry *)iter.Next();
        csVector3 pos(9.9f,9.9f,9.9f);
        gemNPCObject* obj = npcclient->FindEntityID(h->entity_id);
        csString sectorName;

        if (obj)
        {
            iSector* sector;
            psGameObject::GetPosition(obj,pos,sector);
            if(sector)
            {
                sectorName = sector->QueryObject()->GetName();
            }
            CPrintf(CON_CMDOUTPUT, "%6d %5.1f %40s %5.1f",
                    h->entity_id.Unbox(), h->hate_amount, toString(pos, sector).GetDataSafe(),
                    world->Distance(pos,sector,myPos,mySector));
            // Print flags
            if (obj->IsInvisible())
            {
                CPrintf(CON_CMDOUTPUT," Invisible");
            }
            if (obj->IsInvincible())
            {
                CPrintf(CON_CMDOUTPUT," Invincible");
            }
            CPrintf(CON_CMDOUTPUT,"\n");
        }
        else
        {
            // This is an error situation. Should not hate something that isn't online.
            CPrintf(CON_CMDOUTPUT, "Entity: %u Hated: %.1f\n", h->entity_id.Unbox(), h->hate_amount);
        }

    }
    CPrintf(CON_CMDOUTPUT, "\n");
}
