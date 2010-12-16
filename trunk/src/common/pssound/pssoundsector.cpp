/*
 * pssoundsector.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
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
#include <crystalspace.h>

#include "pssound.h"
#include "pscelclient.h"
#include "globals.h"

psSoundSector::psSoundSector (csRef<iDocumentNode> sector)
{
    name = sector->GetAttributeValue("NAME");
    playerposition = csVector3(0);
    timeofday = 12;
    activeambient = NULL;
    activemusic = NULL;
    active = false;

    Load(sector);
}

psSoundSector::~psSoundSector ()
{
    Delete();
}

void psSoundSector::AddAmbient (csRef<iDocumentNode> Node)
{
    psMusic     *ambient;

    ambient = new psMusic;

    ambient->resource         = Node->GetAttributeValue("RESOURCE");
    ambient->type             = Node->GetAttributeValueAsInt("TYPE");
    ambient->minvol           = Node->GetAttributeValueAsFloat("MINVOL");
    ambient->maxvol           = Node->GetAttributeValueAsFloat("MAXVOL");
    ambient->fadedelay        = Node->GetAttributeValueAsInt("FADEDELAY");
    ambient->timeofday        = Node->GetAttributeValueAsInt("TIME");
    ambient->timeofdayrange   = Node->GetAttributeValueAsInt("TIME_RANGE");
    ambient->loopstart        = Node->GetAttributeValueAsInt("LOOPSTART");
    ambient->loopend          = Node->GetAttributeValueAsInt("LOOPEND");
    ambient->active           = false;

    //  for wrong xmls - FIXME when youve fixed the xmls
    if (ambient->timeofday == -1)
    {
        ambient->timeofday = 0;
        ambient->timeofdayrange = 24;
    }

    ambientarray.Push(ambient);
}

void psSoundSector::UpdateAmbient (int type, SoundControl* &ctrl)
{
    psMusic *ambient;

    for (size_t i = 0; i< ambientarray.GetSize(); i++)
    {
        ambient = ambientarray[i];

        // check parameters against our world, is this the track we are searching?
        if (ambient->CheckType(type) == true
            && active == true
            && ctrl->GetToggle() == true
            && ambient->CheckTimeOfDay(timeofday) == true)
        {
            if (ambient->active == true)
            {
                continue;
            }

            if (ambient->Play(LOOP, ctrl))
            {
                ambient->FadeUp();
                activeambient = ambient;
            }
            else // error occured .. get rid of this ambient
            {
                DeleteAmbient(ambient);
                break;
            }
        }
        else if (ambient->active == true)
        {
            // state doesnt matter, this handle will be stopped
            ambient->FadeDownAndStop();
            
            if (activeambient == ambient)
            {
                activeambient = NULL;
            }
        }
    }
}

void psSoundSector::DeleteAmbient (psMusic* &ambient)
{
    ambientarray.Delete(ambient);
    delete ambient;
}

void psSoundSector::AddMusic (csRef<iDocumentNode> Node)
{
    psMusic     *music;

    music = new psMusic;

    music->resource         = Node->GetAttributeValue("RESOURCE");
    music->type             = Node->GetAttributeValueAsInt("TYPE");
    music->minvol           = Node->GetAttributeValueAsFloat("MINVOL");
    music->maxvol           = Node->GetAttributeValueAsFloat("MAXVOL");
    music->fadedelay        = Node->GetAttributeValueAsInt("FADEDELAY");
    music->timeofday        = Node->GetAttributeValueAsInt("TIME");
    music->timeofdayrange   = Node->GetAttributeValueAsInt("TIME_RANGE");
    music->loopstart        = Node->GetAttributeValueAsInt("LOOPSTART");
    music->loopend          = Node->GetAttributeValueAsInt("LOOPEND");
    music->active           = false;

    //  for wrong xmls - FIXME when youve fixed the xmls
    if (music->timeofday == -1)
    {
        music->timeofday = 0;
        music->timeofdayrange = 24;
    }

    musicarray.Push(music);
}

void psSoundSector::UpdateMusic (bool loopToggle, int type,
                                 SoundControl* &ctrl)
{
    psMusic *music;

    for (size_t i = 0; i< musicarray.GetSize(); i++)
    {
        music = musicarray[i];

        // check parameters against our world, is this the track we are searching?
        if (music->CheckType(type) == true
            && active == true
            && ctrl->GetToggle() == true
            && music->CheckTimeOfDay(timeofday) == true)
        {
            if (music->active == true)
            {
                // difficult logic. user doesnt want looping BGM
                // therefor we set this to managed and leave it active
                // so it wont be played again unless he reenabled looping or changes sectors etc ..

                if (loopToggle == false)
                {
                    // set managed as we want keep track of it
                    music->SetManaged();
                    music->DontLoop();
                    continue;
                }
                else
                {
                    // resume looping
                    music->Loop();
                    // we no longer manage it
                    music->SetUnManaged();
                    continue;
                }
            }

            if (music->Play(loopToggle, ctrl))
            {
                music->FadeUp();
                activemusic = music;
                break;
            }
            else // error occured .. get rid of this music
            {
                DeleteMusic(music);
            }
        }
        else if (music->active == true)
        {
            // state doesnt matter, this handle will be stopped
            music->FadeDownAndStop();
            
            if (activemusic == music)
            {
                activemusic = NULL;
            }
        }
    }
}

void psSoundSector::DeleteMusic (psMusic* &music)
{
    musicarray.Delete(music);
    delete music;
}

void psSoundSector::AddEmitter (csRef<iDocumentNode> Node)
{
    psEmitter   *emitter;
    
    emitter = new psEmitter;

    emitter->resource       = Node->GetAttributeValue("RESOURCE");
    emitter->minvol         = Node->GetAttributeValueAsFloat("MINVOL");
    emitter->maxvol         = Node->GetAttributeValueAsFloat("MAXVOL");
    emitter->maxrange       = Node->GetAttributeValueAsFloat("MAX_RANGE");
    emitter->minrange       = Node->GetAttributeValueAsFloat("MIN_RANGE");
    emitter->fadedelay      = Node->GetAttributeValueAsInt("FADEDELAY");
    emitter->factory        = Node->GetAttributeValue("FACTORY");
    emitter->factory_prob   = Node->GetAttributeValueAsFloat("FACTORY_PROBABILITY");
    emitter->position       = csVector3 (Node->GetAttributeValueAsFloat("X"),
                                         Node->GetAttributeValueAsFloat("Y"),
                                         Node->GetAttributeValueAsFloat("Z") );
    emitter->direction      = csVector3 (Node->GetAttributeValueAsFloat("2X"),
                                         Node->GetAttributeValueAsFloat("2Y"),
                                         Node->GetAttributeValueAsFloat("2Z") );
    emitter->timeofday      = Node->GetAttributeValueAsInt("TIME");
    emitter->timeofdayrange = Node->GetAttributeValueAsInt("TIME_RANGE");
    emitter->active         = false;

    if (emitter->timeofday == -1)
    {
        emitter->timeofday = 0;
        emitter->timeofdayrange = 24;
    }    
    emitterarray.Push(emitter);
}


//update on position change
void psSoundSector::UpdateEmitter (SoundControl* &ctrl)
{
    psEmitter *emitter;

    // start/stop all emitters in range
    for (size_t i = 0; i< emitterarray.GetSize(); i++)
    {
        emitter = emitterarray[i];

        if (emitter->CheckRange(playerposition) == true
            && active == true
            && ctrl->GetToggle() == true
            && emitter->CheckTimeOfDay(timeofday) == true)
        {
            if (emitter->active == true)
            {
                continue;
            }

            if (!emitter->Play(ctrl))
            {
                // error occured .. emitter cant be played .. remove it
                DeleteEmitter(emitter);
                break;
            }

        }
        else if (emitter->active != false)
        {
            emitter->Stop();
        }
    }
}

void psSoundSector::DeleteEmitter (psEmitter* &emitter)
{
    emitterarray.Delete(emitter);
    delete emitter;
}

void psSoundSector::AddEntity (csRef<iDocumentNode> Node)
{
    psEntity    *entity;
    
    entity = new psEntity;

    entity->resource        = Node->GetAttributeValue("RESOURCE");
    entity->name            = Node->GetAttributeValue("NAME");
    entity->minvol          = Node->GetAttributeValueAsFloat("MINVOL");
    entity->maxvol          = Node->GetAttributeValueAsFloat("MAXVOL");
    entity->minrange        = Node->GetAttributeValueAsFloat("MIN_RANGE");
    entity->maxrange        = Node->GetAttributeValueAsFloat("MAX_RANGE");
    entity->delay_before    = Node->GetAttributeValueAsInt("DELAY_BEFORE");
    entity->delay_after     = Node->GetAttributeValueAsInt("DELAY_AFTER");
    entity->probability     = Node->GetAttributeValueAsFloat("PROBABILITY");
    entity->timeofday       = Node->GetAttributeValueAsInt("TIME");
    entity->timeofdayrange  = Node->GetAttributeValueAsInt("TIME_RANGE");
    entity->active = false;

    if (entity->timeofday == -1)
    {
        entity->timeofday = 0;
        entity->timeofdayrange = 24;
    }

    entityarray.Push(entity);
}

void psSoundSector::UpdateEntity (SoundControl* &ctrl)
{
    psEntity *entity;
    iMeshWrapper* mesh;
    csVector3 rangeVec;
    float range;

    const csPDelArray<GEMClientObject>& entities = psengine->GetCelClient()
                                                   ->GetEntities();

    for (size_t i = 0; i < entityarray.GetSize(); i++)
    {
        entity = entityarray[i];

        if (entity->active == true)
        {
            if (entity->when <= 0)
            {
                // SndSysMgr will pick the dead sound up
                entity->active = false;
            }
            else
            {
                entity->when = (entity->when - 50);
            }

            continue;
        }

        for (size_t a = 0; a < entities.GetSize(); a++)
        {
            if ((mesh = entities[a]->GetMesh()) == NULL)
            {
                continue;
            }

            rangeVec = entities[a]->GetPosition() - (const csVector3&) playerposition;
            range = rangeVec.Norm();

            if (range <= entity->maxrange
                && csStrCaseCmp(entity->name, mesh->QueryObject()->GetName()) == 0
                && rng.Get() <= entity->probability
                && entity->CheckTimeOfDay(timeofday) == true)
            {
                csPrintf("iobject name %s %f %f\n", mesh->QueryObject()->GetName(), range, entity->maxrange);
                if (!entity->Play(ctrl, entities[a]->GetPosition()))
                {
                    DeleteEntity(entity);
                    break;
                }
            }
        }
    }
}

void psSoundSector::DeleteEntity (psEntity* &entity)
{
    entityarray.Delete(entity);
    delete entity;
}

void psSoundSector::Load (csRef<iDocumentNode> sector)
{
    csRef<iDocumentNodeIterator> Itr;
    
    Itr = sector->GetNodes("AMBIENT");
    
    while (Itr->HasNext())
    {
        AddAmbient(Itr->Next());
    }

    Itr = sector->GetNodes("BACKGROUND");

    while (Itr->HasNext())
    {
        AddMusic(Itr->Next());
    }
    
    Itr = sector->GetNodes("EMITTER");
    
    while (Itr->HasNext())
    {
        AddEmitter(Itr->Next());
    }                

    Itr = sector->GetNodes("ENTITY");

    while (Itr->HasNext())
    {
        AddEntity(Itr->Next());
    }
}

void psSoundSector::Reload(csRef<iDocumentNode> sector)
{
    Delete();
    Load(sector);
}

void psSoundSector::Delete()
{
    for (size_t i = 0; i< musicarray.GetSize(); i++)
    {
        DeleteMusic(musicarray[i]);
    }
    
    for (size_t i = 0; i< ambientarray.GetSize(); i++)
    {
        DeleteAmbient(ambientarray[i]);
    }

    for (size_t i = 0; i< emitterarray.GetSize(); i++)
    {
        DeleteEmitter(emitterarray[i]);
    }

    for (size_t i = 0; i < entityarray.GetSize(); i++)
    {
        DeleteEntity(entityarray[i]);
    }
}
