# vim:set ts=2 sw=2 sts=2 noet:
ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir)/librabbitmq

lib_LTLIBRARIES = librabbitmq/librabbitmq.la

librabbitmq_librabbitmq_la_CFLAGS = \
	$(SSL_CFLAGS) \
	$(AM_CFLAGS)

librabbitmq_librabbitmq_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	$(NO_UNDEFINED) \
	$(SSL_LIBS)

include_HEADERS = \
	librabbitmq/amqp.h \
	librabbitmq/amqp_framing.h \
	librabbitmq/amqp_tcp_socket.h

if SSL
include_HEADERS += librabbitmq/amqp_ssl_socket.h
endif

librabbitmq_librabbitmq_la_SOURCES = \
	librabbitmq/amqp_api.c \
	librabbitmq/amqp_connection.c \
	librabbitmq/amqp_consumer.c \
	librabbitmq/amqp_framing.c \
	librabbitmq/amqp_mem.c \
	librabbitmq/amqp_private.h \
	librabbitmq/amqp_socket.c \
	librabbitmq/amqp_socket.h \
	librabbitmq/amqp_table.c \
	librabbitmq/amqp_table.h \
	librabbitmq/amqp_tcp_socket.c \
	librabbitmq/amqp_tcp_socket.h \
	librabbitmq/amqp_time.c \
	librabbitmq/amqp_time.h \
	librabbitmq/amqp_url.c

if SSL_OPENSSL
librabbitmq_librabbitmq_la_SOURCES += \
  librabbitmq/amqp_hostcheck.c \
  librabbitmq/amqp_hostcheck.h \
  librabbitmq/amqp_openssl.c \
  librabbitmq/amqp_openssl_hostname_validation.c \
  librabbitmq/amqp_openssl_hostname_validation.h
if OS_APPLE
librabbitmq_librabbitmq_la_CFLAGS += -Wno-deprecated-declarations
endif
endif

if OS_UNIX
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/unix/threads.h
librabbitmq_librabbitmq_la_CFLAGS += -I$(top_srcdir)/librabbitmq/unix
endif

if OS_WIN32
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/win32/threads.h
librabbitmq_librabbitmq_la_CFLAGS += \
	-I$(top_srcdir)/librabbitmq/win32 \
	-I$(top_srcdir)/librabbitmq/win32/msinttypes
endif


check_PROGRAMS = \
	tests/test_tables \
	tests/test_parse_url \
	tests/test_hostcheck \
	tests/test_status_enum \
	tests/test_sasl_mechanism \
	tests/test_merge_capabilities

TESTS = $(check_PROGRAMS)

tests_test_tables_SOURCES = tests/test_tables.c
tests_test_tables_LDADD = librabbitmq/librabbitmq.a

tests_test_parse_url_SOURCES = tests/test_parse_url.c
tests_test_parse_url_LDADD = librabbitmq/librabbitmq.a

tests_test_hostcheck_SOURCES = \
  tests/test_hostcheck.c \
	librabbitmq/amqp_hostcheck.c

tests_test_status_enum_SOURCES = tests/test_status_enum.c
tests_test_status_enum_LDADD = librabbitmq/librabbitmq.a

tests_test_sasl_mechanism_SOURCES = tests/test_sasl_mechanism.c
tests_test_sasl_mechanism_LDADD = librabbitmq/librabbitmq.a

tests_test_merge_capabilities_SOURCES = tests/test_merge_capabilities.c
tests_test_merge_capabilities_LDADD = librabbitmq/librabbitmq.a

noinst_LTLIBRARIES =

if EXAMPLES
noinst_LTLIBRARIES += examples/libutils.la

examples_libutils_la_SOURCES = \
	examples/utils.c \
	examples/utils.h
examples_libutils_la_CFLAGS = $(AM_CFLAGS)

if OS_UNIX
examples_libutils_la_SOURCES += examples/unix/platform_utils.c
endif

if OS_WIN32
examples_libutils_la_SOURCES += examples/win32/platform_utils.c
examples_libutils_la_CFLAGS += -I$(top_srcdir)/tools/win32/msinttypes
endif

noinst_PROGRAMS = \
	examples/amqp_bind \
	examples/amqp_consumer \
	examples/amqp_connect_timeout \
	examples/amqp_exchange_declare \
	examples/amqp_listen \
	examples/amqp_listenq \
	examples/amqp_producer \
	examples/amqp_rpc_sendstring_client \
	examples/amqp_sendstring \
	examples/amqp_unbind

examples_amqp_sendstring_SOURCES = examples/amqp_sendstring.c
examples_amqp_sendstring_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_exchange_declare_SOURCES = examples/amqp_exchange_declare.c
examples_amqp_exchange_declare_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_listen_SOURCES = examples/amqp_listen.c
examples_amqp_listen_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_producer_SOURCES = examples/amqp_producer.c
examples_amqp_producer_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_consumer_SOURCES = examples/amqp_consumer.c
examples_amqp_consumer_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_connect_timeout_SOURCES = examples/amqp_connect_timeout.c
examples_amqp_connect_timeout_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_unbind_SOURCES = examples/amqp_unbind.c
examples_amqp_unbind_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_bind_SOURCES = examples/amqp_bind.c
examples_amqp_bind_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_listenq_SOURCES = examples/amqp_listenq.c
examples_amqp_listenq_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqp_rpc_sendstring_client_SOURCES = \
	examples/amqp_rpc_sendstring_client.c
examples_amqp_rpc_sendstring_client_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

if SSL
noinst_PROGRAMS += \
	examples/amqps_bind \
	examples/amqps_connect_timeout \
	examples/amqps_consumer \
	examples/amqps_exchange_declare \
	examples/amqps_listen \
	examples/amqps_listenq \
	examples/amqps_producer \
	examples/amqps_sendstring \
	examples/amqps_unbind

examples_amqps_bind_SOURCES = examples/amqps_bind.c
examples_amqps_bind_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqps_connect_timeout_SOURCES = examples/amqps_connect_timeout.c
examples_amqps_connect_timeout_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqps_consumer_SOURCES = examples/amqps_consumer.c
examples_amqps_consumer_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqps_exchange_declare_SOURCES = examples/amqps_exchange_declare.c
examples_amqps_exchange_declare_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqps_listen_SOURCES = examples/amqps_listen.c
examples_amqps_listen_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqps_listenq_SOURCES = examples/amqps_listenq.c
examples_amqps_listenq_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqps_producer_SOURCES = examples/amqps_producer.c
examples_amqps_producer_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqps_sendstring_SOURCES = examples/amqps_sendstring.c
examples_amqps_sendstring_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la

examples_amqps_unbind_SOURCES = examples/amqps_unbind.c
examples_amqps_unbind_LDADD = \
	examples/libutils.la \
	librabbitmq/librabbitmq.la
endif
endif

if TOOLS
noinst_LTLIBRARIES += tools/libcommon.la

tools_libcommon_la_SOURCES = \
	tools/common.c \
	tools/common.h
tools_libcommon_la_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(top_srcdir)/tools

tools_platform_CFLAGS = $(AM_CFLAGS)

if OS_UNIX
tools_libcommon_la_SOURCES += \
	tools/unix/process.c \
	tools/unix/process.h
tools_platform_CFLAGS += -I$(top_srcdir)/tools/unix
endif

if OS_WIN32
tools_libcommon_la_SOURCES += \
	tools/win32/compat.c \
	tools/win32/compat.h \
	tools/win32/process.c \
	tools/win32/process.h
tools_platform_CFLAGS += \
	-I$(top_srcdir)/tools/win32 \
	-I$(top_srcdir)/tools/win32/msinttypes
endif

bin_PROGRAMS = \
	tools/amqp-consume \
	tools/amqp-declare-queue \
	tools/amqp-delete-queue \
	tools/amqp-get \
	tools/amqp-publish

tools_amqp_publish_SOURCES = tools/publish.c
tools_amqp_publish_CFLAGS = \
	$(AM_CFLAGS) \
	$(POPT_CFLAGS) \
	$(tools_platform_CFLAGS) \
	-I$(top_srcdir)/tools
tools_amqp_publish_LDADD = \
	$(POPT_LIBS) \
	tools/libcommon.la \
	librabbitmq/librabbitmq.la

tools_amqp_get_SOURCES = tools/get.c
tools_amqp_get_CFLAGS = \
	$(AM_CFLAGS) \
	$(POPT_CFLAGS) \
	$(tools_platform_CFLAGS) \
	-I$(top_srcdir)/tools
tools_amqp_get_LDADD = \
	$(POPT_LIBS) \
	tools/libcommon.la \
	librabbitmq/librabbitmq.la

tools_amqp_consume_SOURCES = tools/consume.c
tools_amqp_consume_CFLAGS = \
	$(AM_CFLAGS) \
	$(POPT_CFLAGS) \
	$(tools_platform_CFLAGS) \
	-I$(top_srcdir)/tools
tools_amqp_consume_LDADD = \
	$(POPT_LIBS) \
	tools/libcommon.la \
	librabbitmq/librabbitmq.la

tools_amqp_declare_queue_SOURCES = tools/declare_queue.c
tools_amqp_declare_queue_CFLAGS = \
	$(AM_CFLAGS) \
	$(POPT_CFLAGS) \
	$(tools_platform_CFLAGS) \
	-I$(top_srcdir)/tools
tools_amqp_declare_queue_LDADD = \
	$(POPT_LIBS) \
	tools/libcommon.la \
	librabbitmq/librabbitmq.la

tools_amqp_delete_queue_SOURCES = tools/delete_queue.c
tools_amqp_delete_queue_CFLAGS = \
	$(AM_CFLAGS) \
	$(POPT_CFLAGS) \
	$(tools_platform_CFLAGS) \
	-I$(top_srcdir)/tools
tools_amqp_delete_queue_LDADD = \
	$(POPT_LIBS) \
	tools/libcommon.la \
	librabbitmq/librabbitmq.la

if DOCS
man_MANS = \
	$(top_builddir)/tools/doc/amqp-publish.1 \
	$(top_builddir)/tools/doc/amqp-consume.1 \
	$(top_builddir)/tools/doc/amqp-get.1 \
	$(top_builddir)/tools/doc/amqp-declare-queue.1 \
	$(top_builddir)/tools/doc/amqp-delete-queue.1 \
	$(top_builddir)/tools/doc/librabbitmq-tools.7

# xmlto's --searchpath doesn't get passed through to xmllint, so we disable
# xmllint validation with --skip-validation for the benefit of build/source
# separation as required by distcheck, debian packaging etc.
XMLTO_FLAGS = \
	--skip-validation \
	--searchpath $(abs_top_builddir)/tools/doc

%.1: %.xml $(top_builddir)/tools/doc/man-date.ent
	$(AM_V_GEN)$(XMLTO) $(XMLTO_FLAGS) -o $(top_builddir)/tools/doc man $<

%.7: %.xml $(top_builddir)/tools/doc/man-date.ent
	$(AM_V_GEN)$(XMLTO) $(XMLTO_FLAGS) -o $(top_builddir)/tools/doc man $<

$(top_builddir)/tools/doc/man-date.ent:
	$(AM_V_at)$(MKDIR_P) $(top_builddir)/tools/doc
	$(AM_V_GEN)date +'%Y-%m-%d' > $@
endif # DOCS
endif # TOOLS

EXTRA_DIST = \
	AUTHORS \
	CMakeLists.txt \
	CONTRIBUTING.md \
	ChangeLog.md \
	LICENSE-MIT \
	README-win32.md \
	README.md \
	README.vms \
	THANKS \
	TODO \
	cmake \
	codegen \
	descrip.mms \
	docs \
	examples/CMakeLists.txt \
	examples/descrip.mms \
	librabbitmq/CMakeLists.txt \
	librabbitmq/codegen.py \
	librabbitmq/descrip.mms \
	librabbitmq/win32 \
	tests/CMakeLists.txt \
	tests/test_tables.expected \
	tests/win32 \
	tools/CMakeLists.txt \
	tools/doc/amqp-consume.xml \
	tools/doc/amqp-declare-queue.xml \
	tools/doc/amqp-delete-queue.xml \
	tools/doc/amqp-get.xml \
	tools/doc/amqp-publish.xml \
	tools/doc/librabbitmq-tools.xml \
	vms

MOSTLYCLEANFILES = \
	$(man_MANS) \
	$(top_builddir)/tools/doc/man-date.ent

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = librabbitmq.pc

clean-local:
	-$(MAKE) -C codegen clean

distclean-local:
	-$(MAKE) -C codegen distclean
