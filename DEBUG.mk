# This file controls CXXDEBUGFLAGS for the library, daemon, and examples
# (all 3 dir.mk files include this file directly or indirectly)

# There are three things you may wish to change:

# (1) Uncomment the following to enable use of the debug log.
#EnableDebugLog = 1

# (2) Uncomment the following to disable object garbage collection
#DisableObjGC = 1

# (3) Uncomment the following to compile with extended debugging info (-g compile option)
#     Only works for Unix platforms.
#EnableDashG = 1

##########################################
# Do not modify anything below this line #
##########################################

ifdef EnableDebugLog
RDI_NO_DEBUG_FLAG = 
else
RDI_NO_DEBUG_FLAG = -DNDEBUG
endif

ifdef DisableObjGC
RDI_NO_OBJ_GC_FLAG = -DNO_OBJ_GC
else
RDI_NO_OBJ_GC_FLAG =
endif

ifeq ($(platform),hppa_hpux_11.00)
RDI_OPT_FLAG = +O1
else
RDI_OPT_FLAG = -O2
endif

ifdef EnableDashG
ifdef UnixPlatform
RDI_OPT_FLAG = -g
endif
endif

CXXDEBUGFLAGS = $(RDI_OPT_FLAG) $(RDI_NO_DEBUG_FLAG) $(RDI_NO_OBJ_GC_FLAG)

ifdef TW_DEBUG
  OMNITHREAD_CPPFLAGS += -DTW_DEBUG
endif
