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
perl.html python.html tcl.html java.html ruby.html php.html \
build-docs fix-pod-xhtml fix-python-xhtml \
overview.png overview.ps overview.pdf api pod pydoc \
redland.pod kernel-doc \
$(man_MANS)

DOC_CLASSES=node statement model storage storage_hashes storage_list stream parser parser_raptor parser_repat query query_triples serializer serializer_raptor concepts init digest digest_md5 digest_openssl hash hash_bdb hash_cursor hash_memory heuristics iterator list uri utf8

POD_MODULES=RDF/Redland RDF/Redland/Iterator RDF/Redland/Model RDF/Redland/Node RDF/Redland/Parser RDF/Redland/RSS RDF/Redland/Serializer RDF/Redland/Statement RDF/Redland/Storage RDF/Redland/Stream RDF/Redland/URI

DOC_C_FILES=$(DOC_CLASSES:%=$(srcdir)/../librdf/rdf_%.c)
API_DOCBOOK_FILES=$(DOC_CLASSES:%=%.sgml)

PERL_HTML_POD_FILES=$(POD_MODULES:%=pod/%.html)
PYTHON_HTML_PYDOC_FILES=pydoc/RDF.html

CLEANFILES=README
MAINTAINERCLEANFILES=api.sgml api pod pydoc $(API_DOCBOOK_FILES)

PERL=perl

# Create some text files from HTML sources
LYNX=lynx
LYNXARGS=-dump -nolist

SUFFIXES = .html .txt .sgml .ps .pdf .pm .pod .3

api: api/index.html

pod: $(PERL_HTML_POD_FILES)

pydoc: $(PYTHON_HTML_PYDOC_FILES)

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
@MAINT@overview.ps: overview.xfig
@MAINT@	fig2dev -L ps -l dummy -z A4 overview.xfig overview.pdf
@MAINT@
@MAINT@overview.pdf: overview.ps
@MAINT@	ps2pdf overview.ps overview.pdf
@MAINT@
@MAINT@api-big.html: api.sgml
@MAINT@	-jade -t sgml -ihtml -d /usr/lib/sgml/stylesheets/cygnus-both.dsl -V nochunks $< > $@
@MAINT@
@MAINT@$(PERL_HTML_POD_FILES): pod/%.html: $(srcdir)/../perl/lib/%.pm $(srcdir)/fix-pod-xhtml
@MAINT@	mkdir -p pod/RDF/Redland
@MAINT@	-pod2html --noindex --htmlroot /docs/pod --infile=$< | tidy -asxml -wrap 1000 2>/dev/null | perl $(srcdir)/fix-pod-xhtml $@
@MAINT@	rm -f pod2htm*
@MAINT@
@MAINT@$(srcdir)/../python/RDF.html: $(srcdir)/../python/RDF.py
@MAINT@	cd $(srcdir)/../python && make RDF.html
@MAINT@$(PYTHON_HTML_PYDOC_FILES): pydoc/%.html: $(srcdir)/../python/RDF.html $(srcdir)/fix-python-xhtml
@MAINT@	mkdir -p pydoc
@MAINT@	-tidy -asxml -wrap 1000 $< 2>/dev/null | perl $(srcdir)/fix-python-xhtml $@

if MAINTAINER_MODE
redland.3: redland.pod
	pod2man --section=3 --center="Redland RDF Library" \
	--release="`date +%d-%b-%Y`" --date="@PACKAGE@ @LIBRDF_VERSION@" $< | \
	sed -e "s;REDLAND_VERSION_STRING;@LIBRDF_VERSION@;" > $@
	rm -f pod2htm*

clean-local:
	rm -rf api.junk DBTOHTML* pod2htm*

distclean-local:
	rm -rf $(API_DOCBOOK_FILES)

else
# This is the best that can be done without pod2man
redland.3: redland.pod
	cp $< $@

clean-local:
	rm -rf $(API_DOCBOOK_FILES) api.junk DBTOHTML* pod2htm*
endif

README: README.html
	$(LYNX) $(LYNXARGS) $< > $@

@SET_MAKE@
