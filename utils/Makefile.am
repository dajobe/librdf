MYSQL_UTILS=rdf-tree

bin_PROGRAMS=redland-db-upgrade rdfproc

EXTRA_PROGRAMS=$(MYSQL_UTILS)

man_MANS = redland-db-upgrade.1 rdfproc.1

EXTRA_DIST= rdfproc.html \
fix-groff-xhtml \
$(man_MANS)

CLEANFILES=*.db $(EXTRA_PROGRAMS)

AM_CPPFLAGS=@CPPFLAGS@ @LIBRDF_CPPFLAGS@ @LIBRDF_INTERNAL_CPPFLAGS@ -I$(top_srcdir)/librdf @LIBRDF_EXTERNAL_CPPFLAGS@
AM_CFLAGS=@CFLAGS@ $(MEM)
AM_LDFLAGS=@LIBRDF_LDFLAGS@ @LIBRDF_LIBS@ $(top_builddir)/librdf/librdf.la @LIBRDF_INTERNAL_LIBS@ @LIBRDF_EXTERNAL_LIBS@

redland_db_upgrade_SOURCES = db_upgrade.c

rdfproc_SOURCES = rdfproc.c rdfproc_getopt.h
rdfproc_LDADD = @RDFPROC_EXTRA_OBJS@
rdfproc_DEPENDENCIES = @RDFPROC_EXTRA_OBJS@
EXTRA_rdfproc_SOURCES = getopt.c

mysql-utils: $(MYSQL_UTILS)

@MAINT@rdfproc.html: $(srcdir)/rdfproc.1
@MAINT@	-groff -man -Thtml -P-l $< | tidy -asxml -wrap 1000 2>/dev/null | perl $(srcdir)/fix-groff-xhtml $@

