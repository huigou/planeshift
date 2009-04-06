/*
* location.cpp - author: Keith Fulton <keith@paqrat.com>
*
* Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include "location.h"
#include "util/consoleout.h"
#include "util/log.h"
#include "util/psstring.h"


/*------------------------------------------------------------------*/

Location::~Location()
{
    while (locs.GetSize())
    {
        Location * loc = locs.Pop();
        delete loc;
    }
}

iSector* Location::GetSector(iEngine * engine)
{
    if (sector) return sector;
    
    sector = engine->FindSector(sectorName.GetDataSafe());
    return sector;
}

int Location::GetSectorID(iDataConnection *db,const char* name)
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


bool Location::Load(iResultRow& row, iEngine *engine, iDataConnection *db)
{
    id         = row.GetInt("id");
    name       = row["name"];
    pos.x      = row.GetFloat("x");
    pos.y      = row.GetFloat("y");
    pos.z      = row.GetFloat("z");
    rot_angle  = row.GetFloat("angle");;
    radius     = row.GetFloat("radius");
    sectorName = row["sector"];
    // Cache sector if engine is available.
    if (engine)
    {
        sector     = engine->FindSector(sectorName);
    }
    else
    {
        sector = NULL;
    }
    
    id_prev_loc_in_region = row.GetInt("id_prev_loc_in_region");

    return true;
}


bool Location::Import(iDocumentNode *node, iDataConnection *db,int typeID)
{
    name       = node->GetAttributeValue("name");
    pos.x      = node->GetAttributeValueAsFloat("x");
    pos.y      = node->GetAttributeValueAsFloat("y");
    pos.z      = node->GetAttributeValueAsFloat("z");
    rot_angle  = node->GetAttributeValueAsFloat("angle");
    radius     = node->GetAttributeValueAsFloat("radius");
    sectorName = node->GetAttributeValue("sector");
    id_prev_loc_in_region = 0; // Not suppored for import.


    const char * fields[] = 
        {"type_id","id_prev_loc_in_region","name","x","y","z","angle","radius","flags","loc_sector_id"};
    psStringArray values;
    values.FormatPush("%d",typeID);
    values.FormatPush("%d",id_prev_loc_in_region);
    values.Push(name);
    values.FormatPush("%.2f",pos.x);
    values.FormatPush("%.2f",pos.y);
    values.FormatPush("%.2f",pos.z);
    values.FormatPush("%.2f",rot_angle);
    values.FormatPush("%.2f",radius);
    csString flagStr;
    values.Push(flagStr);
    values.FormatPush("%d",GetSectorID(db,sectorName));

    if (id == -1)
    {
        id = db->GenericInsertWithID("sc_locations",fields,values);
        if (id == 0)
        {
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",id);
        return db->GenericUpdateWithID("sc_locations","id",idStr,fields,values);    
    }

    return true;
}

/*------------------------------------------------------------------*/

LocationType::~LocationType()
{
    while (locs.GetSize())
    {
        Location * loc = locs.Pop(); //removes reference
        //now delete the location (a polygon). Since this will delete all points
        //on the polygon, and the first is a reference to loc, make sure to 
        //delete that reference first:
        loc->locs.DeleteIndex(0); 
        // now delete the polygon
        delete loc;
    }
}


bool LocationType::Load(iDocumentNode *node)
{
    name = node->GetAttributeValue("name");
    if ( !name.Length() )
    {
        CPrintf(CON_ERROR, "Location Types must all have name attributes.\n");
        return false;
    }

    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        // This is a widget so read it's factory to create it.
        if ( strcmp( node->GetValue(), "loc" ) == 0 )
        {
            Location *newloc = new Location;
            newloc->pos.x      = node->GetAttributeValueAsFloat("x");
            newloc->pos.y      = node->GetAttributeValueAsFloat("y");
            newloc->pos.z      = node->GetAttributeValueAsFloat("z");
            newloc->rot_angle  = node->GetAttributeValueAsFloat("angle");
            newloc->radius     = node->GetAttributeValueAsFloat("radius");
            newloc->sectorName = node->GetAttributeValue("sector");
            locs.Push(newloc);
        }
    }
    return true;
}

bool LocationType::Import(iDocumentNode *node, iDataConnection *db)
{
    name = node->GetAttributeValue("name");
    if ( !name.Length() )
    {
        CPrintf(CON_ERROR, "Location Types must all have name attributes.\n");
        return false;
    }

    const char * fields[] = 
        {"name"};
    psStringArray values;
    values.Push(name);

    if (id == -1)
    {
        id = db->GenericInsertWithID("sc_location_type",fields,values);
        if (id == 0)
        {
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",id);
        return db->GenericUpdateWithID("sc_location_type","id",idStr,fields,values);
    }
    

    return true;
}


bool LocationType::Load(iResultRow& row, iEngine * engine, iDataConnection *db)
{
    id   = row.GetInt("id");
    name = row["name"];
    if ( !name.Length() )
    {
        CPrintf(CON_ERROR, "Location Types must all have name attributes.\n");
        return false;
    }

    // Load all with same master location type
    Result rs(db->Select("select loc.*,s.name as sector from sc_locations loc, sectors s where loc.loc_sector_id = s.id and type_id = %d and id_prev_loc_in_region <= 0",id)); // Only load locations, regions to be loaded later

    if (!rs.IsValid())
    {
        Error2("Could not load locations from db: %s",db->GetLastError() );
        return false;
    }
    for (int i=0; i<(int)rs.Count(); i++)
    {    
        Location *newloc = new Location;
        newloc->Load(rs[i],engine,db);
        locs.Push(newloc);
    }

    // Load region with same master location type
    Result rs2(db->Select("select loc.*,s.name as sector from sc_locations loc, sectors s where loc.loc_sector_id = s.id and type_id = %d and id_prev_loc_in_region > 0",id)); // Only load regions, locations has been loaded allready

    csArray<Location*> tmpLocs;

    if (!rs2.IsValid())
    {
        Error2("Could not load locations from db: %s",db->GetLastError() );
        return false;
    }
    for (int i=0; i<(int)rs2.Count(); i++)
    {    
        Location *newloc = new Location;
        newloc->Load(rs2[i],engine,db);

        tmpLocs.Push(newloc);
    }
    while (tmpLocs.GetSize())
    {
        Location *curr, *first;
        curr = first = tmpLocs.Pop();
        bool   found;
        first->locs.Push(first);
        do
        {
            found = false;
            for (size_t i= 0; i<tmpLocs.GetSize();i++)
            {
                if (curr->id == tmpLocs[i]->id_prev_loc_in_region)
                {
                    curr = tmpLocs[i];
                    tmpLocs.DeleteIndex(i);
                    first->locs.Push(curr);
                    found = true;
                    break;
                }
            }
            
        } while (found);
        
        //when not a closed loop of at least 3 points, delete this
        //polygon, but continue with rest.
        if (first->locs.GetSize() <= 2)
        {
            Error1("Only two locs for region defined!");
            //delete all locations in 'polygon'. When deleting first,
            //it will recursively delete its polygon locations, in this
            //case including itself. So remove that reference first
            first->locs.DeleteIndex(0);
            delete first;
        }
        else if (curr->id != first->id_prev_loc_in_region)
        {
            Error1("First and last loc not connected!");
            //delete all locations in 'polygon'. When deleting first,
            //it will recursively delete its polygon locations, in this
            //case including itself. So remove that reference first
            first->locs.DeleteIndex(0);
            delete first; 
        }
        else
        {
            locs.Push(first);
        }
    }    
    
    return true;
}

bool LocationType::CheckWithinBounds(iEngine * engine, const csVector3& p,const iSector* sector)
{
    for (size_t i = 0; i < locs.GetSize(); i++)
    {
        if (locs[i]->CheckWithinBounds(engine,p,sector)) return true;
    }
    
    return false;
}

bool Location::CheckWithinBounds(iEngine * engine,const csVector3& p,const iSector* sector)
{
    if (!IsRegion())
        return false;

    if (GetSector(engine) != sector)
        return false;
    
    // Thanks to http://astronomy.swin.edu.au/~pbourke/geometry/insidepoly/
    // for this example code.
    int counter = 0;
    size_t i,N=locs.GetSize();
    float xinters;
    csVector3 p1,p2;

    p1 = locs[0]->pos;
    for (i=1; i<=N; i++)
    {
        p2 = locs[i % N]->pos;
        if (p.z > MIN(p1.z,p2.z))
        {
            if (p.z <= MAX(p1.z,p2.z))
            {
                if (p.x <= MAX(p1.x,p2.x))
                {
                    if (p1.z != p2.z)
                    {
                        xinters = (p.z-p1.z)*(p2.x-p1.x)/(p2.z-p1.z)+p1.x;
                        if (p1.x == p2.x || p.x <= xinters)
                            counter++;
                    }
                }
            }
        }
        p1 = p2;
    }

    return (counter % 2 != 0);
}

/*------------------------------------------------------------------*/
