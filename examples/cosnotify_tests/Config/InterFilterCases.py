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

# module InterFilterCases: provides list InterFilterCases.L: inter-filter supplier-consumer test cases

import Clients
import Helpers
from Config import TopConf


######################################################################
# CONFIG PARAMS
######################################################################

VERBOSE = 0
REUSE_CHANNEL = 1

TEST_INDICES = [] # empty list puts no constraints on test indices
#TEST_INDICES = range(832,1040)

PUSH_CLIENTS  = TopConf.IFILT_PUSH_CLIENTS
PULL_CLIENTS  = TopConf.IFILT_PULL_CLIENTS

SUP_BSIZE = 6 # for batches
CON_BSIZE = 4 # for batches

NUM_CON = 1 
NUM_SUP = 1

######################################################################

class _InterFilterCase:
    """consists of:
    + Tdoc  -- test doc
    + Tname -- shorthand for Tdoc
    + c_ifopr  -- which interfilter op to use ('AND' or 'OR') between consumer proxy and admin
    + s_ifopr  -- which interfilter op to use ('AND' or 'OR') between supplier proxy and admin
    + phases -- sequence of _InterFilterPhase
    + numCon
    + numSup
    + conCreate
    + conHelper
    + conBSize
    + supCreate
    + supHelper
    + supBSize
    """
    pass

######################################################################

class _InterFilterPhase:
    """consists of: 
    + Tdoc  -- e.g., 'Phase 1: add filter to consumer proxy'
    + Tname -- shorthand description, e.g., 'cproxy-lt5')
    + remove_filters -- remove all filters before adding new ones (1 == yes, 0 == no)
    + add_constraints -- for case where filter already exists at loc: 
          add_constraints == 1 means add constraints to current filter 
          add_constraints == 0 means create new filter
    + loc   -- (oneof 'cproxy', 'cadmin', 'sproxy', 'sadmin')
    + constraints : seq of [evs, cexpr]
    + stotal -- supply #s 0, 1, ... stotal-1
    + cexpect -- seq of expected values 
    """
    pass

######################################################################
# define some phase constructors
######################################################################

def mk_helper(p, phasenum, remfilters, addcons, loc, expr, exprshort):
    if (remfilters == 1):
	p.Tdoc = "Phase %d: remove all filters, add filter (%s) to %s" % (phasenum, expr, loc)
	p.Tname = "remove_filters-%s-addfilt-%s" % (loc, exprshort)
    elif (addcons == 1):
	p.Tdoc = "Phase %d: add constraint (%s) to existing filter at %s" % (phasenum, expr, loc)
	p.Tname = "%s-addcon-%s" % (loc, exprshort)
    else:
	p.Tdoc = "Phase %d: add filter (%s) to %s" % (phasenum, expr, loc)
	p.Tname = "%s-addfilt-%s" % (loc, exprshort)

# make phase with filter cexpr = $ul < 5
def mk_lt5(phasenum, remfilters, addcons, loc, stotal, cexpect):
    p = _InterFilterPhase()
    mk_helper(p, phasenum, remfilters, addcons, loc, "$ul < 5", "lt5")
    p.remove_filters = remfilters
    p.add_constraints = addcons
    p.loc = loc
    p.constraints = []
    p.constraints.append( [ [], "$ul < 5" ] )
    p.stotal = stotal
    p.cexpect = cexpect
    return p

# make phase with filter cexpr = $ul <= 5
def mk_le5(phasenum, remfilters, addcons, loc, stotal, cexpect):
    p = _InterFilterPhase()
    mk_helper(p, phasenum, remfilters, addcons, loc, "con1:$ul<5 + con2:$ul==5", "le5")
    p.remove_filters = remfilters
    p.add_constraints = addcons
    p.loc = loc
    p.constraints = []
    p.constraints.append( [ [], "$ul < 5" ] )
    p.constraints.append( [ [], "$ul == 5"] )
    p.stotal = stotal
    p.cexpect = cexpect
    return p

# make phase with filter cexpr = $ul > 5
def mk_gt5(phasenum, remfilters, addcons, loc, stotal, cexpect):
    p = _InterFilterPhase()
    mk_helper(p, phasenum, remfilters, addcons, loc, "$ul > 5", "gt5")
    p.remove_filters = remfilters
    p.add_constraints = addcons
    p.loc = loc
    p.constraints = []
    p.constraints.append( [ [], "$ul > 5" ] )
    p.stotal = stotal
    p.cexpect = cexpect
    return p

# make phase with filter cexpr = $ul > 6
def mk_gt6(phasenum, remfilters, addcons, loc, stotal, cexpect):
    p = _InterFilterPhase()
    mk_helper(p, phasenum, remfilters, addcons, loc, "$ul > 6", "gt6")
    p.remove_filters = remfilters
    p.add_constraints = addcons
    p.loc = loc
    p.constraints = []
    p.constraints.append( [ [], "$ul > 6" ]  )
    p.stotal = stotal
    p.cexpect = cexpect
    return p

# make phase with filter cexpr = $ul >= 5
def mk_ge5(phasenum, remfilters, addcons, loc, stotal, cexpect):
    p = _InterFilterPhase()
    mk_helper(p, phasenum, remfilters, addcons, loc, "con1:$ul>5 + con2:$ul==5", "ge5")
    p.remove_filters = remfilters
    p.add_constraints = addcons
    p.loc = loc
    p.constraints = []
    p.constraints.append( [ [], "$ul > 5" ] )
    p.constraints.append( [ [], "$ul == 5" ] )
    p.stotal = stotal
    p.cexpect = cexpect
    return p

# make phase with filter cexpr = $ul == 5
def mk_eq5(phasenum, remfilters, addcons, loc, stotal, cexpect):
    p = _InterFilterPhase()
    mk_helper(p, phasenum, remfilters, addcons, loc, "$ul == 5", "eq5")
    p.remove_filters = remfilters
    p.add_constraints = addcons
    p.loc = loc
    p.constraints = []
    p.constraints.append( [ [], "$ul == 5" ] )
    p.stotal = stotal
    p.cexpect = cexpect
    return p

# make phase with no filters added
def mk_none(phasenum, remfilters, stotal, cexpect):
    p = _InterFilterPhase()
    if (remfilters == 1):
	p.Tdoc = "Phase %d: remove all filters, no filter added" % phasenum
	p.Tname = "remove_filters-nofilter"
    else:
	p.Tdoc = "Phase %d: no filter added" % phasenum
	p.Tname = "nofilter"
    p.remove_filters = remfilters
    p.add_constraints = 0
    p.loc = None
    p.constraints = []
    p.stotal = stotal
    p.cexpect = cexpect
    return p

######################################################################
# helpers
######################################################################

def fillcase(t, s_combo, c_combo):
    t.numCon = NUM_CON
    t.numSup = NUM_SUP
    t.supHelper, sup_client, t.supBSize = s_combo
    t.supCreate = sup_client.Create
    t.conHelper, con_client, t.conBSize = c_combo
    t.conCreate = con_client.Create
    combo_name = t.supHelper.Tname + "." + sup_client.Tname + "." + t.conHelper.Tname + "." + con_client.Tname
    combo_doc = "  " + t.supHelper.Tdoc + "\n  " + sup_client.Tdoc + "\n  " + t.conHelper.Tdoc + "\n  " + con_client.Tdoc
    for p in t.phases:
	t.Tdoc = t.Tdoc + "    " + p.Tdoc + "\n"
	t.Tname = t.Tname + "." + p.Tname
    t.Tdoc = t.Tdoc + combo_doc
    t.Tname = t.Tname + "." + combo_name
    return t

# and semantics
def mkand(s_combo, c_combo, ifopr, loc1, loc2):
    t = _InterFilterCase()
    t.c_ifopr = ifopr
    t.s_ifopr = ifopr
    t.phases = []
    t.phases.append( mk_le5(1, 0, 0, loc1, 10, [1L,2L,3L,4L,5L] ) )
    t.phases.append( mk_ge5(2, 0, 0, loc2, 10, [5L] ) )
    t.phases.append( mk_none(3, 1, 10, [1L,2L,3L,4L,5L,6L,7L,8L,9L,10L] ) )
    t.Tdoc = "InterFilter %s Test : %s-%s\n" % (ifopr, loc1, loc2)
    t.Tname = "InterFilter%s" % ifopr
    fillcase(t, s_combo, c_combo)
    return t

def mkand2(s_combo, c_combo, ifopr, loc1, loc2):
    t = _InterFilterCase()
    t.c_ifopr = ifopr
    t.s_ifopr = ifopr
    t.phases = []
    t.phases.append( mk_le5(1, 0, 0, loc1, 10, [1L,2L,3L,4L,5L] ) )
    t.phases.append( mk_ge5(2, 1, 0, loc2, 10, [5L,6L,7L,8L,9L,10L] ) ) # removes le5 filter
    t.phases.append( mk_le5(3, 0, 0, loc1, 10, [5L] ) ) # adds it back again
    t.phases.append( mk_none(4, 1, 10, [1L,2L,3L,4L,5L,6L,7L,8L,9L,10L] ) ) # no filters
    t.Tdoc = "InterFilter %s Test Variant 2: %s-%s-%s\n" % (ifopr, loc1, loc2, loc1)
    t.Tname = "InterFilter%s" % ifopr
    fillcase(t, s_combo, c_combo)
    return t

# or semantics
def mkor(s_combo, c_combo, ifopr, loc1, loc2):
    t = _InterFilterCase()
    t.c_ifopr = ifopr
    t.s_ifopr = ifopr
    t.phases = []
    t.phases.append( mk_lt5(1, 0, 0, loc1, 10, [1L,2L,3L,4L] ) )
    t.phases.append( mk_gt5(2, 0, 0, loc2, 10, [1L,2L,3L,4L,6L,7L,8L,9L,10L] ) )  # no 5 expected
    t.phases.append( mk_none(3, 1, 10, [1L,2L,3L,4L,5L,6L,7L,8L,9L,10L] ) )
    t.Tdoc = "InterFilter %s Test : %s-%s\n" % (ifopr, loc1, loc2)
    t.Tname = "InterFilter%s" % ifopr
    fillcase(t, s_combo, c_combo)
    return t

def mkor2(s_combo, c_combo, ifopr, loc1, loc2):
    t = _InterFilterCase()
    t.c_ifopr = ifopr
    t.s_ifopr = ifopr
    t.phases = []
    t.phases.append( mk_lt5(1, 0, 0, loc1, 10, [1L,2L,3L,4L] ) )
    t.phases.append( mk_eq5(2, 0, 1, loc1, 10, [1L,2L,3L,4L,5L] ) ) # add constraint to existing filter
    t.phases.append( mk_gt6(3, 0, 0, loc2, 10, [1L,2L,3L,4L,5L,7L,8L,9L,10L] ) )  # no 6 expected
    t.phases.append( mk_none(4, 1, 10, [1L,2L,3L,4L,5L,6L,7L,8L,9L,10L] ) )
    t.Tdoc = "InterFilter %s Test : %s-%s\n" % (ifopr, loc1, loc2)
    t.Tname = "InterFilter%s" % ifopr
    fillcase(t, s_combo, c_combo)
    return t

######################################################################
# AND test cases with and semantics - variant one
######################################################################

# all cproxy variants
def mkand_cproxy_cadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "cproxy", "cadmin")

def mkand_cadmin_cproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "cadmin", "cproxy")

def mkand_cproxy_sproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "cproxy", "sproxy")

def mkand_sproxy_cproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "sproxy", "cproxy")

def mkand_cproxy_sadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "cproxy", "sadmin")

def mkand_sadmin_cproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "sadmin", "cproxy")

# remaining cadmin variants

def mkand_cadmin_sproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "cadmin", "sproxy")

def mkand_sproxy_cadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "sproxy", "cadmin")

def mkand_cadmin_sadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "cadmin", "sadmin")

def mkand_sadmin_cadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "sadmin", "cadmin")

# remaining sproxy/sadmin variants

def mkand_sproxy_sadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "sproxy", "sadmin")

def mkand_sadmin_sproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "AND", "sadmin", "sproxy")

######################################################################
# AND test cases with and semantics - variant two
######################################################################

# all cproxy variants
def mkand2_cproxy_cadmin(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "cproxy", "cadmin")

def mkand2_cadmin_cproxy(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "cadmin", "cproxy")

def mkand2_cproxy_sproxy(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "cproxy", "sproxy")

def mkand2_sproxy_cproxy(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "sproxy", "cproxy")

def mkand2_cproxy_sadmin(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "cproxy", "sadmin")

def mkand2_sadmin_cproxy(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "sadmin", "cproxy")

# remaining cadmin variants

def mkand2_cadmin_sproxy(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "cadmin", "sproxy")

def mkand2_sproxy_cadmin(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "sproxy", "cadmin")

def mkand2_cadmin_sadmin(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "cadmin", "sadmin")

def mkand2_sadmin_cadmin(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "sadmin", "cadmin")

# remaining sproxy/sadmin variants

def mkand2_sproxy_sadmin(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "sproxy", "sadmin")

def mkand2_sadmin_sproxy(s_combo, c_combo):
    return mkand2(s_combo, c_combo, "AND", "sadmin", "sproxy")

######################################################################
# AND test cases with or semantics
######################################################################

# all x-x variants for same x
def mkand_cproxy_cproxy(s_combo, c_combo):
    return mkor(s_combo, c_combo, "AND", "cproxy", "cproxy")

def mkand_cadmin_cadmin(s_combo, c_combo):
    return mkor(s_combo, c_combo, "AND", "cadmin", "cadmin")

def mkand_sproxy_sproxy(s_combo, c_combo):
    return mkor(s_combo, c_combo, "AND", "sproxy", "sproxy")

def mkand_sadmin_sadmin(s_combo, c_combo):
    return mkor(s_combo, c_combo, "AND", "sadmin", "sadmin")

######################################################################
# OR test cases with and semantics
######################################################################

# all c-s variants with cproxy
def mkor_cproxy_sproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "OR", "cproxy", "sproxy")

def mkor_sproxy_cproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "OR", "sproxy", "cproxy")

def mkor_cproxy_sadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "OR", "cproxy", "sadmin")

def mkor_sadmin_cproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "OR", "sadmin", "cproxy")

# all c-s variants with cadmin
def mkor_cadmin_sproxy(s_combo, c_combo):
    return mkand(s_combo, c_combo, "OR", "cadmin", "sproxy")

def mkor_sproxy_cadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "OR", "sproxy", "cadmin")

def mkor_cadmin_sadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "OR", "cadmin", "sadmin")

def mkor_sadmin_cadmin(s_combo, c_combo):
    return mkand(s_combo, c_combo, "OR", "sadmin", "cadmin")

######################################################################
# OR test cases with or semantics - variant one
######################################################################

# all c-c pairs
def mkor_cproxy_cadmin(s_combo, c_combo):
    return mkor(s_combo, c_combo, "OR", "cproxy", "cadmin")

def mkor_cadmin_cproxy(s_combo, c_combo):
    return mkor(s_combo, c_combo, "OR", "cadmin", "cproxy")

def mkor_cproxy_cproxy(s_combo, c_combo):
    return mkor(s_combo, c_combo, "OR", "cproxy", "cproxy")

def mkor_cadmin_cadmin(s_combo, c_combo):
    return mkor(s_combo, c_combo, "OR", "cadmin", "cadmin")

# all s-s pairs
def mkor_sproxy_sadmin(s_combo, c_combo):
    return mkor(s_combo, c_combo, "OR", "sproxy", "sadmin")

def mkor_sadmin_sproxy(s_combo, c_combo):
    return mkor(s_combo, c_combo, "OR", "sadmin", "sproxy")

def mkor_sproxy_sproxy(s_combo, c_combo):
    return mkor(s_combo, c_combo, "OR", "sproxy", "sproxy")

def mkor_sadmin_sadmin(s_combo, c_combo):
    return mkor(s_combo, c_combo, "OR", "sadmin", "sadmin")

######################################################################
# OR test cases with or semantics - variant 2
######################################################################

# all c-c pairs
def mkor2_cproxy_cadmin(s_combo, c_combo):
    return mkor2(s_combo, c_combo, "OR", "cproxy", "cadmin")

def mkor2_cadmin_cproxy(s_combo, c_combo):
    return mkor2(s_combo, c_combo, "OR", "cadmin", "cproxy")

def mkor2_cproxy_cproxy(s_combo, c_combo):
    return mkor2(s_combo, c_combo, "OR", "cproxy", "cproxy")

def mkor2_cadmin_cadmin(s_combo, c_combo):
    return mkor2(s_combo, c_combo, "OR", "cadmin", "cadmin")

# all s-s pairs
def mkor2_sproxy_sadmin(s_combo, c_combo):
    return mkor2(s_combo, c_combo, "OR", "sproxy", "sadmin")

def mkor2_sadmin_sproxy(s_combo, c_combo):
    return mkor2(s_combo, c_combo, "OR", "sadmin", "sproxy")

def mkor2_sproxy_sproxy(s_combo, c_combo):
    return mkor2(s_combo, c_combo, "OR", "sproxy", "sproxy")

def mkor2_sadmin_sadmin(s_combo, c_combo):
    return mkor2(s_combo, c_combo, "OR", "sadmin", "sadmin")

######################################################################
# all test case makers
######################################################################

_all_and_makers = []
_all_and_makers.append( mkand_cproxy_cadmin )
_all_and_makers.append( mkand_cadmin_cproxy )
_all_and_makers.append( mkand_cproxy_sproxy )
_all_and_makers.append( mkand_sproxy_cproxy )
_all_and_makers.append( mkand_cproxy_sadmin )
_all_and_makers.append( mkand_sadmin_cproxy )
_all_and_makers.append( mkand_cadmin_sproxy )
_all_and_makers.append( mkand_sproxy_cadmin )
_all_and_makers.append( mkand_cadmin_sadmin )
_all_and_makers.append( mkand_sadmin_cadmin )
_all_and_makers.append( mkand_sproxy_sadmin )
_all_and_makers.append( mkand_sadmin_sproxy )

_all_and_makers.append( mkand2_cproxy_cadmin )
_all_and_makers.append( mkand2_cadmin_cproxy )
_all_and_makers.append( mkand2_cproxy_sproxy )
_all_and_makers.append( mkand2_sproxy_cproxy )
_all_and_makers.append( mkand2_cproxy_sadmin )
_all_and_makers.append( mkand2_sadmin_cproxy )
_all_and_makers.append( mkand2_cadmin_sproxy )
_all_and_makers.append( mkand2_sproxy_cadmin )
_all_and_makers.append( mkand2_cadmin_sadmin )
_all_and_makers.append( mkand2_sadmin_cadmin )
_all_and_makers.append( mkand2_sproxy_sadmin )
_all_and_makers.append( mkand2_sadmin_sproxy )

_all_and_makers.append( mkand_cproxy_cproxy )
_all_and_makers.append( mkand_cadmin_cadmin )
_all_and_makers.append( mkand_sproxy_sproxy )
_all_and_makers.append( mkand_sadmin_sadmin )

_all_or_makers = []
_all_or_makers.append( mkor_cproxy_sproxy )
_all_or_makers.append( mkor_sproxy_cproxy )
_all_or_makers.append( mkor_cproxy_sadmin )
_all_or_makers.append( mkor_sadmin_cproxy )
_all_or_makers.append( mkor_cadmin_sproxy )
_all_or_makers.append( mkor_sproxy_cadmin )
_all_or_makers.append( mkor_cadmin_sadmin )
_all_or_makers.append( mkor_sadmin_cadmin )

_all_or_makers.append( mkor_cproxy_cadmin )
_all_or_makers.append( mkor_cadmin_cproxy )
_all_or_makers.append( mkor_cproxy_cproxy )
_all_or_makers.append( mkor_cadmin_cadmin )
_all_or_makers.append( mkor_sproxy_sadmin )
_all_or_makers.append( mkor_sadmin_sproxy )
_all_or_makers.append( mkor_sproxy_sproxy )
_all_or_makers.append( mkor_sadmin_sadmin )

_all_or_makers.append( mkor2_cproxy_cadmin )
_all_or_makers.append( mkor2_cadmin_cproxy )
_all_or_makers.append( mkor2_cproxy_cproxy )
_all_or_makers.append( mkor2_cadmin_cadmin )
_all_or_makers.append( mkor2_sproxy_sadmin )
_all_or_makers.append( mkor2_sadmin_sproxy )
_all_or_makers.append( mkor2_sproxy_sproxy )
_all_or_makers.append( mkor2_sadmin_sadmin )

_all_makers = _all_and_makers + _all_or_makers

######################################################################
# clients
######################################################################

_any_sup_clients     = []
_any_con_clients     = []
_struct_sup_clients  = []
_struct_con_clients  = []
_batch_sup_clients   = []
_batch_con_clients   = []

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

_any_sup_helpers = [ Helpers.Sup_Any_MySt ]
_sev_sup_helpers = [ Helpers.Sup_ULEvent  ]

_any_con_helpers = [ Helpers.Con_Any_MySt ]
_sev_con_helpers = [ Helpers.Con_ULEvent  ]

######################################################################
# helper-client combos (with batch size)
######################################################################

_any_sup_combos = []
_any_con_combos = []
_sev_sup_combos = []
_sev_con_combos = []

for c in _any_sup_clients:
    for h in _any_sup_helpers:
	_any_sup_combos.append( [h, c, 1] )

for c in _any_con_clients:
    for h in _any_con_helpers:
	_any_con_combos.append( [h, c, 1] )

for c in _struct_sup_clients:
    for h in _sev_sup_helpers:
	_sev_sup_combos.append( [h, c, 1] )

for c in _struct_con_clients:
    for h in _sev_con_helpers:
	_sev_con_combos.append( [h, c, 1] )

for c in _batch_sup_clients:
    for h in _sev_sup_helpers:
	_sev_sup_combos.append( [h, c, SUP_BSIZE] )

for c in _batch_con_clients:
    for h in _sev_con_helpers:
	_sev_con_combos.append( [h, c, CON_BSIZE] )

# build list of (sup_combo, con_combo) pairs that are compatible
_compat = []
for sup in _any_sup_combos:
    for con in _any_con_combos:
	_compat.append( [sup, con] )
for sup in _sev_sup_combos:
    for con in _sev_con_combos:
	_compat.append( [sup, con] )

# build list of compat test cases
_compat_cases = []
for sup, con in _compat:
    for mkr in _all_makers:
	_compat_cases.append( mkr(sup, con) )

######################################################################################

# define the exported list InterFilterCases.L:

L = _compat_cases

