#!/bin/sh -f

FIXLEXOUT='fixlexout.pl'
FIXYACCOUT='fixyaccout.pl'
LEX_SOURCE='lex.l'
YACC_SOURCE='yacc.y'
LEX='flex -i -+ -Cf' 
YACC='bison -d -y' 
INCLUDE_DIRS=

$YACC -d $YACC_SOURCE $INCLUDE_DIRS

$FIXYACCOUT

$LEX lex.l

$FIXLEXOUT

