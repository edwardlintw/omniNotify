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

# Module Sup_Any_MyUnType

# Notes: Sup_Any_MyUnType sets up a supplier for CORBA::Any
# events that contain MyUnTest.MyUnType values.
#
# Some facts about the supplied values:
#
#  *  1 in 10 have tagnum 0, _d e0, tagid e0_ar5 and value
#        a string array such that Sup_Any_MyUnType.matchstr is at index 2
#  +  1 in 10 have tagnum 2, _d e2, tagid e2_str and value 'foo'
#  +  1 in 10 have tagnum 5, _d e5, tagid e5_l   and value 100
#  +  1 in 10 have tagnum 6, _d e6, tagid e6_s   and value 27
#  +  1 in 10 have tagnum 7, _d e7, tagid def_b  and value 0 (false)
#
#  *  2 in 10 have tagnum 0
#  *  2 in 10 have tagnum 2
#  *  2 in 10 have tagnum 5
#  *  2 in 10 have tagnum 6
#  *  2 in 10 have tagnum 7
#
#  *  (other interesting facts??)

from omniORB import CORBA
import CosNotification
import MyUnTest
from MyUnTest import MyUnType

# public state
Tdoc = "Supply CORBA::Any vals containing MyUnTest.MyUnType values"
Tname = "Sup_Any_MyUnType"

match_str = "match string"
match_long = 100
match_short = 27
match_bool = 1
not_match_bool = 0

sup_offr = [ CosNotification.EventType("","%ANY") ] 
sup_constraints = None
sup_chfn = None
sup_msec = 0

# maps indexed by idx arg
sup_total = {} 

# private state
__str_array1 = ["StringAtIndex0", "StringAtIndex1", match_str, "StringAtIndex3", "StringAtIndex4" ]
__str_array2 = ["StringAtIndex0", "StringAtIndex1", "StringAtIndex2", "StringAtIndex3", "StringAtIndex4" ]

def sup_evfn(objnm, idx, bsize, num_events, num_batches, verbose):
    "supply an any containing a MyUnType value"
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
	if (counter == 0):
	    u = MyUnType(e0_ar5 = __str_array1)
	elif (counter == 1):
	    u = MyUnType(e0_ar5 = __str_array2)
	elif (counter == 2):
	    u = MyUnType(e2_str = match_str)
	elif (counter == 3):
	    u = MyUnType(e2_str = 'NOT ' + match_str)
	elif (counter == 4):
	    u = MyUnType(e5_l = match_long)
	elif (counter == 5):
	    u = MyUnType(e5_l = match_long + 1)
	elif (counter == 6):
	    u = MyUnType(e6_s = match_short)
	elif (counter == 7):
	    u = MyUnType(e6_s = match_short + 1)
	elif (counter == 8):
	    u = MyUnType(def_b = match_bool)
	elif (counter == 9):
	    u = MyUnType(def_b = not_match_bool)
	tc = CORBA.TypeCode(CORBA.id(MyUnType))
	a = CORBA.Any(tc, u)
	if (verbose > 0):
	    print objnm, ": sup_evfn: union value _d = ", u._d
	    print objnm, ": sup_evfn: union value _v = ", u._v
	    print objnm, ": sup_evfn: any value has typecode kind =", a.typecode().kind()
	    print objnm, ": sup_evfn: any value =", a.value(a.typecode())
	batch.append(a)
    return batch
