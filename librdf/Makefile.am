# -*- Mode: Makefile -*-
#
# Makefile.am - top level automake file for librdf
#
# $Id$
#
# (C) Dave Beckett 2000 ILRT, University of Bristol
# http://www.ilrt.bristol.ac.uk/people/cmdjb/
#
#
# This program is free software distributed under either of these licenses:
#   1. The GNU Lesser General Public License (LGPL)
# OR ALTERNATIVELY
#   2. The modified BSD license
#
# See LICENSE.html or LICENSE.txt for the full license terms.
#

SUBDIRS=@SD@

bin_SCRIPTS = librdf-config
lib_LIBRARIES = librdf.a
include_HEADERS = librdf.h

librdf_a_SOURCES = rdf_init.c rdf_uri.c rdf_node.c rdf_statement.c \
rdf_digest.c rdf_hash.c rdf_hash_memory.c rdf_model.c rdf_iterator.c \
rdf_list.c rdf_memory.c rdf_storage.c rdf_storage_list.c rdf_stream.c \
rdf_parser.c \
librdf.h rdf_types.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_digest.h rdf_hash.h rdf_hash_gdbm.h \
rdf_hash_memory.h rdf_model.h rdf_storage.h rdf_iterator.h \
rdf_context.h rdf_list.h rdf_hash_bdb.h rdf_storage.h rdf_storage_list.h \
rdf_stream.h rdf_parser.h


librdf_a_LIBADD = @LIBOBJS@ @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ 
librdf_a_DEPENDENCIES = @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@

EXTRA_librdf_a_SOURCES = rdf_hash_gdbm.c rdf_hash_bdb.c \
rdf_digest_md5.c rdf_digest_openssl.c \
rdf_parser_sirpac.c rdf_parser_libwww.c \
example.c example2.c

EXTRA_DIST=ChangeLog \
README      NEWS      TODO \
README.html NEWS.html TODO.html \
INSTALL.html LICENSE.html FAQS.html BUGS.html \
librdf-config.in \
memcmp.c

TESTS=rdf_node_test rdf_digest_test rdf_hash_test rdf_uri_test \
rdf_statement_test rdf_model_test rdf_storage_test

CLEANFILES=$(TESTS) test example

# Why is this not in the default makefile?
CC=@CC@

CFLAGS=@CFLAGS@
STANDARD_CFLAGS=@STANDARD_CFLAGS@



# Rules to construct test programs

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

example: example.c librdf.a
	$(LINK) -I$(srcdir) -I. $(srcdir)/example.c librdf.a $(LIBS)

example2: example2.c librdf.a
	$(LINK) -I$(srcdir) -I. $(srcdir)/example2.c librdf.a $(LIBS)



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
