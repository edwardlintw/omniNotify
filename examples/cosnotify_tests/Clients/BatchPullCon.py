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

# Module BatchPullCon defines class BatchPullCon.C + constructor BatchPullCon.Create

import CosNotifyChannelAdmin # import stubs
import CosNotifyComm__POA # import skeletons
import CliMixin, ConMixin, PullConMixin
from Utils import CliUtils

# public module state
Tdoc = "Using CosNotifyComm::SequencePullConsumer Client"
Tname = "BatchPullCon"

class C(CosNotifyComm__POA.SequencePullConsumer, CliMixin.C, ConMixin.C, PullConMixin.C):
    "pull consumer of CosNotification::EventBatch"

    # IDL methods that just call mixin methods
    def disconnect_sequence_pull_consumer(self):
        "channel informs of disconnect"
	return self.L_disconnect()

    # Three helpers are required

    def L_pull_helper(self):
	"try to pull an event batch from the channel -- return None if no batch"
	batch, has_batch = self.__my_proxy.try_pull_structured_events(self.__batch_size)
	if (has_batch):
	    self.num_batches += 1
	    return batch
	return None

    def L_connect_helper(self):
	"connect self to my_proxy"
	self.__my_proxy.connect_sequence_pull_consumer(self._this())

    def L_cleanup_helper(self, proxy):
	"disconnect self from proxy"
	proxy.disconnect_sequence_pull_supplier()

# Stand-Alone Constructor
def Create(chan, maxevs, bsize, objnm, idx, efn, tfn, constraints, mil, opr, vb):
    "stand-alone constructor"
    admin, proxy = CliUtils.get_proxy_for_con(CosNotifyChannelAdmin.SEQUENCE_EVENT, "pull", opr,
					      CosNotifyChannelAdmin.SequenceProxyPullSupplier,
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
