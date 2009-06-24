/*
 * psraceinfo.cpp
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <iutil/vfs.h>
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"

#include "../psserver.h"
#include "../cachemanager.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psraceinfo.h"
#include "pssectorinfo.h"

psRaceInfo::psRaceInfo()
{
    uid=0;
    mesh_name=NULL;
    base_texture_name=NULL;
    size.Set(0.0f);
    memset(&attributes,0,sizeof(attributes));
    natural_armor_id=0;
    walkMinSpeed=0.0;
    walkBaseSpeed=3.0;
    walkMaxSpeed=3.0;
    runMinSpeed=3.0;
    runBaseSpeed=5.0;
    runMaxSpeed=10.0;
}

psRaceInfo::~psRaceInfo()
{
}

bool psRaceInfo::Load(iResultRow& row)
{
    uid  = row.GetUInt32("id");
    name = row["name"];
    initialCP = row.GetUInt32("initial_cp");
    race = row.GetUInt32("race");
    helmGroup = row["helm"];
    BracerGroup = row["bracer"];
    sex = row["sex"];

    gender = CacheManager::GetSingleton().ConvertGenderString(row["sex"]);

    iResultSet * rs = db->Select("SELECT * FROM race_spawns WHERE raceid = %lu", race);

    if (!rs || rs->Count() == 0)
    {
        Error2("Race spawn points for race %d not found.", race);
        return false;
    }

    for (unsigned int i = 0 ; i < rs->Count() ; i++)
    {
        psRaceStartingLocation startingLoc;
        startingLoc.x = (*rs)[i].GetFloat("x");
        startingLoc.y = (*rs)[i].GetFloat("y");
        startingLoc.z = (*rs)[i].GetFloat("z");
        startingLoc.yrot = (*rs)[i].GetFloat("yrot");

        psSectorInfo *secinfo=CacheManager::GetSingleton().GetSectorInfoByID((*rs)[i].GetUInt32("sector_id"));
        if (secinfo==NULL)
        {
            Error3("Unresolvable sector id %lu in start_sector_id field of race info for race %u.  Failing!",
                   (*rs)[i].GetUInt32("sector_id"),race);
            return false;
        }

        startingLoc.sector_name = secinfo->name;

        startingLocations.Push(startingLoc);
    }

    rs->Release();

    size.x = row.GetFloat("size_x");
    size.y = row.GetFloat("size_y");
    size.z = row.GetFloat("size_z");

    baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL] = row.GetFloat("base_physical_regen_still");
    baseRegen[PSRACEINFO_STAMINA_PHYSICAL_WALK]  = row.GetFloat("base_physical_regen_walk");
    baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]   = row.GetFloat("base_mental_regen_still");
    baseRegen[PSRACEINFO_STAMINA_MENTAL_WALK]    = row.GetFloat("base_mental_regen_walk");

    const char *meshname=CacheManager::GetSingleton().FindCommonString(row.GetUInt32("cstr_id_mesh"));
    if (meshname==NULL)
    {
        Warning3(LOG_ANY,"Unresolvable mesh id %lu in mesh_id field of race info for race %s. Using NULL mesh.",
                 row.GetUInt32("cstr_id_mesh"),name.GetData() );
    }
    mesh_name = meshname;

    const char *textname=CacheManager::GetSingleton().FindCommonString(row.GetUInt32("cstr_id_base_texture"));
    if (textname==NULL)
    {
        Warning3(LOG_ANY,"Unresolvable texture id %lu in base_texture_id field of race info for race %s. Using NULL texture.",
                 row.GetUInt32("base_texture_id"),name.GetData() );
    }
    base_texture_name = textname;

    // Load starting stats
    SetBaseAttribute(PSITEMSTATS_STAT_STRENGTH      ,row.GetUInt32("start_str"));
    SetBaseAttribute(PSITEMSTATS_STAT_ENDURANCE     ,row.GetUInt32("start_end"));
    SetBaseAttribute(PSITEMSTATS_STAT_AGILITY       ,row.GetUInt32("start_agi"));
    SetBaseAttribute(PSITEMSTATS_STAT_INTELLIGENCE  ,row.GetUInt32("start_int"));
    SetBaseAttribute(PSITEMSTATS_STAT_WILL          ,row.GetUInt32("start_will"));
    SetBaseAttribute(PSITEMSTATS_STAT_CHARISMA      ,row.GetUInt32("start_cha"));

    // Load natural armor
    natural_armor_id = row.GetUInt32("armor_id");

    return true;
}

bool psRaceInfo::LoadBaseSpeeds(iObjectRegistry *object_reg)
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( object_reg);

    csRef<iDocumentSystem> xml (
         csQueryRegistry<iDocumentSystem> (object_reg));

    csRef<iDocument> doc = xml->CreateDocument();

    csString meshFileName;
    meshFileName.Format("/planeshift/models/%s/%s.cal3d",mesh_name,mesh_name);
    csRef<iDataBuffer> buf (vfs->ReadFile (meshFileName.GetData()));
    if (!buf || !buf->GetSize ())
    {
        Error2("Error loading race mesh file. %s\n", meshFileName.GetData());
        return false;
    }

    const char* error = doc->Parse( buf );

    if( error )
    {
        Error3("Error %s loading file %s\n",error, meshFileName.GetData());
        return false;
    }

    csRef<iDocumentNode> cal3dlib = doc->GetRoot()->GetNode("library");
    csRef<iDocumentNode> meshfact = cal3dlib->GetNode("meshfact");
    csRef<iDocumentNode> params = meshfact->GetNode("params");

    csRef<iDocumentNodeIterator> animations = params->GetNodes("animation");
    while ( animations->HasNext() )
    {
        csRef<iDocumentNode> animation = animations->Next();

        if (strcasecmp(animation->GetAttributeValue("name"),"walk") == 0)
        {
            walkBaseSpeed = animation->GetAttributeValueAsFloat("base_vel");
            walkMinSpeed = animation->GetAttributeValueAsFloat("min_vel");
            walkMaxSpeed = animation->GetAttributeValueAsFloat("max_vel");
        }
        else if (strcasecmp(animation->GetAttributeValue("name"),"run") == 0)
        {
            runBaseSpeed = animation->GetAttributeValueAsFloat("base_vel");
            runMinSpeed = animation->GetAttributeValueAsFloat("min_vel");
            runMaxSpeed = animation->GetAttributeValueAsFloat("max_vel");
        }
    }

    Debug8(LOG_STARTUP,0,"LoadedBaseSpeed Walk %.2f(%.2f,%.2f) Run %.2f(%.2f,%.2f) for %s.",
           walkBaseSpeed,walkMinSpeed,walkMaxSpeed,runBaseSpeed,runMinSpeed,runMaxSpeed,name.GetData());

    return true;
}

float psRaceInfo::GetBaseAttribute(PSITEMSTATS_STAT attrib)
{
    if (attrib<0 || attrib>=PSITEMSTATS_STAT_COUNT)
        return 0.0f;

    return ((float)attributes[attrib])/10.0f;
}

void psRaceInfo::SetBaseAttribute(PSITEMSTATS_STAT attrib, float val)
{
    if (attrib<0 || attrib>=PSITEMSTATS_STAT_COUNT)
        return;
    if (val<0.0f)
        val=0.0f;
    attributes[attrib]=(unsigned short)(val*10.0f);
}

void psRaceInfo::GetStartingLocation(float& x,float& y, float& z,float& rot,const char*& sectorname)
{
    psRaceStartingLocation selectedLoc = startingLocations[psserver->GetRandom((uint32)startingLocations.GetSize())];
    x = selectedLoc.x;
    y = selectedLoc.y;
    z = selectedLoc.z;
    rot = selectedLoc.yrot;
    sectorname = selectedLoc.sector_name;
};

const char* psRaceInfo::GetHonorific()
{
    switch (gender)
    {
    case PSCHARACTER_GENDER_FEMALE:
        return "Madam";
    case PSCHARACTER_GENDER_MALE:
        return "Sir";
    default:
        return "Gemma";
    }
}

const char* psRaceInfo::GetObjectPronoun()
{
    switch (gender)
    {
    case PSCHARACTER_GENDER_MALE:
        return "him";
    case PSCHARACTER_GENDER_FEMALE:
        return "her";
    default:
        return "kra";
    }
}

const char* psRaceInfo::GetPossessive()
{
    switch (gender)
    {
    case PSCHARACTER_GENDER_MALE:
        return "his";
    case PSCHARACTER_GENDER_FEMALE:
        return "her";
    default:
        return "kras";
    }
}

csString psRaceInfo::ReadableRaceGender()
{
    switch (gender)
    {
    case PSCHARACTER_GENDER_FEMALE:
        return "Female " + name;
    case PSCHARACTER_GENDER_MALE:
        return "Male " + name;
    default:
        return name;
    }
}
