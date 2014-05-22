/*
 * scoreelements.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2014 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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


template<typename MeasureElementType>
Measure<MeasureElementType>::MeasureAttributes::MeasureAttributes()
: tempo(UNDEFINED_MEASURE_ATTRIBUTE), beats(UNDEFINED_MEASURE_ATTRIBUTE),
beatType(UNDEFINED_MEASURE_ATTRIBUTE), fifths(UNDEFINED_MEASURE_ATTRIBUTE)
{
}

template<typename MeasureElementType>
bool Measure<MeasureElementType>::MeasureAttributes::IsUndefined() const
{
    return tempo == UNDEFINED_MEASURE_ATTRIBUTE &&
        beats == UNDEFINED_MEASURE_ATTRIBUTE &&
        beatType == UNDEFINED_MEASURE_ATTRIBUTE &&
        fifths == UNDEFINED_MEASURE_ATTRIBUTE;
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::MeasureAttributes::UpdateAttributes(
    const typename Measure<MeasureElementType>::MeasureAttributes &attributes)
{
    if(attributes.beats != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        beats = attributes.beats;
    }
    if(attributes.beatType != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        beatType = attributes.beatType;
    }
    if(attributes.fifths != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        fifths = attributes.fifths;
    }
    if(attributes.tempo != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        tempo = attributes.tempo;
    }
}

template<typename MeasureElementType>
Measure<MeasureElementType>::Measure()
  : isEnding(false), isStartRepeat(false), nEndRepeat(0), attributes(0)
{
}

template<typename MeasureElementType>
Measure<MeasureElementType>::~Measure()
{
    DeleteAttributes();
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::Fit(const MeasureAttributes* attributes_)
{
    int measDuration = 0; // the duration that the measure is supposed to be
    int currDuration = 0; // the sum of the duration of the elements
    size_t cutIdx = 0; // the index contains the index of the first element to be cut

    // Convenience variables
    int beats = UNDEFINED_MEASURE_ATTRIBUTE;
    int beatType = UNDEFINED_MEASURE_ATTRIBUTE;

    // Getting beat information
    if(attributes != 0)
    {
        beats = attributes->GetBeats();
        beatType = attributes->GetBeatType();
    }
    if(attributes_ != 0 && (beats == UNDEFINED_MEASURE_ATTRIBUTE ||
        beatType == UNDEFINED_MEASURE_ATTRIBUTE))
    {
            beats = attributes_->GetBeats();
            beatType = attributes_->GetBeatType();
    }

    CS_ASSERT(beats != UNDEFINED_MEASURE_ATTRIBUTE &&
        beatType != UNDEFINED_MEASURE_ATTRIBUTE);

    // Determining the measure duration
    measDuration = DURATION_QUARTER_DIVISIONS * beats / beatType;

    // Determining which notes exceed the measure
    while(cutIdx < elements.GetSize() && currDuration <= measDuration)
    {
        currDuration += elements[cutIdx].GetDuration();
        cutIdx++;
    }

    // Cutting or filling
    if(currDuration > measDuration) // cut
    {
        int prevDuration = currDuration - elements.Get(cutIdx).GetDuration();

        // If there's a note that exceeds the measure duration we can cut it first
        if(prevDuration < measDuration)
        {
            Duration tempDuration = GetBiggestDuration(measDuration - prevDuration);
            elements.Get(cutIdx).SetDuration(tempDuration);
            cutIdx++;
        }
        for(size_t i = elements.GetSize() - 1; i >= cutIdx; i--)
        {
            DeleteElement(i);
        }
    }
    else if(currDuration < measDuration) // fill
    {
        Duration tempDuration;

        while(currDuration < measDuration)
        {
            tempDuration = GetBiggestDuration(measDuration - currDuration);
            PushElement(MeasureElementType(tempDuration));
            currDuration += tempDuration;
        }
    }
}

template<typename MeasureElementType>
typename Measure<MeasureElementType>::MeasureAttributes
	Measure<MeasureElementType>::GetAttributes() const
{
    if(attributes == 0)
    {
        return MeasureAttributes(); // all attributes are undefined
    }
    return *attributes;
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::InsertElement(size_t n, const MeasureElementType &element)
{
    if(!elements.Insert(n, element))
    {
        PushElement(element);
    }
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::SetBeat(int beats, int beatType)
{
    if(beats != UNDEFINED_MEASURE_ATTRIBUTE && beatType != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        // check if beatType_ is a power of 2
        CS_ASSERT((beatType > 0) && ((beatType & (~beatType + 1)) == beatType));
        CS_ASSERT(beats > 0);
        CreateAttributes();
    }

    // doesn't make sense if one is defined and the other is not
    if(beats == UNDEFINED_MEASURE_ATTRIBUTE || beatType == UNDEFINED_MEASURE_ATTRIBUTE)
    {
        beats = UNDEFINED_MEASURE_ATTRIBUTE;
        beatType = UNDEFINED_MEASURE_ATTRIBUTE;
    }

    if(attributes != 0)
    {
        attributes->SetBeats(beats);
        attributes->SetBeatType(beatType);
        UpdateAttributes();
    }
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::SetEnding(bool isEnding_)
{
    isEnding = isEnding_;
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::SetFifths(int fifths)
{
    if(fifths != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        CS_ASSERT(fifths >= -7 && fifths <= 7);
        CreateAttributes();
    }
    if(attributes != 0)
    {
        attributes->SetFifths(fifths);
        UpdateAttributes();
    }
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::SetNEndRepeat(int nEndRepeat_)
{
    CS_ASSERT(nEndRepeat_ >= 0);
    nEndRepeat = nEndRepeat_;
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::SetStartRepeat(bool isStartRepeat_)
{
    isStartRepeat = isStartRepeat_;
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::SetTempo(int tempo)
{
    if(tempo != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        CS_ASSERT(tempo > 0);
        CreateAttributes();
    }
    if(attributes != 0)
    {
        attributes->SetTempo(tempo);
        UpdateAttributes();
    }
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::CreateAttributes()
{
    if(attributes == 0)
    {
        attributes = new MeasureAttributes();
    }
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::DeleteAttributes()
{
    if(attributes != 0)
    {
        delete attributes;
        attributes = 0;
    }
}

template<typename MeasureElementType>
void Measure<MeasureElementType>::UpdateAttributes()
{
    if(attributes != 0)
    {
        if(attributes->IsUndefined())
        {
            DeleteAttributes();
        }
    }
}

