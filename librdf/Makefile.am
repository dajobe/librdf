#
# automake-file for librdf
#
# $Source$
# $Id$
#
#

noinst_LIBRARIES = librdf.a

librdf_a_SOURCES = rdf_init.c rdf_uri.c rdf_node.c rdf_statement.c rdf_digest.c \
rdf_hash.c rdf_hash_list.c rdf_model.c \
rdf_config.h \
rdf_uri.h rdf_node.h rdf_statement.h rdf_digest.h rdf_hash.h rdf_hash_gdbm.h \
rdf_hash_list.h rdf_model.h rdf_storage.h rdf_iterator.h \
rdf_assertion_context.h \
bithelp.h types.h ripemd160.h

librdf_a_LIBADD = @DIGEST_OBJS@ @HASH_OBJS@
librdf_a_DEPENDENCIES = @DIGEST_OBJS@ @HASH_OBJS@
EXTRA_librdf_a_SOURCES = md5.c sha1.c ripemd160.c rdf_hash_gdbm.c \
rdf_digest_openssl.c

EXTRA_DIST=ChangeLog

TESTS=rdf_node_test rdf_digest_test rdf_hash_test rdf_statement_test \
rdf_model_test

CLEANFILES=$(TESTS) test.gdbm

CFLAGS=-g -Wall -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls

rdf_node_test: rdf_node.c librdf.a
	$(LINK) -I$(srcdir) -I. -DRDF_NODE_TEST $(srcdir)/rdf_node.c librdf.a $(LIBS)

rdf_digest_test: rdf_digest.c librdf.a
	$(LINK) -I$(srcdir) -I. -DRDF_DIGEST_TEST $(srcdir)/rdf_digest.c librdf.a $(LIBS)

rdf_hash_test: rdf_hash.c librdf.a
	$(LINK) -I$(srcdir) -I. -DRDF_HASH_TEST $(srcdir)/rdf_hash.c librdf.a $(LIBS)

rdf_statement_test: rdf_statement.c librdf.a
	$(LINK) -I$(srcdir) -I. -DRDF_STATEMENT_TEST $(srcdir)/rdf_statement.c librdf.a $(LIBS)

rdf_model_test: rdf_model.c librdf.a
	$(LINK) -I$(srcdir) -I. -DRDF_MODEL_TEST $(srcdir)/rdf_model.c librdf.a $(LIBS)

@SET_MAKE@
