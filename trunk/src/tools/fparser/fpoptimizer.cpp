/***************************************************************************\
|* Function Parser for C++ v3.1.4                                          *|
|*-------------------------------------------------------------------------*|
|* Function optimizer                                                      *|
|*-------------------------------------------------------------------------*|
|* Copyright: Joel Yliluoma                                                *|
\***************************************************************************/

/* NOTE:
   This is a concatenation of all the header and source files of the
   original optimizer source code. All the code has been concatenated
   into this single file for convenience of usage (in other words, to
   simply use the optimizer, it's enough to add this file to the project
   rather than a multitude of files which the original optimizer source
   code is composed of).

   Thus this file exists for the usage of the Function parser library
   only, and is not suitable for developing it further. If you want to
   develop the library further, you should download the development
   version of the library, which has all the original source files.
 */

#include "fpconfig.h"
#ifdef FP_SUPPORT_OPTIMIZER

#ifdef _MSC_VER

typedef unsigned long long fphash_t;
#define FPHASH_CONST(x) x##ULL
#pragma warning(disable:4351)

#else

#include <stdint.h>
typedef uint_fast64_t fphash_t;
#define FPHASH_CONST(x) x##ULL

#endif
#include <vector>

#include "fpconfig.h"
#include "fparser.h"


namespace FPoptimizer_Grammar
{
    struct Grammar;
}

namespace FPoptimizer_CodeTree
{
    class CodeTreeParserData;
    class CodeTree;

    class CodeTreeP
    {
    public:
        CodeTreeP()                   : p(0)   { }
        CodeTreeP(CodeTree*        b) : p(b)   { Birth(); }
        CodeTreeP(const CodeTreeP& b) : p(&*b) { Birth(); }

        inline CodeTree& operator* () const { return *p; }
        inline CodeTree* operator->() const { return p; }

        CodeTreeP& operator= (CodeTree*        b) { Set(b); return *this; }
        CodeTreeP& operator= (const CodeTreeP& b) { Set(&*b); return *this; }

        ~CodeTreeP() { Forget(); }

    private:
        inline static void Have(CodeTree* p2);
        inline void Forget();
        inline void Birth();
        inline void Set(CodeTree* p2);
    private:
        CodeTree* p;
    };

    class CodeTree
    {
        friend class CodeTreeParserData;
        friend class CodeTreeP;

        int RefCount;

    public:
        /* Describing the codetree node */
        unsigned Opcode;
        union
        {
            double   Value;   // In case of cImmed: value of the immed
            unsigned Var;     // In case of cVar:   variable number
            unsigned Funcno;  // In case of cFCall or cPCall
        };
        struct Param
        {
            CodeTreeP param; // param node
            bool      sign;  // true = negated or inverted

            Param()                           : param(),  sign()  {}
            Param(CodeTree*        p, bool s) : param(p), sign(s) {}
            Param(const CodeTreeP& p, bool s) : param(p), sign(s) {}
        };

        // Parameters for the function
        //  These use the sign:
        //   For cAdd: operands to add together (0 to n)
        //             sign indicates that the value is negated before adding (0-x)
        //   For cMul: operands to multiply together (0 to n)
        //             sign indicates that the value is inverted before multiplying (1/x)
        //   For cAnd: operands to bitwise-and together (0 to n)
        //             sign indicates that the value is inverted before anding (!x)
        //   For cOr:  operands to bitwise-or together (0 to n)
        //             sign indicates that the value is inverted before orring (!x)
        //  These don't use the sign (sign is always false):
        //   For cMin: operands to select the minimum of
        //   For cMax: operands to select the maximum of
        //   For cImmed, not used
        //   For cVar,   not used
        //   For cIf:  operand 1 = condition, operand 2 = yes-branch, operand 3 = no-branch
        //   For anything else: the parameters required by the operation/function
        std::vector<Param> Params;

        /* Internal operation */
        fphash_t      Hash;
        size_t        Depth;
        CodeTree*     Parent;
        const FPoptimizer_Grammar::Grammar* OptimizedUsing;
    public:
        CodeTree();
        ~CodeTree();

        /* Generates a CodeTree from the given bytecode */
        static CodeTreeP GenerateFrom(
            const std::vector<unsigned>& byteCode,
            const std::vector<double>& immed,
            const FunctionParser::Data& data);

        class ByteCodeSynth;
        void SynthesizeByteCode(
            std::vector<unsigned>& byteCode,
            std::vector<double>&   immed,
            size_t& stacktop_max);
        void SynthesizeByteCode(ByteCodeSynth& synth);

        /* Regenerates the hash.
         * child_triggered=false: Recurse to children
         * child_triggered=true:  Recurse to parents
         */
        void Rehash(bool child_triggered);
        void Recalculate_Hash_NoRecursion();

        void Sort();
        void Sort_Recursive();

        void SetParams(const std::vector<Param>& RefParams);
        void AddParam(const Param& param);
        void DelParam(size_t index);

        /* Clones the tree. (For parameter duplication) */
        CodeTree* Clone();

        bool    IsImmed() const;
        double GetImmed() const { return Value; }
        bool    IsLongIntegerImmed() const { return IsImmed() && GetImmed() == (double)GetLongIntegerImmed(); }
        long   GetLongIntegerImmed() const { return (long)GetImmed(); }
        bool      IsVar() const;
        unsigned GetVar() const { return Var; }

        void NegateImmed() { if(IsImmed()) Value = -Value;       }
        void InvertImmed() { if(IsImmed()) Value = 1.0 / Value;  }
        void NotTheImmed() { if(IsImmed()) Value = Value == 0.0; }

    private:
        void ConstantFolding();

    private:
        CodeTree(const CodeTree&);
        CodeTree& operator=(const CodeTree&);
    };

    inline void CodeTreeP::Forget()
    {
        if(!p) return;
        p->RefCount -= 1;
        if(!p->RefCount) delete p;
        //assert(p->RefCount >= 0);
    }
    inline void CodeTreeP::Have(CodeTree* p2)
    {
        if(p2) ++(p2->RefCount);
    }
    inline void CodeTreeP::Birth()
    {
        Have(p);
    }
    inline void CodeTreeP::Set(CodeTree* p2)
    {
        Have(p2);
        Forget();
        p = p2;
    }
}
#define FPOPT_NAN_CONST (-1712345.25) /* Would use 0.0 / 0.0 here, but some compilers don't accept it. */

namespace FPoptimizer_CodeTree
{
    class CodeTree;
}

namespace FPoptimizer_Grammar
{
    typedef unsigned OpcodeType;

    enum TransformationType
    {
        None,    // default
        Negate,  // 0-x
        Invert,  // 1/x
        NotThe   // !x
    };

    enum SpecialOpcode
    {
        NumConstant = 0xFFFB, // Holds a particular value (syntax-time constant)
        ImmedHolder,          // Holds a particular immed
        NamedHolder,          // Holds a particular named param (of any kind)
        SubFunction,          // Holds an opcode and the params
        RestHolder            // Holds anything else
      //GroupFunction         // For parse-time functions
    };

    enum ParamMatchingType
    {
        PositionalParams, // this set of params in this order
        SelectedParams,   // this set of params in any order
        AnyParams         // these params are included
    };

    enum RuleType
    {
        ProduceNewTree, // replace self with the first (and only) from replaced_param
        ReplaceParams   // replace indicate params with replaced_params
    };

    enum SignBalanceType
    {
        BalanceDontCare,
        BalanceMoreNeg,
        BalanceMorePos,
        BalanceEqual
    };

    struct MatchResultType
    {
        bool found:16;
        bool has_more:16;

        MatchResultType(bool f,bool m) : found(f),has_more(m) { }
    };
    static const MatchResultType
        NoMatch(false,false),       // No match, don't try to increment match_index
        TryMore(false,true),        // No match, but try to increment match_index
        FoundSomeMatch(true,true),  // Found match, but we may have more
        FoundLastMatch(true,false); // Found match, don't have more

    // For iterating through match candidates
    template<typename Payload>
    struct MatchPositionSpec
    {
        unsigned roundno;
        bool     done;
        Payload  data;
        MatchPositionSpec() : roundno(0), done(false), data() { }
    };

    /***/

    struct MatchedParams
    {
        ParamMatchingType type    : 6;
        SignBalanceType   balance : 2;
        // count,index to plist[]
        unsigned         count : 8;
        unsigned         index : 16;

        struct CodeTreeMatch;

        MatchResultType
            Match(FPoptimizer_CodeTree::CodeTree& tree,
                  CodeTreeMatch& match,
                  unsigned long match_index,
                  bool recursion) const;

        void ReplaceParams(FPoptimizer_CodeTree::CodeTree& tree,
                           const MatchedParams& matcher, CodeTreeMatch& match) const;

        void ReplaceTree(FPoptimizer_CodeTree::CodeTree& tree,
                         const MatchedParams& matcher, CodeTreeMatch& match) const;

        void SynthesizeTree(
            FPoptimizer_CodeTree::CodeTree& tree,
            const MatchedParams& matcher,
            MatchedParams::CodeTreeMatch& match) const;
    };

    struct ParamSpec
    {
        OpcodeType opcode : 16;
        bool     sign     : 1;
        TransformationType
           transformation  : 3;
        unsigned minrepeat : 3;
        bool     anyrepeat : 1;

        // For NumConstant:   index to clist[]
        // For ImmedHolder:   index is the slot
        // For RestHolder:    index is the slot
        // For NamedHolder:   index is the slot
        // For SubFunction:   index to flist[]
        // For anything else
        //  =  GroupFunction: index,count to plist[]
        unsigned count : 8;
        unsigned index : 16;

        MatchResultType Match(
            FPoptimizer_CodeTree::CodeTree& tree,
            MatchedParams::CodeTreeMatch& match,
            TransformationType transf,
            unsigned long match_index) const;

        bool GetConst(
            const MatchedParams::CodeTreeMatch& match,
            double& result) const;

        void SynthesizeTree(
            FPoptimizer_CodeTree::CodeTree& tree,
            const MatchedParams& matcher,
            MatchedParams::CodeTreeMatch& match) const;
    };
    struct Function
    {
        OpcodeType opcode : 16;
        // index to mlist[]
        unsigned   index  : 16;

        MatchResultType
            Match(FPoptimizer_CodeTree::CodeTree& tree,
                  MatchedParams::CodeTreeMatch& match,
                  unsigned long match_index) const;
    };
    struct Rule
    {
        unsigned  n_minimum_params : 8;
        RuleType  type             : 8;
        // index to mlist[]
        unsigned  repl_index       : 16;

        Function  func;

        bool ApplyTo(FPoptimizer_CodeTree::CodeTree& tree) const;
    };
    struct Grammar
    {
        // count,index to rlist[]
        unsigned index : 16;
        unsigned count : 16;

        bool ApplyTo(FPoptimizer_CodeTree::CodeTree& tree,
                     bool recursion=false) const;
    };

    extern const struct GrammarPack
    {
        const double*         clist;
        const ParamSpec*      plist;
        const MatchedParams*  mlist;
        const Function*       flist;
        const Rule*           rlist;
        Grammar               glist[3];
    } pack;
}
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define CONSTANT_E     2.71828182845904509080  // exp(1)
#define CONSTANT_PI    M_PI                    // atan2(0,-1)
#define CONSTANT_L10   2.30258509299404590109  // log(10)
#define CONSTANT_L2    0.69314718055994530943  // log(2)
#define CONSTANT_L10I  0.43429448190325176116  // 1/log(10)
#define CONSTANT_L2I   1.4426950408889634074   // 1/log(2)
#define CONSTANT_L10E  CONSTANT_L10I           // log10(e)
#define CONSTANT_L10EI CONSTANT_L10            // 1/log10(e)
#define CONSTANT_L2E   CONSTANT_L2I            // log2(e)
#define CONSTANT_L2EI  CONSTANT_L2             // 1/log2(e)
#define CONSTANT_DR    (180.0 / M_PI)          // 180/pi
#define CONSTANT_RD    (M_PI / 180.0)          // pi/180


#include <string>

const std::string FP_GetOpcodeName(unsigned opcode, bool pad=false);
/* crc32 */

#ifdef _MSC_VER

 typedef unsigned int crc32_t;

#else

 #include <stdint.h>
 typedef uint_least32_t crc32_t;

#endif

namespace crc32
{
    enum { startvalue = 0xFFFFFFFFUL, poly = 0xEDB88320UL };

    /* This code constructs the CRC32 table at compile-time,
     * avoiding the need for a huge explicitly written table of magical numbers. */
    template<crc32_t crc> // One byte of a CRC32 (eight bits):
    struct b8
    {
        enum { b1 = (crc & 1) ? (poly ^ (crc >> 1)) : (crc >> 1),
               b2 = (b1  & 1) ? (poly ^ (b1  >> 1)) : (b1  >> 1),
               b3 = (b2  & 1) ? (poly ^ (b2  >> 1)) : (b2  >> 1),
               b4 = (b3  & 1) ? (poly ^ (b3  >> 1)) : (b3  >> 1),
               b5 = (b4  & 1) ? (poly ^ (b4  >> 1)) : (b4  >> 1),
               b6 = (b5  & 1) ? (poly ^ (b5  >> 1)) : (b5  >> 1),
               b7 = (b6  & 1) ? (poly ^ (b6  >> 1)) : (b6  >> 1),
               res= (b7  & 1) ? (poly ^ (b7  >> 1)) : (b7  >> 1) };
    };
    inline crc32_t update(crc32_t crc, unsigned/* char */b) // __attribute__((pure))
    {
        // Four values of the table
        #define B4(n) b8<n>::res,b8<n+1>::res,b8<n+2>::res,b8<n+3>::res
        // Sixteen values of the table
        #define R(n) B4(n),B4(n+4),B4(n+8),B4(n+12)
        // The whole table, index by steps of 16
        static const crc32_t table[256] =
        { R(0x00),R(0x10),R(0x20),R(0x30), R(0x40),R(0x50),R(0x60),R(0x70),
          R(0x80),R(0x90),R(0xA0),R(0xB0), R(0xC0),R(0xD0),R(0xE0),R(0xF0) };
        #undef R
        #undef B4
        return ((crc >> 8) /* & 0x00FFFFFF*/) ^ table[/*(unsigned char)*/(crc^b)&0xFF];
    }
    inline crc32_t calc_upd(crc32_t c, const unsigned char* buf, size_t size)
    {
        crc32_t value = c;
        for(size_t p=0; p<size; ++p) value = update(value, buf[p]);
        return value;
    }
    inline crc32_t calc(const unsigned char* buf, size_t size)
    {
        return calc_upd(startvalue, buf, size);
    }
}
#include <string>
#include <sstream>
#include <assert.h>

#include <iostream>

#include "fpconfig.h"
#include "fptypes.h"


using namespace FPoptimizer_Grammar;
using namespace FUNCTIONPARSERTYPES;

const std::string FP_GetOpcodeName(unsigned opcode, bool pad)
{
#if 1
    /* Symbolic meanings for the opcodes? */
    const char* p = 0;
    switch(OPCODE(opcode))
    {
        case cAbs: p = "cAbs"; break;
        case cAcos: p = "cAcos"; break;
        case cAcosh: p = "cAcosh"; break;
        case cAsin: p = "cAsin"; break;
        case cAsinh: p = "cAsinh"; break;
        case cAtan: p = "cAtan"; break;
        case cAtan2: p = "cAtan2"; break;
        case cAtanh: p = "cAtanh"; break;
        case cCeil: p = "cCeil"; break;
        case cCos: p = "cCos"; break;
        case cCosh: p = "cCosh"; break;
        case cCot: p = "cCot"; break;
        case cCsc: p = "cCsc"; break;
        case cEval: p = "cEval"; break;
        case cExp: p = "cExp"; break;
        case cFloor: p = "cFloor"; break;
        case cIf: p = "cIf"; break;
        case cInt: p = "cInt"; break;
        case cLog: p = "cLog"; break;
        case cLog2: p = "cLog2"; break;
        case cLog10: p = "cLog10"; break;
        case cMax: p = "cMax"; break;
        case cMin: p = "cMin"; break;
        case cPow: p = "cPow"; break;
        case cSec: p = "cSec"; break;
        case cSin: p = "cSin"; break;
        case cSinh: p = "cSinh"; break;
        case cSqrt: p = "cSqrt"; break;
        case cTan: p = "cTan"; break;
        case cTanh: p = "cTanh"; break;
        case cImmed: p = "cImmed"; break;
        case cJump: p = "cJump"; break;
        case cNeg: p = "cNeg"; break;
        case cAdd: p = "cAdd"; break;
        case cSub: p = "cSub"; break;
        case cMul: p = "cMul"; break;
        case cDiv: p = "cDiv"; break;
        case cMod: p = "cMod"; break;
        case cEqual: p = "cEqual"; break;
        case cNEqual: p = "cNEqual"; break;
        case cLess: p = "cLess"; break;
        case cLessOrEq: p = "cLessOrEq"; break;
        case cGreater: p = "cGreater"; break;
        case cGreaterOrEq: p = "cGreaterOrEq"; break;
        case cNot: p = "cNot"; break;
        case cAnd: p = "cAnd"; break;
        case cOr: p = "cOr"; break;
        case cDeg: p = "cDeg"; break;
        case cRad: p = "cRad"; break;
        case cFCall: p = "cFCall"; break;
        case cPCall: p = "cPCall"; break;
#ifdef FP_SUPPORT_OPTIMIZER
        case cVar: p = "cVar"; break;
        case cDup: p = "cDup"; break;
        case cInv: p = "cInv"; break;
        case cFetch: p = "cFetch"; break;
        case cPopNMov: p = "cPopNMov"; break;
        case cSqr: p = "cSqr"; break;
        case cRDiv: p = "cRDiv"; break;
        case cRSub: p = "cRSub"; break;
        case cNotNot: p = "cNotNot"; break;
        case cRSqrt: p = "cRSqrt"; break;
#endif
        case cNop: p = "cNop"; break;
        case VarBegin: p = "VarBegin"; break;
    }
    switch( SpecialOpcode(opcode) )
    {
        case NumConstant:   p = "NumConstant"; break;
        case ImmedHolder:   p = "ImmedHolder"; break;
        case NamedHolder:   p = "NamedHolder"; break;
        case RestHolder:    p = "RestHolder"; break;
        case SubFunction:   p = "SubFunction"; break;
      //case GroupFunction: p = "GroupFunction"; break;
    }
    std::stringstream tmp;
    //if(!p) std::cerr << "o=" << opcode << "\n";
    assert(p);
    tmp << p;
    if(pad) while(tmp.str().size() < 12) tmp << ' ';
    return tmp.str();
#else
    /* Just numeric meanings */
    std::stringstream tmp;
    tmp << opcode;
    if(pad) while(tmp.str().size() < 5) tmp << ' ';
    return tmp.str();
#endif
}
#include <cmath>
#include <list>
#include <algorithm>

#include "fptypes.h"


#ifdef FP_SUPPORT_OPTIMIZER

using namespace FUNCTIONPARSERTYPES;
//using namespace FPoptimizer_Grammar;


namespace FPoptimizer_CodeTree
{
    CodeTree::CodeTree()
        : RefCount(0), Opcode(), Params(), Hash(), Depth(1), Parent(), OptimizedUsing(0)
    {
    }

    CodeTree::~CodeTree()
    {
    }

    void CodeTree::Rehash(
        bool child_triggered)
    {
        /* If we were triggered by a parent, recurse to children */
        if(!child_triggered)
        {
            for(size_t a=0; a<Params.size(); ++a)
                Params[a].param->Rehash(false);
        }

        Recalculate_Hash_NoRecursion();

        /* If we were triggered by a child, recurse to the parent */
        if(child_triggered && Parent)
        {
            //assert(Parent->RefCount > 0);
            Parent->Rehash(true);
        }
    }

    struct ParamComparer
    {
        bool operator() (const CodeTree::Param& a, const CodeTree::Param& b) const
        {
            if(a.param->Depth != b.param->Depth)
                return a.param->Depth > b.param->Depth;
            if(a.sign != b.sign) return a.sign < b.sign;
            return a.param->Hash < b.param->Hash;
        }
    };

    void CodeTree::Sort()
    {
        /* If the tree is commutative, order the parameters
         * in a set order in order to make equality tests
         * efficient in the optimizer
         */
        switch(Opcode)
        {
            case cAdd:
            case cMul:
            case cMin:
            case cMax:
            case cAnd:
            case cOr:
            case cEqual:
            case cNEqual:
                std::sort(Params.begin(), Params.end(), ParamComparer());
                break;
            case cLess:
                if(ParamComparer() (Params[1], Params[0]))
                    { std::swap(Params[0], Params[1]); Opcode = cGreater; }
                break;
            case cLessOrEq:
                if(ParamComparer() (Params[1], Params[0]))
                    { std::swap(Params[0], Params[1]); Opcode = cGreaterOrEq; }
                break;
            case cGreater:
                if(ParamComparer() (Params[1], Params[0]))
                    { std::swap(Params[0], Params[1]); Opcode = cLess; }
                break;
            case cGreaterOrEq:
                if(ParamComparer() (Params[1], Params[0]))
                    { std::swap(Params[0], Params[1]); Opcode = cLessOrEq; }
                break;
        }
    }

    void CodeTree::Sort_Recursive()
    {
        Sort();
        for(size_t a=0; a<Params.size(); ++a)
            Params[a].param->Sort_Recursive();
        Recalculate_Hash_NoRecursion();
    }

    void CodeTree::Recalculate_Hash_NoRecursion()
    {
        fphash_t NewHash = Opcode * FPHASH_CONST(0x3A83A83A83A83A0);
        Depth = 1;
        switch(Opcode)
        {
            case cImmed:
                // FIXME: not portable - we're casting double* into uint_least64_t*
                if(Value != 0.0)
                    NewHash ^= *(fphash_t*)&Value;
                break; // no params
            case cVar:
                NewHash ^= (Var<<24) | (Var>>24);
                break; // no params
            case cFCall: case cPCall:
                NewHash ^= (Funcno<<24) | (Funcno>>24);
                /* passthru */
            default:
            {
                size_t MaxChildDepth = 0;
                for(size_t a=0; a<Params.size(); ++a)
                {
                    if(Params[a].param->Depth > MaxChildDepth)
                        MaxChildDepth = Params[a].param->Depth;

                    NewHash += (1+Params[a].sign)*FPHASH_CONST(0x2492492492492492);
                    NewHash *= FPHASH_CONST(1099511628211);
                    //assert(&*Params[a].param != this);
                    NewHash += Params[a].param->Hash;
                }
                Depth += MaxChildDepth;
            }
        }
        if(Hash != NewHash)
        {
            Hash = NewHash;
            OptimizedUsing = 0;
        }
    }

    CodeTree* CodeTree::Clone()
    {
        CodeTree* result = new CodeTree;
        result->Opcode = Opcode;
        switch(Opcode)
        {
            case cImmed:
                result->Value  = Value;
                break;
            case cVar:
                result->Var = Var;
                break;
            case cFCall: case cPCall:
                result->Funcno = Funcno;
                break;
        }
        result->SetParams(Params);
        result->Hash   = Hash;
        result->Depth  = Depth;
        //assert(Parent->RefCount > 0);
        result->Parent = Parent;
        return result;
    }

    void CodeTree::AddParam(const Param& param)
    {
        Params.push_back(param);
        Params.back().param->Parent = this;
    }

    void CodeTree::SetParams(const std::vector<Param>& RefParams)
    {
        Params = RefParams;
        /**
        *** Note: The only reason we need to CLONE the children here
        ***       is because they must have the correct Parent field.
        ***       The Parent is required because of backward-recursive
        ***       hash regeneration. Is there any way around this?
        */

        for(size_t a=0; a<Params.size(); ++a)
        {
            Params[a].param = Params[a].param->Clone();
            Params[a].param->Parent = this;
        }
    }

    void CodeTree::DelParam(size_t index)
    {
        Params.erase(Params.begin() + index);
    }
}

#endif
/* This file is automatically generated. Do not edit... */
#include "fpconfig.h"
#include "fptypes.h"

using namespace FPoptimizer_Grammar;
using namespace FUNCTIONPARSERTYPES;

namespace
{
    const double clist[] =
    {
        3.141592653589793115997963468544185161590576171875, /* 0 */
        0.5, /* 1 */
        0, /* 2 */
        1, /* 3 */
        2.7182818284590450907955982984276488423347473144531, /* 4 */
        -1, /* 5 */
        2, /* 6 */
        -2, /* 7 */
        -0.5, /* 8 */
        0.017453292519943295474371680597869271878153085708618, /* 9 */
        57.29577951308232286464772187173366546630859375, /* 10 */
        0.4342944819032517611567811854911269620060920715332, /* 11 */
        1.4426950408889633870046509400708600878715515136719, /* 12 */
        0.69314718055994528622676398299518041312694549560547, /* 13 */
        2.3025850929940459010936137929093092679977416992188, /* 14 */
    };

    const ParamSpec plist[] =
    {
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 0 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	0	}, /* 1    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 2 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	1	}, /* 3    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 4    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 5    	*/
        {cAcos       , false, None  , 1, false, 1,	5	}, /* 6    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 7    	*/
        {cAcosh      , false, None  , 1, false, 1,	7	}, /* 8    	*/
        {cAsin       , false, None  , 1, false, 1,	4	}, /* 9    	*/
        {cAsinh      , false, None  , 1, false, 1,	4	}, /* 10    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 11    	*/
        {cAtan       , false, None  , 1, false, 1,	5	}, /* 12    	*/
        {cAtanh      , false, None  , 1, false, 1,	7	}, /* 13    	*/
        {cCeil       , false, None  , 1, false, 1,	11	}, /* 14    	*/
        {cCos        , false, None  , 1, false, 1,	5	}, /* 15    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 16 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	2	}, /* 17    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 18 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 19 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	3	}, /* 20    	*/
        {NumConstant , false, None  , 1, false, 0,	0	}, /* 21    	*/
        {NumConstant , false, None  , 1, false, 0,	1	}, /* 22    	*/
        {cMul        , false, None  , 1, false, 2,	21	}, /* 23    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 24 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	4	}, /* 25    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 26    	*/
        {cCosh       , false, None  , 1, false, 1,	7	}, /* 27    	*/
        {cFloor      , false, None  , 1, false, 1,	7	}, /* 28    	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 29    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 30 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 31 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 32 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 33    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 34 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 35 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 36 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 37 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 38 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 39 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 40    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 41    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 42 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 43 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	6	}, /* 44    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 45    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 46    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 47 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 48    	*/
        {SubFunction , false, None  , 1, false, 0,	7	}, /* 49    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 50 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	8	}, /* 51    	*/
        {SubFunction , false, None  , 1, false, 0,	9	}, /* 52    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 53 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 54    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 55    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 56 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 57 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	10	}, /* 58    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 59 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 60    	*/
        {SubFunction , false, None  , 1, false, 0,	11	}, /* 61    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 62 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	12	}, /* 63    	*/
        {SubFunction , false, None  , 1, false, 0,	13	}, /* 64    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 65 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 66    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 67    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 68 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 69 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	14	}, /* 70    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 71    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 72    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 73 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 74    	*/
        {SubFunction , false, None  , 1, false, 0,	15	}, /* 75    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 76 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	16	}, /* 77    	*/
        {SubFunction , false, None  , 1, false, 0,	17	}, /* 78    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 79 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 80    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 81    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 82 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 83 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	18	}, /* 84    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 85    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 86    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 87 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 88    	*/
        {SubFunction , false, None  , 1, false, 0,	19	}, /* 89    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 90 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	20	}, /* 91    	*/
        {SubFunction , false, None  , 1, false, 0,	21	}, /* 92    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 93 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 94    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 95    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 96 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 97    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 98    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 99 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	22	}, /* 100    	*/
        {SubFunction , false, None  , 1, false, 0,	23	}, /* 101    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 102    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 103    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 104 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	24	}, /* 105    	*/
        {SubFunction , false, None  , 1, false, 0,	25	}, /* 106    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 107 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	26	}, /* 108    	*/
        {SubFunction , false, None  , 1, false, 0,	27	}, /* 109    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 110 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 111    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 112    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 113 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 114    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 115    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 116 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	28	}, /* 117    	*/
        {SubFunction , false, None  , 1, false, 0,	29	}, /* 118    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 119    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 120    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 121    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 122    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 123 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	30	}, /* 124    	*/
        {SubFunction , false, None  , 1, false, 0,	31	}, /* 125    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 126 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	32	}, /* 127    	*/
        {SubFunction , false, None  , 1, false, 0,	33	}, /* 128    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 129 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 130    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 131    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 132 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 133    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 134    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 135 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	34	}, /* 136    	*/
        {SubFunction , false, None  , 1, false, 0,	35	}, /* 137    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 138    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 139    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 140 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	36	}, /* 141    	*/
        {SubFunction , false, None  , 1, false, 0,	37	}, /* 142    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 143 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	38	}, /* 144    	*/
        {SubFunction , false, None  , 1, false, 0,	39	}, /* 145    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 146 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 147    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 148    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 149 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 150    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 151    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 152 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	40	}, /* 153    	*/
        {SubFunction , false, None  , 1, false, 0,	41	}, /* 154    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 155    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 156    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 157    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 158    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 159 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	42	}, /* 160    	*/
        {SubFunction , false, None  , 1, false, 0,	43	}, /* 161    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 162 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	44	}, /* 163    	*/
        {SubFunction , false, None  , 1, false, 0,	45	}, /* 164    	*/
        {SubFunction , false, None  , 1, false, 0,	46	}, /* 165    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 166 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 167 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 168 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 169 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 170 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	47	}, /* 171    	*/
        {SubFunction , false, None  , 1, false, 0,	48	}, /* 172    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 173 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 174 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 175 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 176 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 177 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	49	}, /* 178    	*/
        {cLog        , false, None  , 1, false, 1,	5	}, /* 179    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 180 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 181 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	50	}, /* 182    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 183 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	51	}, /* 184    	*/
        {SubFunction , false, None  , 1, false, 0,	52	}, /* 185    	*/
        {NumConstant , false, None  , 1, false, 0,	4	}, /* 186    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 187 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	53	}, /* 188    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 189    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 190    	*/
        {SubFunction , false, None  , 1, false, 0,	54	}, /* 191    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 192    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 193    	*/
        {SubFunction , false, None  , 1, false, 0,	55	}, /* 194    	*/
        {SubFunction , false, None  , 1, false, 0,	56	}, /* 195    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 196 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	57	}, /* 197    	*/
        {SubFunction , false, None  , 1, false, 0,	58	}, /* 198    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 199    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 200    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 201    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 202    	*/
        {cMax        , false, None  , 1, false, 2,	201	}, /* 203    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 204 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 205 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	59	}, /* 206    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 207    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 208    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 209    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 210    	*/
        {cMin        , false, None  , 1, false, 2,	209	}, /* 211    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 212 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 213 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 214    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 215 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 216    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 217 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	4	}, /* 218    	*/
        {SubFunction , false, None  , 1, false, 0,	60	}, /* 219    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 220    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 221    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 222    	*/
        {NumConstant , false, None  , 1, false, 0,	4	}, /* 223    	*/
        {SubFunction , false, None  , 1, false, 0,	62	}, /* 224    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 225    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 226    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 227 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	63	}, /* 228    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 229    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 230    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 231    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 232    	*/
        {cPow        , false, None  , 1, false, 2,	231	}, /* 233    	*/
        {cLog        , false, Invert, 1, false, 1,	4	}, /* 234    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 235    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 236    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 237    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 238    	*/
        {SubFunction , false, None  , 1, false, 0,	64	}, /* 239    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 240    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 241    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 242 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	65	}, /* 243    	*/
        {cLog        , true , None  , 1, false, 1,	5	}, /* 244    	*/
        {SubFunction , false, None  , 1, false, 0,	60	}, /* 245    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 246    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 247    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 248    	*/
        {SubFunction , false, None  , 1, false, 0,	66	}, /* 249    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 250    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 251    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 252 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	67	}, /* 253    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 254 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	5	}, /* 255    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 256 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	68	}, /* 257    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 258 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 259    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 260 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 261    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 262 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	69	}, /* 263    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 264    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 265    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 266 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	70	}, /* 267    	*/
        {SubFunction , true , None  , 1, false, 0,	71	}, /* 268    	*/
        {SubFunction , false, None  , 1, false, 0,	72	}, /* 269    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 270 "z"	*/
        {SubFunction , true , None  , 1, false, 0,	73	}, /* 271    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 272    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 273    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 274    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 275 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	74	}, /* 276    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 277    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 278    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 279 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	75	}, /* 280    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 281 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 282 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	76	}, /* 283    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 284 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 285 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 286 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 287 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	77	}, /* 288    	*/
        {cSin        , false, None  , 1, false, 1,	4	}, /* 289    	*/
        {SubFunction , false, None  , 1, false, 0,	78	}, /* 290    	*/
        {SubFunction , false, None  , 1, false, 0,	79	}, /* 291    	*/
        {SubFunction , true , None  , 1, false, 0,	80	}, /* 292    	*/
        {SubFunction , false, None  , 1, false, 0,	81	}, /* 293    	*/
        {NumConstant , false, None  , 1, false, 0,	0	}, /* 294    	*/
        {NumConstant , false, None  , 1, false, 0,	1	}, /* 295    	*/
        {cMul        , false, None  , 1, false, 2,	294	}, /* 296    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 297 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	82	}, /* 298    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 299    	*/
        {cSinh       , false, None  , 1, false, 1,	5	}, /* 300    	*/
        {cTan        , false, None  , 1, false, 1,	5	}, /* 301    	*/
        {SubFunction , false, None  , 1, false, 0,	84	}, /* 302    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 303 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	85	}, /* 304    	*/
        {SubFunction , true , None  , 1, false, 0,	86	}, /* 305    	*/
        {SubFunction , false, None  , 1, false, 0,	87	}, /* 306    	*/
        {cTanh       , false, None  , 1, false, 1,	4	}, /* 307    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 308    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 309    	*/
        {SubFunction , false, None  , 1, false, 0,	88	}, /* 310    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 311    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 312    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 313    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 314    	*/
        {SubFunction , false, None  , 1, false, 0,	89	}, /* 315    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 316    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 317    	*/
        {SubFunction , false, None  , 1, false, 0,	90	}, /* 318    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 319    	*/
        {RestHolder  , false, None  , 1, false, 0,	4	}, /* 320    	*/
        {SubFunction , false, None  , 1, false, 0,	91	}, /* 321    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 322    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 323    	*/
        {SubFunction , true , None  , 1, false, 0,	92	}, /* 324    	*/
        {ImmedHolder , true , None  , 1, false, 0,	0	}, /* 325    	*/
        {ImmedHolder , false, Negate, 1, false, 0,	0	}, /* 326    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 327    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 328    	*/
        {SubFunction , true , None  , 1, false, 0,	93	}, /* 329    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 330    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 331    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 332    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 333    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 334    	*/
        {SubFunction , true , None  , 1, false, 0,	94	}, /* 335    	*/
        {ImmedHolder , false, Negate, 1, false, 0,	0	}, /* 336    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 337    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 338    	*/
        {SubFunction , false, None  , 1, false, 0,	95	}, /* 339    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 340    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 341    	*/
        {SubFunction , false, None  , 1, false, 0,	96	}, /* 342    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 343    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 344    	*/
        {SubFunction , true , None  , 1, false, 0,	97	}, /* 345    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 346    	*/
        {RestHolder  , false, None  , 1, false, 0,	4	}, /* 347    	*/
        {SubFunction , false, None  , 1, false, 0,	98	}, /* 348    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 349    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 350    	*/
        {SubFunction , false, None  , 1, false, 0,	99	}, /* 351    	*/
        {SubFunction , false, None  , 1, false, 0,	100	}, /* 352    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 353    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 354    	*/
        {SubFunction , true , None  , 1, false, 0,	101	}, /* 355    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 356    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 357    	*/
        {SubFunction , false, None  , 1, false, 0,	102	}, /* 358    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 359    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 360    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 361    	*/
        {SubFunction , true , None  , 1, false, 0,	103	}, /* 362    	*/
        {SubFunction , false, None  , 1, false, 0,	100	}, /* 363    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 364    	*/
        {SubFunction , false, None  , 1, false, 0,	104	}, /* 365    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 366    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 367    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 368    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 369    	*/
        {cAdd        , false, None  , 1, false, 2,	368	}, /* 370    	*/
        {SubFunction , false, None  , 1, false, 0,	60	}, /* 371    	*/
        {SubFunction , false, None  , 1, false, 0,	105	}, /* 372    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 373 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 374 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	106	}, /* 375    	*/
        {SubFunction , false, None  , 1, false, 0,	107	}, /* 376    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 377    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 378    	*/
        {SubFunction , false, None  , 1, false, 0,	108	}, /* 379    	*/
        {SubFunction , false, None  , 1, false, 0,	101	}, /* 380    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 381    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 382    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 383    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 384    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 385    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 386    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 387    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 388    	*/
        {SubFunction , false, None  , 1, false, 0,	109	}, /* 389    	*/
        {SubFunction , false, None  , 1, false, 0,	110	}, /* 390    	*/
        {SubFunction , false, None  , 1, false, 0,	111	}, /* 391    	*/
        {SubFunction , false, None  , 1, false, 0,	112	}, /* 392    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 393    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 394    	*/
        {SubFunction , false, None  , 1, false, 0,	113	}, /* 395    	*/
        {SubFunction , false, None  , 1, false, 0,	114	}, /* 396    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 397    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 398    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 399    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 400    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 401    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 402    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 403    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 404    	*/
        {SubFunction , false, None  , 1, false, 0,	115	}, /* 405    	*/
        {SubFunction , true , None  , 1, false, 0,	116	}, /* 406    	*/
        {SubFunction , false, None  , 1, false, 0,	111	}, /* 407    	*/
        {SubFunction , true , None  , 1, false, 0,	112	}, /* 408    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 409    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 410    	*/
        {SubFunction , false, None  , 1, false, 0,	117	}, /* 411    	*/
        {SubFunction , false, None  , 1, false, 0,	118	}, /* 412    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 413 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 414    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 415    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 416 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 417    	*/
        {SubFunction , false, None  , 1, false, 0,	120	}, /* 418    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 419    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 420    	*/
        {SubFunction , false, None  , 1, false, 0,	119	}, /* 421    	*/
        {SubFunction , true , None  , 1, false, 0,	121	}, /* 422    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 423    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 424    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 425 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	122	}, /* 426    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 427    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 428    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 429    	*/
        {SubFunction , true , None  , 1, false, 0,	124	}, /* 430    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 431 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	125	}, /* 432    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 433    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 434    	*/
        {SubFunction , false, None  , 1, false, 0,	126	}, /* 435    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 436    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 437    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 438    	*/
        {SubFunction , true , None  , 1, false, 0,	128	}, /* 439    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 440 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	129	}, /* 441    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 442    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 443    	*/
        {SubFunction , false, None  , 1, false, 0,	130	}, /* 444    	*/
        {SubFunction , false, None  , 1, false, 0,	127	}, /* 445    	*/
        {SubFunction , true , None  , 1, false, 0,	131	}, /* 446    	*/
        {SubFunction , false, None  , 1, false, 0,	123	}, /* 447    	*/
        {SubFunction , false, None  , 1, false, 0,	132	}, /* 448    	*/
        {SubFunction , false, None  , 1, false, 0,	133	}, /* 449    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 450 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 451    	*/
        {SubFunction , false, None  , 1, false, 0,	134	}, /* 452    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 453    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 454    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 455 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 456    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 457    	*/
        {SubFunction , false, None  , 1, false, 0,	135	}, /* 458    	*/
        {SubFunction , false, None  , 1, false, 0,	136	}, /* 459    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 460    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 461    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 462 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	137	}, /* 463    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 464    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 465    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 466    	*/
        {SubFunction , true , None  , 1, false, 0,	139	}, /* 467    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 468 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	140	}, /* 469    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 470    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 471    	*/
        {SubFunction , false, None  , 1, false, 0,	141	}, /* 472    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 473    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 474    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 475    	*/
        {SubFunction , true , None  , 1, false, 0,	143	}, /* 476    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 477 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	144	}, /* 478    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 479    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 480    	*/
        {SubFunction , false, None  , 1, false, 0,	145	}, /* 481    	*/
        {SubFunction , false, None  , 1, false, 0,	142	}, /* 482    	*/
        {SubFunction , false, None  , 1, false, 0,	146	}, /* 483    	*/
        {SubFunction , false, None  , 1, false, 0,	138	}, /* 484    	*/
        {SubFunction , false, None  , 1, false, 0,	147	}, /* 485    	*/
        {SubFunction , false, None  , 1, false, 0,	148	}, /* 486    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 487 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 488    	*/
        {SubFunction , false, None  , 1, false, 0,	149	}, /* 489    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 490    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 491    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 492 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 493    	*/
        {SubFunction , false, None  , 1, false, 0,	151	}, /* 494    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 495    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 496    	*/
        {SubFunction , false, None  , 1, false, 0,	150	}, /* 497    	*/
        {SubFunction , false, None  , 1, false, 0,	152	}, /* 498    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 499    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 500    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 501 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	153	}, /* 502    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 503    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 504    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 505    	*/
        {SubFunction , true , None  , 1, false, 0,	155	}, /* 506    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 507 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	156	}, /* 508    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 509    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 510    	*/
        {SubFunction , false, None  , 1, false, 0,	157	}, /* 511    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 512    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 513    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 514    	*/
        {SubFunction , true , None  , 1, false, 0,	159	}, /* 515    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 516 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	160	}, /* 517    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 518    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 519    	*/
        {SubFunction , false, None  , 1, false, 0,	161	}, /* 520    	*/
        {SubFunction , false, None  , 1, false, 0,	158	}, /* 521    	*/
        {SubFunction , false, None  , 1, false, 0,	162	}, /* 522    	*/
        {SubFunction , false, None  , 1, false, 0,	154	}, /* 523    	*/
        {SubFunction , false, None  , 1, false, 0,	163	}, /* 524    	*/
        {SubFunction , false, None  , 1, false, 0,	164	}, /* 525    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 526 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 527    	*/
        {SubFunction , false, None  , 1, false, 0,	165	}, /* 528    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 529    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 530    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 531 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 532    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 533    	*/
        {SubFunction , false, None  , 1, false, 0,	166	}, /* 534    	*/
        {SubFunction , true , None  , 1, false, 0,	167	}, /* 535    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 536    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 537    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 538 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	168	}, /* 539    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 540    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 541    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 542    	*/
        {SubFunction , true , None  , 1, false, 0,	170	}, /* 543    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 544 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	171	}, /* 545    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 546    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 547    	*/
        {SubFunction , false, None  , 1, false, 0,	172	}, /* 548    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 549    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 550    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 551    	*/
        {SubFunction , true , None  , 1, false, 0,	174	}, /* 552    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 553 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	175	}, /* 554    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 555    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 556    	*/
        {SubFunction , false, None  , 1, false, 0,	176	}, /* 557    	*/
        {SubFunction , false, None  , 1, false, 0,	173	}, /* 558    	*/
        {SubFunction , true , None  , 1, false, 0,	177	}, /* 559    	*/
        {SubFunction , false, None  , 1, false, 0,	169	}, /* 560    	*/
        {SubFunction , false, None  , 1, false, 0,	178	}, /* 561    	*/
        {SubFunction , false, None  , 1, false, 0,	179	}, /* 562    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 563 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 564    	*/
        {SubFunction , false, None  , 1, false, 0,	180	}, /* 565    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 566    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 567    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 568 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 569    	*/
        {SubFunction , false, None  , 1, false, 0,	182	}, /* 570    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 571    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 572    	*/
        {SubFunction , false, None  , 1, false, 0,	181	}, /* 573    	*/
        {SubFunction , true , None  , 1, false, 0,	183	}, /* 574    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 575    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 576    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 577 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	184	}, /* 578    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 579    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 580    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 581    	*/
        {SubFunction , true , None  , 1, false, 0,	186	}, /* 582    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 583 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	187	}, /* 584    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 585    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 586    	*/
        {SubFunction , false, None  , 1, false, 0,	188	}, /* 587    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 588    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 589    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 590    	*/
        {SubFunction , true , None  , 1, false, 0,	190	}, /* 591    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 592 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	191	}, /* 593    	*/
        {RestHolder  , false, None  , 1, false, 0,	5	}, /* 594    	*/
        {RestHolder  , true , None  , 1, false, 0,	6	}, /* 595    	*/
        {SubFunction , false, None  , 1, false, 0,	192	}, /* 596    	*/
        {SubFunction , false, None  , 1, false, 0,	189	}, /* 597    	*/
        {SubFunction , true , None  , 1, false, 0,	193	}, /* 598    	*/
        {SubFunction , false, None  , 1, false, 0,	185	}, /* 599    	*/
        {SubFunction , false, None  , 1, false, 0,	194	}, /* 600    	*/
        {SubFunction , false, None  , 1, false, 0,	195	}, /* 601    	*/
        {SubFunction , false, None  , 1, false, 0,	100	}, /* 602    	*/
        {SubFunction , false, None  , 1, false, 0,	196	}, /* 603    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 604    	*/
        {SubFunction , false, None  , 1, false, 0,	198	}, /* 605    	*/
        {SubFunction , false, None  , 1, false, 0,	197	}, /* 606    	*/
        {SubFunction , false, None  , 1, false, 0,	199	}, /* 607    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 608 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 609 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	200	}, /* 610    	*/
        {SubFunction , false, None  , 1, false, 0,	201	}, /* 611    	*/
        {SubFunction , false, None  , 1, false, 0,	100	}, /* 612    	*/
        {SubFunction , false, None  , 1, false, 0,	196	}, /* 613    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 614    	*/
        {SubFunction , false, None  , 1, false, 0,	203	}, /* 615    	*/
        {SubFunction , false, None  , 1, false, 0,	202	}, /* 616    	*/
        {SubFunction , true , None  , 1, false, 0,	204	}, /* 617    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 618 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 619 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	205	}, /* 620    	*/
        {SubFunction , false, None  , 1, false, 0,	206	}, /* 621    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 622    	*/
        {SubFunction , false, None  , 1, false, 0,	207	}, /* 623    	*/
        {SubFunction , false, None  , 1, false, 0,	100	}, /* 624    	*/
        {SubFunction , false, None  , 1, false, 0,	198	}, /* 625    	*/
        {SubFunction , false, None  , 1, false, 0,	208	}, /* 626    	*/
        {SubFunction , false, None  , 1, false, 0,	209	}, /* 627    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 628 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 629 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	210	}, /* 630    	*/
        {SubFunction , false, None  , 1, false, 0,	211	}, /* 631    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 632    	*/
        {SubFunction , false, None  , 1, false, 0,	196	}, /* 633    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 634    	*/
        {SubFunction , false, None  , 1, false, 0,	203	}, /* 635    	*/
        {SubFunction , false, None  , 1, false, 0,	212	}, /* 636    	*/
        {SubFunction , true , None  , 1, false, 0,	213	}, /* 637    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 638 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 639 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	214	}, /* 640    	*/
        {SubFunction , false, None  , 1, false, 0,	215	}, /* 641    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 642 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 643    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 644    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 645 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	216	}, /* 646    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 647    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 648    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 649 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	217	}, /* 650    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 651 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	218	}, /* 652    	*/
        {SubFunction , false, None  , 1, false, 0,	219	}, /* 653    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 654 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 655    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 656    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 657 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	220	}, /* 658    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 659    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 660    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 661 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	221	}, /* 662    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 663 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	222	}, /* 664    	*/
        {SubFunction , false, None  , 1, false, 0,	223	}, /* 665    	*/
        {SubFunction , false, None  , 1, false, 0,	224	}, /* 666    	*/
        {SubFunction , false, None  , 1, false, 0,	225	}, /* 667    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 668    	*/
        {SubFunction , false, None  , 1, false, 0,	203	}, /* 669    	*/
        {SubFunction , true , None  , 1, false, 0,	226	}, /* 670    	*/
        {SubFunction , false, None  , 1, false, 0,	227	}, /* 671    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 672 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 673 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	228	}, /* 674    	*/
        {SubFunction , true , None  , 1, false, 0,	229	}, /* 675    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 676    	*/
        {SubFunction , false, None  , 1, false, 0,	207	}, /* 677    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 678    	*/
        {SubFunction , false, None  , 1, false, 0,	198	}, /* 679    	*/
        {SubFunction , true , None  , 1, false, 0,	230	}, /* 680    	*/
        {SubFunction , true , None  , 1, false, 0,	231	}, /* 681    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 682 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 683 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	232	}, /* 684    	*/
        {SubFunction , true , None  , 1, false, 0,	233	}, /* 685    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 686    	*/
        {SubFunction , false, None  , 1, false, 0,	196	}, /* 687    	*/
        {SubFunction , true , None  , 1, false, 0,	234	}, /* 688    	*/
        {SubFunction , true , None  , 1, false, 0,	213	}, /* 689    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 690 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 691 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	235	}, /* 692    	*/
        {SubFunction , true , None  , 1, false, 0,	236	}, /* 693    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 694 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 695    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 696    	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 697 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	237	}, /* 698    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 699    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 700    	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 701 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	238	}, /* 702    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 703 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	239	}, /* 704    	*/
        {SubFunction , false, None  , 1, false, 0,	240	}, /* 705    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 706 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 707    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 708    	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 709 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	241	}, /* 710    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 711    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 712    	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 713 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	242	}, /* 714    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 715 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	243	}, /* 716    	*/
        {SubFunction , false, None  , 1, false, 0,	244	}, /* 717    	*/
        {NamedHolder , false, None  , 2, true , 0,	0	}, /* 718 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 719 "x"	*/
        {NamedHolder , false, None  , 2, true , 0,	0	}, /* 720 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	245	}, /* 721    	*/
        {NamedHolder , true , None  , 2, true , 0,	0	}, /* 722 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 723 "x"	*/
        {NamedHolder , false, None  , 2, true , 0,	0	}, /* 724 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	246	}, /* 725    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 726 "a"	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 727    	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 728 "b"	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 729    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 730 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 731 "b"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 732    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 733    	*/
        {SubFunction , false, None  , 1, false, 0,	247	}, /* 734    	*/
        {SubFunction , false, None  , 1, false, 0,	248	}, /* 735    	*/
        {SubFunction , false, None  , 1, false, 0,	249	}, /* 736    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 737 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 738 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	250	}, /* 739    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 740    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 741    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 742    	*/
        {SubFunction , false, None  , 1, false, 0,	252	}, /* 743    	*/
        {NumConstant , false, None  , 1, false, 0,	7	}, /* 744    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 745 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 746 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	253	}, /* 747    	*/
        {SubFunction , false, None  , 1, false, 0,	251	}, /* 748    	*/
        {SubFunction , false, None  , 1, false, 0,	254	}, /* 749    	*/
        {NumConstant , false, None  , 1, false, 0,	0	}, /* 750    	*/
        {NumConstant , false, None  , 1, false, 0,	1	}, /* 751    	*/
        {cMul        , false, None  , 1, false, 2,	750	}, /* 752    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 753 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	255	}, /* 754    	*/
        {SubFunction , true , None  , 1, false, 0,	256	}, /* 755    	*/
        {SubFunction , false, None  , 1, false, 0,	86	}, /* 756    	*/
        {NumConstant , false, None  , 1, false, 0,	5	}, /* 757    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 758    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 759    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 760    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 761    	*/
        {SubFunction , true , None  , 1, false, 0,	257	}, /* 762    	*/
        {SubFunction , false, None  , 1, false, 0,	258	}, /* 763    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 764    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 765    	*/
        {SubFunction , false, None  , 1, false, 0,	259	}, /* 766    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 767    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 768    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 769 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 770    	*/
        {SubFunction , true , None  , 1, false, 0,	260	}, /* 771    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 772 "x"	*/
        {ImmedHolder , false, Negate, 1, false, 0,	0	}, /* 773    	*/
        {SubFunction , false, None  , 1, false, 0,	261	}, /* 774    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 775    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 776    	*/
        {SubFunction , true , None  , 1, false, 0,	262	}, /* 777    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 778    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 779    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 780    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 781    	*/
        {cMul        , false, None  , 1, false, 2,	780	}, /* 782    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 783    	*/
        {ImmedHolder , true , None  , 1, false, 0,	1	}, /* 784    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 785    	*/
        {ImmedHolder , false, Invert, 1, false, 0,	1	}, /* 786    	*/
        {cMul        , false, None  , 1, false, 2,	785	}, /* 787    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 788    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 789 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	263	}, /* 790    	*/
        {SubFunction , false, None  , 1, false, 0,	264	}, /* 791    	*/
        {cLog        , false, Invert, 1, false, 1,	5	}, /* 792    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 793    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 794 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	265	}, /* 795    	*/
        {SubFunction , false, None  , 1, false, 0,	266	}, /* 796    	*/
        {cLog        , true , None  , 1, false, 1,	4	}, /* 797    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 798 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 799 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	267	}, /* 800    	*/
        {SubFunction , false, None  , 1, false, 0,	268	}, /* 801    	*/
        {SubFunction , true , None  , 1, false, 0,	60	}, /* 802    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 803    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 804 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 805 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	269	}, /* 806    	*/
        {SubFunction , false, None  , 1, false, 0,	270	}, /* 807    	*/
        {SubFunction , false, None  , 1, false, 0,	271	}, /* 808    	*/
        {cLog        , false, Invert, 1, false, 1,	5	}, /* 809    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 810    	*/
        {cLog        , false, Invert, 1, false, 1,	4	}, /* 811    	*/
        {SubFunction , false, None  , 1, false, 0,	272	}, /* 812    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 813 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	273	}, /* 814    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 815    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 816 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 817 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	274	}, /* 818    	*/
        {SubFunction , false, None  , 1, false, 0,	275	}, /* 819    	*/
        {SubFunction , false, None  , 1, false, 0,	276	}, /* 820    	*/
        {cLog        , true , None  , 1, false, 1,	7	}, /* 821    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 822    	*/
        {cLog        , false, Invert, 1, false, 1,	4	}, /* 823    	*/
        {SubFunction , false, None  , 1, false, 0,	277	}, /* 824    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 825 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	278	}, /* 826    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 827 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 828 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 829 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	279	}, /* 830    	*/
        {SubFunction , false, None  , 1, false, 0,	280	}, /* 831    	*/
        {SubFunction , false, None  , 1, false, 0,	281	}, /* 832    	*/
        {SubFunction , true , None  , 1, false, 0,	282	}, /* 833    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 834    	*/
        {SubFunction , true , None  , 1, false, 0,	73	}, /* 835    	*/
        {SubFunction , false, None  , 1, false, 0,	283	}, /* 836    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 837 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	284	}, /* 838    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 839 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 840 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 841 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 842 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	285	}, /* 843    	*/
        {SubFunction , false, None  , 1, false, 0,	286	}, /* 844    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 845 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 846 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 847 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	287	}, /* 848    	*/
        {SubFunction , false, None  , 1, false, 0,	288	}, /* 849    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 850 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 851 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 852 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 853 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	289	}, /* 854    	*/
        {SubFunction , true , None  , 1, false, 0,	290	}, /* 855    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 856 "y"	*/
        {NamedHolder , true , None  , 1, false, 0,	2	}, /* 857 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 858 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	291	}, /* 859    	*/
        {SubFunction , false, None  , 1, false, 0,	292	}, /* 860    	*/
        {SubFunction , false, None  , 1, false, 0,	293	}, /* 861    	*/
        {SubFunction , true , None  , 1, false, 0,	224	}, /* 862    	*/
        {SubFunction , false, None  , 1, false, 0,	294	}, /* 863    	*/
        {SubFunction , false, None  , 1, false, 0,	295	}, /* 864    	*/
        {SubFunction , true , None  , 1, false, 0,	296	}, /* 865    	*/
        {SubFunction , false, None  , 1, false, 0,	297	}, /* 866    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 867 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 868 "y"	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 869 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	298	}, /* 870    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 871 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 872 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 873 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	299	}, /* 874    	*/
        {SubFunction , false, None  , 1, false, 0,	300	}, /* 875    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 876 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 877 "y"	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 878 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	301	}, /* 879    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 880 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 881 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 882 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	302	}, /* 883    	*/
        {SubFunction , false, None  , 1, false, 0,	303	}, /* 884    	*/
        {ImmedHolder , true , None  , 1, false, 0,	0	}, /* 885    	*/
        {ImmedHolder , true , None  , 1, false, 0,	1	}, /* 886    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 887    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 888    	*/
        {cMul        , true , None  , 1, false, 2,	887	}, /* 889    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 890    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 891 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	304	}, /* 892    	*/
        {SubFunction , true , None  , 1, false, 0,	305	}, /* 893    	*/
        {cLog        , false, None  , 1, false, 1,	5	}, /* 894    	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 895 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 896 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 897 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	306	}, /* 898    	*/
        {SubFunction , true , None  , 1, false, 0,	307	}, /* 899    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 900    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 901 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 902 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 903 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 904 "z"	*/
        {SubFunction , true , None  , 1, false, 0,	308	}, /* 905    	*/
        {SubFunction , true , None  , 1, false, 0,	309	}, /* 906    	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 907 "y"	*/
        {NamedHolder , true , None  , 1, false, 0,	2	}, /* 908 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 909 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	310	}, /* 910    	*/
        {SubFunction , false, None  , 1, false, 0,	311	}, /* 911    	*/
        {SubFunction , true , None  , 1, false, 0,	80	}, /* 912    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 913    	*/
        {SubFunction , true , None  , 1, false, 0,	312	}, /* 914    	*/
        {SubFunction , false, None  , 1, false, 0,	313	}, /* 915    	*/
        {SubFunction , true , None  , 1, false, 0,	297	}, /* 916    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 917 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 918 "y"	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 919 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	314	}, /* 920    	*/
        {NamedHolder , false, Negate, 1, true , 0,	0	}, /* 921 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 922 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 923 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	315	}, /* 924    	*/
        {SubFunction , false, None  , 1, false, 0,	316	}, /* 925    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 926 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 927 "y"	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 928 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	317	}, /* 929    	*/
        {NamedHolder , false, Negate, 1, true , 0,	0	}, /* 930 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 931 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 932 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	318	}, /* 933    	*/
        {SubFunction , false, None  , 1, false, 0,	319	}, /* 934    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 935 "x"	*/
        {NamedHolder , false, None  , 2, true , 0,	0	}, /* 936 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	320	}, /* 937    	*/
        {NamedHolder , true , None  , 2, true , 0,	0	}, /* 938 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 939 "x"	*/
        {NamedHolder , false, Negate, 2, true , 0,	0	}, /* 940 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	321	}, /* 941    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 942    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 943    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 944    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 945    	*/
        {cMod        , false, None  , 1, false, 2,	944	}, /* 946    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 947 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 948 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 949 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 950 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 951 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 952 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 953 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 954 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 955 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 956 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 957 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 958 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 959 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 960 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	322	}, /* 961    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 962 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 963 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	323	}, /* 964    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 965 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 966 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	324	}, /* 967    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 968 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 969 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	325	}, /* 970    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 971 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 972 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	326	}, /* 973    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 974 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 975 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	327	}, /* 976    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 977 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 978 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	328	}, /* 979    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 980 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 981 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	329	}, /* 982    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 983 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 984 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	330	}, /* 985    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 986 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 987 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	331	}, /* 988    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 989 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 990 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	332	}, /* 991    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 992 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 993 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	333	}, /* 994    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 995    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 996    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 997    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 998    	*/
        {SubFunction , true , None  , 1, false, 0,	334	}, /* 999    	*/
        {SubFunction , false, None  , 1, false, 0,	335	}, /* 1000    	*/
        {SubFunction , false, None  , 1, false, 0,	336	}, /* 1001    	*/
        {SubFunction , false, None  , 1, false, 0,	337	}, /* 1002    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1003    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1004    	*/
        {SubFunction , false, None  , 1, false, 0,	338	}, /* 1005    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1006    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1007    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1008 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1009 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	339	}, /* 1010    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1011 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1012 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	340	}, /* 1013    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1014 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1015 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	341	}, /* 1016    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1017 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1018 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	342	}, /* 1019    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1020 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1021 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	343	}, /* 1022    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1023 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1024 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	344	}, /* 1025    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1026 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1027 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	345	}, /* 1028    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1029 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1030 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	346	}, /* 1031    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1032 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1033 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	347	}, /* 1034    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1035 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1036 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	348	}, /* 1037    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1038 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1039 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	349	}, /* 1040    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1041 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1042 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	350	}, /* 1043    	*/
        {SubFunction , true , None  , 1, false, 0,	336	}, /* 1044    	*/
        {SubFunction , true , None  , 1, false, 0,	351	}, /* 1045    	*/
        {SubFunction , true , None  , 1, false, 0,	335	}, /* 1046    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1047 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1048 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1049 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1050 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1051 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1052 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	352	}, /* 1053    	*/
        {SubFunction , false, None  , 1, false, 0,	353	}, /* 1054    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 1055 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 1056 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1057 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1058 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1059 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	5	}, /* 1060 "c"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1061 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	5	}, /* 1062 "c"	*/
        {SubFunction , false, None  , 1, false, 0,	354	}, /* 1063    	*/
        {SubFunction , false, None  , 1, false, 0,	355	}, /* 1064    	*/
        {SubFunction , false, None  , 1, false, 0,	356	}, /* 1065    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1066 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1067 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1068 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	5	}, /* 1069 "c"	*/
        {SubFunction , false, None  , 1, false, 0,	357	}, /* 1070    	*/
        {SubFunction , false, None  , 1, false, 0,	358	}, /* 1071    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1072    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1073    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 1074    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 1075    	*/
        {SubFunction , true , None  , 1, false, 0,	359	}, /* 1076    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1077    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1078    	*/
        {SubFunction , false, None  , 1, false, 0,	360	}, /* 1079    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1080    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1081    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1082 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1083 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	361	}, /* 1084    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1085 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1086 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	362	}, /* 1087    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1088 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1089 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	363	}, /* 1090    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1091 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1092 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	364	}, /* 1093    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1094 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1095 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	365	}, /* 1096    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1097 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1098 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	366	}, /* 1099    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1100 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1101 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	367	}, /* 1102    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1103 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1104 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	368	}, /* 1105    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1106 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1107 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	369	}, /* 1108    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1109 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1110 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	370	}, /* 1111    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1112 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1113 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	371	}, /* 1114    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1115 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1116 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	372	}, /* 1117    	*/
        {SubFunction , true , None  , 1, false, 0,	337	}, /* 1118    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1119    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1120    	*/
        {SubFunction , true , None  , 1, false, 0,	373	}, /* 1121    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 1122    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 1123    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1124 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1125 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1126 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1127 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1128 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1129 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	374	}, /* 1130    	*/
        {SubFunction , false, None  , 1, false, 0,	375	}, /* 1131    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 1132 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 1133 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1134 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1135 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	376	}, /* 1136    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1137 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1138 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	377	}, /* 1139    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1140 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1141 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	378	}, /* 1142    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1143 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1144 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	379	}, /* 1145    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1146 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1147 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	380	}, /* 1148    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1149 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1150 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	381	}, /* 1151    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1152 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1153 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	382	}, /* 1154    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1155 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1156 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	383	}, /* 1157    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1158 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1159 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	384	}, /* 1160    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1161 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1162 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	385	}, /* 1163    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1164 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1165 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	386	}, /* 1166    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1167 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1168 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	387	}, /* 1169    	*/
        {SubFunction , false, None  , 1, false, 0,	388	}, /* 1170    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1171    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1172    	*/
        {SubFunction , false, None  , 1, false, 0,	389	}, /* 1173    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1174    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1175    	*/
        {SubFunction , false, None  , 1, false, 0,	390	}, /* 1176    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1177    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1178    	*/
        {SubFunction , false, None  , 1, false, 0,	391	}, /* 1179    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1180    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1181    	*/
        {SubFunction , false, None  , 1, false, 0,	392	}, /* 1182    	*/
        {NumConstant , false, None  , 1, false, 0,	4	}, /* 1183    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1184 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	393	}, /* 1185    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1186 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	8	}, /* 1187    	*/
        {SubFunction , false, None  , 1, false, 0,	394	}, /* 1188    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1189 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	1	}, /* 1190    	*/
        {SubFunction , false, None  , 1, false, 0,	395	}, /* 1191    	*/
        {NumConstant , false, None  , 1, false, 0,	9	}, /* 1192    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1193    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1194    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1195    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1196    	*/
        {SubFunction , false, None  , 1, false, 0,	396	}, /* 1197    	*/
        {SubFunction , false, None  , 1, false, 0,	397	}, /* 1198    	*/
        {NumConstant , false, None  , 1, false, 0,	10	}, /* 1199    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1200    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1201    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1202    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1203    	*/
        {SubFunction , false, None  , 1, false, 0,	398	}, /* 1204    	*/
        {SubFunction , false, None  , 1, false, 0,	399	}, /* 1205    	*/
        {SubFunction , true , None  , 1, false, 0,	100	}, /* 1206    	*/
        {SubFunction , false, None  , 1, false, 0,	400	}, /* 1207    	*/
        {SubFunction , false, None  , 1, false, 0,	401	}, /* 1208    	*/
        {SubFunction , false, None  , 1, false, 0,	402	}, /* 1209    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 1210    	*/
        {NumConstant , false, None  , 1, false, 0,	11	}, /* 1211    	*/
        {SubFunction , false, None  , 1, false, 0,	403	}, /* 1212    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 1213    	*/
        {NumConstant , false, None  , 1, false, 0,	12	}, /* 1214    	*/
        {SubFunction , false, None  , 1, false, 0,	404	}, /* 1215    	*/
        {SubFunction , true , None  , 1, false, 0,	61	}, /* 1216    	*/
        {NumConstant , false, None  , 1, false, 0,	13	}, /* 1217    	*/
        {SubFunction , true , None  , 1, false, 0,	404	}, /* 1218    	*/
        {SubFunction , true , None  , 1, false, 0,	60	}, /* 1219    	*/
        {NumConstant , false, None  , 1, false, 0,	14	}, /* 1220    	*/
        {SubFunction , true , None  , 1, false, 0,	405	}, /* 1221    	*/
        {SubFunction , false, None  , 1, false, 0,	406	}, /* 1222    	*/
    };

    const MatchedParams mlist[] =
    {
        {PositionalParams, BalanceDontCare, 1, 0 }, /* 0 */
        {PositionalParams, BalanceDontCare, 1, 1 }, /* 1 */
        {PositionalParams, BalanceDontCare, 1, 2 }, /* 2 */
        {PositionalParams, BalanceDontCare, 1, 3 }, /* 3 */
        {PositionalParams, BalanceDontCare, 1, 4 }, /* 4 */
        {PositionalParams, BalanceDontCare, 1, 6 }, /* 5 */
        {PositionalParams, BalanceDontCare, 1, 7 }, /* 6 */
        {PositionalParams, BalanceDontCare, 1, 8 }, /* 7 */
        {PositionalParams, BalanceDontCare, 1, 9 }, /* 8 */
        {PositionalParams, BalanceDontCare, 1, 10 }, /* 9 */
        {PositionalParams, BalanceDontCare, 1, 11 }, /* 10 */
        {PositionalParams, BalanceDontCare, 1, 12 }, /* 11 */
        {PositionalParams, BalanceDontCare, 1, 13 }, /* 12 */
        {PositionalParams, BalanceDontCare, 1, 14 }, /* 13 */
        {PositionalParams, BalanceDontCare, 1, 5 }, /* 14 */
        {PositionalParams, BalanceDontCare, 1, 15 }, /* 15 */
        {PositionalParams, BalanceDontCare, 1, 16 }, /* 16 */
        {PositionalParams, BalanceDontCare, 1, 17 }, /* 17 */
        {PositionalParams, BalanceDontCare, 1, 18 }, /* 18 */
        {PositionalParams, BalanceDontCare, 1, 19 }, /* 19 */
        {PositionalParams, BalanceDontCare, 1, 20 }, /* 20 */
        {AnyParams       , BalanceDontCare, 2, 23 }, /* 21 */
        {PositionalParams, BalanceDontCare, 1, 25 }, /* 22 */
        {PositionalParams, BalanceDontCare, 1, 26 }, /* 23 */
        {PositionalParams, BalanceDontCare, 1, 27 }, /* 24 */
        {PositionalParams, BalanceDontCare, 1, 28 }, /* 25 */
        {PositionalParams, BalanceDontCare, 3, 29 }, /* 26 */
        {PositionalParams, BalanceDontCare, 1, 32 }, /* 27 */
        {PositionalParams, BalanceDontCare, 3, 33 }, /* 28 */
        {PositionalParams, BalanceDontCare, 3, 36 }, /* 29 */
        {AnyParams       , BalanceDontCare, 3, 39 }, /* 30 */
        {PositionalParams, BalanceDontCare, 3, 42 }, /* 31 */
        {PositionalParams, BalanceDontCare, 2, 45 }, /* 32 */
        {PositionalParams, BalanceDontCare, 3, 47 }, /* 33 */
        {PositionalParams, BalanceDontCare, 2, 50 }, /* 34 */
        {PositionalParams, BalanceDontCare, 1, 52 }, /* 35 */
        {AnyParams       , BalanceDontCare, 3, 53 }, /* 36 */
        {PositionalParams, BalanceDontCare, 3, 56 }, /* 37 */
        {PositionalParams, BalanceDontCare, 2, 40 }, /* 38 */
        {PositionalParams, BalanceDontCare, 3, 59 }, /* 39 */
        {PositionalParams, BalanceDontCare, 2, 62 }, /* 40 */
        {PositionalParams, BalanceDontCare, 1, 64 }, /* 41 */
        {AnyParams       , BalanceDontCare, 3, 65 }, /* 42 */
        {PositionalParams, BalanceDontCare, 3, 68 }, /* 43 */
        {PositionalParams, BalanceDontCare, 2, 71 }, /* 44 */
        {PositionalParams, BalanceDontCare, 3, 73 }, /* 45 */
        {PositionalParams, BalanceDontCare, 2, 76 }, /* 46 */
        {PositionalParams, BalanceDontCare, 1, 78 }, /* 47 */
        {AnyParams       , BalanceDontCare, 3, 79 }, /* 48 */
        {PositionalParams, BalanceDontCare, 3, 82 }, /* 49 */
        {PositionalParams, BalanceDontCare, 2, 85 }, /* 50 */
        {PositionalParams, BalanceDontCare, 3, 87 }, /* 51 */
        {PositionalParams, BalanceDontCare, 2, 90 }, /* 52 */
        {PositionalParams, BalanceDontCare, 1, 92 }, /* 53 */
        {AnyParams       , BalanceDontCare, 3, 93 }, /* 54 */
        {AnyParams       , BalanceDontCare, 3, 96 }, /* 55 */
        {PositionalParams, BalanceDontCare, 3, 99 }, /* 56 */
        {PositionalParams, BalanceDontCare, 2, 102 }, /* 57 */
        {PositionalParams, BalanceDontCare, 2, 97 }, /* 58 */
        {PositionalParams, BalanceDontCare, 3, 104 }, /* 59 */
        {PositionalParams, BalanceDontCare, 2, 107 }, /* 60 */
        {PositionalParams, BalanceDontCare, 1, 109 }, /* 61 */
        {AnyParams       , BalanceDontCare, 3, 110 }, /* 62 */
        {AnyParams       , BalanceDontCare, 3, 113 }, /* 63 */
        {PositionalParams, BalanceDontCare, 3, 116 }, /* 64 */
        {PositionalParams, BalanceDontCare, 2, 119 }, /* 65 */
        {PositionalParams, BalanceDontCare, 2, 121 }, /* 66 */
        {PositionalParams, BalanceDontCare, 3, 123 }, /* 67 */
        {PositionalParams, BalanceDontCare, 2, 126 }, /* 68 */
        {PositionalParams, BalanceDontCare, 1, 128 }, /* 69 */
        {AnyParams       , BalanceDontCare, 3, 129 }, /* 70 */
        {AnyParams       , BalanceDontCare, 3, 132 }, /* 71 */
        {PositionalParams, BalanceDontCare, 3, 135 }, /* 72 */
        {PositionalParams, BalanceDontCare, 2, 138 }, /* 73 */
        {PositionalParams, BalanceDontCare, 3, 140 }, /* 74 */
        {PositionalParams, BalanceDontCare, 2, 143 }, /* 75 */
        {PositionalParams, BalanceDontCare, 1, 145 }, /* 76 */
        {AnyParams       , BalanceDontCare, 3, 146 }, /* 77 */
        {AnyParams       , BalanceDontCare, 3, 149 }, /* 78 */
        {PositionalParams, BalanceDontCare, 3, 152 }, /* 79 */
        {PositionalParams, BalanceDontCare, 2, 155 }, /* 80 */
        {PositionalParams, BalanceDontCare, 2, 157 }, /* 81 */
        {PositionalParams, BalanceDontCare, 3, 159 }, /* 82 */
        {PositionalParams, BalanceDontCare, 2, 162 }, /* 83 */
        {PositionalParams, BalanceDontCare, 1, 164 }, /* 84 */
        {AnyParams       , BalanceDontCare, 1, 18 }, /* 85 */
        {PositionalParams, BalanceDontCare, 3, 165 }, /* 86 */
        {PositionalParams, BalanceDontCare, 3, 168 }, /* 87 */
        {PositionalParams, BalanceDontCare, 1, 171 }, /* 88 */
        {PositionalParams, BalanceDontCare, 3, 172 }, /* 89 */
        {PositionalParams, BalanceDontCare, 3, 175 }, /* 90 */
        {PositionalParams, BalanceDontCare, 1, 178 }, /* 91 */
        {PositionalParams, BalanceDontCare, 1, 179 }, /* 92 */
        {PositionalParams, BalanceDontCare, 2, 180 }, /* 93 */
        {PositionalParams, BalanceDontCare, 1, 182 }, /* 94 */
        {PositionalParams, BalanceDontCare, 2, 183 }, /* 95 */
        {PositionalParams, BalanceDontCare, 1, 185 }, /* 96 */
        {PositionalParams, BalanceDontCare, 2, 186 }, /* 97 */
        {AnyParams       , BalanceDontCare, 3, 188 }, /* 98 */
        {PositionalParams, BalanceDontCare, 1, 191 }, /* 99 */
        {PositionalParams, BalanceDontCare, 2, 192 }, /* 100 */
        {PositionalParams, BalanceDontCare, 1, 194 }, /* 101 */
        {PositionalParams, BalanceDontCare, 2, 195 }, /* 102 */
        {PositionalParams, BalanceDontCare, 1, 197 }, /* 103 */
        {AnyParams       , BalanceDontCare, 1, 54 }, /* 104 */
        {AnyParams       , BalanceDontCare, 1, 198 }, /* 105 */
        {PositionalParams, BalanceDontCare, 1, 54 }, /* 106 */
        {AnyParams       , BalanceDontCare, 2, 199 }, /* 107 */
        {PositionalParams, BalanceDontCare, 1, 203 }, /* 108 */
        {AnyParams       , BalanceDontCare, 2, 204 }, /* 109 */
        {AnyParams       , BalanceDontCare, 1, 40 }, /* 110 */
        {AnyParams       , BalanceDontCare, 1, 206 }, /* 111 */
        {AnyParams       , BalanceDontCare, 2, 207 }, /* 112 */
        {PositionalParams, BalanceDontCare, 1, 211 }, /* 113 */
        {AnyParams       , BalanceDontCare, 2, 212 }, /* 114 */
        {PositionalParams, BalanceDontCare, 2, 214 }, /* 115 */
        {PositionalParams, BalanceDontCare, 1, 29 }, /* 116 */
        {PositionalParams, BalanceDontCare, 2, 216 }, /* 117 */
        {PositionalParams, BalanceDontCare, 1, 33 }, /* 118 */
        {PositionalParams, BalanceDontCare, 2, 218 }, /* 119 */
        {AnyParams       , BalanceDontCare, 3, 220 }, /* 120 */
        {PositionalParams, BalanceDontCare, 2, 223 }, /* 121 */
        {PositionalParams, BalanceDontCare, 2, 225 }, /* 122 */
        {PositionalParams, BalanceDontCare, 2, 227 }, /* 123 */
        {PositionalParams, BalanceDontCare, 2, 229 }, /* 124 */
        {PositionalParams, BalanceDontCare, 1, 233 }, /* 125 */
        {AnyParams       , BalanceDontCare, 4, 234 }, /* 126 */
        {PositionalParams, BalanceDontCare, 2, 238 }, /* 127 */
        {PositionalParams, BalanceDontCare, 2, 240 }, /* 128 */
        {PositionalParams, BalanceDontCare, 2, 242 }, /* 129 */
        {AnyParams       , BalanceDontCare, 4, 244 }, /* 130 */
        {PositionalParams, BalanceDontCare, 2, 248 }, /* 131 */
        {PositionalParams, BalanceDontCare, 2, 250 }, /* 132 */
        {PositionalParams, BalanceDontCare, 2, 252 }, /* 133 */
        {PositionalParams, BalanceDontCare, 2, 254 }, /* 134 */
        {PositionalParams, BalanceDontCare, 1, 256 }, /* 135 */
        {PositionalParams, BalanceDontCare, 1, 257 }, /* 136 */
        {PositionalParams, BalanceDontCare, 2, 258 }, /* 137 */
        {PositionalParams, BalanceDontCare, 2, 260 }, /* 138 */
        {AnyParams       , BalanceMoreNeg , 2, 54 }, /* 139 */
        {PositionalParams, BalanceDontCare, 2, 262 }, /* 140 */
        {PositionalParams, BalanceDontCare, 2, 264 }, /* 141 */
        {PositionalParams, BalanceDontCare, 2, 266 }, /* 142 */
        {PositionalParams, BalanceDontCare, 1, 268 }, /* 143 */
        {PositionalParams, BalanceDontCare, 1, 269 }, /* 144 */
        {PositionalParams, BalanceDontCare, 1, 270 }, /* 145 */
        {AnyParams       , BalanceDontCare, 4, 271 }, /* 146 */
        {PositionalParams, BalanceDontCare, 2, 275 }, /* 147 */
        {PositionalParams, BalanceDontCare, 2, 277 }, /* 148 */
        {PositionalParams, BalanceDontCare, 2, 279 }, /* 149 */
        {PositionalParams, BalanceDontCare, 2, 281 }, /* 150 */
        {PositionalParams, BalanceDontCare, 2, 283 }, /* 151 */
        {PositionalParams, BalanceDontCare, 2, 285 }, /* 152 */
        {PositionalParams, BalanceDontCare, 2, 287 }, /* 153 */
        {PositionalParams, BalanceDontCare, 1, 289 }, /* 154 */
        {PositionalParams, BalanceDontCare, 1, 290 }, /* 155 */
        {PositionalParams, BalanceDontCare, 1, 291 }, /* 156 */
        {PositionalParams, BalanceDontCare, 1, 292 }, /* 157 */
        {PositionalParams, BalanceDontCare, 1, 293 }, /* 158 */
        {AnyParams       , BalanceDontCare, 2, 296 }, /* 159 */
        {PositionalParams, BalanceDontCare, 1, 298 }, /* 160 */
        {PositionalParams, BalanceDontCare, 1, 299 }, /* 161 */
        {PositionalParams, BalanceDontCare, 1, 300 }, /* 162 */
        {PositionalParams, BalanceDontCare, 1, 301 }, /* 163 */
        {PositionalParams, BalanceDontCare, 1, 302 }, /* 164 */
        {PositionalParams, BalanceDontCare, 1, 303 }, /* 165 */
        {PositionalParams, BalanceDontCare, 1, 304 }, /* 166 */
        {PositionalParams, BalanceDontCare, 1, 305 }, /* 167 */
        {PositionalParams, BalanceDontCare, 1, 306 }, /* 168 */
        {PositionalParams, BalanceDontCare, 1, 307 }, /* 169 */
        {PositionalParams, BalanceDontCare, 0, 0 }, /* 170 */
        {PositionalParams, BalanceDontCare, 1, 48 }, /* 171 */
        {AnyParams       , BalanceDontCare, 1, 48 }, /* 172 */
        {AnyParams       , BalanceDontCare, 2, 308 }, /* 173 */
        {AnyParams       , BalanceDontCare, 1, 310 }, /* 174 */
        {PositionalParams, BalanceDontCare, 2, 311 }, /* 175 */
        {AnyParams       , BalanceMoreNeg , 2, 313 }, /* 176 */
        {AnyParams       , BalanceDontCare, 3, 315 }, /* 177 */
        {AnyParams       , BalanceDontCare, 1, 318 }, /* 178 */
        {PositionalParams, BalanceDontCare, 2, 319 }, /* 179 */
        {PositionalParams, BalanceDontCare, 3, 321 }, /* 180 */
        {PositionalParams, BalanceDontCare, 1, 324 }, /* 181 */
        {AnyParams       , BalanceDontCare, 1, 325 }, /* 182 */
        {PositionalParams, BalanceDontCare, 1, 326 }, /* 183 */
        {AnyParams       , BalanceDontCare, 2, 327 }, /* 184 */
        {AnyParams       , BalanceDontCare, 1, 329 }, /* 185 */
        {PositionalParams, BalanceDontCare, 2, 330 }, /* 186 */
        {AnyParams       , BalanceDontCare, 3, 332 }, /* 187 */
        {AnyParams       , BalanceDontCare, 1, 335 }, /* 188 */
        {PositionalParams, BalanceDontCare, 3, 336 }, /* 189 */
        {PositionalParams, BalanceDontCare, 1, 339 }, /* 190 */
        {AnyParams       , BalanceMoreNeg , 2, 340 }, /* 191 */
        {AnyParams       , BalanceDontCare, 3, 342 }, /* 192 */
        {AnyParams       , BalanceDontCare, 1, 345 }, /* 193 */
        {PositionalParams, BalanceDontCare, 2, 346 }, /* 194 */
        {PositionalParams, BalanceDontCare, 3, 348 }, /* 195 */
        {PositionalParams, BalanceDontCare, 1, 351 }, /* 196 */
        {PositionalParams, BalanceDontCare, 2, 352 }, /* 197 */
        {AnyParams       , BalanceDontCare, 2, 354 }, /* 198 */
        {PositionalParams, BalanceDontCare, 2, 356 }, /* 199 */
        {PositionalParams, BalanceDontCare, 1, 358 }, /* 200 */
        {PositionalParams, BalanceDontCare, 2, 359 }, /* 201 */
        {AnyParams       , BalanceDontCare, 2, 361 }, /* 202 */
        {PositionalParams, BalanceDontCare, 2, 363 }, /* 203 */
        {PositionalParams, BalanceDontCare, 1, 365 }, /* 204 */
        {AnyParams       , BalanceDontCare, 2, 366 }, /* 205 */
        {PositionalParams, BalanceDontCare, 1, 370 }, /* 206 */
        {AnyParams       , BalanceDontCare, 2, 18 }, /* 207 */
        {PositionalParams, BalanceDontCare, 1, 31 }, /* 208 */
        {AnyParams       , BalanceDontCare, 2, 371 }, /* 209 */
        {PositionalParams, BalanceDontCare, 2, 373 }, /* 210 */
        {PositionalParams, BalanceDontCare, 1, 375 }, /* 211 */
        {PositionalParams, BalanceDontCare, 1, 376 }, /* 212 */
        {PositionalParams, BalanceDontCare, 2, 377 }, /* 213 */
        {AnyParams       , BalanceDontCare, 2, 379 }, /* 214 */
        {PositionalParams, BalanceDontCare, 1, 354 }, /* 215 */
        {AnyParams       , BalanceDontCare, 4, 381 }, /* 216 */
        {AnyParams       , BalanceDontCare, 4, 385 }, /* 217 */
        {AnyParams       , BalanceDontCare, 2, 389 }, /* 218 */
        {PositionalParams, BalanceDontCare, 2, 387 }, /* 219 */
        {PositionalParams, BalanceDontCare, 2, 391 }, /* 220 */
        {PositionalParams, BalanceDontCare, 3, 393 }, /* 221 */
        {PositionalParams, BalanceDontCare, 1, 396 }, /* 222 */
        {AnyParams       , BalanceDontCare, 4, 397 }, /* 223 */
        {AnyParams       , BalanceDontCare, 4, 401 }, /* 224 */
        {AnyParams       , BalanceDontCare, 2, 405 }, /* 225 */
        {PositionalParams, BalanceDontCare, 2, 407 }, /* 226 */
        {PositionalParams, BalanceDontCare, 3, 409 }, /* 227 */
        {PositionalParams, BalanceDontCare, 1, 412 }, /* 228 */
        {AnyParams       , BalanceDontCare, 3, 413 }, /* 229 */
        {PositionalParams, BalanceDontCare, 2, 416 }, /* 230 */
        {AnyParams       , BalanceDontCare, 3, 418 }, /* 231 */
        {AnyParams       , BalanceDontCare, 2, 421 }, /* 232 */
        {PositionalParams, BalanceDontCare, 2, 423 }, /* 233 */
        {PositionalParams, BalanceDontCare, 2, 425 }, /* 234 */
        {PositionalParams, BalanceDontCare, 2, 427 }, /* 235 */
        {PositionalParams, BalanceDontCare, 2, 429 }, /* 236 */
        {PositionalParams, BalanceDontCare, 2, 431 }, /* 237 */
        {PositionalParams, BalanceDontCare, 3, 433 }, /* 238 */
        {PositionalParams, BalanceDontCare, 2, 436 }, /* 239 */
        {PositionalParams, BalanceDontCare, 2, 438 }, /* 240 */
        {PositionalParams, BalanceDontCare, 2, 440 }, /* 241 */
        {PositionalParams, BalanceDontCare, 3, 442 }, /* 242 */
        {PositionalParams, BalanceDontCare, 2, 445 }, /* 243 */
        {PositionalParams, BalanceDontCare, 2, 447 }, /* 244 */
        {PositionalParams, BalanceDontCare, 1, 449 }, /* 245 */
        {PositionalParams, BalanceDontCare, 2, 450 }, /* 246 */
        {AnyParams       , BalanceDontCare, 3, 452 }, /* 247 */
        {AnyParams       , BalanceDontCare, 3, 455 }, /* 248 */
        {AnyParams       , BalanceDontCare, 2, 458 }, /* 249 */
        {PositionalParams, BalanceDontCare, 2, 460 }, /* 250 */
        {PositionalParams, BalanceDontCare, 2, 462 }, /* 251 */
        {PositionalParams, BalanceDontCare, 2, 464 }, /* 252 */
        {PositionalParams, BalanceDontCare, 2, 466 }, /* 253 */
        {PositionalParams, BalanceDontCare, 2, 468 }, /* 254 */
        {PositionalParams, BalanceDontCare, 3, 470 }, /* 255 */
        {PositionalParams, BalanceDontCare, 2, 473 }, /* 256 */
        {PositionalParams, BalanceDontCare, 2, 475 }, /* 257 */
        {PositionalParams, BalanceDontCare, 2, 477 }, /* 258 */
        {PositionalParams, BalanceDontCare, 3, 479 }, /* 259 */
        {PositionalParams, BalanceDontCare, 2, 482 }, /* 260 */
        {PositionalParams, BalanceDontCare, 2, 484 }, /* 261 */
        {PositionalParams, BalanceDontCare, 1, 486 }, /* 262 */
        {PositionalParams, BalanceDontCare, 2, 487 }, /* 263 */
        {AnyParams       , BalanceDontCare, 3, 489 }, /* 264 */
        {PositionalParams, BalanceDontCare, 2, 492 }, /* 265 */
        {AnyParams       , BalanceDontCare, 3, 494 }, /* 266 */
        {AnyParams       , BalanceDontCare, 2, 497 }, /* 267 */
        {PositionalParams, BalanceDontCare, 2, 499 }, /* 268 */
        {PositionalParams, BalanceDontCare, 2, 501 }, /* 269 */
        {PositionalParams, BalanceDontCare, 2, 503 }, /* 270 */
        {PositionalParams, BalanceDontCare, 2, 505 }, /* 271 */
        {PositionalParams, BalanceDontCare, 2, 507 }, /* 272 */
        {PositionalParams, BalanceDontCare, 3, 509 }, /* 273 */
        {PositionalParams, BalanceDontCare, 2, 512 }, /* 274 */
        {PositionalParams, BalanceDontCare, 2, 514 }, /* 275 */
        {PositionalParams, BalanceDontCare, 2, 516 }, /* 276 */
        {PositionalParams, BalanceDontCare, 3, 518 }, /* 277 */
        {PositionalParams, BalanceDontCare, 2, 521 }, /* 278 */
        {PositionalParams, BalanceDontCare, 2, 523 }, /* 279 */
        {PositionalParams, BalanceDontCare, 1, 525 }, /* 280 */
        {PositionalParams, BalanceDontCare, 2, 526 }, /* 281 */
        {AnyParams       , BalanceDontCare, 3, 528 }, /* 282 */
        {AnyParams       , BalanceDontCare, 3, 531 }, /* 283 */
        {AnyParams       , BalanceDontCare, 2, 534 }, /* 284 */
        {PositionalParams, BalanceDontCare, 2, 536 }, /* 285 */
        {PositionalParams, BalanceDontCare, 2, 538 }, /* 286 */
        {PositionalParams, BalanceDontCare, 2, 540 }, /* 287 */
        {PositionalParams, BalanceDontCare, 2, 542 }, /* 288 */
        {PositionalParams, BalanceDontCare, 2, 544 }, /* 289 */
        {PositionalParams, BalanceDontCare, 3, 546 }, /* 290 */
        {PositionalParams, BalanceDontCare, 2, 549 }, /* 291 */
        {PositionalParams, BalanceDontCare, 2, 551 }, /* 292 */
        {PositionalParams, BalanceDontCare, 2, 553 }, /* 293 */
        {PositionalParams, BalanceDontCare, 3, 555 }, /* 294 */
        {PositionalParams, BalanceDontCare, 2, 558 }, /* 295 */
        {PositionalParams, BalanceDontCare, 2, 560 }, /* 296 */
        {PositionalParams, BalanceDontCare, 1, 562 }, /* 297 */
        {PositionalParams, BalanceDontCare, 2, 563 }, /* 298 */
        {AnyParams       , BalanceDontCare, 3, 565 }, /* 299 */
        {PositionalParams, BalanceDontCare, 2, 568 }, /* 300 */
        {AnyParams       , BalanceDontCare, 3, 570 }, /* 301 */
        {AnyParams       , BalanceDontCare, 2, 573 }, /* 302 */
        {PositionalParams, BalanceDontCare, 2, 575 }, /* 303 */
        {PositionalParams, BalanceDontCare, 2, 577 }, /* 304 */
        {PositionalParams, BalanceDontCare, 2, 579 }, /* 305 */
        {PositionalParams, BalanceDontCare, 2, 581 }, /* 306 */
        {PositionalParams, BalanceDontCare, 2, 583 }, /* 307 */
        {PositionalParams, BalanceDontCare, 3, 585 }, /* 308 */
        {PositionalParams, BalanceDontCare, 2, 588 }, /* 309 */
        {PositionalParams, BalanceDontCare, 2, 590 }, /* 310 */
        {PositionalParams, BalanceDontCare, 2, 592 }, /* 311 */
        {PositionalParams, BalanceDontCare, 3, 594 }, /* 312 */
        {PositionalParams, BalanceDontCare, 2, 597 }, /* 313 */
        {PositionalParams, BalanceDontCare, 2, 599 }, /* 314 */
        {PositionalParams, BalanceDontCare, 1, 601 }, /* 315 */
        {SelectedParams  , BalanceDontCare, 2, 602 }, /* 316 */
        {SelectedParams  , BalanceDontCare, 2, 604 }, /* 317 */
        {AnyParams       , BalanceDontCare, 2, 606 }, /* 318 */
        {PositionalParams, BalanceDontCare, 2, 608 }, /* 319 */
        {PositionalParams, BalanceDontCare, 1, 610 }, /* 320 */
        {PositionalParams, BalanceDontCare, 1, 611 }, /* 321 */
        {SelectedParams  , BalanceDontCare, 2, 612 }, /* 322 */
        {PositionalParams, BalanceDontCare, 1, 36 }, /* 323 */
        {SelectedParams  , BalanceDontCare, 2, 614 }, /* 324 */
        {AnyParams       , BalanceDontCare, 2, 616 }, /* 325 */
        {PositionalParams, BalanceDontCare, 2, 618 }, /* 326 */
        {PositionalParams, BalanceDontCare, 1, 620 }, /* 327 */
        {PositionalParams, BalanceDontCare, 1, 621 }, /* 328 */
        {SelectedParams  , BalanceDontCare, 2, 622 }, /* 329 */
        {SelectedParams  , BalanceDontCare, 2, 624 }, /* 330 */
        {AnyParams       , BalanceDontCare, 2, 626 }, /* 331 */
        {PositionalParams, BalanceDontCare, 2, 628 }, /* 332 */
        {PositionalParams, BalanceDontCare, 1, 630 }, /* 333 */
        {PositionalParams, BalanceDontCare, 1, 631 }, /* 334 */
        {SelectedParams  , BalanceDontCare, 2, 632 }, /* 335 */
        {SelectedParams  , BalanceDontCare, 2, 634 }, /* 336 */
        {AnyParams       , BalanceDontCare, 2, 636 }, /* 337 */
        {PositionalParams, BalanceDontCare, 2, 638 }, /* 338 */
        {PositionalParams, BalanceDontCare, 1, 640 }, /* 339 */
        {PositionalParams, BalanceDontCare, 1, 641 }, /* 340 */
        {AnyParams       , BalanceDontCare, 3, 642 }, /* 341 */
        {AnyParams       , BalanceDontCare, 2, 645 }, /* 342 */
        {PositionalParams, BalanceDontCare, 2, 647 }, /* 343 */
        {PositionalParams, BalanceDontCare, 2, 649 }, /* 344 */
        {PositionalParams, BalanceDontCare, 2, 651 }, /* 345 */
        {PositionalParams, BalanceDontCare, 1, 653 }, /* 346 */
        {AnyParams       , BalanceDontCare, 3, 654 }, /* 347 */
        {AnyParams       , BalanceDontCare, 2, 657 }, /* 348 */
        {PositionalParams, BalanceDontCare, 2, 659 }, /* 349 */
        {PositionalParams, BalanceDontCare, 2, 661 }, /* 350 */
        {PositionalParams, BalanceDontCare, 2, 663 }, /* 351 */
        {PositionalParams, BalanceDontCare, 1, 665 }, /* 352 */
        {PositionalParams, BalanceDontCare, 1, 42 }, /* 353 */
        {SelectedParams  , BalanceDontCare, 2, 666 }, /* 354 */
        {SelectedParams  , BalanceDontCare, 2, 668 }, /* 355 */
        {AnyParams       , BalanceDontCare, 2, 670 }, /* 356 */
        {PositionalParams, BalanceDontCare, 2, 672 }, /* 357 */
        {PositionalParams, BalanceDontCare, 1, 674 }, /* 358 */
        {PositionalParams, BalanceDontCare, 1, 675 }, /* 359 */
        {SelectedParams  , BalanceDontCare, 2, 676 }, /* 360 */
        {SelectedParams  , BalanceDontCare, 2, 678 }, /* 361 */
        {AnyParams       , BalanceDontCare, 2, 680 }, /* 362 */
        {PositionalParams, BalanceDontCare, 2, 682 }, /* 363 */
        {PositionalParams, BalanceDontCare, 1, 684 }, /* 364 */
        {PositionalParams, BalanceDontCare, 1, 685 }, /* 365 */
        {SelectedParams  , BalanceDontCare, 2, 686 }, /* 366 */
        {AnyParams       , BalanceDontCare, 2, 688 }, /* 367 */
        {PositionalParams, BalanceDontCare, 2, 690 }, /* 368 */
        {PositionalParams, BalanceDontCare, 1, 692 }, /* 369 */
        {PositionalParams, BalanceDontCare, 1, 693 }, /* 370 */
        {AnyParams       , BalanceDontCare, 3, 694 }, /* 371 */
        {AnyParams       , BalanceDontCare, 2, 697 }, /* 372 */
        {PositionalParams, BalanceDontCare, 2, 699 }, /* 373 */
        {PositionalParams, BalanceDontCare, 2, 701 }, /* 374 */
        {PositionalParams, BalanceDontCare, 2, 703 }, /* 375 */
        {PositionalParams, BalanceDontCare, 1, 705 }, /* 376 */
        {AnyParams       , BalanceDontCare, 3, 706 }, /* 377 */
        {AnyParams       , BalanceDontCare, 2, 709 }, /* 378 */
        {PositionalParams, BalanceDontCare, 2, 711 }, /* 379 */
        {PositionalParams, BalanceDontCare, 2, 713 }, /* 380 */
        {PositionalParams, BalanceDontCare, 2, 715 }, /* 381 */
        {PositionalParams, BalanceDontCare, 1, 717 }, /* 382 */
        {AnyParams       , BalanceDontCare, 1, 718 }, /* 383 */
        {PositionalParams, BalanceDontCare, 2, 719 }, /* 384 */
        {PositionalParams, BalanceDontCare, 1, 721 }, /* 385 */
        {AnyParams       , BalanceDontCare, 1, 722 }, /* 386 */
        {PositionalParams, BalanceDontCare, 2, 723 }, /* 387 */
        {PositionalParams, BalanceDontCare, 1, 725 }, /* 388 */
        {PositionalParams, BalanceDontCare, 2, 726 }, /* 389 */
        {PositionalParams, BalanceDontCare, 2, 728 }, /* 390 */
        {AnyParams       , BalanceDontCare, 4, 730 }, /* 391 */
        {AnyParams       , BalanceDontCare, 3, 734 }, /* 392 */
        {PositionalParams, BalanceDontCare, 2, 737 }, /* 393 */
        {PositionalParams, BalanceDontCare, 2, 739 }, /* 394 */
        {PositionalParams, BalanceDontCare, 2, 741 }, /* 395 */
        {PositionalParams, BalanceDontCare, 2, 743 }, /* 396 */
        {PositionalParams, BalanceDontCare, 3, 745 }, /* 397 */
        {PositionalParams, BalanceDontCare, 2, 748 }, /* 398 */
        {AnyParams       , BalanceDontCare, 2, 752 }, /* 399 */
        {PositionalParams, BalanceDontCare, 1, 754 }, /* 400 */
        {PositionalParams, BalanceDontCare, 1, 755 }, /* 401 */
        {PositionalParams, BalanceDontCare, 1, 756 }, /* 402 */
        {AnyParams       , BalanceDontCare, 3, 757 }, /* 403 */
        {PositionalParams, BalanceDontCare, 2, 760 }, /* 404 */
        {PositionalParams, BalanceDontCare, 1, 762 }, /* 405 */
        {PositionalParams, BalanceDontCare, 1, 763 }, /* 406 */
        {AnyParams       , BalanceDontCare, 1, 33 }, /* 407 */
        {AnyParams       , BalanceDontCare, 2, 764 }, /* 408 */
        {AnyParams       , BalanceDontCare, 1, 766 }, /* 409 */
        {PositionalParams, BalanceDontCare, 2, 767 }, /* 410 */
        {PositionalParams, BalanceDontCare, 2, 769 }, /* 411 */
        {AnyParams       , BalanceDontCare, 1, 771 }, /* 412 */
        {PositionalParams, BalanceDontCare, 2, 772 }, /* 413 */
        {PositionalParams, BalanceDontCare, 1, 774 }, /* 414 */
        {AnyParams       , BalanceDontCare, 2, 775 }, /* 415 */
        {AnyParams       , BalanceDontCare, 1, 777 }, /* 416 */
        {AnyParams       , BalanceDontCare, 2, 778 }, /* 417 */
        {PositionalParams, BalanceDontCare, 1, 782 }, /* 418 */
        {AnyParams       , BalanceDontCare, 2, 783 }, /* 419 */
        {PositionalParams, BalanceDontCare, 1, 787 }, /* 420 */
        {PositionalParams, BalanceDontCare, 2, 788 }, /* 421 */
        {PositionalParams, BalanceDontCare, 1, 790 }, /* 422 */
        {AnyParams       , BalanceDontCare, 2, 791 }, /* 423 */
        {PositionalParams, BalanceDontCare, 2, 793 }, /* 424 */
        {PositionalParams, BalanceDontCare, 1, 795 }, /* 425 */
        {AnyParams       , BalanceDontCare, 2, 796 }, /* 426 */
        {PositionalParams, BalanceDontCare, 2, 798 }, /* 427 */
        {PositionalParams, BalanceDontCare, 1, 800 }, /* 428 */
        {AnyParams       , BalanceDontCare, 2, 801 }, /* 429 */
        {PositionalParams, BalanceDontCare, 2, 803 }, /* 430 */
        {AnyParams       , BalanceDontCare, 2, 805 }, /* 431 */
        {PositionalParams, BalanceDontCare, 1, 807 }, /* 432 */
        {AnyParams       , BalanceDontCare, 2, 808 }, /* 433 */
        {PositionalParams, BalanceDontCare, 2, 810 }, /* 434 */
        {PositionalParams, BalanceDontCare, 2, 812 }, /* 435 */
        {PositionalParams, BalanceDontCare, 1, 814 }, /* 436 */
        {PositionalParams, BalanceDontCare, 2, 815 }, /* 437 */
        {AnyParams       , BalanceDontCare, 2, 817 }, /* 438 */
        {PositionalParams, BalanceDontCare, 1, 819 }, /* 439 */
        {AnyParams       , BalanceDontCare, 2, 820 }, /* 440 */
        {PositionalParams, BalanceDontCare, 2, 822 }, /* 441 */
        {PositionalParams, BalanceDontCare, 2, 824 }, /* 442 */
        {PositionalParams, BalanceDontCare, 1, 826 }, /* 443 */
        {PositionalParams, BalanceDontCare, 2, 827 }, /* 444 */
        {AnyParams       , BalanceDontCare, 2, 829 }, /* 445 */
        {PositionalParams, BalanceDontCare, 1, 831 }, /* 446 */
        {PositionalParams, BalanceDontCare, 1, 167 }, /* 447 */
        {AnyParams       , BalanceDontCare, 2, 832 }, /* 448 */
        {PositionalParams, BalanceDontCare, 2, 834 }, /* 449 */
        {PositionalParams, BalanceDontCare, 2, 836 }, /* 450 */
        {PositionalParams, BalanceDontCare, 1, 838 }, /* 451 */
        {PositionalParams, BalanceDontCare, 2, 839 }, /* 452 */
        {PositionalParams, BalanceDontCare, 2, 841 }, /* 453 */
        {AnyParams       , BalanceDontCare, 2, 843 }, /* 454 */
        {PositionalParams, BalanceDontCare, 2, 845 }, /* 455 */
        {PositionalParams, BalanceDontCare, 2, 847 }, /* 456 */
        {PositionalParams, BalanceDontCare, 1, 849 }, /* 457 */
        {PositionalParams, BalanceDontCare, 2, 850 }, /* 458 */
        {PositionalParams, BalanceDontCare, 2, 852 }, /* 459 */
        {AnyParams       , BalanceDontCare, 2, 854 }, /* 460 */
        {PositionalParams, BalanceDontCare, 2, 856 }, /* 461 */
        {PositionalParams, BalanceDontCare, 2, 858 }, /* 462 */
        {PositionalParams, BalanceDontCare, 1, 860 }, /* 463 */
        {AnyParams       , BalanceDontCare, 2, 861 }, /* 464 */
        {PositionalParams, BalanceDontCare, 1, 863 }, /* 465 */
        {AnyParams       , BalanceDontCare, 2, 864 }, /* 466 */
        {PositionalParams, BalanceDontCare, 1, 866 }, /* 467 */
        {PositionalParams, BalanceDontCare, 2, 867 }, /* 468 */
        {AnyParams       , BalanceDontCare, 2, 869 }, /* 469 */
        {PositionalParams, BalanceDontCare, 2, 871 }, /* 470 */
        {PositionalParams, BalanceDontCare, 2, 873 }, /* 471 */
        {PositionalParams, BalanceDontCare, 1, 875 }, /* 472 */
        {PositionalParams, BalanceDontCare, 2, 876 }, /* 473 */
        {AnyParams       , BalanceDontCare, 2, 878 }, /* 474 */
        {PositionalParams, BalanceDontCare, 2, 880 }, /* 475 */
        {PositionalParams, BalanceDontCare, 2, 882 }, /* 476 */
        {PositionalParams, BalanceDontCare, 1, 884 }, /* 477 */
        {AnyParams       , BalanceDontCare, 2, 885 }, /* 478 */
        {PositionalParams, BalanceDontCare, 1, 889 }, /* 479 */
        {PositionalParams, BalanceDontCare, 2, 890 }, /* 480 */
        {PositionalParams, BalanceDontCare, 1, 892 }, /* 481 */
        {AnyParams       , BalanceDontCare, 2, 893 }, /* 482 */
        {PositionalParams, BalanceDontCare, 1, 895 }, /* 483 */
        {PositionalParams, BalanceDontCare, 2, 896 }, /* 484 */
        {PositionalParams, BalanceDontCare, 1, 898 }, /* 485 */
        {AnyParams       , BalanceDontCare, 2, 899 }, /* 486 */
        {PositionalParams, BalanceDontCare, 1, 619 }, /* 487 */
        {PositionalParams, BalanceDontCare, 2, 901 }, /* 488 */
        {PositionalParams, BalanceDontCare, 2, 903 }, /* 489 */
        {AnyParams       , BalanceDontCare, 2, 905 }, /* 490 */
        {PositionalParams, BalanceDontCare, 2, 907 }, /* 491 */
        {PositionalParams, BalanceDontCare, 2, 909 }, /* 492 */
        {PositionalParams, BalanceDontCare, 1, 911 }, /* 493 */
        {AnyParams       , BalanceDontCare, 2, 912 }, /* 494 */
        {AnyParams       , BalanceDontCare, 2, 914 }, /* 495 */
        {PositionalParams, BalanceDontCare, 1, 916 }, /* 496 */
        {PositionalParams, BalanceDontCare, 2, 917 }, /* 497 */
        {AnyParams       , BalanceDontCare, 2, 919 }, /* 498 */
        {PositionalParams, BalanceDontCare, 2, 921 }, /* 499 */
        {PositionalParams, BalanceDontCare, 2, 923 }, /* 500 */
        {PositionalParams, BalanceDontCare, 1, 925 }, /* 501 */
        {PositionalParams, BalanceDontCare, 2, 926 }, /* 502 */
        {AnyParams       , BalanceDontCare, 2, 928 }, /* 503 */
        {PositionalParams, BalanceDontCare, 2, 930 }, /* 504 */
        {PositionalParams, BalanceDontCare, 2, 932 }, /* 505 */
        {PositionalParams, BalanceDontCare, 1, 934 }, /* 506 */
        {AnyParams       , BalanceDontCare, 1, 720 }, /* 507 */
        {PositionalParams, BalanceDontCare, 2, 935 }, /* 508 */
        {PositionalParams, BalanceDontCare, 1, 937 }, /* 509 */
        {AnyParams       , BalanceDontCare, 1, 938 }, /* 510 */
        {PositionalParams, BalanceDontCare, 2, 939 }, /* 511 */
        {PositionalParams, BalanceDontCare, 1, 941 }, /* 512 */
        {PositionalParams, BalanceDontCare, 2, 942 }, /* 513 */
        {PositionalParams, BalanceDontCare, 1, 946 }, /* 514 */
        {PositionalParams, BalanceDontCare, 2, 947 }, /* 515 */
        {PositionalParams, BalanceDontCare, 1, 60 }, /* 516 */
        {PositionalParams, BalanceDontCare, 2, 949 }, /* 517 */
        {PositionalParams, BalanceDontCare, 2, 951 }, /* 518 */
        {PositionalParams, BalanceDontCare, 2, 953 }, /* 519 */
        {PositionalParams, BalanceDontCare, 2, 955 }, /* 520 */
        {PositionalParams, BalanceDontCare, 2, 957 }, /* 521 */
        {PositionalParams, BalanceDontCare, 2, 959 }, /* 522 */
        {PositionalParams, BalanceDontCare, 1, 961 }, /* 523 */
        {PositionalParams, BalanceDontCare, 2, 962 }, /* 524 */
        {PositionalParams, BalanceDontCare, 1, 964 }, /* 525 */
        {PositionalParams, BalanceDontCare, 2, 965 }, /* 526 */
        {PositionalParams, BalanceDontCare, 1, 967 }, /* 527 */
        {PositionalParams, BalanceDontCare, 2, 968 }, /* 528 */
        {PositionalParams, BalanceDontCare, 1, 970 }, /* 529 */
        {PositionalParams, BalanceDontCare, 2, 971 }, /* 530 */
        {PositionalParams, BalanceDontCare, 1, 973 }, /* 531 */
        {PositionalParams, BalanceDontCare, 2, 974 }, /* 532 */
        {PositionalParams, BalanceDontCare, 1, 976 }, /* 533 */
        {PositionalParams, BalanceDontCare, 2, 977 }, /* 534 */
        {PositionalParams, BalanceDontCare, 1, 979 }, /* 535 */
        {PositionalParams, BalanceDontCare, 2, 980 }, /* 536 */
        {PositionalParams, BalanceDontCare, 1, 982 }, /* 537 */
        {PositionalParams, BalanceDontCare, 2, 983 }, /* 538 */
        {PositionalParams, BalanceDontCare, 1, 985 }, /* 539 */
        {PositionalParams, BalanceDontCare, 2, 986 }, /* 540 */
        {PositionalParams, BalanceDontCare, 1, 988 }, /* 541 */
        {PositionalParams, BalanceDontCare, 2, 989 }, /* 542 */
        {PositionalParams, BalanceDontCare, 1, 991 }, /* 543 */
        {PositionalParams, BalanceDontCare, 2, 992 }, /* 544 */
        {PositionalParams, BalanceDontCare, 1, 994 }, /* 545 */
        {AnyParams       , BalanceMoreNeg , 2, 995 }, /* 546 */
        {PositionalParams, BalanceDontCare, 2, 997 }, /* 547 */
        {PositionalParams, BalanceDontCare, 1, 999 }, /* 548 */
        {PositionalParams, BalanceDontCare, 1, 1000 }, /* 549 */
        {PositionalParams, BalanceDontCare, 1, 1001 }, /* 550 */
        {AnyParams       , BalanceDontCare, 1, 1002 }, /* 551 */
        {AnyParams       , BalanceDontCare, 2, 1003 }, /* 552 */
        {AnyParams       , BalanceDontCare, 1, 1005 }, /* 553 */
        {PositionalParams, BalanceDontCare, 2, 1006 }, /* 554 */
        {AnyParams       , BalanceDontCare, 1, 1000 }, /* 555 */
        {PositionalParams, BalanceDontCare, 2, 1008 }, /* 556 */
        {AnyParams       , BalanceDontCare, 1, 1010 }, /* 557 */
        {PositionalParams, BalanceDontCare, 2, 1011 }, /* 558 */
        {PositionalParams, BalanceDontCare, 1, 1013 }, /* 559 */
        {PositionalParams, BalanceDontCare, 2, 1014 }, /* 560 */
        {AnyParams       , BalanceDontCare, 1, 1016 }, /* 561 */
        {PositionalParams, BalanceDontCare, 2, 1017 }, /* 562 */
        {PositionalParams, BalanceDontCare, 1, 1019 }, /* 563 */
        {PositionalParams, BalanceDontCare, 2, 1020 }, /* 564 */
        {AnyParams       , BalanceDontCare, 1, 1022 }, /* 565 */
        {PositionalParams, BalanceDontCare, 2, 1023 }, /* 566 */
        {PositionalParams, BalanceDontCare, 1, 1025 }, /* 567 */
        {PositionalParams, BalanceDontCare, 2, 1026 }, /* 568 */
        {AnyParams       , BalanceDontCare, 1, 1028 }, /* 569 */
        {PositionalParams, BalanceDontCare, 2, 1029 }, /* 570 */
        {PositionalParams, BalanceDontCare, 1, 1031 }, /* 571 */
        {PositionalParams, BalanceDontCare, 2, 1032 }, /* 572 */
        {AnyParams       , BalanceDontCare, 1, 1034 }, /* 573 */
        {PositionalParams, BalanceDontCare, 2, 1035 }, /* 574 */
        {PositionalParams, BalanceDontCare, 1, 1037 }, /* 575 */
        {PositionalParams, BalanceDontCare, 2, 1038 }, /* 576 */
        {AnyParams       , BalanceDontCare, 1, 1040 }, /* 577 */
        {PositionalParams, BalanceDontCare, 2, 1041 }, /* 578 */
        {PositionalParams, BalanceDontCare, 1, 1043 }, /* 579 */
        {AnyParams       , BalanceDontCare, 1, 1044 }, /* 580 */
        {AnyParams       , BalanceDontCare, 2, 54 }, /* 581 */
        {AnyParams       , BalanceDontCare, 1, 1045 }, /* 582 */
        {AnyParams       , BalanceDontCare, 1, 1046 }, /* 583 */
        {AnyParams       , BalanceDontCare, 2, 1047 }, /* 584 */
        {AnyParams       , BalanceDontCare, 2, 1049 }, /* 585 */
        {AnyParams       , BalanceDontCare, 2, 1051 }, /* 586 */
        {AnyParams       , BalanceDontCare, 2, 1053 }, /* 587 */
        {AnyParams       , BalanceDontCare, 2, 1055 }, /* 588 */
        {AnyParams       , BalanceDontCare, 2, 1057 }, /* 589 */
        {AnyParams       , BalanceDontCare, 2, 1059 }, /* 590 */
        {AnyParams       , BalanceDontCare, 2, 1061 }, /* 591 */
        {AnyParams       , BalanceDontCare, 3, 1063 }, /* 592 */
        {PositionalParams, BalanceDontCare, 2, 1066 }, /* 593 */
        {PositionalParams, BalanceDontCare, 2, 1068 }, /* 594 */
        {PositionalParams, BalanceDontCare, 2, 1070 }, /* 595 */
        {AnyParams       , BalanceMoreNeg , 2, 1072 }, /* 596 */
        {PositionalParams, BalanceDontCare, 2, 1074 }, /* 597 */
        {PositionalParams, BalanceDontCare, 1, 1076 }, /* 598 */
        {AnyParams       , BalanceDontCare, 1, 1001 }, /* 599 */
        {AnyParams       , BalanceDontCare, 2, 1077 }, /* 600 */
        {AnyParams       , BalanceDontCare, 1, 1079 }, /* 601 */
        {PositionalParams, BalanceDontCare, 2, 1080 }, /* 602 */
        {PositionalParams, BalanceDontCare, 2, 1082 }, /* 603 */
        {AnyParams       , BalanceDontCare, 1, 1084 }, /* 604 */
        {PositionalParams, BalanceDontCare, 2, 1085 }, /* 605 */
        {PositionalParams, BalanceDontCare, 1, 1087 }, /* 606 */
        {PositionalParams, BalanceDontCare, 2, 1088 }, /* 607 */
        {AnyParams       , BalanceDontCare, 1, 1090 }, /* 608 */
        {PositionalParams, BalanceDontCare, 2, 1091 }, /* 609 */
        {PositionalParams, BalanceDontCare, 1, 1093 }, /* 610 */
        {PositionalParams, BalanceDontCare, 2, 1094 }, /* 611 */
        {AnyParams       , BalanceDontCare, 1, 1096 }, /* 612 */
        {PositionalParams, BalanceDontCare, 2, 1097 }, /* 613 */
        {PositionalParams, BalanceDontCare, 1, 1099 }, /* 614 */
        {PositionalParams, BalanceDontCare, 2, 1100 }, /* 615 */
        {AnyParams       , BalanceDontCare, 1, 1102 }, /* 616 */
        {PositionalParams, BalanceDontCare, 2, 1103 }, /* 617 */
        {PositionalParams, BalanceDontCare, 1, 1105 }, /* 618 */
        {PositionalParams, BalanceDontCare, 2, 1106 }, /* 619 */
        {AnyParams       , BalanceDontCare, 1, 1108 }, /* 620 */
        {PositionalParams, BalanceDontCare, 2, 1109 }, /* 621 */
        {PositionalParams, BalanceDontCare, 1, 1111 }, /* 622 */
        {PositionalParams, BalanceDontCare, 2, 1112 }, /* 623 */
        {AnyParams       , BalanceDontCare, 1, 1114 }, /* 624 */
        {PositionalParams, BalanceDontCare, 2, 1115 }, /* 625 */
        {PositionalParams, BalanceDontCare, 1, 1117 }, /* 626 */
        {AnyParams       , BalanceDontCare, 1, 1118 }, /* 627 */
        {AnyParams       , BalanceDontCare, 2, 1119 }, /* 628 */
        {AnyParams       , BalanceDontCare, 1, 1121 }, /* 629 */
        {PositionalParams, BalanceDontCare, 2, 1122 }, /* 630 */
        {AnyParams       , BalanceDontCare, 2, 1124 }, /* 631 */
        {AnyParams       , BalanceDontCare, 2, 1126 }, /* 632 */
        {AnyParams       , BalanceDontCare, 2, 1128 }, /* 633 */
        {AnyParams       , BalanceDontCare, 2, 1130 }, /* 634 */
        {AnyParams       , BalanceDontCare, 2, 1132 }, /* 635 */
        {PositionalParams, BalanceDontCare, 2, 1134 }, /* 636 */
        {PositionalParams, BalanceDontCare, 1, 1136 }, /* 637 */
        {PositionalParams, BalanceDontCare, 2, 1137 }, /* 638 */
        {PositionalParams, BalanceDontCare, 1, 1139 }, /* 639 */
        {PositionalParams, BalanceDontCare, 2, 1140 }, /* 640 */
        {PositionalParams, BalanceDontCare, 1, 1142 }, /* 641 */
        {PositionalParams, BalanceDontCare, 2, 1143 }, /* 642 */
        {PositionalParams, BalanceDontCare, 1, 1145 }, /* 643 */
        {PositionalParams, BalanceDontCare, 2, 1146 }, /* 644 */
        {PositionalParams, BalanceDontCare, 1, 1148 }, /* 645 */
        {PositionalParams, BalanceDontCare, 2, 1149 }, /* 646 */
        {PositionalParams, BalanceDontCare, 1, 1151 }, /* 647 */
        {PositionalParams, BalanceDontCare, 2, 1152 }, /* 648 */
        {PositionalParams, BalanceDontCare, 1, 1154 }, /* 649 */
        {PositionalParams, BalanceDontCare, 2, 1155 }, /* 650 */
        {PositionalParams, BalanceDontCare, 1, 1157 }, /* 651 */
        {PositionalParams, BalanceDontCare, 2, 1158 }, /* 652 */
        {PositionalParams, BalanceDontCare, 1, 1160 }, /* 653 */
        {PositionalParams, BalanceDontCare, 2, 1161 }, /* 654 */
        {PositionalParams, BalanceDontCare, 1, 1163 }, /* 655 */
        {PositionalParams, BalanceDontCare, 2, 1164 }, /* 656 */
        {PositionalParams, BalanceDontCare, 1, 1166 }, /* 657 */
        {PositionalParams, BalanceDontCare, 2, 1167 }, /* 658 */
        {PositionalParams, BalanceDontCare, 1, 1169 }, /* 659 */
        {PositionalParams, BalanceDontCare, 1, 1170 }, /* 660 */
        {AnyParams       , BalanceDontCare, 2, 1171 }, /* 661 */
        {PositionalParams, BalanceDontCare, 1, 1173 }, /* 662 */
        {PositionalParams, BalanceDontCare, 2, 1174 }, /* 663 */
        {PositionalParams, BalanceDontCare, 1, 1176 }, /* 664 */
        {AnyParams       , BalanceDontCare, 2, 1177 }, /* 665 */
        {PositionalParams, BalanceDontCare, 1, 1179 }, /* 666 */
        {PositionalParams, BalanceDontCare, 2, 1180 }, /* 667 */
        {PositionalParams, BalanceDontCare, 1, 1182 }, /* 668 */
        {PositionalParams, BalanceDontCare, 2, 1183 }, /* 669 */
        {PositionalParams, BalanceDontCare, 1, 1185 }, /* 670 */
        {PositionalParams, BalanceDontCare, 2, 1186 }, /* 671 */
        {PositionalParams, BalanceDontCare, 1, 1188 }, /* 672 */
        {PositionalParams, BalanceDontCare, 2, 1189 }, /* 673 */
        {PositionalParams, BalanceDontCare, 1, 1191 }, /* 674 */
        {AnyParams       , BalanceDontCare, 3, 1192 }, /* 675 */
        {PositionalParams, BalanceDontCare, 2, 1195 }, /* 676 */
        {PositionalParams, BalanceDontCare, 1, 1197 }, /* 677 */
        {PositionalParams, BalanceDontCare, 1, 1198 }, /* 678 */
        {AnyParams       , BalanceDontCare, 3, 1199 }, /* 679 */
        {PositionalParams, BalanceDontCare, 2, 1202 }, /* 680 */
        {PositionalParams, BalanceDontCare, 1, 1204 }, /* 681 */
        {PositionalParams, BalanceDontCare, 1, 1205 }, /* 682 */
        {AnyParams       , BalanceDontCare, 1, 1206 }, /* 683 */
        {PositionalParams, BalanceDontCare, 1, 1207 }, /* 684 */
        {AnyParams       , BalanceDontCare, 1, 292 }, /* 685 */
        {PositionalParams, BalanceDontCare, 1, 1208 }, /* 686 */
        {AnyParams       , BalanceDontCare, 1, 305 }, /* 687 */
        {PositionalParams, BalanceDontCare, 1, 1209 }, /* 688 */
        {AnyParams       , BalanceDontCare, 2, 1210 }, /* 689 */
        {PositionalParams, BalanceDontCare, 1, 1212 }, /* 690 */
        {AnyParams       , BalanceDontCare, 2, 1213 }, /* 691 */
        {PositionalParams, BalanceDontCare, 1, 1215 }, /* 692 */
        {AnyParams       , BalanceDontCare, 2, 1216 }, /* 693 */
        {PositionalParams, BalanceDontCare, 1, 1218 }, /* 694 */
        {AnyParams       , BalanceDontCare, 2, 1219 }, /* 695 */
        {PositionalParams, BalanceDontCare, 1, 1221 }, /* 696 */
        {PositionalParams, BalanceDontCare, 1, 1222 }, /* 697 */
    };

    const Function flist[] =
    {
        {cNot        , 0 }, /* 0 */
        {cNotNot     , 2 }, /* 1 */
        {cAcos       , 16 }, /* 2 */
        {cAdd        , 19 }, /* 3 */
        {cAdd        , 21 }, /* 4 */
        {cSin        , 18 }, /* 5 */
        {cAdd        , 30 }, /* 6 */
        {cAdd        , 32 }, /* 7 */
        {cIf         , 33 }, /* 8 */
        {cAdd        , 34 }, /* 9 */
        {cMul        , 36 }, /* 10 */
        {cMul        , 38 }, /* 11 */
        {cIf         , 39 }, /* 12 */
        {cMul        , 40 }, /* 13 */
        {cAnd        , 42 }, /* 14 */
        {cAnd        , 44 }, /* 15 */
        {cIf         , 45 }, /* 16 */
        {cAnd        , 46 }, /* 17 */
        {cOr         , 48 }, /* 18 */
        {cOr         , 50 }, /* 19 */
        {cIf         , 51 }, /* 20 */
        {cOr         , 52 }, /* 21 */
        {cAdd        , 54 }, /* 22 */
        {cAdd        , 55 }, /* 23 */
        {cAdd        , 57 }, /* 24 */
        {cAdd        , 58 }, /* 25 */
        {cIf         , 59 }, /* 26 */
        {cAdd        , 60 }, /* 27 */
        {cMul        , 62 }, /* 28 */
        {cMul        , 63 }, /* 29 */
        {cMul        , 65 }, /* 30 */
        {cMul        , 66 }, /* 31 */
        {cIf         , 67 }, /* 32 */
        {cMul        , 68 }, /* 33 */
        {cAnd        , 70 }, /* 34 */
        {cAnd        , 71 }, /* 35 */
        {cAnd        , 73 }, /* 36 */
        {cAnd        , 58 }, /* 37 */
        {cIf         , 74 }, /* 38 */
        {cAnd        , 75 }, /* 39 */
        {cOr         , 77 }, /* 40 */
        {cOr         , 78 }, /* 41 */
        {cOr         , 80 }, /* 42 */
        {cOr         , 81 }, /* 43 */
        {cIf         , 82 }, /* 44 */
        {cOr         , 83 }, /* 45 */
        {cNot        , 85 }, /* 46 */
        {cIf         , 87 }, /* 47 */
        {cNotNot     , 85 }, /* 48 */
        {cIf         , 90 }, /* 49 */
        {cPow        , 93 }, /* 50 */
        {cLog        , 16 }, /* 51 */
        {cMul        , 95 }, /* 52 */
        {cPow        , 97 }, /* 53 */
        {cMul        , 98 }, /* 54 */
        {cMul        , 100 }, /* 55 */
        {cLog        , 101 }, /* 56 */
        {cAdd        , 102 }, /* 57 */
        {cMax        , 104 }, /* 58 */
        {cMin        , 110 }, /* 59 */
        {cLog        , 18 }, /* 60 */
        {cLog        , 2 }, /* 61 */
        {cMul        , 120 }, /* 62 */
        {cMul        , 122 }, /* 63 */
        {cMul        , 126 }, /* 64 */
        {cMul        , 128 }, /* 65 */
        {cMul        , 130 }, /* 66 */
        {cMul        , 132 }, /* 67 */
        {cMul        , 135 }, /* 68 */
        {cAdd        , 139 }, /* 69 */
        {cAdd        , 141 }, /* 70 */
        {cPow        , 142 }, /* 71 */
        {cMul        , 143 }, /* 72 */
        {cLog        , 145 }, /* 73 */
        {cMul        , 146 }, /* 74 */
        {cMul        , 148 }, /* 75 */
        {cPow        , 150 }, /* 76 */
        {cMul        , 152 }, /* 77 */
        {cAsin       , 18 }, /* 78 */
        {cAdd        , 135 }, /* 79 */
        {cSin        , 2 }, /* 80 */
        {cAdd        , 157 }, /* 81 */
        {cAdd        , 159 }, /* 82 */
        {cCos        , 2 }, /* 83 */
        {cAtan       , 0 }, /* 84 */
        {cAdd        , 165 }, /* 85 */
        {cTan        , 18 }, /* 86 */
        {cAdd        , 167 }, /* 87 */
        {cAdd        , 173 }, /* 88 */
        {cAdd        , 176 }, /* 89 */
        {cMul        , 177 }, /* 90 */
        {cAdd        , 179 }, /* 91 */
        {cMul        , 180 }, /* 92 */
        {cAdd        , 184 }, /* 93 */
        {cMul        , 187 }, /* 94 */
        {cMul        , 189 }, /* 95 */
        {cAdd        , 191 }, /* 96 */
        {cMul        , 192 }, /* 97 */
        {cAdd        , 194 }, /* 98 */
        {cMul        , 195 }, /* 99 */
        {cCos        , 18 }, /* 100 */
        {cPow        , 197 }, /* 101 */
        {cPow        , 199 }, /* 102 */
        {cPow        , 201 }, /* 103 */
        {cPow        , 203 }, /* 104 */
        {cLog        , 208 }, /* 105 */
        {cMul        , 210 }, /* 106 */
        {cLog        , 211 }, /* 107 */
        {cPow        , 213 }, /* 108 */
        {cMul        , 216 }, /* 109 */
        {cMul        , 217 }, /* 110 */
        {cMul        , 58 }, /* 111 */
        {cMul        , 219 }, /* 112 */
        {cAdd        , 220 }, /* 113 */
        {cMul        , 221 }, /* 114 */
        {cMul        , 223 }, /* 115 */
        {cMul        , 224 }, /* 116 */
        {cAdd        , 226 }, /* 117 */
        {cMul        , 227 }, /* 118 */
        {cMul        , 229 }, /* 119 */
        {cPow        , 230 }, /* 120 */
        {cMul        , 231 }, /* 121 */
        {cMin        , 233 }, /* 122 */
        {cPow        , 234 }, /* 123 */
        {cMin        , 235 }, /* 124 */
        {cAdd        , 236 }, /* 125 */
        {cPow        , 237 }, /* 126 */
        {cMul        , 238 }, /* 127 */
        {cMin        , 239 }, /* 128 */
        {cAdd        , 240 }, /* 129 */
        {cPow        , 241 }, /* 130 */
        {cMul        , 242 }, /* 131 */
        {cAdd        , 243 }, /* 132 */
        {cMul        , 244 }, /* 133 */
        {cPow        , 246 }, /* 134 */
        {cMul        , 247 }, /* 135 */
        {cMul        , 248 }, /* 136 */
        {cMin        , 250 }, /* 137 */
        {cPow        , 251 }, /* 138 */
        {cMin        , 252 }, /* 139 */
        {cAdd        , 253 }, /* 140 */
        {cPow        , 254 }, /* 141 */
        {cMul        , 255 }, /* 142 */
        {cMin        , 256 }, /* 143 */
        {cAdd        , 257 }, /* 144 */
        {cPow        , 258 }, /* 145 */
        {cMul        , 259 }, /* 146 */
        {cAdd        , 260 }, /* 147 */
        {cMul        , 261 }, /* 148 */
        {cPow        , 263 }, /* 149 */
        {cMul        , 264 }, /* 150 */
        {cPow        , 265 }, /* 151 */
        {cMul        , 266 }, /* 152 */
        {cMin        , 268 }, /* 153 */
        {cPow        , 269 }, /* 154 */
        {cMin        , 270 }, /* 155 */
        {cAdd        , 271 }, /* 156 */
        {cPow        , 272 }, /* 157 */
        {cMul        , 273 }, /* 158 */
        {cMin        , 274 }, /* 159 */
        {cAdd        , 275 }, /* 160 */
        {cPow        , 276 }, /* 161 */
        {cMul        , 277 }, /* 162 */
        {cAdd        , 278 }, /* 163 */
        {cMul        , 279 }, /* 164 */
        {cPow        , 281 }, /* 165 */
        {cMul        , 282 }, /* 166 */
        {cMul        , 283 }, /* 167 */
        {cMin        , 285 }, /* 168 */
        {cPow        , 286 }, /* 169 */
        {cMin        , 287 }, /* 170 */
        {cAdd        , 288 }, /* 171 */
        {cPow        , 289 }, /* 172 */
        {cMul        , 290 }, /* 173 */
        {cMin        , 291 }, /* 174 */
        {cAdd        , 292 }, /* 175 */
        {cPow        , 293 }, /* 176 */
        {cMul        , 294 }, /* 177 */
        {cAdd        , 295 }, /* 178 */
        {cMul        , 296 }, /* 179 */
        {cPow        , 298 }, /* 180 */
        {cMul        , 299 }, /* 181 */
        {cPow        , 300 }, /* 182 */
        {cMul        , 301 }, /* 183 */
        {cMin        , 303 }, /* 184 */
        {cPow        , 304 }, /* 185 */
        {cMin        , 305 }, /* 186 */
        {cAdd        , 306 }, /* 187 */
        {cPow        , 307 }, /* 188 */
        {cMul        , 308 }, /* 189 */
        {cMin        , 309 }, /* 190 */
        {cAdd        , 310 }, /* 191 */
        {cPow        , 311 }, /* 192 */
        {cMul        , 312 }, /* 193 */
        {cAdd        , 313 }, /* 194 */
        {cMul        , 314 }, /* 195 */
        {cCos        , 208 }, /* 196 */
        {cMul        , 316 }, /* 197 */
        {cSin        , 208 }, /* 198 */
        {cMul        , 317 }, /* 199 */
        {cAdd        , 319 }, /* 200 */
        {cCos        , 320 }, /* 201 */
        {cMul        , 322 }, /* 202 */
        {cSin        , 323 }, /* 203 */
        {cMul        , 324 }, /* 204 */
        {cAdd        , 326 }, /* 205 */
        {cCos        , 327 }, /* 206 */
        {cCos        , 323 }, /* 207 */
        {cMul        , 329 }, /* 208 */
        {cMul        , 330 }, /* 209 */
        {cAdd        , 332 }, /* 210 */
        {cSin        , 333 }, /* 211 */
        {cMul        , 335 }, /* 212 */
        {cMul        , 336 }, /* 213 */
        {cAdd        , 338 }, /* 214 */
        {cSin        , 339 }, /* 215 */
        {cMul        , 341 }, /* 216 */
        {cMul        , 343 }, /* 217 */
        {cAdd        , 344 }, /* 218 */
        {cMul        , 345 }, /* 219 */
        {cMul        , 347 }, /* 220 */
        {cMul        , 349 }, /* 221 */
        {cAdd        , 350 }, /* 222 */
        {cMul        , 351 }, /* 223 */
        {cCos        , 0 }, /* 224 */
        {cCos        , 353 }, /* 225 */
        {cMul        , 354 }, /* 226 */
        {cMul        , 355 }, /* 227 */
        {cAdd        , 357 }, /* 228 */
        {cCos        , 358 }, /* 229 */
        {cMul        , 360 }, /* 230 */
        {cMul        , 361 }, /* 231 */
        {cAdd        , 363 }, /* 232 */
        {cCos        , 364 }, /* 233 */
        {cMul        , 366 }, /* 234 */
        {cAdd        , 368 }, /* 235 */
        {cSin        , 369 }, /* 236 */
        {cMul        , 371 }, /* 237 */
        {cMul        , 373 }, /* 238 */
        {cAdd        , 374 }, /* 239 */
        {cMul        , 375 }, /* 240 */
        {cMul        , 377 }, /* 241 */
        {cMul        , 379 }, /* 242 */
        {cAdd        , 380 }, /* 243 */
        {cMul        , 381 }, /* 244 */
        {cMul        , 384 }, /* 245 */
        {cMul        , 387 }, /* 246 */
        {cPow        , 389 }, /* 247 */
        {cPow        , 390 }, /* 248 */
        {cMul        , 391 }, /* 249 */
        {cAdd        , 393 }, /* 250 */
        {cPow        , 394 }, /* 251 */
        {cMul        , 395 }, /* 252 */
        {cAdd        , 396 }, /* 253 */
        {cMul        , 397 }, /* 254 */
        {cAdd        , 399 }, /* 255 */
        {cTan        , 400 }, /* 256 */
        {cMul        , 404 }, /* 257 */
        {cAdd        , 405 }, /* 258 */
        {cMul        , 408 }, /* 259 */
        {cPow        , 411 }, /* 260 */
        {cPow        , 413 }, /* 261 */
        {cMul        , 415 }, /* 262 */
        {cPow        , 421 }, /* 263 */
        {cLog        , 422 }, /* 264 */
        {cPow        , 424 }, /* 265 */
        {cLog        , 425 }, /* 266 */
        {cPow        , 427 }, /* 267 */
        {cLog        , 428 }, /* 268 */
        {cPow        , 430 }, /* 269 */
        {cMul        , 431 }, /* 270 */
        {cLog        , 432 }, /* 271 */
        {cMul        , 434 }, /* 272 */
        {cAdd        , 435 }, /* 273 */
        {cPow        , 437 }, /* 274 */
        {cMul        , 438 }, /* 275 */
        {cLog        , 439 }, /* 276 */
        {cMul        , 441 }, /* 277 */
        {cAdd        , 442 }, /* 278 */
        {cPow        , 444 }, /* 279 */
        {cMul        , 445 }, /* 280 */
        {cLog        , 446 }, /* 281 */
        {cLog        , 447 }, /* 282 */
        {cMul        , 449 }, /* 283 */
        {cAdd        , 450 }, /* 284 */
        {cPow        , 452 }, /* 285 */
        {cPow        , 453 }, /* 286 */
        {cAdd        , 455 }, /* 287 */
        {cPow        , 456 }, /* 288 */
        {cPow        , 458 }, /* 289 */
        {cPow        , 459 }, /* 290 */
        {cAdd        , 461 }, /* 291 */
        {cPow        , 462 }, /* 292 */
        {cSin        , 0 }, /* 293 */
        {cTan        , 2 }, /* 294 */
        {cSinh       , 2 }, /* 295 */
        {cCosh       , 2 }, /* 296 */
        {cTanh       , 2 }, /* 297 */
        {cPow        , 468 }, /* 298 */
        {cAdd        , 470 }, /* 299 */
        {cPow        , 471 }, /* 300 */
        {cPow        , 473 }, /* 301 */
        {cAdd        , 475 }, /* 302 */
        {cPow        , 476 }, /* 303 */
        {cPow        , 480 }, /* 304 */
        {cLog        , 481 }, /* 305 */
        {cPow        , 484 }, /* 306 */
        {cLog        , 485 }, /* 307 */
        {cPow        , 488 }, /* 308 */
        {cPow        , 489 }, /* 309 */
        {cAdd        , 491 }, /* 310 */
        {cPow        , 492 }, /* 311 */
        {cSinh       , 18 }, /* 312 */
        {cCosh       , 18 }, /* 313 */
        {cPow        , 497 }, /* 314 */
        {cAdd        , 499 }, /* 315 */
        {cPow        , 500 }, /* 316 */
        {cPow        , 502 }, /* 317 */
        {cAdd        , 504 }, /* 318 */
        {cPow        , 505 }, /* 319 */
        {cPow        , 508 }, /* 320 */
        {cPow        , 511 }, /* 321 */
        {cEqual      , 522 }, /* 322 */
        {cNEqual     , 524 }, /* 323 */
        {cNEqual     , 526 }, /* 324 */
        {cEqual      , 528 }, /* 325 */
        {cLess       , 530 }, /* 326 */
        {cGreaterOrEq, 532 }, /* 327 */
        {cLessOrEq   , 534 }, /* 328 */
        {cGreater    , 536 }, /* 329 */
        {cGreater    , 538 }, /* 330 */
        {cLessOrEq   , 540 }, /* 331 */
        {cGreaterOrEq, 542 }, /* 332 */
        {cLess       , 544 }, /* 333 */
        {cOr         , 547 }, /* 334 */
        {cNotNot     , 18 }, /* 335 */
        {cNot        , 18 }, /* 336 */
        {cNot        , 2 }, /* 337 */
        {cAnd        , 552 }, /* 338 */
        {cEqual      , 556 }, /* 339 */
        {cNEqual     , 558 }, /* 340 */
        {cNEqual     , 560 }, /* 341 */
        {cEqual      , 562 }, /* 342 */
        {cLess       , 564 }, /* 343 */
        {cGreaterOrEq, 566 }, /* 344 */
        {cLessOrEq   , 568 }, /* 345 */
        {cGreater    , 570 }, /* 346 */
        {cGreater    , 572 }, /* 347 */
        {cLessOrEq   , 574 }, /* 348 */
        {cGreaterOrEq, 576 }, /* 349 */
        {cLess       , 578 }, /* 350 */
        {cAnd        , 581 }, /* 351 */
        {cEqual      , 585 }, /* 352 */
        {cNEqual     , 586 }, /* 353 */
        {cEqual      , 589 }, /* 354 */
        {cEqual      , 590 }, /* 355 */
        {cEqual      , 591 }, /* 356 */
        {cEqual      , 593 }, /* 357 */
        {cEqual      , 594 }, /* 358 */
        {cAnd        , 597 }, /* 359 */
        {cOr         , 600 }, /* 360 */
        {cEqual      , 603 }, /* 361 */
        {cNEqual     , 605 }, /* 362 */
        {cNEqual     , 607 }, /* 363 */
        {cEqual      , 609 }, /* 364 */
        {cLess       , 611 }, /* 365 */
        {cGreaterOrEq, 613 }, /* 366 */
        {cLessOrEq   , 615 }, /* 367 */
        {cGreater    , 617 }, /* 368 */
        {cGreater    , 619 }, /* 369 */
        {cLessOrEq   , 621 }, /* 370 */
        {cGreaterOrEq, 623 }, /* 371 */
        {cLess       , 625 }, /* 372 */
        {cOr         , 628 }, /* 373 */
        {cEqual      , 632 }, /* 374 */
        {cNEqual     , 633 }, /* 375 */
        {cEqual      , 636 }, /* 376 */
        {cEqual      , 638 }, /* 377 */
        {cNEqual     , 640 }, /* 378 */
        {cNEqual     , 642 }, /* 379 */
        {cLess       , 644 }, /* 380 */
        {cLess       , 646 }, /* 381 */
        {cLessOrEq   , 648 }, /* 382 */
        {cLessOrEq   , 650 }, /* 383 */
        {cGreater    , 652 }, /* 384 */
        {cGreater    , 654 }, /* 385 */
        {cGreaterOrEq, 656 }, /* 386 */
        {cGreaterOrEq, 658 }, /* 387 */
        {cNot        , 549 }, /* 388 */
        {cAnd        , 661 }, /* 389 */
        {cAnd        , 663 }, /* 390 */
        {cOr         , 665 }, /* 391 */
        {cOr         , 667 }, /* 392 */
        {cExp        , 2 }, /* 393 */
        {cRSqrt      , 2 }, /* 394 */
        {cSqrt       , 18 }, /* 395 */
        {cMul        , 676 }, /* 396 */
        {cRad        , 677 }, /* 397 */
        {cMul        , 680 }, /* 398 */
        {cDeg        , 681 }, /* 399 */
        {cSec        , 2 }, /* 400 */
        {cCsc        , 2 }, /* 401 */
        {cCot        , 18 }, /* 402 */
        {cLog10      , 18 }, /* 403 */
        {cLog2       , 18 }, /* 404 */
        {cLog10      , 2 }, /* 405 */
        {cNot        , 550 }, /* 406 */
    };

    const Rule rlist[] =
    {
        {1, ProduceNewTree,    3,	{ cNot        , 1 } }, /* 0 */
        {1, ProduceNewTree,    5,	{ cAcos       , 4 } }, /* 1 */
        {1, ProduceNewTree,    7,	{ cAcosh      , 6 } }, /* 2 */
        {1, ProduceNewTree,    8,	{ cAsin       , 4 } }, /* 3 */
        {1, ProduceNewTree,    9,	{ cAsinh      , 4 } }, /* 4 */
        {1, ProduceNewTree,    11,	{ cAtan       , 10 } }, /* 5 */
        {1, ProduceNewTree,    12,	{ cAtanh      , 6 } }, /* 6 */
        {1, ProduceNewTree,    13,	{ cCeil       , 10 } }, /* 7 */
        {1, ProduceNewTree,    15,	{ cCos        , 14 } }, /* 8 */
        {1, ProduceNewTree,    18,	{ cCos        , 17 } }, /* 9 */
        {1, ReplaceParams ,    18,	{ cCos        , 20 } }, /* 10 */
        {1, ProduceNewTree,    23,	{ cCos        , 22 } }, /* 11 */
        {1, ProduceNewTree,    24,	{ cCosh       , 6 } }, /* 12 */
        {1, ProduceNewTree,    25,	{ cFloor      , 6 } }, /* 13 */
        {3, ProduceNewTree,    27,	{ cIf         , 26 } }, /* 14 */
        {3, ProduceNewTree,    0,	{ cIf         , 28 } }, /* 15 */
        {3, ProduceNewTree,    18,	{ cIf         , 29 } }, /* 16 */
        {3, ProduceNewTree,    35,	{ cIf         , 31 } }, /* 17 */
        {3, ProduceNewTree,    41,	{ cIf         , 37 } }, /* 18 */
        {3, ProduceNewTree,    47,	{ cIf         , 43 } }, /* 19 */
        {3, ProduceNewTree,    53,	{ cIf         , 49 } }, /* 20 */
        {3, ProduceNewTree,    61,	{ cIf         , 56 } }, /* 21 */
        {3, ProduceNewTree,    69,	{ cIf         , 64 } }, /* 22 */
        {3, ProduceNewTree,    76,	{ cIf         , 72 } }, /* 23 */
        {3, ProduceNewTree,    84,	{ cIf         , 79 } }, /* 24 */
        {3, ProduceNewTree,    88,	{ cIf         , 86 } }, /* 25 */
        {3, ProduceNewTree,    91,	{ cIf         , 89 } }, /* 26 */
        {1, ProduceNewTree,    92,	{ cLog        , 14 } }, /* 27 */
        {1, ProduceNewTree,    96,	{ cLog        , 94 } }, /* 28 */
        {1, ProduceNewTree,    103,	{ cLog        , 99 } }, /* 29 */
        {1, ProduceNewTree,    18,	{ cMax        , 18 } }, /* 30 */
        {1, ReplaceParams ,    106,	{ cMax        , 105 } }, /* 31 */
        {2, ReplaceParams ,    108,	{ cMax        , 107 } }, /* 32 */
        {2, ReplaceParams ,    0,	{ cMax        , 109 } }, /* 33 */
        {1, ProduceNewTree,    0,	{ cMin        , 0 } }, /* 34 */
        {1, ReplaceParams ,    106,	{ cMin        , 111 } }, /* 35 */
        {2, ReplaceParams ,    113,	{ cMin        , 112 } }, /* 36 */
        {2, ReplaceParams ,    0,	{ cMin        , 114 } }, /* 37 */
        {2, ProduceNewTree,    116,	{ cPow        , 115 } }, /* 38 */
        {2, ProduceNewTree,    118,	{ cPow        , 117 } }, /* 39 */
        {2, ProduceNewTree,    2,	{ cPow        , 119 } }, /* 40 */
        {2, ReplaceParams ,    123,	{ cPow        , 121 } }, /* 41 */
        {2, ProduceNewTree,    125,	{ cPow        , 124 } }, /* 42 */
        {2, ReplaceParams ,    129,	{ cPow        , 127 } }, /* 43 */
        {2, ReplaceParams ,    133,	{ cPow        , 131 } }, /* 44 */
        {2, ProduceNewTree,    136,	{ cPow        , 134 } }, /* 45 */
        {2, ProduceNewTree,    118,	{ cPow        , 137 } }, /* 46 */
        {2, ProduceNewTree,    2,	{ cPow        , 138 } }, /* 47 */
        {2, ProduceNewTree,    144,	{ cPow        , 140 } }, /* 48 */
        {2, ReplaceParams ,    149,	{ cPow        , 147 } }, /* 49 */
        {2, ReplaceParams ,    153,	{ cPow        , 151 } }, /* 50 */
        {1, ProduceNewTree,    154,	{ cSin        , 4 } }, /* 51 */
        {1, ProduceNewTree,    2,	{ cSin        , 155 } }, /* 52 */
        {1, ProduceNewTree,    158,	{ cSin        , 156 } }, /* 53 */
        {1, ProduceNewTree,    161,	{ cSin        , 160 } }, /* 54 */
        {1, ProduceNewTree,    162,	{ cSinh       , 14 } }, /* 55 */
        {1, ProduceNewTree,    163,	{ cTan        , 14 } }, /* 56 */
        {1, ProduceNewTree,    0,	{ cTan        , 164 } }, /* 57 */
        {1, ProduceNewTree,    168,	{ cTan        , 166 } }, /* 58 */
        {1, ProduceNewTree,    169,	{ cTanh       , 4 } }, /* 59 */
        {0, ProduceNewTree,    171,	{ cAdd        , 170 } }, /* 60 */
        {1, ProduceNewTree,    18,	{ cAdd        , 18 } }, /* 61 */
        {1, ReplaceParams ,    170,	{ cAdd        , 172 } }, /* 62 */
        {1, ReplaceParams ,    175,	{ cAdd        , 174 } }, /* 63 */
        {1, ReplaceParams ,    181,	{ cAdd        , 178 } }, /* 64 */
        {1, ReplaceParams ,    183,	{ cAdd        , 182 } }, /* 65 */
        {1, ReplaceParams ,    186,	{ cAdd        , 185 } }, /* 66 */
        {1, ReplaceParams ,    190,	{ cAdd        , 188 } }, /* 67 */
        {1, ReplaceParams ,    196,	{ cAdd        , 193 } }, /* 68 */
        {2, ReplaceParams ,    200,	{ cAdd        , 198 } }, /* 69 */
        {2, ReplaceParams ,    204,	{ cAdd        , 202 } }, /* 70 */
        {2, ReplaceParams ,    206,	{ cAdd        , 205 } }, /* 71 */
        {2, ReplaceParams ,    170,	{ cAdd        , 207 } }, /* 72 */
        {2, ReplaceParams ,    212,	{ cAdd        , 209 } }, /* 73 */
        {2, ReplaceParams ,    215,	{ cAdd        , 214 } }, /* 74 */
        {2, ReplaceParams ,    222,	{ cAdd        , 218 } }, /* 75 */
        {2, ReplaceParams ,    228,	{ cAdd        , 225 } }, /* 76 */
        {2, ReplaceParams ,    245,	{ cAdd        , 232 } }, /* 77 */
        {2, ReplaceParams ,    262,	{ cAdd        , 249 } }, /* 78 */
        {2, ReplaceParams ,    280,	{ cAdd        , 267 } }, /* 79 */
        {2, ReplaceParams ,    297,	{ cAdd        , 284 } }, /* 80 */
        {2, ReplaceParams ,    315,	{ cAdd        , 302 } }, /* 81 */
        {2, ReplaceParams ,    321,	{ cAdd        , 318 } }, /* 82 */
        {2, ReplaceParams ,    328,	{ cAdd        , 325 } }, /* 83 */
        {2, ReplaceParams ,    334,	{ cAdd        , 331 } }, /* 84 */
        {2, ReplaceParams ,    340,	{ cAdd        , 337 } }, /* 85 */
        {2, ReplaceParams ,    346,	{ cAdd        , 342 } }, /* 86 */
        {2, ReplaceParams ,    352,	{ cAdd        , 348 } }, /* 87 */
        {2, ReplaceParams ,    359,	{ cAdd        , 356 } }, /* 88 */
        {2, ReplaceParams ,    365,	{ cAdd        , 362 } }, /* 89 */
        {2, ReplaceParams ,    370,	{ cAdd        , 367 } }, /* 90 */
        {2, ReplaceParams ,    376,	{ cAdd        , 372 } }, /* 91 */
        {2, ReplaceParams ,    382,	{ cAdd        , 378 } }, /* 92 */
        {2, ReplaceParams ,    385,	{ cAdd        , 383 } }, /* 93 */
        {2, ReplaceParams ,    388,	{ cAdd        , 386 } }, /* 94 */
        {3, ReplaceParams ,    398,	{ cAdd        , 392 } }, /* 95 */
        {0, ProduceNewTree,    215,	{ cMul        , 170 } }, /* 96 */
        {1, ProduceNewTree,    2,	{ cMul        , 2 } }, /* 97 */
        {1, ProduceNewTree,    402,	{ cMul        , 401 } }, /* 98 */
        {1, ProduceNewTree,    406,	{ cMul        , 403 } }, /* 99 */
        {1, ProduceNewTree,    171,	{ cMul        , 172 } }, /* 100 */
        {1, ReplaceParams ,    170,	{ cMul        , 407 } }, /* 101 */
        {1, ReplaceParams ,    410,	{ cMul        , 409 } }, /* 102 */
        {1, ReplaceParams ,    414,	{ cMul        , 412 } }, /* 103 */
        {1, ReplaceParams ,    141,	{ cMul        , 416 } }, /* 104 */
        {2, ReplaceParams ,    418,	{ cMul        , 417 } }, /* 105 */
        {2, ReplaceParams ,    420,	{ cMul        , 419 } }, /* 106 */
        {2, ReplaceParams ,    170,	{ cMul        , 207 } }, /* 107 */
        {2, ReplaceParams ,    323,	{ cMul        , 423 } }, /* 108 */
        {2, ReplaceParams ,    208,	{ cMul        , 426 } }, /* 109 */
        {2, ReplaceParams ,    323,	{ cMul        , 429 } }, /* 110 */
        {2, ProduceNewTree,    436,	{ cMul        , 433 } }, /* 111 */
        {2, ProduceNewTree,    443,	{ cMul        , 440 } }, /* 112 */
        {2, ProduceNewTree,    451,	{ cMul        , 448 } }, /* 113 */
        {2, ReplaceParams ,    457,	{ cMul        , 454 } }, /* 114 */
        {2, ReplaceParams ,    463,	{ cMul        , 460 } }, /* 115 */
        {2, ReplaceParams ,    465,	{ cMul        , 464 } }, /* 116 */
        {2, ReplaceParams ,    467,	{ cMul        , 466 } }, /* 117 */
        {2, ReplaceParams ,    472,	{ cMul        , 469 } }, /* 118 */
        {2, ReplaceParams ,    477,	{ cMul        , 474 } }, /* 119 */
        {2, ReplaceParams ,    479,	{ cMul        , 478 } }, /* 120 */
        {2, ReplaceParams ,    483,	{ cMul        , 482 } }, /* 121 */
        {2, ReplaceParams ,    487,	{ cMul        , 486 } }, /* 122 */
        {2, ReplaceParams ,    493,	{ cMul        , 490 } }, /* 123 */
        {2, ReplaceParams ,    167,	{ cMul        , 494 } }, /* 124 */
        {2, ReplaceParams ,    496,	{ cMul        , 495 } }, /* 125 */
        {2, ReplaceParams ,    501,	{ cMul        , 498 } }, /* 126 */
        {2, ReplaceParams ,    506,	{ cMul        , 503 } }, /* 127 */
        {2, ReplaceParams ,    509,	{ cMul        , 507 } }, /* 128 */
        {2, ReplaceParams ,    512,	{ cMul        , 510 } }, /* 129 */
        {2, ProduceNewTree,    514,	{ cMod        , 513 } }, /* 130 */
        {2, ProduceNewTree,    516,	{ cEqual      , 515 } }, /* 131 */
        {2, ProduceNewTree,    116,	{ cNEqual     , 517 } }, /* 132 */
        {2, ProduceNewTree,    171,	{ cLess       , 518 } }, /* 133 */
        {2, ProduceNewTree,    215,	{ cLessOrEq   , 519 } }, /* 134 */
        {2, ProduceNewTree,    116,	{ cGreater    , 520 } }, /* 135 */
        {2, ProduceNewTree,    118,	{ cGreaterOrEq, 521 } }, /* 136 */
        {1, ProduceNewTree,    525,	{ cNot        , 523 } }, /* 137 */
        {1, ProduceNewTree,    529,	{ cNot        , 527 } }, /* 138 */
        {1, ProduceNewTree,    533,	{ cNot        , 531 } }, /* 139 */
        {1, ProduceNewTree,    537,	{ cNot        , 535 } }, /* 140 */
        {1, ProduceNewTree,    541,	{ cNot        , 539 } }, /* 141 */
        {1, ProduceNewTree,    545,	{ cNot        , 543 } }, /* 142 */
        {0, ProduceNewTree,    116,	{ cAnd        , 170 } }, /* 143 */
        {0, ReplaceParams ,    548,	{ cAnd        , 546 } }, /* 144 */
        {1, ProduceNewTree,    549,	{ cAnd        , 18 } }, /* 145 */
        {1, ProduceNewTree,    550,	{ cAnd        , 19 } }, /* 146 */
        {1, ReplaceParams ,    135,	{ cAnd        , 551 } }, /* 147 */
        {1, ReplaceParams ,    554,	{ cAnd        , 553 } }, /* 148 */
        {1, ReplaceParams ,    18,	{ cAnd        , 555 } }, /* 149 */
        {1, ReplaceParams ,    559,	{ cAnd        , 557 } }, /* 150 */
        {1, ReplaceParams ,    563,	{ cAnd        , 561 } }, /* 151 */
        {1, ReplaceParams ,    567,	{ cAnd        , 565 } }, /* 152 */
        {1, ReplaceParams ,    571,	{ cAnd        , 569 } }, /* 153 */
        {1, ReplaceParams ,    575,	{ cAnd        , 573 } }, /* 154 */
        {1, ReplaceParams ,    579,	{ cAnd        , 577 } }, /* 155 */
        {1, ReplaceParams ,    2,	{ cAnd        , 580 } }, /* 156 */
        {1, ReplaceParams ,    141,	{ cAnd        , 582 } }, /* 157 */
        {1, ReplaceParams ,    19,	{ cAnd        , 583 } }, /* 158 */
        {2, ReplaceParams ,    2,	{ cAnd        , 584 } }, /* 159 */
        {2, ProduceNewTree,    171,	{ cAnd        , 207 } }, /* 160 */
        {2, ProduceNewTree,    171,	{ cAnd        , 587 } }, /* 161 */
        {2, ReplaceParams ,    19,	{ cAnd        , 588 } }, /* 162 */
        {3, ReplaceParams ,    595,	{ cAnd        , 592 } }, /* 163 */
        {0, ProduceNewTree,    118,	{ cOr         , 170 } }, /* 164 */
        {0, ReplaceParams ,    598,	{ cOr         , 596 } }, /* 165 */
        {1, ProduceNewTree,    549,	{ cOr         , 18 } }, /* 166 */
        {1, ProduceNewTree,    550,	{ cOr         , 19 } }, /* 167 */
        {1, ReplaceParams ,    19,	{ cOr         , 599 } }, /* 168 */
        {1, ReplaceParams ,    602,	{ cOr         , 601 } }, /* 169 */
        {1, ReplaceParams ,    18,	{ cOr         , 555 } }, /* 170 */
        {1, ReplaceParams ,    606,	{ cOr         , 604 } }, /* 171 */
        {1, ReplaceParams ,    610,	{ cOr         , 608 } }, /* 172 */
        {1, ReplaceParams ,    614,	{ cOr         , 612 } }, /* 173 */
        {1, ReplaceParams ,    618,	{ cOr         , 616 } }, /* 174 */
        {1, ReplaceParams ,    622,	{ cOr         , 620 } }, /* 175 */
        {1, ReplaceParams ,    626,	{ cOr         , 624 } }, /* 176 */
        {1, ReplaceParams ,    18,	{ cOr         , 627 } }, /* 177 */
        {1, ReplaceParams ,    630,	{ cOr         , 629 } }, /* 178 */
        {1, ReplaceParams ,    19,	{ cOr         , 583 } }, /* 179 */
        {2, ReplaceParams ,    18,	{ cOr         , 631 } }, /* 180 */
        {2, ProduceNewTree,    118,	{ cOr         , 207 } }, /* 181 */
        {2, ProduceNewTree,    118,	{ cOr         , 634 } }, /* 182 */
        {2, ReplaceParams ,    19,	{ cOr         , 635 } }, /* 183 */
        {1, ProduceNewTree,    639,	{ cNotNot     , 637 } }, /* 184 */
        {1, ProduceNewTree,    643,	{ cNotNot     , 641 } }, /* 185 */
        {1, ProduceNewTree,    647,	{ cNotNot     , 645 } }, /* 186 */
        {1, ProduceNewTree,    651,	{ cNotNot     , 649 } }, /* 187 */
        {1, ProduceNewTree,    655,	{ cNotNot     , 653 } }, /* 188 */
        {1, ProduceNewTree,    659,	{ cNotNot     , 657 } }, /* 189 */
        {1, ProduceNewTree,    660,	{ cNotNot     , 550 } }, /* 190 */
        {1, ProduceNewTree,    664,	{ cNotNot     , 662 } }, /* 191 */
        {1, ProduceNewTree,    668,	{ cNotNot     , 666 } }, /* 192 */
        {1, ReplaceParams ,    18,	{ cNotNot     , 549 } }, /* 193 */
        {2, ProduceNewTree,    670,	{ cPow        , 669 } }, /* 194 */
        {2, ProduceNewTree,    672,	{ cPow        , 671 } }, /* 195 */
        {2, ProduceNewTree,    674,	{ cPow        , 673 } }, /* 196 */
        {1, ProduceNewTree,    678,	{ cMul        , 675 } }, /* 197 */
        {1, ProduceNewTree,    682,	{ cMul        , 679 } }, /* 198 */
        {1, ReplaceParams ,    684,	{ cMul        , 683 } }, /* 199 */
        {1, ReplaceParams ,    686,	{ cMul        , 685 } }, /* 200 */
        {1, ReplaceParams ,    688,	{ cMul        , 687 } }, /* 201 */
        {2, ReplaceParams ,    690,	{ cMul        , 689 } }, /* 202 */
        {2, ReplaceParams ,    692,	{ cMul        , 691 } }, /* 203 */
        {2, ReplaceParams ,    694,	{ cMul        , 693 } }, /* 204 */
        {2, ReplaceParams ,    696,	{ cMul        , 695 } }, /* 205 */
        {1, ProduceNewTree,    697,	{ cNotNot     , 18 } }, /* 206 */
    };
}

namespace FPoptimizer_Grammar
{
    const GrammarPack pack =
    {
        clist, plist, mlist, flist, rlist,
        {
            {0, 1 }, /* 0 */
            {1, 193 }, /* 1 */
            {194, 13 }, /* 2 */
        }
    };
}

//#include <stdio.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <assert.h>

#include "fpconfig.h"
#include "fparser.h"
#include "fptypes.h"

#ifdef FP_SUPPORT_OPTIMIZER

using namespace FUNCTIONPARSERTYPES;

//#define DEBUG_SUBSTITUTIONS

namespace
{
    /* Little birds tell me that std::equal_range() is practically worthless
     * due to the insane limitation that the two parameters for Comp() must
     * be of the same type. Hence we must reinvent the wheel and implement
     * our own here. This is practically identical to the one from
     * GNU libstdc++, except rewritten. -Bisqwit
     */
    template<typename It, typename T, typename Comp>
    std::pair<It, It>
    MyEqualRange(It first, It last, const T& val, Comp comp)
    {
        size_t len = last-first;
        while(len > 0)
        {
            size_t half = len/2;
            It middle(first); middle += half;
            if(comp(*middle, val))
            {
                first = middle;
                ++first;
                len = len - half - 1;
            }
            else if(comp(val, *middle))
            {
                len = half;
            }
            else
            {
                // The following implements this:
                // // left = lower_bound(first, middle, val, comp);
                It left(first);
              {///
                It& first2 = left;
                It last2(middle);
                size_t len2 = last2-first2;
                while(len2 > 0)
                {
                    size_t half2 = len2 / 2;
                    It middle2(first2); middle2 += half2;
                    if(comp(*middle2, val))
                    {
                        first2 = middle2;
                        ++first2;
                        len2 = len2 - half2 - 1;
                    }
                    else
                        len2 = half2;
                }
                // left = first2;  - not needed, already happens due to reference
              }///
                first += len;
                // The following implements this:
                // // right = upper_bound(++middle, first, val, comp);
                It right(++middle);
              {///
                It& first2 = right;
                It& last2 = first;
                size_t len2 = last2-first2;
                while(len2 > 0)
                {
                    size_t half2 = len2 / 2;
                    It middle2(first2); middle2 += half2;
                    if(comp(val, *middle2))
                        len2 = half2;
                    else
                    {
                        first2 = middle2;
                        ++first2;
                        len2 = len2 - half2 - 1;
                    }
                }
                // right = first2;  - not needed, already happens due to reference
              }///
                return std::pair<It,It> (left,right);
            }
        }
        return std::pair<It,It> (first,first);
    }
}

namespace FPoptimizer_CodeTree
{
    void CodeTree::ConstantFolding()
    {
        // Insert here any hardcoded constant-folding optimizations
        // that you want to be done at bytecode->codetree conversion time.
    }
}

namespace FPoptimizer_Grammar
{
    static double GetPackConst(size_t index)
    {
        double res = pack.clist[index];
    #if 0
        if(res == FPOPT_NAN_CONST)
        {
        #ifdef NAN
            return NAN;
        #else
            return 0.0; // Should be 0.0/0.0, but some compilers don't like that
        #endif
        }
    #endif
        return res;
    }

    /* A helper for std::equal_range */
    struct OpcodeRuleCompare
    {
        bool operator() (const FPoptimizer_CodeTree::CodeTree& tree, const Rule& rule) const
        {
            /* If this function returns true, len=half.
             */

            if(tree.Opcode != rule.func.opcode)
                return tree.Opcode < rule.func.opcode;

            if(tree.Params.size() < rule.n_minimum_params)
            {
                // Tree has fewer params than required?
                return true; // Failure
            }
            return false;
        }
        bool operator() (const Rule& rule, const FPoptimizer_CodeTree::CodeTree& tree) const
        {
            /* If this function returns true, rule will be excluded from the equal_range
             */

            if(rule.func.opcode != tree.Opcode)
                return rule.func.opcode < tree.Opcode;

            if(rule.n_minimum_params < tree.Params.size())
            {
                // Tree has more params than the pattern has?
                switch(pack.mlist[rule.func.index].type)
                {
                    case PositionalParams:
                    case SelectedParams:
                        return true; // Failure
                    case AnyParams:
                        return false; // Not a failure
                }
            }
            return false;
        }
    };

#ifdef DEBUG_SUBSTITUTIONS
    void DumpTree(const FPoptimizer_CodeTree::CodeTree& tree);
    static const char ImmedHolderNames[2][2] = {"%","&"};
    static const char NamedHolderNames[6][2] = {"x","y","z","a","b","c"};
#endif

    /* Apply the grammar to a given CodeTree */
    bool Grammar::ApplyTo(
        FPoptimizer_CodeTree::CodeTree& tree,
        bool recursion) const
    {
        bool changed = false;

#ifdef DEBUG_SUBSTITUTIONS
        if(!recursion)
        {
            std::cout << "Input:  ";
            DumpTree(tree);
            std::cout << "\n" << std::flush;
        }
#else
        recursion=recursion;
#endif
        if(tree.OptimizedUsing != this)
        {
            /* First optimize all children */
            for(size_t a=0; a<tree.Params.size(); ++a)
            {
                if( ApplyTo( *tree.Params[a].param, true ) )
                {
                    changed = true;
                }
            }

            /* Figure out which rules _may_ match this tree */
            typedef const Rule* ruleit;

            std::pair<ruleit, ruleit> range
                = MyEqualRange(pack.rlist + index,
                               pack.rlist + index + count,
                               tree,
                               OpcodeRuleCompare());

            while(range.first < range.second)
            {
                /* Check if this rule matches */
                if(range.first->ApplyTo(tree))
                {
                    changed = true;
                    break;
                }
                ++range.first;
            }

            if(!changed)
            {
                tree.OptimizedUsing = this;
            }
        }

#ifdef DEBUG_SUBSTITUTIONS
        if(!recursion)
        {
            std::cout << "Output: ";
            DumpTree(tree);
            std::cout << "\n" << std::flush;
        }
#endif
        return changed;
    }

    /* Store information about a potential match,
     * in order to iterate through candidates
     */
    struct MatchedParams::CodeTreeMatch
    {
        // Which parameters were matched -- these will be replaced if AnyParams are used
        std::vector<size_t> param_numbers;

        // Which values were saved for ImmedHolders?
        std::map<unsigned, double> ImmedMap;
        // Which codetrees were saved for each NameHolder? And how many?
        std::map<unsigned, std::pair<fphash_t, size_t> > NamedMap;
        // Which codetrees were saved for each RestHolder?
        std::map<unsigned,
          std::vector<fphash_t> > RestMap;

        // Examples of each codetree
        std::map<fphash_t, FPoptimizer_CodeTree::CodeTreeP> trees;

        CodeTreeMatch() : param_numbers(), ImmedMap(), NamedMap(), RestMap() { }
    };

#ifdef DEBUG_SUBSTITUTIONS
    void DumpMatch(const Function& input,
                   const FPoptimizer_CodeTree::CodeTree& tree,
                   const MatchedParams& replacement,
                   const MatchedParams::CodeTreeMatch& matchrec,
                   bool DidMatch=true);
    void DumpFunction(const Function& input);
    void DumpParam(const ParamSpec& p);
    void DumpParams(const MatchedParams& mitem);
#endif

    /* Apply the rule to a given CodeTree */
    bool Rule::ApplyTo(
        FPoptimizer_CodeTree::CodeTree& tree) const
    {
        const Function&      input  = func;
        const MatchedParams& repl   = pack.mlist[repl_index];

        if(input.opcode == tree.Opcode)
        {
            for(unsigned long match_index=0; ; ++match_index)
            {
                MatchedParams::CodeTreeMatch matchrec;
                MatchResultType mr =
                    pack.mlist[input.index].Match(tree, matchrec,match_index, false);
                if(!mr.found && mr.has_more) continue;
                if(!mr.found) break;

    #ifdef DEBUG_SUBSTITUTIONS
                DumpMatch(input, tree, repl, matchrec);
    #endif

                const MatchedParams& params = pack.mlist[input.index];
                switch(type)
                {
                    case ReplaceParams:
                        repl.ReplaceParams(tree, params, matchrec);
    #ifdef DEBUG_SUBSTITUTIONS
                        std::cout << "  ParmReplace: ";
                        DumpTree(tree);
                        std::cout << "\n" << std::flush;
    #endif
                        return true;
                    case ProduceNewTree:
                        repl.ReplaceTree(tree,   params, matchrec);
    #ifdef DEBUG_SUBSTITUTIONS
                        std::cout << "  TreeReplace: ";
                        DumpTree(tree);
                        std::cout << "\n" << std::flush;
    #endif
                        return true;
                }
                break; // should be unreachable
            }
        }
        #ifdef DEBUG_SUBSTITUTIONS
        // Report mismatch
        MatchedParams::CodeTreeMatch matchrec;
        DumpMatch(input, tree, repl, matchrec, false);
        #endif
        return false;
    }


    /* Match the given function to the given CodeTree.
     */
    MatchResultType Function::Match(
        FPoptimizer_CodeTree::CodeTree& tree,
        MatchedParams::CodeTreeMatch& match,
        unsigned long match_index) const
    {
        if(opcode != tree.Opcode) return NoMatch;
        return pack.mlist[index].Match(tree, match, match_index, true);
    }


    /* This struct is used by MatchedParams::Match() for backtracking. */
    struct ParamMatchSnapshot
    {
        MatchedParams::CodeTreeMatch snapshot;
                                    // Snapshot of the state so far
        size_t            parampos; // Which position was last chosen?
        std::vector<bool> used;     // Which params were allocated?

        size_t            matchpos;
    };

    /* Match the given list of ParamSpecs using the given ParamMatchingType
     * to the given CodeTree.
     * The CodeTree is already assumed to be a function type
     * -- i.e. it is assumed that the caller has tested the Opcode of the tree.
     */
    MatchResultType MatchedParams::Match(
        FPoptimizer_CodeTree::CodeTree& tree,
        MatchedParams::CodeTreeMatch& match,
        unsigned long match_index,
        bool recursion) const
    {
        /*        match_index is a feature for backtracking.
         *
         *        For example,
         *          cMul (cAdd x) (cAdd x)
         *        Applied to:
         *          (a+b)*(c+b)
         *
         *        Match (cAdd x) to (a+b) may first capture "a" into "x",
         *        and then Match(cAdd x) for (c+b) will fail,
         *        because there's no "a" there.
         *
         *        However, match_index can be used to indicate that the
         *        _second_ matching will be used, so that "b" will be
         *        captured into "x".
         */


        /* First, check if the tree has any chances of matching... */
        /* Figure out what we need. */
        struct Needs
        {
            struct Needs_Pol
            {
                int SubTrees;
                int Others;
                unsigned SubTreesDetail[VarBegin];

                Needs_Pol(): SubTrees(0), Others(0), SubTreesDetail()
                {
                }
            } polarity[2]; // 0=positive, 1=negative
            int Immeds;

            Needs(): polarity(), Immeds() { }
        } NeedList;

        // Figure out what we need
        size_t minimum_need = 0;
        for(unsigned a=0; a<count; ++a)
        {
            const ParamSpec& param = pack.plist[index+a];
            Needs::Needs_Pol& needs = NeedList.polarity[param.sign];
            switch(param.opcode)
            {
                case SubFunction:
                    needs.SubTrees += 1;
                    assert( pack.flist[param.index].opcode < VarBegin );
                    needs.SubTreesDetail[ pack.flist[param.index].opcode ] += 1;
                    ++minimum_need;
                    break;
                case NumConstant:
                case ImmedHolder:
                default: // GroupFunction:
                    NeedList.Immeds += 1;
                    ++minimum_need;
                    break;
                case NamedHolder:
                    needs.Others += param.minrepeat;
                    ++minimum_need;
                    break;
                case RestHolder:
                    break;
            }
        }
        if(tree.Params.size() < minimum_need)
        {
            // Impossible to satisfy
            return NoMatch;
        }

        // Figure out what we have (note: we already assume that the opcode of the tree matches!)
        for(size_t a=0; a<tree.Params.size(); ++a)
        {
            Needs::Needs_Pol& needs = NeedList.polarity[tree.Params[a].sign];
            unsigned opcode = tree.Params[a].param->Opcode;
            switch(opcode)
            {
                case cImmed:
                    if(NeedList.Immeds > 0) NeedList.Immeds -= 1;
                    else needs.Others -= 1;
                    break;
                case cVar:
                case cFCall:
                case cPCall:
                    needs.Others -= 1;
                    break;
                default:
                    assert( opcode < VarBegin );
                    if(needs.SubTrees > 0
                    && needs.SubTreesDetail[opcode] > 0)
                    {
                        needs.SubTrees -= 1;
                        needs.SubTreesDetail[opcode] -= 1;
                    }
                    else needs.Others -= 1;
            }
        }

        // Check whether all needs were satisfied
        if(NeedList.Immeds > 0
        || NeedList.polarity[0].SubTrees > 0
        || NeedList.polarity[0].Others > 0
        || NeedList.polarity[1].SubTrees > 0
        || NeedList.polarity[1].Others > 0)
        {
            // Something came short, impossible to satisfy.
            return NoMatch;
        }

        if(type != AnyParams)
        {
            if(NeedList.Immeds < 0
            || NeedList.polarity[0].SubTrees < 0
            || NeedList.polarity[0].Others < 0
            || NeedList.polarity[1].SubTrees < 0
            || NeedList.polarity[1].Others < 0
            || count != tree.Params.size())
            {
                // Something was too much.
                return NoMatch;
            }
        }

        TransformationType transf = None;
        switch(tree.Opcode)
        {
            case cAdd: transf = Negate; break;
            case cMul: transf = Invert; break;
            case cAnd:
            case cOr:  transf = NotThe; break;
        }

        switch(type)
        {
            case PositionalParams:
            {
                /*DumpTree(tree);
                std::cout << "<->";
                DumpParams(*this);
                std::cout << " -- ";*/

                std::vector<MatchPositionSpec<CodeTreeMatch> > specs;
                specs.reserve(count);
                //fprintf(stderr, "Enter loop %lu\n", match_index);
                for(unsigned a=0; a<count; ++a)
                {
                    specs.resize(a+1);

                PositionalParamsMatchingLoop:;
                    // Match this parameter.
                    MatchResultType mr = pack.plist[index+a].Match(
                        *tree.Params[a].param, match,
                        tree.Params[a].sign ? transf : None,
                        specs[a].roundno);

                    specs[a].done = !mr.has_more;

                    // If it was not found, backtrack...
                    if(!mr.found)
                    {
                    LoopThisRound:
                        while(specs[a].done)
                        {
                            // Backtrack
                            if(a <= 0) return NoMatch; //
                            specs.resize(a);
                            --a;
                            match = specs[a].data;
                        }
                        ++specs[a].roundno;
                        goto PositionalParamsMatchingLoop;
                    }
                    // If found...
                    if(!recursion)
                        match.param_numbers.push_back(a);
                    specs[a].data = match;

                    if(a == count-1U && match_index > 0)
                    {
                        // Skip this match
                        --match_index;
                        goto LoopThisRound;
                    }
                }
                /*std::cout << " yay?\n";*/
                // Match = no mismatch.
                bool final_try = true;
                for(unsigned a=0; a<count; ++a)
                    if(!specs[a].done) { final_try = false; break; }
                //fprintf(stderr, "Exit  loop %lu\n", match_index);
                return MatchResultType(true, !final_try);
            }
            case AnyParams:
            case SelectedParams:
            {
                const size_t n_tree_params = tree.Params.size();

                unsigned N_PositiveRestHolders = 0;
                unsigned N_NegativeRestHolders = 0;
                for(unsigned a=0; a<count; ++a)
                {
                    const ParamSpec& param = pack.plist[index+a];
                    if(param.opcode == RestHolder)
                    {
                        if(param.sign)
                            ++N_NegativeRestHolders;
                        else
                            ++N_PositiveRestHolders;
                    }
                }

                bool HasRestHolders = N_PositiveRestHolders || N_NegativeRestHolders;

                #ifdef DEBUG_SUBSTITUTIONS
                if((type == AnyParams) && recursion && !HasRestHolders)
                {
                    std::cout << "Recursed AnyParams with no RestHolders?\n";
                    DumpParams(*this);
                }
                #endif

                if(!HasRestHolders && recursion && count != n_tree_params)
                {
                    /*DumpTree(tree);
                    std::cout << "<->";
                    DumpParams(*this);
                    std::cout << " -- fail due to recursion&&count!=n_tree_params";*/
                    return NoMatch; // Impossible match.
                }

                /*std::cout << "Matching ";
                DumpTree(tree); std::cout << " with ";
                DumpParams(*this);
                std::cout << " , match_index=" << match_index << "\n" << std::flush;*/

                std::vector<ParamMatchSnapshot> position(count);
                std::vector<bool>               used(n_tree_params);

                unsigned p=0;

                for(; p<count; ++p)
                {
                    position[p].snapshot  = match;
                    position[p].parampos  = 0;
                    position[p].matchpos  = 0;
                    position[p].used      = used;

                    //fprintf(stderr, "posA: p=%u count=%u\n", p, count);

                backtrack:
                  {
                    if(pack.plist[index+p].opcode == RestHolder)
                    {
                        // RestHolders always match. They're filled afterwards.
                        position[p].parampos = n_tree_params;
                        position[p].matchpos = 0;
                        continue;
                    }

                    size_t whichparam = position[p].parampos;
                    size_t whichmatch = position[p].matchpos;

                    /* a          = param index in the syntax specification
                     * whichparam = param index in the tree received from parser
                     */

                    /*fprintf(stderr, "posB: p=%u, whichparam=%lu, whichmatch=%lu\n",
                        p,whichparam,whichmatch);*/
                    while(whichparam < n_tree_params)
                    {
                        if(used[whichparam])
                        {
                        NextParamNumber:
                            ++whichparam;
                            whichmatch = 0;
                            continue;
                        NextMatchNumber:
                            ++whichmatch;
                        }

                        /*std::cout << "Maybe [" << p << "]:";
                        DumpParam(pack.plist[index+p]);
                        std::cout << " <-> ";
                        if(tree.Params[whichparam].sign) std::cout << '~';
                        DumpTree(*tree.Params[whichparam].param);
                        std::cout << "...?\n" << std::flush;*/

                        MatchResultType mr = pack.plist[index+p].Match(
                            *tree.Params[whichparam].param, match,
                            tree.Params[whichparam].sign ? transf : None,
                            whichmatch);

                        /*std::cout << "In ";
                        DumpTree(tree); std::cout << std::flush;
                        fprintf(stderr, ", trying param %lu, match %lu (matchindex %lu); got %s,%s: ",
                            whichparam,whichmatch, match_index,
                            mr.found?"found":"not found",
                            mr.has_more?"more":"no more"); fflush(stderr);
                        DumpParam(pack.plist[index+p]); std::cout << "\n" << std::flush;*/

                        if(!mr.found)
                        {
                        NextParamTest:
                            if(!mr.has_more) goto NextParamNumber;
                            goto NextMatchNumber;
                        }

                        /*std::cout << "woo... " << a << ", " << b << "\n";*/
                        /* NamedHolders require a special treatment,
                         * because a repetition count may be issued
                         * for them.
                         */
                        if(pack.plist[index+p].opcode == NamedHolder)
                        {
                            // Verify the MinRepeat & AnyRepeat case
                            unsigned MinRepeat = pack.plist[index+p].minrepeat;
                            bool AnyRepeat     = pack.plist[index+p].anyrepeat;
                            unsigned HadRepeat = 1;

                            for(size_t repeat_pos = whichparam+1;
                                repeat_pos < n_tree_params && (HadRepeat < MinRepeat || AnyRepeat);
                                ++repeat_pos)
                            {
                                /*fprintf(stderr, "Req @ %lu = %d:%16lX, got @ %lu = %d:%16lX\n",
                                    whichparam, tree.Params[whichparam].sign,
                                                tree.Params[whichparam].param->Hash,
                                    repeat_pos, tree.Params[repeat_pos].sign,
                                                tree.Params[repeat_pos].param->Hash);*/

                                if(tree.Params[repeat_pos].param->Hash
                                == tree.Params[whichparam].param->Hash
                                && tree.Params[repeat_pos].sign
                                == tree.Params[whichparam].sign
                                && !used[repeat_pos])
                                {
                                    ++HadRepeat;
                                }
                            }
                            /*fprintf(stderr, "Got repeat %u, needs %u\n", HadRepeat,MinRepeat);*/
                            if(HadRepeat < MinRepeat)
                            {
                                match = position[p].snapshot;
                                used  = position[p].used;
                                goto NextParamTest; // No sufficient repeat count here
                            }

                            used[whichparam] = true;
                            if(!recursion) match.param_numbers.push_back(whichparam);

                            HadRepeat = 1;
                            for(size_t repeat_pos = whichparam+1;
                                repeat_pos < n_tree_params && (HadRepeat < MinRepeat || AnyRepeat);
                                ++repeat_pos)
                            {
                                if(tree.Params[repeat_pos].param->Hash
                                == tree.Params[whichparam].param->Hash
                                && tree.Params[repeat_pos].sign
                                == tree.Params[whichparam].sign
                                && !used[repeat_pos])
                                {
                                    ++HadRepeat;
                                    used[repeat_pos] = true;
                                    if(!recursion) match.param_numbers.push_back(repeat_pos);
                                }
                            }
                            if(AnyRepeat)
                                match.NamedMap[pack.plist[index+p].index].second = HadRepeat;
                        }
                        else
                        {
                            used[whichparam] = true;
                            if(!recursion) match.param_numbers.push_back(whichparam);
                        }
                        position[p].parampos = mr.has_more ? whichparam : (whichparam+1);
                        position[p].matchpos = mr.has_more ? (whichmatch+1) : 0;
                        goto ok;
                    }

                    /*DumpParam(param);
                    std::cout << " didn't match anything in ";
                    DumpTree(tree);
                    std::cout << "\n";*/
                  }

                    // No match for this param, try backtracking.
                DiscardedThisAttempt:
                    while(p > 0)
                    {
                        --p;
                        ParamMatchSnapshot& prevpos = position[p];
                        if(prevpos.parampos < n_tree_params)
                        {
                            // Try another combination.
                            match = prevpos.snapshot;
                            used  = prevpos.used;
                            goto backtrack;
                        }
                    }
                    // If we cannot backtrack, break. No possible match.
                    /*if(!recursion)
                        std::cout << "Drats!\n";*/
                    if(match_index == 0)
                        return NoMatch;
                    break;
                ok:;
                    /*if(!recursion)
                        std::cout << "Match for param " << a << " at " << b << std::endl;*/

                    if(p == count-1U && match_index > 0)
                    {
                        // Skip this match
                        --match_index;
                        goto DiscardedThisAttempt;
                    }
                }
                /*fprintf(stderr, "End loop, match_index=%lu\n", match_index); fflush(stderr);*/

                /* We got a match. */

                // If the rule cares about the balance of
                // negative restholdings versus positive restholdings,
                // verify them.
                if(balance != BalanceDontCare)
                {
                    unsigned n_pos_restholdings = 0;
                    unsigned n_neg_restholdings = 0;

                    for(unsigned a=0; a<count; ++a)
                    {
                        const ParamSpec& param = pack.plist[index+a];
                        if(param.opcode == RestHolder)
                        {
                            for(size_t b=0; b<n_tree_params; ++b)
                                if(tree.Params[b].sign == param.sign && !used[b])
                                {
                                    if(param.sign)
                                        n_neg_restholdings += 1;
                                    else
                                        n_pos_restholdings += 1;
                                }
                        }
                    }
                    switch(balance)
                    {
                        case BalanceMoreNeg:
                            if(n_neg_restholdings <= n_pos_restholdings) return NoMatch;
                            break;
                        case BalanceMorePos:
                            if(n_pos_restholdings <= n_neg_restholdings) return NoMatch;
                            break;
                        case BalanceEqual:
                            if(n_pos_restholdings != n_neg_restholdings) return NoMatch;
                            break;
                        case BalanceDontCare: ;
                    }
                }

                unsigned pos_rest_remain = N_PositiveRestHolders;
                unsigned neg_rest_remain = N_NegativeRestHolders;

                // Verify if we have RestHolder constraints.
                for(unsigned a=0; a<count; ++a)
                {
                    const ParamSpec& param = pack.plist[index+a];
                    if(param.opcode == RestHolder)
                    {
                        std::map<unsigned, std::vector<fphash_t> >::iterator
                            i = match.RestMap.lower_bound(param.index);

                        if(i != match.RestMap.end() && i->first == param.index)
                        {
                            unsigned& n_remaining_restholders_of_this_kind =
                                param.sign ? neg_rest_remain : pos_rest_remain;
                            /*fprintf(stderr, "Does restholder %u match in", param.index);
                            fflush(stderr); DumpTree(tree); std::cout << "? " << std::flush;*/

                            const std::vector<fphash_t>& RefRestList = i->second;
                            for(size_t r=0; r<RefRestList.size(); ++r)
                            {
                                for(size_t b=0; b<n_tree_params; ++b)
                                    if(tree.Params[b].sign == param.sign
                                    && !used[b]
                                    && tree.Params[b].param->Hash == RefRestList[r])
                                    {
                                        used[b] = true;
                                        goto SatisfiedRestHolder;
                                    }
                                // Unsatisfied RestHolder constraint
                                /*fprintf(stderr, "- no\n");*/
                                p=count-1;
                                goto DiscardedThisAttempt;
                            SatisfiedRestHolder:;
                            }
                            --n_remaining_restholders_of_this_kind;
                            /*fprintf(stderr, "- yes\n");*/
                        }
                    }
                }

                // Now feed any possible RestHolders the remaining parameters.
                bool more_restholder_options = false;
                for(unsigned a=0; a<count; ++a)
                {
                    const ParamSpec& param = pack.plist[index+a];
                    if(param.opcode == RestHolder)
                    {
                        std::map<unsigned, std::vector<fphash_t> >::iterator
                            i = match.RestMap.lower_bound(param.index);
                        if(i != match.RestMap.end() && i->first == param.index) continue;

                        std::vector<fphash_t>& RestList = match.RestMap[param.index]; // mark it up

                        unsigned& n_remaining_restholders_of_this_kind =
                            param.sign ? neg_rest_remain : pos_rest_remain;

                        unsigned n_remaining_params = 0;
                        for(size_t b=0; b<n_tree_params; ++b)
                            if(tree.Params[b].sign == param.sign && !used[b])
                                ++n_remaining_params;

                        /*fprintf(stderr, "[index %lu] For restholder %u, %u remains, %u remaining of kind\n",
                            match_index,
                            (unsigned)param.index, (unsigned)n_remaining_params,
                            (unsigned)n_remaining_restholders_of_this_kind);
                            fflush(stderr);*/

                        if(n_remaining_params > 0)
                        {
                            if(n_remaining_params > 8) n_remaining_params = 8;
                            unsigned n_remaining_combinations = 1 << n_remaining_params;

                            unsigned n_options = n_remaining_restholders_of_this_kind > 1
                                ? n_remaining_combinations
                                : 1;
                            size_t selection = n_remaining_combinations - 1;
                            if(n_options > 1)
                            {
                                --n_options;
                                selection = match_index % (n_options); ++selection;
                                match_index /= n_options;
                            }
                            if(selection+1 < n_options) more_restholder_options = true;

                            /*fprintf(stderr, "- selected %u/%u\n", selection, n_options); fflush(stderr);*/

                            unsigned matchbit = 1;
                            for(size_t b=0; b<n_tree_params; ++b)
                                if(tree.Params[b].sign == param.sign && !used[b])
                                {
                                    if(selection & matchbit)
                                    {
                                        /*fprintf(stderr, "- uses param %lu\n", b);*/
                                        if(!recursion)
                                            match.param_numbers.push_back(b);
                                        fphash_t hash = tree.Params[b].param->Hash;
                                        RestList.push_back(hash);
                                        match.trees.insert(
                                            std::make_pair(hash, tree.Params[b].param) );

                                        used[b] = true;
                                    }
                                    if(matchbit < 0x80U) matchbit <<= 1;
                                }
                        }
                        --n_remaining_restholders_of_this_kind;
                    }
                }
                /*std::cout << "Returning match for ";
                DumpTree(tree);
                std::cout << "\n               with ";
                DumpParams(*this); std::cout << std::flush;
                fprintf(stderr, ", %s hope for more (now %lu)\n",
                    more_restholder_options ? "with" : "without", match_index); fflush(stderr);*/
                return more_restholder_options ? FoundSomeMatch : FoundLastMatch;
            }
        }
        return NoMatch;
    }

    MatchResultType ParamSpec::Match(
        FPoptimizer_CodeTree::CodeTree& tree,
        MatchedParams::CodeTreeMatch& match,
        TransformationType transf,
        unsigned long match_index) const
    {
        assert(opcode != RestHolder); // RestHolders are supposed to be handler by the caller

        switch(OpcodeType(opcode))
        {
            case NumConstant:
            {
                if(!tree.IsImmed()) return NoMatch;
                double res = tree.GetImmed();
                if(transformation == Negate) res = -res;
                if(transformation == Invert) res = 1/res;
                double res2 = GetPackConst(index);
                if(transf == Negate) res2 = -res2;
                if(transf == Invert) res2 = 1/res2;
                if(transf == NotThe) res2 = res2 != 0;
                if(res != res2) return NoMatch;
                return FoundLastMatch; // Previously unknown NumConstant, good
            }
            case ImmedHolder:
            {
                if(!tree.IsImmed()) return NoMatch;
                double res = tree.GetImmed();
                if(transformation == Negate) res = -res;
                if(transformation == Invert) res = 1/res;
                std::map<unsigned, double>::iterator
                    i = match.ImmedMap.lower_bound(index);
                if(i != match.ImmedMap.end() && i->first == index)
                {
                    double res2 = i->second;
                    if(transf == Negate) res2 = -res2;
                    if(transf == Invert) res2 = 1/res2;
                    if(transf == NotThe) res2 = res2 != 0;
                    return res == res2 ? FoundLastMatch : NoMatch;
                }
                if(sign != (transf != None)) return NoMatch;

                match.ImmedMap.insert(i, std::make_pair((unsigned)index, res));
                return FoundLastMatch; // Previously unknown ImmedHolder, good
            }
            case NamedHolder:
            {
                if(sign != (transf != None)) return NoMatch;
                std::map<unsigned, std::pair<fphash_t, size_t> >::iterator
                    i = match.NamedMap.lower_bound(index);
                if(i != match.NamedMap.end() && i->first == index)
                {
                    /*fprintf(stderr, "NamedHolder found: %16lX -- tested against %16lX\n", i->second.first, tree.Hash);*/
                    return tree.Hash == i->second.first
                           ? FoundLastMatch
                           : NoMatch;
                }
                match.NamedMap.insert(i, std::make_pair(index, std::make_pair(tree.Hash, 1)));
                match.trees.insert(std::make_pair(tree.Hash, &tree));
                return FoundLastMatch; // Previously unknown NamedHolder, good
            }
            case RestHolder:
            {
                break;
            }
            case SubFunction:
            {
                if(sign != (transf != None)) return NoMatch;
                return pack.flist[index].Match(tree, match, match_index);
            }
            default:
            {
                if(!tree.IsImmed()) return NoMatch;
                double res = tree.GetImmed();
                if(transformation == Negate) res = -res;
                if(transformation == Invert) res = 1/res;
                double res2;
                if(!GetConst(match, res2)) return NoMatch;
                if(transf == Negate) res2 = -res2;
                if(transf == Invert) res2 = 1/res2;
                if(transf == NotThe) res2 = res2 != 0;
                return res == res2 ? FoundLastMatch : NoMatch;
            }
        }
        return NoMatch;
    }

    bool ParamSpec::GetConst(
        const MatchedParams::CodeTreeMatch& match,
        double& result) const
    {
        switch(OpcodeType(opcode))
        {
            case NumConstant:
                result = GetPackConst(index);
                break;
            case ImmedHolder:
            {
                std::map<unsigned, double>::const_iterator
                    i = match.ImmedMap.find(index);
                if(i == match.ImmedMap.end()) return false; // impossible
                result = i->second;
                break;
            }
            case NamedHolder:
            {
                std::map<unsigned, std::pair<fphash_t, size_t> >::const_iterator
                    i = match.NamedMap.find(index);
                if(i == match.NamedMap.end()) return false; // impossible
                result = (double) i->second.second;
                break;
            }
            case RestHolder:
            {
                // Not enumerable
                return false;
            }
            case SubFunction:
            {
                // Not enumerable
                return false;
            }
            default:
            {
                switch(OPCODE(opcode))
                {
                    case cAdd:
                        result=0;
                        for(unsigned p=0; p<count; ++p)
                        {
                            double tmp;
                            if(!pack.plist[index+p].GetConst(match, tmp)) return false;
                            result += tmp;
                        }
                        break;
                    case cMul:
                        result=1;
                        for(unsigned p=0; p<count; ++p)
                        {
                            double tmp;
                            if(!pack.plist[index+p].GetConst(match, tmp)) return false;
                            result *= tmp;
                        }
                        break;
                    case cMin:
                        for(unsigned p=0; p<count; ++p)
                        {
                            double tmp;
                            if(!pack.plist[index+p].GetConst(match, tmp)) return false;
                            if(p == 0 || tmp < result) result = tmp;
                        }
                        break;
                    case cMax:
                        for(unsigned p=0; p<count; ++p)
                        {
                            double tmp;
                            if(!pack.plist[index+p].GetConst(match, tmp)) return false;
                            if(p == 0 || tmp > result) result = tmp;
                        }
                        break;
                    case cSin: if(!pack.plist[index].GetConst(match, result))return false;
                               result = std::sin(result); break;
                    case cCos: if(!pack.plist[index].GetConst(match, result))return false;
                               result = std::cos(result); break;
                    case cTan: if(!pack.plist[index].GetConst(match, result))return false;
                               result = std::tan(result); break;
                    case cAsin: if(!pack.plist[index].GetConst(match, result))return false;
                                result = std::asin(result); break;
                    case cAcos: if(!pack.plist[index].GetConst(match, result))return false;
                                result = std::acos(result); break;
                    case cAtan: if(!pack.plist[index].GetConst(match, result))return false;
                                result = std::atan(result); break;
                    case cSinh: if(!pack.plist[index].GetConst(match, result))return false;
                                result = std::sinh(result); break;
                    case cCosh: if(!pack.plist[index].GetConst(match, result))return false;
                                result = std::cosh(result); break;
                    case cTanh: if(!pack.plist[index].GetConst(match, result))return false;
                                 result = std::tanh(result); break;
#ifndef FP_NO_ASINH
                    case cAsinh: if(!pack.plist[index].GetConst(match, result))return false;
                                 result = asinh(result); break;
                    case cAcosh: if(!pack.plist[index].GetConst(match, result))return false;
                                 result = acosh(result); break;
                    case cAtanh: if(!pack.plist[index].GetConst(match, result))return false;
                                 result = atanh(result); break;
#endif
                    case cCeil: if(!pack.plist[index].GetConst(match, result))return false;
                                result = std::ceil(result); break;
                    case cFloor: if(!pack.plist[index].GetConst(match, result))return false;
                                 result = std::floor(result); break;
                    case cLog: if(!pack.plist[index].GetConst(match, result))return false;
                               result = std::log(result); break;
                    case cLog2: if(!pack.plist[index].GetConst(match, result))return false;
                                result = std::log(result) * CONSTANT_L2I;
                                //result = std::log2(result);
                                break;
                    case cLog10: if(!pack.plist[index].GetConst(match, result))return false;
                                 result = std::log10(result); break;
                    case cPow:
                    {
                        if(!pack.plist[index+0].GetConst(match, result))return false;
                        double tmp;
                        if(!pack.plist[index+1].GetConst(match, tmp))return false;
                        result = std::pow(result, tmp);
                        break;
                    }
                    case cMod:
                    {
                        if(!pack.plist[index+0].GetConst(match, result))return false;
                        double tmp;
                        if(!pack.plist[index+1].GetConst(match, tmp))return false;
                        result = std::fmod(result, tmp);
                        break;
                    }
                    default:
                        return false;
                }
            }
        }
        if(transformation == Negate) result = -result;
        if(transformation == Invert) result = 1.0 / result;
        return true;
    }

    void MatchedParams::SynthesizeTree(
        FPoptimizer_CodeTree::CodeTree& tree,
        const MatchedParams& matcher,
        MatchedParams::CodeTreeMatch& match) const
    {
        for(unsigned a=0; a<count; ++a)
        {
            const ParamSpec& param = pack.plist[index+a];
            if(param.opcode == RestHolder)
            {
                // Add children directly to this tree
                param.SynthesizeTree(tree, matcher, match);
            }
            else
            {
                FPoptimizer_CodeTree::CodeTree* subtree = new FPoptimizer_CodeTree::CodeTree;
                param.SynthesizeTree(*subtree, matcher, match);
                subtree->Sort();
                subtree->Recalculate_Hash_NoRecursion(); // rehash this, but not the children, nor the parent
                FPoptimizer_CodeTree::CodeTree::Param p(subtree, param.sign) ;
                tree.AddParam(p);
            }
        }
    }

    void MatchedParams::ReplaceParams(
        FPoptimizer_CodeTree::CodeTree& tree,
        const MatchedParams& matcher,
        MatchedParams::CodeTreeMatch& match) const
    {
        // Replace the 0-level params indicated in "match" with the ones we have

        // First, construct the tree recursively using the "match" info
        SynthesizeTree(tree, matcher, match);

        // Remove the indicated params
        std::sort(match.param_numbers.begin(), match.param_numbers.end());
        for(size_t a=match.param_numbers.size(); a-->0; )
        {
            size_t num = match.param_numbers[a];
            tree.DelParam(num);
        }
        tree.Sort();
        tree.Rehash(true); // rehash this and its parents, but not its children
    }

    void MatchedParams::ReplaceTree(
        FPoptimizer_CodeTree::CodeTree& tree,
        const MatchedParams& matcher,
        CodeTreeMatch& match) const
    {
        // Replace the entire tree with one indicated by our Params[0]
        // Note: The tree is still constructed using the holders indicated in "match".
        std::vector<FPoptimizer_CodeTree::CodeTree::Param> OldParams = tree.Params;
        tree.Params.clear();
        pack.plist[index].SynthesizeTree(tree, matcher, match);

        tree.Sort();
        tree.Rehash(true);  // rehash this and its parents, but not its children
    }

    /* Synthesizes a new tree based on the given information
     * in ParamSpec. Assume the tree is empty, don't deallocate
     * anything. Don't touch Hash, Parent.
     */
    void ParamSpec::SynthesizeTree(
        FPoptimizer_CodeTree::CodeTree& tree,
        const MatchedParams& matcher,
        MatchedParams::CodeTreeMatch& match) const
    {
        switch(SpecialOpcode(opcode))
        {
            case RestHolder:
            {
                std::map<unsigned, std::vector<fphash_t> >
                    ::const_iterator i = match.RestMap.find(index);

                assert(i != match.RestMap.end());

                /*std::cout << std::flush;
                fprintf(stderr, "Restmap %u, sign %d, size is %u -- params %u\n",
                    (unsigned) i->first, sign, (unsigned) i->second.size(),
                    (unsigned) tree.Params.size());*/

                for(size_t a=0; a<i->second.size(); ++a)
                {
                    fphash_t hash = i->second[a];

                    std::map<fphash_t, FPoptimizer_CodeTree::CodeTreeP>
                        ::const_iterator j = match.trees.find(hash);

                    assert(j != match.trees.end());

                    FPoptimizer_CodeTree::CodeTree* subtree = j->second->Clone();
                    FPoptimizer_CodeTree::CodeTree::Param p(subtree, sign);
                    tree.AddParam(p);
                }
                /*fprintf(stderr, "- params size became %u\n", (unsigned)tree.Params.size());
                fflush(stderr);*/
                break;
            }
            case SubFunction:
            {
                const Function& fitem = pack.flist[index];
                tree.Opcode = fitem.opcode;
                const MatchedParams& mitem = pack.mlist[fitem.index];
                mitem.SynthesizeTree(tree, matcher, match);
                break;
            }
            case NamedHolder:
                if(!anyrepeat && minrepeat == 1)
                {
                    /* Literal parameter */
                    std::map<unsigned, std::pair<fphash_t, size_t> >
                        ::const_iterator i = match.NamedMap.find(index);

                    assert(i != match.NamedMap.end());

                    fphash_t hash = i->second.first;

                    std::map<fphash_t, FPoptimizer_CodeTree::CodeTreeP>
                        ::const_iterator j = match.trees.find(hash);

                    assert(j != match.trees.end());

                    tree.Opcode = j->second->Opcode;
                    switch(tree.Opcode)
                    {
                        case cImmed: tree.Value = j->second->Value; break;
                        case cVar:   tree.Var   = j->second->Var;  break;
                        case cFCall:
                        case cPCall: tree.Funcno = j->second->Funcno; break;
                    }

                    tree.SetParams(j->second->Params);
                    break;
                }
                // passthru; x+ is synthesized as the number, not as the tree
            case NumConstant:
            case ImmedHolder:
            default:
                tree.Opcode = cImmed;
                GetConst(match, tree.Value); // note: return value is ignored
                break;
        }
    }

#ifdef DEBUG_SUBSTITUTIONS
    void DumpParam(const ParamSpec& p)
    {
        //std::cout << "/*p" << (&p-pack.plist) << "*/";

        if(p.sign) std::cout << '~';
        if(p.transformation == Negate) std::cout << '-';
        if(p.transformation == Invert) std::cout << '/';

        switch(SpecialOpcode(p.opcode))
        {
            case NumConstant: std::cout << GetPackConst(p.index); break;
            case ImmedHolder: std::cout << ImmedHolderNames[p.index]; break;
            case NamedHolder: std::cout << NamedHolderNames[p.index]; break;
            case RestHolder: std::cout << '<' << p.index << '>'; break;
            case SubFunction: DumpFunction(pack.flist[p.index]); break;
            default:
            {
                std::string opcode = FP_GetOpcodeName(p.opcode).substr(1);
                for(size_t a=0; a<opcode.size(); ++a) opcode[a] = std::toupper(opcode[a]);
                std::cout << opcode << '(';
                for(unsigned a=0; a<p.count; ++a)
                {
                    if(a > 0) std::cout << ' ';
                    DumpParam(pack.plist[p.index+a]);
                }
                std::cout << " )";
            }
        }
        if(p.anyrepeat && p.minrepeat==1) std::cout << '*';
        if(p.anyrepeat && p.minrepeat==2) std::cout << '+';
    }

    void DumpParams(const MatchedParams& mitem)
    {
        //std::cout << "/*m" << (&mitem-pack.mlist) << "*/";

        if(mitem.type == PositionalParams) std::cout << '[';
        if(mitem.type == SelectedParams) std::cout << '{';

        for(unsigned a=0; a<mitem.count; ++a)
        {
            std::cout << ' ';
            DumpParam(pack.plist[mitem.index + a]);
        }

        switch(mitem.balance)
        {
            case BalanceMorePos: std::cout << " =+"; break;
            case BalanceMoreNeg: std::cout << " =-"; break;
            case BalanceEqual:   std::cout << " =="; break;
            case BalanceDontCare: break;
        }

        if(mitem.type == PositionalParams) std::cout << " ]";
        if(mitem.type == SelectedParams) std::cout << " }";
    }

    void DumpFunction(const Function& fitem)
    {
        //std::cout << "/*f" << (&fitem-pack.flist) << "*/";

        std::cout << '(' << FP_GetOpcodeName(fitem.opcode);
        DumpParams(pack.mlist[fitem.index]);
        std::cout << ')';
    }
    void DumpTree(const FPoptimizer_CodeTree::CodeTree& tree)
    {
        //std::cout << "/*" << tree.Depth << "*/";
        const char* sep2 = "";
        //std::cout << '[' << std::hex << tree.Hash << ']' << std::dec;
        switch(tree.Opcode)
        {
            case cImmed: std::cout << tree.Value; return;
            case cVar:   std::cout << "Var" << tree.Var; return;
            case cAdd: sep2 = " +"; break;
            case cMul: sep2 = " *"; break;
            case cAnd: sep2 = " &"; break;
            case cOr: sep2 = " |"; break;
            default:
                std::cout << FP_GetOpcodeName(tree.Opcode);
                if(tree.Opcode == cFCall || tree.Opcode == cPCall)
                    std::cout << ':' << tree.Funcno;
        }
        std::cout << '(';
        if(tree.Params.size() <= 1 && *sep2) std::cout << (sep2+1) << ' ';
        for(size_t a=0; a<tree.Params.size(); ++a)
        {
            if(a > 0) std::cout << ' ';
            if(tree.Params[a].sign) std::cout << '~';

            DumpTree(*tree.Params[a].param);

            if(tree.Params[a].param->Parent != &tree)
            {
                std::cout << "(?""?""?))";
            }

            if(a+1 < tree.Params.size()) std::cout << sep2;
        }
        std::cout << ')';
    }
    void DumpMatch(const Function& input,
                   const FPoptimizer_CodeTree::CodeTree& tree,
                   const MatchedParams& replacement,
                   const MatchedParams::CodeTreeMatch& matchrec,
                   bool DidMatch)
    {
        std::cout <<
            "Found " << (DidMatch ? "match" : "mismatch") << ":\n"
            "  Pattern    : ";
        DumpFunction(input);
        std::cout << "\n"
            "  Replacement: ";
        DumpParams(replacement);
        std::cout << "\n";

        std::cout <<
            "  Tree       : ";
        DumpTree(tree);
        std::cout << "\n";

        for(std::map<unsigned, std::pair<fphash_t, size_t> >::const_iterator
            i = matchrec.NamedMap.begin(); i != matchrec.NamedMap.end(); ++i)
        {
            std::cout << "           " << NamedHolderNames[i->first] << " = ";
            DumpTree(*matchrec.trees.find(i->second.first)->second);
            std::cout << " (" << i->second.second << " matches)\n";
        }

        for(std::map<unsigned, double>::const_iterator
            i = matchrec.ImmedMap.begin(); i != matchrec.ImmedMap.end(); ++i)
        {
            std::cout << "           " << ImmedHolderNames[i->first] << " = ";
            std::cout << i->second << std::endl;
        }

        for(std::map<unsigned, std::vector<fphash_t> >::const_iterator
            i = matchrec.RestMap.begin(); i != matchrec.RestMap.end(); ++i)
        {
            for(size_t a=0; a<i->second.size(); ++a)
            {
                fphash_t hash = i->second[a];
                std::cout << "         <" << i->first << "> = ";
                DumpTree(*matchrec.trees.find(hash)->second);
                std::cout << std::endl;
            }
            if(i->second.empty())
                std::cout << "         <" << i->first << "> = <empty>\n";
        }
        std::cout << std::flush;
    }
#endif
}

#endif
#include "fpconfig.h"
#include "fparser.h"
#include "fptypes.h"


using namespace FUNCTIONPARSERTYPES;

#ifdef FP_SUPPORT_OPTIMIZER
namespace FPoptimizer_CodeTree
{
    bool    CodeTree::IsImmed() const { return Opcode == cImmed; }
    bool    CodeTree::IsVar()   const { return Opcode == cVar; }
}

using namespace FPoptimizer_CodeTree;

void FunctionParser::Optimize()
{
    CopyOnWrite();

    //PrintByteCode(std::cout);

    FPoptimizer_CodeTree::CodeTreeP tree
        = CodeTree::GenerateFrom(data->ByteCode, data->Immed, *data);

    while(FPoptimizer_Grammar::pack.glist[0].ApplyTo(*tree))
        {}

    while(FPoptimizer_Grammar::pack.glist[1].ApplyTo(*tree))
        {}

    while(FPoptimizer_Grammar::pack.glist[2].ApplyTo(*tree))
        {}

    tree->Sort_Recursive();

    std::vector<unsigned> byteCode;
    std::vector<double> immed;
    size_t stacktop_max = 0;
    tree->SynthesizeByteCode(byteCode, immed, stacktop_max);

    /*std::cout << std::flush;
    std::cerr << std::flush;
    fprintf(stderr, "Estimated stacktop %u\n", (unsigned)stacktop_max);
    fflush(stderr);*/

    if(data->StackSize != stacktop_max)
    {
        data->StackSize = stacktop_max; // note: gcc warning is meaningful
        data->Stack.resize(stacktop_max);
    }

    data->ByteCode.swap(byteCode);
    data->Immed.swap(immed);

    //PrintByteCode(std::cout);
}

#endif
#include <cmath>
#include <list>
#include <cassert>

#include "fptypes.h"

#ifdef FP_SUPPORT_OPTIMIZER

using namespace FUNCTIONPARSERTYPES;
//using namespace FPoptimizer_Grammar;

#ifndef FP_GENERATING_POWI_TABLE
static const unsigned MAX_POWI_BYTECODE_LENGTH = 15;
#else
static const unsigned MAX_POWI_BYTECODE_LENGTH = 999;
#endif
static const unsigned MAX_MULI_BYTECODE_LENGTH = 5;

#define POWI_TABLE_SIZE 256
#define POWI_WINDOW_SIZE 3
#ifndef FP_GENERATING_POWI_TABLE
static const
#endif
signed char powi_table[POWI_TABLE_SIZE] =
{
      0,   1,   1,   1,   2,   1,   3,   1, /*   0 -   7 */
      4,   1,   5,   1,   6,   1,  -2,   5, /*   8 -  15 */
      8,   1,   9,   1,  10,  -3,  11,   1, /*  16 -  23 */
     12,   5,  13,   9,  14,   1,  15,   1, /*  24 -  31 */
     16,   1,  17,  -5,  18,   1,  19,  13, /*  32 -  39 */
     20,   1,  21,   1,  22,   9,  -2,   1, /*  40 -  47 */
     24,   1,  25,  17,  26,   1,  27,  11, /*  48 -  55 */
     28,   1,  29,   8,  30,   1,  -2,   1, /*  56 -  63 */
     32,   1,  33,   1,  34,   1,  35,   1, /*  64 -  71 */
     36,   1,  37,  25,  38, -11,  39,   1, /*  72 -  79 */
     40,   9,  41,   1,  42,  17,   1,  29, /*  80 -  87 */
     44,   1,  45,   1,  46,  -3,  32,  19, /*  88 -  95 */
     48,   1,  49,  33,  50,   1,  51,   1, /*  96 - 103 */
     52,  35,  53,   8,  54,   1,  55,  37, /* 104 - 111 */
     56,   1,  57,  -5,  58,  13,  59, -17, /* 112 - 119 */
     60,   1,  61,  41,  62,  25,  -2,   1, /* 120 - 127 */
     64,   1,  65,   1,  66,   1,  67,  45, /* 128 - 135 */
     68,   1,  69,   1,  70,  48,  16,   8, /* 136 - 143 */
     72,   1,  73,  49,  74,   1,  75,   1, /* 144 - 151 */
     76,  17,   1,  -5,  78,   1,  32,  53, /* 152 - 159 */
     80,   1,  81,   1,  82,  33,   1,   2, /* 160 - 167 */
     84,   1,  85,  57,  86,   8,  87,  35, /* 168 - 175 */
     88,   1,  89,   1,  90,   1,  91,  61, /* 176 - 183 */
     92,  37,  93,  17,  94,  -3,  64,   2, /* 184 - 191 */
     96,   1,  97,  65,  98,   1,  99,   1, /* 192 - 199 */
    100,  67, 101,   8, 102,  41, 103,  69, /* 200 - 207 */
    104,   1, 105,  16, 106,  24, 107,   1, /* 208 - 215 */
    108,   1, 109,  73, 110,  17, 111,   1, /* 216 - 223 */
    112,  45, 113,  32, 114,   1, 115, -33, /* 224 - 231 */
    116,   1, 117,  -5, 118,  48, 119,   1, /* 232 - 239 */
    120,   1, 121,  81, 122,  49, 123,  13, /* 240 - 247 */
    124,   1, 125,   1, 126,   1,  -2,  85  /* 248 - 255 */
}; /* as in gcc, but custom-optimized for stack calculation */
static const int POWI_CACHE_SIZE = 256;

#define FPO(x) /**/
//#define FPO(x) x

static const struct SequenceOpCode
{
    double basevalue;
    unsigned op_flip;
    unsigned op_normal, op_normal_flip;
    unsigned op_inverse, op_inverse_flip;
} AddSequence = {0.0, cNeg, cAdd, cAdd, cSub, cRSub },
  MulSequence = {1.0, cInv, cMul, cMul, cDiv, cRDiv };

class FPoptimizer_CodeTree::CodeTree::ByteCodeSynth
{
public:
    ByteCodeSynth()
        : ByteCode(), Immed(), StackTop(0), StackMax(0)
    {
        /* estimate the initial requirements as such */
        ByteCode.reserve(64);
        Immed.reserve(8);
    }

    void Pull(std::vector<unsigned>& bc,
              std::vector<double>&   imm,
              size_t& StackTop_max)
    {
        ByteCode.swap(bc);
        Immed.swap(imm);
        StackTop_max = StackMax;
    }

    size_t GetByteCodeSize() const { return ByteCode.size(); }
    size_t GetStackTop()     const { return StackTop; }

    void PushVar(unsigned varno)
    {
        ByteCode.push_back(varno);
        SetStackTop(StackTop+1);
    }

    void PushImmed(double immed)
    {
        ByteCode.push_back(cImmed);
        Immed.push_back(immed);
        SetStackTop(StackTop+1);
    }

    void StackTopIs(fphash_t hash)
    {
        if(StackTop > 0)
        {
            StackHash[StackTop-1].first = true;
            StackHash[StackTop-1].second = hash;
        }
    }

    void AddOperation(unsigned opcode, unsigned eat_count, unsigned produce_count = 1)
    {
        SetStackTop(StackTop - eat_count);

        if(opcode == cMul && ByteCode.back() == cDup)
            ByteCode.back() = cSqr;
        else
            ByteCode.push_back(opcode);
        SetStackTop(StackTop + produce_count);
    }

    void DoPopNMov(size_t targetpos, size_t srcpos)
    {
        ByteCode.push_back(cPopNMov);
        ByteCode.push_back( (unsigned) targetpos);
        ByteCode.push_back( (unsigned) srcpos);

        SetStackTop(srcpos+1);
        StackHash[targetpos] = StackHash[srcpos];
        SetStackTop(targetpos+1);
    }

    void DoDup(size_t src_pos)
    {
        if(src_pos == StackTop-1)
        {
            ByteCode.push_back(cDup);
        }
        else
        {
            ByteCode.push_back(cFetch);
            ByteCode.push_back( (unsigned) src_pos);
        }
        SetStackTop(StackTop + 1);
        StackHash[StackTop-1] = StackHash[src_pos];
    }

    bool FindAndDup(fphash_t hash)
    {
        for(size_t a=StackHash.size(); a-->0; )
        {
            if(StackHash[a].first && StackHash[a].second == hash)
            {
                DoDup(a);
                return true;
            }
        }
        return false;
    }

    void SynthIfStep1(size_t& ofs)
    {
        SetStackTop(StackTop-1); // the If condition was popped.

        ofs = ByteCode.size();
        ByteCode.push_back(cIf);
        ByteCode.push_back(0); // code index
        ByteCode.push_back(0); // Immed index
    }
    void SynthIfStep2(size_t& ofs)
    {
        SetStackTop(StackTop-1); // ignore the pushed then-branch result.

        ByteCode[ofs+1] = unsigned( ByteCode.size()+2 );
        ByteCode[ofs+2] = unsigned( Immed.size()      );

        ofs = ByteCode.size();
        ByteCode.push_back(cJump);
        ByteCode.push_back(0); // code index
        ByteCode.push_back(0); // Immed index
    }
    void SynthIfStep3(size_t& ofs)
    {
        SetStackTop(StackTop-1); // ignore the pushed else-branch result.

        ByteCode[ofs+1] = unsigned( ByteCode.size()-1 );
        ByteCode[ofs+2] = unsigned( Immed.size()      );

        SetStackTop(StackTop+1); // one or the other was pushed.
    }

private:
    void SetStackTop(size_t value)
    {
        StackTop = value;
        if(StackTop > StackMax) StackMax = StackTop;
        StackHash.resize(value);
    }

private:
    std::vector<unsigned> ByteCode;
    std::vector<double>   Immed;

    std::vector<std::pair<bool/*known*/, fphash_t/*hash*/> > StackHash;
    size_t StackTop;
    size_t StackMax;
};

namespace
{
    using namespace FPoptimizer_CodeTree;

    bool AssembleSequence(
                  CodeTree& tree, long count,
                  const SequenceOpCode& sequencing,
                  CodeTree::ByteCodeSynth& synth,
                  size_t max_bytecode_grow_length);

    class PowiCache
    {
    private:
        int cache[POWI_CACHE_SIZE];
        int cache_needed[POWI_CACHE_SIZE];

    public:
        PowiCache()
            : cache(), cache_needed() /* Assume we have no factors in the cache */
        {
            /* Decide which factors we would need multiple times.
             * Output:
             *   cache[]        = these factors were generated
             *   cache_needed[] = number of times these factors were desired
             */
            cache[1] = 1; // We have this value already.
        }

        bool Plan_Add(long value, int count)
        {
            if(value >= POWI_CACHE_SIZE) return false;
            //FPO(fprintf(stderr, "%ld will be needed %d times more\n", count, need_count));
            cache_needed[value] += count;
            return cache[value] != 0;
        }

        void Plan_Has(long value)
        {
            if(value < POWI_CACHE_SIZE)
                cache[value] = 1; // This value has been generated
        }

        void Start(size_t value1_pos)
        {
            for(int n=2; n<POWI_CACHE_SIZE; ++n)
                cache[n] = -1; /* Stack location for each component */

            Remember(1, value1_pos);

            DumpContents();
        }

        int Find(long value) const
        {
            if(value < POWI_CACHE_SIZE)
            {
                if(cache[value] >= 0)
                {
                    // found from the cache
                    FPO(fprintf(stderr, "* I found %ld from cache (%u,%d)\n",
                        value, (unsigned)cache[value], cache_needed[value]));
                    return cache[value];
                }
            }
            return -1;
        }

        void Remember(long value, size_t stackpos)
        {
            if(value >= POWI_CACHE_SIZE) return;

            FPO(fprintf(stderr, "* Remembering that %ld can be found at %u (%d uses remain)\n",
                value, (unsigned)stackpos, cache_needed[value]));
            cache[value] = (int) stackpos;
        }

        void DumpContents() const
        {
            FPO(for(int a=1; a<POWI_CACHE_SIZE; ++a)
                if(cache[a] >= 0 || cache_needed[a] > 0)
                {
                    fprintf(stderr, "== cache: sp=%d, val=%d, needs=%d\n",
                        cache[a], a, cache_needed[a]);
                })
        }

        int UseGetNeeded(long value)
        {
            if(value >= 0 && value < POWI_CACHE_SIZE)
                return --cache_needed[value];
            return 0;
        }
    };

    size_t AssembleSequence_Subdivide(
        long count,
        PowiCache& cache,
        const SequenceOpCode& sequencing,
        CodeTree::ByteCodeSynth& synth);

    void Subdivide_Combine(
        size_t apos, long aval,
        size_t bpos, long bval,
        PowiCache& cache,

        unsigned cumulation_opcode,
        unsigned cimulation_opcode_flip,

        CodeTree::ByteCodeSynth& synth);
}

namespace
{
    typedef
        std::map<fphash_t,  std::pair<size_t, CodeTreeP> >
        TreeCountType;

    void FindTreeCounts(TreeCountType& TreeCounts, CodeTreeP tree)
    {
        TreeCountType::iterator i = TreeCounts.lower_bound(tree->Hash);
        if(i == TreeCounts.end() || i->first != tree->Hash)
            TreeCounts.insert(i, std::make_pair(tree->Hash, std::make_pair(size_t(1), tree)));
        else
            i->second.first += 1;

        for(size_t a=0; a<tree->Params.size(); ++a)
            FindTreeCounts(TreeCounts, tree->Params[a].param);
    }

    void RememberRecursivelyHashList(std::set<fphash_t>& hashlist,
                                     CodeTreeP tree)
    {
        hashlist.insert(tree->Hash);
        for(size_t a=0; a<tree->Params.size(); ++a)
            RememberRecursivelyHashList(hashlist, tree->Params[a].param);
    }
#if 0
    void PowiTreeSequence(CodeTree& tree, const CodeTreeP param, long value)
    {
        tree.Params.clear();
        if(value < 0)
        {
            tree.Opcode = cInv;
            CodeTree* subtree = new CodeTree;
            PowiTreeSequence(*subtree, param, -value);
            tree.AddParam( CodeTree::Param(subtree, false) );
            tree.Recalculate_Hash_NoRecursion();
        }
        else
        {
            assert(value != 0 && value != 1);
            long half = 1;
            if(value < POWI_TABLE_SIZE)
                half = powi_table[value];
            else if(value & 1)
                half = value & ((1 << POWI_WINDOW_SIZE) - 1); // that is, value & 7
            else
                half = value / 2;
            long otherhalf = value-half;
            if(half > otherhalf || half<0) std::swap(half,otherhalf);

            if(half == 1)
                tree.AddParam( CodeTree::Param(param->Clone(), false) );
            else
            {
                CodeTree* subtree = new CodeTree;
                PowiTreeSequence(*subtree, param, half);
                tree.AddParam( CodeTree::Param(subtree, false) );
            }

            bool otherhalf_sign = otherhalf < 0;
            if(otherhalf < 0) otherhalf = -otherhalf;

            if(otherhalf == 1)
                tree.AddParam( CodeTree::Param(param->Clone(), otherhalf_sign) );
            else
            {
                CodeTree* subtree = new CodeTree;
                PowiTreeSequence(*subtree, param, otherhalf);
                tree.AddParam( CodeTree::Param(subtree, otherhalf_sign) );
            }

            tree.Opcode = cMul;

            tree.Sort();
            tree.Recalculate_Hash_NoRecursion();
        }
    }
    void ConvertPowi(CodeTree& tree)
    {
        if(tree.Opcode == cPow)
        {
            const CodeTree::Param& p0 = tree.Params[0];
            const CodeTree::Param& p1 = tree.Params[1];

            if(p1.param->IsLongIntegerImmed())
            {
                FPoptimizer_CodeTree::CodeTree::ByteCodeSynth temp_synth;

                if(AssembleSequence(*p0.param, p1.param->GetLongIntegerImmed(),
                    MulSequence,
                    temp_synth,
                    MAX_POWI_BYTECODE_LENGTH)
                  )
                {
                    // Seems like a good candidate!
                    // Redo the tree as a powi sequence.
                    CodeTreeP param = p0.param;
                    PowiTreeSequence(tree, param, p1.param->GetLongIntegerImmed());
                }
            }
        }
        for(size_t a=0; a<tree.Params.size(); ++a)
            ConvertPowi(*tree.Params[a].param);
    }
#endif
}

namespace FPoptimizer_CodeTree
{
    void CodeTree::SynthesizeByteCode(
        std::vector<unsigned>& ByteCode,
        std::vector<double>&   Immed,
        size_t& stacktop_max)
    {
        ByteCodeSynth synth;
    #if 0
        /* Convert integer powi sequences into trees
         * to put them into the scope of the CSE
         */
        /* Disabled: Seems to actually slow down */
        ConvertPowi(*this);
    #endif

        /* Find common subtrees */
        TreeCountType TreeCounts;
        FindTreeCounts(TreeCounts, this);

        /* Synthesize some of the most common ones */
        std::set<fphash_t> AlreadyDoneTrees;
    FindMore: ;
        size_t best_score = 0;
        TreeCountType::const_iterator synth_it;
        for(TreeCountType::const_iterator
            i = TreeCounts.begin();
            i != TreeCounts.end();
            ++i)
        {
            size_t score = i->second.first;
            if(score < 2) continue; // It must always occur at least twice
            if(i->second.second->Depth < 2) continue; // And it must not be a simple expression
            if(AlreadyDoneTrees.find(i->first)
            != AlreadyDoneTrees.end()) continue; // And it must not yet have been synthesized
            score *= i->second.second->Depth;
            if(score > best_score)
                { best_score = score; synth_it = i; }
        }
        if(best_score > 0)
        {
            /* Synthesize the selected tree */
            synth_it->second.second->SynthesizeByteCode(synth);
            /* Add the tree and all its children to the AlreadyDoneTrees list,
             * to prevent it from being re-synthesized
             */
            RememberRecursivelyHashList(AlreadyDoneTrees, synth_it->second.second);
            goto FindMore;
        }

        /* Then synthesize the actual expression */
        SynthesizeByteCode(synth);
      #ifndef FP_DISABLE_EVAL
        /* Ensure that the expression result is
         * the only thing that remains in the stack
         */
        /* Removed: Fparser does not seem to care! */
        /* But if cEval is supported, it still needs to be done. */
        if(synth.GetStackTop() > 1)
            synth.DoPopNMov(0, synth.GetStackTop()-1);
      #endif
        synth.Pull(ByteCode, Immed, stacktop_max);
    }

    void CodeTree::SynthesizeByteCode(ByteCodeSynth& synth)
    {
        // If the synth can already locate our operand in the stack,
        // never mind synthesizing it again, just dup it.
        if(synth.FindAndDup(Hash))
        {
            return;
        }

        switch(Opcode)
        {
            case cVar:
                synth.PushVar(GetVar());
                break;
            case cImmed:
                synth.PushImmed(GetImmed());
                break;
            case cAdd:
            case cMul:
            case cMin:
            case cMax:
            case cAnd:
            case cOr:
            {
                // Operand re-sorting:
                // If the first param has a sign, try to find a param
                // that does _not_ have a sign and put it first.
                // This can be done because params are commutative
                // when they are grouped with their signs.
                if(!Params.empty() && Params[0].sign)
                {
                    for(size_t a=1; a<Params.size(); ++a)
                        if(!Params[a].sign)
                        {
                            std::swap(Params[0], Params[a]);
                            break;
                        }
                }

                // Try to ensure that Immeds don't have a sign
                for(size_t a=0; a<Params.size(); ++a)
                {
                    CodeTreeP& param = Params[a].param;
                    if(Params[a].sign && param->IsImmed())
                        switch(Opcode)
                        {
                            case cAdd: param->NegateImmed(); Params[a].sign=false; break;
                            case cMul: if(param->GetImmed() == 0.0) break;
                                       param->InvertImmed(); Params[a].sign=false; break;
                            case cAnd:
                            case cOr:  param->NotTheImmed(); Params[a].sign=false; break;
                        }
                }

                if(Opcode == cMul) // Special treatment for cMul sequences
                {
                    // If the paramlist contains an Immed, and that Immed
                    // fits in a long-integer, try to synthesize it
                    // as add-sequences instead.
                    for(size_t a=0; a<Params.size(); ++a)
                    {
                        Param p = Params[a];
                        CodeTreeP& param = p.param;
                        if(!p.sign && param->IsLongIntegerImmed())
                        {
                            long value = param->GetLongIntegerImmed();
                            Params.erase(Params.begin()+a);

                            bool success = AssembleSequence(
                                *this, value, AddSequence,
                                synth,
                                MAX_MULI_BYTECODE_LENGTH);

                            // Readd the token so that we don't need
                            // to deal with allocationd/deallocation here.
                            Params.insert(Params.begin()+a, p);

                            if(success)
                            {
                                // this tree was treated just fine
                                synth.StackTopIs(Hash);
                                return;
                            }
                        }
                    }
                }

                int n_stacked = 0;
                for(size_t a=0; a<Params.size(); ++a)
                {
                    CodeTreeP const & param = Params[a].param;
                    bool               sign = Params[a].sign;

                    param->SynthesizeByteCode(synth);
                    ++n_stacked;

                    if(sign) // Is the operand negated/inverted?
                    {
                        if(n_stacked == 1)
                        {
                            // Needs unary negation/invertion. Decide how to accomplish it.
                            switch(Opcode)
                            {
                                case cAdd:
                                    synth.AddOperation(cNeg, 1); // stack state: -1+1 = +0
                                    break;
                                case cMul:
                                    synth.AddOperation(cInv, 1); // stack state: -1+1 = +0
                                    break;
                                case cAnd:
                                case cOr:
                                    synth.AddOperation(cNot, 1); // stack state: -1+1 = +0
                                    break;
                            }
                            // Note: We could use RDiv or RSub when the first
                            // token is negated/inverted and the second is not, to
                            // avoid cNeg/cInv/cNot, but thanks to the operand
                            // re-sorting in the beginning of this code, this
                            // situation never arises.
                            // cNeg/cInv/cNot is only synthesized when the group
                            // consists entirely of negated/inverted items.
                        }
                        else
                        {
                            // Needs binary negation/invertion. Decide how to accomplish it.
                            switch(Opcode)
                            {
                                case cAdd:
                                    synth.AddOperation(cSub, 2); // stack state: -2+1 = -1
                                    break;
                                case cMul:
                                    synth.AddOperation(cDiv, 2); // stack state: -2+1 = -1
                                    break;
                                case cAnd:
                                case cOr:
                                    synth.AddOperation(cNot,   1);   // stack state: -1+1 = +0
                                    synth.AddOperation(Opcode, 2); // stack state: -2+1 = -1
                                    break;
                            }
                            n_stacked = n_stacked - 2 + 1;
                        }
                    }
                    else if(n_stacked > 1)
                    {
                        // Cumulate at the earliest opportunity.
                        synth.AddOperation(Opcode, 2); // stack state: -2+1 = -1
                        n_stacked = n_stacked - 2 + 1;
                    }
                }
                if(n_stacked == 0)
                {
                    // Uh, we got an empty cAdd/cMul/whatever...
                    // Synthesize a default value.
                    // This should never happen.
                    switch(Opcode)
                    {
                        case cAdd:
                        case cOr:
                            synth.PushImmed(0);
                            break;
                        case cMul:
                        case cAnd:
                            synth.PushImmed(1);
                            break;
                        case cMin:
                        case cMax:
                            //synth.PushImmed(NaN);
                            synth.PushImmed(0);
                            break;
                    }
                    ++n_stacked;
                }
                assert(n_stacked == 1);
                break;
            }
            case cPow:
            {
                const Param& p0 = Params[0];
                const Param& p1 = Params[1];

                if(!p1.param->IsLongIntegerImmed()
                || !AssembleSequence( /* Optimize integer exponents */
                        *p0.param, p1.param->GetLongIntegerImmed(),
                        MulSequence,
                        synth,
                        MAX_POWI_BYTECODE_LENGTH)
                  )
                {
                    p0.param->SynthesizeByteCode(synth);
                    p1.param->SynthesizeByteCode(synth);
                    synth.AddOperation(Opcode, 2);
                }
                break;
            }
            case cIf:
            {
                size_t ofs;
                // If the parameter amount is != 3, we're screwed.
                Params[0].param->SynthesizeByteCode(synth); // expression
                synth.SynthIfStep1(ofs);
                Params[1].param->SynthesizeByteCode(synth); // true branch
                synth.SynthIfStep2(ofs);
                Params[2].param->SynthesizeByteCode(synth); // false branch
                synth.SynthIfStep3(ofs);
                break;
            }
            case cFCall:
            {
                // If the parameter count is invalid, we're screwed.
                for(size_t a=0; a<Params.size(); ++a)
                    Params[a].param->SynthesizeByteCode(synth);
                synth.AddOperation(Opcode, (unsigned) Params.size());
                synth.AddOperation(Funcno, 0, 0);
                break;
            }
            case cPCall:
            {
                // If the parameter count is invalid, we're screwed.
                for(size_t a=0; a<Params.size(); ++a)
                    Params[a].param->SynthesizeByteCode(synth);
                synth.AddOperation(Opcode, (unsigned) Params.size());
                synth.AddOperation(Funcno, 0, 0);
                break;
            }
            default:
            {
                // If the parameter count is invalid, we're screwed.
                for(size_t a=0; a<Params.size(); ++a)
                    Params[a].param->SynthesizeByteCode(synth);
                synth.AddOperation(Opcode, (unsigned) Params.size());
                break;
            }
        }
        synth.StackTopIs(Hash);
    }
}

namespace
{
    void PlanNtimesCache
        (long value,
         PowiCache& cache,
         int need_count,
         int recursioncount=0)
    {
        if(value < 1) return;

    #ifdef FP_GENERATING_POWI_TABLE
        if(recursioncount > 32) throw false;
    #endif

        if(cache.Plan_Add(value, need_count)) return;

        long half = 1;
        if(value < POWI_TABLE_SIZE)
            half = powi_table[value];
        else if(value & 1)
            half = value & ((1 << POWI_WINDOW_SIZE) - 1); // that is, value & 7
        else
            half = value / 2;

        long otherhalf = value-half;
        if(half > otherhalf || half<0) std::swap(half,otherhalf);

        FPO(fprintf(stderr, "value=%ld, half=%ld, otherhalf=%ld\n", value,half,otherhalf));

        if(half == otherhalf)
        {
            PlanNtimesCache(half,      cache, 2, recursioncount+1);
        }
        else
        {
            PlanNtimesCache(half,      cache, 1, recursioncount+1);
            PlanNtimesCache(otherhalf>0?otherhalf:-otherhalf,
                                       cache, 1, recursioncount+1);
        }

        cache.Plan_Has(value);
    }

    bool AssembleSequence(
        CodeTree& tree, long count,
        const SequenceOpCode& sequencing,
        CodeTree::ByteCodeSynth& synth,
        size_t max_bytecode_grow_length)
    {
        CodeTree::ByteCodeSynth backup = synth;
        const size_t bytecodesize_backup = synth.GetByteCodeSize();

        if(count == 0)
        {
            synth.PushImmed(sequencing.basevalue);
        }
        else
        {
            tree.SynthesizeByteCode(synth);

            if(count < 0)
            {
                synth.AddOperation(sequencing.op_flip, 1);
                count = -count;
            }

            if(count > 1)
            {
                /* To prevent calculating the same factors over and over again,
                 * we use a cache. */
                PowiCache cache;
                PlanNtimesCache(count, cache, 1);

                size_t stacktop_desired = synth.GetStackTop();

                cache.Start( synth.GetStackTop()-1 );

                FPO(fprintf(stderr, "Calculating result for %ld...\n", count));
                size_t res_stackpos = AssembleSequence_Subdivide(
                    count, cache, sequencing,
                    synth);

                size_t n_excess = synth.GetStackTop() - stacktop_desired;
                if(n_excess > 0 || res_stackpos != stacktop_desired-1)
                {
                    // Remove the cache values
                    synth.DoPopNMov(stacktop_desired-1, res_stackpos);
                }
            }
        }

        size_t bytecode_grow_amount = synth.GetByteCodeSize() - bytecodesize_backup;
        if(bytecode_grow_amount > max_bytecode_grow_length)
        {
            synth = backup;
            return false;
        }
        return true;
    }

    size_t AssembleSequence_Subdivide(
        long value,
        PowiCache& cache,
        const SequenceOpCode& sequencing,
        CodeTree::ByteCodeSynth& synth)
    {
        int cachepos = cache.Find(value);
        if(cachepos >= 0)
        {
            // found from the cache
            return cachepos;
        }

        long half = 1;
        if(value < POWI_TABLE_SIZE)
            half = powi_table[value];
        else if(value & 1)
            half = value & ((1 << POWI_WINDOW_SIZE) - 1); // that is, value & 7
        else
            half = value / 2;
        long otherhalf = value-half;
        if(half > otherhalf || half<0) std::swap(half,otherhalf);

        FPO(fprintf(stderr, "* I want %ld, my plan is %ld + %ld\n", value, half, value-half));

        if(half == otherhalf)
        {
            size_t half_pos = AssembleSequence_Subdivide(half, cache, sequencing, synth);

            // self-cumulate the subdivide result
            Subdivide_Combine(half_pos,half, half_pos,half, cache,
                sequencing.op_normal, sequencing.op_normal_flip,
                synth);
        }
        else
        {
            long part1 = half;
            long part2 = otherhalf>0?otherhalf:-otherhalf;

            size_t part1_pos = AssembleSequence_Subdivide(part1, cache, sequencing, synth);
            size_t part2_pos = AssembleSequence_Subdivide(part2, cache, sequencing, synth);

            FPO(fprintf(stderr, "Subdivide(%ld: %ld, %ld)\n", value, half, otherhalf));

            Subdivide_Combine(part1_pos,part1, part2_pos,part2, cache,
                otherhalf>0 ? sequencing.op_normal      : sequencing.op_inverse,
                otherhalf>0 ? sequencing.op_normal_flip : sequencing.op_inverse_flip,
                synth);
        }
        size_t stackpos = synth.GetStackTop()-1;
        cache.Remember(value, stackpos);
        cache.DumpContents();
        return stackpos;
    }

    void Subdivide_Combine(
        size_t apos, long aval,
        size_t bpos, long bval,
        PowiCache& cache,
        unsigned cumulation_opcode,
        unsigned cumulation_opcode_flip,
        CodeTree::ByteCodeSynth& synth)
    {
        /*FPO(fprintf(stderr, "== making result for (sp=%u, val=%d, needs=%d) and (sp=%u, val=%d, needs=%d), stacktop=%u\n",
            (unsigned)apos, aval, aval>=0 ? cache_needed[aval] : -1,
            (unsigned)bpos, bval, bval>=0 ? cache_needed[bval] : -1,
            (unsigned)synth.GetStackTop()));*/

        // Figure out whether we can trample a and b
        int a_needed = cache.UseGetNeeded(aval);
        int b_needed = cache.UseGetNeeded(bval);

        bool flipped = false;

        #define DUP_BOTH() do { \
            if(apos < bpos) { size_t tmp=apos; apos=bpos; bpos=tmp; flipped=!flipped; } \
            FPO(fprintf(stderr, "-> dup(%u) dup(%u) op\n", (unsigned)apos, (unsigned)bpos)); \
            synth.DoDup(apos); \
            synth.DoDup(apos==bpos ? synth.GetStackTop()-1 : bpos); } while(0)
        #define DUP_ONE(p) do { \
            FPO(fprintf(stderr, "-> dup(%u) op\n", (unsigned)p)); \
            synth.DoDup(p); \
        } while(0)

        if(a_needed > 0)
        {
            if(b_needed > 0)
            {
                // If they must both be preserved, make duplicates
                // First push the one that is at the larger stack
                // address. This increases the odds of possibly using cDup.
                DUP_BOTH();

                //SCENARIO 1:
                // Input:  x B A x x
                // Temp:   x B A x x A B
                // Output: x B A x x R
                //SCENARIO 2:
                // Input:  x A B x x
                // Temp:   x A B x x B A
                // Output: x A B x x R
            }
            else
            {
                // A must be preserved, but B can be trampled over

                // SCENARIO 1:
                //  Input:  x B x x A
                //   Temp:  x B x x A A B   (dup both, later first)
                //  Output: x B x x A R
                // SCENARIO 2:
                //  Input:  x A x x B
                //   Temp:  x A x x B A
                //  Output: x A x x R       -- only commutative cases
                // SCENARIO 3:
                //  Input:  x x x B A
                //   Temp:  x x x B A A B   (dup both, later first)
                //  Output: x x x B A R
                // SCENARIO 4:
                //  Input:  x x x A B
                //   Temp:  x x x A B A     -- only commutative cases
                //  Output: x x x A R
                // SCENARIO 5:
                //  Input:  x A B x x
                //   Temp:  x A B x x A B   (dup both, later first)
                //  Output: x A B x x R

                // if B is not at the top, dup both.
                if(bpos != synth.GetStackTop()-1)
                    DUP_BOTH();    // dup both
                else
                {
                    DUP_ONE(apos); // just dup A
                    flipped=!flipped;
                }
            }
        }
        else if(b_needed > 0)
        {
            // B must be preserved, but A can be trampled over
            // This is a mirror image of the a_needed>0 case, so I'll cut the chase
            if(apos != synth.GetStackTop()-1)
                DUP_BOTH();
            else
                DUP_ONE(bpos);
        }
        else
        {
            // Both can be trampled over.
            // SCENARIO 1:
            //  Input:  x B x x A
            //   Temp:  x B x x A B
            //  Output: x B x x R
            // SCENARIO 2:
            //  Input:  x A x x B
            //   Temp:  x A x x B A
            //  Output: x A x x R       -- only commutative cases
            // SCENARIO 3:
            //  Input:  x x x B A
            //  Output: x x x R         -- only commutative cases
            // SCENARIO 4:
            //  Input:  x x x A B
            //  Output: x x x R
            // SCENARIO 5:
            //  Input:  x A B x x
            //   Temp:  x A B x x A B   (dup both, later first)
            //  Output: x A B x x R
            // SCENARIO 6:
            //  Input:  x x x C
            //   Temp:  x x x C C   (c is both A and B)
            //  Output: x x x R

            if(apos == bpos && apos == synth.GetStackTop()-1)
                DUP_ONE(apos); // scenario 6
            else if(apos == synth.GetStackTop()-1 && bpos == synth.GetStackTop()-2)
            {
                FPO(fprintf(stderr, "-> op\n")); // scenario 3
                flipped=!flipped;
            }
            else if(apos == synth.GetStackTop()-2 && bpos == synth.GetStackTop()-1)
                FPO(fprintf(stderr, "-> op\n")); // scenario 4
            else if(apos == synth.GetStackTop()-1)
                DUP_ONE(bpos); // scenario 1
            else if(bpos == synth.GetStackTop()-1)
            {
                DUP_ONE(apos); // scenario 2
                flipped=!flipped;
            }
            else
                DUP_BOTH(); // scenario 5
        }
        // Add them together.
        synth.AddOperation(flipped ? cumulation_opcode_flip : cumulation_opcode, 2);
    }
}

#endif
#include <cmath>
#include <cassert>

#include "fptypes.h"

#include "fparser.h"


#ifdef FP_SUPPORT_OPTIMIZER

using namespace FUNCTIONPARSERTYPES;
//using namespace FPoptimizer_Grammar;


namespace FPoptimizer_CodeTree
{
    class CodeTreeParserData
    {
    private:
        std::vector<CodeTreeP> stack;
    public:
        CodeTreeParserData() : stack() { }

        void Eat(unsigned nparams, OPCODE opcode)
        {
            CodeTreeP newnode = new CodeTree;
            newnode->Opcode = opcode;
            size_t stackhead = stack.size() - nparams;
            for(unsigned a=0; a<nparams; ++a)
            {
                CodeTree::Param param;
                param.param = stack[stackhead + a];
                param.sign  = false;
                newnode->AddParam(param);
            }
            stack.resize(stackhead);
            stack.push_back(newnode);
        }

        void EatFunc(unsigned params, OPCODE opcode, unsigned funcno)
        {
            Eat(params, opcode);
            stack.back()->Funcno = funcno;
        }

        void AddConst(double value)
        {
            CodeTreeP newnode = new CodeTree;
            newnode->Opcode = cImmed;
            newnode->Value  = value;
            stack.push_back(newnode);
        }

        void AddVar(unsigned varno)
        {
            CodeTreeP newnode = new CodeTree;
            newnode->Opcode = cVar;
            newnode->Var    = varno;
            stack.push_back(newnode);
        }

        void SetLastOpParamSign(unsigned paramno)
        {
            stack.back()->Params[paramno].sign = true;
        }

        void SwapLastTwoInStack()
        {
            std::swap(stack[stack.size()-1],
                      stack[stack.size()-2]);
        }

        void Dup()
        {
            Fetch(stack.size()-1);
        }

        void Fetch(size_t which)
        {
            stack.push_back(stack[which]->Clone());
        }

        void PopNMov(size_t target, size_t source)
        {
            stack[target] = stack[source];
            stack.resize(target+1);
        }

        CodeTreeP PullResult()
        {
            CodeTreeP result = stack.back();
            stack.resize(stack.size()-1);
            result->Rehash(false);
            result->Sort_Recursive();
            return result;
        }

        void CheckConst()
        {
            // Check if the last token on stack can be optimized with constant math
            CodeTreeP result = stack.back();
            result->ConstantFolding();
        }
    private:
        CodeTreeParserData(const CodeTreeParserData&);
        CodeTreeParserData& operator=(const CodeTreeParserData&);
    };

    CodeTreeP CodeTree::GenerateFrom(
        const std::vector<unsigned>& ByteCode,
        const std::vector<double>& Immed,
        const FunctionParser::Data& fpdata)
    {
        CodeTreeParserData data;
        std::vector<size_t> labels;

        for(size_t IP=0, DP=0; ; ++IP)
        {
            while(!labels.empty() && labels.back() == IP)
            {
                // The "else" of an "if" ends here
                data.Eat(3, cIf);
                labels.erase(labels.end()-1);
            }
            if(IP >= ByteCode.size()) break;

            unsigned opcode = ByteCode[IP];
            if(OPCODE(opcode) >= VarBegin)
            {
                data.AddVar(opcode);
            }
            else
            {
                switch(opcode)
                {
                    // Specials
                    case cIf:
                        IP += 2;
                        continue;
                    case cJump:
                        labels.push_back(ByteCode[IP+1]+1);
                        IP += 2;
                        continue;
                    case cImmed:
                        data.AddConst(Immed[DP++]);
                        break;
                    case cDup:
                        data.Dup();
                        break;
                    case cNop:
                        break;
                    case cFCall:
                    {
                        unsigned funcno = ByteCode[++IP];
                        unsigned params = fpdata.FuncPtrs[funcno].params;
                        data.EatFunc(params, OPCODE(opcode), funcno);
                        break;
                    }
                    case cPCall:
                    {
                        unsigned funcno = ByteCode[++IP];
                        unsigned params = fpdata.FuncParsers[funcno].params;
                        data.EatFunc(params, OPCODE(opcode), funcno);
                        break;
                    }
                    // Unary operators requiring special attention
                    case cInv: // from fpoptimizer
                        data.Eat(1, cMul); // Unary division is inverse multiplying
                        data.SetLastOpParamSign(0);
                        break;
                    case cNeg:
                        data.Eat(1, cAdd); // Unary minus is negative adding.
                        data.SetLastOpParamSign(0);
                        break;
                    case cSqr: // from fpoptimizer
                        data.Dup();
                        data.Eat(2, cMul);
                        break;
                    // Unary functions requiring special attention
                    case cDeg:
                        data.AddConst(CONSTANT_DR);
                        data.Eat(2, cMul);
                        break;
                    case cRad:
                        data.AddConst(CONSTANT_RD);
                        data.Eat(2, cMul);
                        break;
                    case cExp:
                        data.AddConst(CONSTANT_E);
                        data.SwapLastTwoInStack();
                        data.Eat(2, cPow);
                        break;
                    case cSqrt:
                        data.AddConst(0.5);
                        data.Eat(2, cPow);
                        break;
                    case cCot:
                        data.Eat(1, cTan);
                        data.Eat(1, cMul);
                        data.SetLastOpParamSign(0);
                        break;
                    case cCsc:
                        data.Eat(1, cSin);
                        data.Eat(1, cMul);
                        data.SetLastOpParamSign(0);
                        break;
                    case cSec:
                        data.Eat(1, cCos);
                        data.Eat(1, cMul);
                        data.SetLastOpParamSign(0);
                        break;
                    case cLog10:
                        data.Eat(1, cLog);
                        data.AddConst(CONSTANT_L10I);
                        data.Eat(2, cMul);
                        break;
                    case cLog2:
                        data.Eat(1, cLog);
                        data.AddConst(CONSTANT_L2I);
                        data.Eat(2, cMul);
                        break;
                    // Binary operators requiring special attention
                    case cSub:
                        data.Eat(2, cAdd); // Minus is negative adding
                        data.SetLastOpParamSign(1);
                        break;
                    case cRSub: // from fpoptimizer
                        data.Eat(2, cAdd);
                        data.SetLastOpParamSign(0); // negate param0 instead of param1
                        break;
                    case cDiv:
                        data.Eat(2, cMul); // Divide is inverse multiply
                        data.SetLastOpParamSign(1);
                        break;
                    case cRDiv: // from fpoptimizer
                        data.Eat(2, cMul);
                        data.SetLastOpParamSign(0); // invert param0 instead of param1
                        break;
                    case cRSqrt: // from fpoptimizer
                        data.AddConst(-0.5);
                        data.Eat(2, cPow);
                        break;
                    // Binary operators not requiring special attention
                    case cAdd: case cMul:
                    case cMod: case cPow:
                    case cEqual: case cLess: case cGreater:
                    case cNEqual: case cLessOrEq: case cGreaterOrEq:
                    case cAnd: case cOr:
                        data.Eat(2, OPCODE(opcode));
                        break;
                    // Unary operators not requiring special attention
                    case cNot:
                    case cNotNot: // from fpoptimizer
                        data.Eat(1, OPCODE(opcode));
                        break;
                    // Special opcodes generated by fpoptimizer itself
                    case cFetch:
                        data.Fetch(ByteCode[++IP]);
                        break;
                    case cPopNMov:
                    {
                        unsigned stackOffs_target = ByteCode[++IP];
                        unsigned stackOffs_source = ByteCode[++IP];
                        data.PopNMov(stackOffs_target, stackOffs_source);
                        break;
                    }
                    // Note: cVar should never be encountered in bytecode.
                    // Other functions
#ifndef FP_DISABLE_EVAL
                    case cEval:
                    {
                        unsigned paramcount = fpdata.variableRefs.size();
                        data.Eat(paramcount, OPCODE(opcode));
                        break;
                    }
#endif
                    default:
                        unsigned funcno = opcode-cAbs;
                        assert(funcno < FUNC_AMOUNT);
                        const FuncDefinition& func = Functions[funcno];
                        data.Eat(func.params, OPCODE(opcode));
                        break;
                }
            }
            data.CheckConst();
        }
        return data.PullResult();
    }
}

#endif

#endif
