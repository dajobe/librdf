# -*- Mode: Makefile -*-
#
# Makefile.am - docs automake file for Redland
#
# $Id$
#
# Copyright (C) 2000-2004 David Beckett - http://purl.org/net/dajobe/
# Institute for Learning and Research Technology - http://www.ilrt.bris.ac.uk/
# University of Bristol - http://www.bristol.ac.uk/
# 
# This package is Free Software or Open Source available under the
# following licenses (these are alternatives):
#   1. GNU Lesser General Public License (LGPL)
#   2. GNU General Public License (GPL)
#   3. Mozilla Public License (MPL)
# 
# See LICENSE.html or LICENSE.txt at the top of this package for the
# full license terms.
# 
# 

man_MANS=redland.3

EXTRA_DIST=api.sgml.in api.sgml README.html README \
storage.html \
build-docs \
api \
redland.pod kernel-doc \
$(man_MANS)

DOC_CLASSES=init node statement model \
storage stream parser query query_results serializer concepts uri \
digest hash heuristics iterator utf8

DOC_C_FILES=$(DOC_CLASSES:%=$(srcdir)/../librdf/rdf_%.c)
API_DOCBOOK_FILES=$(DOC_CLASSES:%=%.sgml)
API_HTML_FILES=$(DOC_CLASSES:%=api_%.html)

CLEANFILES=README
MAINTAINERCLEANFILES=api.sgml api $(API_DOCBOOK_FILES) \
$(API_HTML_FILES)

PERL=perl

# Create some text files from HTML sources
LYNX=lynx
LYNXARGS=-dump -nolist

SUFFIXES = .html .txt .sgml .ps .pdf .pm .pod .3

api: api/index.html

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

@MAINT@$(API_DOCBOOK_FILES): %.sgml: $(srcdir)/../librdf/rdf_%.c $(srcdir)/kernel-doc
@MAINT@	$(PERL) $(srcdir)/kernel-doc -docbook $< > $@
@MAINT@
@MAINT@api.sgml: $(srcdir)/build-docs api.sgml.in $(API_DOCBOOK_FILES)
@MAINT@	$(PERL) $(srcdir)/build-docs api.sgml.in > api.sgml
@MAINT@
@MAINT@$(API_HTML_FILES): api_%.html: $(srcdir)/../librdf/rdf_%.c $(srcdir)/kernel-doc
@MAINT@	$(PERL) $(srcdir)/kernel-doc -html $< > $@
@MAINT@
@MAINT@api.html: $(srcdir)/build-docs api.html.in $(API_HTML_FILES)
@MAINT@	$(PERL) $(srcdir)/build-docs api.html.in > api.html
@MAINT@
@MAINT@overview.ps: overview.xfig
@MAINT@	fig2dev -L ps -l dummy -z A4 overview.xfig overview.pdf
@MAINT@
@MAINT@overview.pdf: overview.ps
@MAINT@	ps2pdf overview.ps overview.pdf


if MAINTAINER_MODE
redland.3: redland.pod
	pod2man --section=3 --center="Redland RDF Application Framework" \
	--release="`date +%d-%b-%Y`" --date="@PACKAGE@ @PACKAGE_VERSION@" $< | \
	sed -e "s;REDLAND_VERSION_STRING;@PACKAGE_VERSION@;" > $@
	rm -f pod2htm*

clean-local:
	rm -rf api.junk DBTOHTML* pod2htm*

distclean-local:
	rm -rf $(API_DOCBOOK_FILES)

else
clean-local:
	rm -rf $(API_DOCBOOK_FILES) api.junk DBTOHTML* pod2htm*
endif

README: README.html
	$(LYNX) $(LYNXARGS) $< > $@

@SET_MAKE@
