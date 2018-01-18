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

# module InterFilterTest: provides class InterFilterTest.C

# STEP 1: consumer and supplier are created using:
#   conCreate     : consumer constructor
#   conHelper     : consumer helper (NB filter spec in helper, if any, is ignored)
#                   (con_evfn, con_chfn, con_msec)
#   supCreate : supplier constructor (NB filter spec in helper, if any, is ignored)
#   SupHelper     : supplier helper module 
#                   (sup_evfn, sup_chfn, sup_msec)
#
# STEP 2: Run each phase p, as follows
#   2a: if p.remove_filters is true, remove all filters
#   2b: reset the supplier and consumer using
#         len(p.cexpected), p.stotal
#   2c: install specified filter /constraints (if any) based on p.constraints, p.add_constraints
#         in specified location p.loc
#   2d: connect supplier and consumer, wait done
#   2e: compare collected consumer result to p.cexpect
#         (both are sequences of numbers)
#
# STEP 3: cleanup: disconnect clients, destroy admins and filters
#         report/return final result 

from Config import TopConf

def mknm(s, tnum, ctr):
    return "%s%d:%d" % (s, tnum, ctr)

class C:
    "InterFilterTest.C: parameterized supplier-consumer test with multiple phases"

    def __init__(self, tcase):
	self.__testCase = tcase

    def construct_clients(self, tnum, tname, tc, channel, verbose):
	"returns (cons, sups) -- or (None, None) on error"
	# construct appropriate supplier and consumer
	if (verbose > 0):
	    print tname, ": creating consumer(s)"
	cons = [ ]
	ctr = 1
	for i in range(tc.numCon):
	    cnm = mknm("con", tnum, ctr)
	    ctr += 1
	    cIdx = tc.Tname + cnm
	    con = tc.conCreate(chan   = channel,
			       maxevs = 0, # to be set later
			       bsize  = tc.conBSize,
			       objnm  = cnm,
			       idx    = cIdx,
			       efn    = tc.conHelper.con_evfn,
			       tfn    = tc.conHelper.con_chfn,
			       constraints = None, # tc.con_constraints is ignored
			       mil    = tc.conHelper.con_msec,
			       opr    = tc.c_ifopr,
			       vb     = verbose)
	    if (con == None):
		print tname, ": Error: Failed to create consumer"
		return None, None # failure
	    cons.append(con)
	if (verbose > 0):
	    print tname, ": creating supplier(s)"
	sups = [ ]
	ctr = 1
	for i in range(tc.numSup):
	    snm = mknm("sup", tnum, ctr)
	    ctr += 1
	    sIdx = tc.Tname + snm
	    sup = tc.supCreate(chan   = channel,
			       maxevs = 0, # to be set later
			       bsize  = tc.supBSize,
			       objnm  = snm,
			       idx    = sIdx,
			       efn    = tc.supHelper.sup_evfn,
			       tfn    = tc.supHelper.sup_chfn,
			       ofs    = tc.supHelper.sup_offr,
			       constraints = None, # tc.supHelper.sup_constraints is ignored
			       mil    = tc.supHelper.sup_msec,
			       opr    = tc.s_ifopr,
			       vb     = verbose)
	    if (sup == None):
		print tname, ": Error: Failed to create supplier"
		return None, None # failure
	    sups.append(sup)
	return cons, sups

    def remove_all_filters(self, pname, cons, sups, verbose):
	"remove all filters"
	for con in cons:
	    if (verbose > 0):
		print pname, "removing all filters for", con.obj_name
	    con.L_rem_all_filters()
	for sup in sups:
	    if (verbose > 0):
		print pname, "removing all filters for", sup.obj_name
	    sup.L_rem_all_filters()
	return

    def reset_clients_and_helpers(self, pname, tc, cons, sups, phase, verbose):
	"""reset cons and sups, including clearing stats
	returns 0 if OK, 1 on error
	"""
	# reset the consumers
	err = 0
	smaxevs = phase.stotal
	cmaxevs = phase.stotal # give chance to consume more than the expected events
	for con in cons:
	    if (verbose > 0):
		print pname, "resetting maxevs for", con.obj_name, "to", cmaxevs
	    b = con.L_reset(cmaxevs)
	    if (b == 1):
		err = 1
	    tc.conHelper.con_total[con.test_idx] = 0
	    tc.conHelper.con_valid[con.test_idx] = 0
	    tc.conHelper.consumed[con.test_idx] = [ ]
	for sup in sups:
	    if (verbose > 0):
		print pname, "resetting maxevs for", sup.obj_name, "to", smaxevs
	    b = sup.L_reset(smaxevs)
	    tc.supHelper.sup_total[sup.test_idx] = 0
	    if (b == 1):
		err = 1
	return err

    def add_filter_or_constraints(self, pname, cons, sups, phase, verbose):
	"returns 0 if OK, 1 on error"
	# add filter(s)/constraints
	if ((phase.loc == None) or (phase.constraints == None) or (len(phase.constraints) == 0)):
	    if (verbose > 0):
		print pname, "no filter/constraints to add for this phase"
	    return 0 # OK
	err = 0
	if (phase.loc == "cproxy"):
	    targs = cons
	    targloc = "proxy"
	elif (phase.loc == "cadmin"):
	    targs = cons
	    targloc = "admin"
	elif (phase.loc == "sproxy"):
	    targs = sups
	    targloc = "proxy"
	elif (phase.loc == "sadmin"):
	    targs = sups
	    targloc = "admin"
	else:
	    print pname, "config error, invalid location", phase.loc
	    err = 1
	if (err == 0):
	    for targ in targs:
		b = 0
		if (phase.add_constraints == 1):
		    if (verbose > 0):
			print pname, "adding constraints to existing filter at proxy of", targ.obj_name
		    b = targ.L_add_constraints(targloc, phase.constraints)
		else:
		    if (verbose > 0):
			print pname, "adding new filter to", targloc, "of", targ.obj_name
		    b = targ.L_add_filter(targloc, phase.constraints)
		    if (verbose > 0):
			print pname, "result of L_add_filter =", b
		if (b == 1):
		    err = 1
		    break
	return err

    def run_phase(self, pname, tc, cons, sups, phase, verbose):
	"run one phase (connect and wait done), returns 1 on any_error, 0 for OK"
	# connect the consumer(s)
	any_err = 0
	connected_cons = [ ]
	connected_sups = [ ]
	for con in cons:
	    if (verbose > 0):
		print pname, "connecting consumer", con.obj_name
	    if (con.L_connect() == 1):
		print pname, "Error: Failed to connect consumer"
		any_err = 1
		break
	    connected_cons.append(con)
	# connect the supplier(s)
	if (any_err == 0):
	    for sup in sups:
		if (verbose > 0):
		    print pname, "connecting supplier", sup.obj_name
		if (sup.L_connect() == 1):
		    print pname, "Error: Failed to connect supplier"
		    any_err = 1
		    break
		connected_sups.append(sup)
	# wait for each -- IFILT_MAX_WAIT second wait
	max_wait = TopConf.IFILT_MAX_WAIT
	if (verbose > 0):
	    print pname, "waiting for connected consumer(s)"
	for con in connected_cons:
	    if (any_err == 1):
		max_wait == 0.1 # force timeouts to occur
	    status = con.L_wait_done(max_wait)
	    if (status == 1):
		print pname, "possible failure: consumer wait_done resulted in com error status"
		any_err = 1
	    if (status == 2):
		print pname, "possible failure: consumer wait_done resulted in TIMEOUT (", max_wait, "secs passed)"
	if (verbose > 0):
	    print pname, "waiting for connected supplier(s)"
	for sup in connected_sups:
	    if (any_err == 1):
		max_wait == 0.1 # force timeouts to occur
	    status = sup.L_wait_done(max_wait)
	    if (status == 1):
		print pname, "possible failure: supplier wait done resulted in com error status"
		any_err = 1
	    if (status == 2):
		print pname, "possible failure: consumer wait_done resulted in TIMEOUT (", max_wait, "secs passed)"
	# sum up the stats
	res = 0
	s_total = 0
	for sup in sups:
	    if (tc.supHelper.sup_total.has_key(sup.test_idx)):
		s_total += tc.supHelper.sup_total[sup.test_idx]
	consumed = [ ]
	for con in cons:
	    consumed = [ ]
	    if (tc.conHelper.consumed.has_key(con.test_idx)):
		consumed = tc.conHelper.consumed[con.test_idx]
	    if (consumed != phase.cexpect):
		print pname, "consumer", con.obj_name, ": consumed != expected :", consumed, "!=", phase.cexpect
		res = 1
	# return any_err and the stats
	s_total_expected = tc.numSup * phase.stotal
	if (s_total != s_total_expected):
	    print pname, "FAILURE: sup_total = ", s_total, ", expecting", s_total_expected
	    res = 1 # failure
	if (res == 0): 
	    print pname, "SUCCESS:", s_total, "supplied, consumed == expected: ", consumed, "==", phase.cexpect
	else:
	    any_err =1
	return any_err

    def cleanup_all(self, tname, cons, sups, verbose):
	"no return value"
	# cleanup
	for con in cons:
	    if (verbose > 0):
		print tname, "cleanup for consumer", con.obj_name
	    con.L_cleanup()
	for sup in sups:
	    if (verbose > 0):
		print tname, "cleanup for supplier", sup.obj_name
	    sup.L_cleanup()

    def run(self, tnum, channel, verbose):
	"Run test case self.__testCase.  Return 1 on error, 0 on SUCCESS"
	tc = self.__testCase
	tname = "InterFilterTest"
	if (verbose > 0):
	    print tname, ": run called"
	# construct appropriate supplier and consumer
	cons, sups = self.construct_clients(tnum, tname, tc, channel, verbose)
	if ((cons == None) or (sups == None)):
	    return 1 # error
	# process each phase
	phase_index = 0
	any_err = 0
	for phase in tc.phases:
	    phase_index += 1
	    pname = "%s PHASE %d :" % (tname, phase_index)
	    if (phase.remove_filters == 1):
		self.remove_all_filters(pname, cons, sups, verbose)
	    if (any_err == 0):
		# print "XXX", pname, ": calling add_filter_or_constraints"
		any_err = self.add_filter_or_constraints(pname, cons, sups, phase, verbose)
	    if (any_err == 0):
		any_err = self.reset_clients_and_helpers(pname, tc, cons, sups, phase, verbose)
	    if (any_err == 0):
		any_err = self.run_phase(pname, tc, cons, sups, phase, verbose)
	    if (any_err == 1):
		print pname, "had error, so skipping other phases"
		break
	# final cleanup
	self.cleanup_all(tname, cons, sups, verbose)
	if (any_err == 0):
	    print tname, "TOTAL SUCCESS: all phases passed"
	    return 0 # SUCCESS
	return 1 # FAILURE

