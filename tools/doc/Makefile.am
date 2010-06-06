EXTRA_DIST = \
	publish.xml \
	consume.xml \
	get.xml \
	declare_queue.xml \
	delete_queue.xml \
	librabbitmq-tools.xml

if TOOLS_DOC
man_MANS = \
	amqp-publish.1 \
	amqp-consume.1 \
	amqp-get.1 \
	amqp-declare-queue.1 \
	amqp-delete-queue.1 \
	librabbitmq-tools.7
MOSTLYCLEANFILES = man-date.ent $(man_MANS)

# automake complains about % pattern rules, and suffix rules don't
# support multiple dependencies, so we have to expand all these out.
#
# Also, xmlto's --searchpath doesn't get passed through to xmllint, so
# we disable xmllint validation with --skip-validation for the benefit
# of build/source separation as required by distcheck, debian
# packaging etc.
amqp-publish.1: publish.xml man-date.ent
	$(XMLTO) --skip-validation --searchpath $(CURDIR) man $<
amqp-consume.1: consume.xml man-date.ent
	$(XMLTO) --skip-validation --searchpath $(CURDIR) man $<
amqp-get.1: get.xml man-date.ent
	$(XMLTO) --skip-validation --searchpath $(CURDIR) man $<
amqp-declare-queue.1: declare_queue.xml man-date.ent
	$(XMLTO) --skip-validation --searchpath $(CURDIR) man $<
amqp-delete-queue.1: delete_queue.xml man-date.ent
	$(XMLTO) --skip-validation --searchpath $(CURDIR) man $<
librabbitmq-tools.7: librabbitmq-tools.xml man-date.ent
	$(XMLTO) --skip-validation --searchpath $(CURDIR) man $<

man-date.ent:
	date +'%Y-%m-%d' >$@

endif
