%pure_parser

%{
/* Description:
 *    Parser grammar for the Notification Service default constraint grammar
 */

/*
   The expressions derived by the constraint nonterminal
   are from the trader constraint language as given in Appendix B
   of the OMG Trading Object Service specification, modified
   as defined in OMG Notification Service, section 2.4.

   Note that unary plus and minus can only be applied to numbers.
   This is lame, but part of the spec.  Thus to negate an expr
   you must write, e.g.,  -1 * (expr) 

   Note that this grammar does not differentiate between
   . foo (a regular dot component part) and the special forms
   . _d, . _length, . _type_id, . _repos_id,   etc
   (the constructed constraint recognizes these cases)
*/

#define RDI_YACC_Y

#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include "RDIStringDefs.h"
#include "RDIStaticEvalDefs.h"

#define YYPARSE_PARAM parm
#define YYLEX_PARAM parm

#define _ps ((RDI_PCState*)parm)

/* _ps:
     _ps->e       : error              (bool) 
     _ps->b       : error buf          (char[1024])
     _ps->r_tree  : result treee  (RDI_Constraint*)

   After parse, if _ps->r_tree is NULL, then _ps->e contains an error msg
*/

#define yylex(lvalp, psparm) \
 (((RDI_PCState*)psparm)->next_token((void*)lvalp))

#define yyerror(s)  do { _ps->e = 1; sprintf(_ps->b, "%s (parse failed)", s); } while (0)

%}

%union{
  /* the type of yylval */
  void* noval;     /* should be set to NULL */
  char* sb;
  RDI_Constraint* cexpr;
}

%token <sb> TOK_DIGITS
%token <sb> TOK_EXPONENT
%token <sb> TOK_IDENT
%token <sb> TOK_STRING

%token  <noval> TOK_EQ TOK_NE TOK_LE TOK_LT TOK_GE TOK_GT
%token  <noval> TOK_AND TOK_OR 
%token  <noval> TOK_DEFAULT TOK_EXIST TOK_NOT
%token  <noval> TOK_IN TOK_SUBSTR TOK_DOT
%token  <noval> TOK_PLUS TOK_MINUS TOK_MUL TOK_DIV TOK_PCNT
%token  <noval> TOK_LPAR TOK_RPAR TOK_LB TOK_RB
%token  <noval> TOK_DOLLAR
%token  <noval> TOK_TRUE TOK_FALSE

%token <sb> TOK_OTHER

%type <cexpr> bool_and
%type <cexpr> bool_compare
%type <cexpr> bool_or
%type <cexpr> component
%type <cexpr> comp_factor
%type <cexpr> compdone
%type <cexpr> constraint
%type <cexpr> expr
%type <cexpr> expr_in
%type <cexpr> expr_substr
%type <cexpr> factor
%type <cexpr> factor_not
%type <cexpr> global
%type <cexpr> number
%type <cexpr> string
%type <cexpr> enumident
%type <cexpr> term

%type <sb> lconst dconst

%left TOK_OR
%left TOK_AND
%right UMINUS UPLUS

%%
global        : { $$ = NULL; _ps->r_tree = NULL; _ps->e = 0; }
              | constraint
                { $$ = $1; _ps->r_tree = $1; _ps->e = 0; _ps->unregcexpr($1); }
              ;
constraint    : bool_or
              ;
bool_or       : bool_or TOK_OR bool_and
                { $$ = RDI_Constraint::NewOr(_ps, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | bool_and
              ;
bool_and      : bool_and TOK_AND bool_compare 
                { $$ = RDI_Constraint::NewAnd(_ps, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | bool_compare
              ;
bool_compare  : expr_in TOK_EQ expr_in
                { $$ = RDI_Constraint::NewCompare(_ps, RDI_CompareOp_eq, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | expr_in TOK_NE expr_in
                { $$ = RDI_Constraint::NewCompare(_ps, RDI_CompareOp_ne, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | expr_in TOK_LE expr_in
                { $$ = RDI_Constraint::NewCompare(_ps, RDI_CompareOp_le, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | expr_in TOK_LT expr_in
                { $$ = RDI_Constraint::NewCompare(_ps, RDI_CompareOp_lt, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | expr_in TOK_GE expr_in
                { $$ = RDI_Constraint::NewCompare(_ps, RDI_CompareOp_ge, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | expr_in TOK_GT expr_in
                { $$ = RDI_Constraint::NewCompare(_ps, RDI_CompareOp_gt, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | expr_in
              ;
expr_in       : expr_substr TOK_IN comp_factor
                { $$ = RDI_Constraint::NewIn(_ps, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              /* allowed by trader grammar, always an error at runtime: */
              | expr_substr TOK_IN TOK_IDENT
                { $$ = RDI_Constraint::NewRuntimeError(_ps, RDI_RTRet_NOT_SUPPORTED);
		  _ps->unregdelcexpr($1); _ps->unregdelstr($3); _ps->regcexpr($$); }
              | expr_substr
              ;
expr_substr   : expr TOK_SUBSTR expr
                { $$ = RDI_Constraint::NewSubStr(_ps, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | expr
              ;
expr          : expr TOK_PLUS term
                { $$ = RDI_Constraint::NewArith(_ps, RDI_ArithOp_add, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | expr TOK_MINUS term
                { $$ = RDI_Constraint::NewArith(_ps, RDI_ArithOp_sub, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | term
              ;
term          : term TOK_MUL factor_not
                { $$ = RDI_Constraint::NewArith(_ps, RDI_ArithOp_mul, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | term TOK_DIV factor_not 
                { $$ = RDI_Constraint::NewArith(_ps, RDI_ArithOp_div, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | term TOK_PCNT factor_not 
                { $$ = RDI_Constraint::NewArith(_ps, RDI_ArithOp_rem, $1, $3);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->unregcexpr($3); _ps->regcexpr($$); }
              | factor_not
              ;
factor_not    : TOK_NOT factor
                { $$ = RDI_Constraint::NewNot(_ps, $2);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($2); _ps->regcexpr($$); }
              | factor
              ;
factor        : number
              | TOK_PLUS number %prec UPLUS
                { $$ = $2; }
              | TOK_MINUS number %prec UMINUS
                { $$ = $2->negate(_ps);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($2); _ps->regcexpr($$); }
              | enumident
              | string
              | comp_factor
              | TOK_EXIST compdone
                { $$ = RDI_Constraint::NewExist(_ps, $2);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($2); _ps->regcexpr($$); }
              | TOK_DEFAULT compdone
                { $$ = RDI_Constraint::NewDefault(_ps, $2);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($2); _ps->regcexpr($$); }
              | TOK_TRUE
                { $$ = RDI_Constraint::NewBoolConst(_ps, 1);
		  if (_ps->e) { YYABORT; } 
                  _ps->regcexpr($$); }
              | TOK_FALSE
                { $$ = RDI_Constraint::NewBoolConst(_ps, 0);
		  if (_ps->e) { YYABORT; }
		  _ps->regcexpr($$); }
              | TOK_LPAR bool_or TOK_RPAR
                { $$ = $2; }
              ;
comp_factor   : compdone
                { $$ = RDI_Constraint::NewCompEnd(_ps, $1);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
/* compdone finalizes a component (detects special cases, etc) */ 
compdone      : component
                { $$ = $1->comp_done(_ps);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              ;
/* a component always starts with '$', has an optional top-level ident,
 * and a sequence of zero or more "component parts"
 */
component     : TOK_DOLLAR
                { $$ = RDI_Constraint::NewComp(_ps, NULL);
		  if (_ps->e) { YYABORT; }
		  _ps->regcexpr($$); }
              | TOK_DOLLAR TOK_IDENT
                { $$ = RDI_Constraint::NewComp(_ps, $2); _ps->unregdelstr($2);
		  if (_ps->e) { YYABORT; }
		  _ps->regcexpr($$); }
              | component TOK_DOT TOK_IDENT
                { $$ = $1->add_dot_ident(_ps, $3); _ps->unregdelstr($3); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_DIGITS
                { $$ = $1->add_dot_num(_ps, $3); _ps->unregdelstr($3); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_LB TOK_DIGITS TOK_RB
                { $$ = $1->add_index(_ps, $3); _ps->unregdelstr($3); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_LPAR TOK_IDENT TOK_RPAR
                { $$ = $1->add_assoc(_ps, $3); _ps->unregdelstr($3); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_LPAR TOK_RPAR
                { $$ = $1->add_tag_default(_ps);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_LPAR TOK_TRUE TOK_RPAR
                { $$ = $1->add_tag_bool(_ps, 1);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_LPAR TOK_FALSE TOK_RPAR
                { $$ = $1->add_tag_bool(_ps, 0);
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_LPAR TOK_STRING TOK_RPAR
                { $$ = $1->add_tag_char(_ps, $4); _ps->unregdelstr($4); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_LPAR TOK_IDENT TOK_RPAR
                { $$ = $1->add_tag_ident(_ps, $4); _ps->unregdelstr($4); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_LPAR TOK_DIGITS TOK_RPAR
                { $$ = $1->add_tag_num(_ps, $4, 0); _ps->unregdelstr($4); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_LPAR TOK_PLUS TOK_DIGITS TOK_RPAR
                { $$ = $1->add_tag_num(_ps, $5, 0); _ps->unregdelstr($5); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              | component TOK_DOT TOK_LPAR TOK_MINUS TOK_DIGITS TOK_RPAR
                { $$ = $1->add_tag_num(_ps, $5, 1); _ps->unregdelstr($5); 
		  if (_ps->e) { YYABORT; }
                  _ps->unregcexpr($1); _ps->regcexpr($$); }
              ;
number        : lconst
                { $$ = RDI_Constraint::NewLConst(_ps, $1); _ps->unregdelstr($1);
		  if (_ps->e) { YYABORT; }
                  _ps->regcexpr($$); }
              | dconst
                { $$ = RDI_Constraint::NewDConst(_ps, $1); _ps->unregdelstr($1);
		  if (_ps->e) { YYABORT; }
                  _ps->regcexpr($$); }
              ;
/* XXX It would be more efficient to compute the # without building up a string */
lconst        : TOK_DIGITS
              ;
dconst        : TOK_EXPONENT
              | TOK_DOT TOK_DIGITS
                { $$ = RDI_BUILD_STRING(".", $2); _ps->unregdelstr($2); _ps->regstr($$); }
              | TOK_DIGITS TOK_DOT TOK_DIGITS
                { $$ = RDI_BUILD_STRING($1, ".", $3); _ps->unregdelstr($1); _ps->unregdelstr($3); _ps->regstr($$); }
              | TOK_DOT TOK_EXPONENT
                { $$ = RDI_BUILD_STRING(".", $2); _ps->unregdelstr($2); _ps->regstr($$); }
              | TOK_DIGITS TOK_DOT TOK_EXPONENT
                { $$ = RDI_BUILD_STRING($1, ".", $3); _ps->unregdelstr($1); _ps->unregdelstr($3); _ps->regstr($$); }
              ;
string        : TOK_STRING
                { $$ = RDI_Constraint::NewSConst(_ps, $1); _ps->unregdelstr($1);
		  if (_ps->e) { YYABORT; }
                  _ps->regcexpr($$); }
              ;
/* the only stand-alone idents that are legal are enum values */
enumident     : TOK_IDENT
                { $$ = RDI_Constraint::NewIdent(_ps, $1); _ps->unregdelstr($1);
		  if (_ps->e) { YYABORT; }
                  _ps->regcexpr($$); }
              ;

%%

