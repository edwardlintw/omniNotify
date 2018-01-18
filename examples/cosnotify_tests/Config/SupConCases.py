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

# module SupConCases: provides list SupConCases.L: supplier-consumer test cases

import TopConf
import Clients
import Helpers


######################################################################
# CONFIG PARAMS
######################################################################

VERBOSE = 0
REUSE_CHANNEL = 1

TEST_INDICES = [] # empty list puts no constraints on test indices

# This sequence caused problems before the deadlock bug for 1.1 was fixed (1.1 bug # 6)
#TEST_INDICES = [ 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195 ]

# This sequence causes problems when the python test suite runs on a Linux box and the notifd runs on a Sun
# (or vice versa???)
#TEST_INDICES = range(60, 80) + range(140, 160)

# test cases involving Con_Any_Double_numgt
#TEST_INDICES = [ 64, 74, 144, 154 ]

# This list covers one case each of any sup/con push/pull and struct sup/con push/pull
# *** THIS SET OF INDICES IS A VERY GOOD STARTING POINT FOR TESTING ***
TEST_INDICES = [ 0, 10, 80, 90, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254, 256, 258, 260, 262 ]

DO_NORMAL     = TopConf.SUPCON_DO_NORMAL
DO_SPECIAL    = TopConf.SUPCON_DO_SPECIAL
DO_INCOMPAT   = TopConf.SUPCON_DO_INCOMPAT
PUSH_CLIENTS  = TopConf.SUPCON_PUSH_CLIENTS
PULL_CLIENTS  = TopConf.SUPCON_PULL_CLIENTS

NUM_SUP = 2
NUM_CON = 3

SUP_BSIZE = 6 # for batches
CON_BSIZE = 4 # for batches
SUPPLY_TOTAL = 120

class _SupConCase:
    pass

######################################################################
# aux functions
######################################################################

def make_test_case(sup, con, nsup, ncon, sup_tot, con_tot, con_valid):
    sup_helper, sup_client, sup_bsize = sup
    con_helper, con_client, con_bsize = con
    c = _SupConCase()
    c.Tdoc      = "  " + sup_helper.Tdoc + "\n  " + sup_client.Tdoc + "\n  " + con_helper.Tdoc + "\n  " + con_client.Tdoc
    c.numSup    = nsup
    c.numCon    = ncon
    c.Tname     = sup_helper.Tname + "." + sup_client.Tname + "." + con_helper.Tname + "." + con_client.Tname
    c.supCreate = sup_client.Create
    c.supHelper = sup_helper
    c.conCreate = con_client.Create
    c.conHelper = con_helper
    c.supTotal  = sup_tot
    c.supBSize  = sup_bsize
    c.conTotal  = con_tot
    c.conBSize  = con_bsize
    c.conValid  = con_valid
    return c

def make_compat_test_case(sup, con, nsup, ncon, tot):
    contot = convalid = int(nsup * tot * con[0].con_pcnt)
    return make_test_case(sup, con, nsup, ncon, tot, contot, convalid)

def make_incompat_test_case(sup, con, nsup, ncon, tot):
    return make_test_case(sup, con, nsup, ncon, tot, 0, 0)

######################################################################
# clients
######################################################################

_any_sup_clients     = [ ]
_any_con_clients     = [ ]
_struct_sup_clients  = [ ]
_struct_con_clients  = [ ]
_batch_sup_clients   = [ ]
_batch_con_clients   = [ ]

if (PUSH_CLIENTS == 1):
    _any_sup_clients     += [ Clients.AnyPushSup ]
    _any_con_clients     += [ Clients.AnyPushCon ]
    _struct_sup_clients  += [ Clients.StructPushSup ]
    _struct_con_clients  += [ Clients.StructPushCon ]
    _batch_sup_clients   += [ Clients.BatchPushSup ]
    _batch_con_clients   += [ Clients.BatchPushCon ]


if (PULL_CLIENTS == 1):
    _any_sup_clients     += [ Clients.AnyPullSup ]
    _any_con_clients     += [ Clients.AnyPullCon ]
    _struct_sup_clients  += [ Clients.StructPullSup ]
    _struct_con_clients  += [ Clients.StructPullCon ]
    _batch_sup_clients   += [ Clients.BatchPullSup ]
    _batch_con_clients   += [ Clients.BatchPullCon ]

######################################################################
# helpers
######################################################################

_numeric_sup_helpers = [ Helpers.Sup_Any_ULong,
			 Helpers.Sup_Any_Long,
			 Helpers.Sup_Any_Float,
			 Helpers.Sup_Any_Double ]

_numeric_con_helpers = [ Helpers.Con_Any_numeq,
			 Helpers.Con_Any_numne,
			 Helpers.Con_Any_numlt,
			 Helpers.Con_Any_numle,
			 Helpers.Con_Any_numgt,
			 Helpers.Con_Any_numge,
			 Helpers.Con_Any_numaddeq,
			 Helpers.Con_Any_numsubeq,
			 Helpers.Con_Any_nummuleq,
			 Helpers.Con_Any_numdiveq ]

_string_sup_helpers = [ Helpers.Sup_Any_String ]

_string_con_helpers = [ Helpers.Con_Any_streq,
			Helpers.Con_Any_strne,
			Helpers.Con_Any_substr ]

_uniontype_sup_helpers = [ Helpers.Sup_Any_MyUnType ]

_uniontype_con_helpers = [ Helpers.Con_Any_MyUnType_tagdef_boolneq,
			   Helpers.Con_Any_MyUnType_tagid_index_streq,
			   Helpers.Con_Any_MyUnType_tagid_longeq,
			   Helpers.Con_Any_MyUnType_dotid_longeq,
			   Helpers.Con_Any_MyUnType_dotid_streq,
			   Helpers.Con_Any_MyUnType_tagnum_index_streq,
			   Helpers.Con_Any_MyUnType_tagnum_longeq,
			   Helpers.Con_Any_MyUnType_tagnum_shorteq,
			   Helpers.Con_Any_MyUnType_tagnum_streq,
			   Helpers.Con_Any_MyUnType_tagnum_index_EXIST,
			   Helpers.Con_Any_MyUnType_tagnum_index_IN,
			   Helpers.Con_Any_MyUnType_tagnum_LENGTH,
			   Helpers.Con_Any_MyUnType_tagnum_index_streq_NOT,
			   Helpers.Con_Any_MyUnType_tagnum_index_streq_OR,
			   Helpers.Con_Any_MyUnType_tagnum_index_streq_AND ]

_stockbond_sup_helpers = [ Helpers.Sup_StockBondTicker ]

_stockbond_con_helpers = [ Helpers.Con_StockBondTicker_dnameeq,
			   Helpers.Con_StockBondTicker_fieldstreq ]

_special_numeric_con_helpers = [ Helpers.Con_Any_divbyzero ]

######################################################################
# helper-client combos (with batch size)
######################################################################

_any_numeric_sup_combos   = [ ]
_any_numeric_con_combos   = [ ]
_any_string_sup_combos    = [ ]
_any_string_con_combos    = [ ]
_any_uniontype_sup_combos = [ ]
_any_uniontype_con_combos = [ ]
_stockbond_sup_combos     = [ ]
_stockbond_con_combos     = [ ]

for c in _any_sup_clients:
    for h in _numeric_sup_helpers:
	_any_numeric_sup_combos.append( [h, c, 1] )
    for h in _string_sup_helpers:
	_any_string_sup_combos.append( [h, c, 1] )
    for h in _uniontype_sup_helpers:
	_any_uniontype_sup_combos.append( [h, c, 1] )

for c in _any_con_clients:
    for h in _numeric_con_helpers:
	_any_numeric_con_combos.append( [h, c, 1] )
    for h in _string_con_helpers:
	_any_string_con_combos.append( [h, c, 1] )
    for h in _uniontype_con_helpers:
	_any_uniontype_con_combos.append( [h, c, 1] )

for c in _struct_sup_clients:
    for h in _stockbond_sup_helpers:
	_stockbond_sup_combos.append( [h, c, 1] )

for c in _struct_con_clients:
    for h in _stockbond_con_helpers:
	_stockbond_con_combos.append( [h, c, 1] )

for c in _batch_sup_clients:
    for h in _stockbond_sup_helpers:
	_stockbond_sup_combos.append( [h, c, SUP_BSIZE] )

for c in _batch_con_clients:
    for h in _stockbond_con_helpers:
	_stockbond_con_combos.append( [h, c, CON_BSIZE] )

# special cases where expected case is nothing is consumed
_special_any_numeric_con_combos = [ ]
for c in _any_con_clients:
    for h in _special_numeric_con_helpers:
	_special_any_numeric_con_combos.append( [h, c, 1] )

# build lists of all sup/con combos

_all_any_sup_combos = [ ]
_all_any_sup_combos += _any_numeric_sup_combos
_all_any_sup_combos += _any_string_sup_combos
_all_any_sup_combos += _any_uniontype_sup_combos

_all_any_con_combos = [ ]
_all_any_con_combos += _any_numeric_con_combos 
_all_any_con_combos += _special_any_numeric_con_combos 
_all_any_con_combos += _any_string_con_combos 
_all_any_con_combos += _any_uniontype_con_combos

_all_ev_sup_combos = [ ]
_all_ev_sup_combos += _stockbond_sup_combos

_all_ev_con_combos = [ ]
_all_ev_con_combos += _stockbond_con_combos

_all_sup_combos = _all_any_sup_combos + _all_ev_sup_combos
_all_con_combos = _all_any_con_combos + _all_ev_con_combos

# build list of (sup_combo, con_combo) pairs that are compatible
_compat = [ ]
if (DO_NORMAL == 1):
    for sup in _any_numeric_sup_combos:
	for con in _any_numeric_con_combos:
	    _compat.append( [sup, con] )
    for sup in _any_string_sup_combos:
	for con in _any_string_con_combos:
	    _compat.append( [sup, con] )
    for sup in _any_uniontype_sup_combos:
	for con in _any_uniontype_con_combos:
	    _compat.append( [sup, con] )
    for sup in _stockbond_sup_combos:
	for con in _stockbond_con_combos:
	    _compat.append( [sup, con] )

# build list of (sup_combo, con_combo) pairs that are DESIGNED to produce
# special zero-events-consumed cases

_special = [ ]
if (DO_SPECIAL == 1):
    for sup in _any_numeric_sup_combos:
	for con in _special_any_numeric_con_combos:
	    _special.append( [sup, con] )

# build list of (sup_combo, con_combo) pairs that are incompat
_incompat = [ ]
if (DO_INCOMPAT == 1):
    for sup in _all_sup_combos:
	for con in _all_con_combos:
	    pair = [sup, con]
	    if (( _compat.count(pair) == 0) and ( _special.count(pair) == 0)):
		_incompat.append(pair)

# XXX TMP
#_tmp_compat = [ [ [Helpers.Sup_Any_String, Clients.AnyPushSup, 1],
#		  [Helpers.Con_Any_substr,  Clients.AnyPushCon, 1] ] ]
#_compat = _tmp_compat

# build list of compat test cases
_compat_cases = [ ]
for sup, con in _compat:
    sup_helper, sup_client, sup_bsize = sup
    con_helper, con_client, con_bsize = con
    # Use special consumer helpers when we supply Doubles
    if (sup_helper == Helpers.Sup_Any_Double):
	if (con_helper == Helpers.Con_Any_numeq):
	    con_helper = Helpers.Con_Any_Double_numeq
	if (con_helper == Helpers.Con_Any_numne):
	    con_helper = Helpers.Con_Any_Double_numne
	if (con_helper == Helpers.Con_Any_numlt):
	    con_helper = Helpers.Con_Any_Double_numlt
	if (con_helper == Helpers.Con_Any_numle):
	    con_helper = Helpers.Con_Any_Double_numle
	if (con_helper == Helpers.Con_Any_numgt):
	    con_helper = Helpers.Con_Any_Double_numgt
	if (con_helper == Helpers.Con_Any_numge):
	    con_helper = Helpers.Con_Any_Double_numge
	if (con_helper == Helpers.Con_Any_numaddeq):
	    con_helper = Helpers.Con_Any_Double_numaddeq
	if (con_helper == Helpers.Con_Any_numsubeq):
	    con_helper = Helpers.Con_Any_Double_numsubeq
	if (con_helper == Helpers.Con_Any_nummuleq):
	    con_helper = Helpers.Con_Any_Double_nummuleq
	if (con_helper == Helpers.Con_Any_numdiveq):
	    con_helper = Helpers.Con_Any_Double_numdiveq
    modsup = [ sup_helper, sup_client, sup_bsize ]
    modcon = [ con_helper, con_client, con_bsize ]
    _compat_cases.append(make_compat_test_case(modsup, modcon, NUM_SUP, NUM_CON, SUPPLY_TOTAL))

# build list of special test cases
_special_cases = [ ]
for sup, con in _special:
    _special_cases.append(make_compat_test_case(sup, con, NUM_SUP, NUM_CON, SUPPLY_TOTAL))

# build list of incompat test cases
_incompat_cases = [ ]
for sup, con in _incompat:
    _incompat_cases.append(make_incompat_test_case(sup, con, NUM_SUP, NUM_CON, SUPPLY_TOTAL))

######################################################################################

# define the exported list SupConCases.L:

L = _compat_cases + _special_cases + _incompat_cases

