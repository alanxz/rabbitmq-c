if TOOLS
TOOLS_SUBDIR=tools
else
TOOLS_SUBDIR=
endif

SUBDIRS=librabbitmq tests examples $(TOOLS_SUBDIR)

squeakyclean: maintainer-clean
	rm -f Makefile.in librabbitmq/Makefile.in tests/Makefile.in examples/Makefile.in tools/Makefile.in
	rm -f aclocal.m4
	rm -f config.guess config.h.in* config.sub configure
	rm -f depcomp install-sh ltmain.sh missing
	rm -rf codegen

codegen:
	mkdir -p $@
	cp -r "$(top_srcdir)/$(AMQP_CODEGEN_DIR)"/* $@
	$(MAKE) -C $@ clean

EXTRA_DIST=codegen debian LICENSE-MIT
