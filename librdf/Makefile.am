#
# automake-file for librdf
#
# $Source$
# $Id$
#
#

SUBDIRS=@SD@

lib_LIBRARIES = librdf.a

librdf_a_SOURCES = rdf_init.c rdf_uri.c rdf_node.c rdf_statement.c \
rdf_digest.c rdf_hash.c rdf_hash_list.c rdf_model.c rdf_iterator.c \
rdf_list.c rdf_memory.c rdf_storage.c rdf_storage_list.c rdf_stream.c \
rdf_parser.c \
librdf.h rdf_config.h rdf_types.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_digest.h rdf_hash.h rdf_hash_gdbm.h \
rdf_hash_list.h rdf_model.h rdf_storage.h rdf_iterator.h \
rdf_context.h rdf_list.h rdf_hash_bdb.h rdf_storage.h rdf_storage_list.h \
rdf_stream.h rdf_parser.h \
example.c

librdf_a_LIBADD = @LIBOBJS@ @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@ 
librdf_a_DEPENDENCIES = @DIGEST_OBJS@ @HASH_OBJS@ @PARSER_OBJS@

EXTRA_librdf_a_SOURCES = md5.c rdf_hash_gdbm.c rdf_digest_openssl.c \
rdf_hash_bdb.c

EXTRA_DIST=ChangeLog \
README      NEWS      TODO \
README.html NEWS.html TODO.html \
INSTALL.html LICENSE.html FAQS.html \
librdf-config.in \
memcmp.c

TESTS=rdf_node_test rdf_digest_test rdf_hash_test rdf_statement_test \
rdf_model_test rdf_storage_test

CLEANFILES=$(TESTS) test.gdbm test.db

# Why is this not in the default makefile?
CC=@CC@

CFLAGS=-Wall
@MAINT@ CFLAGS=-DLIBRDF_DEBUG=1 -g -Wall -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls



# Rules to construct test programs

rdf_node_test: rdf_node.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_node.c librdf.a $(LIBS)

rdf_digest_test: rdf_digest.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_digest.c librdf.a $(LIBS)

rdf_hash_test: rdf_hash.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_hash.c librdf.a $(LIBS)

rdf_statement_test: rdf_statement.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_statement.c librdf.a $(LIBS)

rdf_model_test: rdf_model.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_model.c librdf.a $(LIBS)

rdf_storage_test: rdf_storage.c librdf.a
	$(LINK) -I$(srcdir) -I. -DSTANDALONE $(srcdir)/rdf_storage.c librdf.a $(LIBS)

example: example.c librdf.a
	$(LINK) -I$(srcdir) -I. $(srcdir)/example.c librdf.a $(LIBS)



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

@SET_MAKE@
