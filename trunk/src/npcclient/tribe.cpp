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
#include "util/location.h"

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
#include "recipe.h"
#include "recipetreenode.h"
#include "gem.h"
#include "networkmgr.h"

extern iDataConnection *db;

const char* Tribe::AssetTypeStr[] = {"ASSET_TYPE_ITEM","ASSET_TYPE_BUILDING","ASSET_TYPE_BUILDINGSPOT"};
const char* Tribe::AssetStatusStr[] = {"ASSET_STATUS_NOT_APPLICABLE","ASSET_STATUS_NOT_USED","ASSET_STATUS_INCONSTRUCTION","ASSET_STATUS_CONSTRUCTED"};

Tribe::Tribe(EventManager *eventmngr)
    :homeSector(0),accWealthGrowth(0.0),deathRate(0.0),resourceRate(0.0)
{
    lastGrowth   = csGetTicks();
    lastDeath    = csGetTicks();
    lastResource = csGetTicks();
    lastAdvance  = csGetTicks();
    eventManager = eventmngr;
}

Tribe::~Tribe()
{
}

bool Tribe::Load(iResultRow& row)
{
    id                              = row.GetInt("id");
    name                            = row["name"];
    // Add tribal recipe
    tribalRecipe = new RecipeTreeNode(recipeManager->GetRecipe(row.GetInt("tribal_recipe")),0);

    homePos                         = csVector3(row.GetFloat("home_x"),row.GetFloat("home_y"),row.GetFloat("home_z"));
    homeRadius                      = row.GetFloat("home_radius");
    homeSectorName                  = row["home_sector_name"];
    maxSize                         = row.GetInt("max_size");
    wealthResourceName              = row["wealth_resource_name"];
    wealthResourceNick              = row["wealth_resource_nick"];
    wealthResourceArea              = row["wealth_resource_area"];

    wealthGatherNeed                = row["wealth_gather_need"];

    wealthResourceGrowth            = row.GetFloat("wealth_resource_growth");
    wealthResourceGrowthActive      = row.GetFloat("wealth_resource_growth_active");
    wealthResourceGrowthActiveLimit = row.GetInt("wealth_resource_growth_active_limit"); 
        
    reproductionCost                = row.GetInt("reproduction_cost");
    npcIdleBehavior                 = row["npc_idle_behavior"];

    return true;
}

bool Tribe::LoadMember(iResultRow& row)
{
    MemberID mID;
    mID.pid               = row.GetInt("member_id");
    mID.tribeMemberType   = row["member_type"];

    membersId.Push(mID);
    
    return true;
}

bool Tribe::AddMember(PID pid, const char* tribeMemberType)
{
    MemberID mID;
    mID.pid = pid;
    mID.tribeMemberType = tribeMemberType;

    membersId.Push(mID);

    // Add to members list in db
    db->Command("INSERT INTO tribe_members (tribe_id,member_id,member_type) "
                "VALUES (%u,%u,'%s')", GetID(), pid.Unbox(),tribeMemberType);

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
    newRes.id     = row.GetInt("id");
    newRes.name   = row["name"];
    newRes.amount = row.GetInt("amount");
    newRes.nick   = row["nick"];
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

bool Tribe::AttachMember(NPC * npc, const char* tribeMemberType)
{
    // Some checks to see if this NPC is fitt for this Tribe
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
    csString name = perception->GetName(npc);
    
    CS::Utility::StringArray<> strarr;
    strarr.SplitString(name, ":");
    csString pcptName = strarr[0];
    
    if (pcptName == "transfer")
    {
        InventoryPerception *invPcpt = dynamic_cast<InventoryPerception*>(perception);
        if (!invPcpt) return;

        AddResource(perception->GetType(),invPcpt->GetCount());
    }
}

void Tribe::AddResource(csString resource, int amount, csString nick)
{
    for (size_t i=0; i < resources.GetSize(); i++)
    {
        if (resources[i].name == resource)
        {
            resources[i].amount += amount;
            resources[i].nick = nick;
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
    int delta = when - lastAdvance;
    if (delta < 0) // Handle wrappover of tick
    {
        delta = 250; // We just set it to the event timer.
    }
    lastAdvance = when;

    // Manage Wealth
    if ( when - lastGrowth > 1000)
    {
        float growth;
        
        // We need to help tribes that have no members with some resources
        // so that they can spawn the first entity
        if (AliveCount() <= 0 && CountResource(wealthResourceName) < reproductionCost)
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

        if(amount != 0) AddResource(wealthResourceName, amount );
        
        lastGrowth = when;
    }
    else if (when - lastGrowth < 0) // Handle wrappoer of tick
    {
        lastGrowth = when;
    }

    // And manage tribe assignments
    csString perc;
    int      decreaseValue = delta; // Set it to change the scale on recipe wait times

    // Manage cyclic recipes
    for(size_t i=0;i<cyclicRecipes.GetSize();i++)
    {
        cyclicRecipes[i].timeLeft -= decreaseValue;
        
        if(cyclicRecipes[i].timeLeft <= 0)
        {
            // Add the recipe and reset counter
            RecipeTreeNode* newNode = new RecipeTreeNode(cyclicRecipes[i].recipe, 0);
            tribalRecipe->AddChild(newNode);
            newNode->priority = CYCLIC_RECIPE_PRIORITY;
            cyclicRecipes[i].timeLeft = cyclicRecipes[i].timeTotal;
        }
    } 
    
    // Manage standard recipes 
    for(size_t i=0;i < members.GetSize(); i++)
    {
        NPC*       npc = members[i];
        Behavior*  behavior = npc->GetCurrentBehavior();

        // For dead npcs just resurrect them
        if(!npc->IsAlive())
        {
            // Issue the resurrect perception if we have enough
            // resources
            perc = "tribe:resurrect";
            Perception pcpt(perc);
            if(AliveCount() == 0 && (CountResource(wealthResourceName) >= 10 * reproductionCost))
            {
                AddResource(wealthResourceName, -10*reproductionCost);
                npc->TriggerEvent(&pcpt);
            }
            else if(CanGrow())
            {
                AddResource(wealthResourceName,-reproductionCost);
                npc->TriggerEvent(&pcpt);
            }
            continue;
        }

        // If we have any npcs with no assignments then
        // we can parse recipes and send them to work
        if(behavior && strcasecmp(behavior->GetName(),npcIdleBehavior.GetDataSafe())==0)
        {
            Debug3(LOG_TRIBES, id, "*** Found Idle NPC %s(%s) checking recipes ***", npc->GetName(),ShowID(npc->GetEID()));

            // Update recipes wait times
            UpdateRecipeData(decreaseValue);

            // Get best recipe. (highest level, no wait time)
            RecipeTreeNode* bestRecipe = tribalRecipe->GetNextRecipe();

            if(!bestRecipe) 
            {
                // Something went wrong... it should never re-parse the tribal recipe
                // High chances to be a scripting error
                break;
            }

            if(bestRecipe->wait <= 0)
            {
                Debug2(LOG_TRIBES, id, "Applying recipe %s.", bestRecipe->recipe->GetName().GetDataSafe());
                
                bestRecipe->nextStep = recipeManager->ApplyRecipe(
                    bestRecipe, this, bestRecipe->nextStep);
            }

            // If nextStep is -1 => Recipe is Completed
            if(bestRecipe->nextStep == -1)
            { 
                // TODO -- Remove TimeStamp
                csString rName = bestRecipe->recipe->GetName();
                if(rName != "do nothing")
                {
                    Debug2(LOG_TRIBES, id, "Recipe %s completed.", rName.GetData());
                }

                tribalRecipe->RemoveChild(bestRecipe->recipe);
            }

            // If nextStep is -2, the recipe has unmet requirements
            else if(bestRecipe->nextStep == -2)
            {
                bestRecipe->nextStep = 0;
            }
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

iSector* Tribe::GetHomeSector()
{
    if (!homeSector)
    {
        homeSector = npcclient->GetEngine()->GetSectors()->FindByName(homeSectorName);
    }
    return homeSector;
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
    Tribe::Memory* memory = NULL;
    
    if (psGetRandom(100) > 10) // 10% chance for go explore a new resource arae
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

Tribe::Memory* Tribe::GetResource(csString resourceName, NPC* npc)
{
    Tribe::Memory* memory = NULL;
    csVector3      pos;
    iSector*       sector;
    float          rot;
    float          range = -1;
    float          locatedRange;
    npc->GetActiveLocate(pos,sector,rot);

    memory = FindNearestMemory(resourceName.GetData(), pos, sector, range, &locatedRange);
    
    if(!memory)
    {
        memory = FindNearestMemory("mine", pos, sector, range, &locatedRange);
    }

    return memory;
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
        npc->Printf("Has this in private knowledge -> do nothing");
        return;
    }
    
    memory = FindMemory(name,pos,sector,radius);
    if (memory)
    {
        npc->Printf("Has this in tribe knowledge -> do nothing");
        return;
    }
    
    npc->Printf("Store in private memory: '%s' %.2f %.2f %2f %.2f '%s'",name.GetDataSafe(),pos.x,pos.y,pos.z,radius,npc->GetName());
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

void Tribe::SendPerception(const char* pcpt, csArray<NPC*> npcs)
{
    Perception perception(pcpt);
    for(int i=0;i<npcs.GetSize();i++)
    {
        NPC* npc = npcs[i];
        Debug4(LOG_TRIBES,GetID(),"--> Percept npc %s(%s): %s",npc->GetName(),ShowID(npc->GetEID()),perception.ToString(npc).GetDataSafe());

        npc->TriggerEvent(&perception);
    }
}

void Tribe::SendPerception(const char* pcpt)
{
    Perception perception(pcpt);
    TriggerEvent(&perception);
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

Recipe* Tribe::GetTribalRecipe() 
{ 
    return tribalRecipe->recipe; 
}

void Tribe::UpdateRecipeData(int delta)
{
    tribalRecipe->UpdateWaitTimes(delta);
}

void Tribe::AddRecipe(Recipe* recipe, Recipe* parentRecipe, bool reqType)
{
    if(!recipe)
    {
        // Avoid a segFault and signal the issue
        CPrintf(CON_ERROR, "Could not add recipe. Received null pointer for recipe.\n");
        return;
    }
    if(!parentRecipe)
    {
        CPrintf(CON_ERROR, "Could not add recipe. Received null pointer for parent recipe.\n");
        return;
    }

    RecipeTreeNode* newNode = new RecipeTreeNode(recipe, 0);
    if(reqType)
    {
        // Make it distributed
        newNode->requirementParseType = RecipeTreeNode::REQ_DISTRIBUTED;
    }
    tribalRecipe->AddChild(newNode, parentRecipe);
}

void Tribe::AddCyclicRecipe(Recipe* recipe, int time)
{
    Tribe::CyclicRecipe newCycle;
    newCycle.recipe = recipe;
    newCycle.timeTotal = newCycle.timeLeft = time;

    cyclicRecipes.Push(newCycle);
}

void Tribe::DeleteCyclicRecipe(Recipe* recipe)
{
    for(int i=0;i<cyclicRecipes.GetSize();i++)
    {
        if(cyclicRecipes[i].recipe == recipe)
        {
            cyclicRecipes.DeleteIndex(i);
            return;
        }
    }
}

bool Tribe::CheckKnowledge(csString knowHow)
{
    for(int i=0;i<knowledge.GetSize();i++)
    {
        if(knowledge[i] == knowHow)
        {
            return true;
        }
    }
    
    return false;
}

void Tribe::SaveKnowledge(csString knowHow)
{
    const char * fields[] =
        { "tribe_id", "knowledge" };
    psStringArray values;
    values.FormatPush("%d", GetID());
    values.FormatPush("%s", knowHow.GetDataSafe());

    int result = db->GenericInsertWithID("sc_tribe_knowledge",fields,values);
    if (result == 0)
    {
        CPrintf(CON_ERROR, "Failed to save knowledge for tribe. Reason: %s.\n",
                db->GetLastError());
        return;
    }
}

void Tribe::DumpKnowledge()
{
    CPrintf(CON_CMDOUTPUT, "Dumping knowledge for tribe %d\n", GetID());
    if(knowledge.GetSize() == 0)
    {
        CPrintf(CON_CMDOUTPUT, "No knowledge.\n");
    }

    for(int i=0;i<knowledge.GetSize();i++)
    {
        CPrintf(CON_CMDOUTPUT, "%s ", knowledge[i].GetData());
    }

    // TODO -- Change the classic printf with something else
    // I used this just to make it look nicer on the console. Don't think it's ok.
    CPrintf(CON_CMDOUTPUT, "\n");
}

void Tribe::DumpRecipesToConsole()
{
    CPrintf(CON_CMDOUTPUT, "Dumping recipe list for tribe %d\n", GetID());
    CPrintf(CON_CMDOUTPUT, "+-------------------------------------------------+\n");
    CPrintf(CON_CMDOUTPUT, "| No | ID | Name              |Pers|Prio|Wait|Step|\n");
    CPrintf(CON_CMDOUTPUT, "+-------------------------------------------------+\n");
    tribalRecipe->DumpRecipeTree();
    CPrintf(CON_CMDOUTPUT, "+-------------------------------------------------+\n");
    tribalRecipe->DumpRecipeTreeRecipes();

}

bool Tribe::LoadNPCMemoryBuffer(const char* name, csArray<NPC*> npcs)
{
    // Check just to be sure
    if(npcs.GetSize() == 0) return false;

    Memory* newLocation = FindMemory(csString(name));

    if(!newLocation)
    {
        return false;
    }

    for(int i=0;i<npcs.GetSize();i++)
    {
        npcs[i]->SetBufferMemory(newLocation);
    }
    return true;
}

void Tribe::LoadNPCMemoryBuffer(Tribe::Memory* memory, csArray<NPC*> npcs)
{
    if(!memory) return;

    // Just Assign
    for(int i=0;i<npcs.GetSize();i++)
    {
        npcs[i]->SetBufferMemory(memory);
    }
}

void Tribe::ModifyWait(Recipe* recipe, int delta)
{
    tribalRecipe->ModifyWait(recipe, delta);
}

bool Tribe::CheckMembers(const csString& type, int number)
{

    // Handle special case where name is 'number'
    if(type == "number")
    {
        // Check if total number is correct (we don't care if their idle or not
        if(number <= members.GetSize())
        {
            return true;
        }
        
        return false;
    }

    for(int i=0;i<members.GetSize();i++)
    {
        NPC* member = members[i];
        
        if(!member->GetCurrentBehavior())
        {
            // Discard Members with no Behavior
            // This shouldn't happen but... ermm just in case
            continue;
        }

        if ((member->GetTribeMemberType() != type) && (type != "any"))
        {
            // Just skip any members of wrong type.
            continue;
        }


        if(strcasecmp(member->GetCurrentBehavior()->GetName(), npcIdleBehavior.GetDataSafe()) == 0)
        {
            number--;
        }
        if(number == 0)
            return true;
    }
    // We don't have enough members
    if(number != 0)
    {
        return false;
    }
}

bool Tribe::CheckResource(csString resource, int number)
{
    for(int i=0;i<resources.GetSize();i++)
    {
        if(resources[i].name == resource)
        {
            if(resources[i].amount >= number)
                return true;
        }
    }
    return false;
}

bool Tribe::CheckAsset(Tribe::AssetType type, csString item, int number)
{
    Asset* asset = GetAsset(type, item);
    if(!asset)
    {
        return false;
    }

    // If the building exists && is not just a spot for construction
    if(asset->type == Tribe::ASSET_TYPE_BUILDING)
    {
        if (asset->status == Tribe::ASSET_STATUS_CONSTRUCTED)
        {
            //Requirement is met
            return true;
        }
    }
    else if (asset->type == Tribe::ASSET_TYPE_ITEM)
    {
        if(asset->quantity > number)
        {
            // We have enough items, requirement is met
            return true;
        } else
        {
            return false;
        }
    }
    
    return false;
}

void Tribe::Build(NPC* npc)
{
    Asset* buildingSpot = npc->GetBuildingSpot();

    if (buildingSpot->status == ASSET_STATUS_INCONSTRUCTION)
    {
        csVector3 buildingPos = npc->GetTribe()->GetHomePosition();
        iSector*  buildingSector = npc->GetTribe()->GetHomeSector();

        buildingPos += buildingSpot->pos;

        buildingSpot->status = ASSET_STATUS_CONSTRUCTED;

        // Now ask the server to spawn the building. Upon spawn a persit item will be sent to the client.
        // The new building will be added to assets as part of the HandlePersitItem prosessing.
        npcclient->GetNetworkMgr()->QueueSpawnBuildingCommand(npc->GetActor(),buildingPos, buildingSector, buildingSpot->name, GetID());
    }
}

void Tribe::HandlePersitItem(gemNPCItem* item)
{
    csVector3 position;
    iSector*  sector;

    psGameObject::GetPosition(item,position,sector);
    
    // Add the new tribe item as an asset.
    // TODO: Find buildings and items and add them seperate, for now add as BUILDING.
    AddAsset(Tribe::ASSET_TYPE_BUILDING, item->GetName(), position, sector, Tribe::ASSET_STATUS_CONSTRUCTED);
}


csArray<NPC*> Tribe::SelectNPCs(const csString& type, const char* number)
{
    int           count = atoi(number);
    csArray<NPC*> npcs;

    bool selectAny = type.CompareNoCase("any");

    // Loop all members. Check for type and if they are idle.
    for(int i=0;i<members.GetSize();i++)
    {
        NPC* member = members[i];

        if ((member->GetTribeMemberType() != type) && !selectAny)
        {
            // Just skip any members of wrong type.
            continue;
        }

        Behavior* currentBehavior = member->GetCurrentBehavior();
        if(!currentBehavior)
        {
            // Newly created NPC or some malfunction.
            // Just skip and try this npc next loop.
            continue;
        }

        if(strcasecmp(currentBehavior->GetName(),npcIdleBehavior.GetDataSafe())==0)
        {
            count--;
            npcs.Push(member);
        }

        // Check if we found the number requested
        if(count == 0)
            return npcs;

    }

    // Return what we found
    return npcs;
}

csString Tribe::GetBuffer(const csString& bufferName)
{
    csString value = tribeBuffer.Get(bufferName,"");

    Debug3(LOG_TRIBES,GetID(),"Get Buffer(%s) return: '%s'",bufferName.GetDataSafe(),value.GetDataSafe());

    return value;
}

void Tribe::SetBuffer(const csString& bufferName, const csString& value)
{
    Debug3(LOG_TRIBES,GetID(),"Set Buffer(%s,%s)",bufferName.GetDataSafe(),value.GetDataSafe());

    tribeBuffer.PutUnique(bufferName,value);
}

void Tribe::ReplaceBuffers(csString& result)
{
    int index = 0;
    BufferHash::GlobalIterator iter = tribeBuffer.GetIterator();
    while (iter.HasNext())
    {
        csString bufferName;
        csString value = iter.Next(bufferName);
        csString replace("$TBUFFER[");
        replace += bufferName;
        replace += "]";
        
        result.ReplaceAll(replace,value);
    }
}

void Tribe::LoadAsset(iResultRow &row)
{

    Asset asset;
    asset.id = row.GetInt("id");
    asset.name = row["name"];
    asset.type = (AssetType)(row.GetInt("type"));
    csVector3 coords;
    coords[0] = row.GetFloat("coordX");
    coords[1] = row.GetFloat("coordY");
    coords[2] = row.GetFloat("coordZ");
    asset.pos = coords;
    asset.item = NULL;
    asset.sector = npcclient->GetEngine()->GetSectors ()->FindByName(row["sector_name"]);
    asset.status = (AssetStatus)(row.GetInt("status"));
    asset.quantity = row.GetInt("quantity");

    assets.Push(asset);
}

void Tribe::SaveAsset(Tribe::Asset* asset, bool deletion)
{
    const char * fields[] =
        {"tribe_id", "name", "type","coordX", "coordY", "coordZ", "sector_id", "gemItemName", "quantity", "status"};
    
    psStringArray values;
    values.FormatPush("%d",GetID());
    values.FormatPush("%s",asset->name.GetData());
    values.FormatPush("%d",asset->type);
    values.FormatPush("%f",asset->pos[0]);
    values.FormatPush("%f",asset->pos[1]);
    values.FormatPush("%f",asset->pos[2]);
    values.FormatPush("%d",asset->sector?Location::GetSectorID(db,asset->sector->QueryObject()->GetName()):-1);
    values.FormatPush("%s","N/A"); // Work this out when items will be available
    values.FormatPush("%d",asset->quantity);
    values.FormatPush("%d",asset->status);

    if(asset->id == -1)
    {
        // It's a new entry
        asset->id = db->GenericInsertWithID("sc_tribe_assets",fields,values);
        if(id == 0)
        {
            CPrintf(CON_ERROR, "Failed to save asset for tribe: %s.\n", db->GetLastError());
            return;
        }
    }
    else
    {
        // Old entry updated
        csString id;
        id.Format("%d",asset->id);

        if(!db->GenericUpdateWithID("sc_tribe_assets","id",id,fields,values))
        {
            CPrintf(CON_ERROR, "Failed to save asset for tribe: %s.\n", db->GetLastError());
            return;
        }
    }
}

void Tribe::AddAsset(Tribe::AssetType type, csString name, gemNPCItem* item, int quantity, int id)
{
    for(int i=0;i<assets.GetSize();i++)
    {
        if(assets[i].name == name)
        {
            assets[i].quantity += quantity;
            SaveAsset(&assets[i]);
            return;
        }
    }

    // No items like this before in the array
    Asset asset;
    asset.id       = id;
    asset.type     = type;
    asset.name     = name;
    asset.item     = item;
    asset.quantity = quantity;
    asset.pos      = csVector3(0,0,0);
    asset.status   = Tribe::ASSET_STATUS_NOT_APPLICABLE;
    assets.Push(asset);
    SaveAsset(&asset);
}

void Tribe::AddAsset(Tribe::AssetType type, csString name, csVector3 position, iSector* sector, Tribe::AssetStatus status)
{
    // Just Add a new spot asset if it does not exist
    if(GetAsset(type, name, position, sector))
    {
        // Asset already exists in the same spot. Bail
        return;
    }

    Asset asset;
    asset.id       = -1;
    asset.type     = type;
    asset.name     = name;
    asset.item     = NULL;
    asset.pos      = position;
    asset.sector   = sector;
    asset.quantity = 1;
    asset.status   = status;

    assets.Push(asset);
    SaveAsset(&asset);
    return;
}

Tribe::Asset* Tribe::GetAsset(Tribe::AssetType type, csString name)
{
    for(int i=0;i<assets.GetSize();i++)
    {
        if(assets[i].type == type && assets[i].name == name)
        {
            return &assets[i];
        }
    }
    
    // If we get here, the asset does not exist and we return a null one
    return NULL;
}

Tribe::Asset* Tribe::GetAsset(Tribe::AssetType type, csString name, Tribe::AssetStatus status)
{
    for(int i=0;i<assets.GetSize();i++)
    {
        if(assets[i].type == type && assets[i].name == name && assets[i].status == status)
        {
            return &assets[i];
        }
    }
    
    // If we get here, the asset does not exist and we return a null one
    return NULL;
}

Tribe::Asset* Tribe::GetAsset(Tribe::AssetType type, csString name, csVector3 where, iSector* sector)
{
    for(int i=0;i<assets.GetSize();i++)
    {
        if(assets[i].type == type && assets[i].name == name && assets[i].pos == where && assets[i].sector == sector)
        {
            return &assets[i];
        }
    }
    
    return NULL;
}

void Tribe::DeleteAsset(csString name, int quantity)
{
    for(int i=0;i<assets.GetSize();i++)
    {
        if(assets[i].name == name)
        {
            assets[i].quantity -= quantity;
            // Remove entry if nothing remains
            if(assets[i].quantity <= 0)
            {
                assets.DeleteIndex(i);
                return;
            }
            SaveAsset(&assets[i]);
            return;
        }
    }
}

void Tribe::DeleteAsset(csString name, csVector3 pos)
{
    for(int i=0;i<assets.GetSize();i++)
    {
        if(assets[i].name == name && assets[i].pos == pos)
        {
            assets.DeleteIndex(i);
            return;
        }
    }
}

void Tribe::DumpAssets()
{
    CPrintf(CON_CMDOUTPUT, "Dumping asset list for tribe %d\n", GetID());

    if(assets.GetSize() == 0)
    {
        CPrintf(CON_CMDOUTPUT, "No assets.\n");
        return;
    }

    CPrintf(CON_CMDOUTPUT, "%-15s %-25s %-6s %-25s %-15s\n",
            "Name", "Type","Quant", "Position", "Status");

    for(int i=0;i<assets.GetSize();i++)
    {
        CPrintf(CON_CMDOUTPUT, "%-15s %-25s %-6d %-25s %-15s\n",
                assets[i].name.GetData(),
                AssetTypeStr[assets[i].type],
                assets[i].quantity,
                toString(assets[i].pos,assets[i].sector).GetDataSafe(),
                AssetStatusStr[assets[i].status]);
    }
}

void Tribe::DumpBuffers()
{
    CPrintf(CON_CMDOUTPUT, "Dumping tribe buffer list for tribe %d\n", GetID());

    if(tribeBuffer.IsEmpty())
    {
        CPrintf(CON_CMDOUTPUT, "No buffers.\n");
        return;
    }

    CPrintf(CON_CMDOUTPUT, "%-15s %-15s\n",
            "Name", "Value");

    csHash<csString,csString>::GlobalIterator iter = tribeBuffer.GetIterator();
    while (iter.HasNext())
    {
        csString bufferName;
        csString value = iter.Next(bufferName);

        CPrintf(CON_CMDOUTPUT, "%-15s %-15s\n",
                bufferName.GetDataSafe(),
                value.GetDataSafe());
    }
}


void Tribe::ProspectMine(NPC* npc, csString resource, csString nick)
{
    if (!npc->GetActor())
    {
        return;
    }

    csVector3  pos;
    iSector*   sector;
    
    psGameObject::GetPosition(npc->GetActor(), pos, sector );

    // Add this mine to the npcs memory
    AddMemory(resource,pos,sector,5,npc);

    // Update the resource nick
    AddResource(resource, 0, nick);
}
