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

# Module CliMixin: Defines mixin CliMixin.C

from threading import Lock, Condition, Thread
from omniORB import CORBA
import CosEventChannelAdmin,CosNotifyChannelAdmin # import stubs
from Utils import CliUtils
from Utils import TestStats

class C:
    "mixin for building all clients"

    def __init__(self, chan, admin, proxy, maxevs, bsize, objnm, idx, efn, tfn, mil, vb):
        "constructor for all clients"
	# public state
	self.num_events   = 0
	self.num_extra    = 0
	self.num_batches  = 0
	self.num_changes  = 0
	self.obj_name     = objnm
	self.test_idx     = idx
	# private state
	self.__oplock = Lock()
	self.__done_is_1 = Condition(self.__oplock)
	self.__worker_finished = Condition(self.__oplock)
	self.__blocked_pulls_is_0 = Condition(self.__oplock)
	self.__channel  = chan
	self.__my_admin = admin
	self.__my_proxy = proxy
	self.__event_fn = efn
	self.__change_fn = tfn
	self.__max_events = maxevs
	self.__batch_size = bsize
	self.__secs = ( mil * 1.0)/1000.0
	self.__verbose = vb
	self.__pxstate = "init" # oneof (init, connected, connected-paused, disconnected, exception)
	self.__done = 0
	self.__cleaning_up = 0
	self.__blocked_pulls = 0
	self.__worker = None
	self.__evqueue = [ ]
	self.__my_proxy_filters = [ ]
	self.__my_admin_filters = [ ]
	self.__active = self._this() # implicitly activate servant

    # Local variants of IDL methods
    def L_disconnect(self):
        "channel informs of disconnect"
	self.__oplock.acquire()
	if (self.__cleaning_up == 1):
	    # this is no longer a valid obj ref
	    self.__oplock.release()
	    raise CORBA.INV_OBJREF, (0, CORBA.COMPLETED_NO)
	st = self.__pxstate
	self.__pxstate = "disconnected"
	if (st == "connected"):
	    self.__done = 1
	    self.__done_is_1.notifyAll()
	self.__oplock.release()

    def L_add_filter(self, where, constraints):
	"""add filter using constraints
	where is either 'proxy' or 'admin'
	"""
	self.__oplock.acquire()
	w = self.__my_proxy
	flist = self.__my_proxy_filters
	if (where == "admin"):
	    w = self.__my_admin
	    flist = self.__my_admin_filters
	b, f = CliUtils.add_filter(self.obj_name, self.__channel, w, constraints, self.__verbose)
	if (f != None):
	    flist.append(f)
	self.__oplock.release()
	return b

    def L_add_constraints(self, where, constraints):
	"""add constraints to existing filter
	where is either 'proxy' or 'admin'
	"""
	self.__oplock.acquire()
	w = self.__my_proxy
	flist = self.__my_proxy_filters
	if (where == "admin"):
	    w = self.__my_admin
	    flist = self.__my_admin_filters
	if (len(flist) < 1):
	    print self.obj_name, ": ERROR add_constraints target has no filter"
	    return 1 # error
	# just use first filter in list
	f = flist[0]
	b = CliUtils.add_constraints(self.obj_name, f, constraints, self.__verbose)
	self.__oplock.release()
	return b

    def L_connect(self):
	"""
	connect self to proxy; if already connected, that is OK
	start supplying/consuming events
	"""
	self.__oplock.acquire()
	if (self.__done == 1):
	    print self.obj_name, ": programming error, done should be zero when connect is called"
	    self.__oplock.release()
	    return 1 # error
	if (self.__worker != None):
	    print self.obj_name, ": programming error, worker should be None when connect is called"
	    self.__oplock.release()
	    return 1 # error
	if ((self.__pxstate == "disconnected") or (self.__pxstate == "exception")):
	    print self.obj_name, ": cannot resume connection : pxstate is", self.__pxstate
	    self.__oplock.release()
	    return 1 # error
	if ((self.__pxstate != "init") and (self.__pxstate != "connected-paused")):
	    print self.obj_name, ": programming error, pxstate (", self.__pxstate, ") should be init or connected-paused when connect is called"
	    self.__oplock.release()
	    return 1 # error
	if (self.__pxstate == "init"):
	    try:
		self.L_connect_helper()
		self.L_change_reg_helper()
	    except (CORBA.BAD_PARAM):
		print self.obj_name, ": BAD_PARAM exception while connecting"
		self.__oplock.release()
		return 1 # error
	    except (CosEventChannelAdmin.AlreadyConnected):
		print self.obj_name, ": Alread connected"
		self.__oplock.release()
		return 1 # error
	    #except:
	    #	 print self.obj_name, ": Failed to connect"
	    #	 self.__oplock.release()
	    #	 return 1 # error
	    if (self.__verbose > 0):
		print self.obj_name, ": Connected to proxy"
	# if not init, pxstate is connected-paused -- either way change to connected
	self.__pxstate = "connected"
	self.__worker = Thread(target = self)
	self.__worker.start() # start the worker, i.e., __call__(self) is invoked in a separate thread
	self.__oplock.release()
	return 0 # OK

    # return stats: 0 = OK, 1 = communication exception, 2 = timeout
    def L_wait_done(self, timeout):
	"connect self to proxy to start supplying or consuming events"
	self.__oplock.acquire()
	# print "XXX", self.obj_name, ": L_wait_done begin"
	res = 0 # OK
	if ( self.__done == 0 ):
	    # print "XXX", self.obj_name, ": L_wait_done doing done_is_1.wait"
	    self.__done_is_1.wait(timeout)
	else:
	    # print "XXX", self.obj_name, ": L_wait_done -- done begins 1, no initial done wait needed"
	    pass
	if ( self.__done == 0): # timeout occurred
	    # print "XXX", self.obj_name, ": L_wait_done detected timeout"
	    res = 2 # timeout
	    self.__done = 1 # will force worker to clean up
	if (self.__worker != None):
	    # give worker 2 secs to finish up
	    # print "XXX", self.obj_name, ": L_wait_done giving worker 2 secs to finish up"
	    self.__worker_finished.wait(2)
	    if (self.__worker != None):
		# worker must be 'stuck', set state to exception
		# print "XXX", self.obj_name, ": L_wait_done worker must be 'stuck', set state to exception"
		self.__pxstate = "exception"
		self.__worker = None # no way to kill stuck thread??
		TestStats.abandoned1(self.obj_name)
	    else:
		# print "XXX", self.obj_name, ": L_wait_done -- NOW worker is finished"
		pass
	else:
	    # print "XXX", self.obj_name, ": L_wait_done -- worker was finished after initial done wait (if any)"
	    pass
	if (self.__pxstate == "exception"):
	    # print "XXX", self.obj_name, ": L_wait_done detects pxstate is exception"
	    res = 1 # communication error / thread stuck
	    # note that timeout code can change into this error code
	if (res == 0): # XXX
	    # print "XXX", self.obj_name, ": L_wait_done returning OK status"
	    pass
	elif (res == 1): # XXX
	    # print "XXX", self.obj_name, ": L_wait_done returning EXCEPTION status"
	    pass
	elif (res == 2): # XXX
	    # print "XXX", self.obj_name, ": L_wait_done returning TIMEOUT status"
	    pass
	self.__oplock.release()
	return res

    def L_rem_all_filters(self):
	"remove all filters"
	self.__oplock.acquire()
	try:
	    self.__my_proxy.remove_all_filters()
	except:
	    pass
	try:
	    self.__my_admin.remove_all_filters()
	except:
	    pass
	filters = self.__my_admin_filters + self.__my_proxy_filters
	for f in filters:
	    try:
		f.destroy()
	    except:
		pass
	self.__my_proxy_filters = [ ]
	self.__my_admin_filters = [ ]
	self.__oplock.release()

    def L_reset(self, maxevs):
	"""should only be called after L_wait_done (or prior to first connect)
	gets ready for a(nother) connect call
	"""
	self.__oplock.acquire()
	if ((self.__pxstate != "init") and (self.__pxstate != "connected")):
	    print self.obj_name, ": reset called when proxy state =", self.__pxstate, ", cannot continue"
	    self.__oplock.release()
	    return 1 # error
	if ((self.__worker != None) or ((self.__pxstate == "connected") and (self.__done != 1))):
	    print self.obj_name, ": programming error: after connect, only call L_reset after calling L_wait_done"
	    self.__oplock.release()
	    return 1 # error
	if (self.__pxstate == "connected"):
	    self.__pxstate = "connected-paused"
	self.num_events   = 0
	self.num_extra    = 0
	self.num_batches  = 0
	self.__done       = 0
	self.__max_events = maxevs
	self.__evqueue = [ ]
	self.__oplock.release()
	return 0 # OK

    def L_cleanup(self):
	"if no error so far, disconnect from channel"
	self.__oplock.acquire()
	if (self.__cleaning_up == 1):
	    print self.obj_name, ": Coding error, L_cleanup already called"
	    self.__oplock.release()
	    return 1 # error
	self.__cleaning_up = 1 # this acts as a guard
	if ((self.__worker != None) or (self.__done != 1)):
	    print self.obj_name, ": Coding error: only call cleanup after calling wait_done"
	    self.__oplock.release()
	    return 1 # error
	# if there are any 'blocked' pull calls, wait for them to complete
	# (they will raise an exception now that cleaning_up is set to 1
	while (self.__blocked_pulls > 0):
	    print self.obj_name, ": there are", self.__blocked_pulls, "blocked pull calls -- waiting"
	    self.__blocked_pulls_is_0.wait()
	# this method takes sole ownership of some state
	proxy = self.__my_proxy
	admin = self.__my_admin
	filters = self.__my_admin_filters + self.__my_proxy_filters
	self.__my_proxy = None
	self.__my_admin = None
	self.__my_proxy_filters = [ ]
	self.__my_admin_filters = [ ]
	# do not hold oplock while invoking disconnect etc.
	self.__oplock.release()
	if ( ((self.__pxstate == "connected") or (self.__pxstate == "connected-paused")) and (proxy != None) ):
	    try:
		self.L_cleanup_helper(proxy)
	    except:
		pass
	    self.__pxstate = "disconnected"
	try:
	    admin.destroy()
	except:
	    pass
	for f in filters:
	    try:
		f.destroy()
	    except:
		pass
	return 0 # OK

    # Sets __pxstate to exception and __done to 1, and does __done_is_1.notifyAll
    def L_comm_err(self, errmsg):
	"a communication error occurred"
	print self.obj_name, ": ", errmsg
	self.__pxstate = "exception"
	self.__done = 1
	self.__done_is_1.notifyAll()
