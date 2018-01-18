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

# Module GrammarCheck defines GrammarCheck.C, GrammarCheck.Create

from omniORB import CORBA
import CosNotifyFilter # import stubs

class C:
    """Test different grammar strings used in filter construction
    """
    def __init__(self, chan, verbose, args):
	"""
	args is (grammar, expected) expected indicating expected result
	(0 = OK, 1 = InvalidGrammar, 2 = another exception)
	"""
	self.__chan    = chan
	self.__verbose = verbose
	self.__gram, self.__expected = args
	self.__ff = None
	try:
	    self.__ff = chan._get_default_filter_factory()
	except (CORBA.COMM_FAILURE):
	    print "GrammarCheck: Caught COMM_FAILURE obtaining filter factory"

    def run(self):
	"attempt to create filter"
	status = 0
	filter = None
	if (self.__ff == None):
	    return 1, "ERROR no filter factory"  # error
	if (self.__verbose > 0):
	    print "GrammarCheck: testing grammar '" + self.__gram + "'"
	try:
	    filter = self.__ff.create_filter(self.__gram)
	except CosNotifyFilter.InvalidGrammar:
	    status = 1
	except: # some other exception (unexpected)
	    status = 2
	if (filter != None):
	    filter.destroy()
	msg = "GrammarCheck: status = %d, expecting %d" % (status, self.__expected)
	if (status == self.__expected):
	    return 0, msg # OK
	return 1, msg # error

def Create(chan, verbose, args):
    "stand-alone constructor"
    test = C(chan, verbose, args)
    return test
