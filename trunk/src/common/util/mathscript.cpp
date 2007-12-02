/*
 * mathscript.cpp by Keith Fulton <keith@planeshift.it>
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "psconst.h"

#if defined(CS_EXTENSIVE_MEMDEBUG) || defined(CS_MEMORY_TRACKER)
# undef new
# if defined(CS_EXTENSIVE_MEMDEBUG)
#  undef CS_EXTENSIVE_MEMDEBUG
# endif
# if defined(CS_MEMORY_TRACKER)
#  undef CS_MEMORY_TRACKER
# endif
#endif
#include <new>

#include <ctype.h>

#include "util/psstring.h"
#include "util/log.h"
#include <csutil/array.h>
#include <csutil/hash.h>
#include <csutil/xmltiny.h>

#include "../server/globals.h"
#include "psdatabase.h"

#include "util/mathscript.h"
#include "util/strutil.h"
#include "util/consoleout.h"

MathScriptLine::MathScriptLine(const char *line,MathScript *myScript)
{
    scriptLine = line;
    assignee = 0;

    size_t start = scriptLine.FindFirst('=');
    if (start == SIZET_NOT_FOUND)
        start = 0;
    else
    {
        //csString varWithEq = GetWordNumber(scriptLine,1);
        scriptLine[start] = ' ';
        csString var = GetWordNumber(scriptLine,1);
        //CS_ASSERT_MSG("mathscript line has previously invalid operator spacing", var == varWithEq);
        var.Trim();

        bool validAssignee = true;
        for (size_t a=0; a<var.Length(); ++a)
        {
            char c = var[a];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            {
                validAssignee = false;
                break;
            }
        }
        if (validAssignee)
        {
        assignee = myScript->GetOrCreateVar(var);
            scriptLine.DeleteAt(0,start+1);
        }
        else
            scriptLine[start] = '='; // restore the = we removed
    }

    valid = true;

    var_array = NULL;

    fp.AddFunction("rnd", MathScriptEngine::RandomGen, 1);
    fp.AddFunction("pow", MathScriptEngine::Power, 2);
    for (unsigned int a=2; a<12; ++a)
    {
        psString funcName = "customCompoundFunc";
        funcName += a;
        fp.AddFunction(funcName.GetData(), MathScriptEngine::CustomCompoundFunc, a);
    }

    ParseVariables(myScript);
    ParseFormula(myScript);
}

MathScriptLine::~MathScriptLine()
{
    if (var_array)
        delete[] var_array;
}

MathScriptVar *MathScriptLine::FindVariable(const char *name)
{
    size_t i;
    for (i=0; i<variables.GetSize(); i++)
    {
        if (variables[i]->name == name)
            return variables[i];
    }
    return NULL;
}

void MathScriptLine::ParseVariables(MathScript *myScript)
{
    size_t start=0;
    psString word;

    while ( start < scriptLine.Length() )
    {
        scriptLine.GetWord(start,word,false);
        if (!word.Length())
        {
            start++;
            continue;
        }
        if (isupper(word.GetAt(0))) // capital first letter means var name
        {
            // A compound var function starts as Target:WeaponAt(Slot)
            // and turns into customCompoundFuncA(B,Target,Slot)
            // where N is the number of parameters and B is the index of the
            // function name in the lookup table
            // A compound var turns Target::HP into Target_HP
            if (scriptLine.Length() > start+word.Length() && scriptLine[start+word.Length()] == ':')  // compound var name
            {
                MathScriptVar *base = FindVariable(word);
                // get core variable onto list
                if (!base) // unique list
                {
                    base = myScript->GetOrCreateVar(word);
                    variables.Push( base );
                }
                base->type = MathScriptVar::VARTYPE_PTR;
                psString extend;
                scriptLine.GetWord(start+word.Length()+2,extend,false);

                if (scriptLine.Length() > start+word.Length()+extend.Length()+1 && scriptLine[start+word.Length()+1+extend.Length()] == '(')
                {
                    size_t paramCount = 2;
                    size_t endFunc = scriptLine.FindFirst(')',start);
                    if (scriptLine[endFunc-1] != '(')
                        ++paramCount;
                    for (size_t a=start; a<endFunc; ++a)
                        paramCount += scriptLine[a] == ',' ? 1 : 0;

                    scriptLine.DeleteAt(start,word.Length()+2+extend.Length());
                    csString funcPrefix = "customCompoundFunc";
                    funcPrefix += paramCount;
                    funcPrefix += '(';
                    funcPrefix += (unsigned int)MathScriptEngine::customCompoundFunctions.GetSize();
                    funcPrefix += ',';
                    funcPrefix += word;
                    if (paramCount > 2)
                        funcPrefix += ',';
                    scriptLine.Insert(start, funcPrefix);
                    MathScriptEngine::customCompoundFunctions.Push(extend);
                    /*
                    word.Append(',');
                    scriptLine.Insert(start+extend.Length()+1,word);
                    */
                    word = "";
                }
                else
                {
                    MathScriptVar *var = myScript->GetOrCreateVar(word); // get underlying var

                    scriptLine.SetAt(start+word.Length(),'_');
                 
                    word.Append('_');
                    word.Append(extend);
        
                    if (!FindVariable(word)) // unique list
                    {
                        MathScriptVar *ext = myScript->GetOrCreateVar(word);
                        variables.Push( ext );
                        ext->property = extend;
                        ext->SetVariable(var);
                    }
                }
            }
            else if (!FindVariable(word)) // unique list
                variables.Push( myScript->GetOrCreateVar(word) );
        }
        start += word.Length();
    }
    if (variables.GetSize())
        var_array = new double[variables.GetSize()];
}

void MathScriptLine::ParseFormula(MathScript *myScript)
{
    csString varlist;
    varlist = "";
    if (variables.GetSize())
    {
        varlist = variables[0]->name;
        size_t i;
        for (i=1; i<variables.GetSize(); i++)
        {
            varlist.Append(',');
            varlist.Append(variables[i]->name);
        }
    }
    size_t ret = fp.Parse( scriptLine.GetData(), varlist.GetData() );
    if (ret != (size_t)-1)
    {
        valid=false;
        printf( "Caller: %s", myScript->name.GetData());
        Error4("Error in Col %d: %s\n Script: \"%s\" ",(int)ret,fp.ErrorMsg(),scriptLine.GetData() );
    }
    else
        fp.Optimize();
}

void MathScriptLine::Execute()
{
    if (!valid)
    {
        Error2("Attempted to execute bad Mathscript line (%s).\n",scriptLine.GetData());
        if (assignee)
            assignee->SetValue(0);
        return;
    }

    size_t i;
    for (i=0; i<variables.GetSize(); i++)
        var_array[i] = variables[i]->GetValue();

    double ret = fp.Eval(var_array);
    if (assignee)
        assignee->SetValue(ret);
}




MathScript::MathScript(iDocumentNode *node)
{
    name = node->GetAttributeValue("name");
    ParseScript( node->GetContentsValue() );
}

MathScript::MathScript(const char *myname,const char *script)
{
    name = myname;
    ParseScript(script);
}

void MathScript::ParseScript(const char *parsescript)
{
    psString script(parsescript);
    uintptr_t  i,line_start = 0;

    while (line_start < script.Length() )
    {
        i = script.FindFirst(';',line_start);
        if (i==SIZET_NOT_FOUND)
        {
            i = script.Length();
        }
        psString line;
        if (i-line_start > 0)
        {

            script.SubString(line,line_start,i-line_start);

            // avoids empty lines (spaces, tabs, carriage returns)
            line.ReplaceAllSubString("\t"," ");
            line.ReplaceAllSubString("\r"," ");
            line.ReplaceAllSubString("\n"," ");
            line = line.Trim();
            if (!line.IsEmpty()) {
                MathScriptLine *newline = new MathScriptLine(line,this);
                scriptLines.Push(newline);
            }
        }
        line_start = i+1;
    }
}

MathScript::~MathScript()
{
    while (scriptLines.GetSize())
        delete scriptLines.Pop();

    csHash<MathScriptVar *>::GlobalIterator it (variables.GetIterator ());
    while (it.HasNext ())
    {
        MathScriptVar* var = it.Next ();
        delete var;
    }
}

MathScriptVar *MathScript::GetVar(const char *name)
{
    unsigned int key = csHashCompute(name);
    csHash<MathScriptVar*>::Iterator iter = variables.GetIterator(key);
    while (iter.HasNext())
    {
        MathScriptVar *ptr = iter.Next();
        if (ptr->name == name)
            return ptr;
    }
    return NULL;
}

MathScriptVar *MathScript::GetOrCreateVar(const char *name)
{
    MathScriptVar *var = GetVar(name);
    if (var)
        return var;

    // CPrintf(CON_DEBUG, "Creating var <%s> in script <%s>\n",name,this->name.GetData() );

    unsigned int key = csHashCompute(name);
    var = new MathScriptVar;
    var->name = name;
    var->SetValue(0);

    variables.Put(key,var);
    return var;
}

void MathScript::Execute()
{
    size_t i;
    MathScriptVar *exitsignal = GetVar("exit");

    if (exitsignal)
        exitsignal->SetValue(0); // clear exit condition before running

    for (i=0; i<scriptLines.GetSize(); i++)
    {
        scriptLines[i]->Execute();
        if (exitsignal && exitsignal->GetValue()!=0)
        {
            // printf("Terminating mathscript at line %d of %d.\n",i, scriptLines.GetSize());
            break;
        }
    }
}


void MathScript::DumpAllVars()
{
    csHash<MathScriptVar*>::GlobalIterator iter = variables.GetIterator();

    CPrintf(CON_DEBUG, "\nAll vars for '%s'\n-----------------------------------------\n",
            name.GetData());
    while (iter.HasNext())
    {
        MathScriptVar *var = iter.Next();
        CPrintf(CON_DEBUG, "%25s=%1.4f\n",var->name.GetData(),var->GetValue() );
    }
}

MathScriptEngine::MathScriptEngine()
{
    Result result_events(db->Select("SELECT * from math_scripts"));

    if ( result_events.IsValid() )
    {
        for ( size_t index = 0; index < result_events.Count(); index++ )
        {
            unsigned long x = (unsigned long)index; // remove casting warnings

            MathScript *scr = new MathScript(result_events[x]["name"], result_events[x]["math_script"]);
            unsigned int key = csHashCompute(scr->name);
            scripts.Put(key,scr);
		}
	}
}

MathScriptEngine::~MathScriptEngine()
{
    csHash<MathScript *>::GlobalIterator it (scripts.GetIterator ());
    while (it.HasNext ())
    {
        MathScript* script = it.Next ();
        delete script;
    }
    scripts.DeleteAll();
    
    MathScriptEngine::customCompoundFunctions.DeleteAll();
}

MathScript *MathScriptEngine::FindScript(const char *name)
{
    unsigned int key = csHashCompute(name);
    csHash<MathScript*>::Iterator iter = scripts.GetIterator(key);
    while (iter.HasNext())
    {
        MathScript *ptr = iter.Next();
        if (ptr->name == name)
            return ptr;
    }
    return NULL;
}

csRandomGen MathScriptEngine::rng;
csArray<csString> MathScriptEngine::customCompoundFunctions;

double MathScriptEngine::RandomGen(const double *dummy)
{
    return MathScriptEngine::rng.Get();
}

double MathScriptEngine::Power(const double *parms)
{
    return pow(parms[0],parms[1]);
}

double MathScriptEngine::CustomCompoundFunc(const double * parms)
{
    size_t funcIndex = (size_t)parms[0];
    iScriptableVar * v = (iScriptableVar *)(intptr_t)parms[1];
    return v->CalcFunction(customCompoundFunctions[funcIndex], &parms[2]);
}
