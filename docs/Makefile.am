# -*- Mode: Makefile -*-
#
# Makefile.am - docs automake file for Redland
#
# $Id$
#
# Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
# Institute for Learning and Research Technology - http://www.ilrt.org/
# University of Bristol - http://www.bristol.ac.uk/
# 
# This package is Free Software or Open Source available under the
# following licenses (these are alternatives):
#   1. GNU Lesser General Public License (LGPL) Version 2
#   2. GNU General Public License (GPL) Version 2
#   3. Mozilla Public License (MPL) Version 1.1
# and no other versions of those licenses.
# 
# See INSTALL.html or INSTALL.txt at the top of this package for the
# full license terms.
# 


EXTRA_DIST=api.sgml.in api.sgml README.html README \
perl.html python.html tcl.html \
build-docs fix-pod-xhtml overview.png overview.ps overview.pdf api pod

DOC_CLASSES=node statement model storage storage_hashes storage_list stream parser parser_libwww parser_repat parser_rapier parser_sirpac concepts init digest digest_md5 digest_openssl hash hash_bdb hash_cursor hash_memory heuristics iterator list uri utf8

POD_MODULES=RDF RDF/Iterator RDF/Model RDF/Node RDF/Parser RDF/RSS RDF/Statement RDF/Storage RDF/Stream RDF/URI

DOC_C_FILES=$(DOC_CLASSES:%=$(srcdir)/../rdf_%.c)
API_DOCBOOK_FILES=$(DOC_CLASSES:%=%.sgml)

PERL_HTML_POD_FILES=$(POD_MODULES:%=pod/%.html)

CLEANFILES=$(API_DOCBOOK_FILES) README
MAINTAINERCLEANFILES=api.sgml api pod

PERL=perl

# Create some text files from HTML sources
LYNX=lynx
LYNXARGS=-dump -nolist

SUFFIXES = .html .txt .sgml .ps .pdf .pm

api: api/index.html

pod: pod/RDF.html

# Generates a tree of HTML files, not 1 file
api/index.html: api.sgml
	rm -rf api.junk DBTOHTML*
	db2html $<
	rm -rf api.junk

.html.txt:
	$(LYNX) $(LYNXARGS) $< > $@

# These don't work for me
.sgml.ps:
	db2ps $<

.sgml.pdf:
	db2pdf $<

@MAINT@$(API_DOCBOOK_FILES): %.sgml: $(srcdir)/../rdf_%.c
@MAINT@	kernel-doc -docbook $< > $@
@MAINT@
@MAINT@api.sgml: $(srcdir)/build-docs api.sgml.in $(API_DOCBOOK_FILES)
@MAINT@	$(PERL) $(srcdir)/build-docs api.sgml.in > api.sgml
@MAINT@
@MAINT@overview.ps: overview.xfig
@MAINT@	fig2dev -L ps -l dummy -z A4 overview.xfig overview.pdf
@MAINT@
@MAINT@overview.pdf: overview.ps
@MAINT@	ps2pdf overview.ps overview.pdf
@MAINT@
@MAINT@api-big.html: api.sgml
@MAINT@	-jade -t sgml -ihtml -d /usr/lib/sgml/stylesheets/cygnus-both.dsl -V nochunks $? > $@
@MAINT@
@MAINT@pod/RDF.html: $(PERL_HTML_POD_FILES)
@MAINT@
@MAINT@$(PERL_HTML_POD_FILES): pod/%.html: $(srcdir)/../perl/lib/%.pm $(srcdir)/fix-pod-xhtml
@MAINT@	mkdir -p pod/RDF
@MAINT@	-pod2html --noindex --htmlroot /docs/pod --infile=$< | tidy -asxml 2>/dev/null | perl $(srcdir)/fix-pod-xhtml $@
@MAINT@	rm -f pod2html*
@MAINT@

clean-local:
	rm -rf $(API_DOCBOOK_FILES) api.junk DBTOHTML* pod2html*

README: README.html
	$(LYNX) $(LYNXARGS) $< > $@

@SET_MAKE@
