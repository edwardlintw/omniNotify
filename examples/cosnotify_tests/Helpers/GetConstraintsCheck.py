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

# Module GetConstraintsCheck defines GetConstraintsCheck.C, GetConstraintsCheck.Create

from omniORB import CORBA
import CosNotifyFilter # import stubs

class C:
    """Test the get_all_constraints, get_constraints, remove_all_constraints calls
    """
    def __init__(self, chan, verbose, args):
	"""
	args is not used
	"""
	self.__chan    = chan
	self.__verbose = verbose
	self.__ff = None
	try:
	    self.__ff = chan._get_default_filter_factory()
	except (CORBA.COMM_FAILURE):
	    print "ConstraintCheck: Caught COMM_FAILURE obtaining filter factory"

    def run(self):
	"attempt to create filter, add cosntraint, get_all_constraints, get_constraints"
	filter = None
	if (self.__ff == None):
	    return 1, "GetConstraintsCheck: no filter factory"  # error
	try:
	    filter = self.__ff.create_filter("EXTENDED_TCL")
	except CosNotifyFilter.InvalidGrammar:
	    return 1, "GetConstraintsCheck: filter construction failed" # error
	except: # some other exception (unexpected)
	    return 1, "GetConstraintsCheck: unexpected exception constructing filter" # error
	if (self.__verbose > 0):
	    print "GetConstraintsCheck: add this constraint:"
	    print "  $i == 1"
	exp1 = CosNotifyFilter.ConstraintExp([ ], "$i == 1")
	exp = [exp1]
	try:
	    ci = filter.add_constraints(exp)
	    if (len(ci) != 1):
		msg = "GetConstraintsCheck: add_constraints returned seq len %d, expecting len 1" % len(ci)
		return 1, msg
	except CosNotifyFilter.InvalidConstraint:
	    return 1, "GetConstraintsCheck: add_constraints failure"
	except:
	    return 1, "GetConstraintsCheck: unexpected add_constraints failure"
	if (self.__verbose > 0):
	    print "GetConstraintsCheck: add this constraint:"
	    print "  $j == 1"
	exp2 = CosNotifyFilter.ConstraintExp([ ], "$j == 1")
	exp = [exp2]
	try:
	    ci = filter.add_constraints(exp)
	    if (len(ci) != 1):
		msg = "GetConstraintsCheck: add_constraints returned seq len %d, expecting len 1" % len(ci)
		return 1, msg
	except CosNotifyFilter.InvalidConstraint:
	    return 1, "GetConstraintsCheck: add_constraints failure"
	except:
	    return 1, "GetConstraintsCheck: unexpected add_constraints failure"
	constraints = None
	if (self.__verbose > 0):
	    print "GetConstraintsCheck: calling get_all_constraints"
	try:
	    constraints = filter.get_all_constraints()
	except:
	    return 1, "GetConstraintsCheck: get_all_constraints failure"
	if (len(constraints) != 2):
	    msg = "GetConstraintsCheck: get_all_constraints returned seq of len %d, expecting len 2" % len(constraints)
	    return 1, msg
	idlist = [ constraints[0].constraint_id, constraints[1].constraint_id ]
	constraints = None
	if (self.__verbose > 0):
	    print "GetConstraintsCheck: calling get_constraints"
	try: 
	    constraints = filter.get_constraints(idlist)
	except:
	    return 1, "GetConstraintsCheck: get_constraints failure"
	if (len(constraints) != 2):
	    msg = "GetConstraintsCheck: get_constraints returned seq of len %d, expecting len 2" % len(constraints)
	    return 1, msg
	if ( (constraints[0].constraint_expression.constraint_expr != "$i == 1") or
	     (constraints[1].constraint_expression.constraint_expr != "$j == 1") ):
	    return 1, "GetConstraintsCheck: retreived constraints not the same as added constraints"
	if (self.__verbose > 0):
	    print "GetConstraintsCheck: calling remove_all_constraints"
	try:
	    filter.remove_all_constraints()
	except:
	    return 1, "GetConstraintsCheck: remove_all_constraints failure"
	if (self.__verbose > 0):
	    print "GetConstraintsCheck: calling get_all_constraints"
	constraints = None
	try:
	    constraints = filter.get_all_constraints()
	except:
	    return 1, "GetConstraintsCheck: get_all_constraints failure"
	if (len(constraints) != 0):
	    msg = "GetConstraintsCheck: get_all_constraints returned seq of len %d, expecting len 0" % len(constraints)
	    return 1, msg
	if (filter != None):
	    filter.destroy()
	msg = "GetConstraintsCheck: added 2 constraints, retrieved same constraints, removed all constraints, as expected"
	return 0, msg # OK

def Create(chan, verbose, args):
    "stand-alone constructor"
    test = C(chan, verbose, args)
    return test
