# -*- Mode: Makefile -*-
#
# Makefile.am - example automake file for Redland
#
# $Id$
#

EXAMPLES=example1 example2 example3 example4

# Need this so that automake doesn't moan
noinst_PROGRAMS = $(EXAMPLES)

CLEANFILES=$(TESTS) $(EXAMPLES) test.db test1.db

examples: $(EXAMPLES)

example1_SOURCES = example1.c
example1_LDADD = $(srcdir)/../librdf/librdf.la $(srcdir)/../raptor/libraptor.la
example1_LDFLAGS = -static

example2_SOURCES = example2.c
example2_LDADD = $(srcdir)/../librdf/librdf.la $(srcdir)/../raptor/libraptor.la
example2_LDFLAGS = -static

example3_SOURCES = example3.c
example3_LDADD = $(srcdir)/../librdf/librdf.la $(srcdir)/../raptor/libraptor.la
example3_LDFLAGS = -static

example4_SOURCES = example4.c
example4_LDADD = $(srcdir)/../librdf/librdf.la $(srcdir)/../raptor/libraptor.la
example4_LDFLAGS = -static
