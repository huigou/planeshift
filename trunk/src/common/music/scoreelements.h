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

/**
 * An element of a measure with a given duration. It can be a rest, a single note or a
 * chord.
 */
class MeasureElement
{
public:
    /**
     * Constructor.
     *
     * @param duration The duration of this element.
     */
    MeasureElement(Duration duration);

    /**
     * Get the duration of this element.
     *
     * @return The duration of this element.
     */
    Duration GetDuration() { return duration; }

    /**
     * Set the duration of this element.
     *
     * @param newDuration the new duration for this element.
     */
    virtual void SetDuration(Duration newDuration) { duration = newDuration; }

private:
    Duration duration;
};

//--------------------------------------------------

/**
 * Represent a rest in the score. The class does not have particular members but its
 * extension could.
 */
class Rest: public MeasureElement
{
public:
    /**
     * Constructor.
     *
     * @param duration The duration of this rest.
     */
    Rest(Duration duration);
};

//--------------------------------------------------

/**
 * A single note in a musical score.
 */
class Note: public MeasureElement
{
public:
    /**
     * Used to keep track of previous altered notes in the current measure. Hides the
     * implementation of note context updating to other classes. It must be resetted at
     * every measure and updated at every note.
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
         * @return The accidental of the previous note within the same measure having the
         * same name and octave as the given one. In case no previous note satisfying the
         * requirements is found, NO_ACCIDENTAL is returned.
         */
        Accidental GetPreviousAccidental(const Note &note) const;

        /**
         * Empty the list of previously altered note.
         */
        void ResetContext();

        /**
         * Update the list of previous accidental if the given note has any.
         *
         * @param note The current played note.
         */
        void UpdateContext(const Note &context);

    private:
        /**
        * Previous notes with a written accidental. Accidentals are indexed by note octave
        * and name.
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
     * @param duration The duration of this note.
     */
    Note(char name, int octave, Accidental writtenAccidental, Duration duration);

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
     * Get the accidental of the played note. This takes into account the written
     * accidental, the tonality and previous accidental in the same measure.
     *
     * @param context The context where tonality and previous accidentals are kept.
     * @return The accidental with which the note must be played in the context of the
     * tonality of the piece. An eventual accidental written on the score previously
     * have the priority.
     */
    Accidental GetPlayedAccidental(const ScoreContext &context) const;

    /**
     * Get the written accidental.
     *
     * @return The accidental written on the score for this note.
     */
    Accidental GetWrittenAccidental() const { return writtenAccidental; }

    /**
     * Equal operator.
     *
     * @param note The note to be compared with.
     * @return True if the given note has the same name and octave.
     */
    bool operator==(const Note &note) const;

    /**
     * Set the note name.
     *
     * @param name The uppercase name of the note (C, D, E, F, G, A or B).
     */
    void SetName(char name);

    /**
     * Set the note octave number.
     *
     * @param newOctave The octave number of this note. Octave number 4 is the one that
     * contains the central C on the piano.
     */
    void SetOctave(int octave);

    /**
     * Set the written accidental.
     *
     * @param accidental The accidental written on the score for this note.
     */
    void SetWrittenAccidental(Accidental accidental);

private:
    char name; ///< Uppercase name of the note (C, D, E, F, G, A or B).
    int octave; ///< Octave number. C4 is the central C on piano.

    /**
     * The accidental with which the note is written on the score. It does not have
     * anything to do with the tonality. With this representation, one does not have to
     * modify all the notes in the score when changing the tonality of the piece.
     */
    Accidental writtenAccidental;
};

//--------------------------------------------------

/**
 * A group of notes that must be played together.
 */
 class Chord: public MeasureElement
{
public:
    /**
     * Constructor.
     */
    Chord(Duration duration);

    /**
     * Copy a note into the chord. If a note with the same name and octave already exists
     * this will overwrite it or, in other words, its accidental will be overwritten.
     *
     * @param name Name of the note (C, D, E, F, G, A or B). Must be uppercase.
     * @param octave Octave number. Octave number 4 is the one that contains the central
     * C on the piano.
     * @param accidental The accidental with which the note is written on the score.
     * @return True if no note was overwritten, false otherwise.
     */
    bool AddNote(char name, int octave, Accidental writtenAccidental);

    /**
     * Remove all the notes from the chord without releasing the allocated memory.
     */
    void Empty() { notes.Empty(); }

    /**
     * Get the number of notes in this chord.
     *
     * @return the number of notes in this chord.
     */
    size_t GetSize() const { return notes.GetSize(); }

    /**
     * Delete a note with the same name and octave.
     *
     * @param note The note that must be deleted.
     * @return True if the note was found, false otherwise.
     */
    bool RemoveNote(char name, int octave);

    /**
     * Set the duration of this element.
     *
     * @param duration the new duration for this element.
     */
    void SetDuration(Duration duration);

private:
    csArray<Note> notes; ///< The sequence of notes in the chord.
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
