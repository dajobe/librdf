noinst_SCRIPTS=update-api-0912.pl

bin_PROGRAMS=redland-db-upgrade rdfproc

man_MANS = redland-db-upgrade.1

EXTRA_DIST=update-api-0912.pl $(man_MANS)

CLEANFILES=*.db

AM_CPPFLAGS=-I$(top_srcdir)/librdf
AM_LDFLAGS=@LIBRDF_LDFLAGS@ @LIBRDF_LIBS@ $(top_builddir)/librdf/librdf.la @LIBRDF_INTERNAL_LIBS@

redland_db_upgrade_SOURCES = db_upgrade.c

rdfproc_SOURCES = rdfproc.c

