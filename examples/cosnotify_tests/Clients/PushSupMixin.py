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

# Module PushSupMixin defines mixin PushSupMixin.C

from omniORB import CORBA
class C:
    "mixin for building a push supplier client"

    # Local __call__ method (for worker thread)
    def __call__(self):
	"worker thread"
	self.__oplock.acquire()
	# print "XXX", self.obj_name, ": Spawned thread entering push loop"
	if ((self.__verbose > 0)  and (self.__done == 0)):
	    print self.obj_name, ": Spawned thread entering push loop"
	# invariants: _oplock is held entering top of loop and 
	# also whenever we break out of loop
	# (it is *not* held during push and timed wait calls)
	while (self.__done == 0):
	    if (self.__pxstate == "connected"):
		batch = self.L_need_events(self.__batch_size)
		if (batch != None):
		    self.__oplock.release() # do not hold oplock across push
		    comerr = 0
		    comerrstr = ""
		    try:
			self.L_push_helper(batch)
		    except (CORBA.COMM_FAILURE):
			comerr = 1
			comerrstr = "COMM_FAILURE"
		    # except:
		    #    comerr = 1
		    #    comerrstr = "unknown error"
		    self.__oplock.acquire()
		    if (comerr):
			self.L_comm_err(comerrstr + " while pushing event(s)")
			break # break from while loop -- done
		    if (self.__done == 1):
			break # must have disconnected during push
		    if (self.L_supplied_events(batch) == 1):
			break # break from while loop -- done
	    # wait for specified interval
	    secs = self.__secs
	    if (secs == 0):
		secs = .05 # 1/20 sec
	    # print "XXX", self.obj_name, ": push loop doing __done_is_1.wait(", secs, ")"
	    self.__done_is_1.wait(secs) # this releases __oplock during wait
	    # continue push loop
	# done
	# print "XXX", self.obj_name, ": Spawned thread exiting push loop"
	if (self.__verbose > 0):
	    print self.obj_name, ": Spawned thread exiting push loop"
	self.__worker = None # no more worker
	self.__worker_finished.notifyAll()
	self.__oplock.release()
	return 0
