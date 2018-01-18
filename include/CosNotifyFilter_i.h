// -*- Mode: C++; -*-
//                              File      : CosNotifyFilter_i.h
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
//    set of C++ definitions for the CosNotifyFilter module
//
 
#ifndef _COS_NOTIFY_FILTER_I_H_
#define _COS_NOTIFY_FILTER_I_H_

#include "RDIsysdep.h"
#include "thread_wrappers.h"
#include "corba_wrappers.h"

#include "RDIOplocks.h"
#include "RDIstrstream.h"
#include "RDIHash.h"
#include "RDIEvent.h"
#include "RDIStaticEvalDefs.h"
#include "RDIDynamicEvalDefs.h"
#include "CosNotifyShorthands.h"
#include "RDILocksHeld.h"

class EventChannel_i;
class Filter_i;
class MappingFilter_i;

/** RDINotifySubscribe
  *
  * The default 'subscription_change' method does not include filter
  * information and, therefore,  cannot be used in the internal hash
  * tables.  The following class offers a propagate_schange
  * that includes a reference to the filter object whose event types
  * have changed.
  *
  * In addition, when destroy() is called on a given filter, we must
  * inform all objects that reference the filter to cleanup internal
  * structures.
  *
  * Some subscribers ONLY want the filter_destroy_i notifications,
  * some subscribers want both.  Subscribers that only sign up
  * for filter_destroy_i still need to implement a dummy propagate_schange.
  */

class RDINotifySubscribe {
public:
  virtual void propagate_schange(RDI_LocksHeld&             held,
				 const CosN::EventTypeSeq&  added,
				 const CosN::EventTypeSeq&  deled,
				 Filter_i*                  filter) = 0;
  virtual void filter_destroy_i(Filter_i* filter) = 0;
};

typedef RDINotifySubscribe*        RDINotifySubscribe_ptr;

/** ConstraintImpl
  *
  * For each constraint added to a filter, _constraints[i], there is
  * a corresponding implementation-object _constraint_impls[i], that
  * maintains a node with the parsed filter,  against which matching
  * is performed at runtime. 
  */

class ConstraintImpl {
public:
  ConstraintImpl() : just_types(0), node(0)    {;}
  ~ConstraintImpl() 	{ if (node) delete node; node=0; }

  // Create the internal parse tree and any other state which is
  // required for the given constraint expression. In case of an
  // error or invalid constraint expression, NULL is returned...

  static ConstraintImpl* create(const CosNF::ConstraintExp& constraint);

  ConstraintImpl& operator= (const ConstraintImpl& ci) 
	{ just_types=ci.just_types; node=ci.node; return *this; }

  CORBA::Boolean just_types;
  RDI_PCState*   node;
};

WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(ConstraintImpl*, ConstraintImplSeq);

/** Filter_i
  *
  * This class implements the OMG CosNF::Filter interface.
  * A filter has a set of constraints, each having a unique id. Each
  * constraint consists of two parts: an 'event type sequence' and a
  * 'constraint expression'.  The syntax is as follows:
  *
  *  { [{domain, type},{domain, type},...], "constraint expression"}
  *
  * The types in the event type sequence are implicitly OR'd. An '*'
  * in the domain or type name denotes a wildcard,  matching zero or
  * more characters that position.  An empty sequence  is equivalent 
  * to a sequence with the element ["*", "*"].
  *
  * The constraints in a filter are implicitly OR'd. An empty string
  * in constraint expression is considered the same as "true".
  */

class FilterFactory_i;

typedef struct {
  RDINotifySubscribe_ptr   callback;
  CORBA::Boolean           need_schange;
} RDINfyCB;

typedef RDI_Hash<CORBA::Long, Filter_i*> RDIFilterKeyMap;
typedef RDI_HashCursor<CORBA::Long, Filter_i*> RDIFilterKeyMapCursor;

class Filter_i : 
  WRAPPED_SKELETON_SUPER(AttNotification, Filter) {
  friend class FilterFactory_i;
public:
  Filter_i(const char* grammar, FilterFactory_i* factory);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

  // Methods from AttNotification::Filter Interface
  CosNF::FilterID MyFID() { return _fid; }

  // Methods from CosNotifyFilter Interfaces
  void  destroy( WRAPPED_DECLARG_VOID );
  char* constraint_grammar( WRAPPED_DECLARG_VOID )
    			{ return CORBA::string_dup(_constraint_grammar); }

  CosNF::ConstraintInfoSeq* add_constraints(
			  const CosNF::ConstraintExpSeq& constraints
			  WRAPPED_DECLARG );
  void modify_constraints(const CosNF::ConstraintIDSeq&   del_list,
			  const CosNF::ConstraintInfoSeq& mod_list
			  WRAPPED_DECLARG );
  CosNF::ConstraintInfoSeq* get_constraints(const CosNF::ConstraintIDSeq& id_list
                                           WRAPPED_DECLARG );
  CosNF::ConstraintInfoSeq* get_all_constraints( WRAPPED_DECLARG_VOID );
  void remove_all_constraints( WRAPPED_DECLARG_VOID );

  CORBA::Boolean match(const CORBA::Any& event WRAPPED_DECLARG );
  CORBA::Boolean match_structured(const CosN::StructuredEvent& event 
                                  WRAPPED_DECLARG );
  CORBA::Boolean match_typed(const CosN::PropertySeq& event WRAPPED_DECLARG );

  CosNF::CallbackID attach_callback(CosNC::NotifySubscribe_ptr callback
                                   WRAPPED_DECLARG );
  void detach_callback(CosNF::CallbackID callback WRAPPED_DECLARG );
  CosNF::CallbackIDSeq* get_callbacks( WRAPPED_DECLARG_VOID );

  // The following methods are not available to Notification clients

  CORBA::Boolean match_chan(const CORBA::Any& a, EventChannel_i* channel);
  CORBA::Boolean match_structured_chan(const CosN::StructuredEvent& event, EventChannel_i* channel);
  CORBA::Boolean match_typed_chan(const CosN::PropertySeq& event, EventChannel_i* channel);

  void create_ev_types_from_dom_list(CosN::EventTypeSeq& lst_seq);
  void notify_subscribers_i(RDI_LocksHeld&             held,
			    const CosN::EventTypeSeq&  add_seq, 
			    const CosN::EventTypeSeq&  del_seq);

  CORBA::Boolean rdi_match(RDI_StructuredEvent* evnt, EventChannel_i* chan=0);

  CosNF::CallbackID attach_callback_i(RDI_LocksHeld&                held,
				      RDINotifySubscribe_ptr        callback,
				      CORBA::Boolean                need_schange);
  void detach_callback_i(CosNF::CallbackID  callbackID);

  void fadmin_removal_i(RDI_LocksHeld&          held,
			CosNF::CallbackID       callbackID,
			RDINotifySubscribe_ptr  filter_holder);

  CORBA::Boolean  has_callbacks() 	{ return _callbacks.length()   ? 1:0; }
  CORBA::Boolean  has_callbacks_i()	{ return _callbacks_i.length() ? 1:0; }
  unsigned long   getID() const		{ return _fid; }

  RDIstrstream& log_output(RDIstrstream& str) const;

  const AttN::NameSeq& L_my_name() { return _my_name; }

  void out_commands(RDIstrstream& str);
  void out_info_descr(RDIstrstream& str);
  void out_short_descr(RDIstrstream& str);

  // static class functions
  static Filter_i* find_filter(const char* fname);
  static void out_info_filter(RDIstrstream& str, const char* fname);
  static void out_info_all_filters(RDIstrstream& str);
  static AttN::IactSeq* all_children(CORBA::Boolean only_cleanup_candidates);
  static AttN::NameSeq* all_filter_names();
#if 0
  static void cleanup_zero_cbs(RDIstrstream& str); // XXX_OBSOLETE ???
#endif

  static Filter_i* Filter2Filter_i(CosNF::Filter_ptr f);
  // If 'f' is a local filter object with impl type Filter_i,
  // return a valid Filter_i* pointer; otherwise return NULL
  static _nfy_attr Filter_i* null_filter;

#ifndef NO_OBJ_GC
  // filter GC
  static void obj_gc_all(RDI_TimeT curtime, CORBA::ULong deadFilter);
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadFilter);
#endif

private:
  RDIOplockEntry*            _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	             _last_use;
#endif
  CORBA::Boolean             _disposed;        // true once cleanup_and_dispose called
#ifndef NDEBUG
  CORBA::Boolean             _dbgout;
#endif
  AttN::NameSeq              _my_name;
  FilterFactory_i*           _factory;
  unsigned long              _fid;
  long                       _idcounter;
  unsigned long              _hashvalue;
  RDI_TimeT                  _last_detach;
  char*                      _constraint_grammar;
  CosNF::ConstraintInfoSeq*  _constraints;
  ConstraintImplSeq*         _constraint_impls;
  CosNF::CallbackID          _callback_serial;
  CosNF::CallbackID          _callback_i_serial;

  RDI_Hash<CosNF::CallbackID, CosNC::NotifySubscribe_ptr>  _callbacks;
  RDI_Hash<CosNF::CallbackID, RDINfyCB>                    _callbacks_i;
  RDI_Hash<RDI_EventType, void *>                          _flt_dom_ev_types; 
  RDI_Hash<RDI_EventType, CORBA::ULong>                    _flt_all_ev_types;

  // class state
  static _nfy_attr TW_Mutex            _classlock;
  static _nfy_attr CORBA::Long         _classctr;
  static _nfy_attr RDIFilterKeyMap*    _class_keymap;

  // helpers

  // caller should obtain bumped scope lock on filter
  CORBA::Boolean cleanup_and_dispose(RDI_LocksHeld&            held,
				     CORBA::Boolean            only_on_cb_zero,
				     WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  // no lock held by caller
  CORBA::Boolean destroy_i(CORBA::Boolean only_on_cb_zero);

  // caller should obtain bumped scope lock on filter
  void           _remove_all_constraints(RDI_LocksHeld& held);


  CORBA::Boolean _exists_constraint(const CosNF::ConstraintID& cstid,
				    CORBA::ULong& position) const;
  void 		 _update_ev_tables(const CosNF::ConstraintExp& cexpr,
			 	    CosN::EventTypeSeq& add_types, 
				    CosN::EventTypeSeq& del_types);
  void           _remove_constraint(const CosNF::ConstraintID& cstid, 
				    CosN::EventTypeSeq& add_types, 
				    CosN::EventTypeSeq& rem_types);
  CORBA::Boolean _event_is_dominated(const CosN::EventType& t1);
  void           _add_ev_type(CosN::EventTypeSeq& tsq, const RDI_EventType& etp);

  virtual ~Filter_i();
};


/** MappingFilter_i
  *
  * Implementation of the  CosNF::MappingFilter interface.
  * NOTE: this class is not supported yet
  */

class MappingFilter_i : 
  WRAPPED_SKELETON_SUPER(AttNotification, MappingFilter) 
{
  friend class FilterFactory_i;
public:
  MappingFilter_i(const char* grammar, const CORBA::Any& def_value,
		  FilterFactory_i* factory);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

  // Methods from CosNotifyFilter Interfaces

  void  destroy( WRAPPED_DECLARG_VOID );
  char* constraint_grammar( WRAPPED_DECLARG_VOID ) {
    return CORBA::string_dup(_constraint_grammar);
  }
  CORBA::TypeCode_ptr value_type( WRAPPED_DECLARG_VOID ) {
    return _def_value.type();
  }
  CORBA::Any*         default_value( WRAPPED_DECLARG_VOID ) {
    return new CORBA::Any(_def_value);
  }

  CosNF::MappingConstraintInfoSeq* add_mapping_constraints(
				const CosNF::MappingConstraintPairSeq& pair_list
			        WRAPPED_DECLARG );
  void modify_mapping_constraints(const CosNF::ConstraintIDSeq& del_list,
				  const CosNF::MappingConstraintInfoSeq& mod_list
				  WRAPPED_DECLARG );
  CosNF::MappingConstraintInfoSeq* get_mapping_constraints(const CosNF::ConstraintIDSeq& id_list
							   WRAPPED_DECLARG ); 
  CosNF::MappingConstraintInfoSeq* get_all_mapping_constraints( WRAPPED_DECLARG_VOID );
  void remove_all_mapping_constraints( WRAPPED_DECLARG_VOID );

  CORBA::Boolean match(const CORBA::Any & event,
		       WRAPPED_OUTARG_TYPE(CORBA::Any) result_to_set
                       WRAPPED_DECLARG );
  CORBA::Boolean match_structured(const CosN::StructuredEvent & event,
				  WRAPPED_OUTARG_TYPE(CORBA::Any) result_to_set
				  WRAPPED_DECLARG );
  CORBA::Boolean match_typed(const CosN::PropertySeq & event,
			     WRAPPED_OUTARG_TYPE(CORBA::Any) result_to_set
			     WRAPPED_DECLARG );

  // class state
  static _nfy_attr TW_Mutex            _classlock;
  static _nfy_attr CORBA::Long         _classctr;

#ifndef NO_OBJ_GC
#if 0
  // filter GC
  static void obj_gc_all(RDI_TimeT curtime, CORBA::ULong deadFilter);
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadFilter);
#endif
#endif

  // The following methods are not available to Notification clients

  const AttN::NameSeq& L_my_name() { return _my_name; }

 private:
  RDIOplockEntry*            _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	             _last_use;
#endif
  CORBA::Boolean             _disposed;        // true once cleanup_and_dispose called
  AttN::NameSeq              _my_name;
  char*                      _constraint_grammar;
  CORBA::Any                 _def_value;

  // helpers
  CORBA::Boolean cleanup_and_dispose(RDI_LocksHeld&            held,
				     CORBA::Boolean            only_on_cb_zero,
				     WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~MappingFilter_i();
};

/** FilterFactory_i
  *
  * This class implements the  OMG CosNF::FilterFactory 
  * interface.  A filter factory is used to create filter objects
  * for a given constraint language. Each filter factory might be 
  * able to support multiple constraint languages.  However, each
  * one of them MUST support the default constraint language that 
  * is specified in the OMG Notification Service Specification.
  */

class FilterFactory_i : 
  WRAPPED_SKELETON_SUPER(AttNotification, FilterFactory) 
{
  friend class Filter_i;
  friend class MappingFilter_i;
public:
  FilterFactory_i(const char* grammar="EXTENDED_TCL");

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

  CosNF::Filter_ptr        create_filter(const char* grammar WRAPPED_DECLARG );
  CosNF::MappingFilter_ptr create_mapping_filter(const char* grammar,
                                                const CORBA::Any& value
                                                WRAPPED_DECLARG );

  // The following methods are not available to Notification clients

  int            add_grammar(const char* grammar);
  void           del_grammar(const char* grammar);
  CORBA::Boolean is_supported(const char* grammar);

  const AttN::NameSeq& L_my_name() { return _my_name; }

  void out_commands(RDIstrstream& str);
  static void cleanup_all(RDIstrstream& str);

  void cleanup_and_dispose();
  CORBA::Boolean _is_supported(const char* grammar); // oplock already acquired; does the real work

private:
  enum { MAXGR = 5 };

  RDIOplockEntry*     _oplockptr;
  CORBA::Boolean      _disposed;        // true once cleanup_and_dispose called
  AttN::NameSeq       _my_name;
  char*               _clangs[MAXGR];  	// The list of supported grammars
  unsigned int        _nlangs;		// Number of registered grammars

  virtual ~FilterFactory_i();
};

/** FAdminHelper
  *
  * An administrative object is responsible for managing filter objects.
  * It implements the methods  from the OMG CosNF::FilterAdmin
  * interface except for add_filter(), which is overriden in all classes
  * that use this helper class.
  *
  * FAdminHelper does not do any locking; the oplock of the containing class
  * should be used to protect the FAdminHelper methods.
  */

typedef struct FAdminFilterEntry_s {
  CosNF::CallbackID    callback_id;
  Filter_i*            filter;
} FAdminFilterEntry;

class FAdminHelper {
public:
  FAdminHelper(const char *resty);
  ~FAdminHelper();

  // Methods to help implement CosNF::FilterAdmin Interface

  CosNF::Filter_ptr    get_filter(CosNF::FilterID fltrID);
  CosNF::FilterIDSeq*  get_all_filters();
  void                 remove_filter(RDI_LocksHeld&               held,
				     CosNF::FilterID              fltrID,
				     RDINotifySubscribe_ptr       filter_holder);
  void                 remove_all_filters(RDI_LocksHeld&          held,
					  RDINotifySubscribe_ptr  filter_holder);

  // omniNotify-specific methods

  CosNF::FilterID add_filter_i(RDI_LocksHeld&          held,
			       CosNF::Filter_ptr       new_filter, 
			       RDINotifySubscribe_ptr  filter_holder,
			       CORBA::Boolean          need_schange);
#if 0
  CosNF::FilterID add_filter_i(CosNF::Filter_ptr       new_filter); // XXX_REMOVE
#endif

  void            rem_filter_i(Filter_i*   fltr);
  CORBA::Boolean  has_filters() const { return _filters.length() ? 1 : 0; }

  void out_info_filters(RDIstrstream& str);
private:
  const char*                                    _resty;
  CosNF::FilterID                                _serial;
  CosNF::CallbackID                              _callbackID; 
  RDI_Hash<CosNF::FilterID, FAdminFilterEntry>   _filters;
};

////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, const Filter_i& fltr) { return fltr.log_output(str); }

#endif
