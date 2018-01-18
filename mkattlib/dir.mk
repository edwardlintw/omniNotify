include $(MAKEFILE_INC_DIR)libdefs.mk

ifdef Win32Platform
# Win32 needs to link its shared libs against the COS libs to prevent
# a million and one undefined symbol errors. We include cos.mk to get
# the library name.
include $(BASE_OMNI_TREE)/mk/cos.mk
endif

# Uncomment the next line to build BOA versions of the ATT library
# BUILD_BOA_ATT_LIB = 1

# Look for .idl files in <top>/idl/COS and <top>/src/services/omniNotify/idl
vpath %.idl $(IMPORT_TREES:%=%/idl/COS) $(IMPORT_TREES:%=%/src/services/omniNotify/idl) $(DATADIR)/idl/omniORB/COS

DIR_IDLFLAGS += -I. $(patsubst %,-I%/idl/COS,$(IMPORT_TREES))
DIR_IDLFLAGS += -I. $(patsubst %,-I%/src/services/omniNotify/idl,$(IMPORT_TREES))
DIR_IDLFLAGS += -I $(DATADIR)/idl/omniORB/COS

# Look for .hh files in <top>/include/COS
IMPORT_CPPFLAGS += $(patsubst %,-I%/include/COS,$(IMPORT_TREES))

ATTN_SKLIB_NAME    = AttNotification
ATTN_DYNSKLIB_NAME = AttNotificationDynamic

ATTN_SK_OBJS = $(ATTN_INTERFACES:%=%SK.o)
ATTN_SK_SRCS = $(ATTN_INTERFACES:%=%SK.cc)

ATTN_DYNSK_OBJS = $(ATTN_INTERFACES:%=%DynSK.o)
ATTN_DYNSK_SRCS = $(ATTN_INTERFACES:%=%DynSK.cc)

CXXSRCS = $(ATTN_DYNSK_SRCS) $(ATTN_SK_SRCS) 

all:: mkstatic mkshared

export:: mkstatic mkshared

export:: $(ATTN_INTERFACES:%=%.hh) ATTN_sysdep.h
	@(for i in $^; do \
            file="$$i"; \
            dir="$(EXPORT_TREE)/$(INCDIR)/COS"; \
		$(ExportFileToDir) \
          done )

ifdef INSTALLTARGET
install:: $(ATTN_INTERFACES:%=%.hh) ATTN_sysdep.h
	@(dir="$(INSTALLINCDIR)/COS"; \
          for file in $^; do \
            $(ExportFileToDir) \
          done )
endif

veryclean::
	$(RM) $(ATTN_INTERFACES:%=%SK.cc) $(ATTN_INTERFACES:%=%DynSK.cc) \
              $(ATTN_INTERFACES:%=%.hh)


##############################################################################
# Build Static library
##############################################################################

version  := $(word 1,$(subst ., ,$(OMNIORB_VERSION)))

ifndef DontBuildStaticLibrary

sk = static/$(patsubst %,$(LibNoDebugPattern),$(ATTN_SKLIB_NAME)$(version))
dynsk = static/$(patsubst %,$(LibNoDebugPattern),$(ATTN_DYNSKLIB_NAME)$(version))
MDFLAGS += -p static/

mkstatic::
	@(dir=static; $(CreateDir))

mkstatic:: $(sk) $(dynsk) 

$(sk): $(patsubst %, static/%, $(ATTN_SK_OBJS))
	@$(StaticLinkLibrary)

$(dynsk): $(patsubst %, static/%, $(ATTN_DYNSK_OBJS))
	@$(StaticLinkLibrary)

export:: $(sk)
	@$(ExportLibrary)

export:: $(dynsk)
	@$(ExportLibrary)

clean::
	$(RM) static/*.o
	$(RM) $(sk) $(dynsk)

else

mkstatic::

endif

##############################################################################
# Build Shared library
##############################################################################
ifdef BuildSharedLibrary

sharedversion = $(OMNIORB_VERSION)

sknamespec    = $(subst ., ,$(ATTN_SKLIB_NAME).$(sharedversion))
skshared      = shared/$(shell $(SharedLibraryFullName) $(sknamespec))

dynsknamespec = $(subst ., ,$(ATTN_DYNSKLIB_NAME).$(sharedversion))
dynskshared   = shared/$(shell $(SharedLibraryFullName) $(dynsknamespec))

MDFLAGS += -p shared/

ifdef Win32Platform
# in case of Win32 lossage:
imps := $(patsubst $(DLLDebugSearchPattern),$(DLLNoDebugSearchPattern), \
         $(OMNIORB_LIB) $(COS_LIB))
else
imps := $(OMNIORB_LIB)
endif


mkshared::
	@(dir=shared; $(CreateDir))

mkshared:: $(skshared) $(dynskshared) 

$(skshared): $(patsubst %, shared/%, $(ATTN_SK_OBJS))
	@(namespec="$(sknamespec)" extralibs="$(imps)"; \
         $(MakeCXXSharedLibrary))

$(dynskshared): $(patsubst %, shared/%, $(ATTN_DYNSK_OBJS))
	@(namespec="$(dynsknamespec)" extralibs="$(skshared) $(imps)"; \
         $(MakeCXXSharedLibrary))

export:: $(skshared)
	@(namespec="$(sknamespec)"; \
         $(ExportSharedLibrary))

export:: $(dynskshared)
	@(namespec="$(dynsknamespec)"; \
         $(ExportSharedLibrary))

ifdef INSTALLTARGET
install:: $(skshared)
	@(namespec="$(sknamespec)"; \
         $(InstallSharedLibrary))

install:: $(dynskshared)
	@(namespec="$(dynsknamespec)"; \
         $(InstallSharedLibrary))
endif

clean::
	$(RM) shared/*.o
	(dir=shared; $(CleanSharedLibrary))

else

mkshared::

endif

##############################################################################
# Build debug libraries for Win32
##############################################################################
ifdef Win32Platform

all:: mkstaticdbug mkshareddbug

export:: mkstaticdbug mkshareddbug

#####################################################
#      Static debug libraries
#####################################################
dbugversion = $(word 1,$(subst ., ,$(OMNIORB_VERSION)))

skdbug = debug/$(patsubst %,$(LibDebugPattern),$(ATTN_SKLIB_NAME)$(dbugversion))
dynskdbug = debug/$(patsubst %,$(LibDebugPattern),$(ATTN_DYNSKLIB_NAME)$(dbugversion))

MDFLAGS += -p debug/

mkstaticdbug::
	@(dir=debug; $(CreateDir))

mkstaticdbug:: $(skdbug) $(dynskdbug)

$(skdbug): $(patsubst %, debug/%, $(ATTN_SK_OBJS))
	@$(StaticLinkLibrary)

$(dynskdbug): $(patsubst %, debug/%, $(ATTN_DYNSK_OBJS))
	@$(StaticLinkLibrary)

export:: $(skdbug)
	@$(ExportLibrary)

export:: $(dynskdbug)
	@$(ExportLibrary)

clean::
	$(RM) debug/*.o
	$(RM) $(skdbug) $(dynskdbug)

#####################################################
#      DLL debug libraries
#####################################################
shareddbugversion = $(OMNIORB_VERSION)

sknamespec      = $(subst ., ,$(ATTN_SKLIB_NAME).$(shareddbugversion))
skshareddbug    = shareddebug/$(shell $(SharedLibraryDebugFullName) $(sknamespec))

dynsknamespec   = $(subst ., ,$(ATTN_DYNSKLIB_NAME).$(shareddbugversion))
dynskshareddbug = shareddebug/$(shell $(SharedLibraryDebugFullName) $(dynsknamespec))

dbugimps  := $(patsubst $(DLLNoDebugSearchPattern),$(DLLDebugSearchPattern), \
               $(OMNIORB_LIB) $(COS_LIB))

MDFLAGS += -p shareddebug/

mkshareddbug::
	@(dir=shareddebug; $(CreateDir))

mkshareddbug:: $(skshareddbug) $(dynskshareddbug) 

$(skshareddbug): $(patsubst %, shareddebug/%, $(ATTN_SK_OBJS))
	(namespec="$(sknamespec)" debug=1 extralibs="$(dbugimps)"; \
         $(MakeCXXSharedLibrary))

$(dynskshareddbug): $(patsubst %, shareddebug/%, $(ATTN_DYNSK_OBJS))
	@(namespec="$(dynsknamespec)" debug=1 extralibs="$(skshareddbug) $(dbugimps)"; \
         $(MakeCXXSharedLibrary))

export:: $(skshareddbug)
	@(namespec="$(sknamespec)" debug=1; \
         $(ExportSharedLibrary))

export:: $(dynskshareddbug)
	@(namespec="$(dynsknamespec)" debug=1; \
         $(ExportSharedLibrary))

clean::
	$(RM) shareddebug/*.o
	@(dir=shareddebug; $(CleanSharedLibrary))

endif

##############################################################################
# Build Subdirectories
##############################################################################
ifdef BUILD_BOA_ATT_LIB
SUBDIRS = mkBOAlib

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)

clean::
	@$(MakeSubdirs)

veryclean::
	@$(MakeSubdirs)
endif
