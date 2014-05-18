/*
 * basemusicscore.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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
#define INVALID_CURSOR (size_t)-1

template<template<typename> class MeasureType, typename MeasureElementType>
BaseMusicalScore<MeasureType, MeasureElementType>::BaseMusicalScore()
    : cursor(new Cursor(this, EDIT)), mode(EDIT)
{
}

template<template<typename> class MeasureType, typename MeasureElementType>
BaseMusicalScore<MeasureType, MeasureElementType>::~BaseMusicalScore()
{
    delete cursor;
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::AdvanceCursor(
	bool ignoreEndOfMeasure)
{
    return cursor->Advance(ignoreEndOfMeasure);
}

template<template<typename> class MeasureType, typename MeasureElementType>
typename BaseMusicalScore<MeasureType, MeasureElementType>::Cursor*
	BaseMusicalScore<MeasureType, MeasureElementType>::GetEditCursor()
{
    if(mode != EDIT)
    {
        return 0;
    }
    return cursor;
}

template<template<typename> class MeasureType, typename MeasureElementType>
const MeasureType<MeasureElementType>*
	BaseMusicalScore<MeasureType, MeasureElementType>::GetMeasure(size_t n) const
{
    CS_ASSERT(n < measures.GetSize());
    return &measures[n];
}

template<template<typename> class MeasureType, typename MeasureElementType>
const typename BaseMusicalScore<MeasureType, MeasureElementType>::Cursor*
	BaseMusicalScore<MeasureType, MeasureElementType>::GetPlayCursor() const
{
    if(mode != PLAY)
    {
        return 0;
    }
    return cursor;
}

template<template<typename> class MeasureType, typename MeasureElementType>
typename BaseMusicalScore<MeasureType, MeasureElementType>::Cursor*
	BaseMusicalScore<MeasureType, MeasureElementType>::SetEditMode()
{
    if(mode == PLAY)
    {
        delete cursor;
        cursor = new Cursor(this, EDIT);
    }
    return cursor;
}

template<template<typename> class MeasureType, typename MeasureElementType>
const typename BaseMusicalScore<MeasureType, MeasureElementType>::Cursor*
	BaseMusicalScore<MeasureType, MeasureElementType>::SetPlayMode()
{
    if(mode == EDIT)
    {
        delete cursor;
        cursor = new Cursor(this, PLAY);
    }
    return cursor;
}

//---------------------------------------------------------------------------------------

template<template<typename> class MeasureType, typename MeasureElementType>
BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::~Cursor()
{
    if(context != 0)
    {
        delete context;
    }
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::Advance(
	bool ignoreEndOfMeasure)
{
    bool isLastElem;

    // if this is the last element we move the cursor to end-of-score
    if(!HasNext(ignoreEndOfMeasure))
    {
        currElementIdx = INVALID_CURSOR;
        currMeasureIdx = INVALID_CURSOR;
        return false;
    }

    // check if this is the last element of the current measure
    isLastElem = (currElementIdx == score->measures[currMeasureIdx].GetNElements() - 1);
    if(IsEndOfMeasure() || (isLastElem && ignoreEndOfMeasure))
    {
        currElementIdx = 0;

        // in play mode we must take repeats into account and update the context
        if(score->mode == PLAY)
        {
            if(CheckRepeat())
            {
                currMeasureIdx = context->RestoreLastStartRepeat();
            }
            else
            {
                // we must skip all the endings that end a repeat
                // section that has been already performed
                do
                {
                    currMeasureIdx++;
                }while(score->measures[currMeasureIdx].IsEnding() &&
                       score->measures[currMeasureIdx].IsEndRepeat() &&
                       !CheckRepeat());

                // the context doesn't have to be updated with skipped endings
                context->Update(currMeasureIdx, score->measures[currMeasureIdx]);
            }
        }
        else // mode == EDIT
        {
            currMeasureIdx++;
        }
    }
    else
    {
        // we update the context with the accidentals of the previous element
        if(score->mode == PLAY)
        {
            context->Update(score->measures[currMeasureIdx].GetElement(currElementIdx));
        }

        if(isLastElem) // ignoreEndOfMeasure has been checked before
        {
            currElementIdx = INVALID_CURSOR;
        }
        else
        {
            currElementIdx++;
        }
    }

    return true;
}

template<template<typename> class MeasureType, typename MeasureElementType>
MeasureElementType*
	BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::GetCurrentElement()
{
    return const_cast<MeasureElementType*>(
        static_cast<const BaseMusicalScore<MeasureType, MeasureElementType>::Cursor &>
		(*this).GetCurrentElement());
}

template<template<typename> class MeasureType, typename MeasureElementType>
const MeasureElementType*
	BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::GetCurrentElement() const
{
    if(IsValid())
    {
        return &score->measures[currMeasureIdx].GetElement(currElementIdx);
    }
    return 0;
}

template<template<typename> class MeasureType, typename MeasureElementType>
MeasureType<MeasureElementType>*
	BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::GetCurrentMeasure()
{
    return const_cast<MeasureType<MeasureElementType>*>(
        static_cast<const BaseMusicalScore<MeasureType, MeasureElementType>::Cursor &>
		(*this).GetCurrentMeasure());
}

template<template<typename> class MeasureType, typename MeasureElementType>
const MeasureType<MeasureElementType>* 
	BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::GetCurrentMeasure() const
{
    if(IsEndOfScore())
    {
        return 0;
    }
    return &score->measures[currMeasureIdx];
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::HasNext(
	bool ignoreEndOfMeasure) const
{
    if(IsEndOfScore())
    {
        return false;
    }

    if(score->mode == PLAY && CheckRepeat())
    {
        return true;
    }

    // if in edit mode or if there are no repeats to perform
    return HasNextWritten(ignoreEndOfMeasure);
}

template<template<typename> class MeasureType, typename MeasureElementType>
void BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::InsertElementAfter(
	const MeasureElementType &element)
{
    MeasureType<MeasureElementType>* currMeasure; // convenience variable

    if(IsEndOfScore())
    {
        // we push a new measure
        MeasureType<MeasureElementType> measure;
        currMeasureIdx = score->measures.Push(measure);
        CS_ASSERT(IsEndOfMeasure());
    }

    currMeasure = &score->measures[currMeasureIdx];
    if(IsEndOfMeasure())
    {
        currElementIdx = currMeasure->PushElement(element);
        CS_ASSERT(IsValid());
    }
    else if(currElementIdx == currMeasure->GetNElements() - 1)
    {
        currMeasure->PushElement(element);
    }
    else
    {
        currMeasure->InsertElement(currElementIdx + 1, element);
    }
}

template<template<typename> class MeasureType, typename MeasureElementType>
void BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::InsertElementBefore(
	const MeasureElementType &element)
{
    if(IsEndOfScore())
    {
        // we push a new measure
        MeasureType<MeasureElementType> measure;
        currMeasureIdx = score->measures.Push(measure);
        CS_ASSERT(IsEndOfMeasure());
    }

    if(IsValid())
    {
        // the index of the current element increase after the insertion
        score->measures[currMeasureIdx].InsertElement(currElementIdx, element);
        currElementIdx++;
    }
    else // IsEndOfMeasure() == true
    {
        score->measures[currMeasureIdx].PushElement(element);
    }
}

template<template<typename> class MeasureType, typename MeasureElementType>
void BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::InsertMeasureAfter(
	const MeasureType<MeasureElementType> &measure)
{
    if(IsEndOfScore())
    {
        // we move the cursor to the first element of the newly inserted measure
        currMeasureIdx = score->measures.Push(measure);
        if(!measure.IsEmpty())
        {
            currElementIdx = 0;
        } // else the cursor is on end-of-measure
    }
    else if(currMeasureIdx == score->measures.GetSize() - 1)
    {
        // if this is the last measure we must use Push()
        score->measures.Push(measure);
    }
    else
    {
        score->measures.Insert(currMeasureIdx + 1, measure);
    }
}

template<template<typename> class MeasureType, typename MeasureElementType>
void BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::InsertMeasureBefore(
	const MeasureType<MeasureElementType> &measure)
{
    if(IsEndOfScore())
    {
        score->measures.Push(measure);
    }
    else
    {
        // the index of the current measure increases after the insertion
        score->measures.Insert(currMeasureIdx, measure);
        currMeasureIdx++;
    }
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::IsEndOfMeasure() const
{
    return currElementIdx == INVALID_CURSOR && currMeasureIdx != INVALID_CURSOR;
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::IsEndOfScore() const
{
    return currElementIdx == INVALID_CURSOR && currMeasureIdx == INVALID_CURSOR;
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::IsValid() const
{
    return currElementIdx != INVALID_CURSOR;
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::RemoveCurrentElement()
{
    if(!IsValid())
    {
        return false;
    }

    score->measures[currMeasureIdx].DeleteElement(currElementIdx);

    if(currElementIdx > 0)
    {
        currElementIdx--;
    }
    else if(score->measures[currMeasureIdx].IsEmpty())
    {
        currElementIdx = INVALID_CURSOR; // move to end-of-measure
    } // else the cursor is on the element after the removed one

    return true;
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::RemoveCurrentMeasure()
{
    if(IsEndOfScore())
    {
        return false;
    }

    score->measures.DeleteIndex(currMeasureIdx);

    if(score->measures.IsEmpty())
    {
        // move to end-of-score
        currElementIdx = INVALID_CURSOR;
        currMeasureIdx = INVALID_CURSOR;
    }
    else
    {
        if(currMeasureIdx > 0)
        {
            currMeasureIdx--;
        } // else the score is on the measure next to the removed one

        if(score->measures[currMeasureIdx].IsEmpty())
        {
            currElementIdx = INVALID_CURSOR; // move to end-of-measure
        }
        else
        {
            currElementIdx = 0;
        }
    }

    return true;
}

template<template<typename> class MeasureType, typename MeasureElementType>
void BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::Validate()
{
    if(IsValid() && currElementIdx >= score->measures[currMeasureIdx].GetNElements())
    {
        currElementIdx = INVALID_CURSOR;
    }
}

template<template<typename> class MeasureType, typename MeasureElementType>
BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::Cursor(
	BaseMusicalScore<MeasureType, MeasureElementType>* score_,
	typename BaseMusicalScore<MeasureType, MeasureElementType>::ScoreMode mode)
: context(0), score(score_)
{
    if(mode == PLAY)
    {
        context = new ScoreContext();
    }
    Reset();
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::CheckRepeat() const
{
    int nEndRepeat = score->measures[currMeasureIdx].GetNEndRepeat();
    return context->GetNPerformedRepeats(currMeasureIdx) < nEndRepeat;
}

template<template<typename> class MeasureType, typename MeasureElementType>
bool BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::HasNextWritten(
	bool ignoreEndOfMeasure) const
{
    bool isLastMeasure = (currMeasureIdx == score->measures.GetSize() - 1);

    CS_ASSERT(!IsEndOfScore());

    if(IsEndOfMeasure())
    {
        return isLastMeasure;
    }
    CS_ASSERT(IsValid());

    if(ignoreEndOfMeasure && isLastMeasure &&
       currElementIdx == score->measures[currMeasureIdx].GetNElements() - 1)
    {
        return false;
    }
    return true;
}

template<template<typename> class MeasureType, typename MeasureElementType>
void BaseMusicalScore<MeasureType, MeasureElementType>::Cursor::Reset()
{
    if(score->measures.GetSize() == 0)
    {
        // end-of-score
        currElementIdx = INVALID_CURSOR;
        currMeasureIdx = INVALID_CURSOR;
    }
    else if(score->measures[0].IsEmpty())
    {
        // end-of-measure
        currElementIdx = INVALID_CURSOR;
        currMeasureIdx = 0;
    }
    else
    {
        currElementIdx = 0;
        currMeasureIdx = 0;
    }
}
