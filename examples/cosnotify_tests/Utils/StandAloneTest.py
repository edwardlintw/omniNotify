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

# module StandAloneTest: provides class StandAloneTest.C

# for run method, test is object containing:
#   Tdoc   : test doc string
#   Tname  : test name
#   create : test constructor
#   args   : extra args to create call -- create(channel, verbose, args)

class C:
    "StandAloneTest.C: parameterized stand-alone test"

    def __init__(self, tcase):
	self.__testCase = tcase

    def run(self, tnum, channel, verbose):
	"Run test case self.__testCase"
	lverbose = verbose
	tc = self.__testCase
	tname = "StandAloneTest"
	if (lverbose > 0):
	    print tname, ": run called"
	c = tc.create(channel, verbose, tc.args)
	if (c == None):
	    if (lverbose > 0):
		print tname, ": test case create call failed"
		return 1 # error
	res, msg = c.run()
	if (res == 0):
	    print tname, ": SUCCESS:", msg
	else:
	    print tname, ": FAILURE:", msg
	return res
