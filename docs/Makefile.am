# -*- Mode: Makefile -*-
#
# Makefile.am - docs automake file for Redland
#
# $Id$
#
# Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
# Institute for Learning and Research Technology, University of Bristol.
#
#    This package is Free Software available under either of two licenses
#    (see FAQS.html to see why):
# 
# 1. The GNU Lesser General Public License (LGPL)
# 
#    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
#    full license text.
#      _________________________________________________________________
# 
#      Copyright (C) 2000 David Beckett, Institute for Learning and
#      Research Technology, University of Bristol. All Rights Reserved.
# 
#      This library is free software; you can redistribute it and/or
#      modify it under the terms of the GNU Lesser General Public License
#      as published by the Free Software Foundation; either version 2 of
#      the License, or (at your option) any later version.
# 
#      This library is distributed in the hope that it will be useful, but
#      WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#      Lesser General Public License for more details.
# 
#      You should have received a copy of the GNU Lesser General Public
#      License along with this library; if not, write to the Free Software
#      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
#      USA
#      _________________________________________________________________
# 
#    NOTE - under Term 3 of the LGPL, you may choose to license the entire
#    library under the GPL. See COPYING for the full license text.
# 
# 2. The Mozilla Public License
# 
#    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
#    license text.
# 
#    Under MPL section 13. I declare that all of the Covered Code is
#    Multiple Licensed:
#      _________________________________________________________________
# 
#      The contents of this file are subject to the Mozilla Public License
#      version 1.1 (the "License"); you may not use this file except in
#      compliance with the License. You may obtain a copy of the License
#      at http://www.mozilla.org/MPL/
# 
#      Software distributed under the License is distributed on an "AS IS"
#      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
#      the License for the specific language governing rights and
#      limitations under the License.
# 
#      The Initial Developer of the Original Code is David Beckett.
#      Portions created by David Beckett are Copyright (C) 2000 David
#      Beckett, Institute for Learning and Research Technology, University
#      of Bristol. All Rights Reserved.
# 
#      Alternatively, the contents of this file may be used under the
#      terms of the GNU Lesser General Public License, in which case the
#      provisions of the LGPL License are applicable instead of those
#      above. If you wish to allow use of your version of this file only
#      under the terms of the LGPL License and not to allow others to use
#      your version of this file under the MPL, indicate your decision by
#      deleting the provisions above and replace them with the notice and
#      other provisions required by the LGPL License. If you do not delete
#      the provisions above, a recipient may use your version of this file
#      under either the MPL or the LGPL License.


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
