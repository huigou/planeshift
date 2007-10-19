/*
 * weathermanager.h
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
#ifndef __WEATHERMANAGER_H__
#define __WEATHERMANAGER_H__

#include <iengine/sector.h>
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>
#include "util/growarray.h"
#include "util/gameevent.h"

#include "net/messages.h"            // Message definitions

#define GAME_HOUR (10*60*1000)

class psWeatherGameEvent;

#include "bulkobjects/pssectorinfo.h"

/**
 * This class handles generation of any and all weather events in the game,
 * including rain, snow, locust storms, day/night cycles, etc.
 */
class WeatherManager
{
protected:
    csRandomGen* randomgen;
    int current_daynight;

    csArray<psWeatherGameEvent*> ignored; // Used for overriding commands like /rain
    csArray<psWeatherGameEvent*> events; // Ugly, but we need a copy of our events

public:
    WeatherManager();
    ~WeatherManager();

    void Initialize();

    void QueueNextEvent(int delayticks,
                        int eventtype,
                        int eventvalue,
                        int duration,
                        int fade,
                        const char *sector,
                        psSectorInfo *si,
                        uint clientnum = 0,
                        int r = 0,
                        int g = 0,
                        int b = 0);

    void StartWeather(psSectorInfo *si);
    void HandleWeatherEvent(psWeatherGameEvent *event);
    void SendClientCurrentTime(int cnum);
    void UpdateClient(uint32_t cnum);
    int GetCurrentTime() {return current_daynight;}
};

/**
 * When a weather event is created, it goes here.
 */
class psWeatherGameEvent : public psGameEvent
{
protected:
    WeatherManager *weathermanager;
    
public:
    int cr,cg,cb;
    int type, value, duration,fade;
    csString sector;
    psSectorInfo *si;
    uint clientnum;

    psWeatherGameEvent(WeatherManager *mgr,
                       int delayticks,
                       int eventtype,
                       int eventvalue,
                       int duration,
                       int fade,
                       const char *sector,
                       psSectorInfo *si,
                       uint client,
                       int r = 0,
                       int g = 0,
                       int b = 0);

    virtual void Trigger();  // Abstract event processing function

    const char *GetType();
};

#endif

