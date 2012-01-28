/*
 * music.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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


#include <psconfig.h>
#include "music.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <zlib.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
#define SCORE_COMPRESSION_FACTOR 3


void psMusic::NextPitch(char &pitch, uint &octave)
{
    switch(pitch)
    {
    case 'A':
        pitch = 'B';
        break;
    case 'B':
        pitch = 'C';
        octave++;
        break;
    case 'C':
        pitch = 'D';
        break;
    case 'D':
        pitch = 'E';
        break;
    case 'E':
        pitch = 'F';
        break;
    case 'F':
        pitch = 'G';
        break;
    case 'G':
        pitch = 'A';
        break;
    }
}

void psMusic::PreviousPitch(char &pitch, uint &octave)
{
    switch(pitch)
    {
    case 'A':
        pitch = 'G';
        break;
    case 'B':
        pitch = 'A';
        break;
    case 'C':
        pitch = 'B';
        octave--;
        break;
    case 'D':
        pitch = 'C';
        break;
    case 'E':
        pitch = 'D';
        break;
    case 'F':
        pitch = 'E';
        break;
    case 'G':
        pitch = 'F';
        break;
    }
}

bool psMusic::GetMeasures(csRef<iDocument> musicalScore, csRefArray<iDocumentNode> &measures)
{
    csRef<iDocumentNode> partNode;
    csRef<iDocumentNode> measureNode;
    csRef<iDocumentNodeIterator> measureIter;

    if(!CheckValidity(musicalScore, partNode))
    {
        return false;
    }

    // updating array
    measures.Empty();
    measureIter = partNode->GetNodes("measure");

    while(measureIter->HasNext())
    {
        measureNode = measureIter->Next();
        measures.Push(measureNode);
    }

    return true;
}

bool psMusic::GetStatistics(csRef<iDocument> musicalScore, ScoreStatistics &stats)
{
    int beats;
    int tempo;
    int quarterDivisions;
    float timePerMeasure;
    float timePerDivision;

    uint nNotes;                            // number of notes in the score
    uint nChords;                           // number of chords in the score
    float totalLengthNoRests;               // total duration of the score in ms excluding rests.

    uint endingNNotes;                      // number of notes in the previous endings
    uint endingNChords;                     // number of chords in the previous endings
    float endingTotalLengthNoRests;         // total length of the previous endings excluding rests

    uint lastRepeatNNotes;                  // number of notes from the last start repeat
    uint lastRepeatNChords;                 // number of chords from the last start repeat
    float lastRepeatTotalLength;            // total length from the last start repeat
    float lastRepeatTotalLengthNoRests;     // total length from the last start repeat excluding rests

    csRefArray<iDocumentNode> measures;

    // this retrieves the general attributes of the score and checks some syntax
    if(!psMusic::GetAttributes(musicalScore, quarterDivisions, stats.fifths, beats, stats.beatType, tempo)
        || !psMusic::GetMeasures(musicalScore, measures))
    {
        return false;
    }

    timePerDivision = 60.0f / tempo / quarterDivisions * 1000; // (ms)
    timePerMeasure = 60.0f / tempo * beats / stats.beatType * 4 * 1000; // (ms)

    // initializing variables
    nNotes = 0;
    nChords = 0;
    totalLengthNoRests = 0.0;

    endingNNotes = 0;
    endingNChords = 0;
    endingTotalLengthNoRests = 0.0;

    lastRepeatNNotes = 0;
    lastRepeatNChords = 0;
    lastRepeatTotalLength = 0.0;
    lastRepeatTotalLengthNoRests = 0.0;

    stats.totalLength = 0.0;
    stats.maximumPolyphony = 0;
    stats.minimumDuration = 500000; // (ms) this is more than a 8/4 note at bpm = 1

    // parsing the score
    for(size_t i = 0; i < measures.GetSize(); i++)
    {
        int noteDuration;
        uint measureNNotes = 0;
        uint measureNChords = 0;
        float measureDurationNoRests = 0.0;
        int currentChordPolyphony = 0;
        csRef<iDocumentNode> noteNode;
        csRef<iDocumentNode> barlineNode;
        csRef<iDocumentNode> durationNode;
        csRef<iDocumentNodeIterator> notesIter;

        // handling repeats
        barlineNode = measures.Get(i)->GetNode("barline");
        if(barlineNode.IsValid())
        {
            csRef<iDocumentNode> repeatNode = barlineNode->GetNode("repeat");
            if(repeatNode.IsValid())
            {
                const char* direction = repeatNode->GetAttributeValue("direction");
                
                if(direction == 0) // wrong syntax
                {
                    return false;
                }

                // resetting lastRepeat variables data
                if(csStrCaseCmp(direction, "forward") == 0)
                {
                    lastRepeatNNotes = 0;
                    lastRepeatNChords = 0;
                    lastRepeatTotalLength = 0.0;
                    lastRepeatTotalLengthNoRests = 0.0;
                }
            }
        }

        // parsing notes
        notesIter = measures.Get(i)->GetNodes("note");
        while(notesIter->HasNext())
        {
            noteNode = notesIter->Next();
            durationNode = noteNode->GetNode("duration");

            // if there's no durationNode syntax is incorrect
            if(!durationNode.IsValid())
            {
                return false;
            }

            noteDuration = durationNode->GetContentsValueAsInt() * timePerDivision;

            // if this is a rest only the total length must be modified (done later)
            if(noteNode->GetNode("rest").IsValid())
            {
                // resetting this value is needed to not update minimumDuration
                // when the last notes of the score is a rests
                currentChordPolyphony = 0;
            }
            else // this is a note
            {
                measureNNotes++;

                // if the note is part of a chord neither the chords count nor the total
                // length have to be updated. The first notes in a chord doesn't have the
                // tag <chord> so the condition is true for them.
                if(noteNode->GetNode("chord").IsValid())
                {
                    currentChordPolyphony++;
                }
                else // this is a new chord
                {
                    // updating maximum polyphony and minimum duration
                    if(stats.maximumPolyphony < currentChordPolyphony)
                    {
                        stats.maximumPolyphony = currentChordPolyphony;
                    }

                    if(stats.minimumDuration > noteDuration)
                    {
                        stats.minimumDuration = noteDuration;
                    }

                    currentChordPolyphony = 1;
                    measureNChords++;
                    measureDurationNoRests += noteDuration;
                }
            }
        }

        // updating maximum polyphony and minimum duration if the last note was part of a chord
        if(currentChordPolyphony > 1)
        {
            if(stats.maximumPolyphony < currentChordPolyphony)
            {
                stats.maximumPolyphony = currentChordPolyphony;
            }
            if(stats.minimumDuration > noteDuration)
            {
                stats.minimumDuration = noteDuration;
            }
        }

        nNotes += measureNNotes;
        nChords += measureNChords;
        stats.totalLength += timePerMeasure;
        totalLengthNoRests += measureDurationNoRests;

        lastRepeatNNotes +=  measureNNotes;
        lastRepeatNChords += measureNChords;
        lastRepeatTotalLength += timePerMeasure;
        lastRepeatTotalLengthNoRests += measureDurationNoRests;

        // checking if this measure is the end of a repeat
        if(barlineNode.IsValid())
        {
            csRef<iDocumentNode> repeatNode = barlineNode->GetNode("repeat");
            csRef<iDocumentNode> endingNode = barlineNode->GetNode("ending");

            if(repeatNode.IsValid())
            {
                const char* direction = repeatNode->GetAttributeValue("direction");
                if(direction == 0) // syntax is wrong
                {
                    return false;
                }

                if(csStrCaseCmp(direction, "backward") == 0)
                {
                    int repeatCounter = repeatNode->GetAttributeValueAsInt("times");

                    nNotes += lastRepeatNNotes * repeatCounter;
                    nChords += lastRepeatNChords * repeatCounter;
                    stats.totalLength += lastRepeatTotalLength * repeatCounter;
                    totalLengthNoRests += lastRepeatTotalLengthNoRests * repeatCounter;
                }
            }

            // handling endings
            if(endingNode.IsValid())
            {
                int endingNumber = endingNode->GetAttributeValueAsInt("number");

                if(endingNumber == 1) // reset ending data
                {
                    endingNNotes = 0;
                    endingNChords = 0;
                    endingTotalLengthNoRests = 0.0;
                }
                else
                {
                    // previous endings are not played so they aren't taken into account
                    nNotes -= endingNNotes;
                    nChords -= endingNChords;
                    stats.totalLength -= (endingNumber - 1) * timePerMeasure;
                    totalLengthNoRests -= endingTotalLengthNoRests;

                    endingNNotes += measureNNotes;
                    endingNChords += measureNChords;
                    endingTotalLengthNoRests += measureDurationNoRests;
                }
            }
        }
    }

    // setting average values
    if(nChords == 0)
    {
        // here stats.minimumDuration is still 500000
        stats.minimumDuration = 0;
        stats.averageDuration = 0.0;
        stats.averagePolyphony = 0.0;
    }
    else
    {
        stats.averageDuration = totalLengthNoRests / nChords;
        stats.averagePolyphony = (float)nNotes / nChords;
    }

    return true;
}

bool psMusic::GetAttributes(csRef<iDocument> musicalScore, int &quarterDivisions,
                          int &fifths, int &beats, int &beatType, int &tempo)
{
    csRef<iDocumentNode> partNode;
    csRef<iDocumentNode> measureNode;

    if(!CheckValidity(musicalScore, partNode))
    {
        return false;
    }

    // getting first measure
    measureNode = partNode->GetNode("measure");
    if(measureNode == 0)
    {
        return false;
    }

    // getting attributes and direction
    csRef<iDocumentNode> attributesNode = measureNode->GetNode("attributes");
    csRef<iDocumentNode> directionNode = measureNode->GetNode("direction");

    if(attributesNode != 0 && directionNode != 0)
    {
        // checking first level
        csRef<iDocumentNode> keyNode = attributesNode->GetNode("key");
        csRef<iDocumentNode> timeNode = attributesNode->GetNode("time");
        csRef<iDocumentNode> divisionsNode = attributesNode->GetNode("divisions");
        csRef<iDocumentNode> soundNode = directionNode->GetNode("sound");

        if(keyNode == 0 || timeNode == 0 || divisionsNode == 0 || soundNode == 0)
        {
            return false;
        }

        // checking second level
        csRef<iDocumentNode> fifthsNode = keyNode->GetNode("fifths");
        csRef<iDocumentNode> beatsNode = timeNode->GetNode("beats");
        csRef<iDocumentNode> beatTypeNode = timeNode->GetNode("beat-type");

        if(fifthsNode == 0 || beatsNode == 0 || beatTypeNode == 0)
        {
            return false;
        }

        // assigning parameters
        quarterDivisions = divisionsNode->GetContentsValueAsInt();
        fifths = fifthsNode->GetContentsValueAsInt();
        beats = beatsNode->GetContentsValueAsInt();
        beatType = beatTypeNode->GetContentsValueAsInt();
        tempo = soundNode->GetAttributeValueAsInt("tempo", 90);
    }
    else
    {
        return false;
    }

    // checking data integrity
    if(quarterDivisions <= 0)
    {
        quarterDivisions = 1;
    }
    if(beats <= 0)
    {
        beats = 1;
    }
    if(fifths < -7 || fifths > 7)
    {
        fifths = 0;
    }
    if(beatType <= 0)
    {
        beatType = 4;
    }
    if(tempo <= 0)
    {
        tempo = 90;
    }

    return true;
}

bool psMusic::ZCompressSong(const csString &inputScore, csString &outputScore)
{
    int error;                      // last zlib error
    z_stream zStream;               // z stream used to compress data
    size_t inputSize;               // length of the uncompressed musical sheet in XML format
    char* tempBuffer;               // temporary output buffer used for compression
    size_t tempSize;                // length of the temporary buffer

    // be sure that this is and empty string
    outputScore = "";

    // checking if the input is empty
    if(inputScore.IsEmpty())
    {
        outputScore = inputScore;
        return true;
    }

    // initializing temporary buffer
    inputSize = inputScore.Length();
    tempSize = inputSize / SCORE_COMPRESSION_FACTOR;
    tempBuffer = new char[tempSize];

    // initializing z stream
    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;
    zStream.opaque = Z_NULL;
    zStream.next_in = (Bytef*)inputScore.GetData();
    zStream.avail_in = (uInt)inputSize;

    error = deflateInit(&zStream, Z_DEFAULT_COMPRESSION);
    CS_ASSERT(error == Z_OK);

    // compressing score
    do
    {
        // resetting output buffer
        zStream.next_out = (Bytef*)tempBuffer;
        zStream.avail_out = (uInt)tempSize;

        // compressing
        error = deflate(&zStream, Z_FINISH);
        CS_ASSERT(error != Z_STREAM_ERROR);

        // updating output
        outputScore.Append(tempBuffer, tempSize - zStream.avail_out);
    } while(zStream.avail_out == 0);
    CS_ASSERT(error == Z_STREAM_END);

    // cleaning up
    deflateEnd(&zStream);
    delete[] tempBuffer;

    return true;
}

bool psMusic::ZDecompressSong(const csString &inputScore, csString &outputScore)
{
    int error;                      // last zlib error
    z_stream zStream;               // z stream used to decompress data
    size_t inputSize;               // length of the compressed musical sheet
    char* tempBuffer;               // temporary output buffer used for decompression
    size_t tempSize;                // length of the temporary buffer

    // be sure that this is and empty string
    outputScore = "";

    // checking if the input is empty
    if(inputScore.IsEmpty())
    {
        outputScore = inputScore;
        return true;
    }

    // initializing temporary buffer
    inputSize = inputScore.Length();
    tempSize = inputSize * SCORE_COMPRESSION_FACTOR;
    tempBuffer = new char[tempSize];

    // initializing z stream
    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;
    zStream.opaque = Z_NULL;
    zStream.next_in = (Bytef*)inputScore.GetData();
    zStream.avail_in = (uInt)inputSize;

    error = inflateInit(&zStream);
    CS_ASSERT(error == Z_OK);

    // compressing score
    do
    {
        // resetting output buffer
        zStream.next_out = (Bytef*)tempBuffer;
        zStream.avail_out = (uInt)tempSize;

        // decompressing
        error = inflate(&zStream, Z_SYNC_FLUSH);

        // error handling
        CS_ASSERT(error != Z_STREAM_ERROR);
        if(error == Z_DATA_ERROR || error == Z_MEM_ERROR)
        {
            // cleaning up
            deflateEnd(&zStream);
            delete[] tempBuffer;

            return false;
        }

        // updating output
        outputScore.Append(tempBuffer, tempSize - zStream.avail_out);
    } while(error != Z_STREAM_END);

    // cleaning up
    deflateEnd(&zStream);
    delete[] tempBuffer;

    return true;
}

bool psMusic::CheckValidity(csRef<iDocument> musicalScore, csRef<iDocumentNode> &partNode)
{
    csRef<iDocumentNode> documentRoot;
    csRef<iDocumentNode> rootNode;

    csRef<iDocumentNodeIterator> measureIter;

    if(musicalScore == 0)
    {
        return false;
    }

    // checking basic root
    documentRoot = musicalScore->GetRoot();
    if(documentRoot == 0)
    {
        return false;
    }

    // checking root node
    rootNode = documentRoot->GetNode("score-partwise");
    if(rootNode == 0)
    {
        return false;
    }

    // checking part node
    partNode = rootNode->GetNode("part");
    if(partNode == 0)
    {
        return false;
    }

    return true;
}
