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

# Module CliUtils: Helper functions used by all the clients

from omniORB import CORBA
import CosNotification,CosNotifyChannelAdmin,CosNotifyFilter # import stubs

def describe_change(added, deled):
    """describe an offer_change or subscription_change message
    """
    for t in added:
	print "+ %s::%s " % (t.domain_name, t.type_name),
    for t in deled:
	print "- %s::%s " % (t.domain_name, t.type_name),

def send_offer_change(objnm, proxy, added, deled, verbose):
    """send offer_change to proxy
    """
    if (verbose > 0):
	print objnm , ": sending offer change to channel: ",
	describe_change(added, deled)
	print " "
    try:
	proxy.offer_change(added, deled)
    except:
	if (verbose > 0):
	    print objnm, ": ** sending offer change failed **"
	return 1 # error
    return 0 # OK

def send_subscription_change(objnm, proxy, added, deled, verbose):
    """send subscription_change to proxy
    """
    if (verbose > 0):
	print objnm , ": sending subscription change to channel: ",
	describe_change(added, deled)
	print " "
    try:
	proxy.subscription_change(added, deled)
    except:
	if (verbose > 0):
	    print objnm, ": ** sending subscription change failed **"

def get_proxy_consumer(objnm, channel, ctype, push_proxy, opr, verbose):
    """obtain appropriate admin + proxy supplier
    """
    generic_proxy = None
    if (opr == "OR"):
	if (verbose > 0):
	    print "get_proxy_consumer: using CosNotifyChannelAdmin.OR_OP"
	ifoper = CosNotifyChannelAdmin.OR_OP
    else:
	if (verbose > 0):
	    print "get_proxy_consumer: using CosNotifyChannelAdmin.AND_OP"
	ifoper = CosNotifyChannelAdmin.AND_OP
    admin = None
    try:
        admin, admID = channel.new_for_suppliers(ifoper)
	if (admin == None):
            print objnm, ": Failed to obtain admin"
            return None, None
    except:
        print objnm, ": Failed to obtain admin"
        return None, None
    if (verbose > 0):
        print objnm, ": Obtained admin from channel"
    try:
	if (push_proxy == "push"):
            generic_proxy, prxID = admin.obtain_notification_push_consumer(ctype)
	else:
            generic_proxy, prxID = admin.obtain_notification_pull_consumer(ctype)
    except:
	pass
    if (generic_proxy == None):
        print objnm, ": Failed to obtain proxy"
	admin.destroy()
	return None, None
    if (verbose > 0):
	print objnm, ": Obtained proxy from admin"
    return admin, generic_proxy

def get_proxy_supplier(objnm, channel, ctype, push_proxy, opr, verbose):
    """obtain appropriate admin + proxy supplier
    """
    generic_proxy = None
    if (opr == "OR"):
	if (verbose > 0):
	    print "get_proxy_consumer: using CosNotifyChannelAdmin.OR_OP"
	ifoper = CosNotifyChannelAdmin.OR_OP
    else:
	if (verbose > 0):
	    print "get_proxy_consumer: using CosNotifyChannelAdmin.AND_OP"
	ifoper = CosNotifyChannelAdmin.AND_OP
    admin = None
    try:
        admin, admID = channel.new_for_consumers(ifoper)
	if (admin == None):
            print objnm, ": Failed to obtain admin"
            return None, None
    except:
        print objnm, ": Failed to obtain admin"
        return None, None
    if (verbose > 0):
        print objnm, ": Obtained admin from channel"
    try:
	if (push_proxy == "push"):
            generic_proxy, prxID = admin.obtain_notification_push_supplier(ctype)
	else:
            generic_proxy, prxID = admin.obtain_notification_pull_supplier(ctype)
    except:
	pass
    if (generic_proxy == None):
        print objnm, ": Failed to obtain proxy"
	admin.destroy()
	return None, None
    if (verbose > 0):
	print objnm, ": Obtained proxy from admin"
    return admin, generic_proxy

def build_constraints(objnm, constraints, verbose):
    "build constraint list from constraints, return None if invalid"
    exp = [ ]
    for i in range(len(constraints)):
	evs, cexpr = constraints[i]
	if (evs == None):
	    evs = [ ]
	if (cexpr == None):
	    print objnm, ": ERROR malformed cexpr"
	    return None # error
	exp1 = CosNotifyFilter.ConstraintExp(evs, cexpr)
	exp.append(exp1)
	if (verbose > 0):
	    if (len(evs) > 0):
		print objnm, ": Constraint #", (i+1), " has types: ",
		j = 0 
		for t in evs:
		    j += 1  
		    print t.domain_name + "::" + t.type_name,
		    if (j <= len(evs)):
			print ", ",
	    else:
		print objnm, ": Constraint #", (i+1), " has type *::*",
	    print ' and constraint expression "' + cexpr + '"'
    return exp

def add_filter(objnm, chan, fadmin, constraints, verbose):
    """add filter with constraints as given
    returns status, filter
    """
    if ((constraints == None) or (len(constraints) == 0)):
	if (verbose > 0):
            print objnm, ": (no filter used)"
	return 0, None # no error, no filter
    if (verbose > 0):
	print objnm, ": Adding filter with", len(constraints), "constraints:"
    exp = build_constraints(objnm, constraints, verbose)
    if (exp == None):
	return 1, None # error, no filter
    ffp = None
    filter = None
    try:
	ffp = chan._get_default_filter_factory()
	filter = ffp.create_filter("EXTENDED_TCL")
    except (CORBA.COMM_FAILURE):
	print objnm, ": Caught COMM_FAILURE obtaining filter object"
	return 1, None # error, no filter
    except:
	print objnm, ": Caught exception obtaining filter object"
	return 1, None # error, no filter
    if (verbose > 0):
	print objnm, ": Obtained filter from default filter factory"
    try:
	filter.add_constraints(exp)
	fadmin.add_filter(filter)
    except (CosNotifyFilter.InvalidConstraint):
	print objnm, ": Exception thrown : Invalid constraint expression"
	filter.destroy()
	return 1, None # error, no filter
#    except:
#	print objnm, ": Exception thrown while adding constraints"
#	filter.destroy()
#	return 1, None # error, no Filter
    return 0, filter # OK, filter

def add_constraints(objnm, filter, constraints, verbose):
    """add constraints to filter, returns status
    """
    if ((constraints == None) or (len(constraints) == 0)):
	if (verbose > 0):
            print objnm, ": (no constraints specified)"
	return 0 # no error
    if (verbose > 0):
	print objnm, ": Adding", len(constraints), "constraints to filter:"
    exp = build_constraints(objnm, constraints, verbose)
    if (exp == None):
	return 1 # error, no filter
    try:
	filter.add_constraints(exp)
    except (CosNotifyFilter.InvalidConstraint):
	print objnm, ": Exception thrown : Invalid constraint expression"
	return 1 # error
#    except:
#	print objnm, ": Exception thrown while adding constraints"
#	return 1 # error
    return 0 # OK

def get_proxy_for_sup(ctype, push_or_pull, opr, prox_class, chan, objnm, ofs, vb):
    """gets appropriate admin + proxy
    also does offer_change if ofs is not None
    Returns either admin, proxy (status OK) or None, None (status ERROR)
    """
    admin, generic_proxy = get_proxy_consumer(objnm, chan, ctype, push_or_pull, opr, vb)
    if (generic_proxy == None):
	return None, None # failed
    proxy = generic_proxy._narrow(prox_class)
    if (proxy == None):
	try:
	    admin.destroy()
	except:
	    pass
	return None, None # failed
    if (ofs != None):
	b = send_offer_change(objnm, proxy, ofs, [ ], vb)
	if (b == 1):
	    try:
		admin.destroy()
	    except:
		pass
	    return None, None # failed to send offer_change message
    return admin, proxy

def get_proxy_for_con(ctype, push_or_pull, opr, prox_class, chan, objnm, vb):
    """gets appropriate admin + proxy
    Returns either admin, proxy (status OK) or None, None (status ERROR)
    """
    admin, generic_proxy = get_proxy_supplier(objnm, chan, ctype, push_or_pull, opr, vb)
    if (generic_proxy == None):
	return None, None # failed
    proxy = generic_proxy._narrow(prox_class)
    if (proxy == None):
	try:
	    admin.destroy()
	except:
	    pass
	return None, None # failed
    return admin, proxy
