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

# Module ConstraintCheck defines ConstraintCheck.C, ConstraintCheck.Create

from omniORB import CORBA
import CosNotifyFilter # import stubs

class C:
    """Test different constraint strings used with filter
    """
    def __init__(self, chan, verbose, args):
	"""
	args is (cexpr, expected) expected indicating expected result
	(0 = OK, 1 = InvalidConstraint, 2 = another exception)
	"""
	self.__chan    = chan
	self.__verbose = verbose
	self.__cexpr, self.__expected = args
	self.__ff = None
	try:
	    self.__ff = chan._get_default_filter_factory()
	except (CORBA.COMM_FAILURE):
	    print "ConstraintCheck: Caught COMM_FAILURE obtaining filter factory"

    def run(self):
	"attempt to create filter and add cosntraint"
	filter = None
	if (self.__ff == None):
	    return 1, "ERROR no filter factory"  # error
	if (self.__verbose > 0):
	    print "ConstraintCheck: testing this constraint:"
	    print "  " + self.__cexpr
	try:
	    filter = self.__ff.create_filter("EXTENDED_TCL")
	except CosNotifyFilter.InvalidGrammar:
	    return 1, "ERROR filter construction failed" # error
	except: # some other exception (unexpected)
	    return 1, "ERROR unexpected exception constructing filter" # error
	exp1 = CosNotifyFilter.ConstraintExp([ ], self.__cexpr)
	exp = [exp1]
	status = 0
	try:
	    filter.add_constraints(exp)
	except CosNotifyFilter.InvalidConstraint:
	    status = 1
	except:
	    status = 2
	if (filter != None):
	    filter.destroy()
	msg = "ConstraintCheck: status = %d, expecting %d" % (status, self.__expected)
	if (status == self.__expected):
	    return 0, msg # OK
	return 1, msg # error

def Create(chan, verbose, args):
    "stand-alone constructor"
    test = C(chan, verbose, args)
    return test
