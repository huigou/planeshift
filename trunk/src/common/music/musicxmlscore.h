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
 * Extends the class Note by adding the ability to parse MusicXML.
 */
class MusicXMLNote: public Note
{
public:
    /**
     * Load the note step, note and accidental from the given <pitch> node.
     *
     * @param pitchNode a valid pointer to the <pitch> node containing the definition of
     * the note in MusicXML syntax.
     * @return false if the given node syntax is wrong, true otherwise.
     */
    bool LoadXML(csRef<iDocumentNode> &pitchNode);

    /**
     * Convert the note to XML.
     * @return the string containing the note definition in MusicXML syntax.
     */
    csString ToXML();
};

#endif // MUSIC_XML_SCORE_H
