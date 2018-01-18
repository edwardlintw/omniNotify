// -*- Mode: C++; -*-
//                              File      : RDIEval.cc
//                              Package   : omniNotify-Library
//                              Created on: 1-Jan-1998
//                              Authors   : gruber&panagos
//
//    Copyright (C) 1998-2003 AT&T Laboratories -- Research
//
//    This file is part of the omniNotify library
//    and is distributed with the omniNotify release.
//
//    The omniNotify library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free
//    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//    02111-1307, USA
//
//
// Description:
//    Implementation of support functionality for EXTENDED_TCL parser
//

#include "RDIEvalDefs.h"
#include "RDI.h"

////////////////////////////////////////////////////
// TABLES

const char* RDI_WKP2string[] = {
  "$",
  "header",
  "filterable_data",
  "remainder_of_body",
  "fixed_header",
  "variable_header",
  "event_name",
  "event_type",
  "type_name",
  "domain_name"
};

const char* RDI_RTRetCode2string[] = {
  "RDI_RTRet_OK",
  "RDI_RTRet_UNDEFINED",
  "RDI_RTRet_DIVIDE_BY_ZERO",
  "RDI_RTRet_OVERFLOW",
  "RDI_RTRet_OUT_OF_MEMORY",
  "RDI_RTRet_TYPE_MISMATCH",
  "RDI_RTRet_NONE_SUCH",
  "RDI_RTRet_NOT_SUPPORTED"
};

const char* RDI_OpArgT2string[] = {
  "RDI_OpArgT_none",
  "RDI_OpArgT_sc",
  "RDI_OpArgT_bc",
  "RDI_OpArgT_nc_us",
  "RDI_OpArgT_nc_s",
  "RDI_OpArgT_nc_ul",
  "RDI_OpArgT_nc_l",
  "RDI_OpArgT_nc_ull",
  "RDI_OpArgT_nc_ll",
  "RDI_OpArgT_nc_f",
  "RDI_OpArgT_nc_d",
  "RDI_OpArgT_lbl"
};

const char* RDI_OpCode2string[] = {
  "RDI_OpCode_nop",
  "RDI_OpCode_return_b",
  "RDI_OpCode_signal_N",
  "RDI_OpCode_push_cC2c",
  "RDI_OpCode_push_sC2s",
  "RDI_OpCode_push_iC2i",
  "RDI_OpCode_push_bC2b",
  "RDI_OpCode_push_NC2N",
  "RDI_OpCode_push_nC2n_s",
  "RDI_OpCode_push_nC2n_ul",
  "RDI_OpCode_push_nC2n_l",
  "RDI_OpCode_push_nC2n_ull",
  "RDI_OpCode_push_nC2n_ll",
  "RDI_OpCode_push_nC2n_f",
  "RDI_OpCode_push_nC2n_d",
  "RDI_OpCode_ctelt_NC2n",
  "RDI_OpCode_swap_uu2uu",
  "RDI_OpCode_pop_u",
  "RDI_OpCode_pop_uu",
  "RDI_OpCode_cvt_u2b",
  "RDI_OpCode_cvt_u2s",
  "RDI_OpCode_cvt_u2n",
  "RDI_OpCode_or_bb2b",
  "RDI_OpCode_and_bb2b",
  "RDI_OpCode_not_b2b",
  "RDI_OpCode_in_uu2b",
  "RDI_OpCode_add_nn2n",
  "RDI_OpCode_sub_nn2n",
  "RDI_OpCode_mul_nn2n",
  "RDI_OpCode_div_nn2n",
  "RDI_OpCode_rem_nn2n",
  "RDI_OpCode_substr_ss2b",
  "RDI_OpCode_cmp_uu2n",
  "RDI_OpCode_eqz_n2b",
  "RDI_OpCode_nez_n2b",
  "RDI_OpCode_lez_n2b",
  "RDI_OpCode_ltz_n2b",
  "RDI_OpCode_gez_n2b",
  "RDI_OpCode_gtz_n2b",
  "RDI_OpCode_ifT_b2b",
  "RDI_OpCode_ifF_b2b",
  "RDI_OpCode_goto",
  "RDI_OpCode_label",
  "RDI_OpCode_wkp_NC2u",
  "RDI_OpCode_special_sC2u",
  "RDI_OpCode_compend",
  "RDI_OpCode_default_X2b",
  "RDI_OpCode_exist_X2b",
  "RDI_OpCode_dot_len_u2n_l",
  "RDI_OpCode_dot_d_u2u",
  "RDI_OpCode_dot_tid_u2s",
  "RDI_OpCode_dot_rid_u2s",
  "RDI_OpCode_dot_id_usC2u",
  "RDI_OpCode_dot_num_ulC2u",
  "RDI_OpCode_assoc_usC2u",
  "RDI_OpCode_index_ulC2u",
  "RDI_OpCode_tagdef_u2u",
  "RDI_OpCode_tagid_usC2u",
  "RDI_OpCode_tagnum_ulC2u",
  "RDI_OpCode_tagchar_usC2u",
  "RDI_OpCode_tagbool_ubC2u",
  "RDI_OpCode_debug1",
  "RDI_OpCode_debug2"
};

// MAP OpCode -> stack prereq
const RDI_StackPre RDI_Op2StackPre[] = {
  RDI_StackPre_none,      //  RDI_OpCode_nop                
  RDI_StackPre_b,         //  RDI_OpCode_return_b           
  RDI_StackPre_N,         //  RDI_OpCode_signal_N           
  RDI_StackPre_none,      //  RDI_OpCode_push_cC2c          
  RDI_StackPre_none,      //  RDI_OpCode_push_sC2s          
  RDI_StackPre_none,      //  RDI_OpCode_push_iC2i          
  RDI_StackPre_none,      //  RDI_OpCode_push_bC2b          
  RDI_StackPre_none,      //  RDI_OpCode_push_NC2N
  RDI_StackPre_none,      //  RDI_OpCode_push_nC2n_s
  RDI_StackPre_none,      //  RDI_OpCode_push_nC2n_ul
  RDI_StackPre_none,      //  RDI_OpCode_push_nC2n_l
  RDI_StackPre_none,      //  RDI_OpCode_push_nC2n_ull
  RDI_StackPre_none,      //  RDI_OpCode_push_nC2n_ll
  RDI_StackPre_none,      //  RDI_OpCode_push_nC2n_f
  RDI_StackPre_none,      //  RDI_OpCode_push_nC2n_d
  RDI_StackPre_none,      //  RDI_OpCode_ctelt_NC2n         
  RDI_StackPre_XX,        //  RDI_OpCode_swap_uu2uu         
  RDI_StackPre_X,         //  RDI_OpCode_pop_u              
  RDI_StackPre_XX,        //  RDI_OpCode_pop_uu             
  RDI_StackPre_u,         //  RDI_OpCode_cvt_u2b            
  RDI_StackPre_u,         //  RDI_OpCode_cvt_u2s            
  RDI_StackPre_X,         //  RDI_OpCode_cvt_u2n            
  RDI_StackPre_bb,        //  RDI_OpCode_or_bb2b            
  RDI_StackPre_bb,        //  RDI_OpCode_and_bb2b           
  RDI_StackPre_b,         //  RDI_OpCode_not_b2b            
  RDI_StackPre_Xu,        //  RDI_OpCode_in_uu2b            
  RDI_StackPre_nn,        //  RDI_OpCode_add_nn2n           
  RDI_StackPre_nn,        //  RDI_OpCode_sub_nn2n           
  RDI_StackPre_nn,        //  RDI_OpCode_mul_nn2n           
  RDI_StackPre_nn,        //  RDI_OpCode_div_nn2n           
  RDI_StackPre_nn,        //  RDI_OpCode_rem_nn2n           
  RDI_StackPre_ss,        //  RDI_OpCode_substr_ss2b        
  RDI_StackPre_XX,        //  RDI_OpCode_cmp_uu2n           
  RDI_StackPre_n,         //  RDI_OpCode_eqz_n2b            
  RDI_StackPre_n,         //  RDI_OpCode_nez_n2b            
  RDI_StackPre_n,         //  RDI_OpCode_lez_n2b            
  RDI_StackPre_n,         //  RDI_OpCode_ltz_n2b            
  RDI_StackPre_n,         //  RDI_OpCode_gez_n2b            
  RDI_StackPre_n,         //  RDI_OpCode_gtz_n2b            
  RDI_StackPre_b,         //  RDI_OpCode_ifT_b2b            
  RDI_StackPre_b,         //  RDI_OpCode_ifF_b2b          
  RDI_StackPre_none,      //  RDI_OpCode_goto               
  RDI_StackPre_none,      //  RDI_OpCode_label               
  RDI_StackPre_none,      //  RDI_OpCode_wkp_NC2u          
  RDI_StackPre_none,      //  RDI_OpCode_special_sC2u       
  RDI_StackPre_X,         //  RDI_OpCode_compend               
  RDI_StackPre_X,         //  RDI_OpCode_default_X2b        
  RDI_StackPre_X,         //  RDI_OpCode_exist_X2b          
  RDI_StackPre_u,         //  RDI_OpCode_dot_len_u2n_l        
  RDI_StackPre_u,         //  RDI_OpCode_dot_d_u2u          
  RDI_StackPre_u,         //  RDI_OpCode_dot_tid_u2s        
  RDI_StackPre_u,         //  RDI_OpCode_dot_rid_u2s        
  RDI_StackPre_u,         //  RDI_OpCode_dot_id_usC2u       
  RDI_StackPre_u,         //  RDI_OpCode_dot_num_ulC2u      
  RDI_StackPre_u,         //  RDI_OpCode_assoc_usC2u        
  RDI_StackPre_u,         //  RDI_OpCode_index_ulC2u        
  RDI_StackPre_u,         //  RDI_OpCode_tagdef_u2u         
  RDI_StackPre_u,         //  RDI_OpCode_tagid_usC2u        
  RDI_StackPre_u,         //  RDI_OpCode_tagnum_ulC2u       
  RDI_StackPre_u,         //  RDI_OpCode_tagchar_usC2u      
  RDI_StackPre_u,         //  RDI_OpCode_tagbool_ubC2u      
  RDI_StackPre_none,      //  RDI_OpCode_debug1             
  RDI_StackPre_none       //  RDI_OpCode_debug2             
};

// MAP OpCode -> change in top of stack
const RDI_StackEffect RDI_Op2StackEffect[] = {
  RDI_StackEffect_none,   //  RDI_OpCode_nop                  
  RDI_StackEffect_none,   //  RDI_OpCode_return_b             
  RDI_StackEffect_none,   //  RDI_OpCode_signal_N             
  RDI_StackEffect_2c,     //  RDI_OpCode_push_cC2c            
  RDI_StackEffect_2s,     //  RDI_OpCode_push_sC2s            
  RDI_StackEffect_2i,     //  RDI_OpCode_push_iC2i            
  RDI_StackEffect_2b,     //  RDI_OpCode_push_bC2b            
  RDI_StackEffect_2N,     //  RDI_OpCode_push_NC2N
  RDI_StackEffect_2n,     //  RDI_OpCode_push_nC2n_s
  RDI_StackEffect_2n,     //  RDI_OpCode_push_nC2n_ul
  RDI_StackEffect_2n,     //  RDI_OpCode_push_nC2n_l
  RDI_StackEffect_2n,     //  RDI_OpCode_push_nC2n_ull
  RDI_StackEffect_2n,     //  RDI_OpCode_push_nC2n_ll
  RDI_StackEffect_2n,     //  RDI_OpCode_push_nC2n_f
  RDI_StackEffect_2n,     //  RDI_OpCode_push_nC2n_d
  RDI_StackEffect_2n,     //  RDI_OpCode_ctelt_NC2n           
  RDI_StackEffect_swap,   //  RDI_OpCode_swap_uu2uu           
  RDI_StackEffect_X,      //  RDI_OpCode_pop_u                
  RDI_StackEffect_XX,     //  RDI_OpCode_pop_uu               
  RDI_StackEffect_X2b,    //  RDI_OpCode_cvt_u2b              
  RDI_StackEffect_X2s,    //  RDI_OpCode_cvt_u2s              
  RDI_StackEffect_X2n,    //  RDI_OpCode_cvt_u2n              
  RDI_StackEffect_XX2b,   //  RDI_OpCode_or_bb2b              
  RDI_StackEffect_XX2b,   //  RDI_OpCode_and_bb2b             
  RDI_StackEffect_X2b,    //  RDI_OpCode_not_b2b              
  RDI_StackEffect_XX2b,   //  RDI_OpCode_in_uu2b              
  RDI_StackEffect_XX2n,   //  RDI_OpCode_add_nn2n             
  RDI_StackEffect_XX2n,   //  RDI_OpCode_sub_nn2n             
  RDI_StackEffect_XX2n,   //  RDI_OpCode_mul_nn2n             
  RDI_StackEffect_XX2n,   //  RDI_OpCode_div_nn2n             
  RDI_StackEffect_XX2n,   //  RDI_OpCode_rem_nn2n             
  RDI_StackEffect_XX2b,   //  RDI_OpCode_substr_ss2b          
  RDI_StackEffect_XX2n,   //  RDI_OpCode_cmp_uu2n             
  RDI_StackEffect_X2b,    //  RDI_OpCode_eqz_n2b              
  RDI_StackEffect_X2b,    //  RDI_OpCode_nez_n2b              
  RDI_StackEffect_X2b,    //  RDI_OpCode_lez_n2b              
  RDI_StackEffect_X2b,    //  RDI_OpCode_ltz_n2b              
  RDI_StackEffect_X2b,    //  RDI_OpCode_gez_n2b              
  RDI_StackEffect_X2b,    //  RDI_OpCode_gtz_n2b              
  RDI_StackEffect_none,   //  RDI_OpCode_ifT_b2b               
  RDI_StackEffect_none,   //  RDI_OpCode_ifF_b2b               
  RDI_StackEffect_none,   //  RDI_OpCode_goto                 
  RDI_StackEffect_none,   //  RDI_OpCode_label                 
  RDI_StackEffect_2u,     //  RDI_OpCode_wkp_NC2u            
  RDI_StackEffect_2u,     //  RDI_OpCode_special_sC2u         
  RDI_StackEffect_none,   //  RDI_OpCode_compend
  RDI_StackEffect_X2b,    //  RDI_OpCode_default_X2b          
  RDI_StackEffect_X2b,    //  RDI_OpCode_exist_X2b            
  RDI_StackEffect_X2n,    //  RDI_OpCode_dot_len_u2n_l          
  RDI_StackEffect_X2u,    //  RDI_OpCode_dot_d_u2u            
  RDI_StackEffect_X2s,    //  RDI_OpCode_dot_tid_u2s          
  RDI_StackEffect_X2s,    //  RDI_OpCode_dot_rid_u2s          
  RDI_StackEffect_X2u,    //  RDI_OpCode_dot_id_usC2u         
  RDI_StackEffect_X2u,    //  RDI_OpCode_dot_num_ulC2u        
  RDI_StackEffect_X2u,    //  RDI_OpCode_assoc_usC2u          
  RDI_StackEffect_X2u,    //  RDI_OpCode_index_ulC2u          
  RDI_StackEffect_X2u,    //  RDI_OpCode_tagdef_u2u           
  RDI_StackEffect_X2u,    //  RDI_OpCode_tagid_usC2u          
  RDI_StackEffect_X2u,    //  RDI_OpCode_tagnum_ulC2u         
  RDI_StackEffect_X2u,    //  RDI_OpCode_tagchar_usC2u        
  RDI_StackEffect_X2u,    //  RDI_OpCode_tagbool_ubC2u        
  RDI_StackEffect_none,   //  RDI_OpCode_debug1               
  RDI_StackEffect_none    //  RDI_OpCode_debug2               
};

// MAP OpCode -> required argument for this kind of op
const RDI_OpArgT RDI_Op2ArgT[] = {
  RDI_OpArgT_none,     //  RDI_OpCode_nop                
  RDI_OpArgT_none,     //  RDI_OpCode_return_b           
  RDI_OpArgT_none,     //  RDI_OpCode_signal_N           
  RDI_OpArgT_sc,       //  RDI_OpCode_push_cC2c          
  RDI_OpArgT_sc,       //  RDI_OpCode_push_sC2s          
  RDI_OpArgT_sc,       //  RDI_OpCode_push_iC2i          
  RDI_OpArgT_bc,       //  RDI_OpCode_push_bC2b          
  RDI_OpArgT_nc_us,    //  RDI_OpCode_push_NC2N
  RDI_OpArgT_nc_s,     //  RDI_OpCode_push_nC2n_s
  RDI_OpArgT_nc_ul,    //  RDI_OpCode_push_nC2n_ul
  RDI_OpArgT_nc_l,     //  RDI_OpCode_push_nC2n_l
  RDI_OpArgT_nc_ull,   //  RDI_OpCode_push_nC2n_ull
  RDI_OpArgT_nc_ll,    //  RDI_OpCode_push_nC2n_ll
  RDI_OpArgT_nc_f,     //  RDI_OpCode_push_nC2n_f
  RDI_OpArgT_nc_d,     //  RDI_OpCode_push_nC2n_d
  RDI_OpArgT_nc_us,    //  RDI_OpCode_ctelt_NC2n   
  RDI_OpArgT_none,     //  RDI_OpCode_swap_uu2uu         
  RDI_OpArgT_none,     //  RDI_OpCode_pop_u              
  RDI_OpArgT_none,     //  RDI_OpCode_pop_uu             
  RDI_OpArgT_none,     //  RDI_OpCode_cvt_u2b            
  RDI_OpArgT_none,     //  RDI_OpCode_cvt_u2s            
  RDI_OpArgT_none,     //  RDI_OpCode_cvt_u2n            
  RDI_OpArgT_none,     //  RDI_OpCode_or_bb2b            
  RDI_OpArgT_none,     //  RDI_OpCode_and_bb2b           
  RDI_OpArgT_none,     //  RDI_OpCode_not_b2b            
  RDI_OpArgT_none,     //  RDI_OpCode_in_uu2b            
  RDI_OpArgT_none,     //  RDI_OpCode_add_nn2n           
  RDI_OpArgT_none,     //  RDI_OpCode_sub_nn2n           
  RDI_OpArgT_none,     //  RDI_OpCode_mul_nn2n           
  RDI_OpArgT_none,     //  RDI_OpCode_div_nn2n           
  RDI_OpArgT_none,     //  RDI_OpCode_rem_nn2n           
  RDI_OpArgT_none,     //  RDI_OpCode_substr_ss2b        
  RDI_OpArgT_none,     //  RDI_OpCode_cmp_uu2n           
  RDI_OpArgT_none,     //  RDI_OpCode_eqz_n2b            
  RDI_OpArgT_none,     //  RDI_OpCode_nez_n2b            
  RDI_OpArgT_none,     //  RDI_OpCode_lez_n2b            
  RDI_OpArgT_none,     //  RDI_OpCode_ltz_nlb            
  RDI_OpArgT_none,     //  RDI_OpCode_gez_n2b            
  RDI_OpArgT_none,     //  RDI_OpCode_gtz_n2b            
  RDI_OpArgT_lbl,      //  RDI_OpCode_ifT_b2b             
  RDI_OpArgT_lbl,      //  RDI_OpCode_ifF_b2b             
  RDI_OpArgT_lbl,      //  RDI_OpCode_goto               
  RDI_OpArgT_sc,       //  RDI_OpCode_label               
  RDI_OpArgT_nc_us,    //  RDI_OpCode_wkp_NC2u          
  RDI_OpArgT_sc,       //  RDI_OpCode_special_sC2u       
  RDI_OpArgT_none,     //  RDI_OpCode_compend
  RDI_OpArgT_none,     //  RDI_OpCode_default_X2b        
  RDI_OpArgT_none,     //  RDI_OpCode_exist_X2b          
  RDI_OpArgT_none,     //  RDI_OpCode_dot_len_u2n_l        
  RDI_OpArgT_none,     //  RDI_OpCode_dot_d_u2u          
  RDI_OpArgT_none,     //  RDI_OpCode_dot_tid_u2s        
  RDI_OpArgT_none,     //  RDI_OpCode_dot_rid_u2s        
  RDI_OpArgT_sc,       //  RDI_OpCode_dot_id_usC2u       
  RDI_OpArgT_nc_l,     //  RDI_OpCode_dot_num_ulC2u      
  RDI_OpArgT_sc,       //  RDI_OpCode_assoc_usC2u        
  RDI_OpArgT_nc_l,     //  RDI_OpCode_index_ulC2u        
  RDI_OpArgT_none,     //  RDI_OpCode_tagdef_u2u         
  RDI_OpArgT_sc,       //  RDI_OpCode_tagid_usC2u        
  RDI_OpArgT_nc_l,     //  RDI_OpCode_tagnum_ulC2u       
  RDI_OpArgT_sc,       //  RDI_OpCode_tagchar_usC2u      
  RDI_OpArgT_bc,       //  RDI_OpCode_tagbool_ubC2u      
  RDI_OpArgT_none,     //  RDI_OpCode_debug1             
  RDI_OpArgT_none      //  RDI_OpCode_debug2             
};

///////////////////////////////////////////////////////////////////////////////////////////////
// RDI_Op

// used as simple label generator
int   RDI_Op::_newlblctr = 0;

RDI_Op::RDI_Op(RDI_OpCode c) : _code(c), _argT(RDI_OpArgT_none) {
  _arg._v_none = 0; }

RDI_Op::RDI_Op(RDI_OpCode c, char* s) : _code(c), _argT(RDI_OpArgT_sc) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_sc, "op created with wrong ArgT");
  _arg._v_sc = s; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::Boolean b)   : _code(c), _argT(RDI_OpArgT_bc) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_bc, "op created with wrong ArgT");
  _arg._v_bc = b; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::UShort us)   : _code(c), _argT(RDI_OpArgT_nc_us) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_nc_us, "op created with wrong ArgT");
  _arg._v_nc_us = us; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::Short s)   : _code(c), _argT(RDI_OpArgT_nc_s) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_nc_s, "op created with wrong ArgT");
  _arg._v_nc_s = s; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::ULong ul)   : _code(c), _argT(RDI_OpArgT_nc_ul) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_nc_ul, "op created with wrong ArgT");
  _arg._v_nc_ul = ul; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::Long l)   : _code(c), _argT(RDI_OpArgT_nc_l) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_nc_l, "op created with wrong ArgT");
  _arg._v_nc_l = l; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::ULongLong ull)   : _code(c), _argT(RDI_OpArgT_nc_ull) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_nc_ull, "op created with wrong ArgT");
  _arg._v_nc_ull = ull; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::LongLong ll)   : _code(c), _argT(RDI_OpArgT_nc_ll) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_nc_ll, "op created with wrong ArgT");
  _arg._v_nc_ll = ll; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::Float f) : _code(c), _argT(RDI_OpArgT_nc_f) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_nc_f, "op created with wrong ArgT");
  _arg._v_nc_f = f; }

RDI_Op::RDI_Op(RDI_OpCode c, CORBA::Double d) : _code(c), _argT(RDI_OpArgT_nc_d) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_nc_d, "op created with wrong ArgT");
  _arg._v_nc_d = d; }

RDI_Op::RDI_Op(RDI_OpCode c, char* s, int offset) : _code(c), _argT(RDI_OpArgT_lbl) {
  RDI_Assert(RDI_Op2ArgT[c] == RDI_OpArgT_lbl, "op created with wrong ArgT");
  _arg._v_lbl._offset = offset; // use default of 0 == uninitialized
  _arg._v_lbl._lbl    = s; }
    
RDI_Op::~RDI_Op(void) {
  clear();
}

void RDI_Op::clear(CORBA::Boolean freestrings) {
  if (freestrings) {
    if (_argT == RDI_OpArgT_sc) {
      CORBA_STRING_FREE(_arg._v_sc);
    } else if (_argT == RDI_OpArgT_lbl) {
      CORBA_STRING_FREE(_arg._v_lbl._lbl);
    }
  }
  _code = RDI_OpCode_nop; _argT = RDI_OpArgT_none; _arg._v_none = 0;
};

// NB buf assumed to have len RDI_Eval_LBLBUF_LEN
void RDI_Op::newlblnm(const char* s, char buf[]) {
  RDI_Assert(RDI_STRLEN(s)+12 < RDI_Eval_LBLBUF_LEN, "just being careful");
  sprintf(buf, "LBL_%s_%d", s, _newlblctr++);
}

// debugging
void RDI_Op::dbg_output(RDIstrstream& str, CORBA::Boolean signal_const) {
  str << RDI_OpCode2string[_code] << " ";
  if (signal_const) {
    RDI_Assert(_code == RDI_OpCode_push_NC2N, "\n");
    switch ((RDI_RTRetCode)_arg._v_nc_us) {
    case RDI_RTRet_OK:
      str << "RDI_RTRet_OK";
      break;
    case RDI_RTRet_UNDEFINED:
      str << "RDI_RTRet_UNDEFINED";
      break;
    case RDI_RTRet_DIVIDE_BY_ZERO:
      str << "RDI_RTRet_DIVIDE_BY_ZERO";
      break;
    case RDI_RTRet_OVERFLOW:
      str << "RDI_RTRet_OVERFLOW";
      break;
    case RDI_RTRet_OUT_OF_MEMORY:
      str << "RDI_RTRet_OUT_OF_MEMORY";
      break; 
    case RDI_RTRet_TYPE_MISMATCH:
      str << "RDI_RTRet_TYPE_MISMATCH";
      break;
    case RDI_RTRet_NONE_SUCH:
      str << "RDI_RTRet_NONE_SUCH";
      break;
    case RDI_RTRet_NOT_SUPPORTED:
      str << "RDI_RTRet_NOT_SUPPORTED";
      break;
    }
    return;
  }

  switch (_argT) {
  case RDI_OpArgT_none:
    // no arg
    str << "";
    break;
  case RDI_OpArgT_sc:
    // string const
    str << "sc:\"" << _arg._v_sc << "\"";
    break;
  case RDI_OpArgT_bc:
    if (_arg._v_bc) {
      str << "bc:TRUE";
    } else {
      str << "bc:FALSE";
    }
    break;
  case RDI_OpArgT_nc_us:
    str << "lc:" << _arg._v_nc_us;
    break;
  case RDI_OpArgT_nc_s:
    str << "lc:" << _arg._v_nc_s;
    break;
  case RDI_OpArgT_nc_ul:
    str << "lc:" << _arg._v_nc_ul;
    break;
  case RDI_OpArgT_nc_l:
    str << "lc:" << _arg._v_nc_l;
    break;
  case RDI_OpArgT_nc_ull:
    str << "lc:" << _arg._v_nc_ull;
    break;
  case RDI_OpArgT_nc_ll:
    str << "lc:" << _arg._v_nc_ll;
    break;
  case RDI_OpArgT_nc_f:
    str << "dc:" << _arg._v_nc_f;
    break;
  case RDI_OpArgT_nc_d:
    str << "dc:" << _arg._v_nc_d;
    break;
  case RDI_OpArgT_lbl:
    str << "lbl: " << _arg._v_lbl._lbl << "(offset " << _arg._v_lbl._offset << ")";
    break;
  default:
    RDI_Fatal("should not get here");
  }
  return;
}

///////////////////////////////////////////////////////////////
// Logging

RDIstrstream& RDI_Op::log_output(RDIstrstream& str) const { 
  str << RDI_OpCode2string[_code] << " ";

  switch (_argT) {
  case RDI_OpArgT_none:
    // no arg
    break;
  case RDI_OpArgT_sc:
    // string const
    str << "sc:\"" << _arg._v_sc << "\"";
    break;
  case RDI_OpArgT_bc:
    if (_arg._v_bc) {
      str << "bc:TRUE";
    } else {
      str << "bc:FALSE";
    }
    break;
  case RDI_OpArgT_nc_us:
    str << "nc_us:" << _arg._v_nc_us;
    break;
  case RDI_OpArgT_nc_s:
    str << "nc_s:" << _arg._v_nc_s;
    break;
  case RDI_OpArgT_nc_ul:
    str << "nc_ul:" << _arg._v_nc_ul;
    break;
  case RDI_OpArgT_nc_l:
    str << "nc_l:" << _arg._v_nc_l;
    break;
  case RDI_OpArgT_nc_ull:
    str << "nc_ull:" << _arg._v_nc_ull;
    break;
  case RDI_OpArgT_nc_ll:
    str << "nc_ll:" << _arg._v_nc_ll;
    break;
  case RDI_OpArgT_nc_d:
    str << "nc_d:" << _arg._v_nc_d;
    break;
  case RDI_OpArgT_lbl:
    str << "lbl: " << _arg._v_lbl._lbl << "(offset " << _arg._v_lbl._offset << ")";
    break;
  default:
    RDI_Fatal("should not get here");
  }
  return str;
}

