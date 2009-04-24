//=============================================
// Function parser v3.1.2 optimizer by Bisqwit
//=============================================

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
        long GetLongIntegerImmed() const { return (long)GetImmed(); }
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

    /***/

    struct MatchedParams
    {
        ParamMatchingType type    : 6;
        SignBalanceType   balance : 2;
        // count,index to plist[]
        unsigned         count : 8;
        unsigned         index : 16;

        struct CodeTreeMatch;

        bool Match(FPoptimizer_CodeTree::CodeTree& tree,
                   CodeTreeMatch& match,
                   bool recursion = true) const;

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

        bool Match(
            FPoptimizer_CodeTree::CodeTree& tree,
            MatchedParams::CodeTreeMatch& match,
            TransformationType transf) const;

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

        bool Match(FPoptimizer_CodeTree::CodeTree& tree,
                   MatchedParams::CodeTreeMatch& match) const;
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
        0.017453292519943295474371680597869271878153085708618, /* 8 */
        57.29577951308232286464772187173366546630859375, /* 9 */
        0.4342944819032517611567811854911269620060920715332, /* 10 */
        1.4426950408889633870046509400708600878715515136719, /* 11 */
        0.69314718055994528622676398299518041312694549560547, /* 12 */
        2.3025850929940459010936137929093092679977416992188, /* 13 */
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
        {cAcosh      , false, None  , 1, false, 1,	5	}, /* 7    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 8    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 9    	*/
        {cAsin       , false, None  , 1, false, 1,	9	}, /* 10    	*/
        {cAsinh      , false, None  , 1, false, 1,	8	}, /* 11    	*/
        {cAtan       , false, None  , 1, false, 1,	5	}, /* 12    	*/
        {cAtanh      , false, None  , 1, false, 1,	8	}, /* 13    	*/
        {cCeil       , false, None  , 1, false, 1,	8	}, /* 14    	*/
        {cCos        , false, None  , 1, false, 1,	5	}, /* 15    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 16 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	2	}, /* 17    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 18 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	3	}, /* 19    	*/
        {NumConstant , false, None  , 1, false, 0,	0	}, /* 20    	*/
        {NumConstant , false, None  , 1, false, 0,	1	}, /* 21    	*/
        {cMul        , false, None  , 1, false, 2,	20	}, /* 22    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 23 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	4	}, /* 24    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 25    	*/
        {cCosh       , false, None  , 1, false, 1,	4	}, /* 26    	*/
        {cFloor      , false, None  , 1, false, 1,	8	}, /* 27    	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 28    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 29 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 30 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 31 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 32    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 33 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 34 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 35 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 36 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 37 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 38 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 39    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 40    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 41 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 42 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	6	}, /* 43    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 44    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 45    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 46 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 47    	*/
        {SubFunction , false, None  , 1, false, 0,	7	}, /* 48    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 49 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	8	}, /* 50    	*/
        {SubFunction , false, None  , 1, false, 0,	9	}, /* 51    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 52 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 53    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 54    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 55 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 56 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	10	}, /* 57    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 58    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 59    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 60 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 61    	*/
        {SubFunction , false, None  , 1, false, 0,	11	}, /* 62    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 63 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	12	}, /* 64    	*/
        {SubFunction , false, None  , 1, false, 0,	13	}, /* 65    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 66 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 67    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 68    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 69 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 70 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	14	}, /* 71    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 72    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 73    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 74 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 75    	*/
        {SubFunction , false, None  , 1, false, 0,	15	}, /* 76    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 77 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	16	}, /* 78    	*/
        {SubFunction , false, None  , 1, false, 0,	17	}, /* 79    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 80 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 81    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 82    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 83 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 84 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	18	}, /* 85    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 86    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 87    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 88 "y"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 89    	*/
        {SubFunction , false, None  , 1, false, 0,	19	}, /* 90    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 91 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	20	}, /* 92    	*/
        {SubFunction , false, None  , 1, false, 0,	21	}, /* 93    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 94 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 95    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 96    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 97 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 98    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 99    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 100 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	22	}, /* 101    	*/
        {SubFunction , false, None  , 1, false, 0,	23	}, /* 102    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 103    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 104    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 105    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 106    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 107 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	24	}, /* 108    	*/
        {SubFunction , false, None  , 1, false, 0,	25	}, /* 109    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 110 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	26	}, /* 111    	*/
        {SubFunction , false, None  , 1, false, 0,	27	}, /* 112    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 113 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 114    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 115    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 116 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 117    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 118    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 119 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	28	}, /* 120    	*/
        {SubFunction , false, None  , 1, false, 0,	29	}, /* 121    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 122 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	30	}, /* 123    	*/
        {SubFunction , false, None  , 1, false, 0,	31	}, /* 124    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 125 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	32	}, /* 126    	*/
        {SubFunction , false, None  , 1, false, 0,	33	}, /* 127    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 128 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 129    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 130    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 131 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 132    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 133    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 134 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	34	}, /* 135    	*/
        {SubFunction , false, None  , 1, false, 0,	35	}, /* 136    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 137 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	36	}, /* 138    	*/
        {SubFunction , false, None  , 1, false, 0,	37	}, /* 139    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 140 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	38	}, /* 141    	*/
        {SubFunction , false, None  , 1, false, 0,	39	}, /* 142    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 143 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 144    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 145    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 146 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 147    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 148    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 149 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	40	}, /* 150    	*/
        {SubFunction , false, None  , 1, false, 0,	41	}, /* 151    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 152    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 153    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 154    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 155    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 156 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	42	}, /* 157    	*/
        {SubFunction , false, None  , 1, false, 0,	43	}, /* 158    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 159 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	44	}, /* 160    	*/
        {SubFunction , false, None  , 1, false, 0,	45	}, /* 161    	*/
        {SubFunction , false, None  , 1, false, 0,	46	}, /* 162    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 163 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 164 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 165 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 166 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 167 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	47	}, /* 168    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 169 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	48	}, /* 170    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 171 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 172 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 173 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 174 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 175 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	49	}, /* 176    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 177    	*/
        {cLog        , false, None  , 1, false, 1,	5	}, /* 178    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 179 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 180 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	50	}, /* 181    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 182 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	51	}, /* 183    	*/
        {SubFunction , false, None  , 1, false, 0,	52	}, /* 184    	*/
        {NumConstant , false, None  , 1, false, 0,	4	}, /* 185    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 186 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	53	}, /* 187    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 188    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 189    	*/
        {SubFunction , false, None  , 1, false, 0,	54	}, /* 190    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 191    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 192    	*/
        {SubFunction , false, None  , 1, false, 0,	55	}, /* 193    	*/
        {SubFunction , false, None  , 1, false, 0,	56	}, /* 194    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 195 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	57	}, /* 196    	*/
        {SubFunction , false, None  , 1, false, 0,	58	}, /* 197    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 198    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 199    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 200    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 201    	*/
        {cMax        , false, None  , 1, false, 2,	200	}, /* 202    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 203 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 204 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	59	}, /* 205    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 206    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 207    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 208    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 209    	*/
        {cMin        , false, None  , 1, false, 2,	208	}, /* 210    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 211 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 212 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 213    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 214 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 215    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 216 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	4	}, /* 217    	*/
        {SubFunction , false, None  , 1, false, 0,	60	}, /* 218    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 219    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 220    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 221    	*/
        {NumConstant , false, None  , 1, false, 0,	4	}, /* 222    	*/
        {SubFunction , false, None  , 1, false, 0,	62	}, /* 223    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 224    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 225    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 226 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	63	}, /* 227    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 228    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 229    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 230    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 231    	*/
        {cPow        , false, None  , 1, false, 2,	230	}, /* 232    	*/
        {cLog        , false, Invert, 1, false, 1,	4	}, /* 233    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 234    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 235    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 236    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 237    	*/
        {SubFunction , false, None  , 1, false, 0,	64	}, /* 238    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 239    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 240    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 241 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	65	}, /* 242    	*/
        {cLog        , true , None  , 1, false, 1,	5	}, /* 243    	*/
        {SubFunction , false, None  , 1, false, 0,	60	}, /* 244    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 245    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 246    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 247    	*/
        {SubFunction , false, None  , 1, false, 0,	66	}, /* 248    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 249    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 250    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 251 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	67	}, /* 252    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 253 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	5	}, /* 254    	*/
        {SubFunction , false, None  , 1, false, 0,	68	}, /* 255    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 256 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 257    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 258 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 259    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 260    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 261    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 262 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	69	}, /* 263    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 264    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 265    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 266 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	70	}, /* 267    	*/
        {SubFunction , true , None  , 1, false, 0,	71	}, /* 268    	*/
        {SubFunction , false, None  , 1, false, 0,	72	}, /* 269    	*/
        {SubFunction , true , None  , 1, false, 0,	73	}, /* 270    	*/
        {SubFunction , false, None  , 1, false, 0,	74	}, /* 271    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 272    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 273    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 274 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	75	}, /* 275    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 276    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 277    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 278 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	76	}, /* 279    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 280 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 281 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	77	}, /* 282    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 283 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 284 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 285 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 286 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	78	}, /* 287    	*/
        {cSin        , false, None  , 1, false, 1,	8	}, /* 288    	*/
        {SubFunction , false, None  , 1, false, 0,	79	}, /* 289    	*/
        {SubFunction , true , None  , 1, false, 0,	80	}, /* 290    	*/
        {SubFunction , false, None  , 1, false, 0,	81	}, /* 291    	*/
        {NumConstant , false, None  , 1, false, 0,	0	}, /* 292    	*/
        {NumConstant , false, None  , 1, false, 0,	1	}, /* 293    	*/
        {cMul        , false, None  , 1, false, 2,	292	}, /* 294    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 295 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	82	}, /* 296    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 297    	*/
        {cSinh       , false, None  , 1, false, 1,	5	}, /* 298    	*/
        {cTan        , false, None  , 1, false, 1,	8	}, /* 299    	*/
        {SubFunction , false, None  , 1, false, 0,	84	}, /* 300    	*/
        {SubFunction , true , None  , 1, false, 0,	85	}, /* 301    	*/
        {SubFunction , false, None  , 1, false, 0,	86	}, /* 302    	*/
        {cTanh       , false, None  , 1, false, 1,	4	}, /* 303    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 304    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 305    	*/
        {SubFunction , false, None  , 1, false, 0,	87	}, /* 306    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 307    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 308    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 309    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 310    	*/
        {SubFunction , false, None  , 1, false, 0,	88	}, /* 311    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 312    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 313    	*/
        {SubFunction , false, None  , 1, false, 0,	89	}, /* 314    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 315    	*/
        {RestHolder  , false, None  , 1, false, 0,	4	}, /* 316    	*/
        {SubFunction , false, None  , 1, false, 0,	90	}, /* 317    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 318    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 319    	*/
        {SubFunction , true , None  , 1, false, 0,	91	}, /* 320    	*/
        {ImmedHolder , true , None  , 1, false, 0,	0	}, /* 321    	*/
        {ImmedHolder , false, Negate, 1, false, 0,	0	}, /* 322    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 323    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 324    	*/
        {SubFunction , true , None  , 1, false, 0,	92	}, /* 325    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 326    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 327    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 328    	*/
        {SubFunction , true , None  , 1, false, 0,	93	}, /* 329    	*/
        {ImmedHolder , false, Negate, 1, false, 0,	0	}, /* 330    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 331    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 332    	*/
        {SubFunction , false, None  , 1, false, 0,	94	}, /* 333    	*/
        {RestHolder  , false, None  , 1, false, 0,	3	}, /* 334    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 335    	*/
        {SubFunction , false, None  , 1, false, 0,	95	}, /* 336    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 337    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 338    	*/
        {SubFunction , true , None  , 1, false, 0,	96	}, /* 339    	*/
        {SubFunction , false, None  , 1, false, 0,	90	}, /* 340    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 341    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 342    	*/
        {SubFunction , false, None  , 1, false, 0,	97	}, /* 343    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 344    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 345    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 346    	*/
        {SubFunction , true , None  , 1, false, 0,	98	}, /* 347    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 348    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 349    	*/
        {SubFunction , false, None  , 1, false, 0,	99	}, /* 350    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 351    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 352    	*/
        {NumConstant , false, None  , 1, false, 0,	3	}, /* 353    	*/
        {SubFunction , true , None  , 1, false, 0,	100	}, /* 354    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 355    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 356    	*/
        {SubFunction , false, None  , 1, false, 0,	101	}, /* 357    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 358    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 359    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 360    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 361    	*/
        {cAdd        , false, None  , 1, false, 2,	360	}, /* 362    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 363 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 364 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	60	}, /* 365    	*/
        {SubFunction , false, None  , 1, false, 0,	102	}, /* 366    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 367 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 368 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	103	}, /* 369    	*/
        {SubFunction , false, None  , 1, false, 0,	104	}, /* 370    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 371    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 372    	*/
        {SubFunction , false, None  , 1, false, 0,	106	}, /* 373    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 374    	*/
        {SubFunction , false, None  , 1, false, 0,	105	}, /* 375    	*/
        {SubFunction , false, None  , 1, false, 0,	107	}, /* 376    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 377 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 378    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 379    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 380 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 381    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 382    	*/
        {SubFunction , false, None  , 1, false, 0,	108	}, /* 383    	*/
        {SubFunction , false, None  , 1, false, 0,	109	}, /* 384    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 385    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 386    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 387    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 388    	*/
        {SubFunction , false, None  , 1, false, 0,	110	}, /* 389    	*/
        {SubFunction , false, None  , 1, false, 0,	111	}, /* 390    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 391 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	112	}, /* 392    	*/
        {SubFunction , false, None  , 1, false, 0,	113	}, /* 393    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 394 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 395    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 396    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 397 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 398    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 399    	*/
        {SubFunction , false, None  , 1, false, 0,	114	}, /* 400    	*/
        {SubFunction , true , None  , 1, false, 0,	115	}, /* 401    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 402    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 403    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 404    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 405    	*/
        {SubFunction , false, None  , 1, false, 0,	116	}, /* 406    	*/
        {SubFunction , true , None  , 1, false, 0,	117	}, /* 407    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 408 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	118	}, /* 409    	*/
        {SubFunction , false, None  , 1, false, 0,	119	}, /* 410    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 411 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 412    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 413    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 414 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 415    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 416    	*/
        {SubFunction , false, None  , 1, false, 0,	120	}, /* 417    	*/
        {SubFunction , false, None  , 1, false, 0,	121	}, /* 418    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 419    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 420    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 421    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 422    	*/
        {SubFunction , false, None  , 1, false, 0,	122	}, /* 423    	*/
        {SubFunction , false, None  , 1, false, 0,	123	}, /* 424    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 425 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	124	}, /* 426    	*/
        {SubFunction , false, None  , 1, false, 0,	125	}, /* 427    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 428 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 429    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 430    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 431 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 432    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 433    	*/
        {SubFunction , false, None  , 1, false, 0,	126	}, /* 434    	*/
        {SubFunction , true , None  , 1, false, 0,	127	}, /* 435    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 436    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 437    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 438    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 439    	*/
        {SubFunction , false, None  , 1, false, 0,	128	}, /* 440    	*/
        {SubFunction , true , None  , 1, false, 0,	129	}, /* 441    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 442 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	130	}, /* 443    	*/
        {SubFunction , false, None  , 1, false, 0,	131	}, /* 444    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 445    	*/
        {SubFunction , false, None  , 1, false, 0,	132	}, /* 446    	*/
        {SubFunction , false, None  , 1, false, 0,	5	}, /* 447    	*/
        {SubFunction , false, None  , 1, false, 0,	134	}, /* 448    	*/
        {SubFunction , false, None  , 1, false, 0,	133	}, /* 449    	*/
        {SubFunction , false, None  , 1, false, 0,	135	}, /* 450    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 451 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 452 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	136	}, /* 453    	*/
        {SubFunction , false, None  , 1, false, 0,	137	}, /* 454    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 455    	*/
        {SubFunction , false, None  , 1, false, 0,	132	}, /* 456    	*/
        {SubFunction , false, None  , 1, false, 0,	139	}, /* 457    	*/
        {SubFunction , false, None  , 1, false, 0,	140	}, /* 458    	*/
        {SubFunction , false, None  , 1, false, 0,	138	}, /* 459    	*/
        {SubFunction , true , None  , 1, false, 0,	141	}, /* 460    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 461 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 462 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	142	}, /* 463    	*/
        {SubFunction , false, None  , 1, false, 0,	143	}, /* 464    	*/
        {SubFunction , false, None  , 1, false, 0,	139	}, /* 465    	*/
        {SubFunction , false, None  , 1, false, 0,	144	}, /* 466    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 467    	*/
        {SubFunction , false, None  , 1, false, 0,	146	}, /* 468    	*/
        {SubFunction , false, None  , 1, false, 0,	145	}, /* 469    	*/
        {SubFunction , false, None  , 1, false, 0,	147	}, /* 470    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 471 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 472 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	148	}, /* 473    	*/
        {SubFunction , false, None  , 1, false, 0,	149	}, /* 474    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 475    	*/
        {SubFunction , false, None  , 1, false, 0,	132	}, /* 476    	*/
        {SubFunction , false, None  , 1, false, 0,	151	}, /* 477    	*/
        {SubFunction , false, None  , 1, false, 0,	134	}, /* 478    	*/
        {SubFunction , false, None  , 1, false, 0,	150	}, /* 479    	*/
        {SubFunction , true , None  , 1, false, 0,	152	}, /* 480    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 481 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 482 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	153	}, /* 483    	*/
        {SubFunction , false, None  , 1, false, 0,	154	}, /* 484    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 485 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 486    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 487    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 488 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	155	}, /* 489    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 490    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 491    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 492 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	156	}, /* 493    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 494 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	157	}, /* 495    	*/
        {SubFunction , false, None  , 1, false, 0,	158	}, /* 496    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 497 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 498    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 499    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 500 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	159	}, /* 501    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 502    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 503    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 504 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	160	}, /* 505    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 506 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	161	}, /* 507    	*/
        {SubFunction , false, None  , 1, false, 0,	162	}, /* 508    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 509 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 510    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 511    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 512 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 513    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 514    	*/
        {SubFunction , true , None  , 1, false, 0,	163	}, /* 515    	*/
        {SubFunction , true , None  , 1, false, 0,	164	}, /* 516    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 517    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 518    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 519    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 520    	*/
        {SubFunction , true , None  , 1, false, 0,	165	}, /* 521    	*/
        {SubFunction , true , None  , 1, false, 0,	166	}, /* 522    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 523 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	167	}, /* 524    	*/
        {SubFunction , false, None  , 1, false, 0,	168	}, /* 525    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 526 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 527    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 528    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 529 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 530    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 531    	*/
        {SubFunction , true , None  , 1, false, 0,	169	}, /* 532    	*/
        {SubFunction , true , None  , 1, false, 0,	170	}, /* 533    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 534    	*/
        {RestHolder  , true , None  , 1, false, 0,	3	}, /* 535    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 536    	*/
        {RestHolder  , true , None  , 1, false, 0,	4	}, /* 537    	*/
        {SubFunction , true , None  , 1, false, 0,	171	}, /* 538    	*/
        {SubFunction , true , None  , 1, false, 0,	172	}, /* 539    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 540 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	173	}, /* 541    	*/
        {SubFunction , false, None  , 1, false, 0,	174	}, /* 542    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 543    	*/
        {SubFunction , false, None  , 1, false, 0,	132	}, /* 544    	*/
        {SubFunction , true , None  , 1, false, 0,	175	}, /* 545    	*/
        {SubFunction , false, None  , 1, false, 0,	141	}, /* 546    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 547 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 548 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	176	}, /* 549    	*/
        {SubFunction , true , None  , 1, false, 0,	177	}, /* 550    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 551    	*/
        {SubFunction , false, None  , 1, false, 0,	132	}, /* 552    	*/
        {SubFunction , true , None  , 1, false, 0,	178	}, /* 553    	*/
        {SubFunction , true , None  , 1, false, 0,	135	}, /* 554    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 555 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 556 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	179	}, /* 557    	*/
        {SubFunction , true , None  , 1, false, 0,	180	}, /* 558    	*/
        {SubFunction , false, None  , 1, false, 0,	139	}, /* 559    	*/
        {SubFunction , false, None  , 1, false, 0,	144	}, /* 560    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 561    	*/
        {SubFunction , false, None  , 1, false, 0,	146	}, /* 562    	*/
        {SubFunction , true , None  , 1, false, 0,	181	}, /* 563    	*/
        {SubFunction , true , None  , 1, false, 0,	182	}, /* 564    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 565 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 566 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	183	}, /* 567    	*/
        {SubFunction , true , None  , 1, false, 0,	184	}, /* 568    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 569 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 570    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 571    	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 572 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	185	}, /* 573    	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 574 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	186	}, /* 575    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 576 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	187	}, /* 577    	*/
        {SubFunction , false, None  , 1, false, 0,	188	}, /* 578    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 579 "x"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 580    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 581    	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 582 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	189	}, /* 583    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 584    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 585    	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 586 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	190	}, /* 587    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 588 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	191	}, /* 589    	*/
        {SubFunction , false, None  , 1, false, 0,	192	}, /* 590    	*/
        {NamedHolder , false, None  , 2, true , 0,	0	}, /* 591 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 592 "x"	*/
        {NamedHolder , false, None  , 2, true , 0,	0	}, /* 593 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	193	}, /* 594    	*/
        {NamedHolder , true , None  , 2, true , 0,	0	}, /* 595 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 596 "x"	*/
        {NamedHolder , false, None  , 2, true , 0,	0	}, /* 597 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	194	}, /* 598    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 599 "a"	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 600    	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 601 "b"	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 602    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 603 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 604 "b"	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 605    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 606    	*/
        {SubFunction , false, None  , 1, false, 0,	195	}, /* 607    	*/
        {SubFunction , false, None  , 1, false, 0,	196	}, /* 608    	*/
        {SubFunction , false, None  , 1, false, 0,	197	}, /* 609    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 610 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 611 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	198	}, /* 612    	*/
        {NumConstant , false, None  , 1, false, 0,	6	}, /* 613    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 614    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 615    	*/
        {SubFunction , false, None  , 1, false, 0,	200	}, /* 616    	*/
        {NumConstant , false, None  , 1, false, 0,	7	}, /* 617    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 618 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 619 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	201	}, /* 620    	*/
        {SubFunction , false, None  , 1, false, 0,	199	}, /* 621    	*/
        {SubFunction , false, None  , 1, false, 0,	202	}, /* 622    	*/
        {NumConstant , false, None  , 1, false, 0,	0	}, /* 623    	*/
        {NumConstant , false, None  , 1, false, 0,	1	}, /* 624    	*/
        {cMul        , false, None  , 1, false, 2,	623	}, /* 625    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 626 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	203	}, /* 627    	*/
        {SubFunction , true , None  , 1, false, 0,	204	}, /* 628    	*/
        {SubFunction , false, None  , 1, false, 0,	205	}, /* 629    	*/
        {NumConstant , false, None  , 1, false, 0,	5	}, /* 630    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 631    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 632    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 633    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 634    	*/
        {SubFunction , true , None  , 1, false, 0,	206	}, /* 635    	*/
        {SubFunction , false, None  , 1, false, 0,	207	}, /* 636    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 637    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 638    	*/
        {SubFunction , false, None  , 1, false, 0,	208	}, /* 639    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 640    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 641    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 642 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 643    	*/
        {SubFunction , true , None  , 1, false, 0,	209	}, /* 644    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 645 "x"	*/
        {ImmedHolder , false, Negate, 1, false, 0,	0	}, /* 646    	*/
        {SubFunction , false, None  , 1, false, 0,	210	}, /* 647    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 648    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 649    	*/
        {SubFunction , true , None  , 1, false, 0,	211	}, /* 650    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 651    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 652    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 653    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 654    	*/
        {cMul        , false, None  , 1, false, 2,	653	}, /* 655    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 656    	*/
        {ImmedHolder , true , None  , 1, false, 0,	1	}, /* 657    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 658    	*/
        {ImmedHolder , false, Invert, 1, false, 0,	1	}, /* 659    	*/
        {cMul        , false, None  , 1, false, 2,	658	}, /* 660    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 661 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 662 "x"	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 663    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 664 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	212	}, /* 665    	*/
        {SubFunction , false, None  , 1, false, 0,	213	}, /* 666    	*/
        {cLog        , false, Invert, 1, false, 1,	4	}, /* 667    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 668    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 669 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	214	}, /* 670    	*/
        {SubFunction , false, None  , 1, false, 0,	215	}, /* 671    	*/
        {cLog        , true , None  , 1, false, 1,	5	}, /* 672    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 673 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 674 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	216	}, /* 675    	*/
        {SubFunction , false, None  , 1, false, 0,	217	}, /* 676    	*/
        {SubFunction , true , None  , 1, false, 0,	61	}, /* 677    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 678    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 679 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 680 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	218	}, /* 681    	*/
        {SubFunction , false, None  , 1, false, 0,	219	}, /* 682    	*/
        {SubFunction , false, None  , 1, false, 0,	220	}, /* 683    	*/
        {cLog        , false, Invert, 1, false, 1,	5	}, /* 684    	*/
        {SubFunction , false, None  , 1, false, 0,	74	}, /* 685    	*/
        {cLog        , false, Invert, 1, false, 1,	8	}, /* 686    	*/
        {SubFunction , false, None  , 1, false, 0,	221	}, /* 687    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 688 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	222	}, /* 689    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 690    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 691 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 692 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	223	}, /* 693    	*/
        {SubFunction , false, None  , 1, false, 0,	224	}, /* 694    	*/
        {SubFunction , false, None  , 1, false, 0,	225	}, /* 695    	*/
        {cLog        , true , None  , 1, false, 1,	5	}, /* 696    	*/
        {SubFunction , false, None  , 1, false, 0,	221	}, /* 697    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 698 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	226	}, /* 699    	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 700 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 701 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 702 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	227	}, /* 703    	*/
        {SubFunction , false, None  , 1, false, 0,	228	}, /* 704    	*/
        {SubFunction , false, None  , 1, false, 0,	229	}, /* 705    	*/
        {SubFunction , true , None  , 1, false, 0,	230	}, /* 706    	*/
        {SubFunction , false, None  , 1, false, 0,	60	}, /* 707    	*/
        {SubFunction , true , None  , 1, false, 0,	231	}, /* 708    	*/
        {SubFunction , false, None  , 1, false, 0,	232	}, /* 709    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 710 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	233	}, /* 711    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 712 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 713 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 714 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 715 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	234	}, /* 716    	*/
        {SubFunction , false, None  , 1, false, 0,	235	}, /* 717    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 718 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 719 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 720 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	236	}, /* 721    	*/
        {SubFunction , false, None  , 1, false, 0,	237	}, /* 722    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 723 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 724 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 725 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 726 "z"	*/
        {SubFunction , false, None  , 1, false, 0,	238	}, /* 727    	*/
        {SubFunction , true , None  , 1, false, 0,	239	}, /* 728    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 729 "y"	*/
        {NamedHolder , true , None  , 1, false, 0,	2	}, /* 730 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 731 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	240	}, /* 732    	*/
        {SubFunction , false, None  , 1, false, 0,	241	}, /* 733    	*/
        {SubFunction , false, None  , 1, false, 0,	80	}, /* 734    	*/
        {SubFunction , true , None  , 1, false, 0,	83	}, /* 735    	*/
        {SubFunction , false, None  , 1, false, 0,	242	}, /* 736    	*/
        {SubFunction , false, None  , 1, false, 0,	243	}, /* 737    	*/
        {SubFunction , true , None  , 1, false, 0,	244	}, /* 738    	*/
        {SubFunction , false, None  , 1, false, 0,	245	}, /* 739    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 740 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 741 "y"	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 742 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	246	}, /* 743    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 744 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 745 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 746 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	247	}, /* 747    	*/
        {SubFunction , false, None  , 1, false, 0,	248	}, /* 748    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 749 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 750 "y"	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 751 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	249	}, /* 752    	*/
        {NamedHolder , false, None  , 1, true , 0,	0	}, /* 753 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 754 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 755 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	250	}, /* 756    	*/
        {SubFunction , false, None  , 1, false, 0,	251	}, /* 757    	*/
        {ImmedHolder , true , None  , 1, false, 0,	0	}, /* 758    	*/
        {ImmedHolder , true , None  , 1, false, 0,	1	}, /* 759    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 760    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 761    	*/
        {cMul        , true , None  , 1, false, 2,	760	}, /* 762    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 763    	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 764 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	252	}, /* 765    	*/
        {SubFunction , true , None  , 1, false, 0,	253	}, /* 766    	*/
        {cLog        , false, None  , 1, false, 1,	5	}, /* 767    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 768 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 769 "y"	*/
        {SubFunction , false, None  , 1, false, 0,	254	}, /* 770    	*/
        {SubFunction , true , None  , 1, false, 0,	255	}, /* 771    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 772    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 773 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 774 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 775 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	2	}, /* 776 "z"	*/
        {SubFunction , true , None  , 1, false, 0,	256	}, /* 777    	*/
        {SubFunction , true , None  , 1, false, 0,	257	}, /* 778    	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 779 "y"	*/
        {NamedHolder , true , None  , 1, false, 0,	2	}, /* 780 "z"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 781 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	258	}, /* 782    	*/
        {SubFunction , false, None  , 1, false, 0,	259	}, /* 783    	*/
        {SubFunction , true , None  , 1, false, 0,	80	}, /* 784    	*/
        {SubFunction , false, None  , 1, false, 0,	83	}, /* 785    	*/
        {SubFunction , true , None  , 1, false, 0,	205	}, /* 786    	*/
        {SubFunction , true , None  , 1, false, 0,	260	}, /* 787    	*/
        {SubFunction , false, None  , 1, false, 0,	261	}, /* 788    	*/
        {SubFunction , true , None  , 1, false, 0,	262	}, /* 789    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 790 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 791 "y"	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 792 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	263	}, /* 793    	*/
        {NamedHolder , false, Negate, 1, true , 0,	0	}, /* 794 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 795 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 796 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	264	}, /* 797    	*/
        {SubFunction , false, None  , 1, false, 0,	265	}, /* 798    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 799 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	1	}, /* 800 "y"	*/
        {NamedHolder , true , None  , 1, true , 0,	0	}, /* 801 "x"	*/
        {SubFunction , true , None  , 1, false, 0,	266	}, /* 802    	*/
        {NamedHolder , false, Negate, 1, true , 0,	0	}, /* 803 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	1	}, /* 804 "y"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 805 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	267	}, /* 806    	*/
        {SubFunction , false, None  , 1, false, 0,	268	}, /* 807    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 808 "x"	*/
        {NamedHolder , false, None  , 2, true , 0,	0	}, /* 809 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	269	}, /* 810    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 811 "x"	*/
        {NamedHolder , false, Negate, 2, true , 0,	0	}, /* 812 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	270	}, /* 813    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 814    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 815    	*/
        {ImmedHolder , false, None  , 1, false, 0,	0	}, /* 816    	*/
        {ImmedHolder , false, None  , 1, false, 0,	1	}, /* 817    	*/
        {cMod        , false, None  , 1, false, 2,	816	}, /* 818    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 819 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 820 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 821 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 822 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 823 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 824 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	2	}, /* 825    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 826 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 827 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 828 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 829 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 830 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 831 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 832 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 833 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	271	}, /* 834    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 835 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 836 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	272	}, /* 837    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 838 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 839 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	273	}, /* 840    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 841 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 842 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	274	}, /* 843    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 844 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 845 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	275	}, /* 846    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 847 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 848 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	276	}, /* 849    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 850 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 851 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	277	}, /* 852    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 853 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 854 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	278	}, /* 855    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 856 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 857 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	279	}, /* 858    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 859 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 860 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	280	}, /* 861    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 862 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 863 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	281	}, /* 864    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 865 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 866 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	282	}, /* 867    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 868    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 869    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 870    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 871    	*/
        {SubFunction , true , None  , 1, false, 0,	283	}, /* 872    	*/
        {SubFunction , false, None  , 1, false, 0,	284	}, /* 873    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 874    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 875    	*/
        {SubFunction , false, None  , 1, false, 0,	285	}, /* 876    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 877    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 878    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 879 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 880 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	286	}, /* 881    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 882 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 883 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	287	}, /* 884    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 885 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 886 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	288	}, /* 887    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 888 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 889 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	289	}, /* 890    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 891 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 892 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	290	}, /* 893    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 894 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 895 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	291	}, /* 896    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 897 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 898 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	292	}, /* 899    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 900 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 901 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	293	}, /* 902    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 903 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 904 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	294	}, /* 905    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 906 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 907 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	295	}, /* 908    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 909 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 910 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	296	}, /* 911    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 912 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 913 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	297	}, /* 914    	*/
        {SubFunction , true , None  , 1, false, 0,	298	}, /* 915    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 916    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 917    	*/
        {SubFunction , true , None  , 1, false, 0,	299	}, /* 918    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 919    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 920    	*/
        {SubFunction , true , None  , 1, false, 0,	284	}, /* 921    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 922 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 923 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 924 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 925 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 926 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 927 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 928 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 929 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	300	}, /* 930    	*/
        {SubFunction , false, None  , 1, false, 0,	301	}, /* 931    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 932 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 933 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 934 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 935 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 936 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	5	}, /* 937 "c"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 938 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	5	}, /* 939 "c"	*/
        {SubFunction , false, None  , 1, false, 0,	302	}, /* 940    	*/
        {SubFunction , false, None  , 1, false, 0,	303	}, /* 941    	*/
        {SubFunction , false, None  , 1, false, 0,	304	}, /* 942    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 943 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 944 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 945 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	5	}, /* 946 "c"	*/
        {SubFunction , false, None  , 1, false, 0,	305	}, /* 947    	*/
        {SubFunction , false, None  , 1, false, 0,	306	}, /* 948    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 949    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 950    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 951    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 952    	*/
        {SubFunction , true , None  , 1, false, 0,	307	}, /* 953    	*/
        {SubFunction , false, None  , 1, false, 0,	1	}, /* 954    	*/
        {SubFunction , false, None  , 1, false, 0,	298	}, /* 955    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 956    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 957    	*/
        {SubFunction , false, None  , 1, false, 0,	308	}, /* 958    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 959    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 960    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 961 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 962 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	309	}, /* 963    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 964 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 965 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	310	}, /* 966    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 967 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 968 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	311	}, /* 969    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 970 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 971 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	312	}, /* 972    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 973 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 974 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	313	}, /* 975    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 976 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 977 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	314	}, /* 978    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 979 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 980 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	315	}, /* 981    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 982 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 983 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	316	}, /* 984    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 985 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 986 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	317	}, /* 987    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 988 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 989 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	318	}, /* 990    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 991 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 992 "b"	*/
        {SubFunction , true , None  , 1, false, 0,	319	}, /* 993    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 994 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 995 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	320	}, /* 996    	*/
        {SubFunction , true , None  , 1, false, 0,	321	}, /* 997    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 998    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 999    	*/
        {SubFunction , true , None  , 1, false, 0,	322	}, /* 1000    	*/
        {RestHolder  , true , None  , 1, false, 0,	1	}, /* 1001    	*/
        {RestHolder  , false, None  , 1, false, 0,	2	}, /* 1002    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1003 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1004 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1005 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 1006 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1007 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1008 "b"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1009 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1010 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	323	}, /* 1011    	*/
        {SubFunction , false, None  , 1, false, 0,	324	}, /* 1012    	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 1013 "x"	*/
        {NamedHolder , true , None  , 1, false, 0,	0	}, /* 1014 "x"	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1015 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1016 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	325	}, /* 1017    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1018 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1019 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	326	}, /* 1020    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1021 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1022 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	327	}, /* 1023    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1024 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1025 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	328	}, /* 1026    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1027 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1028 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	329	}, /* 1029    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1030 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1031 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	330	}, /* 1032    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1033 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1034 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	331	}, /* 1035    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1036 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1037 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	332	}, /* 1038    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1039 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1040 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	333	}, /* 1041    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1042 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1043 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	334	}, /* 1044    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1045 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1046 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	335	}, /* 1047    	*/
        {NamedHolder , false, None  , 1, false, 0,	3	}, /* 1048 "a"	*/
        {NamedHolder , false, None  , 1, false, 0,	4	}, /* 1049 "b"	*/
        {SubFunction , false, None  , 1, false, 0,	336	}, /* 1050    	*/
        {SubFunction , false, None  , 1, false, 0,	337	}, /* 1051    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1052    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1053    	*/
        {SubFunction , false, None  , 1, false, 0,	338	}, /* 1054    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1055    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1056    	*/
        {SubFunction , false, None  , 1, false, 0,	339	}, /* 1057    	*/
        {SubFunction , false, None  , 1, false, 0,	340	}, /* 1058    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1059    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1060    	*/
        {SubFunction , false, None  , 1, false, 0,	341	}, /* 1061    	*/
        {SubFunction , false, None  , 1, false, 0,	342	}, /* 1062    	*/
        {NumConstant , false, None  , 1, false, 0,	4	}, /* 1063    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1064 "x"	*/
        {SubFunction , false, None  , 1, false, 0,	343	}, /* 1065    	*/
        {NamedHolder , false, None  , 1, false, 0,	0	}, /* 1066 "x"	*/
        {NumConstant , false, None  , 1, false, 0,	1	}, /* 1067    	*/
        {SubFunction , false, None  , 1, false, 0,	344	}, /* 1068    	*/
        {NumConstant , false, None  , 1, false, 0,	8	}, /* 1069    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1070    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1071    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1072    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1073    	*/
        {SubFunction , false, None  , 1, false, 0,	345	}, /* 1074    	*/
        {SubFunction , false, None  , 1, false, 0,	346	}, /* 1075    	*/
        {NumConstant , false, None  , 1, false, 0,	9	}, /* 1076    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1077    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1078    	*/
        {RestHolder  , false, None  , 1, false, 0,	1	}, /* 1079    	*/
        {RestHolder  , true , None  , 1, false, 0,	2	}, /* 1080    	*/
        {SubFunction , false, None  , 1, false, 0,	347	}, /* 1081    	*/
        {SubFunction , false, None  , 1, false, 0,	348	}, /* 1082    	*/
        {SubFunction , true , None  , 1, false, 0,	83	}, /* 1083    	*/
        {SubFunction , false, None  , 1, false, 0,	349	}, /* 1084    	*/
        {SubFunction , true , None  , 1, false, 0,	5	}, /* 1085    	*/
        {SubFunction , false, None  , 1, false, 0,	350	}, /* 1086    	*/
        {SubFunction , false, None  , 1, false, 0,	351	}, /* 1087    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 1088    	*/
        {NumConstant , false, None  , 1, false, 0,	10	}, /* 1089    	*/
        {SubFunction , false, None  , 1, false, 0,	352	}, /* 1090    	*/
        {SubFunction , false, None  , 1, false, 0,	61	}, /* 1091    	*/
        {NumConstant , false, None  , 1, false, 0,	11	}, /* 1092    	*/
        {SubFunction , false, None  , 1, false, 0,	353	}, /* 1093    	*/
        {SubFunction , true , None  , 1, false, 0,	61	}, /* 1094    	*/
        {NumConstant , false, None  , 1, false, 0,	12	}, /* 1095    	*/
        {SubFunction , true , None  , 1, false, 0,	353	}, /* 1096    	*/
        {SubFunction , true , None  , 1, false, 0,	61	}, /* 1097    	*/
        {NumConstant , false, None  , 1, false, 0,	13	}, /* 1098    	*/
        {SubFunction , true , None  , 1, false, 0,	352	}, /* 1099    	*/
        {SubFunction , false, None  , 1, false, 0,	321	}, /* 1100    	*/
        {SubFunction , false, None  , 1, false, 0,	354	}, /* 1101    	*/
    };

    const MatchedParams mlist[] =
    {
        {PositionalParams, BalanceDontCare, 1, 0 }, /* 0 */
        {PositionalParams, BalanceDontCare, 1, 1 }, /* 1 */
        {PositionalParams, BalanceDontCare, 1, 2 }, /* 2 */
        {PositionalParams, BalanceDontCare, 1, 3 }, /* 3 */
        {PositionalParams, BalanceDontCare, 1, 4 }, /* 4 */
        {PositionalParams, BalanceDontCare, 1, 6 }, /* 5 */
        {PositionalParams, BalanceDontCare, 1, 5 }, /* 6 */
        {PositionalParams, BalanceDontCare, 1, 7 }, /* 7 */
        {PositionalParams, BalanceDontCare, 1, 8 }, /* 8 */
        {PositionalParams, BalanceDontCare, 1, 10 }, /* 9 */
        {PositionalParams, BalanceDontCare, 1, 11 }, /* 10 */
        {PositionalParams, BalanceDontCare, 1, 12 }, /* 11 */
        {PositionalParams, BalanceDontCare, 1, 13 }, /* 12 */
        {PositionalParams, BalanceDontCare, 1, 14 }, /* 13 */
        {PositionalParams, BalanceDontCare, 1, 15 }, /* 14 */
        {PositionalParams, BalanceDontCare, 1, 16 }, /* 15 */
        {PositionalParams, BalanceDontCare, 1, 17 }, /* 16 */
        {PositionalParams, BalanceDontCare, 1, 18 }, /* 17 */
        {PositionalParams, BalanceDontCare, 1, 19 }, /* 18 */
        {AnyParams       , BalanceDontCare, 2, 22 }, /* 19 */
        {PositionalParams, BalanceDontCare, 1, 24 }, /* 20 */
        {PositionalParams, BalanceDontCare, 1, 25 }, /* 21 */
        {PositionalParams, BalanceDontCare, 1, 26 }, /* 22 */
        {PositionalParams, BalanceDontCare, 1, 27 }, /* 23 */
        {PositionalParams, BalanceDontCare, 3, 28 }, /* 24 */
        {PositionalParams, BalanceDontCare, 1, 31 }, /* 25 */
        {PositionalParams, BalanceDontCare, 3, 32 }, /* 26 */
        {PositionalParams, BalanceDontCare, 3, 35 }, /* 27 */
        {AnyParams       , BalanceDontCare, 3, 38 }, /* 28 */
        {PositionalParams, BalanceDontCare, 3, 41 }, /* 29 */
        {PositionalParams, BalanceDontCare, 2, 44 }, /* 30 */
        {PositionalParams, BalanceDontCare, 3, 46 }, /* 31 */
        {PositionalParams, BalanceDontCare, 2, 49 }, /* 32 */
        {PositionalParams, BalanceDontCare, 1, 51 }, /* 33 */
        {AnyParams       , BalanceDontCare, 3, 52 }, /* 34 */
        {PositionalParams, BalanceDontCare, 3, 55 }, /* 35 */
        {PositionalParams, BalanceDontCare, 2, 58 }, /* 36 */
        {PositionalParams, BalanceDontCare, 3, 60 }, /* 37 */
        {PositionalParams, BalanceDontCare, 2, 63 }, /* 38 */
        {PositionalParams, BalanceDontCare, 1, 65 }, /* 39 */
        {AnyParams       , BalanceDontCare, 3, 66 }, /* 40 */
        {PositionalParams, BalanceDontCare, 3, 69 }, /* 41 */
        {PositionalParams, BalanceDontCare, 2, 72 }, /* 42 */
        {PositionalParams, BalanceDontCare, 3, 74 }, /* 43 */
        {PositionalParams, BalanceDontCare, 2, 77 }, /* 44 */
        {PositionalParams, BalanceDontCare, 1, 79 }, /* 45 */
        {AnyParams       , BalanceDontCare, 3, 80 }, /* 46 */
        {PositionalParams, BalanceDontCare, 3, 83 }, /* 47 */
        {PositionalParams, BalanceDontCare, 2, 86 }, /* 48 */
        {PositionalParams, BalanceDontCare, 3, 88 }, /* 49 */
        {PositionalParams, BalanceDontCare, 2, 91 }, /* 50 */
        {PositionalParams, BalanceDontCare, 1, 93 }, /* 51 */
        {AnyParams       , BalanceDontCare, 3, 94 }, /* 52 */
        {AnyParams       , BalanceDontCare, 3, 97 }, /* 53 */
        {PositionalParams, BalanceDontCare, 3, 100 }, /* 54 */
        {PositionalParams, BalanceDontCare, 2, 103 }, /* 55 */
        {PositionalParams, BalanceDontCare, 2, 105 }, /* 56 */
        {PositionalParams, BalanceDontCare, 3, 107 }, /* 57 */
        {PositionalParams, BalanceDontCare, 2, 110 }, /* 58 */
        {PositionalParams, BalanceDontCare, 1, 112 }, /* 59 */
        {AnyParams       , BalanceDontCare, 3, 113 }, /* 60 */
        {AnyParams       , BalanceDontCare, 3, 116 }, /* 61 */
        {PositionalParams, BalanceDontCare, 3, 119 }, /* 62 */
        {PositionalParams, BalanceDontCare, 3, 122 }, /* 63 */
        {PositionalParams, BalanceDontCare, 2, 125 }, /* 64 */
        {PositionalParams, BalanceDontCare, 1, 127 }, /* 65 */
        {AnyParams       , BalanceDontCare, 3, 128 }, /* 66 */
        {AnyParams       , BalanceDontCare, 3, 131 }, /* 67 */
        {PositionalParams, BalanceDontCare, 3, 134 }, /* 68 */
        {PositionalParams, BalanceDontCare, 3, 137 }, /* 69 */
        {PositionalParams, BalanceDontCare, 2, 140 }, /* 70 */
        {PositionalParams, BalanceDontCare, 1, 142 }, /* 71 */
        {AnyParams       , BalanceDontCare, 3, 143 }, /* 72 */
        {AnyParams       , BalanceDontCare, 3, 146 }, /* 73 */
        {PositionalParams, BalanceDontCare, 3, 149 }, /* 74 */
        {PositionalParams, BalanceDontCare, 2, 152 }, /* 75 */
        {PositionalParams, BalanceDontCare, 2, 154 }, /* 76 */
        {PositionalParams, BalanceDontCare, 3, 156 }, /* 77 */
        {PositionalParams, BalanceDontCare, 2, 159 }, /* 78 */
        {PositionalParams, BalanceDontCare, 1, 161 }, /* 79 */
        {AnyParams       , BalanceDontCare, 1, 16 }, /* 80 */
        {PositionalParams, BalanceDontCare, 3, 162 }, /* 81 */
        {PositionalParams, BalanceDontCare, 3, 165 }, /* 82 */
        {PositionalParams, BalanceDontCare, 1, 168 }, /* 83 */
        {AnyParams       , BalanceDontCare, 1, 169 }, /* 84 */
        {PositionalParams, BalanceDontCare, 3, 170 }, /* 85 */
        {PositionalParams, BalanceDontCare, 3, 173 }, /* 86 */
        {PositionalParams, BalanceDontCare, 1, 176 }, /* 87 */
        {PositionalParams, BalanceDontCare, 1, 177 }, /* 88 */
        {PositionalParams, BalanceDontCare, 1, 178 }, /* 89 */
        {PositionalParams, BalanceDontCare, 2, 179 }, /* 90 */
        {PositionalParams, BalanceDontCare, 1, 181 }, /* 91 */
        {PositionalParams, BalanceDontCare, 1, 169 }, /* 92 */
        {PositionalParams, BalanceDontCare, 2, 182 }, /* 93 */
        {PositionalParams, BalanceDontCare, 1, 184 }, /* 94 */
        {PositionalParams, BalanceDontCare, 2, 185 }, /* 95 */
        {AnyParams       , BalanceDontCare, 3, 187 }, /* 96 */
        {PositionalParams, BalanceDontCare, 1, 190 }, /* 97 */
        {PositionalParams, BalanceDontCare, 2, 191 }, /* 98 */
        {PositionalParams, BalanceDontCare, 1, 193 }, /* 99 */
        {PositionalParams, BalanceDontCare, 2, 194 }, /* 100 */
        {PositionalParams, BalanceDontCare, 1, 196 }, /* 101 */
        {AnyParams       , BalanceDontCare, 1, 53 }, /* 102 */
        {AnyParams       , BalanceDontCare, 1, 197 }, /* 103 */
        {PositionalParams, BalanceDontCare, 1, 53 }, /* 104 */
        {AnyParams       , BalanceDontCare, 2, 198 }, /* 105 */
        {PositionalParams, BalanceDontCare, 1, 202 }, /* 106 */
        {AnyParams       , BalanceDontCare, 2, 203 }, /* 107 */
        {AnyParams       , BalanceDontCare, 1, 205 }, /* 108 */
        {AnyParams       , BalanceDontCare, 2, 206 }, /* 109 */
        {PositionalParams, BalanceDontCare, 1, 210 }, /* 110 */
        {AnyParams       , BalanceDontCare, 2, 211 }, /* 111 */
        {PositionalParams, BalanceDontCare, 2, 213 }, /* 112 */
        {PositionalParams, BalanceDontCare, 1, 28 }, /* 113 */
        {PositionalParams, BalanceDontCare, 2, 215 }, /* 114 */
        {PositionalParams, BalanceDontCare, 1, 89 }, /* 115 */
        {PositionalParams, BalanceDontCare, 2, 217 }, /* 116 */
        {AnyParams       , BalanceDontCare, 3, 219 }, /* 117 */
        {PositionalParams, BalanceDontCare, 2, 222 }, /* 118 */
        {PositionalParams, BalanceDontCare, 2, 224 }, /* 119 */
        {PositionalParams, BalanceDontCare, 2, 226 }, /* 120 */
        {PositionalParams, BalanceDontCare, 2, 228 }, /* 121 */
        {PositionalParams, BalanceDontCare, 1, 232 }, /* 122 */
        {AnyParams       , BalanceDontCare, 4, 233 }, /* 123 */
        {PositionalParams, BalanceDontCare, 2, 237 }, /* 124 */
        {PositionalParams, BalanceDontCare, 2, 239 }, /* 125 */
        {PositionalParams, BalanceDontCare, 2, 241 }, /* 126 */
        {AnyParams       , BalanceDontCare, 4, 243 }, /* 127 */
        {PositionalParams, BalanceDontCare, 2, 247 }, /* 128 */
        {PositionalParams, BalanceDontCare, 2, 249 }, /* 129 */
        {PositionalParams, BalanceDontCare, 2, 251 }, /* 130 */
        {PositionalParams, BalanceDontCare, 2, 253 }, /* 131 */
        {PositionalParams, BalanceDontCare, 1, 255 }, /* 132 */
        {PositionalParams, BalanceDontCare, 2, 256 }, /* 133 */
        {PositionalParams, BalanceDontCare, 2, 258 }, /* 134 */
        {AnyParams       , BalanceMoreNeg , 2, 260 }, /* 135 */
        {PositionalParams, BalanceDontCare, 2, 262 }, /* 136 */
        {PositionalParams, BalanceDontCare, 2, 264 }, /* 137 */
        {PositionalParams, BalanceDontCare, 2, 266 }, /* 138 */
        {PositionalParams, BalanceDontCare, 1, 268 }, /* 139 */
        {PositionalParams, BalanceDontCare, 1, 269 }, /* 140 */
        {PositionalParams, BalanceDontCare, 1, 166 }, /* 141 */
        {AnyParams       , BalanceDontCare, 4, 270 }, /* 142 */
        {PositionalParams, BalanceDontCare, 2, 274 }, /* 143 */
        {PositionalParams, BalanceDontCare, 2, 276 }, /* 144 */
        {PositionalParams, BalanceDontCare, 2, 278 }, /* 145 */
        {PositionalParams, BalanceDontCare, 2, 280 }, /* 146 */
        {PositionalParams, BalanceDontCare, 2, 282 }, /* 147 */
        {PositionalParams, BalanceDontCare, 2, 284 }, /* 148 */
        {PositionalParams, BalanceDontCare, 2, 286 }, /* 149 */
        {PositionalParams, BalanceDontCare, 1, 288 }, /* 150 */
        {PositionalParams, BalanceDontCare, 1, 289 }, /* 151 */
        {PositionalParams, BalanceDontCare, 1, 290 }, /* 152 */
        {PositionalParams, BalanceDontCare, 1, 291 }, /* 153 */
        {AnyParams       , BalanceDontCare, 2, 294 }, /* 154 */
        {PositionalParams, BalanceDontCare, 1, 296 }, /* 155 */
        {PositionalParams, BalanceDontCare, 1, 297 }, /* 156 */
        {PositionalParams, BalanceDontCare, 1, 298 }, /* 157 */
        {PositionalParams, BalanceDontCare, 1, 299 }, /* 158 */
        {PositionalParams, BalanceDontCare, 1, 300 }, /* 159 */
        {PositionalParams, BalanceDontCare, 1, 301 }, /* 160 */
        {PositionalParams, BalanceDontCare, 1, 302 }, /* 161 */
        {PositionalParams, BalanceDontCare, 1, 303 }, /* 162 */
        {PositionalParams, BalanceDontCare, 0, 0 }, /* 163 */
        {PositionalParams, BalanceDontCare, 1, 47 }, /* 164 */
        {AnyParams       , BalanceDontCare, 1, 47 }, /* 165 */
        {AnyParams       , BalanceDontCare, 2, 304 }, /* 166 */
        {AnyParams       , BalanceDontCare, 1, 306 }, /* 167 */
        {PositionalParams, BalanceDontCare, 2, 307 }, /* 168 */
        {AnyParams       , BalanceMoreNeg , 2, 309 }, /* 169 */
        {AnyParams       , BalanceDontCare, 3, 311 }, /* 170 */
        {AnyParams       , BalanceDontCare, 1, 314 }, /* 171 */
        {PositionalParams, BalanceDontCare, 2, 315 }, /* 172 */
        {PositionalParams, BalanceDontCare, 3, 317 }, /* 173 */
        {PositionalParams, BalanceDontCare, 1, 320 }, /* 174 */
        {AnyParams       , BalanceDontCare, 1, 321 }, /* 175 */
        {PositionalParams, BalanceDontCare, 1, 322 }, /* 176 */
        {AnyParams       , BalanceDontCare, 2, 323 }, /* 177 */
        {AnyParams       , BalanceDontCare, 1, 325 }, /* 178 */
        {AnyParams       , BalanceDontCare, 3, 326 }, /* 179 */
        {AnyParams       , BalanceDontCare, 1, 329 }, /* 180 */
        {PositionalParams, BalanceDontCare, 3, 330 }, /* 181 */
        {PositionalParams, BalanceDontCare, 1, 333 }, /* 182 */
        {AnyParams       , BalanceMoreNeg , 2, 334 }, /* 183 */
        {AnyParams       , BalanceDontCare, 3, 336 }, /* 184 */
        {AnyParams       , BalanceDontCare, 1, 339 }, /* 185 */
        {PositionalParams, BalanceDontCare, 3, 340 }, /* 186 */
        {PositionalParams, BalanceDontCare, 1, 343 }, /* 187 */
        {PositionalParams, BalanceDontCare, 2, 344 }, /* 188 */
        {AnyParams       , BalanceDontCare, 2, 346 }, /* 189 */
        {PositionalParams, BalanceDontCare, 2, 348 }, /* 190 */
        {PositionalParams, BalanceDontCare, 1, 350 }, /* 191 */
        {PositionalParams, BalanceDontCare, 2, 351 }, /* 192 */
        {AnyParams       , BalanceDontCare, 2, 353 }, /* 193 */
        {PositionalParams, BalanceDontCare, 2, 355 }, /* 194 */
        {PositionalParams, BalanceDontCare, 1, 357 }, /* 195 */
        {AnyParams       , BalanceDontCare, 2, 358 }, /* 196 */
        {PositionalParams, BalanceDontCare, 1, 362 }, /* 197 */
        {AnyParams       , BalanceDontCare, 2, 363 }, /* 198 */
        {PositionalParams, BalanceDontCare, 1, 30 }, /* 199 */
        {AnyParams       , BalanceDontCare, 2, 365 }, /* 200 */
        {PositionalParams, BalanceDontCare, 2, 367 }, /* 201 */
        {PositionalParams, BalanceDontCare, 1, 369 }, /* 202 */
        {PositionalParams, BalanceDontCare, 1, 370 }, /* 203 */
        {PositionalParams, BalanceDontCare, 2, 371 }, /* 204 */
        {PositionalParams, BalanceDontCare, 2, 373 }, /* 205 */
        {AnyParams       , BalanceDontCare, 2, 375 }, /* 206 */
        {PositionalParams, BalanceDontCare, 1, 346 }, /* 207 */
        {AnyParams       , BalanceDontCare, 3, 377 }, /* 208 */
        {AnyParams       , BalanceDontCare, 3, 380 }, /* 209 */
        {AnyParams       , BalanceDontCare, 2, 383 }, /* 210 */
        {PositionalParams, BalanceDontCare, 2, 385 }, /* 211 */
        {PositionalParams, BalanceDontCare, 2, 387 }, /* 212 */
        {PositionalParams, BalanceDontCare, 2, 389 }, /* 213 */
        {PositionalParams, BalanceDontCare, 2, 391 }, /* 214 */
        {PositionalParams, BalanceDontCare, 1, 393 }, /* 215 */
        {AnyParams       , BalanceDontCare, 3, 394 }, /* 216 */
        {AnyParams       , BalanceDontCare, 3, 397 }, /* 217 */
        {AnyParams       , BalanceDontCare, 2, 400 }, /* 218 */
        {PositionalParams, BalanceDontCare, 2, 402 }, /* 219 */
        {PositionalParams, BalanceDontCare, 2, 404 }, /* 220 */
        {PositionalParams, BalanceDontCare, 2, 406 }, /* 221 */
        {PositionalParams, BalanceDontCare, 2, 408 }, /* 222 */
        {PositionalParams, BalanceDontCare, 1, 410 }, /* 223 */
        {AnyParams       , BalanceDontCare, 3, 411 }, /* 224 */
        {AnyParams       , BalanceDontCare, 3, 414 }, /* 225 */
        {AnyParams       , BalanceDontCare, 2, 417 }, /* 226 */
        {PositionalParams, BalanceDontCare, 2, 419 }, /* 227 */
        {PositionalParams, BalanceDontCare, 2, 421 }, /* 228 */
        {PositionalParams, BalanceDontCare, 2, 423 }, /* 229 */
        {PositionalParams, BalanceDontCare, 2, 425 }, /* 230 */
        {PositionalParams, BalanceDontCare, 1, 427 }, /* 231 */
        {AnyParams       , BalanceDontCare, 3, 428 }, /* 232 */
        {AnyParams       , BalanceDontCare, 3, 431 }, /* 233 */
        {AnyParams       , BalanceDontCare, 2, 434 }, /* 234 */
        {PositionalParams, BalanceDontCare, 2, 436 }, /* 235 */
        {PositionalParams, BalanceDontCare, 2, 438 }, /* 236 */
        {PositionalParams, BalanceDontCare, 2, 440 }, /* 237 */
        {PositionalParams, BalanceDontCare, 2, 442 }, /* 238 */
        {PositionalParams, BalanceDontCare, 1, 444 }, /* 239 */
        {SelectedParams  , BalanceDontCare, 2, 445 }, /* 240 */
        {PositionalParams, BalanceDontCare, 1, 88 }, /* 241 */
        {SelectedParams  , BalanceDontCare, 2, 447 }, /* 242 */
        {AnyParams       , BalanceDontCare, 2, 449 }, /* 243 */
        {PositionalParams, BalanceDontCare, 2, 451 }, /* 244 */
        {PositionalParams, BalanceDontCare, 1, 453 }, /* 245 */
        {PositionalParams, BalanceDontCare, 1, 454 }, /* 246 */
        {SelectedParams  , BalanceDontCare, 2, 455 }, /* 247 */
        {SelectedParams  , BalanceDontCare, 2, 457 }, /* 248 */
        {AnyParams       , BalanceDontCare, 2, 459 }, /* 249 */
        {PositionalParams, BalanceDontCare, 2, 461 }, /* 250 */
        {PositionalParams, BalanceDontCare, 1, 463 }, /* 251 */
        {PositionalParams, BalanceDontCare, 1, 464 }, /* 252 */
        {SelectedParams  , BalanceDontCare, 2, 465 }, /* 253 */
        {SelectedParams  , BalanceDontCare, 2, 467 }, /* 254 */
        {AnyParams       , BalanceDontCare, 2, 469 }, /* 255 */
        {PositionalParams, BalanceDontCare, 2, 471 }, /* 256 */
        {PositionalParams, BalanceDontCare, 1, 473 }, /* 257 */
        {PositionalParams, BalanceDontCare, 1, 474 }, /* 258 */
        {SelectedParams  , BalanceDontCare, 2, 475 }, /* 259 */
        {SelectedParams  , BalanceDontCare, 2, 477 }, /* 260 */
        {AnyParams       , BalanceDontCare, 2, 479 }, /* 261 */
        {PositionalParams, BalanceDontCare, 2, 481 }, /* 262 */
        {PositionalParams, BalanceDontCare, 1, 483 }, /* 263 */
        {PositionalParams, BalanceDontCare, 1, 484 }, /* 264 */
        {AnyParams       , BalanceDontCare, 3, 485 }, /* 265 */
        {AnyParams       , BalanceDontCare, 2, 488 }, /* 266 */
        {PositionalParams, BalanceDontCare, 2, 490 }, /* 267 */
        {PositionalParams, BalanceDontCare, 2, 492 }, /* 268 */
        {PositionalParams, BalanceDontCare, 2, 494 }, /* 269 */
        {PositionalParams, BalanceDontCare, 1, 496 }, /* 270 */
        {AnyParams       , BalanceDontCare, 3, 497 }, /* 271 */
        {AnyParams       , BalanceDontCare, 2, 500 }, /* 272 */
        {PositionalParams, BalanceDontCare, 2, 502 }, /* 273 */
        {PositionalParams, BalanceDontCare, 2, 504 }, /* 274 */
        {PositionalParams, BalanceDontCare, 2, 506 }, /* 275 */
        {PositionalParams, BalanceDontCare, 1, 508 }, /* 276 */
        {AnyParams       , BalanceDontCare, 3, 509 }, /* 277 */
        {AnyParams       , BalanceDontCare, 3, 512 }, /* 278 */
        {AnyParams       , BalanceDontCare, 2, 515 }, /* 279 */
        {PositionalParams, BalanceDontCare, 2, 517 }, /* 280 */
        {PositionalParams, BalanceDontCare, 2, 519 }, /* 281 */
        {PositionalParams, BalanceDontCare, 2, 521 }, /* 282 */
        {PositionalParams, BalanceDontCare, 2, 523 }, /* 283 */
        {PositionalParams, BalanceDontCare, 1, 525 }, /* 284 */
        {AnyParams       , BalanceDontCare, 3, 526 }, /* 285 */
        {AnyParams       , BalanceDontCare, 3, 529 }, /* 286 */
        {AnyParams       , BalanceDontCare, 2, 532 }, /* 287 */
        {PositionalParams, BalanceDontCare, 2, 534 }, /* 288 */
        {PositionalParams, BalanceDontCare, 2, 536 }, /* 289 */
        {PositionalParams, BalanceDontCare, 2, 538 }, /* 290 */
        {PositionalParams, BalanceDontCare, 2, 540 }, /* 291 */
        {PositionalParams, BalanceDontCare, 1, 542 }, /* 292 */
        {SelectedParams  , BalanceDontCare, 2, 543 }, /* 293 */
        {AnyParams       , BalanceDontCare, 2, 545 }, /* 294 */
        {PositionalParams, BalanceDontCare, 2, 547 }, /* 295 */
        {PositionalParams, BalanceDontCare, 1, 549 }, /* 296 */
        {PositionalParams, BalanceDontCare, 1, 550 }, /* 297 */
        {SelectedParams  , BalanceDontCare, 2, 551 }, /* 298 */
        {AnyParams       , BalanceDontCare, 2, 553 }, /* 299 */
        {PositionalParams, BalanceDontCare, 2, 555 }, /* 300 */
        {PositionalParams, BalanceDontCare, 1, 557 }, /* 301 */
        {PositionalParams, BalanceDontCare, 1, 558 }, /* 302 */
        {SelectedParams  , BalanceDontCare, 2, 559 }, /* 303 */
        {SelectedParams  , BalanceDontCare, 2, 561 }, /* 304 */
        {AnyParams       , BalanceDontCare, 2, 563 }, /* 305 */
        {PositionalParams, BalanceDontCare, 2, 565 }, /* 306 */
        {PositionalParams, BalanceDontCare, 1, 567 }, /* 307 */
        {PositionalParams, BalanceDontCare, 1, 568 }, /* 308 */
        {AnyParams       , BalanceDontCare, 3, 569 }, /* 309 */
        {AnyParams       , BalanceDontCare, 2, 572 }, /* 310 */
        {PositionalParams, BalanceDontCare, 2, 39 }, /* 311 */
        {PositionalParams, BalanceDontCare, 2, 574 }, /* 312 */
        {PositionalParams, BalanceDontCare, 2, 576 }, /* 313 */
        {PositionalParams, BalanceDontCare, 1, 578 }, /* 314 */
        {AnyParams       , BalanceDontCare, 3, 579 }, /* 315 */
        {AnyParams       , BalanceDontCare, 2, 582 }, /* 316 */
        {PositionalParams, BalanceDontCare, 2, 584 }, /* 317 */
        {PositionalParams, BalanceDontCare, 2, 586 }, /* 318 */
        {PositionalParams, BalanceDontCare, 2, 588 }, /* 319 */
        {PositionalParams, BalanceDontCare, 1, 590 }, /* 320 */
        {AnyParams       , BalanceDontCare, 1, 591 }, /* 321 */
        {PositionalParams, BalanceDontCare, 2, 592 }, /* 322 */
        {PositionalParams, BalanceDontCare, 1, 594 }, /* 323 */
        {AnyParams       , BalanceDontCare, 1, 595 }, /* 324 */
        {PositionalParams, BalanceDontCare, 2, 596 }, /* 325 */
        {PositionalParams, BalanceDontCare, 1, 598 }, /* 326 */
        {PositionalParams, BalanceDontCare, 2, 599 }, /* 327 */
        {PositionalParams, BalanceDontCare, 2, 601 }, /* 328 */
        {AnyParams       , BalanceDontCare, 4, 603 }, /* 329 */
        {AnyParams       , BalanceDontCare, 3, 607 }, /* 330 */
        {PositionalParams, BalanceDontCare, 2, 610 }, /* 331 */
        {PositionalParams, BalanceDontCare, 2, 612 }, /* 332 */
        {PositionalParams, BalanceDontCare, 2, 614 }, /* 333 */
        {PositionalParams, BalanceDontCare, 2, 616 }, /* 334 */
        {PositionalParams, BalanceDontCare, 3, 618 }, /* 335 */
        {PositionalParams, BalanceDontCare, 2, 621 }, /* 336 */
        {PositionalParams, BalanceDontCare, 1, 32 }, /* 337 */
        {AnyParams       , BalanceDontCare, 2, 625 }, /* 338 */
        {PositionalParams, BalanceDontCare, 1, 627 }, /* 339 */
        {PositionalParams, BalanceDontCare, 1, 628 }, /* 340 */
        {PositionalParams, BalanceDontCare, 1, 629 }, /* 341 */
        {AnyParams       , BalanceDontCare, 3, 630 }, /* 342 */
        {PositionalParams, BalanceDontCare, 2, 633 }, /* 343 */
        {PositionalParams, BalanceDontCare, 1, 635 }, /* 344 */
        {PositionalParams, BalanceDontCare, 1, 636 }, /* 345 */
        {AnyParams       , BalanceDontCare, 1, 89 }, /* 346 */
        {AnyParams       , BalanceDontCare, 2, 637 }, /* 347 */
        {AnyParams       , BalanceDontCare, 1, 639 }, /* 348 */
        {PositionalParams, BalanceDontCare, 2, 640 }, /* 349 */
        {PositionalParams, BalanceDontCare, 2, 642 }, /* 350 */
        {AnyParams       , BalanceDontCare, 1, 644 }, /* 351 */
        {PositionalParams, BalanceDontCare, 2, 645 }, /* 352 */
        {PositionalParams, BalanceDontCare, 1, 647 }, /* 353 */
        {AnyParams       , BalanceDontCare, 2, 648 }, /* 354 */
        {AnyParams       , BalanceDontCare, 1, 650 }, /* 355 */
        {AnyParams       , BalanceDontCare, 2, 651 }, /* 356 */
        {PositionalParams, BalanceDontCare, 1, 655 }, /* 357 */
        {AnyParams       , BalanceDontCare, 2, 656 }, /* 358 */
        {PositionalParams, BalanceDontCare, 1, 660 }, /* 359 */
        {AnyParams       , BalanceDontCare, 2, 661 }, /* 360 */
        {PositionalParams, BalanceDontCare, 2, 663 }, /* 361 */
        {PositionalParams, BalanceDontCare, 1, 665 }, /* 362 */
        {AnyParams       , BalanceDontCare, 2, 666 }, /* 363 */
        {PositionalParams, BalanceDontCare, 2, 668 }, /* 364 */
        {PositionalParams, BalanceDontCare, 1, 670 }, /* 365 */
        {AnyParams       , BalanceDontCare, 2, 671 }, /* 366 */
        {PositionalParams, BalanceDontCare, 2, 673 }, /* 367 */
        {PositionalParams, BalanceDontCare, 1, 675 }, /* 368 */
        {AnyParams       , BalanceDontCare, 2, 676 }, /* 369 */
        {PositionalParams, BalanceDontCare, 2, 678 }, /* 370 */
        {AnyParams       , BalanceDontCare, 2, 680 }, /* 371 */
        {PositionalParams, BalanceDontCare, 1, 682 }, /* 372 */
        {AnyParams       , BalanceDontCare, 2, 683 }, /* 373 */
        {PositionalParams, BalanceDontCare, 2, 685 }, /* 374 */
        {PositionalParams, BalanceDontCare, 2, 687 }, /* 375 */
        {PositionalParams, BalanceDontCare, 1, 689 }, /* 376 */
        {PositionalParams, BalanceDontCare, 2, 690 }, /* 377 */
        {AnyParams       , BalanceDontCare, 2, 692 }, /* 378 */
        {PositionalParams, BalanceDontCare, 1, 694 }, /* 379 */
        {AnyParams       , BalanceDontCare, 2, 695 }, /* 380 */
        {PositionalParams, BalanceDontCare, 2, 697 }, /* 381 */
        {PositionalParams, BalanceDontCare, 1, 699 }, /* 382 */
        {PositionalParams, BalanceDontCare, 2, 700 }, /* 383 */
        {AnyParams       , BalanceDontCare, 2, 702 }, /* 384 */
        {PositionalParams, BalanceDontCare, 1, 704 }, /* 385 */
        {PositionalParams, BalanceDontCare, 1, 283 }, /* 386 */
        {AnyParams       , BalanceDontCare, 2, 705 }, /* 387 */
        {PositionalParams, BalanceDontCare, 1, 164 }, /* 388 */
        {PositionalParams, BalanceDontCare, 2, 707 }, /* 389 */
        {PositionalParams, BalanceDontCare, 2, 709 }, /* 390 */
        {PositionalParams, BalanceDontCare, 1, 711 }, /* 391 */
        {PositionalParams, BalanceDontCare, 2, 712 }, /* 392 */
        {PositionalParams, BalanceDontCare, 2, 714 }, /* 393 */
        {AnyParams       , BalanceDontCare, 2, 716 }, /* 394 */
        {PositionalParams, BalanceDontCare, 2, 718 }, /* 395 */
        {PositionalParams, BalanceDontCare, 2, 720 }, /* 396 */
        {PositionalParams, BalanceDontCare, 1, 722 }, /* 397 */
        {PositionalParams, BalanceDontCare, 2, 723 }, /* 398 */
        {PositionalParams, BalanceDontCare, 2, 725 }, /* 399 */
        {AnyParams       , BalanceDontCare, 2, 727 }, /* 400 */
        {PositionalParams, BalanceDontCare, 2, 729 }, /* 401 */
        {PositionalParams, BalanceDontCare, 2, 731 }, /* 402 */
        {PositionalParams, BalanceDontCare, 1, 733 }, /* 403 */
        {AnyParams       , BalanceDontCare, 2, 734 }, /* 404 */
        {PositionalParams, BalanceDontCare, 1, 736 }, /* 405 */
        {AnyParams       , BalanceDontCare, 2, 737 }, /* 406 */
        {PositionalParams, BalanceDontCare, 1, 739 }, /* 407 */
        {PositionalParams, BalanceDontCare, 2, 740 }, /* 408 */
        {AnyParams       , BalanceDontCare, 2, 742 }, /* 409 */
        {PositionalParams, BalanceDontCare, 2, 744 }, /* 410 */
        {PositionalParams, BalanceDontCare, 2, 746 }, /* 411 */
        {PositionalParams, BalanceDontCare, 1, 748 }, /* 412 */
        {PositionalParams, BalanceDontCare, 2, 749 }, /* 413 */
        {AnyParams       , BalanceDontCare, 2, 751 }, /* 414 */
        {PositionalParams, BalanceDontCare, 2, 753 }, /* 415 */
        {PositionalParams, BalanceDontCare, 2, 755 }, /* 416 */
        {PositionalParams, BalanceDontCare, 1, 757 }, /* 417 */
        {AnyParams       , BalanceDontCare, 2, 758 }, /* 418 */
        {PositionalParams, BalanceDontCare, 1, 762 }, /* 419 */
        {PositionalParams, BalanceDontCare, 2, 763 }, /* 420 */
        {PositionalParams, BalanceDontCare, 1, 765 }, /* 421 */
        {AnyParams       , BalanceDontCare, 2, 766 }, /* 422 */
        {PositionalParams, BalanceDontCare, 1, 754 }, /* 423 */
        {PositionalParams, BalanceDontCare, 2, 768 }, /* 424 */
        {PositionalParams, BalanceDontCare, 1, 770 }, /* 425 */
        {AnyParams       , BalanceDontCare, 2, 771 }, /* 426 */
        {PositionalParams, BalanceDontCare, 1, 462 }, /* 427 */
        {PositionalParams, BalanceDontCare, 2, 773 }, /* 428 */
        {PositionalParams, BalanceDontCare, 2, 775 }, /* 429 */
        {AnyParams       , BalanceDontCare, 2, 777 }, /* 430 */
        {PositionalParams, BalanceDontCare, 2, 779 }, /* 431 */
        {PositionalParams, BalanceDontCare, 2, 781 }, /* 432 */
        {PositionalParams, BalanceDontCare, 1, 783 }, /* 433 */
        {AnyParams       , BalanceDontCare, 2, 784 }, /* 434 */
        {PositionalParams, BalanceDontCare, 1, 786 }, /* 435 */
        {AnyParams       , BalanceDontCare, 2, 787 }, /* 436 */
        {PositionalParams, BalanceDontCare, 1, 789 }, /* 437 */
        {PositionalParams, BalanceDontCare, 2, 790 }, /* 438 */
        {AnyParams       , BalanceDontCare, 2, 792 }, /* 439 */
        {PositionalParams, BalanceDontCare, 2, 794 }, /* 440 */
        {PositionalParams, BalanceDontCare, 2, 796 }, /* 441 */
        {PositionalParams, BalanceDontCare, 1, 798 }, /* 442 */
        {PositionalParams, BalanceDontCare, 2, 799 }, /* 443 */
        {AnyParams       , BalanceDontCare, 2, 801 }, /* 444 */
        {PositionalParams, BalanceDontCare, 2, 803 }, /* 445 */
        {PositionalParams, BalanceDontCare, 2, 805 }, /* 446 */
        {PositionalParams, BalanceDontCare, 1, 807 }, /* 447 */
        {PositionalParams, BalanceDontCare, 2, 808 }, /* 448 */
        {PositionalParams, BalanceDontCare, 1, 810 }, /* 449 */
        {PositionalParams, BalanceDontCare, 2, 811 }, /* 450 */
        {PositionalParams, BalanceDontCare, 1, 813 }, /* 451 */
        {PositionalParams, BalanceDontCare, 2, 814 }, /* 452 */
        {PositionalParams, BalanceDontCare, 1, 818 }, /* 453 */
        {PositionalParams, BalanceDontCare, 2, 819 }, /* 454 */
        {PositionalParams, BalanceDontCare, 2, 821 }, /* 455 */
        {PositionalParams, BalanceDontCare, 2, 823 }, /* 456 */
        {PositionalParams, BalanceDontCare, 1, 825 }, /* 457 */
        {PositionalParams, BalanceDontCare, 2, 826 }, /* 458 */
        {PositionalParams, BalanceDontCare, 2, 828 }, /* 459 */
        {PositionalParams, BalanceDontCare, 2, 830 }, /* 460 */
        {PositionalParams, BalanceDontCare, 2, 832 }, /* 461 */
        {PositionalParams, BalanceDontCare, 1, 834 }, /* 462 */
        {PositionalParams, BalanceDontCare, 2, 835 }, /* 463 */
        {PositionalParams, BalanceDontCare, 1, 837 }, /* 464 */
        {PositionalParams, BalanceDontCare, 2, 838 }, /* 465 */
        {PositionalParams, BalanceDontCare, 1, 840 }, /* 466 */
        {PositionalParams, BalanceDontCare, 2, 841 }, /* 467 */
        {PositionalParams, BalanceDontCare, 1, 843 }, /* 468 */
        {PositionalParams, BalanceDontCare, 2, 844 }, /* 469 */
        {PositionalParams, BalanceDontCare, 1, 846 }, /* 470 */
        {PositionalParams, BalanceDontCare, 2, 847 }, /* 471 */
        {PositionalParams, BalanceDontCare, 1, 849 }, /* 472 */
        {PositionalParams, BalanceDontCare, 2, 850 }, /* 473 */
        {PositionalParams, BalanceDontCare, 1, 852 }, /* 474 */
        {PositionalParams, BalanceDontCare, 2, 853 }, /* 475 */
        {PositionalParams, BalanceDontCare, 1, 855 }, /* 476 */
        {PositionalParams, BalanceDontCare, 2, 856 }, /* 477 */
        {PositionalParams, BalanceDontCare, 1, 858 }, /* 478 */
        {PositionalParams, BalanceDontCare, 2, 859 }, /* 479 */
        {PositionalParams, BalanceDontCare, 1, 861 }, /* 480 */
        {PositionalParams, BalanceDontCare, 2, 862 }, /* 481 */
        {PositionalParams, BalanceDontCare, 1, 864 }, /* 482 */
        {PositionalParams, BalanceDontCare, 2, 865 }, /* 483 */
        {PositionalParams, BalanceDontCare, 1, 867 }, /* 484 */
        {AnyParams       , BalanceMoreNeg , 2, 868 }, /* 485 */
        {PositionalParams, BalanceDontCare, 2, 870 }, /* 486 */
        {PositionalParams, BalanceDontCare, 1, 872 }, /* 487 */
        {PositionalParams, BalanceDontCare, 1, 873 }, /* 488 */
        {PositionalParams, BalanceDontCare, 1, 364 }, /* 489 */
        {AnyParams       , BalanceDontCare, 1, 1 }, /* 490 */
        {AnyParams       , BalanceDontCare, 2, 874 }, /* 491 */
        {AnyParams       , BalanceDontCare, 1, 876 }, /* 492 */
        {PositionalParams, BalanceDontCare, 2, 877 }, /* 493 */
        {AnyParams       , BalanceDontCare, 1, 873 }, /* 494 */
        {PositionalParams, BalanceDontCare, 2, 879 }, /* 495 */
        {AnyParams       , BalanceDontCare, 1, 881 }, /* 496 */
        {PositionalParams, BalanceDontCare, 2, 882 }, /* 497 */
        {PositionalParams, BalanceDontCare, 1, 884 }, /* 498 */
        {PositionalParams, BalanceDontCare, 2, 885 }, /* 499 */
        {AnyParams       , BalanceDontCare, 1, 887 }, /* 500 */
        {PositionalParams, BalanceDontCare, 2, 888 }, /* 501 */
        {PositionalParams, BalanceDontCare, 1, 890 }, /* 502 */
        {PositionalParams, BalanceDontCare, 2, 891 }, /* 503 */
        {AnyParams       , BalanceDontCare, 1, 893 }, /* 504 */
        {PositionalParams, BalanceDontCare, 2, 894 }, /* 505 */
        {PositionalParams, BalanceDontCare, 1, 896 }, /* 506 */
        {PositionalParams, BalanceDontCare, 2, 897 }, /* 507 */
        {AnyParams       , BalanceDontCare, 1, 899 }, /* 508 */
        {PositionalParams, BalanceDontCare, 2, 900 }, /* 509 */
        {PositionalParams, BalanceDontCare, 1, 902 }, /* 510 */
        {PositionalParams, BalanceDontCare, 2, 903 }, /* 511 */
        {AnyParams       , BalanceDontCare, 1, 905 }, /* 512 */
        {PositionalParams, BalanceDontCare, 2, 906 }, /* 513 */
        {PositionalParams, BalanceDontCare, 1, 908 }, /* 514 */
        {PositionalParams, BalanceDontCare, 2, 909 }, /* 515 */
        {AnyParams       , BalanceDontCare, 1, 911 }, /* 516 */
        {PositionalParams, BalanceDontCare, 2, 912 }, /* 517 */
        {PositionalParams, BalanceDontCare, 1, 914 }, /* 518 */
        {AnyParams       , BalanceDontCare, 1, 915 }, /* 519 */
        {AnyParams       , BalanceDontCare, 2, 916 }, /* 520 */
        {AnyParams       , BalanceDontCare, 1, 918 }, /* 521 */
        {PositionalParams, BalanceDontCare, 2, 919 }, /* 522 */
        {AnyParams       , BalanceDontCare, 1, 921 }, /* 523 */
        {AnyParams       , BalanceDontCare, 2, 922 }, /* 524 */
        {AnyParams       , BalanceDontCare, 2, 924 }, /* 525 */
        {AnyParams       , BalanceDontCare, 2, 926 }, /* 526 */
        {AnyParams       , BalanceDontCare, 2, 928 }, /* 527 */
        {AnyParams       , BalanceDontCare, 2, 930 }, /* 528 */
        {AnyParams       , BalanceDontCare, 2, 932 }, /* 529 */
        {AnyParams       , BalanceDontCare, 2, 934 }, /* 530 */
        {AnyParams       , BalanceDontCare, 2, 936 }, /* 531 */
        {AnyParams       , BalanceDontCare, 2, 938 }, /* 532 */
        {AnyParams       , BalanceDontCare, 3, 940 }, /* 533 */
        {PositionalParams, BalanceDontCare, 2, 943 }, /* 534 */
        {PositionalParams, BalanceDontCare, 2, 945 }, /* 535 */
        {PositionalParams, BalanceDontCare, 2, 947 }, /* 536 */
        {AnyParams       , BalanceMoreNeg , 2, 949 }, /* 537 */
        {PositionalParams, BalanceDontCare, 2, 951 }, /* 538 */
        {PositionalParams, BalanceDontCare, 1, 953 }, /* 539 */
        {PositionalParams, BalanceDontCare, 1, 954 }, /* 540 */
        {PositionalParams, BalanceDontCare, 1, 955 }, /* 541 */
        {AnyParams       , BalanceDontCare, 1, 955 }, /* 542 */
        {AnyParams       , BalanceDontCare, 2, 956 }, /* 543 */
        {AnyParams       , BalanceDontCare, 1, 958 }, /* 544 */
        {PositionalParams, BalanceDontCare, 2, 959 }, /* 545 */
        {AnyParams       , BalanceDontCare, 1, 954 }, /* 546 */
        {PositionalParams, BalanceDontCare, 2, 961 }, /* 547 */
        {AnyParams       , BalanceDontCare, 1, 963 }, /* 548 */
        {PositionalParams, BalanceDontCare, 2, 964 }, /* 549 */
        {PositionalParams, BalanceDontCare, 1, 966 }, /* 550 */
        {PositionalParams, BalanceDontCare, 2, 967 }, /* 551 */
        {AnyParams       , BalanceDontCare, 1, 969 }, /* 552 */
        {PositionalParams, BalanceDontCare, 2, 970 }, /* 553 */
        {PositionalParams, BalanceDontCare, 1, 972 }, /* 554 */
        {PositionalParams, BalanceDontCare, 2, 973 }, /* 555 */
        {AnyParams       , BalanceDontCare, 1, 975 }, /* 556 */
        {PositionalParams, BalanceDontCare, 2, 976 }, /* 557 */
        {PositionalParams, BalanceDontCare, 1, 978 }, /* 558 */
        {PositionalParams, BalanceDontCare, 2, 979 }, /* 559 */
        {AnyParams       , BalanceDontCare, 1, 981 }, /* 560 */
        {PositionalParams, BalanceDontCare, 2, 982 }, /* 561 */
        {PositionalParams, BalanceDontCare, 1, 984 }, /* 562 */
        {PositionalParams, BalanceDontCare, 2, 985 }, /* 563 */
        {AnyParams       , BalanceDontCare, 1, 987 }, /* 564 */
        {PositionalParams, BalanceDontCare, 2, 988 }, /* 565 */
        {PositionalParams, BalanceDontCare, 1, 990 }, /* 566 */
        {PositionalParams, BalanceDontCare, 2, 991 }, /* 567 */
        {AnyParams       , BalanceDontCare, 1, 993 }, /* 568 */
        {PositionalParams, BalanceDontCare, 2, 994 }, /* 569 */
        {PositionalParams, BalanceDontCare, 1, 996 }, /* 570 */
        {AnyParams       , BalanceDontCare, 1, 997 }, /* 571 */
        {AnyParams       , BalanceDontCare, 2, 998 }, /* 572 */
        {AnyParams       , BalanceDontCare, 1, 1000 }, /* 573 */
        {PositionalParams, BalanceDontCare, 2, 1001 }, /* 574 */
        {AnyParams       , BalanceDontCare, 2, 1003 }, /* 575 */
        {AnyParams       , BalanceDontCare, 2, 1005 }, /* 576 */
        {AnyParams       , BalanceDontCare, 2, 1007 }, /* 577 */
        {AnyParams       , BalanceDontCare, 2, 1009 }, /* 578 */
        {AnyParams       , BalanceDontCare, 2, 1011 }, /* 579 */
        {AnyParams       , BalanceDontCare, 2, 1013 }, /* 580 */
        {PositionalParams, BalanceDontCare, 2, 1015 }, /* 581 */
        {PositionalParams, BalanceDontCare, 1, 1017 }, /* 582 */
        {PositionalParams, BalanceDontCare, 2, 1018 }, /* 583 */
        {PositionalParams, BalanceDontCare, 1, 1020 }, /* 584 */
        {PositionalParams, BalanceDontCare, 2, 1021 }, /* 585 */
        {PositionalParams, BalanceDontCare, 1, 1023 }, /* 586 */
        {PositionalParams, BalanceDontCare, 2, 1024 }, /* 587 */
        {PositionalParams, BalanceDontCare, 1, 1026 }, /* 588 */
        {PositionalParams, BalanceDontCare, 2, 1027 }, /* 589 */
        {PositionalParams, BalanceDontCare, 1, 1029 }, /* 590 */
        {PositionalParams, BalanceDontCare, 2, 1030 }, /* 591 */
        {PositionalParams, BalanceDontCare, 1, 1032 }, /* 592 */
        {PositionalParams, BalanceDontCare, 2, 1033 }, /* 593 */
        {PositionalParams, BalanceDontCare, 1, 1035 }, /* 594 */
        {PositionalParams, BalanceDontCare, 2, 1036 }, /* 595 */
        {PositionalParams, BalanceDontCare, 1, 1038 }, /* 596 */
        {PositionalParams, BalanceDontCare, 2, 1039 }, /* 597 */
        {PositionalParams, BalanceDontCare, 1, 1041 }, /* 598 */
        {PositionalParams, BalanceDontCare, 2, 1042 }, /* 599 */
        {PositionalParams, BalanceDontCare, 1, 1044 }, /* 600 */
        {PositionalParams, BalanceDontCare, 2, 1045 }, /* 601 */
        {PositionalParams, BalanceDontCare, 1, 1047 }, /* 602 */
        {PositionalParams, BalanceDontCare, 2, 1048 }, /* 603 */
        {PositionalParams, BalanceDontCare, 1, 1050 }, /* 604 */
        {PositionalParams, BalanceDontCare, 1, 1051 }, /* 605 */
        {AnyParams       , BalanceDontCare, 2, 1052 }, /* 606 */
        {PositionalParams, BalanceDontCare, 1, 1054 }, /* 607 */
        {PositionalParams, BalanceDontCare, 2, 1055 }, /* 608 */
        {PositionalParams, BalanceDontCare, 1, 1057 }, /* 609 */
        {AnyParams       , BalanceDontCare, 2, 39 }, /* 610 */
        {PositionalParams, BalanceDontCare, 1, 1058 }, /* 611 */
        {PositionalParams, BalanceDontCare, 2, 1059 }, /* 612 */
        {PositionalParams, BalanceDontCare, 1, 1061 }, /* 613 */
        {PositionalParams, BalanceDontCare, 1, 1062 }, /* 614 */
        {PositionalParams, BalanceDontCare, 2, 1063 }, /* 615 */
        {PositionalParams, BalanceDontCare, 1, 1065 }, /* 616 */
        {PositionalParams, BalanceDontCare, 2, 1066 }, /* 617 */
        {PositionalParams, BalanceDontCare, 1, 1068 }, /* 618 */
        {AnyParams       , BalanceDontCare, 3, 1069 }, /* 619 */
        {PositionalParams, BalanceDontCare, 2, 1072 }, /* 620 */
        {PositionalParams, BalanceDontCare, 1, 1074 }, /* 621 */
        {PositionalParams, BalanceDontCare, 1, 1075 }, /* 622 */
        {AnyParams       , BalanceDontCare, 3, 1076 }, /* 623 */
        {PositionalParams, BalanceDontCare, 2, 1079 }, /* 624 */
        {PositionalParams, BalanceDontCare, 1, 1081 }, /* 625 */
        {PositionalParams, BalanceDontCare, 1, 1082 }, /* 626 */
        {AnyParams       , BalanceDontCare, 1, 1083 }, /* 627 */
        {PositionalParams, BalanceDontCare, 1, 1084 }, /* 628 */
        {AnyParams       , BalanceDontCare, 1, 1085 }, /* 629 */
        {PositionalParams, BalanceDontCare, 1, 1086 }, /* 630 */
        {AnyParams       , BalanceDontCare, 1, 786 }, /* 631 */
        {PositionalParams, BalanceDontCare, 1, 1087 }, /* 632 */
        {AnyParams       , BalanceDontCare, 2, 1088 }, /* 633 */
        {PositionalParams, BalanceDontCare, 1, 1090 }, /* 634 */
        {AnyParams       , BalanceDontCare, 2, 1091 }, /* 635 */
        {PositionalParams, BalanceDontCare, 1, 1093 }, /* 636 */
        {AnyParams       , BalanceDontCare, 2, 1094 }, /* 637 */
        {PositionalParams, BalanceDontCare, 1, 1096 }, /* 638 */
        {AnyParams       , BalanceDontCare, 2, 1097 }, /* 639 */
        {PositionalParams, BalanceDontCare, 1, 1099 }, /* 640 */
        {PositionalParams, BalanceDontCare, 1, 1100 }, /* 641 */
        {PositionalParams, BalanceDontCare, 1, 1101 }, /* 642 */
    };

    const Function flist[] =
    {
        {cNot        , 0 }, /* 0 */
        {cNotNot     , 2 }, /* 1 */
        {cAcos       , 15 }, /* 2 */
        {cAdd        , 17 }, /* 3 */
        {cAdd        , 19 }, /* 4 */
        {cSin        , 2 }, /* 5 */
        {cAdd        , 28 }, /* 6 */
        {cAdd        , 30 }, /* 7 */
        {cIf         , 31 }, /* 8 */
        {cAdd        , 32 }, /* 9 */
        {cMul        , 34 }, /* 10 */
        {cMul        , 36 }, /* 11 */
        {cIf         , 37 }, /* 12 */
        {cMul        , 38 }, /* 13 */
        {cAnd        , 40 }, /* 14 */
        {cAnd        , 42 }, /* 15 */
        {cIf         , 43 }, /* 16 */
        {cAnd        , 44 }, /* 17 */
        {cOr         , 46 }, /* 18 */
        {cOr         , 48 }, /* 19 */
        {cIf         , 49 }, /* 20 */
        {cOr         , 50 }, /* 21 */
        {cAdd        , 52 }, /* 22 */
        {cAdd        , 53 }, /* 23 */
        {cAdd        , 55 }, /* 24 */
        {cAdd        , 56 }, /* 25 */
        {cIf         , 57 }, /* 26 */
        {cAdd        , 58 }, /* 27 */
        {cMul        , 60 }, /* 28 */
        {cMul        , 61 }, /* 29 */
        {cMul        , 30 }, /* 30 */
        {cMul        , 56 }, /* 31 */
        {cIf         , 63 }, /* 32 */
        {cMul        , 64 }, /* 33 */
        {cAnd        , 66 }, /* 34 */
        {cAnd        , 67 }, /* 35 */
        {cAnd        , 30 }, /* 36 */
        {cAnd        , 56 }, /* 37 */
        {cIf         , 69 }, /* 38 */
        {cAnd        , 70 }, /* 39 */
        {cOr         , 72 }, /* 40 */
        {cOr         , 73 }, /* 41 */
        {cOr         , 75 }, /* 42 */
        {cOr         , 76 }, /* 43 */
        {cIf         , 77 }, /* 44 */
        {cOr         , 78 }, /* 45 */
        {cNot        , 80 }, /* 46 */
        {cIf         , 82 }, /* 47 */
        {cNotNot     , 84 }, /* 48 */
        {cIf         , 86 }, /* 49 */
        {cPow        , 90 }, /* 50 */
        {cLog        , 92 }, /* 51 */
        {cMul        , 93 }, /* 52 */
        {cPow        , 95 }, /* 53 */
        {cMul        , 96 }, /* 54 */
        {cMul        , 98 }, /* 55 */
        {cLog        , 99 }, /* 56 */
        {cAdd        , 100 }, /* 57 */
        {cMax        , 102 }, /* 58 */
        {cMin        , 102 }, /* 59 */
        {cLog        , 15 }, /* 60 */
        {cLog        , 2 }, /* 61 */
        {cMul        , 117 }, /* 62 */
        {cMul        , 119 }, /* 63 */
        {cMul        , 123 }, /* 64 */
        {cMul        , 125 }, /* 65 */
        {cMul        , 127 }, /* 66 */
        {cMul        , 129 }, /* 67 */
        {cMul        , 17 }, /* 68 */
        {cAdd        , 135 }, /* 69 */
        {cAdd        , 137 }, /* 70 */
        {cPow        , 138 }, /* 71 */
        {cMul        , 139 }, /* 72 */
        {cLog        , 141 }, /* 73 */
        {cLog        , 0 }, /* 74 */
        {cMul        , 142 }, /* 75 */
        {cMul        , 144 }, /* 76 */
        {cPow        , 146 }, /* 77 */
        {cMul        , 148 }, /* 78 */
        {cAsin       , 15 }, /* 79 */
        {cSin        , 15 }, /* 80 */
        {cAdd        , 152 }, /* 81 */
        {cAdd        , 154 }, /* 82 */
        {cCos        , 15 }, /* 83 */
        {cAtan       , 2 }, /* 84 */
        {cTan        , 15 }, /* 85 */
        {cAdd        , 160 }, /* 86 */
        {cAdd        , 166 }, /* 87 */
        {cAdd        , 169 }, /* 88 */
        {cMul        , 170 }, /* 89 */
        {cAdd        , 172 }, /* 90 */
        {cMul        , 173 }, /* 91 */
        {cAdd        , 177 }, /* 92 */
        {cMul        , 179 }, /* 93 */
        {cMul        , 181 }, /* 94 */
        {cAdd        , 183 }, /* 95 */
        {cMul        , 184 }, /* 96 */
        {cMul        , 186 }, /* 97 */
        {cPow        , 188 }, /* 98 */
        {cPow        , 190 }, /* 99 */
        {cPow        , 192 }, /* 100 */
        {cPow        , 194 }, /* 101 */
        {cLog        , 199 }, /* 102 */
        {cMul        , 201 }, /* 103 */
        {cLog        , 202 }, /* 104 */
        {cPow        , 204 }, /* 105 */
        {cCos        , 0 }, /* 106 */
        {cPow        , 205 }, /* 107 */
        {cMul        , 208 }, /* 108 */
        {cMul        , 209 }, /* 109 */
        {cMul        , 211 }, /* 110 */
        {cMul        , 212 }, /* 111 */
        {cAdd        , 213 }, /* 112 */
        {cMul        , 214 }, /* 113 */
        {cMul        , 216 }, /* 114 */
        {cMul        , 217 }, /* 115 */
        {cMul        , 219 }, /* 116 */
        {cMul        , 220 }, /* 117 */
        {cAdd        , 221 }, /* 118 */
        {cMul        , 222 }, /* 119 */
        {cMul        , 224 }, /* 120 */
        {cMul        , 225 }, /* 121 */
        {cMul        , 227 }, /* 122 */
        {cMul        , 228 }, /* 123 */
        {cAdd        , 229 }, /* 124 */
        {cMul        , 230 }, /* 125 */
        {cMul        , 232 }, /* 126 */
        {cMul        , 233 }, /* 127 */
        {cMul        , 235 }, /* 128 */
        {cMul        , 236 }, /* 129 */
        {cAdd        , 237 }, /* 130 */
        {cMul        , 238 }, /* 131 */
        {cCos        , 199 }, /* 132 */
        {cMul        , 240 }, /* 133 */
        {cSin        , 241 }, /* 134 */
        {cMul        , 242 }, /* 135 */
        {cAdd        , 244 }, /* 136 */
        {cCos        , 245 }, /* 137 */
        {cMul        , 247 }, /* 138 */
        {cSin        , 0 }, /* 139 */
        {cSin        , 25 }, /* 140 */
        {cMul        , 248 }, /* 141 */
        {cAdd        , 250 }, /* 142 */
        {cCos        , 251 }, /* 143 */
        {cCos        , 25 }, /* 144 */
        {cMul        , 253 }, /* 145 */
        {cSin        , 199 }, /* 146 */
        {cMul        , 254 }, /* 147 */
        {cAdd        , 256 }, /* 148 */
        {cSin        , 257 }, /* 149 */
        {cMul        , 259 }, /* 150 */
        {cCos        , 2 }, /* 151 */
        {cMul        , 260 }, /* 152 */
        {cAdd        , 262 }, /* 153 */
        {cSin        , 263 }, /* 154 */
        {cMul        , 265 }, /* 155 */
        {cMul        , 267 }, /* 156 */
        {cAdd        , 268 }, /* 157 */
        {cMul        , 269 }, /* 158 */
        {cMul        , 271 }, /* 159 */
        {cMul        , 273 }, /* 160 */
        {cAdd        , 274 }, /* 161 */
        {cMul        , 275 }, /* 162 */
        {cMul        , 277 }, /* 163 */
        {cMul        , 278 }, /* 164 */
        {cMul        , 280 }, /* 165 */
        {cMul        , 281 }, /* 166 */
        {cAdd        , 282 }, /* 167 */
        {cMul        , 283 }, /* 168 */
        {cMul        , 285 }, /* 169 */
        {cMul        , 286 }, /* 170 */
        {cMul        , 288 }, /* 171 */
        {cMul        , 289 }, /* 172 */
        {cAdd        , 290 }, /* 173 */
        {cMul        , 291 }, /* 174 */
        {cMul        , 293 }, /* 175 */
        {cAdd        , 295 }, /* 176 */
        {cCos        , 296 }, /* 177 */
        {cMul        , 298 }, /* 178 */
        {cAdd        , 300 }, /* 179 */
        {cCos        , 301 }, /* 180 */
        {cMul        , 303 }, /* 181 */
        {cMul        , 304 }, /* 182 */
        {cAdd        , 306 }, /* 183 */
        {cSin        , 307 }, /* 184 */
        {cMul        , 309 }, /* 185 */
        {cMul        , 311 }, /* 186 */
        {cAdd        , 312 }, /* 187 */
        {cMul        , 313 }, /* 188 */
        {cMul        , 315 }, /* 189 */
        {cMul        , 317 }, /* 190 */
        {cAdd        , 318 }, /* 191 */
        {cMul        , 319 }, /* 192 */
        {cMul        , 322 }, /* 193 */
        {cMul        , 325 }, /* 194 */
        {cPow        , 327 }, /* 195 */
        {cPow        , 328 }, /* 196 */
        {cMul        , 329 }, /* 197 */
        {cAdd        , 331 }, /* 198 */
        {cPow        , 332 }, /* 199 */
        {cMul        , 333 }, /* 200 */
        {cAdd        , 334 }, /* 201 */
        {cMul        , 335 }, /* 202 */
        {cAdd        , 338 }, /* 203 */
        {cTan        , 339 }, /* 204 */
        {cTan        , 0 }, /* 205 */
        {cMul        , 343 }, /* 206 */
        {cAdd        , 344 }, /* 207 */
        {cMul        , 347 }, /* 208 */
        {cPow        , 350 }, /* 209 */
        {cPow        , 352 }, /* 210 */
        {cMul        , 354 }, /* 211 */
        {cPow        , 361 }, /* 212 */
        {cLog        , 362 }, /* 213 */
        {cPow        , 364 }, /* 214 */
        {cLog        , 365 }, /* 215 */
        {cPow        , 367 }, /* 216 */
        {cLog        , 368 }, /* 217 */
        {cPow        , 370 }, /* 218 */
        {cMul        , 371 }, /* 219 */
        {cLog        , 372 }, /* 220 */
        {cMul        , 374 }, /* 221 */
        {cAdd        , 375 }, /* 222 */
        {cPow        , 377 }, /* 223 */
        {cMul        , 378 }, /* 224 */
        {cLog        , 379 }, /* 225 */
        {cAdd        , 381 }, /* 226 */
        {cPow        , 383 }, /* 227 */
        {cMul        , 384 }, /* 228 */
        {cLog        , 385 }, /* 229 */
        {cLog        , 386 }, /* 230 */
        {cLog        , 388 }, /* 231 */
        {cMul        , 389 }, /* 232 */
        {cAdd        , 390 }, /* 233 */
        {cPow        , 392 }, /* 234 */
        {cPow        , 393 }, /* 235 */
        {cAdd        , 395 }, /* 236 */
        {cPow        , 396 }, /* 237 */
        {cPow        , 398 }, /* 238 */
        {cPow        , 399 }, /* 239 */
        {cAdd        , 401 }, /* 240 */
        {cPow        , 402 }, /* 241 */
        {cTan        , 2 }, /* 242 */
        {cSinh       , 15 }, /* 243 */
        {cCosh       , 15 }, /* 244 */
        {cTanh       , 2 }, /* 245 */
        {cPow        , 408 }, /* 246 */
        {cAdd        , 410 }, /* 247 */
        {cPow        , 411 }, /* 248 */
        {cPow        , 413 }, /* 249 */
        {cAdd        , 415 }, /* 250 */
        {cPow        , 416 }, /* 251 */
        {cPow        , 420 }, /* 252 */
        {cLog        , 421 }, /* 253 */
        {cPow        , 424 }, /* 254 */
        {cLog        , 425 }, /* 255 */
        {cPow        , 428 }, /* 256 */
        {cPow        , 429 }, /* 257 */
        {cAdd        , 431 }, /* 258 */
        {cPow        , 432 }, /* 259 */
        {cSinh       , 2 }, /* 260 */
        {cCosh       , 2 }, /* 261 */
        {cTanh       , 0 }, /* 262 */
        {cPow        , 438 }, /* 263 */
        {cAdd        , 440 }, /* 264 */
        {cPow        , 441 }, /* 265 */
        {cPow        , 443 }, /* 266 */
        {cAdd        , 445 }, /* 267 */
        {cPow        , 446 }, /* 268 */
        {cPow        , 448 }, /* 269 */
        {cPow        , 450 }, /* 270 */
        {cEqual      , 461 }, /* 271 */
        {cNEqual     , 463 }, /* 272 */
        {cNEqual     , 465 }, /* 273 */
        {cEqual      , 467 }, /* 274 */
        {cLess       , 469 }, /* 275 */
        {cGreaterOrEq, 471 }, /* 276 */
        {cLessOrEq   , 473 }, /* 277 */
        {cGreater    , 475 }, /* 278 */
        {cGreater    , 477 }, /* 279 */
        {cLessOrEq   , 479 }, /* 280 */
        {cGreaterOrEq, 481 }, /* 281 */
        {cLess       , 483 }, /* 282 */
        {cOr         , 486 }, /* 283 */
        {cNotNot     , 15 }, /* 284 */
        {cAnd        , 491 }, /* 285 */
        {cEqual      , 495 }, /* 286 */
        {cNEqual     , 497 }, /* 287 */
        {cNEqual     , 499 }, /* 288 */
        {cEqual      , 501 }, /* 289 */
        {cLess       , 503 }, /* 290 */
        {cGreaterOrEq, 505 }, /* 291 */
        {cLessOrEq   , 507 }, /* 292 */
        {cGreater    , 509 }, /* 293 */
        {cGreater    , 511 }, /* 294 */
        {cLessOrEq   , 513 }, /* 295 */
        {cGreaterOrEq, 515 }, /* 296 */
        {cLess       , 517 }, /* 297 */
        {cNot        , 15 }, /* 298 */
        {cAnd        , 520 }, /* 299 */
        {cEqual      , 526 }, /* 300 */
        {cNEqual     , 527 }, /* 301 */
        {cEqual      , 530 }, /* 302 */
        {cEqual      , 531 }, /* 303 */
        {cEqual      , 532 }, /* 304 */
        {cEqual      , 534 }, /* 305 */
        {cEqual      , 535 }, /* 306 */
        {cAnd        , 538 }, /* 307 */
        {cOr         , 543 }, /* 308 */
        {cEqual      , 547 }, /* 309 */
        {cNEqual     , 549 }, /* 310 */
        {cNEqual     , 551 }, /* 311 */
        {cEqual      , 553 }, /* 312 */
        {cLess       , 555 }, /* 313 */
        {cGreaterOrEq, 557 }, /* 314 */
        {cLessOrEq   , 559 }, /* 315 */
        {cGreater    , 561 }, /* 316 */
        {cGreater    , 563 }, /* 317 */
        {cLessOrEq   , 565 }, /* 318 */
        {cGreaterOrEq, 567 }, /* 319 */
        {cLess       , 569 }, /* 320 */
        {cNot        , 2 }, /* 321 */
        {cOr         , 572 }, /* 322 */
        {cEqual      , 577 }, /* 323 */
        {cNEqual     , 578 }, /* 324 */
        {cEqual      , 581 }, /* 325 */
        {cEqual      , 583 }, /* 326 */
        {cNEqual     , 585 }, /* 327 */
        {cNEqual     , 587 }, /* 328 */
        {cLess       , 589 }, /* 329 */
        {cLess       , 591 }, /* 330 */
        {cLessOrEq   , 593 }, /* 331 */
        {cLessOrEq   , 595 }, /* 332 */
        {cGreater    , 597 }, /* 333 */
        {cGreater    , 599 }, /* 334 */
        {cGreaterOrEq, 601 }, /* 335 */
        {cGreaterOrEq, 603 }, /* 336 */
        {cNot        , 488 }, /* 337 */
        {cAnd        , 606 }, /* 338 */
        {cAnd        , 608 }, /* 339 */
        {cOr         , 610 }, /* 340 */
        {cOr         , 612 }, /* 341 */
        {cNotNot     , 0 }, /* 342 */
        {cExp        , 0 }, /* 343 */
        {cSqrt       , 15 }, /* 344 */
        {cMul        , 620 }, /* 345 */
        {cRad        , 621 }, /* 346 */
        {cMul        , 624 }, /* 347 */
        {cDeg        , 625 }, /* 348 */
        {cSec        , 0 }, /* 349 */
        {cCsc        , 15 }, /* 350 */
        {cCot        , 0 }, /* 351 */
        {cLog10      , 0 }, /* 352 */
        {cLog2       , 0 }, /* 353 */
        {cNot        , 641 }, /* 354 */
    };

    const Rule rlist[] =
    {
        {1, ProduceNewTree,    3,	{ cNot        , 1 } }, /* 0 */
        {1, ProduceNewTree,    5,	{ cAcos       , 4 } }, /* 1 */
        {1, ProduceNewTree,    7,	{ cAcosh      , 6 } }, /* 2 */
        {1, ProduceNewTree,    9,	{ cAsin       , 8 } }, /* 3 */
        {1, ProduceNewTree,    10,	{ cAsinh      , 8 } }, /* 4 */
        {1, ProduceNewTree,    11,	{ cAtan       , 6 } }, /* 5 */
        {1, ProduceNewTree,    12,	{ cAtanh      , 8 } }, /* 6 */
        {1, ProduceNewTree,    13,	{ cCeil       , 8 } }, /* 7 */
        {1, ProduceNewTree,    14,	{ cCos        , 6 } }, /* 8 */
        {1, ProduceNewTree,    15,	{ cCos        , 16 } }, /* 9 */
        {1, ReplaceParams ,    0,	{ cCos        , 18 } }, /* 10 */
        {1, ProduceNewTree,    21,	{ cCos        , 20 } }, /* 11 */
        {1, ProduceNewTree,    22,	{ cCosh       , 4 } }, /* 12 */
        {1, ProduceNewTree,    23,	{ cFloor      , 8 } }, /* 13 */
        {3, ProduceNewTree,    25,	{ cIf         , 24 } }, /* 14 */
        {3, ProduceNewTree,    0,	{ cIf         , 26 } }, /* 15 */
        {3, ProduceNewTree,    15,	{ cIf         , 27 } }, /* 16 */
        {3, ProduceNewTree,    33,	{ cIf         , 29 } }, /* 17 */
        {3, ProduceNewTree,    39,	{ cIf         , 35 } }, /* 18 */
        {3, ProduceNewTree,    45,	{ cIf         , 41 } }, /* 19 */
        {3, ProduceNewTree,    51,	{ cIf         , 47 } }, /* 20 */
        {3, ProduceNewTree,    59,	{ cIf         , 54 } }, /* 21 */
        {3, ProduceNewTree,    65,	{ cIf         , 62 } }, /* 22 */
        {3, ProduceNewTree,    71,	{ cIf         , 68 } }, /* 23 */
        {3, ProduceNewTree,    79,	{ cIf         , 74 } }, /* 24 */
        {3, ProduceNewTree,    83,	{ cIf         , 81 } }, /* 25 */
        {3, ProduceNewTree,    87,	{ cIf         , 85 } }, /* 26 */
        {1, ProduceNewTree,    89,	{ cLog        , 88 } }, /* 27 */
        {1, ProduceNewTree,    94,	{ cLog        , 91 } }, /* 28 */
        {1, ProduceNewTree,    101,	{ cLog        , 97 } }, /* 29 */
        {1, ProduceNewTree,    15,	{ cMax        , 15 } }, /* 30 */
        {1, ReplaceParams ,    104,	{ cMax        , 103 } }, /* 31 */
        {2, ReplaceParams ,    106,	{ cMax        , 105 } }, /* 32 */
        {2, ReplaceParams ,    15,	{ cMax        , 107 } }, /* 33 */
        {1, ProduceNewTree,    15,	{ cMin        , 15 } }, /* 34 */
        {1, ReplaceParams ,    104,	{ cMin        , 108 } }, /* 35 */
        {2, ReplaceParams ,    110,	{ cMin        , 109 } }, /* 36 */
        {2, ReplaceParams ,    15,	{ cMin        , 111 } }, /* 37 */
        {2, ProduceNewTree,    113,	{ cPow        , 112 } }, /* 38 */
        {2, ProduceNewTree,    115,	{ cPow        , 114 } }, /* 39 */
        {2, ProduceNewTree,    2,	{ cPow        , 116 } }, /* 40 */
        {2, ReplaceParams ,    120,	{ cPow        , 118 } }, /* 41 */
        {2, ProduceNewTree,    122,	{ cPow        , 121 } }, /* 42 */
        {2, ReplaceParams ,    126,	{ cPow        , 124 } }, /* 43 */
        {2, ReplaceParams ,    130,	{ cPow        , 128 } }, /* 44 */
        {2, ProduceNewTree,    132,	{ cPow        , 131 } }, /* 45 */
        {2, ProduceNewTree,    115,	{ cPow        , 133 } }, /* 46 */
        {2, ProduceNewTree,    0,	{ cPow        , 134 } }, /* 47 */
        {2, ProduceNewTree,    140,	{ cPow        , 136 } }, /* 48 */
        {2, ReplaceParams ,    145,	{ cPow        , 143 } }, /* 49 */
        {2, ReplaceParams ,    149,	{ cPow        , 147 } }, /* 50 */
        {1, ProduceNewTree,    150,	{ cSin        , 8 } }, /* 51 */
        {1, ProduceNewTree,    15,	{ cSin        , 151 } }, /* 52 */
        {1, ProduceNewTree,    153,	{ cSin        , 18 } }, /* 53 */
        {1, ProduceNewTree,    156,	{ cSin        , 155 } }, /* 54 */
        {1, ProduceNewTree,    157,	{ cSinh       , 6 } }, /* 55 */
        {1, ProduceNewTree,    158,	{ cTan        , 8 } }, /* 56 */
        {1, ProduceNewTree,    15,	{ cTan        , 159 } }, /* 57 */
        {1, ProduceNewTree,    161,	{ cTan        , 18 } }, /* 58 */
        {1, ProduceNewTree,    162,	{ cTanh       , 4 } }, /* 59 */
        {0, ProduceNewTree,    164,	{ cAdd        , 163 } }, /* 60 */
        {1, ProduceNewTree,    15,	{ cAdd        , 15 } }, /* 61 */
        {1, ReplaceParams ,    163,	{ cAdd        , 165 } }, /* 62 */
        {1, ReplaceParams ,    168,	{ cAdd        , 167 } }, /* 63 */
        {1, ReplaceParams ,    174,	{ cAdd        , 171 } }, /* 64 */
        {1, ReplaceParams ,    176,	{ cAdd        , 175 } }, /* 65 */
        {1, ReplaceParams ,    137,	{ cAdd        , 178 } }, /* 66 */
        {1, ReplaceParams ,    182,	{ cAdd        , 180 } }, /* 67 */
        {1, ReplaceParams ,    187,	{ cAdd        , 185 } }, /* 68 */
        {2, ReplaceParams ,    191,	{ cAdd        , 189 } }, /* 69 */
        {2, ReplaceParams ,    195,	{ cAdd        , 193 } }, /* 70 */
        {2, ReplaceParams ,    197,	{ cAdd        , 196 } }, /* 71 */
        {2, ReplaceParams ,    163,	{ cAdd        , 198 } }, /* 72 */
        {2, ReplaceParams ,    203,	{ cAdd        , 200 } }, /* 73 */
        {2, ReplaceParams ,    207,	{ cAdd        , 206 } }, /* 74 */
        {2, ReplaceParams ,    215,	{ cAdd        , 210 } }, /* 75 */
        {2, ReplaceParams ,    223,	{ cAdd        , 218 } }, /* 76 */
        {2, ReplaceParams ,    231,	{ cAdd        , 226 } }, /* 77 */
        {2, ReplaceParams ,    239,	{ cAdd        , 234 } }, /* 78 */
        {2, ReplaceParams ,    246,	{ cAdd        , 243 } }, /* 79 */
        {2, ReplaceParams ,    252,	{ cAdd        , 249 } }, /* 80 */
        {2, ReplaceParams ,    258,	{ cAdd        , 255 } }, /* 81 */
        {2, ReplaceParams ,    264,	{ cAdd        , 261 } }, /* 82 */
        {2, ReplaceParams ,    270,	{ cAdd        , 266 } }, /* 83 */
        {2, ReplaceParams ,    276,	{ cAdd        , 272 } }, /* 84 */
        {2, ReplaceParams ,    284,	{ cAdd        , 279 } }, /* 85 */
        {2, ReplaceParams ,    292,	{ cAdd        , 287 } }, /* 86 */
        {2, ReplaceParams ,    297,	{ cAdd        , 294 } }, /* 87 */
        {2, ReplaceParams ,    302,	{ cAdd        , 299 } }, /* 88 */
        {2, ReplaceParams ,    308,	{ cAdd        , 305 } }, /* 89 */
        {2, ReplaceParams ,    314,	{ cAdd        , 310 } }, /* 90 */
        {2, ReplaceParams ,    320,	{ cAdd        , 316 } }, /* 91 */
        {2, ReplaceParams ,    323,	{ cAdd        , 321 } }, /* 92 */
        {2, ReplaceParams ,    326,	{ cAdd        , 324 } }, /* 93 */
        {3, ReplaceParams ,    336,	{ cAdd        , 330 } }, /* 94 */
        {0, ProduceNewTree,    337,	{ cMul        , 163 } }, /* 95 */
        {1, ProduceNewTree,    0,	{ cMul        , 0 } }, /* 96 */
        {1, ProduceNewTree,    341,	{ cMul        , 340 } }, /* 97 */
        {1, ProduceNewTree,    345,	{ cMul        , 342 } }, /* 98 */
        {1, ProduceNewTree,    164,	{ cMul        , 165 } }, /* 99 */
        {1, ReplaceParams ,    163,	{ cMul        , 346 } }, /* 100 */
        {1, ReplaceParams ,    349,	{ cMul        , 348 } }, /* 101 */
        {1, ReplaceParams ,    353,	{ cMul        , 351 } }, /* 102 */
        {1, ReplaceParams ,    137,	{ cMul        , 355 } }, /* 103 */
        {2, ReplaceParams ,    357,	{ cMul        , 356 } }, /* 104 */
        {2, ReplaceParams ,    359,	{ cMul        , 358 } }, /* 105 */
        {2, ReplaceParams ,    163,	{ cMul        , 360 } }, /* 106 */
        {2, ReplaceParams ,    199,	{ cMul        , 363 } }, /* 107 */
        {2, ReplaceParams ,    241,	{ cMul        , 366 } }, /* 108 */
        {2, ReplaceParams ,    25,	{ cMul        , 369 } }, /* 109 */
        {2, ProduceNewTree,    376,	{ cMul        , 373 } }, /* 110 */
        {2, ProduceNewTree,    382,	{ cMul        , 380 } }, /* 111 */
        {2, ProduceNewTree,    391,	{ cMul        , 387 } }, /* 112 */
        {2, ReplaceParams ,    397,	{ cMul        , 394 } }, /* 113 */
        {2, ReplaceParams ,    403,	{ cMul        , 400 } }, /* 114 */
        {2, ReplaceParams ,    405,	{ cMul        , 404 } }, /* 115 */
        {2, ReplaceParams ,    407,	{ cMul        , 406 } }, /* 116 */
        {2, ReplaceParams ,    412,	{ cMul        , 409 } }, /* 117 */
        {2, ReplaceParams ,    417,	{ cMul        , 414 } }, /* 118 */
        {2, ReplaceParams ,    419,	{ cMul        , 418 } }, /* 119 */
        {2, ReplaceParams ,    423,	{ cMul        , 422 } }, /* 120 */
        {2, ReplaceParams ,    427,	{ cMul        , 426 } }, /* 121 */
        {2, ReplaceParams ,    433,	{ cMul        , 430 } }, /* 122 */
        {2, ReplaceParams ,    435,	{ cMul        , 434 } }, /* 123 */
        {2, ReplaceParams ,    437,	{ cMul        , 436 } }, /* 124 */
        {2, ReplaceParams ,    442,	{ cMul        , 439 } }, /* 125 */
        {2, ReplaceParams ,    447,	{ cMul        , 444 } }, /* 126 */
        {2, ReplaceParams ,    449,	{ cMul        , 321 } }, /* 127 */
        {2, ReplaceParams ,    451,	{ cMul        , 324 } }, /* 128 */
        {2, ProduceNewTree,    453,	{ cMod        , 452 } }, /* 129 */
        {2, ProduceNewTree,    207,	{ cEqual      , 454 } }, /* 130 */
        {2, ProduceNewTree,    164,	{ cNEqual     , 455 } }, /* 131 */
        {2, ProduceNewTree,    457,	{ cLess       , 456 } }, /* 132 */
        {2, ProduceNewTree,    207,	{ cLessOrEq   , 458 } }, /* 133 */
        {2, ProduceNewTree,    164,	{ cGreater    , 459 } }, /* 134 */
        {2, ProduceNewTree,    207,	{ cGreaterOrEq, 460 } }, /* 135 */
        {1, ProduceNewTree,    464,	{ cNot        , 462 } }, /* 136 */
        {1, ProduceNewTree,    468,	{ cNot        , 466 } }, /* 137 */
        {1, ProduceNewTree,    472,	{ cNot        , 470 } }, /* 138 */
        {1, ProduceNewTree,    476,	{ cNot        , 474 } }, /* 139 */
        {1, ProduceNewTree,    480,	{ cNot        , 478 } }, /* 140 */
        {1, ProduceNewTree,    484,	{ cNot        , 482 } }, /* 141 */
        {0, ProduceNewTree,    164,	{ cAnd        , 163 } }, /* 142 */
        {0, ReplaceParams ,    487,	{ cAnd        , 485 } }, /* 143 */
        {1, ProduceNewTree,    488,	{ cAnd        , 15 } }, /* 144 */
        {1, ProduceNewTree,    1,	{ cAnd        , 489 } }, /* 145 */
        {1, ReplaceParams ,    489,	{ cAnd        , 490 } }, /* 146 */
        {1, ReplaceParams ,    493,	{ cAnd        , 492 } }, /* 147 */
        {1, ReplaceParams ,    15,	{ cAnd        , 494 } }, /* 148 */
        {1, ReplaceParams ,    498,	{ cAnd        , 496 } }, /* 149 */
        {1, ReplaceParams ,    502,	{ cAnd        , 500 } }, /* 150 */
        {1, ReplaceParams ,    506,	{ cAnd        , 504 } }, /* 151 */
        {1, ReplaceParams ,    510,	{ cAnd        , 508 } }, /* 152 */
        {1, ReplaceParams ,    514,	{ cAnd        , 512 } }, /* 153 */
        {1, ReplaceParams ,    518,	{ cAnd        , 516 } }, /* 154 */
        {1, ReplaceParams ,    0,	{ cAnd        , 519 } }, /* 155 */
        {1, ReplaceParams ,    522,	{ cAnd        , 521 } }, /* 156 */
        {1, ReplaceParams ,    17,	{ cAnd        , 523 } }, /* 157 */
        {2, ReplaceParams ,    2,	{ cAnd        , 524 } }, /* 158 */
        {2, ProduceNewTree,    113,	{ cAnd        , 525 } }, /* 159 */
        {2, ProduceNewTree,    113,	{ cAnd        , 528 } }, /* 160 */
        {2, ReplaceParams ,    489,	{ cAnd        , 529 } }, /* 161 */
        {3, ReplaceParams ,    536,	{ cAnd        , 533 } }, /* 162 */
        {0, ProduceNewTree,    115,	{ cOr         , 163 } }, /* 163 */
        {0, ReplaceParams ,    539,	{ cOr         , 537 } }, /* 164 */
        {1, ProduceNewTree,    540,	{ cOr         , 2 } }, /* 165 */
        {1, ProduceNewTree,    541,	{ cOr         , 17 } }, /* 166 */
        {1, ReplaceParams ,    17,	{ cOr         , 542 } }, /* 167 */
        {1, ReplaceParams ,    545,	{ cOr         , 544 } }, /* 168 */
        {1, ReplaceParams ,    2,	{ cOr         , 546 } }, /* 169 */
        {1, ReplaceParams ,    550,	{ cOr         , 548 } }, /* 170 */
        {1, ReplaceParams ,    554,	{ cOr         , 552 } }, /* 171 */
        {1, ReplaceParams ,    558,	{ cOr         , 556 } }, /* 172 */
        {1, ReplaceParams ,    562,	{ cOr         , 560 } }, /* 173 */
        {1, ReplaceParams ,    566,	{ cOr         , 564 } }, /* 174 */
        {1, ReplaceParams ,    570,	{ cOr         , 568 } }, /* 175 */
        {1, ReplaceParams ,    15,	{ cOr         , 571 } }, /* 176 */
        {1, ReplaceParams ,    574,	{ cOr         , 573 } }, /* 177 */
        {1, ReplaceParams ,    17,	{ cOr         , 523 } }, /* 178 */
        {2, ReplaceParams ,    15,	{ cOr         , 575 } }, /* 179 */
        {2, ProduceNewTree,    337,	{ cOr         , 576 } }, /* 180 */
        {2, ProduceNewTree,    337,	{ cOr         , 579 } }, /* 181 */
        {2, ReplaceParams ,    489,	{ cOr         , 580 } }, /* 182 */
        {1, ProduceNewTree,    584,	{ cNotNot     , 582 } }, /* 183 */
        {1, ProduceNewTree,    588,	{ cNotNot     , 586 } }, /* 184 */
        {1, ProduceNewTree,    592,	{ cNotNot     , 590 } }, /* 185 */
        {1, ProduceNewTree,    596,	{ cNotNot     , 594 } }, /* 186 */
        {1, ProduceNewTree,    600,	{ cNotNot     , 598 } }, /* 187 */
        {1, ProduceNewTree,    604,	{ cNotNot     , 602 } }, /* 188 */
        {1, ProduceNewTree,    605,	{ cNotNot     , 541 } }, /* 189 */
        {1, ProduceNewTree,    609,	{ cNotNot     , 607 } }, /* 190 */
        {1, ProduceNewTree,    613,	{ cNotNot     , 611 } }, /* 191 */
        {1, ReplaceParams ,    2,	{ cNotNot     , 614 } }, /* 192 */
        {2, ProduceNewTree,    616,	{ cPow        , 615 } }, /* 193 */
        {2, ProduceNewTree,    618,	{ cPow        , 617 } }, /* 194 */
        {1, ProduceNewTree,    622,	{ cMul        , 619 } }, /* 195 */
        {1, ProduceNewTree,    626,	{ cMul        , 623 } }, /* 196 */
        {1, ReplaceParams ,    628,	{ cMul        , 627 } }, /* 197 */
        {1, ReplaceParams ,    630,	{ cMul        , 629 } }, /* 198 */
        {1, ReplaceParams ,    632,	{ cMul        , 631 } }, /* 199 */
        {2, ReplaceParams ,    634,	{ cMul        , 633 } }, /* 200 */
        {2, ReplaceParams ,    636,	{ cMul        , 635 } }, /* 201 */
        {2, ReplaceParams ,    638,	{ cMul        , 637 } }, /* 202 */
        {2, ReplaceParams ,    640,	{ cMul        , 639 } }, /* 203 */
        {1, ProduceNewTree,    642,	{ cNotNot     , 2 } }, /* 204 */
    };
}

namespace FPoptimizer_Grammar
{
    const GrammarPack pack =
    {
        clist, plist, mlist, flist, rlist,
        {
            {0, 1 }, /* 0 */
            {1, 192 }, /* 1 */
            {193, 12 }, /* 2 */
        }
    };
}

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
            std::cout << "\n";
        }
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
            std::cout << "\n";
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

        MatchedParams::CodeTreeMatch matchrec;
        if(input.opcode == tree.Opcode
        && pack.mlist[input.index].Match(tree, matchrec, false))
        {
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
                    std::cout << "\n";
#endif
                    return true;
                case ProduceNewTree:
                    repl.ReplaceTree(tree,   params, matchrec);
#ifdef DEBUG_SUBSTITUTIONS
                    std::cout << "  TreeReplace: ";
                    DumpTree(tree);
                    std::cout << "\n";
#endif
                    return true;
            }
        }
        else
        {
            //DumpMatch(input, tree, repl, matchrec, false);
        }
        return false;
    }


    /* Match the given function to the given CodeTree.
     */
    bool Function::Match(
        FPoptimizer_CodeTree::CodeTree& tree,
        MatchedParams::CodeTreeMatch& match) const
    {
        return opcode == tree.Opcode
            && pack.mlist[index].Match(tree, match);
    }


    /* This struct is used by MatchedParams::Match() for backtracking. */
    struct ParamMatchSnapshot
    {
        MatchedParams::CodeTreeMatch snapshot;
                                    // Snapshot of the state so far
        size_t            parampos; // Which position was last chosen?
        std::vector<bool> used;     // Which params were allocated?
    };

    /* Match the given list of ParamSpecs using the given ParamMatchingType
     * to the given CodeTree.
     * The CodeTree is already assumed to be a function type
     * -- i.e. it is assumed that the caller has tested the Opcode of the tree.
     */
    bool MatchedParams::Match(
        FPoptimizer_CodeTree::CodeTree& tree,
        MatchedParams::CodeTreeMatch& match,
        bool recursion) const
    {
        /* FIXME: This algorithm still does not cover the possibility
         *        that the ParamSpec needs backtracking.
         *
         *        For example,
         *          cMul (cAdd x) (cAdd x)
         *        Applied to:
         *          (a+b)*(c+b)
         *
         *        Match (cAdd x) to (a+b) may first capture "a" into "x",
         *        and then Match(cAdd x) for (c+b) will fail,
         *        because there's no "a" there.
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
        if(tree.Params.size() < minimum_need) return false;

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
            // Something came short.
            return false;
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
                return false;
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
                for(unsigned a=0; a<count; ++a)
                {
                    const ParamSpec& param = pack.plist[index+a];
                    if(!param.Match(*tree.Params[a].param, match, tree.Params[a].sign ? transf : None))
                    {
                        /*std::cout << " drats at " << a << "!\n";*/
                        return false;
                    }
                    if(!recursion)
                        match.param_numbers.push_back(a);
                }
                /*std::cout << " yay?\n";*/
                // Match = no mismatch.
                return true;
            }
            case AnyParams:
            case SelectedParams:
            {
                const size_t n_tree_params = tree.Params.size();

                bool HasRestHolders = false;
                for(unsigned a=0; a<count; ++a)
                {
                    const ParamSpec& param = pack.plist[index+a];
                    if(param.opcode == RestHolder) { HasRestHolders = true; break; }
                }

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
                    return false;
                }

                std::vector<ParamMatchSnapshot> position(count);
                std::vector<bool>               used(n_tree_params);

                for(unsigned a=0; a<count; ++a)
                {
                    position[a].snapshot  = match;
                    position[a].parampos  = 0;
                    position[a].used      = used;

                    size_t b = 0;
                backtrack:
                    const ParamSpec& param = pack.plist[index+a];

                    if(param.opcode == RestHolder)
                    {
                        // RestHolders always match. They're filled afterwards.
                        continue;
                    }

                    for(; b<n_tree_params; ++b)
                    {
                        if(!used[b])
                        {
                            /*std::cout << "Maybe [" << a << "]:";
                            DumpParam(param);
                            std::cout << " <-> ";
                            if(tree.Params[b].sign) std::cout << '~';
                            DumpTree(*tree.Params[b].param);
                            std::cout << "...?\n";*/

                            if(param.Match(*tree.Params[b].param, match, tree.Params[b].sign ? transf : None))
                            {
                                /*std::cout << "woo... " << a << ", " << b << "\n";*/
                                /* NamedHolders require a special treatment,
                                 * because a repetition count may be issued
                                 * for them.
                                 */
                                if(param.opcode == NamedHolder)
                                {
                                    // Verify the MinRepeat & AnyRepeat case
                                    unsigned MinRepeat = param.minrepeat;
                                    bool AnyRepeat     = param.anyrepeat;
                                    unsigned HadRepeat = 1;

                                    for(size_t c = b+1;
                                        c < n_tree_params && (HadRepeat < MinRepeat || AnyRepeat);
                                        ++c)
                                    {
                                        if(tree.Params[c].param->Hash == tree.Params[b].param->Hash
                                        && tree.Params[c].sign == tree.Params[b].sign)
                                        {
                                            ++HadRepeat;
                                        }
                                    }
                                    if(HadRepeat < MinRepeat)
                                        continue; // No sufficient repeat count here

                                    used[b] = true;
                                    if(!recursion) match.param_numbers.push_back(b);

                                    HadRepeat = 1;
                                    for(size_t c = b+1;
                                        c < n_tree_params && (HadRepeat < MinRepeat || AnyRepeat);
                                        ++c)
                                    {
                                        if(tree.Params[c].param->Hash == tree.Params[b].param->Hash
                                        && tree.Params[c].sign == tree.Params[b].sign)
                                        {
                                            ++HadRepeat;
                                            used[c] = true;
                                            if(!recursion) match.param_numbers.push_back(c);
                                        }
                                    }
                                    if(AnyRepeat)
                                        match.NamedMap[param.index].second = HadRepeat;
                                    position[a].parampos = b+1;
                                    goto ok;
                                }

                                used[b] = true;
                                if(!recursion) match.param_numbers.push_back(b);
                                position[a].parampos = b+1;
                                goto ok;
                            }
                        }
                    }

                    /*DumpParam(param);
                    std::cout << " didn't match anything in ";
                    DumpTree(tree);
                    std::cout << "\n";*/

                    // No match for this param, try backtracking.
                    while(a > 0)
                    {
                        --a;
                        ParamMatchSnapshot& prevpos = position[a];
                        if(prevpos.parampos < n_tree_params)
                        {
                            // Try another combination.
                            b     = prevpos.parampos;
                            match = prevpos.snapshot;
                            used  = prevpos.used;
                            goto backtrack;
                        }
                    }
                    // If we cannot backtrack, break. No possible match.
                    /*if(!recursion)
                        std::cout << "Drats!\n";*/
                    return false;
                ok:;
                    /*if(!recursion)
                        std::cout << "Match for param " << a << " at " << b << std::endl;*/
                }
                // Match = no mismatch.

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
                            if(n_neg_restholdings <= n_pos_restholdings) return false;
                            break;
                        case BalanceMorePos:
                            if(n_pos_restholdings <= n_neg_restholdings) return false;
                            break;
                        case BalanceEqual:
                            if(n_pos_restholdings != n_neg_restholdings) return false;
                            break;
                        case BalanceDontCare: ;
                    }
                }

                // Now feed any possible RestHolders the remaining parameters.
                for(unsigned a=0; a<count; ++a)
                {
                    const ParamSpec& param = pack.plist[index+a];
                    if(param.opcode == RestHolder)
                    {
                        std::vector<fphash_t>& RestList
                            = match.RestMap[param.index]; // mark it up

                        for(size_t b=0; b<n_tree_params; ++b)
                            if(tree.Params[b].sign == param.sign && !used[b])
                            {
                                if(!recursion)
                                    match.param_numbers.push_back(b);
                                fphash_t hash = tree.Params[b].param->Hash;
                                RestList.push_back(hash);
                                match.trees.insert(
                                    std::make_pair(hash, tree.Params[b].param) );
                            }
                    }
                }
                return true;
            }
        }
        return false;
    }

    bool ParamSpec::Match(
        FPoptimizer_CodeTree::CodeTree& tree,
        MatchedParams::CodeTreeMatch& match,
        TransformationType transf) const
    {
        assert(opcode != RestHolder); // RestHolders are supposed to be handler by the caller

        switch(OpcodeType(opcode))
        {
            case NumConstant:
            {
                if(!tree.IsImmed()) return false;
                double res = tree.GetImmed();
                if(transformation == Negate) res = -res;
                if(transformation == Invert) res = 1/res;
                double res2 = GetPackConst(index);
                if(transf == Negate) res2 = -res2;
                if(transf == Invert) res2 = 1/res2;
                if(transf == NotThe) res2 = res2 != 0;
                if(res != res2) return false;
                return true;
            }
            case ImmedHolder:
            {
                if(!tree.IsImmed()) return false;
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
                    return res == res2;
                }
                if(sign != (transf != None)) return false;

                match.ImmedMap.insert(i, std::make_pair((unsigned)index, res));
                return true;
            }
            case NamedHolder:
            {
                if(sign != (transf != None)) return false;
                std::map<unsigned, std::pair<fphash_t, size_t> >::iterator
                    i = match.NamedMap.lower_bound(index);
                if(i != match.NamedMap.end() && i->first == index)
                {
                    return tree.Hash == i->second.first;
                }
                match.NamedMap.insert(i, std::make_pair(index, std::make_pair(tree.Hash, 1)));
                match.trees.insert(std::make_pair(tree.Hash, &tree));
                return true;
            }
            case RestHolder:
                break;
            case SubFunction:
            {
                if(sign != (transf != None)) return false;
                return pack.flist[index].Match(tree, match);
            }
            default:
            {
                if(!tree.IsImmed()) return false;
                double res = tree.GetImmed();
                if(transformation == Negate) res = -res;
                if(transformation == Invert) res = 1/res;
                double res2;
                if(!GetConst(match, res2)) return false;
                if(transf == Negate) res2 = -res2;
                if(transf == Invert) res2 = 1/res2;
                if(transf == NotThe) res2 = res2 != 0;
                return res == res2;
            }
        }
        return false;
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
                result = i->second.second;
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
        switch(ParamMatchingType(opcode))
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
    if(isOptimized) return;
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
        data->StackSize = stacktop_max;
        data->Stack.resize(stacktop_max);
    }

    data->ByteCode.swap(byteCode);
    data->Immed.swap(immed);

    //PrintByteCode(std::cout);

    isOptimized = true;
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
        ByteCode.push_back(targetpos);
        ByteCode.push_back(srcpos);

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
            ByteCode.push_back(src_pos);
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

        ByteCode[ofs+1] = ByteCode.size()+2;
        ByteCode[ofs+2] = Immed.size();

        ofs = ByteCode.size();
        ByteCode.push_back(cJump);
        ByteCode.push_back(0); // code index
        ByteCode.push_back(0); // Immed index
    }
    void SynthIfStep3(size_t& ofs)
    {
        SetStackTop(StackTop-1); // ignore the pushed else-branch result.

        ByteCode[ofs+1] = ByteCode.size()-1;
        ByteCode[ofs+2] = Immed.size();

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
            cache[value] = stackpos;
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
                synth.AddOperation(Opcode, Params.size());
                synth.AddOperation(Funcno, 0, 0);
                break;
            }
            case cPCall:
            {
                // If the parameter count is invalid, we're screwed.
                for(size_t a=0; a<Params.size(); ++a)
                    Params[a].param->SynthesizeByteCode(synth);
                synth.AddOperation(Opcode, Params.size());
                synth.AddOperation(Funcno, 0, 0);
                break;
            }
            default:
            {
                // If the parameter count is invalid, we're screwed.
                for(size_t a=0; a<Params.size(); ++a)
                    Params[a].param->SynthesizeByteCode(synth);
                synth.AddOperation(Opcode, Params.size());
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
            stack.push_back(stack.back()->Clone());
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
                    case cInv:
                        data.Eat(1, cMul); // Unary division is inverse multiplying
                        data.SetLastOpParamSign(0);
                        break;
                    case cNeg:
                        data.Eat(1, cAdd); // Unary minus is negative adding.
                        data.SetLastOpParamSign(0);
                        break;
                    case cSqr:
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
                    case cRSub:
                        data.Eat(2, cAdd);
                        data.SetLastOpParamSign(0); // negate param0 instead of param1
                        break;
                    case cDiv:
                        data.Eat(2, cMul); // Divide is inverse multiply
                        data.SetLastOpParamSign(1);
                        break;
                    case cRDiv:
                        data.Eat(2, cMul);
                        data.SetLastOpParamSign(0); // invert param0 instead of param1
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
                        data.Eat(1, OPCODE(opcode));
                        break;
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
