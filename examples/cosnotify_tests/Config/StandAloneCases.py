#    Copyright (C) 2003 AT&T Laboratories -- Research
#
#    This file is part of a set of test programs distributed with
#    the omniNotify release (under the cosnotify_tests directory).
#
#    The test programs are free software; you can redistribute and/or modify
#    them under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    The tests are distributed in the hope that they will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this file; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
#    USA.

# module StandAloneCases: provides list StandAloneCases.L: stand-alone test cases

import Clients
import Helpers

######################################################################
# CONFIG PARAMS
######################################################################

REUSE_CHANNEL = 1
VERBOSE = 1

TEST_INDICES = [] # empty list puts no constraints on test indices

######################################################################

class _StandAloneCase:
    pass

######################################################################
# define the exported list StandAloneCases.L:

######################################################################
# grammar checks
######################################################################
t1 = _StandAloneCase()
t1.Tdoc = "Test Use of Invalid Grammar in Filter Construction"
t1.Tname = "GrammarCheck.Invalid"
t1.create = Helpers.GrammarCheck.Create
t1.args = ("FooGram", 1)

t2 = _StandAloneCase()
t2.Tdoc = "Test Use of Valid Grammar in Filter Construction"
t2.Tname = "GrammarCheck.Valid"
t2.create = Helpers.GrammarCheck.Create
t2.args = ("EXTENDED_TCL", 0)

_grammar_checks = [ t1, t2 ]

######################################################################
# filter constraint expression checks
######################################################################

class make_constraint_check:
    def __init__ (self):
	self.__ctr = 1

    def make(self, cexpr, expected):
	c = _StandAloneCase()
	c.Tdoc = "Test Parsing of Filter Constraint Expressions"
	c.Tname = "ConstraintCheck%d" % self.__ctr
	self.__ctr += 1
	c.create = Helpers.ConstraintCheck.Create
	c.args = (cexpr, expected)
	return c

_cmkr = make_constraint_check()
_constraint_checks = [ ]

# some valid constraint expressions
_valid_exprs = [ "true",
		 "$type_name == 'Recipes' and (('sugar' in $.inglist) + ('salt' in $.inglist) + ('flour' in $.inglist) + ('lemon' in $.inglist)) > 2",
		 "$.mylist._length == 2 and ($.mylist[1].val * $.mlist[2].val) >= 1000",
		 "$type_name == 'FlinstoneSpot' and $priority >= 1 and $priority <= 5",
		 "exist $temp._type_id and $temp._type_id == 'short'",
		 "$._repos_id == 'IDL:CosNotification/StructuredEvent:1.0'",
		 "$order.date.high + 2 < $curtime.high",
		 "exist $.EventHeader.variable_header(party)",
		 "$type_name == 'FlinstoneSpot' and not ($event_name == 'Barney')",
		 "$cast(list)._repos_id == 'IDL:TV/CastList:1.0'",
		 "default $._d and $.def_ul == 10000",
		 "$.cost1 + $.cost2 * 5.7 + $.cost3 / 0.25 > 500000.0"
                 ]


# some invalid constraint expressions

_invalid_exprs = [ "<doc></doc>",
		   "foo bar zot" ]

for e in _valid_exprs:
    _constraint_checks.append( _cmkr.make(e, 0) )
for e in _invalid_exprs:
    _constraint_checks.append( _cmkr.make(e, 1) )

######################################################################
# get_constraints checks
######################################################################

t3 = _StandAloneCase()
t3.Tdoc = "Test of get_constraints and get_all_constraints"
t3.Tname = "GetConstraintsCheck1"
t3.create = Helpers.GetConstraintsCheck.Create
t3.args = ( )

_get_constraints_checks = [ t3 ]

######################################################################
# Build final list StandAlongCases.L
######################################################################

L = [ ]
L += _grammar_checks
L += _constraint_checks
L += _get_constraints_checks

