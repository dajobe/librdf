#
# automake file for librdf documentation
#
# $Source$
# $Id$
#
#

EXTRA_DIST=api.html

# Create some text files from HTML sources
LYNX=lynx
LYNXARGS=-dump -nolist

SUFFIXES = .html .txt

.html.txt:
	$(LYNX) $(LYNXARGS) $< > $@

@SET_MAKE@
