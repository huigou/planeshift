/*
 * spellmanager.cpp by Anders Reggestad <andersr@pvv.org>
 *
 * Copyright (C) 2001-2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include "globals.h"

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/xmltiny.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "net/msghandler.h"
#include "net/messages.h"
#include "util/eventmanager.h"
#include "util/psxmlparser.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psspell.h"
#include "bulkobjects/psglyph.h"

//=============================================================================
// Server Includes
//=============================================================================
#include "spellmanager.h"
#include "clients.h"
#include "playergroup.h"
#include "gem.h"
#include "psserver.h"
#include "psserverchar.h"
#include "cachemanager.h"
#include "progressionmanager.h"
#include "commandmanager.h"

class psPurifyEvent : public psGameEvent
{
public:
    psPurifyEvent(csTicks duration, gemActor *actor, uint32 glyphItemID) : psGameEvent(0, duration, "psPurifyEvent"), actor(actor), glyphItemID(glyphItemID) { }

    void Trigger()
    {
        if (!actor.IsValid())
            return;

        psserver->GetSpellManager()->EndPurifying(actor->GetCharacterData(), glyphItemID);
    }

protected:
    csWeakRef<gemActor> actor;
    uint32 glyphItemID;
};

//----------------------------------------------------------------------------

SpellManager::SpellManager(ClientConnectionSet *ccs,
                               iObjectRegistry * object_reg)
{
    clients      = ccs;
    this->object_reg = object_reg;

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<SpellManager>(this,&SpellManager::SendGlyphs),MSGTYPE_GLYPH_REQUEST,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);  
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<SpellManager>(this,&SpellManager::HandleAssembler),MSGTYPE_GLYPH_ASSEMBLE,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);  
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<SpellManager>(this,&SpellManager::Cast),MSGTYPE_SPELL_CAST,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);  
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<SpellManager>(this,&SpellManager::StartPurifying),MSGTYPE_PURIFY_GLYPH,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);  
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<SpellManager>(this,&SpellManager::SendSpellBook),MSGTYPE_SPELL_BOOK,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);  
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<SpellManager>(this,&SpellManager::HandleCancelSpell),MSGTYPE_SPELL_CANCEL,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);      
}

SpellManager::~SpellManager()
{
    if (psserver->GetEventManager())
    {
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_GLYPH_REQUEST);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_GLYPH_ASSEMBLE);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_SPELL_CAST);  
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_PURIFY_GLYPH);  
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_SPELL_BOOK);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_SPELL_CANCEL);
    }
}


void SpellManager::HandleCancelSpell(MsgEntry* notused, Client* client)
{
    client->GetActor()->InterruptSpellCasting();
}

void SpellManager::HandleAssembler(MsgEntry* me, Client* client)
{
    psGlyphAssembleMessage mesg;
    mesg.FromClient(me);
    
    csArray<psItemStats*> assembler;
    for (size_t i = 0; i < GLYPH_ASSEMBLER_SLOTS; i++)
    {        
        if (mesg.glyphs[i] != 0)
        {
            psItemStats* stats = CacheManager::GetSingleton().GetBasicItemStatsByID(mesg.glyphs[i]);
            if (stats)
                assembler.Push(stats);
        }
    }
    
    if (!client->GetCharacterData()->Inventory().HasPurifiedGlyphs(assembler))
    {
        Error2("Client %i tried to research spell with glyphs he actually doesn't have", client->GetClientNum());
        SendGlyphs(NULL,client);
        return;
    }

    // Is the Glyph Sequence a Valid one?
    psSpell *spell = FindSpell(client, assembler);

    if (spell)
    {
        // Is this spell already in our spellbook?
        psSpell *knownSpell = client->GetCharacterData()->GetSpellByName(spell->GetName());
        if (knownSpell)
        {
            if (mesg.info)
            {
                psGlyphAssembleMessage newmsginfo(client->GetClientNum(), knownSpell->GetName(), knownSpell->GetImage(), knownSpell->GetDescription());
                newmsginfo.SendMessage();
            }
            else
            {
                psserver->SendSystemInfo(client->GetClientNum(), "You know this spell already.");
            }
            return;
        }
    }
    if (mesg.info)
        return;

    csString name("You failed to discover a spell.");
    csString image(" ");
    csString description(" ");

    const bool success = spell && psserver->GetRandom() * 100.0 < spell->ChanceOfResearchSuccess(client->GetCharacterData());
    ProgressionScript *outcome = psserver->GetProgressionManager()->FindScript(success ? "ResearchSpellSuccess" : "ResearchSpellFailure");
    if (outcome)
    {
        MathEnvironment env;
        env.Define("Actor", client->GetActor());
        outcome->Run(&env);
    }

    if (success)
    {
        description = spell->GetDescription();
        name = spell->GetName();
        image = spell->GetImage();
        SaveSpell(client, name);
    }

    // Clear the description, if this is not valid glyph sequence for our player:
    psGlyphAssembleMessage newmsg(client->GetClientNum(), name, image, description);
    newmsg.SendMessage();
}


void SpellManager::SaveSpell(Client * client, csString spellName)
{
    psSpell * spell = client->GetCharacterData()->GetSpellByName(spellName);
    if (spell)
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "You know the %s spell already!",spellName.GetData());
        return;
    }

    spell = CacheManager::GetSingleton().GetSpellByName(spellName);
    if (!spell)
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "%s isn't a defined spell!",spellName.GetData());
        return;
    }


    client->GetCharacterData()->AddSpell(spell);

    psServer::CharacterLoader.SaveCharacterData(client->GetCharacterData(),client->GetActor());

    SendSpellBook(NULL,client);
    psserver->SendSystemInfo(client->GetClientNum(),
                           "%s added to your spell book!",spellName.GetData());
}

void SpellManager::Cast(MsgEntry *me, Client *client)
{
    psSpellCastMessage msg(me);

    psSpell *spell = NULL;
    csString spellName = msg.spell;
    float kFactor = msg.kFactor;

    // Allow developers to cast any spell, even if unknown to the character.
    if (CacheManager::GetSingleton().GetCommandManager()->Validate(client->GetSecurityLevel(), "cast all spells"))
    {
        spell = CacheManager::GetSingleton().GetSpellByName(spellName);
    }        
    else
    {
        spell = client->GetCharacterData()->GetSpellByName(spellName); 
    }

    //spells with empty glyphlists are not enabled
    if (!spell || spell->GetGlyphList().IsEmpty())
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You don't know a spell called %s.",spellName.GetData());
        return;
    }

    // Clamp kFactor to sane values.
    if (kFactor < 0.0)
        kFactor = 0.0;
    if (kFactor > 100.0)
        kFactor = 100.0;

    csString reason;
    if (!spell->CanCast(client, kFactor, reason))
    {
        psserver->SendSystemInfo(client->GetClientNum(), reason.GetData());
        return;
    }

    spell->Cast(client, kFactor);
}

void SpellManager::SendSpellBook(MsgEntry *notused, Client * client)
{
    psSpellBookMessage mesg(client->GetClientNum());
    csArray<psSpell*> spells = client->GetCharacterData()->GetSpellList();
    
    for (size_t i = 0; i < spells.GetSize(); i++)
    {
        csArray<psItemStats*> glyphs = spells[i]->GetGlyphList();

        csString glyphImages[4];
        CS_ASSERT(glyphs.GetSize() <= 4);

        for (size_t j = 0; j < glyphs.GetSize(); j++)
        {
            glyphImages[j] = glyphs[j]->GetImageName();
        }
            
        mesg.AddSpell(spells[i]->GetName(), spells[i]->GetDescription(),
                      spells[i]->GetWay()->name, spells[i]->GetRealm(),
                      glyphImages[0], glyphImages[1],
                      glyphImages[2], glyphImages[3], spells[i]->GetImage());
    }

    mesg.Construct();
    mesg.SendMessage();
}

void SpellManager::SendGlyphs(MsgEntry *notused, Client * client)
{
    psCharacter * character = client->GetCharacterData();
    csArray <glyphSlotInfo> slots;
    size_t slotNum;
    int wayNum = 0;
    
    character->Inventory().CreateGlyphList(slots);
    
    psRequestGlyphsMessage outMessage( client->GetClientNum() );     
    for (slotNum=0; slotNum < slots.GetSize(); slotNum++)
    {
        csString way;
        PSITEMSTATS_SLOTLIST validSlots;
        int statID;

        validSlots = slots[slotNum].glyphType->GetValidSlots();
        statID = slots[slotNum].glyphType->GetUID();

        if (validSlots & PSITEMSTATS_SLOT_CRYSTAL)
        {
            wayNum = 0;
        }
        else if (validSlots & PSITEMSTATS_SLOT_BLUE) 
        {
            wayNum = 1;
        }
        else if (validSlots & PSITEMSTATS_SLOT_AZURE) 
        {
            wayNum = 2;
        }
        else if (validSlots & PSITEMSTATS_SLOT_BROWN)
        {
            wayNum = 3;
        }
        else if (validSlots & PSITEMSTATS_SLOT_RED)
        {
            wayNum = 4;
        }
        else if (validSlots & PSITEMSTATS_SLOT_DARK)
        {
            wayNum = 5;
        }

        outMessage.AddGlyph( slots[slotNum].glyphType->GetName(),
                             slots[slotNum].glyphType->GetImageName(),
                             slots[slotNum].purifyStatus,
                             wayNum, 
                             statID );
                             
    }
    
    outMessage.Construct();
    outMessage.SendMessage();    
}

psGlyph * FindUnpurifiedGlyph(psCharacter * character, unsigned int statID)
{
    size_t index;
    for (index=0; index < character->Inventory().GetInventoryIndexCount(); index++)
    {
        psItem *item = character->Inventory().GetInventoryIndexItem(index);

        psGlyph *glyph = dynamic_cast <psGlyph*> (item);
        if (glyph != NULL)
        {
            if (glyph->GetBaseStats()->GetUID()==statID   &&   glyph->GetPurifyStatus()==0)
            {
                return glyph;
            }
        }
    }
    return NULL;
}

void SpellManager::StartPurifying(MsgEntry *me, Client * client)
{
    psPurifyGlyphMessage mesg(me);
    int statID = mesg.glyph;

    psCharacter* character = client->GetCharacterData();

    psGlyph* glyph = FindUnpurifiedGlyph(character, statID);

    if (glyph == NULL)
    {
        return;
    }        

    if (glyph->GetStackCount() > 1)
    {
        psGlyph* stackOfGlyphs = glyph;

        glyph = dynamic_cast <psGlyph*> (stackOfGlyphs->SplitStack(1));
        if (glyph == NULL)
        {
            return;
        }            
        
        psItem *glyphItem = glyph; // Needed for reference in next function
        if (!character->Inventory().Add(glyphItem,false,false))
        {
            // Notify the player that and why purification failed
            psserver->SendSystemError(client->GetClientNum(), "You can't purify %s because your inventory is full!", glyph->GetName());
            
            CacheManager::GetSingleton().RemoveInstance(glyphItem);

            // Reset the stack count to account for the one that was destroyed.
            stackOfGlyphs->SetStackCount(stackOfGlyphs->GetStackCount() + 1);
            SendGlyphs(NULL,client);
            return;
        }
        stackOfGlyphs->Save(false);
        glyph->Save(false);
    }

    glyph->PurifyingStarted();
    
    // If the glyph has no ID, we must save it now because we need to have an ID for the purification script
    glyph->ForceSaveIfNew();

    // Should probably just assume PURIFYING glyphs actually finished when we load them next.
    psserver->SendSystemInfo(client->GetClientNum(), "You start to purify %s", glyph->GetName());
    psPurifyEvent *evt = new psPurifyEvent(20000, client->GetActor(), glyph->GetUID());
    psserver->GetEventManager()->Push(evt);

    SendGlyphs(NULL,client);
    psserver->GetCharManager()->SendInventory(client->GetClientNum());
}

void SpellManager::EndPurifying(psCharacter * character, uint32 glyphUID)
{
    Client *client = clients->FindPlayer(character->GetPID());
    if (!client)
    {
        Error1("No purifyer!");
        return;
    }

    psItem *item = character->Inventory().FindItemID(glyphUID);
    if (item)
    {
        psGlyph * glyph = dynamic_cast <psGlyph*> (item);
        if (glyph)
        {
            glyph->PurifyingFinished();
            glyph->Save(false);
            psserver->SendSystemInfo(client->GetClientNum(), "The glyph %s is now purified", glyph->GetName());
            SendGlyphs(NULL,client);
            psserver->GetCharManager()->SendInventory(client->GetClientNum());
            return;
        }
    }
}

psSpell* SpellManager::FindSpell(Client *client, const csArray<psItemStats*> & assembler)
{
    CacheManager::SpellIterator loop = CacheManager::GetSingleton().GetSpellIterator();
    
    psSpell *p;
    
    while (loop.HasNext())
    {
        p = loop.Next();

        if (p->MatchGlyphs(assembler))
        {
            // Combination of glyphs correct
            return p;
        }
    }

    return NULL;
}

