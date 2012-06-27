MYSQL_UTILS=rdf-tree

bin_PROGRAMS=redland-db-upgrade rdfproc

if STORAGE_VIRTUOSO
noinst_PROGRAMS=redland-virtuoso-test
endif

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT=redland-db-upgrade 

EXTRA_PROGRAMS=$(MYSQL_UTILS)

man_MANS = redland-db-upgrade.1 rdfproc.1

EXTRA_DIST= rdfproc.html \
$(man_MANS) \
fix-groff-xhtml

CLEANFILES=*.db $(EXTRA_PROGRAMS) *.plist

AM_CPPFLAGS=@LIBRDF_INTERNAL_CPPFLAGS@ @LIBRDF_CPPFLAGS@ -I$(top_srcdir)/src @LIBRDF_EXTERNAL_CPPFLAGS@
LDADD=@LIBRDF_DIRECT_LIBS@ $(top_builddir)/src/librdf.la
AM_LDFLAGS=@LIBRDF_LDFLAGS@

redland_db_upgrade_SOURCES = db_upgrade.c

redland_virtuoso_test_SOURCES = redland-virtuoso-test.c
redland_virtuoso_test_LDADD= @LIBRDF_DIRECT_LIBS@ @LIBRDF_LDFLAGS@ $(top_builddir)/src/librdf.la

rdfproc_SOURCES = rdfproc.c
if GETOPT
rdfproc_SOURCES += getopt.c rdfproc_getopt.h
endif

ANALYZE = clang
ANALYZE_FLAGS = "--analyze"
# Based on COMPILE target
ANALYZE_COMMAND = $(ANALYZE) \
        $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
        $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) \
        $(ANALYZE_FLAGS)

mysql-utils: $(MYSQL_UTILS)

@MAINT@rdfproc.html: $(srcdir)/rdfproc.1 $(srcdir)/fix-groff-xhtml
@MAINT@	-groff -man -Thtml -P-l $< | tidy -asxml -wrap 1000 2>/dev/null | $(PERL) $(srcdir)/fix-groff-xhtml $@

$(top_builddir)/src/librdf.la:
	cd $(top_builddir)/src && $(MAKE) librdf.la 

if MAINTAINER_MODE
# Run Clang static analyzer over sources.
analyze: $(SOURCES)
	@list='$(SOURCES)'; \
	result=0; \
	for file in $$list; do \
	  if echo $$file | grep '\.c$$' >/dev/null 2>&1; then \
	    $(RECHO) "Analyzing $$file"; \
	    $(ANALYZE_COMMAND) $(srcdir)/$$file; \
	    status=$$?; \
	    if test $$status != 0; then \
	      result=1; \
	    fi; \
	  fi; \
	done; \
	set -e; exit $$result
endif
