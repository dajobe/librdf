# -*- Mode: Makefile -*-
#
# Makefile.am - top level automake file for Redland
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


SUBDIRS=@SD@

bin_SCRIPTS = redland-config
lib_LIBRARIES = librdf.a
# FIXME: Should *not* install all headers
include_HEADERS = redland.h librdf.h \
rdf_types.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_context.h \
rdf_digest.h rdf_hash.h rdf_hash_bdb.h rdf_hash_gdbm.h rdf_hash_memory.h \
rdf_model.h rdf_iterator.h \
rdf_list.h \
rdf_storage.h rdf_storage_list.h rdf_storage_hashes.h \
rdf_stream.h \
rdf_parser.h

librdf_a_SOURCES = rdf_init.c \
rdf_uri.c rdf_node.c rdf_statement.c \
rdf_digest.c rdf_hash.c rdf_hash_cursor.c rdf_hash_memory.c \
rdf_model.c rdf_iterator.c \
rdf_list.c rdf_memory.c \
rdf_storage.c rdf_storage_list.c rdf_storage_hashes.c \
rdf_stream.c \
rdf_parser.c rdf_heuristics.c rdf_files.c \
librdf.h rdf_types.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_context.h \
rdf_digest.h rdf_hash.h rdf_hash_bdb.h rdf_hash_gdbm.h rdf_hash_memory.h \
rdf_model.h rdf_iterator.h \
rdf_list.h \
rdf_storage.h rdf_storage_list.h rdf_storage_hashes.h \
rdf_stream.h \
rdf_parser.h rdf_files.h


librdf_a_LIBADD = @LIBOBJS@ @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ 
librdf_a_DEPENDENCIES = @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@

EXTRA_librdf_a_SOURCES = rdf_hash_gdbm.c rdf_hash_bdb.c \
rdf_digest_md5.c rdf_digest_openssl.c \
rdf_parser_sirpac.c rdf_parser_libwww.c \
example1.c example2.c example3.c example4.c

EXTRA_DIST=ChangeLog \
README      NEWS      TODO \
README.html NEWS.html TODO.html \
INSTALL.html LICENSE.html FAQS.html MPL.html \
redland-config.in \
memcmp.c

TESTS=rdf_node_test rdf_digest_test rdf_hash_test rdf_uri_test \
rdf_statement_test rdf_model_test rdf_storage_test rdf_parser_test \
rdf_files_test rdf_heuristics_test

EXAMPLES=example1 example2 example3 example4

CLEANFILES=$(TESTS) $(EXAMPLES) test.db test1.db

# Memory debugging alternatives
MEM=@MEM@
MEM_LIBS=@MEM_LIBS@

# 1) None (use standard functions directly)
#MEM=
#MEM_LIBS=

# 2) Use internal memory debug
#MEM=-DLIBREDLAND_MEMORY_DEBUG=1
#MEM_LIBS=

# 3) Use dmalloc library
#MEM=-DLIBREDLAND_MEMORY_DEBUG_DMALLOC=1
#MEM_LIBS=-ldmalloc


# Why is this not in the default makefile?
CC=@CC@

CFLAGS=@CFLAGS@ $(MEM)
STANDARD_CFLAGS=@STANDARD_CFLAGS@ $(MEM)
LIBS=@LIBS@ $(MEM_LIBS)


# Rules to construct test programs

# Some people need a little help ;-)
test: check

rdf_node_test: rdf_node.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_node.c librdf.a $(LIBS)

rdf_digest_test: rdf_digest.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_digest.c librdf.a $(LIBS)

rdf_hash_test: rdf_hash.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_hash.c librdf.a $(LIBS)

rdf_uri_test: rdf_uri.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_uri.c librdf.a $(LIBS)

rdf_statement_test: rdf_statement.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_statement.c librdf.a $(LIBS)

rdf_model_test: rdf_model.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_model.c librdf.a $(LIBS)

rdf_storage_test: rdf_storage.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_storage.c librdf.a $(LIBS)

rdf_parser_test: rdf_parser.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_parser.c librdf.a $(LIBS)

rdf_files_test: rdf_files.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_files.c librdf.a $(LIBS)

rdf_heuristics_test: rdf_heuristics.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_heuristics.c librdf.a $(LIBS)

examples: $(EXAMPLES)

example1: example1.c librdf.a
	$(LINK) -I$(srcdir) -I. $(srcdir)/example1.c librdf.a $(LIBS)

example2: example2.c librdf.a
	$(LINK) -I$(srcdir) -I. $(srcdir)/example2.c librdf.a $(LIBS)

example3: example3.c librdf.a
	$(LINK) -I$(srcdir) -I. $(srcdir)/example3.c librdf.a $(LIBS)

example4: example4.c librdf.a
	$(LINK) -I$(srcdir) -I. $(srcdir)/example4.c librdf.a $(LIBS)



# Create some text files from HTML sources
LYNX=lynx
LYNXARGS=-dump -nolist

SUFFIXES = .html .txt

.html.txt:
	$(LYNX) $(LYNXARGS) $< > $@

README: README.html
	$(LYNX) $(LYNXARGS) $< > $@

NEWS: NEWS.html
	$(LYNX) $(LYNXARGS) $< > $@

TODO: TODO.html
	$(LYNX) $(LYNXARGS) $< > $@

# The action is the COMPILE definition but always using least warning CFLAGS
@MAINT@rdf_parser_libwww.o: rdf_parser_libwww.c
@MAINT@	$(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(STANDARD_CFLAGS) -c $<

@SET_MAKE@
