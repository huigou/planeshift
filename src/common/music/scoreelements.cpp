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

// 10 octaves are more than enough and each octave contains maximum
// 7 notes so there is really no need of a bigger hash table
#define PREV_ACCIDENTALS_NAME_SIZE 7
#define PREV_ACCIDENTALS_OCTAVE_SIZE 10

// 2 notes is the minimum for a chord and it very rarely gets over 5 notes
#define NOTES_IN_CAPACITY 5

//--------------------------------------------------

MeasureElement::MeasureElement(Duration duration_)
: duration(duration_)
{
}

//--------------------------------------------------

Rest::Rest(Duration duration)
: MeasureElement(duration)
{
}

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
                csHash<Accidental, char>::csHash(PREV_ACCIDENTALS_NAME_SIZE));
            nameHash->Put(note.name, note.writtenAccidental);
        }
        else
        {
            nameHash->PutUnique(note.name, note.writtenAccidental);
        }
    }
}

Note::Note(char name_, int octave_, Accidental writtenAccidental_, Duration duration_)
: MeasureElement(duration_), octave(octave_), writtenAccidental(writtenAccidental_)
{
    SetName(name_);
}

Accidental Note::GetPlayedAccidental(const ScoreContext &context) const
{
    Accidental prevAccidental;
    int fifths = context.prevAttributes.fifths; // convenience variable

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
    name = newName;

    CS_ASSERT(name >= 'A' && name <= 'G');
}

void Note::SetOctave(int newOctave)
{
    octave = newOctave;
}

void Note::SetWrittenAccidental(Accidental accidental)
{
    writtenAccidental = accidental;
}

//--------------------------------------------------

Chord::Chord(Duration duration)
: MeasureElement(duration), notes(NOTES_IN_CAPACITY)
{
}

bool Chord::AddNote(char name, int octave, Accidental writtenAccidental)
{
    Note note(name, octave, writtenAccidental, GetDuration());
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

bool Chord::RemoveNote(char name, int octave)
{
    Note note(name, octave, NO_ACCIDENTAL, GetDuration());
    return notes.Delete(note);
}

void Chord::SetDuration(Duration duration_)
{
    MeasureElement::SetDuration(duration_);
    for(size_t i = 0; i < notes.GetSize(); i++)
    {
        notes[i].SetDuration(duration_);
    }
}

//--------------------------------------------------

Measure::MeasureAttributes::MeasureAttributes()
: tempo(UNDEFINED_MEASURE_ATTRIBUTE), beats(UNDEFINED_MEASURE_ATTRIBUTE),
beatType(UNDEFINED_MEASURE_ATTRIBUTE), fifths(UNDEFINED_MEASURE_ATTRIBUTE)
{
}
