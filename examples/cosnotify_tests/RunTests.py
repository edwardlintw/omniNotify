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

# module RunTests defines RunTests.run()

import sys
import omniORB
from Utils  import ChanUtils
from Utils  import SupConTest, StandAloneTest, InterFilterTest
from Config import TopConf, SupConCases, StandAloneCases, InterFilterCases

def run_range(run_region, L, I):
    start = 0
    end = len(L)
    if (run_region > 0):
	k = int( len(L)/10 )
	end = k * run_region
	start = end - k
	if (run_region == 10):
	    end = len(L)
    r = range(start, end)
    if ((TopConf.MAX_TESTS > 0) and (TopConf.MAX_TESTS < len(r))):
	r = r[:TopConf.MAX_TESTS]
    if (len(I) > 0):
	r2 = []
	for i in r:
	    if (I.count(i) > 0):
		r2.append(i)
	r = r2
    return r

def run_standalone_tests(run_region, fact, chan, norun):
    """run stand-alone test cases
    """
    verbose = StandAloneCases.VERBOSE
    reuse_chan = StandAloneCases.REUSE_CHANNEL
    tot_tests = 0
    bad_tests = 0
    r = run_range(run_region, StandAloneCases.L, StandAloneCases.TEST_INDICES)
    print "\n------------------------------------------------------------------------------"
    print "run_standalone_tests: START running", len(r), "stand-alone test cases:"
    print "------------------------------------------------------------------------------"
    for i in r:
	print "Test %3d -- %s" % (i+1, StandAloneCases.L[i].Tname)
    print " "
    if ((norun == 1) and (verbose == 0)):
	return
    count = 0
    first = -1
    last = -1
    for i in r:
	if (first == -1):
	    first = i
	last = i
	count += 1
	testCase = StandAloneCases.L[i]
	test = StandAloneTest.C(testCase)
	print "------------------------------------------------------------------------------"
	print "Test", count, " Index", i, "--", testCase.Tname + ':'
	print testCase.Tdoc
	if (norun == 1):
	    continue
	failure = test.run(count, chan, verbose)
	tot_tests += 1
	if (failure == 1):
	    bad_tests += 1
	if (verbose > 0):
	    print "run_standalone_tests: finished test case"
	if (reuse_chan == 0):
	    ChanUtils.destroy_channel(chan, verbose)
	    chan = ChanUtils.get_channel(fact, verbose)
	    if (chan == None):
		print "run_standalone_tests: exiting"
		sys.exit(1)
    print "------------------------------------------------------------------------------"
    print "run_standalone_tests: DONE running", count, "tests (first idx", first, ", last idx", last, ")"
    print "SUMMARY [stand-alone]   Total Tests:", tot_tests, "   Failed Tests:", bad_tests 
    print "------------------------------------------------------------------------------"
    return chan

def run_supcon_tests(run_region, fact, chan, norun):
    """run supplier-consumer test cases
    """
    verbose = SupConCases.VERBOSE
    reuse_chan = SupConCases.REUSE_CHANNEL
    tot_tests = 0
    bad_tests = 0
    r = run_range(run_region, SupConCases.L, SupConCases.TEST_INDICES)
    print "\n------------------------------------------------------------------------------"
    print "run_supcon_tests: START running", len(r), "supplier-consumer test cases:"
    print "------------------------------------------------------------------------------"
    for i in r:
	print "Test %3d -- %s" % (i+1, SupConCases.L[i].Tname)
    print " "
    if ((norun == 1) and (verbose == 0)):
	return
    count = 0
    first = -1
    last = -1
    for i in r:
	if (first == -1):
	    first = i
	last = i
	count += 1
	testCase = SupConCases.L[i]
	test = SupConTest.C(testCase)
	print "------------------------------------------------------------------------------"
	print "Test", count, " Index", i, "--", testCase.Tname + ':'
	print testCase.Tdoc
	if (norun == 1):
	    continue
	failure = test.run(count, chan, verbose)
	tot_tests += 1
	if (failure == 1):
	    bad_tests += 1
	if (verbose > 0):
	    print "run_supcon_tests: finished test case"
	if (reuse_chan == 0):
	    ChanUtils.destroy_channel(chan, verbose)
	    chan = ChanUtils.get_channel(fact, verbose)
	    if (chan == None):
		print "run_supcon_tests: exiting"
		sys.exit(1)
    print "------------------------------------------------------------------------------"
    print "run_supcon_tests: DONE running", count, "tests (first idx", first, ", last idx", last, ")"
    print "SUMMARY [supplier-consumer]   Total Tests:", tot_tests, "   Failed Tests:", bad_tests 
    print "------------------------------------------------------------------------------"
    return chan

def run_ifilt_tests(run_region, fact, chan, norun):
    """run interfilter test cases
    """
    verbose = InterFilterCases.VERBOSE
    reuse_chan = InterFilterCases.REUSE_CHANNEL
    tot_tests = 0
    bad_tests = 0
    r = run_range(run_region, InterFilterCases.L, InterFilterCases.TEST_INDICES)
    print "\n------------------------------------------------------------------------------"
    print "run_ifilt_tests: START running", len(r), "inter-filter test cases:"
    print "------------------------------------------------------------------------------"
    for i in r:
	print "Test %3d -- %s" % (i+1, InterFilterCases.L[i].Tname)
    print " "
    if ((norun == 1) and (verbose == 0)):
	return
    count = 0
    first = -1
    last = -1
    for i in r:
	if (first == -1):
	    first = i
	last = i
	count += 1
	testCase = InterFilterCases.L[i]
	test = InterFilterTest.C(testCase)
	print "------------------------------------------------------------------------------"
	print "Test", count, " Index", i, "--", testCase.Tname + ':'
	print testCase.Tdoc
	if (norun == 1):
	    continue
	failure = test.run(count, chan, verbose)
	tot_tests += 1
	if (failure == 1):
	    bad_tests += 1
	if (verbose > 0):
	    print "run_ifilt_tests: finished test case"
	if (reuse_chan == 0):
	    ChanUtils.destroy_channel(chan, verbose)
	    chan = ChanUtils.get_channel(fact, verbose)
	    if (chan == None):
		print "run_ifilt_tests: exiting"
		sys.exit(1)
    print "------------------------------------------------------------------------------"
    print "run_ifilt_tests: DONE running", count, "tests (first idx", first, ", last idx", last, ")"
    print "SUMMARY [inter-filter]   Total Tests:", tot_tests, "   Failed Tests:", bad_tests 
    print "------------------------------------------------------------------------------"
    return chan

def run(run_region, norun, cfactory_name):
    """run all test cases
    """
    verbose = TopConf.VERBOSE
    if (verbose > 0):
	print "RunTests.run: starting"

    fact = chan = None
    if (norun == 0):
	if (TopConf.TRACE_LEVEL > 0):
	    omniORB.traceLevel(TopConf.TRACE_LEVEL)

	orb, poa = ChanUtils.setup_orb_and_poa(verbose)
	if (orb == None):
	    print "RunTests.run: exiting"
	    sys.exit(1)

	fact = ChanUtils.get_factory(orb, verbose, cfactory_name)
	if (fact == None):
	    print "RunTests.run: exiting"
	    sys.exit(1)

	chan = ChanUtils.get_channel(fact, verbose)
	if (chan == None):
	    print "RunTests.run: exiting"
	    sys.exit(1)

    if (TopConf.RUN_SUPCON == 1):
	chan = run_supcon_tests(run_region, fact, chan, norun)

    if (TopConf.RUN_IFILT == 1):
	chan = run_ifilt_tests(run_region, fact, chan, norun)

    if (TopConf.RUN_STANDALONE == 1):
	chan = run_standalone_tests(run_region, fact, chan, norun)

    if (chan != None):
	ChanUtils.destroy_channel(chan, verbose)

    if (verbose > 0):
	print "RunTests.run: done"
