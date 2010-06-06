SUBDIRS=doc

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
