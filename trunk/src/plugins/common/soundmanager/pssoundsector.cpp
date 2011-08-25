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

#include "soundmanager.h"

psSoundSector::psSoundSector(const char* sectorName, iObjectRegistry* objReg)
{
    name = sectorName;
    objectReg = objReg;

    playerposition = csVector3(0);
    timeofday = 12;
    active = false;

    // initializing pointers to null
    activeambient = 0;
    activemusic = 0;
}

psSoundSector::psSoundSector(csRef<iDocumentNode> sector, iObjectRegistry* objReg)
{
    name = sector->GetAttributeValue("NAME");
    playerposition = csVector3(0);
    timeofday = 12;
    activeambient = NULL;
    activemusic = NULL;
    active = false;

    objectReg = objReg;

    Load(sector);
}

psSoundSector::~psSoundSector()
{
    Delete();
}

void psSoundSector::AddAmbient(csRef<iDocumentNode> Node)
{
    psMusic* ambient;

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
    if(ambient->timeofday == -1)
    {
        ambient->timeofday = 0;
        ambient->timeofdayrange = 24;
    }

    ambientarray.Push(ambient);
}

void psSoundSector::UpdateAmbient(int type, SoundControl* &ctrl)
{
    psMusic* ambient;

    for(size_t i = 0; i< ambientarray.GetSize(); i++)
    {
        ambient = ambientarray[i];

        // check parameters against our world, is this the track we are searching?
        if(ambient->CheckType(type) == true
            && active == true
            && ctrl->GetToggle() == true
            && ambient->CheckTimeOfDay(timeofday) == true)
        {
            if(ambient->active == true)
            {
                continue;
            }

            if(ambient->Play(LOOP, ctrl))
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
        else if(ambient->active == true)
        {
            // state doesnt matter, this handle will be stopped
            ambient->FadeDownAndStop();
            
            if(activeambient == ambient)
            {
                activeambient = NULL;
            }
        }
    }
}

void psSoundSector::DeleteAmbient(psMusic* &ambient)
{
    ambientarray.Delete(ambient);
    delete ambient;
}

void psSoundSector::AddMusic(csRef<iDocumentNode> Node)
{
    psMusic* music;

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
    if(music->timeofday == -1)
    {
        music->timeofday = 0;
        music->timeofdayrange = 24;
    }

    musicarray.Push(music);
}

void psSoundSector::UpdateMusic(bool loopToggle, int type,
                                 SoundControl* &ctrl)
{
    psMusic* music;

    for(size_t i = 0; i< musicarray.GetSize(); i++)
    {
        music = musicarray[i];

        // check parameters against our world, is this the track we are searching?
        if(music->CheckType(type) == true
            && active == true
            && ctrl->GetToggle() == true
            && music->CheckTimeOfDay(timeofday) == true)
        {
            if(music->active == true)
            {
                // difficult logic. user doesnt want looping BGM
                // therefor we set this to managed and leave it active
                // so it wont be played again unless he reenabled looping or changes sectors etc ..

                if(loopToggle == false)
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

            if(music->Play(loopToggle, ctrl))
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
        else if(music->active == true)
        {
            // state doesnt matter, this handle will be stopped
            music->FadeDownAndStop();
            
            if(activemusic == music)
            {
                activemusic = NULL;
            }
        }
    }
}

void psSoundSector::DeleteMusic(psMusic* &music)
{
    musicarray.Delete(music);
    delete music;
}

void psSoundSector::AddEmitter(csRef<iDocumentNode> Node)
{
    psEmitter* emitter;
    
    emitter = new psEmitter;

    emitter->resource       = Node->GetAttributeValue("RESOURCE");
    emitter->minvol         = Node->GetAttributeValueAsFloat("MINVOL");
    emitter->maxvol         = Node->GetAttributeValueAsFloat("MAXVOL");
    emitter->maxrange       = Node->GetAttributeValueAsFloat("MAX_RANGE");
    emitter->minrange       = Node->GetAttributeValueAsFloat("MIN_RANGE");
    emitter->probability    = Node->GetAttributeValueAsFloat("PROBABILITY", 1.0);
    emitter->loop           = Node->GetAttributeValueAsBool("LOOP", true);
    emitter->dopplerEffect  = Node->GetAttributeValueAsBool("DOPPLER_ENABLED", true);
    emitter->fadedelay      = Node->GetAttributeValueAsInt("FADEDELAY");
    emitter->factory        = Node->GetAttributeValue("FACTORY");
    emitter->factory_prob   = Node->GetAttributeValueAsFloat("FACTORY_PROBABILITY");
    emitter->position       = csVector3(Node->GetAttributeValueAsFloat("X"),
                                        Node->GetAttributeValueAsFloat("Y"),
                                        Node->GetAttributeValueAsFloat("Z"));
    emitter->direction      = csVector3(Node->GetAttributeValueAsFloat("2X"),
                                        Node->GetAttributeValueAsFloat("2Y"),
                                        Node->GetAttributeValueAsFloat("2Z"));
    emitter->timeofday      = Node->GetAttributeValueAsInt("TIME");
    emitter->timeofdayrange = Node->GetAttributeValueAsInt("TIME_RANGE");
    emitter->active         = false;

    // adjusting the probability on the update time
    emitter->probability *= SoundManager::updateTime / 1000.0f;

    if(emitter->timeofday == -1)
    {
        emitter->timeofday = 0;
        emitter->timeofdayrange = 24;
    }    
    emitterarray.Push(emitter);
}


//update on position change
void psSoundSector::UpdateEmitter(SoundControl* &ctrl)
{
    psEmitter* emitter;

    // start/stop all emitters in range
    for(size_t i = 0; i< emitterarray.GetSize(); i++)
    {
        emitter = emitterarray[i];

        if(emitter->CheckRange(playerposition) == true
           && active == true
           && ctrl->GetToggle() == true
           && emitter->CheckTimeOfDay(timeofday) == true)
        {
            if(emitter->active == true)
            {
                continue;
            }

            if(rng.Get() <= emitter->probability)
            {
                if(!emitter->Play(ctrl))
                {
                    // error occured .. emitter cant be played .. remove it
                    DeleteEmitter(emitter);
                    break;
                }
            }

        }
        else if(emitter->active != false)
        {
            emitter->Stop();
        }
    }
}

void psSoundSector::DeleteEmitter(psEmitter* &emitter)
{
    emitterarray.Delete(emitter);
    delete emitter;
}

void psSoundSector::AddEntity(csRef<iDocumentNode> Node)
{
    const char* factoryName;
    const char* meshName;
    int state;
    const char* resource;
    const char* startResource;
    float volume;
    float minRange;
    float maxRange;
    float prob;
    int timeOfDayStart;
    int timeOfDayEnd;
    int delayAfter;
    psEntity* entity;

    factoryName   = Node->GetAttributeValue("FACTORY");
    meshName      = Node->GetAttributeValue("MESH");
    state         = Node->GetAttributeValueAsInt("STATE", -1);
    resource      = Node->GetAttributeValue("RESOURCE");
    startResource = Node->GetAttributeValue("STARTING_RESOURCE");
    maxRange      = Node->GetAttributeValueAsFloat("MAX_RANGE", -1.0);
    prob          = Node->GetAttributeValueAsFloat("PROBABILITY", -1.0);

    // checking that all mandatory parameters are present
    if((factoryName == 0 && meshName == 0)
        || (factoryName != 0 && meshName != 0))
    {
        return;
    }
    if(state < 0 || prob < 0.0 || maxRange < 0.0)
    {
        return;
    }
    if(resource == 0 && startResource == 0)
    {
        return;
    }

    // check if an entity with the same name is already defined
    if(meshName == 0)
    {
        entity = factories.Get(factoryName, 0);
    }
    else
    {
        entity = meshes.Get(meshName, 0);
    }

    // if it doesn't exist create it otherwise check the state
    if(entity == 0)
    {
        entity = new psEntity();

        // handle mesh/factory entities
        if(meshName == 0)
        {
            entity->factoryName = factoryName;
            factories.Put(factoryName, entity);
        }
        else
        {
            entity->meshName = meshName;
            meshes.Put(meshName, entity);
        }
    }

    // set all parameters
    volume          = Node->GetAttributeValueAsFloat("VOLUME", VOLUME_NORM);
    minRange        = Node->GetAttributeValueAsFloat("MIN_RANGE");
    delayAfter      = Node->GetAttributeValueAsInt("DELAY_AFTER");
    timeOfDayStart  = Node->GetAttributeValueAsInt("TIME_START", -1);
    timeOfDayEnd    = Node->GetAttributeValueAsInt("TIME_END", 25);

    // adjusting the probability on the update time
    prob = prob / 1000 * SoundManager::updateTime;

    entity->DefineState(state, resource, startResource, volume,
        minRange, maxRange, prob, timeOfDayStart, timeOfDayEnd, delayAfter);
}

void psSoundSector::UpdateEntity(SoundControl* &ctrl, psSoundSector* commonSector)
{
    csRef<iEngine> engine;
    iMeshList* entities;
    psEntity* entity;
    iMeshWrapper* mesh;
    uint meshID;
    const char* meshName;
    const char* factoryName = 0;
    

    engine =  csQueryRegistry<iEngine>(objectReg);
    if(!engine)
    {
        csPrintf("Error: no iEngine plugin!");
        return;
    }

    entities = engine->GetMeshes();

    for(int a = 0; a < entities->GetCount(); a++)
    {
        if((mesh = entities->Get(a)) == 0)
        {
            continue;
        }

        // checking if we already have a temporary entity for the mesh
        // here we don't check the common sector because the common do
        // not have temporary entities
        meshID = mesh->QueryObject()->GetID();
        if((entity = tempEntities.Get(meshID, 0)) != 0)
        {
            UpdateEntityValues(ctrl, entity, mesh);
            continue;
        }


        // check if the factory is defined for this mesh
        meshName = mesh->QueryObject()->GetName();  
        iMeshFactoryWrapper* mfw = mesh->GetFactory();
        if(mfw != 0)
        {
            factoryName = mfw->QueryObject()->GetName();
        }

        // first it look in the meshes and factories of this sector
        if((entity = meshes.Get(meshName, 0)) != 0)
        {
            UpdateEntityValues(ctrl, entity, mesh);
            continue;
        }
        else if(factoryName != 0)
        {
            if((entity = factories.Get(factoryName, 0)) != 0)
            {
                UpdateEntityValues(ctrl, entity, mesh);
                continue;
            }
        }

        // nothing found here: look in the commonSector
        if((entity = commonSector->meshes.Get(meshName, 0)) != 0)
        {
            UpdateEntityValues(ctrl, entity, mesh);
        }
        else if(factoryName != 0)
        {
            if((entity = commonSector->factories.Get(factoryName, 0)) != 0)
            {
                UpdateEntityValues(ctrl, entity, mesh);
            }
        }
    }

    // Cleaning up temporary entities (common sector doesn't have them)
    csArray<psEntity*> tempEnt = tempEntities.GetAll();
    size_t tempEntSize = tempEnt.GetSize();
    
    for(size_t i = 0; i < tempEntSize; i++)
    {
        entity = tempEnt[i];

        if(entity->active == true)
        {
            entity->active = false;
        }
        else
        {
            tempEntities.Delete(entity->GetMeshID(), entity);
            delete entity;
            continue;
        }

        if(!(entity->IsPlaying()))
        {
            entity->ReduceDelay(SoundManager::updateTime);
        }
    }
    
}

void psSoundSector::DeleteEntity(psEntity* &entity)
{
    if(entity->IsTemporary())
    {
        tempEntities.Delete(entity->GetMeshID(), entity);
    }
    else if(entity->meshName.IsEmpty())
    {
        factories.Delete(entity->factoryName, entity);
    }
    else
    {
        meshes.Delete(entity->meshName, entity);
    }

    delete entity;
}

void psSoundSector::SetEntityState(int state, SoundControl* ctrl, iMeshWrapper* mesh, bool forceChange)
{
    psEntity* entity = tempEntities.Get(mesh->QueryObject()->GetID(), 0);

    if(entity == 0)
    {
        return;
    }

    entity->SetState(state, ctrl, mesh->GetMovable()->GetFullPosition(), forceChange);
}

void psSoundSector::Load(csRef<iDocumentNode> sector)
{
    csRef<iDocumentNodeIterator> Itr;
    
    Itr = sector->GetNodes("AMBIENT");
    
    while(Itr->HasNext())
    {
        AddAmbient(Itr->Next());
    }

    Itr = sector->GetNodes("BACKGROUND");

    while(Itr->HasNext())
    {
        AddMusic(Itr->Next());
    }
    
    Itr = sector->GetNodes("EMITTER");
    
    while(Itr->HasNext())
    {
        AddEmitter(Itr->Next());
    }                

    Itr = sector->GetNodes("ENTITY");

    while(Itr->HasNext())
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
    for(size_t i = 0; i< musicarray.GetSize(); i++)
    {
        DeleteMusic(musicarray[i]);
    }
    
    for(size_t i = 0; i< ambientarray.GetSize(); i++)
    {
        DeleteAmbient(ambientarray[i]);
    }

    for(size_t i = 0; i< emitterarray.GetSize(); i++)
    {
        DeleteEmitter(emitterarray[i]);
    }

    // Deleting factories and meshes entities
    csHash<psEntity*, csString>::GlobalIterator entityIter(factories.GetIterator());
    psEntity* entity;

    while(entityIter.HasNext())
    {
        entity = entityIter.Next();
        delete entity;
    }

    entityIter = meshes.GetIterator();
    while(entityIter.HasNext())
    {
        entity = entityIter.Next();
        delete entity;
    }

    factories.DeleteAll();
    meshes.DeleteAll();

    // Deleting temporary entities
    csHash<psEntity*, uint>::GlobalIterator tempEntityIter(tempEntities.GetIterator());

    while(tempEntityIter.HasNext())
    {
        entity = tempEntityIter.Next();
        delete entity;
    }

    tempEntities.DeleteAll();
}

void psSoundSector::UpdateEntityValues(SoundControl* &ctrl, psEntity* entity, iMeshWrapper* mesh)
{
    csVector3 rangeVec;
    float range;

    if(entity->IsPlaying())
    {
        entity->active = true;
        return;
    }

    rangeVec = mesh->GetMovable()->GetFullPosition() - playerposition;
    range = rangeVec.Norm();

    if(active && entity->CheckTimeAndRange(timeofday, range))
    {
        // we need to create the entity even if it won't play anything
        // so that the entity will be ready to play a sound when it change
        // to a state with an higher maxRange for example
        if(!(entity->IsTemporary()))
        {
            entity = new psEntity(entity);
            entity->SetMeshID(mesh->QueryObject()->GetID());
            tempEntities.Put(entity->GetMeshID(), entity);
        }

        // Check if it can play
        if(entity->IsReadyToPlay(timeofday, range)
            && rng.Get() <= entity->GetProbability())
        {
            entity->Play(ctrl, mesh->GetMovable()->GetFullPosition());
        }

        entity->active = true;
    }
}
