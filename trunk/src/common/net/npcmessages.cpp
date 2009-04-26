/*
 * npcmessages.cpp
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

#include "npcmessages.h"
#include <iengine/sector.h>
#include <iengine/engine.h>
#include <iutil/object.h>
#include <csutil/strhashr.h>
#include "util/strutil.h"

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psNPCAuthenticationMessage,MSGTYPE_NPCAUTHENT);

psNPCAuthenticationMessage::psNPCAuthenticationMessage(uint32_t clientnum,
                                                       const char *userid,
                                                       const char *password)
: psAuthenticationMessage(clientnum,userid,password,PS_NPCNETVERSION)
{
    /*
     * This structure is exactly like the regular authentication message,
     * but a different type allows a different subscription, and a different
     * version allows us to change the npc client version without changing
     * the client version.
     */
    msg->SetType(MSGTYPE_NPCAUTHENT);
}

psNPCAuthenticationMessage::psNPCAuthenticationMessage(MsgEntry *message)
: psAuthenticationMessage(message)
{
}

bool psNPCAuthenticationMessage::NetVersionOk()
{
    return netversion == PS_NPCNETVERSION;
}


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psNPCListMessage,MSGTYPE_NPCLIST);

psNPCListMessage::psNPCListMessage(uint32_t clientToken,int size)
{
    msg.AttachNew(new MsgEntry(size));

    msg->SetType(MSGTYPE_NPCLIST);
    msg->clientnum      = clientToken;

    // NB: The message data is actually filled in by npc manager.
}

psNPCListMessage::psNPCListMessage(MsgEntry *message)
{
    if (!message)
        return;
    
    msg = message;
}

csString psNPCListMessage::ToString(AccessPointers * /*access_ptrs*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("TODO");

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psNPCReadyMessage,MSGTYPE_NPCREADY);

psNPCReadyMessage::psNPCReadyMessage()
{
    msg.AttachNew(new MsgEntry());

    msg->SetType(MSGTYPE_NPCREADY);
    msg->clientnum      = 0;
}

psNPCReadyMessage::psNPCReadyMessage(MsgEntry *message)
{
    if (!message)
        return;
    
    msg = message;
}

csString psNPCReadyMessage::ToString(AccessPointers * /*access_ptrs*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("TODO");

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMapListMessage,MSGTYPE_MAPLIST);

psMapListMessage::psMapListMessage(uint32_t clientToken,csString& regions)
{
    msg.AttachNew(new MsgEntry(regions.Length() + 1));

    msg->SetType(MSGTYPE_MAPLIST);
    msg->clientnum      = clientToken;

    msg->Add(regions);
}

psMapListMessage::psMapListMessage(MsgEntry *message)
{
    csString str = message->GetStr();
    
    // Find first delimiter
    size_t loc = str.FindFirst('|');
    if(loc == (size_t) -1)
    	loc = str.Length();
    size_t begin = 0;
    while(begin < str.Length())
    {
    	map.Push(str.Slice(begin, loc - begin));
    	
    	// Move to next delimiter
    	begin = loc + 1;
    	if(begin >= str.Length())
    		break;
    	
    	// Find next delimiter
    	loc = str.FindFirst('|', begin);
    	if(loc == (size_t) -1)
    		loc = str.Length();
    }

}

csString psMapListMessage::ToString(AccessPointers * /*access_ptrs*/)
{
    csString msgtext;
    for (size_t c = 0; c < map.GetSize(); c++)
    {
        msgtext.AppendFmt(" %zu: '%s'",c,map[c].GetDataSafe());
    }
    
    return msgtext;
}


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psNPCRaceListMessage,MSGTYPE_NPCRACELIST);

psNPCRaceListMessage::psNPCRaceListMessage(uint32_t clientToken, int count)
{
    msg.AttachNew(new MsgEntry(sizeof(int16) + count*(100+2*sizeof(float))));

    msg->SetType(MSGTYPE_NPCRACELIST);
    msg->clientnum      = clientToken;
    msg->Add( (int16) count);

}

psNPCRaceListMessage::psNPCRaceListMessage(MsgEntry *message)
{
    size_t count = (size_t)message->GetInt16();
    for (size_t c = 0; c < count; c++)
    {
        NPCRaceInfo_t ri;
        ri.name = message->GetStr();
        ri.walkSpeed = message->GetFloat();
        ri.runSpeed = message->GetFloat();
        raceInfo.Push(ri);
    }
}

void psNPCRaceListMessage::AddRace(csString& name, float walkSpeed, float runSpeed, bool last)
{
    msg->Add(name.GetDataSafe());
    msg->Add( (float)walkSpeed);
    msg->Add( (float)runSpeed);
    if (last)
    {
        msg->ClipToCurrentSize();
    }
}

csString psNPCRaceListMessage::ToString(AccessPointers * /*access_ptrs*/)
{
    csString msgtext;
    for (size_t c = 0; c < raceInfo.GetSize(); c++)
    {
        msgtext.AppendFmt(" name: '%s' walkSpeed: %.2f runSpeed %.2f",
                          raceInfo[c].name.GetDataSafe(),
                          raceInfo[c].walkSpeed,raceInfo[c].runSpeed);
    }
    
    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psNPCCommandsMessage,MSGTYPE_NPCOMMANDLIST);

psNPCCommandsMessage::psNPCCommandsMessage(uint32_t clientToken,int size)
{
    msg.AttachNew(new MsgEntry(size));

    msg->SetType(MSGTYPE_NPCOMMANDLIST);
    msg->clientnum      = clientToken;

    // NB: The message data is actually filled in by npc manager.
}

psNPCCommandsMessage::psNPCCommandsMessage(MsgEntry *message)
{
    if (!message)
        return;
    
    msg = message;
}

csString psNPCCommandsMessage::ToString(AccessPointers * access_ptrs)
{
    csString msgtext;

    char cmd = msg->GetInt8();
    while (cmd != CMD_TERMINATOR && !msg->overrun)
    {
        switch(cmd)
        {
            case psNPCCommandsMessage::CMD_DRDATA:
            {
                msgtext.Append("CMD_DRDATA: ");

                // Extract the data
                uint32_t len = 0;
				void *data = msg->GetBufferPointerUnsafe(len);

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Error2("Received incomplete CMD_DRDATA from NPC client %u.\n",msg->clientnum);
                    break;
                }
                
                if (access_ptrs->msgstrings && access_ptrs->engine)
                {
                    psDRMessage drmsg(data,len,
                                      access_ptrs->msgstrings,
                                      access_ptrs->engine);  // alternate method of cracking 

                    msgtext.Append(drmsg.ToString(access_ptrs));
                }
                else
                {
                    msgtext.Append("Missing msg strings to decode");
                }
                break;
            }
            case psNPCCommandsMessage::CMD_ATTACK:
            {
                msgtext.Append("CMD_ATTACK: ");

                // Extract the data
                EID attacker_id = EID(msg->GetUInt32());
                EID target_id = EID(msg->GetUInt32());

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_ATTACK from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("Attacker: %u Target: %u ", attacker_id.Unbox(), target_id.Unbox());
                break;
            }
            case psNPCCommandsMessage::CMD_SPAWN:
            {
                msgtext.Append("CMD_SPAWN: ");

                // Extract the data
                EID spawner_id = EID(msg->GetUInt32());
                EID spawned_id = EID(msg->GetUInt32());

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_SPAWN from NPC client %u.\n",msg->clientnum);
                    break;
                }
                
                msgtext.AppendFmt("Spawner: %u Spawned: %d ", spawner_id.Unbox(), spawned_id.Unbox());
                break;
            }
            case psNPCCommandsMessage::CMD_TALK:
            {
                msgtext.Append("CMD_TALK: ");

                // Extract the data
                EID speaker_id = EID(msg->GetUInt32());
                const char* text = msg->GetStr();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_TALK from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("Speaker: %u Text: %s ", speaker_id.Unbox(), text);
                break;
            }
            case psNPCCommandsMessage::CMD_VISIBILITY:
            {
                msgtext.Append("CMD_VISIBILITY: ");

                // Extract the data
                EID entity_id = EID(msg->GetUInt32());
                bool status = msg->GetBool();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_VISIBILITY from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Status: %s ", entity_id.Unbox(), status ? "true" : "false");
                break;
            }
            case psNPCCommandsMessage::CMD_PICKUP:
            {
                msgtext.Append("CMD_PICKUP: ");

                // Extract the data
                EID entity_id = EID(msg->GetUInt32());
                EID item_id   = EID(msg->GetUInt32());
                int count     = msg->GetInt16();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_PICKUP from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Item: %u Count: %d ", entity_id.Unbox(), item_id.Unbox(), count);
                break;
            }


            case psNPCCommandsMessage::CMD_EQUIP:
            {
                msgtext.Append("CMD_EQUIP: ");

                // Extract the data
                EID entity_id = EID(msg->GetUInt32());
                csString item = msg->GetStr();
                csString slot = msg->GetStr();
                int count     = msg->GetInt16();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_EQUIP from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Item: %s Slot: %s Count: %d ", entity_id.Unbox(), item.GetData(), slot.GetData(), count);
                break;
            }

            case psNPCCommandsMessage::CMD_DEQUIP:
            {
                msgtext.Append("CMD_DEQUIP: ");

                // Extract the data
                EID entity_id = EID(msg->GetUInt32());
                csString slot = msg->GetStr();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_DEQUIP from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Slot: %s ", entity_id.Unbox(), slot.GetData());
                break;
            }

            case psNPCCommandsMessage::CMD_DIG:
            {
                msgtext.Append("CMD_DIG: ");

                // Extract the data
                EID entity_id = EID(msg->GetUInt32());
                csString resource = msg->GetStr();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_DIG from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Resource: %s ", entity_id.Unbox(), resource.GetData());
                break;
            }

            case psNPCCommandsMessage::CMD_DROP:
            {
                msgtext.Append("CMD_DROP: ");

                // Extract the data
                EID entity_id = EID(msg->GetUInt32());
                csString slot = msg->GetStr();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_DROP from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Slot: %s ", entity_id.Unbox(), slot.GetData());
                break;
            }

            case psNPCCommandsMessage::CMD_TRANSFER:
            {
                msgtext.Append("CMD_TRANSFER: ");

                // Extract the data
                EID entity_id = EID(msg->GetUInt32());
                csString item = msg->GetStr();
                int count = msg->GetInt8();
                csString target = msg->GetStr();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_TRANSFER from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Item: %s Count: %d Target: %s ", entity_id.Unbox(), item.GetDataSafe(), count, target.GetDataSafe());
                break;
            }

            case psNPCCommandsMessage::CMD_RESURRECT:
            {
                msgtext.Append("CMD_RESURRECT: ");

                // Extract the data
                csVector3 where;

                EID character_id = EID(msg->GetUInt32());
                float rot = msg->GetFloat();
                where.x = msg->GetFloat();
                where.y = msg->GetFloat();
                where.z = msg->GetFloat();
                csString sector = msg->GetStr();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_RESURRECT from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("ID: %u Rot: %.2f Where: (%.2f,%.2f,%.2f) Sector: %s ",
                                  character_id.Unbox(),rot,where.x,where.y,where.z,sector.GetDataSafe());
                break;
            }

            case psNPCCommandsMessage::CMD_SEQUENCE:
            {
                msgtext.Append("CMD_SEQUENCE: ");

                csString name = msg->GetStr();
                int cmd = msg->GetUInt8();
                int count = msg->GetInt32();
                
                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_SEQUENCE from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("Name: %s Cmd: %d Count: %d ",
                                  name.GetDataSafe(),cmd,count);
                break;
            }

            case psNPCCommandsMessage::CMD_IMPERVIOUS:
            {
                msgtext.Append("CMD_IMPERVIOUS: ");

                EID entity_id = EID(msg->GetUInt32());
                bool impervious = msg->GetBool();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete CMD_IMPERVIOUS from NPC client %u.\n",msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Impervious: %s ", entity_id.Unbox(), impervious ? "true" : "false");
                break;
            }

            // perceptions go from server to superclient
            
            case psNPCCommandsMessage::PCPT_TALK: 
            {
                msgtext.Append("PCPT_TALK: ");
                EID speaker = EID(msg->GetUInt32());
                EID target  = EID(msg->GetUInt32());
                int faction   = msg->GetInt16();

                msgtext.AppendFmt("Speaker: %u Target: %u Faction: %d ", speaker.Unbox(), target.Unbox(), faction);
                break;
            }
            case psNPCCommandsMessage::PCPT_ATTACK:
            {
                msgtext.Append("PCPT_ATTACK: ");
                EID target   = EID(msg->GetUInt32());
                EID attacker = EID(msg->GetUInt32());

                msgtext.AppendFmt("Target: %u Attacker: %u ", target.Unbox(), attacker.Unbox());
                break;
            }
            case psNPCCommandsMessage::PCPT_GROUPATTACK:
            {
                msgtext.Append("PCPT_GROUPATTACK: ");
                EID target   = EID(msg->GetUInt32());
                msgtext.AppendFmt("Target: %u", target.Unbox());
                int groupCount = msg->GetUInt8();
                for (int i=0; i<groupCount; i++)
                {
                    EID attacker = EID(msg->GetUInt32());
                    int bestSkillSlot = msg->GetInt8();
                    msgtext.AppendFmt("Attacker(%i): %u, BestSkillSlot: %d ", i, attacker.Unbox(), bestSkillSlot);
                }

                break;
            }
            case psNPCCommandsMessage::PCPT_DMG:
            {
                msgtext.Append("PCPT_DMG: ");
                EID attacker = EID(msg->GetUInt32());
                EID target   = EID(msg->GetUInt32());
                float dmg      = msg->GetFloat();

                msgtext.AppendFmt("Attacker: %u Target: %u Dmg: %.1f ", attacker.Unbox(), target.Unbox(), dmg);
                break;
            }
            case psNPCCommandsMessage::PCPT_SPELL:
            {
                msgtext.Append("PCPT_SPELL: ");
                EID caster = EID(msg->GetUInt32());
                EID target = EID(msg->GetUInt32());
                uint32_t strhash = msg->GetUInt32();
                float    severity = msg->GetInt8() / 10;
                csString type = access_ptrs->msgstrings->Request(strhash);

                msgtext.AppendFmt("Caster: %u Target: %u Type: \"%s\"(%u) Severity: %f ", caster.Unbox(), target.Unbox(), type.GetData(), strhash, severity);
                break;
            }
            case psNPCCommandsMessage::PCPT_DEATH:
            {
                msgtext.Append("PCPT_DEATH: ");
                EID who = EID(msg->GetUInt32());

                msgtext.AppendFmt("Who: %u ", who.Unbox());
                break;
            }
            case psNPCCommandsMessage::PCPT_ANYRANGEPLAYER:
            case psNPCCommandsMessage::PCPT_LONGRANGEPLAYER:
            case psNPCCommandsMessage::PCPT_SHORTRANGEPLAYER:
            case psNPCCommandsMessage::PCPT_VERYSHORTRANGEPLAYER:
            {
                csString range = "?";
                if (cmd == psNPCCommandsMessage::PCPT_ANYRANGEPLAYER)
                    range = "ANY";
                if (cmd == psNPCCommandsMessage::PCPT_LONGRANGEPLAYER)
                    range = "LONG";
                if (cmd == psNPCCommandsMessage::PCPT_SHORTRANGEPLAYER)
                    range = "SHORT";
                if (cmd == psNPCCommandsMessage::PCPT_VERYSHORTRANGEPLAYER)
                    range = "VERYSHORT";
                
                msgtext.AppendFmt("PCPT_%sRANGEPLAYER: ",range.GetData());
                EID npcid   = EID(msg->GetUInt32());
                EID player  = EID(msg->GetUInt32());
                float faction = msg->GetFloat();

                msgtext.AppendFmt("NPCID: %u Player: %u Faction: %.0f ", npcid.Unbox(), player.Unbox(), faction);
                break;
            }
            case psNPCCommandsMessage::PCPT_OWNER_CMD:
            {
                msgtext.Append("PCPT_OWNER_CMD: ");
                psPETCommandMessage::PetCommand_t command = (psPETCommandMessage::PetCommand_t) msg->GetUInt32();
                EID owner_id = EID(msg->GetUInt32());
                EID pet_id = EID(msg->GetUInt32());
                EID target_id = EID(msg->GetUInt32());

                msgtext.AppendFmt("Command: %s OwnerID: %u PetID: %u TargetID: %u ",
                                  psPETCommandMessage::petCommandString[command], owner_id.Unbox(), pet_id.Unbox(), target_id.Unbox());
                break;
            }
            case psNPCCommandsMessage::PCPT_OWNER_ACTION:
            {
                msgtext.Append("PCPT_OWNER_ACTION: ");
                int action = msg->GetInt32();
                EID owner_id = EID(msg->GetUInt32());
                EID pet_id = EID(msg->GetUInt32());

                msgtext.AppendFmt("Action: %u OwnerID: %u PetID: %u ", action, owner_id.Unbox(), pet_id.Unbox());
                break;
            }
            case psNPCCommandsMessage::PCPT_INVENTORY:
            {
                msgtext.Append("PCPT_INVENTORY: ");
                EID owner_id = EID(msg->GetUInt32());
                csString item_name = msg->GetStr();
                bool inserted = msg->GetBool();
                int count = msg->GetInt16();

                msgtext.AppendFmt("OwnerID: %u ItemName: %s Inserted: %s Count: %d ", owner_id.Unbox(), item_name.GetData(), inserted ? "true" : "false", count);
                break;
            }
            case psNPCCommandsMessage::PCPT_FLAG:
            {
                msgtext.Append("PCPT_FLAG: ");
                EID owner_id = EID(msg->GetUInt32());
                uint32_t flags = msg->GetUInt32();
                csString str;
                if (flags & INVISIBLE)  str.Append(" INVISIBLE");
                if (flags & INVINCIBLE) str.Append(" INVINCIBLE");

                msgtext.AppendFmt("OwnerID: %u Flags: %s ", owner_id.Unbox(), str.GetDataSafe());
                break;
            }
            case psNPCCommandsMessage::PCPT_NPCCMD:
            {
                msgtext.Append("PCPT_NPCCMD: ");
                EID owner_id = EID(msg->GetUInt32());
                csString cmd   = msg->GetStr();

                msgtext.AppendFmt("OwnerID: %u Cmd: %s ", owner_id.Unbox(), cmd.GetDataSafe());
                break;
            }

            case psNPCCommandsMessage::PCPT_TRANSFER:
            {
                msgtext.Append("PCPT_TRANSFER: ");

                // Extract the data
                EID entity_id = EID(msg->GetUInt32());
                csString item = msg->GetStr();
                int count = msg->GetInt8();
                csString target = msg->GetStr();

                // Make sure we haven't run past the end of the buffer
                if (msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,msg->clientnum,"Received incomplete PCPT_TRANSFER from NPC client %u.\n", msg->clientnum);
                    break;
                }

                msgtext.AppendFmt("EID: %u Item: %s Count: %d Target: %s ", entity_id.Unbox(), item.GetDataSafe(), count, target.GetDataSafe());
                break;
            }


        }
        cmd = msg->GetInt8();
    }
    if (msg->overrun)
    {
        Debug2(LOG_NET,msg->clientnum,"Received unterminated or unparsable psNPCCommandsMessage from client %u.\n",msg->clientnum);
    }


    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psAllEntityPosMessage,MSGTYPE_ALLENTITYPOS);

psAllEntityPosMessage::psAllEntityPosMessage(MsgEntry *message)
{
    msg = message;

    count = msg->GetInt16();
}

void psAllEntityPosMessage::SetLength(int elems, int client)
{
    msg.AttachNew(new MsgEntry(2 + elems * (4 * sizeof(float) + sizeof(uint32_t) + 100*sizeof(char)) ));
    msg->SetType(MSGTYPE_ALLENTITYPOS);
    msg->clientnum      = client;

    msg->Add((int16_t)elems);
}

void psAllEntityPosMessage::Add(EID id, csVector3& pos, iSector*& sector, InstanceID instance, csStringHashReversible* msgstrings)
{
    msg->Add(id.Unbox());
    msg->Add(pos.x);
    msg->Add(pos.y);
    msg->Add(pos.z);

    const char* sectorName = sector->QueryObject ()->GetName ();
    csStringID sectorNameStrId = msgstrings ? msgstrings->Request(sectorName) : csInvalidStringID;

    msg->Add( (uint32_t) sectorNameStrId );

    if (sectorNameStrId == csInvalidStringID)
    {
        msg->Add(sectorName);
    }
    msg->Add( (int32_t)instance );
}

EID psAllEntityPosMessage::Get(csVector3& pos, iSector*& sector, InstanceID& instance, csStringHashReversible* msgstrings, iEngine *engine)
{
    EID eid(msg->GetUInt32());
    pos.x = msg->GetFloat();
    pos.y = msg->GetFloat();
    pos.z = msg->GetFloat();

    csString sectorName;
    csStringID sectorNameStrId;
    sectorNameStrId = msg->GetUInt32();
    if (sectorNameStrId != csStringID(uint32_t(csInvalidStringID)))
    {
        sectorName = msgstrings->Request(sectorNameStrId);
    }
    else
    {
        sectorName = msg->GetStr();
    }
    if(!sectorName.IsEmpty())
    {
        sector = engine->GetSectors ()->FindByName (sectorName);
    }
    else
    {
        sector = NULL;
    }
    instance = msg->GetUInt32();
    return eid;
}

csString psAllEntityPosMessage::ToString(AccessPointers * access_ptrs)
{
    csString msgtext;
    
    msgtext.AppendFmt("Count: %d",count);
    for (int i = 0; i < count; i++)
    {
        csVector3 pos;
        iSector* sector;
        InstanceID instance;
        
        EID eid = Get(pos, sector, instance, access_ptrs->msgstrings, access_ptrs->engine);

        msgtext.AppendFmt(" ID: %u Pos: %s Inst: %d", eid.Unbox(), toString(pos,sector).GetDataSafe(), instance);
    }

    return msgtext;
}


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psNewNPCCreatedMessage,MSGTYPE_NEW_NPC);

psNewNPCCreatedMessage::psNewNPCCreatedMessage(uint32_t clientToken, PID new_npc_id, PID master_id, PID owner_id)
{
    msg.AttachNew(new MsgEntry( 3*sizeof(int) ));

    msg->SetType(MSGTYPE_NEW_NPC);
    msg->clientnum = clientToken;

    msg->Add(new_npc_id.Unbox());
    msg->Add(master_id.Unbox());
    msg->Add(owner_id.Unbox());
}

psNewNPCCreatedMessage::psNewNPCCreatedMessage(MsgEntry *message)
{
    if (!message)
        return;

    new_npc_id = PID(message->GetUInt32());    
    master_id  = PID(message->GetUInt32());
    owner_id   = PID(message->GetUInt32());
}

csString psNewNPCCreatedMessage::ToString(AccessPointers * /*access_ptrs*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("New NPC ID: %d", new_npc_id.Unbox());
    msgtext.AppendFmt(" Master ID: %d", master_id.Unbox());
    msgtext.AppendFmt(" Owner ID: %d",  owner_id.Unbox());

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPETCommandMessage,MSGTYPE_PET_COMMAND);

const char * psPETCommandMessage::petCommandString[]=
    {"CMD_FOLLOW","CMD_STAY","CMD_DISMISS","CMD_SUMMON","CMD_ATTACK",
     "CMD_GUARD","CMD_ASSIST","CMD_STOPATTACK","CMD_NAME","CMD_TARGET"};

psPETCommandMessage::psPETCommandMessage(uint32_t clientToken, int cmd, const char * target, const char * options )
{
    size_t targetlen = 0;
    size_t optionslen = 0;
    if (target!=NULL)
        targetlen = strlen( target);
    if (options!=NULL)
        optionslen = strlen( options);

    msg.AttachNew(new MsgEntry( sizeof(int) + targetlen + optionslen + 2));

    msg->SetType(MSGTYPE_PET_COMMAND);
    msg->clientnum = clientToken;

    msg->Add( (int32_t)cmd );
    msg->Add( target );
    msg->Add( options );
}

psPETCommandMessage::psPETCommandMessage(MsgEntry *message)
{
    if (!message)
        return;

    command = message->GetInt32();    
    target  = message->GetStr();    
    options = message->GetStr();
}

csString psPETCommandMessage::ToString(AccessPointers * /*access_ptrs*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("Command: %s",petCommandString[command]);
    msgtext.AppendFmt(" Target: '%s'",target.GetDataSafe());
    msgtext.AppendFmt(" Options: '%s'",options.GetDataSafe());

    return msgtext;
}


//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psServerCommandMessage,MSGTYPE_NPC_COMMAND);

psServerCommandMessage::psServerCommandMessage( uint32_t clientnum,
                                                    const char* buf)
{

    if ( !buf )
        buf = "";
    
    msg.AttachNew(new MsgEntry( strlen(buf)+1, PRIORITY_HIGH ));
    msg->clientnum  = clientnum;
    msg->SetType(MSGTYPE_NPC_COMMAND);

    msg->Add( buf );
}


psServerCommandMessage::psServerCommandMessage( MsgEntry* msgEntry ) 
{
   if ( !msgEntry )
       return;

   command = msgEntry->GetStr();
}


//---------------------------------------------------------------------------


