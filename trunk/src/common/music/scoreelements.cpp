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

void Note::NoteContext::UpdateContext(const Note &note)
{
    if(note.accidental != NO_ACCIDENTAL)
    {
        csHash<Accidental, char>* nameHash =
            prevAccidentals.GetElementPointer(note.octave);

        if(nameHash == 0)
        {
            nameHash = &prevAccidentals.Put(note.octave,
                csHash<Accidental, char>::csHash(PREV_ACCIDENTALS_NAME_SIZE));
            nameHash->Put(note.name, note.accidental);
        }
        else
        {
            nameHash->PutUnique(note.name, note.accidental);
        }
    }
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

Note::Note(char name_, int octave_, Accidental accidental_)
: name(name_), octave(octave_), accidental(accidental_)
{
    CS_ASSERT(name >= 'A' && name <= 'G');
}

Accidental Note::GetPlayedAccidental(const ScoreContext &context) const
{
    Accidental prevAccidental;
    int fifths = context.prevAttributes.fifths; // convenience variable

    CS_ASSERT(fifths >= -7 && fifths <= 7);

    // accidentals written for this note have priority
    if(accidental != NO_ACCIDENTAL)
    {
        return accidental;
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

//--------------------------------------------------

Measure::MeasureAttributes::MeasureAttributes()
: tempo(UNDEFINED_MEASURE_ATTRIBUTE), beats(UNDEFINED_MEASURE_ATTRIBUTE),
beatType(UNDEFINED_MEASURE_ATTRIBUTE), quarterDivisions(UNDEFINED_MEASURE_ATTRIBUTE),
fifths(UNDEFINED_MEASURE_ATTRIBUTE)
{
}
