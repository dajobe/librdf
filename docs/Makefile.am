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


EXTRA_DIST=api.sgml.in api.sgml README.html README build-docs \
overview.png overview.ps overview.pdf

DOC_CLASSES=digest digest_openssl digest_md5 hash hash_bdb hash_memory iterator list model node parser parser_libwww parser_sirpac statement storage storage_list stream uri

DOC_C_FILES=$(DOC_CLASSES:%=$(srcdir)/rdf_%.c)
API_DOCBOOK_FILES=$(DOC_CLASSES:%=%.sgml)

CLEANFILES=$(API_DOCBOOK_FILES) api.html README
MAINTAINERCLEANFILES=api.sgml

PERL=perl

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
@MAINT@api.sgml: $(srcdir)/build-docs api.sgml.in $(API_DOCBOOK_FILES)
@MAINT@	rm -rf DBTOHTML*
@MAINT@	$(PERL) $(srcdir)/build-docs api.sgml.in > api.sgml
@MAINT@
@MAINT@overview.ps: overview.xfig
@MAINT@	fig2dev -L ps -l dummy -z A4 overview.xfig overview.pdf
@MAINT@
@MAINT@overview.pdf: overview.ps
@MAINT@	ps2pdf overview.ps overview.pdf

clean-local:
	rm -rf $(API_DOCBOOK_FILES) api.html api.junk DBTOHTML*

README: README.html
	$(LYNX) $(LYNXARGS) $< > $@

@SET_MAKE@
