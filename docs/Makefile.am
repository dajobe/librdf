#
# automake file for librdf documentation
#
# $Source$
# $Id$
#
#

EXTRA_DIST=api.sgml.in api.sgml build-docs

DOC_CLASSES=digest digest_openssl digest_md5 hash hash_bdb hash_gdbm hash_memory iterator list model node parser parser_libwww parser_sirpac statement storage storage_list stream uri

DOC_C_FILES=$(DOC_CLASSES:%=$(srcdir)/rdf_%.c)
API_DOCBOOK_FILES=$(DOC_CLASSES:%=%.sgml)

CLEANFILES=$(API_DOCBOOK_FILES) api.html
MAINTAINERCLEANFILES=api.sgml

# Create some text files from HTML sources
LYNX=lynx
LYNXARGS=-dump -nolist

SUFFIXES = .html .txt .sgml .ps .pdf

.html.txt:
	$(LYNX) $(LYNXARGS) $< > $@

.sgml.html:
	db2html $<
	touch $@

.sgml.ps:
	db2ps $<
	touch $@

.sgml.pdf:
	db2pdf $<
	touch $@

@MAINT@$(API_DOCBOOK_FILES): %.sgml: $(srcdir)/../rdf_%.c
@MAINT@	kernel-doc -docbook $< > $@
@MAINT@
@MAINT@api.sgml: build-docs api.sgml.in $(API_DOCBOOK_FILES)
@MAINT@	build-docs api.sgml.in > api.sgml
@MAINT@
clean-local:
	rm -rf $(API_DOCBOOK_FILES) api.html api.junk DBTOHTML*

@SET_MAKE@
