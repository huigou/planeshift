/*
* waypoint.cpp
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

#include <iutil/objreg.h>
#include <iutil/object.h>
#include <csutil/csobject.h>
#include <iutil/vfs.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iengine/engine.h>

#include "util/waypoint.h"
#include "util/location.h"
#include "util/log.h"
#include "util/psstring.h"


Waypoint::Waypoint()
{
    distance = 0.0;
    pi = NULL;
    loc.id = -1;
}

Waypoint::Waypoint(const char* name)
{
    distance = 0.0;
    pi = NULL;
    loc.id = -1;
    loc.name = name;
}


bool Waypoint::Load(iDocumentNode *node, iEngine * engine)
{
    loc.id = 0;
    loc.name = node->GetAttributeValue("name");
    loc.pos.x = node->GetAttributeValueAsFloat("x");
    loc.pos.y = node->GetAttributeValueAsFloat("y");
    loc.pos.z = node->GetAttributeValueAsFloat("z");
    loc.sectorName = node->GetAttributeValue("sector");
    
    if ( loc.sectorName.Length() > 0 )
        loc.sector = engine->FindSector(loc.sectorName);

    loc.radius = node->GetAttributeValueAsFloat("radius");
    loc.rot_angle = 0.0;
    allow_return = node->GetAttributeValueAsBool("allow_return",false); // by default don't allow return to previous point

    return (loc.name.Length()>0);
}

bool Waypoint::Import(iDocumentNode *node, iEngine * engine, iDataConnection *db)
{
    loc.name = node->GetAttributeValue("name");
    loc.pos.x = node->GetAttributeValueAsFloat("x");
    loc.pos.y = node->GetAttributeValueAsFloat("y");
    loc.pos.z = node->GetAttributeValueAsFloat("z");
    loc.sectorName = node->GetAttributeValue("sector");
    
    if ( loc.sectorName.Length() > 0 )
        loc.sector = engine->FindSector(loc.sectorName);

    loc.radius = node->GetAttributeValueAsFloat("radius");
    loc.rot_angle = 0.0;
    allow_return = node->GetAttributeValueAsBool("allow_return",false); // by default don't allow return to previous point


    const char * fields[] = 
        {"name","x","y","z","loc_sector_id","radius","flags"};
    psStringArray values;
    values.Push(loc.name);
    values.FormatPush("%.2f",loc.pos.x);
    values.FormatPush("%.2f",loc.pos.y);
    values.FormatPush("%.2f",loc.pos.z);
    values.FormatPush("%d",loc.GetSectorID(db,loc.sectorName));
    values.FormatPush("%.2f",loc.radius);
    csString flagStr;
    if (allow_return)
    {
        flagStr.Append("ALLOW_RETURN");
    }
    values.Push(flagStr);

    if (loc.id == -1)
    {
        loc.id = db->GenericInsertWithID("sc_waypoints",fields,values);
        if (loc.id == 0)
        {
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",loc.id);
        return db->GenericUpdateWithID("sc_waypoints","id",idStr,fields,values);    
    }

    return true;
}


bool Waypoint::Load(iResultRow& row, iEngine *engine)
{
    loc.id         = row.GetInt("id");
    loc.name       = row["name"];
    loc.pos.x      = row.GetFloat("x");
    loc.pos.y      = row.GetFloat("y");
    loc.pos.z      = row.GetFloat("z");
    loc.sectorName = row["sector"];
    loc.sector     = engine->FindSector(loc.sectorName);
    loc.radius     = row.GetFloat("radius");
    loc.rot_angle  = 0.0;

    psString flagstr(row["flags"]);
    if (flagstr.FindSubString("ALLOW_RETURN",0,true)!=-1)
    {
        allow_return = true;
    }
    else
    {
        allow_return = false;
    }
    return true;
}

bool Waypoint::CheckWithin(iEngine * engine, const csVector3& pos, const iSector* sector)
{
    if (sector == loc.GetSector(engine))
    {
        float range = (loc.pos - pos).Norm();
        if (range <= loc.radius)
            return true;
    }
    
    return false;
}
