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
     * Remove all the notes from the chord.
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
     * Keep general attributes that can change from a measure to another like key
     * signature, beats and tempo. Some attributes can be undefined. This means that the
     * specific attribute takes the same value of the same attribute in the previous
     * measure. Undefined attributes take the value UNDEFINED_MEASURE_ATTRIBUTE.
     */
    class MeasureAttributes
    {
    public:
        /**
         * Initialize all attributes to UNDEFINED_MEASURE_ATTRIBUTE.
         */
        MeasureAttributes();

        /**
         * Get the numerator of the time signature.
         *
         * @return The numerator of the time signature.
         */
        int GetBeats() const { return beats; }

        /**
         * Get the denominator of the time signature.
         *
         * @return The denominator of the time signature.
         */
        int GetBeatType() const { return beatType; }

        /**
         * Get the tonality as the number of accidentals in the key signature.
         *
         * @return The number of accidentals in the key signature. Positive for sharps,
         * negative for flats.
         */
        int GetFifths() const { return fifths; }

        /**
         * Get the tempo.
         *
         * @return The tempo in quarter notes per minute.
         */
        int GetTempo() const { return tempo; }

        /**
         * Check if all attributes are undefined.
         *
         * @return True if all attributes are undefined, false otherwise.
         */
        bool IsUndefined() const;

        /**
         * Set the numerator of the time signature.
         *
         * @param beats The new numerator of the time signature.
         */
        void SetBeats(int beats) { this->beats = beats; }

        /**
         * Set the denominator of the time signature.
         *
         * @param beatType The new denominator of the time signature.
         */
        void SetBeatType(int beatType) { this->beatType = beatType; }

        /**
         * Set the key signature as the number of accidentals.
         *
         * @param fifths The number of accidentals in the key signature. Positive for
         * sharps, negative for flats.
         */
        void SetFifths(int fifths) { this->fifths = fifths; }

        /**
         * Set the tempo.
         *
         * @param tempo The new tempo.
         */
        void SetTempo(int tempo) { this->tempo = tempo; }

        /**
         * Update the measure context with the attributes of the given measure.
         * Attributes that are undefined in the given object are not updated.
         *
         * @param measure The measure with the new attributes.
         */
        void UpdateAttributes(const MeasureAttributes &attributes);

    private:
        int tempo; ///< Quarter notes per minute (BPM).
        int beats; ///< Numerator of the time signature.
        int beatType; ///< Denominator of the time signature.

        /**
         * Number of accidentals in the key signature. Positive for sharps, negative for
         * flats.
         */
        int fifths;
    };

    /**
     * Constructor.
     */
    Measure();

    /**
     * Destructor.
     */
    ~Measure();

    /**
     * Delete the element in position n. If n is not a valid index it does nothing.
     *
     * @param n The index of the measure element.
     */
    void DeleteElement(size_t n);

    /**
     * Delete all elements from the measure.
     */
    void DeleteAllElements();

    /**
     * Remove the elements at the end of the measures that exceeds the total duration or
     * push a rest if there are not enough elements. The total duration is determined
     * from the beat and beat type contained in the attributes. Attributes specified in
     * this measure have priority. If they are undefined, the given attributes are used
     * instead.
     *
     * @param attributes Attributes of previous measures. This can be a null pointer if
     * this measure specifies both beat and beat type. If this is not the case however,
     * the parameter must specify at least information about beat and beat type.
     */
    void Fit(const MeasureAttributes* attributes);

    /**
     * Get a copy of the attributes of this measure.
     *
     * @return The attributes of this measure.
     */
    MeasureAttributes GetAttributes() const;

    /**
     * Get the element at position n.
     *
     * @param n The index of the element.
     * @return The element or 0 if n is not a valid index.
     */
    MeasureElement* GetElement(size_t n) { return elements.Get(n); }

    /**
     * Get the number of elements in this measure.
     *
     * @return The number of elements in this measure.
     */
    int GetNElements() { return elements.GetSize(); }

    /**
     * Get the number of time the repeat at the end of this measure must be performed.
     *
     * @return The number of time the repeat must be performed or 0 if there is not a
     * repeat at the end of the measure.
     */
    int GetNEndRepeat() const { return nEndRepeat; }

    /**
     * Insert the given element after element n. The element will be deallocated when the
     * measure is deleted. If n is greater than the number of elements in this measure,
     * the element is pushed at the end.
     *
     * @param n The number of the element after which the given element must be inserted.
     * @param element The element to insert in the measure.
     */
    void InsertElement(size_t n, MeasureElement* element);

    /**
     * Return true if this is an ending measure.
     *
     * @return True if this is an ending measure, false otherwise.
     */
    bool IsEnding() const { return isEnding; }

    /**
     * Return true if this measure starts a repeat.
     *
     * @return True if this is the start of a repeat, false otherwise.
     */
    bool IsStartRepeat() const { return isStartRepeat; }

    /**
     * Push an element after all the elements currently in the measure. The element will
     * be deallocated when the measure is deleted.
     *
     * @param element The element to push in the measure.
     */
    void PushElement(MeasureElement* element);

    /**
     * Set the beat information. Both beats and beatType must be defined. If only one of
     * them is undefined, both of them will be saved as undefined.
     *
     * @param beats The numerator of the time signature.
     * @param beatType The denominator of the time signature.
     */
    void SetBeat(int beats, int beatType);

    /**
     * Set this measure as an ending.
     *
     * @param isEnding True if this is an ending, false otherwise.
     */
    void SetEnding(bool isEnding);

    /**
     * @copydoc Measure::MeasureAttributes::SetFifths()
     */
    void SetFifths(int fifths);

    /**
     * Set the number of time the repeat at the end of this measure must be performed.
     *
     * @param nEndRepeat The number of time the repeat must be performed or 0 if this
     * measure does not end a repeat.
     */
    void SetNEndRepeat(int nEndRepeat);

    /**
     * Set this measure to start a repeat.
     *
     * @param isEnding True if this is the start of a repeat, false otherwise.
     */
    void SetStartRepeat(bool isStartRepeat);

    /**
     * @copydoc Measure::MeasureAttributes::SetTempo()
     */
    void SetTempo(int tempo);

private:
    bool isEnding; ///< True if this is an ending measure.
    bool isStartRepeat; ///< True if this measure starts a repeat.
    int nEndRepeat; ///< Number of times the repeat must be performed.
    csArray<MeasureElement*> elements; ///< Elements of this measure.

    /**
     * Attributes of this measure. We keep this allocated object only if the measure
     * actually have some attributes that are defined.
     */
    MeasureAttributes* attributes;

    /**
     * Create a new MeasureAttributes object if it does not exists already.
     */
    void CreateAttributes();

    /**
     * Delete the attributes (if it exists) and free the memory.
     */
    void DeleteAttributes();

    /**
     * Returns the biggest duration that can be represented on the score which is less
     * or equal to the given one.
     *
     * @param duration The maximum duration that must be returned.
     * @return The duration value.
     */
    Duration GetBiggestDuration(int duration) const;

    /**
     * Delete the MeasureAttributes object if all its attributes are undefined.
     */
    void UpdateAttributes();
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
    Measure::MeasureAttributes measureAttributes;
};

/** @} */

#endif // SCORE_ELEMENTS_H
