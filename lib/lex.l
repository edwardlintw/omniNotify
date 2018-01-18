/* %option debug */
%option noyywrap

%{
/* Description:
 *    Lexer token definitions for the Notification Service default constraint grammar
 */

/* NB This version of our scanner assumes we are using */
/*      flex with the -i option.                       */  

#ifndef FLEX_SCANNER
/* sorry you must use flex */
  XXX 
#endif

#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include "RDIStringDefs.h"
#include "RDIStaticEvalDefs.h"
#include "RDIParser_y.h"

#ifdef UWIN
/* UWIN's lex should generate these decls! */
int yyback(int*, int);
int yylook(void);
#endif

#define YY_DECL \
  int yyFlexLexer::yylex(YYSTYPE* lvalp, RDI_PCState* ps)

#define YYLEX_USER_INIT \
  _saved_ps = ps;


#define YY_INPUT(buf,result,max_size) \
  if ( (result = _saved_ps->lexer_input( (char *) buf, max_size )) < 0 ) \
    YY_FATAL_ERROR( "input in flex scanner failed" );

#define RDI_NOVAL_TOK \
  (*lvalp).noval = NULL

#define RDI_STRVAL_TOK(str) \
  char* sdup = CORBA_STRING_DUP(str); (*lvalp).sb = sdup; ps->regstr(sdup);

%}

_exponent [0-9]+e[-+]?[0-9]+
_digits [0-9]+

/* a_string '(([^'\\]*)|([^'\\]*\\')|([^'\\]*\\\\))*' */
_string '([^'\\]*(\\\\|\\')?)*'
_ident  [a-z_][a-z0-9_]*

%%
[ \t]          ;

"=="             {RDI_NOVAL_TOK; return(TOK_EQ);}
"!="             {RDI_NOVAL_TOK; return(TOK_NE);}
"<="             {RDI_NOVAL_TOK; return(TOK_LE);}
"<"              {RDI_NOVAL_TOK; return(TOK_LT);}
">="             {RDI_NOVAL_TOK; return(TOK_GE);}
">"              {RDI_NOVAL_TOK; return(TOK_GT);}
"~"              {RDI_NOVAL_TOK; return(TOK_SUBSTR);}
"+"              {RDI_NOVAL_TOK; return(TOK_PLUS);}
"-"              {RDI_NOVAL_TOK; return(TOK_MINUS);}
"*"              {RDI_NOVAL_TOK; return(TOK_MUL);}
"/"              {RDI_NOVAL_TOK; return(TOK_DIV); }
"%"              {RDI_NOVAL_TOK; return(TOK_PCNT); }
"("              {RDI_NOVAL_TOK; return(TOK_LPAR);}
")"              {RDI_NOVAL_TOK; return(TOK_RPAR);}
"["              {RDI_NOVAL_TOK; return(TOK_LB);}
"]"              {RDI_NOVAL_TOK; return(TOK_RB);}
"."              {RDI_NOVAL_TOK; return(TOK_DOT);}
"$"              {RDI_NOVAL_TOK; return(TOK_DOLLAR);}

\\{_ident}       {char* s = (char*)YYText() + 1; RDI_STRVAL_TOK(s); return(TOK_IDENT);}
		 
and              {RDI_NOVAL_TOK; return(TOK_AND);}
default          {RDI_NOVAL_TOK; return(TOK_DEFAULT);}
exist            {RDI_NOVAL_TOK; return(TOK_EXIST);}
false            {RDI_NOVAL_TOK; return(TOK_FALSE);}
in               {RDI_NOVAL_TOK; return(TOK_IN);}
not              {RDI_NOVAL_TOK; return(TOK_NOT);}
or               {RDI_NOVAL_TOK; return(TOK_OR);}
true             {RDI_NOVAL_TOK; return(TOK_TRUE);}

{_exponent}      {RDI_STRVAL_TOK((char*)YYText()); return(TOK_EXPONENT);}
{_digits}        {RDI_STRVAL_TOK((char*)YYText()); return(TOK_DIGITS);}
{_string}        {RDI_STRVAL_TOK((char*)YYText());return(TOK_STRING);}
{_ident}         {RDI_STRVAL_TOK((char*)YYText());return(TOK_IDENT);}

.                {RDI_STRVAL_TOK((char*)YYText()); return(TOK_OTHER);}

%%

