/*
 * mathscript.h by Keith Fulton <keith@planeshift.it>
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
#ifndef __MATHSCRIPT_H__
#define __MATHSCRIPT_H__

#include <csutil/randomgen.h>
#include <../tools/fparser/fparser.h>
#include <csutil/hash.h>
#include <util/psstring.h>
#include <util/scriptvar.h>

class MathScriptVar
{
protected:
    double value;
    iScriptableVar *obj;
    MathScriptVar *var;

    typedef void (*MathScriptVarCallback)(void * arg);
    MathScriptVarCallback changedVarCallback;
    void * changedVarCallbackArg;

public:
    enum 
    { 
        VARTYPE_VALUE,
        VARTYPE_PTR,
        VARTYPE_VAR
    };
    int type;
    csString name;
    csString property;

    MathScriptVar()
    {
        type  = VARTYPE_VALUE;
        value = 0;
        obj   = NULL;
        var   = NULL;
        changedVarCallback = 0;
        changedVarCallbackArg = 0;        
    }
    void SetChangedCallback(MathScriptVarCallback callback, void * arg)
    {
        changedVarCallback = callback;
        changedVarCallbackArg = arg;
    }

    double GetValue()
    {
        if (type == VARTYPE_PTR)
        {
            if (property.Length() )
                return obj->GetProperty(property);
            else
                return (double)(intptr_t)obj;
        }
        else if (type == VARTYPE_VAR)
        {
            if (property.Length() )
                if (var->obj)
                    return var->obj->GetProperty(property);
            return 0;
        }
        else
            return value;
    }
    void SetValue(double v)
    { 
        value = v; 
        if (changedVarCallback)
            changedVarCallback(changedVarCallbackArg);
    }

    void SetObject(iScriptableVar *p)
    {
        type = VARTYPE_PTR;
        obj  = p;
    }
    void SetVariable(MathScriptVar *v)
    {
        type = VARTYPE_VAR;
        var = v;
    }
    void Copy(MathScriptVar *v)
    {
        type = v->type;
        value = v->value;
        var = v->var;
        obj = v->obj;
        name = v->name;
        property = v->property;
    }

    csString Dump() const
    {
        csString str;
        str.Append(name);
        str.Append("(");
        switch (type)
        {
        case VARTYPE_VALUE:
            str.Append("VAL) = ");
            str.Append(value);
            break;
        case VARTYPE_PTR:
            str.Append("PTR) = ");
            str.Append("PTR");
            break;
        case VARTYPE_VAR:
            str.Append("VAR) = ");
            str.Append(var->Dump());
            break;
        }
        return str;
    }
    
};

class MathScript;

/**
 * This holds one line of a (potentially) multi-line script.
 * It knows what <var> is on the left, and what <formula> is 
 * on the right of the = sign.  When run, it executes the
 * formula and sets the result in the Var.  These vars are 
 * shared across Lines, which means the next Line can use
 * that Var as an input.
 */
class MathScriptLine
{
protected:
    FunctionParser fp;
    double *var_array;
    bool valid;
    
    void ParseVariables(MathScript *myScript);
    void ParseFormula(MathScript *myScript);
    MathScriptVar *FindVariable(const char *name);

public:
    psString scriptLine;
    csArray<MathScriptVar*> variables;
    MathScriptVar *assignee;

    MathScriptLine(const char *line,MathScript *myScript);
    ~MathScriptLine();

    void Execute();
};

struct iDocumentNode;

/**
 *  A MathScript is a mini-program to run.
 *  It allows multiple lines. Each line must be
 *  in the form of:  <var> = <formula>.  When
 *  it parses, it makes a hashmap of all the variables
 *  for quick access.
 */
class MathScript
{
protected:
    csArray<MathScriptLine*> scriptLines;

    void ParseScript(const char *parsescript);

public:
    csString name;
    csHash<MathScriptVar*> variables;

    MathScript(iDocumentNode *node);
    MathScript(const char *myname,const char *script);
    ~MathScript();

    MathScriptVar *GetVar(const char *name);
    MathScriptVar *GetOrCreateVar(const char *name);

    void Execute();
    void DumpAllVars();
};

/**
 * This holds all the formulas loaded by /data/rpgrules.xml
 * and provides a container for them.  It also enables adding
 * of some needed functions not built-in to the formula parser.
 */
class MathScriptEngine
{
protected:
    csHash<MathScript*> scripts;

    static csRandomGen rng;


public:
    MathScriptEngine();
    ~MathScriptEngine();

    MathScript *FindScript(const char *name);

    static csArray<csString> customCompoundFunctions;
    static double RandomGen(const double *dummy);
    static double Power(const double *parms);
    static double CustomCompoundFunc(const double * parms);
};

#endif

