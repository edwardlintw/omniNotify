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

# Module Con_Any_MyUnType_tagnum_shorteq

# Notes: sets up a consumer for CORBA::Any
# events that contain MyUnTest.MyUnType values.
# Matches the case where union tagnum is 6 (_d is e6, tagid is e6_s)
# and value Sup_Any_MyUnType.match_short
# Given behavior of Sup_Any_MyUnType, 1/10 matches occur.

from omniORB import CORBA
import CosNotification
import MyUnTest
from MyUnTest import MyUnType
from Helpers import Sup_Any_MyUnType

# public state
Tdoc = "Consume CORBA::Any vals containing MyUnType values with tagnum 6"
Tname = "Con_Any_MyUnType_tagnum_shorteq"

_constraint1 = [ [CosNotification.EventType("","%ANY")], "$.(6) == %d" % Sup_Any_MyUnType.match_short ]
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
	if (v._d != MyUnTest.e6):
	    if (verbose > 0):
		print objnm, ": con_evfn: wrong discriminator value"
	elif (v._v != Sup_Any_MyUnType.match_short):
	    if (verbose > 0):
		print objnm, ': con_evfn: short at index 2 (', v._v, ') != match_short (', Sup_Any_MyUnType.match_short, ')'
	else:
	    con_valid[idx] += 1

