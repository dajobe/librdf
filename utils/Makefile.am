MYSQL_UTILS=rdf-tree

bin_PROGRAMS=redland-db-upgrade rdfproc
noinst_SCRIPTS=touch-mtime.pl

if STORAGE_VIRTUOSO
noinst_PROGRAMS=redland-virtuoso-test
endif

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT=redland-db-upgrade 

EXTRA_PROGRAMS=$(MYSQL_UTILS)

man_MANS = redland-db-upgrade.1 rdfproc.1

EXTRA_DIST= rdfproc.html \
$(man_MANS) \
touch-mtime.pl \
fix-groff-xhtml

CLEANFILES=*.db $(EXTRA_PROGRAMS)

AM_CPPFLAGS=@LIBRDF_INTERNAL_CPPFLAGS@ @LIBRDF_CPPFLAGS@ -I$(top_srcdir)/src @LIBRDF_EXTERNAL_CPPFLAGS@
AM_CFLAGS=@CFLAGS@ $(MEM)
LDADD=@LIBRDF_DIRECT_LIBS@ $(top_builddir)/src/librdf.la
AM_LDFLAGS=@LIBRDF_LDFLAGS@

redland_db_upgrade_SOURCES = db_upgrade.c

redland_virtuoso_test_SOURCES = redland-virtuoso-test.c
redland_virtuoso_test_LDADD= @LIBRDF_DIRECT_LIBS@ @LIBRDF_LDFLAGS@ $(top_builddir)/src/librdf.la

rdfproc_SOURCES = rdfproc.c
if GETOPT
rdfproc_SOURCES += getopt.c rdfproc_getopt.h
endif

mysql-utils: $(MYSQL_UTILS)

@MAINT@rdfproc.html: $(srcdir)/rdfproc.1 $(srcdir)/fix-groff-xhtml
@MAINT@	-groff -man -Thtml -P-l $< | tidy -asxml -wrap 1000 2>/dev/null | $(PERL) $(srcdir)/fix-groff-xhtml $@

$(top_builddir)/src/librdf.la:
	cd $(top_builddir)/src && $(MAKE) librdf.la 
