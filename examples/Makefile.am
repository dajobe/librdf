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

CPPFLAGS=-I$(top_srcdir)/librdf
LDFLAGS=@LIBRDF_LDFLAGS@ $(top_builddir)/librdf/librdf.la $(top_builddir)/raptor/libraptor.la
LIBS=@LIBRDF_LIBS@

example1_SOURCES = example1.c

example2_SOURCES = example2.c

example3_SOURCES = example3.c

example4_SOURCES = example4.c
