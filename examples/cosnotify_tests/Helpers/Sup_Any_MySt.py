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

# Module Sup_Any_MySt

# Notes: sets up a supplier for CORBA::Any
# events that contain MyStTest.MySt values.
#
# Some facts about the supplied values:
#   ul field ranges from 1 .. max_events
#
# module MyStTest {
#   struct MySt {
#     unsigned long ul;
#   };
# };
#

from omniORB import CORBA
import CosNotification
from MyStTest import MySt

# public state
Tdoc = "Supply CORBA::Any vals containing MyStTest.MySt values"
Tname = "Sup_Any_MySt"

sup_offr = [ CosNotification.EventType("","%ANY") ] 
sup_constraints = None
sup_chfn = None
sup_msec = 0

# maps indexed by idx arg
sup_total = {} 

# private state

def sup_evfn(objnm, idx, bsize, num_events, num_batches, verbose):
    "supply an any containing a MyStTest.MySt value"
    # initialize supply stats for idx, if necessary
    if (not sup_total.has_key(idx)):
	sup_total[idx] = 0
    # the rest of the supply method
    batch = [ ]
    while (len(batch) < bsize):
	if (verbose > 0):
	    print objnm, ": supplying event #", num_events
	vs = MySt(num_events)
	tc = CORBA.TypeCode(CORBA.id(MySt))
	a = CORBA.Any(tc, vs)
	sup_total[idx] += 1
	num_events += 1
	if (verbose > 0):
	    print objnm, ": sup_evfn: supplying MySt with ul = ", vs.ul
	batch.append(a)
    return batch
