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

# Module Con_Any_MyUnType_tagnum_index_streq

# Notes: sets up a consumer for CORBA::Any
# events that contain MyUnTest.MyUnType values.
# Matches the case where union tagnum is 0 (_d is e0, tagid is e0_ar5)
# and value has string array with string Sup_Any_MyUnType.matchstr at index 2.
# Given behavior of Sup_Any_MyUnType, 1/10 matches occur.

from omniORB import CORBA
import CosNotification
import MyUnTest
from MyUnTest import MyUnType
from Helpers import Sup_Any_MyUnType

# public state
Tdoc = "Consume CORBA::Any vals containing MyUnType values with tagnum 0"
Tname = "Con_Any_MyUnType_tagnum_index_streq"

_constraint1 = [ [CosNotification.EventType("","%ANY")], "$.(0)[2] == '%s'" % Sup_Any_MyUnType.match_str ]
con_constraints = [ _constraint1 ]
con_chfn = None
con_msec = 0
con_pcnt = .10

# maps indexed by idx arg
con_total = {}
con_valid = {}

def con_evfn(batch, objnm, idx, num_events, num_batches, verbose):
    "consume an any, verify it contains the appropriate MyUnTest value"
    # initialize consume stats for idx, if necessary
    if (not con_valid.has_key(idx)):
	con_valid[idx] = 0
    if (not con_total.has_key(idx)):
	con_total[idx] = 0
    # the rest of the consume method
    if (len(batch) != 1):
	print "Internal Error (?): any consumer should not get an event batch"
    # process like a batch regardless
    for a in batch:
	con_total[idx] += 1
	v = a.value(CORBA.TypeCode(CORBA.id(MyUnType)))
	if (v == None):
	    if (verbose > 0):
		print objnm, ": con_evfn: unexpected non-union value consumed"
		break
	if (verbose > 0):
	    print objnm, ": con_evfn: consumed union value", v._v
	if (v._d != MyUnTest.e0):
	    if (verbose > 0):
		print objnm, ": con_evfn: wrong discriminator value"
	elif (v._v[2] != Sup_Any_MyUnType.match_str):
	    if (verbose > 0):
		print objnm, ': con_evfn: string at index 2 (', v._v[2], ') != match_str (', Sup_Any_MyUnType.match_str, ')'
	else:
	    con_valid[idx] += 1

