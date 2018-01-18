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

# Module SupMixin: Defines mixin SupMixin.C

from omniORB import CORBA
import CosEventChannelAdmin,CosNotifyChannelAdmin # import stubs
from Utils import CliUtils

class C:
    "mixin for building a supplier"

    # IDL methods
    def subscription_change(self, added, deled):
        "handles notification of a change in event type subscriptions by consumers"
	self.__oplock.acquire()
	if (self.__cleaning_up == 1):
	    self.__oplock.release()
	    return
	self.num_changes += 1
	if (self.__change_fn != None):
            self.__change_fn(added, deled, self.obj_name, self.num_changes, self.__verbose)
	elif (self.__verbose > 0):
	    print self.obj_name, ": subscription_change received [#", self.num_changes, "]: ",
	    CliUtils.describe_change(added, deled)
	    print " "
	self.__oplock.release()

    # Local methods
    def L_need_events(self, num_hint):
	if ((self.__max_events > 0) and ((self.__max_events - self.num_events) < num_hint)):
	    num_hint = self.__max_events - self.num_events
	batch = None
	if (self.__event_fn != None):
	    batch = self.__event_fn(self.obj_name, self.test_idx, num_hint, self.num_events+1, self.num_batches, self.__verbose)
	return batch

    # Returns 1 if done is set to 1, else 0
    # As side-effect, does __done_is_1.notifyAll when done is set to 1
    def L_supplied_events(self, batch):
	"a batch of events has been supplied"
	if ((self.__done == 1) or (self.__pxstate == "connected-paused")):
	    print self.obj_name, "XXX L_supplied_events called while pxstate is", self.__pxstate, "and done is", self.__done, "--- num_events not changed; num_extra updated XXX"
	    self.num_extra += len(batch)
	    return self.__done
	self.num_events += len(batch)
	if ((self.__max_events > 0) and (self.num_events >= self.__max_events)):
	    if (self.__verbose > 0):
		print self.obj_name, ": DONE [max_events reached]"
	    self.__done = 1
	    self.__done_is_1.notifyAll()
	return self.__done

    def L_register_supply_fn(self, fn):
        "register a consume function"
	self.__oplock.acquire()
	if (self.__done == 1):
	    self.__oplock.release()
	    return
        self.__event_fn = fn
	self.__oplock.release()

    def L_change_reg_helper(self):
	self.__my_proxy.obtain_subscription_types(CosNotifyChannelAdmin.NONE_NOW_UPDATES_ON)

