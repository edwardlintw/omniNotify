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

# Module PullSupMixin defines mixin PullSupMixin.C

from omniORB import CORBA

class C:
    "mixin for building a pull supplier client"

    # Local methods

    def L_try_pull(self, max_number):
	self.__oplock.acquire()
	if (self.__cleaning_up == 1):
	    # this is no longer a valid obj ref
	    self.__oplock.release()
	    raise CORBA.INV_OBJREF, (0, CORBA.COMPLETED_NO)
	if (self.__batch_size < max_number):
	    max_number = self.__batch_size
	if (max_number < 1):
	    print self.obj_name, ": Internal Error: L_try_pull using max_number batch size < 1, setting to 1"
	    max_number = 1
	if ((self.__done == 1) or (self.__pxstate != "connected")):
	    # cannot supply an event at this time
	    self.__oplock.release()
	    return None # causes a null event to be returned with has_event = false
	if (len(self.__evqueue) < max_number):
	    data = self.L_need_events(max_number - len(self.__evqueue))
	    if (data != None):
		self.__evqueue += data
	res = None
	if (len(self.__evqueue) > 0):
	    if (len(self.__evqueue) <= max_number):
		res = self.__evqueue
		self.__evqueue = [ ]
	    else:
		res = self.__evqueue[ : max_number ]
		self.__evqueue = self.__evqueue[ max_number : ]
	    self.L_supplied_events(res)
	self.__oplock.release()
	return res

    def L_pull(self, max_number):
	self.__oplock.acquire()
	self.__blocked_pulls += 1 # decrement on return or signal
	if (self.__batch_size < max_number):
	    max_number = self.__batch_size
	if (max_number < 1):
	    print self.obj_name, ": Internal Error: L_pull using max_number batch size < 1, setting to 1"
	    max_number = 1
	res = None
	while (1):
	    if (self.__cleaning_up == 1):
		# this is no longer a valid obj ref
		self.__blocked_pulls -= 1
		self.__oplock.release()
		raise CORBA.INV_OBJREF, (0, CORBA.COMPLETED_NO)
	    if ((self.__done == 0) and (self.__pxstate == "connected")):
		if (len(self.__evqueue) < max_number):
		    data = self.L_need_events(max_number - len(self.__evqueue))
		    if (data != None):
			self.__evqueue += data
		if (len(self.__evqueue) > 0):
		    if (len(self.__evqueue) <= max_number):
			res = self.__evqueue
			self.__evqueue = [ ]
		    else:
			res = self.__evqueue[ : max_number ]
			self.__evqueue = self.__evqueue[ max_number : ]
		    break # res is valid
	    self.__done_is_1.wait(.001) # one millisec -- effect is similar to unlock/yield/lock (?)
	# if got here, res is valid
	self.L_supplied_events(res)
	self.__blocked_pulls -= 1
	if (self.__blocked_pulls == 0):
	    self.__blocked_pulls_is_0.notifyAll()
	self.__oplock.release()
	return res
