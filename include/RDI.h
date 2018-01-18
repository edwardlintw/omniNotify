// -*- Mode: C++; -*-
//                              File      : RDI.h
//                              Package   : omniNotify-Library
//                              Created on: 1-July-2001
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
//
// Description:
//    Class RDI provides APIs and global state to support:
//           * report logging
//           * debug logging
//           * singleton RDINotifServer w/ static init_server/get_server functions
//           * anything else?
 
#ifndef __RDI_H__
#define __RDI_H__

#include <stdio.h>
#include <stdlib.h>
#include "RDIsysdep.h"
#include "corba_wrappers.h"
#include "CosNotifyShorthands.h"
#include "CosNotification_i.h"
#include "RDIstrstream.h"

class RDINotifServer;

class RDI {
public:
  static AttN::Server_ptr init_server(int& argc, char** argv);
  static AttN::Server_ptr get_server() { return AttN::Server::_duplicate(_Server); }
  static RDI_ServerQoS*   get_server_qos();

  static RDINotifServer* get_server_i();
  static void wait_for_destroy();

  static void CleanupAll();
  // shutting down daemon; cleanup all global state

  static int OpenDbgFile(const char* pathnm);
  static int OpenRptFile(const char* pathnm);
  static void CloseDbgFile();
  static void CloseRptFile();

  static FILE* DbgFile()      { return _DbgFile; }
  static FILE* RptFile()      { return _RptFile; }

  static unsigned int DbgFlags () { return _DbgFlags;}
  static unsigned int RptFlags () { return _RptFlags;}

  static void AddDbgFlags(unsigned int flgs) { _DbgFlags |= flgs; }
  static void RemDbgFlags(unsigned int flgs) { _DbgFlags &= (~flgs); }
  static void ClrDbgFlags() { _DbgFlags = 0; }

  static void AddRptFlags(unsigned int flgs) { _RptFlags |= flgs; }
  static void RemRptFlags(unsigned int flgs) { _RptFlags &= (~flgs); }
  static void ClrRptFlags() { _RptFlags = 0; }

  static void GetDbgInterval_SecNanosec(unsigned long &s, unsigned long &n) {
    s = _DbgInterval_s;   n = _DbgInterval_n;
  }
  static void SetDbgInterval_msec(unsigned long msec) {
    _DbgInterval_s = msec / 1000;
    _DbgInterval_n = (msec % 1000) * 100000;
  }

  static void GetRptInterval_SecNanosec(unsigned long &s, unsigned long &n) {
    s = _RptInterval_s;   n = _RptInterval_n;
  }
  static void SetRptInterval_msec(unsigned long msec) {
    _RptInterval_s = msec / 1000;
    _RptInterval_n = (msec % 1000) * 100000;
  }

  // Define class similar to omniORB::logger, used for both debug log and report log

  class logger {
  public:
    logger(const char* prefix, FILE* file, FILE* alt_file, const char* flags, const char* srcfile = 0, int srcline = -1);
    ~logger();  // destructor flushes the message

    // str is public instance state
    RDIstrstream  str;

    void flush(CORBA::Boolean do_fflush = 0);
    // output prefix + s to appropriate FILE*; this can be used for another log msg
    void flush_wo_prefix(CORBA::Boolean do_fflush = 0);
    // same thing except the prefix is not output
    void write2FILE(FILE* outf, CORBA::Boolean do_fflush = 0);
    // write2FILE writes this's buffer to specified FILE* without clearing it
    void write2FILE_wo_prefix(FILE* outf, CORBA::Boolean do_fflush = 0);
    // same thing except the prefix is not output

  private:
    logger(const logger&);
    logger& operator=(const logger&);

    char*         _prefix_buf;
    FILE*         _file;
    FILE*         _alt_file;
  }; // class logger

private:
  // INTERNAL STATE (valid for lifetime of daemon)
  static _nfy_attr RDINotifServer*  _Server_i;
  static _nfy_attr AttN::Server_var _Server;

  static _nfy_attr unsigned long    _DbgInterval_s;
  static _nfy_attr unsigned long    _DbgInterval_n;
  static _nfy_attr unsigned long    _RptInterval_s;
  static _nfy_attr unsigned long    _RptInterval_n;
  static _nfy_attr unsigned long    _DbgFlags;
  static _nfy_attr unsigned long    _RptFlags;
  static _nfy_attr FILE*            _DbgFile;
  static _nfy_attr FILE*            _RptFile;

}; // class RDI

// Flags that control debug logging
// (set in RDI::_DbgFlags)
#define RDIDbgDaemonF        (1 <<  1)
#define RDIDbgFactF          (1 <<  2)
#define RDIDbgFiltF          (1 <<  3)
#define RDIDbgFAdminF        (1 <<  4)
#define RDIDbgChanF          (1 <<  5)
#define RDIDbgCAdmF          (1 <<  6)
#define RDIDbgSAdmF          (1 <<  7)
#define RDIDbgCPxyF          (1 <<  8)
#define RDIDbgSPxyF          (1 <<  9)
#define RDIDbgEvQF           (1 << 10)
#define RDIDbgRDIEventF      (1 << 11)
#define RDIDbgEvalF          (1 << 12)
#define RDIDbgCosCPxyF       (1 << 13)
#define RDIDbgCosSPxyF       (1 << 14)
#define RDIDbgNotifQoSF      (1 << 15)
#define RDIDbgAdminQoSF      (1 << 16)
#define RDIDbgNotifQueueF    (1 << 17)

// ...

// Flags that control reporting for admin purposes
// (set in RDI::_RptFlags)
#define RDIRptChanStatsF     (1 <<  1)
#define RDIRptQSizeStatsF    (1 <<  2)
#define RDIRptCnctdConsF     (1 <<  3)
#define RDIRptCnctdSupsF     (1 <<  4)
#define RDIRptCnctdFiltsF    (1 <<  5)
#define RDIRptUnCnctdFiltsF  (1 <<  6)
#define RDIRptRejectsF       (1 <<  7)
#define RDIRptDropsF         (1 <<  8)
#define RDIRptNotifQoSF      (1 <<  9)
#define RDIRptAdminQoSF      (1 << 10)
#define RDIRptServerQoSF     (1 << 11)
#define RDIRptInteractiveF   (1 << 12)
// ...


// Macros that test flags
#define RDIDbgDaemon        (RDI::DbgFlags() & RDIDbgDaemonF       )
#define RDIDbgFact          (RDI::DbgFlags() & RDIDbgFactF         )
#define RDIDbgFilt          (RDI::DbgFlags() & RDIDbgFiltF         )
#define RDIDbgChan          (RDI::DbgFlags() & RDIDbgChanF         )
#define RDIDbgCAdm          (RDI::DbgFlags() & RDIDbgCAdmF         )
#define RDIDbgSAdm          (RDI::DbgFlags() & RDIDbgSAdmF         )
#define RDIDbgCPxy          (RDI::DbgFlags() & RDIDbgCPxyF         )
#define RDIDbgSPxy          (RDI::DbgFlags() & RDIDbgSPxyF         )
#define RDIDbgEvQ           (RDI::DbgFlags() & RDIDbgEvQF          )
#define RDIDbgRDIEvent      (RDI::DbgFlags() & RDIDbgRDIEventF     )
#define RDIDbgFAdmin        (RDI::DbgFlags() &  RDIDbgFAdminF      )
#define RDIDbgEval          (RDI::DbgFlags() & RDIDbgEvalF         )
#define RDIDbgCosCPxy       (RDI::DbgFlags() & RDIDbgCosCPxyF      )
#define RDIDbgCosSPxy       (RDI::DbgFlags() & RDIDbgCosSPxyF      )
#define RDIDbgNotifQoS      (RDI::DbgFlags() & RDIDbgNotifQoSF     )
#define RDIDbgAdminQoS      (RDI::DbgFlags() & RDIDbgAdminQoSF     )
#define RDIDbgNotifQueue    (RDI::DbgFlags() & RDIDbgNotifQueueF   )

#define RDIDbgFactOrChan    (RDI::DbgFlags() & (RDIDbgFactF|RDIDbgChanF) )

#define RDIRptChanStats     (RDI::RptFlags() & RDIRptChanStatsF    )
#define RDIRptQSizeStats    (RDI::RptFlags() & RDIRptQSizeStatsF   )
#define RDIRptCnctdCons     (RDI::RptFlags() & RDIRptCnctdConsF    )
#define RDIRptCnctdSups     (RDI::RptFlags() & RDIRptCnctdSupsF    )
#define RDIRptCnctdFilts    (RDI::RptFlags() & RDIRptCnctdFiltsF   )
#define RDIRptUnCnctdFilts  (RDI::RptFlags() & RDIRptUnCnctdFiltsF )
#define RDIRptRejects       (RDI::RptFlags() & RDIRptRejectsF      )
#define RDIRptDrops         (RDI::RptFlags() & RDIRptDropsF        )
#define RDIRptNotifQoS      (RDI::RptFlags() & RDIRptNotifQoSF     )
#define RDIRptAdminQoS      (RDI::RptFlags() & RDIRptAdminQoSF     )
#define RDIRptServerQoS     (RDI::RptFlags() & RDIRptServerQoSF    )
#define RDIRptInteractive   (RDI::RptFlags() & RDIRptInteractiveF  )

// config file names for the flags
#define RDIDbgDaemon_nm        "DebugDaemon"
#define RDIDbgFact_nm          "DebugChannelFactory"
#define RDIDbgFilt_nm          "DebugFilter"
#define RDIDbgChan_nm          "DebugChannel"
#define RDIDbgCAdm_nm          "DebugConsumerAdmin" 
#define RDIDbgSAdm_nm          "DebugSupplireAdmin"
#define RDIDbgCPxy_nm          "DebugConsumerProxy"
#define RDIDbgSPxy_nm          "DebugSupplierProxy"
#define RDIDbgEvQ_nm           "DebugEventQueue"
#define RDIDbgRDIEvent_nm      "DebugRDIEvent"
#define RDIDbgFAdmin_nm        "DebugFilterAdmin"
#define RDIDbgEval_nm          "DebugFilterEval"
#define RDIDbgCosCPxy_nm       "DebugCosConsumerProxies"
#define RDIDbgCosSPxy_nm       "DebugCosSupplierProxies"
#define RDIDbgNotifQoS_nm      "DebugNotifQoS"
#define RDIDbgAdminQoS_nm      "DebugAdminQoS"
#define RDIDbgNotifQueue_nm    "DebugNotifQueue"

#define RDIRptChanStats_nm     "ReportChannelStats"
#define RDIRptQSizeStats_nm    "ReportQueueSizeStats"
#define RDIRptCnctdCons_nm     "ReportConnectedConsumers" 
#define RDIRptCnctdSups_nm     "ReportConnectedSuppliers"
#define RDIRptCnctdFilts_nm    "ReportConnectedFilters"
#define RDIRptUnCnctdFilts_nm  "ReportUnconnectedFilters"
#define RDIRptRejects_nm       "ReportEventRejections"
#define RDIRptDrops_nm         "ReportEventDrops"
#define RDIRptNotifQoS_nm      "ReportNotifQoS"
#define RDIRptAdminQoS_nm      "ReportAdminQoS"
#define RDIRptServerQoS_nm     "ReportServerQoS"
#define RDIRptInteractive_nm   "ReportInteractive"

#define MacroArg2String(s) #s

// Macros to declare a logger l
#define RDIDbgLogger(l, flags_nm) RDI::logger l("DBG", RDI::DbgFile(), 0, flags_nm, __FILE__, __LINE__)
#define RDIRptLogger(l, flags_nm) RDI::logger l("omniNotify", RDI::RptFile(), 0, flags_nm)
#define RDIDbgLogger1(l) RDIDbgLogger(l, "")
#define RDIRptLogger1(l) RDIRptLogger(l, "")
// Used before the other 2 have been set up, e.g., by notifd startup code.
// Also any time msg should always go to stdout/stderr; if corresponding
// log has been set up, output goes to both stdout/stderr and rpt/dbg log
#define RDIErrLogger(prefix, l) RDI::logger l(prefix, stderr, RDI::DbgFile(), "", __FILE__, __LINE__)
#define RDIOutLogger(prefix, l) RDI::logger l(prefix, stdout, RDI::RptFile(), "")

#ifndef NDEBUG
#define RDIDbgTst(flagtest) flagtest
#else
#define RDIDbgTst(flagtest) 0
#endif

#ifndef NREPORT
#define RDIRptTst(flagtest) flagtest
#else
#define RDIRptTst(flagtest) 0
#endif

#define RDIDbgLogIsFile ((RDI::DbgFile() != stdout) && (RDI::DbgFile() != stderr))
#define RDIRptLogIsFile ((RDI::RptFile() != stdout) && (RDI::RptFile() != stderr))

// these log if NDEBUG is not set and tst holds
#define RDIDbgLog(tst, tst_nm, stuff) do {if (RDIDbgTst(tst)) { RDIDbgLogger(l, tst_nm); l.str << stuff; }} while (0)
#define RDIRptLog(tst, tst_nm, stuff) do {if (RDIRptTst(tst)) { RDIRptLogger(l, tst_nm); l.str << stuff; }} while (0)
// these always log to debug/report logs, regardless of flags or NDEBUG
#define RDIDbgForceLog(stuff) do { RDIDbgLogger1(l); l.str << stuff; } while (0)
#define RDIRptForceLog(stuff) do { RDIRptLogger1(l); l.str << stuff; } while (0)
// these always log to stderr/stdout, regardless of flags or NDEBUG
#define RDIErrLog(stuff) do { RDIErrLogger("DBG", l); l.str << stuff; } while (0)
#define RDIOutLog(stuff) do { RDIOutLogger("omniNotify", l); l.str << stuff; } while (0)

// convenience macros
 
#define RDIDbgDaemonLog(stuff) RDIDbgLog(RDIDbgDaemon, RDIDbgDaemon_nm, stuff)
#define RDIDbgFactLog(stuff) RDIDbgLog(RDIDbgFact, RDIDbgFact_nm, stuff)
#define RDIDbgFiltLog(stuff) RDIDbgLog(RDIDbgFilt, RDIDbgFilt_nm, stuff)
#define RDIDbgChanLog(stuff) RDIDbgLog(RDIDbgChan, RDIDbgChan_nm, stuff)
#define RDIDbgCAdmLog(stuff) RDIDbgLog(RDIDbgCAdm, RDIDbgCAdm_nm, stuff)
#define RDIDbgSAdmLog(stuff) RDIDbgLog(RDIDbgSAdm, RDIDbgSAdm_nm, stuff)
#define RDIDbgCPxyLog(stuff) RDIDbgLog(RDIDbgCPxy, RDIDbgCPxy_nm, stuff)
#define RDIDbgSPxyLog(stuff) RDIDbgLog(RDIDbgSPxy, RDIDbgSPxy_nm, stuff)
#define RDIDbgEvQLog(stuff) RDIDbgLog(RDIDbgEvQ, RDIDbgEvQ_nm, stuff)
#define RDIDbgRDIEventLog(stuff) RDIDbgLog(RDIDbgRDIEvent, RDIDbgRDIEvent_nm, stuff)
#define RDIDbgFAdminLog(stuff) RDIDbgLog(RDIDbgFAdmin, RDIDbgFAdmin_nm, stuff)
#define RDIDbgEvalLog(stuff) RDIDbgLog(RDIDbgEval, RDIDbgEval_nm, stuff)
#define RDIDbgCosCPxyLog(stuff) RDIDbgLog(RDIDbgCosCPxy, RDIDbgCosCPxy_nm, stuff)
#define RDIDbgCosSPxyLog(stuff) RDIDbgLog(RDIDbgCosSPxy, RDIDbgCosSPxy_nm, stuff)
#define RDIDbgNotifQoSLog(stuff) RDIDbgLog(RDIDbgNotifQoS, RDIDbgNotifQoS_nm, stuff)
#define RDIDbgAdminQoSLog(stuff) RDIDbgLog(RDIDbgAdminQoS, RDIDbgAdminQoS_nm, stuff)
#define RDIDbgNotifQueueLog(stuff) RDIDbgLog(RDIDbgNotifQueue, RDIDbgNotifQueue_nm, stuff)

#define RDIRptChanStatsLog(stuff) RDIRptLog(RDIRptChanStats, RDIRptChanStats_nm, stuff)
#define RDIRptQSizeStatsLog(stuff) RDIRptLog(RDIRptQSizeStats, RDIRptQSizeStats_nm, stuff)
#define RDIRptCnctdConsLog(stuff) RDIRptLog(RDIRptCnctdCons, RDIRptCnctdCons_nm, stuff)
#define RDIRptCnctdSupsLog(stuff) RDIRptLog(RDIRptCnctdSups, RDIRptCnctdSups_nm, stuff)
#define RDIRptCnctdFiltsLog(stuff) RDIRptLog(RDIRptCnctdFilts, RDIRptCnctdFilts_nm, stuff)
#define RDIRptUnCnctdFiltsLog(stuff) RDIRptLog(RDIRptUnCnctdFilts, RDIRptUnCnctdFilts_nm, stuff)
#define RDIRptRejectsLog(stuff) RDIRptLog(RDIRptRejects, RDIRptRejects_nm, stuff)
#define RDIRptDropsLog(stuff) RDIRptLog(RDIRptDrops, RDIRptDrops_nm, stuff)
#define RDIRptNotifQoSLog(stuff) RDIRptLog(RDIRptNotifQoS, RDIRptNotifQoS_nm, stuff)
#define RDIRptAdminQoSLog(stuff) RDIRptLog(RDIRptAdminQoS, RDIRptAdminQoS_nm, stuff)
#define RDIRptServerQoSLog(stuff) RDIRptLog(RDIRptServerQoS, RDIRptServerQoS_nm, stuff)
#define RDIRptInteractiveLog(stuff) RDIRptLog(RDIRptInteractive, RDIRptInteractive_nm, stuff)

/////////////////////////////////////////////////////////////////////////////////
// Additional macros
//
// RDI_DBG_PARAM(x)
//	Wrap a parameter that is going to be used only when NDEBUG
//	is not defined (just a trick to avoid compiler warnings)
// RDI_Fatal(str)
//	Fatal error:  the message is logged and program is aborted
// RDI_Assert(x, str)
//	If 'x' is false,  'str' gets logged and program is aborted
// RDI_AssertAllocThrowMaybe(x, str)
//	If 'x' is NULL,  'str' gets logged and
//       CORBA::NO_MEMORY(0, CORBA::COMPLETED_MAYBE) is thrown
// RDI_AssertAllocThrowNo(x, str)
//	If 'x' is NULL,  'str' gets logged and
//       CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO) is thrown
////////////////////////////////////////////////////////////////////////////////

#define __RDI_NULL_STMT do { } while(0)

#ifndef NDEBUG
#  define RDI_DBG_PARAM(x) x
#else
#  define RDI_DBG_PARAM(x)
#endif

#define RDI_Fatal(str) \
  do { RDIDbgForceLog("** Fatal Error **: " << str); abort(); } while (0)

#ifndef NDEBUG
#define RDI_Assert(x, str) \
    do { if (!(x)) RDI_Fatal("assert failed " << MacroArg2String(x) << " " << str); } while (0) 
#else
#define RDI_Assert(x, str) __RDI_NULL_STMT
#endif

#define RDI_AssertAllocThrowMaybe(x, str) \
    do { if (!(x)) { RDIDbgForceLog(str); throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_MAYBE); } } while (0)

#define RDI_AssertAllocThrowNo(x, str) \
    do { if (!(x)) { RDIDbgForceLog(str); throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO); } } while (0)


#endif /*  __RDI_H__  */
