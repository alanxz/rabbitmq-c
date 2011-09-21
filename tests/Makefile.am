check_PROGRAMS = test_tables test_parse_url
TESTS = $(check_PROGRAMS)
EXTRA_DIST = test_tables.expected

AM_CFLAGS = -I$(top_srcdir)/librabbitmq

if GCC
# Because we want to build under Microsoft's C compiler (for which
# there is apparently no demand for C99 support), it's a good idea
# to have gcc tell us when we stray from the old standard.
AM_CFLAGS += -ansi -pedantic
endif

if USE_MSINTTYPES
AM_CFLAGS += -I$(top_srcdir)/msinttypes
endif

AM_LDFLAGS = $(top_builddir)/librabbitmq/librabbitmq.la
