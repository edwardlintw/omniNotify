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

# Module Con_Any_MySt
# Notes: sets up a consumer for CORBA::Any events that contain
# MyStTest.MySt values.  Does not define a filter.
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
Tdoc = "Consume CORBA::Any vals containing MyStTest.MySt values"
Tname = "Con_Any_MySt"

con_constraints = None
con_chfn = None
con_msec = 0
con_pcnt = 1.0

# maps indexed by idx arg
con_total = {}
con_valid = {}
consumed  = {} # used to track the consumed numbers

def con_evfn(batch, objnm, idx, num_events, num_batches, verbose):
    "consume an any, append ul to consumed"
    # initialize consume stats for idx, if necessary
    if (not con_valid.has_key(idx)):
	con_valid[idx] = 0
    if (not con_total.has_key(idx)):
	con_total[idx] = 0
    if (not consumed.has_key(idx)):
	consumed[idx] = {}
    # the rest of the consume method
    if (len(batch) != 1):
	print "Internal Error (?): any consumer should not get an event batch"
    # process like a batch regardless
    for a in batch:
	con_total[idx] += 1
	try:
	    vs = a.value(a.typecode())
	    v = vs.ul
	    consumed[idx].append(v)
	    if (verbose > 0):
		print objnm, ": con_evfn: consumed numeric value", v
	    con_valid[idx] += 1
	    if (verbose > 0):
		print objnm, ": con_evfn: Valid events consumed now = ", con_valid[idx]
	except:
	    if (verbose > 0):
		print objnm, ": con_evfn: error, perhaps ANY does not contain MySt value"

