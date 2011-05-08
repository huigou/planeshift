/***************************************************************************\
|* Function Parser for C++ v4.4                                            *|
|*-------------------------------------------------------------------------*|
|* Function optimizer                                                      *|
|*-------------------------------------------------------------------------*|
|* Copyright: Joel Yliluoma                                                *|
|*                                                                         *|
|* This library is distributed under the terms of the                      *|
|* GNU Lesser General Public License version 3.                            *|
|* (See lgpl.txt and gpl.txt for the license text.)                        *|
\***************************************************************************/

/* NOTE:
 This file contains generated code (from the optimizer sources) and is
 not intended to be modified by hand. If you want to modify the optimizer,
 download the development version of the library.
*/

#include "fpconfig.h"
#ifdef FP_SUPPORT_OPTIMIZER
#include "fparser.h"
#include "fptypes.h"
#include "fpaux.h"
#define iJ3 x22)l12
#define iI3 info.SaveMatchedParamIndex(
#define iH3 tree yI
#define iG3 &*n21){
#define iF3 cAbsIf
#define iE3 "Found "
#define iD3 stackpos
#define iC3 "%d, cost"
#define iB3 "dup(%u) "
#define iA3 t2{assert
#define i93 "immed "<<
#define i83 mFuncParsers
#define i73 param.data
#define i63 ;DumpTree(
#define i53 stderr
#define i43 sep2=" "
#define i33 GetOpcode
#define i23 FPHASH_CONST
#define i13 cache_needed[
#define i03 "PUSH "i63
#define tZ3 fprintf
#define tY3 ::cout<<"Applying "
#define tX3 HANDLE_UNARY_CONST_FUNC
#define tW3 within,
#define tV3 c_count
#define tU3 s_count
#define tT3 MaxOp
#define tS3 3)lT 3*
#define tR3 2)lT 2*
#define tQ3 p0 eT&&
#define tP3 PlusInf
#define tO3 .nN 1));
#define tN3 lE m.
#define tM3 max.val
#define tL3 sim.xC1
#define tK3 .empty()
#define tJ3 t71 xX=
#define tI3 .Immeds
#define tH3 b.Value)
#define tG3 b.Opcode
#define tF3 tO2 first
#define tE3 .data.
#define tD3 ].swap(
#define tC3 codes[b
#define tB3 whydump
#define tA3 ,t5 a),
#define t93 for xN3
#define t83 const xU
#define t73 const eZ
#define t63 ){switch(
#define t53 for(;a<
#define t43 nparams
#define t33 281856,
#define t23 nW 0,
#define t13 cAbs nW
#define t03 yA2 cR1
#define eZ3 iK1 cR1
#define eY3 Params[
#define eX3 Params(
#define eW3 nL1,l4::
#define eV3 nL1,{l4
#define eU3 &&p lS1
#define eT3 fp_pow(
#define eS3 if(n71
#define eR3 TreeCountItem
#define eQ3 if(op==
#define eP3 }case
#define eO3 false;}
#define eN3 for t52
#define eM3 ].first
#define eL3 ,cPow,y41
#define eK3 )){data xH
#define eJ3 ){case
#define eI3 default_function_handling
#define eH3 )&*start_at;
#define eG3 nV2 n91
#define eF3 Ne_Mask
#define eE3 Gt_Mask
#define eD3 Lt_Mask
#define eC3 ,const cZ&
#define eB3 opcode,
#define eA3 FindPos
#define e93 resize(
#define e83 public:
#define e73 pclone
#define e63 yA1;lC
#define e53 ,tree
#define e43 cOr,l6
#define e33 newpow
#define e23 for xE
#define e13 change
#define e03 (count
#define cZ3 133,2,
#define cY3 synth.
#define cX3 byteCode
#define cW3 lY1 nC==
#define cV3 cLog2by
#define cU3 factor_t
#define cT3 .n8 cY3
#define cS3 tree.xY1
#define cR3 value1
#define cQ3 a));if(!
#define cP3 else{if(
#define cO3 (eS val
#define cN3 t5 a).xY1
#define cM3 p2 cO ifp2
#define cL3 cP p2;p2
#define cK3 cAbsNot
#define cJ3 stackptr
#define cI3 cLog);xO
#define cH3 opcodes
#define cG3 did_muli
#define cF3 Value){
#define cE3 used[b]
#define cD3 xF)[0].
#define cC3 info=(*
#define cB3 size_t n
#define cA3 sizeof(
#define c93 iF3,
#define c83 cNotNot,
#define c73 cLess,cR
#define c63 x62 2,2,
#define c53 x62 0,2,
#define c43 444848,
#define c33 if(param
#define c23 (op1==
#define c13 cost_t
#define c03 n21=r.specs;if(r.found){
#define yZ3 ,n21,info
#define yY3 break;}
#define yX3 Ge0Lt1
#define yW3 Gt0Le1
#define yV3 ==cOr)l9
#define yU3 cAdd lM2
#define yT3 :start_at()
#define yS3 eJ3 cImmed:
#define yR3 1),x73(1));
#define yQ3 (list.first
#define yP3 nG3 size()
#define yO3 yF1;++b)
#define yN3 iterator
#define yM3 begin();
#define yL3 TreeSet
#define yK3 parent
#define yJ3 (cond c1 iC2
#define yI3 insert(i
#define yH3 newrel
#define yG3 b_needed
#define yF3 cachepos
#define yE3 half&64)
#define yD3 half=
#define yC3 131,4,1,
#define yB3 131,8,1,
#define yA3 result
#define y93 yA3 yT2
#define y83 yA3 lJ1
#define y73 yA3 eT
#define y63 yA3 lS1
#define y53 yA3 xQ
#define y43 xN yA3
#define y33 y03 iI2;if(
#define y23 ;if(half
#define y13 return p xQ
#define y03 ))return
#define xZ3 )))eL lI
#define xY3 ;}static tM1
#define xX3 );lC yQ
#define xW3 ,iF,1,l51+1);
#define xV3 4,1,2,1,
#define xU3 .second
#define xT3 ;x0 l4
#define xS3 xU3);
#define xR3 xF)[a].
#define xQ3 eY1;if nX2
#define xP3 std xM3<bool>
#define xO3 else{xF=new
#define xN3 (eY1=0;a<y3;++a)
#define xM3 ::vector
#define xL3 1 yE nD1
#define xK3 src_pos
#define xJ3 reserve(
#define xI3 n11 void
#define xH3 treeptr
#define xG3 tI1 void
#define xF3 ImmedTag
#define xE3 a,const
#define xD3 RefCount
#define xC3 Birth();
#define xB3 Rehash()
#define xA3 mulgroup
#define x93 exponent
#define x83 unsigned
#define x73 Value_t
#define x63 7168,
#define x53 x22 cP
#define x43 template lL
#define x33 template lY
#define x23 tmp.nN 0))
#define x13 tmp xC
#define x03 fpdata
#define nZ3 middle
#define nY3 ContainsOtherCandidates
#define nX3 nD2.end()
#define nW3 sqrt_cost
#define nV3 const int
#define nU3 yA3=
#define nT3 mul_count
#define nS3 swap(tmp)
#define nR3 lP1 tmp2)
#define nQ3 maxValue1
#define nP3 minValue1
#define nO3 maxValue0
#define nN3 minValue0
#define nM3 ValueType
#define nL3 eS n5 0),
#define nK3 >x73(
#define nJ3 max.known
#define nI3 abs_mul
#define nH3 eY3 a].
#define nG3 Params.
#define nF3 l8 a));
#define nE3 pos_set
#define nD3 nG1);}if(
#define nC3 t5 0)nC
#define nB3 sim.x4 1,
#define nA3 )xD yC l8
#define n93 switch(lZ1
#define n83 (lS));nF lC
#define n73 ;goto redo;
#define n63 {sim.Eat(
#define n53 subtree
#define n43 invtree
#define n33 MakeHash(
#define n23 rulenumit
#define n13 );}void
#define n03 cGreaterOrEq xJ
#define lZ3 ,cGreaterOrEq
#define lY3 e53,info
#define lX3 );range.xJ2
#define lW3 IsLogicalValue(t5
#define lV3 l7 0,2,
#define lU3 l7 0,1,
#define lT3 cEqual,
#define lS3 lT3 lD
#define lR3 0x4},{{3,
#define lQ3 cNeg,lU 1
#define lP3 MakeEqual
#define lO3 newbase
#define lN3 fp_equal(
#define lM3 branch1op
#define lL3 branch2op
#define lK3 overlap
#define lJ3 truth_b
#define lI3 truth_a
#define lH3 found_dup
#define lG3 void set(
#define lF3 rangeutil
#define lE3 {tJ1 lF
#define lD3 Plan_Has(
#define lC3 StackMax)
#define lB3 yA3 xZ
#define lA3 namespace
#define l93 DelParam(
#define l83 tK=!tK;}
#define l73 inverted
#define l63 IsNever:
#define l53 .known&&
#define l43 ==cNot||
#define l33 if(tC2==
#define l23 ,cAnd,l6
#define l13 const std::t0
#define l03 const char*
#define iZ2 if(lN3
#define iY2 }switch(
#define iX2 depcodes
#define iW2 explicit
#define iV2 cCosh nW
#define iU2 lD cR1
#define iT2 VarBegin
#define iS2 continue;}
#define iR2 ;iS2
#define iQ2 )iR2
#define iP2 ].data);
#define iO2 ;x73
#define iN2 tree.nK1
#define iM2 begin(),
#define iL2 cond_add
#define iK2 cond_mul
#define iJ2 cond_and
#define iI2 IsAlways
#define iH2 bool eJ1
#define iG2 leaf1
#define iF2 costree
#define iE2 sintree
#define iD2 leaf_count
#define iC2 &&cond eD)
#define iB2 .tN1 n]
#define iA2 =GetParam(
#define i92 sub_params
#define i82 )cX1 x22);
#define i72 printf(
#define i62 cbrt_count
#define i52 sqrt_count
#define i42 Finite
#define i32 );eS template set_if<
#define i22 min.n5 0),
#define i12 n51);nF lC
#define i02 {AdoptChildrenWithSameOpcode yN2
#define tZ2 p1 cO ifp1
#define tY2 xC cond nC
#define tX2 {if(rule.l81
#define tW2 tree xC
#define tV2 yH 2,cAdd)
#define tU2 pcall_tree
#define tT2 after_powi
#define tS2 yA3 t61
#define tR2 GetHash().
#define tQ2 e31.SubTrees
#define tP2 e31.Others
#define tO2 parampair.
#define tN2 ,xF1);lC
#define tM2 ;break;
#define tL2 xM3<x83>&t11
#define tK2 )tM2
#define tJ2 info.lR[b].
#define tI2 info=info;
#define tH2 yY eO3
#define tG2 cEqual eW1
#define tF2 cLog nW
#define tE2 ),0},{
#define tD2 std::move(
#define tC2 tree nC
#define tB2 MakeNEqual
#define tA2 constraints=
#define t92 ifdata.ofs
#define t82 x22 false)
#define t72 (IfData&ifdata
#define t62 .constraints
#define t52 (size_t b=0;b<
#define t42 Dump(std::
#define t32 ),Value(
#define t22 ),child);
#define t12 isInteger(
#define t02 Comparison
#define eZ2 needs_flip
#define eY2 (half&63)-1;
#define eX2 value]
#define eW2 ~size_t(0)
#define eV2 xR1 y1+1);
#define eU2 >::res,b8<
#define eT2 cO tree);
#define eS2 mul_item
#define eR2 innersub
#define eQ2 cbrt_cost
#define eP2 best_cost
#define eO2 condition
#define eN2 nominator
#define eM2 TopLevel)
#define eL2 per_item
#define eK2 item_type
#define eJ2 first2
#define eI2 l3 18,1,
#define eH2 x3 lU 2,
#define eG2 );sim.x4 2,
#define eF2 tE 1},0,
#define eE2 n51)));x0
#define eD2 Decision
#define eC2 not_tree
#define eB2 Become(t5
#define eA2 group_by
#define e92 x93=
#define e82 ->second
#define e72 targetpos
#define e62 eL true;}
#define e52 ParamSpec
#define e42 rhs.hash2;}
#define e32 rhs.hash1
#define e22 struct
#define e12 Forget()
#define e02 source_tree
#define cZ2 .n_int_sqrt
#define cY2 nC==cLog2&&
#define cX2 <tQ,c13>
#define cW2 p1_evenness
#define cV2 isNegative(
#define cU2 ByteCodeSynth xN
#define cT2 cNop,cNop}}
#define cS2 cTanh,cNop,
#define cR2 NewHash
#define cQ2 >e22 cG<
#define cP2 matches
#define cO2 .match_tree
#define cN2 nV2 void*)&
#define cM2 cGreater,cR
#define cL2 l5 0,1,
#define cK2 cCos nW
#define cJ2 .constvalue
#define cI2 }data;data.
#define cH2 +=1 eL nI1;
#define cG2 negated
#define cF2 Specializer
#define cE2 yZ2 yA3(
#define cD2 params
#define cC2 coshtree
#define cB2 sinhtree
#define cA2 best_score
#define c92 mulvalue
#define c82 pow_item
#define c72 subgroup
#define c62 nC==cPow&&tW
#define c52 PowiResult
#define c42 )continue;if(
#define c32 x73(2)));
#define c22 break;iY2
#define c12 (size_t a=
#define c02 ;for c12
#define yZ2 ){x73
#define yY2 nU yZ2 tmp=
#define yX2 ){iH3
#define yW2 0));range xN
#define yV2 maxValue
#define yU2 minValue
#define yT2 eT=false;if(
#define yS2 fp_min(yB,
#define yR2 div_tree
#define yQ2 pow_tree
#define yP2 x73 nY
#define yO2 preserve
#define yN2 (tree);
#define yM2 x73(0.5)
#define yL2 PullResult()
#define yK2 dup_or_fetch
#define yJ2 test_order
#define yI2 parampair,
#define yH2 .param_count
#define yG2 shift(index)
#define yF2 rulenumber
#define yE2 cTan,l3 2,1,
#define yD2 cLog,l3 2,1,
#define yC2 cTanh nW
#define yB2 cSinh nW
#define yA2 lJ 2},0,
#define y92 cInv,lU 1,
#define y82 GetDepth()
#define y72 factor_immed
#define y62 changes
#define y52 xO1 cO yC l8
#define y42 cO cond l8
#define y32 (tree,std::cout)
#define y22 l8 0));
#define y12 for(typename
#define y02 exp_diff
#define xZ2 ExponentInfo
#define xY2 lower_bound(
#define xX2 factor
#define xW2 is_logical
#define xV2 newrel_and
#define xU2 t3[c eE
#define xT2 res_stackpos
#define xS2 half_pos
#define xR2 ;iF.Remember(
#define xQ2 ){half&=127;
#define xP2 }c5 e22
#define xO2 fphash_t
#define xN2 .IsDefined(
#define xM2 >>1)):(
#define xL2 CodeTreeData
#define xK2 x93)
#define xJ2 multiply(
#define xI2 eS known)
#define xH2 var_trees
#define xG2 const n91
#define xF2 parent_opcode
#define xE2 GetParam(a eU
#define xD2 changed=true;
#define xC2 log2_exponent
#define xB2 dup_fetch_pos
#define xA2 {cZ start_at;
#define x92 IsNever eM lC
#define x82 grammar
#define x72 cSin nW
#define x62 ,cPow,l2
#define x52 0x12 nJ
#define x42 (Value::tP3)
#define x32 Value_EvenInt
#define x22 .Rehash(
#define x12 ,cTan nW
#define x02 ,t1,synth);
#define nZ2 282870 tD
#define nY2 minimum_need
#define nX2 (&*start_at){xF=(
#define nW2 *)tO2 second;
#define nV2 (const
#define nU2 param=*nV2
#define nT2 ParamHolder:{
#define nS2 MakeFalse,{l4
#define nR2 ConditionType
#define nQ2 DUP_ONE(apos)
#define nP2 (x83
#define nO2 eW|nP2)
#define nN2 SpecialOpcode
#define nM2 =i e82.
#define nL2 fp_max(yB);
#define nK2 assimilated
#define nJ2 fraction
#define nI2 tree t51 eM
#define nH2 DUP_BOTH();
#define nG2 -1-offset].
#define nF2 i33()
#define nE2 nD2.erase(cs_it);
#define nD2 TreeCounts
#define nC2 .l93 a);
#define nB2 ,x73(1))){
#define nA2 bool tK=false;
#define n92 nB OPCODE
#define n82 SetOpcode(
#define n72 found_log2
#define n62 div_params
#define n52 set(fp_ceil e3
#define n42 immed_sum
#define n32 ByteCode[++IP]
#define n22 :sim.Eat(1,
#define n12 OPCODE(opcode)
#define n02 ;sim.Push(
#define lZ2 FactorStack xN
#define lY2 iI2 eM lC
#define lX2 cLessOrEq,
#define lW2 cNotNot nW
#define lV2 cNot nW
#define lU2 replacing_slot
#define lT2 RefParams
#define lS2 if_always[
#define lR2 WhatDoWhenCase
#define lQ2 exponent_immed
#define lP2 new_base_immed
#define lO2 base_immed
#define lN2 )eN xA3)
#define lM2 ||op1==
#define lL2 c7 DelParams()
#define lK2 data[a]xU3
#define lJ2 AddCollection(
#define lI2 if(newrel_or==
#define lH2 .UseGetNeeded(
#define lG2 e8 2,131,
#define lF2 Immed.size());
#define lE2 OptimizedUsing
#define lD2 Var_or_Funcno
#define lC2 lD2;
#define lB2 GetParams(
#define lA2 crc32_t
#define l92 const x73&
#define l82 signed_chain
#define l72 MinusInf
#define l62 return true;
#define l52 n_immeds
#define l42 xC tC2);
#define l32 if(remaining[a])
#define l22 ,PowiCache&iF,
#define l12 ;tree lP1
#define l02 ;tree.SetParam(
#define iZ1 else if(
#define iY1 iZ1!yA3
#define iX1 )e62
#define iW1 stack.size()
#define iV1 FindClone(xX
#define iU1 ByteCode[IP]
#define iT1 denominator]
#define iS1 needs_rehash
#define iR1 AnyWhere_Rec
#define iQ1 ~x83(0)
#define iP1 41,42,43,44,
#define iO1 );p2 x22)x5
#define iN1 )val=func lF1
#define iM1 divgroup
#define iL1 ,l5 2,1,
#define iK1 lJ 1},0,
#define iJ1 constraints&
#define iI1 p1_logical_b
#define iH1 p0_logical_b
#define iG1 p1_logical_a
#define iF1 p0_logical_a
#define iE1 cY3 DoDup(
#define iD1 cache_needed
#define iC1 e8 2,1,e8 2,
#define iB1 treelist
#define iA1 IsDescendantOf(
#define i91 has_bad_balance
#define i81 .SetParamsMove(
#define i71 cU3 xX2
#define i61 2)lT 3*
#define i51 {case iI2:
#define i41 fp_abs(tM3))
#define i31 fp_abs(min.val)
#define i21 cNEqual
#define i11 tE 2},0,0x0},{{
#define i01 Oneness_NotOne|
#define tZ1 Value_IsInteger
#define tY1 Constness_Const
#define tX1 DumpHashesFrom(
#define tW1 lE2(
#define tV1 reltype
#define tU1 l92 i)
#define tT1 const CodeTree&
#define tS1 l92 v
#define tR1 SequenceOpcodes
#define tQ1 cY3 PushImmed(
#define tP1 ;cY3 StackTopIs(
#define tO1 cY3 Find(
#define tN1 sep_list[
#define tM1 inline x83
#define tL1 }inline
#define tK1 goto fail;}
#define tJ1 template<
#define tI1 lT2);
#define tH1 TreeCountType xN
#define tG1 >(x73(1),
#define tF1 {pow.CopyOnWrite
#define tE1 const Rule&rule,
#define tD1 x33 void
#define tC1 <<tree.tR2
#define tB1 ;flipped=!flipped;}
#define tA1 cY3 xP 1
#define t91 ,iF x02
#define t81 ;xX i81
#define t71 ){CodeTree xN
#define t61 ))break;yA3*=
#define t51 .IsImmed()
#define t41 a)t51)
#define t31 std::cout<<"POP "
#define t21 stack[iW1-
#define t11 ByteCode,size_t&IP,size_t limit,size_t y9
#define t01 stack.push_back(
#define eZ1 MaxChildDepth
#define eY1 x83 a
#define eX1 std::pair<It,It>
#define eW1 ,l0 2,
#define eV1 cPow,lD
#define eU1 Sign_Negative
#define eT1 Value_Logical
#define eS1 new_factor_immed
#define eR1 occurance_pos
#define eQ1 exponent_hash
#define eP1 exponent_list
#define eO1 CollectionSet xN
#define eN1 CollectMulGroup(
#define eM1 source_set
#define eL1 x93,yL3
#define eK1 *const func)(
#define eJ1 operator
#define eI1 AddParamMove(
#define eH1 FindAndDup yN2
#define eG1 t5 1)t51&&
#define eF1 .min.set(fp_floor
#define eE1 back().thenbranch
#define eD1 .repl_param_list,
#define eC1 retry_anyparams_3
#define eB1 retry_anyparams_2
#define eA1 e7(),std xM3<
#define e91 needlist_cached_t
#define e81 grammar_rules[*r]
#define e71 CodeTreeImmed xN(
#define e61 by_float_exponent
#define e51 lN3 x93
#define e41 new_exp
#define e31 NeedList
#define e21 end()&&i->first==
#define e11 =comp.AddItem(atree
#define e01 return BecomeZero;
#define cZ1 return BecomeOne;
#define cY1 if(lR.size()<=n2)
#define cX1 ;x93
#define cW1 addgroup
#define cV1 found_log2by
#define cU1 nC==cK3)
#define cT1 ParsePowiMuli(
#define cS1 lD2)
#define cR1 0x4},{{1,
#define cQ1 branch1_backup
#define cP1 branch2_backup
#define cO1 exponent_map
#define cN1 plain_set
#define cM1 LightWeight(
#define cL1 if(value
#define cK1 x33 c4
#define cJ1 x33 static
#define cI1 79,122,123,158,159
#define cH1 lA3 FPoptimizer_Optimize
#define cG1 case SubFunction:{
#define cF1 should_regenerate=true;
#define cE1 should_regenerate,
#define cD1 Collection
#define cC1 CodeTree xN r;r xC
#define cB1 RelationshipResult
#define cA1 Subdivide_Combine(
#define c91 long value
#define c81 )const yY
#define c71 rhs c81 hash1
#define c61 best_sep_factor
#define c51 tE3 subfunc_opcode
#define c41 GetParamCount()
#define c31 needlist_cached
#define c21 eB3 bool pad
#define c11 l93 a);}
#define c01 MakesInteger(
#define yZ1 l92 value
#define yY1 best_sep_cost
#define yX1 MultiplicationRange
#define yW1 pihalf_limits
#define yV1 n_stacked
#define yU1 cR2.hash1
#define yT1 AnyParams_Rec
#define yS1 ApplyGrammar(
#define yR1 cGreaterOrEq,
#define yQ1 Become(value l8 0))
#define yP1 ByteCode[t92+
#define yO1 ByteCode.push_back(
#define yN1 PositionalParams,0}
#define yM1 always_sincostan
#define yL1 Recheck_RefCount_Div
#define yK1 Recheck_RefCount_Mul
#define yJ1 xA3.
#define yI1 xA3;xA3 xC
#define yH1 MultiplyAndMakeLong(
#define yG1 cMul);x23;tmp
#define yF1 .c41
#define yE1 :{CodeTree
#define yD1 x73(0)
#define yC1 covers_plus1
#define yB1 }yY3 case
#define yA1 )yH 2,cPow)
#define y91 if(cY3 FindAndDup(
#define y81 SynthesizeParam(
#define y71 grammar_func
#define y61 252415 tD 24830,
#define y51 xJ 523510 tD
#define y41 l2 0,2,165888 tD
#define y31 cCos,l3 2,1,
#define y21 cIf,lD lR3
#define y11 Modulo_Radians},
#define y01 );tree.l93
#define xZ1 ,n12);
#define xY1 GetImmed()
#define xX1 PositionType
#define xW1 CollectionResult
#define xV1 x33 bool
#define xU1 const_offset
#define xT1 inline TriTruthValue
#define xS1 stacktop_desired
#define xR1 SetStackTop(
#define xQ1 FPoptimizer_ByteCode
#define xP1 1)?(poly^(
#define xO1 changed_if
#define xN1 x73(0.0)yX2
#define xM1 cU2&synth)
#define xL1 (long double)
#define xK1 ::Optimize(){
#define xJ1 (p1.xY1
#define xI1 ;std::cout<<
#define xH1 yD1)
#define xG1 xD leaf2 l8
#define xF1 cond_type
#define xE1 fphash_value_t
#define xD1 Recheck_RefCount_RDiv
#define xC1 SwapLastTwoInStack();
#define xB1 ParamSpec_Extract xN(
#define xA1 fPExponentIsTooLarge(
#define x91 CollectMulGroup_Item(
#define x81 pair<x73,yL3>
#define x71 (tree))goto redo;
#define x61 nL xR1 y1-1);
#define x51 covers_full_cycle
#define x41 AssembleSequence(
#define x31 inverse_nominator
#define x21 252180 tD 281854,
#define x11 <<std::dec<<")";}
#define x01 MakeNotNotP0,l4::
#define nZ1 &&IsLogicalValue(
#define nY1 std::pair<T1,T2>&
#define nX1 tJ1 typename
#define nW1 has_good_balance_found
#define nV1 n_occurrences
#define nU1 found_log2_on_exponent
#define nT1 covers_minus1
#define nS1 needs_resynth
#define nR1 immed_product
#define nQ1 ,2,1)nT if(found[data.
#define nP1 ,l1 0x0},{{3,
#define nO1 c22 bitmask&
#define nN1 Sign_Positive
#define nM1 {DataP slot_holder(y8[
#define nL1 ::MakeTrue
#define nK1 SetParamMove(
#define nJ1 CodeTreeImmed(x73(
#define nI1 Suboptimal
#define nH1 n_as_tanh_param
#define nG1 tree.l93 a
#define nF1 opposite=
#define nE1 xE1(
#define nD1 ByteCode.size()
#define nC1 MatchResultType
#define nB1 xN())yH 2,cMul);lC
#define nA1 public e7,public std xM3<
#define n91 CodeTree xN&
#define n81 yY CodeTree xN(
#define n71 needs_sincos
#define n61 resulting_exponent
#define n51 t5 1).xY1
#define n41 cY3 AddOperation(
#define n31 ,l7 2,1,
#define n21 (*xR3 start_at
#define n11 ;x33
#define n01 Unknown:default:;}
#define lZ1 GetLogicalValue(t5
#define lY1 GetParam(a)
#define lX1 },{l4::MakeNotP1,l4::
#define lW1 },{l4::MakeNotP0,l4::
#define lV1 },{l4::MakeNotNotP1,l4::
#define lU1 yF1;a-->0;)if(
#define lT1 .what!=xS)if(TestCase(
#define lS1 .tM3
#define lR1 cSin,l3 2,1,
#define lQ1 AddFunctionOpcode(
#define lP1 .eI1
#define lO1 tmp lP1 tree);
#define lN1 ;cR2.hash2+=
#define lM1 t51 yZ2
#define lL1 ,ByteCode,IP,limit,y9,stack);
#define lK1 (lS,n51));nF
#define lJ1 .nJ3
#define lI1 SetParams(lB2));
#define lH1 }},{ProduceNewTree,2,1,
#define lG1 o<<"("<<std::hex<<data.
#define lF1 (val);else*this=model;}
#define lE1 IfBalanceGood(
#define lD1 n_as_tan_param
#define lC1 changed_exponent
#define lB1 t71 tmp;x13
#define lA1 retry_positionalparams_2
#define l91 x83 index
#define l81 situation_flags&
#define l71 512 tD 400412,
#define l61 CopyOnWrite();
#define l51 recursioncount
#define l41 PlanNtimesCache(
#define l31 >){int mStackPtr=0;
#define l21 FPoptimizer_Grammar
#define l11 AddOperation(cInv,1,1)nT}
#define l01 GetPositivityInfo(tree)!=
#define iZ ParamSpec_SubFunctionData
#define iY eU3<x73(
#define iX inverse_denominator]
#define iW PositionalParams_Rec
#define iV DumpTreeWithIndent(*this);
#define iU tJ1 x83 Compare>
#define iT ,cPow,l3 2,1,
#define iS yA2 0x0},{{1,
#define iR edited_powgroup
#define iQ has_unknown_max
#define iP has_unknown_min
#define iO static const range xN
#define iN if(keep_powi
#define iM synthed_tree
#define iL 7168 tD 401798,
#define iK )xI1 std::endl;DumpHashes(
#define iJ SelectedParams,0},0,0x0},{{
#define iI by_exponent
#define iH collections
#define iG {switch(type eJ3 cond_or:
#define iF cache
#define iE goto ReplaceTreeWithOne;case
#define iD !=xS)return lS2
#define iC e61.data
#define iB iW2 xL2(
#define iA needs_sinhcosh
#define i9 ,cIf,l0 3,
#define i8 tE2 x73(
#define i7 ();pow xC cLog);tW2 cMul);
#define i6 x33 nD
#define i5 MakeFalse,l4::
#define i4 CalculateResultBoundaries(t5
#define i3 p0=CalculateResultBoundaries(
#define i2 408963 tD 24959,
#define i1 522359 tD 24713,
#define i0 AnyParams,0}},{ReplaceParams,
#define tZ matched_params
#define tY [n2 eM3=true;lR[n2]xU3
#define tX l21::Grammar*
#define tW powgroup l8
#define tV nJ1(
#define tU has_mulgroups_remaining
#define tT const iZ
#define tS MatchInfo xN&
#define tR xB3;i92.push_back(
#define tQ int_exponent_t
#define tP RootPowerTable xN::RootPowers[
#define tO MatchPositionSpec_AnyParams xN
#define tN lA3 FPoptimizer_CodeTree
#define tM n_as_sinh_param
#define tL n_as_cosh_param
#define tK is_signed
#define tJ best_factor
#define tI lB2));yJ1 xB3;
#define tH result_positivity
#define tG biggest_minimum
#define tF 122999 tD 139399,
#define tE x3 AnyParams,
#define tD ,{2,
#define tC 142455 tD 141449,
#define tB ParamSpec_NumConstant xN
#define tA cond_tree
#define t9 else_tree
#define t8 then_tree
#define t7 xB3 l12 r);}
#define t6 tree yF1
#define t5 tree l8
#define t4 ;p1 iJ3 p1
#define t3 relationships
#define t2 n91 tree)
#define t1 sequencing
#define t0 string FP_GetOpcodeName(
#define eZ std xM3<CodeTree xN>
#define eY if_stack
#define eX valueType
#define eW );yO1 0x80000000u
#define eV {if(needs_cow){l61 goto
#define eU );bool needs_cow=GetRefCount()>1;
#define eT .min.known
#define eS m.max.
#define eR x22);tW2 iG2 nC);tree.
#define eQ ]);iE1 found[data.
#define eP (p0 lJ1&&p0 lS1<=fp_const_negativezero xN())
#define eO (tQ3 p0 xQ>=x73(0.0))
#define eN ;eI1
#define eM )return false;
#define eL ;return
#define eK )return IsNever eL Unknown;}
#define eJ n_as_sin_param
#define eI n_as_cos_param
#define eH PowiResolver::
#define eG ];};extern"C"{
#define eF cIf,l3 0,1,
#define eE ].relationship
#define eD .BalanceGood
#define eC eI1 c72
#define eB back().endif_location
#define eA xE1 key
#define e9 eI1 mul);
#define e8 130,1,
#define e7 MatchPositionSpecBase
#define e6 iW2 CodeTree(
#define e5 nJ3=false;
#define e4 x73(1.5)*fp_const_pi xN()
#define e3 )eL m;eP3
#define e2 smallest_maximum
#define e1 ]!=eW2&&found[data.
#define e0 factor_needs_rehashing
#define cZ MatchPositionSpecBaseP
#define cY typename tH1::yN3
#define cX nQ x73(-yR3
#define cW goto ReplaceTreeWithParam0;
#define cV xB1 nR.param_list,
#define cU 27,28,29,30,31,32,33,35,36,
#define cT otherhalf
#define cS StackState
#define cR l2 16,2,
#define cQ :goto ReplaceTreeWithZero;case
#define cP );CodeTree xN
#define cO .AddParam(
#define cN const SequenceOpCode xN
#define cM paramholder_matches[x1]
#define cL nK1 0,xK2;l93 1);
#define cK MatchPositionSpec_PositionalParams xN
#define cJ xG2 tree,std::ostream&o
#define cI paramholder_matches.
#define cH CalculatePowiFactorCost(
#define cG ImmedHashGenerator
#define cF ::map<xO2,std::set<std::string> >
#define cE eI1 comp.cN1[a].value);
#define cD T1,typename T2>inline iH2()(
#define cC has_nonlogical_values
#define cB from_logical_context)
#define cA AnyParams,0}},{ProduceNewTree,
#define c9 for c12 y6 yF1;a-->0;)
#define c8 );void lQ1 x83 eB3 cF2<
#define c7 ;xO1 x22);tW2 op1);tree.
#define c6 POWI_CACHE_SIZE
#define c5 PACKED_GRAMMAR_ATTRIBUTE;
#define c4 static inline CodeTree xN
#define c3 ++IP iR2 if(iU1==cH3.
#define c2 },{l4::xS,l4::Never},{l4::xS,l4::Never}}
#define c1 .FoundChild
#define c0 BalanceResultType
#define yZ xD3(0),Opcode(
#define yY {return
#define yX const yY data->
#define yW +=fp_const_twopi xN();
#define yV x83 c;x83 short l[
#define yU ,161,162,163,164,165,166,167,176,177,178,198,202,210,214,222,234,235,237,238,241,242,243,244,247,248,249,251,253,254,255,256,257}};}e22
#define yT for c12 0;a<c41;++a){if(
#define yS n41 nF2,
#define yR !=eW2){iE1 found[data.
#define yQ ComparisonSetBase::
#define yP c12 t6;a-->0;)
#define yO static void n33 nB xO2&cR2,
#define yN MatchPositionSpec_AnyWhere
#define yM c33 tE3 match_type==
#define yL void OutFloatHex(std::ostream&o,
#define yK CodeTree xN tmp,tmp2;tmp2 xC
#define yJ AddParam(CodeTreeImmed(
#define yI .ReplaceWithImmed(
#define yH ;sim.Eat(
#define yG ,typename CodeTree xN::
#define yF AssembleSequence_Subdivide(
#define yE ]=0x80000000u|x83(
#define yD =fp_cosh(m xQ);eS val=fp_cosh cO3);
#define yC branch2
#define yB fp_sin(min),fp_sin(max))
#define yA fp_const_twopi xN());if(
#define y9 factor_stack_base
#define y8 data->Params
#define y7 iR2 if yQ3.xY1==x73(
#define y6 branch1
#define y5 (n23 r=range.first;r!=range xU3;++r){
#define y4 ,x73(-1)))eV
#define y3 nR yH2
#define y2 {nD2.erase(i iQ2
#define y1 StackTop
#define y0 FPOPT_autoptr
#define xZ +=yA3 eL yA3;}x33 inline x73
#define xY int_exponent
#define xX newnode
#define xW iK1 0x0},{{
#define xV ParamSpec_SubFunction
#define xU ParamSpec_ParamHolder
#define xT has_highlevel_opcodes
#define xS Unchanged
#define xR ,eV1 0x4 nJ
#define xQ .min.val
#define xP GetStackTop()-
#define xO sim.AddConst(
#define xN <x73>
#define xM cJ=std::cout
#define xL best_selected_sep
#define xK tJ1>void FunctionParserBase<
#define xJ ,l2 18,2,
#define xI cAnd,i0
#define xH ->Recalculate_Hash_NoRecursion();}
#define xG size_t a=0;a<yK3 yF1;++a)if(
#define xF position
#define xE c12 0;a<t6;++a)
#define xD .IsIdenticalTo(
#define xC .n82
#define xB e23{range xN
#define xA std xM3<CodeTree>
#define x9 TestImmedConstraints(param t62 e53)eM
#define x8 )){tree.FixIncompleteHashes();}
#define x7 ;tmp2.nN 0));x13 cInv);tmp nR3 eL
#define x6 {tree.SetParam(0,iftree l8 0)cP p1;p1 xC
#define x5 l12 p2);tW2 iftree nC)n73}
#define x4 SwapLastTwoInStack()yH
#define x3 ,cAdd,
#define x2 n63 1,cInv tK2}xO-1 e63
#define x1 paramholder_index
#define x0 l62 case
#define nZ occurance_counts
#define nY >p=i4 a));if(p.
#define nX i63 tree)xI1"\n";
#define nW ,l0 1,
#define nV -->0;){xG2 powgroup=lY1;if(powgroup
#define nU if(t5 0)t51
#define nT tP1*this)eL;}
#define nS const FPoptimizer_CodeTree::n91 tree
#define nR model_tree
#define nQ return range xN(
#define nP eZ&lT2
#define nO ),rangehalf xN model=rangehalf xN()){if(known
#define nN AddParam(t5
#define nM ConstantFolding_LogicCommon(tree,yQ
#define nL ){using lA3 FUNCTIONPARSERTYPES;
#define nK )t71
#define nJ },{{2,
#define nI nX1 Ref>inline void y0<Ref>::
#define nH AnyParams,1},0,0x0},{{
#define nG cOr,i0 16,1,
#define nF goto do_return;}
#define nE ):data(new xL2 xN(
#define nD xL2 xN::xL2(
#define nC .nF2
#define nB FUNCTIONPARSERTYPES::
#define nA b;}};tJ1>e22 Comp<nB
#define n9 lD2(),eX3),Hash(),Depth(1),tW1 0){}
#define n8 SynthesizeByteCode(synth);
#define n7 GetIntegerInfo(t5 0))==iI2)cW
#define n6 l12 xO1 iX1
#define n5 template set_if<cGreater>(x73(
#define n4 while(yS1 nV2 Grammar&)
#define n3 DumpParams xN(param tE3 param_list,i73 yH2,o);
#define n2 restholder_index
#define n1 CodeTree xN x93 cX1 xC cMul)cX1 cO
#define n0 (lS);if(fp_nequal(tmp,xH1 yX2 x73(1)/tmp);nF}lC
#define lZ :if(ParamComparer xN()(eY3 1],eY3 0])){std::swap(eY3 0],eY3 1]);Opcode=
#define lY <typename x73>
#define lX xN tmp;x13 cPow);x23;tmp.yJ x73(
#define lW tY1,0x0},
#define lV eI1 pow l8 1));pow.l93 1);pow x22);iN2 0,pow);goto NowWeAreMulGroup;}
#define lU GroupFunction,0},lW{{
#define lT ,x73(1)/x73(
#define lS t5 0).xY1
#define lR restholder_matches
#define lQ yU1|=key;xE1 crc=(key>>10)|(key<<(64-10))lN1((~nE1 crc))*3)^1234567;}};
#define lP (t5 0)t51&&t5 1)t51 yX2
#define lO xO1;xO1 l42 xO1 lP1 t5 0));xO1 cO y6 l8
#define lN x33 CodeTree xN::CodeTree(
#define lM tree.SetParam(0,t5 0)l8 0))l02 1,CodeTreeImmed(
#define lL lY void cU2::lQ1 x83 eB3 cF2<
#define lK cMul,lU 2,
#define lJ cMul,AnyParams,
#define lI CalculateResultBoundaries(tmp);eP3
#define lH :e13=comp.AddRelationship(atree l8 0),atree l8 1),yQ
#define lG cPow,l0 2
#define lF typename x73>inline iH2()(l92 a,l92 b)yY a
#define lE {range xN m=i4 0));
#define lD yN1,0,
#define lC break;case
#define lB tD1 CodeTree xN::
#define lA l1 0x0 nJ
#define l9 ?0:1)cP xO1;xO1 l42 xO1 i81 tree.lB2));xO1 x22);tW2
#define l8 .GetParam(
#define l7 cAdd,i0
#define l6 SelectedParams,0},0,0x0 nJ
#define l5 lJ 0}},{ReplaceParams,
#define l4 RangeComparisonData
#define l3 yN1},{ProduceNewTree,
#define l2 yN1},{ReplaceParams,
#define l1 cMul,SelectedParams,0},0,
#define l0 lD 0x0},{{
#ifdef _MSC_VER
typedef
x83
int
lA2;
#else
#include <stdint.h>
typedef
uint_least32_t
lA2;
#endif
lA3
crc32{enum{startvalue=0xFFFFFFFFUL,poly=0xEDB88320UL}
;tJ1
lA2
crc>e22
b8{enum{b1=(crc&xP1
crc
xM2
crc>>1),b2=(b1&xP1
b1
xM2
b1>>1),b3=(b2&xP1
b2
xM2
b2>>1),b4=(b3&xP1
b3
xM2
b3>>1),b5=(b4&xP1
b4
xM2
b4>>1),b6=(b5&xP1
b5
xM2
b5>>1),b7=(b6&xP1
b6
xM2
b6>>1),res=(b7&xP1
b7
xM2
b7>>1)}
;}
;inline
lA2
update(lA2
crc,x83
b){
#define B4(n) b8<n eU2 n+1 eU2 n+2 eU2 n+3>::res
#define R(n) B4(n),B4(n+4),B4(n+8),B4(n+12)
static
const
lA2
table[256]={R(0x00),R(0x10),R(0x20),R(0x30),R(0x40),R(0x50),R(0x60),R(0x70),R(0x80),R(0x90),R(0xA0),R(0xB0),R(0xC0),R(0xD0),R(0xE0),R(0xF0)}
;
#undef R
#undef B4
return((crc>>8))^table[(crc^b)&0xFF];tL1
lA2
calc_upd(lA2
c,const
x83
char*buf,size_t
size){lA2
value=c;for(size_t
p=0;p<size;++p)value=update(value,buf[p])eL
value;tL1
lA2
calc
nV2
x83
char*buf,size_t
size)yY
calc_upd(startvalue,buf,size);}
}
#ifndef FPOptimizerAutoPtrHH
#define FPOptimizerAutoPtrHH
nX1
Ref>class
y0{e83
y0():p(0){}
y0(Ref*b):p(b){xC3}
y0
nV2
y0&b):p(b.p){xC3
tL1
Ref&eJ1*(c81*p;tL1
Ref*eJ1->(c81
p;}
y0&eJ1=(Ref*b){Set(b)eL*this;}
y0&eJ1=nV2
y0&b){Set(b.p)eL*this;}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
y0(y0&&b):p(b.p){b.p=0;}
y0&eJ1=(y0&&b){if(p!=b.p){e12;p=b.p;b.p=0;}
return*this;}
#endif
~y0(){e12;}
void
UnsafeSetP(Ref*newp){p=newp;}
void
swap(y0<Ref>&b){Ref*tmp=p;p=b.p;b.p=tmp;}
private:inline
static
void
Have(Ref*p2);inline
void
e12;inline
void
xC3
inline
void
Set(Ref*p2);private:Ref*p;}
;nI
e12{if(!p)return;p->xD3-=1;if(!p->xD3)delete
p;}
nI
Have(Ref*p2){if(p2)++(p2->xD3);}
nI
Birth(){Have(p);}
nI
Set(Ref*p2){Have(p2);e12;p=p2;}
#endif
#include <utility>
e22
Compare2ndRev{nX1
T>inline
iH2()nV2
T&xE3
T&b
c81
a
xU3>b
xU3;}
}
;e22
Compare1st{nX1
cD
const
nY1
xE3
nY1
b
c81
a.first<b.first;}
nX1
cD
const
nY1
a,T1
b
c81
a.first<b;}
nX1
cD
T1
xE3
nY1
b
c81
a<b.first;}
}
;
#ifndef FPoptimizerHashHH
#define FPoptimizerHashHH
#ifdef _MSC_VER
typedef
x83
long
long
xE1;
#define FPHASH_CONST(x) x##ULL
#else
#include <stdint.h>
typedef
uint_fast64_t
xE1;
#define FPHASH_CONST(x) x##ULL
#endif
lA3
FUNCTIONPARSERTYPES{e22
xO2{xE1
hash1,hash2;xO2():hash1(0),hash2(0){}
xO2
nV2
xE1&xE3
xE1&b):hash1(a),hash2(b){}
iH2==nV2
xO2&c71==e32&&hash2==e42
iH2!=nV2
xO2&c71!=e32||hash2!=e42
iH2<nV2
xO2&c71!=e32?hash1<e32:hash2<e42}
;}
#endif
#ifndef FPOptimizer_CodeTreeHH
#define FPOptimizer_CodeTreeHH
#ifdef FP_SUPPORT_OPTIMIZER
#include <vector>
#include <utility>
lA3
l21{e22
Grammar;}
lA3
xQ1{x33
class
ByteCodeSynth;}
tN{x33
class
CodeTree
n11
e22
xL2
n11
class
CodeTree{typedef
y0<xL2
xN>DataP;DataP
data;e83
CodeTree();~CodeTree();e22
OpcodeTag{}
;e6
n92
o,OpcodeTag);e22
FuncOpcodeTag{}
;e6
n92
o,x83
f,FuncOpcodeTag);e22
xF3{}
;e6
tS1,xF3);
#ifdef __GXX_EXPERIMENTAL_CXX0X__
e6
x73&&v,xF3);
#endif
e22
VarTag{}
;e6
x83
varno,VarTag);e22
CloneTag{}
;e6
tT1
b,CloneTag);void
GenerateFrom
nV2
typename
FunctionParserBase
xN::Data&data,bool
keep_powi=false);void
GenerateFrom
nV2
typename
FunctionParserBase
xN::Data&data,const
xA&xH2,bool
keep_powi=false);void
SynthesizeByteCode(std
xM3<x83>&cX3,std
xM3
xN&immed,size_t&stacktop_max);void
SynthesizeByteCode(xQ1::cU2&synth,bool
MustPopTemps=true)const;size_t
SynthCommonSubExpressions(xQ1::xM1
const;void
SetParams
nV2
xA&xG3
SetParamsMove(xA&tI1
CodeTree
GetUniqueRef();
#ifdef __GXX_EXPERIMENTAL_CXX0X__
void
SetParams(xA&&tI1
#endif
void
SetParam(size_t
which,tT1
b);void
nK1
size_t
which,CodeTree&b);void
AddParam(tT1
param);void
eI1
CodeTree&param);void
AddParams
nV2
xA&xG3
AddParamsMove(xA&xG3
AddParamsMove(xA&lT2,size_t
lU2);void
l93
size_t
index);void
DelParams();void
Become(tT1
b);inline
size_t
c41
const
yY
lB2).size();tL1
CodeTree&GetParam(cB3)yY
lB2)[n];tL1
tT1
GetParam(cB3
c81
lB2)[n];tL1
void
n82
n92
o){data->Opcode=o;tL1
n92
nF2
yX
Opcode;tL1
nB
xO2
GetHash()yX
Hash;tL1
const
xA&lB2
c81
y8;tL1
xA&lB2)yY
y8;tL1
size_t
y82
yX
Depth;tL1
l92
xY1
yX
Value;tL1
x83
GetVar()yX
lC2
tL1
x83
GetFuncNo()yX
lC2
tL1
bool
IsDefined(c81
nF2!=nB
cNop;tL1
bool
IsImmed(c81
nF2==nB
cImmed;tL1
bool
IsVar(c81
nF2==nB
iT2;tL1
x83
GetRefCount()yX
xD3;}
void
ReplaceWithImmed(tU1;void
Rehash(bool
constantfolding=true);void
Sort();inline
void
Mark_Incompletely_Hashed(){data->Depth=0;tL1
bool
Is_Incompletely_Hashed()yX
Depth==0;tL1
const
tX
GetOptimizedUsing()yX
lE2;tL1
void
SetOptimizedUsing
nV2
tX
g){data->lE2=g;}
bool
RecreateInversionsAndNegations(bool
prefer_base2=false);void
FixIncompleteHashes();void
swap(CodeTree&b){data.swap(b.data);}
bool
IsIdenticalTo(tT1
b)const;void
l61}
n11
e22
xL2{int
xD3;n92
Opcode
iO2
Value;x83
lC2
eZ
Params;nB
xO2
Hash;size_t
Depth;const
tX
lE2;xL2();xL2
nV2
xL2&b);iB
n92
o);iB
n92
o,x83
f);iB
tU1;
#ifdef __GXX_EXPERIMENTAL_CXX0X__
iB
x73&&i);xL2(xL2&&b);
#endif
bool
IsIdenticalTo
nV2
xL2&b)const;void
Sort();void
Recalculate_Hash_NoRecursion();private:void
eJ1=nV2
xL2&b);}
n11
c4
CodeTreeImmed(tU1
n81
i
yG
xF3());}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
cK1
CodeTreeImmed(x73&&i)n81
tD2
i)yG
xF3());}
#endif
cK1
CodeTreeOp(n92
opcode)n81
opcode
yG
OpcodeTag());}
cK1
CodeTreeFuncOp(n92
eB3
x83
f)n81
eB3
f
yG
FuncOpcodeTag());}
cK1
CodeTreeVar
nP2
varno)n81
varno
yG
VarTag());}
#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
tD1
DumpHashes(xM)xI3
DumpTree(xM)xI3
DumpTreeWithIndent(xM,const
std::string&indent="\\"
);
#endif
}
#endif
#endif
#ifndef FPOptimizer_GrammarHH
#define FPOptimizer_GrammarHH
#include <iostream>
tN{x33
class
CodeTree;}
lA3
l21{enum
ImmedConstraint_Value{ValueMask=0x07,Value_AnyNum=0x0,x32=0x1,Value_OddInt=0x2,tZ1=0x3,Value_NonInteger=0x4,eT1=0x5}
;enum
ImmedConstraint_Sign{SignMask=0x18,Sign_AnySign=0x00,nN1=0x08,eU1=0x10,Sign_NoIdea=0x18}
;enum
ImmedConstraint_Oneness{OnenessMask=0x60,Oneness_Any=0x00,Oneness_One=0x20,Oneness_NotOne=0x40}
;enum
ImmedConstraint_Constness{ConstnessMask=0x180,Constness_Any=0x00,tY1=0x80,Constness_NotConst=0x100}
;enum
Modulo_Mode{Modulo_None=0,Modulo_Radians=1}
;enum
Situation_Flags{LogicalContextOnly=0x01,NotForIntegers=0x02,OnlyForIntegers=0x04,OnlyForComplex=0x08,NotForComplex=0x10}
;enum
nN2{NumConstant,ParamHolder,SubFunction}
;enum
ParamMatchingType{PositionalParams,SelectedParams,AnyParams,GroupFunction}
;enum
RuleType{ProduceNewTree,ReplaceParams}
;
#ifdef __GNUC__
# define PACKED_GRAMMAR_ATTRIBUTE __attribute__((packed))
#else
# define PACKED_GRAMMAR_ATTRIBUTE
#endif
typedef
std::pair<nN2,const
void*>e52
n11
e52
ParamSpec_Extract
nP2
paramlist,l91)n11
bool
ParamSpec_Compare
nV2
void*xE3
void*b,nN2
type);x83
ParamSpec_GetDepCode
nV2
e52&b);e22
xU{l91:8;x83
constraints:9;x83
depcode:15;}
c5
x33
e22
ParamSpec_NumConstant{x73
constvalue;x83
modulo;xP2
iZ{x83
param_count:2;x83
param_list:30;n92
subfunc_opcode:8;ParamMatchingType
match_type:3;x83
n2:5;xP2
xV{iZ
data;x83
constraints:9;x83
depcode:7;xP2
Rule{RuleType
ruletype:2;x83
situation_flags:5;x83
repl_param_count:2+9;x83
repl_param_list:30;iZ
match_tree;xP2
Grammar{x83
rule_count;x83
short
rule_list[999
eG
extern
const
Rule
grammar_rules[];}
tD1
DumpParam
nV2
e52&p,std::ostream&o=std::cout)xI3
DumpParams
nP2
paramlist,x83
count,std::ostream&o=std::cout);}
#endif
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
#define CONSTANT_POS_INF HUGE_VAL
#define CONSTANT_NEG_INF (-HUGE_VAL)
lA3
FUNCTIONPARSERTYPES{x33
inline
x73
fp_const_pihalf()yY
fp_const_pi
xN()*yM2;}
x33
inline
x73
fp_const_twopi(cE2
fp_const_pi
xN());lB3
fp_const_twoe(cE2
fp_const_e
xN());lB3
fp_const_twoeinv(cE2
fp_const_einv
xN());lB3
fp_const_negativezero(){
#ifdef FP_EPSILON
return-fp_epsilon
xN();
#else
return
x73(-1e-14);
#endif
}
}
#ifdef FP_SUPPORT_OPTIMIZER
#include <vector>
#include <utility>
#include <iostream>
cH1{using
lA3
l21;using
tN;using
lA3
FUNCTIONPARSERTYPES
n11
class
MatchInfo{e83
std
xM3<std::pair<bool,eZ> >lR;eZ
paramholder_matches;std
xM3<x83>tZ;e83
MatchInfo():lR(),paramholder_matches(),tZ(){}
e83
bool
SaveOrTestRestHolder
nP2
n2,t73&iB1){cY1{lR.e93
n2+1);lR
tY=iB1
e62
if(lR[n2
eM3==false){lR
tY=iB1
e62
t73&found=lR[n2]xU3;if(iB1.size()!=found.size()eM
for
c12
0;a<iB1.size();++a)if(!iB1[a]xD
found[a]y03
false
e62
void
SaveRestHolder
nP2
n2,eZ&iB1){cY1
lR.e93
n2+1);lR
tY.swap(iB1);}
bool
SaveOrTestParamHolder
nP2
x1,xG2
xH3){if(cI
size()<=x1){cI
xJ3
x1+1);cI
e93
x1);cI
push_back(xH3
iX1
if(!cM
xN2)){cM=xH3
e62
return
xH3
xD
cM
n13
SaveMatchedParamIndex(l91){tZ.push_back(index);}
xG2
GetParamHolderValueIfFound
nP2
x1)const{static
const
CodeTree
xN
dummytree;if(cI
size()<=x1)return
dummytree
eL
cM;}
xG2
GetParamHolderValue
nP2
x1
c81
cM;}
bool
HasRestHolder
nP2
n2
c81
lR.size()>n2&&lR[n2
eM3==true;}
t73&GetRestHolderValues
nP2
n2)const{static
t73
empty_result;cY1
return
empty_result
eL
lR[n2]xU3;}
const
std
xM3<x83>&GetMatchedParamIndexes(c81
tZ;}
void
swap(tS
b){lR.swap(b.lR);cI
swap(b.paramholder_matches);tZ.swap(b.tZ);}
tS
eJ1=nV2
tS
b){lR=b.lR;paramholder_matches=b.paramholder_matches;tZ=b.tZ
eL*this;}
}
;class
e7;typedef
y0<e7>cZ;class
e7{e83
int
xD3;e83
e7():xD3(0){}
virtual~e7(){}
}
;e22
nC1{bool
found;cZ
specs;nC1(bool
f):found(f),specs(){}
nC1(bool
f
eC3
s):found(f),specs(s){}
}
xI3
SynthesizeRule(tE1
n91
tree,tS
info)n11
nC1
TestParam
nV2
e52&yI2
xG2
tree
eC3
start_at,tS
info)n11
nC1
TestParams(tT&nR,xG2
tree
eC3
start_at,tS
info,bool
eM2
n11
bool
yS1
const
Grammar&x82,FPoptimizer_CodeTree::n91
tree,bool
from_logical_context=false)xI3
ApplyGrammars(FPoptimizer_CodeTree::t2
n11
bool
IsLogisticallyPlausibleParamsMatch(tT&cD2,const
t2;}
lA3
l21{tD1
DumpMatch(tE1
nS,const
FPoptimizer_Optimize::tS
info,bool
DidMatch,std::ostream&o=std::cout)xI3
DumpMatch(tE1
nS,const
FPoptimizer_Optimize::tS
info,l03
tB3,std::ostream&o=std::cout);}
#endif
#include <string>
l13
l21::nN2
c21=false);l13
n92
c21=false);
#include <string>
#include <sstream>
#include <assert.h>
#include <iostream>
using
lA3
l21;using
lA3
FUNCTIONPARSERTYPES;l13
l21::nN2
c21){
#if 1
l03
p=0;switch(opcode
eJ3
NumConstant:p="NumConstant"
;lC
ParamHolder:p="ParamHolder"
;lC
SubFunction:p="SubFunction"
tM2}
std::ostringstream
tmp;assert(p);tmp<<p;if(pad)while(tmp.str().size()<12)tmp<<' 'eL
tmp.str();
#else
std::ostringstream
tmp;tmp<<opcode;if(pad)while(tmp.str().size()<5)tmp<<' 'eL
tmp.str();
#endif
}
l13
n92
c21){
#if 1
l03
p=0;switch(opcode
eJ3
cAbs:p="cAbs"
;lC
cAcos:p="cAcos"
;lC
cAcosh:p="cAcosh"
;lC
cArg:p="cArg"
;lC
cAsin:p="cAsin"
;lC
cAsinh:p="cAsinh"
;lC
cAtan:p="cAtan"
;lC
cAtan2:p="cAtan2"
;lC
cAtanh:p="cAtanh"
;lC
cCbrt:p="cCbrt"
;lC
cCeil:p="cCeil"
;lC
cConj:p="cConj"
;lC
cCos:p="cCos"
;lC
cCosh:p="cCosh"
;lC
cCot:p="cCot"
;lC
cCsc:p="cCsc"
;lC
cEval:p="cEval"
;lC
cExp:p="cExp"
;lC
cExp2:p="cExp2"
;lC
cFloor:p="cFloor"
;lC
cHypot:p="cHypot"
;lC
cIf:p="cIf"
;lC
cImag:p="cImag"
;lC
cInt:p="cInt"
;lC
cLog:p="cLog"
;lC
cLog2:p="cLog2"
;lC
cLog10:p="cLog10"
;lC
cMax:p="cMax"
;lC
cMin:p="cMin"
;lC
cPolar:p="cPolar"
;lC
cPow:p="cPow"
;lC
cReal:p="cReal"
;lC
cSec:p="cSec"
;lC
cSin:p="cSin"
;lC
cSinh:p="cSinh"
;lC
cSqrt:p="cSqrt"
;lC
cTan:p="cTan"
;lC
cTanh:p="cTanh"
;lC
cTrunc:p="cTrunc"
;lC
cImmed:p="cImmed"
;lC
cJump:p="cJump"
;lC
cNeg:p="cNeg"
;lC
cAdd:p="cAdd"
;lC
cSub:p="cSub"
;lC
cMul:p="cMul"
;lC
cDiv:p="cDiv"
;lC
cMod:p="cMod"
;lC
cEqual:p="cEqual"
;lC
i21:p="cNEqual"
;lC
cLess:p="cLess"
;lC
cLessOrEq:p="cLessOrEq"
;lC
cGreater:p="cGreater"
;lC
cGreaterOrEq:p="cGreaterOrEq"
;lC
cNot:p="cNot"
;lC
cAnd:p="cAnd"
;lC
cOr:p="cOr"
;lC
cDeg:p="cDeg"
;lC
cRad:p="cRad"
;lC
cFCall:p="cFCall"
;lC
cPCall:p="cPCall"
tM2
#ifdef FP_SUPPORT_OPTIMIZER
case
cFetch:p="cFetch"
;lC
cPopNMov:p="cPopNMov"
;lC
cV3:p="cLog2by"
;lC
cNop:p="cNop"
tM2
#endif
case
cSinCos:p="cSinCos"
;lC
cSinhCosh:p="cSinhCosh"
;lC
cK3:p="cAbsNot"
;lC
cAbsNotNot:p="cAbsNotNot"
;lC
cAbsAnd:p="cAbsAnd"
;lC
cAbsOr:p="cAbsOr"
;lC
iF3:p="cAbsIf"
;lC
cDup:p="cDup"
;lC
cInv:p="cInv"
;lC
cSqr:p="cSqr"
;lC
cRDiv:p="cRDiv"
;lC
cRSub:p="cRSub"
;lC
cNotNot:p="cNotNot"
;lC
cRSqrt:p="cRSqrt"
;lC
iT2:p="VarBegin"
tM2}
std::ostringstream
tmp;assert(p);tmp<<p;if(pad)while(tmp.str().size()<12)tmp<<' 'eL
tmp.str();
#else
std::ostringstream
tmp;tmp<<opcode;if(pad)while(tmp.str().size()<5)tmp<<' 'eL
tmp.str();
#endif
}
#ifdef FP_SUPPORT_OPTIMIZER
#include <vector>
#include <utility>
#ifndef FP_GENERATING_POWI_TABLE
enum{MAX_POWI_BYTECODE_LENGTH=20}
;
#else
enum{MAX_POWI_BYTECODE_LENGTH=999}
;
#endif
enum{MAX_MULI_BYTECODE_LENGTH=3}
;lA3
xQ1{x33
class
ByteCodeSynth{e83
ByteCodeSynth():ByteCode(),Immed(),cS(),y1(0),StackMax(0){ByteCode.xJ3
64);Immed.xJ3
8);cS.xJ3
16
n13
Pull(std
xM3<x83>&bc,std
xM3
xN&imm,size_t&StackTop_max){for(eY1=0;a<nD1;++a){ByteCode[a]&=~0x80000000u;}
ByteCode.swap(bc);Immed.swap(imm);StackTop_max=StackMax;}
size_t
GetByteCodeSize(c81
nD1;}
size_t
GetStackTop(c81
y1;}
void
PushVar
nP2
varno){yO1
varno);eV2}
void
PushImmed(x73
immed
nL
yO1
cImmed);Immed.push_back(immed);eV2}
void
StackTopIs(nS,int
offset=0){if((int)y1>offset){cS[y1
nG2
first=true;cS[y1
nG2
second=tree;}
}
bool
IsStackTop(nS,int
offset=0
c81(int)y1>offset&&cS[y1
nG2
first&&cS[y1
nG2
second
xD
tree);tL1
void
EatNParams
nP2
eat_count){y1-=eat_count;}
void
ProducedNParams
nP2
produce_count){xR1
y1+produce_count
n13
DoPopNMov(size_t
e72,size_t
srcpos
nL
yO1
cPopNMov
nO2
e72
nO2
srcpos);xR1
srcpos+1);cS[e72]=cS[srcpos];xR1
e72+1
n13
DoDup(size_t
xK3
nL
if(xK3==y1-1){yO1
cDup);}
else{yO1
cFetch
nO2
xK3);}
eV2
cS[y1-1]=cS[xK3];}
#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
tJ1
int>void
Dump(){std::ostream&o=std::cout;o<<"Stack state now("
<<y1<<"):\n"
c02
0;a<y1;++a){o<<a<<": "
;if(cS[a
eM3){nS=cS[a]xU3;o<<'['<<std::hex<<(void*)(&tree.lB2))<<std::dec<<','<<tree.GetRefCount()<<']'i63
tree,o);}
else
o<<"?"
;o<<"\n"
;}
o<<std::flush;}
#endif
size_t
eA3(nS)const{for
c12
y1;a-->0;)if(cS[a
eM3&&cS[a]xU3
xD
tree
y03
a
eL
eW2;}
bool
Find(nS
c81
eA3(tree)!=eW2;}
bool
FindAndDup(nS){size_t
pos=eA3
yN2
if(pos!=eW2){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<iE3"duplicate at ["
<<pos<<"]: "
i63
tree)xI1" -- issuing cDup or cFetch\n"
;
#endif
DoDup(pos
iX1
return
eO3
e22
IfData{size_t
ofs;}
;void
SynthIfStep1
t72,n92
op
x61
t92=nD1;yO1
op
eW
eW
n13
SynthIfStep2
t72
x61
yP1
xL3+2);yP1
2
yE
lF2
t92=nD1;yO1
cJump
eW
eW
n13
SynthIfStep3
t72
x61
ByteCode.back()|=0x80000000u;yP1
xL3-1);yP1
2
yE
lF2
xR1
y1+1)c02
0;a<t92;++a){if(ByteCode[a]==cJump&&ByteCode[a+1]==(0x80000000u|(t92-1))){ByteCode[a+xL3-1);ByteCode[a+2
yE
lF2
iY2
ByteCode[a]eJ3
iF3:case
cIf:case
cJump:case
cPopNMov:a+=2;lC
cFCall:case
cPCall:case
cFetch:a+=1
tM2
default:yY3}
}
protected:void
xR1
size_t
value){y1=value;if(y1>lC3{StackMax=y1;cS.e93
lC3;}
}
protected:std
xM3<x83>ByteCode;std
xM3
xN
Immed;std
xM3<std::pair<bool,FPoptimizer_CodeTree::CodeTree
xN> >cS;size_t
y1;size_t
StackMax;private:void
incStackPtr(){if(y1+2>lC3
cS.e93
StackMax=y1+2);}
tJ1
bool
IsIntType,bool
IsComplexType>e22
cF2{}
;e83
void
AddOperation
nP2
eB3
x83
eat_count,x83
produce_count=1){EatNParams(eat_count);lQ1
opcode);ProducedNParams(produce_count
n13
lQ1
x83
eB3
cF2<false,false>c8
false,true>c8
true,false>c8
true,true>);inline
void
lQ1
x83
opcode){lQ1
eB3
cF2<bool(nB
IsIntType
xN::yA3),bool(nB
IsComplexType
xN::yA3)>());}
}
n11
e22
SequenceOpCode
n11
e22
tR1{static
cN
AddSequence;static
cN
MulSequence;}
xI3
x41
long
count,cN&t1,xM1;}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
using
lA3
FUNCTIONPARSERTYPES;lA3
xQ1{x33
e22
SequenceOpCode{x73
basevalue;x83
op_flip;x83
op_normal,op_normal_flip;x83
op_inverse,op_inverse_flip;}
n11
cN
tR1
xN::AddSequence={yD1,cNeg
x3
cAdd,cSub,cRSub}
n11
cN
tR1
xN::MulSequence={x73(1),cInv,cMul,cMul,cDiv,cRDiv}
;
#define findName(a,b,c) "var"
#define TryCompilePowi(o) false
#define mData this
#define mByteCode ByteCode
#define mImmed Immed
x43
false,false
l31
# define FP_FLOAT_VERSION 1
# define FP_COMPLEX_VERSION 0
# include "fp_opcode_add.inc"
# undef FP_COMPLEX_VERSION
# undef FP_FLOAT_VERSION
}
x43
true,false
l31
# define FP_FLOAT_VERSION 0
# define FP_COMPLEX_VERSION 0
# include "fp_opcode_add.inc"
# undef FP_COMPLEX_VERSION
# undef FP_FLOAT_VERSION
}
#ifdef FP_SUPPORT_COMPLEX_NUMBERS
x43
false,true
l31
# define FP_FLOAT_VERSION 1
# define FP_COMPLEX_VERSION 1
# include "fp_opcode_add.inc"
# undef FP_COMPLEX_VERSION
# undef FP_FLOAT_VERSION
}
x43
true,true
l31
# define FP_FLOAT_VERSION 0
# define FP_COMPLEX_VERSION 1
# include "fp_opcode_add.inc"
# undef FP_COMPLEX_VERSION
# undef FP_FLOAT_VERSION
}
#endif
#undef findName
#undef mImmed
#undef mByteCode
#undef mData
#undef TryCompilePowi
}
using
lA3
xQ1;
#define POWI_TABLE_SIZE 256
#define POWI_WINDOW_SIZE 3
lA3
xQ1{
#ifndef FP_GENERATING_POWI_TABLE
extern
const
x83
char
powi_table[POWI_TABLE_SIZE];const
#endif
x83
char
powi_table[POWI_TABLE_SIZE]={0,1,1,1,2,1,2,1,xV3
4,1,2,yB3
2,1,xV3
8,cZ3
yC3
15,1,16,1,2,1,4,1,2,yB3
2,1,4,cZ3
1,16,1,25,yC3
27,5,8,3,2,1,30,1,31,3,32,1,2,1,xV3
8,1,2,yC3
39,1,16,137,2,1,4,cZ3
yB3
45,135,4,31,2,5,32,1,2,131,50,1,51,1,8,3,2,1,54,1,55,3,16,1,57,133,4,137,2,135,60,1,61,3,62,133,63,1,iC1
131,iC1
139,lG2
e8
30,1,130,137,2,31,lG2
e8
e8
130,cZ3
1,e8
e8
2,1,130,133,iC1
61,130,133,62,139,130,137,e8
lG2
e8
e8
iC1
131,e8
e8
130,131,2,133,lG2
130,141,e8
130,cZ3
1,e8
5,135,e8
lG2
e8
lG2
130,133,130,141,130,131,e8
e8
2,131}
;}
static
nV3
c6=256;
#define FPO(x)
lA3{class
PowiCache{private:int
iF[c6];int
iD1[c6];e83
PowiCache():iF(),iD1(){iF[1]=1;}
bool
Plan_Add(c91,int
count){cL1>=c6
eM
iD1[eX2+=count
eL
iF[eX2!=0;}
void
lD3
c91){cL1<c6)iF[eX2=1;}
void
Start(size_t
value1_pos){for(int
n=2;n<c6;++n)iF[n]=-1;Remember(1,value1_pos);DumpContents();}
int
Find(c91)const{cL1<c6){if(iF[eX2>=0){FPO(tZ3(i53,"* I found %ld from cache (%u,%d)\n",value,(unsigned)cache[value],i13 value]))eL
iF[eX2;}
}
return-1;}
void
Remember(c91,size_t
iD3){cL1>=c6)return;FPO(tZ3(i53,"* Remembering that %ld can be found at %u (%d uses remain)\n",value,(unsigned)iD3,i13 value]));iF[eX2=(int)iD3;}
void
DumpContents()const{FPO(for(int a=1;a<POWI_CACHE_SIZE;++a)if(cache[a]>=0||i13 a]>0){tZ3(i53,"== cache: sp=%d, val=%d, needs=%d\n",cache[a],a,i13 a]);})}
int
UseGetNeeded(c91){cL1>=0&&value<c6)return--iD1[eX2
eL
0;}
}
n11
size_t
yF
long
count
l22
cN&t1,xM1
xI3
cA1
size_t
apos,long
aval,size_t
bpos,long
bval
l22
x83
cumulation_opcode,x83
cimulation_opcode_flip,xM1;void
l41
c91
l22
int
need_count,int
l51=0){cL1<1)return;
#ifdef FP_GENERATING_POWI_TABLE
if(l51>32)throw
false;
#endif
if(iF.Plan_Add(value,need_count
y03;long
yD3
1;cL1<POWI_TABLE_SIZE){yD3
powi_table[eX2
y23&128
xQ2
if(yE3
yD3-eY2
FPO(tZ3(i53,"value=%ld, half=%ld, otherhalf=%ld\n",value,half,value/half));l41
half
xW3
iF.lD3
half)eL;}
iZ1
yE3{yD3-eY2}
}
else
cL1&1)yD3
value&((1<<POWI_WINDOW_SIZE)-1);else
yD3
value/2;long
cT=value-half
y23>cT||half<0)std::swap(half,cT);FPO(tZ3(i53,"value=%ld, half=%ld, otherhalf=%ld\n",value,half,otherhalf))y23==cT){l41
half,iF,2,l51+1);}
else{l41
half
xW3
l41
cT>0?cT:-cT
xW3}
iF.lD3
value);}
x33
size_t
yF
c91
l22
cN&t1,xM1{int
yF3=iF.Find(value);if(yF3>=0)yY
yF3;}
long
yD3
1;cL1<POWI_TABLE_SIZE){yD3
powi_table[eX2
y23&128
xQ2
if(yE3
yD3-eY2
FPO(tZ3(i53,"* I want %ld, my plan is %ld * %ld\n",value,half,value/half));size_t
xS2=yF
half
t91
if(iF
lH2
half)>0||xS2!=tA1){iE1
xS2)xR2
half,tA1);}
x41
value/half
x02
size_t
iD3=tA1
xR2
value,iD3);iF.DumpContents()eL
iD3;}
iZ1
yE3{yD3-eY2}
}
else
cL1&1)yD3
value&((1<<POWI_WINDOW_SIZE)-1);else
yD3
value/2;long
cT=value-half
y23>cT||half<0)std::swap(half,cT);FPO(tZ3(i53,"* I want %ld, my plan is %ld + %ld\n",value,half,value-half))y23==cT){size_t
xS2=yF
half
t91
cA1
xS2,half,xS2,half,iF,t1.op_normal,t1.op_normal_flip,synth);}
else{long
part1=half;long
part2=cT>0?cT:-cT;size_t
part1_pos=yF
part1
t91
size_t
part2_pos=yF
part2
t91
FPO(tZ3(i53,"Subdivide(%ld: %ld, %ld)\n",value,half,otherhalf));cA1
part1_pos,part1,part2_pos,part2,iF,cT>0?t1.op_normal:t1.op_inverse,cT>0?t1.op_normal_flip:t1.op_inverse_flip,synth);}
size_t
iD3=tA1
xR2
value,iD3);iF.DumpContents()eL
iD3;}
tD1
cA1
size_t
apos,long
aval,size_t
bpos,long
bval
l22
x83
cumulation_opcode,x83
cumulation_opcode_flip,xM1{int
a_needed=iF
lH2
aval);int
yG3=iF
lH2
bval);bool
flipped=false;
#define DUP_BOTH() do{if(apos<bpos){size_t tmp=apos;apos=bpos;bpos=tmp tB1 FPO(tZ3(i53,"-> "iB3 iB3"op\n",(unsigned)apos,(unsigned)bpos));iE1 apos);iE1 apos==bpos?tA1:bpos);}while(0)
#define DUP_ONE(p) do{FPO(tZ3(i53,"-> "iB3"op\n",(unsigned)p));iE1 p);}while(0)
if(a_needed>0){if(yG3>0){nH2}
cP3
bpos!=tA1)nH2
else{nQ2
tB1}
}
iZ1
yG3>0){if(apos!=tA1)nH2
else
DUP_ONE(bpos);}
cP3
apos==bpos&&apos==tA1)nQ2;iZ1
apos==tA1&&bpos==cY3
xP
2){FPO(tZ3(i53,"-> op\n"))tB1
iZ1
apos==cY3
xP
2&&bpos==tA1)FPO(tZ3(i53,"-> op\n"));iZ1
apos==tA1)DUP_ONE(bpos);iZ1
bpos==tA1){nQ2
tB1
else
nH2}
n41
flipped?cumulation_opcode_flip:cumulation_opcode,2);}
tD1
cM1
long
count,cN&t1,xM1{while
e03<256){int
yD3
xQ1::powi_table[count]y23&128
xQ2
cM1
half
x02
count/=half;}
else
yY3
if
e03==1)return;if(!e03&1)){n41
cSqr,1);cM1
count/2
x02}
else{iE1
tA1);cM1
count-1
x02
n41
cMul,2);}
}
}
lA3
xQ1{tD1
x41
long
count,cN&t1,xM1{if
e03==0)tQ1
t1.basevalue);else{bool
eZ2=false;if
e03<0){eZ2=true;count=-count;}
if(false)cM1
count
x02
iZ1
count>1){PowiCache
iF;l41
count,iF,1);size_t
xS1=cY3
GetStackTop();iF.Start(tA1);FPO(tZ3(i53,"Calculating result for %ld...\n",count));size_t
xT2=yF
count
t91
size_t
n_excess=cY3
xP
xS1;if(n_excess>0||xT2!=xS1-1){cY3
DoPopNMov(xS1-1,xT2);}
}
if(eZ2)n41
t1.op_flip,1);}
}
}
#endif
#ifndef FPOptimizer_ValueRangeHH
#define FPOptimizer_ValueRangeHH
tN{lA3
lF3{iU
e22
Comp{}
;tJ1>e22
Comp<nB
cLess>lE3<nA
cLessOrEq>lE3<=nA
cGreater>lE3>nA
cGreaterOrEq>lE3>=nA
cEqual>lE3==nA
i21>lE3!=b;}
}
;}
x33
e22
rangehalf{x73
val;bool
known;rangehalf():val(),known(false){}
rangehalf(tS1):val(v),known(true){tL1
lG3
tS1){known=true;val=v;}
lG3
x73(eK1
x73
nO
iN1
lG3
x73(eK1
l92
nO
iN1
iU
void
set_if(x73
v,x73(eK1
x73
nO&&lF3::Comp<Compare>()(val,v)iN1
iU
void
set_if(tS1,x73(eK1
l92
nO&&lF3::Comp<Compare>()(val,v)iN1}
n11
e22
range{rangehalf
xN
min,max;range():min(),max(){}
range(x73
mi,x73
ma):min(mi),max(ma){}
range(bool,x73
ma):min(),max(ma){}
range(x73
mi,bool):min(mi),max(){}
void
set_abs();void
set_neg();}
n11
bool
IsLogicalTrueValue
nV2
range
xN&p,bool
abs)n11
bool
IsLogicalFalseValue
nV2
range
xN&p,bool
abs);}
#endif
#ifndef FPOptimizer_RangeEstimationHH
#define FPOptimizer_RangeEstimationHH
tN{enum
TriTruthValue{iI2,IsNever,Unknown}
n11
range
xN
CalculateResultBoundaries
nV2
t2
n11
bool
IsLogicalValue
nV2
t2
n11
TriTruthValue
GetIntegerInfo
nV2
t2
n11
xT1
GetEvennessInfo
nV2
t2{if(!tree
t51)return
Unknown;yZ1=cS3;if(isEvenInteger(value
y33
isOddInteger(value)eK
x33
xT1
GetPositivityInfo
nV2
t2{range
xN
p=CalculateResultBoundaries
yN2
if(p
eT&&p
xQ>=x73(y33
p
lJ1
iY)eK
x33
xT1
GetLogicalValue
eG3
tree,bool
abs){range
xN
p=CalculateResultBoundaries
yN2
if(IsLogicalTrueValue(p,abs
y33
IsLogicalFalseValue(p,abs)eK}
#endif
#ifndef FPOptimizer_ConstantFoldingHH
#define FPOptimizer_ConstantFoldingHH
tN{tD1
ConstantFolding(t2;}
#endif
lA3{using
lA3
FUNCTIONPARSERTYPES;using
tN;e22
ComparisonSetBase{enum{eD3=0x1,Eq_Mask=0x2,Le_Mask=0x3,eE3=0x4,eF3=0x5,Ge_Mask=0x6}
;static
int
Swap_Mask(int
m)yY(m&Eq_Mask)|((m&eD3)?eE3:0)|((m&eE3)?eD3:0);}
enum
cB1{Ok,BecomeZero,BecomeOne,nI1}
;enum
nR2{cond_or,iJ2,iK2,iL2}
;}
n11
e22
ComparisonSet:public
ComparisonSetBase{e22
t02{CodeTree
xN
a;CodeTree
xN
b;int
relationship;t02():a(),b(),relationship(){}
}
;std
xM3<t02>t3;e22
Item{CodeTree
xN
value;bool
cG2;Item():value(),cG2(false){}
}
;std
xM3<Item>cN1;int
xU1;ComparisonSet():t3(),cN1(),xU1(0){}
cB1
AddItem
eG3
a,bool
cG2,nR2
type){for(size_t
c=0;c<cN1.size();++c)if(cN1[c].value
xD
a)){if(cG2!=cN1[c].cG2)iG
cZ1
case
iL2:cN1.erase(cN1.begin()+c);xU1
cH2
case
iJ2:case
iK2:e01}
}
return
nI1;}
Item
pole;pole.value=a;pole.cG2=cG2;cN1.push_back(pole)eL
Ok;}
cB1
AddRelationship(CodeTree
xN
a,CodeTree
xN
b,int
tV1,nR2
type)iG
if(tV1==7)cZ1
lC
iL2:if(tV1==7){xU1
cH2}
lC
iJ2:case
iK2:if(tV1==0)e01
yY3
if(!(a.GetHash()<b.GetHash())){a.swap(b);tV1=Swap_Mask(tV1);}
for(size_t
c=0;c<t3.size();++c){if(t3[c].a
xD
a)&&t3[c].b
xD
b))iG{int
yH3=xU2|tV1;if(yH3==7)cZ1
xU2=yH3
tM2
eP3
iJ2:case
iK2:{int
yH3=xU2&tV1;if(yH3==0)e01
xU2=yH3
tM2
eP3
iL2:{int
newrel_or=xU2|tV1;int
xV2=xU2&tV1;lI2
5&&xV2==0){xU2=eF3
eL
nI1;}
lI2
7&&xV2==0){xU1+=1;t3.erase(t3.begin()+c)eL
nI1;}
lI2
7&&xV2==Eq_Mask){xU2=Eq_Mask;xU1
cH2}
iS2}
return
nI1;}
}
t02
comp;comp.a=a;comp.b=b;comp.relationship=tV1;t3.push_back(comp)eL
Ok;}
}
;nX1
x73,typename
CondType>bool
ConstantFolding_LogicCommon(n91
tree,CondType
xF1,bool
xW2){bool
should_regenerate=false;ComparisonSet
xN
comp;e23{typename
yQ
cB1
e13=yQ
Ok;xG2
atree=t5
a);switch(atree
nC
eJ3
cEqual
lH
Eq_Mask
tN2
i21
lH
eF3
tN2
cLess
lH
eD3
tN2
cLessOrEq
lH
Le_Mask
tN2
cGreater
lH
eE3
tN2
cGreaterOrEq
lH
Ge_Mask
tN2
cNot:e13
e11
l8
0),true
tN2
cNotNot:e13
e11
l8
0),false,xF1
tK2
default:if(xW2||IsLogicalValue(atree))e13
e11,false,xF1);iY2
e13){ReplaceTreeWithZero:iH3
0)eL
true;ReplaceTreeWithOne:iH3
1);x0
yQ
Ok:lC
yQ
BecomeZero
cQ
yQ
BecomeOne:iE
yQ
nI1:cF1
yY3}
if(should_regenerate){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before ConstantFolding_LogicCommon: "
nX
#endif
if(xW2){tree.DelParams();}
else{for
yP{xG2
atree=t5
a);if(IsLogicalValue(atree))nG1);}
}
for
c12
0;a<comp.cN1.size();++a){if(comp.cN1[a].cG2){cC1
cNot);r.cE
r.t7
iZ1!xW2){cC1
cNotNot);r.cE
r.t7
else
tree.cE}
for
c12
0;a<comp.t3.size();++a){cC1
cNop);switch(comp.t3[a
eE
eJ3
yQ
eD3:r
xC
cLess
xX3
Eq_Mask:r
xC
cEqual
xX3
eE3:r
xC
cGreater
xX3
Le_Mask:r
xC
cLessOrEq
xX3
eF3:r
xC
i21
xX3
Ge_Mask:r
xC
cGreaterOrEq
tK2}
r
lP1
comp.t3[a].a);r
lP1
comp.t3[a].b);r.t7
if(comp.xU1!=0)tree.yJ
x73(comp.xU1)));
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After ConstantFolding_LogicCommon: "
nX
#endif
l62}
return
eO3
xV1
ConstantFolding_AndLogic(iA3(tree.i33()==cAnd||tree.i33()==cAbsAnd)eL
nM
iJ2,true);}
xV1
ConstantFolding_OrLogic(iA3(tree.i33()==cOr||tree.i33()==cAbsOr)eL
nM
cond_or,true);}
xV1
ConstantFolding_AddLogicItems(iA3(tree.i33()==cAdd)eL
nM
iL2,false);}
xV1
ConstantFolding_MulLogicItems(iA3(tree.i33()==cMul)eL
nM
iK2,false);}
}
#include <vector>
#include <map>
#include <algorithm>
lA3{using
lA3
FUNCTIONPARSERTYPES;using
tN;e22
CollectionSetBase{enum
xW1{Ok,nI1}
;}
n11
e22
CollectionSet:public
CollectionSetBase{e22
cD1{CodeTree
xN
value;CodeTree
xN
xX2;bool
e0;cD1():value(),xX2(),e0(false){}
cD1
eG3
v,xG2
f):value(v),xX2(f),e0(false){}
}
;std::multimap<xO2,cD1>iH;typedef
typename
std::multimap<xO2,cD1>::yN3
xX1;CollectionSet():iH(){}
xX1
FindIdenticalValueTo
eG3
value){xO2
hash=value.GetHash();for(xX1
i=iH.xY2
hash);i!=iH.e21
hash;++i){cL1
xD
i
e82.value
y03
i;}
return
iH.end();}
bool
Found
nV2
xX1&b)yY
b!=iH.end();}
xW1
AddCollectionTo
eG3
xX2,const
xX1&into_which){cD1&c=into_which
e82;if(c.e0)c.xX2
cO
xX2);else{CodeTree
xN
add;add
xC
cAdd);add
lP1
c.xX2);add
cO
xX2);c.xX2.swap(add);c.e0=true;}
return
nI1;}
xW1
lJ2
xG2
value,xG2
xX2){const
xO2
hash=value.GetHash();xX1
i=iH.xY2
hash);for(;i!=iH.e21
hash;++i){if(i
e82.value
xD
value
y03
AddCollectionTo(xX2,i);}
iH.yI3,std::make_pair(hash,cD1(value,xX2)))eL
Ok;}
xW1
lJ2
xG2
a)yY
lJ2
a,nJ1
1)));}
}
n11
e22
ConstantExponentCollection{typedef
eZ
yL3;typedef
std::x81
xZ2;std
xM3<xZ2>data;ConstantExponentCollection():data(){}
void
MoveToSet_Unique(l92
eL1&eM1){data.push_back(std::x81(eL1()));data.back()xU3.swap(eM1
n13
MoveToSet_NonUnique(l92
eL1&eM1){typename
std
xM3<xZ2>::yN3
i=std::xY2
data.iM2
data.end(),x93,Compare1st());if(i!=data.e21
xK2{i
e82.yI3
e82.end(),eM1.iM2
eM1.end());}
else{data.yI3,std::x81(x93,eM1));}
}
bool
Optimize(){bool
changed=false;std::sort(data.iM2
data.end(),Compare1st());redo:for
c12
0;a<data.size();++a
yZ2
exp_a=data[a
eM3;iZ2
exp_a,x73(1)))continue;for(size_t
b=a+1;b<data.size();++b
yZ2
exp_b=data[b
eM3
iO2
y02=exp_b-exp_a;if(y02>=fp_abs(exp_a))break
iO2
exp_diff_still_probable_integer=y02*x73(16);if(t12
exp_diff_still_probable_integer)&&!(t12
exp_b)&&!t12
y02))){yL3&a_set=lK2;yL3&b_set=data[b]xU3;
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before ConstantExponentCollection iteration:\n"
;t42
cout);
#endif
if(isEvenInteger(exp_b)&&!isEvenInteger(y02+exp_a
nK
tmp2;tmp2
xC
cMul);tmp2
i81
b_set);tmp2
x53
tmp;x13
cAbs);tmp
nR3;tmp
x22);b_set.e93
1);b_set[0].nS3;}
a_set.insert(a_set.end(),b_set.iM2
b_set.end());yL3
b_copy=b_set;data.erase(data.begin()+b);MoveToSet_NonUnique(y02,b_copy);xD2
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After ConstantExponentCollection iteration:\n"
;t42
cout);
#endif
goto
redo;}
}
}
return
changed;}
#ifdef DEBUG_SUBSTITUTIONS
void
t42
ostream&out){for
c12
0;a<data.size();++a){out.precision(12);out<<data[a
eM3<<": "
;eN3
lK2.size();++b){if(b>0)out<<'*'i63
lK2[b],out);}
out<<std::endl;}
}
#endif
}
n11
static
CodeTree
xN
x91
n91
value,bool&xT
t63
value
nC
eJ3
cPow
yE1
xN
e92
value
l8
1);value.yQ1
eL
x93;eP3
cRSqrt:value.yQ1;xT=true
eL
nJ1-0.5));case
cInv:value.yQ1;xT=true
eL
nJ1-1));default:yY3
return
nJ1
1));}
cJ1
void
eN1
eO1&mul,xG2
tree,xG2
xX2,bool&cE1
bool&xT){e23{CodeTree
xN
value(t5
a)cP
x93(x91
value,xT));if(!xX2
t51||xX2.xY1!=x73(1.0
nK
e41;e41
xC
cMul);e41
cO
xK2;e41
cO
xX2);e41
x22)cX1.swap(e41);}
#if 0 /* FIXME: This does not work */
cL1
nC==cMul){if(1){bool
exponent_is_even=x93
t51&&isEvenInteger(x93.xY1);eN3
value
yO3{bool
tmp=false;CodeTree
xN
val(value
l8
b)cP
exp(x91
val,tmp));if(exponent_is_even||(exp
t51&&isEvenInteger(exp.xY1)nK
e41;e41
xC
cMul);e41
cO
xK2;e41
lP1
exp);e41.ConstantFolding();if(!e41
t51||!isEvenInteger(e41.xY1)){goto
cannot_adopt_mul;}
}
}
}
eN1
mul,value,x93,cE1
xT);}
else
cannot_adopt_mul:
#endif
{if(mul.lJ2
value,xK2==CollectionSetBase::nI1)cF1}
}
}
xV1
ConstantFolding_MulGrouping(t2{bool
xT=false;bool
should_regenerate=false;eO1
mul;eN1
mul
e53,nJ1
1)),cE1
xT);typedef
std::pair<CodeTree
xN,eZ>eP1;typedef
std::multimap<xO2,eP1>cO1;cO1
iI;y12
eO1::xX1
j=mul.iH.yM3
j!=mul.iH.end();++j){n91
value=j
e82.value;n91
e92
j
e82.xX2;if(j
e82.e0)x93
x22);const
xO2
eQ1=x93.GetHash();typename
cO1::yN3
i=iI.xY2
eQ1);for(;i!=iI.e21
eQ1;++i)if(i
e82.first
xD
xK2){if(!x93
t51||!e51.xY1,x73(1)))cF1
i
e82
xU3.push_back(value);goto
skip_b;}
iI.yI3,std::make_pair(eQ1,std::make_pair(x93,eZ(size_t(1),value))));skip_b:;}
#ifdef FP_MUL_COMBINE_EXPONENTS
ConstantExponentCollection
xN
e61;y12
cO1::yN3
j,i=iI.yM3
i!=iI.end();i=j){j=i;++j;eP1&list=i
e82;if
yQ3
lM1
e92
list.first.xY1;if(!(x93==xH1)e61.MoveToSet_Unique(x93,list
xS3
iI.erase(i);}
}
if(e61.Optimize())cF1
#endif
if(should_regenerate
t71
before=tree;before.l61
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before ConstantFolding_MulGrouping: "
i63
before)xI1"\n"
;
#endif
tree.DelParams();y12
cO1::yN3
i=iI.yM3
i!=iI.end();++i){eP1&list=i
e82;
#ifndef FP_MUL_COMBINE_EXPONENTS
if
yQ3
lM1
e92
list.first.xY1;if(x93==xH1
continue;if(e51
nB2
tree.AddParamsMove(list
xU3
iQ2}
#endif
CodeTree
xN
mul;mul
xC
cMul);mul
i81
list
xS3
mul
x22);if(xT&&list.first
t51){if
yQ3.xY1==x73(1)/x73(3
nK
cbrt;cbrt
xC
cCbrt);cbrt.e9
cbrt
iJ3
cbrt)y7
0.5
nK
sqrt;sqrt
xC
cSqrt);sqrt.e9
sqrt
iJ3
sqrt)y7-0.5
nK
rsqrt;rsqrt
xC
cRSqrt);rsqrt.e9
rsqrt
iJ3
rsqrt)y7-1
nK
inv;inv
xC
cInv);inv.e9
inv
iJ3
inv
iQ2}
CodeTree
xN
pow;pow
xC
cPow);pow.e9
pow
lP1
list.first);pow
iJ3
pow);}
#ifdef FP_MUL_COMBINE_EXPONENTS
iI.clear()c02
0;a<iC.size();++a
yZ2
e92
iC[a
eM3;if(e51
nB2
tree.AddParamsMove(iC[a]xU3
iQ2
CodeTree
xN
mul;mul
xC
cMul);mul
i81
iC[a]xS3
mul
x53
pow;pow
xC
cPow);pow.e9
pow.yJ
xK2);pow
iJ3
pow);}
#endif
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After ConstantFolding_MulGrouping: "
nX
#endif
return!tree
xD
before);}
return
eO3
xV1
ConstantFolding_AddGrouping(t2{bool
should_regenerate=false;eO1
add;e23{if(t5
a)nC==cMul
c42
add.lJ2
t5
a))==CollectionSetBase::nI1)cF1}
xP3
remaining(t6);size_t
tU=0;e23{xG2
xA3=t5
a);if(xA3
nC==cMul){eN3
xA3
yO3{if(xA3
l8
b)t51)continue;typename
eO1::xX1
c=add.FindIdenticalValueTo(xA3
l8
b));if(add.Found(c
nK
tmp(xA3
yG
CloneTag());tmp.l93
b);tmp
x22);add.AddCollectionTo(tmp,c);cF1
goto
done_a;}
}
remaining[a]=true;tU+=1;done_a:;}
}
if(tU>0){if(tU>1){std
xM3<std::pair<CodeTree
xN,size_t> >nZ;std::multimap<xO2,size_t>eR1;bool
lH3=false;e23
l32{eN3
t5
a)yO3{xG2
p=t5
a)l8
b);const
xO2
p_hash=p.GetHash();for(std::multimap<xO2,size_t>::const_iterator
i=eR1.xY2
p_hash);i!=eR1.e21
p_hash;++i){if(nZ[i
e82
eM3
xD
p)){nZ[i
e82]xU3+=1;lH3=true;goto
found_mulgroup_item_dup;}
}
nZ.push_back(std::make_pair(p,size_t(1)));eR1.insert(std::make_pair(p_hash,nZ.size()-1));found_mulgroup_item_dup:;}
}
if(lH3
t71
eA2;{size_t
max=0;for(size_t
p=0;p<nZ.size();++p)if(nZ[p]xU3<=1)nZ[p]xU3=0;else{nZ[p]xU3*=nZ[p
eM3.y82;if(nZ[p]xU3>max){eA2=nZ[p
eM3;max=nZ[p]xU3;}
}
}
CodeTree
xN
group_add;group_add
xC
cAdd);
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Duplicate across some trees: "
i63
eA2)xI1" in "
nX
#endif
e23
l32
eN3
t5
a)yO3
if(eA2
xD
t5
a)l8
b)nK
tmp(t5
a)yG
CloneTag());tmp.l93
b);tmp
x22);group_add
lP1
tmp);remaining[a]=false
tM2}
group_add
x53
group;group
xC
cMul);group
lP1
eA2);group
lP1
group_add);group
x22);add.lJ2
group);cF1}
}
e23
l32{if(add.lJ2
t5
a))==CollectionSetBase::nI1)cF1}
}
if(should_regenerate){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before ConstantFolding_AddGrouping: "
nX
#endif
tree.DelParams();y12
eO1::xX1
j=add.iH.yM3
j!=add.iH.end();++j){n91
value=j
e82.value;n91
coeff=j
e82.xX2;if(j
e82.e0)coeff
x22);if(coeff
t51){iZ2
coeff.xY1,xH1
c42
lN3
coeff.xY1
nB2
tree
lP1
value
iQ2}
CodeTree
xN
mul;mul
xC
cMul);mul
lP1
value);mul
lP1
coeff);mul
x22);tree.e9}
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After ConstantFolding_AddGrouping: "
nX
#endif
l62}
return
eO3}
lA3{using
lA3
FUNCTIONPARSERTYPES;using
tN
n11
bool
ConstantFolding_IfOperations(iA3(tree.i33()==cIf||tree.i33()==iF3);for(;;){if(nC3==cNot){tW2
cIf);t5
0).eB2
0)y22
t5
1).swap(t5
2));}
iZ1
t5
0)cU1{tW2
iF3);t5
0).eB2
0)y22
t5
1).swap(t5
2));}
else
c22
lZ1
0),tC2==iF3))i51
tree.eB2
1));x0
l63
tree.eB2
2));x0
n01
if(nC3==cIf||nC3==iF3
t71
cond=t5
0
cP
lI3;lI3
tY2==cIf?cNotNot:cAbsNotNot);lI3
y42
1));ConstantFolding(lI3
cP
lJ3;lJ3
tY2==cIf?cNotNot:cAbsNotNot);lJ3
y42
2));ConstantFolding(lJ3);if(lI3
t51||lJ3.IsImmed(nK
t8;t8
tY2);t8
y42
1));t8
tO3
t8.nN
2));t8
x53
t9;t9
tY2);t9
y42
2));t9
tO3
t9.nN
2));t9
x22);tW2
cond
nC)l02
0,cond
y22
iN2
1,t8);iN2
2,t9
iX1}
if(t5
1)nC==t5
2)nC&&(t5
1)nC==cIf||t5
1)nC==iF3
nK&iG2=t5
1
cP&leaf2=t5
2);if(iG2
l8
0)xG1
0))&&(iG2
l8
1)xG1
1))||iG2
l8
2)xG1
2))nK
t8;t8
l42
t8.nN
0));t8
cO
iG2
l8
1));t8
cO
leaf2
l8
1));t8
x53
t9;t9
l42
t9.nN
0));t9
cO
iG2
l8
2));t9
cO
leaf2
l8
2));t9
eR
SetParam(0,iG2
y22
iN2
1,t8);iN2
2,t9
iX1
if(iG2
l8
1)xG1
1))&&iG2
l8
2)xG1
2)nK
tA;tA
l42
tA
lP1
t5
0));tA
cO
iG2
y22
tA
cO
leaf2
y22
tA
eR
nK1
0,tA)l02
2,iG2
l8
2))l02
1,iG2
l8
1)iX1
if(iG2
l8
1)xG1
2))&&iG2
l8
2)xG1
1)nK
eC2;eC2
xC
leaf2
nC==cIf?cNot:cK3);eC2
cO
leaf2
y22
eC2
x53
tA;tA
l42
tA
lP1
t5
0));tA
cO
iG2
y22
tA
lP1
eC2);tA
eR
nK1
0,tA)l02
2,iG2
l8
2))l02
1,iG2
l8
1)iX1}
n91
y6=t5
1
cP&yC=t5
2);if(y6
xD
yC)){tree.eB2
1)iX1
const
OPCODE
op1=y6
nC;const
OPCODE
op2=yC
nC;if
c23
op2){if(y6
yF1==1
t71
lO
0));y52
0))lL2
n6
if(y6
yF1==2&&yC
yF1==2){if(y6
l8
0
nA3
0)nK
param0=y6
l8
0
cP
lO
1));y52
1))lL2
l12
param0)n6
if(y6
l8
1
nA3
1)nK
param1=y6
l8
1
cP
lO
0));y52
0))lL2
l12
xO1)l12
param1
iX1}
if
c23
yU3
cMul
lM2
cAnd
lM2
cOr
lM2
cAbsAnd
lM2
cAbsOr
lM2
cMin
lM2
cMax){eZ
lK3;c9{for(size_t
b=yC
yF1;b-->0;){if(y6
l8
a
nA3
b))){if(lK3
tK3){y6.l61
yC.l61}
lK3.push_back(y6
nF3
yC.l93
b);y6
nC2
yY3}
}
if(!lK3
tK3){y6
x22);yC
x53
xO1;xO1
l42
xO1
i81
tree.lB2))c7
SetParamsMove(lK3)n6}
}
if
c23
yU3
cMul||c23
cAnd
nZ1
yC))||c23
cOr
nZ1
yC))){c9
if(y6
l8
a)xD
yC)){y6.l61
y6
nC2
y6
x53
cP1=yC;yC=tV
op1==yU3
cOr)l9
op1)l12
cP1)n6}
if(c23
cAnd
lM2
cOr)&&op2==cNotNot){n91
lL3=yC
l8
0);c9
if(y6
l8
a)xD
lL3)){y6.l61
y6
nC2
y6
x53
cP1=lL3;yC=tV
op1
yV3
op1)l12
cP1)n6}
if(op2==cAdd||op2==cMul||(op2==cAnd
nZ1
y6))||(op2==cOr
nZ1
y6))){for
c12
yC
lU1
yC
l8
a)xD
y6)){yC.l61
yC
nC2
yC
x53
cQ1=y6;y6=tV
op2==cAdd||op2
yV3
op2)l12
cQ1)n6}
if((op2==cAnd||op2==cOr)&&op1==cNotNot){n91
lM3=y6
l8
0)c02
yC
lU1
yC
l8
a)xD
lM3)){yC.l61
yC
nC2
yC
x53
cQ1=lM3;y6=tV
op2
yV3
op2)l12
cQ1)n6}
return
eO3}
#include <limits>
lA3{using
lA3
FUNCTIONPARSERTYPES;using
tN
n11
int
maxFPExponent()yY
std::numeric_limits
xN::max_exponent;}
xV1
xA1
x73
base,x73
xK2{if(base<xH1
l62
iZ2
base,xH1||lN3
base,x73(1)y03
false
eL
x93>=x73(maxFPExponent
xN())/fp_log2(base);}
xV1
ConstantFolding_PowOperations(iA3(tree.i33()==cPow);nU&&t5
1)lM1
const_value=eT3
lS,n51);iH3
const_value)eL
eO3
if(eG1
lN3
n51
nB2
tree.eB2
0)iX1
nU&&lN3
lS
nB2
iH3
1)eL
eO3
nU&&t5
1)nC==cMul){bool
y62=false
iO2
lO2=lS;CodeTree
xN
xA3=t5
1)c02
xA3
lU1
xA3
l8
a)lM1
imm=xA3
l8
a).xY1;{if(xA1
lO2,imm))break
iO2
lP2=eT3
lO2,imm);iZ2
lP2,xH1)break;if(!y62){y62=true;yJ1
l61}
lO2=lP2;yJ1
l93
a
tK2}
}
if(y62){yJ1
xB3;
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before pow-mul change: "
nX
#endif
t5
0).Become(e71
lO2));t5
1).Become(xA3);
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After pow-mul change: "
nX
#endif
}
}
if(eG1
nC3==cMul
yZ2
lQ2=n51
iO2
y72=1.0;bool
y62=false;n91
xA3=t5
0)c02
xA3
lU1
xA3
l8
a)lM1
imm=xA3
l8
a).xY1;{if(xA1
imm,lQ2))break
iO2
eS1=eT3
imm,lQ2);iZ2
eS1,xH1)break;if(!y62){y62=true;yJ1
l61}
y72*=eS1;yJ1
l93
a
tK2}
}
if(y62){yJ1
Rehash(cP
e33;e33
xC
cPow);e33
i81
tree.lB2));e33
t82;tW2
cMul)l12
e33);tree
cO
e71
y72)iX1}
if(nC3==cPow&&eG1
t5
0)l8
1)lM1
a=t5
0)l8
1).xY1
iO2
b=n51
iO2
c=a*b;if(isEvenInteger(a)&&!isEvenInteger(c
nK
lO3;lO3
xC
cAbs);lO3.nN
0)y22
lO3
x22);iN2
0,lO3);}
else
tree.SetParam(0,t5
0)l8
0))l02
1,e71
c));}
return
eO3}
lA3{using
lA3
FUNCTIONPARSERTYPES;using
tN;e22
l4{enum
eD2{MakeFalse=0,MakeTrue=1,tB2=2,lP3=3,MakeNotNotP0=4,MakeNotNotP1=5,MakeNotP0=6,MakeNotP1=7,xS=8}
;enum
lR2{Never=0,Eq0=1,Eq1=2,yW3=3,yX3=4}
;eD2
if_identical;eD2
lS2
4];e22{eD2
what:4;lR2
when:4;}
iF1,iG1,iH1,iI1
n11
eD2
Analyze
eG3
a,xG2
b)const{if(a
xD
b
y03
if_identical;range
xN
i3
a);range
xN
p1=CalculateResultBoundaries(b);if(p0
lJ1&&p1
eT){if(p0
lS1<p1
xQ&&lS2
0]iD
0];if(p0
lS1<=p1
xQ&&lS2
1]iD
1];}
if(tQ3
p1
lJ1){if(p0
xQ>p1
lS1&&lS2
2]iD
2];if(p0
xQ>=p1
lS1&&lS2
3]iD
3];}
if(IsLogicalValue(a)){if(iF1
lT1
iF1.when,p1
y03
iF1.what;if(iH1
lT1
iH1.when,p1
y03
iH1.what;}
if(IsLogicalValue(b)){if(iG1
lT1
iG1.when,p0
y03
iG1.what;if(iI1
lT1
iI1.when,p0
y03
iI1.what;}
return
xS;}
cJ1
bool
TestCase(lR2
when,const
range
xN&p){if(!p
eT||!p
lJ1
eM
switch(when
eJ3
Eq0:y13==x73(0.0)eU3==p
xQ;case
Eq1:y13==x73(1.0)eU3==p
lS1;case
yW3:y13>yD1
eU3<=x73(1);case
yX3:y13>=yD1
iY
1);default:;}
return
eO3}
;lA3
RangeComparisonsData{static
const
l4
Data[6]={{l4
eV3::i5
xS,l4::i5
xS}
,{l4::x01
Eq1
lV1
Eq1
lW1
Eq0
lX1
Eq0}
}
,{l4::nS2
eW3
xS,l4
eW3
xS}
,{l4::x01
Eq0
lV1
Eq0
lW1
Eq1
lX1
Eq1}
}
,{l4::nS2
eW3
tB2,l4::i5
MakeFalse
lW1
yW3
lV1
yX3
c2,{l4
eV3::xS,l4
eW3
i5
lP3
lW1
yX3
lV1
yW3
c2,{l4::nS2::i5
i5
MakeTrue,l4::tB2}
,{l4::x01
yX3
lX1
yW3
c2,{l4
eV3::i5
lP3,l4::xS,l4
nL1}
,{l4::x01
yW3
lX1
yX3
c2}
;}
xV1
ConstantFolding_Comparison(t2{using
lA3
RangeComparisonsData;assert(tree.i33()>=cEqual&&tree.i33()<=cGreaterOrEq);switch(Data[tC2-cEqual].Analyze(t5
0),t5
1))eJ3
l4::MakeFalse:iH3
0)xT3
nL1:iH3
1)xT3::lP3:tW2
cEqual)xT3::tB2:tW2
i21)xT3::MakeNotNotP0:tW2
cNotNot
y01
1)xT3::MakeNotNotP1:tW2
cNotNot
y01
0)xT3::MakeNotP0:tW2
cNot
y01
1)xT3::MakeNotP1:tW2
cNot
y01
0)xT3::xS:;}
if(t5
1)t51)switch(nC3
eJ3
cAsin:lM
fp_sin(eE2
cAcos:lM
fp_cos(n51)));tW2
tC2==cLess?cGreater:tC2==cLessOrEq?cGreaterOrEq:tC2==cGreater?cLess:tC2==cGreaterOrEq?cLessOrEq:tC2);x0
cAtan:lM
fp_tan(eE2
cLog:lM
fp_exp(eE2
cSinh:lM
fp_asinh(eE2
cTanh:if(fp_less(fp_abs(n51)nB2
lM
fp_atanh(n51))iX1
break;default:yY3
return
eO3}
#include <list>
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
using
lA3
FUNCTIONPARSERTYPES;lA3{
#ifdef DEBUG_SUBSTITUTIONS
yL
double
d){union{double
d;uint_least64_t
h;cI2
d=d;lG1
h
x11
#ifdef FP_SUPPORT_FLOAT_TYPE
yL
float
f){union{float
f;uint_least32_t
h;cI2
f=f;lG1
h
x11
#endif
#ifdef FP_SUPPORT_LONG_DOUBLE_TYPE
yL
long
double
ld){union{long
double
ld;e22{uint_least64_t
a;x83
short
b;}
s;cI2
ld=ld;lG1
s.b<<data.s.a
x11
#endif
#ifdef FP_SUPPORT_LONG_INT_TYPE
yL
long
ld){o<<"("
<<std::hex<<ld
x11
#endif
#endif
}
tN{lN
nE)){}
lN
l92
i
yG
xF3
nE
i
eK3
#ifdef __GXX_EXPERIMENTAL_CXX0X__
lN
x73&&i
yG
xF3
nE
tD2
i)eK3
#endif
lN
x83
v
yG
VarTag
nE
iT2,v
eK3
lN
n92
o
yG
OpcodeTag
nE
o
eK3
lN
n92
o,x83
f
yG
FuncOpcodeTag
nE
o,f
eK3
lN
xG2
b
yG
CloneTag
nE*b.data)){}
x33
CodeTree
xN::~CodeTree(){}
lB
ReplaceWithImmed(tU1{
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Replacing "
i63*this);if(IsImmed())OutFloatHex(std::cout,xY1)xI1" with const value "
<<i;OutFloatHex(std::cout,i)xI1"\n"
;
#endif
data=new
xL2
xN(i);}
x33
e22
ParamComparer{iH2()eG3
a,xG2
b)const{if(a.y82!=b.y82)return
a.y82<b.y82
eL
a.GetHash()<b.GetHash();}
}
xI3
xL2
xN::Sort(t63
Opcode
eJ3
cAdd:case
cMul:case
cMin:case
cMax:case
cAnd:case
cAbsAnd:case
cOr:case
cAbsOr:case
cHypot:case
cEqual:case
i21:std::sort(nG3
iM2
nG3
end(),ParamComparer
xN());lC
cLess
lZ
cGreater;}
lC
cLessOrEq
lZ
cGreaterOrEq;}
lC
cGreater
lZ
cLess;}
lC
cGreaterOrEq
lZ
cLessOrEq;}
break;default:yY3}
lB
AddParam
eG3
param){y8.push_back(param);}
lB
eI1
n91
param){y8.push_back(CodeTree
xN());y8.back().swap(param);}
lB
SetParam(size_t
which,xG2
b)nM1
which
iP2
y8[which]=b;}
lB
nK1
size_t
which,n91
b)nM1
which
iP2
y8[which
tD3
b);}
lB
AddParams
nV2
nP){y8.insert(y8.end(),lT2.iM2
lT2.end());}
lB
AddParamsMove(nP){size_t
endpos=y8.size(),added=lT2.size();y8.e93
endpos+added,CodeTree
xN());for(size_t
p=0;p<added;++p)y8[endpos+p
tD3
lT2[p]);}
lB
AddParamsMove(nP,size_t
lU2)nM1
lU2
iP2
l93
lU2);AddParamsMove(tI1}
lB
SetParams
nV2
nP){eZ
tmp(tI1
y8.nS3;}
lB
SetParamsMove(nP){y8.swap(tI1
lT2.clear();}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
lB
SetParams(eZ&&lT2){SetParamsMove(tI1}
#endif
lB
l93
size_t
index){eZ&Params=y8;
#ifdef __GXX_EXPERIMENTAL_CXX0X__
nG3
erase(nG3
begin()+index);
#else
eY3
index].data=0;for(size_t
p=index;p+1<yP3;++p)eY3
p]tE3
UnsafeSetP(&*eY3
p+1
iP2
eY3
yP3-1]tE3
UnsafeSetP(0);nG3
e93
yP3-1);
#endif
}
lB
DelParams(){y8.clear();}
xV1
CodeTree
xN::IsIdenticalTo
eG3
b)const{if(&*data==&*b.data)return
true
eL
data->IsIdenticalTo(*b.data);}
xV1
xL2
xN::IsIdenticalTo
nV2
xL2
xN&b)const{if(Hash!=b.Hash
eM
if(Opcode!=tG3
eM
switch(Opcode
yS3
return
lN3
Value,tH3;case
iT2:return
lD2==b.lC2
case
cFCall:case
cPCall:if(lD2!=b.lD2
eM
break;default:yY3
if(yP3!=b.yP3
eM
for
c12
0;a<yP3;++a){if(!eY3
a]xD
b.eY3
a])eM}
l62}
lB
Become
eG3
b){if(&b!=this&&&*data!=&*b.data){DataP
tmp=b.data;l61
data.nS3;}
}
lB
CopyOnWrite(){if(GetRefCount()>1)data=new
xL2
xN(*data);}
x33
CodeTree
xN
CodeTree
xN::GetUniqueRef(){if(GetRefCount()>1)return
CodeTree
xN(*this,CloneTag())eL*this;}
i6):yZ
cNop
t32),n9
i6
const
xL2&b):yZ
tG3
t32
tH3,lD2(b.cS1,eX3
b.Params),Hash(b.Hash),Depth(b.Depth),tW1
b.lE2){}
i6
tU1:yZ
cImmed
t32
i),n9
#ifdef __GXX_EXPERIMENTAL_CXX0X__
i6
xL2
xN&&b):yZ
tG3
t32
tD2
tH3),lD2(b.cS1,eX3
tD2
b.Params)),Hash(b.Hash),Depth(b.Depth),tW1
b.lE2){}
i6
x73&&i):yZ
cImmed
t32
tD2
i)),n9
#endif
i6
n92
o):yZ
o
t32),n9
i6
n92
o,x83
f):yZ
o
t32),lD2(f),eX3),Hash(),Depth(1),tW1
0){}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <iostream>
using
lA3
FUNCTIONPARSERTYPES;
#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
lA3{tD1
tX1
nS,std
cF&done,std::ostream&o){e23
tX1
t5
a),done,o);std::ostringstream
buf
i63
tree,buf);done[tree.GetHash()].insert(buf.str());}
}
#endif
tN{
#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
tD1
DumpHashes(cJ){std
cF
done;tX1
tree,done,o);for(std
cF::const_iterator
i=done.yM3
i!=done.end();++i){const
std::set<std::string>&flist=i
e82;if(flist.size()!=1)o<<"ERROR - HASH COLLISION?\n"
;for(std::set<std::string>::const_iterator
j=flist.yM3
j!=flist.end();++j){o<<'['<<std::hex<<i->first.hash1<<','<<i->first.hash2<<']'<<std::dec;o<<": "
<<*j<<"\n"
;}
}
}
tD1
DumpTree(cJ){l03
i43;switch(tC2
yS3
o<<cS3
eL;case
iT2:o<<"Var"
<<(tree.GetVar()-iT2)eL;case
cAdd:i43"+"
;lC
cMul:i43"*"
;lC
cAnd:i43"&"
;lC
cOr:i43"|"
;lC
cPow:i43"^"
tM2
default:i43;o<<FP_GetOpcodeName(tC2);l33
cFCall||tC2==cPCall)o<<':'<<tree.GetFuncNo();}
o<<'(';if(t6<=1&&sep2[1])o<<(sep2+1)<<' ';e23{if(a>0)o<<' 'i63
t5
a),o);if(a+1<t6)o<<sep2;}
o<<')';}
tD1
DumpTreeWithIndent(cJ,const
std::string&indent){o<<'['<<std::hex<<(void*)(&tree.lB2))<<std::dec<<','<<tree.GetRefCount()<<']';o<<indent<<'_';switch(tC2
yS3
o<<"cImmed "
<<cS3;o<<'\n'eL;case
iT2:o<<"VarBegin "
<<(tree.GetVar()-iT2);o<<'\n'eL;default:o<<FP_GetOpcodeName(tC2);l33
cFCall||tC2==cPCall)o<<':'<<tree.GetFuncNo();o<<'\n';}
e23{std::string
ind=indent;for(size_t
p=0;p<ind.size();p+=2)if(ind[p]=='\\')ind[p]=' ';ind+=(a+1<t6)?" |"
:" \\"
;DumpTreeWithIndent(t5
a),o,ind);}
o<<std::flush;}
#endif
}
#endif
using
lA3
l21;using
lA3
FUNCTIONPARSERTYPES;
#include <cctype>
lA3
l21{xV1
ParamSpec_Compare
nV2
void*aa,const
void*bb,nN2
type
t63
type
eJ3
nT2
xU&a=*(xU*)aa;xU&b=*(xU*)bb
eL
a
t62==b
t62&&a.index==b.index&&a.depcode==b.depcode;eP3
NumConstant:{tB&a=*(tB*)aa;tB&b=*(tB*)bb
eL
lN3
a
cJ2,b
cJ2)&&a.modulo==b.modulo;}
cG1
xV&a=*(xV*)aa;xV&b=*(xV*)bb
eL
a
t62==b
t62&&a
c51==b
c51&&a
tE3
match_type==b
tE3
match_type&&a.data
yH2==b.data
yH2&&a
tE3
param_list==b
tE3
param_list&&a
tE3
n2==b
tE3
n2&&a.depcode==b.depcode;}
}
l62}
x83
ParamSpec_GetDepCode
nV2
e52&b
t63
b.first
eJ3
nT2
t83*s=nV2
xU*)b
xU3
eL
s->depcode;}
cG1
const
xV*s=nV2
xV*)b
xU3
eL
s->depcode;}
default:yY3
return
0;}
tD1
DumpParam
nV2
e52&yI2
std::ostream&o){static
const
char
ParamHolderNames[][2]={"%"
,"&"
,"x"
,"y"
,"z"
,"a"
,"b"
,"c"
}
;x83
tA2
0;switch(tF3
eJ3
NumConstant:{const
tB&nU2
tB
nW2
using
lA3
FUNCTIONPARSERTYPES;o.precision(12);o<<param
cJ2
tM2
eP3
nT2
t83&nU2
xU
nW2
o<<ParamHolderNames[param.index];tA2
param
t62
tM2}
cG1
const
xV&nU2
xV
nW2
tA2
param
t62;yM
GroupFunction){c33
c51==cNeg){o<<"-"
;n3}
iZ1
param
c51==cInv){o<<"/"
;n3}
else{std::string
opcode=FP_GetOpcodeName((n92)param
c51).substr(1)c02
0;a<opcode.size();++a)opcode[a]=(char)std::toupper(opcode[a]);o<<opcode<<"( "
;n3
o<<" )"
;}
}
else{o<<'('<<FP_GetOpcodeName((n92)param
c51)<<' ';yM
PositionalParams)o<<'[';yM
SelectedParams)o<<'{';n3
c33
tE3
n2!=0)o<<" <"
<<param
tE3
n2<<'>';yM
PositionalParams)o<<"]"
;yM
SelectedParams)o<<"}"
;o<<')';}
yY3
iY2
ImmedConstraint_Value(iJ1
ValueMask)eJ3
ValueMask:lC
Value_AnyNum:lC
x32:o<<"@E"
;lC
Value_OddInt:o<<"@O"
;lC
tZ1:o<<"@I"
;lC
Value_NonInteger:o<<"@F"
;lC
eT1:o<<"@L"
;c22
ImmedConstraint_Sign(iJ1
SignMask)eJ3
SignMask:lC
Sign_AnySign:lC
nN1:o<<"@P"
;lC
eU1:o<<"@N"
;c22
ImmedConstraint_Oneness(iJ1
OnenessMask)eJ3
OnenessMask:lC
Oneness_Any:lC
Oneness_One:o<<"@1"
;lC
Oneness_NotOne:o<<"@M"
;c22
ImmedConstraint_Constness(iJ1
ConstnessMask)eJ3
ConstnessMask:lC
tY1:if(tF3==ParamHolder){t83&nU2
xU
nW2
c33.index<2)yY3
o<<"@C"
;lC
Constness_NotConst:o<<"@V"
;lC
Oneness_Any:yY3}
tD1
DumpParams
nP2
paramlist,x83
count,std::ostream&o){for(eY1=0;a<count;++a){if(a>0)o<<' ';const
e52&param=xB1
paramlist,a);DumpParam
xN(param,o);x83
depcode=ParamSpec_GetDepCode(param);if(depcode!=0)o<<"@D"
<<depcode;}
}
}
#include <algorithm>
using
lA3
l21;using
lA3
FUNCTIONPARSERTYPES;lA3{t83
plist_p[37]={{2,0,0x0}
tD
0,0x4}
tD
nN1,0x0}
tD
eU1|Constness_NotConst,0x0}
tD
Sign_NoIdea,0x0}
tD
eT1,0x0}
,{3,Sign_NoIdea,0x0}
,{3,0,0x0}
,{3,eT1,0x0}
,{3,0,0x8}
,{3,Value_OddInt,0x0}
,{3,Value_NonInteger,0x0}
,{3,x32,0x0}
,{3,nN1,0x0}
,{0,eU1|lW{0,lW{0,nN1|lW{0,x32|lW{0,tY1,0x1}
,{0,tZ1|nN1|lW{0,i01
tY1,0x1}
,{0,i01
lW{0,Oneness_One|lW{0,eT1|lW{1,lW{1,x32|lW{1,i01
lW{1,tZ1|lW{1,nN1|lW{1,eU1|lW{6,0,0x0}
,{4,0,0x0}
,{4,tZ1,0x0}
,{4,lW{4,0,0x16}
,{5,0,0x0}
,{5,lW}
n11
e22
plist_n_container{static
const
tB
plist_n[20];}
n11
const
tB
plist_n_container
xN::plist_n[20]={{x73(-2
i8-1
i8-0.5
i8-0.25
i8
0
tE2
fp_const_deg_to_rad
xN(tE2
fp_const_einv
xN(tE2
fp_const_log10inv
xN(i8
0.5
tE2
fp_const_log2
xN(i8
1
tE2
fp_const_log2inv
xN(i8
2
tE2
fp_const_log10
xN(tE2
fp_const_e
xN(tE2
fp_const_rad_to_deg
xN(tE2-fp_const_pihalf
xN(),y11{yD1,y11{fp_const_pihalf
xN(),y11{fp_const_pi
xN(),y11}
;const
xV
plist_s[511]={{{1,15,lQ3,397,lQ3,471,lQ3,15,cNeg,GroupFunction,0}
,tY1,0x1}
,{{1,15,y92
24,y92
459,y92
460,y92
492,cInv,lU
2,333122
x3
l0
2,48276
x3
l6
260151
x3
l6
464028
x3
l6
172201
x3
l6
48418
x3
l6
1331
x3
l6
172354
x3
l6
39202
x3
l6
312610
x3
l6
470469
x3
l6
296998
x3
l6
47
x3
SelectedParams,0}
,0,0x4
nJ
162863
x3
l6
25030
x3
l6
7168
x3
l6
199680
x3
l6
35847
x3
l6
60440
x3
l6
30751
x3
l6
186549
x3
l6
270599
x3
l6
60431
x3
l6
259119
x3
l6
291840
x3
l6
283648
x3
l6
220360
x3
l6
220403
x3
l6
239890
x3
l6
240926
x3
l6
31751
x3
l6
382998
x3
SelectedParams,0}
,0,0x4
nJ
384239
x3
l6
385071
x3
l6
386289
x3
l6
18674
x3
l6
61682
x3
l6
283969
x3
l6
283971
x3
l6
283976
x3
l6
386376
x3
l6
245767
x3
l6
385062
x3
l6
210991
x3
SelectedParams,0}
,0,0x1
nJ
15814
x3
l6
393254
x3
SelectedParams,0}
,0,0x5
nJ
393254
x3
l6
321583
x3
l6
297007
x3
l6
393263
x3
l6
393263
x3
SelectedParams,0}
,0,0x5
nJ
162871
x3
l6
258103
x3
iJ
0,0
x3
nH
0,0
i11
1,45
x3
nH
1,53
x3
nH
1,54
x3
nH
1,55
x3
nH
1,56
x3
nH
1,26
x3
nH
1,259
eF2
0x16}
,{{1,253
x3
nH
1,272
i11
1,327
eF2
0x16}
,{{1,0
x3
nH
1,21
x3
nH
1,441
eF2
cR1
443
eF2
cR1
0
eF2
cR1
0
tE
2}
,0,cR1
15
x3
nH
1,24
tE
2}
,0,0x0
nJ
58392
i11
0,0
tE
1}
,nN1,0x0
nJ
24591
eH2
33807
eH2
48143
eH2
289816
eH2
295960
eH2
307200,lA
315470,lA
39202,lA
121894,lA
421926,lA
429094,lA
437286,lA
289073,lA
325002,lA
334218,lA
7627,lA
7700,lA
7724,lA
38,lA
50587,lA
405504,lA
31751,lA
404487,lA
76816,l1
x52
318479,lA
319503
nP1
7340060,l1
x52
332833,lA
329764
nP1
7340176,lA
89387,lA
339273,lA
332170,lA
487462,lA
490534,lA
497702,lA
509990,lA
336947,lA
342016,lA
518182,lA
503808,lA
286727,lA
91151,lA
131087,lA
328719,l1
x52
7217,lA
304143,l1
0x14
nJ
296976,l1
lR3
7340056,l1
lR3
7340061,l1
x52
7192,lA
7447,lA
24583,lA
335887,l1
0x10
nJ
7206,lA
x63
lA
343078,l1
0x6
nJ
354342,lA
365606,lA
58768,lA
426398,l1
0x12}
,{{3,40272286,l1
x52
484766,l1
0x12}
,{{3,40330654,l1
x52
50600,lA
62918,lA
426456,l1
x52
484824,l1
x52
296998,l1
0x4
nJ
428070
nP1
40330712,l1
x52
50665,lA
367654,l1
0x6
nJ
38372,lA
121856
nP1
47655936,lA
471040,lA
367631,l1
0x7
nJ
343097,l1
0x7
nJ
39275,lA
39278,l1
0x4
nJ
39338,l1
lR3
15779300,lA
343055,l1
0x7}
,{{3,15779238,lA
436262,lA
502822,lA
39275,l1
0x4
nJ
39403,l1
0x4
nJ
35847,lA
15,l1
0x4
nJ
376870,lA
15,lA
122904,lA
121880,lA
30751,lA
57
nP1
7340177,lA
15681
nP1
67573791,lA
39240,lA
385039,l1
0x1
nJ
15760
nP1
64009216,lA
562176,l1
0x0}
,{{0,0,xW
0,0,iS
2,eZ3
2,t03
3,eZ3
3,t03
38,xW
1,38,iS
14,xW
1,57,xW
1,16,iK1
0x0
nJ
464959,iK1
0x1}
,{{1,306,xW
1,327,yA2
0x0
nJ
465223,iK1
0x16}
,{{1,292,eZ3
293,t03
294,xW
1,295,iS
400,xW
1,0,xW
1,454,xW
1,459,xW
1,16,iK1
0x1}
,{{1,57,yA2
0x1}
,{{1,0,iS
21,xW
1,15,iK1
0x0
nJ
24591,xW
1,24,iS
511,yA2
0x0
nJ
46095,lK
46104,lK
50200,lK
287789,lK
66584,lK
407836,lK
15397,lK
62504,lK
39951,lK
24591,lK
33807,lK
62509,lK
15409,lK
50176,lG,283648,lG,19456,lG,27648,lG,90112,lG,86016,lG,190464,eV1
x52
195584,lG,196608,eV1
0x14
nJ
482304,lG,14342,lG,58375,lG,46147
xR
46151,lG,284679,lG,50183,lG,7183,lG,46157
xR
38991
xR
50279,lG,50280,lG,50257,lG,50258,lG,46193,lG,50295,lG,283809,lG,284835,lG,24822,lG,10240,lG,11264,lG,7170,lG,x63
lG,17408,lG,197632,lG,470016,lG,25607,lG,121871,lG,39368,lG,7192,lG,121887,lG,416811,lG,252979,lG,50262,lG,46154,lG,38919,lG,62,lG,50281,lG,40050
xR
7566,eV1
0x10
nJ
415787,lG,416819,lG,39326
xR
39326,lG,39332,eV1
0x5
nJ
39333,eV1
0x1
nJ
50590
xR
50590,lG,39338
xR
39338,lG,39335,eV1
0x5
nJ
15786
xR
146858,lG,39366,lG,39373,lG,39374,lG,39384
xR
50648
xR
50648,lG,24,eV1
0x6
nJ
24,lG,62,eV1
0x6
nJ
40049,lG,46194
xR
43,lG,43
xR
415795,lG,51,lG,51
xR
50266,lG,50176
xR
50267,lG,39159,lG,39183
xR
7168
xR
31744,lG,98304,lG,31746,lG,109592,lG,39403
xR
39405
xR
39405,lG,39414,lG,39414
xR
16384,lG,15,lG,39024,eV1
0x5
nJ
39027,eV1
0x5
nJ
62853,lG,39416,lG,15360,lG,15,eV1
0x1
nJ
16,lG,7183,eV1
0x1
nJ
7172,cPow,yN1,nN1,0x0
nJ
24591,cPow,lU
2,63521,cPow,lU
2,62500,cPow,lU
2,50453,cPow,lU
2,50200,cPow,lU
2,62488,cPow,lU
1,0,t13
7,t13
196,t13
0,cAcos
t23
cAcosh
t23
cAsin
t23
cAsinh
nW
119,cAsinh
t23
cAtan
eW1
308224,cAtan2
eW1
x63
cAtan2
t23
cAtanh
nW
246,cCeil
t23
cCeil,iU2
0,cK2
0,cCos,iU2
7,cK2
81,cK2
82,cK2
119,cK2
237,cK2
255,cK2
217,iV2
237,iV2
458,iV2
0,cCosh,iU2
0,iV2
246,cFloor
t23
cFloor,lD
0x4
nJ
311587,cHypot
eW1
323875,cHypot
eW1
323899,cHypot,l0
3,32513024,y21
34627584
i9
31493120,y21
89213952
i9
149042176
i9
247699456
i9
301234176
i9
488062976
i9
492261376
i9
62933514
i9
62933514,y21
62933520
i9
62933520,y21
24664064
i9
573080576
i9
565189632
i9
32513024
i9
559963136
i9
7891968
i9
582524928,cIf
nW
119,cInt
nW
246,tF2
0,tF2
7,tF2
31,tF2
196,tF2
359,tF2
15,cLog,lU
1,24,cLog,lU
1,0,cLog10
t23
cLog2
eW1
x63
cMax
eW1
35847,cMax
eW1
30751,cMax
t23
cMax,AnyParams,1}
,0,0x4
nJ
x63
cMin
eW1
35847,cMin
eW1
30751,cMin
t23
cMin,AnyParams,1}
,0,0x4
nJ
24591,cMin,lU
1,0,x72
7,x72
81,x72
82,x72
119,x72
149,x72
233,cSin,lD
0x5}
,{{1,246,x72
255,x72
254,x72
0,cSin,iU2
273,cSin,lD
0x1}
,{{1,217,yB2
233,cSinh,lD
0x5}
,{{1,246,yB2
254,yB2
255,yB2
458,yB2
0,cSinh,iU2
0,yB2
15,cSqrt,lU
1,0
x12
0,cTan,iU2
117,cTan,iU2
118
x12
233
x12
246
x12
273
x12
254
x12
255
x12
0,yC2
0,cTanh,iU2
216,yC2
233,yC2
246,yC2
254,yC2
255,yC2
0,cTrunc
eW1
15384,cSub,lU
2,15384,cDiv,lU
2,470476,cDiv,lU
2,121913,tG2
x63
lS3
x52
x63
tG2
31744,lS3
0x20
nJ
31751,lS3
0x24
nJ
31751,tG2
121913,i21
eW1
x63
cLess,lD
x52
41984,cLess,lD
0x4
nJ
41984,cLess
eW1
7,cLess
eW1
x63
cLessOrEq
eW1
295158,cLessOrEq
eW1
x63
cGreater,lD
x52
41984,cGreater,lD
0x4
nJ
41984,cGreater
eW1
7,cGreater
eW1
x63
yR1
l0
2,295158
lZ3
t23
lV2
244,lV2
7,lV2
544,lV2
547,lV2
548,lV2
550,lV2
15,lV2
31,lV2
553,lV2
554,cNot
eW1
7700
l23
7168
l23
35847
l23
30751
l23
457759
l23
460831,cAnd,iJ
0,0,cAnd,nH
2,x63
e43
7700,e43
35847,e43
457759,e43
460831,e43
30751,cOr,iJ
1,0,lW2
81,lW2
131,lW2
244,lW2
245,lW2
246,cDeg
nW
246,cRad
eW1
x63
cAbsAnd,l6
x63
cAbsOr,iJ
1,0,cK3
t23
cAbsNotNot,l0
3,32513024,c93
lD
0x0}
,}
;}
lA3
l21{const
Rule
grammar_rules[260]={{ProduceNewTree,17,1,0,{1,0,cAbs,eI2
409,{1,146,cAtan,eI2
403
tD
1326,cAtan2,eI2
405
tD
309249,cAtan2
xJ
253174
tD
255224,cAtan2
xJ
259324
tD
257274,cAtan2,eI2
152,{1,252,cCeil,l3
2,1,480,{1,68,y31
476,{1,122,y31
477,{1,124,y31
151,{1,125,y31
419,{1,123,y31
0,{1,403,cCos,l2
2,1,246,{1,252,cCos,l2
18,1,0,{1,400,y31
303,{1,406,cCosh,l2
2,1,246,{1,252,cCosh,l2
18,1,0,{1,400,cCosh,l3
2,1,452,{1,121,cFloor,eI2
150,{1,252,cFloor,l3
0,1,157,{3,7382016,eF
543,{3,8430592,eF
550,{3,8436736,eF
158,{3,42998784,eF
544,{3,42999808,eF
556,{3,43039744,eF
551,{3,49291264,eF
532,{3,49325056,eF
463,{3,1058312,eF
467,{3,1058318,eF
467,{3,9438728,eF
463,{3,9438734,cIf,l2
0,3,32542219,{3,36732428,cIf,l2
0,3,32542225,{3,36732434,cIf,l3
16,1,567,{3,32513026,cIf,l3
16,1,509,{3,449213961,cIf,l3
16,1,509,{3,433500687,cIf,l3
2,1,76,{1,256,yD2
69,{1,258,yD2
404,{1,72,yD2
160,{1,147,cLog,l2
0,1,0
tD
481281,cMax,i0
16,1,439
tD
c43
cMax,i0
0,1,0
tD
477185,cMin,i0
16,1,440
tD
c43
cMin,cA
0,1,154
tD
24832,cPow,l3
0,1,154
tD
25854,cPow,l3
0,1,155
tD
129039
eL3
32062
eL3
32063
eL3
32064
c53
166288
tD
32137
eL3
33089
c53
7168
tD
12688
c53
7434
tD
12553
iT
429
tD
46146
iT
430
tD
46153
iT
431
tD
46150
iT
173
tD
81935
iT
172
tD
130082
iT
178
tD
133154
c63
470016
tD
464911
c63
274432
tD
273423
c63
251904
tD
266274
c63
251904
tD
263186
iT
175,{1,252,lR1
421,{1,68,lR1
151,{1,122,lR1
419,{1,124,lR1
174,{1,125,lR1
476,{1,123,lR1
0,{1,405,lR1
176,{1,252,cSinh,l3
2,1,333,{1,404,cSinh,l3
2,1,177,{1,252,yE2
0,{1,408,yE2
179,{1,410,yE2
180,{1,252,cTanh,l2
0,1,436
tD
443407,lU3
435
tD
c43
lU3
171
tD
268549,lU3
184
tD
276749,lU3
183
tD
276500,lV3
59701
tD
303289,lV3
59702
tD
305339,lV3
59728
tD
346306,lV3
157004
tD
193724,lV3
174245
tD
171172,lV3
243878
tD
194724
x3
cA
2,1,340,{1,313
tE
1
lH1
330,{1,361
tE
1}
}
,{ReplaceParams,2,1,365
tD
1386
n31
339
tD
1361
n31
457
tD
466374
n31
47
tD
353636
n31
346
tD
203823
n31
357
tD
202799
n31
474
tD
208079
n31
475
tD
209103
n31
417
tD
213193
n31
210
tD
213194
n31
418
tD
216265
n31
212
tD
202086
n31
205
tD
368998
n31
214
tD
368853
n31
223
tD
224474
n31
225
tD
229594
n31
366
tD
502185
n31
220
tD
501982
n31
227
tD
226729
n31
226
tD
226794
n31
363
tD
234921
n31
426
tD
376037
n31
491
tD
376030
n31
491
tD
233897
n31
426
tD
373226
n31
227
tD
372958,l7
2,2,406760
tD
236753,l7
2,2,59762
tD
236913,lU3
371
tD
1396,cL2
94
tD
24705,cL2
95
tD
24708,cL2
438
tD
443407,cL2
437
tD
c43
cL2
98
tD
99701,cL2
106
tD
101704,cL2
100
tD
110920,l5
0,2,108559
tD
103752,l5
0,2,102415
tD
104776,lJ
0
lH1
110
tD
111634,cMul,SelectedParams,0
lH1
561,{1,52,lJ
1
lH1
562,{1,42,lJ
1}
}
,{ReplaceParams,2,1,480
tD
492582
iL1
488
tD
498726
iL1
381
tD
435578
iL1
491
tD
435703
iL1
426
tD
502142
iL1
414
tD
493947
iL1
493
tD
349666
iL1
342
tD
364014
iL1
380
tD
425315
iL1
472
tD
425454
iL1
47
tD
506351
iL1
499
tD
352739
iL1
47
tD
510448
iL1
501
tD
512038
iL1
502
tD
355818
iL1
348
tD
387575
iL1
505
tD
357861
iL1
497
tD
351710
iL1
508
tD
519206
iL1
504
tD
395375
iL1
388
tD
394356
iL1
461
tD
45510
iL1
353
tD
51552
iL1
462
tD
49606
iL1
354
tD
47456,l5
2,2,359926
tD
358890,l5
16,1,92
tD
1157,l5
16,1,93
tD
1158,l5
16,1,402
tD
411024,l5
16,2,58768
tD
1466,l5
16,2,15760
tD
1468,l5
17,1,0,{1,400,l5
17,1,57,{1,14,lJ
0}
}
,{ProduceNewTree,4,1,532
tD
41,lT3
l3
4,1,0
tD
5167,lT3
cR
41984
tD
409641,lT3
cR
i1
lT3
cR
tC
lT3
cR
tF
cEqual
y51
24849,cEqual
xJ
i2
cEqual
xJ
nZ2
281873,cEqual
xJ
iL
cEqual
xJ
l71
lT3
l3
4,1,556
tD
41,i21,l3
4,1,532
tD
5167,i21,cR
41984
tD
409641,i21,cR
i1
i21,cR
tC
i21,cR
tF
i21
y51
24849,i21
xJ
i2
i21
xJ
nZ2
281873,i21
xJ
iL
i21
xJ
l71
i21,cR
i1
c73
tC
c73
tF
cLess,eI2
565
tD
46080,cLess
y51
24832,cLess
xJ
y61
cLess
xJ
i2
cLess
xJ
nZ2
t33
cLess
xJ
x21
cLess
xJ
iL
cLess
xJ
l71
cLess,l3
20,1,556
tD
409641,c73
i1
lX2
cR
tC
lX2
cR
tF
lX2
eI2
559
tD
409615,cLessOrEq
y51
24832,cLessOrEq
xJ
y61
cLessOrEq
xJ
i2
cLessOrEq
xJ
nZ2
t33
cLessOrEq
xJ
x21
cLessOrEq
xJ
iL
cLessOrEq
xJ
l71
lX2
l3
20,1,556
tD
409647,lX2
cR
i1
cM2
tC
cM2
tF
cGreater,eI2
533
tD
409615,cGreater
y51
24832,cGreater
xJ
y61
cGreater
xJ
i2
cGreater
xJ
nZ2
t33
cGreater
xJ
x21
cGreater
xJ
iL
cGreater
xJ
l71
cGreater,l3
20,1,532
tD
409647,cM2
i1
yR1
cR
tC
yR1
cR
tF
yR1
eI2
566
tD
46080
lZ3
y51
24832
lZ3
xJ
y61
n03
i2
n03
nZ2
281856
lZ3
xJ
x21
n03
iL
n03
l71
yR1
l3
20,1,532
tD
409641,yR1
l3
4,1,513,{1,137,cNot,l3
16,1,565,{1,2,cNot,l2
0,1,446
tD
c43
xI
0,2,530947,{3,541595138,cAnd,cA
16,1,560,{1,5,cAnd,AnyParams,1}
}
,{ReplaceParams,16,1,563
tD
13314,xI
16,1,538
tD
547348,xI
16,1,541
tD
456220,xI
16,1,542
tD
460316,xI
0,1,451
tD
c43
nG
564
tD
13314,nG
557
tD
8197,nG
535
tD
547348,nG
536
tD
456220,nG
537
tD
460316,nG
558
tD
143365,cOr,cA
4,1,519,{1,137,c83
l3
16,1,566,{1,2,c83
l3
17,1,0,{1,0,c83
eI2
531,{1,256,cAbsNotNot,cA
18,1,525,{1,254,cAbsNotNot,cA
0,1,566,{3,43039744,c93
l3
0,1,565,{3,49325056,c93
l3
16,1,448,{3,32513580,c93
l2
16,3,32542219,{3,36732428,c93
yN1}
,}
;e22
grammar_optimize_abslogical_type{yV
9
eG
grammar_optimize_abslogical_type
grammar_optimize_abslogical={9,{34,190,226,236,240,245,252,258,259}
}
;}
e22
grammar_optimize_ignore_if_sideeffects_type{yV
59
eG
grammar_optimize_ignore_if_sideeffects_type
grammar_optimize_ignore_if_sideeffects={59,{0,20,21,22,23,24,25,26,cU
iP1
78,cI1
yU
grammar_optimize_nonshortcut_logical_evaluation_type{yV
56
eG
grammar_optimize_nonshortcut_logical_evaluation_type
grammar_optimize_nonshortcut_logical_evaluation={56,{0,25,cU
iP1
78,cI1,161,162,163,164,165,166,167,176,177,178,198,202,210,214,222,234,235,237,238,239,241,242,243,244,246,247,248,249,250,251,253,254,255,256,257}
}
;}
e22
grammar_optimize_round1_type{yV
125
eG
grammar_optimize_round1_type
grammar_optimize_round1={125,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,19,25,cU
37,38,iP1
45,46,47,48,49,50,51,52,53,54,58,59,60,61,62,63,64,65,66,67,68,69,70,71,78,79,80,81,82,83,84,85,86,87,88,93,94,95,96,97,98,99,100,101,117,118,119,120,121,122,123,124,125,126,127,128,129,132,158,159,160
yU
grammar_optimize_round2_type{yV
103
eG
grammar_optimize_round2_type
grammar_optimize_round2={103,{0,15,16,17,25,cU
39,40,iP1
45,46,47,48,49,50,51,52,53,54,59,60,72,73,78,79,83,84,85,89,90,91,92,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,119,122,123,124,125,126,127,128,133,157,158,159,160
yU
grammar_optimize_round3_type{yV
79
eG
grammar_optimize_round3_type
grammar_optimize_round3={79,{74,75,76,77,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,168,169,170,171,172,173,174,175,179,180,181,182,183,184,185,186,187,188,189,191,192,193,194,195,196,197,199,200,201,203,204,205,206,207,208,209,211,212,213,215,216,217,218,219,220,221,223,224,225,227,228,229,230,231,232,233}
}
;}
e22
grammar_optimize_round4_type{yV
10
eG
grammar_optimize_round4_type
grammar_optimize_round4={10,{18,55,56,57,130,131,153,154,155,156}
}
;}
e22
grammar_optimize_shortcut_logical_evaluation_type{yV
53
eG
grammar_optimize_shortcut_logical_evaluation_type
grammar_optimize_shortcut_logical_evaluation={53,{0,25,cU
iP1
78,cI1,161,162,163,164,165,166,167,176,177,178,198,202,210,214,222,234,235,237,238,241,242,243,244,247,248,249,251,253,254,255,256,257}
}
;}
}
lA3
l21{x33
e52
ParamSpec_Extract
nP2
paramlist,l91){index=(paramlist>>(index*10))&1023;if(index>=57)return
e52(SubFunction,cN2
plist_s[index-57]);if(index>=37)return
e52(NumConstant,cN2
plist_n_container
xN::plist_n[index-37])eL
e52(ParamHolder,cN2
plist_p[index]);}
}
#ifdef FP_SUPPORT_OPTIMIZER
#include <stdio.h>
#include <algorithm>
#include <map>
#include <sstream>
using
lA3
FUNCTIONPARSERTYPES;using
lA3
l21;using
tN;using
cH1;lA3{nX1
It,typename
T,typename
Comp>eX1
MyEqualRange(It
first,It
last,const
T&val,Comp
comp){size_t
len=last-first;while(len>0){size_t
yD3
len/2;It
nZ3(first);nZ3+=half;if(comp(*nZ3,val)){first=nZ3;++first;len=len-half-1;}
iZ1
comp(val,*nZ3)){len=half;}
else{It
left(first);{It&eJ2=left;It
last2(nZ3);size_t
len2=last2-eJ2;while(len2>0){size_t
half2=len2/2;It
middle2(eJ2);middle2+=half2;if(comp(*middle2,val)){eJ2=middle2;++eJ2;len2=len2-half2-1;}
else
len2=half2;}
}
first+=len;It
right(++nZ3);{It&eJ2=right;It&last2=first;size_t
len2=last2-eJ2;while(len2>0){size_t
half2=len2/2;It
middle2(eJ2);middle2+=half2;if(comp(val,*middle2))len2=half2;else{eJ2=middle2;++eJ2;len2=len2-half2-1;}
}
}
return
eX1(left,right);}
}
return
eX1(first,first);}
x33
e22
OpcodeRuleCompare{iH2()eG3
tree,x83
yF2)const{const
Rule&rule=grammar_rules[yF2]eL
tC2<rule
cO2.subfunc_opcode;}
iH2()nP2
yF2,const
t2
const{const
Rule&rule=grammar_rules[yF2]eL
rule
cO2.subfunc_opcode<tC2;}
}
n11
bool
TestRuleAndApplyIfMatch(tE1
n91
tree,bool
cB{MatchInfo
xN
info;nC1
found(false,cZ());if((rule.l81
LogicalContextOnly)&&!cB{tK1
if(nB
IsIntType
xN::yA3)tX2
NotForIntegers)tK1
else
tX2
OnlyForIntegers)tK1
if(nB
IsComplexType
xN::yA3)tX2
NotForComplex)tK1
else
tX2
OnlyForComplex)tK1
for(;;){
#ifdef DEBUG_SUBSTITUTIONS
#endif
found=TestParams(rule
cO2
e53,found.specs,info,true);if(found.found)break;if(!&*found.specs){fail:;
#ifdef DEBUG_SUBSTITUTIONS
DumpMatch(rule
lY3,false);
#endif
return
eO3}
#ifdef DEBUG_SUBSTITUTIONS
DumpMatch(rule
lY3,true);
#endif
SynthesizeRule(rule
lY3
iX1}
cH1{xV1
yS1
const
Grammar&x82,n91
tree,bool
cB{if(tree.GetOptimizedUsing()==&x82){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Already optimized:  "
i63
tree)xI1"\n"
<<std::flush;
#endif
return
eO3
if(true){bool
changed=false;switch(tC2
eJ3
cNot:case
cNotNot:case
cAnd:case
cOr:e23
if(yS1
x82
tA3
true))xD2
lC
cIf:case
iF3:if(yS1
x82,t5
0),tC2==cIf))xD2
for
c12
1;a<t6;++a)if(yS1
x82
tA3
cB)xD2
break;default:e23
if(yS1
x82
tA3
false))xD2}
if(changed){tree.Mark_Incompletely_Hashed(iX1}
typedef
const
x83
short*n23;std::pair<n23,n23>range=MyEqualRange(x82.rule_list,x82.rule_list+x82.rule_count
e53,OpcodeRuleCompare
xN());std
xM3<x83
short>rules;rules.xJ3
range
xU3-range.first);for
y5
if(IsLogisticallyPlausibleParamsMatch(e81
cO2
e53))rules.push_back(*r);}
range.first=!rules
tK3?&rules[0]:0;range
xU3=!rules
tK3?&rules[rules.size()-1]+1:0;if(range.first!=range
xU3){
#ifdef DEBUG_SUBSTITUTIONS
if(range.first!=range
xU3){std::cout<<"Input ("
<<FP_GetOpcodeName(tC2)<<")["
<<t6<<"]"
;if(cB
std::cout<<"(Logical)"
;x83
first=iQ1,prev=iQ1;l03
sep=", rules "
;for
y5
if(first==iQ1)first=prev=*r;iZ1*r==prev+1)prev=*r;else{std::cout<<sep<<first;sep=","
;if(prev!=first)std::cout<<'-'<<prev;first=prev=*r;}
}
if(first!=iQ1){std::cout<<sep<<first;if(prev!=first)std::cout<<'-'<<prev;}
std::cout<<": "
i63
tree)xI1"\n"
<<std::flush;}
#endif
bool
changed=false;for
y5
#ifndef DEBUG_SUBSTITUTIONS
if(!IsLogisticallyPlausibleParamsMatch(e81
cO2
e53))continue;
#endif
if(TestRuleAndApplyIfMatch(e81
e53,cB){xD2
yY3}
if(changed){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Changed."
<<std::endl
xI1"Output: "
i63
tree)xI1"\n"
<<std::flush;
#endif
tree.Mark_Incompletely_Hashed(iX1}
tree.SetOptimizedUsing(&x82)eL
eO3
tD1
ApplyGrammars(FPoptimizer_CodeTree::t2{
#ifdef DEBUG_SUBSTITUTIONS
std
tY3"grammar_optimize_round1\n"
;
#endif
n4
grammar_optimize_round1
e53
x8
#ifdef DEBUG_SUBSTITUTIONS
std
tY3"grammar_optimize_round2\n"
;
#endif
n4
grammar_optimize_round2
e53
x8
#ifdef DEBUG_SUBSTITUTIONS
std
tY3"grammar_optimize_round3\n"
;
#endif
n4
grammar_optimize_round3
e53
x8
#ifndef FP_ENABLE_SHORTCUT_LOGICAL_EVALUATION
#ifdef DEBUG_SUBSTITUTIONS
std
tY3"grammar_optimize_nonshortcut_logical_evaluation\n"
;
#endif
n4
grammar_optimize_nonshortcut_logical_evaluation
e53
x8
#endif
#ifdef DEBUG_SUBSTITUTIONS
std
tY3"grammar_optimize_round4\n"
;
#endif
n4
grammar_optimize_round4
e53
x8
#ifdef FP_ENABLE_SHORTCUT_LOGICAL_EVALUATION
#ifdef DEBUG_SUBSTITUTIONS
std
tY3"grammar_optimize_shortcut_logical_evaluation\n"
;
#endif
n4
grammar_optimize_shortcut_logical_evaluation
e53
x8
#endif
#ifdef FP_ENABLE_IGNORE_IF_SIDEEFFECTS
#ifdef DEBUG_SUBSTITUTIONS
std
tY3"grammar_optimize_ignore_if_sideeffects\n"
;
#endif
n4
grammar_optimize_ignore_if_sideeffects
e53
x8
#endif
#ifdef DEBUG_SUBSTITUTIONS
std
tY3"grammar_optimize_abslogical\n"
;
#endif
n4
grammar_optimize_abslogical
e53
x8
#undef C
}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
#include <algorithm>
#include <assert.h>
#include <cstring>
#include <cmath>
#include <memory> /* for auto_ptr */
using
lA3
FUNCTIONPARSERTYPES;using
lA3
l21;using
tN;using
cH1;lA3{xV1
TestImmedConstraints
nP2
bitmask,const
t2{switch(bitmask&ValueMask
eJ3
Value_AnyNum:case
ValueMask:lC
x32:if(GetEvennessInfo(tree)!=lY2
Value_OddInt:if(GetEvennessInfo(tree)!=x92
tZ1:if(GetIntegerInfo(tree)!=lY2
Value_NonInteger:if(GetIntegerInfo(tree)!=x92
eT1:if(!IsLogicalValue(tree)eM
nO1
SignMask
eJ3
Sign_AnySign:lC
nN1:if(l01
lY2
eU1:if(l01
x92
Sign_NoIdea:if(l01
Unknown
eM
nO1
OnenessMask
eJ3
Oneness_Any:case
OnenessMask:lC
Oneness_One:if(!nI2
if(!lN3
fp_abs(cS3),x73(1))eM
lC
Oneness_NotOne:if(!nI2
iZ2
fp_abs(cS3),x73(1))eM
nO1
ConstnessMask
eJ3
Constness_Any:lC
tY1:if(!nI2
lC
Constness_NotConst:if(nI2
yY3
l62}
tJ1
x83
extent,x83
nbits,typename
eK2=x83
int>e22
nbitmap{private:static
const
x83
bits_in_char=8;static
const
x83
eL2=(cA3
eK2)*bits_in_char)/nbits;eK2
data[(extent+eL2-1)/eL2];e83
void
inc(l91,int
by=1){data[pos(index)]+=by*eK2(1<<yG2);tL1
void
dec(l91){inc(index,-1);}
int
get(l91
c81(data[pos(index)]>>yG2)&mask()xY3
pos(l91)yY
index/eL2
xY3
shift(l91)yY
nbits*(index%eL2)xY3
mask()yY(1<<nbits)-1
xY3
mask(l91)yY
mask()<<yG2;}
}
;e22
Needs{int
SubTrees:8;int
Others:8;int
nY2:8;int
Immeds:8;nbitmap<iT2,2>SubTreesDetail;Needs(){std::memset(this,0,cA3*this));}
Needs
nV2
Needs&b){std::memcpy(this,&b,cA3
b));}
Needs&eJ1=nV2
Needs&b){std::memcpy(this,&b,cA3
b))eL*this;}
}
n11
Needs
CreateNeedList_uncached(tT&cD2){Needs
e31;for(eY1=0;a<cD2
yH2;++a){const
e52&parampair=xB1
cD2.param_list,a);switch(tF3){cG1
const
xV&nU2
xV
nW2
yM
GroupFunction)++e31
tI3;else{++tQ2;assert(i73.subfunc_opcode<VarBegin);e31.SubTreesDetail.inc(param
c51);}
++e31.nY2
tM2
eP3
NumConstant:case
ParamHolder:++tP2;++e31.nY2
tM2}
}
return
e31;}
x33
Needs&CreateNeedList(tT&cD2){typedef
std::map<tT*,Needs>e91;static
e91
c31;e91::yN3
i=c31.xY2&cD2);if(i!=c31.e21&cD2)return
i
e82
eL
c31.yI3,std::make_pair(&cD2,CreateNeedList_uncached
xN(cD2)))e82;}
x33
CodeTree
xN
CalculateGroupFunction
nV2
e52&yI2
const
tS
info
t63
tF3
eJ3
NumConstant:{const
tB&nU2
tB*)tO2
second
eL
CodeTreeImmed(param
cJ2);eP3
nT2
t83&nU2
xU*)tO2
second
eL
info.GetParamHolderValueIfFound(param.index);}
cG1
const
xV&nU2
xV
nW2
CodeTree
y43;yA3
xC
param
c51);yA3.lB2).xJ3
i73
yH2);for(eY1=0;a<i73
yH2;++a
t71
tmp(CalculateGroupFunction(xB1
param
tE3
param_list,a),info));yA3
lP1
tmp);}
yA3
x22)eL
yA3;}
}
return
CodeTree
xN();}
}
cH1{xV1
IsLogisticallyPlausibleParamsMatch(tT&cD2,const
t2{Needs
e31(CreateNeedList
xN(cD2));size_t
t43=t6;if(t43<size_t(e31.nY2))tH2
for
c12
0;a<t43;++a){x83
opcode=t5
a)nC;switch(opcode
yS3
if(e31
tI3>0)--e31
tI3;else--tP2;lC
iT2:case
cFCall:case
cPCall:--tP2
tM2
default:assert(opcode<VarBegin);if(tQ2>0&&e31.SubTreesDetail.get(opcode)>0){--tQ2;e31.SubTreesDetail.dec(opcode);}
else--tP2;}
}
if(e31
tI3>0||tQ2>0||tP2>0)tH2
if(cD2.match_type!=AnyParams){if(0||tQ2<0||tP2<0)tH2}
l62}
x33
nC1
TestParam
nV2
e52&yI2
xG2
tree
eC3
start_at,tS
info
t63
tF3
eJ3
NumConstant:{const
tB&nU2
tB
nW2
if(!nI2
x73
imm=cS3;switch(param.modulo
eJ3
Modulo_None:lC
Modulo_Radians:imm=fp_mod(imm,yA
imm<xH1
imm
yW
if(imm>fp_const_pi
xN())imm-=fp_const_twopi
xN(tK2}
return
lN3
imm,param
cJ2);eP3
nT2
t83&nU2
xU
nW2
if(!x9
return
info.SaveOrTestParamHolder(param.index
e53);}
cG1
const
xV&nU2
xV
nW2
yM
GroupFunction){if(!x9
CodeTree
xN
y71=CalculateGroupFunction(yI2
info);
#ifdef DEBUG_SUBSTITUTIONS
DumpHashes(y71)xI1*nV2
void**)&y71.xY1
xI1"\n"
xI1*nV2
void**)&cS3
xI1"\n"
;DumpHashes(tree)xI1"Comparing "
i63
y71)xI1" and "
i63
tree)xI1": "
xI1(y71
xD
tree)?"true"
:"false"
)xI1"\n"
;
#endif
return
y71
xD
tree);}
cP3!&*start_at){if(!x9
if(tC2!=param
c51
eM}
return
TestParams(i73
e53,start_at,info,false);}
}
}
return
eO3
x33
e22
iW
xA2
MatchInfo
xN
info;iW()yT3,info(){}
}
n11
class
MatchPositionSpec_PositionalParams:nA1
iW
xN>{e83
iW2
MatchPositionSpec_PositionalParams(cB3):eA1
iW
xN>(n){}
}
;e22
iR1
xA2
iR1()yT3{}
}
;class
yN:nA1
iR1>{e83
x83
trypos;iW2
yN(cB3):eA1
iR1>(n),trypos(0){}
}
n11
nC1
TestParam_AnyWhere
nV2
e52&yI2
xG2
tree
eC3
start_at,tS
info,xP3&used,bool
eM2{y0<yN>xF;xQ3
yN*eH3
a=xF->trypos;goto
retry_anywhere_2;}
xO3
yN(t6);a=0;}
t53
t6;++a){if(used[a])continue;retry_anywhere:{nC1
r=TestParam(yI2
t5
a)yZ3);c03
used[a]=true;if(eM2
iI3
a);xF->trypos=a
eL
nC1(true,&*xF);}
}
retry_anywhere_2:if(iG3
goto
retry_anywhere;}
}
return
eO3
x33
e22
yT1
xA2
MatchInfo
xN
info;xP3
used;iW2
yT1(size_t
t43)yT3,info(),used(t43){}
}
n11
class
MatchPositionSpec_AnyParams:nA1
yT1
xN>{e83
iW2
MatchPositionSpec_AnyParams(cB3,size_t
m):eA1
yT1
xN>(n,yT1
xN(m)){}
}
n11
nC1
TestParams(tT&nR,xG2
tree
eC3
start_at,tS
info,bool
eM2{if(nR.match_type!=AnyParams){if(y3!=t6
eM}
if(!IsLogisticallyPlausibleParamsMatch(nR
e53))tH2
switch(nR.match_type
eJ3
PositionalParams:{y0<cK>xF;xQ3
cK*eH3
a=y3-1;goto
lA1;}
xO3
cK(y3);a=0;}
t53
y3;++a){(*xR3
tI2
retry_positionalparams:{nC1
r=TestParam(cV
a),t5
a)yZ3);c03
iS2}
lA1:if(iG3
cC3
xR3
info;goto
retry_positionalparams;}
if(a>0){--a;goto
lA1;}
cC3
cD3
info
eL
eO3
if(eM2
t93
iI3
a)eL
nC1(true,&*xF);eP3
SelectedParams:case
AnyParams:{y0<tO>xF;xP3
used(t6);std
xM3<x83>iX2(y3);std
xM3<x83>yJ2(y3);t93{const
e52
parampair=cV
a);iX2[a]=ParamSpec_GetDepCode(parampair);}
{x83
b=0;t93
if(iX2[a]!=0)yJ2[b++]=a;t93
if(iX2[a]==0)yJ2[b++]=a;}
xQ3
tO*eH3
if(y3==0){a=0;goto
retry_anyparams_4;}
a=y3-1;goto
eB1;}
xO3
tO(y3,t6);a=0;if(y3!=0){(*cD3
tI2(*cD3
used=used;}
}
t53
y3;++a){if(a>0){(*xR3
tI2(*xR3
used=used;}
retry_anyparams:{nC1
r=TestParam_AnyWhere
xN(cV
yJ2[a])e53
yZ3,used,eM2;c03
iS2}
eB1:if(iG3
cC3
xR3
info;used=(*xR3
used;goto
retry_anyparams;}
eC1:if(a>0){--a;goto
eB1;}
cC3
cD3
info
eL
eO3
retry_anyparams_4:if(nR.n2!=0){if(!TopLevel||!info.HasRestHolder(nR.n2)){eZ
cP2;cP2.xJ3
t6);for
nP2
b=0;b<t6;++b){if(cE3)continue;cP2.push_back(t5
b));cE3=true;if(eM2
iI3
b);}
if(!info.SaveOrTestRestHolder(nR.n2,cP2)){goto
eC1;}
}
else{t73&cP2=info.GetRestHolderValues(nR.n2)c02
0;a<cP2.size();++a){bool
found=false;for
nP2
b=0;b<t6;++b){if(cE3
c42
cP2[a]xD
t5
b))){cE3=true;if(eM2
iI3
b);found=true
tM2}
}
if(!found){goto
eC1;}
}
}
}
return
nC1(true,y3?&*xF:0);eP3
GroupFunction:yY3
return
eO3}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
#include <algorithm>
#include <assert.h>
using
tN;using
cH1;lA3{x33
CodeTree
xN
y81
const
e52&yI2
tS
info,bool
inner=true
t63
tF3
eJ3
NumConstant:{const
tB&nU2
tB*)tO2
second
eL
CodeTreeImmed(param
cJ2);eP3
nT2
t83&nU2
xU*)tO2
second
eL
info.GetParamHolderValue(param.index);}
cG1
const
xV&nU2
xV
nW2
CodeTree
xN
tree;tW2
param
c51);for(eY1=0;a<i73
yH2;++a
t71
nparam=y81
xB1
param
tE3
param_list,a),info,true)l12
nparam);}
c33
tE3
n2!=0){eZ
trees(info.GetRestHolderValues(param
tE3
n2));tree.AddParamsMove(trees);if(t6==1){assert(tree.i33()==cAdd||tree.i33()==cMul||tree.i33()==cMin||tree.i33()==cMax||tree.i33()==cAnd||tree.i33()==cOr||tree.i33()==cAbsAnd||tree.i33()==cAbsOr);tree.eB2
0));}
iZ1
t6==0
t63
tC2
eJ3
cAdd:case
cOr:tree=nJ1
0));lC
cMul:case
cAnd:tree=nJ1
1));default:yY3}
}
if(inner)tree
x22)eL
tree;}
}
return
CodeTree
xN();}
}
cH1{tD1
SynthesizeRule(tE1
n91
tree,tS
info
t63
rule.ruletype
eJ3
ProduceNewTree:{tree.Become(y81
xB1
rule
eD1
0),info,false)tK2
eP3
ReplaceParams:default:{std
xM3<x83>list=info.GetMatchedParamIndexes();std::sort(list.iM2
list.end())c02
list.size();a-->0;)tree.l93
list[a]);for(eY1=0;a<rule.repl_param_count;++a
t71
nparam=y81
xB1
rule
eD1
a),info,true)l12
nparam);}
yY3}
}
}
#endif
#ifdef DEBUG_SUBSTITUTIONS
#include <sstream>
#include <cstring>
using
lA3
FUNCTIONPARSERTYPES;using
lA3
l21;using
tN;using
cH1;lA3
l21{tD1
DumpMatch(tE1
xG2
tree,const
tS
info,bool
DidMatch,std::ostream&o){DumpMatch(rule
lY3,DidMatch?iE3"match"
:iE3"mismatch"
,o);}
tD1
DumpMatch(tE1
xG2
tree,const
tS
info,l03
tB3,std::ostream&o){static
const
char
ParamHolderNames[][2]={"%"
,"&"
,"x"
,"y"
,"z"
,"a"
,"b"
,"c"
}
;o<<tB3<<" (rule "
<<(&rule-grammar_rules)<<")"
<<":\n  Pattern    : "
;{e52
tmp;tmp.first=SubFunction;xV
tmp2;tmp2.data=rule
cO2;tmp
xU3=cN2
tmp2;DumpParam
xN(tmp,o);}
o<<"\n  Replacement: "
;DumpParams
xN(rule
eD1
rule.repl_param_count,o);o<<"\n"
;o<<"  Tree       : "
i63
tree,o);o<<"\n"
;if(!std::strcmp(tB3,iE3"match"
))DumpHashes(tree,o)c02
0;a<info.cI
size();++a){if(!info.paramholder_matches[a]xN2))continue;o<<"           "
<<ParamHolderNames[a]<<" = "
i63
info.paramholder_matches[a],o);o<<"\n"
;}
eN3
info.lR.size();++b){if(!tJ2
first)continue
c02
0;a<tJ2
second.size();++a){o<<"         <"
<<b<<"> = "
i63
tJ2
second[a],o);o<<std::endl;}
}
o<<std::flush;}
}
#endif
#include <list>
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
using
lA3
FUNCTIONPARSERTYPES;lA3{xV1
MarkIncompletes(FPoptimizer_CodeTree::t2{if(tree.Is_Incompletely_Hashed())l62
bool
iS1=false;e23
iS1|=MarkIncompletes(t5
a));if(iS1)tree.Mark_Incompletely_Hashed()eL
iS1;}
tD1
FixIncompletes(FPoptimizer_CodeTree::t2{if(tree.Is_Incompletely_Hashed()){e23
FixIncompletes(t5
a));tree
x22);}
}
}
tN{lB
Sort(){data->Sort();}
lB
Rehash(bool
constantfolding){if(constantfolding)ConstantFolding(*this);else
Sort();data
xH
x33
e22
cG{yO
l92
cF3
yU1=0;
#if 0
long
double
value=Value;eA=crc32::calc(nV2
x83
char*)&value,cA3
value));key^=(key<<24);
#elif 0
union{e22{x83
char
filler1[16]iO2
v;x83
char
filler2[16];}
buf2;e22{x83
char
filler3[cA3
x73)+16-cA3
xE1)];eA;}
buf1;}
data;memset(&data,0,cA3
data));data.buf2.v=Value;eA=data.buf1.key;
#else
int
x93
iO2
nJ2=std::frexp(Value,&xK2;eA=nP2(x93+0x8000)&0xFFFF);if(nJ2<0){nJ2=-nJ2;key=key^0xFFFF;}
else
key+=0x10000;nJ2-=yM2;key<<=39;key|=nE1(nJ2+nJ2)*x73(1u<<31))<<8;
#endif
lQ
#ifdef FP_SUPPORT_COMPLEX_NUMBERS
nX1
T
cQ2
std::complex<T> >{yO
const
std::complex<T>&cF3
cG<T>::n33
cR2,Value.real());nB
xO2
temp;cG<T>::n33
temp,Value.imag());yU1^=temp.hash2;cR2.hash2^=temp.hash1;}
}
;
#endif
#ifdef FP_SUPPORT_LONG_INT_TYPE
tJ1
cQ2
long>{yO
long
cF3
eA=Value;lQ
#endif
#ifdef FP_SUPPORT_GMP_INT_TYPE
tJ1
cQ2
GmpInt>{yO
const
GmpInt&cF3
eA=Value.toInt();lQ
#endif
tD1
xL2
xN::Recalculate_Hash_NoRecursion(){xO2
cR2(nE1
Opcode)<<56,Opcode*i23(0x1131462E270012B));Depth=1;switch(Opcode
yS3{cG
xN::n33
cR2,Value
tK2
eP3
iT2:{yU1|=nE1
cS1<<48
lN1((nE1
cS1)*11)^i23(0x3A83A83A83A83A0)tM2
eP3
cFCall:case
cPCall:{yU1|=nE1
cS1<<48
lN1((~nE1
cS1)*7)^3456789;}
default:{size_t
eZ1=0
c02
0;a<yP3;++a){if(nH3
y82>eZ1)eZ1=nH3
y82;yU1+=((nH3
tR2
hash1*(a+1))>>12)lN1
nH3
tR2
hash1
lN1(3)*i23(0x9ABCD801357);cR2.hash2*=i23(0xECADB912345)lN1(~nH3
tR2
hash2)^4567890;}
Depth+=eZ1;}
}
if(Hash!=cR2){Hash=cR2;lE2=0;}
}
lB
FixIncompleteHashes(){MarkIncompletes(*this);FixIncompletes(*this);}
}
#endif
#include <cmath>
#include <list>
#include <cassert>
#ifdef FP_SUPPORT_OPTIMIZER
using
lA3
FUNCTIONPARSERTYPES;lA3{using
tN
n11
bool
x41
xG2
tree,long
count,const
xQ1::SequenceOpCode
xN&t1,xQ1::cU2&synth,size_t
max_bytecode_grow_length);static
const
e22
SinCosTanDataType{OPCODE
whichopcode;OPCODE
inverse_opcode;enum{eN2,denominator,x31,inverse_denominator}
;OPCODE
codes[4];}
SinCosTanData[12]={{cTan,cCot,{cSin,cCos,cCsc,cSec}
}
,{cCot,cCot,{cCos,cSin,cSec,cCsc}
}
,{cCos,cSec,{cSin,cTan,cCsc,cCot}
}
,{cSec,cCos,{cTan,cSin,cCot,cCsc}
}
,{cSin,cCsc,{cCos,cCot,cSec,cTan}
}
,{cCsc,cSin,{cCot,cCos,cTan,cSec}
}
,{cS2{cSinh,cCosh,cT2,{cSinh,cNop,{cS2
cNop,cCosh}
}
,{cCosh,cNop,{cSinh,cS2
cNop}
}
,{cNop,cTanh,{cCosh,cSinh,cT2,{cNop,cSinh,{cNop,cTanh,cCosh,cNop}
}
,{cNop,cCosh,{cTanh,cSinh,cT2}
;}
tN{lB
SynthesizeByteCode(std
xM3<x83>&ByteCode,std
xM3
xN&Immed,size_t&stacktop_max){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Making bytecode for:\n"
;iV
#endif
while(RecreateInversionsAndNegations()){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"One change issued, produced:\n"
;iV
#endif
FixIncompleteHashes();}
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Actually synthesizing, after recreating inv/neg:\n"
;iV
#endif
xQ1::cU2
synth;SynthesizeByteCode(synth,false);cY3
Pull(ByteCode,Immed,stacktop_max);}
lB
SynthesizeByteCode(xQ1::cU2&synth,bool
MustPopTemps)const{y91*this))yY;}
for
c12
0;a<12;++a){const
SinCosTanDataType&data=SinCosTanData[a];if(data.whichopcode!=cNop){if(nF2!=data.whichopcode)continue;CodeTree
n43;n43.lI1
n43
xC
data.inverse_opcode);n43
t82;y91
n43)){cY3
l11
cP3
nF2!=cInv
c42
GetParam(0)nC!=data.inverse_opcode)continue;y91
GetParam(0))){cY3
l11
size_t
found[4];eN3
4;++b){CodeTree
tree;if(data.tC3]==cNop){tW2
cInv);CodeTree
n53;n53.lI1
n53
xC
data.tC3^2]);n53
t82
l12
n53);}
else{tree.lI1
tW2
data.tC3]);}
tree
t82;found[b]=cY3
eA3
yN2}
if(found[data.eN2
e1
iT1
yR
eN2
eQ
iT1);n41
cDiv
nQ1
eN2
e1
iX
yR
eN2
eQ
iX);n41
cMul
nQ1
x31
e1
iX
yR
x31
eQ
iX);n41
cRDiv
nQ1
x31
e1
iT1
yR
x31
eQ
iT1);n41
cMul,2,1);cY3
l11
size_t
n_subexpressions_synthesized=SynthCommonSubExpressions(synth);switch(nF2
eJ3
iT2:cY3
PushVar(GetVar());lC
cImmed:tQ1
xY1);lC
cAdd:case
cMul:case
cMin:case
cMax:case
cAnd:case
cOr:case
cAbsAnd:case
cAbsOr:{if(nF2==cMul){bool
cG3=false;yT
lY1
t51&&isLongInteger(lY1.xY1)){c91=makeLongInteger(lY1.xY1);CodeTree
tmp(*this,typename
CodeTree::CloneTag());tmp
nC2
tmp
x22);if(x41
tmp,value,xQ1::tR1
xN::AddSequence,synth,MAX_MULI_BYTECODE_LENGTH)){cG3=true
tM2}
}
}
if(cG3)yY3
int
yV1=0;xP3
done(c41,false);CodeTree
iM;iM
xC
nF2);for(;;){bool
found=false;yT
done[a]c42
cY3
IsStackTop(lY1)){found=true;done[a]=true;lY1.n8
iM
cO
lY1);if(++yV1>1){yS
2);iM
t82
tP1
iM);yV1=yV1-2+1;}
}
}
if(!found)yY3
yT
done[a])continue;lY1.n8
iM
cO
lY1);if(++yV1>1){yS
2);iM
t82
tP1
iM);yV1=yV1-2+1;}
}
if(yV1==0
t63
nF2
eJ3
cAdd:case
cOr:case
cAbsOr:tQ1
0);lC
cMul:case
cAnd:case
cAbsAnd:tQ1
1);lC
cMin:case
cMax:tQ1
0
tK2
default:yY3++yV1;}
assert(n_stacked==1)tM2
eP3
cPow:{tT1
p0
iA2
0);tT1
p1
iA2
1);if(!p1
t51||!isLongInteger
xJ1)||!x41
p0,makeLongInteger
xJ1),xQ1::tR1
xN::MulSequence,synth,MAX_POWI_BYTECODE_LENGTH)){p0.n8
p1.n8
yS
2);yB1
cIf:case
iF3:{typename
xQ1::cU2::IfData
ifdata;GetParam(0)cT3
SynthIfStep1(ifdata,nF2);GetParam(1)cT3
SynthIfStep2(ifdata);GetParam(2)cT3
SynthIfStep3(ifdata
tK2
eP3
cFCall:case
cPCall:{for
c12
0;a<c41;++a)lY1.n8
yS
nP2)c41);n41
0x80000000u|GetFuncNo(),0,0
tK2}
default:{for
c12
0;a<c41;++a)lY1.n8
yS
nP2)c41
tK2}
}
cY3
StackTopIs(*this);if(MustPopTemps&&n_subexpressions_synthesized>0){size_t
top=cY3
GetStackTop();cY3
DoPopNMov(top-1-n_subexpressions_synthesized,top-1);}
}
}
lA3{xV1
x41
xG2
tree,long
count,const
xQ1::SequenceOpCode
xN&t1,xQ1::cU2&synth,size_t
max_bytecode_grow_length){if
e03!=0){xQ1::cU2
backup=synth;tree.n8
size_t
bytecodesize_backup=cY3
GetByteCodeSize();xQ1::x41
count
x02
size_t
bytecode_grow_amount=cY3
GetByteCodeSize()-bytecodesize_backup;if(bytecode_grow_amount>max_bytecode_grow_length){synth=backup
eL
eO3
l62}
else{xQ1::x41
count,t1,synth
iX1}
}
#endif
#include <cmath>
#include <cassert>
#ifdef FP_SUPPORT_OPTIMIZER
using
lA3
FUNCTIONPARSERTYPES;lA3{using
tN;
#define FactorStack std xM3
const
e22
PowiMuliType{x83
opcode_square;x83
opcode_cumulate;x83
opcode_invert;x83
opcode_half;x83
opcode_invhalf;}
iseq_powi={cSqr,cMul,cInv,cSqrt,cRSqrt}
,iseq_muli={iQ1
x3
cNeg,iQ1,iQ1}
n11
x73
cT1
const
PowiMuliType&cH3,const
std
tL2,lZ2&stack
cE2
1);while(IP<limit){if(iU1==cH3.opcode_square){if(!t12
tS2
2;c3
opcode_invert){nU3-yA3;c3
opcode_half){if(yA3>yD1&&isEvenInteger(tS2
yM2;c3
opcode_invhalf){if(yA3>yD1&&isEvenInteger(tS2
x73(-0.5);++IP
iR2
size_t
xB2=IP
iO2
lhs(1);if(iU1==cFetch){l91=n32;if(index<y9||size_t(index-y9)>=iW1){IP=xB2
tM2}
lhs=stack[index-y9];goto
yK2;}
if(iU1==cDup){lhs=yA3;goto
yK2;yK2:t01
yA3);++IP
iO2
subexponent=cT1
cH3
lL1
if(IP>=limit||iU1!=cH3.opcode_cumulate){IP=xB2
tM2}
++IP;stack.pop_back();yA3+=lhs*subexponent
iR2
yY3
return
yA3;}
x33
x73
ParsePowiSequence
nV2
std
tL2){lZ2
stack;t01
x73(1))eL
cT1
iseq_powi
lL1}
x33
x73
ParseMuliSequence
nV2
std
tL2){lZ2
stack;t01
x73(1))eL
cT1
iseq_muli
lL1}
x33
class
CodeTreeParserData{e83
iW2
CodeTreeParserData(bool
k_powi):stack(),clones(),keep_powi(k_powi){}
void
Eat(size_t
t43,OPCODE
opcode
t71
xX;xX
xC
opcode);eZ
cD2=Pop(t43)t81
cD2);if(!keep_powi)switch(opcode
eJ3
cTanh
yE1
xN
sinh,cosh;sinh
xC
cSinh);sinh
cO
xX
y22
sinh
x22);cosh
xC
cCosh);cosh
lP1
xX
y22
cosh
x53
pow;pow
xC
cPow);pow
lP1
cosh);pow.yJ
x73(-1)));pow
x22);xX
xC
cMul);xX.nK1
0,sinh);xX
lP1
pow
tK2
eP3
cTan
yE1
xN
sin,cos;sin
xC
cSin);sin
cO
xX
y22
sin
x22);cos
xC
cCos);cos
lP1
xX
y22
cos
x53
pow;pow
xC
cPow);pow
lP1
cos);pow.yJ
x73(-1)));pow
x22);xX
xC
cMul);xX.nK1
0,sin);xX
lP1
pow
tK2
eP3
cPow:{xG2
p0=xX
l8
0);xG2
p1=xX
l8
1);if(p1
nC==cAdd){eZ
xA3(p1
yF1)c02
0;a<p1
yF1;++a
t71
pow;pow
xC
cPow);pow
cO
p0);pow
cO
p1
nF3
pow
x22);xA3[a
tD3
pow);}
xX
xC
cMul)t81
xA3);}
yY3
default:yY3
xX
x22!keep_powi);iV1,false);
#ifdef DEBUG_SUBSTITUTIONS
t31<<t43<<", "
<<FP_GetOpcodeName(opcode)<<"->"
<<FP_GetOpcodeName(xX
nC)<<": "
i03
xX
iK
xX);
#endif
t01
xX
n13
EatFunc(size_t
t43,OPCODE
eB3
x83
funcno
tJ3
CodeTreeFuncOp
xN(eB3
funcno);eZ
cD2=Pop(t43)t81
cD2);xX
t82;
#ifdef DEBUG_SUBSTITUTIONS
t31<<t43<<", "
i03
xX
iK
xX);
#endif
iV1);t01
xX
n13
AddConst(yZ1
tJ3
CodeTreeImmed(value);iV1);Push(xX
n13
AddVar
nP2
varno
tJ3
CodeTreeVar
xN(varno);iV1);Push(xX
n13
SwapLastTwoInStack(){t21
1
tD3
t21
2]n13
Dup(){Fetch(iW1-1
n13
Fetch(size_t
which){Push(stack[which]);}
nX1
T>void
Push(T
tree){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<i03
tree
iK
tree);
#endif
t01
tree
n13
PopNMov(size_t
target,size_t
source){stack[target]=stack[source];stack.e93
target+1);}
CodeTree
xN
yL2{clones.clear(cP
yA3(stack.back());stack.e93
iW1-1)eL
yA3;}
eZ
Pop(size_t
n_pop){eZ
yA3(n_pop);for
nP2
n=0;n<n_pop;++n)yA3[n
tD3
t21
n_pop+n]);
#ifdef DEBUG_SUBSTITUTIONS
for(cB3=n_pop;n-->0;){t31
i63
yA3[n]iK
yA3[n]);}
#endif
stack.e93
iW1-n_pop)eL
yA3;}
size_t
GetStackTop(c81
iW1;}
private:void
FindClone(n91,bool=true)yY;}
private:eZ
stack;std::multimap<xO2,CodeTree
xN>clones;bool
keep_powi;private:CodeTreeParserData
nV2
CodeTreeParserData&);CodeTreeParserData&eJ1=nV2
CodeTreeParserData&);}
n11
e22
IfInfo{CodeTree
xN
eO2;CodeTree
xN
thenbranch;size_t
endif_location;IfInfo():eO2(),thenbranch(),endif_location(){}
}
;}
tN{lB
GenerateFrom
nV2
typename
FunctionParserBase
xN::Data&x03,bool
keep_powi){eZ
xH2;xH2.xJ3
x03.mVariablesAmount);for
nP2
n=0;n<x03.mVariablesAmount;++n){xH2.push_back(CodeTreeVar
xN(n+iT2));}
GenerateFrom(x03,xH2,keep_powi);}
lB
GenerateFrom
nV2
typename
FunctionParserBase
xN::Data&x03,const
xA&xH2,bool
keep_powi){const
std
xM3<x83>&ByteCode=x03.mByteCode;const
std
xM3
xN&Immed=x03.mImmed;
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"ENTERS GenerateFrom()\n"
;
#endif
CodeTreeParserData
xN
sim(keep_powi);std
xM3<IfInfo
xN>eY;for(size_t
IP=0,DP=0;;++IP){tT2:while(!eY
tK3&&(eY.eB==IP||(IP<nD1&&iU1==cJump&&eY.eE1
xN2)))){CodeTree
elsebranch=sim.yL2
n02
eY.back().eO2)n02
eY.eE1)n02
elsebranch)yH
3,cIf);eY.pop_back();}
if(IP>=nD1)break;x83
opcode=iU1;if((opcode==cSqr||opcode==cDup||(opcode==cInv&&!IsIntType
xN::yA3)||opcode==cNeg||opcode==cSqrt||opcode==cRSqrt||opcode==cFetch)){size_t
was_ip=IP
iO2
e92
ParsePowiSequence
xN(ByteCode,IP,eY
tK3?nD1:eY.eB,sim.xP
1);if(x93!=x73(1.0)){xO
x93
yA1;goto
tT2;}
if(opcode==cDup||opcode==cFetch||opcode==cNeg
yZ2
xX2=ParseMuliSequence
xN(ByteCode,IP,eY
tK3?nD1:eY.eB,sim.xP
1);if(xX2!=x73(1.0)){xO
xX2)yH
2,cMul);goto
tT2;}
}
IP=was_ip;}
if(n12>=iT2){l91=opcode-iT2
n02
xH2[index]);}
else{switch(n12
eJ3
cIf:case
iF3:{eY.e93
eY.size()+1);CodeTree
res(sim.yL2);eY.back().eO2.swap(res);eY.eB=nD1;IP+=2
iR2
case
cJump
yE1
res(sim.yL2);eY.eE1.swap(res);eY.eB=ByteCode[IP+1]+1;IP+=2
iR2
case
cImmed:xO
Immed[DP++]);lC
cDup:sim.Dup();lC
cNop:lC
cFCall:{x83
funcno=n32;assert(funcno<fpdata.mFuncPtrs.size());x83
cD2=x03.mFuncPtrs[funcno].mParams;sim.EatFunc(cD2,n12,funcno
tK2
eP3
cPCall:{x83
funcno=n32;assert(funcno<fpdata.i83.size());const
FunctionParserBase
xN&p=*x03.i83[funcno].mParserPtr;x83
cD2=x03.i83[funcno].mParams;xA
paramlist=sim.Pop(cD2);CodeTree
tU2;tU2.GenerateFrom(*p.mData,paramlist)n02
tU2
tK2
eP3
cInv:xO
1
eG2
cDiv);lC
cNeg
n22
cNeg
tK2
xO
0
eG2
cSub);lC
cSqr:xO
2
e63
cSqrt:xO
yM2
e63
cRSqrt:xO
x73(-0.5)e63
cCbrt:xO
x73(1)/x73(3)e63
cDeg:xO
fp_const_rad_to_deg
nB1
cRad:xO
fp_const_deg_to_rad
nB1
cExp:iN)goto
eI3;xO
fp_const_e
xN()eG2
cPow);lC
cExp2:iN)goto
eI3;xO
2.0
eG2
cPow);lC
cCot
n22
cTan);iN)x2
cCsc
n22
cSin);iN)x2
cSec
n22
cCos);iN)x2
cInt:
#ifndef __x86_64
iN)n63
1,cInt
tK2}
#endif
xO
yM2)tV2
yH
1,cFloor);lC
cLog10
n22
cI3
fp_const_log10inv
nB1
cLog2
n22
cI3
fp_const_log2inv
nB1
cV3:nB3
cI3
fp_const_log2inv
xN())yH
3,cMul);lC
cHypot:xO
2
yA1;tL3
xO
2
yA1
tV2;xO
yM2
e63
cSinCos:sim.Dup()yH
1,cSin);nB3
cCos);lC
cSinhCosh:sim.Dup()yH
1,cSinh);nB3
cCosh);lC
cRSub:tL3
case
cSub:iN)n63
2,cSub
tK2}
xO-1)yH
2,cMul)tV2;lC
cRDiv:tL3
case
cDiv:iN||IsIntType
xN::yA3)n63
2,cDiv
tK2}
xO-1
yA1
yH
2,cMul);lC
cAdd:case
cMul:case
cMod:case
cPow:case
cEqual:case
cLess:case
cGreater:case
i21:case
cLessOrEq:case
cGreaterOrEq:case
cAnd:case
cOr:case
cAbsAnd:case
cAbsOr:sim.Eat(2
xZ1
lC
cNot:case
cNotNot:case
cK3:case
cAbsNotNot:sim.Eat(1
xZ1
lC
cFetch:sim.Fetch(n32);lC
cPopNMov:{x83
stackOffs_target=n32;x83
stackOffs_source=n32;sim.PopNMov(stackOffs_target,stackOffs_source
tK2}
#ifndef FP_DISABLE_EVAL
case
cEval:{size_t
paramcount=x03.mVariablesAmount
yH
paramcount
xZ1
yY3
#endif
default:eI3:;x83
funcno=opcode-cAbs;assert(funcno<FUNC_AMOUNT);const
FuncDefinition&func=Functions[funcno]yH
func.cD2
xZ1
yY3}
}
Become(sim.yL2);
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Produced tree:\n"
;iV
#endif
}
}
#endif
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
#include <assert.h>
#define FP_MUL_COMBINE_EXPONENTS
lA3{using
lA3
FUNCTIONPARSERTYPES;using
tN
n11
static
void
AdoptChildrenWithSameOpcode(t2{
#ifdef DEBUG_SUBSTITUTIONS
bool
nK2=false;
#endif
for
yP
if(t5
a)nC==tC2){
#ifdef DEBUG_SUBSTITUTIONS
if(!nK2){std::cout<<"Before assimilation: "
nX
nK2=true;}
#endif
tree.AddParamsMove(t5
a).GetUniqueRef().lB2),a);}
#ifdef DEBUG_SUBSTITUTIONS
if(nK2){std::cout<<"After assimilation:   "
nX}
#endif
}
}
tN{tD1
ConstantFolding(t2{tree.Sort();
#ifdef DEBUG_SUBSTITUTIONS
void*cJ3=0
xI1"["
<<(&cJ3)<<"]Runs ConstantFolding for: "
nX
DumpHashes(tree)xI1
std::flush;
#endif
if(false){redo:;tree.Sort();
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"["
<<(&cJ3)<<"]Re-runs ConstantFolding: "
nX
DumpHashes
yN2
#endif
}
if(tC2!=cImmed){range
xN
p=CalculateResultBoundaries
yN2
if(p
eT&&p
lJ1&&p
xQ==p
lS1
yX2
p
xQ);nF}
if(false){ReplaceTreeWithOne:iH3
x73(1));goto
do_return;ReplaceTreeWithZero:iH3
xH1;goto
do_return;ReplaceTreeWithParam0:
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Before replace: "
xI1
std::hex<<'['tC1
hash1<<','tC1
hash2<<']'<<std::dec
nX
#endif
tree.eB2
0));
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"After replace: "
xI1
std::hex<<'['tC1
hash1<<','tC1
hash2<<']'<<std::dec
nX
#endif
goto
redo;iY2
tC2
yS3
lC
iT2:lC
cAnd:case
cAbsAnd:i02
bool
cC=false;for
yP{if(!lW3
a)))cC=true;n93
a),tC2==cAbsAnd)eJ3
IsNever
cQ
iI2:nG1);lC
n01
iY2
t6
eJ3
0:iE
1:tW2
tC2==cAnd?cNotNot:cAbsNotNot)n73
default:l33
cAnd||!cC)if(ConstantFolding_AndLogic
x71
yB1
cOr:case
cAbsOr:i02
bool
cC=false;for
yP{if(!lW3
a)))cC=true;n93
a),tC2==cAbsOr))i51
iE
l63
nG1);lC
n01
iY2
t6
eJ3
0
cQ
1:tW2
tC2==cOr?cNotNot:cAbsNotNot)n73
default:l33
cOr||!cC)if(ConstantFolding_OrLogic
x71
yB1
cNot:case
cK3:{x83
nF1
0;switch(nC3
eJ3
cEqual:nF1
i21;lC
i21:nF1
cEqual;lC
cLess:nF1
cGreaterOrEq;lC
cGreater:nF1
cLessOrEq;lC
cLessOrEq:nF1
cGreater;lC
cGreaterOrEq:nF1
cLess;lC
cNotNot:nF1
cNot;lC
cNot:nF1
cNotNot;lC
cK3:nF1
cAbsNotNot;lC
cAbsNotNot:nF1
cK3
tM2
default:yY3
if(opposite){tW2
OPCODE(opposite));tree
i81
t5
0).GetUniqueRef().lB2))n73
iY2
lZ1
0)e53
cU1
eJ3
iI2
cQ
l63
iE
n01
l33
cNot&&GetPositivityInfo(t5
0))==iI2)tW2
cK3);if(nC3==cIf||nC3==iF3
t71
iftree=t5
0);xG2
ifp1=iftree
l8
1);xG2
ifp2=iftree
l8
2);if(ifp1
nC
l43
ifp1
cU1
x6
ifp1
nC==cNot?cNotNot:cAbsNotNot);tZ2
l8
0))t4
cL3
l42
cM3
iO1
if(ifp2
nC
l43
ifp2
cU1
x6
tC2);tZ2)t4
cL3
xC
ifp2
nC==cNot?cNotNot:cAbsNotNot);cM3
l8
0)iO1
yB1
cNotNot:case
cAbsNotNot:{if(lW3
0)))cW
n93
0),tC2==cAbsNotNot)eJ3
IsNever
cQ
iI2:iE
n01
l33
cNotNot&&GetPositivityInfo(t5
0))==iI2)tW2
cAbsNotNot);if(nC3==cIf||nC3==iF3
t71
iftree=t5
0);xG2
ifp1=iftree
l8
1);xG2
ifp2=iftree
l8
2);if(ifp1
nC
l43
ifp1
cU1{tree.SetParam(0,iftree
y22
tree
cO
ifp1
cL3
l42
cM3
iO1
if(ifp2
nC
l43
ifp2
cU1
x6
tC2);tZ2)t4);tree
cO
ifp2);tW2
iftree
nC)n73}
yB1
cIf:case
iF3:{if(ConstantFolding_IfOperations
x71
yY3
case
cMul:{NowWeAreMulGroup:;AdoptChildrenWithSameOpcode
yN2
x73
nR1=x73(1);size_t
l52=0;bool
nS1=false;e23{if(!t5
t41
continue
iO2
immed=cN3;if(immed==xH1
goto
ReplaceTreeWithZero;nR1*=immed;++l52;}
if(l52>1||(l52==1&&lN3
nR1,x73(1))))nS1=true;if(nS1){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"cMul: Will add new "
i93
nR1<<"\n"
;
#endif
for
yP
if(t5
t41{
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<" - For that, deleting "
i93
cN3
xI1"\n"
;
#endif
nD3!lN3
nR1,x73(1)))tree
cO
e71
nR1));iY2
t6
eJ3
0:iE
1:cW
default:if(ConstantFolding_MulGrouping
x71
if(ConstantFolding_MulLogicItems
x71
yB1
cAdd:i02
x73
n42=0.0;size_t
l52=0;bool
nS1=false;e23{if(!t5
t41
continue
iO2
immed=cN3;n42+=immed;++l52;}
if(l52>1||(l52==1&&n42==xH1)nS1=true;if(nS1){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"cAdd: Will add new "
i93
n42<<"\n"
xI1"In: "
nX
#endif
for
yP
if(t5
t41{
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<" - For that, deleting "
i93
cN3
xI1"\n"
;
#endif
nD3!(n42==x73(0.0)))tree
cO
e71
n42));iY2
t6
eJ3
0
cQ
1:cW
default:if(ConstantFolding_AddGrouping
x71
if(ConstantFolding_AddLogicItems
x71
yB1
cMin:i02
size_t
yO2=0;range
xN
e2;e23{while(a+1<t6&&t5
a)xD
t5
a+1)))nG1+1);range<yP2
max
l53(!e2
lJ1||(p
lS1)<e2
lS1)){e2
lS1=p
lS1;e2
lJ1=true;yO2=a;}
}
if(e2
lJ1)for
yP{range<yP2
min
l53
a!=yO2&&p
xQ>=e2
lS1)nD3
t6==1){cW
yB1
cMax:i02
size_t
yO2=0;range
xN
tG;e23{while(a+1<t6&&t5
a)xD
t5
a+1)))nG1+1);range<yP2
min
l53(!tG
eT||p
xQ>tG
xQ)){tG
xQ=p
xQ;tG
eT=true;yO2=a;}
}
if(tG
eT){for
yP{range<yP2
max
l53
a!=yO2&&(p
lS1)<tG
xQ){nG1);}
}
}
if(t6==1){cW
yB1
cEqual:case
i21:case
cLess:case
cGreater:case
cLessOrEq:case
cGreaterOrEq:if(ConstantFolding_Comparison
x71
lC
cAbs:{range
xN
i3
t5
0));if
eO
cW
if
eP{tW2
cMul);tree.yJ
x73(1)));goto
NowWeAreMulGroup;}
if(nC3==cMul){xG2
p=t5
0);eZ
nE3;eZ
neg_set
c02
0;a<p
yF1;++a){i3
p
nF3
if
eO{nE3.push_back(p
nF3}
if
eP{neg_set.push_back(p
nF3}
}
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"Abs: mul group has "
<<nE3.size()<<" pos, "
<<neg_set.size()<<"neg\n"
;
#endif
if(!nE3
tK3||!neg_set
tK3){
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"AbsReplace-Before: "
i63
tree)xI1"\n"
<<std::flush;DumpHashes
y32;
#endif
CodeTree
xN
e73;e73
xC
cMul)c02
0;a<p
yF1;++a){i3
p
nF3
if(eO||eP){}
else
e73
cO
p
nF3}
e73
x53
nI3;nI3
xC
cAbs);nI3
lP1
e73);nI3
x53
yI1
cMul);xA3
lP1
nI3);yJ1
AddParamsMove(nE3);if(!neg_set
tK3){if(neg_set.size()%2)yJ1
yJ
x73(-1)));yJ1
AddParamsMove(neg_set);}
tree.Become(xA3);
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"AbsReplace-After: "
;DumpTree
y32
xI1"\n"
<<std::flush;DumpHashes
y32;
#endif
goto
NowWeAreMulGroup;}
}
yY3
#define HANDLE_UNARY_CONST_FUNC(funcname) nU yX2 funcname(lS));nF
case
cLog:tX3(fp_log);if(nC3==cPow
t71
pow=t5
0);if(GetPositivityInfo(pow
l8
0))==iI2)tF1
i7
tree.lV
if(GetEvennessInfo(pow
l8
1))==iI2)tF1(cP
abs;abs
xC
cAbs);abs
lP1
pow
y22
abs.Rehash
i7
pow.nK1
0,abs);tree.lV}
iZ1
nC3==cAbs
t71
pow=t5
0)l8
0);if(pow
nC==cPow)tF1(cP
abs;abs
xC
cAbs);abs
lP1
pow
y22
abs.Rehash
i7
pow.nK1
0,abs);tree.lV}
lC
cAcosh:tX3(fp_acosh);lC
cAsinh:tX3(fp_asinh);lC
cAtanh:tX3(fp_atanh);lC
cAcos:tX3(fp_acos);lC
cAsin:tX3(fp_asin);lC
cAtan:tX3(fp_atan);lC
cCosh:tX3(fp_cosh);lC
cSinh:tX3(fp_sinh);lC
cTanh:tX3(fp_tanh);lC
cSin:tX3(fp_sin);lC
cCos:tX3(fp_cos);lC
cTan:tX3(fp_tan);lC
cCeil:if(n7
tX3(fp_ceil);lC
cTrunc:if(n7
tX3(fp_trunc);lC
cFloor:if(n7
tX3(fp_floor);lC
cInt:if(n7
tX3(fp_int);lC
cCbrt:tX3(fp_cbrt);lC
cSqrt:tX3(fp_sqrt);lC
cExp:tX3(fp_exp);lC
cLog2:tX3(fp_log2);lC
cLog10:tX3(fp_log10);lC
cV3:if
lP
fp_log2(lS)*i12
cArg:tX3(fp_arg);lC
cConj:tX3(fp_conj);lC
cImag:tX3(fp_imag);lC
cReal:tX3(fp_real);lC
cPolar:if
lP
fp_polar
lK1
lC
cMod:if
lP
fp_mod
lK1
lC
cAtan2:{range
xN
i3
t5
yW2
p1=i4
1));nU&&lN3
lS,xH1){if(p1
lJ1&&(p1
lS1)<xH1{iH3
fp_const_pi
xN());nF
if(p1
eT&&p1
xQ>=xN1
xH1;nF}
if(eG1
lN3
n51,xH1){if(p0
lJ1&&(p0
lS1)<xH1{iH3-fp_const_pihalf
xN());nF
if(tQ3
p0
xQ>xH1{iH3
fp_const_pihalf
xN());nF}
if
lP
fp_atan2
lK1
if((p1
eT&&p1
xQ>xH1||(p1
lJ1&&(p1
lS1)<fp_const_negativezero
xN()nK
yQ2;yQ2
xC
cPow);yQ2
lP1
t5
1));yQ2.yJ
x73(-1)));yQ2
x53
yR2;yR2
xC
cMul);yR2
lP1
t5
0));yR2
lP1
yQ2);yR2
x22);tW2
cAtan);iN2
0,yR2
y01
1);yB1
cPow:{if(ConstantFolding_PowOperations
x71
yY3
case
cDiv:nU&&eG1
n51!=xN1
lS/i12
cInv:nU&&lS!=xN1
x73(1)/lS);nF
lC
cSub:if
lP
lS-i12
cNeg:nU
yX2-lS);nF
lC
cRad:nU
yX2
RadiansToDegrees
n83
cDeg:nU
yX2
DegreesToRadians
n83
cSqr:nU
yX2
lS*lS);nF
lC
cExp2:tX3(fp_exp2);lC
cRSqrt:nU
yX2
x73(1)/fp_sqrt
n83
cCot:yY2
fp_tan
n0
cSec:yY2
fp_cos
n0
cCsc:yY2
fp_sin
n0
cHypot:if
lP
fp_hypot
lK1
lC
cRDiv:case
cRSub:case
cDup:case
cFetch:case
cPopNMov:case
cSinCos:case
cSinhCosh:case
cNop:case
cJump:lC
cPCall:case
cFCall:case
cEval:yY3
do_return:;
#ifdef DEBUG_SUBSTITUTIONS
std::cout<<"["
<<(&cJ3)<<"]Done ConstantFolding, result: "
nX
DumpHashes
yN2
#endif
}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
tN{tD1
range
xN::set_abs(nL
bool
has_negative=!min.known||min.val<x73();bool
has_positive=!nJ3||tM3
nK3);bool
crosses_axis=has_negative&&has_positive;rangehalf
xN
newmax;if(min
l53
nJ3)newmax.set(fp_max(i31,i41);if(crosses_axis)min.set(x73());cP3
min
l53
nJ3)min.set(fp_min(i31,i41);iZ1
min.known)min.set(i31);else
min.set(i41;}
max=newmax;}
tD1
range
xN::set_neg(){std::swap(min,max);min.val=-min.val;tM3=-tM3;}
xV1
IsLogicalTrueValue
nV2
range
xN&p,bool
abs){if(nB
IsIntType
xN::yA3){if(p
eT&&p
xQ>=x73(1))l62
if(!abs&&p
lJ1
eU3<=x73(-1))l62}
cP3
p
eT&&p
xQ>=yM2)l62
if(!abs&&p
lJ1
eU3<=x73(-0.5))l62}
return
eO3
xV1
IsLogicalFalseValue
nV2
range
xN&p,bool
abs){if(nB
IsIntType
xN::yA3){if(abs)return
p
lJ1
iY
1);else
return
p
eT&&p
lJ1&&p
xQ
nK3-1)iY
1);}
cP3
abs)return
p
lJ1
iY
0.5);else
return
p
eT&&p
lJ1&&p
xQ
nK3-0.5)iY
0.5);}
}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
using
lA3
FUNCTIONPARSERTYPES;using
tN;tN{x33
range
xN
CalculateResultBoundaries
nV2
t2
#ifdef DEBUG_SUBSTITUTIONS_extra_verbose
{using
lA3
FUNCTIONPARSERTYPES;range
xN
tmp=CalculateResultBoundaries_do(tree)xI1"Estimated boundaries: "
;if(tmp
eT)std::cout<<tmp
xQ;else
std::cout<<"-inf"
xI1" .. "
;if(tmp
lJ1)std::cout<<tmp
lS1;else
std::cout<<"+inf"
xI1": "
i63
tree)xI1
std::endl
eL
tmp;}
x33
range
xN
CodeTree
xN::CalculateResultBoundaries_do
nV2
t2
#endif
{iO
yW1(-fp_const_pihalf
xN(),fp_const_pihalf
xN());iO
pi_limits(-fp_const_pi
xN(),fp_const_pi
xN());iO
abs_pi_limits(yD1,fp_const_pi
xN());iO
plusminus1_limits(x73(-yR3
using
lA3
std;switch(tC2
yS3
nQ
cS3,cS3);case
cAnd:case
cAbsAnd:case
cOr:case
cAbsOr:case
cNot:case
cK3:case
cNotNot:case
cAbsNotNot:case
cEqual:case
i21:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:{nQ
yD1,x73(1));eP3
cAbs:tN3
set_abs(e3
cLog:tN3
i22
fp_log);nL3
fp_log
e3
cLog2:tN3
i22
fp_log2);nL3
fp_log2
e3
cLog10:tN3
i22
fp_log10);nL3
fp_log10
e3
cAcosh:tN3
min.template
set_if<cGreaterOrEq
tG1
fp_acosh
i32
cGreaterOrEq
tG1
fp_acosh
e3
cAsinh:tN3
min.set(fp_asinh);eS
set(fp_asinh
e3
cAtanh:tN3
min.n5-1),fp_atanh
i32
cLess
tG1
fp_atanh
e3
cAcos:lE
nQ(eS
known&&cO3)<x73(1))?fp_acos
cO3):yD1,(m
eT&&(m
xQ)>=x73(-1))?fp_acos(m
xQ):fp_const_pi
xN());eP3
cAsin:tN3
min.n5-1),fp_asin,yW1
xQ
i32
cLess
tG1
fp_asin,yW1
lS1
e3
cAtan:tN3
min.set(fp_atan,yW1
xQ);eS
set(fp_atan,yW1
lS1
e3
cAtan2:{range
xN
i3
t5
yW2
p1=i4
1));nU&&lN3
lS,xH1)yY
abs_pi_limits;}
if(eG1
lN3
n51,xH1)yY
yW1;}
return
pi_limits;eP3
cSin:lE
bool
x51=!m
eT||!eS
known||cO3-m
xQ)>=(yA
x51)cX
x73
min=fp_mod(m
xQ,yA
min<xH1
min
yW
x73
max=fp_mod
cO3,yA
max<xH1
max
yW
if(max<min)max
yW
bool
yC1=(min<=fp_const_pihalf
xN()&&max>=fp_const_pihalf
xN());bool
nT1=(min<=e4&&max>=e4);if(yC1&&nT1)cX
if(nT1)nQ
x73(-1),nL2
if(yC1)nQ
yS2
x73(1));nQ
yS2
nL2
eP3
cCos:lE
if(m
eT)m
xQ+=fp_const_pihalf
xN();if(xI2
eS
val+=fp_const_pihalf
xN();bool
x51=!m
eT||!eS
known||cO3-m
xQ)>=(yA
x51)cX
x73
min=fp_mod(m
xQ,yA
min<xH1
min
yW
x73
max=fp_mod
cO3,yA
max<xH1
max
yW
if(max<min)max
yW
bool
yC1=(min<=fp_const_pihalf
xN()&&max>=fp_const_pihalf
xN());bool
nT1=(min<=e4&&max>=e4);if(yC1&&nT1)cX
if(nT1)nQ
x73(-1),nL2
if(yC1)nQ
yS2
x73(1));nQ
yS2
nL2
eP3
cTan:{nQ);eP3
cCeil:lE
eS
n52
cFloor:lE
m
eF1
e3
cTrunc:lE
m
eF1);eS
n52
cInt:lE
m
eF1);eS
n52
cSinh:tN3
min.set(fp_sinh);eS
set(fp_sinh
e3
cTanh:tN3
min.set(fp_tanh,plusminus1_limits.min);eS
set(fp_tanh,plusminus1_limits.max
e3
cCosh:lE
if(m
eT){if(xI2{if(m
xQ>=yD1&&eS
val>=xH1{m
xQ
yD}
iZ1(m
xQ)<yD1&&eS
val>=xH1{x73
tmp
yD
if(tmp>eS
val)eS
val=tmp;m
xQ=x73(1);}
else{m
xQ
yD
std::swap(m
xQ,eS
val);}
}
cP3
m
xQ>=xH1{m.e5
m
xQ=fp_cosh(m
xQ);}
else{m.e5
m
xQ=x73(1);}
}
}
else{m
eT=true;m
xQ=x73(1);if(xI2{m
xQ=fp_cosh
cO3);m.e5}
else
m.e5}
return
m;eP3
cIf:case
iF3:{range
xN
res1=i4
1));range
xN
res2=i4
2));if(!res2
eT)res1
eT=false;iZ1
res1
eT&&(res2
xQ)<res1
xQ)res1
xQ=res2
xQ;if(!res2
lJ1)res1.e5
iZ1
res1
lJ1&&(res2
lS1)>res1
lS1)res1
lS1=res2
lS1
eL
res1;eP3
cMin:{bool
iP=false;bool
iQ=false;range
y43;xB
m=i4
cQ3
m
eT)iP=true;iY1
eT||(m
xQ)<y53)y53=m
xQ;if(!xI2
iQ=true;iY1
lJ1||cO3)<y63)y63=eS
val;}
if(iP)y93
iQ)yA3.e5
return
yA3;eP3
cMax:{bool
iP=false;bool
iQ=false;range
y43;xB
m=i4
cQ3
m
eT)iP=true;iY1
eT||m
xQ>y53)y53=m
xQ;if(!xI2
iQ=true;iY1
lJ1||eS
val>y63)y63=eS
val;}
if(iP)y93
iQ)yA3.e5
return
yA3;eP3
cAdd:{range
y43(yD1,xH1;xB
item=i4
a));if(item
eT)y53+=item
xQ;else
y93
item
lJ1)y63+=item
lS1;else
yA3.e5
if(!y73&&!y83)yY3
if(y73&&y83&&y53>y63)std::swap(y53,y63)eL
yA3;eP3
cMul:{e22
Value{enum
nM3{i42,l72,tP3}
;nM3
eX
iO2
value;Value(nM3
t):eX(t),value(0){}
Value(x73
v):eX(i42),value(v){}
bool
cV2
c81
eX==l72||(eX==i42&&value<xH1;}
void
eJ1*=nV2
Value&rhs){if(eX==i42&&rhs.eX==i42)value*=rhs.value;else
eX=(cV2)!=rhs.cV2)?l72:tP3);}
iH2<nV2
Value&rhs
c81(eX==l72&&rhs.eX!=l72)||(eX==i42&&(rhs.eX==tP3||(rhs.eX==i42&&value<rhs.value)));}
}
;e22
yX1{Value
yU2,yV2;yX1():yU2
x42,yV2(Value::l72){}
void
xJ2
Value
cR3,const
Value&value2){cR3*=value2;if(cR3<yU2)yU2=cR3;if(yV2<cR3)yV2=cR3;}
}
;range
y43(x73(yR3
xB
item=i4
cQ3
item
eT&&!item
lJ1)nQ);Value
nN3=y73?Value(y53):Value(Value::l72);Value
nO3=y83?Value(y63):Value
x42;Value
nP3=item
eT?Value(item
xQ):Value(Value::l72);Value
nQ3=item
lJ1?Value(item
lS1):Value
x42;yX1
range;range.xJ2
nN3,nP3
lX3
nN3,nQ3
lX3
nO3,nP3
lX3
nO3,nQ3);if(range.yU2.eX==Value::i42)y53=range.yU2.value;else
y93
range.yV2.eX==Value::i42)y63=range.yV2.value;else
yA3.e5
if(!y73&&!y83)yY3
if(y73&&y83&&y53>y63)std::swap(y53,y63)eL
yA3;eP3
cMod:{range
xN
x=i4
yW2
y=i4
1));if(y
lJ1){if(y
lS1>=xH1{if(!x
eT||(x
xQ)<xH1
nQ-y
lS1,y
lS1);else
nQ
yD1,y
lS1);}
cP3!x
lJ1||(x
lS1)>=xH1
nQ
y
lS1,-y
lS1);else
nQ
y
lS1,fp_const_negativezero
xN());}
}
else
nQ);eP3
cPow:{if(eG1
n51==xH1{nQ
x73(yR3}
nU&&lS==xH1{nQ
yD1,xH1;}
nU&&lN3
lS
nB2
nQ
x73(yR3}
if(eG1
n51>yD1&&GetEvennessInfo(t5
1))==iI2
yZ2
e92
n51;range
xN
tmp=i4
yW2
yA3;y73=true;y53=0;if(tmp
eT&&tmp
xQ>=xH1
y53=eT3
tmp
xQ,xK2;iZ1
tmp
lJ1&&tmp
lS1<=xH1
y53=eT3
tmp
lS1,xK2;yA3.e5
if(tmp
eT&&tmp
lJ1){y83=true;y63=fp_max(fp_abs(tmp
xQ),fp_abs(tmp
lS1));y63=eT3
y63,xK2;}
return
yA3;}
range
xN
i3
t5
yW2
p1=i4
1));TriTruthValue
p0_positivity=(tQ3(p0
xQ)>=xH1?iI2:(p0
lJ1&&(p0
lS1)<yD1?l63
Unknown);TriTruthValue
cW2=GetEvennessInfo(t5
1));TriTruthValue
tH=Unknown;switch(p0_positivity)i51
tH=iI2;lC
l63{tH=cW2
tM2}
default:switch(cW2)i51
tH=iI2;lC
l63
lC
Unknown:{if(eG1!t12
n51)&&n51>=xH1{tH=iI2;}
yY3}
iY2
tH)i51{x73
min=yD1;if(tQ3
p1
eT){min=eT3
p0
xQ,p1
xQ);if(p0
xQ<yD1&&(!p1
lJ1||p1
lS1>=xH1&&min>=xH1
min=yD1;}
if(tQ3
p0
xQ>=yD1&&p0
lJ1&&p1
lJ1
yZ2
max=eT3
p0
lS1,p1
lS1);if(min>max)std::swap(min,max);nQ
min,max);}
nQ
min,false);eP3
l63{nQ
false,fp_const_negativezero
xN());}
default:{yY3
yB1
cNeg:tN3
set_neg(e3
cSub:{yK
cNeg);tmp2
tO3
x13
cAdd);x23;tmp
nR3
eL
lI
cInv
yE1
lX-1
xZ3
cDiv:{yK
cInv);tmp2
tO3
x13
yG1
nR3
eL
lI
cRad
yE1
xN
tmp;x13
yG1.yJ
fp_const_rad_to_deg
xN(xZ3
cDeg
yE1
xN
tmp;x13
yG1.yJ
fp_const_deg_to_rad
xN(xZ3
cSqr
yE1
lX
2
xZ3
cExp
yE1
xN
tmp;x13
cPow);tmp.yJ
fp_const_e
xN()));x23
eL
lI
cExp2
yE1
xN
tmp;x13
cPow);tmp.yJ
c32
x23
eL
lI
cCbrt:tN3
min.set(fp_cbrt);eS
set(fp_cbrt
e3
cSqrt:lE
if(m
eT)m
xQ=(m
xQ)<yD1?0:fp_sqrt(m
xQ);if(xI2
eS
val=cO3)<yD1?0:fp_sqrt
cO3
e3
cRSqrt
yE1
lX-0.5
xZ3
cHypot
yE1
xN
xsqr,ysqr,add,sqrt;xsqr.nN
0));xsqr.yJ
c32
ysqr
tO3
ysqr.yJ
c32
xsqr
xC
cPow);ysqr
xC
cPow);add
lP1
xsqr);add
lP1
ysqr);add
xC
cAdd);sqrt
lP1
add);sqrt
xC
cSqrt)eL
CalculateResultBoundaries(sqrt);eP3
cV3:{yK
cLog2);tmp2.nN
0));x13
cMul);tmp
nR3;tmp.nN
1))eL
lI
cCot:{yK
cTan)x7
lI
cSec:{yK
cCos)x7
lI
cCsc:{yK
cSin)x7
CalculateResultBoundaries(tmp);}
lC
cRDiv:case
cRSub:case
cDup:case
cFetch:case
cPopNMov:case
cSinCos:case
cSinhCosh:case
cNop:case
cJump:case
iT2:lC
cArg:case
cConj:case
cImag:case
cReal:case
cPolar:lC
cPCall:lC
cFCall:lC
cEval:yY3
nQ);}
x33
TriTruthValue
GetIntegerInfo
nV2
t2{switch(tC2
yS3
return
t12
cS3)?iI2:IsNever;case
cFloor:case
cCeil:case
cTrunc:case
cInt:return
iI2;case
cAnd:case
cOr:case
cNot:case
cNotNot:case
cEqual:case
i21:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:return
iI2;case
cIf:{TriTruthValue
a=GetIntegerInfo(t5
1));TriTruthValue
b=GetIntegerInfo(t5
2));if(a==b)return
a
eL
Unknown;eP3
cAdd:case
cMul:{for
yP
if(GetIntegerInfo(t5
a))!=iI2)return
Unknown
eL
iI2;}
default:yY3
return
Unknown;}
xV1
IsLogicalValue
nV2
t2{switch(tC2
yS3
return
lN3
cS3,xH1||lN3
cS3,x73(1));case
cAnd:case
cOr:case
cNot:case
cNotNot:case
cAbsAnd:case
cAbsOr:case
cK3:case
cAbsNotNot:case
cEqual:case
i21:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:x0
cMul:{for
yP
if(!lW3
a)y03
false
e62
case
cIf:case
iF3:yY
lW3
1))nZ1
t5
2));}
default:yY3
return
eO3}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
using
lA3
FUNCTIONPARSERTYPES;
#if defined(__x86_64) || !defined(FP_SUPPORT_CBRT)
# define CBRT_IS_SLOW
#endif
#if defined(DEBUG_POWI) || defined(DEBUG_SUBSTITUTIONS)
#include <cstdio>
#endif
lA3
xQ1{extern
const
x83
char
powi_table[256];}
lA3{using
tN
n11
bool
IsOptimizableUsingPowi(long
immed,long
penalty=0){xQ1::cU2
synth;cY3
PushVar(iT2);size_t
bytecodesize_backup=cY3
GetByteCodeSize();xQ1::x41
immed,xQ1::tR1
xN::MulSequence,synth);size_t
bytecode_grow_amount=cY3
GetByteCodeSize()-bytecodesize_backup
eL
bytecode_grow_amount<size_t(MAX_POWI_BYTECODE_LENGTH-penalty);}
tD1
ChangeIntoRootChain(n91
tree,bool
l73,long
i52,long
i62){while(i62>0
lB1
cCbrt);lO1
tmp
x22);tree.nS3;--i62;}
while(i52>0
lB1
cSqrt);if(l73){x13
cRSqrt);l73=eO3
lO1
tmp
x22);tree.nS3;--i52;}
if(l73
lB1
cInv);lO1
tree.nS3;}
}
x33
e22
RootPowerTable{static
const
x73
RootPowers[(1+4)*(1+3)];}
n11
const
x73
tP(1+4)*(1+3)]={x73(1)lT
tR3
tR3
2*tR3
2*2*2)lT
tS3
i61
2*i61
2*2*i61
2*2*2*i61
tS3
3*i61
3*2*i61
3*2*2*i61
3*2*2*2*i61
3*tS3
3*3*i61
3*3*2*i61
3*3*2*2*i61
3*3*2*2*2*2)}
;e22
PowiResolver{static
const
x83
MaxSep=4;static
nV3
tT3=5;typedef
int
cU3;typedef
long
c13;typedef
long
tQ;e22
c52{c52():n_int_sqrt(0),n_int_cbrt(0),sep_list(),n61(0){}
int
n_int_sqrt;int
n_int_cbrt;int
tN1
MaxSep];tQ
n61;}
n11
static
c52
CreatePowiResult(x73
xK2{c52
yA3;cU3
tJ=FindIntegerFactor(xK2;if(tJ==0){
#ifdef DEBUG_POWI
i72"no factor found for %Lg\n"
,xL1
xK2;
#endif
return
yA3;}
yA3.n61=yH1
x93,tJ);c13
eP2=EvaluateFactorCost(tJ,0,0,0)+cH
yA3.n61);int
tU3=0;int
tV3=0;int
nT3=0;
#ifdef DEBUG_POWI
i72"orig = %Lg\n"
,xL1
xK2;i72"plain factor = "
iC3" %ld\n"
,(int)tJ,(long)eP2);
#endif
for
nP2
n_s=0;n_s<MaxSep;++n_s){int
xL=0;c13
yY1=eP2;cU3
c61=tJ;for(int
s=1;s<tT3*4;++s){
#ifdef CBRT_IS_SLOW
if(s>=tT3)break;
#endif
int
n_sqrt=s%tT3;int
n_cbrt=s/tT3;if(n_sqrt+n_cbrt>4)continue
iO2
lC1=x93;lC1-=tP
s];i71=FindIntegerFactor(lC1);if(xX2!=0){tQ
xY=yH1
lC1,xX2);c13
cost=EvaluateFactorCost(xX2,tU3+n_sqrt,tV3+n_cbrt,nT3+1)+cH
xY);
#ifdef DEBUG_POWI
i72"Candidate sep %u (%d*sqrt %d*cbrt)factor = "
iC3" %ld (for %Lg to %ld)\n"
,s,n_sqrt,n_cbrt,xX2,(long)cost,xL1
lC1,(long)xY);
#endif
if(cost<yY1){xL=s;c61=xX2;yY1=cost;}
}
}
if(!xL)break;
#ifdef DEBUG_POWI
i72"CHOSEN sep %u (%d*sqrt %d*cbrt)factor = "
iC3" %ld, exponent %Lg->%Lg\n"
,xL,xL%tT3,xL/tT3,c61,yY1,xL1(xK2,xL1(x93-tP
xL]));
#endif
yA3.tN1
n_s]=xL
cX1-=tP
xL];tU3+=xL%tT3;tV3+=xL/tT3;eP2=yY1;tJ=c61;nT3+=1;}
yA3.n61=yH1
x93,tJ);
#ifdef DEBUG_POWI
i72"resulting exponent is %ld (from exponent=%Lg, best_factor=%Lg)\n"
,yA3.n61,xL1
x93,xL1
tJ);
#endif
while(tJ%2==0){++yA3
cZ2;tJ/=2;}
while(tJ%3==0){++yA3.n_int_cbrt;tJ/=3;}
return
yA3;}
private:static
c13
cH
tQ
xY){static
std::map
cX2
iF;if(xY<0){c13
cost=22
eL
cost+cH-xY);}
std::map
cX2::yN3
i=iF.xY2
xY);if(i!=iF.e21
xY)return
i
e82;std::pair
cX2
yA3(xY,0.0);c13&cost=yA3
xU3;while(xY>1){int
xX2=0;if(xY<256){xX2=xQ1::powi_table[xY];if(xX2&128)xX2&=127;else
xX2=0;if(xX2&64)xX2=-(xX2&63)-1;}
if(xX2){cost+=cH
xX2);xY/=xX2
iR2
if(!(xY&1)){xY/=2;cost+=6;}
else{cost+=7;xY-=1;}
}
iF.yI3,yA3)eL
cost;}
cJ1
tQ
yH1
yZ1,i71)yY
makeLongInteger(value*x73(xX2));}
cJ1
bool
c01
yZ1,i71
yZ2
v=value*x73(xX2)eL
isLongInteger(v);}
cJ1
cU3
FindIntegerFactor(yZ1){i71=(2*2*2*2);
#ifdef CBRT_IS_SLOW
#else
xX2*=(3*3*3);
#endif
cU3
nU3
0;if(c01
value,xX2)){nU3
xX2;while((xX2%2)==0&&c01
value,xX2/2))nU3
xX2/=2;while((xX2%3)==0&&c01
value,xX2/3))nU3
xX2/=3;}
#ifdef CBRT_IS_SLOW
if(yA3==0){if(c01
value,3
y03
3;}
#endif
return
yA3;}
static
int
EvaluateFactorCost(int
xX2,int
s,int
c,int
nmuls){nV3
nW3=6;
#ifdef CBRT_IS_SLOW
nV3
eQ2=25;
#else
nV3
eQ2=8;
#endif
int
nU3
s*nW3+c*eQ2;while(xX2%2==0){xX2/=2;yA3+=nW3;}
while(xX2%3==0){xX2/=3;yA3+=eQ2;}
yA3+=nmuls
eL
yA3;}
}
;}
tN{xV1
CodeTree
xN::RecreateInversionsAndNegations(bool
prefer_base2){bool
changed=false
c02
0;a<c41;++a)if(lY1.RecreateInversionsAndNegations(prefer_base2))xD2
if(changed){exit_changed:Mark_Incompletely_Hashed(iX1
switch(nF2
eJ3
cMul:{eZ
n62;CodeTree
xN
n72,cV1;if(true){bool
nU1=false
iO2
xC2=0
c02
c41;a
nV
c62
0)cY2
tW
1)t51){nU1=true;xC2=tW
1).xY1
tM2}
}
if(nU1
yZ2
immeds=1.0
c02
c41;a
nV
t51){immeds*=powgroup.xY1;c11}
for
c12
c41;a-->0;){n91
powgroup=lY1;if(powgroup
c62
0)cY2
tW
1).IsImmed(nK&log2=tW
0);log2.l61
log2
xC
cV3);log2.yJ
eT3
immeds,x73(1)/xC2)));log2
x22
tK2}
}
}
}
for
c12
c41;a
nV
c62
1)t51){xG2
exp_param=tW
1)iO2
e92
exp_param.xY1;if(e51,x73(-1))){l61
n62.push_back(lY1
y22
c11
iZ1
x93<yD1&&t12
x93
nK
iR;iR
xC
cPow);iR
cO
tW
0));iR.yJ-xK2);iR
x22);n62.push_back(iR);l61
c11}
iZ1
powgroup
cY2!n72
xN2)){n72=tW
0);l61
c11
iZ1
powgroup
nC==cV3&&!cV1
xN2)){cV1=powgroup;l61
c11}
if(!n62
tK3){xD2
CodeTree
xN
iM1;iM1
xC
cMul);iM1
i81
n62);iM1
x53
yI1
cMul);yJ1
SetParamsMove(tI
if(yJ1
IsImmed()&&lN3
yJ1
xY1
nB2
n82
cInv)eN
iM1);}
cP3
yJ1
y82>=iM1.y82){n82
cDiv
lN2
eN
iM1);}
else{n82
cRDiv)eN
iM1
lN2;}
}
}
if(n72
xN2
nK
yI1
nF2);yJ1
SetParamsMove(tI
while(yJ1
RecreateInversionsAndNegations(prefer_base2))yJ1
FixIncompleteHashes();n82
cV3)eN
n72
lN2;xD2}
if(cV1
xN2
nK
yI1
cMul);xA3
lP1
cV1
l8
1));yJ1
AddParamsMove(tI
while(yJ1
RecreateInversionsAndNegations(prefer_base2))yJ1
FixIncompleteHashes();DelParams();n82
cV3)eN
cV1
l8
0)lN2;xD2
yB1
cAdd:{eZ
i92
c02
c41;a-->0;)if(cW3
cMul){nA2
yK1:;n91
xA3=xE2
for(size_t
b=xA3
yF1;b-->0;){if(xA3
l8
b)lM1
xX2=xA3
l8
b).xY1;iZ2
xX2
y4
yK1;}
yJ1
l61
yJ1
l93
b);l83
iZ1
lN3
xX2,x73(-2)))eV
yK1;}
yJ1
l61
yJ1
l93
b);yJ1
yJ
c32
l83}
}
if(tK){yJ1
tR
xA3);c11}
iZ1
cW3
cDiv&&!IsIntType
xN::yA3){nA2
yL1:;n91
iM1=xE2
if(iM1
l8
0)t51){iZ2
iM1
l8
0).xY1
y4
yL1;}
iM1.l61
iM1.l93
0);iM1
xC
cInv);l83}
if(tK)eV
yL1;}
iM1.tR
iM1);c11}
iZ1
cW3
cRDiv&&!IsIntType
xN::yA3){nA2
xD1:;n91
iM1=xE2
if(iM1
l8
1)t51){iZ2
iM1
l8
1).xY1
y4
xD1;}
iM1.l61
iM1.l93
1);iM1
xC
cInv);l83}
if(tK)eV
xD1;}
iM1.tR
iM1);c11}
if(!i92
tK3){
#ifdef DEBUG_SUBSTITUTIONS
i72"Will make a Sub conversion in:\n"
);fflush(stdout);iV
#endif
CodeTree
xN
c72;c72
xC
cAdd);c72
i81
i92);c72
x53
cW1;cW1
xC
cAdd);cW1
i81
lB2));cW1
x22);if(cW1
t51&&lN3
cW1.xY1,xH1){n82
cNeg);eC);}
cP3
cW1.y82==1){n82
cRSub);eC)eN
cW1);}
iZ1
c72
nC==cAdd){n82
cSub)eN
cW1);eC
y22
for
c12
1;a<c72
yF1;++a
t71
eR2;eR2
xC
cSub);eR2
i81
lB2));eR2
t82
eN
eR2);eC
nF3}
}
else{n82
cSub)eN
cW1);eC);}
}
#ifdef DEBUG_SUBSTITUTIONS
i72"After Sub conversion:\n"
);fflush(stdout);iV
#endif
yB1
cPow:{xG2
p0
iA2
0);xG2
p1
iA2
1);if(p1
t51){if
xJ1!=yD1&&!isInteger
xJ1)){eH
c52
r=eH
CreatePowiResult(fp_abs
xJ1));if(r.n61!=0){bool
l82=false;if
xJ1<yD1&&r.tN1
0]==0&&r
cZ2>0){l82=true;}
#ifdef DEBUG_POWI
i72"Will resolve powi %Lg as powi(chain(%d,%d),%ld)"
,xL1
fp_abs
xJ1),r
cZ2,r.n_int_cbrt,r.n61);for
nP2
n=0;n<eH
MaxSep;++n){if(r
iB2==0)break;int
n_sqrt=r
iB2%eH
tT3;int
n_cbrt=r
iB2/eH
tT3;i72"*chain(%d,%d)"
,n_sqrt,n_cbrt);}
i72"\n"
);
#endif
CodeTree
xN
e02
iA2
0
cP
c82=e02;c82.l61
ChangeIntoRootChain(c82,l82,r
cZ2,r.n_int_cbrt);c82
x53
pow;if(r.n61!=1){pow
xC
cPow);pow
lP1
c82);pow.yJ
x73(r.n61)));}
else
pow.swap(c82
cP
mul;mul
xC
cMul);mul
lP1
pow);for
nP2
n=0;n<eH
MaxSep;++n){if(r
iB2==0)break;int
n_sqrt=r
iB2%eH
tT3;int
n_cbrt=r
iB2/eH
tT3;CodeTree
xN
eS2=e02;eS2.l61
ChangeIntoRootChain(eS2,false,n_sqrt,n_cbrt);eS2
x22);mul
lP1
eS2);}
if
xJ1<yD1&&!l82){mul
x22);n82
cInv);nK1
0,mul);l93
1);}
else{n82
cMul);SetParamsMove(mul.lB2));}
#ifdef DEBUG_POWI
iV
#endif
xD2
yY3}
}
if(nF2==cPow&&(!p1
t51||!isLongInteger
xJ1)||!IsOptimizableUsingPowi
xN(makeLongInteger
xJ1)))){if(p0
t51&&p0.xY1
nK3
0.0)){if(prefer_base2
yZ2
c92=fp_log2(p0.xY1);iZ2
c92
nB2
l93
0);}
else{n1
e71
c92))cX1
cO
p1
i82
cL}
n82
cExp2);xD2}
else{x73
c92=fp_log(p0.xY1);iZ2
c92
nB2
l93
0);}
else{n1
e71
c92))cX1
cO
p1
i82
cL}
n82
cExp);xD2}
}
iZ1
GetPositivityInfo(p0)==iI2){if(prefer_base2
t71
log;log
xC
cLog2);log
cO
p0);log
x22);n1
p1)cX1
lP1
log
i82
n82
cExp2);cL
xD2}
else{CodeTree
xN
log;log
xC
cLog);log
cO
p0);log
x22);n1
p1)cX1
lP1
log
i82
n82
cExp);cL
xD2}
}
yB1
cDiv:{if(GetParam(0)t51&&lN3
GetParam(0).xY1
nB2
n82
cInv);l93
0);}
yY3
default:yY3
if(changed)goto
exit_changed
eL
changed;}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
using
lA3
FUNCTIONPARSERTYPES;lA3{using
tN;class
eR3{size_t
nV1;size_t
eI;size_t
eJ;size_t
lD1;size_t
tL;size_t
tM;size_t
nH1;e83
eR3():nV1(0),eI(0),eJ(0),lD1(0),tL(0),tM(0),nH1(0){}
void
AddFrom(OPCODE
op){nV1+=1;eQ3
cCos)++eI;eQ3
cSin)++eJ;eQ3
cSec)++eI;eQ3
cCsc)++eJ;eQ3
cTan)++lD1;eQ3
cCot)++lD1;eQ3
cSinh)++tM;eQ3
cCosh)++tL;eQ3
cTanh)++nH1;}
size_t
GetCSEscore()const{size_t
nU3
nV1
eL
yA3;}
int
NeedsSinCos()const{bool
yM1=(nV1==(eI+eJ+lD1));if((lD1&&(eJ||eI))||(eJ&&eI)){if(yM1)return
1
eL
2;}
return
0;}
int
NeedsSinhCosh()const{bool
yM1=(nV1==(tL+tM+nH1));if((nH1&&(tM||tL))||(tM&&tL)){if(yM1)return
1
eL
2;}
return
0;}
size_t
MinimumDepth()const{size_t
n_sincos=std::min(eI,eJ);size_t
n_sinhcosh=std::min(tL,tM);if(n_sincos==0&&n_sinhcosh==0)return
2
eL
1;}
}
n11
class
TreeCountType:public
std::multimap<xO2,std::pair<eR3,CodeTree
xN> >{}
xI3
FindTreeCounts(tH1&nD2,xG2
tree,OPCODE
xF2,bool
skip_root=false){cY
i=nD2.xY2
tree.GetHash());if(!skip_root){bool
found=false;for(;i!=nD2.e21
tree.GetHash();++i){if(tree
xD
i
e82
xU3)){i
e82.first.AddFrom(xF2);found=true
tM2}
}
if(!found){eR3
count;count.AddFrom(xF2);nD2.yI3,std::make_pair(tree.GetHash(),std::make_pair
e03
e53)));}
}
e23
FindTreeCounts(nD2
tA3
tC2);}
e22
c0{bool
BalanceGood;bool
FoundChild;}
n11
c0
lE1
xG2
root,xG2
child){if(root
xD
child)){c0
nU3{true,true}
eL
yA3;}
c0
nU3{true,false}
;if(root
nC==cIf||root
nC==iF3){c0
cond=lE1
root
l8
0
t22
c0
y6=lE1
root
l8
1
t22
c0
yC=lE1
root
l8
2
t22
if(cond
c1||y6
c1||yC
c1){yA3
c1=true;}
yA3
eD=((y6
c1==yC
c1)||yJ3)&&(cond
eD||(y6
c1&&yC
c1))&&(y6
eD||yJ3)&&(yC
eD||yJ3);}
else{bool
i91=false;bool
nW1=false;for(size_t
b=root
yF1,a=0;a<b;++a){c0
tmp=lE1
root
l8
a
t22
if(tmp
c1)yA3
c1=true;if(tmp
eD==false)i91=true;iZ1
tmp
c1)nW1=true;}
if(i91&&!nW1)yA3
eD=eO3
return
yA3;}
xV1
nY3
eG3
tW3
xG2
tree,const
xQ1::cU2&synth,const
tH1&nD2){for(size_t
b=t6,a=0;a<b;++a){xG2
leaf=t5
a);cY
synth_it;y12
tH1::const_iterator
i=nD2.yM3
i!=nX3;++i){if(i->first!=leaf.GetHash())continue;const
eR3&occ
nM2
first;size_t
score=occ.GetCSEscore();xG2
candidate
nM2
second;if(tO1
candidate)c42
leaf.y82<occ.MinimumDepth()c42
score<2
c42
lE1
tW3
leaf)eD==false)continue
e62
if(nY3(tW3
leaf,synth,nD2))l62}
return
eO3
xV1
iA1
xG2
yK3,xG2
expr){for(xG
yK3
l8
a)xD
expr))l62
for(xG
iA1
yK3
l8
a),expr
y03
true
eL
eO3
xV1
GoodMomentForCSE
eG3
yK3,xG2
expr){if(yK3
nC==cIf)l62
for(xG
yK3
l8
a)xD
expr))l62
size_t
iD2=0;for(xG
iA1
yK3
l8
a),expr))++iD2
eL
iD2!=1;}
}
tN{x33
size_t
CodeTree
xN::SynthCommonSubExpressions(xQ1::xM1
const{if(c41==0)return
0;size_t
stacktop_before=cY3
GetStackTop();tH1
nD2;FindTreeCounts(nD2,*this,nF2,true);for(;;){size_t
cA2=0;
#ifdef DEBUG_SUBSTITUTIONS_CSE
std::cout<<"Finding a CSE candidate, root is:"
<<std::endl;DumpHashes(*this);
#endif
cY
cs_it(nX3);for(cY
j=nD2.yM3
j!=nX3;){cY
i(j++);const
eR3&occ
nM2
first;size_t
score=occ.GetCSEscore();xG2
tree
nM2
second;
#ifdef DEBUG_SUBSTITUTIONS_CSE
std::cout<<"Score "
<<score<<":\n"
<<std::flush;DumpTreeWithIndent
yN2
#endif
if(tO1
tree))y2
if(tree.y82<occ.MinimumDepth())y2
if(score<2)y2
if(lE1*this
e53)eD==false)y2
if(nY3(*this
e53,synth,nD2)){iS2
if(!GoodMomentForCSE(*this
e53))y2
score*=tree.y82;if(score>cA2){cA2=score;cs_it=i;}
}
if(cA2<=0){
#ifdef DEBUG_SUBSTITUTIONS_CSE
std::cout<<"No more CSE candidates.\n"
<<std::flush;
#endif
yY3
xG2
tree=cs_it
e82
xU3;
#ifdef DEBUG_SUBSTITUTIONS_CSE
std::cout<<iE3"Common Subexpression:"
;DumpTree
xN(tree)xI1
std::endl;
#endif
#if 0
int
n71=occ.NeedsSinCos();int
iA=occ.NeedsSinhCosh(cP
iE2,iF2,cB2,cC2;eS3){iE2
eT2
iE2
xC
cSin);iE2
x22);iF2
eT2
iF2
xC
cCos);iF2
x22);if(tO1
iE2)||tO1
iF2)){eS3==2){nE2
iS2
n71=0;}
}
if(iA){cB2
eT2
cB2
xC
cSinh);cB2
x22);cC2
eT2
cC2
xC
cCosh);cC2
x22);if(tO1
cB2)||tO1
cC2)){if(iA==2){nE2
iS2
iA=0;}
}
#endif
tree.SynthesizeByteCode(synth,false);nE2
#ifdef DEBUG_SUBSTITUTIONS_CSE
cY3
template
Dump<0>()xI1"Done with Common Subexpression:"
;DumpTree
xN(tree)xI1
std::endl;
#endif
#if 0
eS3){eS3==2||iA){cY3
eH1}
n41
cSinCos,1,2)tP1
iE2,1)tP1
iF2,0);}
if(iA){eS3)cY3
eH1
if(iA==2){cY3
eH1}
n41
cSinhCosh,1,2)tP1
cB2,1)tP1
cC2,0);}
#endif
}
return
cY3
xP
stacktop_before;}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
tD1
FunctionParserBase
xN
xK1
using
tN;CopyOnWrite(cP
tree;tree.GenerateFrom(*mData);FPoptimizer_Optimize::ApplyGrammars
yN2
std
xM3<x83>cX3;std
xM3
xN
immed;size_t
stacktop_max=0;tree.SynthesizeByteCode(cX3,immed,stacktop_max);if(mData->mStackSize!=stacktop_max){mData->mStackSize=x83(stacktop_max);
#if !defined(FP_USE_THREAD_SAFE_EVAL) && \
    !defined(FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA)
mData->mStack.e93
stacktop_max);
#endif
}
mData->mByteCode.swap(cX3);mData->mImmed.swap(immed);}
#ifdef FP_SUPPORT_MPFR_FLOAT_TYPE
xK
MpfrFloat>xK1}
#endif
#ifdef FP_SUPPORT_GMP_INT_TYPE
xK
GmpInt>xK1}
#endif
#ifdef FP_SUPPORT_COMPLEX_DOUBLE_TYPE
xK
std::complex<double> >xK1}
#endif
#ifdef FP_SUPPORT_COMPLEX_FLOAT_TYPE
xK
std::complex<float> >xK1}
#endif
#ifdef FP_SUPPORT_COMPLEX_LONG_DOUBLE_TYPE
xK
std::complex<long
double> >xK1}
#endif
FUNCTIONPARSER_INSTANTIATE_TYPES
#endif

#endif
