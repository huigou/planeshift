/*
 * instrumentmngr.h, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "instrumentmngr.h"

//====================================================================================
// Project Includes
//====================================================================================
#include "util/psxmlparser.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "manager.h"
#include "instrument.h"
#include "songhandle.h"


InstrumentManager::InstrumentManager(iObjectRegistry* objectReg, const char* instrumentsFileName)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root;
    csRef<iDocumentNode> instrRootNode;

    if((doc = ParseFile(objectReg, instrumentsFileName)).IsValid()
        && (root = doc->GetRoot()).IsValid()
        && (instrRootNode = root->GetNode("instruments")).IsValid())
    {
        csRef<iDocumentNode> instrumentNode;
        csRef<iDocumentNodeIterator> instrIter;

        instrIter = instrRootNode->GetNodes("instrument");
        while(instrIter->HasNext())
        {
            uint polyphony;
            const char* name;
            Instrument* instr;
            csRef<iDocumentNode> note;
            csRef<iDocumentNodeIterator> notesIter;

            instrumentNode = instrIter->Next();
            name = instrumentNode->GetAttributeValue("name");
            polyphony = instrumentNode->GetAttributeValueAsInt("polyphony", 1);
            instr = new Instrument(polyphony);

            // adding instrument
            instruments.Put(name, instr);

            instr->volume = instrumentNode->GetAttributeValueAsFloat("volume", VOLUME_NORM);
            instr->minDist = instrumentNode->GetAttributeValueAsFloat("min_dist");
            instr->maxDist = instrumentNode->GetAttributeValueAsFloat("max_dist");
            
            notesIter = instrumentNode->GetNodes("note");
            while(notesIter->HasNext())
            {
                int alter;
                uint octave;
                const char* step;
                const char* resource;

                note = notesIter->Next();
                resource = note->GetAttributeValue("resource");
                step = note->GetAttributeValue("step");
                alter = note->GetAttributeValueAsInt("alter");
                octave = note->GetAttributeValueAsInt("octave");
                instr->AddNote(resource, *step, alter, octave);
            }
        }
    }
}

InstrumentManager::~InstrumentManager()
{
    Instrument* instr;
    csHash<Instrument*, csString>::GlobalIterator instrIter(instruments.GetIterator());

    while(instrIter.HasNext())
    {
        instr = instrIter.Next();
        delete instr;
    }

    instruments.DeleteAll();
}

bool InstrumentManager::PlaySong(SoundControl* sndCtrl, csVector3 pos, csVector3 dir, SoundHandle* &songHandle,
                                 csRef<iDocument> musicalSheet, const char* instrName, float errorRate)
{
    Instrument* instr;

    if(instrName == 0)
    {
        return false;
    }

    instr = instruments.Get(instrName, 0);
    if(instr == 0)
    {
        return false;
    }

    songHandle = new SongHandle(musicalSheet, instr);
    static_cast<SongHandle*>(songHandle)->SetErrorRate(errorRate);

    return SoundSystemManager::GetSingleton().Play3DSound("song", DONT_LOOP, 0, 0, instr->volume,
                     sndCtrl, pos, dir, instr->minDist, instr->maxDist, 0, CS_SND3D_ABSOLUTE, songHandle, false);
}
