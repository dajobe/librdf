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


# Subdirectories to build/install etc.
SUBDIRS=@SD@
# Subdirectories to use for distribution
DIST_SUBDIRS=docs perl python tcl java ruby expat repat debian

bin_SCRIPTS = redland-config
lib_LTLIBRARIES = librdf.la

# Need this so that automake doesn't moan
noinst_PROGRAMS = $(EXAMPLES)

include_HEADERS = redland.h librdf.h \
rdf_init.h \
rdf_hash.h \
rdf_digest.h \
rdf_types.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_concepts.h \
rdf_model.h rdf_iterator.h \
rdf_storage.h \
rdf_stream.h \
rdf_parser.h \
rdf_heuristics.h \
rdf_list.h \
rdf_hash_bdb.h rdf_hash_gdbm.h rdf_hash_memory.h \
rdf_files.h \
rdf_storage_list.h rdf_storage_hashes.h rdf_utf8.h \
rdf_query.h \
rdf_query_triples.h

JAVA_CLASSES=@JAVA_CLASSES@
pkgdata_DATA = $(JAVA_CLASSES)

librdf_la_SOURCES = rdf_init.c \
rdf_uri.c rdf_node.c rdf_statement.c \
rdf_digest.c rdf_hash.c rdf_hash_cursor.c rdf_hash_memory.c \
rdf_model.c rdf_iterator.c rdf_concepts.c \
rdf_list.c rdf_memory.c \
rdf_storage.c rdf_storage_list.c rdf_storage_hashes.c \
rdf_stream.c \
rdf_parser.c rdf_heuristics.c rdf_files.c rdf_utf8.c \
rdf_query.c \
rdf_query_triples.c \
redland.h librdf.h \
rdf_init.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_concepts.h \
rdf_digest.h rdf_hash.h rdf_hash_bdb.h rdf_hash_gdbm.h rdf_hash_memory.h \
rdf_types.h \
rdf_model.h rdf_iterator.h \
rdf_list.h \
rdf_storage.h rdf_storage_list.h rdf_storage_hashes.h \
rdf_stream.h \
rdf_parser.h \
rdf_heuristics.h rdf_files.h rdf_utf8.h \
rdf_query.h \
rdf_query_triples.h

librdf_la_LIBADD = @LTLIBOBJS@ @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ \
@XML_OBJS@
librdf_la_DEPENDENCIES = @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ \
@XML_OBJS@

EXTRA_librdf_la_SOURCES = rdf_hash_gdbm.c rdf_hash_bdb.c \
rdf_digest_md5.c rdf_digest_openssl.c \
rdf_parser_sirpac.c rdf_parser_libwww.c \
rdf_parser_raptor.c \
rdf_parser_repat.c \
example1.c example2.c example3.c example4.c \
@RAPTOR_SRCS@

EXTRA_DIST=ChangeLog \
README      NEWS      TODO      LICENSE.txt \
README.html NEWS.html TODO.html LICENSE.html \
INSTALL.html FAQS.html MPL.html \
redland-config.in \
memcmp.c Redland.i \
PrintConsumer.java PrintParser.java \
$(JAVA_CLASSES) \
autogen.sh \
Makefile.PL \
redland.spec redland.spec.in \
@RAPTOR_INCS@ \
ex1.rdf ex2.rdf

librdf_la_LDFLAGS = -version-info @LIBRDF_LIBTOOL_VERSION@

TESTS=rdf_node_test rdf_digest_test rdf_hash_test rdf_uri_test \
rdf_statement_test rdf_model_test rdf_storage_test rdf_parser_test \
rdf_files_test rdf_heuristics_test rdf_utf8_test rdf_concepts_test \
rdf_query_test

EXAMPLES=example1 example2 example3 example4

CLEANFILES=$(TESTS) $(EXAMPLES) test.db test1.db
MAINTAINERCLEANFILES=$(JAVA_CLASSES)

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

CFLAGS=@CFLAGS@ $(MEM)
STANDARD_CFLAGS=@STANDARD_CFLAGS@ $(MEM)
LIBS=@LIBS@ $(MEM_LIBS)


# Rules to construct test programs

# Some people need a little help ;-)
test: check

rdf_node_test: rdf_node.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_node.c librdf.la $(LIBS)

rdf_digest_test: rdf_digest.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_digest.c librdf.la $(LIBS)

rdf_hash_test: rdf_hash.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_hash.c librdf.la $(LIBS)

rdf_uri_test: rdf_uri.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_uri.c librdf.la $(LIBS)

rdf_statement_test: rdf_statement.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_statement.c librdf.la $(LIBS)

rdf_model_test: rdf_model.c librdf.la ex1.rdf ex2.rdf
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_model.c librdf.la $(LIBS)

rdf_storage_test: rdf_storage.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_storage.c librdf.la $(LIBS)

rdf_parser_test: rdf_parser.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_parser.c librdf.la $(LIBS)

rdf_files_test: rdf_files.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_files.c librdf.la $(LIBS)

rdf_heuristics_test: rdf_heuristics.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_heuristics.c librdf.la $(LIBS)

rdf_utf8_test: rdf_utf8.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_utf8.c librdf.la $(LIBS)

rdf_concepts_test: rdf_concepts.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_concepts.c librdf.la $(LIBS)

rdf_query_test: rdf_query.c librdf.la
	$(LINK) -static -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_query.c librdf.la $(LIBS)

examples: $(EXAMPLES)

example1_SOURCES = example1.c
example1_LDADD = librdf.la
example1_LDFLAGS = -static

example2_SOURCES = example2.c
example2_LDADD = librdf.la
example2_LDFLAGS = -static

example3_SOURCES = example3.c
example3_LDADD = librdf.la
example3_LDFLAGS = -static

example4_SOURCES = example4.c
example4_LDADD = librdf.la
example4_LDFLAGS = -static


# Create some text files from HTML sources
LYNX=lynx
HTML_TO_TEXT=TERM=vt100 $(LYNX) -dump -nolist

SUFFIXES = .html .txt

.html.txt:
	$(HTML_TO_TEXT) $< > $@

README: README.html
	$(HTML_TO_TEXT) $< > $@

NEWS: NEWS.html
	$(HTML_TO_TEXT) $< > $@

TODO: TODO.html
	$(HTML_TO_TEXT) $< > $@

# The action is the COMPILE definition but always using least warning CFLAGS
@MAINT@rdf_parser_libwww.o: rdf_parser_libwww.c
@MAINT@	$(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(STANDARD_CFLAGS) -c $<

@SET_MAKE@

$(top_builddir)/expat/xmlparse/xmlparse.lo $(top_builddir)/expat/xmlparse/hashtable.lo $(top_builddir)/expat/xmltok/xmlrole.lo $(top_builddir)/expat/xmltok/xmltok.lo:
	cd expat && $(MAKE) $(AM_MAKEFLAGS) all

$(top_builddir)/repat/rdfparse.lo: $(top_builddir)/expat/xmlparse/xmlparse.lo $(top_builddir)/expat/xmlparse/hashtable.lo $(top_builddir)/expat/xmltok/xmlrole.lo $(top_builddir)/expat/xmltok/xmltok.lo
	cd repat && $(MAKE) $(AM_MAKEFLAGS) all

# Copy these by hand since they are symlinks to the latest versions
# in the home raptor source tree and the default rule just copies the
# (broken) links into the distribution which then fails to work.
#
# Note: this can't be protected with @MAINT@ since automake only
# activates this rule if it is normally in the Makefile.am
dist-hook: redland.spec
	@for file in @RAPTOR_SRCS@ @RAPTOR_INCS@ fake.c; do \
          if test -r $(srcdir)/$$file; then \
	    rm -f $(distdir)/$$file; \
	    cp -p $(srcdir)/$$file $(distdir)/$$file; \
	  fi; \
	done

rpm: distcheck
	rpm -ta $(distdir).tar.gz
