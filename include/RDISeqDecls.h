// -*- Mode: C++; -*-
//                              File      : RDISeqDecls.h
//                              Package   : omniNotify-Library
//                              Created on: 13-Oct-2003
//                              Authors   : gruber
//
//    Copyright (C) 1998-2003 AT&T Laboratories -- Research
//
//    This file is part of the omniNotify library
//    and is distributed with the omniNotify release.
//
//    The omniNotify library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free
//    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//    02111-1307, USA
//
// Description:  Some sequence declarations
//

#ifndef __RDISEQDECLS_H__
#define __RDISEQDECLS_H__

#include "CosNotifyShorthands.h"

class Filter_i;

class EventChannel_i;
class SupplierAdmin_i;
class ConsumerAdmin_i;
class EventProxyPushSupplier_i;
class EventProxyPullSupplier_i;
class EventProxyPushConsumer_i;
class EventProxyPullConsumer_i;

class RDIProxySupplier;
class ProxyPullSupplier_i;
class StructuredProxyPullSupplier_i;
class SequenceProxyPullSupplier_i;
class ProxyPushSupplier_i;
class StructuredProxyPushSupplier_i;
class SequenceProxyPushSupplier_i;

class RDIProxyConsumer;
class ProxyPullConsumer_i;
class StructuredProxyPullConsumer_i;
class SequenceProxyPullConsumer_i;
class ProxyPushConsumer_i;
class StructuredProxyPushConsumer_i;
class SequenceProxyPushConsumer_i;

// Some sequence-of-rep-pointers types
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(Filter_i*,                      FilterPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(EventChannel_i*,                EventChannelPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(ConsumerAdmin_i*,               ConsumerAdminPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(SupplierAdmin_i*,               SupplierAdminPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(EventProxyPushSupplier_i*,      EventProxyPushSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(EventProxyPullSupplier_i*,      EventProxyPullSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(EventProxyPushConsumer_i*,      EventProxyPushConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(EventProxyPullConsumer_i*,      EventProxyPullConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(RDIProxySupplier*,              RDIProxySuppliPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(ProxyPullSupplier_i*,           ProxyPullSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(StructuredProxyPullSupplier_i*, StructuredProxyPullSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(SequenceProxyPullSupplier_i*,   SequenceProxyPullSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(ProxyPushSupplier_i*,           ProxyPushSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(StructuredProxyPushSupplier_i*, StructuredProxyPushSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(SequenceProxyPushSupplier_i*,   SequenceProxyPushSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(RDIProxyConsumer*,              RDIProxyConsumPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(ProxyPullConsumer_i*,           ProxyPullConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(StructuredProxyPullConsumer_i*, StructuredProxyPullConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(SequenceProxyPullConsumer_i*,   SequenceProxyPullConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(ProxyPushConsumer_i*,           ProxyPushConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(StructuredProxyPushConsumer_i*, StructuredProxyPushConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(SequenceProxyPushConsumer_i*,   SequenceProxyPushConsumerPtrSeq);

// Some sequence-of-ref-ptr types

WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNF::Filter_ptr,                      CosNF_FilterPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::EventChannel_ptr,                CosNA_EventChannelPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::ConsumerAdmin_ptr,               CosNA_ConsumerAdminPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::SupplierAdmin_ptr,               CosNA_SupplierAdminPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosEvCA::ProxyPushSupplier_ptr,         CosEvCA_EventProxyPushSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosEvCA::ProxyPullSupplier_ptr,         CosEvCA_EventProxyPullSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosEvCA::ProxyPushConsumer_ptr,         CosEvCA_EventProxyPushConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosEvCA::ProxyPullConsumer_ptr,         CosEvCA_EventProxyPullConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::ProxyPullSupplier_ptr,           CosNA_ProxyPullSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::StructuredProxyPullSupplier_ptr, CosNA_StructuredProxyPullSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::SequenceProxyPullSupplier_ptr,   CosNA_SequenceProxyPullSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::ProxyPushSupplier_ptr,           CosNA_ProxyPushSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::StructuredProxyPushSupplier_ptr, CosNA_StructuredProxyPushSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::SequenceProxyPushSupplier_ptr,   CosNA_SequenceProxyPushSupplierPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::ProxyPullConsumer_ptr,           CosNA_ProxyPullConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::StructuredProxyPullConsumer_ptr, CosNA_StructuredProxyPullConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::SequenceProxyPullConsumer_ptr,   CosNA_SequenceProxyPullConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::ProxyPushConsumer_ptr,           CosNA_ProxyPushConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::StructuredProxyPushConsumer_ptr, CosNA_StructuredProxyPushConsumerPtrSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNA::SequenceProxyPushConsumer_ptr,   CosNA_SequenceProxyPushConsumerPtrSeq);

#endif  /*  !__RDISEQDECLS_H__  */
