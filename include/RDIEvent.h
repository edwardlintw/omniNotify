// -*- Mode: C++; -*-
//                              File      : RDIEvent.h
//                              Package   : omniNotify-Library
//                              Created on: 1-Jan-1998
//                              Authors   : gruber&panagos
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
//
// Description:
//    proprietary interface
//
 
#ifndef _RDI_EVENT_H_
#define _RDI_EVENT_H_

#include <time.h>
#include "thread_wrappers.h"
#include "RDIBitmap.h"
#include "RDIHash.h"
#include "RDITimeWrappers.h"
#include "RDIStringDefs.h"
#include "RDIHashFuncs.h"
#include "RDIDynamicEvalDefs.h"
#include "CosNotifyShorthands.h"

// XXX For get_dname_rtval and get_tname_rtval, if _isany and no such member, 
// XXX of the encap any, currently we use "" and "%ANY" respectively
// XXX (the strings used in the se mapped encoding).
// XXX Also for get_etype_rtval if _isany and no such member we return
// XXX rtval with event type containing "" :: "%ANY".
// XXX Should we use NULL instead (meaning 'not found')? 

/** omniNotify EVENT TYPE
 * A wrapper around the  CosN::EventType structure that
 * provides some usefull member functions
 */

class RDI_EventType : public CosN::EventType {
public:
  inline RDI_EventType();
  inline RDI_EventType(const RDI_EventType& etype);
  inline RDI_EventType(const CosN::EventType& etype);
  inline RDI_EventType(const char* domain, const char* type);

  inline RDI_EventType& operator= (const CosN::EventType& etype);
  inline RDI_EventType& operator= (const RDI_EventType& etype);

  inline int operator== (const RDI_EventType& obj);
  inline int operator!= (const RDI_EventType& obj);

  inline static unsigned int hash(const void* etype);
  inline static int          rank(const void* left, const void* right);

  inline static int valid_sequence(CosN::EventTypeSeq& tseq,
				   CORBA::ULong&  invalid_index, 
				   CORBA::Boolean i_am_consumer=0);

  inline static void externalize_sequence(CosN::EventTypeSeq& tseq);

  // Check if event type 'et1' dominates event type 'et2'. The result 
  // is true in the following cases:
  // - et1.domain_name == "*" or "" && et1.type_name == "*" or "%ALL"
  // - et1.domain_name == "*" or "" && et1.type_name == et2.type_name
  // - et1.domain_name == et2.domain_name && et1.type_name == "*"
  // - et1.domain_name == et2.domain_name && et1.type_name == et2.type_name

  inline static int dominates(const CosN::EventType& et1,
			      const CosN::EventType& et2);

  // Given a sequence of event types, a sequence of added event types
  // and a sequence of deleted types,  compute the types that have to
  // be added to and deleted from the original sequence

  inline static void compute_diff(const CosN::EventTypeSeq& orgnl,
			          const CosN::EventTypeSeq& added,
				  const CosN::EventTypeSeq& deled,
				  CosN::EventTypeSeq& toadd,
				  CosN::EventTypeSeq& todel);

  // Given a sequence of event types, a sequence of added event types
  // and a sequence of deleted types, update the original sequence

  inline static void update(CosN::EventTypeSeq& orgnl,
			    const CosN::EventTypeSeq& added,
			    const CosN::EventTypeSeq& deled);
};

inline RDI_EventType::RDI_EventType()
{ domain_name = (const char *) "*"; type_name = (const char *) "*"; }

inline RDI_EventType::RDI_EventType(const RDI_EventType& etype)
{ domain_name = etype.domain_name; type_name = etype.type_name; }

inline RDI_EventType::RDI_EventType(const CosN::EventType& etype)
{ domain_name = etype.domain_name; type_name = etype.type_name; }

inline RDI_EventType::RDI_EventType(const char* domain, const char* type)
{ domain_name = domain; type_name = type; }

inline RDI_EventType& 
RDI_EventType::operator= (const CosN::EventType& etype)
{ domain_name = etype.domain_name; type_name = etype.type_name; return *this; }

inline RDI_EventType& RDI_EventType::operator= (const RDI_EventType& etype)
{ domain_name = etype.domain_name; type_name = etype.type_name; return *this; }

inline int RDI_EventType::operator== (const RDI_EventType& obj)
{ return (RDI_STR_NEQ(domain_name, obj.domain_name) ? 0 :
	  (RDI_STR_NEQ(type_name, obj.type_name) ? 0 : 1)); }

inline int RDI_EventType::operator!= (const RDI_EventType& obj)
{  return (RDI_STR_NEQ(domain_name, obj.domain_name) ? 1 :
	   (RDI_STR_NEQ(type_name, obj.type_name) ? 1 : 0)); }

inline unsigned int RDI_EventType::hash(const void* etype)
{ RDI_EventType* t = (RDI_EventType *) etype;
 return (RDI_StrHash(t->domain_name) << 24) ^ RDI_StrHash(t->type_name);
}

inline int RDI_EventType::rank(const void* left, const void* right)
{ RDI_EventType *l = (RDI_EventType *) left, *r = (RDI_EventType *) right;
 int drank = RDI_StrRank(l->domain_name, r->domain_name);
 return (drank == 0) ? RDI_StrRank(l->type_name, r->type_name) : drank;
}

inline int RDI_EventType::dominates(const CosN::EventType& et1,
                                    const CosN::EventType& et2)
{
  if ((RDI_STRLEN(et1.domain_name)==0) || (RDI_STR_EQ(et1.domain_name, "*"))) {
    if (RDI_STR_EQ(et1.type_name, "*") || RDI_STR_EQ(et1.type_name, "%ALL")) {
      return 1;
    }
    return RDI_STR_EQ(et1.type_name, et2.type_name) ? 1 : 0;
  } else if (RDI_STR_EQ(et1.domain_name, et2.domain_name)) {
    if ( RDI_STR_EQ(et1.type_name, "*") ) {
      return 1;
    }
    return RDI_STR_EQ(et1.type_name, et2.type_name) ? 1 : 0;
  }
  return 0;
}

inline void RDI_EventType::compute_diff(
					const CosN::EventTypeSeq& orgnl,
					const CosN::EventTypeSeq& added,
					const CosN::EventTypeSeq& deled,
					CosN::EventTypeSeq& toadd,
					CosN::EventTypeSeq& todel)
{
  CORBA::ULong ix, iz, sz;
  RDI_Bitmap otmap((unsigned int) orgnl.length());
  RDI_Bitmap ntmap((unsigned int) added.length());
  toadd.length(0); todel.length(0);
  otmap.clear();   ntmap.clear();

  if ( deled.length() != 0 ) {
    for (iz=0; iz < orgnl.length(); iz++) {
      for (ix=0; ix < deled.length(); ix++) {
	if ( RDI_STR_EQ(orgnl[iz].domain_name, deled[ix].domain_name) &&
	     RDI_STR_EQ(orgnl[iz].type_name, deled[ix].type_name) ) {
	  otmap.set((unsigned int) iz);
	}
      }
    }
  }
  if ( added.length() != 0 ) {
    for (iz=0; iz < orgnl.length(); iz++) {
      for (ix=0; ix < added.length(); ix++) {
	if ( RDI_STR_EQ(orgnl[iz].domain_name, added[ix].domain_name) &&
	     RDI_STR_EQ(orgnl[iz].type_name, added[ix].type_name) ) {
	  ntmap.set((unsigned int) ix);
	  if ( otmap.is_set((unsigned int) iz) )
	    otmap.clear((unsigned int) iz);
	}
      }
    }
  }
  sz = 0;
  for (iz=0; iz < orgnl.length(); iz++) {
    if ( otmap.is_set((unsigned int) iz) ) {
      todel.length(sz + 1);
      todel[sz].domain_name = orgnl[iz].domain_name;
      todel[sz++].type_name = orgnl[iz].type_name;
    }
  }
  sz = 0;
  for (iz=0; iz < added.length(); iz++) {
    if ( ntmap.is_clear((unsigned int) iz) ) {
      toadd.length(sz + 1);
      toadd[sz].domain_name = added[iz].domain_name;
      toadd[sz++].type_name = added[iz].type_name;
    }
  }
}

inline void RDI_EventType::update(CosN::EventTypeSeq& orgnl,
				  const CosN::EventTypeSeq& added,
				  const CosN::EventTypeSeq& deled)
{
  CORBA::ULong ix, iz;
  RDI_Bitmap otmap((unsigned int) orgnl.length());
  RDI_Bitmap ntmap((unsigned int) added.length());
  otmap.clear(); ntmap.clear();

  if ( deled.length() != 0 ) {
    for (iz=0; iz < orgnl.length(); iz++) {
      for (ix=0; ix < deled.length(); ix++) {
	if ( RDI_STR_EQ(orgnl[iz].domain_name, deled[ix].domain_name) &&
	     RDI_STR_EQ(orgnl[iz].type_name, deled[ix].type_name) ) { 
	  otmap.set((unsigned int) iz);
	}
      }
    }
  }
  if ( added.length() != 0 ) {
    for (iz=0; iz < orgnl.length(); iz++) {
      for (ix=0; ix < added.length(); ix++) {
	if ( RDI_STR_EQ(orgnl[iz].domain_name, added[ix].domain_name) &&
	     RDI_STR_EQ(orgnl[iz].type_name, added[ix].type_name) ) {
	  ntmap.set((unsigned int) ix);
	  if ( otmap.is_set((unsigned int) iz) )
	    otmap.clear((unsigned int) iz);
	}
      }
    }
  }
  if ( otmap.num_set() != 0 ) {		// Shift all non-deleted entries
    ix = (CORBA::ULong) otmap.first_set();
    for (iz = ix + 1; iz < orgnl.length(); iz++) {
      if ( otmap.is_clear((unsigned int) iz) ) {
	orgnl[ix].domain_name = orgnl[iz].domain_name;
	orgnl[ix++].type_name = orgnl[iz].type_name;
      }
    }
    orgnl.length(ix);
  }
  ix = orgnl.length();
  for (iz=0; iz < added.length(); iz++) {	// Add any new entries 
    if ( ntmap.is_clear((unsigned int) iz) ) {
      orgnl.length(ix + 1);
      orgnl[ix].domain_name = added[iz].domain_name;
      orgnl[ix++].type_name = added[iz].type_name;
    }
  }
}

// All forms of *::* are replaced with *::*   Valid forms include:
//       "" :: ""
//       "*" :: ""
//       "" :: "*"
//       "" :: "%ALL"
// We do not yet support wildcard use in domain or type that has some literals.
// For non-"*" names, only alphanumerics and underscore are allowed and the
// name must begin with an alpha or underscore.  The exceptions:
//     type_name can be: %ALL, %TYPED, %ANY
//

inline int RDI_EventType::valid_sequence(CosN::EventTypeSeq& tseq,
					 CORBA::ULong&  invalid_index,
					 CORBA::Boolean i_am_consumer)
{
  for (CORBA::ULong ix = 0; ix < tseq.length(); ix++) {
    if (!tseq[ix].type_name || RDI_STRLEN(tseq[ix].type_name) == 0) {
      tseq[ix].type_name = (const char*)"*"; // => string_dup
    }
    if (!tseq[ix].domain_name || RDI_STRLEN(tseq[ix].domain_name) == 0) {
      tseq[ix].domain_name = (const char*)"*"; // => string_dup
    }
    if (RDI_STR_EQ(tseq[ix].type_name, "%ALL")) {
      // %ALL only valid if domain_name is * (or empty ... which was changed to *)
      if (RDI_STR_NEQ(tseq[ix].domain_name, "*")) {
	invalid_index = ix;
	return 0;
      }
      tseq[ix].type_name = (const char*)"*"; // => string_free then string_dup
    }
    if ( (RDI_STRLEN(tseq[ix].type_name) > 1) && RDI_STRCHR(tseq[ix].type_name, '*') ) {
      // Cannot use wildcard in a normal type name
      invalid_index = ix;
      return 0;
    }
    if ( (RDI_STRLEN(tseq[ix].domain_name) > 1) && RDI_STRCHR(tseq[ix].domain_name, '*') ) {
      // Cannot use wildcard in a normal domain name
      invalid_index = ix;
      return 0;
    }
  }
  return 1;
}

// ** We replace the internal form of *::*   with  ""::"%ALL"
// because the spec seems to say this is required.  Note that the
// replacement is done in a copy of the type sequence that is only used
// by the ChangePool (it would be a disaster to replace *::* in other
// internal data structures because we test for *::* internally)
inline void RDI_EventType::externalize_sequence(CosN::EventTypeSeq& tseq) {
  for (CORBA::ULong ix = 0; ix < tseq.length(); ix++) {
    if ( RDI_STR_EQ(tseq[ix].domain_name, "*") &&
	 RDI_STR_EQ(tseq[ix].type_name, "*") ) {
      tseq[ix].domain_name = (const char*)"";    // => string_free then string_dup
      tseq[ix].type_name = (const char*)"%ALL";  // => string_free then string_dup
    }
  }
}


///////////////////////////////////////////////////////////////////////
// ***************************************************************** //
///////////////////////////////////////////////////////////////////////

/** omniNotify STRUCTURED EVENT
 * A wrapper around the CosN::StructuredEvent structure
 * that includes a timestamp - assigned by the event channel - and
 * an explicit reference counter.
 */

class RDI_StructuredEvent {
  friend class RDI_EventQueue;
public:
  enum state_t { INVALID, 	/* Invalid event state; at creation */
		 NEWBORN,	/* Just received it from a supplier */
		 DISPATCHED,	/* Has been dispatched for matching */
		 PENDING 	/* Not dispatched but may be needed */
  };

  inline  RDI_StructuredEvent(const CosN::StructuredEvent& evnt);
  inline  RDI_StructuredEvent(const RDI_StructuredEvent& evnt);
  inline  RDI_StructuredEvent(const CORBA::Any& evnt);
  inline  ~RDI_StructuredEvent();

  long    ref_counter(void) const  			{ return _rcnt; }
  void    incr_ref_counter_lock_held()                    { ++_rcnt; }
  void    decr_ref_counter_lock_held()                    { --_rcnt; }

  state_t get_state() const				{ return _stat; }
  void    set_state(state_t stat)			{ _stat = stat; }

  const CosN::StructuredEvent& get_cos_event() const	{ return _evnt; }

  CORBA::ULongLong get_timestamp() const		{ return _atms; }
  void set_timestamp(const CORBA::ULongLong atms)	{ _atms = atms; }

  void set_arrival_time() {
    RDI_UtcT ctime;
    ctime.set_curtime();
    _atms = ctime.greenwich_time();
  }
  const CosN::EventType& get_type() const
  { return _evnt.header.fixed_header.event_type; }

  // Retrieve fields of a structured event.  The following functions
  // return pointers that should NOT be freed --- they can return a
  // pointer into the middle of a struct

  inline const char*    get_event_name(void) const;
  inline const char*    get_domain_name(void) const;
  inline const char*    get_type_name(void) const;

  inline static void DupCosEvent(CosN::StructuredEvent&       dest, 
			         const CosN::StructuredEvent& from);
  struct Val_t {
    RDI_RTVal* _vahdr_val;
    RDI_RTVal* _fdata_val;
    Val_t() : _vahdr_val(0), _fdata_val(0) {;}
  };
  struct Key_t {
    const char* _nm;
    Key_t(const char* n = 0) : _nm(n) {;}

    inline int operator== (const Key_t& k);
    inline int operator!= (const Key_t& k);

    inline static unsigned int hash(const void* keyt);
    inline static int          rank(const void* left, const void* right);
  };

  inline const RDI_RTVal* get_top_rtval   (void);
  inline const RDI_RTVal* get_ename_rtval (void);
  inline const RDI_RTVal* get_dname_rtval (void);
  inline const RDI_RTVal* get_etype_rtval (void);
  inline const RDI_RTVal* get_tname_rtval (void);
  inline const RDI_RTVal* get_hdr_rtval   (void);
  inline const RDI_RTVal* get_vahdr_rtval (void);
  inline const RDI_RTVal* get_fdata_rtval (void);
  inline const RDI_RTVal* get_fhdr_rtval  (void);
  inline const RDI_RTVal* get_rob_rtval   (void);

  inline const RDI_RTVal* lookup_vahdr_rtval (unsigned int idx);
  inline const RDI_RTVal* lookup_fdata_rtval (unsigned int idx);

  inline const RDI_RTVal* lookup_vahdr_rtval (const char* nm);
  inline const RDI_RTVal* lookup_fdata_rtval (const char* nm);
  inline const RDI_RTVal* lookup_rtval       (const char* nm);

  // N.B. only the lock macros below should call this function
  TW_Mutex& macro_get_mutex()    			{ return _lock; }

private:
  typedef RDI_Hash<Key_t, Val_t> RTValMap;
  typedef RDI_HashCursor<Key_t, Val_t> RTValMapCursor;

  TW_Mutex             _lock;
  long                 _rcnt;	// An explicit  reference counter
  CORBA::Boolean       _isany;  // is _evnt an encaps CORBA::Any?
  CORBA::ULongLong     _atms;	// Timestamp @ arrival to channel
                                // (units = RDI_UtcT::greenwich_time)
  state_t              _stat;	// The current state of the event
  CosN::StructuredEvent _evnt;	// Actual  CosNotification  event
  RDI_StructuredEvent* _next;

  RDI_StructuredEvent()	{;}

  void n_incr_ref_counter_lock_held(long rcnt)        { _rcnt+=rcnt; }

  RTValMap*      _vmap;
  RDI_RTVal*     _vahdr_rtval; // array
  RDI_RTVal*     _fdata_rtval; // array

  CORBA::Boolean _set_cached_ename_rtval;
  CORBA::Boolean _set_cached_dname_rtval;
  CORBA::Boolean _set_cached_tname_rtval;
  CORBA::Boolean _set_cached_top_rtval;
  CORBA::Boolean _set_cached_hdr_rtval;
  CORBA::Boolean _set_cached_vahdr_rtval;
  CORBA::Boolean _set_cached_fdata_rtval;
  CORBA::Boolean _set_cached_fhdr_rtval;
  CORBA::Boolean _set_cached_etype_rtval;
  CORBA::Boolean _set_cached_rob_rtval;

  RDI_RTVal*     _cached_ename_rtval;
  RDI_RTVal*     _cached_dname_rtval;
  RDI_RTVal*     _cached_tname_rtval;
  RDI_RTVal*     _cached_top_rtval;
  RDI_RTVal*     _cached_hdr_rtval;
  RDI_RTVal*     _cached_vahdr_rtval;
  RDI_RTVal*     _cached_fdata_rtval;
  RDI_RTVal*     _cached_fhdr_rtval;
  RDI_RTVal*     _cached_etype_rtval;
  RDI_RTVal*     _cached_rob_rtval;

  inline void _init_vmap(void);
  inline void _init_vmap_from_any(void);
};

#define RDI_SEVENT_SCOPE_LOCK(nm, evp, whatfn) \
  TW_SCOPE_LOCK(nm, (evp)->macro_get_mutex(), "event", whatfn)

#define RDI_SEVENT_INCR_REF_COUNTER(evp, whatfn) do { \
  TW_SCOPE_LOCK(_tmp_sevent_lock, (evp)->macro_get_mutex(), "event", whatfn); \
  evp->incr_ref_counter_lock_held(); \
} while (0)

#define RDI_SEVENT_DECR_REF_COUNTER(evp, whatfn) do { \
  TW_SCOPE_LOCK(_tmp_sevent_lock, (evp)->macro_get_mutex(), "event", whatfn); \
  evp->decr_ref_counter_lock_held(); \
} while (0)

// Execute some protected code [evp lock held] followed by some
// unprotected code [evp lock not held], with both code fragments in the same scope.
// This macro should only be used when RDI_SEVENT_SCOPE_LOCK cannot
// be used (only when both code fragments must be in the same scope).
#define RDI_SEVENT_PROT_UNPROT(evp, whatfn, prot_code, unprot_code) \
do { \
  CORBA::Boolean _evlocked = 0; \
  TW_Mutex& _tmp_lock = (evp)->macro_get_mutex(); \
  try { \
    TW_ABOUT_TO_LOCK(&_tmp_lock, 0, "event", whatfn, __FILE__, __LINE__); \
    _tmp_lock.lock(); \
    TW_JUST_LOCKED(&_tmp_lock, 0, "event", whatfn, __FILE__, __LINE__); \
    _evlocked = 1; \
    prot_code; \
    TW_ABOUT_TO_UNLOCK(&_tmp_lock, 0, "event", whatfn, __FILE__, __LINE__); \
    _tmp_lock.unlock(); \
    _evlocked = 0; \
    unprot_code; \
  } catch (...) { \
    if (_evlocked) { \
      TW_ABOUT_TO_UNLOCK(&_tmp_lock, 0, "event", whatfn, __FILE__, __LINE__); \
      _tmp_lock.unlock(); \
    } \
    throw; \
  } \
} while (0)

// --------------------------------------------------------- //

inline RDI_StructuredEvent::RDI_StructuredEvent(const CosN::StructuredEvent& e) :
  _lock(), _rcnt(1), _isany(0), _stat(NEWBORN), _next(0), _vmap(0), 
  _vahdr_rtval(0), _fdata_rtval(0), _set_cached_ename_rtval(0), 
  _set_cached_dname_rtval(0), _set_cached_tname_rtval(0), 
  _set_cached_top_rtval(0),   _set_cached_hdr_rtval(0),
  _set_cached_vahdr_rtval(0), _set_cached_fdata_rtval(0), 
  _set_cached_fhdr_rtval(0),  _set_cached_etype_rtval(0), 
  _set_cached_rob_rtval(0),   _cached_ename_rtval(0), _cached_dname_rtval(0),
  _cached_tname_rtval(0),     _cached_top_rtval(0),   _cached_hdr_rtval(0), 
  _cached_vahdr_rtval(0),     _cached_fdata_rtval(0), _cached_fhdr_rtval(0),
  _cached_etype_rtval(0),     _cached_rob_rtval(0)
{
  _atms = 0;
  DupCosEvent(_evnt, e);
  _isany = RDI_STR_EQ(_evnt.header.fixed_header.event_type.type_name, "%ANY");
}

inline RDI_StructuredEvent::RDI_StructuredEvent(const RDI_StructuredEvent& e) :
  _lock(), _rcnt(1), _atms(e._atms), 
  _stat(e._stat), _next(0), _vmap(0), _vahdr_rtval(0), 
  _fdata_rtval(0), _set_cached_ename_rtval(0), _set_cached_dname_rtval(0), 
  _set_cached_tname_rtval(0), _set_cached_top_rtval(0), 
  _set_cached_hdr_rtval(0),   _set_cached_vahdr_rtval(0), 
  _set_cached_fdata_rtval(0), _set_cached_fhdr_rtval(0),  
  _set_cached_etype_rtval(0), _set_cached_rob_rtval(0), _cached_ename_rtval(0),
  _cached_dname_rtval(0),     _cached_tname_rtval(0),   _cached_top_rtval(0),
  _cached_hdr_rtval(0),       _cached_vahdr_rtval(0),   _cached_fdata_rtval(0),
  _cached_fhdr_rtval(0),      _cached_etype_rtval(0),   _cached_rob_rtval(0)
{ 
  DupCosEvent(_evnt, e._evnt); 
  _isany = RDI_STR_EQ(_evnt.header.fixed_header.event_type.type_name, "%ANY");
}

inline RDI_StructuredEvent::RDI_StructuredEvent(const CORBA::Any& e) : 
  _lock(), _rcnt(1), _isany(1), _stat(NEWBORN), 
  _next(0), _vmap(0), _vahdr_rtval(0), _fdata_rtval(0), 
  _set_cached_ename_rtval(0), _set_cached_dname_rtval(0), 
  _set_cached_tname_rtval(0), _set_cached_top_rtval(0),   
  _set_cached_hdr_rtval(0),   _set_cached_vahdr_rtval(0), 
  _set_cached_fdata_rtval(0), _set_cached_fhdr_rtval(0),  
  _set_cached_etype_rtval(0), _set_cached_rob_rtval(0), _cached_ename_rtval(0),
  _cached_dname_rtval(0),     _cached_tname_rtval(0),   _cached_top_rtval(0),
  _cached_hdr_rtval(0),       _cached_vahdr_rtval(0),   _cached_fdata_rtval(0),
  _cached_fhdr_rtval(0),      _cached_etype_rtval(0),   _cached_rob_rtval(0)
{
  _atms = 0;
  _evnt.header.fixed_header.event_name = CORBA_STRING_DUP("");
  _evnt.header.fixed_header.event_type.domain_name = CORBA_STRING_DUP("");
  _evnt.header.fixed_header.event_type.type_name   = CORBA_STRING_DUP("%ANY");
  _evnt.header.variable_header.length(0);
  _evnt.filterable_data.length(0);
  _evnt.remainder_of_body = e;
}

inline RDI_StructuredEvent::~RDI_StructuredEvent()
{
  // introduce lock scope in new scope so that lock is released before destructor ends
  {
    TW_SCOPE_LOCK(_tmp_sevent_lock, _lock, "event", "RDI_StructuredEvent::~RDI_StructuredEvent");
    RDIDbgRDIEventLog("Deleting RDI_StructuredEvent with ref count " << _rcnt << '\n');
    _next = 0; 
    RDI_DELNULL(_vmap);
    if (_vahdr_rtval) { delete [] _vahdr_rtval; _vahdr_rtval = 0; }
    if (_fdata_rtval) { delete [] _fdata_rtval; _fdata_rtval = 0; }
    RDI_DELNULL(_cached_ename_rtval);
    RDI_DELNULL(_cached_dname_rtval);
    RDI_DELNULL(_cached_tname_rtval);
    RDI_DELNULL(_cached_top_rtval);
    RDI_DELNULL(_cached_hdr_rtval);
    RDI_DELNULL(_cached_vahdr_rtval);
    RDI_DELNULL(_cached_fdata_rtval);
    RDI_DELNULL(_cached_fhdr_rtval);
    RDI_DELNULL(_cached_etype_rtval);
    RDI_DELNULL(_cached_rob_rtval);
  }
}

// ----------- string get_ methods ----------- //

inline const char* RDI_StructuredEvent::get_event_name(void) const
{ return _evnt.header.fixed_header.event_name; }

inline const char* RDI_StructuredEvent::get_domain_name(void) const
{ return _evnt.header.fixed_header.event_type.domain_name; }

inline const char* RDI_StructuredEvent::get_type_name(void) const
{ return _evnt.header.fixed_header.event_type.type_name; }

// ----------- copy COS event ----------- //

inline void RDI_StructuredEvent::DupCosEvent(CosN::StructuredEvent&       d,
					     const CosN::StructuredEvent& f)
{ 
  CORBA::ULong ix;
  if (f.header.fixed_header.event_type.domain_name) {
    d.header.fixed_header.event_type.domain_name =
      f.header.fixed_header.event_type.domain_name;
  } else {
    d.header.fixed_header.event_type.domain_name = CORBA_STRING_DUP("");
  }
  if (f.header.fixed_header.event_type.type_name) {
    d.header.fixed_header.event_type.type_name = 
      f.header.fixed_header.event_type.type_name;
  } else {
    d.header.fixed_header.event_type.type_name = CORBA_STRING_DUP("");
  }
  if (f.header.fixed_header.event_name) {
    d.header.fixed_header.event_name = f.header.fixed_header.event_name;
  } else {
    d.header.fixed_header.event_name = CORBA_STRING_DUP("");
  }
  d.header.variable_header.length(f.header.variable_header.length());
  for (ix = 0; ix < f.header.variable_header.length(); ix++) {
    d.header.variable_header[ix].name  = f.header.variable_header[ix].name;
    d.header.variable_header[ix].value = f.header.variable_header[ix].value;
  }
  d.filterable_data.length(f.filterable_data.length());
  for (ix = 0; ix < f.filterable_data.length(); ix++) { 
    d.filterable_data[ix].name  = f.filterable_data[ix].name;
    d.filterable_data[ix].value = f.filterable_data[ix].value;
  }
  d.remainder_of_body = f.remainder_of_body;
}

inline void RDI_StructuredEvent::_init_vmap(void) 
{
  unsigned int i;
  if (_vmap) return;
  if (_isany) {
    _init_vmap_from_any();
    return;
  }

  RDIDbgRDIEventLog("_init_vmap: starting init\n");
  Key_t key;
  Val_t vnode;
  _vmap = new RTValMap(Key_t::hash, Key_t::rank);
  if (_evnt.header.variable_header.length())
    _vahdr_rtval = new RDI_RTVal[_evnt.header.variable_header.length()];
  if (_evnt.filterable_data.length())
    _fdata_rtval = new RDI_RTVal[_evnt.filterable_data.length()];

  for (i=0; i < _evnt.header.variable_header.length(); i++) {
    _vahdr_rtval[i].init_from_any( _evnt.header.variable_header[i].value );
    const char* nm = _evnt.header.variable_header[i].name;
    key._nm = nm;
    if ( nm ) {
      vnode._vahdr_val = &(_vahdr_rtval[i]);
      vnode._fdata_val = 0;
      RDIDbgRDIEventLog("_init_vmap: calling insert for nm " << nm
			<< " vahdr_val = " << _vahdr_rtval[i] << '\n');
      if ( _vmap->insert(key, vnode) ) {
	RDIDbgRDIEventLog("XXX failed to insert " << nm << " into vmap\n");
	// keep going...
      }
    }
  }

  for (i=0; i < _evnt.filterable_data.length(); i++) {
    _fdata_rtval[i].init_from_any( _evnt.filterable_data[i].value );
    const char* nm = _evnt.filterable_data[i].name;
    key._nm = nm;
    if ( nm ) {
      RDIDbgRDIEventLog("_init_vmap: calling lookup for nm " << nm << '\n');
      if (_vmap->lookup(key, vnode)) {	// update existing vnode
	RDIDbgRDIEventLog("_init_vmap: found it, so calling replace for nm " << nm
			  << " fdata_val = " << _fdata_rtval[i] << '\n');
	vnode._fdata_val = &(_fdata_rtval[i]);
	_vmap->replace(key, vnode);
      } else { // insert new vnode
	RDIDbgRDIEventLog("_init_vmap: not found, so calling insert for nm " << nm
			  << " fdata_val = " << _fdata_rtval[i] << '\n');
	vnode._vahdr_val = 0;
	vnode._fdata_val = &(_fdata_rtval[i]);
	if ( _vmap->insert(key, vnode) ) {
	  RDIDbgRDIEventLog("XXX failed to insert " << nm << " into vmap\n");
	  // keep going...
	}
      }
    }
  }

#ifndef NDEBUG
  RTValMapCursor cursor(_vmap);
  RDIDbgRDIEventLog("XXX _vmap now contains these buckets:\n");
  while ( cursor.is_valid() ) {
    const Key_t key = cursor.key();
    const Val_t vnode = cursor.const_val();
    RDIDbgRDIEventLog("XXX nm = " << key._nm << '\n');
    if (vnode._vahdr_val) {
      RDIDbgRDIEventLog("XXX vahdr_val = " << *(vnode._vahdr_val) << '\n');
    } else {
      RDIDbgRDIEventLog("XXX vahdr_val = NULL\n");
    }
    if (vnode._fdata_val) {
      RDIDbgRDIEventLog("XXX fdata_val = " << *(vnode._fdata_val) << '\n');
    } else {
      RDIDbgRDIEventLog("XXX fdata_val = NULL\n");
    }

    ++cursor;
  }
#endif
}

inline void RDI_StructuredEvent::_init_vmap_from_any(void) 
{
  RDIDbgRDIEventLog("_init_vmap_from_any: starting init\n");
  unsigned int                            iter_i, member_count;
  WRAPPED_DYNANY_MODULE::DynAny_var       da_elt;
  WRAPPED_DYNANY_MODULE::DynStruct_var    da_struct;
  WRAPPED_DYNANY_MODULE::DynUnion_var     da_union;

  Key_t key;
  Val_t vnode;
  _vmap = new RTValMap(Key_t::hash, Key_t::rank);

  const RDI_RTVal* rtv = get_top_rtval();
  if (! rtv) { // should not happen
    RDIDbgRDIEventLog("XXX _init_vmap_from_any: get_top_rtval failed\n");
    return;
  }
  if (rtv->_tckind != RDI_rtk_dynany) {
    // not a struct or union; nothing to init
    return;
  }
  // try to narrow to DynStruct
  try {
    da_struct = WRAPPED_DYNANY_MODULE::DynStruct::_narrow(rtv->_v_dynanyval._my_ptr);
    if (!CORBA::is_nil(da_struct)) {
      // first count the members
      for (member_count = 0; ; member_count++) {
	char* nm = 0;
	try {
	  if (member_count == 0) da_struct->seek(0);
	  else da_struct->next();
	  nm = da_struct->current_member_name();
	} catch(...) { } // catch exceptions due to seek, next, current_member_name
	if (nm == NULL) break; // no more components
      }
      if (member_count) {
	_vahdr_rtval = new RDI_RTVal[member_count];
	for (iter_i = 0; iter_i < member_count; iter_i++) {
	  char* nm = 0;
	  try {
	    if (iter_i == 0) da_struct->seek(0);
	    else da_struct->next();
	    nm = da_struct->current_member_name();
	  } catch(...) { } // catch exceptions due to seek, next, current_member_name
	  if (nm == NULL) break; // no more components
	  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
	  try {
	    da_elt = da_struct->current_component();
	  } catch (...) { } // catch exception due to invalid current_component
	  if (CORBA::is_nil(da_elt)) {
	    RDIDbgRDIEventLog("XXX _init_vmap_from_any: unexpected failure to extract struct member\n");
	    continue; // skip
	  }
	  _vahdr_rtval[iter_i].set_dynany(da_elt, 0, 0); // vahdr not in charge of destroying a "top" dynany
	  _vahdr_rtval[iter_i].simplify();
	  key._nm = nm;
	  vnode._vahdr_val = &(_vahdr_rtval[iter_i]);
	  vnode._fdata_val = 0;
	  RDIDbgRDIEventLog("_init_vmap_from_any: calling insert for nm " << nm
			    << " vahdr_val = " << _vahdr_rtval[iter_i] << '\n');
	  if ( _vmap->insert(key, vnode) ) {
	    RDIDbgRDIEventLog("XXX failed to insert " << nm << " into vmap\n");
	    // keep going...
	  }
	}
      }
    }
  } catch (...) {;} // not a struct
  // try to narrow to DynUnion
  try {
    da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(rtv->_v_dynanyval._my_ptr);
    if (!CORBA::is_nil(da_union)) {
      char* nm = 0;
      try {
	nm = da_union->member_name();
      } catch (...) { } // catch exception due to member_name
      if (nm == NULL) {
	RDIDbgRDIEventLog("XXX _init_vmap_from_any: could not extract union member name\n");
	return;
      }
      da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
      try {
	da_elt = da_union->member();
      } catch (...) { } // catch exception due to member
      if (CORBA::is_nil(da_elt)) {
	RDIDbgRDIEventLog("XXX _init_vmap_from_any: unexpected failure to extract member value from a dynunion\n");
	return;
      }
      // a union has only one member
      _vahdr_rtval = new RDI_RTVal[1];
      _vahdr_rtval[0].set_dynany(da_elt, 0, 0); // vahdr not in charge of destroying a "top" dynany
      _vahdr_rtval[0].simplify();
      key._nm = nm;
      vnode._vahdr_val = &(_vahdr_rtval[0]);
      vnode._fdata_val = 0;
      RDIDbgRDIEventLog("_init_vmap_from_any: calling insert for nm " << nm
			<< " vahdr_val = " << _vahdr_rtval[0] << '\n');
      if ( _vmap->insert(key, vnode) ) {
	RDIDbgRDIEventLog("XXX failed to insert " << nm << " into vmap\n");
      }
    }
  } catch (...) {;} // not union

#ifndef NDEBUG
  RTValMapCursor cursor(_vmap);
  RDIDbgRDIEventLog("XXX _vmap now contains these buckets:\n");
  while ( cursor.is_valid() ) {
    const Key_t key = cursor.key();
    const Val_t vnode = cursor.const_val();
    RDIDbgRDIEventLog("XXX nm = " << key._nm << '\n');
    if (vnode._vahdr_val) {
      RDIDbgRDIEventLog("XXX vahdr_val = " << *(vnode._vahdr_val) << '\n');
    } else {
      RDIDbgRDIEventLog("XXX vahdr_val = NULL\n");
    }
    if (vnode._fdata_val) {
      RDIDbgRDIEventLog("XXX fdata_val = " << *(vnode._fdata_val) << '\n');
    } else {
      RDIDbgRDIEventLog("XXX fdata_val = NULL\n");
    }
    ++cursor;
  }
#endif
}

inline const RDI_RTVal* RDI_StructuredEvent::lookup_vahdr_rtval(unsigned int idx) {
  if (_isany) return 0;
  if (idx >= _evnt.header.variable_header.length()) return 0;
  _init_vmap();
  return &(_vahdr_rtval[idx]);
}

inline const RDI_RTVal* RDI_StructuredEvent::lookup_fdata_rtval(unsigned int idx) {
  if (_isany) return 0;
  if (idx >= _evnt.filterable_data.length()) return 0;
  _init_vmap();
  return &(_fdata_rtval[idx]);
}


inline const RDI_RTVal* RDI_StructuredEvent::lookup_vahdr_rtval(const char* nm) {
  if (_isany) return 0;
  Key_t key(nm);
  Val_t vnode;
  _init_vmap();
  if (_vmap->lookup(key, vnode)) {
    return vnode._vahdr_val; // could be NULL
  }
  return 0;
}

inline const RDI_RTVal* RDI_StructuredEvent::lookup_fdata_rtval(const char* nm) {
  if (_isany) return 0;
  Key_t key(nm);
  Val_t vnode;
  _init_vmap();
  if (_vmap->lookup(key, vnode)) {
    return vnode._fdata_val; // could be NULL
  }
  return 0;
}

// if nm is in both vahdr and fdata, vahdr wins
inline const RDI_RTVal* RDI_StructuredEvent::lookup_rtval(const char* nm) {
  Key_t key(nm);
  Val_t vnode;
  _init_vmap();
  if (_vmap->lookup(key, vnode)) {
    if ( vnode._vahdr_val ) return vnode._vahdr_val;
    return vnode._fdata_val;
  }
  return 0;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_top_rtval   (void) {
  if (_set_cached_top_rtval) return _cached_top_rtval;

  _cached_top_rtval = new RDI_RTVal;
  if (_isany) {
    _cached_top_rtval->init_from_any( _evnt.remainder_of_body );
  } else {
    CORBA::Any a;
    a <<= _evnt;
    _cached_top_rtval->init_from_any( a );
  }
  _set_cached_top_rtval = 1;
  return _cached_top_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_dname_rtval   (void) {
  if (_set_cached_dname_rtval) return _cached_dname_rtval;
  _cached_dname_rtval = new RDI_RTVal;
  if (_isany) { // if exists top-level 'domain_name' in any use it, otherwise fall through
    const RDI_RTVal* rtv = lookup_rtval("domain_name");
    if (rtv) {
      *_cached_dname_rtval = *rtv; // see RTVal operator=
      _set_cached_dname_rtval = 1;
      return _cached_dname_rtval;
    }
  }
  _cached_dname_rtval->set_string(_evnt.header.fixed_header.event_type.domain_name, 0);
  _set_cached_dname_rtval = 1;
  return _cached_dname_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_tname_rtval   (void) {
  if (_set_cached_tname_rtval) return _cached_tname_rtval;
  _cached_tname_rtval = new RDI_RTVal;
  if (_isany) { // if exists top-level 'type_name' in any use it, otherwise fall through
    const RDI_RTVal* rtv = lookup_rtval("type_name");
    if (rtv) {
      *_cached_tname_rtval = *rtv; // see RTVal operator=
      _set_cached_tname_rtval = 1;
      return _cached_tname_rtval;
    }
  }
  _cached_tname_rtval->set_string(_evnt.header.fixed_header.event_type.type_name, 0);
  _set_cached_tname_rtval = 1;
  return _cached_tname_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_etype_rtval (void) {
  if (_set_cached_etype_rtval) return _cached_etype_rtval;

  if (_isany) { // if exists top-level 'event_type' in any use it, otherwise fall through
    const RDI_RTVal* rtv = lookup_rtval("event_type");
    if (rtv) {
      _cached_etype_rtval = new RDI_RTVal;
      *_cached_etype_rtval = *rtv; // see RTVal operator=
      _set_cached_etype_rtval = 1; 
      return _cached_etype_rtval;
    }
  }

  CORBA::Any a;
  a <<= _evnt.header.fixed_header.event_type;
  _cached_etype_rtval = new RDI_RTVal;
  _cached_etype_rtval->init_from_any( a );
  _set_cached_etype_rtval = 1; 
  return _cached_etype_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_ename_rtval   (void) {
  if (_set_cached_ename_rtval) return _cached_ename_rtval;

  if (_isany) { // if exists top-level 'event_name' in any use it, otherwise use NULL
    const RDI_RTVal* rtv = lookup_rtval("event_name");
    if (rtv) {
      _cached_ename_rtval = new RDI_RTVal;
      *_cached_ename_rtval = *rtv; // see RTVal operator=
    } else {
      _cached_ename_rtval = 0;
    }
  } else {
    _cached_ename_rtval = new RDI_RTVal;
    _cached_ename_rtval->set_string(_evnt.header.fixed_header.event_name, 0);
  }
  _set_cached_ename_rtval = 1;
  return _cached_ename_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_hdr_rtval   (void) {
  if (_set_cached_hdr_rtval) return _cached_hdr_rtval;

  if (_isany) { // if exists top-level 'header' in any use it, otherwise use NULL
    const RDI_RTVal* rtv = lookup_rtval("header");
    if (rtv) {
      _cached_hdr_rtval = new RDI_RTVal;
      *_cached_hdr_rtval = *rtv; // see RTVal operator=
    } else {
      _cached_hdr_rtval = 0;
    }
  } else { // structured event
    _cached_hdr_rtval = new RDI_RTVal;
    CORBA::Any a;
    a <<= _evnt.header;
    _cached_hdr_rtval->init_from_any( a );
  }
  _set_cached_hdr_rtval = 1;
  return _cached_hdr_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_vahdr_rtval (void) {
  if (_set_cached_vahdr_rtval) return _cached_vahdr_rtval;

  if (_isany) { // if exists top-level 'variable_header' in any use it, otherwise use NULL
    const RDI_RTVal* rtv = lookup_rtval("variable_header");
    if (rtv) {
      _cached_vahdr_rtval = new RDI_RTVal;
      *_cached_vahdr_rtval = *rtv; // see RTVal operator=
    } else {
      _cached_vahdr_rtval = 0;
    }
  } else { // structured event
    CORBA::Any a;
    a <<= _evnt.header.variable_header;
    _cached_vahdr_rtval = new RDI_RTVal;
    _cached_vahdr_rtval->init_from_any( a );
  }
  _set_cached_vahdr_rtval = 1;
  return _cached_vahdr_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_fdata_rtval (void) {
  if (_set_cached_fdata_rtval) return _cached_fdata_rtval;

  if (_isany) { // if exists top-level 'filterable_data' in any use it, otherwise use NULL
    const RDI_RTVal* rtv = lookup_rtval("filterable_data");
    if (rtv) {
      _cached_fdata_rtval = new RDI_RTVal;
      *_cached_fdata_rtval = *rtv; // see RTVal operator=
    } else {
      _cached_fdata_rtval = 0;
    }
  } else { // structured event
    CORBA::Any a;
    a <<= _evnt.filterable_data;
    _cached_fdata_rtval = new RDI_RTVal;
    _cached_fdata_rtval->init_from_any( a );
  }
  _set_cached_fdata_rtval = 1;
  return _cached_fdata_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_fhdr_rtval  (void) {
  if (_set_cached_fhdr_rtval) return _cached_fhdr_rtval;

  if (_isany) { // if exists top-level 'fixed_header' in any use it, otherwise use NULL
    const RDI_RTVal* rtv = lookup_rtval("fixed_header");
    if (rtv) {
      _cached_fhdr_rtval = new RDI_RTVal;
      *_cached_fhdr_rtval = *rtv; // see RTVal operator=
    } else {
      _cached_fhdr_rtval = 0;
    }
  } else { // structured event
    CORBA::Any a;
    a <<= _evnt.header.fixed_header;
    _cached_fhdr_rtval = new RDI_RTVal;
    _cached_fhdr_rtval->init_from_any( a );
  }
  _set_cached_fhdr_rtval = 1;
  return _cached_fhdr_rtval;
}

inline const RDI_RTVal* RDI_StructuredEvent::get_rob_rtval   (void) {
  if (_set_cached_rob_rtval) return _cached_rob_rtval;

  if (_isany) { // if exists top-level 'remainder_of_body' in any use it, otherwise use NULL
    const RDI_RTVal* rtv = lookup_rtval("remainder_of_body");
    if (rtv) {
      _cached_rob_rtval = new RDI_RTVal;
      *_cached_rob_rtval = *rtv; // see RTVal operator=
    } else {
      _cached_rob_rtval = 0;
    }
  } else { // structured event
    _cached_rob_rtval = new RDI_RTVal;
    _cached_rob_rtval->init_from_any( _evnt.remainder_of_body );
  }
  _set_cached_rob_rtval = 1;
  return _cached_rob_rtval;
}

////////////////////////////////////

inline int 
RDI_StructuredEvent::Key_t::operator== (const RDI_StructuredEvent::Key_t& k)
{ return RDI_STR_EQ(_nm, k._nm); }

inline int 
RDI_StructuredEvent::Key_t::operator!= (const RDI_StructuredEvent::Key_t& k)
{  return RDI_STR_NEQ(_nm, k._nm); }

inline unsigned int RDI_StructuredEvent::Key_t::hash(const void* v)
{
  RDI_StructuredEvent::Key_t* k = (RDI_StructuredEvent::Key_t *) v;
  return (RDI_StrHash(k->_nm));
}

inline int RDI_StructuredEvent::Key_t::rank(const void* left, const void* right)
{
  RDI_StructuredEvent::Key_t* kl = (RDI_StructuredEvent::Key_t *) left;
  RDI_StructuredEvent::Key_t* kr = (RDI_StructuredEvent::Key_t *) right;
  return ( RDI_StrRank(kl->_nm, kr->_nm) );
}


#endif
