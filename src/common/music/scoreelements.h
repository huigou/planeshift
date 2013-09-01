/*
 * scoreelements.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef SCORE_ELEMENTS_H
#define SCORE_ELEMENTS_H


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <csutil/hash.h>

//====================================================================================
// Project Includes
//====================================================================================

//====================================================================================
// Local Includes
//====================================================================================
#include "musicutil.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
#define UNDEFINED_MEASURE_ATTRIBUTE -100

struct ScoreContext;

using namespace psMusic;

/**
 * \addtogroup common_music
 * @{ */

//--------------------------------------------------

/**
 * A single note in a musical score.
 */
class Note
{
public:
    /**
     * Used to keep track of previous altered notes in the current measure. Hides the
     * implementation of note context updating to other classes.
     */
    class NoteContext
    {
    public:
        /**
         * Constructor.
         */
        NoteContext();

        /**
         * Get an eventual accidental that previously appeared in the same measure.
         *
         * @param note The note that should be checked.
         * @return The accidental in case there where previous altered notes or
         * NO_ACCIDENTAL if none.
         */
        Accidental GetPreviousAccidental(const Note &note) const;

        /**
         * Update the list of previous accidental if the given note has any.
         *
         * @param note The current played note.
         */
        void UpdateContext(const Note &context);

        /**
         * Empty the list of previously altered note.
         */
        void ResetContext();

    private:
        /**
        * Previous notes with a written accidental. Accidentals are indexed by note octave
        * and name. Must be resetted at every measure and updated at every note.
        */
        csHash<csHash<Accidental, char>, int> prevAccidentals;
    };

    /**
     * Constructor.
     *
     * @param name Name of the note (C, D, E, F, G, A or B). Must be uppercase.
     * @param octave Octave number. Octave number 4 is the one that contains the central
     * C on the piano.
     * @param accidental The accidental with which the note is written on the score. This
     * is not related to the tonality of the piece but only on its representation. In
     * other words the variable accidental must be different than UNALTERED if and only if
     * an actual accidental is written on the musical score.
     */
    Note(char name, int octave, Accidental accidental);

    /**
     * Get the note name.
     *
     * @return The uppercase name of the note (C, D, E, F, G, A or B).
     */
    char GetName() const { return name; }

    /**
     * Get the note octave number.
     *
     * @return The octave number of this note. Octave number 4 is the one that contains
     * the central C on the piano.
     */
    int GetOctave() const { return octave; }

    /**
     * Get the accidental of the played note. This, depending on the tonality, may not be
     * the same as the written accidental.
     *
     * @param context The context where tonality and previous accidentals are kept.
     * @return The accidental with which the note must be played in the context of the
     * tonality of the piece. An eventual accidental written on the score have the
     * priority.
     */
    Accidental GetPlayedAccidental(const ScoreContext &context) const;

private:
    char name; ///< Uppercase name of the note (C, D, E, F, G, A or B).
    int octave; ///< Octave number. C4 is the central C on piano.

    /**
     * The accidental with which the note is written on the score. It does not have
     * anything to do with the tonality. With this representation, one does not have to
     * modify all the notes in the score when changing the tonality of the piece.
     */
    Accidental accidental;
};

//--------------------------------------------------

class Chord
{
};

//--------------------------------------------------

class Measure
{
public:
    /**
     * Keep general attributes that can change from a measure to another like key signature,
     * beats and tempo. Some attributes can be undefined. This means that the specific
     * attribute takes the same value as in the previous measure. Undefined attributes take
     * the value UNDEFINED_MEASURE_ATTRIBUTE.
     */
    struct MeasureAttributes
    {
        int tempo; ///< Quarter notes per minute (BPM).
        int beats; ///< Numerator of the time signature.
        int beatType; ///< Denominator of the time signature.

        /**
         * Unit of measure for notes duration in terms of divisions per quarter.
         */
        int quarterDivisions;

        /**
         * Number of accidentals in the key signature. Positive for sharps, negative for
         * flats.
         */
        int fifths;

        MeasureAttributes();
    };
};

//--------------------------------------------------

/**
 * Keep track of previous accidentals in the same measure and attributes of previous
 * measures when they are not specified in the current one. This struct make sense only
 * when it refers to a specific point in the musical score.
 */
struct ScoreContext
{
    // TODO how to handle repeats for measure attributes?

    /**
     * Used to keep track of previous accidentals in the same measure.
     */
    Note::NoteContext noteContext;

    /**
     * Attributes specified in the score up to now. Must be updated at every measure.
     */
    Measure::MeasureAttributes prevAttributes;
};

/** @} */

#endif // SCORE_ELEMENTS_H
