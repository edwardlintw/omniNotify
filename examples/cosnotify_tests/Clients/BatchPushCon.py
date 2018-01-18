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

# Module BatchPushCon defines class BatchPushCon.C + constructor BatchPushCon.Create

from omniORB import CORBA
import CosNotification, CosNotifyChannelAdmin # import stubs
import CosNotifyComm__POA # import skeletons
import CliMixin, ConMixin, PingCliMixin
from Utils import CliUtils

# public module state
Tdoc = "Using CosNotifyComm::SequencePushConsumer Client"
Tname = "BatchPushCon"

class C(CosNotifyComm__POA.SequencePushConsumer, CliMixin.C, ConMixin.C, PingCliMixin.C):
    "push consumer of CosNotification::EventBatch"

    # IDL methods that just call mixin methods
    def push_structured_events(self, batch):
        "channel pushed a CosNotification::EventBatch"
	self.__oplock.acquire()
	if (self.__cleaning_up == 1):
	    # this is no longer a valid obj ref
	    self.__oplock.release()
	    raise CORBA.INV_OBJREF, (0, CORBA.COMPLETED_NO)
	self.num_batches += 1
	self.L_got_events(batch)
	self.__oplock.release()

    def disconnect_sequence_push_consumer(self):
        "channel informs of disconnect"
	return self.L_disconnect()

    # Two helpers are required

    def L_connect_helper(self):
	"connect self to my_proxy"
	# first change MaximumBatchSize
	prop1 = CosNotification.Property("MaximumBatchSize",
					 CORBA.Any(CORBA._tc_long, self.__batch_size))
	self.__my_proxy.set_qos( [prop1] )
	# connect
	self.__my_proxy.connect_sequence_push_consumer(self._this())

    def L_cleanup_helper(self, proxy):
	"disconnect self from proxy"
	proxy.disconnect_sequence_push_supplier()

# Stand-Alone Constructor
def Create(chan, maxevs, bsize, objnm, idx, efn, tfn, constraints, mil, opr, vb):
    "stand-alone constructor"
    admin, proxy = CliUtils.get_proxy_for_con(CosNotifyChannelAdmin.SEQUENCE_EVENT, "push", opr,
					      CosNotifyChannelAdmin.SequenceProxyPushSupplier,
					      chan, objnm, vb)
    if (proxy == None):
	return None
    client = C(chan, admin, proxy, maxevs, bsize, objnm, idx, efn, tfn, mil, vb)
    b = client.L_add_filter("proxy", constraints)
    if (b == 1):
	try:
	    admin.destroy()
	except:
	    pass
	return None
    return client
