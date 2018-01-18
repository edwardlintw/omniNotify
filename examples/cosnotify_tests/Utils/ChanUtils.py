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

# XXX setup should probably not be responsible for activating the POA

######################################################################
# Module ChanUtils:

import sys
import omniORB
from omniORB import CORBA, PortableServer
import CosNaming,CosNotification,CosNotifyChannelAdmin
from Utils  import PropUtils

##############################################################
def setup_orb_and_poa(verbose):
    # Initialise the ORB
    if (CORBA.ORB_ID == "omniORB4"):
        sys.argv += [ '-ORBmaxGIOPConnectionPerServer', '1000', '-ORBoneCallPerConnection', '1' ]
    else:
        omniORB.maxTcpConnectionPerServer(1000)
    orb = CORBA.ORB_init(sys.argv, CORBA.ORB_ID)

    # Find the root POA
    poa = orb.resolve_initial_references("RootPOA")

    # Activate the POA
    poaManager = poa._get_the_POAManager()
    poaManager.activate()

    return orb, poa

##############################################################
def get_factory(orb, verbose, cfactory_name):
    # Obtain a reference to the root naming context
    obj         = orb.resolve_initial_references("NameService")
    rootContext = obj._narrow(CosNaming.NamingContext)

    if rootContext is None:
        print "ChanUtils.get_factory: Failed to narrow the root naming context"
        return None # error

    if (verbose >= 0):
	print 'ChanUtils.get_factory: Resolve the name ', cfactory_name, '.', cfactory_name
    name = [CosNaming.NameComponent(cfactory_name, cfactory_name)]

    try:
        obj = rootContext.resolve(name)

    except CosNaming.NamingContext.NotFound, ex:
        print "ChanUtils.get_factory: Name not found:", cfactory_name, '.', cfactory_name
        return None # error

    # Narrow the object to EventChannelFactory
    fact = obj._narrow(CosNotifyChannelAdmin.EventChannelFactory)

    if (fact is None):
        print "ChanUtils.factory: Object reference is not a CosNotifyChannelAdmin.EventChannelFactory"
        return None # error

    return fact

##############################################################
def get_channel_qos(fact, nqos, aqos, verbose):
    "Get a new channel"
    chan = None
    try:
        chan, chan_id = fact.create_channel(nqos, aqos)
    except CosNotification.UnsupportedAdmin, eseqlist:
	if (verbose > 0):
	    print "get_channel_qos: UnsupportedAdmin exception"
	    for eseq in eseqlist:
		PropUtils.print_propseq(eseq)
        return None # error
    except CosNotification.UnsupportedQoS, eseqlist:
	if (verbose > 0):
	    print "get_channel_qos: UnsupportedQoS exception"
	    for eseq in eseqlist:
		PropUtils.print_propseq(eseq)
        return None # error
    except (CORBA.COMM_FAILURE):
	print "get_channel_qos: Caught COMM_FAILURE"
	return None # error
#    except:
#        print "ChanUtils.get_channel: create_channel failed"
#        return None # error

    if (chan is None):
        print "ChanUtils.get_channel: Could not obtain channel from factory"
        return None # error

    if (verbose > 0):
	print "ChanUtils.get_channel: New channel has chan_id = " , chan_id

    return chan

##############################################################
def get_channel(fact, verbose):
    "Get a new channel"
    nqos =  []
    aqos =  []
    return get_channel_qos(fact, nqos, aqos, verbose)

##############################################################
def get_channel_id(fact, id, verbose):

    # Get channel with channel ident id 
    try:
        chan = fact.get_event_channel(id)
    except (CORBA.COMM_FAILURE):
	print "get_channel_id: Caught COMM_FAILURE"
	return None # error
    except:
        print "ChanUtils.get_channel_id:  get_event_channel(", id, "): failed"
        return None # error

    if (chan is None):
        print "ChanUtils.get_channel_id: get_event_channel(", id, "): failed"
        return None # error

    if (verbose > 0):
	print "ChanUtils.get_channel_id: returning channel with id", id

    return chan

##############################################################
def destroy_channel(chan, verbose):
    # Destroy the channel
    try:
	if (verbose > 0):
	    print "ChanUtils.destroy_channel: calling chan.destroy()"
        chan.destroy()
    except:
	if (verbose > 0):
	    print "ChanUtils.destroy_channel: chan.destroy failed"
	return 1
    if (verbose > 0):
	print "ChanUtils.destroy_channel: Destroyed the channel"
    return 0

