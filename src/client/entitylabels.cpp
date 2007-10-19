/*
 * entitylabels.cpp - Author: Ondrej Hurt
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// CS INCLUDES
#include <psconfig.h>
#include <csutil/scf.h>
#include <csutil/csstring.h>
#include <csgeom/vector2.h>
#include <iutil/objreg.h>
#include <iengine/mesh.h>
#include <imesh/object.h>
#include <iengine/movable.h>
#include <ivideo/graph2d.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <cstool/objmodel.h>
#include <csutil/eventnames.h>

// CEL INCLUDES
#include <physicallayer/pl.h>
#include <behaviourlayer/behave.h>

// PS INCLUDES
#include "globals.h"
#include "gui/psmainwidget.h"
#include "paws/pawsmanager.h"
#include "util/psxmlparser.h"
#include "util/log.h"
#include "util/psxmlparser.h"
#include "entitylabels.h"
#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"
#include "effects/pseffectobj.h"
#include "effects/pseffectobjtextable.h"


#define LABEL_FONT             "/this/data/ttf/LiberationSans-Regular.ttf"
#define SPRITE_PLUGIN_NAME     "crystalspace.mesh.object.sprite.2d"
#define LABEL_MESH_NAME        "EntityLabel"
#define ENTITY_LABEL_SPACING   5       // horizontal space between label parts
#define LABEL_MARGIN           2
#define CONFIG_FILE_NAME       "/this/data/options/entitylabels.xml"
#define CONFIG_FILE_NAME_DEF   "/this/data/options/entitylabels_def.xml"

#define SCALE    0.004
#define BORDER_SIZE 2

int MakeColor(csPixelFormat * fmt, int r, int g, int b);
int ParseColor(const csString & str, iGraphics2D *g2d);

psEntityLabels::psEntityLabels()
{    
    visibility = LABEL_ALWAYS;
    showGuild = true;
    
    underMouse = NULL;
}

psEntityLabels::~psEntityLabels()
{
    if (eventhandler)
        eventQueue->RemoveListener(eventhandler);    
}

bool psEntityLabels::Initialize(iObjectRegistry * object_reg, psCelClient * _celClient)
{
    celClient  = _celClient;
    
    vfs = psengine->GetVFS();
    CS_ASSERT(vfs);
    
    eventhandler.AttachNew(new EventHandler(this));

    eventQueue =  csQueryRegistry<iEventQueue> (object_reg);
    if(!eventQueue)
    {
        Error1("No iEventQueue found!");
        CS_ASSERT(eventQueue);
        return false;
    }

    csEventID esub[] = {
      csevFrame (object_reg),
      //csevMouseEvent (object_reg),
      CS_EVENTLIST_END
    };
    eventQueue->RegisterListener(eventhandler, esub);

    LoadFromFile();
    return true;
}

void psEntityLabels::Configure(psEntityLabelVisib _visibility, bool _showGuild)
{
    // Hide all on changed visibility to refresh what we're showing
    if (visibility != _visibility)
        HideAllLabels();

    bool refreshGuilds = (showGuild != _showGuild);

    visibility = _visibility;
    showGuild = _showGuild;

    // Refresh guild labels if our display option has changed
    if (refreshGuilds)
        RefreshGuildLabels();
}

void psEntityLabels::GetConfiguration(psEntityLabelVisib & _visibility, bool & _showGuild)
{
    _visibility = visibility;
    _showGuild = showGuild;
}

bool psEntityLabels::HandleEvent(iEvent & ev)
{
    static unsigned int count = 0;
    if (++count%10 != 0)  // Update once every 10th frame
        return false;

    if (celClient->GetMainPlayer() == NULL)
        return false;  // Not loaded yet

    if (visibility == LABEL_ALWAYS)
    {
        UpdateVisibility();
    }
    else if (visibility == LABEL_ONMOUSE)
    {
        UpdateMouseover();
    }

    return false;
}

void psEntityLabels::SetObjectText(GEMClientObject* object)
{
    if ( !object )
    {
        Warning1( LOG_ANY, "NULL object passed..." );
        return;
    }

    int colour = 0xff0000;  // Default color, on inanimate objects
    if ( object->IsAlive() )
    {
        int flags = object->Flags();
        bool invisible = flags & psPersistActor::INVISIBLE;
        
        if ( invisible )
        {
            colour = 0xffffff;                    
        }
        else
        {
            int type = object->GetMasqueradeType();
            if (type > 26)
                type = 26;

            switch ( type )    
            {
                case 0: // player
                    colour = 0x00ff00;
                    break;

                case -1: // NPC
                    colour = 0x00ffff;
                    break;

                default:
                case 21: // GM1 or unknown group
                    colour = 0x008000;
                    break;            

                case 22:
                case 23:
                case 24:
                case 25: // GM2-5
                    colour = 0xffff80;
                    break;

                case 26: // dev char
                    colour = 0xff8080;
                    break;
            }                    
        }
    }

    // Is our object an actor?
    GEMClientActor* actor = dynamic_cast<GEMClientActor*>(object);

    // Grouped with have other color
    if (actor && actor->IsGroupedWith(celClient->GetMainPlayer()))
      colour = 0x0080ff;

    psEffectTextRow nameRow;
    psEffectTextRow guildRow;

    nameRow.text = object->GetName();
    nameRow.align = ETA_CENTER;
    nameRow.colour = colour;
    nameRow.hasShadow = false;
    nameRow.hasOutline = true;
    nameRow.outlineColour = 0;

    if (actor && showGuild)
    {
        csString guild( actor->GetGuildName() );
        csString guild2( psengine->GetGuildName() );

        if ( guild.Length() )
        if ( guild.Length() )
        {
            // If same guild, indicate with color
            if (guild == guild2)
                colour = 0xf6dfa6;

            guildRow.text = "<" + guild + ">";
            guildRow.align = ETA_CENTER;
            guildRow.colour = colour;
            guildRow.hasShadow = false;
            guildRow.hasOutline = true;
            guildRow.outlineColour = 0;
        }
    }

    // Apply
    psEffect* entityLabel = object->GetEntityLabel();
    if(!entityLabel)
    {
        // Wierd case
        Error2("Lost entity label of object %s!",object->GetName());
        return;
    }

    psEffectObjTextable* txt = entityLabel->GetMainTextObj();
    if(!txt)
    {
        // Ill-modded effect
        Error1("Effect 'entitylabel' has no text object");
        return;
    }

	size_t nameCharCount = nameRow.text.Length();
	size_t guildCharCount = guildRow.text.Length();
	size_t maxCharCount = nameCharCount > guildCharCount ? nameCharCount : guildCharCount;
	float scale = sqrt((float)maxCharCount) / 4.0f;

    // Finally set the text, with a black outline
    if (guildRow.text.Length())
	{
        txt->SetText(2, &nameRow, &guildRow);
		scale *= 1.5f;
	}
    else
        txt->SetText(1, &nameRow);

    entityLabel->SetScaling(scale, 1.0f);
}

void psEntityLabels::CreateLabelOfObject(GEMClientObject *object)
{
    iMeshWrapper* mesh = object->pcmesh->GetMesh();

    // Has it got a mesh to attach to?
    if (!mesh || !mesh->GetMeshObject())
        return;

    // Get the height of the model
    const csBox3& boundBox = mesh->GetMeshObject()->GetObjectModel()->GetObjectBoundingBox();

    psEffectManager* effectMgr = psengine->GetEffectManager();

    // Create the effect
    unsigned int id = effectMgr->RenderEffect( "entitylabel", 
                                               csVector3(0.0f,boundBox.Max(1) + 0.25f,0.0f),
                                               mesh );

    psEffect* effect = effectMgr->FindEffect(id);
    object->SetEntityLabel(effect);
    effectMgr->ShowEffect(id,false);  // Hide until told to show

    // Update text
    SetObjectText(object);
}


void psEntityLabels::DeleteLabelOfObject(GEMClientObject* object)
{
    if(!object)
        return;

    if(object->GetEntityLabel())
        psengine->GetEffectManager()->DeleteEffect(object->GetEntityLabel()->GetUniqueID());

    object->SetEntityLabel(NULL);
}


void psEntityLabels::OnObjectArrived( GEMClientObject* object )
{
    CS_ASSERT_MSG("Effects Manager must exist before loading entity labels!", psengine->GetEffectManager() );

    if (!object)
    {
        Debug1( LOG_ANY, 0, "NULL object passed to psEntityLabels::OnObjectArried" );
        return;
    }

    // Action location?
    if (object->GetObjectType() == GEM_ACTION_LOC)
        return;

    DeleteLabelOfObject(object); // just to be sure
    if ( object->GetEntity() != celClient->GetMainActor() )
        CreateLabelOfObject( object );
}


bool psEntityLabels::LoadFromFile()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, entityLabelsNode, optionNode;
    csString option;
    
    csString fileName = CONFIG_FILE_NAME;
    if (!vfs->Exists(fileName))
    {
        fileName = CONFIG_FILE_NAME_DEF;
    }

    doc = ParseFile(psengine->GetObjectRegistry(), fileName);
    if (doc == NULL)
    {
        Error2("Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("entitylabels.xml has no XML root");
        return false;
    }
    entityLabelsNode = root->GetNode("EntityLabels");
    if (entityLabelsNode == NULL)
    {
        Error1("entitylabels.xml has no <EntityLabels> tag");
        return false;
    }
    
    optionNode = entityLabelsNode->GetNode("Visibility");
    if (optionNode != NULL)
    {
        option = optionNode->GetAttributeValue("value");
        if (option == "always")
            visibility = LABEL_ALWAYS;
        else if (option == "mouse")
            visibility = LABEL_ONMOUSE;
        else if (option == "never")
            visibility = LABEL_NEVER;
    }
    
    optionNode = entityLabelsNode->GetNode("ShowGuild");
    if (optionNode != NULL)
    {
        option = optionNode->GetAttributeValue("value");
        showGuild = (option == "yes");
    }

    return true;
}

inline void psEntityLabels::UpdateVisibility()
{
    csVector3 here = celClient->GetMainPlayer()->Pos();
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=1; i < entities.GetSize(); i++)  // Skip first entity, which is the main actor
    {
        GEMClientObject* object = entities.Get(i);

        iMeshWrapper* mesh = object->pcmesh->GetMesh();
        if (!mesh)
            continue;

        // Only show labels within range
        csVector3 there = mesh->GetMovable()->GetPosition();
        int range = (object->GetObjectType() == GEM_ITEM) ? RANGE_TO_SEE_ITEM_LABELS : RANGE_TO_SEE_ACTOR_LABELS ;
        bool show = ( (here-there).Norm() < range );

        ShowLabelOfObject(object,show);
    }
}

inline void psEntityLabels::UpdateMouseover()
{
    GEMClientObject* lastUnderMouse = underMouse;

    // Get mouse position
    psPoint mouse = PawsManager::GetSingleton().GetMouse()->GetPosition();

    // Find out the object
    underMouse = psengine->GetMainWidget()->FindMouseOverObject(mouse.x, mouse.y);

    // Is this a new object?
    if (underMouse != lastUnderMouse)
    {
        // Hide old
        if (lastUnderMouse != NULL)
            ShowLabelOfObject(lastUnderMouse,false);

        // Show new
        if (underMouse != NULL && underMouse->GetEntity() != celClient->GetMainActor())
        {
            iMeshWrapper* mesh = underMouse->pcmesh->GetMesh();
            if (mesh)
            {
                // Only show labels within range
                csVector3 here = celClient->GetMainPlayer()->Pos();
                csVector3 there = mesh->GetMovable()->GetPosition();
                int range = (underMouse->GetObjectType() == GEM_ITEM) ? RANGE_TO_SEE_ITEM_LABELS : RANGE_TO_SEE_ACTOR_LABELS ;
                bool show = ( (here-there).Norm() < range );

                ShowLabelOfObject(underMouse,show);
            }
        }
    }
}

bool psEntityLabels::SaveToFile()
{
    csString xml;
    csString visibilityStr, showGuildStr;

    switch (visibility)
    {
        case LABEL_ALWAYS:  visibilityStr = "always"; break;
        case LABEL_ONMOUSE: visibilityStr = "mouse"; break;
        case LABEL_NEVER:   visibilityStr = "never"; break;
    }
    showGuildStr = showGuild ? "yes" : "no";
    
    xml = "<EntityLabels>\n";
    xml += "    <Visibility value=\"" + visibilityStr + "\"/>\n";
    xml += "    <ShowGuild value=\"" + showGuildStr + "\"/>\n";
    xml += "</EntityLabels>\n";
    
    return vfs->WriteFile(CONFIG_FILE_NAME, xml.GetData(), xml.Length());
}

void psEntityLabels::RemoveObject( GEMClientObject* object )
{
    DeleteLabelOfObject(object);

    if (underMouse == object)
        underMouse = NULL;
}

inline void psEntityLabels::ShowLabelOfObject(GEMClientObject* object, bool show)
{
    psEffect* effect = object->GetEntityLabel();
    if (effect != NULL)
        psengine->GetEffectManager()->ShowEffect(effect->GetUniqueID(),show);
}

void psEntityLabels::RepaintAllLabels()
{
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=1; i < entities.GetSize(); i++)  // Skip first entity, which is the main actor
        RepaintObjectLabel( entities.Get(i) );
}

void psEntityLabels::HideAllLabels()
{
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=1; i < entities.GetSize(); i++)  // Skip first entity, which is the main actor
        ShowLabelOfObject( entities.Get(i), false );
}

void psEntityLabels::RefreshGuildLabels()
{
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=1; i < entities.GetSize(); i++)  // Skip first entity, which is the main actor
    {
        GEMClientActor* actor = dynamic_cast<GEMClientActor*>(entities.Get(i));
        if ( actor && csString(actor->GetGuildName()).Length() )
            OnObjectArrived(actor);
    }
}

void psEntityLabels::RepaintObjectLabel(GEMClientObject* object)
{
    if (object && object->GetObjectType() != GEM_ACTION_LOC)
        SetObjectText(object);
}

void psEntityLabels::LoadAllEntityLabels()
{
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=0; i<entities.GetSize(); i++)
        OnObjectArrived( entities.Get(i) );
}
