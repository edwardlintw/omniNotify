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

# Module ConMixin: Defines mixin ConMixin.C

from omniORB import CORBA
import CosEventChannelAdmin,CosNotifyChannelAdmin # import stubs
from Utils import CliUtils

class C:
    "mixin for building a consumer"

    # IDL methods
    def offer_change(self, added, deled):
        "handles notification of a change in event types offered by suppliers"
	self.__oplock.acquire()
	if (self.__cleaning_up == 1):
	    self.__oplock.release()
	    return
	self.num_changes += 1
	if (self.__change_fn != None):
            self.__change_fn(added, deled, self.obj_name, self.num_changes, self.__verbose)
	elif (self.__verbose > 0):
	    print self.obj_name, ": offer_change received [#", self.num_changes, "]: ",
	    CliUtils.describe_change(added, deled)
	    print " " 
	self.__oplock.release()

    # Local methods

    # NB L_got_event assumes lock has been acquired
    # also assumes num_batches has been updated (but only by batch consumer)
    # Returns 1 if done is set to 1, else 0
    # As side-effect, does __done_is_1.notifyAll when done is set to 1
    def L_got_events(self, data):
        "got an event / event batch"
	if ((self.__done == 1) or (self.__pxstate == "connected-paused")):
	    print self.obj_name, "XXX L_got_events called while pxstate is", self.__pxstate, "and done is", self.__done, "--- num_events not changed; num_extra updated XXX"
	    self.num_extra += len(data)
	    return self.__done
	self.num_events += len(data)
	if (self.__event_fn != None):
            self.__event_fn(data, self.obj_name, self.test_idx, self.num_events, self.num_batches, self.__verbose)
	elif (self.__verbose > 0):
	    print self.obj_name, ": event count =", self.num_events, " batch count =", self.num_batches
	if ((self.__max_events > 0) and (self.num_events >= self.__max_events)):
	    if (self.__verbose > 0):
		print self.obj_name, ": DONE [max_events reached]"
	    self.__done = 1
	    self.__done_is_1.notifyAll()
	return self.__done

    def L_register_consume_fn(self, fn):
        "register a consume function"
	self.__oplock.acquire()
	if (self.__done == 1):
	    self.__oplock.release()
	    return
        self.__event_fn = fn
	self.__oplock.release()

    def L_change_reg_helper(self):
	self.__my_proxy.obtain_offered_types(CosNotifyChannelAdmin.NONE_NOW_UPDATES_ON)
