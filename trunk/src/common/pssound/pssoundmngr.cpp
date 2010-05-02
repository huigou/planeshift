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

#include "sound/sound.h"
#include "pssound.h"
#include "pssoundmngr.h"
#include "pscelclient.h"
#include "globals.h"
#include "util/psxmlparser.h"

/* FIXME NAMESPACE */
// reminder: paws and effects need that thing (still)
SoundSystemManager *SndSysMgr;

psSoundManager::psSoundManager (iObjectRegistry* objectReg)
{
    activesector = NULL;    ///<
    combat = 0;             ///< combat stance 0 peace 1 fighting 2 dead (TODO refactor me)
    weather = 1;            ///< weather from weather.h 1 is sunshine

    SndSysMgr = new SoundSystemManager(objectReg);

    // load sector data
    LoadSectors ();

    // request all needed soundcontrols
    mainSndCtrl = SndSysMgr->mainSndCtrl;

    ambientSndCtrl = SndSysMgr->GetSoundControl();
    ambientSndCtrl->SetCallback(this, &UpdateAmbientCallback);
    
    musicSndCtrl = SndSysMgr->GetSoundControl();
    musicSndCtrl->SetCallback(this, &UpdateMusicCallback);
    
    voiceSndCtrl = SndSysMgr->GetSoundControl();
    actionSndCtrl = SndSysMgr->effectSndCtrl;
    guiSndCtrl = SndSysMgr->guiSndCtrl;

    // request a new queue for playing voicefiles
    voicequeue = new SoundQueue(voiceSndCtrl, VOLUME_NORM);
    
    loopBGM.SetCallback(this, &UpdateMusicCallback);
    combatMusic.SetCallback(this, &UpdateMusicCallback);

    

    LastUpdateTime = csGetTicks();

    //TODO: NEED TO INITIALIZE ALL VARIABLES!
}

psSoundManager::~psSoundManager ()
{
    UnloadSectors();
    delete voicequeue;
    delete SndSysMgr;
    // Note: SndSysMgr should take care of SoundControls .. we can ignore them
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
        activesector->UpdateEmitter(ambientSndCtrl);
        activesector->UpdateEntity(ambientSndCtrl);

        LastUpdateTime = csGetTicks();
    }

    // dont forget to Update our SoundSystemManager
    // remember that it has a own throttle
    SndSysMgr->Update();
}

void psSoundManager::UpdateMusicCallback(void* object)
{
    psSoundManager *which = (psSoundManager *) object;
    if (which->activesector != NULL)
    {
        which->activesector->UpdateMusic(which->loopBGM.GetToggle(), which->combat, which->musicSndCtrl);
    }
}

void psSoundManager::UpdateAmbientCallback(void* object)
{
    psSoundManager *which = (psSoundManager *) object;
    if (which->activesector != NULL)
    {
        which->activesector->UpdateAmbient(which->weather, which->ambientSndCtrl);
    }
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
 * Transfer ambient/music Handles from oldsector to newsector if needed
 * if loopBGM is false then the handle might be invalid
 * this is no problem because we dont touch it
 * all we do is copy the address
 */

void psSoundManager::TransferHandles (psSoundSector* &oldsector,
                                      psSoundSector* &newsector)
{
    for (size_t j = 0; j< newsector->musicarray.GetSize(); j++)
    {
        if (oldsector->activemusic == NULL)
        {
            break;
        }

        if (csStrCaseCmp(newsector->musicarray[j]->resource,
                         oldsector->activemusic->resource) == 0)
        {
            /* yay active resource with the same name - steal the handle*/
            newsector->musicarray[j]->handle = oldsector->activemusic->handle;
            /* set handle to NULL */
            oldsector->activemusic->handle = NULL;
            newsector->activemusic = newsector->musicarray[j];
            /* set the sound to active */
            newsector->musicarray[j]->active = true;
            /* set it to inactive to prevent damage on the handle */
            oldsector->activemusic->active = false;
            /* set it to NULL to avoid problems */
            oldsector->activemusic = NULL;
            /* update callback */
            newsector->activemusic->UpdateHandleCallback();
        }
    }

    for (size_t j = 0; j< activesector->ambientarray.GetSize(); j++)
    {
        if (oldsector->activeambient == NULL)
        {
            break;
        }

        if (csStrCaseCmp(newsector->ambientarray[j]->resource,
                         oldsector->activeambient->resource) == 0)
        {
            /* yay active resource with the same name - steal the handle*/
            newsector->ambientarray[j]->handle = oldsector->activeambient->handle;
            /* set handle to NULL */
            oldsector->activeambient->handle = NULL;
            newsector->activeambient = newsector->ambientarray[j];
            /* set the sound to active */
            newsector->ambientarray[j]->active = true;
            /* set it to inactive to prevent damage on the handle */
            oldsector->activeambient->active = false;
            /* set it to NULL to avoid problems */
            oldsector->activeambient = NULL;
            /* update callback */
            newsector->activeambient->UpdateHandleCallback();
        }
    }
}




void psSoundManager::ConvertFactoriesToEmitter (psSoundSector* &sector)
{
    iSector                 *searchSector;
    iMeshFactoryWrapper     *factory;
    iMeshWrapper            *mesh;
    iMeshList               *meshes;
    iEngine                 *engine;
    psEmitter               *emitter;
    psEmitter               *factoryemitter;

    engine = psengine->GetEngine();

    /*
     * convert emitters with factory attributes to real emitters
     * positions are random but are only generated once
     */

    for (size_t j = 0; j< sector->emitterarray.GetSize(); j++)
    {
        if (!sector->emitterarray[j]->factory)
        {
            continue;
        }

        factoryemitter = sector->emitterarray[j];
                
        if (!(searchSector = engine->FindSector( sector->name, NULL )))
        {
            Error2("sector %s not found\n", (const char *) sector);
            continue;
        }

        if (!(factory = engine->GetMeshFactories()
                        ->FindByName(factoryemitter->factory)))
        {
            Error2("Could not find factory name %s", (const char *) factoryemitter->factory);
            continue;
        }

        meshes = searchSector->GetMeshes();

        for (int k = 0; k < meshes->GetCount(); k++)
        {
            mesh = meshes->Get(k);

            if (mesh->GetFactory() == factory)
            {
                if (rng.Get() <= factoryemitter->factory_prob)
                {
                    emitter = new psEmitter;

                    emitter->resource = csString(factoryemitter->resource);
                    emitter->minvol   = factoryemitter->minvol;
                    emitter->maxvol   = factoryemitter->maxvol;
                    emitter->maxrange = factoryemitter->maxrange;
                    emitter->position = mesh->GetMovable()->GetPosition();
                    emitter->active   = false;

                    sector->emitterarray.Push(emitter);
                }
            }
        }
        /* delete the factory node */
        sector->DeleteEmitter(factoryemitter);
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
    psSoundSector        *oldsector;
    psSoundSector        *newsector;

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

    if (FindSector(sector, newsector) == true)
    {
        /* FIXME - hack for #4268 */
        if (activesector != NULL)
        {
            if (csStrCaseCmp(activesector->name, sector) == 0)
            {
                return;
            }
        }

        oldsector = activesector;
        activesector = newsector;
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

void psSoundManager::Reload()
{
    csString currentsector = csString(activesector->name);
    Unload();
    ReloadSectors();
    Load(currentsector);
}

void psSoundManager::Unload()
{
    activesector->active = false;
    UpdateSector(activesector);
    activesector = NULL;
}

bool psSoundManager::FindSector (const char *name, psSoundSector* &sector)
{
    for (size_t i = 0; i< sectordata.GetSize(); i++)
    {
        if (strcmp(name, sectordata[i]->name) != 0)
        {
            continue;
        }
        else
        {
            sector = sectordata[i];
            return true;
        }
    }
    
    sector = NULL;
    return false;
}

/*
 * Update everything in the given sector
 */

void psSoundManager::UpdateSector (psSoundSector* &sector)
{
    sector->UpdateMusic(loopBGM.GetToggle(), combat, musicSndCtrl);
    sector->UpdateAmbient(weather, ambientSndCtrl);
    sector->UpdateEmitter(ambientSndCtrl);
    sector->UpdateEntity(ambientSndCtrl);
}


/*
 * were always using camera height and rotation
 * position can be players position or cameras position
 * updated every frame? ..
 */

void psSoundManager::UpdateListener ( iView* view )
{
    csVector3 hearpoint;
    csMatrix3 matrix;
    csVector3 front;
    csVector3 top;

    /* TODO wrong way todo this */
    if (SndSysMgr->Initialised == false)
    {
        return;
    }

    if (listenerOnCamera.GetToggle() == false)
    {
        hearpoint = playerposition;
    }
    else
    {
        // take position/direction from view->GetCamera ()
        hearpoint = view->GetPerspectiveCamera()->GetCamera()
                        ->GetTransform().GetOrigin();
    }

    matrix = view->GetPerspectiveCamera()->GetCamera()
                  ->GetTransform().GetT2O();
    front = matrix.Col3();
    top   = matrix.Col2();

    SndSysMgr->UpdateListener (hearpoint, front, top);
}


/*
 * Set functions which trigger updates
 */

void psSoundManager::SetTimeOfDay (int newTimeofday)
{
    if (activesector != NULL)
    {
        activesector->timeofday = newTimeofday;
        activesector->UpdateAmbient(weather, ambientSndCtrl);
        activesector->UpdateMusic(loopBGM.GetToggle(), combat, musicSndCtrl);
    }
}

void psSoundManager::SetPosition (csVector3 playerpos)
{
    playerposition = playerpos;
    
    if (activesector != NULL)
    {
        activesector->playerposition = playerpos;
    }
}

/*
 * Engine calls SetWeather every frame or so
 * update is only called if weather is changing
 */

void psSoundManager::SetWeather (int newWeather)
{
    if (weather != newWeather && activesector)
    {
        weather = newWeather;
        activesector->UpdateAmbient(weather, ambientSndCtrl);
    }
}

void psSoundManager::SetCombatStance (int newCombatstance)
{
    if (combatMusic.GetToggle() == true)
    {
        combat = newCombatstance;
    }
    else
    {
        combat = 0; /* 0 is peace */
    }
    
    if(activesector)
        activesector->UpdateMusic(loopBGM.GetToggle(), combat, musicSndCtrl);
}

/*
 * FIXME Remove GetCombatStance when you fix the victory effect
 */

int psSoundManager::GetCombatStance ()
{
    return combat;
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

/*
 * load a all sector xmls and make them usable
 * will overwrite the target sector if it exists 
 * its only parsing the xml most of it is hardcoded
 *
 */

bool psSoundManager::LoadSectors ()
{
    csRef<iDataBuffer>      xpath;
    const char*             dir;
    csRef<iStringArray>     files;
    psSoundSector          *tmpsector;
    csRef<iVFS>             vfs;

    csRef<iDocumentNodeIterator> sectorIter;
    
    if (!(vfs = csQueryRegistry<iVFS> (psengine->GetObjectRegistry())))
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
        csString name(files->Get(i));
        csRef<iDocument> doc;
        csRef<iDocumentNode> root, mapNode;

        if ((doc=ParseFile(psengine->GetObjectRegistry(), name))
            && (root=doc->GetRoot())
            && (mapNode=root->GetNode("MAP_SOUNDS")))
        {
            sectorIter = mapNode->GetNodes("SECTOR");

            while (sectorIter->HasNext())
            {
                csRef<iDocumentNode> sector = sectorIter->Next();

                if (FindSector (sector->GetAttributeValue("NAME"), tmpsector) == true)
                {
                    tmpsector->Reload(sector);
                }
                else
                {
                    tmpsector = new psSoundSector(sector);
                    sectordata.Push(tmpsector);
                }
            }
        }
    }

    return true;
}

void psSoundManager::ReloadSectors ()
{
    UnloadSectors();
    LoadSectors();
}

void psSoundManager::UnloadSectors ()
{
    for (size_t i = 0; i < sectordata.GetSize(); i++)
    {
        delete sectordata[i];
    }

    sectordata.DeleteAll();
}

