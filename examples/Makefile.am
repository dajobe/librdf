# -*- Mode: Makefile -*-
#
# Makefile.am - example automake file for Redland
#
# $Id$
#

EXAMPLES=example1 example2 example3 example4
DBUS_EXAMPLES=redland_dbus
GUI_EXAMPLES=

# Need this so that automake doesn't moan
noinst_PROGRAMS = $(EXAMPLES)
EXTRA_PROGRAMS = $(DBUS_EXAMPLES) $(GUI_EXAMPLES)

CLEANFILES=$(TESTS) $(EXAMPLES) $(DBUS_EXAMPLES) $(GUI_EXAMPLES) test*db

examples: $(EXAMPLES)

AM_CPPFLAGS=-I$(top_srcdir)/librdf
AM_LDFLAGS=@LIBRDF_LDFLAGS@ @LIBRDF_LIBS@ $(top_builddir)/librdf/librdf.la @LIBRDF_INTERNAL_LIBS@

example1_SOURCES = example1.c

example2_SOURCES = example2.c

example3_SOURCES = example3.c

example4_SOURCES = example4.c


redland_dbus_SOURCES = redland_dbus.c
redland_dbus_LDADD=$(top_builddir)/librdf/librdf.la $(top_builddir)/raptor/libraptor.la
redland_dbus_CFLAGS=$(shell pkg-config --cflags dbus-1 glib-2.0 dbus-glib-1 gthread)
redland_dbus_LDFLAGS=$(shell pkg-config --libs dbus-1 glib-2.0 dbus-glib-1 gthread)
redland_dbus_DEPENDENCIES = @LIBRDF_LDFLAGS@ $(top_builddir)/librdf/librdf.la $(top_builddir)/raptor/libraptor.la

# redland_gui_SOURCES = redland_gui.c
# redland_gui_LDADD=$(top_builddir)/librdf/librdf.la $(top_builddir)/raptor/libraptor.la
# redland_gui_CFLAGS=$(shell pkg-config --cflags gtk+-2.0)
# redland_gui_LDFLAGS=$(shell pkg-config --libs gtk+-2.0)
# redland_gui_DEPENDENCIES = @LIBRDF_LDFLAGS@ $(top_builddir)/librdf/librdf.la $(top_builddir)/raptor/libraptor.la
