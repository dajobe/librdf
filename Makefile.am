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

bin_SCRIPTS = redland-config
noinst_SCRIPTS = redland-src-config

# Subdirectories to build/install etc.
SUBDIRS=@SD@ repat raptor librdf examples
CONFIG_SUBDIRS=raptor
# Subdirectories to use for distribution
DIST_SUBDIRS=librdf docs examples \
perl python tcl java ruby \
expat repat raptor \
debian

EXTRA_DIST=ChangeLog \
README      NEWS      TODO      LICENSE.txt \
README.html NEWS.html TODO.html LICENSE.html \
INSTALL.html FAQS.html MPL.html \
Redland.i \
autogen.sh \
Makefile.PL \
redland-config.in \
redland-src-config.in

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

@SET_MAKE@

$(top_builddir)/repat/librepat.la:
	cd $(top_builddir)/repat && $(MAKE) $(AM_MAKEFLAGS)

$(top_builddir)/raptor/libraptor.la:
	cd raptor && $(MAKE) $(AM_MAKEFLAGS)

$(top_builddir)/raptor/lib/libraptor.la: $(top_builddir)/raptor/libraptor.la
	cd raptor && $(MAKE) $(AM_MAKEFLAGS) finish LOCAL_LIB_DIR=../lib

$(top_builddir)/librdf/libraptor.la:
	cd librdf && $(MAKE) $(AM_MAKEFLAGS)

$(top_builddir)/librdf/lib/librdf.la: $(top_builddir)/librdf/librdf.la
	cd librdf && $(MAKE) $(AM_MAKEFLAGS) finish LOCAL_LIB_DIR=../lib

finish: $(top_builddir)/raptor/lib/libraptor.la $(top_builddir)/librdf/lib/librdf.la
	rm -rf $(top_builddir)/lib
	mkdir $(top_builddir)/lib
