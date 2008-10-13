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

const char* psTribe::TribeNeedName[] =
{
    "NOTHING",
    "EXPLORE",
    "WALK",
    "DIG",
    "REPRODUCE",
    "RESURRECT"
};

psTribe::psTribe()
    :home_sector(0)
{
    InitializeNeedSet();
}

psTribe::~psTribe()
{
}

bool psTribe::Load(iResultRow& row)
{
    id   = row.GetInt("id");
    name = row["name"];

    home_pos = csVector3(row.GetFloat("home_x"),row.GetFloat("home_y"),row.GetFloat("home_z"));
    home_radius = row.GetFloat("home_radius");
    home_sector_name = row["home_sector_name"];
    max_size = row.GetInt("max_size");
    wealth_resource_name = row["wealth_resource_name"];
    wealth_resource_nick = row["wealth_resource_nick"];
    wealth_resource_area = row["wealth_resource_area"];
    reproduction_cost = row.GetInt("reproduction_cost");

    return true;
}

bool psTribe::LoadMember(iResultRow& row)
{
    int member_id   = row.GetInt("member_id");

    members_id.Push(member_id);
    
    return true;
}

bool psTribe::LoadMemory(iResultRow& row)
{
    Memory * memory = new Memory;
    
    memory->id   = row.GetInt("id");
    memory->name = row["name"];
    memory->pos = csVector3(row.GetFloat("loc_x"),row.GetFloat("loc_y"),row.GetFloat("loc_z"));
    memory->radius = row.GetFloat("radius");
    memory->sector_name = row["sector_name"];
    // Try to find the sector. Will probably fail at this point.
    memory->sector = npcclient->GetEngine()->FindSector(memory->sector_name);
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

void psTribe::SaveMemory(Memory * memory)
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

bool psTribe::LoadResource(iResultRow& row)
{
    Resource new_res;
    new_res.id  = row.GetInt("id");
    new_res.name  = row["name"];
    new_res.amount = row.GetInt("amount");
    resources.Push(new_res);

	return true;
}

void psTribe::SaveResource(Resource* resource, bool new_resource)
{
    const char * fields[] = 
        {"tribe_id","name","amount"};
    psStringArray values;
    values.FormatPush("%d",GetID());
    values.FormatPush("%s",resource->name.GetDataSafe());
    values.FormatPush("%d",resource->amount);

    if (new_resource)
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


bool psTribe::CheckAttach(NPC * npc)
{
    for (size_t i=0; i < members_id.GetSize(); i++)
    {
        if (npc->PID() == members_id[i])
        {
            AttachMember(npc);
            return true;
        }
    }
    for (size_t i=0; i < members.GetSize(); i++)
    {
        if (strcmp(npc->GetName(),members[i]->GetName())==0 &&
            npc->PID() != members[i]->PID())
        {
            AttachMember(npc);

            // Add to members list in db
            db->Command("INSERT INTO tribe_members (tribe_id,member_id) "
                        "VALUES (%u,%u)",GetID(),npc->PID());
            members_id.Push(npc->PID());
            return true;
        }
    }
    
    return false;
}

bool psTribe::AttachMember(NPC * npc)
{
    npc->SetTribe(this);
    for (size_t i=0; i < members.GetSize(); i++)
    {
        if (npc->PID() == members[i]->PID())
        {
            return true;
        }
    }

    // Not in member list so add
    members.Push(npc);

    return true;
}


bool psTribe::HandleDeath(NPC * npc)
{
    dead_members.Push(npc);

    // Make sure memories that isn't stored in the tribe is forgotten.
    ForgetMemories(npc);

    return false;
}

int psTribe::AliveCount() const
{
    int count = 0;
    for (size_t i=0; i < members.GetSize(); i++)
    {
        NPC *npc = members[i];
        if (npc->IsAlive()) count++;
    }
    return count;
}


void psTribe::HandlePerception(NPC * npc, Perception *perception)
{
    csString name = perception->GetName();
    
    csArray<csString> strarr = psSplit(name,':');
    
    if (strarr[0] == "transfer")
    {
        InventoryPerception *inv_pcpt = dynamic_cast<InventoryPerception*>(perception);
        if (!inv_pcpt) return;

        AddResource(perception->GetType(),inv_pcpt->GetCount());
    }
}

void psTribe::AddResource(csString resource, int amount)
{
    for (size_t i=0; i < resources.GetSize(); i++)
    {
        if (resources[i].name == resource)
        {
            resources[i].amount += amount;
            SaveResource(&resources[i],false); // Update resource
            return;
        }
    }
    Resource new_res;
    new_res.name  = resource;
    new_res.amount = amount;
    SaveResource(&new_res,true); // New resource
    resources.Push(new_res);
}

int psTribe::CountResource(csString resource) const
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


void psTribe::Advance(csTicks when,EventManager *eventmgr)
{
    for (size_t i=0; i < members.GetSize(); i++)
    {
        NPC *npc = members[i];
        
        Behavior * behavior = npc->GetCurrentBehavior();

        if ((behavior && strcmp(behavior->GetName(),"do nothing")==0) ||
            (!npc->IsAlive()) )
        {
            if (npc->IsAlive())
            {
                // TODO: Call this only once when returning to home.
                npc->Printf("Share memories with tribe");
                ShareMemories(npc);
            }
            
            csString perc;
            switch (Brain(npc))
            {
            case EXPLORE:
                perc.Format("tribe:explore");
                break;
            case DIG:
                perc.Format("tribe:dig");
                break;
            case REPRODUCE:
                AddResource(wealth_resource_name,-reproduction_cost);
                perc.Format("tribe:reproduce");
                break;
            case RESURRECT:
                perc.Format("tribe:resurrect");
                break;
            case WALK: 
                perc.Format("tribe:path%d",psGetRandom(2)+1);
                break;
            default:
                continue; // Do nothing
            }

            npc->Printf("Tribe brain perception '%s'",perc.GetDataSafe());
            
            Perception perception(perc);
            npcclient->TriggerEvent(npc,&perception);
        }
    }
}

bool psTribe::ShouldGrow() const
{
    return members.GetSize() < (size_t)GetMaxSize();
}

bool psTribe::CanGrow() const
{
    return CountResource(wealth_resource_name) >= reproduction_cost;
}

void psTribe::InitializeNeedSet()
{
   
    needSet = new psTribeNeedSet(this);

    psTribeNeedNothing   * nothing   = new psTribeNeedNothing();
    needSet->AddNeed( nothing );
    psTribeNeedExplore   * explore   = new psTribeNeedExplore();
    needSet->AddNeed( explore );
    psTribeNeedDig       * dig       = new psTribeNeedDig(explore);
    needSet->AddNeed( dig );
    psTribeNeedReproduce * reproduce = new psTribeNeedReproduce(dig);
    needSet->AddNeed( reproduce );
    psTribeNeedWalk      * walk      = new psTribeNeedWalk();
    needSet->AddNeed( walk );
}

psTribe::TribeNeed psTribe::Brain(NPC * npc)
{
    // Handle special case for dead npc's
    if (!npc->IsAlive())
    {
        if (AliveCount() == 0) // Resurrect without cost if every member is dead.
        {
            return RESURRECT;
        }
        else if (CanGrow())
        {
            AddResource(wealth_resource_name,-reproduction_cost);
            return RESURRECT;
        }
        else
        {
            needSet->MaxNeed("Dig");
        }
        return NOTHING;        
    }

    // Continue on for live NPCs

    needSet->UpdateNeed(npc);
    
    return needSet->CalculateNeed(npc);
}

int psTribe::GetMaxSize() const
{
    int size = max_size;
    
    if (size == -1 || size > TRIBE_UNLIMITED_SIZE)
    {
        size = TRIBE_UNLIMITED_SIZE; // NPC Client definition of unlimited size
    }

    return size; 
}


void psTribe::GetHome(csVector3& pos, float& radius, iSector* &sector)
{ 
    pos = home_pos; 
    radius = home_radius; 
    if (home_sector == NULL)
    {
        home_sector = npcclient->GetEngine()->FindSector(home_sector_name);
    }
    sector = home_sector;
}

bool psTribe::GetResource(NPC* npc, csVector3 start_pos, iSector * start_sector, csVector3& located_pos, iSector* &located_sector, float range, bool random)
{
    float located_range=0.0;
    psTribe::Memory * memory = NULL;
    
    if (psGetRandom(100) > 10) // 10% chance for go explor a new resource arae
    {
        csString neededResource = GetNeededResource();

        if (random)
        {
            memory = FindRandomMemory(neededResource,start_pos,start_sector,range,&located_range);
        }
        else
        {
            memory = FindNearestMemory(neededResource,start_pos,start_sector,range,&located_range);
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
            memory = FindRandomMemory(area,start_pos,start_sector,range,&located_range);
        }
        else
        {
            memory = FindNearestMemory(area,start_pos,start_sector,range,&located_range);
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

    located_pos = memory->pos;
    located_sector = memory->GetSector();

    return true;
}

const char* psTribe::GetNeededResource()
{
    return wealth_resource_name;
}

const char* psTribe::GetNeededResourceNick()
{
    return wealth_resource_nick;
}

const char* psTribe::GetNeededResourceAreaType()
{
    return wealth_resource_area;
}


void psTribe::Memorize(NPC * npc, Perception * perception)
{
    // Retriv date from the perception
    csString  name = perception->GetType();
    float     radius = perception->GetRadius();
    csVector3 pos;
    iSector*  sector;
    perception->GetLocation(pos,sector);
        
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

psTribe::Memory* psTribe::FindPrivMemory(csString name,const csVector3& pos, iSector* sector, float radius, NPC * npc)
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

psTribe::Memory* psTribe::FindMemory(csString name,const csVector3& pos, iSector* sector, float radius)
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

psTribe::Memory* psTribe::FindMemory(csString name)
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

void psTribe::AddMemory(csString name,const csVector3& pos, iSector* sector, float radius, NPC * npc)
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

void psTribe::ShareMemories(NPC * npc)
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

iSector* psTribe::Memory::GetSector()
{
    if (sector) return sector;

    sector = npcclient->GetEngine()->FindSector(sector_name);
    return sector;
}


void psTribe::ForgetMemories(NPC * npc)
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

psTribe::Memory *psTribe::FindNearestMemory(const char *name, const csVector3& pos, const iSector* sector, float range, float *found_range)
{
    Memory * nearest = NULL;

    float min_range = range*range;    // Working with Squared values
    if (range == -1) min_range = -1;  // -1*-1 = 1, will use -1 later
    
    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();   

        if (memory->name == name && memory->npc == NULL)
        {
            float dist2 = npcclient->GetWorld()->Distance(pos,sector,memory->pos,memory->GetSector());
            
            if (min_range < 0 || dist2 < min_range)
            {
                min_range = dist2;
                nearest = memory;
            }
        }
    }

    if (nearest && found_range)  // found closest one
    {
        *found_range = sqrt(min_range);
    }

    return nearest;
}

psTribe::Memory *psTribe::FindRandomMemory(const char *name, const csVector3& pos, const iSector* sector, float range, float *found_range)
{
    csArray<Memory*> nearby;
    csArray<float> dist;

    float min_range = range*range;    // Working with Squared values
    if (range == -1) min_range = -1;  // -1*-1 = 1, will use -1 later

    csList<Memory*>::Iterator it(memories);
    while (it.HasNext())
    {
        Memory * memory = it.Next();

        if (memory->name == name && memory->npc == NULL)
        {
            float dist2 = npcclient->GetWorld()->Distance(pos,sector,memory->pos,memory->GetSector());

            if (min_range < 0 || dist2 < min_range)
            {
                nearby.Push(memory);
                dist.Push(dist2);
            }
        }
    }

    if (nearby.GetSize()>0)  // found one or more closer than range
    {
        size_t pick = psGetRandom((uint32)nearby.GetSize());
        
        if (found_range) *found_range = sqrt(dist[pick]);

        return nearby[pick];
    }
    return NULL;
}
