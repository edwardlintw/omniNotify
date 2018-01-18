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

# Module Sup_Any_String

# Notes: sets up a supplier of CORBA::Any
# events that contain string values
# Cycles through ten values, only one of which
# contains the string 'a match string'

from omniORB import CORBA
import CosNotification

# public state
Tdoc = "Supply CORBA::Any vals containing string values"
Tname = "Sup_Any_String"

sup_offr = [ CosNotification.EventType("","%ANY") ] 
sup_constraints = None
sup_chfn = None
sup_msec = 0

# maps indexed by idx arg
sup_total = {} 

# private state
_foo_strings = [ "foo one", "foo two", "foo three", "a match string", "foo five",
		 "foo six", "foo seven", "foo eight", "foo nine", "foo ten" ]

def sup_evfn(objnm, idx, bsize, num_events, num_batches, verbose):
    "supply an any. 1 in 10 contains an appropriate ulong value"
    # initialize supply stats for idx, if necessary
    if (not sup_total.has_key(idx)):
	sup_total[idx] = 0
    # the rest of the supply method
    batch = [ ]
    while (len(batch) < bsize): 
	if (verbose > 0):
	    print objnm, ": supplying event #", num_events
	counter = sup_total[idx] % 10
	sup_total[idx] += 1
	num_events += 1
	a = CORBA.Any(CORBA._tc_string, _foo_strings[counter])
	if (verbose > 0):
	    print objnm, ': sup_evfn: any value = "' + _foo_strings[counter] + '"'
	batch.append(a)
    return batch
