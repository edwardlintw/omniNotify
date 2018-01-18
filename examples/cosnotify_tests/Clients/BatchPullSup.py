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

# Module BatchPullSup defines class BatchPullSup.C + constructor BatchPullSup.Create

from omniORB import CORBA
import CosNotification, CosNotifyChannelAdmin # import stubs
import CosNotifyComm__POA # import skeletons
import CliMixin, SupMixin, PullSupMixin, PingCliMixin
from Utils import CliUtils

# public module state
Tdoc = "Using CosNotifyComm::SequencePullSupplier Client"
Tname = "BatchPullSup"

class C(CosNotifyComm__POA.SequencePullSupplier, CliMixin.C, SupMixin.C, PullSupMixin.C, PingCliMixin.C):
    "pull supplier of CosNotification::EventBatch"

    # IDL methods
    def pull_structured_events(self, max_number):
	"channel pulls an event batch"
	batch = self.L_pull(max_number)
	self.num_batches += 1
	return batch

    def try_pull_structured_events(self, max_number):
	"channel tries to pull an event batch"
	batch = self.L_try_pull(max_number)
	if (batch == None):
	    batch = [ ]
	    return batch, CORBA.FALSE # has_event = false
	self.num_batches += 1
	return batch, CORBA.TRUE # has_event = true

    def disconnect_sequence_pull_supplier(self):
        "channel informs of disconnect"
	return self.L_disconnect()

    # Two  helpers are required

    def L_connect_helper(self):
	"connect self to my_proxy"
	# first change MaximumBatchSize
	prop1 = CosNotification.Property("MaximumBatchSize",
					 CORBA.Any(CORBA._tc_long, self.__batch_size))
	self.__my_proxy.set_qos( [prop1] )
	# connect
	self.__my_proxy.connect_sequence_pull_supplier(self._this())

    def L_cleanup_helper(self, proxy):
	"disconnect self from proxy"
	proxy.disconnect_sequence_pull_consumer()

# Stand-Alone Constructor
def Create(chan, maxevs, bsize, objnm, idx, efn, tfn, ofs, constraints, mil, opr, vb):
    "stand-alone constructor"
    admin, proxy = CliUtils.get_proxy_for_sup(CosNotifyChannelAdmin.SEQUENCE_EVENT, "pull", opr,
					      CosNotifyChannelAdmin.SequenceProxyPullConsumer,
					      chan, objnm, ofs, vb)
    if (proxy == None):
	return None
    client = C(chan, admin, proxy, maxevs, bsize, objnm, idx, efn, tfn, mil, vb)
    client.num_batches = 1 # tracks the NEXT batch number
    b = client.L_add_filter("proxy", constraints)
    if (b == 1):
	try:
	    admin.destroy()
	except:
	    pass
	return None
    return client
