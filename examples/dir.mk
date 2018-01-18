include $(MAKEFILE_INC_DIR)../DEBUG.mk

# Make sure these examples use POA
ifdef CosUsesBoa
undef CosUsesBoa
endif

include $(MAKEFILE_INC_DIR)../trilib.mk

ifdef HPUX
# XXX Why is HPUX linker choking on COSDynamic ?
EXAMPLE_LIB = $(COS_LIB_NODYN) $(CORBA_LIB)
EXAMPLE_LIB_DEPEND = $(COS_LIB_NODYN_DEPEND) $(CORBA_LIB_DEPEND)
else
EXAMPLE_LIB = $(COS_LIB) $(CORBA_LIB)
EXAMPLE_LIB_DEPEND = $(COS_LIB_DEPEND) $(CORBA_LIB_DEPEND)
endif

DIR_CPPFLAGS = $(CORBA_CPPFLAGS)
DIR_CPPFLAGS += $(COS_CPPFLAGS)
DIR_CPPFLAGS += -DUSE_stub_in_nt_dll

# add services/include/omniNotify
IMPORT_CPPFLAGS += $(patsubst %,-I%/src/services/include/omniNotify,$(IMPORT_TREES))

CXXSRCS = \
  ndadmin.cc \
  sample_functions.cc \
  sample_clients.cc \
  legacy_clients.cc \
  demo_add_filter.cc \
  demo_offer_change.cc \
  demo_subscription_change.cc \
  any_pull_consumer.cc \
  any_pull_supplier.cc \
  any_push_consumer.cc \
  any_push_supplier.cc \
  batch_pull_consumer.cc \
  batch_pull_supplier.cc \
  batch_push_consumer.cc \
  batch_push_supplier.cc \
  legacy_pull_consumer.cc \
  legacy_pull_supplier.cc \
  legacy_push_consumer.cc \
  legacy_push_supplier.cc \
  struct_pull_consumer.cc \
  struct_pull_supplier.cc \
  struct_push_consumer.cc \
  struct_push_supplier.cc \
  s512_push_consumer.cc \
  s512_push_supplier.cc \
  ten_any_pull_consumers.cc \
  some_notify_clients.cc \
  all_cosnotify_clients.cc

CXXOBJS = $(CXXSRCS:.cc=.o)

ndadmin = $(patsubst %,$(BinPattern),ndadmin)

ten_any_pull_consumers = $(patsubst %,$(BinPattern),ten_any_pull_consumers)
some_notify_clients = $(patsubst %,$(BinPattern),some_notify_clients)
all_cosnotify_clients = $(patsubst %,$(BinPattern),all_cosnotify_clients)

demo_add_filter = $(patsubst %,$(BinPattern),demo_add_filter)
demo_offer_change = $(patsubst %,$(BinPattern),demo_offer_change)
demo_subscription_change = $(patsubst %,$(BinPattern),demo_subscription_change)

any_pull_consumer    = $(patsubst %,$(BinPattern),any_pull_consumer)
any_pull_supplier    = $(patsubst %,$(BinPattern),any_pull_supplier)
any_push_consumer    = $(patsubst %,$(BinPattern),any_push_consumer)
any_push_supplier    = $(patsubst %,$(BinPattern),any_push_supplier)
batch_pull_consumer  = $(patsubst %,$(BinPattern),batch_pull_consumer)
batch_pull_supplier  = $(patsubst %,$(BinPattern),batch_pull_supplier)
batch_push_consumer  = $(patsubst %,$(BinPattern),batch_push_consumer)
batch_push_supplier  = $(patsubst %,$(BinPattern),batch_push_supplier)
legacy_pull_consumer = $(patsubst %,$(BinPattern),legacy_pull_consumer)
legacy_pull_supplier = $(patsubst %,$(BinPattern),legacy_pull_supplier)
legacy_push_consumer = $(patsubst %,$(BinPattern),legacy_push_consumer)
legacy_push_supplier = $(patsubst %,$(BinPattern),legacy_push_supplier)
struct_pull_consumer = $(patsubst %,$(BinPattern),struct_pull_consumer)
struct_pull_supplier = $(patsubst %,$(BinPattern),struct_pull_supplier)
struct_push_consumer = $(patsubst %,$(BinPattern),struct_push_consumer)
struct_push_supplier = $(patsubst %,$(BinPattern),struct_push_supplier)
s512_push_consumer   = $(patsubst %,$(BinPattern),s512_push_consumer)
s512_push_supplier   = $(patsubst %,$(BinPattern),s512_push_supplier)

ALL_TARGETS = \
  $(ndadmin)  \
  $(ten_any_pull_consumers)  \
  $(some_notify_clients)  \
  $(all_cosnotify_clients)  \
  $(demo_add_filter)  \
  $(demo_offer_change)  \
  $(demo_subscription_change)  \
  $(any_pull_consumer)   \
  $(any_pull_supplier)   \
  $(any_push_consumer)   \
  $(any_push_supplier)   \
  $(batch_pull_consumer) \
  $(batch_pull_supplier) \
  $(batch_push_consumer) \
  $(batch_push_supplier) \
  $(legacy_pull_consumer) \
  $(legacy_pull_supplier) \
  $(legacy_push_consumer) \
  $(legacy_push_supplier) \
  $(struct_pull_consumer) \
  $(struct_pull_supplier) \
  $(struct_push_consumer) \
  $(struct_push_supplier) \
  $(s512_push_consumer) \
  $(s512_push_supplier)

TARGETS = $(ALL_TARGETS)

all:: $(TARGETS)

export:: $(TARGETS)
	@(module="omninfyexamples"; $(ExportExecutable))

ifdef INSTALLTARGET
install_examples:: $(TARGETS)
	@$(InstallExecutable)

veryclean::
	@(dir=$(INSTALLBINDIR); \
	 for file in $(TARGETS); do \
		base=`basename $$file`; \
		echo $(RM) $$dir/$$base; \
		$(RM) $$dir/$$base; \
		done;)
else
veryclean::
	@(dir=$(EXPORT_TREE)/$(BINDIR); \
	 for file in $(TARGETS); do \
		base=`basename $$file`; \
		echo $(RM) $$dir/$$base; \
		$(RM) $$dir/$$base; \
		done;)

endif

clean::
	$(RM) $(TARGETS) *.d

$(ndadmin) : ndadmin.o sample_functions.o $(BOTH_LIB_DEPEND)
	@(libs="$(BOTH_LIB)"; $(CXXExecutable))

$(ten_any_pull_consumers) : ten_any_pull_consumers.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(some_notify_clients) : some_notify_clients.o sample_clients.o legacy_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(all_cosnotify_clients) : all_cosnotify_clients.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(demo_add_filter) : demo_add_filter.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(demo_offer_change) : demo_offer_change.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(demo_subscription_change) : demo_subscription_change.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(any_pull_consumer) : any_pull_consumer.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(any_pull_supplier) : any_pull_supplier.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(any_push_consumer) : any_push_consumer.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(any_push_supplier) : any_push_supplier.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(batch_pull_consumer) : batch_pull_consumer.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(batch_pull_supplier) : batch_pull_supplier.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(batch_push_consumer) : batch_push_consumer.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(batch_push_supplier) : batch_push_supplier.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(struct_pull_consumer) : struct_pull_consumer.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(struct_pull_supplier) : struct_pull_supplier.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(struct_push_consumer) : struct_push_consumer.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(struct_push_supplier) : struct_push_supplier.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(s512_push_consumer) : s512_push_consumer.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(s512_push_supplier) : s512_push_supplier.o sample_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(legacy_pull_consumer) : legacy_pull_consumer.o legacy_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(legacy_pull_supplier) : legacy_pull_supplier.o legacy_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(legacy_push_consumer) : legacy_push_consumer.o legacy_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))

$(legacy_push_supplier) : legacy_push_supplier.o legacy_clients.o sample_functions.o $(EXAMPLE_LIB_DEPEND)
	@(libs="$(EXAMPLE_LIB)"; $(CXXExecutable))
