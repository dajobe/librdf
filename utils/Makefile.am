noinst_SCRIPTS=update-api-0912.pl

bin_PROGRAMS=redland-db-upgrade

CPPFLAGS=-I$(top_srcdir)/librdf
LDFLAGS=@LIBRDF_LDFLAGS@ $(top_builddir)/librdf/librdf.la $(top_builddir)/raptor/libraptor.la
LIBS=@LIBRDF_LIBS@

redland_db_upgrade_SOURCES = db_upgrade.c
