#!/usr/bin/env python

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

import sys
from Utils import ChanUtils
from TestCases.FilterTest1 import FilterTest1
from TestCases.FilterTest1b import FilterTest1b

# testSet = [FilterTest1b, FilterTest1]
testSet = [FilterTest1]

verbose = 1

if (verbose > 0):
    print "sanity_check: starting"

# Sanity_check a set of tests

for testCase in testSet:
    test = testCase()
    if (verbose > 0):
	print "sanity_check: next test case:"
	print testCase.Tdoc
    test.sanity_check(verbose)
    if (verbose > 0):
	print "sanity_check: finished test case"

# Done for now
if (verbose > 0):
    print "sanity_check: done"

