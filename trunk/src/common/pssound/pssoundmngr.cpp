/*
 * pssoundmngr.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
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
#include "util/psxmlparser.h"
#include "pssoundmngr.h"
#include "pscelclient.h"
#include "globals.h"

/* FIXME NAMESPACE */
// reminder: paws and effects need that thing (still)
SoundSystemManager *SndSysMgr;

psSoundManager::psSoundManager (iObjectRegistry* objectReg)
{
    activesector = NULL;    ///<
    combat = 0;             ///< combat stance 0 peace 1 fighting 2 dead (TODO refactor me)
    weather = 1;            ///< weather from weather.h 1 is sunshine
    timeofday = 12;         ///< timeofday initial is 12am / 12:00

    SndSysMgr = new SoundSystemManager(objectReg);

    // load sector data
    LoadData (objectReg, sectordata);

    // request all needed soundcontrols
    mainSndCtrl = SndSysMgr->mainSndCtrl;
    ambientSndCtrl = SndSysMgr->GetSoundControl();
    musicSndCtrl = SndSysMgr->GetSoundControl();
    voiceSndCtrl = SndSysMgr->GetSoundControl();
    actionSndCtrl = SndSysMgr->effectSndCtrl;
    guiSndCtrl = SndSysMgr->guiSndCtrl;

    // request a new queue for playing voicefiles
    voicequeue = new SoundQueue(voiceSndCtrl, VOLUME_NORM);

    LastUpdateTime = csGetTicks();

    chatToggle = false;

    //TODO: NEED TO INITIALIZE ALL VARIABLES!
}

psSoundManager::~psSoundManager ()
{
    delete voicequeue;
    delete SndSysMgr;
    // Note: SndSysMgr should take care of SoundControls .. we can ignore them
    for(size_t i = 0; i < sectordata.GetSize(); i++)
        delete sectordata[i];
    sectordata.DeleteAll();
}

/*
 * Updates things that cant be triggered by
 * a event (yet). E.g. Check if theres a Voice to play (TODO: callback)
 * or if there are Emitters or Entities in or out of range that
 * need a update
 *
 * Im not sure if those can ever be event based
 * sure we know that theres something moving
 * but ten to twenty times per second is enough
 * not sure how often it would be called if
 * executed by engine
 * 
 * FIXME csticks
 *
 */

void psSoundManager::Update ()
{
    SndTime = csGetTicks();

    // twenty times per second
    if (activesector != NULL && LastUpdateTime + 50 <= SndTime)
    {
        voicequeue->Work();
        UpdateEmitter(activesector);
        UpdateEntity(activesector);

        LastUpdateTime = csGetTicks();
    }

    // dont forget to Update our SoundSystemManager
    // remember that it has a own throttle
    SndSysMgr->Update();
}

/*
 * Transfer ambient/music Handles from oldsector to newsector if needed
 * if loopBGM is false then the handle might be invalid
 * this is no problem because we dont touch it
 * all we do is copy the address
 */

void psSoundManager::TransferHandles (sctdata* &oldsector, sctdata* &newsector)
{
    for (size_t j = 0; j< newsector->music.GetSize(); j++)
    {
        if (oldsector->activemusic == NULL)
        {
            break;
        }

        if (csStrCaseCmp(newsector->music[j]->resource,
                         oldsector->activemusic->resource) == 0)
        {
            csPrintf("About to steal a music handle\n");
            /* yay active resource with the same name - steal the handle*/
            newsector->music[j]->handle = oldsector->activemusic->handle;
            newsector->activemusic = newsector->music[j];
            /* set the sound to active */
            newsector->music[j]->active = true;
            /* set it to inactive to prevent damage on the handle */
            oldsector->activemusic->active = false;
            /* set it to NULL to avoid problems */
            oldsector->activemusic = NULL;
        }
    }

    for (size_t j = 0; j< activesector->ambient.GetSize(); j++)
    {
        if (oldsector->activeambient == NULL)
        {
            break;
        }

        if (csStrCaseCmp(newsector->ambient[j]->resource,
                         oldsector->activeambient->resource) == 0)
        {
            csPrintf("About to steal a ambient handle\n");
            /* yay active resource with the same name - steal the handle*/
            newsector->ambient[j]->handle = oldsector->activeambient->handle;
            newsector->activeambient = newsector->ambient[j];
            /* set the sound to active */
            newsector->ambient[j]->active = true;
            /* set it to inactive to prevent damage on the handle */
            oldsector->activeambient->active = false;
            /* set it to NULL to avoid problems */
            oldsector->activemusic = NULL;
        }
    }
}




void psSoundManager::ConvertFactoriesToEmitter (sctdata* &sector)
{
    iSector                 *searchSector;
    iMeshFactoryWrapper     *factory;
    iMeshWrapper            *mesh;
    iMeshList               *meshes;
    iEngine                 *engine;
    sct_emitter             *emitter;

    engine = psengine->GetEngine();

    /*
     * convert emitters with factory attributes to real emitters
     * positions are random but are only generated once
     */

    for (size_t j = 0; j< sector->emitter.GetSize(); j++)
    {
        if (!sector->emitter[j]->factory)
        {
            continue;
        }

        if (!(searchSector = engine->FindSector( sector->name, NULL )))
        {
            Error2("sector %s not found\n", (const char *) sector);
            continue;
        }

        if (!(factory = engine->GetMeshFactories()
                        ->FindByName(sector->emitter[j]->factory)))
        {
            Error2("Could not find factory name %s", (const char *) sector->emitter[j]->factory);
            continue;
        }

        meshes = searchSector->GetMeshes();

        for (int k = 0; k < meshes->GetCount(); k++)
        {
            mesh = meshes->Get(k);

            if (mesh->GetFactory() == factory)
            {
                if (rng.Get() <= sector->emitter[j]->factory_prob)
                {
                    emitter = new sct_emitter;

                    emitter->resource = csString(sector->emitter[j]->resource);
                    emitter->minvol   = sector->emitter[j]->minvol;
                    emitter->maxvol   = sector->emitter[j]->maxvol;
                    emitter->maxrange = sector->emitter[j]->maxrange;
                    emitter->position = mesh->GetMovable()->GetPosition();
                    emitter->active   = false;

                    sector->emitter.Push(emitter);
                }
            }
        }

        /* delete the factory node */
        sector->emitter.Delete(sector->emitter[j]);
        j--;

    }
}


/*
 * LoadSector is called when when the player enters a sector (modehandler.cpp)
 *
 * It loads the new sector and unloads the old one
 * FACTORY Emitters are converted to real EMITTERS (if not already done)
 * \ this is only done ONCE per sector
 *
 * it also moves background and ambient music into the new sector
 * if they are not changing and are already playing
 *
 * Old sector will be cleaned at the end of this function
 * unused music and sounds will be unloaded
 *
 * NOTE: has nothing todo with updates on OpenAL .. those are done by CS
 *
 */


void psSoundManager::Load (const char* sector)
{
    sctdata        *oldsector;

    /* TODO wrong way todo this */
    if (SndSysMgr->Initialised == false)
    {
        return;
    }

    /*
     * i dont want this function called if were doing nasty stuff like that
     * FIXME
     */

    if (strcmp(sector, "SectorWhereWeKeepEntitiesResidingInUnloadedMaps") == 0)
    {
        return;
    }

    /*
     * set the wanted sector to our active sector (if found)
     */

    for (size_t i = 0; i< sectordata.GetSize(); i++)
    {
        if (strcmp(sector, sectordata[i]->name) != 0)
        {
            continue;
        }

        /* FIXME - hack for #4268 */
        if (activesector != NULL)
        {
            if (csStrCaseCmp(activesector->name, sector) == 0)
            {
                return;
            }
        }

        oldsector = activesector;
        activesector = sectordata[i];
        activesector->active = true;

        /* works only on loaded sectors! */
        ConvertFactoriesToEmitter(activesector);

        if (oldsector != NULL)
        {
            /*
             * hijack active ambient and music if they are played in the new sector
             * in the old sector there MUST be only ONE of each
             */

            TransferHandles(oldsector, activesector);

            /* set old sector to inactive and make a update (will stop all sounds) */
            oldsector->active = false;
            UpdateSector(oldsector);

        }

        /*
         * Call Update start sound in the new sector
         */

        UpdateSector(activesector);
        return;
     }
}

/*
 * Update everything in the given sector
 */

void psSoundManager::UpdateSector (sctdata* &sector)
{
    csPrintf("UpdateSector\n");
    UpdateEmitter(sector);
    UpdateAmbient(sector);
    UpdateMusic(sector);
    UpdateEntity(sector);
}

void psSoundManager::UpdateAmbient (sctdata* &sector)
{
    sct_ambient *ambient;

    if (sector == NULL)
    {
        return;
    }

    csPrintf("update ambient\n");

    /*
     *  Set Ambient Music and weather
     *  i pretend that xml-files are always correct
     *  there should be only ONE ambient playing
     *
     * ALL ambient and music sounds start at minvol and are faded in
     */

    for (size_t i = 0; i< sector->ambient.GetSize(); i++)
    {
        ambient = sector->ambient[i];

        if (ambient->weather == weather
            && sector->active == true
            && GetAmbientToggle() == true
            && (ambient->timeofday <= timeofday
            && ambient->timeofdayrange >= timeofday))
        {
            if (ambient->active == true)
            {
                continue;
            }

            csPrintf("adding a %s\n",(const char *) ambient->resource);

            ambient->active = true;

            if (SndSysMgr->Play2DSound (ambient->resource, LOOP,
                                        ambient->loopstart, ambient->loopend,
                                        ambient->minvol, ambientSndCtrl,
                                        ambient->handle))
            {
    
                ambient->handle->Fade((ambient->maxvol - ambient->minvol),
                                       ambient->fadedelay, FADE_UP);
    
                ambient->handle->preset_volume = ambient->maxvol;
                sector->activeambient = ambient;
            }
            else // error occured .. get rid of this ambient
            {
                sector->ambient.Delete(ambient);
                delete ambient;
                break;
            }

        }
        else if (ambient->active == true)
        {
            csPrintf("removing a %s\n",(const char *) ambient->resource);
            ambient->active = false;
            ambient->handle->Fade(ambient->maxvol, ambient->fadedelay, FADE_STOP);
            if (sector->activeambient == ambient)
            {
                sector->activeambient = NULL;
            }
        }
    }
}

void psSoundManager::UpdateMusic (sctdata* &sector)
{
    sct_music *music;

    if (sector == NULL)
    {
        return;
    }

    csPrintf("update music\n");

    /*
     * set music based on combat mode
     * set combatmode to 0 if toggle is off
     */

    if (GetCombatToggle() == false)
    {
        combat = 0;
    }

    for (size_t i = 0; i< sector->music.GetSize(); i++)
    {
        music = sector->music[i];

              //csPrintf("%i %i %i\n", music->timeofday, music->timeofdayrange, timeofday);

        if (music->type == combat
            && sector->active == true
            && GetMusicToggle() == true
            && (music->timeofday <= timeofday
            && music->timeofdayrange >= timeofday))
        {
            if (music->active == true)
            {
                /*
                 * difficult logic. user doesnt want looping BGM
                 * therefor we keep the handle and leave it active
                 * so it wont be played again
                 * music continues if he enables loopBGM
                 */
                if (loopBGM == false)
                {
                    /* set autoremove to false we want to keep the handle */
                    music->handle->SetAutoRemove(false);
                    music->handle->sndstream->SetLoopState(DONT_LOOP);
                    continue;
                }
                else
                {
                    /* enable looping because the toggle is true */
                    music->handle->sndstream->SetLoopState(LOOP);
                    /* Unpause the stream because it might be paused */
                    music->handle->sndstream->Unpause();
                    /* set autoremove to true so it gets removed if someone pauses it */
                    music->handle->SetAutoRemove(true);
                    continue;
                }
            }

            csPrintf("adding b %s\n", (const char *) music->resource);
            music->active = true;

            if (SndSysMgr->Play2DSound (music->resource, loopBGM,
                                        music->loopstart, music->loopend,
                                        music->minvol, musicSndCtrl,
                                        music->handle))
            {
                music->handle->Fade((music->maxvol - music->minvol),
                                     music->fadedelay, FADE_UP);
    
                music->handle->preset_volume = music->maxvol;
                sector->activemusic = music;
            }
            else // error occured .. get rid of this music
            {
                sector->music.Delete(music);
                delete music;
                break;
            }
        }
        else if (music->active == true)
        {
            csPrintf("removing b %s\n", (const char *) music->resource);

            /*
             * remove a handle which have been paused because of loopBGM
             * that means user is going to hear the BGM again if he enters
             * a new sector and the previous sector didnt play this one
             * or reenables music, leaves combatmode, if timeofday changes
             * very complex
             */
            if (loopBGM == false)
            {
                /*
                 * handle is there but maybe paused or whatever
                 * get rid of it
                 */
                music->handle->SetAutoRemove(true);
            }
            music->active = false;
            music->handle->Fade(music->maxvol, music->fadedelay, FADE_STOP);
            if (sector->activemusic == music)
            {
                sector->activemusic = NULL;
            }
        }
    }
}

/*
update on position change
*/


void psSoundManager::UpdateEmitter (sctdata* &sector)
{
    sct_emitter *emitter;
    csVector3 rangeVec;
    float range;

    if (sector == NULL)
    {
        return;
    }

    /*
     * start/stop all emitters in range
     */

    for (size_t i = 0; i< sector->emitter.GetSize(); i++)
    {
        emitter = sector->emitter[i];
        rangeVec = emitter->position - playerposition;
        range = rangeVec.Norm();

        if (!range) /* if range is NAN */
        {
            break;
        }

        if (range <= emitter->maxrange
            && sector->active == true
            && GetAmbientToggle() == true
            && (emitter->timeofday <= timeofday
            && emitter->timeofdayrange >= timeofday))
        {
            if (emitter->active == true)
            {
                continue;
            }

            csPrintf("adding e %s\n", (const char *) emitter->resource);
            emitter->active = true;

            if (!SndSysMgr->Play3DSound (emitter->resource, LOOP, 0, 0,
                                         emitter->maxvol, ambientSndCtrl,
                                         emitter->position, emitter->direction,
                                         emitter->minrange, emitter->maxrange,
                                         VOLUME_ZERO, CS_SND3D_ABSOLUTE,
                                         emitter->handle))
            {
                // error occured .. emitter cant be played .. remove it
                sector->emitter.Delete(emitter);
                delete emitter;
                break;
            }

        }
        else if (emitter->active != false)
        {
            csPrintf("removing e %s\n", (const char *) sector->emitter[i]->resource);
            emitter->active = false;
            emitter->handle->sndstream->Pause();
        }
    }
}

/*
 * this is the most powerfull part of the new soundmanager
 * it can add sounds to any kind of entity
 * e.g. items / actors (NPC/PC) and effects
 *
 * i hope it capable to replace PlayEffect / StopEffect
 *
 * tbh this function doesnt belong here
 * the whole logic whould be in pscelclient.cpp
 *
 * not implementet yet:
 *   get real labals e.g. "Tefusangling"
 *   take animation into account (IDLE, PEACE ...)
 *   update position
 */

void psSoundManager::UpdateEntity (sctdata* &sector)
{
    sct_entity *entity;
    iMeshWrapper* mesh;
    csVector3 rangeVec;
    float range;

    const csPDelArray<GEMClientObject>& entities = psengine->GetCelClient()
                                                   ->GetEntities();

    if (sector == NULL)
    {
        return;
    }

    for (size_t i = 0; i < sector->entity.GetSize(); i++)
    {
        entity = sector->entity[i];

        if (entity->active == true)
        {
            if (entity->when <= 0)
            {
                /* SndSysMgr will pick the dead sound up */
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

            rangeVec = entities[a]->GetPosition() - playerposition;
            range = rangeVec.Norm();

            if (range <= entity->maxrange
                && csStrCaseCmp(entity->name, mesh->QueryObject()->GetName()) == 0
                && rng.Get() <= entity->probability
                && (entity->timeofday <= timeofday
                && entity->timeofdayrange >= timeofday))
            {
                csPrintf("iobject name %s %f %f\n", mesh->QueryObject()->GetName(), range, entity->maxrange);
                /* play sound */
                entity->active = true;
                entity->when = (entity->delay_after*1000);
                
                if (!SndSysMgr->Play3DSound (entity->resource, DONT_LOOP, 0, 0,
                                             entity->maxvol, ambientSndCtrl,
                                             entities[a]->GetPosition(), 0,
                                             entity->minrange, entity->maxrange,
                                             VOLUME_ZERO, CS_SND3D_ABSOLUTE,
                                             entity->handle))
                {
                    sector->entity.Delete(entity);
                    delete entity;
                    break;
                }
            }
        }
    }
}

/*
 * were always using camera height and rotation
 * position can be players position or cameras position
 * updated every frame? ..
 */

void psSoundManager::UpdateListener ( iView* view )
{
    /* TODO wrong way todo this */
    if (SndSysMgr->Initialised == false)
    {
        return;
    }

    // take position/direction from view->GetCamera ()
    csVector3 v = view->GetPerspectiveCamera()->GetCamera()
                      ->GetTransform().GetOrigin();
    csMatrix3 m = view->GetPerspectiveCamera()->GetCamera()
                      ->GetTransform().GetT2O();
    csVector3 f = m.Col3();
    csVector3 t = m.Col2();

    if (listenerOnCamera == false)
    {
        v = playerposition;
    }

    SndSysMgr->UpdateListener (v, f, t);
}

void psSoundManager::PlayActionSound (const char *name)
{
    SoundHandle *Handle;
    SndSysMgr->Play2DSound(name, DONT_LOOP, 0, 0, VOLUME_NORM,
                           actionSndCtrl, Handle);
}

void psSoundManager::PlayGUISound (const char *name)
{
    SoundHandle *Handle;
    SndSysMgr->Play2DSound(name, DONT_LOOP, 0, 0, VOLUME_NORM,
                           guiSndCtrl, Handle);
}


/*
 * load a all sector xmls into an given array to make them usable
 * its only parsing the xml most of it is hardcoded
 *
 * Get*Node functions are utilized.
 * i dont like how this works but its only called once
 * and easy to understand and maintain
 *
 * still i consider replacing it with something more generic
 */

bool psSoundManager::LoadData (iObjectRegistry* objectReg, csArray<sctdata*> &sectordata)
{
    csRef<iDataBuffer>        xpath;
    const char*                        dir;
    csRef<iStringArray> files;
    sctdata                                *tmpsectordata;

    csRef<iDocumentNodeIterator> sectorIter;

    if (!(vfs =  csQueryRegistry<iVFS> (objectReg)))
    {
        return false;
    }

    xpath = vfs->ExpandPath("/planeshift/soundlib/areas/");
    dir = **xpath;
    files = vfs->FindFiles(dir);

    if (!files)
    {
        return false;
    }

    for (size_t i=0; i < files->GetSize(); i++)
    {
        csString name( files->Get(i) );
        csRef<iDocument> doc;
        csRef<iDocumentNode> root, mapNode;

        if ((doc=ParseFile( objectReg, name ))
            && (root=doc->GetRoot())
            && (mapNode=root->GetNode("MAP_SOUNDS")))
        {
            sectorIter = mapNode->GetNodes("SECTOR");

            while ( sectorIter->HasNext() )
            {
                tmpsectordata = new sctdata;

                csRef<iDocumentNode> sector = sectorIter->Next();
                tmpsectordata->name = sector->GetAttributeValue("NAME");

                GetAmbientNodes(tmpsectordata->ambient, sector->GetNodes("AMBIENT"));
                GetEmitterNodes(tmpsectordata->emitter, sector->GetNodes("EMITTER"));
                GetMusicNodes(tmpsectordata->music, sector->GetNodes("BACKGROUND"));
                GetEntityNodes(tmpsectordata->entity, sector->GetNodes("ENTITY"));

                tmpsectordata->activeambient = NULL;
                tmpsectordata->activemusic = NULL;

                sectordata.Push(tmpsectordata);
            }
        }
    }

    return true;
}

void psSoundManager::GetAmbientNodes (csArray<sct_ambient*> &ambient_sounds,
                 csRef<iDocumentNodeIterator> Itr)
{
    csRef<iDocumentNode>    Node;
    sct_ambient            *ambient;

    while ( Itr->HasNext() )
    {
        Node = Itr->Next();

        ambient = new sct_ambient;

        ambient->resource       = Node->GetAttributeValue("RESOURCE");
        ambient->minvol         = Node->GetAttributeValueAsFloat("MINVOL");
        ambient->maxvol         = Node->GetAttributeValueAsFloat("MAXVOL");
        ambient->fadedelay      = Node->GetAttributeValueAsInt("FADEDELAY");
        ambient->timeofday      = Node->GetAttributeValueAsInt("TIME");
        ambient->timeofdayrange = Node->GetAttributeValueAsInt("TIME_RANGE");
        ambient->weather        = Node->GetAttributeValueAsInt("WEATHER");
        ambient->trigger        = Node->GetAttributeValue("TRIGGER");
        ambient->loopstart      = Node->GetAttributeValueAsInt("LOOPSTART");
        ambient->loopend        = Node->GetAttributeValueAsInt("LOOPEND");
        ambient->timeofday      = Node->GetAttributeValueAsInt("TIME");
        ambient->timeofdayrange = Node->GetAttributeValueAsInt("TIME_RANGE");
        ambient->active         = false;

        if (ambient->timeofday == -1)
        {
            ambient->timeofday = 0;
            ambient->timeofdayrange = 24;
        }

        ambient_sounds.Push(ambient);
    }
}



void psSoundManager::GetEmitterNodes(csArray<sct_emitter*> &emitter_sounds,
                csRef<iDocumentNodeIterator> Itr)
{
    csRef<iDocumentNode>    Node;
    sct_emitter            *emitter;

    while ( Itr->HasNext() )
    {
        Node = Itr->Next();

        emitter = new sct_emitter;

        emitter->resource       = Node->GetAttributeValue("RESOURCE");
        emitter->minvol         = Node->GetAttributeValueAsFloat("MINVOL");
        emitter->maxvol         = Node->GetAttributeValueAsFloat("MAXVOL");
        emitter->maxrange       = Node->GetAttributeValueAsFloat("MAX_RANGE");
        emitter->minrange       = Node->GetAttributeValueAsFloat("MIN_RANGE");
        emitter->fadedelay      = Node->GetAttributeValueAsInt("FADEDELAY");
        emitter->mesh           = Node->GetAttributeValue("MESH");
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

        emitter_sounds.Push(emitter);
    }
}

void psSoundManager::GetMusicNodes (csArray<sct_music*> &music_sounds, csRef<iDocumentNodeIterator> Itr)
{
    csRef<iDocumentNode>    Node;
    sct_music              *music;

    while ( Itr->HasNext() )
    {
        Node = Itr->Next();

        music = new sct_music;

        music->resource         = Node->GetAttributeValue("RESOURCE");
        music->type             = Node->GetAttributeValueAsInt("TYPE");
        music->minvol           = Node->GetAttributeValueAsFloat("MINVOL");
        music->maxvol           = Node->GetAttributeValueAsFloat("MAXVOL");
        music->fadedelay        = Node->GetAttributeValueAsInt("FADEDELAY");
        music->timeofday        = Node->GetAttributeValueAsInt("TIME");
        music->timeofdayrange   = Node->GetAttributeValueAsInt("TIME_RANGE");
        music->weather          = Node->GetAttributeValueAsInt("WEATHER");
        music->loopstart        = Node->GetAttributeValueAsInt("LOOPSTART");
        music->loopend          = Node->GetAttributeValueAsInt("LOOPEND");
        music->active           = false;

        if (music->timeofday == -1)
        {
            music->timeofday = 0;
            music->timeofdayrange = 24;
        }

        music_sounds.Push(music);
    }
}

void psSoundManager::GetEntityNodes (csArray<sct_entity*> &entity_sounds, csRef<iDocumentNodeIterator> Itr)
{
    csRef<iDocumentNode>    Node;
    sct_entity             *entity;

    while ( Itr->HasNext() )
    {
        Node = Itr->Next();

        entity = new sct_entity;

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

        entity_sounds.Push(entity);
    }
}

/*
 * Set functions which trigger updates
 */

void psSoundManager::SetTimeOfDay (int newTimeofday)
{
    csPrintf("settimeofday\n");
    timeofday = newTimeofday;
    UpdateAmbient(activesector);
    UpdateMusic(activesector);
}

/*
 * Set/Get functions for toggles which trigger updates
 */

/*
 * Engine calls SetWeather every frame or so
 * update is only called if weather is changing
 */

void psSoundManager::SetWeather (int newWeather)
{
    if (weather != newWeather)
    {
        csPrintf("setweather\n");
        weather = newWeather;
        UpdateAmbient(activesector);
    }
}

void psSoundManager::SetLoopBGMToggle (bool toggle)
{
    loopBGM = toggle;
    UpdateMusic(activesector);
}

bool psSoundManager::GetLoopBGMToggle ()
{
    return loopBGM;
}

void psSoundManager::SetCombatToggle (bool toggle)
{
    combatMusic = toggle;
    UpdateMusic(activesector);
}

bool psSoundManager::GetCombatToggle ()
{
    return combatMusic;
}

void psSoundManager::SetCombatStance (int newCombatstance)
{
    combat = newCombatstance;
    UpdateMusic(activesector);
}

/*
 * FIXME Remove GetCombatStance when you fix the victory effect
 */

int psSoundManager::GetCombatStance ()
{
    return combat;
}

void psSoundManager::SetMusicToggle (bool toggle)
{
    musicSndCtrl->SetToggle(toggle);
    UpdateMusic(activesector);
}

bool psSoundManager::GetMusicToggle ()
{
    return musicSndCtrl->GetToggle();
}

void psSoundManager::SetAmbientToggle (bool toggle)
{
    ambientSndCtrl->SetToggle(toggle);
    UpdateAmbient(activesector);
    /* to be consistent (not really needed) */
    UpdateEmitter(activesector);
}

bool psSoundManager::GetAmbientToggle ()
{
    return ambientSndCtrl->GetToggle();
}

void psSoundManager::SetChatToggle (bool toggle)
{
    chatToggle = toggle;
}

bool psSoundManager::GetChatToggle ()
{
    return chatToggle;
}

void psSoundManager::SetVoiceToggle (bool toggle)
{
    voiceSndCtrl->SetToggle(toggle);

    if (toggle == false)
    {
        voicequeue->Purge();
    }
}


bool psSoundManager::GetVoiceToggle ()
{
    return voiceSndCtrl->GetToggle();
}

void psSoundManager::SetListenerOnCameraPos (bool toggle)
{
    listenerOnCamera = toggle;
}

bool psSoundManager::GetListenerOnCameraPos ()
{
    return listenerOnCamera;
}
