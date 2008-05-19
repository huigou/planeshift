/*
 * PaladinJr.cpp - Author: Andrew Dai
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
 */
#include <psconfig.h>
#include <iengine/movable.h>

#include "engine/celbase.h"
#include <iutil/object.h>
#include "util/serverconsole.h"
#include "gem.h"
#include "client.h"
#include "clients.h"
#include "globals.h"
#include "psserver.h"
#include "playergroup.h"
#include "engine/linmove.h"
#include "cachemanager.h"

#include "paladinjr.h"

/*
 * This is the maximum ms of latency that a client is presumed to have.
 */
#define MAX_ACCUMULATED_LAG 10000

void PaladinJr::Initialize(EntityManager* celbase)
{
    iConfigManager* configmanager = psserver->GetConfig();
    enabled = configmanager->GetBool("PlaneShift.Paladin.Enabled");

    const csPDelArray<psCharMode>& modes = CacheManager::GetSingleton().GetCharModes();
    const csPDelArray<psMovement>& moves = CacheManager::GetSingleton().GetMovements();
    
    maxVelocity.Set(0.0f);
    csVector3 maxMod(0);
    
    for(size_t i = 0;i < moves.GetSize(); i++)
    {
        maxVelocity.x = MAX(maxVelocity.x, moves[i]->base_move.x);
        maxVelocity.y = MAX(maxVelocity.y, moves[i]->base_move.y);
        maxVelocity.z = MAX(maxVelocity.z, moves[i]->base_move.z);
    }
    for(size_t i = 0;i < modes.GetSize(); i++)
    {
        maxMod.x = MAX(maxMod.x, modes[i]->move_mod.x);
        maxMod.y = MAX(maxMod.y, modes[i]->move_mod.y);
        maxMod.z = MAX(maxMod.z, modes[i]->move_mod.z);
    }
    maxVelocity.x *= maxMod.x;
    maxVelocity.y *= maxMod.y;
    maxVelocity.z *= maxMod.z;

    // Running forward while strafing
    maxSpeed = 1;


    watchTime = configmanager->GetInt("PlaneShift.Paladin.WatchTime", 30000);
   
    target = NULL;
    entitymanager = celbase;
}

void PaladinJr::PredictClient(Client* client, psDRMessage& currUpdate)
{
    // Don't check GMs/Devs
    if(client->GetSecurityLevel())
        return;

    // Speed check always enabled
    SpeedCheck(client, currUpdate);

    if (!enabled)
        return;

    checkClient = false;

    if (target && (csGetTicks() - started > watchTime))
    {
        checked.Add(target->GetClientNum());
        target = NULL;
        started = csGetTicks();
    }

    if (checked.In(client->GetClientNum()))
    {
        if (!target && csGetTicks() - started > PALADIN_MAX_SWITCH_TIME)
        {
            // We have checked every client online
            started = csGetTicks();
            target = client;
            lastUpdate = currUpdate;
#ifdef PALADIN_DEBUG
            CPrintf(CON_DEBUG, "Now checking client %d\n", target->GetClientNum());
#endif
            checked.DeleteAll();
        }
        return;
    }

    if (!target)
    {
        started = csGetTicks();
        target = client;
        lastUpdate = currUpdate;
#ifdef PALADIN_DEBUG
        CPrintf(CON_DEBUG, "Now checking client %d\n", target->GetClientNum());
#endif
        return;
    }
    else if (target != client)
        return;

    float yrot;
    iSector* sector;


    client->GetActor()->SetDRData(lastUpdate);

    origPos = lastUpdate.pos;
    vel = lastUpdate.vel;
    angVel = lastUpdate.ang_vel;

    //if (vel.x == 0 && vel.z == 0)
    //{
    //    // Minimum speed to cope with client-side timing discrepencies
    //    vel.x = vel.z = -1;
    //}

    // Paladin Jr needs CD enabled on the entity.
    client->GetActor()->pcmove->UseCD(true);
    client->GetActor()->pcmove->SetVelocity(vel);

    // TODO: Assuming maximum lag, need to add some kind of lag prediction here.
    // Note this ignores actual DR packet time interval because we cannot rely
    // on it so must assume a maximal value.
    //
    // Perform the extrapolation here:
    client->GetActor()->pcmove->UpdateDRDelta(2000);

    // Find the extrapolated position
    client->GetActor()->pcmove->GetLastPosition(predictedPos,yrot,sector);

#ifdef PALADIN_DEBUG
    CPrintf(CON_DEBUG, "Predicted: pos.x = %f, pos.y = %f, pos.z = %f\n",predictedPos.x,predictedPos.y,predictedPos.z);
#endif

    maxmove = predictedPos-origPos;

    // No longer need CD checking
    client->GetActor()->pcmove->UseCD(false);

    lastUpdate = currUpdate;
    checkClient = true;
    return;
}

void PaladinJr::CheckClient(Client* client)
{
    if (!enabled || !checkClient || client->GetSecurityLevel())
        return;

    csVector3 pos;
    float yrot;
    iSector* sector;
    csVector3 posChange;


    client->GetActor()->GetPosition(pos,yrot,sector);
    posChange = pos-origPos;

//#ifdef PALADIN_DEBUG
//    CPrintf(CON_DEBUG, "Actual: pos.x = %f, pos.y = %f, pos.z = %f\nDifference = %f\n",pos.x,pos.y,pos.z, (predictedPos - pos).Norm());
//        CPrintf(CON_DEBUG, "Actual displacement: x = %f y = %f z = %f\n", posChange.x, posChange.y, posChange.z);
//        CPrintf(CON_DEBUG, "Maximum extrapolated displacement: x = %f y = %f z = %f\n",maxmove.x, maxmove.y, maxmove.z);
//#endif

    // TODO:
    // Height checking disabled for now because jump data is not sent.
    if (fabs(posChange.x) > fabs(maxmove.x) || fabs(posChange.z) > fabs(maxmove.z))
    {
#ifdef PALADIN_DEBUG
        CPrintf(CON_DEBUG, "CD violation registered for client %s.\n", client->GetName());
        CPrintf(CON_DEBUG, "Details:\n");
        CPrintf(CON_DEBUG, "Original position: x = %f y = %f z = %f\n", origPos.x, origPos.y, origPos.z);
        CPrintf(CON_DEBUG, "Actual displacement: x = %f y = %f z = %f\n", posChange.x, posChange.y, posChange.z);
        CPrintf(CON_DEBUG, "Maximum extrapolated displacement: x = %f y = %f z = %f\n",maxmove.x, maxmove.y, maxmove.z);
        CPrintf(CON_DEBUG, "Previous velocity: x = %f y = %f z = %f\n", vel.x, vel.y, vel.z);
#endif

        csString buf;
        buf.Format("%s, %s, %s, %.3f %.3f %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %s\n", 
                   client->GetName(), "CD violation", sector->QueryObject()->GetName(),origPos.x, origPos.y, origPos.z, 
                   maxmove.x, maxmove.y, maxmove.z, posChange.x, posChange.y, posChange.z, vel.x, vel.y, vel.z, 
                   angVel.x, angVel.y, angVel.z, PALADIN_VERSION);
        psserver->GetLogCSV()->Write(CSV_PALADIN, buf);
    }
}

void PaladinJr::SpeedCheck(Client* client, psDRMessage& currUpdate)
{
    csVector3 oldpos;
    // Dummy variables
    float yrot;
    iSector* sector;

    client->GetActor()->pcmove->GetLastClientPosition (oldpos, yrot, sector);
    
    // If no previous observations then we have nothing to check against.
    if(!sector)
        return;

    float dist = sqrt (pow((currUpdate.pos.x - oldpos.x), 2.0f) +
        pow((currUpdate.pos.z - oldpos.z), 2.0f));

    csTicks timedelta = client->GetActor()->pcmove->ClientTimeDiff();

    float max_noncheat_distance=maxSpeed*timedelta/1000;

    if (fabs(currUpdate.vel.x) <= maxVelocity.x && currUpdate.vel.y <= maxVelocity.y && fabs(currUpdate.vel.z) <= maxVelocity.z && dist<max_noncheat_distance
        + maxSpeed*client->accumulatedLag/1000)
    {
        if (dist < max_noncheat_distance)
        {
            // Calculate the "unused movement time" here and add it to the
            // accumulated lag.
            client->accumulatedLag += (csTicks)((max_noncheat_distance-dist)
                * 1000.0f/maxSpeed);
            if (client->accumulatedLag > MAX_ACCUMULATED_LAG)
                client->accumulatedLag = MAX_ACCUMULATED_LAG;
        }
        else
        {   
            // Subtract from the accumulated lag.
            if(client->accumulatedLag > (csTicks)((dist-max_noncheat_distance) * 1000.0f/maxSpeed))
               client->accumulatedLag-=(csTicks)((dist-max_noncheat_distance)
                * 1000.0f/maxSpeed);
        }
    }
    else
    {

        // Report cheater
        csVector3 vel;
        csVector3 angVel;
        csString buf;
        csString type;
        csString sectorName(sector->QueryObject()->GetName());

        // Player has probably been warped
        if (sector != currUpdate.sector)
        {
            return;
            //sectorName.Append(" to ");
            //sectorName.Append(currUpdate.sectorName);
            //type = "Possible Speed Violation";
        }
        else if(fabs(currUpdate.vel.x) > maxVelocity.x || currUpdate.vel.y > maxVelocity.y || fabs(currUpdate.vel.z) > maxVelocity.z)
            type = "Speed Violation (Hack confirmed)";
        else
            type = "Speed Violation";

        vel = client->GetActor()->pcmove->GetVelocity();
        client->GetActor()->pcmove->GetAngularVelocity(angVel);
        buf.Format("%s, %s, %s, %.3f %.3f %.3f, %.3f 0 %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %s\n",
                   client->GetName(), type.GetData(), sectorName.GetData(),oldpos.x, oldpos.y, oldpos.z,
                   max_noncheat_distance, max_noncheat_distance, 
                   currUpdate.pos.x - oldpos.x, currUpdate.pos.y - oldpos.y, currUpdate.pos.z - oldpos.z,
                   vel.x, vel.y, vel.z, angVel.x, angVel.y, angVel.z, PALADIN_VERSION);
        psserver->GetLogCSV()->Write(CSV_PALADIN, buf);

        Debug5(LOG_CHEAT, client->GetClientNum(),"Player %s traversed %1.2fm in %u msec with an accumulated lag allowance of %u ms. Cheat detected!\n",
            client->GetName (),dist,timedelta,client->accumulatedLag);
    }
}

