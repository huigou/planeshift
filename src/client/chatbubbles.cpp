/*
* Author: Andrew Robberts
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

#include <psconfig.h>
#include <iengine/movable.h>
#include <iengine/mesh.h>
#include <imesh/objmodel.h>
#include <imesh/object.h>
#include "chatbubbles.h"
#include "pscelclient.h"
#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"

//#define DISABLE_CHAT_BUBBLES


psChatBubbles::psChatBubbles()
             : psengine(0), msgHandler(0)
{
}

psChatBubbles::~psChatBubbles()
{
  if (msgHandler)
    msgHandler->Unsubscribe(this, MSGTYPE_CHAT);
}

bool psChatBubbles::Initialize(psEngine * psengine)
{
  this->psengine = psengine;
  msgHandler = psengine->GetMsgHandler();

  msgHandler->Subscribe(this, MSGTYPE_CHAT);
	Load("/this/data/options/chatbubbles.xml");
	return true;
}

bool psChatBubbles::Load(const char * filename)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root;
    csRef<iVFS> vfs;
    csRef<iDocumentSystem>  xml;
    const char* error;

	chatTypes.DeleteAll();

    vfs = psengine->GetVFS();
    assert(vfs);
    csRef<iDataBuffer> buff = vfs->ReadFile(filename);
    if (buff == NULL)
    {
        Error2("Could not find file: %s", filename);
        return false;
    }
    xml = psengine->GetXMLParser ();
    doc = xml->CreateDocument();
    assert(doc);
    error = doc->Parse( buff );
    if ( error )
    {
        Error3("Parse error in %s: %s", filename, error);
        return false;
    }
    if (doc == NULL)
        return false;

    root = doc->GetRoot();
    if (root == NULL)
    {
        Error2("No root in XML %s", filename);
        return false;
    }

	csRef<iDocumentNode> chatBubblesNode = root->GetNode("chat_bubbles");
	if (!chatBubblesNode)
	{
		Error2("No chat_bubbles node in %s", filename);
		return false;
	}

	bubbleMaxLineLen = chatBubblesNode->GetAttributeValueAsInt("maxLineLen");
	bubbleShortPhraseCharCount = chatBubblesNode->GetAttributeValueAsInt("shortPhraseCharCount");
	bubbleLongPhraseLineCount = chatBubblesNode->GetAttributeValueAsInt("longPhraseLineCount");

    csRef<iDocumentNodeIterator> nodes = chatBubblesNode->GetNodes("chat");
	while (nodes->HasNext())
	{
		csRef<iDocumentNode> chatNode = nodes->Next();

		int r, g, b;
		BubbleChatType chat = { CHAT_SAY, {"", 0, false, 0, false, 0, ETA_LEFT}, "chatbubble_"};

		csString type = chatNode->GetAttributeValue("type");
		type.Downcase();
		if (type == "say")
			chat.chatType = CHAT_SAY;
		else if (type == "tell")
			chat.chatType = CHAT_TELL;
		else if (type == "group")
			chat.chatType = CHAT_GROUP;
		else if (type == "guild")
			chat.chatType = CHAT_GUILD;
		else if (type == "auction")
			chat.chatType = CHAT_AUCTION;
		else if (type == "shout")
			chat.chatType = CHAT_SHOUT;
		else if (type == "me")
			chat.chatType = CHAT_ME;
		else if (type == "tellself")
			chat.chatType = CHAT_TELLSELF;
		else if (type == "my")
			chat.chatType = CHAT_MY;
		else if (type == "npc")
			chat.chatType = CHAT_NPC;
		else if (type == "npc_me")
			chat.chatType = CHAT_NPC_ME;
		else if (type == "npc_my")
			chat.chatType = CHAT_NPC_MY;
		else if (type == "npc_narrate")
			chat.chatType = CHAT_NPC_NARRATE;

		// color
		r = chatNode->GetAttributeValueAsInt("colorR");
		g = chatNode->GetAttributeValueAsInt("colorG");
		b = chatNode->GetAttributeValueAsInt("colorB");
		chat.textSettings.colour = psengine->GetG2D()->FindRGB(r, g, b);

		// shadow
		chat.textSettings.hasShadow = (chatNode->GetAttribute("shadowR") != 0);
		r = chatNode->GetAttributeValueAsInt("shadowR");
		g = chatNode->GetAttributeValueAsInt("shadowG");
		b = chatNode->GetAttributeValueAsInt("shadowB");
		chat.textSettings.shadowColour = psengine->GetG2D()->FindRGB(r, g, b);

		// outline
		chat.textSettings.hasOutline = (chatNode->GetAttribute("outlineR") != 0);
		r = chatNode->GetAttributeValueAsInt("outlineR");
		g = chatNode->GetAttributeValueAsInt("outlineG");
		b = chatNode->GetAttributeValueAsInt("outlineB");
		chat.textSettings.outlineColour = psengine->GetG2D()->FindRGB(r, g, b);

		// align
		csString align = chatNode->GetAttributeValue("align");
		align.Downcase();
		if (align == "right")
			chat.textSettings.align = ETA_RIGHT;
		else if (align == "center")
			chat.textSettings.align = ETA_CENTER;
		else
			chat.textSettings.align = ETA_LEFT;

		// prefix
		strcpy(chat.effectPrefix, chatNode->GetAttributeValue("effectPrefix"));

		chatTypes.Push(chat);
	}
  return true;
}

bool psChatBubbles::Verify(MsgEntry * msg, unsigned int flags, Client *& client)
{
  return true;
}

void psChatBubbles::HandleMessage(MsgEntry * msg, Client * client)
{
#ifdef DISABLE_CHAT_BUBBLES
    return;
#endif
    
  size_t a;
  if (msg->GetType() != MSGTYPE_CHAT)
    return;

  psChatMessage chatMsg(msg);

  // Get the first name of the person (needed for NPCs with both the first and the last name)
  csString firstName;
  if ((a = chatMsg.sPerson.FindFirst(' ')) != (size_t)-1)
      firstName = chatMsg.sPerson.Slice(0, a);
  else
      firstName = chatMsg.sPerson;

  GEMClientActor* actor = psengine->GetCelClient()->GetActorByName(firstName);
  if (!actor)
    return;

  // get the template for the chat type
  const BubbleChatType* type = 0;
  size_t len = chatTypes.GetSize();
  for (a=0; a<len; ++a)
  {
      if (chatMsg.iChatType == chatTypes[a].chatType)
      {
          type = &chatTypes[a];
          break;
      }
  }
  if (!type)
      return;

  static csArray<psEffectTextRow> rowBuffer;
  rowBuffer.Empty();

  
  // build the text rows
  const size_t textLen = chatMsg.sText.Length();
  const char * text = chatMsg.sText.GetData();
  psEffectTextRow chat = type->textSettings;

  // create a row character by character while calculating word wrap
  char line[256];
  int linePos = 0;
  int maxRowLen = 0;
  int lastSpace = 0;
  for (a=0; a<textLen; ++a)
  {
      if (text[a] == ' ' && linePos == 0)
          continue;
      
      if (text[a] == ' ')
          lastSpace = linePos;

      line[linePos] = text[a];
      ++linePos;

      if (linePos < (int)bubbleMaxLineLen)
          continue;
      
      if (lastSpace == 0)
      {
          line[linePos-1] = '-';
          --a;
      }
      else if (text[a] != ' ')
      {
          a -= linePos - lastSpace;
          linePos = lastSpace;
      }

      if (linePos > maxRowLen)
          maxRowLen = linePos;

      lastSpace = 0;
      line[linePos] = 0;
      linePos = 0;
      chat.text = line;
      rowBuffer.Push(chat);
  }
  if (linePos > 0)
  {
      if (linePos > maxRowLen)
          maxRowLen = linePos;

      line[linePos] = 0;
      chat.text = line;
      rowBuffer.Push(chat);
  }

  // decide on good effect size
  csString effectName = type->effectPrefix;
  if (rowBuffer.GetSize() == 1 && maxRowLen <= (int)bubbleShortPhraseCharCount)
      effectName += "shortphrase";
  else if (rowBuffer.GetSize() > bubbleLongPhraseLineCount)
      effectName += "longphrase";
  else
      effectName += "normal";

  // create the effect
  psEffectManager * effectManager = psengine->GetEffectManager();
  iMeshWrapper * mesh = actor->pcmesh->GetMesh();
  const csBox3& boundBox = mesh->GetMeshObject()->GetObjectModel()->GetObjectBoundingBox();
  unsigned int bubbleID = effectManager->RenderEffect("chatbubble", //effectName, 
          csVector3(0, boundBox.Max(1) + 0.5f, 0), mesh);
  
  // delete existing bubble if it exists
  psEffect * bubble = effectManager->FindEffect(actor->GetChatBubbleID());
  if (bubble)
      effectManager->DeleteEffect(actor->GetChatBubbleID());
  
  bubble = psengine->GetEffectManager()->FindEffect(bubbleID);
  if (!bubble)
  {
      Error2("Couldn't create effect '%s'", effectName.GetData());
      return;
  }

  actor->SetChatBubbleID(bubbleID);

  psEffectObjTextable * txt = bubble->GetMainTextObj();
  if (!txt)
  {
      Error2("Effect '%s' has no text object", effectName.GetData());
      return;
  }
  txt->SetText(rowBuffer);
}
