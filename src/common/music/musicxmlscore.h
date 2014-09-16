/*
 * musicxmlscore.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef MUSIC_XML_SCORE_H
#define MUSIC_XML_SCORE_H


//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================

//====================================================================================
// Local Includes
//====================================================================================
#include "scoreelements.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------

/**
* \addtogroup common_music
* @{ */

/**
 * Extends the class MeasureElement by adding the ability to parse MusicXML.
 */
class MusicXMLElement: public MeasureElement
{
public:
    /**
     * Load a note from the given \<note\> node. If this element contain already one or
     * more note, only notes that present the \<chord/\> tag are accepted.
     *
     * @param noteNode a reference to the \<note\> node containing the definition of the
     * note in MusicXML syntax.
     * @param divisions divisions per quarter used by the score to indicate duration.
     * @return false if the given node syntax is wrong or if this note is not part of
     * this chord, true otherwise.
     */
    bool LoadXMLNote(const csRef<iDocumentNode> &noteNode, int divisions);

    /**
     * Convert the note to XML.
     * @return the string containing the note definition in MusicXML syntax.
     */
    csString ToXML();
};

#include "musicxmlscore.hpp"

/**
 * @} */

#endif // MUSIC_XML_SCORE_H
