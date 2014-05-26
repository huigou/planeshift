/*
 * scoreelements.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include "scoreelements.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================

//====================================================================================
// Local Includes
//====================================================================================

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
using namespace psMusic;

// 10 octaves are more than enough and each octave contains maximum
// 7 notes so there is really no need of a bigger hash table
#define PREV_ACCIDENTALS_NAME_SIZE 7
#define PREV_ACCIDENTALS_OCTAVE_SIZE 5

//--------------------------------------------------

Note::NoteContext::NoteContext()
: prevAccidentals(PREV_ACCIDENTALS_OCTAVE_SIZE)
{
}

Accidental Note::NoteContext::GetPreviousAccidental(const Note &note) const
{
    Accidental prevAccidental = NO_ACCIDENTAL;
    const csHash<Accidental, char>* nameHash =
        prevAccidentals.GetElementPointer(note.octave);

    if(nameHash != 0)
    {
        prevAccidental = nameHash->Get(note.name, NO_ACCIDENTAL);
    }
    return prevAccidental;
}

void Note::NoteContext::ResetContext()
{
    // we delete only the internal hash elements so that we don't have to reallocate
    // the memory later, octaves are more or less the same in the whole piece anyway
    csHash<Accidental, char> *nameHash;
    csHash<csHash<Accidental, char>, int>::GlobalIterator
        nameIter(prevAccidentals.GetIterator());

    while(nameIter.HasNext())
    {
        nameHash = &nameIter.Next();
        nameHash->DeleteAll();
    }
}

void Note::NoteContext::UpdateContext(const Note &note)
{
    if(note.writtenAccidental != NO_ACCIDENTAL)
    {
        csHash<Accidental, char>* nameHash =
            prevAccidentals.GetElementPointer(note.octave);

        if(nameHash == 0)
        {
            nameHash = &prevAccidentals.Put(note.octave,
                csHash<Accidental, char>(PREV_ACCIDENTALS_NAME_SIZE));
            nameHash->Put(note.name, note.writtenAccidental);
        }
        else
        {
            nameHash->PutUnique(note.name, note.writtenAccidental);
        }
    }
}

void Note::NoteContext::UpdateContext(const MeasureElement &element)
{
    for(size_t i = 0; i < element.GetNNotes(); i++)
    {
        UpdateContext(element.GetNote(i));
    }
}

Note::Note(): name('C'), octave(4), writtenAccidental(NO_ACCIDENTAL)
{
}

Note::Note(char name_, int octave_, Accidental writtenAccidental_)
: writtenAccidental(writtenAccidental_)
{
    SetName(name_);
    SetOctave(octave_);
}

Accidental Note::GetPlayedAccidental(const ScoreContext &context) const
{
    Accidental prevAccidental;
    int fifths = context.measureAttributes.GetFifths(); // convenience variable

    CS_ASSERT(fifths >= -7 && fifths <= 7);

    // accidentals written for this note have priority
    if(writtenAccidental != NO_ACCIDENTAL)
    {
        return writtenAccidental;
    }

    // then accidentals written on previous note with same name and octave
    prevAccidental = context.noteContext.GetPreviousAccidental(*this);
    if(prevAccidental != NO_ACCIDENTAL)
    {
        return prevAccidental;
    }

    // finally the tonality decide
    switch(name)
    {
    case 'A':
        if(fifths <= -3)
        {
            return FLAT;
        }
        else if(fifths >= 5)
        {
            return SHARP;
        }
        break;
    case 'B':
        if(fifths <= -1)
        {
            return FLAT;
        }
        else if(fifths >= 7)
        {
            return SHARP;
        }
        break;
    case 'C':
        if(fifths <= -6)
        {
            return FLAT;
        }
        else if(fifths >= 2)
        {
            return SHARP;
        }
        break;
    case 'D':
        if(fifths <= -4)
        {
            return FLAT;
        }
        else if(fifths >= 4)
        {
            return SHARP;
        }
        break;
    case 'E':
        if(fifths <= -2)
        {
            return FLAT;
        }
        else if(fifths >= 6)
        {
            return SHARP;
        }
        break;
    case 'F':
        if(fifths <= -7)
        {
            return FLAT;
        }
        else if(fifths >= 1)
        {
            return SHARP;
        }
        break;
    case 'G':
        if(fifths <= -5)
        {
            return FLAT;
        }
        else if(fifths >= 3)
        {
            return SHARP;
        }
        break;
    }

    return NO_ACCIDENTAL;
}

bool Note::operator==(const Note &note) const
{
    return this->name == note.name && this->octave == note.octave;
}

void Note::SetName(char newName)
{
    CS_ASSERT(newName >= 'A' && newName <= 'G');
    name = newName;
}

void Note::SetOctave(int newOctave)
{
    CS_ASSERT(newOctave >= 0 && newOctave <= 9);
    octave = newOctave;
}

void Note::SetWrittenAccidental(Accidental accidental)
{
    writtenAccidental = accidental;
}

//--------------------------------------------------

MeasureElement::MeasureElement(Duration duration_)
: duration(duration_)
{
}

bool MeasureElement::AddNote(char name, int octave, Accidental writtenAccidental)
{
    Note note(name, octave, writtenAccidental);
    size_t noteIdx = notes.Find(note);
    if(noteIdx == csArrayItemNotFound)
    {
        notes.Push(note);
        return true;
    }
    else
    {
        notes[noteIdx].SetWrittenAccidental(note.GetWrittenAccidental());
        return false;
    }
}

bool MeasureElement::RemoveNote(char name, int octave)
{
    Note note(name, octave, NO_ACCIDENTAL);
    return notes.Delete(note);
}

//--------------------------------------------------

ScoreContext::ScoreContext()
: lastStartRepeatID(0)
{
}

int ScoreContext::GetNPerformedRepeats(int measureID) const
{
    return repeatsDone.Get(measureID, 0);
}

int ScoreContext::RestoreLastStartRepeat()
{
    measureAttributes = lastStartRepeatAttributes;
    return lastStartRepeatID;
}

void ScoreContext::Update(const MeasureElement &element)
{
    noteContext.UpdateContext(element);
}

void ScoreContext::Update(int measureID, const Measure<MeasureElement> &measure)
{
    noteContext.ResetContext();
    measureAttributes.UpdateAttributes(measure.GetAttributes());

    if(measure.IsStartRepeat())
    {
        lastStartRepeatID = measureID;
        lastStartRepeatAttributes = measureAttributes;

        // we don't need previous repeats anymore at this point
        repeatsDone.DeleteAll();
    }

    if(measure.GetNEndRepeat() > 0 && !repeatsDone.Contains(measureID))
    {
        repeatsDone.Put(measureID, 0);
    }
}
