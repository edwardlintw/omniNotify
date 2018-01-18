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

# Module PingCliMixin: Defines mixin PingCliMixin.C

class C:
    "mixin for building a ping client"

    # Local __call__ method (for worker thread)
    def __call__(self):
	"worker thread"
	self.__oplock.acquire()
	# print "XXX", self.obj_name, ": Spawned thread entering ping loop"
	if ((self.__verbose > 0)and (self.__done == 0) and (self.__secs > 0)):
	    print self.obj_name, ": Spawned thread entering ping loop"
	# invariant: _oplock held at top of loop
	while ((self.__done == 0) and (self.__secs > 0)):
	    self.__oplock.release() # do not hold oplock across ping
	    try:
		self.__my_proxy.MyType() # ping
	    except:
		self.__oplock.acquire()
		self.L_comm_err("communication error while pinging proxy using MyType()")
		break # break from while loop -- done
	    self.__oplock.acquire()
	    if (self.__done == 1):
		break # must have disconnected during ping
	    # wait for specified interval
	    # print "XXX", self.obj_name, ": ping loop doing __done_is_1.wait(", self.__secs, ")"
	    self.__done_is_1.wait(self.__secs) # this releases __oplock during wait
	    # continue ping loop
	# done
	if (self.__verbose > 0):
	    print self.obj_name, ": Spawned thread exiting ping loop"
	# print "XXX", self.obj_name, ": Spawned thread exiting ping loop"
	self.__worker = None # no more worker
	self.__worker_finished.notifyAll()
	self.__oplock.release()
	return 0
