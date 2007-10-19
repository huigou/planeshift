/*
 * localization.cpp - Author: Ondrej Hurt
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
#include <iutil/document.h>
#include <iutil/vfs.h>

// COMMON INCLUDES
#include "util/log.h"

// CLIENT INCLUDES
#include "localization.h"



#define DEFAULT_FILE_PATH "/this/"


csRef<iDocument> ParseFile(iObjectRegistry* object_reg, const csString & name);


psLocalization::psLocalization()
{
    object_reg = NULL;
}

psLocalization::~psLocalization()
{
    ClearStringTable();
}

void psLocalization::Initialize(iObjectRegistry* _object_reg)
{
    object_reg = _object_reg;
}

void psLocalization::SetLanguage(const csString & _lang)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, tblRoot, item;
    csRef<iDocumentNodeIterator> itemIter;
    psStringTableItem * itemData;
    csString fileName;

    assert(object_reg != NULL);

    ClearStringTable();
    lang = _lang;

    fileName = "/this/lang/" + lang + "/stringtable.xml";

    // If the file is not available at all, we will just silently quit because it is not required.
    // But any other errors will be reported.
    if ( ! FileExists(fileName) )
        return;

    doc = ParseFile(object_reg, fileName);
    if (doc == NULL)
    {
        Error2("Parsing of string table file %s failed", fileName.GetData());
        return;
    }
    root = doc->GetRoot();
    if (root == NULL)    
    {
        Error2("String table file %s has no root", fileName.GetData());
        return;
    }

    tblRoot = root->GetNode("StringTable");
    if (tblRoot == NULL)
    {
        Error2("String table file %s must have <StringTable> tag", fileName.GetData());
        return;
    }

    itemIter = tblRoot->GetNodes("item");
    while (itemIter->HasNext())
    {
        item = itemIter->Next();

        itemData = new psStringTableItem;
        itemData->orig   =  item->GetAttributeValue("orig");
        itemData->trans  =  item->GetAttributeValue("trans");
        stringTbl.Put(itemData->orig.GetData(), itemData);
    }
}


csString psLocalization::FindLocalizedFile(const csString & shortPath)
{
    csString fullPath;
    
    fullPath = "/this/lang/";
    fullPath += lang;
    fullPath += "/";
    fullPath += shortPath;
    if (FileExists(fullPath))
        return fullPath;
    else
    {    
        fullPath = DEFAULT_FILE_PATH + shortPath;
        return fullPath;
    }
}

csString psLocalization::Translate(const csString & orig)
{
    psStringTableItem * item;
    psStringTableHash::Iterator iter = stringTbl.GetIterator(orig.GetData());
    
    while (iter.HasNext())
    {
        item = (psStringTableItem*)iter.Next();
        if (item->orig == orig)
            return item->trans;
    }
    return orig;
}

bool psLocalization::FileExists(const csString & fileName)
{
    csRef<iVFS> vfs;

    vfs =  csQueryRegistry<iVFS > ( object_reg);
    assert(vfs);
    return vfs->Exists(fileName);
}

void psLocalization::ClearStringTable()
{
    psStringTableItem * item;
    psStringTableHash::GlobalIterator iter = stringTbl.GetIterator();
    
    while (iter.HasNext())
    {
        item = (psStringTableItem*)iter.Next();
        delete item;
    }
    stringTbl.DeleteAll();
}
