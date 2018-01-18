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

# module RunTests defines RunTests.run()

import sys
import omniORB
import CosNotification,CosNotifyChannelAdmin,TimeBase
from omniORB import CORBA
from Utils  import CliUtils,ChanUtils,PropUtils

verbose = 1
tracelevel = 0

# --------------------------------------------------------------------------------
# helper routine

def usage():
    print "\nFooTest.py [-c <name>]"
    print "    -c <name>   : specify the name to use to lookup the ChannelFactory"

def print_chan_properties(chan):
    # call validate_qos with empty argument
    proprangeseq = chan.validate_qos( [] )
    print " "
    print "validate_qos([]) returned proprangeseq of length", len(proprangeseq)
    print " "
    PropUtils.print_propseq(proprangeseq)

    # call get_qos
    propseq = chan.get_qos( )
    print " "
    print "get_qos() returned propseq of length", len(propseq)
    print " "
    PropUtils.print_propseq(propseq)

    # call get_admin
    adpropseq = chan.get_admin( )
    print " "
    print "get_admin() returned adpropseq of length", len(adpropseq)
    print " "
    PropUtils.print_propseq(adpropseq)
    print " "
    return

# --------------------------------------------------------------------------------
# startup code

if (verbose > 0):
    print "FooTest: starting"

cfactory_name = "ChannelFactory"
i = 1
while (i < len(sys.argv)):
    if (sys.argv[i] == "-c"):
        i += 1
        if (i < len(sys.argv)):
            cfactory_name = sys.argv[i]
            i += 1
            continue
        print "-c must be followed by a channel factory name"
        usage()
        sys.exit(-1)
    print "invalid argument:", sys.argv[i]
    usage()
    sys.exit(-1)

if (tracelevel > 0):
    omniORB.traceLevel(tracelevel)

orb, poa = ChanUtils.setup_orb_and_poa(verbose)
if (orb == None):
    print "FooTests: exiting"
    sys.exit(1)

fact = ChanUtils.get_factory(orb, verbose, cfactory_name)
if (fact == None):
    print "FooTests exiting"
    sys.exit(1)

# --------------------------------------------------------------------------------
# stats

num_tests = 0
num_failed = 0

# --------------------------------------------------------------------------------
# test valid channel creation
if (verbose > 0):
    print("\nTest valid channel creation")
num_tests += 1

chan = None
aqos = [ ]
nqos = [ ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel

if (chan != None):
    if (verbose > 0):
	print "FooTests: get_channel_qos OK"
	print_chan_properties(chan)
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: chan not created -- unexpected!"
    num_failed += 1


# --------------------------------------------------------------------------------
# test valid channel creation with the 3 valid combos of MaxQueueLength and RejectNewEvents
if (verbose > 0):
    print("\nTest valid channel creation with MaxQueueLength 10000, RejectNewEvents true (should work)")
num_tests += 1


chan = None
a1 = CORBA.Any(CORBA._tc_long, 10000)
prop1 = CosNotification.Property("MaxQueueLength", a1)
a2 = CORBA.Any(CORBA._tc_boolean, 1)
prop2 = CosNotification.Property("RejectNewEvents", a2)
aqos = [ prop1, prop2 ]
nqos = [ ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel

if (chan != None):
    if (verbose > 0):
	print "FooTests: get_channel_qos OK"
	print_chan_properties(chan)
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: chan not created -- unexpected!"
    num_failed += 1

#----------------
if (verbose > 0):
    print("\nTest valid channel creation with MaxQueueLength 0, RejectNewEvents false (should work)")
num_tests += 1


chan = None
a1 = CORBA.Any(CORBA._tc_long, 0)
prop1 = CosNotification.Property("MaxQueueLength", a1)
a2 = CORBA.Any(CORBA._tc_boolean, 0)
prop2 = CosNotification.Property("RejectNewEvents", a2)
aqos = [ prop1, prop2 ]
nqos = [ ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel

if (chan != None):
    if (verbose > 0):
	print "FooTests: get_channel_qos OK"
	print_chan_properties(chan)
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: chan not created -- unexpected!"
    num_failed += 1

#----------------
if (verbose > 0):
    print("\nTest valid channel creation with MaxQueueLength 0, RejectNewEvents true (should work)\nThe true setting is meaningless but not invalid (?)")
num_tests += 1


chan = None
a1 = CORBA.Any(CORBA._tc_long, 0)
prop1 = CosNotification.Property("MaxQueueLength", a1)
a2 = CORBA.Any(CORBA._tc_boolean, 1)
prop2 = CosNotification.Property("RejectNewEvents", a2)
aqos = [ prop1, prop2 ]
nqos = [ ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel

if (chan != None):
    if (verbose > 0):
	print "FooTests: get_channel_qos OK"
	print_chan_properties(chan)
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: chan not created -- unexpected!"
    num_failed += 1

# --------------------------------------------------------------------------------
# test invalid channel creation with the combos of MaxQueueLength and RejectNewEvents that fails
if (verbose > 0):
    print("\nTest channel creation with MaxQueueLength 10000, RejectNewEvents false (should fail)")

num_tests += 1

chan = None
a1 = CORBA.Any(CORBA._tc_long, 10000)
prop1 = CosNotification.Property("MaxQueueLength", a1)
a2 = CORBA.Any(CORBA._tc_boolean, 0)
prop2 = CosNotification.Property("RejectNewEvents", a2)
aqos = [ prop1, prop2 ]
nqos = [ ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel

if (chan != None):
    if (verbose > 0):
	print "FooTests: get_channel_qos OK -- unexpected!!"
    num_failed += 1
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: as expected, chan not created"

# --------------------------------------------------------------------------------
# test valid setting of MaximumBatchSize at different levels (chan, cadmin, cproxy)

if (verbose > 0):
    print("\nTest valid setting of MaximumBatchSize at different levels (chan, cadmin, cproxy)")

chan = None
aqos = [ ]
a = CORBA.Any(CORBA._tc_long, 17)
prop = CosNotification.Property("MaximumBatchSize", a)
nqos = [ prop ]
chan = ChanUtils.get_channel_qos(fact, nqos, [], verbose) # get new channel

if (chan != None):
    if (verbose > 0):
	print "FooTests: get_channel_qos OK"
	print_chan_properties(chan)
else:
    print "FooTests: chan not created -- unexpected!!"
    num_failed += 1

if (chan != None):
    if (verbose > 0):
	print "FooTests: chan was created -- getting admin, proxy"
    admin, proxy = CliUtils.get_proxy_for_con(CosNotifyChannelAdmin.STRUCTURED_EVENT, "push", "AND",
					      CosNotifyChannelAdmin.StructuredProxyPushSupplier,
					      chan, "FooTest", verbose)
    if (admin != None):
	if (verbose > 0):
	    print "Trying set_qos on admin"
	num_tests += 1
	try:
	    admin.set_qos(nqos)
	    if (verbose > 0):
		print "Setting on admin worked, as expected"
	except CosNotification.UnsupportedQoS, eseqlist:
	    if (verbose > 0):
		print "UnsupportedQoS exception -- unexpected!!"
		num_failed += 1
		for eseq in eseqlist:
		    PropUtils.print_propseq(eseq)
	except:
	    pass

    if (proxy != None):
	if (verbose > 0):
	    print "Trying set_qos on proxy"
	num_tests += 1
	try:
	    proxy.set_qos(nqos)
	    if (verbose > 0):
		print "Setting on proxy worked, as expected"
	except CosNotification.UnsupportedQoS, eseqlist:
	    num_failed += 1
	    if (verbose > 0):
		print "UnsupportedQoS exception -- unexpected!!"
		for eseq in eseqlist:
		    PropUtils.print_propseq(eseq)
	except:
	    pass
	
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)

# --------------------------------------------------------------------------------
# test UNAVAILABLE_PROPERTY by trying to set StartTime
if (verbose > 0):
    print("\nTest UNSUPPORTED by trying to set StartTime")
num_tests += 1

chan = None
aqos = [ ]
tm = TimeBase.TimeT(0,0)
minUtcT = TimeBase.UtcT(tm,0,0,0)
a = CORBA.Any(TimeBase._tc_UtcT, minUtcT)
prop = CosNotification.Property("StartTime", a)
nqos = [ prop ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel
if (verbose > 0):
    print "SHOULD SEE UNSUPPORTED ERROR ABOVE\n"

if (chan != None):
    if (verbose > 0):
	print "FooTests: chan was created -- unexpected!!"
    num_failed += 1
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: as expected, chan not created"

# --------------------------------------------------------------------------------
# test UNSUPPORTED_PROPERTY by trying to set EventReliability at the admin level
if (verbose > 0):
    print("\nTest UNSUPPORTED_PROPERTY by trying to set EventReliability at the admin level")
num_tests += 1

chan = None
aqos = [ ]
a = CORBA.Any(CORBA._tc_short, 3)
prop = CosNotification.Property("EventReliability", a)
nqos = [ prop ]
chan = ChanUtils.get_channel_qos(fact, [], [], verbose) # get new channel

if (chan != None):
    if (verbose > 0):
	print "FooTests: chan was created -- getting admin"
    admin, proxy = CliUtils.get_proxy_for_con(CosNotifyChannelAdmin.STRUCTURED_EVENT, "push", "AND",
					      CosNotifyChannelAdmin.StructuredProxyPushSupplier,
					      chan, "FooTest", verbose)
    if (admin != None):
	if (verbose > 0):
	    print "Trying set_qos on admin"
	try:
	    admin.set_qos(nqos)
	except CosNotification.UnsupportedQoS, eseqlist:
	    if (verbose > 0):
		print "UnsupportedQoS exception"
		for eseq in eseqlist:
		    PropUtils.print_propseq(eseq)
	except:
	    pass
	if (verbose > 0):
	    print "SHOULD SEE UNSUPPORTED_PROPERTY ERROR ABOVE\n"
	
    if (proxy != None):
	if (verbose > 0):
	    print "Trying set_qos on proxy"
	try:
	    proxy.set_qos(nqos)
	except CosNotification.UnsupportedQoS, eseqlist:
	    if (verbose > 0):
		print "UnsupportedQoS exception"
		for eseq in eseqlist:
		    PropUtils.print_propseq(eseq)
	except:
	    pass
	if (verbose > 0):
	    print "SHOULD SEE UNSUPPORTED_PROPERTY ERROR ABOVE\n"
	
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)

# --------------------------------------------------------------------------------
# test BAD_PROPERTY by trying to set FooProperty with a _tc_short
if (verbose > 0):
    print("\nTest BAD_PROPERTY by trying to set FooProperty with a _tc_short")
num_tests += 1

chan = None
aqos = [ ]
a = CORBA.Any(CORBA._tc_ushort, 100)
prop = CosNotification.Property("FooProperty", a)
nqos = [ prop ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel
if (verbose > 0):
    print "SHOULD SEE BAD_PROPERTY ERROR ABOVE\n"

if (chan != None):
    if (verbose > 0):
	print "FooTests: chan was created -- unexpected!!"
    num_failed += 1
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: as expected, chan not created"

# --------------------------------------------------------------------------------
# test BAD_TYPE by trying to set MaximumBatchSize with a _tc_short
if (verbose > 0):
    print("\nTest BAD_TYPE by trying to set MaximumBatchSize with a _tc_short")
num_tests += 1

chan = None
aqos = [ ]
a = CORBA.Any(CORBA._tc_ushort, 100)
prop = CosNotification.Property("MaximumBatchSize", a)
nqos = [ prop ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel
if (verbose > 0):
    print "SHOULD SEE BAD_TYPE ERROR ABOVE\n"

if (chan != None):
    if (verbose > 0):
	print "FooTests: chan was created -- unexpected!!"
    num_failed += 1
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: as expected, chan not created"

# --------------------------------------------------------------------------------
# test BAD_VALUE by trying to set MaximumBatchSize to -2
if (verbose > 0):
    print("\nTest BAD_VALUE by trying to set MaximumBatchSize to -2")
num_tests += 1

chan = None
aqos = [ ]
a = CORBA.Any(CORBA._tc_long, -2)
prop = CosNotification.Property("MaximumBatchSize", a)
nqos = [ prop ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel
if (verbose > 0):
    print "SHOULD SEE BAD_VALUE ERROR ABOVE, INCLUDING VALID RANGE\n"

if (chan != None):
    if (verbose > 0):
	print "FooTests: chan was created -- unexpected!!"
    num_failed += 1
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: as expected, chan not created"

# --------------------------------------------------------------------------------
# test UNSUPPORTED_VALUE by trying to set EventReliability to Persistent
if (verbose > 0):
    print("\nTest UNSUPPORTED_VALUE by trying to set EventReliability to Persistent")
num_tests += 1

chan = None
aqos = [ ]
a = CORBA.Any(CORBA._tc_short, CosNotification.Persistent)
prop = CosNotification.Property("EventReliability", a)
nqos = [ prop ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel
if (verbose > 0):
    print "SHOULD SEE UNSUPPORTED_VALUE ERROR ABOVE, INCLUDING VALID RANGE\n"

if (chan != None):
    if (verbose > 0):
	print "FooTests: chan was created -- unexpected!!"
    num_failed += 1
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: as expected, chan not created"

# --------------------------------------------------------------------------------
# test BAD_TYPE by trying to set RejectNewEvents with a _tc_long
if (verbose > 0):
    print("\nTest BAD_TYPE by trying to set RejectNewEvents with a _tc_long")
num_tests += 1

chan = None
a = CORBA.Any(CORBA._tc_long, 100)
prop = CosNotification.Property("RejectNewEvents", a)
aqos = [ prop ]
nqos = [ ]
chan = ChanUtils.get_channel_qos(fact, nqos, aqos, verbose) # get new channel
if (verbose > 0):
    print "SHOULD SEE BAD_TYPE ERROR ABOVE\n"

if (chan != None):
    if (verbose > 0):
	print "FooTests: chan was created -- unexpected!!"
    num_failed += 1
    # cleanup
    ChanUtils.destroy_channel(chan, verbose)
else:
    print "FooTests: as expected, chan not created"

# --------------------------------------------------------------------------------
# ALL DONE
if (verbose > 0):
    print "\nFooTests: REPORT: Total Tests:", num_tests, "  Failed Tests:", num_failed
