bin_PROGRAMS = amqp-publish amqp-get amqp-consume amqp-declare-queue amqp-delete-queue

AM_CFLAGS = -I$(top_srcdir)/librabbitmq
AM_LDFLAGS = $(top_builddir)/librabbitmq/librabbitmq.la

LDADD=$(LIBPOPT)

noinst_HEADERS = common.h

amqp_publish_SOURCES = publish.c common.c
amqp_get_SOURCES = get.c common.c
amqp_consume_SOURCES = consume.c common.c
amqp_declare_queue_SOURCES = declare_queue.c common.c
amqp_delete_queue_SOURCES = delete_queue.c common.c

if TOOLS_DOC
man_MANS = \
	doc/amqp-publish.1 \
	doc/amqp-consume.1 \
	doc/amqp-get.1 \
	doc/amqp-declare-queue.1 \
	doc/amqp-delete-queue.1 \
	doc/librabbitmq-tools.7
MOSTLYCLEANFILES = doc/man-date.ent

# automake complains about % pattern rules, and suffix rules don't
# support multiple dependencies, so we have to expand all these out.
doc/amqp-publish.1: doc/publish.xml doc/man-date.ent
	$(XMLTO) man -o doc $<
doc/amqp-consume.1: doc/consume.xml doc/man-date.ent
	$(XMLTO) man -o doc $<
doc/amqp-get.1: doc/get.xml doc/man-date.ent
	$(XMLTO) man -o doc $<
doc/amqp-declare-queue.1: doc/declare_queue.xml doc/man-date.ent
	$(XMLTO) man -o doc $<
doc/amqp-delete-queue.1: doc/delete_queue.xml doc/man-date.ent
	$(XMLTO) man -o doc $<
doc/librabbitmq-tools.7: doc/librabbitmq-tools.xml doc/man-date.ent
	$(XMLTO) man -o doc $<

doc/man-date.ent:
	date +'%Y-%m-%d' >$@

clean::
	rm -f doc/man-date.ent

endif
