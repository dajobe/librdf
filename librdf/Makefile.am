# -*- Mode: Makefile -*-
#
# Makefile.am - top level automake file for Redland
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


lib_LTLIBRARIES = librdf.la

include_HEADERS = redland.h librdf.h \
rdf_init.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_concepts.h \
rdf_model.h \
rdf_iterator.h \
rdf_storage.h \
rdf_stream.h \
rdf_parser.h \
rdf_utf8.h \
rdf_query.h \
rdf_serializer.h

librdf_la_SOURCES = rdf_init.c \
rdf_uri.c rdf_node.c rdf_statement.c \
rdf_digest.c rdf_hash.c rdf_hash_cursor.c rdf_hash_memory.c \
rdf_model.c rdf_model_storage.c \
rdf_iterator.c rdf_concepts.c \
rdf_list.c \
rdf_storage.c rdf_storage_list.c rdf_storage_hashes.c \
rdf_stream.c \
rdf_parser.c rdf_heuristics.c rdf_files.c rdf_utf8.c \
rdf_query.c \
rdf_query_triples.c \
rdf_serializer.c \
rdf_serializer_raptor.c rdf_serializer_rdfxml.c \
redland.h librdf.h \
rdf_internal.h \
rdf_init.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_concepts.h \
rdf_digest.h rdf_hash.h rdf_hash_bdb.h rdf_hash_memory.h \
rdf_types.h \
rdf_model.h rdf_model_storage.h \
rdf_iterator.h \
rdf_list.h \
rdf_storage.h rdf_storage_list.h rdf_storage_hashes.h \
rdf_stream.h \
rdf_parser.h \
rdf_heuristics.h rdf_files.h rdf_utf8.h \
rdf_query.h \
rdf_query_triples.h \
rdf_serializer.h

librdf_la_LIBADD = \
@LTLIBOBJS@ @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ @STORAGE_OBJS@ \
@LIBRDF_INTERNAL_LIBS@

librdf_la_DEPENDENCIES = \
@DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ @STORAGE_OBJS@ \
@LIBRDF_INTERNAL_LIBS@

EXTRA_librdf_la_SOURCES = rdf_hash_bdb.c \
rdf_digest_md5.c rdf_digest_sha1.c rdf_digest_openssl.c \
rdf_parser_raptor.c \
rdf_storage_tstore.c rdf_storage_tstore.h \
rdf_storage_mysql.c rdf_storage_mysql.h

EXTRA_DIST=\
memcmp.c \
redland.spec redland.spec.in \
redland.pc.in

librdf_la_LDFLAGS = -version-info @LIBRDF_LIBTOOL_VERSION@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = redland.pc


TESTS=rdf_node_test rdf_digest_test rdf_hash_test rdf_uri_test \
rdf_statement_test rdf_model_test rdf_storage_test rdf_parser_test \
rdf_files_test rdf_heuristics_test rdf_utf8_test rdf_concepts_test \
rdf_query_test rdf_serializer_test rdf_stream_test rdf_iterator_test

CLEANFILES=$(TESTS) test.db test1.db

# Memory debugging alternatives
MEM=@MEM@
MEM_LIBS=@MEM_LIBS@

# 1) None (use standard functions directly)
#MEM=
#MEM_LIBS=

# 2) Use internal memory debug
#MEM=-DLIBRDF_MEMORY_DEBUG=1
#MEM_LIBS=

# 3) Use dmalloc library
#MEM=-DLIBRDF_MEMORY_DEBUG_DMALLOC=1
#MEM_LIBS=-ldmalloc


# Use tar, whatever it is called (better be GNU tar though)
TAR=@TAR@

# Why is this not in the default makefile?
CC=@CC@

AM_CPPFLAGS=@CPPFLAGS@ @LIBRDF_CPPFLAGS@ @LIBRDF_INTERNAL_CPPFLAGS@ @LIBRDF_EXTERNAL_CPPFLAGS@
AM_CFLAGS=@CFLAGS@ $(MEM)
STANDARD_CFLAGS=@STANDARD_CFLAGS@ $(MEM)
AM_LDFLAGS=@LIBRDF_LDFLAGS@ @LIBRDF_LIBS@ @LIBRDF_EXTERNAL_LIBS@ $(MEM_LIBS)

LOCAL_LIB_DIR=lib

# Rules to construct test programs

# Some people need a little help ;-)
test: check

rdf_node_test: rdf_node.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_node.c librdf.la

rdf_digest_test: rdf_digest.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_digest.c librdf.la

rdf_hash_test: rdf_hash.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_hash.c librdf.la

# This needs raptor.h
rdf_uri_test: rdf_uri.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE @LIBRDF_INTERNAL_CPPFLAGS@ @LIBRDF_EXTERNAL_CPPFLAGS@ $(srcdir)/rdf_uri.c librdf.la

rdf_statement_test: rdf_statement.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_statement.c librdf.la

rdf_model_test: rdf_model.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_model.c librdf.la

rdf_storage_test: rdf_storage.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_storage.c librdf.la

rdf_parser_test: rdf_parser.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_parser.c librdf.la

rdf_files_test: rdf_files.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_files.c librdf.la

rdf_heuristics_test: rdf_heuristics.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_heuristics.c librdf.la

rdf_utf8_test: rdf_utf8.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_utf8.c librdf.la

rdf_concepts_test: rdf_concepts.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_concepts.c librdf.la

rdf_query_test: rdf_query.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_query.c librdf.la

rdf_serializer_test: rdf_serializer.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_serializer.c librdf.la

rdf_stream_test: rdf_stream.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_stream.c librdf.la

rdf_iterator_test: rdf_iterator.c librdf.la
	$(LINK) $(CPPFLAGS) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_iterator.c librdf.la

@SET_MAKE@

$(top_builddir)/raptor/libraptor.la:
	cd $(top_builddir)/raptor && $(MAKE) $(AM_MAKEFLAGS) libraptor.la
