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

#include "util/log.h"
#include <csutil/randomgen.h>
#include <csutil/xmltiny.h>

#include "../server/globals.h"
#include "psdatabase.h"

#include "util/mathscript.h"
#include "util/consoleout.h"

csString MathVar::ToString() const
{
    if (type == VARTYPE_OBJ)
        return obj->ToString();
    if (type == VARTYPE_STR)
        return MathScriptEngine::GetString(value);
    return csString().Format("%.2f", value);
}

csString MathVar::Dump() const
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
        case VARTYPE_STR:
            str.Append("STR) = ");
            str.Append(MathScriptEngine::GetString(value));
            break;
        case VARTYPE_OBJ:
            str.Append("OBJ) = ");
            str.Append("PTR");
            break;
    }
    return str;
}

//----------------------------------------------------------------------------

MathEnvironment::~MathEnvironment()
{
    csHash<MathVar*, csString>::GlobalIterator it(variables.GetIterator());
    while (it.HasNext())
    {
        delete it.Next();
    }
}

MathVar* MathEnvironment::Lookup(const char *name) const
{
    MathVar *var = variables.Get(name, NULL);
    if (!var && parent)
        var = parent->Lookup(name);

    return var;
}

void MathEnvironment::Define(const char *name, double value)
{
    MathVar *var = variables.Get(name, NULL);
    if (var)
    {
        var->SetValue(value);
        return;
    }

    var = new MathVar;
    var->SetValue(value);
    variables.Put(name, var);
}

void MathEnvironment::Define(const char *name, iScriptableVar *obj)
{
    MathVar *var = variables.Get(name, NULL);
    if (var)
    {
        var->SetObject(obj);
        return;
    }

    var = new MathVar;
    var->SetObject(obj);
    variables.Put(name, var);
}

void MathEnvironment::DumpAllVars() const
{
    csHash<MathVar*, csString>::ConstGlobalIterator it(variables.GetIterator());
    while (it.HasNext())
    {
        MathVar *var = it.Next();
        CPrintf(CON_DEBUG, "%25s=%1.4f\n", var->name.GetData(), var->GetValue());
    }
}

void MathEnvironment::InterpolateString(csString & str) const
{
    csString varName;
    size_t pos = (size_t)-1;
    while ((pos = str.Find("${", pos+1)) != SIZET_NOT_FOUND)
    {
        size_t end = str.Find("}", pos+2);
        if (end == SIZET_NOT_FOUND)
            continue; // invalid - unterminated ${
        if (end < pos+3)
            continue; // invalid - empty ${}

        str.SubString(varName, pos+2, end-(pos+2));
        MathVar *var = Lookup(varName);
        if (!var)
            continue; // invalid - required variable not in environment.  leave it as is.

        str.DeleteAt(pos, end-pos+1);
        str.Insert(pos, var->ToString());
    }
}

//----------------------------------------------------------------------------

MathStatement* MathStatement::Create(const csString & line, const char *name)
{
    size_t assignAt = line.FindFirst('=');
    if (assignAt == SIZET_NOT_FOUND || assignAt == 0 || assignAt == line.Length())
        return NULL;

    MathStatement *stmt = new MathStatement;
    csString & assignee = stmt->assignee;
    line.SubString(assignee, 0, assignAt);
    assignee.Trim();

    bool validAssignee = isupper(assignee.GetAt(0)) != 0;
    for (size_t i = 1; validAssignee && i < assignee.Length(); i++)
    {
        if (!isalnum(assignee[i]) && assignee[i] != '_')
            validAssignee = false;
    }
    if (!validAssignee && assignee != "exit") // exit is special
    {
        Error3("Parse error in MathStatement >%s: %s<: Invalid assignee.", name, line.GetData());
        delete stmt;
        return NULL;
    }

    csString expression;
    line.SubString(expression, assignAt+1);
    stmt->expression = MathExpression::Create(expression, name);
    if (!stmt->expression)
    {
        delete stmt;
        return NULL;
    }

    // Lame hack - mark   Var = "String";  as a string type, for proper display.
    // In general, we can't know, since with fparser everything is a double.
    expression.LTrim();
    if (expression.GetAt(0) == '\'' || expression.GetAt(0) == '"')
        stmt->assigneeType = VARTYPE_STR;

    return stmt;
}

MathStatement::~MathStatement()
{
    if (expression)
    {
        delete expression;
        expression = NULL;
    }
}

void MathStatement::Evaluate(MathEnvironment *env)
{
    env->Define(assignee, expression->Evaluate(env));
    if (assigneeType != VARTYPE_VALUE)
    {
        MathVar *var = env->Lookup(assignee);
        var->type = assigneeType;
    }
}

//----------------------------------------------------------------------------

MathScript* MathScript::Create(const char *name, const csString & script)
{
    MathScript *s = new MathScript(name);

    size_t start = 0;
    size_t semicolonAt = 0;

    while (start < script.Length())
    {
        semicolonAt = script.FindFirst(';', start);
        if (semicolonAt == SIZET_NOT_FOUND)
            semicolonAt = script.Length();

        if (semicolonAt - start > 0)
        {
            csString line = script.Slice(start, semicolonAt - start);
            line.Collapse();
            if (!line.IsEmpty())
            {
                MathStatement *st = MathStatement::Create(line, name);
                if (!st)
                {
                    delete s;
                    return NULL;
                }
                s->scriptLines.Push(st);
            }
        }
        start = semicolonAt+1;
    }
    return s;
}

MathScript::~MathScript()
{
    while (scriptLines.GetSize())
        delete scriptLines.Pop();
}

void MathScript::Evaluate(MathEnvironment *env)
{
    MathVar *exitsignal = env->Lookup("exit");
    if (exitsignal)
        exitsignal->SetValue(0); // clear exit condition before running

    for (size_t i = 0; i < scriptLines.GetSize(); i++)
    {
        scriptLines[i]->Evaluate(env);
        if (exitsignal && exitsignal->GetValue() != 0.0)
        {
            // printf("Terminating mathscript at line %d of %d.\n",i, scriptLines.GetSize());
            break;
        }
    }
}

//----------------------------------------------------------------------------

MathScriptEngine::MathScriptEngine()
{
    Result result(db->Select("SELECT * from math_scripts"));
    if (!result.IsValid())
        return;

    for (unsigned long i = 0; i < result.Count(); i++ )
    {
        MathScript *scr = MathScript::Create(result[i]["name"], result[i]["math_script"]);
        if (!scr)
        {
            Error2("Failed to load MathScript >%s<.", result[i]["name"]);
            continue;
        }
        scripts.Put(scr->Name(), scr);
    }
}

MathScriptEngine::~MathScriptEngine()
{
    csHash<MathScript*, csString>::GlobalIterator it(scripts.GetIterator());
    while (it.HasNext())
    {
        delete it.Next();
    }
    scripts.DeleteAll();
    
    MathScriptEngine::customCompoundFunctions.Empty();
    MathScriptEngine::stringLiterals.Empty();
}

MathScript *MathScriptEngine::FindScript(const csString & name)
{
    return scripts.Get(name, NULL);
}

csRandomGen MathScriptEngine::rng;
csStringSet MathScriptEngine::customCompoundFunctions;
csStringSet MathScriptEngine::stringLiterals;

double MathScriptEngine::RandomGen(const double *limit)
{
   return MathScriptEngine::rng.Get()*limit[0];
}


double MathScriptEngine::Power(const double *parms)
{
    return pow(parms[0],parms[1]);
}

double MathScriptEngine::CustomCompoundFunc(const double * parms)
{
    size_t funcIndex = (size_t)parms[0];
    iScriptableVar * v = (iScriptableVar *)(intptr_t)parms[1];
    return v->CalcFunction(customCompoundFunctions.Request(funcIndex), &parms[2]);
}

csString MathScriptEngine::GetString(double id)
{
    const char *str = stringLiterals.Request(unsigned(id));
    return str ? str : "";
}

//----------------------------------------------------------------------------

MathExpression::MathExpression()
{
    fp.AddFunction("rnd", MathScriptEngine::RandomGen, 1);
    fp.AddFunction("pow", MathScriptEngine::Power, 2);
    for (int n = 2; n < 12; n++)
    {
        csString funcName("customCompoundFunc");
        funcName.Append(n);
        fp.AddFunction(funcName.GetData(), MathScriptEngine::CustomCompoundFunc, n);
    }
}

MathExpression* MathExpression::Create(const char *expression, const char *name)
{
    MathExpression* exp = new MathExpression;
    exp->name = name;

    if (!exp->Parse(expression))
    {
        delete exp;
        return NULL;
    }

    return exp;
}

void MathExpression::AddToFPVarList(const csSet<csString> & set, csString & fpVars)
{
    csSet<csString>::GlobalIterator it(set.GetIterator());
    while (it.HasNext())
    {
        fpVars.Append(it.Next());
        fpVars.Append(',');
    }
}

bool MathExpression::Parse(const char *exp)
{
    CS_ASSERT(exp);

    // SCANNER: creates a list of tokens.
    csArray<csString> tokens;
    size_t start = SIZET_NOT_FOUND;
    char quote = '\0';
    for (size_t i = 0; exp[i] != '\0'; i++)
    {
        char c = exp[i];

        // are we in a string literal?
        if (quote)
        {
            // found a closing quote?
            if (c == quote && exp[i-1] != '\\')
            {
                quote = '\0';
                csString token(exp+start, i-start);

                // strip slashes.
                for (size_t j = 0; j < token.Length(); j++)
                {
                    if (token[j] == '\\')
                        token.DeleteAt(j++); // remove and skip what it escaped
                }

                tokens.Push(token);
                start = SIZET_NOT_FOUND;
            }
            // otherwise, it's part of the string...ignore it.
            continue;
        }

        // alpha, numeric, and underscores don't break a token
        if (isalnum(c) || c == '_')
        {
            if (start == SIZET_NOT_FOUND) // and they can start one
                start = i;
            continue;
        }
        // everything else breaks the token...
        if (start != SIZET_NOT_FOUND)
        {
            tokens.Push(csString(exp+start, i-start));
            start = SIZET_NOT_FOUND;
        }

        // check if it's starting a string literal
        if (c == '\'' || c == '"')
        {
            quote = c;
            start = i;
            continue;
        }

        // ...otherwise, if it's not whitespace, it's a token by itself.
        if (!isspace(c))
        {
            tokens.Push(csString(c));
        }
    }
    // Push the last token, too
    if (start != SIZET_NOT_FOUND)
        tokens.Push(exp+start);

    //for (size_t i = 0; i < tokens.GetSize(); i++)
        //printf("Token[%d] = %s\n", int(i), tokens[i].GetDataSafe());

    // PARSER: (kind of)
    for (size_t i = 0; i < tokens.GetSize(); i++)
    {
        if (tokens[i] == ":")
        {
            if (i+1 == tokens.GetSize() || !isalpha(tokens[i+1].GetAt(0)))
            {
                Error4("Parse error in MathExpression >%s: %s<: Expected property or method after ':' operator; found >%s<.", name, exp, tokens[i+1].GetData());
                return false;
            }
            if (!isupper(tokens[i-1].GetAt(0)))
            {
                Error4("Parse error in MathExpression >%s: %s<: ':' Expected variable before ':' operator; found >%s<.", name, exp, tokens[i-1].GetData());
                return false;
            }

            // The previous variable must be an object.
            requiredObjs.Add(tokens[i-1]);

            // Is it a method call?
            if (i+2 < tokens.GetSize() && tokens[i+2] == "(")
            {
                // Methods start as Target:WeaponAt(Slot) and turn into customCompoundFuncN(X,Target,Slot)
                // where N-2 is the number of parameters and X is the index in a global lookup table.
                int paramCount = 2; // customCompoundFunc takes two args as boilerplate.

                // Count the number of params via the number of commas.  First one doesn't come with a comma.
                if (i+3 < tokens.GetSize() && tokens[i+3] != ")")
                    paramCount++;
                for (size_t j = i+3; j < tokens.GetSize() && tokens[j] != ")"; j++)
                {
                    if (tokens[j] == "(")
                        while (tokens[++j] != ")"); // fast forward; skip over nested calls for now.

                    if (tokens[j] == ",")
                        paramCount++;
                }

                // Build the replacement call - replace all four tokens.
                csString object = tokens[i-1];
                csString method = tokens[i+1];
                tokens[i-1] = "customCompoundFunc";
                tokens[ i ].Format("%d", paramCount);
                tokens[i+1] = "(";
                tokens[i+2].Format("%u,%s", CS::StringIDValue(MathScriptEngine::customCompoundFunctions.Request(method)), object.GetData());
                if (paramCount > 2)
                    tokens[i+2].Append(',');
                i += 2; // skip method name & paren - we just dealt with them.
            }
            else
            {
                // Found a property reference, i.e. Actor:HP
                tokens[i] = "_"; // fparser can't deal with ':', so change it to '_'.
                propertyRefs.Add(csString().Format("%s:%s", tokens[i-1].GetData(), tokens[i+1].GetData()));
                i++; // skip next token - we already dealt with the property.
            }
        }
        else // not dealing with a colon
        {
            // Record any string literals and replace them with their table index.
            if (tokens[i].GetAt(0) == '"' || tokens[i].GetAt(0) == '\'')
            {
                // remove quote (scanner already omitted the closing quote)
                tokens[i].DeleteAt(0);
                CS::StringIDValue id = MathScriptEngine::stringLiterals.Request(tokens[i]);
                tokens[i].Format("%u", id);
            }

            // Jot down any variable names (tokens starting with [A-Z])
            if (isupper(tokens[i].GetAt(0)))
                requiredVars.Add(tokens[i]);
        }
    }

    // Parse the formula.
    csString fpVars;
    csSet<csString>::GlobalIterator it(requiredVars.GetIterator());
    while (it.HasNext())
    {
        fpVars.Append(it.Next());
        fpVars.Append(',');
    }
    it = propertyRefs.GetIterator();
    while (it.HasNext())
    {
        csString ref = it.Next();
        ref.ReplaceAll(":", "_");
        fpVars.Append(ref);
        fpVars.Append(',');
    }
    if (!fpVars.IsEmpty())
        fpVars.Truncate(fpVars.Length() - 1); // remove the last training ','

    // Rebuild the expression now that method calls & properties are transformed
    csString expression;
    for (size_t i = 0; i < tokens.GetSize(); i++)
        expression.Append(tokens[i]);

    //printf("Final expression: %s\n", expression.GetData());

    size_t ret = fp.Parse(expression.GetData(), fpVars.GetDataSafe());
    if (ret != (size_t) -1)
    {
        Error5("Parse error in MathExpression >%s: %s< at column %zu: %s", name, expression.GetData(), ret, fp.ErrorMsg());
        return false;
    }

    fp.Optimize();
    return true;

}

double MathExpression::Evaluate(const MathEnvironment *env)
{
    double *values = new double [requiredVars.GetSize() + propertyRefs.GetSize()];
    size_t i = 0;

    csSet<csString>::GlobalIterator it(requiredVars.GetIterator());
    while (it.HasNext())
    {
        const csString & varName = it.Next();
        MathVar *var = env->Lookup(varName);
        if (!var)
        {
            Error3("Error in >%s<: Required variable >%s< not supplied in environment.", name, varName.GetData());
            CS_ASSERT(false);
            return 0.0; 
        }
        values[i++] = var->GetValue();
    }

    it = requiredObjs.GetIterator();
    while (it.HasNext())
    {
        const csString & objName = it.Next();
        MathVar *var = env->Lookup(objName);
        CS_ASSERT(var); // checked as part of requiredVars
        if (var->Type() != VARTYPE_OBJ)
        {
            Error3("Error in >%s<: Type inference requires >%s< to be an iScriptableVar, but it isn't.", name, objName.GetData());
            CS_ASSERT(false);
            return 0.0;
        }
        if (!var->GetObject())
        {
            Error3("Error in >%s<: Given a NULL iScriptableVar* for >%s<.", name, objName.GetData());
            CS_ASSERT(false);
            return 0.0;
        }
    }

    it = propertyRefs.GetIterator();
    while (it.HasNext())
    {
        const csString & ref = it.Next();
        size_t colonAt = ref.FindFirst(':');
        csString objname = ref.Slice(0, colonAt);
        csString prop    = ref.Slice(colonAt + 1);

        MathVar *var = env->Lookup(objname);
        CS_ASSERT(var); // checked as part of requiredVars
        iScriptableVar *obj = var->GetObject();
        CS_ASSERT(obj); // checked as part of requiredObjs
        values[i++] = obj->GetProperty(prop);
    }

    double ret = fp.Eval(values);
    delete [] values;
    return ret;
}

