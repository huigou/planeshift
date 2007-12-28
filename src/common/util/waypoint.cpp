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

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/objreg.h>
#include <iutil/object.h>
#include <csutil/csobject.h>
#include <iutil/vfs.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iengine/engine.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/waypoint.h"
#include "util/location.h"
#include "util/log.h"
#include "util/psstring.h"
#include "util/strutil.h"


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

Waypoint::Waypoint(csString& name, 
                   csVector3& pos, csString& sectorName,
                   float radius, csString& flags)
{
    distance = 0.0;
    pi = NULL;
    loc.id = -1;
    loc.name = name;
    loc.pos = pos;
    loc.sectorName = sectorName;
    loc.sector = NULL;
    loc.radius = radius;
    loc.rot_angle = 0.0;
    
    SetFlags(flags);
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
    values.Push(GetFlags());

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

    SetFlags(row["flags"]);

    return true;
}

void Waypoint::AddLink(psPath * path, Waypoint * wp, psPath::Direction direction, float distance)
{
    links.Push(wp);
    paths.Push(path);
    pathDir.Push(direction);
    dists.Push(distance);
    prevent_wander.Push(path->noWander);
}

void Waypoint::AddAlias(csString alias)
{
    aliases.Push(alias);
}


void Waypoint::SetFlags(const csString & flagstr)
{
    allow_return = isFlagSet(flagstr,"ALLOW_RETURN");
    underground  = isFlagSet(flagstr,"UNDERGROUND");
    underwater   = isFlagSet(flagstr,"UNDERWATER");
    priv         = isFlagSet(flagstr,"PRIVATE");
    pub          = isFlagSet(flagstr,"PUBLIC");
    city         = isFlagSet(flagstr,"CITY");
    indoor       = isFlagSet(flagstr,"INDOOR");
}

csString Waypoint::GetFlags()
{
    csString flagStr;
    bool added = false;
    if (allow_return)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("ALLOW_RETURN");
        added = true;
    }
    if (underground)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("UNDERGROUND");
        added = true;
    }
    if (underwater)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("UNDERWATER");
        added = true;
    }
    if (priv)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("PRIVATE");
        added = true;
    }
    if (pub)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("PUBLIC");
        added = true;
    }
    if (city)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("CITY");
        added = true;
    }
    if (indoor)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("INDOOR");
        added = true;
    }
    return flagStr;
}

csString Waypoint::GetAliases()
{
    csString str;
    
    for (size_t i = 0; i < aliases.GetSize(); i++)
    {
        if (i != 0) str.Append(", ");
        str.Append(aliases[i]);
    }
    return str;
}


int Waypoint::Create(iDataConnection *db)
{
    const char *fieldnames[]=
        {
            "name",
            "x",
            "y",
            "z",
            "radius",
            "flags",
            "loc_sector_id"
        };

    psStringArray values;
    values.FormatPush("%s", loc.name.GetDataSafe());
    values.FormatPush("%10.2f",loc.pos.x);
    values.FormatPush("%10.2f",loc.pos.y);
    values.FormatPush("%10.2f",loc.pos.z);
    values.FormatPush("%10.2f",loc.radius);
    values.FormatPush("%s",GetFlags().GetDataSafe());
    values.FormatPush("0");

    loc.id = db->GenericInsertWithID("sc_waypoints",fieldnames,values);

    if (loc.id==0)
    {
        Error2("Failed to create new WP Error %s",db->GetLastError());
        return -1;
    }

    psString cmd;
    cmd.Format("update sc_waypoints set loc_sector_id=(select id from sectors where name='%s') where id=%d",
               loc.sectorName.GetDataSafe(),loc.id);
    db->Command(cmd);
    
    return loc.id;
}

bool Waypoint::CreateAlias(iDataConnection * db, csString alias)
{
    int res =db->Command("insert into sc_waypoint_aliases(wp_id,alias) values(%d,'%s')",
                         GetID(),alias.GetDataSafe());
    if (res != 1)
    {
        return false;
    }
    
    AddAlias(alias);
    return true;
}


bool Waypoint::Adjust(iDataConnection * db, csVector3 & pos, csString sector)
{
    int result = db->CommandPump("UPDATE sc_waypoints SET x=%.2f,y=%.2f,z=%.2f,"
                                 "loc_sector_id=(select id from sectors where name='%s') WHERE id=%d",
                                 pos.x,pos.y,pos.z,sector.GetDataSafe(),loc.id);

    loc.pos = pos;
    loc.sectorName = sector;
    loc.sector = NULL;

    return (result == 1);
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

