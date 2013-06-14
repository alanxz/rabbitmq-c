# vim:set ts=2 sw=2 sts=2 noet:
ACLOCAL_AMFLAGS = -I m4

if REGENERATE_AMQP_FRAMING
amqp_framing_dir = $(top_builddir)/librabbitmq/gen
AM_CFLAGS = -I$(amqp_framing_dir) -I$(top_srcdir)/librabbitmq
else
AM_CFLAGS = -I$(top_srcdir)/librabbitmq
endif #REGENERATE_AMQP_FRAMING


lib_LTLIBRARIES = librabbitmq/librabbitmq.la

librabbitmq_librabbitmq_la_SOURCES = \
	librabbitmq/amqp-socket.h \
	librabbitmq/amqp_tcp_socket.c \
	librabbitmq/amqp_api.c \
	librabbitmq/amqp_connection.c \
	librabbitmq/amqp_mem.c \
	librabbitmq/amqp_private.h \
	librabbitmq/amqp_socket.c \
	librabbitmq/amqp_table.c \
	librabbitmq/amqp_url.c \
	librabbitmq/amqp_timer.h \
	librabbitmq/amqp_timer.c

if REGENERATE_AMQP_FRAMING
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/gen/amqp_framing.c
else
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/amqp_framing.c
endif

if SSL_CYASSL
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/amqp_cyassl.c
endif

if SSL_GNUTLS
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/amqp_gnutls.c
endif

if SSL_OPENSSL
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/amqp_openssl.c
endif

if SSL_POLARSSL
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/amqp_polarssl.c
endif

librabbitmq_librabbitmq_la_CFLAGS = \
	-I$(top_srcdir)/librabbitmq \
	$(SSL_CFLAGS) \
	$(AM_CFLAGS)

librabbitmq_librabbitmq_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	$(NO_UNDEFINED) \
	$(SSL_LIBS)

if OS_UNIX
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/unix/threads.h
librabbitmq_librabbitmq_la_CFLAGS += -I$(top_srcdir)/librabbitmq/unix
endif

if OS_WIN32
librabbitmq_librabbitmq_la_SOURCES += librabbitmq/win32/threads.h
librabbitmq_librabbitmq_la_CFLAGS += -I$(top_srcdir)/librabbitmq/win32
librabbitmq_librabbitmq_la_CFLAGS += -I$(top_srcdir)/librabbitmq/win32/msinttypes
endif

include_HEADERS = \
	$(top_srcdir)/librabbitmq/amqp.h \
	$(top_builddir)/librabbitmq/amqp_tcp_socket.h

if SSL
include_HEADERS += librabbitmq/amqp_ssl_socket.h
endif

if REGENERATE_AMQP_FRAMING

if PYTHON3
codegen_py = $(top_builddir)/librabbitmq/python3/codegen3.py
codegenlib_py = $(top_builddir)/librabbitmq/python3/amqp_codegen.py
codegenlib_path = $(top_builddir)/librabbitmq/python3

$(codegenlib_path):
	$(AM_V_at)$(MKDIR_P) $@

$(codegen_py): $(top_srcdir)/librabbitmq/codegen.py $(codegenlib_path)
	$(AM_V_GEN)$(PYTHON2TO3) $(top_srcdir)/librabbitmq/codegen.py | patch -p0 -o $(codegen_py)

$(codegenlib_py): $(top_srcdir)/codegen/amqp_codegen.py $(codegenlib_path)
	$(AM_V_GEN)$(PYTHON2TO3) $(top_srcdir)/codegen/amqp_codegen.py | patch -p0 -o $(codegenlib_py)
else
codegen_py = $(top_srcdir)/librabbitmq/codegen.py
codegenlib_py = $(top_srcdir)/codegen/amqp_codegen.py
codegenlib_path = $(top_srcdir)/codegen
endif

amqp_codegen_json = $(top_srcdir)/codegen/amqp-rabbitmq-0.9.1.json

amqp_framing_h = $(amqp_framing_dir)/amqp_framing.h
amqp_framing_c = $(amqp_framing_dir)/amqp_framing.c

$(amqp_framing_dir):
	$(AM_V_at)$(MKDIR_P) $@

$(amqp_framing_h): $(amqp_codegen_json) $(codegen_py) $(codegenlib_py) $(amqp_framing_dir)
	$(AM_V_GEN)PYTHONPATH=$(codegenlib_path) $(PYTHON) $(codegen_py) header $< $@

$(amqp_framing_c): $(amqp_codegen_json) $(codegen_py) $(codegenlib_py) $(amqp_framing_dir)
	$(AM_V_GEN)PYTHONPATH=$(codegenlib_path) $(PYTHON) $(codegen_py) body $< $@

BUILT_SOURCES = \
	$(amqp_framing_h) \
	$(amqp_framing_c)

include_HEADERS += $(amqp_framing_h)

else
include_HEADERS += $(top_srcdir)/librabbitmq/amqp_framing.h
endif #REGENERATE_AMQP_FRAMING

check_PROGRAMS = \
	tests/test_tables \
	tests/test_parse_url

TESTS = $(check_PROGRAMS)

tests_test_tables_SOURCES = tests/test_tables.c
tests_test_tables_LDADD = librabbitmq/librabbitmq.la

tests_test_parse_url_SOURCES = tests/test_parse_url.c
tests_test_parse_url_LDADD = librabbitmq/librabbitmq.la

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
tools_libcommon_la_SOURCES += tools/unix/process.c
tools_libcommon_la_SOURCES += tools/unix/process.h
tools_platform_CFLAGS += -I$(top_srcdir)/tools/unix
endif

if OS_WIN32
tools_libcommon_la_SOURCES += tools/win32/compat.c
tools_libcommon_la_SOURCES += tools/win32/compat.h
tools_libcommon_la_SOURCES += tools/win32/process.c
tools_libcommon_la_SOURCES += tools/win32/process.h
tools_platform_CFLAGS += -I$(top_srcdir)/tools/win32
tools_platform_CFLAGS += -I$(top_srcdir)/tools/win32/msinttypes
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
	$(man_MANS) \
	LICENSE-MIT \
	codegen/LICENSE \
	codegen/LICENSE-MPL-RabbitMQ \
	codegen/Makefile \
	codegen/README.extensions.md \
	codegen/amqp-rabbitmq-0.8.json \
	codegen/amqp-rabbitmq-0.9.1.json \
	codegen/amqp_codegen.py \
	codegen/demo_extension.json \
	codegen/license_info \
	debian \
	librabbitmq/codegen.py \
	tests/test_tables.expected \
	tools/doc/amqp-consume.xml \
	tools/doc/amqp-declare-queue.xml \
	tools/doc/amqp-delete-queue.xml \
	tools/doc/amqp-get.xml \
	tools/doc/amqp-publish.xml \
	tools/doc/librabbitmq-tools.xml

MOSTLYCLEANFILES = \
	$(man_MANS) \
	$(top_builddir)/tools/doc/man-date.ent

if REGENERATE_AMQP_FRAMING
MOSTLYCLEANFILES += \
	$(amqp_framing_h) \
	$(amqp_framing_c)
endif #REGENERATE_AMQP_FRAMING

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = librabbitmq.pc

clean-local:
	-$(MAKE) -C codegen clean

distclean-local:
	-$(MAKE) -C codegen distclean
