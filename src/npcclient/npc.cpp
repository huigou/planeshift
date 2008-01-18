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


#include <propclass/linmove.h>
#include <propclass/colldet.h>
#include <propclass/mesh.h>
#include <physicallayer/entity.h>
#include <physicallayer/propclas.h>
#include <ivaria/collider.h>

#include "net/msghandler.h"
#include "net/npcmessages.h"
#include "npc.h"
#include "util/log.h"
#include "npcclient.h"
#include "globals.h"
#include "gem.h"
#include "util/psdatabase.h"
#include "util/location.h"
#include "util/strutil.h"
#include "engine/psworld.h"
#include "networkmgr.h"
#include "util/waypoint.h"

extern iDataConnection *db;


NPC::NPC(): checked(false) 
{ 
    brain=NULL; 
    pid=0;
    oldID=0;
    last_update=0; 
    entity=NULL; 
    npcActor=NULL;
    movable=NULL;
    linmove=NULL; 
    colldet=NULL;
    DRcounter=0; 
    active_locate_sector=NULL;
    active_locate_angle=0.0;
    active_locate_wp = NULL;
    ang_vel=vel=999; 
    walkVelocity=runVelocity=0.0; // Will be cached
    region=NULL; 
    last_perception=NULL; 
    debugging=0; 
    alive=false; 
    owner_id=(uint32_t)-1;
    target_id=(uint32_t)-1;
    tribe=NULL;
    raceInfo=NULL;
    checkedSector=NULL;
    checked = false;
    checkedResult = false;
    disabled = false;
}

NPC::~NPC()
{
    if (brain)
    {
        delete brain; 
    }
}


bool NPC::Load(iResultRow& row,BinaryRBTree<NPCType>& npctypes)
{
    name = row["name"];
    pid   = row.GetInt("char_id");
    if ( pid == 0 )
    {
        Error1("NPC has no id attribute. Error in XML");
        return false;
    }

    type = row["npctype"];
    if ( type.Length() == 0 )
    {
        Error1("NPC has no type attribute. Error in XML");
        return false;
    }

    region_name = row["region"]; // optional

    NPCType key(type),*t;
    t = npctypes.Find(&key);
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

    owner_id = row.GetInt("char_id_owner");

    const char *e = row["disabled"];
    if (e && (*e=='Y' || *e=='y'))
    {
        disabled = true;
    }
    else
    {
        disabled = false;
    }

    brain = new NPCType(*t); // deep copy constructor

    return true; // success
}

bool NPC::InsertCopy(int use_char_id, int ownerPID)
{
    int r = db->Command("insert into sc_npc_definitions "
                        "(name, char_id, npctype, region, ang_vel_override, move_vel_override, console_debug, char_id_owner) values "
                        "('%s', %d,      '%s',    '%s',     %f,               %f, '%c',%d)",
                        name.GetData(), use_char_id, type.GetData(), region_name.GetData(), ang_vel, vel, IsDebugging()?'Y':'N', ownerPID);

    if (r!=1)
    {
        Error3("Error in InsertCopy: %s->%s",db->GetLastQuery(),db->GetLastError() );
    }
    else
    {
        Debug2(LOG_NEWCHAR,use_char_id,"Inserted %s",db->GetLastQuery());
    }
    return (r==1);
}

void NPC::SetActor(gemNPCActor * actor)
{
    npcActor = actor;

    // Initialize active location to a known ok value
    if (npcActor)
    {
        entity = actor->GetEntity();

        iSector *sector;
        psGameObject::GetPosition(entity,active_locate_pos,active_locate_angle,sector);

        // Cache some pointers
        csRef<iPcLinearMovement> lm = CEL_QUERY_PROPCLASS(entity->GetPropertyClassList(), 
                                                          iPcLinearMovement);
        linmove = lm;


        csRef<iPcCollisionDetection> cd = CEL_QUERY_PROPCLASS(entity->GetPropertyClassList(), 
                                                              iPcCollisionDetection);
        colldet = cd;

        csRef<iPcMesh> pcmesh = CEL_QUERY_PROPCLASS(entity->GetPropertyClassList(), 
                                                    iPcMesh);
        movable = pcmesh->GetMesh()->GetMovable();
    }
    else
    {
        entity = NULL;
        linmove = NULL;
        colldet = NULL;
        movable = NULL;
    }
}


void NPC::Advance(csTicks when,EventManager *eventmgr)
{
    if (last_update && !disabled)
    {
        brain->Advance(when-last_update,this,eventmgr);
    }

    last_update = when;
}

void NPC::ResumeScript(EventManager *eventmgr,Behavior *which)
{
    if (!disabled)
    {
        brain->ResumeScript(this,eventmgr,which);
    }
}

void NPC::TriggerEvent(Perception *pcpt,EventManager *eventmgr)
{
    if (!disabled)
    {
        Printf(15,"Got event %s",pcpt->ToString().GetData() );
        brain->FirePerception(this,eventmgr,pcpt);
    }
}

void NPC::SetLastPerception(Perception *pcpt)
{
    if (last_perception)
        delete last_perception;
    last_perception = pcpt;
}

iCelEntity *NPC::GetMostHated(float range, bool include_invisible, bool include_invincible)
{
    iSector *sector=NULL;
    csVector3 pos;
    float yrot;
    psGameObject::GetPosition(entity,pos,yrot,sector);

    iCelEntity * hated = hatelist.GetMostHated(sector,pos,range,GetRegion(),include_invisible,include_invincible);

    if (hated)
    {
        Printf(5,"Found most hated: %s(EID: %u)",hated->GetName(),hated->GetID());
        
    }
    else
    {
        Printf(5,"Found no hated entity");
    }

    return hated;
}

void NPC::AddToHateList(iCelEntity *attacker, float delta)
{
    Printf("Adding %1.2f to hatelist score for %s(EID: %u).",
           delta,attacker->GetName(),attacker->GetID() );
    hatelist.AddHate(attacker->GetID(),delta);
    if(IsDebugging(5))
    {
        DumpHateList();
    }
}

void NPC::RemoveFromHateList(PS_ID who)
{
    if (hatelist.Remove(who))
    {
        Printf("Removed PID: %u from hate list.",who );
    }
}

float NPC::GetEntityHate(iCelEntity *ent)
{
    return hatelist.GetHate( ent->GetID() );
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

void NPC::Disable()
{
    disabled = true;

    // Stop the movement
    
    // Set Vel to zero again
    GetLinMove()->SetVelocity( csVector3(0,0,0) );
    GetLinMove()->SetAngularVelocity( 0 );

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(this);
    npcclient->GetNetworkMgr()->QueueImperviousCommand(GetEntity(),true);
}

void NPC::DumpState()
{
    csVector3 loc;
    iSector* sector;
    float rot;
    int instance = -1;

    psGameObject::GetPosition(entity,loc,rot,sector);
    if (npcActor)
    {
        instance = npcActor->GetInstance();
    }

    CPrintf(CON_CMDOUTPUT, "States for %s (PID: %u)\n",name.GetData(),pid);
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");
    CPrintf(CON_CMDOUTPUT, "Position:            %s\n",toString(loc,sector).GetDataSafe());
    CPrintf(CON_CMDOUTPUT, "Rotation:            %.2f\n",rot);
    CPrintf(CON_CMDOUTPUT, "Instance:            %d\n",instance);
    CPrintf(CON_CMDOUTPUT, "Debugging:           %d\n",debugging);
    CPrintf(CON_CMDOUTPUT, "DR Counter:          %d\n",DRcounter);
    CPrintf(CON_CMDOUTPUT, "Alive:               %s\n",alive?"True":"False");
    CPrintf(CON_CMDOUTPUT, "Disabled:            %s\n",disabled?"True":"False");
    CPrintf(CON_CMDOUTPUT, "Checked:             %s\n",checked?"True":"False");
    CPrintf(CON_CMDOUTPUT, "Active locate:       %s\n",toString(active_locate_pos,active_locate_sector).GetDataSafe());
    CPrintf(CON_CMDOUTPUT, "Active locate WP:    %s\n",active_locate_wp?active_locate_wp->GetName():"");
    CPrintf(CON_CMDOUTPUT, "Ang vel:             %.2f\n",ang_vel);
    CPrintf(CON_CMDOUTPUT, "Vel:                 %.2f\n",vel);
    CPrintf(CON_CMDOUTPUT, "Walk velocity:       %.2f\n",walkVelocity);
    CPrintf(CON_CMDOUTPUT, "Run velocity:        %.2f\n",runVelocity);
    CPrintf(CON_CMDOUTPUT, "Owner:               %s\n",GetOwnerName());
    CPrintf(CON_CMDOUTPUT, "Region:              %s\n",GetRegion()?GetRegion()->GetName():"(None)");
    CPrintf(CON_CMDOUTPUT, "Target:              %s\n",GetTarget()?GetTarget()->GetName():"");
}


void NPC::DumpBehaviorList()
{
    CPrintf(CON_CMDOUTPUT, "Behaviors for %s (PID: %u)\n",name.GetData(),pid);
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");
    
    brain->DumpBehaviorList(this);
}

void NPC::DumpReactionList()
{
    CPrintf(CON_CMDOUTPUT, "Reactions for %s (PID: %u)\n",name.GetData(),pid);
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");
    
    brain->DumpReactionList(this);
}

void NPC::DumpHateList()
{
    iSector *sector=NULL;
    csVector3 pos;
    float yrot;
    psGameObject::GetPosition(entity,pos,yrot,sector);

    CPrintf(CON_CMDOUTPUT, "Hate list for %s (PID: %u)\n",name.GetData(),pid );
    CPrintf(CON_CMDOUTPUT, "---------------------------------------------\n");

    hatelist.DumpHateList(pos,sector);
}

void NPC::ClearState()
{
    Printf(5,"ClearState");
    brain->ClearState();
    last_perception = NULL;
    hatelist.Clear();
    SetAlive(false);
    // Enable position check next time npc is attached
    checked = false;
    disabled = false;
}

void NPC::GetNearestEntity(uint32_t& target_id,csVector3& dest,csString& name,float range)
{
    csVector3 loc;
    iSector* sector;
    float rot,min_range;
    target_id = (uint32_t)-1;

    psGameObject::GetPosition(entity,loc,rot,sector);

    csRef<iCelEntityList> nearlist = npcclient->GetPlLayer()->FindNearbyEntities(sector,loc,range);
    if (nearlist)
    {
        min_range=range;
        for (size_t i=0; i<nearlist->GetCount(); i++)
        {
            iCelEntity *ent = nearlist->Get(i);
            if(ent == entity)
                continue;
            csVector3 loc2;
            iSector *sector2;
            float rot2;
            psGameObject::GetPosition(ent,loc2,rot2,sector2);

            float dist = npcclient->GetWorld()->Distance(loc, sector, loc2, sector2);
            if (dist < min_range)
            {
                min_range = dist;
                dest = loc2;
                name = ent->GetName();
                target_id = ent->GetID();
            }
        }
    }
}

iCelEntity* NPC::GetNearestVisibleFriend(float range)
{
    csVector3 loc;
    iSector* sector;
    float rot,min_range;
    iCelEntity *friendEnt = NULL;

    psGameObject::GetPosition(entity,loc,rot,sector);

    csRef<iCelEntityList> nearlist = npcclient->GetPlLayer()->FindNearbyEntities(sector,loc,range);
    if (nearlist)
    {
        min_range=range;
        for (size_t i=0; i<nearlist->GetCount(); i++)
        {
            iCelEntity *ent = nearlist->Get(i);
            NPC* npcFriend = npcclient->FindAttachedNPC(ent);

            if (!npcFriend || npcFriend == this)
                continue;

            csVector3 loc2, isect;
            iSector *sector2;
            float rot2;
            psGameObject::GetPosition(ent,loc2,rot2,sector2);

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
    return friendEnt;
}

void NPC::Printf(const char *msg,...)
{
    va_list args;
    va_start(args, msg);
    VPrintf(5,msg,args);
    va_end(args);
}

void NPC::Printf(int debug, const char *msg,...)
{
    if (!IsDebugging(debug))
        return;

    char str[1024];
    va_list args;

    va_start(args, msg);
    vsprintf(str, msg, args);
    va_end(args);

    CPrintf(CON_CMDOUTPUT, "%s (%u)> %s\n",GetName(),pid,str);
}

void NPC::VPrintf(int debug, const char *msg, va_list args)
{
    if (!IsDebugging(debug))
        return;

    char str[1024];
    vsprintf(str, msg, args);

    CPrintf(CON_CMDOUTPUT, "%s (%u)> %s\n",GetName(),pid,str);
}

iCelEntity *NPC::GetTarget()
{
    // If something is targeted, use it.
    if (target_id != (uint32_t)-1 && target_id != 0)
    {
        // Check if visible
        gemNPCObject * obj = npcclient->FindEntityID(target_id);
        if (obj && !obj->IsVisible()) return NULL;

        iCelEntity *target = npcclient->FindEntity(target_id);
        return target;
    }
    else  // if not, try the last perception entity
    {
        if (GetLastPerception())
        {
            iCelEntity *target = GetLastPerception()->GetTarget();
            Printf(5,"GetTarget returning last perception entity: %s",target ? target->GetName() : "None specified");
            return target;
        }
        return NULL;
    }
}

void NPC::SetTarget(iCelEntity *ent)
{
    if (ent == NULL)
    {
        Printf(10,"Clearing target");
        target_id = (uint32_t)~0;
    }
    else
    {
        Printf(10,"Setting target to: %s",ent->GetName());
        target_id = ent->GetID();
    }
}

iCelEntity *NPC::GetOwner()
{
    if (owner_id != -1)
    {
        gemNPCObject *obj = npcclient->FindCharacterID( owner_id );
        if (obj)
        {
            return obj->GetEntity();
        }
        else
        {
            return NULL;
        }
    }        
    else
        return NULL;        
}

const char * NPC::GetOwnerName()
{
    if (owner_id != -1)
    {
        gemNPCObject *obj = npcclient->FindCharacterID( owner_id );
        if (obj)
        {
            return obj->GetName();
        }
    }        

    return "";        
}

void NPC::SetTribe(psTribe * new_tribe)
{
    tribe = new_tribe;
}

psTribe * NPC::GetTribe()
{
    return tribe;
}

RaceInfo_t* NPC::GetRaceInfo()
{
    if (!raceInfo && npcActor)
    {
        raceInfo = npcclient->GetRaceInfo(npcActor->GetName());
    }
    
    return raceInfo;
}



void NPC::CheckPosition()
{
    // We only need to check the position once
    csRef<iPcMesh> pcmesh = CEL_QUERY_PROPCLASS(entity->GetPropertyClassList(), 
    iPcMesh);

    if(checked)
    {
        if(checkedPos == pcmesh->GetMesh()->GetMovable()->GetPosition() && checkedSector == pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0))
        {
            SetAlive(checkedResult);
            CPrintf(CON_NOTIFY,"Extrapolation skipped, result of: %s\n", checkedResult ? "Alive" : "Dead");
            return;
        }
    }
    if(!alive || linmove->IsPath())
        return;

    // Give the npc a jump start to make sure gravity will be applied.
    csVector3 startVel(0.0f,1.0f,0.0f);
    csVector3 vel;

    linmove->AddVelocity(startVel);
    linmove->SetOnGround(false);
    csVector3 pos(pcmesh->GetMesh()->GetMovable()->GetPosition());
    // See what happens in the next 10 seconds
    int count = 100;
    while (count--)
    {
        linmove->ExtrapolatePosition(0.1f);
    }
    vel = linmove->GetVelocity();
    // Bad starting position - npc is falling at high speed, server should automatically kill it
    if(vel.y < -50)
    {
        CPrintf(CON_ERROR,"Got bad starting location %f %f %f, killing %s (PID: %u/EID: %u).\n",
                pos.x,pos.y,pos.z,name.GetData(),pid,entity->GetID());
        SetAlive(false);
    }

    if(vel == startVel)
    {
        // Collision detection is not being applied!
        linmove->SetVelocity(csVector3(0.0f, 0.0f, 0.0f));
        psGameObject::SetPosition(entity, pos);
    }
    checked = true;
    checkedPos = pos;
    checkedSector = pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0);
    checkedResult = alive;
}

//-----------------------------------------------------------------------------


void HateList::AddHate(int entity_id,float delta)
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

iCelEntity *HateList::GetMostHated(iSector *sector, csVector3& pos, float range, LocationType * region, bool include_invisible, bool include_invincible)
{
    iCelEntity *most = NULL;
    float most_hate_amount=0;

    csRef<iCelEntityList> list = npcclient->GetPlLayer()->FindNearbyEntities(sector,pos,range);
    for (size_t i=0; i<list->GetCount(); i++)
    {
        HateListEntry *h = hatelist.Get( list->Get(i)->GetID(),0 );
        if (h)
        {
            gemNPCObject * obj = npcclient->FindEntityID(list->Get(i)->GetID());

            if (!obj) continue;

            // Skipp if invisible or invincible
            if (obj->IsInvisible() && !include_invisible) continue;
            if (obj->IsInvincible() && !include_invincible) continue;
            
            if (!most || h->hate_amount > most_hate_amount)
            {
                // Don't include if a region is defined and not within region.
                if (region && !region->CheckWithinBounds(npcclient->GetEngine(),pos,sector)) 
                {
                    continue;
                }
                
                most = list->Get(i);
                most_hate_amount = h->hate_amount;
            }
        }
    }
    return most;
}

bool HateList::Remove(int entity_id)
{
    return hatelist.DeleteAll(entity_id);
}

void HateList::Clear()
{
    hatelist.DeleteAll();
}

float HateList::GetHate(int ent)
{
    HateListEntry *h = hatelist.Get(ent, 0);
    if (h)
        return h->hate_amount;
    else
        return 0;
}

void HateList::DumpHateList(const csVector3& myPos, iSector *mySector)
{
    csHash<HateListEntry*>::GlobalIterator iter = hatelist.GetIterator();

    CPrintf(CON_CMDOUTPUT, "%6s %5s %-40s %5s %s\n",
            "Entity","Hated","Pos","Range","Flags");

    while (iter.HasNext())
    {
        HateListEntry *h = (HateListEntry *)iter.Next();
        csVector3 pos(9.9f,9.9f,9.9f);
        gemNPCObject* obj = npcclient->FindEntityID(h->entity_id);
        csString sectorName;
        float yrot;

        if (obj)
        {
            iSector* sector;
            psGameObject::GetPosition(obj->GetEntity(),pos,yrot,sector);
            if(sector)
            {
                sectorName = sector->QueryObject()->GetName();
            }

            pos = obj->pcmesh->GetMesh()->GetMovable()->GetPosition();
            CPrintf(CON_CMDOUTPUT, "%6d %5.1f %40s %5.1f",
                    h->entity_id,h->hate_amount,toString(pos,sector).GetDataSafe(),
                    npcclient->GetWorld()->Distance(pos,sector,myPos,mySector));
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
            CPrintf(CON_CMDOUTPUT, "Entity: %d Hated: %.1f\n",
                    h->entity_id,h->hate_amount);
        }

    }
    CPrintf(CON_CMDOUTPUT, "\n");
}
