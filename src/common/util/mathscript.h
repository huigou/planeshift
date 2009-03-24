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

#include <../tools/fparser/fparser.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <csutil/randomgen.h>
#include <csutil/set.h>
#include <csutil/strset.h>
#include <util/scriptvar.h>

//#if SIZEOF_DOUBLE < SIZEOF_VOIDP
//#error "MathScript doesn't work on platforms where double-precision floating points are smaller than pointers.
//#endif

enum MathType
{
    VARTYPE_VALUE,
    VARTYPE_OBJ
};

class MathVar
{
protected:
    double value;
    iScriptableVar *obj;
    MathVar *var;

    typedef void (*MathScriptVarCallback)(void * arg);
    MathScriptVarCallback changedVarCallback;
    void * changedVarCallbackArg;

public:
    MathType type;

    csString name;

    MathVar()
    {
        type  = VARTYPE_VALUE;
        value = 0;
        obj   = NULL;
        changedVarCallback = NULL;
        changedVarCallbackArg = NULL;        
    }

    void SetChangedCallback(MathScriptVarCallback callback, void * arg)
    {
        changedVarCallback = callback;
        changedVarCallbackArg = arg;
    }

    MathType Type() { return type; }

    double GetValue()
    {
        if (type == VARTYPE_OBJ)
            return (double)(intptr_t)obj;

        return value;
    }

    void SetValue(double v)
    {
        if (type == VARTYPE_OBJ)
            obj = *reinterpret_cast<iScriptableVar**>(&v); // Scary non-portible madness!
        else
            value = v; 

        if (changedVarCallback)
            changedVarCallback(changedVarCallbackArg);
    }

    void SetObject(iScriptableVar *p)
    {
        type = VARTYPE_OBJ;
        obj  = p;
    }

    iScriptableVar *GetObject()
    {
        return (type == VARTYPE_OBJ) ? obj : NULL;
    }

    void Copy(MathVar *v)
    {
        type = v->type;
        value = v->value;
        obj = v->obj;
        name = v->name;
    }

    csString ToString()
    {
        if (type == VARTYPE_OBJ)
            return obj->ToString();

        return csString().Format("%.2f", value);
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
        case VARTYPE_OBJ:
            str.Append("OBJ) = ");
            str.Append("PTR");
            break;
        }
        return str;
    }
    
};

class MathEnvironment
{
public:
    MathEnvironment() : parent(NULL) { }
    MathEnvironment(const MathEnvironment *parent) : parent(parent) { }
    ~MathEnvironment();

    MathVar* Lookup(const char *name) const;
    void Define(const char *name, double value);
    void Define(const char *name, iScriptableVar *obj);

    void DumpAllVars() const;

    /// Perform string interpolation, i.e. replacing ${...} with the appropriate variable.
    void InterpolateString(csString & str) const;

protected:
    const MathEnvironment *parent;
    csHash<MathVar*, csString> variables;
};

class MathExpression
{
public:
    static MathExpression* Create(const char *expression, const char *name = "");
    double Evaluate(const MathEnvironment *env);

protected:
    MathExpression();

    void AddToFPVarList(const csSet<csString> & set, csString & fpVars);
    bool Parse(const char *expression);

    csSet<csString> requiredVars;
    csSet<csString> requiredObjs; // a subset of requiredVars which are known to be objects; for type checking
    csSet<csString> propertyRefs; // full names like Target:HP
    FunctionParser fp;

    const char *name; // used for debugging
};

/**
 * This holds one line of a (potentially) multi-line script.
 * It knows what <var> is on the left, and what <formula> is 
 * on the right of the = sign.  When run, it executes the
 * formula and sets the result in the Var.  These vars are 
 * shared across Lines, which means the next Line can use
 * that Var as an input.
 */
class MathStatement
{
public:
    ~MathStatement();

    static MathStatement* Create(const csString & expression, const char *name);
    void Evaluate(MathEnvironment *env);

protected:
    MathStatement() : expression(NULL) { }

    csString assignee;
    MathExpression *expression;
};


/**
 *  A MathScript is a mini-program to run.
 *  It allows multiple lines. Each line must be
 *  in the form of:  <var> = <formula>.  When
 *  it parses, it makes a hashmap of all the variables
 *  for quick access.
 */
class MathScript
{
public:
    static MathScript* Create(const char *name, const csString & script);
    ~MathScript();

    const csString & Name() { return name; }

    void Evaluate(MathEnvironment *env);

protected:
    MathScript(const char *name) : name(name) { }
    csString name;
    csArray<MathStatement*> scriptLines;
};

/**
 * This holds all the formulas loaded by /data/rpgrules.xml
 * and provides a container for them.  It also enables adding
 * of some needed functions not built-in to the formula parser.
 */
class MathScriptEngine
{
protected:
    csHash<MathScript*, csString> scripts;
    static csRandomGen rng;

public:
    MathScriptEngine();
    ~MathScriptEngine();

    MathScript *FindScript(const csString & name);

    static csStringSet stringLiterals;
    static csStringSet customCompoundFunctions;
    static double RandomGen(const double *dummy);
    static double Power(const double *parms);
    static double CustomCompoundFunc(const double * parms);
    static csString GetString(double id);
};

#endif

