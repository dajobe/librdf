MYSQL_UTILS=rdf-digest rdf-load rdf-tree

noinst_SCRIPTS=update-api-0912.pl

bin_PROGRAMS=redland-db-upgrade rdfproc

EXTRA_PROGRAMS=$(MYSQL_UTILS)

man_MANS = redland-db-upgrade.1 rdfproc.1

EXTRA_DIST=update-api-0912.pl $(man_MANS)

CLEANFILES=*.db $(EXTRA_PROGRAMS)

AM_CPPFLAGS=@CPPFLAGS@ @LIBRDF_CPPFLAGS@ @LIBRDF_INTERNAL_CPPFLAGS@ -I$(top_srcdir)/librdf
AM_CFLAGS=@CFLAGS@ $(MEM)
AM_LDFLAGS=@LIBRDF_LDFLAGS@ @LIBRDF_LIBS@ $(top_builddir)/librdf/librdf.la @LIBRDF_INTERNAL_LIBS@

redland_db_upgrade_SOURCES = db_upgrade.c

rdfproc_SOURCES = rdfproc.c rdfproc_getopt.h
rdfproc_LDADD = @RDFPROC_EXTRA_OBJS@
rdfproc_DEPENDENCIES = @RDFPROC_EXTRA_OBJS@
EXTRA_rdfproc_SOURCES = getopt.c

mysql-utils: $(MYSQL_UTILS)
