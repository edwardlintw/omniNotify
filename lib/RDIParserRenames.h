// -*- Mode: C++; -*-
//                              File      : RDIParserRenames.h
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
//    XXX TODO XXX
//
 
#ifndef __parser_renames_h__
#define __parser_renames_h__

#undef yyparse
//#undef yylex
// #undef yyerror
#undef yylval
#undef yyval  
#undef yychar
#undef yydebug 
#undef yyerrflag
#undef yynerrs 
#undef YYSTYPE
#undef yytext
#undef yywrap

#define	yyparse    RDI_Parser_parse
//#define yylex      RDI_Parser_lex
//#define yyerror    RDI_Parser_error
#define yylval     RDI_Parser_lval
#define yyval      RDI_Parser_val
#define yychar     RDI_Parser_char
#define yydebug    RDI_Parser_debug
#define yyerrflag  RDI_Parser_errflag
#define yynerrs    RDI_Parser_nerrs
#define YYSTYPE    RDI_Parser_lval_type
#define yytext     RDI_Parser_text
//#define yywrap     RDI_Parser_wrap

//extern int  RDI_Parser_lex(void);
//extern int  RDI_Parser_get_char(void);
//extern void RDI_Parser_unget_char(int); 

#if !defined(__GNUC__) && (defined(OSF) || defined(__hpux__))
#include <alloca.h>
#endif

#endif /* __parser_renames_h__ */
