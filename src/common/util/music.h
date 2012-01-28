/*
 * music.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef PSMUSIC_H
#define PSMUSIC_H


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <csutil/refarr.h>
#include <csutil/csstring.h>
#include <iutil/document.h>


/**
 * This struct keeps general information about a score.
 */
struct ScoreStatistics
{
    ScoreStatistics()
    {
        totalLength = 0.0;
        minimumDuration = 0;
        maximumPolyphony = 0;
        averageDuration = 0.0;
        averagePolyphony = 0.0;

        beatType = 0;
        fifths = 0;
    }

    float totalLength;          ///< total duration of the score in milliseconds.
    int minimumDuration;        ///< duration of the shortest note in the score in ms.
    int maximumPolyphony;       ///< maximum number of notes played at the same time.
    float averageDuration;      ///< average duration of the score's notes in ms.
    float averagePolyphony;     ///< average number of notes played at the same time.

    int beatType;               ///< beat type of the score.
    int fifths;                 ///< tonality as number of sharps (if > 0) or flats (if < 0).
};


/**
 * This class contains a set of functions that are usefull for the processing of music
 * and musical scores.
 */
class psMusic
{
public:
    /**
     * Turns the given pitch into the next one in the scale.
     * @param pitch the pitch of the note.
     * @param octave the octave of the note.
     */
    static void NextPitch(char &pitch, uint &octave);

    /**
     * Turns the given pitch into the previous one in the scale.
     * @param pitch the pitch of the note.
     * @param octave the octave of the note.
     */
    static void PreviousPitch(char &pitch, uint &octave);

    /**
     * Gets the XML nodes representing the measures contained in the musical score.
     *
     * @param musicalScore the musical score.
     * @param measures a reference to a csRefArray that will contain the XML nodes.
     * @return true if the document is a valid musical score, false otherwise.
     */
    static bool GetMeasures(csRef<iDocument> score, csRefArray<iDocumentNode> &measures);

    /**
     * Returns the length of the score in milliseconds in the parameter length. Rests
     * are counted only for determining the song's total length but they are excluded
     * from other statistics. If the score is empty (or has only rests), the average
     * polyphony and duration are set to 0.
     *
     * @param musicalScore the musical score.
     * @param stats the retrieved statistics of the given score.
     * @return true if the document is a valid musical score, false otherwise.
     */
    static bool GetStatistics(csRef<iDocument> musicalScore, ScoreStatistics &stats);

    /**
     * Gets the attributes in the first measure of the given score. The provided
     * attributes are always valid. If the musical score contains non valid attributes
     * (e.g. beatType <= 0), they are set to a valid default value.
     *
     * @param musicalScore the musical score.
     * @param quarterDivisions the number of divisions in a quarter for the score.
     * @param fifths the tonality of the score.
     * @param beats beats of the song.
     * @param beatType the beat type of the song.
     * @param tempo the beat per minutes of the song.
     * @return true if the document is a valid musical score and the attributes could be
     * found in the first measure, false otherwise. False is returned also if there are
     * not measures in the given score.
     */
    static bool GetAttributes(csRef<iDocument> musicalScore, int &quarterDivisions,
        int &fifths, int &beats, int &beatType, int &tempo);

    /**
     * Compress a song with the zlib compression algorithm.
     * @attention the output string is not a normal string but a sequence of bytes. It
     * could contain null characters. Do not treat it like a null terminated string.
     *
     * @param inputScore the musical sheet in uncompressed XML format.
     * @param outputScore at the end this string will contain the compressed score.
     * @return true if the compression is done without errors, false otherwise.
     */
    static bool ZCompressSong(const csString &inputScore, csString &outputScore);

    /**
     * Decompress a song with the zlib compression algorithm.
     * @param inputScore the compressed musical sheet.
     * @param outputScore at the end this string will contain the uncompressed score.
     * @return true if the decompression is done without errors, false otherwise.
     */
    static bool ZDecompressSong(const csString &inputScore, csString &outputScore);

private:
    /**
     * Checks if the given document is a valid musical score and provide the <part> node.
     *
     * @param musicalScore the musical score.
     * @param partNode a reference that will contain the part XML node.
     * @return true if the document is valid, false otherwise.
     */
    static bool CheckValidity(csRef<iDocument> musicalScore, csRef<iDocumentNode> &partNode);
};

#endif // PSMUSIC_H

