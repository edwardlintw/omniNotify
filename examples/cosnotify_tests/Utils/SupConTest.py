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

# module SupConTest: provides class SupConTest.C

# for run method, test is object containing:
#   Tdoc      :  test doc string
#   Tname     : test name
#   conCreate : consumer constructor
#   conHelper : consumer helper module
#               (con_evfn, con_chfn, con_constraints, con_msec)
#   supCreate : supplier constructor
#   SupHelper : supplier helper module
#                (sup_evfn, sup_chfn, sup_constraints, sup_msec)
#   supBSize  : supply batch size
#   supTotal  : (expected) # events supplied
#   conBSize  : consume batch size
#   conTotal  : (expected) total # events consumed
#   conValid  : expected valid # events consumed

from Config import TopConf

def mknm(s, tnum, ctr):
    return "%s%d:%d" % (s, tnum, ctr)

class C:
    "SupConTest.C: parameterized supplier-consumer test"

    def __init__(self, tcase):
	self.__testCase = tcase

    def run(self, tnum, channel, verbose):
	"Run test case self.__testCase"
	lverbose = verbose
	tc = self.__testCase
	tname = "SupConTest"
	if (lverbose > 0):
	    print tname, ": run called"
	# construct appropriate supplier and consumer
	conMaxEv = tc.conTotal
	if (tc.conTotal == 0):
	    conMaxEv = 1 # must attempt to consume at least 1 event
	if (lverbose > 0):
	    print tname, ": creating consumer(s)"
	cons = [ ]
	conIdx = [ ]
	ctr = 1
	for i in range(tc.numCon):
	    cnm = mknm("con", tnum, ctr)
	    ctr += 1
	    cIdx = tc.Tname + cnm
	    conIdx.append(cIdx)
	    con = tc.conCreate(chan   = channel,
			       maxevs = conMaxEv,
			       bsize  = tc.conBSize,
			       objnm  = cnm,
			       idx    = cIdx,
			       efn    = tc.conHelper.con_evfn,
			       tfn    = tc.conHelper.con_chfn,
			       constraints = tc.conHelper.con_constraints,
			       mil    = tc.conHelper.con_msec,
			       opr    = "AND",
			       vb     = verbose)
	    if (con == None):
		print tname, ": Error: Failed to create consumer"
		return 1 # failure
	    cons.append(con)
	# connect the consumer(s)
	if (lverbose > 0):
	    print tname, ": connecting consumer(s)"
	for con in cons:
	    if (con.L_connect() == 1):
		print tname, ": Error: Failed to connect consumer"
		return 1 # failure
	if (lverbose > 0):
	    print tname, ": creating supplier(s)"
	sups = [ ]
	supIdx = [ ]
	ctr = 1
	for i in range(tc.numSup):
	    snm = mknm("sup", tnum, ctr)
	    ctr += 1
	    sIdx = tc.Tname + snm
	    supIdx.append(sIdx)
	    sup = tc.supCreate(chan   = channel,
			       maxevs = tc.supTotal,
			       bsize  = tc.supBSize,
			       objnm  = snm,
			       idx    = sIdx,
			       efn    = tc.supHelper.sup_evfn,
			       tfn    = tc.supHelper.sup_chfn,
			       ofs    = tc.supHelper.sup_offr,
			       constraints = tc.supHelper.sup_constraints,
			       mil    = tc.supHelper.sup_msec,
			       opr    = "AND",
			       vb     = verbose)
	    if (sup == None):
		print tname, ": Error: Failed to create supplier"
		return 1 # failure
	    sups.append(sup)
	# connect the supplier(s)
	if (lverbose > 0):
	    print tname, ": connecting supplier(s)"
	for sup in sups:
	    if (sup.L_connect() == 1):
		print tname, ": Error: Failed to connect supplier"
		return 1 # failure
	# wait for each -- max 15 second wait
	if (lverbose > 0):
	    print tname, ": waiting for consumer(s)"
	max_wait = TopConf.SUPCON_MAX_WAIT
	c_extra = 0
	for con in cons:
	    status = con.L_wait_done(max_wait)
	    if (status == 1):
		print tname, ": possible failure: consumer wait_done resulted in com error status"
	    if (status == 2):
		print tname, ": possible failure: consumer wait_done resulted in TIMEOUT (", max_wait, "secs passed)"
		max_wait = 0.1 # already waited the necessary 15 seconds
	    c_extra += con.num_extra
	    if (lverbose > 0):
		print tname, ": cleanup for consumer"
	    con.L_cleanup()
	if (lverbose > 0):
	    print tname, ": waiting for supplier(s)"
	for sup in sups:
	    status = sup.L_wait_done(max_wait)
	    if (status == 1):
		print tname, ": possible failure: supplier wait done resulted in com error status"
	    if (status == 2):
		print tname, ": possible failure: consumer wait_done resulted in TIMEOUT (", max_wait, "secs passed)"
		max_wait = 0.1 # already waited the necessary 15 seconds
	    if (lverbose > 0):
		print tname, ": cleanup for supplier"
	    sup.L_cleanup()
	res = 0 # success
	s_total = 0
	c_total = 0
	c_valid = 0
	for i in range(len(supIdx)):
	    if (tc.supHelper.sup_total.has_key(supIdx[i])):
		s_total += tc.supHelper.sup_total[supIdx[i]]
	for i in range(len(conIdx)):
	    if (tc.conHelper.con_total.has_key(conIdx[i])):
		c_total += tc.conHelper.con_total[conIdx[i]]
	    if (tc.conHelper.con_valid.has_key(conIdx[i])):
		c_valid += tc.conHelper.con_valid[conIdx[i]]
	s_total_expected = tc.numSup * tc.supTotal
	c_total_expected = tc.numCon * tc.conTotal
	c_valid_expected = tc.numCon * tc.conValid
	if (s_total != s_total_expected):
	    print tname, ": FAILURE: sup_total = ", s_total, ", expecting", s_total_expected
	    res = 1 # failure
	if (c_total != c_total_expected):
	    print tname, ": FAILURE: con_total = ", c_total, ", expecting", c_total_expected
	    res = 1 # failure
	if (c_valid != c_valid_expected):
	    print tname, ": FAILURE: con_valid = ", c_valid, ", expecting", c_valid_expected
	    res = 1 # failure
	if (res == 0): 
	    print tname, ": SUCCESS:", s_total, "supplied,", c_total, "consumed,", c_valid, "valid, as expected"
	if (c_extra != 0):
	    print tname, ": =oops=> FAILURE: con_extra = ", c_extra, ", expecting 0"
	    res = 1 # failure
	return res
