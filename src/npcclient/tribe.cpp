/*
* tribe.cpp
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <csutil/stringarray.h>
#include <iutil/object.h>
#include <iengine/sector.h>
#include <iengine/engine.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/strutil.h"
#include "util/psutil.h"

#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "tribe.h"
#include "globals.h"
#include "npc.h"
#include "npcclient.h"
#include "npcbehave.h"
#include "perceptions.h"
#include "tribeneed.h"

extern iDataConnection *db;

Tribe::Tribe()
    :homeSector(0),accWealthGrowth(0.0),deathRate(0.0),resourceRate(0.0)
{
    lastGrowth = csGetTicks();
    lastDeath = csGetTicks();
    lastResource = csGetTicks();
}

Tribe::~Tribe()
{
}

bool Tribe::Load(iResultRow& row)
{
    id   = row.GetInt("id");
    name = row["name"];

    homePos = csVector3(row.GetFloat("home_x"),row.GetFloat("home_y"),row.GetFloat("home_z"));
    homeRadius = row.GetFloat("home_radius");
    homeSectorName = row["home_sector_name"];
    maxSize = row.GetInt("max_size");
    wealthResourceName = row["wealth_resource_name"];
    wealthResourceNick = row["wealth_resource_nick"];
    wealthResourceArea = row["wealth_resource_area"];

    wealthGatherNeed = row["wealth_gather_need"];

    wealthResourceGrowth = row.GetFloat("wealth_resource_growth");
    wealthResourceGrowthActive = row.GetFloat("wealth_resource_growth_active");
    wealthResourceGrowthActiveLimit = row.GetInt("wealth_resource_growth_active_limit"); 
        
    reproductionCost = row.GetInt("reproduction_cost");
    npcIdleBehavior = row["npc_idle_behavior"];

    return true;
}

bool Tribe::LoadNeed(iResultRow& row)
{
    unsigned int needSetIndex    = row.GetInt("need_set");
    int          needId          = row.GetInt("need_id");
    csString     needType        = row["type"];
    csString     needName        = row["name"];
    csString     perception      = row["perception"];
    csString     dependName      = row["depend"];
    float        needStartValue  = row.GetFloat("need_start_value");
    float        needGrowthValue = row.GetFloat("need_growth_value");

    TribeNeedSet* needSet = GetNeedSet(needSetIndex, true);
    
    TribeNeed *depend = needSet->Find( dependName );
    TribeNeed *need = NULL;

    if (needType.CompareNoCase(TribeNeed::TribeNeedTypeName[TribeNeed::GENERIC]))
    {
        need = new TribeNeedGeneric(needName,perception,needStartValue,needGrowthValue);
    }
    else if (needType.CompareNoCase(TribeNeed::TribeNeedTypeName[TribeNeed::TIME_OF_DAY]))
    {
        CS::Utility::StringArray<> arguments;
        arguments.SplitString(row.GetString("arguments"), ",");
        if (arguments.GetSize() != 2)
        {
            Error3("No start and end time for time of day vent for tribe %d need %d",id,needId);
            return false;
        }
            
        int startHour = atoi(arguments[0]);
        int endHour   = atoi(arguments[1]);
            
        need = new TribeNeedTimeOfDay(needName,perception,needStartValue,needGrowthValue,startHour,endHour);
    }
    else
    {
        // The rest of the needs are depened on other needs.
        // Check that we have the dependend need.

        if (depend == NULL)
        {
            Error5("Failed to find dependend need '%s' for the %s need %d for tribe %d",
                   dependName.GetDataSafe(),needType.GetDataSafe(),needId,id);
            return false;
        }

        if (needType.CompareNoCase(TribeNeed::TribeNeedTypeName[TribeNeed::RESOURCE_AREA]))
        {
            need = new TribeNeedResourceArea(needName,perception,needStartValue,needGrowthValue,depend);
        }
        else if (needType.CompareNoCase(TribeNeed::TribeNeedTypeName[TribeNeed::REPRODUCE]))
        {
            need = new TribeNeedReproduce(needName,perception,needStartValue,needGrowthValue,depend);
        }
        else if (needType.CompareNoCase(TribeNeed::TribeNeedTypeName[TribeNeed::RESOURCE_RATE]))
        {
            float limit = row.GetFloat("arguments");
            if (limit <= 0.0)
            {
                Error3("No limit for RESOURCE_RATE for tribe %d need %d",id,needId);
                return false;
            }

            need = new TribeNeedResourceRate(needName,perception,needStartValue,needGrowthValue,depend,limit);
        }
        else if (needType.CompareNoCase(TribeNeed::TribeNeedTypeName[TribeNeed::DEATH_RATE]))
        {
            float limit = row.GetFloat("arguments");
            if (limit <= 0.0)
            {
                Error3("No limit for DEATH_RATE for tribe %d need %d",id,needId);
                return false;
            }
            
            need = new TribeNeedDeathRate(needName,perception,needStartValue,needGrowthValue,depend,limit);
        }
        else
        {
            Error3("Could not mach need '%s' for tribe %d",needName.GetDataSafe(),id);
            return false;
        }
    }
    
    if (need)
    {
        needSet->AddNeed( need );
        return true;
    }

    return false;
}


bool Tribe::LoadMember(iResultRow& row)
{
    MemberID mID;
    mID.pid               = row.GetInt("member_id");
    mID.tribeMemberType   = row.GetInt("member_type");

    membersId.Push(mID);
    
    return true;
}

bool Tribe::AddMember(PID pid, uint32_t tribeMemberType)
{
    MemberID mID;
    mID.pid = pid;
    mID.tribeMemberType = tribeMemberType;

    membersId.Push(mID);

    // Add to members list in db
    db->Command("INSERT INTO tribe_members (tribe_id,member_id,member_type) "
                "VALUES (%u,%u,%u)", GetID(), pid.Unbox(),tribeMemberType);

    return true;
}

bool Tribe::LoadMemory(iResultRow& row)
{
    Memory * memory = new Memory;
    
    memory->id   = row.GetInt("id");
    memory->name = row["name"];
    memory->pos = csVector3(row.GetFloat("loc_x"),row.GetFloat("loc_y"),row.GetFloat("loc_z"));
    memory->radius = row.GetFloat("radius");
    memory->sectorName = row["sector_name"];
    // Try to find the sector. Will probably fail at this point.
    memory->sector = npcclient->GetEngine()->FindSector(memory->sectorName);
    memory->npc = NULL; // Not a privat memory
    
    memories.PushBack(memory);

    return true;
}

int GetSectorID(iDataConnection *db,const char* name)
{
    // Load all with same master location type
    Result rs(db->Select("select id from sectors where name='%s'",name)); 

    if (!rs.IsValid())
    {
        Error2("Could not find sector id from db: %s",db->GetLastError() );
        return -1;
    }
    return rs[0].GetInt("id");
}

void Tribe::SaveMemory(Memory * memory)
{
    const char * fields[] = 
        {"tribe_id","name","loc_x","loc_y","loc_z","sector_id","radius"};
    psStringArray values;
    values.FormatPush("%d",GetID());
    values.FormatPush("%s",memory->name.GetDataSafe());
    values.FormatPush("%.2f",memory->pos.x);
    values.FormatPush("%.2f",memory->pos.y);
    values.FormatPush("%.2f",memory->pos.z);
    values.FormatPush("%d",GetSectorID(db,memory->GetSector()->QueryObject()->GetName()));
    values.FormatPush("%.2f",memory->radius);
    
    memory->id = db->GenericInsertWithID("sc_tribe_memories",fields,values);
    if (id == 0)
    {
        CPrintf(CON_ERROR, "Failed to save memory for tribe: %s.\n",
                db->GetLastError());
        return;
    }
}

bool Tribe::LoadResource(iResultRow& row)
{
    Resource newRes;
    newRes.id  = row.GetInt("id");
    newRes.name  = row["name"];
    newRes.amount = row.GetInt("amount");
    resources.Push(newRes);

    return true;
}

void Tribe::SaveResource(Resource* resource, bool newResource)
{
    const char * fields[] = 
        {"tribe_id","name","amount"};
    psStringArray values;
    values.FormatPush("%d",GetID());
    values.FormatPush("%s",resource->name.GetDataSafe());
    values.FormatPush("%d",resource->amount);

    if (newResource)
    {
        resource->id = db->GenericInsertWithID("sc_tribe_resources",fields,values);
        if (id == 0)
        {
            CPrintf(CON_ERROR, "Failed to save resource for tribe: %s.\n",
                    db->GetLastError());
            return;
        }
    }
    else
    {
        csString id;
        id.Format("%d",resource->id);
        
        if (!db->GenericUpdateWithID("sc_tribe_resources","id",id,fields,values))
        {
            CPrintf(CON_ERROR, "Failed to save resource for tribe: %s.\n",
                    db->GetLastError());
            return;
        }
        
    }
    
}


bool Tribe::CheckAttach(NPC * npc)
{
    for (size_t i=0; i < membersId.GetSize(); i++)
    {
        if (npc->GetPID() == membersId[i].pid)
        {
            // Found a member so attach to the tribe.
            return AttachMember(npc, membersId[i].tribeMemberType);
        }
    }

    return true; // Not part of tribe but didn't fail either
}

bool Tribe::AttachMember(NPC * npc, uint32_t tribeMemberType)
{
    // Some checks to see if this NPC is fitt for this Tribe
    if (tribeMemberType >= needSet.GetSize())
    {
        Error5("Trying to attach a NPC %s(%u) that have a type %d largert than the supported type %zu",
	       npc->GetName(),npc->GetPID().Unbox(),tribeMemberType, needSet.GetSize());
        return false;
    }

    Behavior * idleBehavior = npc->GetBrain()->Find(npcIdleBehavior.GetDataSafe());
    if (!idleBehavior)
    {
        Error4("Trying to attach a NPC %s(%u) to tribe without matching idle behavior of %s",
	       npc->GetName(),npc->GetPID().Unbox(), npcIdleBehavior.GetDataSafe());
        // Dump the behavior list so that we see what behaviors we have for this npc.
	npc->DumpBehaviorList();
        return false;
    }

    npc->SetTribe(this);
    npc->SetTribeMemberType( tribeMemberType );
    for (size_t i=0; i < members.GetSize(); i++)
    {
        if (npc->GetPID() == members[i]->GetPID())
        {
            return true;
        }
    }

    // Not in member list so add
    members.Push(npc);

    return true;
}


bool Tribe::HandleDeath(NPC * npc)
{
    deadMembers.Push(npc);

    // Make sure memories that isn't stored in the tribe is forgotten.
    ForgetMemories(npc);

    UpdateDeathRate();
    return false;
}

int Tribe::AliveCount() const
{
    int count = 0;
    for (size_t i=0; i < members.GetSize(); i++)
    {
        NPC *npc = members[i];
        if (npc->IsAlive()) count++;
    }
    return count;
}


void Tribe::HandlePerception(NPC * npc, Perception *perception)
{
    csString name = perception->GetName();
    
    CS::Utility::StringArray<> strarr;
    strarr.SplitString(name, ":");
    
    if (strarr[0] == csString("transfer"))
    {
        InventoryPerception *invPcpt = dynamic_cast<InventoryPerception*>(perception);
        if (!invPcpt) return;

        AddResource(perception->GetType(),invPcpt->GetCount());
    }
}

void Tribe::AddResource(csString resource, int amount)
{
    for (size_t i=0; i < resources.GetSize(); i++)
    {
        if (resources[i].name == resource)
        {
            resources[i].amount += amount;
            SaveResource(&resources[i],false); // Update resource

            if (resource == GetNeededResource() && amount > 0)
            {
                UpdateResourceRate( amount );
            }
            
            return;
        }
    }
    Resource newRes;
    newRes.name  = resource;
    newRes.amount = amount;
    SaveResource(&newRes,true); // New resource
    resources.Push(newRes);

    if (resource == GetNeededResource() && amount > 0)
    {
        UpdateResourceRate( amount );
    }
}

int Tribe::CountResource(csString resource) const
{
    for (size_t i=0; i < resources.GetSize(); i++)
    {
        if (resources[i].name == resource)
        {
            return resources[i].amount;
        }
    }
    return 0;
}


void Tribe::Advance(csTicks when, EventManager *eventmgr)
{
    if ( when - lastGrowth > 1000)
    {
        float growth;
        
        // We need to help tribes that have no members with some resources
        // so that they can spawn the first entity
        if (AliveCount() <= 0)
        {
            growth = wealthResourceGrowth;
        }
        else if (CountResource(wealthResourceName) < wealthResourceGrowthActiveLimit)
        {
            // Some tribes need constant growth in wealth, though capped to a limit
            // to prevent tribes with no strain on the resources to grow
            // infinit in wealth
            growth = wealthResourceGrowthActive;
        }
        else
        {
            growth = 0;
        }

        // Now calculate the growth. Adding what part that wasn't added
        // the last time this code where run.
        accWealthGrowth += growth* ((when - lastGrowth)/1000.0);
        int amount = int(floor(accWealthGrowth));
        accWealthGrowth -= amount;

        AddResource(wealthResourceName, amount );
        
        lastGrowth = when;
    }
    else if (when - lastGrowth < 0) // Handle wrappoer of tick
    {
        lastGrowth = when;
    }
    
	
    for (size_t i=0; i < members.GetSize(); i++)
    {
        NPC *npc = members[i];
        
        Behavior * behavior = npc->GetCurrentBehavior();

        if ((behavior && strcasecmp(behavior->GetName(),npcIdleBehavior.GetDataSafe())==0) ||
            (!npc->IsAlive()) )
        {
            csString perc;

            TribeNeed *need = Brain(npc);
            if (!need || need->GetPerception().IsEmpty())
            {
                continue; // Do noting
            }
            
            switch (need->GetNeedType())
            {
            case TribeNeed::GENERIC:
            case TribeNeed::RESOURCE_AREA:
                perc = need->GetPerception();
                break;
            case TribeNeed::REPRODUCE:
                AddResource(wealthResourceName,-reproductionCost);
                perc = need->GetPerception();
                break;
            default:
                continue; // Do nothing
            }

            npc->Printf("Tribe brain perception '%s'",perc.GetDataSafe());
            
            Perception perception(perc);
            npc->TriggerEvent(&perception);
        }
    }
}

bool Tribe::ShouldGrow() const
{
    return members.GetSize() < (size_t)GetMaxSize();
}

bool Tribe::CanGrow() const
{
    return CountResource(wealthResourceName) >= reproductionCost;
}

void Tribe::DumpNeeds() const
{

    ConstTribeNeedSetGlobalIterator iter(needSet.GetIterator());
    while (iter.HasNext())
    {
        unsigned int memberType;
        const TribeNeedSet* tns = iter.Next(memberType);
        CPrintf(CON_CMDOUTPUT,"%-25s(%3d) %10s %-20s %6s %6s   %s\n",
                "Need for TribeMemberType",memberType,"Value","Perception","Start","Growth","Depend on");

        TribeNeedSet::ConstNeedIterator it(tns->GetIterator());
        while (it.HasNext())
        {
            const TribeNeed* need = it.Next();
            CPrintf(CON_CMDOUTPUT,"%-30s %10.2f %-20s %6.2f %6.2f -> %s\n",
                    need->GetTypeAndName().GetDataSafe(),
                    need->current_need,
                    need->GetPerception().GetDataSafe(),
                    need->GetNeedStartValue(),
                    need->GetNeedGrowthValue(),
                    need->GetNeed()->GetTypeAndName().GetDataSafe());
        }
    }
}


TribeNeed* Tribe::Brain(NPC * npc)
{
    TribeNeedSet* needSet = GetNeedSet(npc); // Get the need set for this npc's
                                             // member type
    
    // Handle special case for dead npc's
    if (!npc->IsAlive())
    {
        if (AliveCount() == 0 && CountResource(wealthResourceName) >= 10 * reproductionCost) // Resurrect with large cost if every member is dead.
        {
            AddResource(wealthResourceName,-10*reproductionCost); 
            return needSet->Find("Resurrect");
        }
        else if (CanGrow())
    	{
            AddResource(wealthResourceName,-reproductionCost); 
            return needSet->Find("Resurrect");
    	}
        else
        {
            needSet->MaxNeed(wealthGatherNeed); // Next live NPC will start gather resources
        }
        return NULL;        
    }
    
    // Continue on for live NPCs

    needSet->UpdateNeed(npc);
    
    TribeNeed *nextNeed = GetNeedSet(npc)->CalculateNeed(npc);

    // Check if the most needed need has some depenency that needs to be done first.
    nextNeed = const_cast<TribeNeed*>(nextNeed->GetNeed());

    return nextNeed;
}

void Tribe::UpdateDeathRate()
{
    csTicks now = csGetTicks();

    int delta = now - lastDeath; // TODO: Fix rollover situation

    float rate = delta;

    float N = 0.9;

    deathRate = (1.0-N)*rate + N*deathRate;
    lastDeath = now;
}

void Tribe::UpdateResourceRate(int amount)
{
    csTicks now = csGetTicks();

    int delta = now - lastResource; // TODO: Fix rollover situation

    float rate = delta/amount;

    float N = 0.9;

    resourceRate = (1.0-N)*rate + N*resourceRate;
    lastResource = now;
}


TribeNeedSet* Tribe::GetNeedSet(NPC* npc)
{
    return GetNeedSet(npc->GetTribeMemberType(), false);
}


TribeNeedSet* Tribe::GetNeedSet(unsigned int memberType, bool create)
{
    TribeNeedSet *tns = needSet.Get(memberType, NULL);
    

    if (tns == NULL && create)
    {
        tns = needSet.Put(memberType, new TribeNeedSet(this));
    }
    

    return tns;
}



int Tribe::GetMaxSize() const
{
    int size = maxSize;
    
    if (size == -1 || size > TRIBE_UNLIMITED_SIZE)
    {
        size = TRIBE_UNLIMITED_SIZE; // NPC Client definition of unlimited size
    }

    return size; 
}

int Tribe::GetReproductionCost() const
{
    return reproductionCost;
}



void Tribe::GetHome(csVector3& pos, float& radius, iSector* &sector)
{ 
    pos = homePos; 
    radius = homeRadius; 
    if (homeSector == NULL)
    {
        homeSector = npcclient->GetEngine()->FindSector(homeSectorName);
        if (!homeSector)
        {
            Error3("Failed to find sector for tribe %s home for homeSectorName: %s",
                   GetName(),homeSectorName.GetDataSafe());
        }
    }
    sector = homeSector;
}

void Tribe::SetHome(const csVector3& pos, float radius, iSector* sector)
{ 
    homePos = pos; 
    homeSector = sector;
    homeSectorName = sector->QueryObject()->GetName();
    homeRadius = radius;
    
    // Consider adding storrage of this new position to DB here
    // TODO: Store to DB.
}

bool Tribe::CheckWithinBoundsTribeHome(NPC* npc, const csVector3& pos, const iSector* sector)
{
    float dist = npcclient->GetWorld()->Distance(homePos, homeSector, pos, sector);

    npc->Printf(5,"Range to tribe with radius %.2f is %.2f",homeRadius,dist);
    

    if (dist > homeRadius)
    {
        return false;
    }
    else
    {
        return true;
    }
}


bool Tribe::GetResource(NPC* npc, csVector3 startPos, iSector * startSector, csVector3& locatedPos, iSector* &locatedSector, float range, bool random)
{
    float locatedRange=0.0;
    Tribe::Memory * memory = NULL;
    
    if (psGetRandom(100) > 10) // 10% chance for go explor a new resource arae
    {
        csString neededResource = GetNeededResource();

        if (random)
        {
            memory = FindRandomMemory(neededResource,startPos,startSector,range,&locatedRange);
        }
        else
        {
            memory = FindNearestMemory(neededResource,startPos,startSector,range,&locatedRange);
        }
        if (memory)
        {
            npc->Printf("Found needed resource: %s at %s",neededResource.GetDataSafe(),toString(memory->pos,memory->GetSector()).GetDataSafe());
        }
        else
        {
            npc->Printf("Didn't find needed resource: %s",neededResource.GetDataSafe());
        }
        
    }
    if (!memory)
    {
        csString area = GetNeededResourceAreaType();
        if (random)
        {
            memory = FindRandomMemory(area,startPos,startSector,range,&locatedRange);
        }
        else
        {
            memory = FindNearestMemory(area,startPos,startSector,range,&locatedRange);
        }

        if (memory)
        {
            npc->Printf("Found resource area: %s at %s",area.GetDataSafe(),toString(memory->pos,memory->GetSector()).GetDataSafe());
        }
        else
        {
            npc->Printf("Didn't find resource area: %s",area.GetDataSafe());
        }

    }
    if (!memory)
    {
        npc->Printf("Couldn't locate resource for npc.",npc->GetName() );
        return false;
    }

    locatedPos = memory->pos;
    locatedSector = memory->GetSector();

    return true;
}

const char* Tribe::GetNeededResource()
{
    return wealthResourceName.GetDataSafe();
}

const char* Tribe::GetNeededResourceNick()
{
    return wealthResourceNick.GetDataSafe();
}

const char* Tribe::GetNeededResourceAreaType()
{
    return wealthResourceArea.GetDataSafe();
}

float Tribe::GetWealthResourceGrowth() const
{
    return wealthResourceGrowth;
}

float Tribe::GetWealthResourceGrowthActive() const
{
    return wealthResourceGrowthActive;
}

int Tribe::GetWealthResourceGrowthActiveLimit() const
{
    return wealthResourceGrowthActiveLimit;
}


void Tribe::Memorize(NPC * npc, Perception * perception)
{
    // Retriv date from the perception
    csString  name = perception->GetType();
    float     radius = perception->GetRadius();
    csVector3 pos;
    iSector*  sector = NULL;

    if (!perception->GetLocation(pos,sector))
    {
        npc->Printf("Failed to memorize '%s' perception without valid position",name.GetDataSafe());
        return;
    }
        
    // Store the perception if not known from before

    Memory* memory = FindPrivMemory(name,pos,sector,radius,npc);
    if (memory)
    {
        npc->Printf("Has this in privat knowledge -> do nothing");
        return;
    }
    
    memory = FindMemory(name,pos,sector,radius);
    if (memory)
    {
        npc->Printf("Has this in tribe knowledge -> do nothing");
        return;
    }
    
    npc->Printf("Store in privat memory: '%s' %.2f %.2f %2f %.2f '%s'",name.GetDataSafe(),pos.x,pos.y,pos.z,radius,npc->GetName());
    AddMemory(name,pos,sector,radius,npc);
}

Tribe::Memory* Tribe::FindPrivMemory(csString name,const csVector3& pos, iSector* sector, float radius, NPC * npc)
{
    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();
        if (memory->name == name && memory->GetSector() == sector && memory->npc == npc)
        {
            float dist = (memory->pos - pos).Norm();
            if (dist <= radius)
            {
                return memory;
            }
        }
    }
    return NULL; // Found nothing
}

Tribe::Memory* Tribe::FindMemory(csString name,const csVector3& pos, iSector* sector, float radius)
{
    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();
        if (memory->name == name && memory->GetSector() == sector && memory->npc == NULL)
        {
            float dist = (memory->pos - pos).Norm();
            if (dist <= radius)
            {
                return memory;
            }
        }
    }
    return NULL; // Found nothing
}

Tribe::Memory* Tribe::FindMemory(csString name)
{
    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();
        if (memory->name == name && memory->npc == NULL)
        {
            return memory;
        }
    }
    return NULL; // Found nothing
}

void Tribe::AddMemory(csString name,const csVector3& pos, iSector* sector, float radius, NPC * npc)
{
    Memory * memory = new Memory;
    memory->id     = -1;
    memory->name   = name;
    memory->pos    = pos;
    memory->sector = sector;
    memory->radius = radius;
    memory->npc    = npc;
    memories.PushBack(memory);
}

void Tribe::ShareMemories(NPC * npc)
{
    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();
        if (memory->npc == npc)
        {
            if (FindMemory(memory->name,memory->pos,memory->GetSector(),memory->radius))
            {
                // Tribe know this so delete the memory.
                memories.Delete(it);
                delete memory;
            }
            else
            {
                memory->npc = NULL; // Remove private indicator.
                SaveMemory(memory);
            }
        }
    }    
}

iSector* Tribe::Memory::GetSector()
{
    if (sector) return sector;

    sector = npcclient->GetEngine()->FindSector(sectorName);
    return sector;
}


void Tribe::ForgetMemories(NPC * npc)
{
    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();
        if (memory->npc == npc)
        {
            memories.Delete(it);
            delete memory;
        }
    }    
}

Tribe::Memory *Tribe::FindNearestMemory(const char *name, const csVector3& pos, const iSector* sector, float range, float *foundRange)
{
    Memory * nearest = NULL;

    float minRange = range*range;    // Working with Squared values
    if (range == -1) minRange = -1;  // -1*-1 = 1, will use -1 later
    
    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();   

        if (memory->name == name && memory->npc == NULL)
        {
            float dist2 = npcclient->GetWorld()->Distance(pos,sector,memory->pos,memory->GetSector());
            
            if (minRange < 0 || dist2 < minRange)
            {
                minRange = dist2;
                nearest = memory;
            }
        }
    }

    if (nearest && foundRange)  // found closest one
    {
        *foundRange = sqrt(minRange);
    }

    return nearest;
}

Tribe::Memory *Tribe::FindRandomMemory(const char *name, const csVector3& pos, const iSector* sector, float range, float *foundRange)
{
    csArray<Memory*> nearby;
    csArray<float> dist;

    float minRange = range*range;    // Working with Squared values
    if (range == -1) minRange = -1;  // -1*-1 = 1, will use -1 later

    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();

        if (memory->name == name && memory->npc == NULL)
        {
            float dist2 = npcclient->GetWorld()->Distance(pos,sector,memory->pos,memory->GetSector());

            if (minRange < 0 || dist2 < minRange)
            {
                nearby.Push(memory);
                dist.Push(dist2);
            }
        }
    }

    if (nearby.GetSize()>0)  // found one or more closer than range
    {
        size_t pick = psGetRandom((uint32)nearby.GetSize());
        
        if (foundRange) *foundRange = sqrt(dist[pick]);

        return nearby[pick];
    }
    return NULL;
}

void Tribe::TriggerEvent(Perception *pcpt, float maxRange,
                         csVector3 *basePos, iSector *baseSector)
{
    for (size_t i=0; i < members.GetSize(); i++)
    {
        NPC *npc = members[i];
        npc->TriggerEvent( pcpt, maxRange, basePos, baseSector );
    }
}

gemNPCActor* Tribe::GetMostHated(NPC* npc, float range, bool includeInvisible, bool includeInvincible, float* hate)
{
    float mostHatedAmount = 0.0;
    gemNPCActor* mostHated = NULL;
    
    csVector3  pos;
    iSector*   sector;
    
    psGameObject::GetPosition(npc->GetActor(), pos, sector );

    for (size_t i=0; i < members.GetSize(); i++)
    {
        NPC *member = members[i];
        float hateValue = 0;

        gemNPCActor* hated = member->GetMostHated(pos, sector, range, NULL,
                                                  includeInvisible, includeInvincible,
                                                  &hateValue );
        if (!mostHated || hateValue > mostHatedAmount)
        {
            mostHatedAmount = hateValue;
            mostHated = hated;
        }
    }

    if (hate)
    {
        *hate = mostHatedAmount;
    }

    return mostHated;
}

void Tribe::ScopedTimerCallback(const ScopedTimer* timer)
{
    CPrintf(CON_WARNING,"Used %u time to process tick for tribe: %s(ID: %u)\n",
            timer->TimeUsed(),GetName(),GetID());
    npcclient->ListTribes(GetName());
}
