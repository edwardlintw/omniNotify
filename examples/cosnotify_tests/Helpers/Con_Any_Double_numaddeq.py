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

# Module Con_Any_numaddeq
# Notes: sets up a consumer for CORBA::Any events that contain 
# numeric values.  Uses constraint '$ + 55  == 62', thus when paired with a supplier
# that cycles through values from 0 to 9, 1 in 10 will match.

from omniORB import CORBA
import CosNotification
from Utils import NumUtils

# public state
Tdoc = "Consume CORBA::Any vals containing numeric val where val + 55 = 62 (val must be 7)"
Tname = "Con_Any_Double_numaddeq"

#_constraint1 = [ [CosNotification.EventType("","%ANY")], "$ + 55 == 62" ]
_constraint1 = [ [CosNotification.EventType("","%ANY")], "($ + 55 > 61.9) and ($ + 55 < 62.1) " ]
con_constraints = [ _constraint1 ]
con_chfn = None
con_msec = 0
con_pcnt = .10

# maps indexed by idx arg
con_total = {}
con_valid = {}

def con_evfn(batch, objnm, idx, num_events, num_batches, verbose):
    "consume an any, verify it contains the appropriate value"
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
	v = NumUtils.any2number(a)
	if (v == None):
	    if (verbose > 0):
		print objnm, ": con_evfn: unexpected value consumed (not a number)"
	    break
	if (verbose > 0):
	    print objnm, ": con_evfn: consumed numeric value", v
	if (v != 7.0):
	    if (verbose > 0):
		print objnm, ": con_evfn: value should have been 7!"
	else:
	    con_valid[idx] += 1
	    if (verbose > 0):
		print objnm, ": con_evfn: Valid events consumed now = ", con_valid[idx]
