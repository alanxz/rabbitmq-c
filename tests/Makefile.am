noinst_PROGRAMS = test_tables

AM_CFLAGS = -I$(top_srcdir)/librabbitmq -I$(top_srcdir)/librabbitmq/$(PLATFORM_DIR)

if GCC
# Because we want to build under Microsoft's C compiler (for which
# there is apparently no demand for C99 support), it's a good idea
# to have gcc tell us when we stray from the old standard.
AM_CFLAGS += -ansi -pedantic
endif

AM_LDFLAGS = $(top_builddir)/librabbitmq/librabbitmq.la
