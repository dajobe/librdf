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
SUBDIRS=@SD@ librdf examples
CONFIG_SUBDIRS=raptor
# Subdirectories to use for distribution
DIST_SUBDIRS=docs perl python tcl java ruby expat repat debian

EXTRA_DIST=ChangeLog \
README      NEWS      TODO      LICENSE.txt \
README.html NEWS.html TODO.html LICENSE.html \
INSTALL.html FAQS.html MPL.html \
Redland.i \
autogen.sh \
Makefile.PL

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

$(SRCDIR)/repat/librepat.la:
	cd $(srcdir)/../repat && $(MAKE) $(AM_MAKEFLAGS) all
