/***************************************************************************\
|* Function Parser for C++ v4.1                                            *|
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
#define FPOPTIMIZER_MERGED_FILE
#include "fparser.h"
#include "fptypes.h"
#define oA3 ,cSinh dD
#define o93 (p gC a));}
#define o83 goto oD
#define o73 pclone
#define o63 oV.mL2
#define o53 "%g\n",
#define o43 );Value
#define o33 "Found "
#define o23 ;for aK
#define o13 " factor "
#define o03 "dup(%u) "
#define dZ3 FuncParsers
#define dY3 "immed "<<
#define dX3 "PUSH ";q13(
#define dW3 stderr
#define dV3 sep2=" "
#define dU3 cache_needed[
#define dT3 fprintf
#define dS3 FPHASH_CONST
#define dR3 "Applying "
#define dQ3 ||tree.GetOpcode
#define dP3 HANDLE_UNARY_CONST_FUNC
#define dO3 (p1 hF1
#define dN3 {double
#define dM3 ,dQ,info
#define dL3 ,{{2,
#define dK3 within,
#define dJ3 c_count
#define dI3 s_count
#define dH3 qP2 2*2
#define dG3 g8 cMul
#define dF3 gB3 min
#define dE3 ;double
#define dD3 {MinMaxTree
#define dC3 :qJ h3 qR*
#define dB3 case 1:
#define dA3 case 0:
#define d93 fp_pow(
#define d83 m6 cond
#define d73 cAbsIf)
#define d63 [p gV3
#define d53 ).hH 2))
#define d43 ].first
#define d33 b dM a.
#define d23 a,const
#define d13 ,factor
#define d03 +=1;g61
#define hZ3 Gt_Mask
#define hY3 Lt_Mask
#define hX3 stackpos
#define hW3 empty()
#define hV3 qL3 2);
#define hU3 info.qM
#define hT3 info.qT
#define hS3 whydump
#define hR3 hF bool>
#define hQ3 ;dQ=r aH1
#define hP3 oN(),hF
#define hO3 CalculateGroupFunction
#define hN3 (param.
#define hM3 nparams
#define hL3 cTan,q2
#define hK3 cLog,q2
#define hJ3 cAtan2,
#define hI3 dD 0,
#define hH3 cAbs dD
#define hG3 dY1 2,
#define hF3 0x7},{{
#define hE3 cNeg,gL
#define hD3 dN;++a)
#define hC3 gD cAdd
#define hB3 gD a51
#define hA3 cPCall:
#define h93 for aU2
#define h83 g5 dK1(
#define h73 FindPos
#define h63 }oR hQ1
#define h53 }oR hP1
#define h43 hF dK1>
#define h33 ,double>
#define h23 ::res,b8<
#define h13 .end()
#define h03 );dE1 hS1
#define gZ3 half&=127
#define gY3 .what wA
#define gX3 ;for a81
#define gW3 gX3 a=
#define gV3 ].second
#define gU3 }static a82
#define gT3 MarkIncompletes(
#define gS3 true;}
#define gR3 mF1 2,
#define gQ3 fpdata
#define gP3 params
#define gO3 data->
#define gN3 ){h43
#define gM3 (count
#define gL3 133,2,
#define gK3 Needs
#define gJ3 gY 1,
#define gI3 byteCode
#define gH3 oM1){if(
#define gG3 ,-1.0)){
#define gF3 cLog2by)
#define gE3 ));pow m7
#define gD3 ;oY oG1}
#define gC3 cPow&&wP
#define gB3 result.
#define gA3 printf(
#define g93 factor=
#define g83 child)
#define g73 if(op==
#define g63 AddFrom(
#define g53 value1
#define g43 fp_floor
#define g33 hS1(ifp2
#define g23 =w32 gC
#define g13 opposite
#define g03 <<std::
#define qZ3 cAdd mS1
#define qY3 iterator
#define qX3 parent
#define qW3 SubTrees
#define qV3 SubTreesDetail
#define qU3 insert(i
#define qT3 newrel
#define qS3 if(w11==
#define qR3 [funcno].
#define qQ3 gE[++IP]
#define qP3 cMul);hA
#define qO3 );pow hS1(
#define qN3 gC 0));
#define qM3 opcodes
#define qL3 synth d2
#define qK3 did_muli
#define qJ3 .size()
#define qI3 (factor
#define qH3 qJ3;++
#define qG3 aC2 start_at,aX2
#define qF3 };enum
#define qE3 synth,
#define qD3 nparam
#define qC3 sizeof(
#define qB3 480 oF
#define qA3 227546,
#define q93 oR1 q1
#define q83 oR1 wH1
#define q73 445,{3,
#define q63 402,{3,
#define q53 398,{3,
#define q43 q0 2,
#define q33 oR1 q7
#define q23 |qD 0,
#define q13 DumpTree
#define q03 break;aQ
#define aZ2 oV.Immeds
#define aY2 ;}};struct
#define aX2 qR1 info
#define aW2 ):start_at()
#define aV2 switch(aS1.first){case
#define aU2 a81 a=0;a<
#define aT2 (gO3
#define aS2 bool g5
#define aR2 ].data);
#define aQ2 {if(apos
#define aP2 b_needed
#define aO2 cachepos
#define aN2 half=
#define aM2 131,4,1,
#define aL2 131,8,1,
#define aK2 src_pos
#define aJ2 reserve(
#define aI2 treeptr
#define aH2 ).dG case
#define aG2 .resize(
#define aF2 const hL
#define aE2 );range.aV1
#define aD2 1.0,1.0);
#define aC2 ,const o5&
#define aB2 2,1,4,1,2,
#define aA2 qV1-aD2
#define a92 const;void
#define a82 inline oA2
#define a72 }inline
#define a62 );void
#define a52 o6 a62
#define a42 (gE[IP]==
#define a32 const{if(
#define a22 FloatEqual(
#define a12 }case
#define a02 ;q9 qA
#define mZ2 ;MinMaxTree
#define mY2 false)
#define mX2 .hash1
#define mW2 if(list.first
#define mV2 IsEvenIntegerConst(
#define mU2 .begin(),
#define mT2 bool wB1
#define mS2 RefCount
#define mR2 q12 void
#define mQ2 Birth()
#define mP2 invtree
#define mO2 mulgroup
#define mN2 return
#define mM2 exponent
#define mL2 Others
#define mK2 middle
#define mJ2 oR1 q2
#define mI2 ifdata
#define mH2 sqrt_cost
#define mG2 mul_count
#define mF2 maxValue1
#define mE2 minValue1
#define mD2 maxValue0
#define mC2 minValue0
#define mB2 ValueType
#define mA2 w2 pow.qQ
#define m92 abs_mul
#define m82 pos_set
#define m72 MakeEqual
#define m62 newbase
#define m52 branch1op
#define m42 ==cOr oI1
#define m32 branch2op
#define m22 overlap
#define m12 truth_b
#define m02 truth_a
#define wZ2 found_dup
#define wY2 mM2,
#define wX2 Select1st
#define wW2 mE1 Eat(1,
#define wV2 mE1 h4 gD
#define wU2 mE1 Push(
#define wT2 condition
#define wS2 MatchPositionSpec_PositionalParams
#define wR2 {}};class
#define wQ2 rulenumit
#define wP2 cIf m2 o1
#define wO2 dZ1 gY 2,
#define wN2 cEqual,q7
#define wM2 ),Params(
#define wL2 integer
#define wK2 case cFCall:
#define wJ2 true;case
#define wI2 Plan_Has(
#define wH2 (qR,hY);q9
#define wG2 ;}void
#define wF2 +1)wG2
#define wE2 has_max
#define wD2 has_min
#define wC2 if hX 0)m9
#define wB2 GetParam(
#define wA2 inverted
#define w92 IsNever:
#define w82 .data.
#define w72 namespace FPoptimizer_CodeTree
#define w62 )result
#define w52 if(m gI)
#define w42 m.max
#define w32 iftree
#define w22 cond_add
#define w12 cond_mul
#define w02 cond_and
#define oZ2 cPow);qA
#define oY2 default_function_handling
#define oX2 mF1 1,
#define oW2 depcodes
#define oV2 *gP)[a].
#define oU2 explicit
#define oT2 costree
#define oS2 sintree
#define oR2 leaf_count
#define oQ2 sub_params
#define oP2 m9 cLog2&&
#define oO2 swap(tmp);
#define oN2 cbrt_count
#define oM2 sqrt_count
#define oL2 mM2);
#define oK2 PlusInf
#define oJ2 Finite
#define oI2 fp_ceil(o8
#define oH2 MakeNEqual
#define oG2 gB1 r;r m6
#define oF2 Dump(std::
#define oE2 negated
#define oD2 pcall_tree
#define oC2 after_powi
#define oB2 ,cAdd o9
#define oA2 unsigned
#define o92 .GetHash()
#define o82 {o5 start_at;
#define o72 break;result*=
#define o62 EvaluateFactorCost(
#define o52 ;break;}
#define o42 mY2;qS1
#define o32 gP3)
#define o22 cCos,q2
#define o12 cLog dD
#define o02 q43 6144,
#define dZ2 cInv,gL
#define dY2 constvalue
#define dX2 qJ2 size()
#define dW2 needs_flip
#define dV2 value]
#define dU2 ~size_t(0)
#define dT2 Rule&rule,
#define dS2 hO fphash_t
#define dR2 GetImmed()
#define dQ2 qS1 Find(
#define dP2 mul_item
#define dO2 innersub
#define dN2 cbrt_cost
#define dM2 best_cost
#define dL2 AddParam(
#define dK2 not_tree
#define dJ2 (leaf2 gC
#define dI2 group_by
#define dH2 );wH=!wH;}
#define dG2 mM2=
#define dF2 Decision
#define dE2 FixIncompletes(
#define dD2 continue;
#define dC2 ;dD2}
#define dB2 ->second.
#define dA2 BecomeOne
#define d92 TopLevel)
#define d82 per_item
#define d72 item_type
#define d62 first2
#define d52 oF 386426
#define d42 targetpos
#define d32 eat_count
#define d22 rhs.hash2;}
#define d12 rhs mX2
#define d02 Forget()
#define hZ2 source_tree
#define hY2 mN2 result;}
#define hX2 RootPowers[
#define hW2 hS1 hX 1));
#define hV2 p1_evenness
#define hU2 isNegative(
#define hT2 ;m.min=1.0;
#define hS2 2.0*o3);if(
#define hR2 double tmp=
#define hQ2 neg_set
#define hP2 (result
#define hO2 p1.w31 p1);
#define hN2 p1 hS1(ifp1
#define hM2 const_value
#define hL2 wB2 a)gV if(
#define hK2 wB2 2)
#define hJ2 hS1(cond gC
#define hI2 wB2 0)
#define hH2 cLog mE1 hV
#define hG2 :sim.hV
#define hF2 StackTopIs(
#define hE2 matches
#define hD2 info=(*gP)[
#define hC2 else{gP=new
#define hB2 grammar
#define hA2 );goto redo;
#define h92 (!tree oM1)hZ
#define h82 oK1 void*)&
#define h72 cSin,q2
#define h62 cLess,gY 2,
#define h52 cTan dD
#define h42 cCos dD
#define h32 },0,0x1},{{
#define h22 ;oA2
#define h12 :8 h22
#define h02 constraints
#define gZ2 best_score
#define gY2 mulvalue
#define gX2 pow_item
#define gW2 subgroup
#define gV2 PowiResult
#define gU2 maxValue
#define gT2 minValue
#define gS2 covers_plus1
#define gR2 gI=false
#define gQ2 if(hS)m.min=
#define gP2 div_tree
#define gO2 pow_tree
#define gN2 preserve
#define gM2 ;SetParam(
#define gL2 .GetOpcode
#define gK2 default:;}
#define gJ2 hM2=
#define gI2 factor_immed
#define gH2 base_immed
#define gG2 changes
#define gF2 is_logical
#define gE2 exp_diff
#define gD2 ExponentInfo
#define gC2 lower_bound(
#define gB2 (newrel_or==
#define gA2 newrel_and
#define g92 .PullResult(
#define g82 dup_or_fetch
#define g72 gE,size_t&a0
#define g62 gR wI1 89,a3
#define g52 ,o);o<<"\n";
#define g42 test_order
#define g32 =*oK1 h0 mG1
#define g22 .param_count
#define g12 shift(index)
#define g02 rulenumber
#define qZ2 AnyParams,q4
#define qY2 cAbsNotNot
#define qX2 cTanh dD
#define qW2 },0,dY1 1,
#define qV2 h02&
#define qU2 h02=
#define qT2 (tree gC a),
#define qS2 GetDepth()
#define qR2 res_stackpos
#define qQ2 half_pos
#define qP2 ),1.0/(
#define qO2 ;hN cInv);tmp.mC
#define qN2 CalculatePowiFactorCost(
#define qM2 ,mE,1,mY+1);
#define qL2 SetOpcode(
#define qK2 Params[
#define qJ2 Params.
#define qI2 );DelParam(
#define qH2 ,w5,synth);
#define qG2 .h02
#define qF2 ;if(half
#define qE2 hO OPCODE
#define qD2 }break;a12
#define qC2 std::cout<<
#define qB2 ;qC2
#define qA2 :mP1 comp.qF
#define q92 =hI2
#define q82 (*this,std::cout);
#define q72 :GetOpcode()==
#define q62 for a81 b=0;b<
#define q52 ;h93
#define q42 SelectedParams,0},0,
#define q32 ,0,0x0},{{
#define q22 2}q32
#define q12 ;inline
#define q02 const q12
#define aZ1 >>1)):(
#define aY1 changed_if
#define aX1 AddParamMove
#define aW1 mO2)
#define aV1 multiply(
#define aU1 MakeFalse
#define aT1 ,leaf1 gC
#define aS1 parampair
#define aR1 cOr,qI 2,
#define aQ1 second.second;
#define aP1 second.first;
#define aO1 log2_exponent
#define aN1 TreeCountType
#define aM1 covers_minus1
#define aL1 FloatEqual aX
#define aK1 AddItem(atree
#define aJ1 ConditionType
#define aI1 dup_fetch_pos
#define aH1 .specs;if(r.found){
#define aG1 *)&*start_at;
#define aF1 ,hT q4 508 oF
#define aE1 cSin dD
#define aD1 SubFunction:{
#define aC1 UseGetNeeded(
#define aB1 SpecialOpcode
#define aA1 ByteCodeSynth
#define a91 synth_it
#define a81 (size_t
#define a71 a81 a=hQ
#define a61 IsDefined()
#define a51 cMul);qA
#define a41 FactorStack
#define a31 .match_tree
#define a21 changed=
#define a11 )qB2 std::endl;
#define a01 DUP_BOTH();
#define mZ1 StackTop
#define mY1 found_log2
#define mX1 div_params
#define mW1 parent_opcode)
#define mV1 TreeCounts
#define mU1 immed_sum
#define mT1 new_base_immed
#define mS1 ||op1==
#define mR1 (leaf1 gC
#define mQ1 remaining
#define mP1 change=
#define mO1 if_always[
#define mN1 WhatDoWhenCase
#define mM1 AddCollection(
#define mL1 OPCODE(opcode)
#define mK1 var_trees
#define mJ1 qS1 DoDup(
#define mI1 ,tree g0
#define mH1 ,{1,211,
#define mG1 *)aS1.second;
#define mF1 sim.Eat(
#define mE1 );sim.
#define mD1 ;stack aG2
#define mC1 240849 oF
#define mB1 457937 oF
#define mA1 cAnd,qI 2,
#define m91 cGreater,gY 2,
#define m81 0x12}dL3
#define m71 DumpHashesFrom
#define m61 replacing_slot
#define m51 DUP_ONE(apos);
#define m41 flipped
#define m31 oO 2,131,
#define m21 Grammar
#define m11 OptimizedUsing
#define m01 fphash_value_t
#define wZ1 crc32_t
#define wY1 signed_chain
#define wX1 PowiResolver
#define wW1 MinusInf
#define wV1 ;else result
#define wU1 n_immeds
#define wT1 p1_logical_b
#define wS1 p0_logical_b
#define wR1 p1_logical_a
#define wQ1 p0_logical_a
#define wP1 gQ[c].wJ
#define wO1 stack qJ3
#define wN1 FindClone(hA
#define wM1 needs_rehash
#define wL1 AnyWhere_Rec
#define wK1 minimum_need
#define wJ1 ~oA2(0)
#define wI1 47,48,49,50,
#define wH1 q1 166912 oF
#define wG1 else if(
#define wF1 (half&63)-1;
#define wE1 cache_needed
#define wD1 oO 2,1,oO 2,
#define wC1 treelist
#define wB1 operator
#define wA1 IsDescendantOf(
#define w91 best_sep_factor
#define w81 has_bad_balance
#define w71 {case IsAlways:
#define w61 ;qL2 cMul)
#define w51 .DelParam(
#define w41 {dA3 goto mV dB3
#define w31 Rehash()w2
#define w21 w31 r);}
#define w11 reltype
#define w01 0.5 gD oZ2
#define oZ1 .SetParamsMove(
#define oY1 (m.min
#define oX1 assimilated
#define oW1 q92 gV MinMaxTree
#define oV1 o1 false,2,
#define oU1 GetParams()
#define oT1 best_sep_cost
#define oS1 TreeCountItem
#define oR1 ,cPow,
#define oQ1 ;wG1!result
#define oP1 ):mS2(0),Opcode(
#define oO1 a1&&p0.min>=0.0)
#define oN1 .sep_list[n]
#define oM1 .IsImmed()
#define oL1 const std::w4
#define oK1 (const
#define oJ1 oK1 ParamSpec&aS1,
#define oI1 )?0.0:1.0);q8
#define oH1 ConstantFolding_Assimilate();
#define oG1 DelParam(a);}
#define oF1 oG1 if(
#define oE1 hB1 GetImmed(
#define oD1 new_factor_immed
#define oC1 for a81 a=d5.
#define oB1 occurance_pos
#define oA1 exponent_hash
#define o91 exponent_list
#define o81 CollectionSet
#define o71 CollectMulGroup(
#define o61 source_set
#define o51 stack[wO1-
#define o41 stack.push_back(
#define o31 MaxChildDepth
#define o21 repl_param_list,
#define o11 std::pair<It,It>
#define o01 cNot dD
#define dZ1 cLessOrEq,
#define dY1 0x4},{{
#define dX1 Value_Logical
#define dW1 Value_EvenInt
#define dV1 case ParamHolder:{
#define dU1 (cond.dH&&cond.wO))
#define dT1 :qS1 PushImmed(
#define dS1 case VarBegin:
#define dR1 ,PowiCache&mE,
#define dQ1 ;mN2
#define dP1 dQ1 result;}
#define dO1 void g5
#define dN1 AddOperation(
#define dM1 );gE[mI2.ofs+
#define dL1 produce_count
#define dK1 CodeTree
#define dJ1 FindIntegerFactor
#define dI1 CONSTANT_PIHALF);
#define dH1 -CONSTANT_PIHALF,
#define dG1 dK1 p2;p2 m6
#define dF1 by_float_exponent
#define dE1 new_exp
#define dD1 ;aV.hash2+=(
#define dC1 end()&&i->first==
#define dB1 back().thenbranch
#define dA1 retry_anyparams_3
#define d91 retry_anyparams_2
#define d81 needlist_cached_t
#define d71 grammar_rules[*r]
#define d61 mN2 ParamSpec(
#define d51 cPow,q7 0x1}dL3
#define d41 if(qM qJ3<=qN)
#define d31 hF double>&immed,
#define d21 oU2 dK1
#define d11 PositionalParams,
#define d01 ;mM2
#define hZ1 addgroup
#define hY1 ))goto redo;
#define hX1 qX hI2
#define hW1 qX dK1(
#define hV1 :Value(Value::
#define hU1 hB1 IsImmed())
#define hT1 found_log2by
#define hS1 .AddParam
#define hR1 ;q13(*this hC1
#define hQ1 MakeNotP1,q5::
#define hP1 MakeNotP0,q5::
#define hO1 MakeTrue
#define hN1 branch1_backup
#define hM1 branch2_backup
#define hL1 ;SetParamMove(
#define hK1 else_tree
#define hJ1 exponent_map
#define hI1 plain_set
#define hH1 ParsePowiMuli(
#define hG1 ,gE,IP,limit,hP,stack);
#define hF1 .dR2
#define hE1 mO2.
#define hD1 .aX1(
#define hC1 )qB2"\n";
#define hB1 wB2 a).
#define hA1 g9 g22
#define h91 Oneness_NotOne
#define h81 LightWeight(
#define h71 ,mE qH2
#define h61 if(value
#define h51 ContainsOtherCandidates(
#define h41 oS1&occ=
#define h31 wB2 1)
#define h21 qL2 leaf1.mB
#define h11 should_regenerate=true;
#define h01 should_regenerate,
#define gZ1 Collection
#define gY1 mN2 BecomeZero;
#define gX1 RelationshipResult
#define gW1 ParamSpec_Extract(
#define gV1 cGreaterOrEq,gY 2,
#define gU1 Subdivide_Combine(
#define gT1 long value
#define gS1 oA2(Immed qJ3);
#define gR1 oA2(gE qJ3
#define gQ1 needlist_cached
#define gP1 Value_IsInteger
#define gO1 m11(
#define gN1 opcode,bool pad
#define gM1 GetParamCount()
#define gL1 MakesInteger(
#define gK1 aX1(tree);
#define gJ1 n_occurrences
#define gI1 MultiplicationRange
#define gH1 n_stacked
#define gG1 m01(crc)
#define gF1 AnyParams_Rec
#define gE1 dM gO3
#define gD1 needs_sincos
#define gC1 changed_exponent
#define gB1 ){dK1
#define gA1 if(GetOpcode()==
#define g91 aX1 hX 0));
#define g81 PositionType
#define g71 CollectionResult
#define g61 mN2 mL;
#define g51 const_offset
#define g41 IsIntegerConst(
#define g31 mL1);
#define g21 SynthesizeParam(
#define g11 grammar_func
#define g01 214463 oF 24793,
#define qZ1 cNotNot dD
#define qY1 Modulo_Radians},
#define qX1 IsLogicalValue()
#define qW1 stacktop_desired
#define qV1 mN2 dK
#define qU1 (w42);}mN2 m;a12
#define qT1 qS1 gA 1
#define qS1 synth.
#define qR1 MatchInfo&
#define qQ1 CodeTreeData::
#define qP1 SequenceOpCode
#define qO1 .Become(value qN3
#define qN1 CollectMulGroup_Item(
#define qM1 a;if(&*start_at){gP=(
#define qL1 DataP slot_holder(gX[
#define qK1 aA1&synth){
#define qJ1 .o2,GetOpcode()==
#define qI1 MakeNotNotP1,q5::
#define qH1 MakeNotNotP0,q5::
#define qG1 >::Optimize(){}
#define qF1 IsImmed())dN3
#define qE1 std::pair<double,h43>
#define qD1 std::pair<T1,T2>&
#define qC1 template<typename
#define qB1 AssembleSequence(
#define qA1 cAbsOr,AnyParams,
#define q91 214251 oF 227545,
#define q81 found_log2_on_exponent
#define q71 has_good_balance_found
#define q61 needs_resynth
#define q51 g41 result)
#define q41 break;}switch(bitmask&
#define q31 Constness_Const,0x1},{
#define q21 hF oA2>&gI3,
#define q11 divgroup
#define q01 bool wH=false;dK1&
#define aZ immed_product
#define aY GetLogicalValue hX
#define aX (mM2
#define aW cond_type
#define aV NewHash
#define aU cEqual,q1
#define aT =IfBalanceGood(root gC
#define aS cIf,q0 3,
#define aR Sign_Positive
#define aQ default:break;}
#define aP RefParams
#define aO MatchResultType
#define aN const h43
#define aM :g13=
#define aL ;if(d92 info.SaveMatchedParamIndex(
#define aK (oA2
#define aJ for aK a=0 oB
#define aI tree.IsAlwaysSigned(
#define aH const dK1&
#define aG ,1.0/(3*3*
#define aF goto ReplaceTreeWithOne;
#define aE retry_positionalparams_2
#define aD cNEqual,q1
#define aC cIf,q2
#define aB ,qG 0x12},{{
#define aA .IsAlwaysInteger(wL2)
#define a9 {h43&Params=gX;
#define a8 first=true;qM[qN gV3
#define a7 collections
#define a6 FPoptimizer_Grammar
#define a5 6144 oF 266498
#define a4 &1)?(poly^(
#define a3 122,123,124,125,
#define a2 PlanNtimesCache(
#define a1 .wD2
#define a0 IP,size_t limit,size_t hP
#define mZ oA2 index
#define mY recursioncount
#define mX ParamSpec_SubFunctionData
#define mW inline dO1
#define mV ReplaceTreeWithZero;
#define mU case ComparisonSet::
#define mT PositionalParams_Rec
#define mS cAbsAnd,qZ2
#define mR gE.push_back(
#define mQ [mZ1-1-offset].
#define mP switch(type){case cond_or:
#define mO MinMaxTree&p,bool abs){if(
#define mN DumpTreeWithIndent(*this);
#define mM Opcode){case cImmed:mN2
#define mL Suboptimal
#define mK edited_powgroup
#define mJ has_unknown_max
#define mI has_unknown_min
#define mH synthed_tree
#define mG mP2.qL gA1
#define mF fp_const_pi<double>()
#define mE cache
#define mD matched_params
#define mC aX1(tmp2
#define mB GetOpcode());
#define mA }oR hK Never}oR hK Never}}
#define m9 gL2()==
#define m8 MatchPositionSpec_AnyParams
#define m7 .Rehash();
#define m6 .qL2
#define m5 mO2;mO2 m6
#define m4 if(tmp!=0.0 h3 1.0/tmp;q9}qA
#define m3 {oX2 cInv);g1 oZ2
#define m2 ,d11
#define m1 pow;pow m6 cPow);pow.
#define m0 dF1.data
#define wZ :data(new CodeTreeData
#define wY n_as_sin_param
#define wX n_as_cos_param
#define wW aU1,q5::
#define wV StackState
#define wU gI&&p0.max<=NEGATIVE_MAXIMUM)
#define wT static const int
#define wS 458752 oF 27905,
#define wR 349494 oF 24837,
#define wQ 456814 oF 24697,
#define wP powgroup gC
#define wO BalanceGood
#define wN has_mulgroups_remaining
#define wM by_exponent
#define wL cGreater,q1
#define wK Rehash();oQ2.push_back(
#define wJ relationship
#define wI namespace FPoptimizer_ByteCode
#define wH is_signed
#define wG oU1);mO2 m7
#define wF result_positivity
#define wE fp_sin(min),fp_sin(max))
#define wD biggest_minimum
#define wC cond_tree
#define wB then_tree
#define wA !=Unchanged)if(TestCase(
#define w9 319799 oF 269574,
#define w8 113774 oF 120949,
#define w7 127086 oF 126073,
#define w6 cAdd,AnyParams,
#define w5 sequencing
#define w4 string FP_GetOpcodeName(
#define w3 gC a).IsIdenticalTo(
#define w2 ;aX1(
#define w1 );qA ComparisonSet::
#define w0 if_stack
#define oZ gW1 g9.param_list,
#define oY CopyOnWrite();
#define oX extern const
#define oW oY1);w52 w42=
#define oV NeedList
#define oU ];};extern"C"{
#define oT aX1(gW2
#define oS valueType
#define oR ,{q5::
#define oQ aX1(mul);
#define oP back().endif_location
#define oO 130,1,
#define oN MatchPositionSpecBase
#define oM smallest_maximum
#define oL )w2 aY1)oG}
#define oK ;p2.w31 p2);qL2 w32.mB goto redo;}
#define oJ tree gL2()
#define oI ++IP dC2 if a42 qM3.
#define oH FPoptimizer_ByteCode::
#define oG dQ1 true;
#define oF ,{2,
#define oE aH tree,std::ostream&o
#define oD ReplaceTreeWithParam0;
#define oC factor_needs_rehashing
#define oB ;a<hA1;++a)
#define oA dC2 mW2 hF1==
#define o9 ,GroupFunction,0},qD{2,
#define o8 w42)dQ1 m;a12
#define o7 (hS){if oY1<0.0)hS=false;else m.min=
#define o6 h43&aP
#define o5 MatchPositionSpecBaseP
#define o4 best_factor
#define o3 CONSTANT_PI
#define o2 CalculateResultBoundaries()
#define o1 0}},{ReplaceParams,
#define o0 }},{ProduceNewTree,false,1,
#define dZ qH q22
#define dY otherhalf
#define dX best_selected_sep
#define dW :{g8 cPow hX1 hW1
#define dV fp_cosh oY1);w42=fp_cosh(w42);
#define dU FPoptimizer_CodeTree::dK1&tree,
#define dT if(keep_powi)
#define dS if(ParamComparer()(Params g4
#define dR public oN,public hF
#define dQ (oV2 start_at
#define dP w82 subfunc_opcode
#define dO hL1 0,mM2 qI2 1);
#define dN tree.gM1
#define dM )const{mN2
#define dL const fphash_t&rhs dM hash1
#define dK MinMaxTree(
#define dJ branch2
#define dI !=Unchanged)mN2 mO1
#define dH FoundChild
#define dG dR2))hA2
#define dF aX1(comp.hI1[a].value);
#define dE T1,typename T2>inline mT2()(
#define dD ,q0 1,
#define dC q3 2,
#define dB ));hK1 hS1
#define dA from_logical_context)
#define d9 dZ1 q1
#define d8 ParamSpec_NumConstant
#define d7 POWI_CACHE_SIZE
#define d6 BalanceResultType
#define d5 branch1
#define d4 const mX
#define d3 h93 gM1;++a)
#define d2 .dN1 GetOpcode(),
#define d1 std::map<fphash_t,std::set<std::string> >
#define d0 MatchPositionSpec_AnyWhere
#define hZ mN2 false;
#define hY h31 hF1
#define hX (wB2
#define hW if(param w82 match_type==
#define hV AddConst(
#define hU AssembleSequence_Subdivide(
#define hT cOr,AnyParams,
#define hS m a1
#define hR w52{if(w42<0.0)m gR2;else w42=
#define hQ 0;a<qX3.gM1;++a)if(
#define hP factor_stack_base
#define hO FUNCTIONPARSERTYPES::
#define hN tmp2 hS1 hX 0));tmp m6
#define hM :{dK1 tmp,tmp2;tmp2 m6
#define hL a6::m21*
#define hK Unchanged,q5::
#define hJ cAnd,AnyParams,
#define hI cGreaterOrEq,q1
#define hH IsIdenticalTo dJ2
#define hG d3 dD3
#define hF std::vector<
#define hE while(ApplyGrammar(C
#define hD gM1;a-->0;)if(
#define hC for(wQ2 r=range.first;r!=range.second;++r){
#define hB {mV1.erase(i)dC2
#define hA newnode
#define h9 PACKED_GRAMMAR_ATTRIBUTE;struct
#define h8 FPOPT_autoptr
#define h7 abs_int_exponent
#define h6 mN2 wJ2 q5::
#define h5 has_highlevel_opcodes
#define h4 SwapLastTwoInStack(
#define h3 ){gJ2
#define h2 oK1 FPoptimizer_CodeTree::dK1&tree
#define h1 cMul o9
#define h0 ParamSpec_SubFunction
#define gZ ParamSpec_ParamHolder
#define gY q7 dY1
#define gX gO3 Params
#define gW SetStackTop(mZ1
#define gV .o2;
#define gU ,w6 q4
#define gT oA2 c h22 char l[
#define gS 160,161,162,172,183,195,233,234,235,236,237,238,239,240,241,244,245,246,247,248,249,250,251,254,255,0,1,2,3,4,5,6,7,8,11,12}};}struct
#define gR 27,28,29,30,31,32,33,34,35,36,37,38,39,40,
#define gQ relationships
#define gP position
#define gO TestImmedConstraints(param qG2,tree))hZ
#define gN paramholder_index
#define gM occurance_counts
#define gL GroupFunction,0},qD{1,
#define gK dU std::ostream&o=std::cout
#define gJ p=hL2 p
#define gI .wE2
#define gH qJ&&h31 oM1 h3
#define gG ()==cAbsNot){SetParam(0,w32 qN3 dK1 p1;p1 m6
#define gF qH 1}q32
#define gE ByteCode
#define gD mE1 Eat(2,
#define gC .wB2
#define gB template<>void FunctionParserBase<
#define gA GetStackTop()-
#define g9 model_tree
#define g8 dK1 tmp;tmp m6
#define g7 {mN2 ConstantFolding_LogicCommon(ComparisonSet::
#define g6 <hD3 if(ApplyGrammar(hB2,tree gC a),
#define g5 dK1::
#define g4 [1],qK2 0])){std::swap(qK2 0],qK2 1]);Opcode=
#define g3 qC1 Ref>inline void h8<Ref>::
#define g2 mP2;mP2.SetParams(oU1);mP2 m6
#define g1 break;}sim.hV-1 gD
#define g0 )){tree.FixIncompleteHashes();}
#define qZ DumpParams(param w82 param_list,param.data g22,o);
#define qY a81 a=gM1;a-->0;){aH powgroup=wB2 a);if(powgroup.
#define qX );tmp hS1(
#define qW SynthesizeByteCode(synth);
#define qV using namespace FUNCTIONPARSERTYPES;
#define qU dK1 mM2 d01 m6 cMul)d01 hS1(
#define qT paramholder_matches
#define qS if hX 0).IsAlwaysInteger(true))o83
#define qR hI2 hF1
#define qQ h31);pow w51 1);pow m7 SetParamMove(0,pow);goto NowWeAreMulGroup;}
#define qP cPow,gY 2,
#define qO w6 1}q32 1,
#define qN restholder_index
#define qM restholder_matches
#define qL Rehash(mY2;if(qS1 FindAndDup(mP2)){qS1 dN1 cInv,1,1)dQ1;}}
#define qK SetParam(0,hI2 gC 0))gM2 1,dK1(
#define qJ if hX 0)oM1
#define qI SelectedParams,0}q32
#define qH cMul,AnyParams,
#define qG cMul,q42
#define qF AddRelationship(atree gC 0),atree gC 1),ComparisonSet::
#define qE )dQ1 tmp gV a12
#define qD Constness_Const,0x0},{
#define qC cPow,q43
#define qB :dD3 m q92 gV
#define qA break;case
#define q9 goto ReplaceTreeWithConstValue;}
#define q8 dK1 aY1;aY1 m6 mB aY1 oZ1 oU1);aY1 m7 qL2
#define q7 d11 0},0,
#define q6 cAdd,qI 2,
#define q5 RangeComparisonData
#define q4 o1 false,1,
#define q3 qG 0x0},{{
#define q2 d11 0 o0
#define q1 d11 oV1
#define q0 q7 0x0},{{
#ifdef _MSC_VER
typedef
oA2
int
wZ1;
#else
#include <stdint.h>
typedef
uint_least32_t
wZ1;
#endif
namespace
crc32{enum{startvalue=0xFFFFFFFFUL,poly=0xEDB88320UL}
;template<wZ1
crc>struct
b8{enum{b1=(crc
a4
crc
aZ1
crc>>1),b2=(b1
a4
b1
aZ1
b1>>1),b3=(b2
a4
b2
aZ1
b2>>1),b4=(b3
a4
b3
aZ1
b3>>1),b5=(b4
a4
b4
aZ1
b4>>1),b6=(b5
a4
b5
aZ1
b5>>1),b7=(b6
a4
b6
aZ1
b6>>1),res=(b7
a4
b7
aZ1
b7>>1)}
;}
q12
wZ1
update(wZ1
crc,oA2
b){
#define B4(n) b8<n>h23 n+1>h23 n+2>h23 n+3>::res
#define R(n) B4(n),B4(n+4),B4(n+8),B4(n+12)
static
const
wZ1
table[256]={R(0x00),R(0x10),R(0x20),R(0x30),R(0x40),R(0x50),R(0x60),R(0x70),R(0x80),R(0x90),R(0xA0),R(0xB0),R(0xC0),R(0xD0),R(0xE0),R(0xF0)}
;
#undef R
#undef B4
mN2((crc>>8))^table[(crc^b)&0xFF];a72
wZ1
calc_upd(wZ1
c,const
oA2
char*buf,size_t
size){wZ1
value=c
gX3
p=0;p<size;++p)value=update(value,buf[p])dQ1
value;a72
wZ1
calc
oK1
oA2
char*buf,size_t
size){mN2
calc_upd(startvalue,buf,size);}
}
#ifndef FPOptimizerAutoPtrHH
#define FPOptimizerAutoPtrHH
qC1
Ref>class
h8{public:h8():p(0){}
h8(Ref*b):p(b){mQ2;}
h8
oK1
h8&b):p(b.p){mQ2;a72
Ref&wB1*(dM*p;a72
Ref*wB1->(dM
p;}
h8&wB1=(Ref*b){Set(b)dQ1*this;}
h8&wB1=oK1
h8&b){Set(b.p)dQ1*this;}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
h8(h8&&b):p(b.p){b.p=0;}
h8&wB1=(h8&&b){if(p!=b.p){d02;p=b.p;b.p=0;}
mN2*this;}
#endif
~h8(){d02
wG2
UnsafeSetP(Ref*newp){p=newp
wG2
swap(h8<Ref>&b){Ref*tmp=p;p=b.p;b.p=tmp;}
private:inline
static
void
Have(Ref*p2)mR2
d02
mR2
mQ2
mR2
Set(Ref*p2);private:Ref*p;}
;g3
d02{if(!p)mN2;p->mS2-=1;if(!p->mS2)delete
p;}
g3
Have(Ref*p2){if(p2)++(p2->mS2);}
g3
mQ2{Have(p);}
g3
Set(Ref*p2){Have(p2);d02;p=p2;}
#endif
#ifndef FPoptimizerHashHH
#define FPoptimizerHashHH
#ifdef _MSC_VER
typedef
oA2
long
long
m01;
#define FPHASH_CONST(x) x##ULL
#else
#include <stdint.h>
typedef
uint_fast64_t
m01;
#define FPHASH_CONST(x) x##ULL
#endif
namespace
FUNCTIONPARSERTYPES{struct
fphash_t{m01
hash1,hash2;mT2==(dL==d12&&hash2==d22
mT2!=(dL!=d12||hash2!=d22
mT2<(dL!=d12?hash1<d12:hash2<d22}
;}
#endif
#ifndef FPOptimizer_CodeTreeHH
#define FPOptimizer_CodeTreeHH
#ifdef FP_SUPPORT_OPTIMIZER
#include <vector>
#include <utility>
#ifdef FP_EPSILON
#define NEGATIVE_MAXIMUM (-FP_EPSILON)
#else
#define NEGATIVE_MAXIMUM (-1e-14)
#endif
namespace
a6{struct
m21;}
wI{class
aA1;}
w72{class
dK1;struct
MinMaxTree
dN3
min,max;bool
wD2,wE2;dK):min(),max(),wD2(mY2,wE2(mY2{}
dK
double
mi,double
ma):min(mi),max(ma),wD2(true),wE2(true){}
dK
bool,double
ma):min(),max(ma),wD2(mY2,wE2(true){}
dK
double
mi,bool):min(mi),max(),wD2(true),wE2(mY2{}
}
;struct
CodeTreeData;class
dK1{typedef
h8<CodeTreeData>DataP;DataP
data;public:public:dK1();~dK1();d21(double
v);struct
VarTag{}
;d21
aK
varno,VarTag);struct
CloneTag{}
;d21(aH
b,CloneTag
a62
GenerateFrom
oK1
q21
const
d31
const
FunctionParser::Data&data,bool
keep_powi=mY2;void
GenerateFrom
oK1
q21
const
d31
const
FunctionParser::Data&data,aN&mK1,bool
keep_powi=mY2;void
SynthesizeByteCode(q21
d31
size_t&stacktop_max
a62
SynthesizeByteCode(oH
aA1&qE3
bool
MustPopTemps=true)const;size_t
SynthCommonSubExpressions(oH
aA1&synth)a92
SetParams
oK1
a52
SetParamsMove(o6);dK1
GetUniqueRef();
#ifdef __GXX_EXPERIMENTAL_CXX0X__
void
SetParams(h43&&aP);
#endif
void
SetParam
a81
which,aH
b
a62
SetParamMove
a81
which,dK1&b
a62
dL2
aH
param
a62
aX1(dK1&param
a62
AddParams
oK1
a52
AddParamsMove(a52
AddParamsMove(o6,size_t
m61
a62
DelParam
a81
index
a62
DelParams(a62
Become(aH
b)q12
size_t
GetParamCount(dM
oU1
qJ3;a72
dK1&GetParam
a81
n){mN2
oU1[n];a72
aH
GetParam
a81
n
dM
oU1[n];a72
void
qL2
qE2
o)mR2
SetFuncOpcode(qE2
o,oA2
f)mR2
SetVar
aK
v)mR2
SetImmed(double
v)q12
qE2
GetOpcode()q02
dS2
GetHash()q02
aN&oU1
q02
h43&oU1
q12
size_t
qS2
q02
const
double&dR2
q02
oA2
GetVar()q02
oA2
GetFuncNo()q02
bool
IsDefined(dM
GetOpcode()!=hO
cNop;a72
bool
IsImmed(dM
GetOpcode()==hO
cImmed;a72
bool
IsVar(dM
GetOpcode()==hO
VarBegin;}
bool
IsLongIntegerImmed(dM
IsImmed()&&dR2==(double)GetLongIntegerImmed();}
long
GetLongIntegerImmed(dM(long)dR2;}
bool
qX1
q02
oA2
GetRefCount()const
mZ2
CalculateResultBoundaries_do()const
mZ2
o2
const;enum
TriTruthValue{IsAlways,IsNever,Unknown}
;TriTruthValue
GetEvennessInfo()const;bool
IsAlwaysSigned(bool
positive)const;bool
IsAlwaysParity(bool
odd
dM
GetEvennessInfo()==(odd?w92
IsAlways);}
bool
IsAlwaysInteger(bool
wL2)a92
ConstantFolding();bool
ConstantFolding_AndLogic();bool
ConstantFolding_OrLogic();bool
ConstantFolding_MulLogicItems();bool
ConstantFolding_AddLogicItems();bool
ConstantFolding_IfOperations();bool
ConstantFolding_PowOperations();bool
ConstantFolding_ComparisonOperations();qC1
CondType>bool
ConstantFolding_LogicCommon(CondType
aW,bool
gF2);bool
ConstantFolding_MulGrouping();bool
ConstantFolding_AddGrouping();bool
oH1
void
Rehash(bool
constantfolding=true)mR2
Mark_Incompletely_Hashed()q12
bool
Is_Incompletely_Hashed()q02
aF2
GetOptimizedUsing()q02
void
SetOptimizedUsing
oK1
hL
g);bool
RecreateInversionsAndNegations(bool
prefer_base2=mY2;void
FixIncompleteHashes(a62
swap(dK1&b){data.swap(b.data);}
bool
IsIdenticalTo(aH
b)a92
oY}
;struct
CodeTreeData{int
mS2;qE2
Opcode;union
dN3
Value
h22
Var
h22
Funcno;}
;h43
Params;dS2
Hash;size_t
Depth;aF2
m11;CodeTreeData();CodeTreeData
oK1
CodeTreeData&b);oU2
CodeTreeData(double
i);
#ifdef __GXX_EXPERIMENTAL_CXX0X__
CodeTreeData(CodeTreeData&&b);
#endif
bool
IsIdenticalTo
oK1
CodeTreeData&b)a92
Sort(a62
Recalculate_Hash_NoRecursion();}
;mW
qL2
qE2
o){gO3
Opcode=o;}
mW
SetFuncOpcode(qE2
o,oA2
f){qL2
o);gO3
Funcno=f;}
mW
SetVar
aK
v){qL2
hO
VarBegin);gO3
Var=v;}
mW
SetImmed(double
v){qL2
hO
cImmed);gO3
Value=v;a72
qE2
g5
GetOpcode(gE1
Opcode;a72
dS2
g5
GetHash(gE1
Hash;a72
aN&g5
GetParams(dM
gX;a72
h43&g5
oU1{mN2
gX;a72
size_t
g5
GetDepth(gE1
Depth;a72
const
double&g5
GetImmed(gE1
Value;a72
oA2
g5
GetVar(gE1
Var;a72
oA2
g5
GetFuncNo(gE1
Funcno;a72
aF2
g5
GetOptimizedUsing(gE1
m11;}
mW
SetOptimizedUsing
oK1
hL
g){gO3
m11=g;a72
oA2
g5
GetRefCount(gE1
mS2;}
mW
Mark_Incompletely_Hashed(){gO3
Depth=0;a72
aS2
Is_Incompletely_Hashed(gE1
Depth==0;}
#ifdef FUNCTIONPARSER_SUPPORT_DEBUG_OUTPUT
void
DumpHashes
oK1
gK
a62
q13
oK1
gK
a62
DumpTreeWithIndent
oK1
gK,const
std::string&indent="\\"
);
#endif
}
#endif
#endif
#ifndef FPOPT_NAN_CONST
#include <iostream>
#define FPOPT_NAN_CONST (-1712345.25)
w72{class
dK1;}
namespace
a6{enum
ImmedConstraint_Value{ValueMask=0x07,Value_AnyNum=0x0,dW1=0x1,Value_OddInt=0x2,gP1=0x3,Value_NonInteger=0x4,dX1=0x5
qF3
ImmedConstraint_Sign{SignMask=0x18,Sign_AnySign=0x00,aR=0x08,Sign_Negative=0x10,Sign_NoIdea=0x18
qF3
ImmedConstraint_Oneness{OnenessMask=0x60,Oneness_Any=0x00,Oneness_One=0x20,h91=0x40
qF3
ImmedConstraint_Constness{ConstnessMask=0x80,Constness_Any=0x00,Constness_Const=0x80
qF3
Modulo_Mode{Modulo_None=0,Modulo_Radians=1
qF3
aB1{NumConstant,ParamHolder,SubFunction
qF3
ParamMatchingType{d11
SelectedParams,AnyParams,GroupFunction
qF3
RuleType{ProduceNewTree,ReplaceParams}
;
#ifdef __GNUC__
# define PACKED_GRAMMAR_ATTRIBUTE __attribute__((packed))
#else
# define PACKED_GRAMMAR_ATTRIBUTE
#endif
typedef
std::pair<aB1,const
void*>ParamSpec;ParamSpec
ParamSpec_Extract
aK
paramlist,mZ);bool
ParamSpec_Compare
oK1
void*d23
void*b,aB1
type)h22
ParamSpec_GetDepCode
oK1
ParamSpec&b);struct
gZ{mZ
h12
h02
h12
depcode:16;}
h9
d8
dN3
dY2
h22
modulo;}
h9
mX{oA2
param_count:2
h22
param_list:30;qE2
subfunc_opcode:8;ParamMatchingType
match_type:3
h22
qN:5;}
h9
h0{mX
data
h22
h02
h12
depcode:8;}
h9
Rule{RuleType
ruletype:2;bool
logical_context:1
h22
repl_param_count:2+13
h22
repl_param_list:30;mX
match_tree;}
h9
m21{oA2
rule_count
h22
char
rule_list[999
oU
oX
Rule
grammar_rules[];
#ifndef FPOPTIMIZER_MERGED_FILE
oX
m21
grammar_optimize_round1;oX
m21
grammar_optimize_round2;oX
m21
grammar_optimize_round3;oX
m21
grammar_optimize_round4;oX
m21
grammar_optimize_shortcut_logical_evaluation;oX
m21
grammar_optimize_nonshortcut_logical_evaluation;oX
m21
grammar_optimize_ignore_if_sideeffects;oX
m21
grammar_optimize_abslogical;oX
m21
grammar_optimize_base2_expand;
#endif
}
void
DumpParam
oK1
ParamSpec&p,std::ostream&o=std::cout
a62
DumpParams
aK
paramlist,oA2
count,std::ostream&o=std::cout);}
#endif
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
#define CONSTANT_E 2.71828182845904523536
#define CONSTANT_EI 0.3678794411714423215955
#define CONSTANT_2E 7.3890560989306502272304
#define CONSTANT_2EI 0.135335283236612691894
#define CONSTANT_PI M_PI
#define CONSTANT_L10 2.30258509299404590109
#define CONSTANT_L2 0.69314718055994530943
#define CONSTANT_L10I 0.43429448190325176116
#define CONSTANT_L2I 1.4426950408889634074
#define CONSTANT_L10E CONSTANT_L10I
#define CONSTANT_L10EI CONSTANT_L10
#define CONSTANT_L10B 0.3010299956639811952137
#define CONSTANT_L10BI 3.3219280948873623478703
#define CONSTANT_LB10 CONSTANT_L10BI
#define CONSTANT_LB10I CONSTANT_L10B
#define CONSTANT_L2E CONSTANT_L2I
#define CONSTANT_L2EI CONSTANT_L2
#define CONSTANT_DR (180.0/M_PI)
#define CONSTANT_RD (M_PI/180.0)
#define CONSTANT_POS_INF HUGE_VAL
#define CONSTANT_NEG_INF (-HUGE_VAL)
#define CONSTANT_PIHALF (M_PI/2)
#define CONSTANT_TWOPI (M_PI+M_PI)
#ifdef FP_SUPPORT_OPTIMIZER
#include <vector>
#include <utility>
#include <iostream>
namespace
FPoptimizer_Optimize{using
namespace
a6;using
w72;qV
class
MatchInfo{public:hF
std::pair<bool,h43> >qM;h43
qT;hF
oA2>mD;public:bool
SaveOrTestRestHolder
aK
qN,aN&wC1){d41{qM
aG2
qN+1);qM[qN].a8=wC1
oG}
if(qM[qN
d43==mY2{qM[qN].a8=wC1
oG}
aN&found=qM[qN
gV3;if(wC1
qJ3!=found
qJ3)hZ
h93
wC1
qH3
a)if(!wC1[a].IsIdenticalTo(found[a]))hZ
mN2
true
wG2
SaveRestHolder
aK
qN,h43&wC1){d41
qM
aG2
qN+1);qM[qN].a8.swap(wC1);}
bool
SaveOrTestParamHolder
aK
gN,aH
aI2){if(qT
qJ3<=gN){qT.aJ2
gN+1);qT
aG2
gN);qT.push_back(aI2)oG}
if(!qT[gN].a61){qT[gN]=aI2
oG}
mN2
aI2.IsIdenticalTo(qT[gN])wG2
SaveMatchedParamIndex(mZ){mD.push_back(index);}
aH
GetParamHolderValueIfFound
aK
gN)const{static
const
dK1
dummytree;if(qT
qJ3<=gN)mN2
dummytree
dQ1
qT[gN];}
aH
GetParamHolderValue
aK
gN
dM
qT[gN];}
bool
HasRestHolder
aK
qN
dM
qM
qJ3>qN&&qM[qN
d43==gS3
aN&GetRestHolderValues
aK
qN)const{static
aN
empty_result;d41
mN2
empty_result
dQ1
qM[qN
gV3;}
const
hF
oA2>&GetMatchedParamIndexes(dM
mD
wG2
swap(qR1
b){qM.swap(b.qM);qT.swap(b.qT);mD.swap(b.mD);}
qR1
wB1=oK1
qR1
b){qM=b.qM;qT=b.qT;mD=b.mD
dQ1*this;}
}
;class
oN;typedef
h8<oN>o5;class
oN{public:int
mS2;public:oN():mS2(0){}
virtual~oN(){}
}
;struct
aO{bool
found;o5
specs;aO(bool
f):found(f),specs(){}
aO(bool
f
aC2
s):found(f),specs(s){}
}
;void
SynthesizeRule
oK1
dT2
dK1&tree,aX2);aO
TestParam
oJ1
aH
tree
qG3);aO
TestParams(d4&g9,aH
tree
qG3,bool
d92;bool
ApplyGrammar
oK1
m21&hB2,dU
bool
from_logical_context=mY2;void
ApplyGrammars(FPoptimizer_CodeTree::dK1&tree);bool
IsLogisticallyPlausibleParamsMatch(d4&gP3,aH
tree);}
namespace
a6{void
DumpMatch
oK1
dT2
const
dU
const
FPoptimizer_Optimize::aX2,bool
DidMatch,std::ostream&o=std::cout
a62
DumpMatch
oK1
dT2
const
dU
const
FPoptimizer_Optimize::aX2,const
char*hS3,std::ostream&o=std::cout);}
#endif
#include <string>
oL1
a6::aB1
gN1=mY2;oL1
qE2
gN1=mY2;
#include <string>
#include <sstream>
#include <assert.h>
#include <iostream>
using
namespace
a6;qV
oL1
a6::aB1
gN1){
#if 1
const
char*p=0;switch(opcode){case
NumConstant:p="NumConstant"
;qA
ParamHolder:p="ParamHolder"
;qA
SubFunction:p="SubFunction"
o52
std::ostringstream
tmp;assert(p);tmp<<p;if(pad)while(tmp.str()qJ3<12)tmp<<' 'dQ1
tmp.str();
#else
std::ostringstream
tmp;tmp<<opcode;if(pad)while(tmp.str()qJ3<5)tmp<<' 'dQ1
tmp.str();
#endif
}
oL1
qE2
gN1){
#if 1
const
char*p=0;switch(opcode){case
cAbs:p="cAbs"
;qA
cAcos:p="cAcos"
;qA
cAcosh:p="cAcosh"
;qA
cAsin:p="cAsin"
;qA
cAsinh:p="cAsinh"
;qA
cAtan:p="cAtan"
;qA
cAtan2:p="cAtan2"
;qA
cAtanh:p="cAtanh"
;qA
cCbrt:p="cCbrt"
;qA
cCeil:p="cCeil"
;qA
cCos:p="cCos"
;qA
cCosh:p="cCosh"
;qA
cCot:p="cCot"
;qA
cCsc:p="cCsc"
;qA
cEval:p="cEval"
;qA
cExp:p="cExp"
;qA
cExp2:p="cExp2"
;qA
cFloor:p="cFloor"
;qA
cHypot:p="cHypot"
;qA
cIf:p="cIf"
;qA
cInt:p="cInt"
;qA
cLog:p="cLog"
;qA
cLog2:p="cLog2"
;qA
cLog10:p="cLog10"
;qA
cMax:p="cMax"
;qA
cMin:p="cMin"
;qA
cPow:p="cPow"
;qA
cSec:p="cSec"
;qA
cSin:p="cSin"
;qA
cSinh:p="cSinh"
;qA
cSqrt:p="cSqrt"
;qA
cTan:p="cTan"
;qA
cTanh:p="cTanh"
;qA
cTrunc:p="cTrunc"
;qA
cImmed:p="cImmed"
;qA
cJump:p="cJump"
;qA
cNeg:p="cNeg"
;qA
cAdd:p="cAdd"
;qA
cSub:p="cSub"
;qA
cMul:p="cMul"
;qA
cDiv:p="cDiv"
;qA
cMod:p="cMod"
;qA
cEqual:p="cEqual"
;qA
cNEqual:p="cNEqual"
;qA
cLess:p="cLess"
;qA
cLessOrEq:p="cLessOrEq"
;qA
cGreater:p="cGreater"
;qA
cGreaterOrEq:p="cGreaterOrEq"
;qA
cNot:p="cNot"
;qA
cAnd:p="cAnd"
;qA
cOr:p="cOr"
;qA
cDeg:p="cDeg"
;qA
cRad:p="cRad"
;qA
cFCall:p="cFCall"
;qA
hA3
p="cPCall"
;break;
#ifdef FP_SUPPORT_OPTIMIZER
case
cFetch:p="cFetch"
;qA
cPopNMov:p="cPopNMov"
;qA
cLog2by:p="cLog2by"
;qA
cSinCos:p="cSinCos"
;break;
#endif
case
cAbsNot:p="cAbsNot"
;qA
qY2:p="cAbsNotNot"
;qA
cAbsAnd:p="cAbsAnd"
;qA
cAbsOr:p="cAbsOr"
;qA
cAbsIf:p="cAbsIf"
;qA
cDup:p="cDup"
;qA
cInv:p="cInv"
;qA
cSqr:p="cSqr"
;qA
cRDiv:p="cRDiv"
;qA
cRSub:p="cRSub"
;qA
cNotNot:p="cNotNot"
;qA
cRSqrt:p="cRSqrt"
;qA
cNop:p="cNop"
;qA
VarBegin:p="VarBegin"
o52
std::ostringstream
tmp;assert(p);tmp<<p;if(pad)while(tmp.str()qJ3<12)tmp<<' 'dQ1
tmp.str();
#else
std::ostringstream
tmp;tmp<<opcode;if(pad)while(tmp.str()qJ3<5)tmp<<' 'dQ1
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
;wI{class
aA1{public:aA1():gE(),Immed(),mZ1(0),StackMax(0){gE.aJ2
64);Immed.aJ2
8);wV.aJ2
16)wG2
Pull(hF
oA2>&bc,hF
double>&imm,size_t&StackTop_max){gE.swap(bc);Immed.swap(imm);StackTop_max=StackMax;}
size_t
GetByteCodeSize(dM
gE
qJ3;}
size_t
GetStackTop(dM
mZ1
wG2
PushVar
aK
varno){mR
varno);gW
wF2
PushImmed(double
immed){qV
mR
cImmed);Immed.push_back(immed);gW
wF2
StackTopIs
h2,int
offset=0){if((int)mZ1>offset){wV
mQ
first=true;wV
mQ
second=tree;}
}
bool
IsStackTop
h2,int
offset=0
dM(int)mZ1>offset&&wV
mQ
first&&wV
mQ
second.IsIdenticalTo(tree)wG2
EatNParams
aK
d32){gW-d32)wG2
ProducedNParams
aK
dL1){gW+dL1)wG2
AddOperation
aK
opcode,oA2
d32,oA2
dL1=1){EatNParams(d32);qV
if(!gE.hW3&&opcode==cMul&&gE.back()==cDup)gE.back()=cSqr;else
mR
opcode);ProducedNParams(dL1)wG2
DoPopNMov
a81
d42,size_t
srcpos){qV
mR
cPopNMov);mR
aK)d42);mR
aK)srcpos);SetStackTop(srcpos+1);wV[d42]=wV[srcpos];SetStackTop(d42
wF2
DoDup
a81
aK2){qV
if(aK2==mZ1-1){mR
cDup);}
else{mR
cFetch);mR
aK)aK2);}
gW+1);wV[mZ1-1]=wV[aK2];}
size_t
h73
h2)const{for
a81
a=mZ1;a-->0;)if(wV[a
d43&&wV[a
gV3.IsIdenticalTo(tree))mN2
a
dQ1
dU2;}
bool
Find
h2
dM
h73(tree)!=dU2;}
bool
FindAndDup
h2){size_t
pos=h73(tree);if(pos!=dU2){
#ifdef DEBUG_SUBSTITUTIONS
qC2
o33"duplicate at ["
<<pos<<"]: "
;q13(tree)qB2" -- issuing cDup or cFetch\n"
;
#endif
DoDup(pos)oG}
hZ}
struct
IfData{size_t
ofs;}
;void
SynthIfStep1(IfData&mI2,qE2
op){qV
gW-1);mI2.ofs=gE
qJ3;mR
op);mR
0);mR
0)wG2
SynthIfStep2(IfData&mI2){qV
gW-1
dM1
1]=gR1+2
dM1
2]=gS1
mI2.ofs=gE
qJ3;mR
cJump);mR
0);mR
0)wG2
SynthIfStep3(IfData&mI2){qV
gW-1
dM1
1]=gR1-1
dM1
2]=gS1
gW+1)q52
mI2.ofs;++a){if(gE[a]==cJump&&gE[a+1]==mI2.ofs-1){gE[a+1]=gR1-1);gE[a+2]=gS1}
switch(gE[a]){case
cAbsIf:case
cIf:case
cJump:case
cPopNMov:a+=2;qA
cFCall:case
hA3
case
cFetch:a+=1;q03}
}
protected:void
SetStackTop
a81
value){mZ1=value;if(mZ1>StackMax){StackMax=mZ1;wV
aG2
StackMax);}
}
private:hF
oA2>gE;hF
double>Immed;hF
std::pair<bool,FPoptimizer_CodeTree::dK1> >wV;size_t
mZ1;size_t
StackMax;}
;struct
qP1;oX
qP1
AddSequence;oX
qP1
MulSequence;void
qB1
long
count,const
qP1&w5,aA1&synth);}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
qV
wI{const
struct
qP1
dN3
basevalue
h22
op_flip
h22
op_normal,op_normal_flip
h22
op_inverse,op_inverse_flip;}
AddSequence={0.0,cNeg,cAdd,cAdd,cSub,cRSub}
,MulSequence={1.0,cInv,cMul,cMul,cDiv,cRDiv}
;}
using
wI;
#define POWI_TABLE_SIZE 256
#define POWI_WINDOW_SIZE 3
wI{
#ifndef FP_GENERATING_POWI_TABLE
oX
oA2
char
powi_table[POWI_TABLE_SIZE];const
#endif
oA2
char
powi_table[POWI_TABLE_SIZE]={0,1,1,1,2,1,aB2
1,4,1,2,aL2
aB2
1,8,gL3
aM2
15,1,16,1,aB2
aL2
2,1,4,gL3
1,16,1,25,aM2
27,5,8,3,2,1,30,1,31,3,32,1,aB2
1,8,1,2,aM2
39,1,16,137,2,1,4,gL3
aL2
45,135,4,31,2,5,32,1,2,131,50,1,51,1,8,3,2,1,54,1,55,3,16,1,57,133,4,137,2,135,60,1,61,3,62,133,63,1,wD1
131,wD1
139,m31
oO
30,1,130,137,2,31,m31
oO
oO
130,gL3
1,oO
oO
2,1,130,133,wD1
61,130,133,62,139,130,137,oO
m31
oO
oO
wD1
131,oO
oO
130,131,2,133,m31
130,141,oO
130,gL3
1,oO
5,135,oO
m31
oO
m31
130,133,130,141,130,131,oO
oO
2,131}
;}
wT
d7=256;
#define FPO(x)
namespace{class
PowiCache{private:int
mE[d7];int
wE1[d7];public:PowiCache():mE(),wE1(){mE[1]=1;}
bool
Plan_Add(gT1,int
count){h61>=d7)hZ
wE1[dV2+=count
dQ1
mE[dV2!=0
wG2
wI2
gT1){h61<d7)mE[dV2=1
wG2
Start
a81
value1_pos){for(int
n=2;n<d7;++n)mE[n]=-1;Remember(1,value1_pos);DumpContents();}
int
Find(gT1)const{h61<d7){if(mE[dV2>=0){FPO(dT3(dW3,"* I found %ld from cache (%u,%d)\n",value,(unsigned)cache[value],dU3 value]))dQ1
mE[dV2;}
}
mN2-1
wG2
Remember(gT1,size_t
hX3){h61>=d7)mN2;FPO(dT3(dW3,"* Remembering that %ld can be found at %u (%d uses remain)\n",value,(unsigned)stackpos,dU3 value]));mE[dV2=(int)hX3
wG2
DumpContents()const{FPO(for(int a=1;a<POWI_CACHE_SIZE;++a)if(cache[a]>=0||dU3 a]>0){dT3(dW3,"== cache: sp=%d, val=%d, needs=%d\n",cache[a],a,dU3 a]);})}
int
aC1
gT1){h61>=0&&value<d7)mN2--wE1[dV2
dQ1
0;}
}
;size_t
hU
long
count
dR1
const
qP1&w5,aA1&synth
a62
gU1
size_t
apos,long
aval,size_t
bpos,long
bval
dR1
oA2
cumulation_opcode,oA2
cimulation_opcode_flip,aA1&synth
a62
a2
gT1
dR1
int
need_count,int
mY=0){h61<1)mN2;
#ifdef FP_GENERATING_POWI_TABLE
if(mY>32)throw
false;
#endif
if(mE.Plan_Add(value,need_count))mN2;long
aN2
1;h61<POWI_TABLE_SIZE){aN2
powi_table[dV2
qF2&128){gZ3
qF2&64)aN2-wF1
FPO(dT3(dW3,"value=%ld, half=%ld, otherhalf=%ld\n",value,half,value/half));a2
half
qM2
mE.wI2
half)dQ1;}
wG1
half&64){aN2-wF1}
}
else
h61&1)aN2
value&((1<<POWI_WINDOW_SIZE)-1);else
aN2
value/2;long
dY=value-half
qF2>dY||half<0)std::swap(half,dY);FPO(dT3(dW3,"value=%ld, half=%ld, otherhalf=%ld\n",value,half,otherhalf))qF2==dY){a2
half,mE,2,mY+1);}
else{a2
half
qM2
a2
dY>0?dY:-dY
qM2}
mE.wI2
value);}
size_t
hU
gT1
dR1
const
qP1&w5,qK1
int
aO2=mE.Find(value);if(aO2>=0){mN2
aO2;}
long
aN2
1;h61<POWI_TABLE_SIZE){aN2
powi_table[dV2
qF2&128){gZ3
qF2&64)aN2-wF1
FPO(dT3(dW3,"* I want %ld, my plan is %ld * %ld\n",value,half,value/half));size_t
qQ2=hU
half
h71
if(mE.aC1
half)>0||qQ2!=qT1){mJ1
qQ2);mE.Remember(half,qT1);}
qB1
value/half
qH2
size_t
hX3=qT1;mE.Remember(value,hX3);mE.DumpContents()dQ1
hX3;}
wG1
half&64){aN2-wF1}
}
else
h61&1)aN2
value&((1<<POWI_WINDOW_SIZE)-1);else
aN2
value/2;long
dY=value-half
qF2>dY||half<0)std::swap(half,dY);FPO(dT3(dW3,"* I want %ld, my plan is %ld + %ld\n",value,half,value-half))qF2==dY){size_t
qQ2=hU
half
h71
gU1
qQ2,half,qQ2,half,mE,w5.op_normal,w5.op_normal_flip,synth);}
else{long
part1=half;long
part2=dY>0?dY:-dY;size_t
part1_pos=hU
part1
h71
size_t
part2_pos=hU
part2
h71
FPO(dT3(dW3,"Subdivide(%ld: %ld, %ld)\n",value,half,otherhalf));gU1
part1_pos,part1,part2_pos,part2,mE,dY>0?w5.op_normal:w5.op_inverse,dY>0?w5.op_normal_flip:w5.op_inverse_flip,synth);}
size_t
hX3=qT1;mE.Remember(value,hX3);mE.DumpContents()dQ1
hX3
wG2
gU1
size_t
apos,long
aval,size_t
bpos,long
bval
dR1
oA2
cumulation_opcode,oA2
cumulation_opcode_flip,qK1
int
a_needed=mE.aC1
aval);int
aP2=mE.aC1
bval);bool
m41=false;
#define DUP_BOTH() do aQ2<bpos){size_t tmp=apos;apos=bpos;bpos=tmp;m41=!m41;}FPO(dT3(dW3,"-> "o03 o03"op\n",(unsigned)apos,(unsigned)bpos));mJ1 apos);mJ1 apos==bpos?qT1:bpos);}while(0)
#define DUP_ONE(p) do{FPO(dT3(dW3,"-> "o03"op\n",(unsigned)p));mJ1 p);}while(0)
if(a_needed>0){if(aP2>0){a01}
else{if(bpos!=qT1)a01
else{m51
m41=!m41;}
}
}
wG1
aP2>0)aQ2!=qT1)a01
else
DUP_ONE(bpos);}
else
aQ2==bpos&&apos==qT1)m51
wG1
apos==qT1&&bpos==qS1
gA
2){FPO(dT3(dW3,"-> op\n"));m41=!m41;}
wG1
apos==qS1
gA
2&&bpos==qT1)FPO(dT3(dW3,"-> op\n"));wG1
apos==qT1)DUP_ONE(bpos);wG1
bpos==qT1){m51
m41=!m41;}
else
a01}
qS1
dN1
m41?cumulation_opcode_flip:cumulation_opcode,2)wG2
h81
long
count,const
qP1&w5,qK1
while
gM3<256){int
aN2
oH
powi_table[count]qF2&128){gZ3;h81
half
qH2
count/=half;}
else
break;}
if
gM3==1)mN2;if(!gM3&1)){qS1
dN1
cSqr,1);h81
count/2
qH2}
else{mJ1
qT1);h81
count-1
qH2
qS1
dN1
cMul,2);}
}
}
wI{void
qB1
long
count,const
qP1&w5,qK1
if
gM3==0)qS1
PushImmed(w5.basevalue);else{bool
dW2=false;if
gM3<0){dW2=true;count=-count;}
if(mY2
h81
count
qH2
wG1
count>1){PowiCache
mE;a2
count,mE,1);size_t
qW1=qS1
GetStackTop();mE.Start(qT1);FPO(dT3(dW3,"Calculating result for %ld...\n",count));size_t
qR2=hU
count
h71
size_t
n_excess=qS1
gA
qW1;if(n_excess>0||qR2!=qW1-1){qS1
DoPopNMov(qW1-1,qR2);}
}
if(dW2)qS1
dN1
w5.op_flip,1);}
}
}
#endif
#include <list>
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
qV
w72{h83)wZ){gO3
Opcode=cNop;}
h83
double
i)wZ(i)){gO3
Recalculate_Hash_NoRecursion();}
g5
dK1
aK
v,g5
VarTag)wZ){gO3
Opcode=VarBegin;gO3
Var=v;gO3
Recalculate_Hash_NoRecursion();}
h83
aH
b,g5
CloneTag)wZ(*b.data)){}
g5~dK1(){}
struct
ParamComparer{mT2()(aH
a,aH
b)a32
a.qS2!=b.qS2)mN2
a.qS2>b.qS2
dQ1
a
o92<b
o92;}
}
;void
qQ1
Sort(){switch(Opcode){case
cAdd:case
cMul:case
cMin:case
cMax:case
cAnd:case
cOr:case
cHypot:case
cEqual:case
cNEqual:std::sort(qJ2
begin(),qJ2
end(),ParamComparer());qA
cLess:dS
cGreater;}
qA
cLessOrEq:dS
cGreaterOrEq;}
qA
cGreater:dS
cLess;}
qA
cGreaterOrEq:dS
cLessOrEq;}
q03}
dO1
dL2
aH
param){gX.push_back(param);}
dO1
aX1(dK1&param){gX.push_back(dK1());gX.back().swap(param);}
dO1
SetParam
a81
which,aH
b){qL1
which
aR2
gX[which]=b;}
dO1
SetParamMove
a81
which,dK1&b){qL1
which
aR2
gX[which].swap(b);}
dO1
AddParams
oK1
o6){gX.insert(gX
h13,aP
mU2
aP
h13);}
dO1
AddParamsMove(o6){size_t
endpos=gX
qJ3,added=aP
qJ3;gX
aG2
endpos+added,dK1())gX3
p=0;p<added;++p)gX[endpos+p].swap(aP[p]);}
dO1
AddParamsMove(o6,size_t
m61){qL1
m61].data
qI2
m61);AddParamsMove(aP);}
dO1
SetParams
oK1
o6
gN3
tmp(aP);gX.oO2}
dO1
SetParamsMove(o6){gX.swap(aP);aP.clear();}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
dO1
SetParams(h43&&aP){SetParamsMove(aP);}
#endif
dO1
DelParam
a81
index)a9
#ifdef __GXX_EXPERIMENTAL_CXX0X__
qJ2
erase(qJ2
begin()+index);
#else
qK2
index].data=0
gX3
p=index;p+1<dX2;++p)qK2
p]w82
UnsafeSetP(&*qK2
p+1
aR2
qK2
dX2-1]w82
UnsafeSetP(0);qJ2
resize(dX2-1);
#endif
}
dO1
DelParams(){gX.clear();}
g5
TriTruthValue
g5
GetEvennessInfo()a32!IsImmed())mN2
Unknown;if(!IsLongIntegerImmed())mN2
Unknown
dQ1(GetLongIntegerImmed()&1)?w92
IsAlways;}
aS2
qX1
const{switch
aT2
mM
a22
gO3
Value,0.0)||a22
gO3
Value,1.0);case
cAnd:case
cOr:case
cNot:case
cNotNot:case
cAbsAnd:case
cAbsOr:case
cAbsNot:case
qY2:case
cEqual:case
cNEqual:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:mN2
wJ2
cMul:a9
h93
dX2;++a)if(!qK2
a].qX1)hZ
mN2
true;a12
cIf:case
cAbsIf:a9
mN2
qK2
1].qX1&&qK2
2].qX1;}
aQ
hZ}
aS2
IsAlwaysInteger(bool
wL2)const{switch
aT2
mM
IsLongIntegerImmed()?wL2==true:wL2==false;case
cFloor:case
cInt:mN2
wL2==wJ2
cAnd:case
cOr:case
cNot:case
cNotNot:case
cEqual:case
cNEqual:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:mN2
wL2==wJ2
cIf:a9
mN2
qK2
1]aA&&qK2
2]aA
oG
a12
cAdd:case
cMul:{for
a81
a=hD!wB2
a)aA)hZ
mN2
gS3
aQ
hZ}
aS2
IsAlwaysSigned(bool
positive)const
dD3
tmp=o2;if(positive)mN2
tmp
a1&&tmp.min>=0.0&&(!tmp
gI||tmp.max>=0.0);else
mN2
tmp
gI&&tmp.max<0.0&&(!tmp
a1||tmp.min<0.0);}
aS2
IsIdenticalTo(aH
b)a32&*data==&*b.data)mN2
true
dQ1
gO3
IsIdenticalTo(*b.data);}
bool
qQ1
IsIdenticalTo
oK1
CodeTreeData&b)a32
Hash!=b.Hash)hZ
if(Opcode!=b.Opcode)hZ
switch(mM
a22
Value,b.Value);dS1
mN2
Var==b.Var;wK2
case
hA3
if(Funcno!=b.Funcno)hZ
q03
if(dX2!=b.dX2)hZ
h93
dX2;++a){if(!qK2
a].IsIdenticalTo(b.qK2
a]))hZ}
mN2
gS3
dO1
Become(aH
b){if(&b!=this&&&*data!=&*b.data){DataP
tmp=b.data;oY
data.oO2}
}
dO1
CopyOnWrite(){if
aT2
mS2>1)data=new
CodeTreeData(*data);}
dK1
g5
GetUniqueRef(){if
aT2
mS2>1)mN2
dK1(*this,CloneTag())dQ1*this;}
qQ1
CodeTreeData(oP1
cNop
wM2),Hash(),Depth(1),gO1
0){}
qQ1
CodeTreeData
oK1
CodeTreeData&b
oP1
b.Opcode
wM2
b.Params),Hash(b.Hash),Depth(b.Depth),gO1
b.m11){switch(Opcode){dS1
Var=b.Var;qA
cImmed:Value=b.Value;qA
hA3
wK2
Funcno=b.Funcno;q03}
#ifdef __GXX_EXPERIMENTAL_CXX0X__
qQ1
CodeTreeData(CodeTreeData&&b
oP1
b.Opcode
wM2
b.Params),Hash(b.Hash),Depth(b.Depth),gO1
b.m11){switch(Opcode){dS1
Var=b.Var;qA
cImmed:Value=b.Value;qA
hA3
wK2
Funcno=b.Funcno;q03}
#endif
qQ1
CodeTreeData(double
i
oP1
cImmed
wM2),Hash(),Depth(1),gO1
0){Value=i;}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <iostream>
qV
#ifdef FUNCTIONPARSER_SUPPORT_DEBUG_OUTPUT
namespace{void
m71
h2,d1&done,std::ostream&o){h93
hD3
m71
qT2
done,o);std::ostringstream
buf;q13(tree,buf);done[tree
o92].insert(buf.str());}
}
#endif
w72{
#ifdef FUNCTIONPARSER_SUPPORT_DEBUG_OUTPUT
void
DumpHashes(oE){d1
done;m71(tree,done,o);for(d1::const_iterator
i=done.begin();i!=done
h13;++i){const
std::set<std::string>&flist=i->second;if(flist
qJ3!=1)o<<"ERROR - HASH COLLISION?\n"
;for(std::set<std::string>::const_iterator
j=flist.begin();j!=flist
h13;++j){o<<'['g03
hex<<i->first
mX2<<','<<i->first.hash2<<']'g03
dec;o<<": "
<<*j<<"\n"
;}
}
}
void
q13(oE){const
char*dV3;switch(oJ){case
cImmed:o<<tree
hF1
dQ1;dS1
o<<"Var"
<<(tree.GetVar()-VarBegin)dQ1;case
cAdd:dV3"+"
;qA
cMul:dV3"*"
;qA
cAnd:dV3"&"
;qA
cOr:dV3"|"
;qA
cPow:dV3"^"
;break;default:dV3;o<<FP_GetOpcodeName(oJ);if(oJ==cFCall||oJ==cPCall)o<<':'<<tree.GetFuncNo();}
o<<'(';if(dN<=1&&sep2[1])o<<(sep2+1)<<' 'q52
hD3{if(a>0)o<<' ';q13
qT2
o);if(a+1<dN)o<<sep2;}
o<<')'wG2
DumpTreeWithIndent(oE,const
std::string&indent){o<<'['g03
hex<<(void*)(&tree.oU1)g03
dec<<','<<tree.GetRefCount()<<']';o<<indent<<'_';switch(oJ){case
cImmed:o<<"cImmed "
<<tree
hF1;o<<'\n'dQ1;dS1
o<<"VarBegin "
<<(tree.GetVar()-VarBegin);o<<'\n'dQ1;default:o<<FP_GetOpcodeName(oJ);if(oJ==cFCall||oJ==cPCall)o<<':'<<tree.GetFuncNo();o<<'\n';}
h93
hD3{std::string
ind=indent
gX3
p=0;p<ind
qJ3;p+=2)if(ind[p]=='\\')ind[p]=' ';ind+=(a+1<dN)?" |"
:" \\"
;DumpTreeWithIndent
qT2
o,ind);}
o
g03
flush;}
#endif
}
#endif
using
namespace
a6;qV
#include <cctype>
namespace
a6{bool
ParamSpec_Compare
oK1
void*aa,const
void*bb,aB1
type){switch(type){dV1
gZ&a=*(gZ*)aa;gZ&b=*(gZ*)bb
dQ1
a
qG2==b
qG2&&a.index==b.index&&a.depcode==b.depcode;a12
NumConstant:{d8&a=*(d8*)aa;d8&b=*(d8*)bb
dQ1
a22
a.dY2,b.dY2)&&a.modulo==b.modulo;a12
aD1
h0&a=*(h0*)aa;h0&b=*(h0*)bb
dQ1
a
qG2==b
qG2&&a
dP==b
dP&&a
w82
match_type==b
w82
match_type&&a.data
g22==b.data
g22&&a
w82
param_list==b
w82
param_list&&a
w82
qN==b
w82
qN&&a.depcode==b.depcode;}
}
mN2
gS3
oA2
ParamSpec_GetDepCode
oK1
ParamSpec&b){switch(b.first){dV1
const
gZ*s=oK1
gZ*)b.second
dQ1
s->depcode;a12
aD1
const
h0*s=oK1
h0*)b.second
dQ1
s->depcode;}
aQ
mN2
0
wG2
DumpParam
oJ1
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
h22
qU2
0;aV2
NumConstant:{const
d8&param=*oK1
d8
mG1
o.precision(12);o<<param.dY2
o52
dV1
const
gZ&param=*oK1
gZ
mG1
o<<ParamHolderNames[param.index];qU2
param
qG2
o52
case
aD1
const
h0&param
g32
qU2
param
qG2;hW
GroupFunction){if(param
dP==cNeg){o<<"-"
;qZ}
wG1
param
dP==cInv){o<<"/"
;qZ}
else{std::string
opcode=FP_GetOpcodeName(param
dP).substr(1)q52
opcode
qH3
a)opcode[a]=(char)std::toupper(opcode[a]);o<<opcode<<"( "
;qZ
o<<" )"
;}
}
else{o<<'('<<FP_GetOpcodeName(param
dP)<<' ';hW
PositionalParams)o<<'[';hW
SelectedParams)o<<'{';qZ
if(param
w82
qN!=0)o<<" <"
<<param
w82
qN<<'>';hW
PositionalParams)o<<"]"
;hW
SelectedParams)o<<"}"
;o<<')';}
break;}
}
switch(ImmedConstraint_Value(qV2
ValueMask)){case
ValueMask:qA
Value_AnyNum:qA
dW1:o<<"@E"
;qA
Value_OddInt:o<<"@O"
;qA
gP1:o<<"@I"
;qA
Value_NonInteger:o<<"@F"
;qA
dX1:o<<"@L"
o52
switch(ImmedConstraint_Sign(qV2
SignMask)){case
SignMask:qA
Sign_AnySign:qA
aR:o<<"@P"
;qA
Sign_Negative:o<<"@N"
o52
switch(ImmedConstraint_Oneness(qV2
OnenessMask)){case
OnenessMask:qA
Oneness_Any:qA
Oneness_One:o<<"@1"
;qA
h91:o<<"@M"
o52}
void
DumpParams
aK
paramlist,oA2
count,std::ostream&o){for
aK
a=0;a<count;++a){if(a>0)o<<' ';const
ParamSpec&param=gW1
paramlist,a);DumpParam(param,o)h22
depcode=ParamSpec_GetDepCode(param);if(depcode!=0)o<<"@D"
<<depcode;}
}
}
#include <algorithm>
using
namespace
a6;qV
namespace{const
gZ
plist_p[33]={{2,0,0x0}
oF
0,0x4}
oF
aR,0x0}
oF
Sign_NoIdea,0x0}
oF
dX1,0x0}
,{3,Sign_NoIdea,0x0}
,{3,0,0x0}
,{3,dX1,0x0}
,{3,0,0x8}
,{3,Value_OddInt,0x0}
,{3,Value_NonInteger,0x0}
,{3,dW1,0x0}
,{3,aR,0x0}
,{0,Sign_Negative
q23
qD
0,aR
q23
dW1
q23
q31
0,gP1|aR
q23
h91|q31
0,h91
q23
Oneness_One
q23
dX1|qD
1,dW1|qD
1,qD
1,h91|qD
1,gP1|qD
1,aR|qD
6,0,0x0}
,{4,0,0x0}
,{4,0,0x16}
,{4,gP1,0x0}
,{5,0,0x0}
,}
;const
d8
plist_n[19]={{-2,0}
,{-1,0}
,{-0.5,0}
,{0,0}
,{CONSTANT_RD,0}
,{CONSTANT_EI,0}
,{CONSTANT_L10I,0}
,{0.5,0}
,{CONSTANT_L2,0}
,{1,0}
,{CONSTANT_L2I,0}
oF
0}
,{CONSTANT_L10,0}
,{CONSTANT_E,0}
,{CONSTANT_DR,0}
,{dH1
qY1{0,qY1{CONSTANT_PIHALF,qY1{o3,qY1}
;const
h0
plist_s[468]={{{1,14,hE3
14,cNeg,GroupFunction,0}
,q31{1,406,hE3
407,hE3
14,dZ2
24,dZ2
130,dZ2
134,dZ2
394,dZ2
395,dZ2
407,cInv
o9
296205,cAdd,q43
262178,cAdd,q42
0x5}
dL3
262178,q6
42,cAdd,q42
hG3
55338,q6
138282,q6
217130,q6
261154,q6
261162,q6
262186,q6
262186,cAdd,q42
0x5}
dL3
138290,q6
188458,cAdd,q42
0x1}
dL3
216114,q6
271394,q6
271402,q6
316458,q6
222258,q6
6144,q6
139264,q6
249856,q6
250880,q6
29702,q6
175110,q6
55310,q6
56334,q6
278549,cAdd,q42
hG3
56344,q6
28701,q6
32774,q6
17568,q6
54432,q6
43187,q6
281780,q6
280757,q6
168159,q6
169202,q6
233701,q6
236774,q6
35071,q6
43263,q6
281868,q6
1326,q6
14724,q6
24964,q6
401612,q6
403848,cAdd,qI
0,0,w6
1}
q32
0,0,w6
q22
1,40,qO
48,qO
49,qO
50,qO
51,qO
0,w6
1
qW2
0,w6
2
qW2
0,qO
20,qO
14,qO
25,qO
24,w6
q22
2,53272,w6
q22
1,212,qO
220,w6
q22
1,233,w6
1}
,0,0x16}
,{{1,325,w6
1}
,0,0x16}
,{{0,0,w6
1}
,aR,0x0}
dL3
53288
oB2
40974
oB2
43022
oB2
24590
oB2
53272
oB2
34,dC
6178,dC
112674,dC
273442,qG
0x6}
dL3
306210,qG
0x6}
dL3
327714,dC
345122,dC
363554,dC
368674,dC
369698,dC
373794,dC
380962,dC
423970,dC
427042,dC
432162,dC
435234,dC
444450,dC
452642,dC
299054,q3
3,58761216,dC
6144,dC
504832,dC
14,dC
70670,dC
128014,dC
271374,qG
0x1}
dL3
289792,q3
3,42346496,dC
29702,dC
113688,dC
112664,dC
28701,dC
32774,dC
273422,qG
hF3
2,294912,dC
306190,qG
hF3
3,64360477,dC
404480,dC
52,dC
85050,dC
86075,dC
35062,dC
35062,qG
hG3
35064,qG
hG3
35071,dC
35084,dC
14623,dC
35181,qG
dY1
3,14725486,dC
57732,dC
35238,qG
hG3
34216,dC
45424,q3
3,14725544,dC
306228,qG
hF3
2,45484,dC
102681,dC
103706,dC
257316,dC
298324,dC
287060,dC
346452,dC
45409,dC
307266,dC
366948
aB
3,36018532
aB
2,421220
aB
2,421273
aB
3,36072804
aB
2,6538,dC
367001
aB
3,36072857
aB
2,6618,dC
6645,q3
0,0,gF
0,0,dZ
1,34,gF
1,34,dZ
1,0,qH
1
qW2
0,qH
2
qW2
0,gF
1,0,dZ
1,13,gF
1,15,gF
1,15,qH
1
h32
1,20,gF
1,14,gF
1,24,dZ
2,24590,gF
1,52,gF
1,52,qH
2
h32
1,270,qH
1
qW2
271,qH
2
qW2
273,dZ
1,276,gF
1,277,gF
1,278,dZ
1,294,gF
1,325,dZ
1,342,gF
1,447,dZ
2,397372,qH
1
h32
2,397637,qH
1}
,0,0x16}
,{{1,388,gF
1,394,gF
2,57384,h1
40974,h1
24590,h1
40984,h1
35971,h1
35973,h1
38,qC
38,qP
46,qC
46,qP
356390,qC
215086,qC
356398,qC
357414,qC
357422,qC
45056,qC
14,qC
15,qC
14,d51
6144,qP
6158,d51
14336,qC
15360,qC
27654,qC
24582,qC
29698,qC
24
q33
0x6}
dL3
24,qC
6168,qC
16384,qC
18432,qC
26624,qC
29696,qC
68608,qC
87040,qC
88088,qC
89088,qC
90112,qC
92160,qC
246784,qC
403456,qC
415744,qC
416768,qC
34822,qC
9216,qC
10240,qC
6146,qC
6144,qC
13317,qC
23558,qC
53254,qC
112654,qC
112669,qC
248838,qC
6158,qC
34881
q33
0x5}
dL3
34888
q33
0x5}
dL3
35026,qC
35032,qP
57,qC
34884,qP
35911,qP
41030,qP
41031,qP
35919,qC
41039,qC
41062,qC
41055,qP
41063,qC
45166,qC
57605,qC
63749,qC
63752,qC
35172,qP
35172,qC
35178
q33
0x5}
dL3
35179,d51
35181,qP
35181,qC
45412,qP
45412,qC
14701,qP
135533,qC
35183
q33
0x5}
dL3
35204,qC
35206,qC
35212,qC
35213,qC
35225,qP
35238,qP
35245,qP
35245,qC
35254,qC
35254,qP
35256,qC
57
q33
0x6}
dL3
45465,qP
45465,qC
24785,qC
6147,cPow
m2
0}
,aR,0x0}
dL3
24590,cPow
o9
57368
oR1
gL
0,hH3
6,hH3
155,hH3
0,cAcos
hI3
cAcosh
hI3
cAsin
hI3
cAsinh
dD
110,cAsinh
hI3
cAtan,o02
hJ3
q43
296960,cAtan2
hI3
cAtanh
hI3
cCeil,gJ3
209,cCeil
hI3
h42
0,cCos,gJ3
6,h42
81,h42
82,h42
110,h42
173,h42
224,h42
0,cCosh,gJ3
0,cCosh
dD
172,cCosh
dD
173,cCosh
dD
393,cCosh
hI3
cCot
hI3
cCsc
hI3
cFloor,gJ3
209,cFloor,q43
300311,cHypot,q0
3,30414848,aS
498096128,aS
499128320,aS
29392896,cIf,gY
3,30414848,cIf,gY
3,6778880,aS
23555072,aS
95514624,aS
174234624,aS
418791424,aS
422989824,aS
512205824,aS
519553024,cIf
dD
110,cInt
hI3
o12
6,o12
29,o12
155,o12
209,o12
267,o12
14,cLog,gL
24,cLog,gL
0,cLog10
hI3
cLog2,o02
cMax,q43
28701,cMax,q43
32774,cMax
hI3
cMax,AnyParams,1}
,0,hG3
6144,cMin,q43
28701,cMin,q43
32774,cMin
hI3
cMin,AnyParams,1}
,0,hG3
43022,cMin
o9
24590,cMin,gL
0,cSec
hI3
aE1
0,cSin,gJ3
6,aE1
81,aE1
82,aE1
110,aE1
137,aE1
157,cSin,q7
0x5}
,{{1,209,aE1
217,aE1
221,cSin,q7
0x1}
,{{1,224,aE1
0,cSinh,gJ3
0
oA3
157,cSinh,q7
0x5}
,{{1,172
oA3
209
oA3
217
oA3
224
oA3
393,cSinh
hI3
h52
0,cTan,gJ3
74,cTan,gJ3
76,h52
157,h52
209,h52
221,h52
217,h52
224,h52
0,qX2
0,cTanh,gJ3
162,qX2
157,qX2
209,qX2
217,qX2
224,qX2
0,cTrunc,q43
14360,cSub
o9
14360,cDiv
o9
403851,cDiv
o9
6144,wN2
m81
6144,cEqual,q43
29696,wN2
0x20}
dL3
29702,wN2
0x24}
dL3
29702,cEqual,q43
36864,h62
6,cLess,q43
14,h62
6144,cLess,q7
m81
24576,h62
36864,wO2
24,wO2
6144,dZ1
q7
m81
6144,dZ1
q43
14336,wO2
245969,dZ1
q43
36864,m91
24,m91
6144,cGreater,q7
m81
14336,m91
36864,gV1
14,gV1
6144,cGreaterOrEq,q7
m81
24576,gV1
245969,cGreaterOrEq
hI3
o01
6,o01
14,o01
29,o01
154,o01
174,o01
175,o01
487,o01
490,o01
491,o01
494,o01
497,o01
498,cNot,o02
mA1
28701,mA1
32774,mA1
385053,mA1
388125,mA1
6618,cAnd,qI
0,0,hJ
1}
q32
2,6144,aR1
28701,aR1
32774,aR1
385053,aR1
388125,aR1
6618,cOr,qI
0,0,hT
1}
q32
1,0,qZ1
6,qZ1
81,qZ1
119,qZ1
154,qZ1
156,qZ1
174,qZ1
175,qZ1
209,cDeg
dD
209,cRad,q0
3,516954112,cAbsAnd,qI
3,524294144,cAbsOr,qI
1,0,cAbsNot
hI3
cAbsNot,gJ3
0,qY2
hI3
qY2,gJ3
6,qY2
dD
29,qY2,q0
3,30414848,cAbsIf,q7
0x0}
,}
;}
namespace
a6{const
Rule
grammar_rules[270]={{ProduceNewTree,true,1,0,{1,0,cAbs,q2
352,{1,169,cAtan,q2
345
oF
1331,hJ3
q2
347
oF
314369,hJ3
q1
215249
oF
219349,hJ3
q2
146
mH1
cCeil,q2
420,{1,80,o22
414,{1,113,o22
415,{1,115,o22
144,{1,116,o22
361,{1,114,o22
0,{1,345,cCos
m2
q4
0,{1,342,cCos
m2
q4
209
mH1
o22
308,{1,348,cCosh
m2
q4
0,{1,342,cCosh
m2
q4
209
mH1
cCosh,q2
142
mH1
cFloor,q2
387,{1,112,cFloor,q2
207,{3,6328320,aC
519,{3,30414850,aC
492,{3,7376896,aC
494,{3,7383040,aC
208,{3,37754880,aC
487,{3,37755904,aC
501,{3,37791744,aC
499,{3,44047360,aC
474,{3,44077056,aC
q53
1057225,aC
q53
1057229,aC
q63
1057235,aC
q63
1057239,aC
q63
8390089,aC
q63
8390093,aC
q53
8390099,aC
q53
8390103,aC
q73
389384646,aC
q73
389384651,aC
q73
371576273,aC
q73
371576277,wP2
false,3,30441927,{3,33584590,wP2
true,3,30414848,{3,30938112,wP2
true,3,30414848,{3,543168512,aC
109,{1,218,hK3
108,{1,232,hK3
346,{1,105,hK3
197,{1,198,cLog
m2
q4
383
d52,cMax,qZ2
0
oF
414721,cMax,qZ2
384
d52,cMin,qZ2
0
oF
410625,cMin,AnyParams,0
o0
373
oF
41023
mJ2
196
oF
24794
mJ2
195
oF
109582
mJ2
196
oF
23769
mJ2
193
oF
130078
mJ2
153
oF
131102
mJ2
194
oF
124942
q83
29980
q83
29981
q83
29982
q93
167254
oF
30035
q93
53618
oF
13721
q93
53656
oF
13668
q93
53617
oF
13741
q93
403456
oF
397326
q93
244736
oF
243726
q93
214016
oF
242718
q93
214016
oF
241681
q83
32031
q93
6144
oF
11606
q93
6359
oF
11498
mJ2
148
mH1
h72
363,{1,80,h72
144,{1,113,h72
361,{1,115,h72
147,{1,116,h72
414,{1,114,h72
0,{1,347,h72
150
mH1
cSinh,q2
306,{1,346,cSinh,q2
151
mH1
hL3
0,{1,350,hL3
161,{1,351,hL3
152
mH1
cTanh,q2
254,{1,305,w6
1
o0
253,{1,304,w6
1}
}
,{ReplaceParams,false,1,252
oF
1327
gU
250
oF
1325
gU
381
d52
gU
391
oF
398724
gU
192
oF
232674
gU
191
oF
219362
gU
159
oF
230619
gU
158
oF
230419
gU
42
oF
326992
gU
320
oF
144426
gU
337
oF
143402
gU
412
oF
206029
gU
413
oF
208077
gU
359
oF
207047
gU
143
oF
207048
gU
360
oF
211143
gU
190
oF
142602
gU
187
oF
343306
gU
186
oF
343178
gU
188
oF
193704
gU
183
oF
174248
gU
248
oF
431468
gU
176
oF
431286
gU
149
oF
182636
gU
145
oF
182693
gU
246
oF
189804
gU
365
oF
255161
gU
422
oF
255158
gU
422
oF
181612
gU
365
oF
253349
gU
149
oF
253110,w6
0
o0
106
oF
76817,cMul,SelectedParams,0
o0
509,{1,47,qH
1
o0
510,{1,37,qH
1}
}
,{ReplaceParams,false,1,297
oF
1322,qH
q4
382
d52,qH
q4
484
oF
486874,qH
q4
487
oF
514549,qH
q4
492
oF
514522,qH
q4
98
oF
24702,qH
q4
99
oF
24696,qH
q4
344
oF
351574,qH
q4
97
oF
91395,qH
q4
104
oF
80140,qH
q4
96
oF
79116,qH
q4
420
oF
428066,qH
q4
427
oF
436258,qH
q4
332
oF
373065,qH
q4
422
oF
373175,qH
q4
365
oF
431437,qH
q4
356
oF
420170,qH
q4
429
oF
320922,qH
q4
314
oF
336302,qH
q4
331
oF
365896,qH
q4
409
oF
365998,qH
q4
42
oF
440751,qH
q4
435
oF
324003,qH
q4
42
oF
444848,qH
q4
437
oF
446498,qH
q4
438
oF
325029,qH
q4
318
oF
337335,qH
q4
441
oF
331175,qH
q4
433
oF
322976,qH
q4
444
oF
453666,qH
q4
440
oF
303168,qH
q4
334
oF
302153,qH
q4
396
oF
40324,qH
q4
326
oF
46404,qH
q4
397
oF
44420,qH
q4
327
oF
42308,qH
oV1
106510
oF
95500,qH
oV1
98318
oF
96524,qH
oV1
330166
oF
329125,qH
o1
true,1,0,{1,342,qH
o1
true,1,52,{1,13,qH
oV1
36864
oF
37206,aU
wQ
aU
mB1
24797,aU
wR
aU
w7
aU
w8
aU
mC1
227549,aU
wS
aU
w9
aU
a5,aU
36864
oF
37206,aD
wQ
aD
mB1
24797,aD
wR
aD
w7
aD
w8
aD
mC1
227549,aD
wS
aD
w9
aD
a5,cNEqual,q2
513
oF
40960,cLess,q2
478
oF
14678,cLess,q1
mB1
24794,cLess,q1
g01
cLess,q1
wQ
cLess,q1
wR
cLess,q1
w7
cLess,q1
w8
cLess,q1
mC1
qA3
cLess,q1
q91
cLess,q1
wS
cLess,q1
w9
cLess,q1
a5,cLess,q2
505
oF
350222,d9
mB1
24794,d9
g01
d9
wQ
d9
wR
d9
w7
d9
w8
d9
mC1
qA3
d9
q91
d9
wS
d9
w9
d9
a5,d9
mB1
24794,wL
g01
wL
wQ
wL
wR
wL
w7
wL
w8
wL
mC1
qA3
wL
q91
wL
wS
wL
w9
wL
a5,cGreater,q2
515
oF
40960,hI
mB1
24794,hI
g01
hI
wQ
hI
wR
hI
w7
hI
w8
hI
mC1
qA3
hI
q91
hI
wS
hI
w9
hI
a5,cGreaterOrEq,q2
513,{1,2,cNot,q2
506,{1,4,hJ
1
o0
511
oF
12290,hJ
1}
}
,{ReplaceParams,false,1,385
d52,hJ
q4
484
oF
486874,hJ
q4
485
oF
389597,hJ
q4
486
oF
384477,hJ
q4
qB3
469460,hJ
q4
qB3
469448,hJ
q4
479
oF
526804,hJ
q4
qB3
477652,hJ
oV1
464322,{3,473371073,hJ
0
o0
512
oF
12290,hT
1}
}
,{ReplaceParams,false,1,503
oF
7172,hT
q4
386
d52,hT
q4
481
oF
486874,hT
q4
482
oF
389597,hT
q4
483
oF
384477
aF1
483791
aF1
483798,hT
q4
507
oF
528847
aF1
471503,hT
q4
504
oF
132100,hT
0
o0
515,{1,2,cNotNot
m2
0}
}
,{ProduceNewTree,true,1,0,{1,0,cNotNot
m2
q4
qB3
469460,mS
qB3
469448,mS
479
oF
526804,mS
qB3
477652,mS
508
oF
483791,qA1
q4
508
oF
483798,qA1
q4
507
oF
528847,qA1
q4
508
oF
471503,qA1
0
o0
473,{1,218,qY2,AnyParams,0
o0
464,{1,217,qY2,AnyParams,0}
}
,{ProduceNewTree,true,1,0,{1,0,qY2,q2
374,{3,30415349,cAbsIf,q2
515,{3,37791744,cAbsIf,q2
513,{3,44077056,cAbsIf
m2
o1
false,3,30441927,{3,33584590,cAbsIf
m2
0}
}
,}
;struct
grammar_optimize_abslogical_type{gT
12
oU
grammar_optimize_abslogical_type
grammar_optimize_abslogical={12,{20,41,42,182,218,230,232,242,253,9,10,13}
}
;}
struct
grammar_optimize_ignore_if_sideeffects_type{gT
67
oU
grammar_optimize_ignore_if_sideeffects_type
grammar_optimize_ignore_if_sideeffects={67,{0,19,21,22,23,24,25,26,g62
gS
grammar_optimize_nonshortcut_logical_evaluation_type{gT
64
oU
grammar_optimize_nonshortcut_logical_evaluation_type
grammar_optimize_nonshortcut_logical_evaluation={64,{0,25,g62
160,161,162,172,183,195,231,233,234,235,236,237,238,239,240,241,243,244,245,246,247,248,249,250,251,252,254,255,0,1,2,3,4,5,6,7,8,11,12}
}
;}
struct
grammar_optimize_round1_type{gT
125
oU
grammar_optimize_round1_type
grammar_optimize_round1={125,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,17,25,gR
43,44,wI1
52,53,54,55,56,57,58,59,60,61,65,66,67,68,69,70,71,72,73,74,75,76,77,78,89,90,91,92,93,94,95,96,97,98,99,100,101,102,118,121,a3
126,127,128,129,130,131,132,157,158,gS
grammar_optimize_round2_type{gT
108
oU
grammar_optimize_round2_type
grammar_optimize_round2={108,{0,14,15,16,25,gR
45,46,wI1
52,54,55,56,57,58,59,60,61,69,70,71,79,80,85,86,87,88,89,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,121,a3
128,129,130,131,133,157,158,159,gS
grammar_optimize_round3_type{gT
85
oU
grammar_optimize_round3_type
grammar_optimize_round3={85,{81,82,83,84,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,163,164,165,166,167,168,169,170,171,173,174,175,176,177,178,179,180,181,184,185,186,187,188,189,190,191,192,193,194,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,219,220,221,222,223,224,225,226,227,228,229}
}
;}
struct
grammar_optimize_round4_type{gT
11
oU
grammar_optimize_round4_type
grammar_optimize_round4={11,{18,51,62,63,64,119,120,153,154,155,156}
}
;}
struct
grammar_optimize_shortcut_logical_evaluation_type{gT
61
oU
grammar_optimize_shortcut_logical_evaluation_type
grammar_optimize_shortcut_logical_evaluation={61,{0,25,g62
160,161,162,172,183,195,233,234,235,236,237,238,239,240,241,244,245,246,247,248,249,250,251,254,255,0,1,2,3,4,5,6,7,8,11,12}
}
;}
}
namespace
a6{ParamSpec
ParamSpec_Extract
aK
paramlist,mZ){index=(paramlist>>(index*10))&1023;if(index>=52)d61
SubFunction,h82
plist_s[index-52]);if(index>=33)d61
NumConstant,h82
plist_n[index-33]);d61
ParamHolder,h82
plist_p[index]);}
}
#ifdef FP_SUPPORT_OPTIMIZER
#include <stdio.h>
#include <algorithm>
#include <map>
#include <sstream>
qV
using
namespace
a6;using
w72;using
namespace
FPoptimizer_Optimize;namespace{qC1
It,typename
T,typename
Comp>o11
MyEqualRange(It
first,It
last,const
T&val,Comp
comp){size_t
len=last-first;while(len>0){size_t
aN2
len/2;It
mK2(first);mK2+=half;if(comp(*mK2,val)){first=mK2;++first;len=len-half-1;}
wG1
comp(val,*mK2)){len=half;}
else{It
left(first);{It&d62=left;It
last2(mK2);size_t
len2=last2-d62;while(len2>0){size_t
half2=len2/2;It
middle2(d62);middle2+=half2;if(comp(*middle2,val)){d62=middle2;++d62;len2=len2-half2-1;}
else
len2=half2;}
}
first+=len;It
right(++mK2);{It&d62=right;It&last2=first;size_t
len2=last2-d62;while(len2>0){size_t
half2=len2/2;It
middle2(d62);middle2+=half2;if(comp(val,*middle2))len2=half2;else{d62=middle2;++d62;len2=len2-half2-1;}
}
}
mN2
o11(left,right);}
}
mN2
o11(first,first);}
struct
OpcodeRuleCompare{mT2()(aH
tree,oA2
g02)const{const
Rule&rule=grammar_rules[g02]dQ1
oJ<rule
a31.subfunc_opcode;}
mT2()aK
g02,aH
tree)const{const
Rule&rule=grammar_rules[g02]dQ1
rule
a31.subfunc_opcode<oJ;}
}
;bool
TestRuleAndApplyIfMatch
oK1
dT2
dK1&tree,bool
dA{MatchInfo
info;aO
found(false,o5());if(rule.logical_context&&!dA{goto
fail;}
for(;;){
#ifdef DEBUG_SUBSTITUTIONS
#endif
found=TestParams(rule
a31,tree,found.specs,info,true);if(found.found)break;if(!&*found.specs){fail:;
#ifdef DEBUG_SUBSTITUTIONS
DumpMatch(rule,tree,info,mY2;
#endif
hZ}
}
#ifdef DEBUG_SUBSTITUTIONS
DumpMatch(rule,tree,info,true);
#endif
SynthesizeRule(rule,tree,info)oG}
}
namespace
FPoptimizer_Optimize{bool
ApplyGrammar
oK1
m21&hB2,dK1&tree,bool
dA{if(tree.GetOptimizedUsing()==&hB2){
#ifdef DEBUG_SUBSTITUTIONS
qC2"Already optimized:  "
;q13(tree)qB2"\n"
g03
flush;
#endif
hZ}
if(true){bool
a21
false;switch(oJ){case
cNot:case
cNotNot:case
cAnd:case
cOr:for
a81
a=0;a
g6
true))a21
true;qA
cIf:case
cAbsIf:if(ApplyGrammar(hB2,tree
gC
0),oJ==cIf))a21
true
gW3
1;a
g6
dA)a21
true;break;default:for
a81
a=0;a
g6
mY2)a21
gS3
if(changed){tree.Mark_Incompletely_Hashed()oG}
}
typedef
const
oA2
char*wQ2;std::pair<wQ2,wQ2>range=MyEqualRange(hB2.rule_list,hB2.rule_list+hB2.rule_count,tree,OpcodeRuleCompare());if(range.first!=range.second){
#ifdef DEBUG_SUBSTITUTIONS
hF
oA2
char>rules;rules.aJ2
range.second-range.first);hC
if(IsLogisticallyPlausibleParamsMatch(d71
a31,tree))rules.push_back(*r);}
range.first=&rules[0];range.second=&rules[rules
qJ3-1]+1;if(range.first!=range.second){qC2"Input ("
<<FP_GetOpcodeName(oJ)<<")["
<<dN<<"]"
;if(dA
qC2"(Logical)"
h22
first=wJ1,prev=wJ1;const
char*sep=", rules "
;hC
if(first==wJ1)first=prev=*r;wG1*r==prev+1)prev=*r;else{qC2
sep<<first;sep=","
;if(prev!=first)qC2'-'<<prev;first=prev=*r;}
}
if(first!=wJ1){qC2
sep<<first;if(prev!=first)qC2'-'<<prev;}
qC2": "
;q13(tree)qB2"\n"
g03
flush;}
#endif
bool
a21
false;hC
#ifndef DEBUG_SUBSTITUTIONS
if(!IsLogisticallyPlausibleParamsMatch(d71
a31,tree))dD2
#endif
if(TestRuleAndApplyIfMatch(d71,tree,dA){a21
true
o52}
if(changed){
#ifdef DEBUG_SUBSTITUTIONS
qC2"Changed."
g03
endl
qB2"Output: "
;q13(tree)qB2"\n"
g03
flush;
#endif
tree.Mark_Incompletely_Hashed()oG}
}
tree.SetOptimizedUsing(&hB2);hZ}
void
ApplyGrammars(FPoptimizer_CodeTree::dK1&tree){
#ifdef FPOPTIMIZER_MERGED_FILE
#define C *oK1 m21*)&
#else
#define C
#endif
#ifdef DEBUG_SUBSTITUTIONS
qC2
dR3"grammar_optimize_round1\n"
;
#endif
hE
grammar_optimize_round1
mI1
#ifdef DEBUG_SUBSTITUTIONS
qC2
dR3"grammar_optimize_round2\n"
;
#endif
hE
grammar_optimize_round2
mI1
#ifdef DEBUG_SUBSTITUTIONS
qC2
dR3"grammar_optimize_round3\n"
;
#endif
hE
grammar_optimize_round3
mI1
#ifndef FP_ENABLE_SHORTCUT_LOGICAL_EVALUATION
#ifdef DEBUG_SUBSTITUTIONS
qC2
dR3"grammar_optimize_nonshortcut_logical_evaluation\n"
;
#endif
hE
grammar_optimize_nonshortcut_logical_evaluation
mI1
#endif
#ifdef DEBUG_SUBSTITUTIONS
qC2
dR3"grammar_optimize_round4\n"
;
#endif
hE
grammar_optimize_round4
mI1
#ifdef FP_ENABLE_SHORTCUT_LOGICAL_EVALUATION
#ifdef DEBUG_SUBSTITUTIONS
qC2
dR3"grammar_optimize_shortcut_logical_evaluation\n"
;
#endif
hE
grammar_optimize_shortcut_logical_evaluation
mI1
#endif
#ifdef FP_ENABLE_IGNORE_IF_SIDEEFFECTS
#ifdef DEBUG_SUBSTITUTIONS
qC2
dR3"grammar_optimize_ignore_if_sideeffects\n"
;
#endif
hE
grammar_optimize_ignore_if_sideeffects
mI1
#endif
#ifdef DEBUG_SUBSTITUTIONS
qC2
dR3"grammar_optimize_abslogical\n"
;
#endif
hE
grammar_optimize_abslogical
mI1
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
qV
using
namespace
a6;using
w72;using
namespace
FPoptimizer_Optimize;namespace{bool
TestImmedConstraints
aK
bitmask,aH
tree){switch(bitmask&ValueMask){case
Value_AnyNum:case
ValueMask:qA
dW1:if(tree.GetEvennessInfo()!=g5
IsAlways)hZ
qA
Value_OddInt:if(tree.GetEvennessInfo()!=g5
IsNever)hZ
qA
gP1:if(!tree.IsAlwaysInteger(true))hZ
qA
Value_NonInteger:if(!tree.IsAlwaysInteger(mY2)hZ
qA
dX1:if(!tree.qX1)hZ
q41
SignMask){case
Sign_AnySign:qA
aR:if(!aI
true))hZ
qA
Sign_Negative:if(!aI
mY2)hZ
qA
Sign_NoIdea:if(aI
true))hZ
if(aI
mY2)hZ
q41
OnenessMask){case
Oneness_Any:case
OnenessMask:qA
Oneness_One:if
h92
if(!a22
fabs(tree
hF1),1.0))hZ
qA
h91:if
h92
if(a22
fabs(tree
hF1),1.0))hZ
q41
ConstnessMask){case
Constness_Any:qA
Constness_Const:if
h92
break;}
mN2
gS3
template<oA2
extent,oA2
nbits,typename
d72=oA2
int>struct
nbitmap{private:static
const
oA2
bits_in_char=8;static
const
oA2
d82=(qC3
d72)*bits_in_char)/nbits;d72
data[(extent+d82-1)/d82];public:void
inc(mZ,int
by=1){data[pos(index)]+=by*d72(1<<g12);a72
void
dec(mZ){inc(index,-1);}
int
get(mZ
dM(data[pos(index)]>>g12)&mask();gU3
pos(mZ){mN2
index/d82;gU3
shift(mZ){mN2
nbits*(index%d82);gU3
mask(){mN2(1<<nbits)-1;gU3
mask(mZ){mN2
mask()<<g12
aY2
gK3{int
qW3:8;int
mL2:8;int
wK1:8;int
Immeds:8;nbitmap<VarBegin,2>qV3;gK3(){std::memset(this,0,qC3*this));}
gK3
oK1
gK3&b){std::memcpy(this,&b,qC3
b));}
gK3&wB1=oK1
gK3&b){std::memcpy(this,&b,qC3
b))dQ1*this;}
}
;gK3
CreateNeedList_uncached(d4&o32{gK3
oV
o23
a=0;a<gP3
g22;++a){const
ParamSpec&aS1=gW1
gP3.param_list,a);aV2
aD1
const
h0&param
g32
hW
GroupFunction)++aZ2;else{++oV.qW3;assert(param.data.subfunc_opcode<VarBegin);oV.qV3.inc(param
dP);}
++oV.wK1
o52
case
NumConstant:case
ParamHolder:++o63;++oV.wK1
o52}
mN2
oV;}
gK3&CreateNeedList(d4&o32{typedef
std::map<d4*,gK3>d81;static
d81
gQ1;d81::qY3
i=gQ1.gC2&o32;if(i!=gQ1.dC1&o32
mN2
i->second
dQ1
gQ1.qU3,std::make_pair(&gP3,CreateNeedList_uncached(o32))->second;}
dK1
hO3
oJ1
const
aX2){aV2
NumConstant:{const
d8&param=*oK1
d8*)aS1.second
dQ1
dK1
hN3
dY2);}
dV1
const
gZ&param=*oK1
gZ*)aS1.second
dQ1
info.GetParamHolderValueIfFound
hN3
index);a12
aD1
const
h0&param
g32
dK1
result;result
m6
param
dP);gB3
oU1.aJ2
param.data
g22)o23
a=0;a<param.data
g22;++a
gB1
tmp(hO3(gW1
param
w82
param_list,a),info));result
hD1
tmp);}
result
m7
hY2}
mN2
dK1();}
}
namespace
FPoptimizer_Optimize{bool
IsLogisticallyPlausibleParamsMatch(d4&gP3,aH
tree){gK3
oV(CreateNeedList(o32);size_t
hM3=dN;if(hM3<size_t(oV.wK1)){hZ}
h93
hM3;++a){oA2
opcode=tree
gC
a)gL2();switch(opcode){case
cImmed:if(aZ2>0)--aZ2;else--o63;qA
VarBegin:wK2
case
hA3--o63;break;default:assert(opcode<VarBegin);if(oV.qW3>0&&oV.qV3.get(opcode)>0){--oV.qW3;oV.qV3.dec(opcode);}
else--o63;}
}
if(aZ2>0||oV.qW3>0||o63>0){hZ}
if(gP3.match_type!=AnyParams){if(0||oV.qW3<0||o63<0){hZ}
}
mN2
gS3
aO
TestParam
oJ1
aH
tree
qG3){aV2
NumConstant:{const
d8&param=*oK1
d8
mG1
if
h92
double
imm=tree
hF1;switch
hN3
modulo){case
Modulo_None:qA
Modulo_Radians:imm=fp_mod(imm,mF*2.0);if(imm<0)imm+=mF*2.0;if(imm>mF)imm-=mF*2.0
o52
mN2
a22
imm,param.dY2);}
dV1
const
gZ&param=*oK1
gZ
mG1
if(!gO
mN2
info.SaveOrTestParamHolder
hN3
index,tree);a12
aD1
const
h0&param
g32
hW
GroupFunction){if(!gO
dK1
g11=hO3(aS1,info);
#ifdef DEBUG_SUBSTITUTIONS
DumpHashes(g11)qB2*oK1
void**)&g11.GetImmed(hC1
qC2*oK1
void**)&tree.GetImmed(hC1
DumpHashes(tree)qB2"Comparing "
;q13(g11)qB2" and "
;q13(tree)qB2": "
qB2(g11.IsIdenticalTo(tree)?"true"
:"false"
hC1
#endif
mN2
g11.IsIdenticalTo(tree);}
else{if(!&*start_at){if(!gO
if(oJ!=param
dP)hZ}
mN2
TestParams
hN3
data,tree,start_at,info,mY2;}
}
}
hZ}
struct
mT
o82
MatchInfo
info;mT(aW2,info()wR2
wS2:dR
mT>{public:oU2
wS2
a81
n):hP3
mT>(n){}
}
;struct
wL1
o82
wL1(aW2
wR2
d0:dR
wL1>{public:oA2
trypos;oU2
d0
a81
n):hP3
wL1>(n),trypos(0){}
}
;aO
TestParam_AnyWhere
oJ1
aH
tree
qG3,hR3&used,bool
d92{h8<d0>gP
h22
qM1
d0
aG1
a=gP->trypos;goto
retry_anywhere_2;}
hC2
d0(dN);a=0;}
for(;a<hD3{if(used[a])dD2
retry_anywhere:{aO
r=TestParam(aS1,tree
gC
a)dM3)hQ3
used[a]=true
aL
a);gP->trypos=a
dQ1
aO(true,&*gP);}
}
retry_anywhere_2:if(&*dQ){goto
retry_anywhere;}
}
hZ}
struct
gF1
o82
MatchInfo
info;hR3
used;oU2
gF1
a81
hM3
aW2,info(),used(hM3)wR2
m8:dR
gF1>{public:oU2
m8
a81
n,size_t
m):hP3
gF1>(n,gF1(m)){}
}
;aO
TestParams(d4&g9,aH
tree
qG3,bool
d92{if(g9.match_type!=AnyParams){if(hA1!=dN)hZ}
if(!IsLogisticallyPlausibleParamsMatch(g9,tree)){hZ}
switch(g9.match_type){case
PositionalParams:{h8<wS2>gP
h22
qM1
wS2
aG1
a=hA1-1;goto
aE;}
hC2
wS2(hA1);a=0;}
for(oB{(oV2
info=info;retry_positionalparams:{aO
r=TestParam(oZ
a),tree
gC
a)dM3)hQ3
dD2}
}
aE:if(&*dQ){hD2
a].info;goto
retry_positionalparams;}
if(a>0){--a;goto
aE;}
hD2
0].info;hZ}
if(d92
aJ
info.SaveMatchedParamIndex(a)dQ1
aO(true,&*gP);a12
SelectedParams:case
AnyParams:{h8<m8>gP;hR3
used(dN);hF
oA2>oW2(hA1);hF
oA2>g42(hA1);aJ{const
ParamSpec
aS1=oZ
a);oW2[a]=ParamSpec_GetDepCode(aS1);}
{oA2
b=0;aJ
if(oW2[a]!=0)g42[b++]=a;aJ
if(oW2[a]==0)g42[b++]=a;}
oA2
qM1
m8
aG1
if(hA1==0){a=0;goto
retry_anyparams_4;}
a=hA1-1;goto
d91;}
hC2
m8(hA1,dN);a=0;if(hA1!=0){(*gP)[0].info=info;(*gP)[0].used=used;}
}
for(oB{if(a>0){(oV2
info=info;(oV2
used=used;}
retry_anyparams:{aO
r=TestParam_AnyWhere(oZ
g42[a]),tree
dM3,used,d92
hQ3
dD2}
}
d91:if(&*dQ){hD2
a].info;used=(oV2
used;goto
retry_anyparams;}
dA1:if(a>0){--a;goto
d91;}
hD2
0].info;hZ}
retry_anyparams_4:if(g9.qN!=0){if(!TopLevel||!info.HasRestHolder(g9.qN)gN3
hE2;hE2.aJ2
dN)o23
b=0;b<dN;++b){if(used[b])dD2
hE2.push_back(tree
gC
b));used[b]=true
aL
b);}
if(!info.SaveOrTestRestHolder(g9.qN,hE2)){goto
dA1;}
}
else{aN&hE2=info.GetRestHolderValues(g9.qN)q52
hE2
qH3
a){bool
found=false
o23
b=0;b<dN;++b){if(used[b])dD2
if(hE2[a].IsIdenticalTo(tree
gC
b))){used[b]=true
aL
b);found=true
o52}
if(!found){goto
dA1;}
}
}
}
mN2
aO(true,hA1?&*gP:0);a12
GroupFunction:break;}
hZ}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
#include <algorithm>
#include <assert.h>
namespace
FPoptimizer_Optimize{void
g21
const
ParamSpec&aS1,dK1&tree,aX2,bool
inner=true){aV2
NumConstant:{const
d8&param=*oK1
d8
mG1
tree.SetImmed
hN3
dY2);if(inner)tree.Rehash(mY2
o52
dV1
const
gZ&param=*oK1
gZ
mG1
tree.Become(info.GetParamHolderValue
hN3
index))o52
case
aD1
const
h0&param
g32
tree
m6
param
dP)o23
a=0;a<param.data
g22;++a
gB1
qD3;g21
gW1
param
w82
param_list,a),qD3,info,true);tree
hD1
qD3);}
if(param
w82
qN!=0
gN3
trees(info.GetRestHolderValues(param
w82
qN));tree.AddParamsMove(trees);if(dN==1){assert(tree.GetOpcode()==cAdd dQ3()==cMul dQ3()==cMin dQ3()==cMax dQ3()==cAnd dQ3()==cOr dQ3()==cAbsAnd dQ3()==cAbsOr);tree.Become(tree
qN3}
wG1
dN==0){switch(oJ){case
cAdd:case
cOr:tree=dK1(0.0);qA
cMul:case
cAnd:tree=dK1(1.0);aQ}
}
if(inner)tree
m7
break;}
}
}
void
SynthesizeRule
oK1
dT2
dK1&tree,aX2){switch(rule.ruletype){case
ProduceNewTree:{tree.DelParams();g21
gW1
rule.o21
0),tree,info,mY2
o52
case
ReplaceParams:default:{hF
oA2>list=info.GetMatchedParamIndexes();std::sort(list
mU2
list
h13)gW3
list
qJ3;a-->0;)tree
w51
list[a])o23
a=0;a<rule.repl_param_count;++a
gB1
qD3;g21
gW1
rule.o21
a),qD3,info,true);tree
hD1
qD3);}
break;}
}
}
}
#endif
#ifdef DEBUG_SUBSTITUTIONS
#include <sstream>
#include <cstring>
qV
using
namespace
a6;using
w72;using
namespace
FPoptimizer_Optimize;namespace
a6{void
DumpMatch
oK1
dT2
aH
tree,const
aX2,bool
DidMatch,std::ostream&o){DumpMatch(rule,tree,info,DidMatch?o33"match"
:o33"mismatch"
,o)wG2
DumpMatch
oK1
dT2
aH
tree,const
aX2,const
char*hS3,std::ostream&o){static
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
;o<<hS3<<" (rule "
<<(&rule-grammar_rules)<<")"
<<":\n  Pattern    : "
;{ParamSpec
tmp;tmp.first=SubFunction;h0
tmp2;tmp2.data=rule
a31;tmp.second=h82
tmp2;DumpParam(tmp,o);}
o<<"\n  Replacement: "
;DumpParams(rule.o21
rule.repl_param_count
g52
o<<"  Tree       : "
;q13(tree
g52
if(!std::strcmp(hS3,o33"match"
))DumpHashes(tree,o)q52
hT3
qH3
a){if(!hT3[a].a61)dD2
o<<"           "
<<ParamHolderNames[a]<<" = "
;q13(hT3[a]g52}
q62
hU3
qH3
b){if(!hU3[b
d43)continue
q52
hU3[b
gV3
qH3
a){o<<"         <"
<<b<<"> = "
;q13(hU3[b
gV3[a],o);o
g03
endl;}
}
o
g03
flush;}
}
#endif
#include <list>
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
qV
namespace{bool
gT3
FPoptimizer_CodeTree::dK1&tree){if(tree.Is_Incompletely_Hashed())mN2
true;bool
wM1=false
q52
hD3
wM1|=gT3
tree
gC
a));if(wM1)tree.Mark_Incompletely_Hashed()dQ1
wM1
wG2
dE2
FPoptimizer_CodeTree::dK1&tree){if(tree.Is_Incompletely_Hashed()){h93
hD3
dE2
tree
gC
a));tree
m7}
}
}
w72{dO1
Rehash(bool
constantfolding){if(constantfolding)ConstantFolding();gO3
Sort();gO3
Recalculate_Hash_NoRecursion()wG2
qQ1
Recalculate_Hash_NoRecursion(){fphash_t
aV={Opcode*dS3(0x3A83A83A83A83A0),Opcode*dS3(0x1131462E270012B)}
;Depth=1;switch(Opcode){case
cImmed:if(Value!=0.0){wZ1
crc=crc32::calc(oK1
oA2
char*)&Value,qC3
Value));aV
mX2^=crc|(gG1<<dS3(32))dD1(~gG1)*3)^1234567;}
qA
VarBegin:aV
mX2^=(Var<<24)|(Var>>24)dD1
m01(Var)*5)^2345678;qA
cFCall:case
hA3{wZ1
crc=crc32::calc(oK1
oA2
char*)&Funcno,qC3
Funcno));aV
mX2^=(crc<<24)|(crc>>24)dD1(~gG1)*7)^3456789;}
default:{size_t
o31=0
q52
dX2;++a){if(qK2
a].qS2>o31)o31=qK2
a].qS2;aV
mX2+=(1)*dS3(0x2492492492492492);aV
mX2*=dS3(1099511628211);aV
mX2+=qK2
a]o92
mX2
dD1
3)*dS3(0x9ABCD801357);aV.hash2*=dS3(0xECADB912345)dD1~qK2
a]o92
mX2)^4567890;}
Depth+=o31;}
}
if(Hash!=aV){Hash=aV;m11=0;}
}
dO1
FixIncompleteHashes(){gT3*this);dE2*this);}
}
#endif
#include <cmath>
#include <list>
#include <cassert>
#ifdef FP_SUPPORT_OPTIMIZER
qV
namespace{using
w72;bool
qB1
aH
tree,long
count,const
oH
qP1&w5,oH
aA1&qE3
size_t
max_bytecode_grow_length);}
w72{dO1
SynthesizeByteCode(hF
oA2>&gE,hF
double>&Immed,size_t&stacktop_max){
#ifdef DEBUG_SUBSTITUTIONS
qC2"Making bytecode for:\n"
;mN
#endif
while(RecreateInversionsAndNegations()){
#ifdef DEBUG_SUBSTITUTIONS
qC2"One change issued, produced:\n"
;mN
#endif
FixIncompleteHashes();}
#ifdef DEBUG_SUBSTITUTIONS
qC2"Actually synthesizing, after recreating inv/neg:\n"
;mN
#endif
oH
aA1
synth;SynthesizeByteCode(qE3
o42
Pull(gE,Immed,stacktop_max);}
dO1
SynthesizeByteCode(oH
aA1&qE3
bool
MustPopTemps)a32
qS1
FindAndDup(*this)){mN2;}
gA1
cSec
gB1
g2
cCos);mG
cSin
gB1
g2
cCsc);mG
cCsc
gB1
g2
cSin);mG
cCos
gB1
g2
cSec);mP2.qL
size_t
n_subexpressions_synthesized=SynthCommonSubExpressions(synth);switch(GetOpcode()){dS1
qS1
PushVar(GetVar());qA
cImmed
dT1
dR2);qA
cAdd:case
cMul:case
cMin:case
cMax:case
cAnd:case
cOr:case
cAbsAnd:case
cAbsOr:{gA1
cMul){bool
qK3=false;d3{if
hX
a).IsLongIntegerImmed()){gT1=hB1
GetLongIntegerImmed();dK1
tmp(*this,g5
CloneTag());tmp
w51
a);tmp
m7
if(qB1
tmp,value,oH
AddSequence,qE3
MAX_MULI_BYTECODE_LENGTH)){qK3=true
o52}
}
if(qK3)break;}
int
gH1=0;hR3
done(gM1,mY2;dK1
mH;mH
m6
mB
for(;;){bool
found=false;d3{if(done[a])dD2
if(qS1
IsStackTop
hX
a))){found=true;done[a]=true;hB1
qW
mH
hS1
hX
a));if(++gH1>1){hV3
mH.Rehash(o42
hF2
mH);gH1=gH1-2+1;}
}
}
if(!found)break;}
d3{if(done[a])dD2
hB1
qW
mH
hS1
hX
a));if(++gH1>1){hV3
mH.Rehash(o42
hF2
mH);gH1=gH1-2+1;}
}
if(gH1==0){switch(GetOpcode()){case
cAdd:case
cOr:case
cAbsOr
dT1
0);qA
cMul:case
cAnd:case
cAbsAnd
dT1
1);qA
cMin:case
cMax
dT1
0);q03++gH1;}
assert(n_stacked==1)o52
case
cPow:{aH
p0
q92;aH
p1=h31;if(!p1.IsLongIntegerImmed()||!qB1
p0,p1.GetLongIntegerImmed(),oH
MulSequence,qE3
MAX_POWI_BYTECODE_LENGTH)){p0.qW
p1.qW
hV3
qD2
cIf:case
cAbsIf:{oH
aA1::IfData
mI2;hI2.qW
qS1
SynthIfStep1(mI2,mB
h31.qW
qS1
SynthIfStep2(mI2);hK2.qW
qS1
SynthIfStep3(mI2)o52
wK2
case
hA3{d3
hB1
qW
qL3
aK)gM1);qS1
dN1
GetFuncNo(),0,0)o52
default:{d3
hB1
qW
qL3
aK)gM1)o52}
qS1
hF2*this);if(MustPopTemps&&n_subexpressions_synthesized>0){size_t
top=qS1
GetStackTop();qS1
DoPopNMov(top-1-n_subexpressions_synthesized,top-1);}
}
}
namespace{bool
qB1
aH
tree,long
count,const
oH
qP1&w5,oH
aA1&qE3
size_t
max_bytecode_grow_length){if
gM3!=0){oH
aA1
backup=synth;tree.qW
size_t
bytecodesize_backup=qS1
GetByteCodeSize();oH
qB1
count
qH2
size_t
bytecode_grow_amount=qS1
GetByteCodeSize()-bytecodesize_backup;if(bytecode_grow_amount>max_bytecode_grow_length){synth=backup;hZ}
mN2
gS3
else{oH
qB1
count,w5,synth)oG}
}
}
#endif
#include <cmath>
#include <cassert>
#ifdef FP_SUPPORT_OPTIMIZER
qV
namespace{using
w72;typedef
hF
double>a41;const
struct
PowiMuliType{oA2
opcode_square
h22
opcode_cumulate
h22
opcode_invert
h22
opcode_half
h22
opcode_invhalf;}
iseq_powi={cSqr,cMul,cInv,cSqrt,cRSqrt}
,iseq_muli={wJ1,cAdd,cNeg,wJ1,wJ1}
dE3
hH1
const
PowiMuliType&qM3,const
hF
oA2>&g72,a41&stack)dN3
result=1;while(IP<limit){if
a42
qM3.opcode_square){if(!q51)o72
2;oI
opcode_invert){result=-result;oI
opcode_half){if(q51&&result>0&&((long
w62)%2==0)o72
0.5;oI
opcode_invhalf){if(q51&&result>0&&((long
w62)%2==0)o72-0.5;++IP
dC2
size_t
aI1=IP
dE3
lhs=1.0;if
a42
cFetch){mZ=qQ3;if(index<hP||size_t(index-hP)>=wO1){IP=aI1
o52
lhs=stack[index-hP];goto
g82;}
if
a42
cDup){lhs=result;goto
g82;g82:o41
result);++IP
dE3
subexponent=hH1
qM3
hG1
if(IP>=limit||gE[IP]!=qM3.opcode_cumulate){IP=aI1
o52++IP;stack.pop_back();result+=lhs*subexponent
dC2
break;}
hY2
double
ParsePowiSequence
oK1
hF
oA2>&g72){a41
stack;o41
1.0)dQ1
hH1
iseq_powi
hG1}
double
ParseMuliSequence
oK1
hF
oA2>&g72){a41
stack;o41
1.0)dQ1
hH1
iseq_muli
hG1}
class
CodeTreeParserData{public:oU2
CodeTreeParserData(bool
k_powi):stack(),keep_powi(k_powi){}
void
Eat
a81
hM3,OPCODE
opcode
gB1
hA;hA
m6
opcode);h43
gP3=Pop(hM3);hA
oZ1
o32;if(!keep_powi)switch(opcode){case
cTanh:{dK1
sinh,cosh;sinh
m6
cSinh);sinh
hS1(hA
qN3
sinh
m7
cosh
m6
cCosh);cosh
hD1
hA
qN3
cosh
m7
dK1
m1
aX1(cosh
qO3
dK1(-1.0
gE3
hA
m6
qP3.SetParamMove(0,sinh);hA
hD1
pow)o52
case
cTan:{dK1
sin,cos;sin
m6
cSin);sin
hS1(hA
qN3
sin
m7
cos
m6
cCos);cos
hD1
hA
qN3
cos
m7
dK1
m1
aX1(cos
qO3
dK1(-1.0
gE3
hA
m6
qP3.SetParamMove(0,sin);hA
hD1
pow)o52
case
cPow:{aH
p0=hA
gC
0);aH
p1=hA
gC
1);if(p1
m9
cAdd
gN3
mO2(p1.gM1)q52
p1.gM1;++a
gB1
m1
dL2
p0
qO3
p1
gC
a
gE3
mO2[a].swap(pow);}
hA
m6
qP3
oZ1
aW1;}
break;}
aQ
hA.Rehash(!keep_powi);wN1,mY2;
#ifdef DEBUG_SUBSTITUTIONS
qC2"POP "
<<hM3<<", "
<<FP_GetOpcodeName(opcode)<<"->"
<<FP_GetOpcodeName(hA
gL2())<<": "
dX3
hA
a11
#endif
o41
hA)wG2
EatFunc
a81
hM3,OPCODE
opcode,oA2
funcno
gB1
hA;hA.SetFuncOpcode(opcode,funcno);h43
gP3=Pop(hM3);hA
oZ1
o32;hA.Rehash(mY2;
#ifdef DEBUG_SUBSTITUTIONS
qC2"POP "
<<hM3<<", "
dX3
hA
a11
#endif
wN1);o41
hA)wG2
hV
double
value
gB1
hA(value);wN1);Push(hA)wG2
AddVar
aK
varno
gB1
hA(varno,g5
VarTag());wN1);Push(hA)wG2
h4){o51
1].swap(o51
2])wG2
Dup(){Fetch(wO1-1)wG2
Fetch
a81
which){Push(stack[which]);}
qC1
T>void
Push(T
tree){
#ifdef DEBUG_SUBSTITUTIONS
qC2
dX3
tree
a11
#endif
o41
tree)wG2
PopNMov
a81
target,size_t
source){stack[target]=stack[source]mD1
target+1);}
dK1
PullResult(){clones.clear();dK1
result(stack.back())mD1
wO1-1)dP1
h43
Pop
aK
n_pop
gN3
result(n_pop)o23
n=0;n<n_pop;++n
w62[n].swap(o51
n_pop+n])mD1
wO1-n_pop)dP1
size_t
GetStackTop(dM
wO1;}
private:void
FindClone(dK1&,bool=true){mN2;}
private:h43
stack;std::multimap<fphash_t,dK1>clones;bool
keep_powi;private:CodeTreeParserData
oK1
CodeTreeParserData&);CodeTreeParserData&wB1=oK1
CodeTreeParserData&);}
;struct
IfInfo{dK1
wT2;dK1
thenbranch;size_t
endif_location;}
;}
w72{dO1
GenerateFrom
oK1
hF
oA2>&gE,const
hF
double>&Immed,const
FunctionParser::Data&gQ3,bool
keep_powi
gN3
mK1;mK1.aJ2
gQ3.numVariables)o23
n=0;n<gQ3.numVariables;++n){mK1.push_back(dK1(n+VarBegin,g5
VarTag()));}
GenerateFrom(gE,Immed,gQ3,mK1,keep_powi);}
dO1
GenerateFrom
oK1
hF
oA2>&gE,const
hF
double>&Immed,const
FunctionParser::Data&gQ3,aN&mK1,bool
keep_powi){CodeTreeParserData
sim(keep_powi);hF
IfInfo>w0
gX3
IP=0,DP=0;;++IP){oC2:while(!w0.hW3&&(w0.oP==IP||(IP<gE
qJ3&&gE[IP]==cJump&&w0.dB1.a61))gB1
elsebranch=sim
g92
wU2
w0.back().wT2
wU2
w0.dB1
wU2
elsebranch
mE1
Eat(3,cIf);w0.pop_back();}
if(IP>=gE
qJ3)break
h22
opcode=gE[IP];if((opcode==cSqr||opcode==cDup||opcode==cInv||opcode==cNeg||opcode==cSqrt||opcode==cRSqrt||opcode==cFetch)){size_t
was_ip=IP
dE3
dG2
ParsePowiSequence(gE,IP,w0.hW3?gE
qJ3:w0.oP,sim.gA
1);if
aX!=1.0){sim.hV
mM2
gD
cPow);goto
oC2;}
if(opcode==cDup||opcode==cFetch||opcode==cNeg)dN3
g93
ParseMuliSequence(gE,IP,w0.hW3?gE
qJ3:w0.oP,sim.gA
1);if
qI3!=1.0){sim.hV
factor
gD
cMul);goto
oC2;}
}
IP=was_ip;}
if(mL1>=VarBegin){sim.Push(mK1[opcode-VarBegin]);}
else{switch(mL1){case
cIf:case
cAbsIf:{w0
aG2
w0
qJ3+1);dK1
res(sim
g92));w0.back().wT2.swap(res);w0.oP=gE
qJ3;IP+=2
dC2
case
cJump:{dK1
res(sim
g92));w0.dB1.swap(res);w0.oP=gE[IP+1]+1;IP+=2
dC2
case
cImmed
hG2
Immed[DP++]);qA
cDup:sim.Dup();qA
cNop:qA
cFCall:{oA2
funcno=qQ3;assert(funcno<fpdata.FuncPtrs.size())h22
gP3=gQ3.FuncPtrs
qR3
gP3;sim.EatFunc(gP3,mL1,funcno)o52
case
hA3{oA2
funcno=qQ3;assert(funcno<fpdata.dZ3.size());const
FunctionParserBase<double>&p=*gQ3.dZ3
qR3
parserPtr
h22
gP3=gQ3.dZ3
qR3
gP3;h43
paramlist=sim.Pop(o32;dK1
oD2;oD2.GenerateFrom(p.gO3
gE,p.gO3
Immed,*p.data,paramlist
wU2
oD2)o52
case
cInv
hG2
1
wV2
cDiv);qA
cNeg:oX2
cNeg);break;sim.hV
0
wV2
cSub);qA
cSqr
hG2
2
gD
oZ2
cSqrt
hG2
w01
cRSqrt
hG2-w01
cCbrt
hG2
1.0/3.0
gD
oZ2
cDeg
hG2
CONSTANT_DR
hB3
cRad
hG2
CONSTANT_RD
hB3
cExp:dT
goto
oY2;sim.hV
CONSTANT_E
wV2
oZ2
cExp2:dT
goto
oY2;sim.hV
2.0
wV2
oZ2
cCot:oX2
cTan);dT
m3
cCsc:oX2
cSin);dT
m3
cSec:oX2
cCos);dT
m3
cInt:
#ifndef __x86_64
dT{oX2
cInt)o52
#endif
sim.hV
0.5
hC3
wW2
cFloor);qA
cLog10:oX2
hH2
CONSTANT_L10I
hB3
cLog2:oX2
hH2
CONSTANT_L2I
hB3
cLog2by:sim.h4
wW2
hH2
CONSTANT_L2I
mE1
Eat(3,a51
cHypot
hG2
2
gD
cPow
mE1
h4
mE1
hV
2
gD
cPow
hC3
mE1
hV
w01
cSinCos:sim.Dup(wW2
cSin
mE1
h4
wW2
cCos);qA
cSub:dT{gR3
cSub);g1
cMul
hC3);qA
cRSub:sim.h4);dT{gR3
cSub);g1
cMul
hC3);qA
cDiv:dT{gR3
cDiv);g1
cPow
hB3
cRDiv:sim.h4);dT{gR3
cDiv);g1
cPow
hB3
cAdd:case
cMul:case
cMod:case
cPow:case
cEqual:case
cLess:case
cGreater:case
cNEqual:case
cLessOrEq:case
cGreaterOrEq:case
cAnd:case
cOr:case
cAbsAnd:case
cAbsOr:gR3
g31
qA
cNot:case
cNotNot:case
cAbsNot:case
qY2:oX2
g31
qA
cFetch:sim.Fetch(qQ3);qA
cPopNMov:{oA2
stackOffs_target=qQ3
h22
stackOffs_source=qQ3;sim.PopNMov(stackOffs_target,stackOffs_source)o52
#ifndef FP_DISABLE_EVAL
case
cEval:{size_t
paramcount=gQ3.numVariables;mF1
paramcount,g31
break;}
#endif
default:oY2:h22
funcno=opcode-cAbs;assert(funcno<FUNC_AMOUNT);const
FuncDefinition&func=Functions[funcno];mF1
func.gP3,g31
break;}
}
}
Become(sim
g92));
#ifdef DEBUG_SUBSTITUTIONS
qC2"Produced tree:\n"
;mN
#endif
}
}
#endif
#include <algorithm>
#ifdef FP_SUPPORT_OPTIMIZER
qV
using
w72;
#define FP_MUL_COMBINE_EXPONENTS
#ifdef _MSC_VER
#include <float.h>
#define isinf(x) (!_finite(x))
#endif
namespace{bool
IsLogicalTrueValue
oK1
mO
p
a1&&p.min>=0.5)mN2
true;if(!abs&&p
gI&&p.max<=-0.5)mN2
true;hZ}
bool
IsLogicalFalseValue
oK1
mO
abs)mN2
p
gI&&p.max<0.5;else
mN2
p
a1&&p
gI&&p.min>-0.5&&p.max<0.5;}
int
GetLogicalValue
oK1
mO
IsLogicalTrueValue(p,abs))mN2
1;if(IsLogicalFalseValue(p,abs))mN2
0
dQ1-1;}
struct
ComparisonSet{wT
hY3=0x1;wT
Eq_Mask=0x2;wT
Le_Mask=0x3;wT
hZ3=0x4;wT
Ne_Mask=0x5;wT
Ge_Mask=0x6;static
int
Swap_Mask(int
m){mN2(m&Eq_Mask)|((m&hY3)?hZ3:0)|((m&hZ3)?hY3:0);}
struct
Comparison{dK1
a;dK1
b;int
wJ;}
;hF
Comparison>gQ;struct
Item{dK1
value;bool
oE2;}
;hF
Item>hI1;int
g51;enum
gX1{Ok,BecomeZero,dA2,mL
qF3
aJ1{cond_or,w02,w12,w22}
;ComparisonSet():gQ(),hI1(),g51(0){}
gX1
AddItem(aH
a,bool
oE2,aJ1
type){for
a81
c=0;c<hI1
qH3
c)if(hI1[c].value.IsIdenticalTo(a)){if(oE2!=hI1[c].oE2){mP
mN2
dA2;case
w22:hI1.erase(hI1.begin()+c);g51
d03
case
w02:case
w12:gY1}
}
g61}
Item
pole;pole.value=a;pole.oE2=oE2;hI1.push_back(pole)dQ1
Ok;}
gX1
AddRelationship(dK1
a,dK1
b,int
w11,aJ1
type){mP
qS3
7)mN2
dA2;qA
w22:qS3
7){g51
d03}
qA
w02:case
w12:qS3
0)gY1
break;}
if(!(a
o92<b
o92)){a.swap(b);w11=Swap_Mask(w11);}
for
a81
c=0;c<gQ
qH3
c){if(gQ[c].a.IsIdenticalTo(a)&&gQ[c].b.IsIdenticalTo(b)){mP{int
qT3=wP1|w11;if(qT3==7)mN2
dA2;wP1=qT3
o52
case
w02:case
w12:{int
qT3=wP1&w11;if(qT3==0)gY1
wP1=qT3
o52
case
w22:{int
newrel_or=wP1|w11;int
gA2=wP1&w11;if
gB2
5&&gA2==0){wP1=Ne_Mask;g61}
if
gB2
7&&gA2==0){g51+=1;gQ.erase(gQ.begin()+c);g61}
if
gB2
7&&gA2==Eq_Mask){wP1=Eq_Mask;g51
d03}
dD2}
}
g61}
}
Comparison
comp;comp.a=a;comp.b=b;comp.wJ=w11;gQ.push_back(comp)dQ1
Ok
aY2
o81{struct
gZ1{dK1
value;dK1
factor;bool
oC;gZ1():value()d13(),oC(mY2{}
gZ1(aH
v,aH
f):value(v)d13(f),oC(mY2{}
}
;std::multimap<fphash_t,gZ1>a7;enum
g71{Ok,mL}
;typedef
std::multimap<fphash_t,gZ1>::qY3
g81;g81
FindIdenticalValueTo(aH
value){fphash_t
hash=value
o92;for(g81
i=a7.gC2
hash);i!=a7.dC1
hash;++i){h61.IsIdenticalTo(i
dB2
value))mN2
i;}
mN2
a7
h13;}
bool
Found
oK1
g81&b){mN2
b!=a7
h13;}
g71
AddCollectionTo(aH
factor,const
g81&into_which){gZ1&c=into_which->second;if(c.oC)c.factor
hS1
qI3);else{dK1
add;add
m6
cAdd);add
hD1
c.factor);add
hS1
qI3);c.factor.swap(add);c.oC=gS3
g61}
g71
mM1
aH
value,aH
factor){const
fphash_t
hash=value
o92;g81
i=a7.gC2
hash);for(;i!=a7.dC1
hash;++i){if(i
dB2
value.IsIdenticalTo(value))mN2
AddCollectionTo
qI3,i);}
a7.qU3,std::make_pair(hash,gZ1(value
d13)))dQ1
Ok;}
g71
mM1
aH
a){mN2
mM1
a,dK1(1.0))aY2
Select2ndRev{qC1
T>inline
mT2()oK1
T&d23
T&d33
second>b.second
aY2
wX2{qC1
dE
const
qD1
d23
qD1
d33
first<b.first;}
qC1
dE
const
qD1
a,T1
d33
first<b;}
qC1
dE
T1
d23
qD1
b
dM
a<b.first;}
}
;bool
mV2
double
v){mN2
g41
v)&&((long)v%2)==0;}
struct
ConstantExponentCollection{typedef
qE1
gD2;hF
gD2>data;void
MoveToSet_Unique(double
wY2
h43&o61){data.push_back(qE1
aX,h43()));data.back().second.swap(o61)wG2
MoveToSet_NonUnique(double
wY2
h43&o61){hF
gD2>::qY3
i=std::gC2
data
mU2
data
h13,wY2
wX2());if(i!=data.dC1
mM2){i
dB2
qU3
dB2
end(),o61
mU2
o61
h13);}
else{data.qU3,qE1
aX,o61));}
}
bool
Optimize(){bool
a21
false;std::sort(data
mU2
data
h13,wX2());redo:h93
data
qH3
a)dN3
exp_a=data[a
d43;if(a22
exp_a,1.0))dD2
for
a81
b=a+1;b<data
qH3
b)dN3
exp_b=data[b
d43
dE3
gE2=exp_b-exp_a;if(gE2>=fabs(exp_a))break
dE3
exp_diff_still_probable_integer=gE2*16.0;if(g41
exp_diff_still_probable_integer)&&!(g41
exp_b)&&!g41
gE2))gN3&a_set=data[a
gV3;h43&b_set=data[b
gV3;
#ifdef DEBUG_SUBSTITUTIONS
qC2"Before ConstantExponentCollection iteration:\n"
;oF2
cout);
#endif
if(g41
exp_b)&&mV2
exp_b)&&!mV2
gE2+exp_a)gB1
tmp2;tmp2
m6
cMul);tmp2
oZ1
b_set);tmp2
m7
g8
cAbs);tmp.mC);tmp
m7
b_set
aG2
1);b_set[0].oO2}
a_set.insert(a_set
h13,b_set
mU2
b_set
h13);h43
b_copy=b_set;data.erase(data.begin()+b);MoveToSet_NonUnique(gE2,b_copy);a21
true;
#ifdef DEBUG_SUBSTITUTIONS
qC2"After ConstantExponentCollection iteration:\n"
;oF2
cout);
#endif
goto
redo;}
}
}
mN2
changed;}
#ifdef DEBUG_SUBSTITUTIONS
void
oF2
ostream&out){h93
data
qH3
a){out.precision(12);out<<data[a
d43<<": "
;q62
data[a
gV3
qH3
b){if(b>0)out<<'*';q13(data[a
gV3[b],out);}
out
g03
endl;}
}
#endif
}
;struct
q5{enum
dF2{aU1=0,hO1=1,oH2=2,m72=3,MakeNotNotP0=4,MakeNotNotP1=5,MakeNotP0=6,MakeNotP1=7,Unchanged=8
qF3
mN1{Never=0,Eq0=1,Eq1=2,Gt0Le1=3,Ge0Lt1=4}
;dF2
if_identical;dF2
mO1
4];struct{dF2
what:4;mN1
when:4;}
wQ1,wR1,wS1,wT1;dF2
Analyze(aH
a,aH
b)a32
a.IsIdenticalTo(b))mN2
if_identical
mZ2
p0=a
gV
MinMaxTree
p1=b
gV
if(p0
gI&&p1
a1){if(p0.max<p1.min&&mO1
0]dI
0];if(p0.max<=p1.min&&mO1
1]dI
1];}
if(p0
a1&&p1
gI){if(p0.min>p1.max&&mO1
2]dI
2];if(p0.min>=p1.max&&mO1
3]dI
3];}
if(a.qX1){if(wQ1
gY3
wQ1.when,p1))mN2
wQ1.what;if(wS1
gY3
wS1.when,p1))mN2
wS1.what;}
if(b.qX1){if(wR1
gY3
wR1.when,p0))mN2
wR1.what;if(wT1
gY3
wT1.when,p0))mN2
wT1.what;}
mN2
Unchanged;}
static
bool
TestCase(mN1
when,const
MinMaxTree&p){if(!p
a1||!p
gI)hZ
switch(when){case
Eq0:mN2
p.min==0.0&&p.max==p.min;case
Eq1:mN2
p.min==1.0&&p.max==p.max;case
Gt0Le1:mN2
p.min>0&&p.max<=1;case
Ge0Lt1:mN2
p.min>=0&&p.max<1;gK2
hZ}
}
;}
w72{qC1
CondType>aS2
ConstantFolding_LogicCommon(CondType
aW,bool
gF2){bool
should_regenerate=false;ComparisonSet
comp;d3{ComparisonSet::gX1
mP1
ComparisonSet::Ok;aH
atree=wB2
a);switch(atree
gL2()){case
cEqual
qA2
Eq_Mask,aW);qA
cNEqual
qA2
Ne_Mask,aW);qA
cLess
qA2
hY3,aW);qA
cLessOrEq
qA2
Le_Mask,aW);qA
cGreater
qA2
hZ3,aW);qA
cGreaterOrEq
qA2
Ge_Mask,aW);qA
cNot:mP1
comp.aK1
gC
0),true,aW);qA
cNotNot:mP1
comp.aK1
gC
0),false,aW);break;default:if(gF2||atree.qX1)mP1
comp.aK1,false,aW);}
switch(change){ReplaceTreeWithZero:data=new
CodeTreeData(0.0)oG
ReplaceTreeWithOne:data=new
CodeTreeData(1.0)oG
mU
Ok:qA
ComparisonSet::BecomeZero:goto
mV
mU
dA2:aF
mU
mL:h11
break;}
}
if(should_regenerate){
#ifdef DEBUG_SUBSTITUTIONS
qC2"Before ConstantFolding_LogicCommon: "
hR1
#endif
if(gF2){DelParams();}
else{for
a81
a=gM1;a-->0;){aH
atree=wB2
a);if(atree.qX1)oG1}
h93
comp.hI1
qH3
a){if(comp.hI1[a].oE2
oG2
cNot);r.dF
r.w21
wG1!gF2
oG2
cNotNot);r.dF
r.w21
else
dF}
h93
comp.gQ
qH3
a
oG2
cNop);switch(comp.gQ[a].wJ){mU
hY3:r
m6
cLess
w1
Eq_Mask:r
m6
cEqual
w1
hZ3:r
m6
cGreater
w1
Le_Mask:r
m6
cLessOrEq
w1
Ne_Mask:r
m6
cNEqual
w1
Ge_Mask:r
m6
cGreaterOrEq)o52
r
hD1
comp.gQ[a].a);r
hD1
comp.gQ[a].b);r.w21
if(comp.g51!=0)dL2
dK1(double(comp.g51)));
#ifdef DEBUG_SUBSTITUTIONS
qC2"After ConstantFolding_LogicCommon: "
hR1
#endif
mN2
gS3
hZ}
aS2
ConstantFolding_AndLogic()g7
w02,true);}
aS2
ConstantFolding_OrLogic()g7
cond_or,true);}
aS2
ConstantFolding_AddLogicItems()g7
w22,mY2;}
aS2
ConstantFolding_MulLogicItems()g7
w12,mY2;}
static
dK1
qN1
dK1&value,bool&h5){switch(value
gL2()){case
cPow:{dK1
dG2
value
gC
1);value
qO1
mN2
mM2;a12
cRSqrt:value
qO1
h5=true
dQ1
dK1(-0.5);case
cInv:value
qO1
h5=true
dQ1
dK1(-1.0);aQ
mN2
dK1(1.0);}
static
void
o71
o81&mul,aH
tree,aH
factor,bool&h01
bool&h5){h93
dN;++a
gB1
value(tree
gC
a));dK1
mM2(qN1
value,h5));if(!factor
oM1||factor
hF1!=1.0
gB1
dE1;dE1
m6
cMul
h03
aX
h03
qI3);dE1
m7
mM2.swap(dE1);}
#if 0 /* FIXME: This does not work */
h61
m9
cMul){if(1){bool
exponent_is_even=mM2
oM1&&IsEvenIntegerConst
aX
hF1);q62
value.gM1;++b){bool
tmp=false;dK1
val(value
gC
b));dK1
exp(qN1
val,tmp));if(exponent_is_even||(exp
oM1&&mV2
exp
hF1))gB1
dE1;dE1
m6
cMul
h03
aX);dE1
hD1
exp);dE1.ConstantFolding();if(!dE1
oM1||!mV2
dE1
hF1)){goto
cannot_adopt_mul;}
}
}
}
o71
mul,value,wY2
h01
h5);}
else
cannot_adopt_mul:
#endif
{if(mul.mM1
value,mM2)==o81::mL)h11}
}
}
aS2
ConstantFolding_MulGrouping(){bool
h5=false;bool
should_regenerate=false;o81
mul;o71
mul,*this,dK1(1.0),h01
h5);typedef
std::pair<dK1,h43>o91;typedef
std::multimap<fphash_t,o91>hJ1;hJ1
wM;for(o81::g81
j=mul.a7.begin();j!=mul.a7
h13;++j
gB1&value=j
dB2
value;dK1&dG2
j
dB2
factor;if(j
dB2
oC)mM2
m7
const
fphash_t
oA1=mM2
o92;hJ1::qY3
i=wM.gC2
oA1);for(;i!=wM.dC1
oA1;++i)if(i
dB2
first.IsIdenticalTo
aX)){if(!mM2
oM1||!aL1
hF1,1.0))h11
i
dB2
second.push_back(value);goto
skip_b;}
wM.qU3,std::make_pair(oA1,std::make_pair
aX,h43
a81(1),value))));skip_b:;}
#ifdef FP_MUL_COMBINE_EXPONENTS
ConstantExponentCollection
dF1;for(hJ1::qY3
j,i=wM.begin();i!=wM
h13;i=j){j=i;++j;o91&list=i->second;mW2.qF1
dG2
list.first
hF1;if(!aX==0.0))dF1.MoveToSet_Unique
aX,list.second);wM.erase(i);}
}
if(dF1.Optimize())h11
#endif
if(should_regenerate
gB1
before=*this;before.oY
#ifdef DEBUG_SUBSTITUTIONS
qC2"Before ConstantFolding_MulGrouping: "
;q13(before
hC1
#endif
DelParams();for(hJ1::qY3
i=wM.begin();i!=wM
h13;++i){o91&list=i->second;
#ifndef FP_MUL_COMBINE_EXPONENTS
mW2.qF1
dG2
list.first
hF1;if
aX==0.0)dD2
if(aL1,1.0)){AddParamsMove(list.second)dC2}
#endif
dK1
mul;mul
m6
cMul);mul
oZ1
list.second);mul
m7
if(h5&&list.first
oM1){mW2
hF1==1.0/3.0
gB1
cbrt;cbrt
m6
cCbrt);cbrt.oQ
cbrt.w31
cbrt)oA
0.5
gB1
sqrt;sqrt
m6
cSqrt);sqrt.oQ
sqrt.w31
sqrt)oA-0.5
gB1
rsqrt;rsqrt
m6
cRSqrt);rsqrt.oQ
rsqrt.w31
rsqrt)oA-1.0
gB1
inv;inv
m6
cInv);inv.oQ
inv.w31
inv)dC2}
dK1
m1
oQ
pow
hD1
list.first);pow.w31
pow);}
#ifdef FP_MUL_COMBINE_EXPONENTS
wM.clear()q52
m0
qH3
a)dN3
dG2
m0[a
d43;if(aL1,1.0)){AddParamsMove(m0[a
gV3)dC2
dK1
mul;mul
m6
cMul);mul
oZ1
m0[a
gV3);mul
m7
dK1
m1
oQ
pow
hS1(dK1
aX));pow.w31
pow);}
#endif
#ifdef DEBUG_SUBSTITUTIONS
qC2"After ConstantFolding_MulGrouping: "
hR1
#endif
mN2!IsIdenticalTo(before);}
hZ}
aS2
ConstantFolding_AddGrouping(){bool
should_regenerate=false;o81
add;d3{if
hX
a)m9
cMul)dD2
if(add.AddCollection
hX
a))==o81::mL)h11}
hR3
mQ1(gM1);size_t
wN=0;d3{aH
mO2=wB2
a);if(mO2
m9
cMul){q62
hE1
gM1;++b){if(mO2
gC
b)oM1)dD2
o81::g81
c=add.FindIdenticalValueTo(mO2
gC
b));if(add.Found(c)gB1
tmp(mO2,g5
CloneTag());tmp
w51
b);tmp
m7
add.AddCollectionTo(tmp,c);h11
goto
done_a;}
}
mQ1[a]=true;wN+=1;done_a:;}
}
if(wN>0){if(wN>1){hF
std::pair<dK1,size_t> >gM;std::multimap<fphash_t,size_t>oB1;bool
wZ2=false;d3
if(mQ1[a]){q62
hB1
gM1;++b){aH
p=wB2
a)gC
b);const
fphash_t
p_hash=p
o92;for(std::multimap<fphash_t,size_t>::const_iterator
i=oB1.gC2
p_hash);i!=oB1.dC1
p_hash;++i){if(gM[i->second
d43.IsIdenticalTo(p)){gM[i->second
gV3+=1;wZ2=true;goto
found_mulgroup_item_dup;}
}
gM.push_back(std::make_pair(p,size_t(1)));oB1.insert(std::make_pair(p_hash,gM
qJ3-1));found_mulgroup_item_dup:;}
}
if(wZ2
gB1
dI2;{size_t
max=0
gX3
p=0;p<gM
qH3
p)if(gM
d63<=1)gM
d63=0;else{gM
d63*=gM[p
d43.qS2;if(gM
d63>max){dI2=gM[p
d43;max=gM
d63;}
}
}
dK1
group_add;group_add
m6
cAdd);
#ifdef DEBUG_SUBSTITUTIONS
qC2"Duplicate across some trees: "
;q13(dI2)qB2" in "
hR1
#endif
d3
if(mQ1[a])q62
hB1
gM1;++b)if(dI2.IsIdenticalTo
hX
a)gC
b))gB1
tmp
hX
a),g5
CloneTag());tmp
w51
b);tmp
m7
group_add
hD1
tmp);mQ1[a]=false
o52
group_add
m7
dK1
group;group
m6
cMul);group
hD1
dI2);group
hD1
group_add);group
m7
add.mM1
group);h11}
}
d3
if(mQ1[a]){if(add.AddCollection
hX
a))==o81::mL)h11}
}
if(should_regenerate){
#ifdef DEBUG_SUBSTITUTIONS
qC2"Before ConstantFolding_AddGrouping: "
hR1
#endif
DelParams();for(o81::g81
j=add.a7.begin();j!=add.a7
h13;++j
gB1&value=j
dB2
value;dK1&coeff=j
dB2
factor;if(j
dB2
oC)coeff
m7
if(coeff
gH3
coeff
hF1==0.0)dD2
if(a22
coeff
hF1,1.0)){aX1(value)dC2}
dK1
mul;mul
m6
cMul);mul
hD1
value);mul
hD1
coeff);mul
m7
oQ}
#ifdef DEBUG_SUBSTITUTIONS
qC2"After ConstantFolding_AddGrouping: "
hR1
#endif
mN2
gS3
hZ}
aS2
ConstantFolding_IfOperations(){for(;;){wC2
cNot){qL2
cIf);hI2.Become
hX
0)qN3
h31.swap
hX
2));}
else
wC2
cAbsNot){qL2
d73;hI2.Become
hX
0)qN3
h31.swap
hX
2));}
else
break;}
wC2
cIf||hI2
m9
cAbsIf
gB1
cond
q92;dK1
m02;m02
d83
m9
cIf?cNotNot:qY2);m02
hJ2
1));m02.ConstantFolding();dK1
m12;m12
d83
m9
cIf?cNotNot:qY2);m12
hJ2
2));m12.ConstantFolding();if(m02
oM1||m12
oM1
gB1
wB;wB
d83.mB
wB
hJ2
1));wB
hW2
wB
hS1
hX
2));wB
m7
dK1
hK1;hK1
d83
gL2(dB(cond
gC
2
dB
hX
1
dB
hX
2));hK1
m7
qL2
cond.mB
SetParam(0,cond
gC
0))hL1
1,wB)hL1
2,hK1)oG}
}
if
hX
1)m9
hK2
gL2()&&hX
1)m9
cIf||h31
m9
d73
gB1&leaf1=h31;dK1&leaf2=hK2;if
mR1
0).hH
0))&&mR1
1).hH
1))||leaf1
gC
2
d53)gB1
wB;wB
m6
mB
wB
hS1
hX
0));wB
hS1
mR1
1));wB
hS1
dJ2
1));wB
m7
dK1
hK1;hK1
m6
GetOpcode(dB
hX
0
dB
mR1
2
dB
dJ2
2));hK1
m7
h21
SetParam(0
aT1
0))hL1
1,wB)hL1
2,hK1)oG}
if
mR1
1).hH
1))&&leaf1
gC
2
d53
gB1
wC;wC
m6
mB
wC.g91
wC
hS1
mR1
0));wC
hS1
dJ2
0));wC
m7
h21
SetParamMove(0,wC)gM2
2
aT1
2))gM2
1
aT1
1))oG}
if
mR1
1
d53&&leaf1
gC
2).hH
1))gB1
dK2;dK2
m6
leaf2
m9
cIf?cNot:cAbsNot);dK2
hS1
dJ2
0));dK2
m7
dK1
wC;wC
m6
mB
wC.g91
wC
hS1
mR1
0));wC
hD1
dK2);wC
m7
h21
SetParamMove(0,wC)gM2
2
aT1
2))gM2
1
aT1
1))oG}
}
MinMaxTree
p
q92
gV
switch(GetLogicalValue(p,GetOpcode()==d73){dB3
Become
hX
1))oG
dA3
Become
hX
2))oG
gK2
dK1&d5=h31;dK1&dJ=hK2;if(d5.IsIdenticalTo(dJ)){Become
hX
1))oG}
const
OPCODE
op1=d5
gL2();const
OPCODE
op2=dJ
gL2();if(op1==op2){if(d5.gM1==1
gB1
aY1;aY1
m6
mB
aY1.g91
aY1
hS1(d5
qN3
aY1
hS1(dJ
qN3
aY1
m7
qL2
op1);DelParams(oL
if(op1==qZ3
cMul
mS1
cAnd
mS1
cOr
mS1
cAbsAnd
mS1
cAbsOr
mS1
cMin
mS1
cMax
gN3
m22;oC1
gM1;a-->0;){for
a81
b=dJ.gM1;b-->0;){if(d5
w3
dJ
gC
b))){if(m22.hW3){d5.oY
dJ.oY}
m22.push_back(d5
gC
a));dJ
w51
b);d5
w51
a)o52}
}
if(!m22.hW3){d5
m7
dJ
m7
q8
op1);SetParamsMove(m22
oL}
}
if(op1==qZ3
cMul||(op1==cAnd&&dJ.qX1)||(op1==cOr&&dJ.qX1)){oC1
hD
d5
w3
dJ)){d5.oY
d5
w51
a);d5
m7
dK1
hM1=dJ;dJ=dK1((op1==qZ3
cOr
oI1
op1)w2
hM1
oL}
if((op1==cAnd
mS1
cOr)&&op2==cNotNot
gB1&m32=dJ
gC
0);oC1
hD
d5
w3
m32)){d5.oY
d5
w51
a);d5
m7
dK1
hM1=m32;dJ=dK1((op1
m42
op1)w2
hM1
oL}
if(op2==cAdd||op2==cMul||(op2==cAnd&&d5.qX1)||(op2==cOr&&d5.qX1)){for
a81
a=dJ.hD
dJ
w3
d5)){dJ.oY
dJ
w51
a);dJ
m7
dK1
hN1=d5;d5=dK1((op2==cAdd||op2
m42
op2)w2
hN1
oL}
if((op2==cAnd||op2==cOr)&&op1==cNotNot
gB1&m52=d5
gC
0)gW3
dJ.hD
dJ
w3
m52)){dJ.oY
dJ
w51
a);dJ
m7
dK1
hN1=m52;d5=dK1((op2
m42
op2)w2
hN1
oL}
hZ}
aS2
ConstantFolding_PowOperations(){qJ&&h31.qF1
gJ2
d93
qR,hY);data=new
CodeTreeData(hM2);hZ}
if
hX
1)oM1&&(float)hY==1.0){Become
hX
0))oG}
qJ&&(float)qR==1.0){data=new
CodeTreeData(1.0);hZ}
qJ&&h31
m9
cMul){bool
gG2=false
dE3
gH2=qR;dK1
mO2=h31
gW3
hE1
hD
mO2
gC
a).qF1
imm=mO2
gC
a)hF1;dN3
mT1=d93
gH2,imm);if(isinf(new_base_immed)||mT1==0.0){break;}
if(!gG2){gG2=true;hE1
oY}
gH2=mT1;hE1
DelParam(a)o52}
if(gG2){mO2
m7
#ifdef DEBUG_SUBSTITUTIONS
qC2"Before pow-mul change: "
hR1
#endif
hI2.Become(dK1(gH2));h31.Become(aW1;
#ifdef DEBUG_SUBSTITUTIONS
qC2"After pow-mul change: "
hR1
#endif
}
}
if
hX
1)oM1&&hI2
m9
cMul)dN3
exponent_immed=hY
dE3
gI2=1.0;bool
gG2=false;dK1&mO2
q92
gW3
hE1
hD
mO2
gC
a).qF1
imm=mO2
gC
a)hF1;dN3
oD1=d93
imm,exponent_immed);if(isinf(new_factor_immed)||oD1==0.0){break;}
if(!gG2){gG2=true;hE1
oY}
gI2*=oD1;hE1
DelParam(a)o52}
if(gG2){mO2
m7
dK1
newpow;newpow
m6
cPow);newpow
oZ1
oU1)w61
w2
newpow);dL2
dK1(gI2))oG}
}
wC2
cPow&&h31
oM1&&hI2
gC
1).qF1
a
q92
gC
1)hF1
dE3
b=hY
dE3
c=a*b;if(mV2
a)&&!mV2
c)gB1
m62;m62
m6
cAbs);m62
hS1
hX
0)qN3
m62
m7
SetParamMove(0,m62);}
else
qK
c));}
hZ}
aS2
ConstantFolding_ComparisonOperations(){static
const
q5
Data[6]={{q5::hO1
oR
wW
hK
wW
Unchanged}
oR
qH1
Eq1}
oR
qI1
Eq1
h53
Eq0
h63
Eq0}
}
oR
aU1
oR
hO1,q5::hK
hO1,q5::Unchanged}
oR
qH1
Eq0}
oR
qI1
Eq0
h53
Eq1
h63
Eq1}
}
oR
aU1
oR
hO1,q5::oH2,q5::wW
aU1
h53
Gt0Le1}
oR
qI1
Ge0Lt1
mA
oR
hO1
oR
hK
hO1,q5::wW
m72
h53
Ge0Lt1}
oR
qI1
Gt0Le1
mA
oR
aU1
oR
wW
wW
hO1,q5::oH2}
oR
qH1
Ge0Lt1
h63
Gt0Le1
mA
oR
hO1
oR
wW
m72,q5::hK
hO1}
oR
qH1
Gt0Le1
h63
Ge0Lt1
mA}
;switch(Data[GetOpcode()-cEqual].Analyze
hX
0),h31)){case
q5::aU1:data=new
CodeTreeData(0.0);h6
hO1:data=new
CodeTreeData(1.0);h6
m72:qL2
cEqual);h6
oH2:qL2
cNEqual);h6
MakeNotNotP0:qL2
cNotNot
qI2
1);h6
MakeNotNotP1:qL2
cNotNot
qI2
0);h6
MakeNotP0:qL2
cNot
qI2
1);h6
MakeNotP1:qL2
cNot
qI2
0);h6
Unchanged:;}
hZ}
aS2
ConstantFolding_Assimilate(){bool
oX1=false
gW3
hD
wB2
a)m9
GetOpcode()){
#ifdef DEBUG_SUBSTITUTIONS
if(!oX1){qC2"Before assimilation: "
hR1
oX1=gS3
#endif
AddParamsMove
hX
a).GetUniqueRef().oU1,a);}
#ifdef DEBUG_SUBSTITUTIONS
if(oX1){qC2"After assimilation:   "
hR1}
#endif
mN2
oX1;}
dO1
ConstantFolding(){
#ifdef DEBUG_SUBSTITUTIONS
qC2"Runs ConstantFolding for: "
hR1
#endif
using
namespace
std;redo:dE3
gJ2
1.0;if(GetOpcode()!=cImmed)dD3
p=o2;if(p
a1&&p
gI&&p.min==p.max
h3
p.min;q9}
if(mY2{ReplaceTreeWithOne:gJ2
1.0;goto
ReplaceTreeWithConstValue;ReplaceTreeWithZero:gJ2
0.0;ReplaceTreeWithConstValue:
#ifdef DEBUG_SUBSTITUTIONS
qC2"Replacing "
;q13(*this);if(IsImmed())qC2"("
g03
hex<<*oK1
uint_least64_t*)&dR2
g03
dec<<")"
qB2" with const value "
<<hM2
qB2"("
g03
hex<<*oK1
uint_least64_t*)&hM2
g03
dec<<")"
qB2"\n"
;
#endif
data=new
CodeTreeData(hM2)dQ1;ReplaceTreeWithParam0:
#ifdef DEBUG_SUBSTITUTIONS
qC2"Before replace: "
hR1
#endif
Become
hX
0));
#ifdef DEBUG_SUBSTITUTIONS
qC2"After replace: "
hR1
#endif
goto
redo;}
switch(GetOpcode()){case
cImmed:qA
VarBegin:qA
cAnd:case
cAbsAnd:{oH1
for
a81
a=gM1;a-->0;)switch(aY
a)qJ1
cAbsAnd))w41
DelParam(a);break;gK2
switch(gM1){dA3
aF
dB3
qL2
GetOpcode()==cAnd?cNotNot:qY2
hA2
default:gA1
cAnd)if(ConstantFolding_AndLogic(hY1
qD2
cOr:case
cAbsOr:{oH1
for
a81
a=gM1;a-->0;)switch(aY
a)qJ1
cAbsOr)){dB3
aF
dA3
DelParam(a);break;gK2
switch(gM1)w41
qL2
GetOpcode()==cOr?cNotNot:qY2
hA2
default:gA1
cOr)if(ConstantFolding_OrLogic(hY1
qD2
cNot:case
cAbsNot:{oA2
g13=0;switch
hX
0)gL2()){case
cEqual
aM
cNEqual;qA
cNEqual
aM
cEqual;qA
cLess
aM
cGreaterOrEq;qA
cGreater
aM
cLessOrEq;qA
cLessOrEq
aM
cGreater;qA
cGreaterOrEq
aM
cLess;qA
cNot
aM
cNotNot;qA
cAbsNot
aM
qY2;qA
qY2
aM
cAbsNot;q03
if(g13){qL2
OPCODE(g13));SetParamsMove
hX
0).GetUniqueRef().oU1
hA2}
switch(aY
0)qJ1
cAbsNot)){dB3
goto
mV
dA3
aF
gK2
gA1
cNot&&hI2.IsAlwaysSigned(true))qL2
cAbsNot);wC2
cIf||hI2
m9
cAbsIf
gB1
w32
q92;aH
ifp1
g23
1);aH
ifp2
g23
2);if(ifp1
m9
cNot||ifp1
gL2
gG
ifp1
m9
cNot?cNotNot:qY2);hN2
qN3
hO2
dG1
mB
p2
g33)oK
if(ifp2
m9
cNot||ifp2
gL2
gG
mB
hN2);hO2
dG1
ifp2
m9
cNot?cNotNot:qY2);p2
g33
gC
0))oK
qD2
cNotNot:case
qY2:{if
hX
0).qX1)o83
switch(aY
0)qJ1
qY2))w41
aF
gK2
gA1
cNotNot&&hI2.IsAlwaysSigned(true))qL2
qY2);wC2
cIf||hI2
m9
cAbsIf
gB1
w32
q92;aH
ifp1
g23
1);aH
ifp2
g23
2);if(ifp1
m9
cNot||ifp1
m9
cAbsNot){SetParam(0,w32
qN3
dL2
ifp1);dG1
mB
p2
g33)oK
if(ifp2
m9
cNot||ifp2
gL2
gG
mB
hN2);hO2
dL2
ifp2);qL2
w32.mB
goto
redo;}
qD2
cIf:case
cAbsIf:{if(ConstantFolding_IfOperations(hY1
break;a12
cMul:{NowWeAreMulGroup:;oH1
double
aZ=1.0;size_t
wU1=0;bool
q61=false;d3{if(!hU1
dD2
double
immed=oE1);if(immed==0.0)goto
mV
aZ*=immed;++wU1;}
if(wU1>1||(wU1==1&&a22
aZ,1.0)))q61=true;if(q61){
#ifdef DEBUG_SUBSTITUTIONS
qC2"cMul: Will add new "
dY3
aZ<<"\n"
;
#endif
for
a81
a=hD
hU1{
#ifdef DEBUG_SUBSTITUTIONS
qC2" - For that, deleting "
dY3
oE1
hC1
#endif
oF1!a22
aZ,1.0))dL2
dK1(aZ));}
switch(gM1){dA3
aF
dB3
o83
default:if(ConstantFolding_MulGrouping(hY1
if(ConstantFolding_MulLogicItems(hY1
qD2
cAdd:{oH1
double
mU1=0.0;size_t
wU1=0;bool
q61=false;d3{if(!hU1
dD2
double
immed=oE1);mU1+=immed;++wU1;}
if(wU1>1||(wU1==1&&mU1==0.0))q61=true;if(q61){
#ifdef DEBUG_SUBSTITUTIONS
qC2"cAdd: Will add new "
dY3
mU1<<"\n"
qB2"In: "
hR1
#endif
for
a81
a=hD
hU1{
#ifdef DEBUG_SUBSTITUTIONS
qC2" - For that, deleting "
dY3
oE1
hC1
#endif
oF1!(mU1==0.0))dL2
dK1(mU1));}
switch(gM1)w41
o83
default:if(ConstantFolding_AddGrouping(hY1
if(ConstantFolding_AddLogicItems(hY1
qD2
cMin:{oH1
size_t
gN2=0
mZ2
oM;d3{while(a+1<gM1&&hB1
IsIdenticalTo
hX
a+1)))DelParam(a+1)mZ2
gJ
gI&&(!oM
gI||p.max<oM.max)){oM.max=p.max;oM
gI=true;gN2=a;}
}
if(oM
gI)for
a81
a=gM1;a-->0;)dD3
gJ
a1&&a!=gN2&&p.min>=oM.max)oF1
gM1==1){o83
qD2
cMax:{oH1
size_t
gN2=0
mZ2
wD;d3{while(a+1<gM1&&hB1
IsIdenticalTo
hX
a+1)))DelParam(a+1)mZ2
gJ
a1&&(!wD
a1||p.min>wD.min)){wD.min=p.min;wD
a1=true;gN2=a;}
}
if(wD
a1){for
a81
a=gM1;a-->0;)dD3
gJ
gI&&a!=gN2&&p.max<wD.min){oG1}
}
if(gM1==1){o83
qD2
cEqual:case
cNEqual:case
cLess:case
cGreater:case
cLessOrEq:case
cGreaterOrEq:if(ConstantFolding_ComparisonOperations(hY1
if
hX
1)oM1)switch
hX
0)gL2()){case
cAsin:qK
fp_sin
hX
1
aH2
cAcos:qK
fp_cos
hX
1)hF1)));qL2
GetOpcode()==cLess?cGreater
q72
cLessOrEq?cGreaterOrEq
q72
cGreater?cLess
q72
cGreaterOrEq?cLessOrEq:mB
goto
redo;case
cAtan:qK
fp_tan
hX
1
aH2
cLog:qK
fp_exp
hX
1
aH2
cSinh:qK
fp_asinh
hX
1
aH2
cTanh:if(fabs
hX
1)hF1)<1.0){qK
fp_atanh
hX
1).dG}
q03
qA
cAbs:dD3
p0
q92
gV
if(p0
oO1
o83
if(p0
wU{qL2
cMul);dL2
dK1(-1.0));goto
NowWeAreMulGroup;}
wC2
cMul){aH
p
q92;h43
m82;h43
hQ2
q52
p.gM1;++a){p0=p
gC
a)gV
if(p0
oO1{m82.push_back
o93
if(p0
wU{hQ2.push_back
o93}
#ifdef DEBUG_SUBSTITUTIONS
qC2"Abs: mul group has "
<<m82
qJ3<<" pos, "
<<hQ2
qJ3<<"neg\n"
;
#endif
if(!m82.hW3||!hQ2.hW3){
#ifdef DEBUG_SUBSTITUTIONS
qC2"AbsReplace-Before: "
;q13(*this)qB2"\n"
g03
flush;DumpHashes
q82
#endif
dK1
o73;o73
m6
cMul)q52
p.gM1;++a){p0=p
gC
a)gV
if((p0
oO1||(p0
wU){}
else
o73
hS1
o93
o73
m7
dK1
m92;m92
m6
cAbs);m92
hD1
o73);m92
m7
dK1
m5
cMul);mO2
hD1
m92);hE1
AddParamsMove(m82);if(!hQ2.hW3){if(hQ2
qJ3%2)hE1
dL2
dK1(-1.0));hE1
AddParamsMove(hQ2);}
Become(aW1;
#ifdef DEBUG_SUBSTITUTIONS
qC2"AbsReplace-After: "
;q13
q82
qC2"\n"
g03
flush;DumpHashes
q82
#endif
goto
NowWeAreMulGroup;}
}
break;}
#define HANDLE_UNARY_CONST_FUNC(funcname) qJ h3 funcname(qR);q9
case
cLog:dP3(fp_log);wC2
cPow
gB1
pow
q92;if(pow
gC
0).IsAlwaysSigned(true)){pow.oY
pow
m6
cLog)w61
mA2
if(pow
gC
1).IsAlwaysParity(mY2){pow.oY
dK1
abs;abs
m6
cAbs);abs
hD1
pow
qN3
abs
m7
pow
m6
cLog)w61;pow.SetParamMove(0,abs)mA2}
else
wC2
cAbs
gB1
pow
q92
gC
0);if(pow
m9
cPow){pow.oY
dK1
abs;abs
m6
cAbs);abs
hD1
pow
qN3
abs
m7
pow
m6
cLog)w61;pow.SetParamMove(0,abs)mA2}
qA
cAcosh:dP3(fp_acosh);qA
cAsinh:dP3(fp_asinh);qA
cAtanh:dP3(fp_atanh);qA
cAcos:dP3(fp_acos);qA
cAsin:dP3(fp_asin);qA
cAtan:dP3(fp_atan);qA
cCosh:dP3(fp_cosh);qA
cSinh:dP3(fp_sinh);qA
cTanh:dP3(fp_tanh);qA
cSin:dP3(fp_sin);qA
cCos:dP3(fp_cos);qA
cTan:dP3(fp_tan);qA
cCeil:qS
dP3(fp_ceil);qA
cTrunc:qS
dP3(fp_trunc);qA
cFloor:qS
dP3(fp_floor);qA
cInt:qS
dP3(fp_int);qA
cCbrt:dP3(fp_cbrt);qA
cSqrt:dP3(fp_sqrt);qA
cExp:dP3(fp_exp);qA
cLog2:dP3(fp_log2);qA
cLog10:dP3(fp_log10);qA
cLog2by:gH
fp_log2(qR)*hY
a02
cMod:gH
fp_mod
wH2
qA
cAtan2:dD3
p0
oW1
p1=h31
gV
qJ&&a22
qR,0.0)){if(p1
gI&&p1.max<0
h3
o3;q9
if(p1
a1&&p1.min>=0.0
h3
0.0;q9}
if
hX
1)oM1&&FloatEqual
hX
1)hF1,0.0)){if(p0
gI&&p0.max<0
h3-CONSTANT_PIHALF;q9
if(p0
a1&&p0.min>0
h3
CONSTANT_PIHALF;q9}
gH
fp_atan2
wH2
if((p1
a1&&p1.min>0.0)||(p1
gI&&p1.max<NEGATIVE_MAXIMUM)gB1
gO2;gO2
m6
cPow);gO2.aX1
hX
1));gO2
hS1(dK1(-1.0));gO2
m7
dK1
gP2;gP2
m6
cMul);gP2.g91
gP2
hD1
gO2);gP2
m7
qL2
cAtan)hL1
0,gP2
qI2
1);qD2
cPow:{if(ConstantFolding_PowOperations(hY1
break;a12
cDiv:qJ&&h31
oM1&&hY!=0.0
h3
qR/hY
a02
cInv:qJ&&qR!=0.0
h3
1.0/qR
a02
cSub:gH
qR-hY
a02
cNeg:qJ
h3-qR
a02
cRad
dC3
CONSTANT_RD
a02
cDeg
dC3
CONSTANT_DR
a02
cSqr
dC3
qR
a02
cExp2:dP3(fp_exp2);qA
cRSqrt:qJ
h3
1.0/fp_sqrt(qR)a02
cCot:qJ){hR2
fp_tan(qR);m4
cSec:qJ){hR2
fp_cos(qR);m4
cCsc:qJ){hR2
fp_sin(qR);m4
cHypot:gH
fp_hypot
wH2
qA
cRDiv:case
cRSub:case
cDup:case
cFetch:case
cPopNMov:case
cSinCos:case
cNop:case
cJump:qA
hA3
wK2
case
cEval:break;}
}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
qV
using
w72;w72
dD3
g5
o2
const
#ifdef DEBUG_SUBSTITUTIONS_extra_verbose
dD3
tmp=CalculateResultBoundaries_do()qB2"Estimated boundaries: "
;if(tmp
a1)qC2
tmp.min;else
qC2"-inf"
qB2" .. "
;if(tmp
gI)qC2
tmp.max;else
qC2"+inf"
qB2": "
;FPoptimizer_CodeTree::q13(*this)qB2
std::endl
dQ1
tmp;}
MinMaxTree
g5
CalculateResultBoundaries_do()const
#endif
{using
namespace
std;switch(GetOpcode()){case
cImmed:qV1
dR2,dR2);case
cAnd:case
cAbsAnd:case
cOr:case
cAbsOr:case
cNot:case
cAbsNot:case
cNotNot:case
qY2:case
cEqual:case
cNEqual:case
cLess:case
cLessOrEq:case
cGreater:case
cGreaterOrEq:{qV1
0.0,1.0);a12
cAbs
qB
bool
spans_across_zero=(!hS||m.min<0.0)&&(!m
gI||w42>=0.0);gQ2
fabs
oW
fabs(w42);if(hS&&m
gI&&m.min>w42)std::swap
oY1,w42);if(spans_across_zero){if(!hS)m
gR2;m.min=0.0;hS=gS3
mN2
m;a12
cLog
qB
if
o7
fp_log
oY1);}
hR
fp_log
qU1
cLog2
qB
if
o7
fp_log2
oY1);}
hR
fp_log2
qU1
cLog10
qB
if
o7
fp_log10
oY1);}
hR
fp_log10
qU1
cAcosh
qB
if(hS){if
oY1<=1.0)hS=false;else
m.min=fp_acosh
oY1);}
w52{if(w42<=1.0)m
gR2;else
w42=fp_acosh
qU1
cAsinh
qB
gQ2
fp_asinh
oW
fp_asinh(o8
cAtanh
qB
gQ2
fp_atanh
oW
fp_atanh(o8
cAcos
qB
qV1
m
gI?fp_acos(w42):0.0,hS?fp_acos
oY1):o3);a12
cAsin
qB
qV1
hS?fp_asin
oY1):dH1
m
gI?fp_asin(w42):dI1
a12
cAtan
qB
qV1
hS?fp_atan
oY1):dH1
m
gI?fp_atan(w42):dI1
a12
cAtan2:dD3
p0
oW1
p1=h31
gV
qJ&&a22
qR,0.0)){qV1
0.0,o3);}
if
hX
1)oM1&&FloatEqual
hX
1)hF1,0.0)){qV1
dH1
dI1}
qV1-o3,o3);a12
cSin
qB
bool
covers_full_cycle=!hS||!m
gI||(w42-m.min)>=(hS2
covers_full_cycle)aA2
double
min=fmod
oY1,hS2
min<0)min+=2.0*o3
dE3
max=fmod(w42,hS2
max<0)max+=2.0*o3;if(max<min)max+=2.0*o3;bool
gS2=(min<=CONSTANT_PIHALF&&max>=dI1
bool
aM1=(min<=1.5*o3&&max>=1.5*o3);if(gS2&&aM1)aA2
if(aM1)qV1-1.0,fp_max(wE);if(gS2)qV1
fp_min(wE,1.0);qV1
fp_min(wE,fp_max(wE);a12
cCos:{aA2
a12
cTan:{qV1);a12
cCeil
qB
w52
w42=oI2
cFloor
qB
gQ2
g43
oY1)dQ1
m;a12
cTrunc
qB
gQ2
g43
oW
oI2
cInt
qB
m.min=g43
oY1);w42=oI2
cSinh
qB
gQ2
fp_sinh
oW
fp_sinh(o8
cTanh
qB
gQ2
fp_tanh
oY1);else{hS=true;m.min=-1.0;}
w52
w42=fp_tanh(w42);else{m
gI=true;w42=1.0;}
mN2
m;a12
cCosh
qB
if(hS){w52{if
oY1>=0.0&&w42>=0.0){m.min=dV}
else
if
oY1<0.0&&w42>=0.0){hR2
dV
if(tmp>w42)w42=tmp
hT2}
else{m.min=dV
std::swap
oY1,w42);}
}
else{if
oY1>=0.0){m
gR2;m.min=fp_cosh
oY1);}
else{m
gR2
hT2}
}
}
else{hS=true
hT2
w52{m.min=fp_cosh(w42);m
gR2;}
else
m
gR2;}
mN2
m;a12
cIf:case
cAbsIf:dD3
res1=h31
gV
MinMaxTree
res2=hK2
gV
if(!res2
a1)res1
a1=false;wG1
res1
a1&&res2.min<res1.min)res1.min=res2.min;if(!res2
gI)res1
gR2;wG1
res1
gI&&res2.max>res1.max)res1.max=res2.max
dQ1
res1;a12
cMin:{bool
mI=false;bool
mJ=false
mZ2
result;hG
m=hL2!hS)mI=true
oQ1
a1||m.min<dF3
w62.min=m.min;if(!m
gI)mJ=true
oQ1
gI||w42<gB3
max
w62.max=w42;}
if(mI
w62
a1=false;if(mJ
w62
gR2
dP1
case
cMax:{bool
mI=false;bool
mJ=false
mZ2
result;hG
m=hL2!hS)mI=true
oQ1
a1||m.min>dF3
w62.min=m.min;if(!m
gI)mJ=true
oQ1
gI||w42>gB3
max
w62.max=w42;}
if(mI
w62
a1=false;if(mJ
w62
gR2
dP1
case
cAdd:dD3
result(0.0,0.0);hG
item=hL2
item
a1
w62.min+=item.min
wV1
a1=false;if(item
gI
w62.max+=item.max
wV1
gR2;if(!result
a1&&!result
gI)break;}
if
hP2
a1&&result
gI&&dF3>gB3
max)std::swap
hP2.min,gB3
max)dP1
case
cMul:{struct
Value{enum
mB2{oJ2,wW1,oK2}
;mB2
oS
dE3
value;Value(mB2
t):oS(t),value(0){}
Value(double
v):oS(oJ2),value(v){}
bool
hU2
dM
oS==wW1||(oS==oJ2&&value<0.0)wG2
wB1*=oK1
Value&rhs){if(oS==oJ2&&rhs.oS==oJ2)value*=rhs.value;else
oS=(hU2)!=rhs.hU2)?wW1:oK2);}
mT2<oK1
Value&rhs
dM(oS==wW1&&rhs.oS!=wW1)||(oS==oJ2&&(rhs.oS==oK2||(rhs.oS==oJ2&&value<rhs.value)))aY2
gI1{Value
gT2,gU2;gI1():gT2(Value::oK2),gU2(Value::wW1){}
void
aV1
Value
g53,const
Value&value2){g53*=value2;if(g53<gT2)gT2=g53;if(gU2<g53)gU2=g53;}
}
mZ2
result(aD2
hG
item=hL2!item
a1&&!item
gI)qV1
o43
mC2=result
a1?Value
hP2.min)hV1
wW1
o43
mD2=result
gI?Value
hP2.max)hV1
oK2
o43
mE2=item
a1?Value(item.min)hV1
wW1
o43
mF2=item
gI?Value(item.max)hV1
oK2);gI1
range;range.aV1
mC2,mE2
aE2
mC2,mF2
aE2
mD2,mE2
aE2
mD2,mF2);if(range.gT2.oS==Value::oJ2
w62.min=range.gT2.value
wV1
a1=false;if(range.gU2.oS==Value::oJ2
w62.max=range.gU2.value
wV1
gR2;if(!result
a1&&!result
gI)break;}
if
hP2
a1&&result
gI&&dF3>gB3
max)std::swap
hP2.min,gB3
max)dP1
case
cMod:dD3
x
oW1
y=h31
gV
if(y
gI){if(y.max>=0.0){if(!x
a1||x.min<0)qV1-y.max,y.max);else
qV1
0.0,y.max);}
else{if(!x
gI||x.max>=0)qV1
y.max,-y.max);else
qV1
y.max,NEGATIVE_MAXIMUM);}
}
else
qV1);a12
cPow:{if
hX
1)oM1&&hY==0.0){qV1
aD2}
qJ&&qR==0.0){qV1
0.0,0.0);}
qJ&&a22
qR,1.0)){qV1
aD2}
if
hX
1)oM1&&hY>0&&h31.IsAlwaysParity(mY2)dN3
dG2
hY
mZ2
tmp
oW1
result;result
a1=true;dF3=0;if(tmp
a1&&tmp.min>=0
w62.min=d93
tmp.min,oL2
wG1
tmp
gI&&tmp.max<=0
w62.min=d93
tmp.max,oL2
result
gR2;if(tmp
a1&&tmp
gI){result
gI=true;gB3
max=std::max(fabs(tmp.min),fabs(tmp.max));gB3
max=fp_pow
hP2.max,oL2}
hY2
MinMaxTree
p0
oW1
p1=h31
gV
TriTruthValue
p0_positivity=(p0
oO1?IsAlways:(p0
gI&&p0.max<0.0?w92
Unknown);TriTruthValue
hV2=h31.GetEvennessInfo();TriTruthValue
wF=Unknown;switch(p0_positivity)w71
wF=IsAlways;qA
w92{wF=hV2
o52
default:switch(hV2)w71
wF=IsAlways;qA
w92
qA
Unknown:{if
hX
1)oM1&&h31.IsAlwaysInteger(mY2&&hY>=0.0){wF=IsAlways;}
break;}
}
}
switch(wF)w71
dN3
min=0.0;if(p0
a1&&p1
a1){min=pow(p0.min,p1.min);if(p0.min<0.0&&(!p1
gI||p1.max>=0.0)&&min>=0.0)min=0.0;}
if(p0
a1&&p0.min>=0.0&&p0
gI&&p1
gI)dN3
max=pow(p0.max,p1.max);if(min>max)std::swap(min,max);qV1
min,max);}
qV1
min,mY2;a12
w92{qV1
false,NEGATIVE_MAXIMUM);}
default:{break;}
qD2
cNeg:{dG3
hW1-1.0)hX1
qE
cSub
hM
cNeg);tmp2
hW2
tmp
m6
cAdd
hX1);tmp.mC
qE
cInv
dW-1.0)qE
cDiv
hM
cInv);tmp2
hW2
tmp
m6
cMul
hX1);tmp.mC
qE
cRad:{dG3
hX1
hW1
CONSTANT_RD)qE
cDeg:{dG3
hX1
hW1
CONSTANT_DR)qE
cSqr
dW
2.0)qE
cExp:{g8
cPow
hW1
CONSTANT_E)hX1
qE
cExp2:{g8
cPow
hW1
2.0)hX1
qE
cCbrt:dD3
result
q92
gV
if
hP2
a1
w62.min=fp_cbrt
hP2.min);if
hP2
gI
w62.max=fp_cbrt
hP2.max)dP1
case
cSqrt:dD3
result
q92
gV
if
hP2
a1
w62.min=dF3<0?0:fp_sqrt
hP2.min);if
hP2
gI
w62.max=gB3
max<0?0:fp_sqrt
hP2.max)dP1
case
cRSqrt
dW-0.5)qE
cHypot:{dK1
xsqr,ysqr,add,sqrt;xsqr
hS1
hX
0));xsqr
hS1(dK1(2));ysqr
hW2
ysqr
hS1(dK1(2));xsqr
m6
cPow);ysqr
m6
cPow);add
hD1
xsqr);add
hD1
ysqr);add
m6
cAdd);sqrt
hD1
add);sqrt
m6
cSqrt)dQ1
sqrt
gV
a12
cLog2by
hM
cLog2);hN
cMul);tmp.mC
qX
h31
qE
cCot
hM
cTan)qO2
qE
cSec
hM
cCos)qO2
qE
cCsc
hM
cSin)qO2)dQ1
tmp
gV}
qA
cRDiv:case
cRSub:case
cDup:case
cFetch:case
cPopNMov:case
cSinCos:case
cNop:case
cJump:dS1
qA
hA3
qA
cFCall:qA
cEval:break;}
qV1);}
}
#endif
#ifdef FP_SUPPORT_OPTIMIZER
qV
#if defined(__x86_64) || !defined(FP_SUPPORT_CBRT)
# define CBRT_IS_SLOW
#endif
wI{oX
oA2
char
powi_table[256];}
namespace{using
w72;class
oS1{size_t
gJ1;size_t
wX;size_t
wY;public:oS1():gJ1(0),wX(0),wY(0){}
void
g63
OPCODE
op){gJ1+=1;g73
cCos)++wX;g73
cSin)++wY;g73
cSec)++wX;g73
cCsc)++wY;}
size_t
GetCSEscore()const{size_t
result=gJ1
dP1
int
NeedsSinCos()a32
wX>0&&wY>0){if(gJ1==wX+wY)mN2
1
dQ1
2;}
mN2
0;}
size_t
MinimumDepth()const{size_t
n_sincos=std::min(wX,wY);if(n_sincos==0)mN2
2
dQ1
1;}
}
;typedef
std::multimap<fphash_t,std::pair<oS1,dK1> >aN1;void
FindTreeCounts(aN1&mV1,aH
tree,OPCODE
mW1{aN1::qY3
i=mV1.gC2
tree
o92);bool
found=false;for(;i!=mV1.dC1
tree
o92;++i){if(tree.IsIdenticalTo(i
dB2
second)){i
dB2
first.g63
mW1;found=true
o52}
if(!found){oS1
count;count.g63
mW1;mV1.qU3,std::make_pair(tree
o92,std::make_pair
gM3,tree)));}
h93
hD3
FindTreeCounts(mV1,tree
gC
a),oJ);}
struct
d6{bool
wO;bool
dH;}
;d6
IfBalanceGood(aH
root,aH
g83{if(root.IsIdenticalTo(g83){d6
result={true,true}
dP1
d6
result={true,false}
;if(root
m9
cIf||root
m9
d73{d6
cond
aT
0),g83;d6
d5
aT
1),g83;d6
dJ
aT
2),g83;if(cond.dH||d5.dH||dJ.dH){gB3
dH=gS3
gB3
wO=((d5.dH==dJ.dH)||dU1&&(cond.wO||(d5.dH&&dJ.dH))&&(d5.wO||dU1&&(dJ.wO||dU1;}
else{bool
w81=false;bool
q71=false
gX3
b=root.gM1,a=0;a<b;++a){d6
tmp
aT
a),g83;if(tmp.dH
w62.dH=true;if(tmp.wO==mY2
w81=true;wG1
tmp.dH)q71=gS3
if(w81&&!q71
w62.wO=false;}
hY2
bool
IsOptimizableUsingPowi(long
immed,long
penalty=0){oH
aA1
synth;qS1
PushVar(0);size_t
bytecodesize_backup=qS1
GetByteCodeSize();oH
qB1
immed,oH
MulSequence,synth);size_t
bytecode_grow_amount=qS1
GetByteCodeSize()-bytecodesize_backup
dQ1
bytecode_grow_amount<size_t(MAX_POWI_BYTECODE_LENGTH-penalty)wG2
ChangeIntoRootChain(dK1&tree,bool
wA2,long
oM2,long
oN2){while(oN2>0){g8
cCbrt);tmp.gK1
tmp
m7
tree.oO2--oN2;}
while(oM2>0){g8
cSqrt);if(wA2){tmp
m6
cRSqrt);wA2=false;}
tmp.gK1
tmp
m7
tree.oO2--oM2;}
if(wA2){g8
cInv);tmp.gK1
tree.oO2}
}
double
qN2
long
h7){static
std::map<long
h33
mE;std::map<long
h33::qY3
i=mE.gC2
h7);if(i!=mE.dC1
h7)mN2
i->second;std::pair<long
h33
result(h7,0.0)dE3&cost=gB3
second;while(h7>1){int
g93
0;if(h7<256){g93
oH
powi_table[h7];if
qI3&128)factor&=127;else
g93
0;if
qI3&64)g93-qI3&63)-1;}
if
qI3){cost+=qN2
factor);h7/=factor
dC2
if(!(h7&1)){h7/=2;cost+=3;}
else{cost+=3.5;h7-=1;}
}
mE.qU3,result)dQ1
cost;}
struct
wX1{static
const
oA2
MaxSep=4;struct
gV2{gV2():n_int_sqrt(0),n_int_cbrt(0),resulting_exponent(0),sep_list(){}
int
n_int_sqrt;int
n_int_cbrt;long
resulting_exponent;int
sep_list[MaxSep];}
;gV2
CreatePowiResult(double
mM2)const{static
const
double
hX2(1+4)*(1+3)]={1.0,1.0/(2
dH3
dH3*2
dH3*2*2
qP2
3
qP2
3*2
qP2
3*2*2
qP2
3*2*2*2
qP2
3*2*2*2*2
qP2
3*3)aG
2)aG
2*2)aG
2*2*2)aG
2*2*2*2)aG
3)aG
3*2)aG
3*2*2)aG
3*2*2*2)aG
3*2*2*2*2)}
;gV2
result;int
o4=dJ1
aX);if(o4==0){
#ifdef DEBUG_POWI
gA3"no"
o13"found for "
o53
oL2
#endif
hY2
double
dM2=o62
o4,0,0,0)+qN2
long
aX*o4));int
dI3=0;int
dJ3=0;int
mG2=0;
#ifdef DEBUG_POWI
gA3"orig = "
o53
oL2
gA3"plain"
o13"= %d, cost "
o53
o4,dM2);
#endif
for
aK
n_s=0;n_s<MaxSep;++n_s){int
dX=0
dE3
oT1=dM2;int
w91=o4;for(int
s=1;s<5*4;++s){
#ifdef CBRT_IS_SLOW
if(s>=5)break;
#endif
int
n_sqrt=s%5;int
n_cbrt=s/5;if(n_sqrt+n_cbrt>4)dD2
double
gC1=mM2;gC1-=hX2
s];int
g93
dJ1(gC1);if
qI3!=0)dN3
cost=o62
factor,dI3+n_sqrt,dJ3+n_cbrt,mG2+1)+qN2
long(gC1*factor));
#ifdef DEBUG_POWI
gA3"%d sqrt %d cbrt"
o13"= %d, cost "
o53
n_sqrt,n_cbrt
d13,cost);
#endif
if(cost<oT1){dX=s;w91=factor;oT1=cost;}
}
}
if(!dX)break;gB3
sep_list[n_s]=dX
d01-=hX2
dX];dI3+=dX%5;dJ3+=dX/5;dM2=oT1;o4=w91;mG2+=1;}
gB3
resulting_exponent=(long)aX*o4+0.5);while(o4%2==0){++gB3
n_int_sqrt;o4/=2;}
while(o4%3==0){++gB3
n_int_cbrt;o4/=3;}
hY2
private:bool
gL1
double
value,int
factor)const
dN3
v=value*double
qI3)dE3
diff=fabs(v-(double)(long)(v+0.5))dQ1
diff<1e-9;}
int
dJ1(double
value)const{int
g93(2*2*2*2);
#ifdef CBRT_IS_SLOW
#else
factor*=(3*3*3);
#endif
int
result=0;if(gL1
value
d13)){result=factor;while(qI3%2)==0&&gL1
value
d13/2)w62=factor/=2;while(qI3%3)==0&&gL1
value
d13/3)w62=factor/=3;}
#ifdef CBRT_IS_SLOW
if
hP2==0){if(gL1
value,3))mN2
3;}
#endif
hY2
int
o62
int
factor,int
s,int
c,int
nmuls)const{const
int
mH2=6;
#ifdef CBRT_IS_SLOW
const
int
dN2=25;
#else
const
int
dN2=8;
#endif
int
result=s*mH2+c*dN2;while
qI3%2==0){factor/=2;result+=mH2;}
while
qI3%3==0){factor/=3;result+=dN2;}
result+=nmuls
dP1}
;}
w72{aS2
RecreateInversionsAndNegations(bool
prefer_base2){bool
a21
false;d3
if
hX
a).RecreateInversionsAndNegations(prefer_base2))a21
true;if(changed){exit_changed:Mark_Incompletely_Hashed()oG}
switch(GetOpcode()){case
cMul:{h43
mX1;dK1
mY1,hT1;if(true){bool
q81=false
dE3
aO1=0;for
qY
GetOpcode()==gC3
0)oP2
wP
1)oM1){q81=true;aO1=wP
1)hF1
o52}
if(q81)dN3
immeds=1.0;for
qY
IsImmed()){immeds*=powgroup
hF1;oG1}
for
a81
a=gM1;a-->0;gB1&powgroup=wB2
a);if(powgroup
m9
gC3
0)oP2
wP
1)oM1
gB1&log2=wP
0);log2.oY
log2
m6
gF3;log2
hS1(dK1(d93
immeds,1.0/aO1)));log2
m7
break;}
}
}
}
for
qY
GetOpcode()==gC3
1)oM1){aH
exp_param=wP
1)dE3
dG2
exp_param
hF1;if(aL1
gG3
oY
mX1.push_back
hX
a)qN3
oG1
else
if
aX<0&&IsIntegerConst
aX)gB1
mK;mK
m6
cPow);mK
hS1(wP
0));mK
hS1(dK1(-mM2));mK
m7
mX1.push_back(mK)gD3
wG1
powgroup
oP2!mY1.a61){mY1=wP
0);oY
oG1
wG1
powgroup
m9
cLog2by&&!hT1.a61){hT1=powgroup
gD3
if(!mX1.hW3){a21
true;dK1
q11;q11
m6
cMul);q11
oZ1
mX1);q11
m7
dK1
m5
cMul);hE1
SetParamsMove(wG
if(hE1
IsImmed()&&a22
hE1
dR2,1.0)){qL2
cInv)w2
q11);}
else{if(hE1
qS2>=q11.qS2){qL2
cDiv)w2
aW1
w2
q11);}
else{qL2
cRDiv)w2
q11)w2
aW1;}
}
}
if(mY1.a61
gB1
m5
mB
hE1
SetParamsMove(wG
while(hE1
RecreateInversionsAndNegations(prefer_base2))hE1
FixIncompleteHashes();qL2
gF3
w2
mY1)w2
aW1;a21
gS3
if(hT1.a61
gB1
m5
cMul);mO2
hD1
hT1
gC
1));hE1
AddParamsMove(wG
while(hE1
RecreateInversionsAndNegations(prefer_base2))hE1
FixIncompleteHashes();DelParams();qL2
gF3
w2
hT1
gC
0))w2
aW1;a21
true;qD2
cAdd:{h43
oQ2
gW3
hD
wB2
a)m9
cMul){q01
mO2=wB2
a)gX3
b=hE1
gM1;b-->0;){if(mO2
gC
b).qF1
g93
mO2
gC
b)hF1;if(a22
factor
gG3
hE1
oY
hE1
DelParam(b
dH2
wG1
a22
factor,-2.0)){hE1
oY
hE1
DelParam(b);hE1
dL2
dK1(2.0)dH2}
}
if(wH){hE1
wK
aW1
gD3
else
if
hX
a)m9
cDiv){q01
q11=wB2
a);if(q11
gC
0)gH3
a22
q11.qR
gG3
q11.oY
q11
w51
0);q11
m6
cInv
dH2}
if(wH){q11.wK
q11)gD3
else
if
hX
a)m9
cRDiv){q01
q11=wB2
a);if(q11
gC
1)gH3
a22
q11
gC
1)hF1
gG3
q11.oY
q11
w51
1);q11
m6
cInv
dH2}
if(wH){q11.wK
q11)gD3
if(!oQ2.hW3
gB1
gW2;gW2
m6
cAdd);gW2
oZ1
oQ2);gW2
m7
dK1
hZ1;hZ1
m6
cAdd);hZ1
oZ1
oU1);hZ1
m7
if(hZ1
oM1&&a22
hZ1
hF1,0.0)){qL2
cNeg);oT);}
else{if(hZ1.qS2==1){qL2
cRSub);oT)w2
hZ1);}
wG1
gW2
m9
cAdd){qL2
cSub)w2
hZ1);oT
qN3
for
a81
a=1;a<gW2.gM1;++a
gB1
dO2;dO2
m6
cSub);dO2
oZ1
oU1);dO2.Rehash(mY2
w2
dO2);oT
gC
a));}
}
else{qL2
cSub)w2
hZ1);oT);}
}
qD2
cPow:{aH
p0
q92;aH
p1=h31;if(p1
gH3
p1
hF1!=0.0&&!p1.IsLongIntegerImmed()){wX1::gV2
r=wX1().CreatePowiResult(fabs
dO3));if(r.resulting_exponent!=0){bool
wY1=false;if
dO3<0&&r.sep_list[0]==0&&r.n_int_sqrt>0){wY1=gS3
#ifdef DEBUG_POWI
gA3"Will resolve powi %g as powi(chain(%d,%d),%ld)"
,fabs
dO3),r.n_int_sqrt,r.n_int_cbrt,r.resulting_exponent)o23
n=0;n<wX1::MaxSep;++n){if(r
oN1==0)break;int
n_sqrt=r
oN1%5;int
n_cbrt=r
oN1/5;gA3"*chain(%d,%d)"
,n_sqrt,n_cbrt);}
gA3"\n"
);
#endif
dK1
hZ2
q92;dK1
gX2=hZ2;gX2.oY
ChangeIntoRootChain(gX2,wY1,r.n_int_sqrt,r.n_int_cbrt);gX2
m7
dK1
pow;if(r.resulting_exponent!=1){pow
m6
cPow);pow
hD1
gX2
qO3
dK1(double(r.resulting_exponent)));}
else
pow.swap(gX2);dK1
mul;mul
m6
cMul);mul
hD1
pow)o23
n=0;n<wX1::MaxSep;++n){if(r
oN1==0)break;int
n_sqrt=r
oN1%5;int
n_cbrt=r
oN1/5;dK1
dP2=hZ2;dP2.oY
ChangeIntoRootChain(dP2,false,n_sqrt,n_cbrt);dP2
m7
mul
hD1
dP2);}
if
dO3<0&&!wY1){mul
m7
qL2
cInv)hL1
0,mul
qI2
1);}
else{qL2
cMul);SetParamsMove(mul.oU1);}
#ifdef DEBUG_POWI
mN
#endif
a21
true
o52}
}
gA1
cPow&&(!p1.IsLongIntegerImmed()||!IsOptimizableUsingPowi(p1.GetLongIntegerImmed()))){if(p0
oM1&&p0
hF1>0.0){if(prefer_base2)dN3
gY2=fp_log2(p0
hF1);if(gY2==1.0){DelParam(0);}
else{qU
dK1(gY2))d01
hS1(p1)d01.Rehash()dO}
qL2
cExp2);a21
gS3
else
dN3
gY2=std::log(p0
hF1);if(gY2==1.0){DelParam(0);}
else{qU
dK1(gY2))d01
hS1(p1)d01.Rehash()dO}
qL2
cExp);a21
gS3}
wG1
p0.IsAlwaysSigned(true)){if(prefer_base2
gB1
log;log
m6
cLog2);log
hS1(p0);log
m7
qU
p1)d01
hD1
log)d01
m7
qL2
cExp2)dO
a21
gS3
else{dK1
log;log
m6
cLog);log
hS1(p0);log
m7
qU
p1)d01
hD1
log)d01
m7
qL2
cExp)dO
a21
gS3}
}
break;}
aQ
if(changed)goto
exit_changed
dQ1
changed;}
bool
h51
aH
dK3
aH
tree,const
oH
aA1&qE3
const
aN1&mV1){for
a81
b=dN,a=0;a<b;++a){aH
leaf=tree
gC
a);aN1::qY3
a91;for(aN1::const_iterator
i=mV1.begin();i!=mV1
h13;++i){if(i->first!=leaf
o92)dD2
const
h41
i->aP1
size_t
score=occ.GetCSEscore();aH
candidate=i->aQ1
if(dQ2
candidate))dD2
if(leaf.qS2<occ.MinimumDepth())dD2
if(score<2)dD2
if(IfBalanceGood(dK3
leaf).wO==mY2
continue
oG}
if(h51
dK3
leaf,qE3
mV1))mN2
gS3
hZ}
bool
wA1
aH
qX3,aH
expr){for
a71
qX3
w3
expr))mN2
true;for
a71
wA1
qX3
gC
a),expr))mN2
true;hZ}
bool
GoodMomentForCSE(aH
qX3,aH
expr){if(qX3
m9
cIf)mN2
true;for
a71
qX3
w3
expr))mN2
true;size_t
oR2=0;for
a71
wA1
qX3
gC
a),expr))++oR2
dQ1
oR2!=1;}
size_t
g5
SynthCommonSubExpressions(oH
aA1&synth)const{size_t
stacktop_before=qS1
GetStackTop();aN1
mV1;FindTreeCounts(mV1,*this,mB
#ifdef DEBUG_SUBSTITUTIONS_CSE
DumpHashes(*this);
#endif
for(;;){size_t
gZ2=0;aN1::qY3
a91;for(aN1::qY3
j,i=mV1.begin();i!=mV1
h13;i=j){j=i;++j;const
h41
i->aP1
size_t
score=occ.GetCSEscore();aH
tree=i->aQ1
#ifdef DEBUG_SUBSTITUTIONS_CSE
qC2"Score "
<<score<<":\n"
;DumpTreeWithIndent(tree);
#endif
if(dQ2
tree))hB
if(tree.qS2<occ.MinimumDepth())hB
if(score<2)hB
if(IfBalanceGood(*this,tree).wO==mY2
hB
if(h51*this,tree,qE3
mV1)){dD2}
if(!GoodMomentForCSE(*this,tree))hB
score*=tree.qS2;if(score>gZ2){gZ2=score;a91=i;}
}
if(gZ2<=0)break;const
h41
a91->aP1
aH
tree=a91->aQ1
#ifdef DEBUG_SUBSTITUTIONS_CSE
qC2
o33"Common Subexpression:"
;q13(tree
hC1
#endif
int
gD1=occ.NeedsSinCos();dK1
oS2,oT2;if(gD1){oS2
hS1(tree);oS2
m6
cSin);oS2
m7
oT2
hS1(tree);oT2
m6
cCos);oT2
m7
if(dQ2
oS2)||dQ2
oT2)){if(gD1==2){mV1.erase(a91)dC2
gD1=0;}
}
tree.SynthesizeByteCode(qE3
mY2;mV1.erase(a91);
#ifdef DEBUG_SUBSTITUTIONS_CSE
qC2"Done with Common Subexpression:"
;q13(tree
hC1
#endif
if(gD1){if(gD1==2){mJ1
qT1);}
qS1
dN1
cSinCos,1,2);qS1
hF2
oS2,1);qS1
hF2
oT2,0);}
}
mN2
qS1
gA
stacktop_before;}
}
#endif
qV
#ifdef FP_SUPPORT_OPTIMIZER
using
w72;gB
double>::Optimize(){typedef
double
Value_t;oY
dK1
tree;tree.GenerateFrom
aT2
gE,gO3
Immed,*data);FPoptimizer_Optimize::ApplyGrammars(tree);hF
oA2>gI3;hF
Value_t>immed;size_t
stacktop_max=0;tree.SynthesizeByteCode(gI3,immed,stacktop_max);if
aT2
StackSize!=stacktop_max){gO3
StackSize=stacktop_max;
#ifndef FP_USE_THREAD_SAFE_EVAL
gO3
Stack
aG2
stacktop_max);
#endif
}
gO3
gE.swap(gI3);gO3
Immed.swap(immed);}
gB
float
qG1
gB
long
double
qG1
gB
long
qG1
#ifdef FP_SUPPORT_MPFR_FLOAT_TYPE
gB
MpfrFloat
qG1
#endif
#ifdef FP_SUPPORT_GMP_INT_TYPE
gB
GmpInt
qG1
#endif
FUNCTIONPARSER_INSTANTIATE_TYPES
#endif

#endif
