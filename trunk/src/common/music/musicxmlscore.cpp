/*
 * musicxmlscore.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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

#include "musicxmlscore.h"

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


bool MusicXMLElement::LoadXMLNote(const csRef<iDocumentNode> &noteNode, int divisions)
{
    char step;
    int octave;
    int duration;
    Accidental accidental = NO_ACCIDENTAL;
    csRef<iDocumentNode> durationNode;
    csRef<iDocumentNode> pitchNode;
    csRef<iDocumentNode> stepNode;
    csRef<iDocumentNode> octaveNode;
    csRef<iDocumentNode> accidentalNode;

    if(!noteNode.IsValid())
    {
        return false;
    }

    // Check that this note belongs to this element
    if(this->GetNNotes() > 0 && !noteNode->GetNode("chord").IsValid())
    {
        return false;
    }

    // Loading duration
    durationNode = noteNode->GetNode("duration");
    if(!durationNode.IsValid())
    {
        return false;
    }
    duration = durationNode->GetContentsValueAsInt();
    if(duration <= 0)
    {
        return false;
    }
    duration = duration * QUARTER_DURATION / divisions; // convert duration from quarters to sixteenths
    if(!CheckDuration(duration))
    {
        return false;
    }
    this->SetDuration(static_cast<Duration>(duration));

    // If this is a rest, we don't need to do anything else
    if(noteNode->GetNode("rest").IsValid())
    {
        return true;
    }

    // Loading name and octave
    pitchNode = noteNode->GetNode("pitch");
    if(!pitchNode.IsValid())
    {
        return false;
    }

    stepNode = pitchNode->GetNode("step");
    octaveNode = pitchNode->GetNode("octave");
    if(!stepNode.IsValid() || !octaveNode.IsValid())
    {
        return false;
    }

    step = stepNode->GetContentsValue()[0];
    if(step < 'A' || step > 'G')
    {
        return false;
    }

    octave = octaveNode->GetContentsValueAsInt();
    if(octave < 0 || octave > 9) // octave limits according to xsd specification
    {
        return true;
    }

    // Loading accidental. Right now it considers both <alter> and <accidental> as the
    // written accidental of the note (with priority to <accidental>). This is wrong but
    // it's needed for backwards compatibility. In the future, only <accidental> should
    // be parsed.
    accidentalNode = noteNode->GetNode("accidental");
    if(accidentalNode.IsValid())
    {
        csString accidentalStr = accidentalNode->GetContentsValue();
        if(accidentalStr == "flat")
        {
            accidental = FLAT;
        }
        else if(accidentalStr == "sharp")
        {
            accidental = SHARP;
        }
        else
        {
            return false;
        }
    }
    else
    {
        csRef<iDocumentNode> alterNode = pitchNode->GetNode("alter");
        if(alterNode.IsValid())
        {
            int alter = alterNode->GetContentsValueAsInt();
            switch(alter)
            {
            case 0:
                accidental = NO_ACCIDENTAL;
                break;
            case 1:
                accidental = SHARP;
                break;
            case -1:
                accidental = FLAT;
                break;
            default:
                return false;
            }
        }
    }

    this->AddNote(step, octave, accidental);

    return true;
}

csString MusicXMLElement::ToXML()
{
    csString element("<note>");

    if(this->IsRest())
    {
        element.AppendFmt("<rest/><duration>%d</duration></note>", this->GetDuration());
        return element;
    }

    for(size_t i = 0; i < this->GetNNotes(); i++)
    {
        Note note = this->GetNote(i);

        if(i > 0)
        {
            element += "<note><chord/>";
        }

        element.AppendFmt("<pitch><step>%c</step><octave>%d</octave></pitch><duration>%d</duration>",
                          note.GetName(), note.GetOctave(), this->GetDuration());
        switch(note.GetWrittenAccidental())
        {
        case SHARP:
            element += "<accidental>sharp</accidental>";
            break;
        case FLAT:
            element += "<accidental>flat</accidental>";
            break;
        default:
            break;
        }
        element += "</note>";
    }
    return element;
}

