noinst_PROGRAMS = amqp_sendstring amqp_exchange_declare amqp_listen amqp_producer amqp_consumer \
	amqp_unbind amqp_bind amqp_listenq

AM_CFLAGS = -I$(top_srcdir)/librabbitmq
AM_LDFLAGS = $(top_builddir)/librabbitmq/librabbitmq.la

noinst_HEADERS = example_utils.h

amqp_sendstring_SOURCES = amqp_sendstring.c example_utils.c
amqp_exchange_declare_SOURCES = amqp_exchange_declare.c example_utils.c
amqp_listen_SOURCES = amqp_listen.c example_utils.c
amqp_producer_SOURCES = amqp_producer.c example_utils.c
amqp_consumer_SOURCES = amqp_consumer.c example_utils.c
amqp_unbind_SOURCES = amqp_unbind.c example_utils.c
amqp_bind_SOURCES = amqp_bind.c example_utils.c
amqp_listenq_SOURCES = amqp_listenq.c example_utils.c
