MYSQL_UTILS=rdf-tree

bin_PROGRAMS=redland-db-upgrade rdfproc

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT=redland-db-upgrade

EXTRA_PROGRAMS=$(MYSQL_UTILS)

man_MANS = redland-db-upgrade.1 rdfproc.1

EXTRA_DIST= rdfproc.html \
$(man_MANS)

CLEANFILES=*.db $(EXTRA_PROGRAMS)

AM_CPPFLAGS=@CPPFLAGS@ @LIBRDF_CPPFLAGS@ @LIBRDF_INTERNAL_CPPFLAGS@ -I$(top_srcdir)/librdf @LIBRDF_EXTERNAL_CPPFLAGS@
AM_CFLAGS=@CFLAGS@ $(MEM)
AM_LDFLAGS=@LIBRDF_LDFLAGS@ @LIBRDF_LIBS@ $(top_builddir)/librdf/librdf.la @LIBRDF_INTERNAL_LIBS@ @LIBRDF_EXTERNAL_LIBS@

redland_db_upgrade_SOURCES = db_upgrade.c

rdfproc_SOURCES = rdfproc.c
if GETOPT
rdfproc_SOURCES += getopt.c rdfproc_getopt.h
endif

mysql-utils: $(MYSQL_UTILS)

@MAINT@rdfproc.html: $(srcdir)/rdfproc.1 $(top_srcdir)/raptor/fix-groff-xhtml
@MAINT@	-groff -man -Thtml -P-l $< | tidy -asxml -wrap 1000 2>/dev/null | $(PERL) $(top_srcdir)/raptor/fix-groff-xhtml $@

