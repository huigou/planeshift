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

bool MusicXMLNote::LoadXML(csRef<iDocumentNode> &pitchNode)
{
    csRef<iDocumentNode> stepNode = pitchNode->GetNode("step");
    csRef<iDocumentNode> alterNode = pitchNode->GetNode("alter");
    csRef<iDocumentNode> octaveNode = pitchNode->GetNode("octave");

    if(!stepNode.IsValid() || !octaveNode.IsValid())
    {
        return false;
    }

    char step = stepNode->GetContentsValue()[0];
    if(step < 'A' || step > 'G')
    {
        return false;
    }
    this->SetName(step);

    if(!alterNode.IsValid())
    {
        this->SetWrittenAccidental(NO_ACCIDENTAL);
    }
    else // the alter node is optional
    {
        int alter = alterNode->GetContentsValueAsInt();
        switch(alter)
        {
        case 0:
            this->SetWrittenAccidental(NO_ACCIDENTAL);
            break;
        case 1:
            this->SetWrittenAccidental(SHARP);
            break;
        case -1:
            this->SetWrittenAccidental(FLAT);
            break;
        default:
            return false;
        }
    }

    int octave = octaveNode->GetContentsValueAsInt();
    if(octave < 0 || octave > 9) // octave limits according to xsd specification
    {
        return true;
    }
    this->SetOctave(octave);

    return true;
}

csString MusicXMLNote::ToXML()
{
    int alterXML = 0;
    Accidental alter = this->GetWrittenAccidental();
    switch(alter)
    {
    case SHARP:
        alterXML = 1;
        break;
    case FLAT:
        alterXML = -1;
        break;
    }

    csString pitch("<pitch>");
    pitch.AppendFmt("<step>%c</step>", this->GetName());
    if(alterXML != 0)
    {
        pitch.AppendFmt("<alter>%d</alter>", alterXML);
    }
    pitch.AppendFmt("<octave>%d</octave></pitch>", this->GetOctave());
    return pitch;
}

