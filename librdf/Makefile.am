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
#   1. GNU Lesser General Public License (LGPL) Version 2
#   2. GNU General Public License (GPL) Version 2
#   3. Mozilla Public License (MPL) Version 1.1
# and no other versions of those licenses.
# 
# See INSTALL.html or INSTALL.txt at the top of this package for the
# full license terms.
# 


# Subdirectories to build/install etc.
SUBDIRS=@SD@
# Subdirectories to use for distribution
DIST_SUBDIRS=docs perl python tcl expat repat

bin_SCRIPTS = redland-config
lib_LIBRARIES = librdf.a

include_HEADERS = redland.h librdf.h \
rdf_init.h \
rdf_hash.h \
rdf_digest.h \
rdf_types.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_concepts.h \
rdf_model.h rdf_iterator.h \
rdf_storage.h \
rdf_stream.h \
rdf_parser.h rapier.h \
rdf_heuristics.h \
rdf_list.h \
rdf_hash_bdb.h rdf_hash_gdbm.h rdf_hash_memory.h \
rdf_files.h \
rdf_storage_list.h rdf_storage_hashes.h rdf_utf8.h

JAVA_CLASSES=@JAVA_CLASSES@
pkgdata_DATA = $(JAVA_CLASSES)

librdf_a_SOURCES = rdf_init.c \
rdf_uri.c rdf_node.c rdf_statement.c \
rdf_digest.c rdf_hash.c rdf_hash_cursor.c rdf_hash_memory.c \
rdf_model.c rdf_iterator.c rdf_concepts.c \
rdf_list.c rdf_memory.c \
rdf_storage.c rdf_storage_list.c rdf_storage_hashes.c \
rdf_stream.c \
rdf_parser.c rdf_heuristics.c rdf_files.c rdf_utf8.c \
redland.h librdf.h \
rdf_init.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_concepts.h \
rdf_digest.h rdf_hash.h rdf_hash_bdb.h rdf_hash_gdbm.h rdf_hash_memory.h \
rdf_types.h \
rdf_model.h rdf_iterator.h \
rdf_list.h \
rdf_storage.h rdf_storage_list.h rdf_storage_hashes.h \
rdf_stream.h \
rdf_parser.h rapier.h \
rdf_heuristics.h rdf_files.h rdf_utf8.h


librdf_a_LIBADD = @LIBOBJS@ @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ \
@XML_OBJS@
librdf_a_DEPENDENCIES = @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ \
@XML_OBJS@

EXTRA_librdf_a_SOURCES = rdf_hash_gdbm.c rdf_hash_bdb.c \
rdf_digest_md5.c rdf_digest_openssl.c \
rdf_parser_sirpac.c rdf_parser_libwww.c \
rdf_parser_rapier.c rapier_parse.c \
rdf_parser_repat.c \
example1.c example2.c example3.c example4.c

EXTRA_DIST=ChangeLog \
README      NEWS      TODO      LICENSE.txt \
README.html NEWS.html TODO.html LICENSE.html \
INSTALL.html FAQS.html MPL.html \
redland-config.in \
memcmp.c Redland.i \
PrintConsumer.java PrintParser.java \
$(JAVA_CLASSES) \
autogen.sh \
redland.spec.in

TESTS=rdf_node_test rdf_digest_test rdf_hash_test rdf_uri_test \
rdf_statement_test rdf_model_test rdf_storage_test rdf_parser_test \
rdf_files_test rdf_heuristics_test rdf_utf8_test rdf_concepts_test

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

rdf_utf8_test: rdf_utf8.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_utf8.c librdf.a $(LIBS)

rdf_concepts_test: rdf_concepts.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_concepts.c librdf.a $(LIBS)

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

$(top_builddir)/expat/xmlparse/xmlparse.o $(top_builddir)/expat/xmlparse/hashtable.o $(top_builddir)/expat/xmltok/xmlrole.o $(top_builddir)/expat/xmltok/xmltok.o:
	cd expat && $(MAKE) $(AM_MAKEFLAGS) all

$(top_builddir)/repat/rdfparse.o: $(top_builddir)/expat/xmlparse/xmlparse.o $(top_builddir)/expat/xmlparse/hashtable.o $(top_builddir)/expat/xmltok/xmlrole.o $(top_builddir)/expat/xmltok/xmltok.o
	cd repat && $(MAKE) $(AM_MAKEFLAGS) all

# Copy these by hand since they are symlinks to the latest versions
# in the hom erapier source tree and the default rule just copies the
# (broken) links into the distribution which then fails to work.
#
# Note: this can't be protected with @MAINT@ since automake only
# activates this rule if it is normally in the Makefile.am
dist-hook:
	@for file in rapier_parse.c rapier.h; do \
	  rm -f $(distdir)/$$file; \
	  cp -p $(srcdir)/$$file $(distdir)/$$file; \
	done
