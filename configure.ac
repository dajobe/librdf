dnl -*- Mode: autoconf -*-
dnl
dnl configure.ac - top level autoconf file for Redland librdf
dnl (Process this file with autoconf to produce a configure script.)
dnl
dnl Copyright (C) 2000-2012 David Beckett http://www.dajobe.org/
dnl Copyright (C) 2000-2005 University of Bristol, UK http://www.bristol.ac.uk/
dnl 
dnl This package is Free Software and part of Redland http://librdf.org/
dnl 
dnl It is licensed under the following three licenses as alternatives:
dnl   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
dnl   2. GNU General Public License (GPL) V2 or any newer version
dnl   3. Apache License, V2.0 or any newer version
dnl 
dnl You may not use this file except in compliance with at least one of
dnl the above three licenses.
dnl 
dnl See LICENSE.html or LICENSE.txt at the top of this package for the
dnl complete terms and further detail along with the license texts for
dnl the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
dnl 
dnl 


AC_INIT([Redland librdf RDF API Library],[1.0.17],[http://bugs.librdf.org/],[redland])
AC_PREREQ([2.62])
AC_CONFIG_SRCDIR(src/rdf_node.c)
AC_REVISION($Revision$)dnl
AC_CONFIG_HEADERS([src/rdf_config.h])
AC_CONFIG_AUX_DIR(build)
AC_CONFIG_MACRO_DIR(build)
AM_INIT_AUTOMAKE([1.11 check-news std-options -Wobsolete -Wportability -Wsyntax -Wunsupported -Wextra-portability])
AM_MAINTAINER_MODE

AC_CANONICAL_HOST


changequote(<<, >>)dnl
version_major=`echo $VERSION | sed -e 's/^\([^\.]*\)\.\([^\.]*\)\.\(.*\)$/\1/'`
version_minor=`echo $VERSION | sed -e 's/^\([^\.]*\)\.\([^\.]*\)\.\(.*\)$/\2/'`
version_release=`echo $VERSION | sed -e 's/^\([^\.]*\)\.\([^\.]*\)\.\(.*\)$/\3/'`
changequote([, ])dnl

version_decimal=`expr $version_major \* 10000 + $version_minor \* 100 + $version_release`

AC_DEFINE_UNQUOTED(LIBRDF_VERSION_MAJOR, $version_major, [Major version number])
AC_DEFINE_UNQUOTED(LIBRDF_VERSION_MINOR, $version_minor, [Minor version number])
AC_DEFINE_UNQUOTED(LIBRDF_VERSION_RELEASE, $version_release, [Release version number])
AC_DEFINE_UNQUOTED(LIBRDF_VERSION_DECIMAL, $version_decimal, [Release version as a decimal])

# for redland-config.in
LIBRDF_VERSION_MAJOR=$version_major
LIBRDF_VERSION_MINOR=$version_minor
LIBRDF_VERSION_RELEASE=$version_release
LIBRDF_VERSION_DECIMAL=$version_decimal
AC_SUBST(LIBRDF_VERSION_MAJOR)
AC_SUBST(LIBRDF_VERSION_MINOR)
AC_SUBST(LIBRDF_VERSION_RELEASE)
AC_SUBST(LIBRDF_VERSION_DECIMAL)


# Libtool versioning
#
# CURRENT
#      The most recent interface number that this library implements.
# 
# REVISION
#      The implementation number of the CURRENT interface.
# 
# AGE
#      The difference between the newest and oldest interfaces that this
#      library implements.  In other words, the library implements all the
#      interface numbers in the range from number `CURRENT - AGE' to
#      `CURRENT'.
#
# Rules:
#  1. Start with version information of `0:0:0' for each libtool library.
#
#  2. Update the version information only immediately before a public
#     release of your software.  More frequent updates are unnecessary,
#     and only guarantee that the current interface number gets larger
#     faster.
#
#  3. If the library source code has changed at all since the last
#     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed since the last public release,
#     then set AGE to 0.
#
# syntax: CURRENT[:REVISION[:AGE]]
LIBRDF_LIBTOOL_VERSION=0:0:0
AC_SUBST(LIBRDF_LIBTOOL_VERSION)

dnl Checks for programs.
AM_SANITY_CHECK
AM_PROG_AR
AC_PROG_CC
AM_PROG_CC_C_O
AC_MSG_CHECKING(whether $CC is clang)
CC_IS_CLANG=no
if $CC 2>&1 | grep clang >/dev/null 2>&1; then
  CC_IS_CLANG=yes
else
  :
fi
AC_MSG_RESULT($CC_IS_CLANG)
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

m4_undefine([AC_PROG_CXX])
m4_defun([AC_PROG_CXX],[])
m4_undefine([AC_PROG_F77])
m4_defun([AC_PROG_F77],[])

# Libtool and LTDL initialising
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen])
LTDL_INIT([convenience])

# Find a tar command for 'make dist'
AC_CHECK_PROGS(TAR, gnutar gtar tar)

AC_CHECK_PROGS(PERL, perl)
# Gnome
PKG_PROG_PKG_CONFIG


AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)

AC_CHECK_PROGS(RECHO, echo)
RECHO_C=
RECHO_N=
case `$RECHO -n x` in
  -n*)
    case `$RECHO 'xy\c'` in
      *c*)
         ;;
      xy)
         RECHO_C='\c'
         ;;
    esac;;
  *)
    RECHO_N='-n'
    ;;
esac


release_version=no
AC_ARG_ENABLE(release, [  --enable-release        Turn on optimizations (for maintainer).  ], \
 if test "$enableval" = "yes"; then \
	  release_version=yes
fi;)	

modular=yes
AC_ARG_ENABLE(modular, [  --disable-modular       Build storage backends into librdf library.  ], \
 if test "$enableval" = "no"; then \
	  modular=no
fi;)	

dnl compiler checks

AC_DEFUN([REDLAND_CC_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])
  redland_save_CFLAGS="$CFLAGS"
  if test $CC_IS_CLANG = yes; then
    # Make unknown warnings into errors for clang so the test below works
    CFLAGS="-Werror $CFLAGS"
  fi
  CFLAGS="$CFLAGS $1"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([ ])], [redland_cc_flag=yes], [redland_cc_flag=no])
  CFLAGS="$redland_save_CFLAGS"
  if test "X$redland_cc_flag" = "Xyes"; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
  AC_MSG_RESULT($redland_cc_flag)
])

# GCC warning options
# http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
#
# Too noisy:
#  -Wconversion: 
#  -Wformat-nonliteral : variables for format strings
#  -Wno-system-headers : not debugging system
#  -Wunused-parameter : variables can be marked  __attribute__('unused')
#
# Apple gcc specific (probably): -Wshorten-64-to-32
possible_warnings="\
-Wall \
-Wc++-compat \
-Wextra \
-Wunused \
\
-Waggregate-return \
-Wbad-function-cast \
-Wcast-align \
-Wdeclaration-after-statement \
-Wdisabled-optimization \
-Wdiv-by-zero \
-Wendif-labels \
-Werror-implicit-function-declaration \
-Wfloat-equal \
-Wformat-security \
-Wframe-larger-than=4096 \
-Winit-self \
-Winline \
-Wmissing-declarations \
-Wmissing-field-initializers \
-Wmissing-format-attribute \
-Wmissing-noreturn \
-Wmissing-prototypes \
-Wnested-externs \
-Wold-style-definition \
-Wpacked \
-Wpointer-arith \
-Wredundant-decls \
-Wshadow \
-Wsign-compare \
-Wsign-conversion \
-Wstrict-prototypes \
-Wswitch-enum \
-Wundef \
-Wunsafe-loop-optimizations \
-Wwrite-strings \
\
-Wshorten-64-to-32
\
-Wno-conversion \
-Wno-format-nonliteral \
-Wno-system-headers \
-Wno-unused-parameter \
"

warning_cflags=
if test "$USE_MAINTAINER_MODE" = yes; then
  AC_MSG_CHECKING(for supported $CC warning flags)
  AC_MSG_RESULT($warning_cflags)
  for warning in $possible_warnings; do
    REDLAND_CC_TRY_FLAG([$warning], [warning_cflags="$warning_cflags $warning"])
  done
  AC_MSG_CHECKING($CC supports warning flags)
  AC_MSG_RESULT($warning_cflags)
fi

MAINTAINER_CPPFLAGS="$warning_cflags"


# Externally linked libraries - appear in redland-config
# -Dfoo -Idir
LIBRDF_CPPFLAGS=$CPPFLAGS
# -Ldir
LIBRDF_LDFLAGS=$LDFLAGS
# -llib
LIBRDF_LIBS=$LIBS

# Internally linked libraries - never given to users
# -Dfoo -Idir
LIBRDF_INTERNAL_CPPFLAGS=
# -Ldir -llib
LIBRDF_INTERNAL_LIBS=
# Internal dependencies like libltdl when built locally
LIBRDF_INTERNAL_DEPS=

# Libraries linked externally also may be internal - presently just
# raptor and rasqal but in future may include others
# Currently only substituted into redland-src-config.
# -Dfoo -Idir
LIBRDF_EXTERNAL_CPPFLAGS=
# -Ldir -llib
LIBRDF_EXTERNAL_LIBS=

# do these first because somehow they get messed up by bdb checks
AC_CHECK_TYPES([byte])
AC_CHECK_TYPES([u32])
AC_CHECK_TYPES([u64])

AC_CHECK_SIZEOF(unsigned char, 1)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)
AC_CHECK_SIZEOF(unsigned long long, 8)
  
dnl Checks for libraries.


dnl Oracle Berkeley DB
AC_ARG_WITH(bdb, [  --with-bdb=DIR          Oracle Berkeley DB install area (default=/usr)], bdb_prefix="$withval", bdb_prefix="none") 
AC_ARG_WITH(bdb_lib, [  --with-bdb-lib=DIR      Oracle Berkeley DB lib directory (default=/usr/lib)], bdb_lib_dir="$withval", bdb_lib_dir="none") 
AC_ARG_WITH(bdb_include, [  --with-bdb-include=DIR  Oracle Berkeley DB include directory (default=/usr/include)], bdb_include_dir="$withval", bdb_include_dir="none") 
AC_ARG_WITH(bdb_dbname, [  --with-bdb-dbname=NAME  Oracle Berkeley DB library name (auto)], bdb_dbname="$withval", bdb_dbname="none")

bdb_version=unknown
bdb_available=Missing

# debian multiarch
multiarch=`dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null`

if test "x$bdb_prefix" != "xno" ; then

  if test "x$bdb_prefix" = "xyes" ; then
    bdb_prefix="none"
  fi

  # Only prefix given - set lib and include
  if test "X$bdb_prefix" != "Xnone" -a "X$bdb_lib_dir" = "Xnone" -a "X$bdb_include_dir" = "Xnone" ; then
    bdb_lib_dir="$bdb_prefix/lib"
    bdb_include_dir="$bdb_prefix/include"
  fi

  # Nothing given - search
  if test "X$bdb_prefix" = "Xnone" -a "X$bdb_lib_dir" = "Xnone" -a "X$bdb_include_dir" = "Xnone" -a "X$bdb_dbname" = "Xnone" ; then
    bdb_prefix="/usr"
    bdb_lib_dir="/usr/lib"
    bdb_include_dir="/usr/include"

    extraprefix=
    # OSX fink area added if found and /sw/bin is in the PATH
    if test -d /sw/include -a -d /sw/lib ; then
      if echo $PATH |grep /sw/bin >/dev/null; then
	extraprefix=/sw
      fi
    fi

    # Sigh!  And this still might not be good enough.
    for bdbc_version in 5.3 5.2 5.1 5.0 4.9 4.8 4.7 4.6 4.5 4.4 4.3 4.2 4.1 4.0 4 3.3 3.2 3.1 3 2; do
      bdbc_major=`echo $bdbc_version | sed -e 's/^\(.\).*$/\1/'`;
      bdbc_version2=`echo $bdbc_version | sed -e 's/^\(.\).\(.\)$/\1\2/'`

      for bdbc_prefix in $extraprefix /usr/local/BerkeleyDB.$bdbc_version /usr/local /opt/local /usr; do
	bdbc_libdir=$bdbc_prefix/lib

	for bdbc_incdir in $bdbc_prefix/include/db$bdbc_version $bdbc_prefix/include/db$bdbc_version2 $bdbc_prefix/include/db$bdbc_major $bdbc_prefix/include ; do
	  if test -r $bdbc_incdir/db.h ; then
	    header_bdbc_version=`sed -ne 's/^.*DB_VERSION_STRING.*Berkeley DB \(...\).*/\1/p' $bdbc_incdir/db.h`
	    if test "X$header_bdbc_version" = "X$bdbc_version" ; then 
	      for bdbc_name in db-$bdbc_version db$bdbc_version db-$bdbc_version2 db$bdbc_version2; do
	        bdbc_libdir_multiarch=
	        if test "X$multiarch" != X; then
	          bdbc_libdir_multiarch=$bdbc_libdir/$multiarch
	        fi
	        for bdbc_libdir2 in $bdbc_libdir_multiarch $bdbc_libdir $bdbc_libdir/db$bdbc_version $bdbc_libdir/db$bdbc_version2; do
		  if test -r $bdbc_libdir2/lib$bdbc_name.a -o -r $bdbc_libdir2/lib$bdbc_name.so -o -r $bdbc_libdir2/lib$bdbc_name.dylib; then
		    bdb_version=$bdbc_version
		    bdb_prefix=$bdbc_prefix
		    bdb_include_dir=$bdbc_incdir
		    bdb_lib_dir=$bdbc_libdir2
		    bdb_dbname=$bdbc_name
		    break 5
		  fi
	        done
	      done
	    fi
	  fi

	done
       done
     done

  fi

  AC_MSG_CHECKING([Oracle Berkeley DB (BDB) version])
  AC_MSG_RESULT($bdb_version)
  AC_MSG_CHECKING(location of BDB libs)
  AC_MSG_RESULT($bdb_lib_dir)
  AC_MSG_CHECKING(location of BDB includes)
  AC_MSG_RESULT($bdb_include_dir)

  if test "$bdb_prefix" != no; then
    nLDFLAGS=$LDFLAGS
    nCPPFLAGS=

    if test "$bdb_lib_dir" != /usr/lib; then
      nLDFLAGS="-L$bdb_lib_dir";
    fi

    if test "$bdb_dbname" = none; then
      # Yuck, yuck, yuck - can't they create a bdb-config program like GNOME?
      dnl need to change quotes to allow square brackets
      changequote(<<, >>)dnl
      bdb_dbname=`cd $bdb_lib_dir; ls -1 libdb[-0-9.]*.a libdb[-0-9.]*.{so,dylib}* 2>/dev/null | sed -e 's/^lib//' -e 's/\.a$//' -e 's/\.so.*$//' -e 's/\.dylib.*$//' | head -1`
      changequote([, ])dnl
      if test "X$bdb_dbname" = X; then
	AC_MSG_WARN(Cannot find the BDB library name from the files in $bdb_lib_dir)
	AC_MSG_WARN(Assuming it is 'db' and will link like -ldb)
	AC_MSG_WARN(Use --with-bdb-dbname=NAME if this is wrong)
	bdb_dbname=db
      fi
    fi

    AC_MSG_CHECKING(name of BDB library)
    AC_MSG_RESULT($bdb_dbname)

    if test "$bdb_include_dir" != /usr/include; then
      nCPPFLAGS="-I$bdb_include_dir";
    fi

    LDFLAGS="$nLDFLAGS $LDFLAGS"
    CPPFLAGS="$nCPPFLAGS $CPPFLAGS"
    LIBS="-l$bdb_dbname $LIBS"

    AC_CHECK_HEADERS(db.h)
    have_libdb=no
    if test "$ac_cv_header_db_h" = yes ; then

      AC_MSG_CHECKING(for BDB V1 dbopen)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[dbopen(NULL, 0, 0, DB_BTREE, NULL);]])],[AC_DEFINE(HAVE_DBOPEN, 1, BDB has dbopen method)
		  if test $have_libdb = no; then
		    bdb_available="Version 1.x - not supported"
		    have_libdb=no
		  fi
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

      AC_MSG_CHECKING(for BDB V2 db_open)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[db_open(NULL, DB_BTREE, 0, 0, NULL, NULL, NULL);]])],[AC_DEFINE(HAVE_DB_OPEN, 1, BDB has db_open method)
		  have_libdb=yes
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

      AC_MSG_CHECKING(for BDB V3/V4.x db_create)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[db_create(NULL, NULL, 0);]])],[AC_DEFINE(HAVE_DB_CREATE, 1, BDB has db_create method)
		  have_libdb=yes
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

      have_bdb_open_7_args=no
      AC_MSG_CHECKING(for BDB V4.1+ DB->open with 7 arguments)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[DB *bdb; bdb->open(bdb, NULL, NULL, NULL, DB_BTREE, 0, 0);]])],[AC_DEFINE(HAVE_BDB_OPEN_7_ARGS, 1, BDB has open method with 7 args)
		  have_libdb=yes
		  have_bdb_open_7_args=yes
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

      if test "X$have_bdb_open_7_args" = Xno; then
        # Cannot have 6 args if 7 args worked
	AC_MSG_CHECKING(for BDB V4.0 DB->open with 6 arguments)
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		     #include <db.h>]], [[DB *bdb; bdb->open(bdb, NULL, NULL, DB_BTREE, 0, 0);]])],[AC_DEFINE(HAVE_BDB_OPEN_6_ARGS, 1, BDB has open method with 6 args)
		    have_libdb=yes
		    AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])
      fi

      AC_MSG_CHECKING(for BDB DB->close with 2 arguments)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[DB *bdb; bdb->close(bdb, 0);]])],[AC_DEFINE(HAVE_BDB_CLOSE_2_ARGS, 1, BDB has close method with 2 args)
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

      AC_MSG_CHECKING(for BDB DB->fd with 2 arguments)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[DB *bdb; bdb->fd(bdb, 0);]])],[AC_DEFINE(HAVE_BDB_FD_2_ARGS, 1, BDB has fd method with 2 args)
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

      AC_MSG_CHECKING(for BDB DB->set_flags)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[DB *bdb; bdb->set_flags(bdb, 0);]])],[AC_DEFINE(HAVE_BDB_SET_FLAGS, 1, BDB has set_flags method)
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

      AC_MSG_CHECKING(whether DB_TXN defined in db.h)
      dnl In BDB, DB_TXN is a pointer to a structure never defined
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[DB_TXN* ptr=(DB_TXN*)NULL]])],[AC_DEFINE(HAVE_BDB_DB_TXN, 1, BDB defines DB_TXN)
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])
      AC_MSG_CHECKING(whether DBC defined in db.h)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <db.h>]], [[size_t len=sizeof(DBC)]])],[AC_DEFINE(HAVE_BDB_CURSOR, 1, BDB defines DBC)
		  AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])
      AC_MSG_CHECKING(number of arguments to db_cursor)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
		   #include <db.h>]], [[DB* db; db->cursor(db, NULL, NULL, 0);]])],[AC_DEFINE(HAVE_BDB_CURSOR_4_ARGS, 1, BDB cursor method has 4 arguments)
		  AC_MSG_RESULT(4)],[AC_MSG_RESULT(3)])
    fi

    if test "X$have_libdb" = Xyes; then
      bdb_available="Version $bdb_version (library $bdb_dbname in $bdb_lib_dir)"
      LIBRDF_LIBS="$LIBRDF_LIBS $nLDFLAGS -l$bdb_dbname"
      LIBRDF_CPPFLAGS="$LIBRDF_CPPFLAGS $nCPPFLAGS"
    fi

  fi

fi

CPPFLAGS="$LIBRDF_CPPFLAGS"
LDFLAGS="$LIBRDF_LDFLAGS"
LIBS="$LIBRDF_LIBS"


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(errno.h stdlib.h unistd.h string.h fcntl.h time.h sys/time.h sys/stat.h getopt.h stddef.h)
AC_HEADER_TIME

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN

dnl Checks for library functions.
AC_CHECK_FUNCS(getopt getopt_long memcmp mkstemp mktemp tmpnam gettimeofday getenv)

AM_CONDITIONAL(MEMCMP, test $ac_cv_func_memcmp = no)
AM_CONDITIONAL(GETOPT, test $ac_cv_func_getopt = no -a $ac_cv_func_getopt_long = no)

dnl Checks for URI resolvers

CPPFLAGS="$LIBRDF_CPPFLAGS"
LDFLAGS="$LIBRDF_LDFLAGS"
LIBS="$LIBRDF_LIBS"


dnl Checks for RDF parsers
parser_modules=raptor
rdf_parsers_available=

# Check for rasqal before raptor so that we can see if rasqal was already
# built with raptor2 and can then enforce raptor2 is required

RASQAL_MIN_VERSION=0.9.25
RASQAL_MAX_VERSION=0.9.99
AC_SUBST(RASQAL_MIN_VERSION)
AC_SUBST(RASQAL_MAX_VERSION)
PKG_CHECK_MODULES([RASQAL],[rasqal >= $RASQAL_MIN_VERSION rasqal <= $RASQAL_MAX_VERSION],[
  RASQAL_VERSION=`$PKG_CONFIG rasqal --modversion 2>/dev/null`
],[
  AC_MSG_ERROR(Rasqal is not installed or too old - see http://librdf.org/rasqal/ to get a version in range $RASQAL_MIN_VERSION to $RASQAL_MAX_VERSION)
])

LIBRDF_EXTERNAL_CPPFLAGS="$RASQAL_CFLAGS $LIBRDF_EXTERNAL_CPPFLAGS"
LIBRDF_EXTERNAL_LIBS="$RASQAL_LIBS $LIBRDF_EXTERNAL_LIBS"

rdf_query="rasqal(system $RASQAL_VERSION)"


# Check for raptor
RAPTOR_MIN_VERSION=2.0.7

PKG_CHECK_MODULES([RAPTOR2],[raptor2 >= $RAPTOR_MIN_VERSION],[
  RAPTOR_VERSION=`$PKG_CONFIG raptor2 --modversion 2>/dev/null`
],[
  AC_MSG_ERROR(Raptor2 is not installed or too old - see http://librdf.org/raptor/ to get version $RAPTOR_MIN_VERSION or newer)
])

LIBRDF_DIRECT_LIBS="$LIBRDF_DIRECT_LIBS $RAPTOR2_LIBS"
LIBRDF_DIRECT_LIBS="$LIBRDF_DIRECT_LIBS $RASQAL_LIBS"

LIBRDF_EXTERNAL_CPPFLAGS="$RAPTOR2_CFLAGS $LIBRDF_EXTERNAL_CPPFLAGS"
LIBRDF_EXTERNAL_LIBS="$LIBRDF_DIRECT_LIBS $LIBRDF_EXTERNAL_LIBS"

rdf_parsers_available="$parser_available raptor(system $RAPTOR_VERSION)"

AC_SUBST(RAPTOR_MIN_VERSION)

dnl for redland.pc.in mostly
HAVE_RAPTOR2_API=1
AC_SUBST(HAVE_RAPTOR2_API)


dnl Checks for modules
digest_modules="md5 sha1 ripemd160"

AC_MSG_CHECKING(digests wanted)
AC_ARG_ENABLE(digests, [  --enable-digests=LIST   Use digests (default=md5 sha1 ripemd160)], digest_modules="$enableval") 
AC_MSG_RESULT($digest_modules)

DIGEST_OBJS=
DIGEST_SRCS=

digest_modules_available=


dnl hashes
HASH_OBJS=
HASH_SRCS=

AC_MSG_CHECKING(for bdb hash support)
if test "$have_libdb" = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_BDB_HASH, 1, [Have BDB hash support])
  HASH_OBJS="$HASH_OBJS rdf_hash_bdb.lo"
  HASH_SRCS="$HASH_SRCS rdf_hash_bdb.c"
else
  AC_MSG_RESULT(no)
fi


AC_SUBST(HASH_OBJS)
AC_SUBST(HASH_SRCS)


dnl Storages
persistent_storages="/file/tstore/mysql/sqlite/"
persistent_store=no
all_storages="memory file hashes trees mysql sqlite tstore postgresql virtuoso"
always_available_storages="memory file hashes trees"

dnl default availabilities and enablements
for storage in $all_storages; do
  eval $storage'_storage_available=no'
  eval $storage'_storage=no'
done
for storage in $always_available_storages; do
  eval $storage'_storage_available=yes'
  eval $storage'_storage=yes'
done

REDLAND_MODULE_PATH=""
AC_ARG_WITH(module-path, [  --with-module-path=PATH Default search path for modules (default=/usr/local/lib/redland:${libdir}/redland], REDLAND_MODULE_PATH="$witheval")
if test "x$REDLAND_MODULE_PATH" = "x"; then
	REDLAND_MODULE_PATH=$libdir"/redland"
	if test "x$prefix" != "xNONE"; then
		REDLAND_MODULE_PATH="/usr/local/lib/redland:"$REDLAND_MODULE_PATH
	fi
fi
AC_SUBST(REDLAND_MODULE_PATH)

storages_available="memory file hashes(memory) trees"
if test "x$have_libdb" = xyes; then
  storages_available="$storages_available hashes(bdb $bdb_version)"
fi

AC_ARG_WITH(threestore, [  --with-threestore(=CONFIG|yes|no)  Location of 3store-config (default=auto)], tstore_config="$withval", tstore_config="")
if test "X$tstore_config" != "Xno"; then
  if test "X$tstore_config" != "Xyes" ; then
    TSTORE_CONFIG=$tstore_config
  fi
  tstore_config=yes
fi
if test $tstore_config = yes -a "X$TSTORE_CONFIG" = "X" ; then
  AC_CHECK_PROGS(TSTORE_CONFIG, 3store-config)
fi

AC_MSG_CHECKING(for 3store library)
if test "X$TSTORE_CONFIG" != X -a "X$PKG_CONFIG" != X ; then
  tstore_version=`$TSTORE_CONFIG --version`

  storages_available="$storages_available 3store($tstore_version)"
  tstore_storage_available=yes

  AC_MSG_RESULT(yes - version $tstore_version)
else
  AC_MSG_RESULT(no)
fi


AC_ARG_WITH(mysql, [  --with-mysql(=CONFIG|yes|no) Location of MySQL 3/4 mysql_config (default=auto)], mysql_config="$withval", mysql_config="")
if test "X$mysql_config" != "Xno"; then
  if test "X$mysql_config" != "Xyes" ; then
    MYSQL_CONFIG=$mysql_config
  fi
  mysql_config=yes
fi
if test $mysql_config = yes -a "X$MYSQL_CONFIG" = "X" ; then
  AC_CHECK_PROGS(MYSQL_CONFIG, mysql_config)
fi

libmysql_min_version=3.23.56
# Also tested on 4.0.14

mysql_cflags=
mysql_libs=
if test "X$MYSQL_CONFIG" != X; then
  dnl need to change quotes to allow square brackets
  changequote(<<, >>)dnl
  mysql_libs=`$MYSQL_CONFIG --libs | sed -e "s/'//g"`
  # Stop the MySQL config program adding -Os or -O<n> options to CFLAGS
  mysql_cflags=`$MYSQL_CONFIG --cflags | sed -e "s/'//g" -e "s/[ \t]-O[A-Za-z0-9]*/ /" `
  changequote([, ])dnl

  LIBS="$LIBRDF_LIBS $mysql_libs"
  CPPFLAGS="$LIBRDF_CPPFLAGS $mysql_cflags"
  AC_MSG_CHECKING(for mysql library)
  LIBMYSQL_VERSION=`$MYSQL_CONFIG --version`
  libmysql_version_dec=`echo $LIBMYSQL_VERSION | $AWK -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`
  libmysql_min_version_dec=`echo $libmysql_min_version | $AWK -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`
  AC_MSG_RESULT(yes - version $LIBMYSQL_VERSION)
  if test $libmysql_version_dec -lt $libmysql_min_version_dec; then
    AC_MSG_WARN(Using libmysql $LIBMYSQL_VERSION is unsupported - $libmysql_min_version or newer recommended.)
  else
    storages_available="$storages_available mysql($LIBMYSQL_VERSION)"
    mysql_storage_available=yes
  fi

  AC_MSG_CHECKING(whether MYSQL_OPT_RECONNECT is declared)
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([#include <mysql.h>
    int x=MYSQL_OPT_RECONNECT;])],
    AC_DEFINE(HAVE_MYSQL_OPT_RECONNECT, 1, [have MYSQL_OPT_RECONNECT declared])
    AC_MSG_RESULT(yes),
    AC_MSG_RESULT(no))

  LIBS="$LIBRDF_LIBS"
  CPPFLAGS="$LIBRDF_CPPFLAGS"
fi



AC_ARG_WITH(sqlite, [  --with-sqlite(=yes|no) Enable SQLite store (default=auto)], with_sqlite="$withval", with_sqlite="auto")

if test "X$with_sqlite" != Xno; then
  PKG_CHECK_MODULES([SQLITE],[sqlite3],[
    SQLITE_VERSION=`$PKG_CONFIG sqlite3 --modversion`
    with_sqlite=yes
  ],[with_sqlite=no])
fi

AC_MSG_CHECKING(using sqlite library)
if test "X$with_sqlite" != Xno ; then
  storages_available="$storages_available sqlite($SQLITE_VERSION)"
  sqlite_storage_available=yes
  AC_MSG_RESULT(yes - version $SQLITE_VERSION)

  CPPFLAGS="$LIBRDF_CPPFLAGS $SQLITE_CFLAGS"
  AC_CHECK_HEADERS(sqlite3.h sqlite.h)
  CPPFLAGS="$LIBRDF_CPPFLAGS"
else
  AC_MSG_RESULT(no)
fi

AC_ARG_WITH(postgresql, [  --with-postgresql(=CONFIG|yes|no)  Location of pg_config (default=auto)], pg_config="$withval", postgresql_config="")
if test "X$pg_config" != "Xno"; then
  if test "X$pg_config" != "Xyes" ; then
    PG_CONFIG=$pg_config
  fi
  pg_config=yes
fi
if test $pg_config = yes -a "X$PG_CONFIG" = "X" ; then
  AC_CHECK_PROGS(PG_CONFIG, pg_config)
fi

AC_MSG_CHECKING(for postgresql library)
if test "X$PG_CONFIG" != X ; then
  postgresql_version=`$PG_CONFIG --version | sed -e 's/^.* //'`
  storages_available="$storages_available postgresql($postgresql_version)"
  postgresql_storage_available=yes

  AC_MSG_RESULT(yes - version $postgresql_version)
  
  CPPFLAGS="$LIBRDF_CPPFLAGS -I`$PG_CONFIG --includedir`"
  AC_CHECK_HEADERS(libpq-fe.h)
  CPPFLAGS="$LIBRDF_CPPFLAGS"
  
  LIBS="$LIBRDF_LIBS -L`$PG_CONFIG --libdir` -lpq"
else
  AC_MSG_RESULT(no)
fi


##########################################################################
#                                                                        #
# OpenLink Virtuoso storage support                                      #
#                                                                        #
##########################################################################

AC_ARG_WITH(virtuoso, [  --with-virtuoso(=yes|no) Enable Virtuoso RDF store (default=auto)], with_virtuoso="$withval", with_virtuoso="auto")

##
##  Checkout ODBC situation
##

#
#  Initialize variables
#
found_odbc=no
ODBC_CFLAGS=
ODBC_LIBS=


#
#  Check for iODBC support
#
if test "$found_odbc" = "no"
then
AC_ARG_WITH(iodbc, [dnl
AS_HELP_STRING([--with-iodbc(=DIR)],[Select iODBC support])
  AS_HELP_STRING([],[DIR is the iODBC base install directory])
  AS_HELP_STRING([],[(default=/usr/local)])
] , [
    if test "$withval" = "yes"
    then
	withval=/usr/local
    fi

    PATH_IODBC="$withval/bin:$PATH"
    AC_PATH_PROG(IODBC_CONFIG, iodbc-config, no, [$PATH_IODBC])

    AC_MSG_CHECKING([for iODBC support])
    if test -f "$withval/lib/libiodbc.dylib"
    then
	found_odbc=iODBC				# Mac OS X
	ODBC_CFLAGS="-I$withval/include"
	ODBC_LIBS="-L$withval/lib -liodbc"
	AC_MSG_RESULT(yes)
    elif test -x "$IODBC_CONFIG"
    then
	found_odbc=iODBC
	ODBC_CFLAGS=`$IODBC_CONFIG --cflags`
	ODBC_LIBS=`$IODBC_CONFIG --libs`
	AC_MSG_RESULT(yes)
    elif test -f "$withval/lib/libiodbc.a"
    then
	found_odbc=iODBC
	ODBC_CFLAGS="-I$withval/include"
	ODBC_LIBS="-L$withval/lib -liodbc"
	AC_MSG_RESULT(yes)
    else
	AC_MSG_RESULT(no)
    fi
])
fi


#
#  Check for UnixODBC support
#
if test "$found_odbc" = "no"
then
AC_ARG_WITH(unixodbc, [dnl
AS_HELP_STRING([--with-unixodbc(=DIR)],[Select UnixODBC support])
  AS_HELP_STRING([],[DIR is the UnixODBC base install directory])
  AS_HELP_STRING([],[(default=/usr/local)])
], [
    if test "$withval" = "yes"
    then
	withval=/usr/local
    fi

    AC_MSG_CHECKING([for UnixODBC support])
    if test -f "$withval/lib/libodbc.a"
    then
	found_odbc=unixODBC
	ODBC_CFLAGS="-I$withval/include"
	ODBC_LIBS="-L$withval/lib -lodbc"
	AC_MSG_RESULT(yes)
    else
	AC_MSG_RESULT(no)
    fi
])
fi


#
#  Check for DataDirect support
#
if test "$found_odbc" = "no"
then
AC_ARG_WITH(datadirect, [dnl
AS_HELP_STRING([--with-datadirect(=DIR)],[Select DataDirect support])
  AS_HELP_STRING([],[DIR is the DataDirect base install directory])
  AS_HELP_STRING([],[(default=/usr/local)])
], [
    if test "$withval" = "yes"
    then
	withval=/usr/local
    fi

    AC_MSG_CHECKING([for DataDirect support])
    if test -f "$withval/lib/libodbc.a"
    then
	found_odbc=DataDirect
	ODBC_CFLAGS="-I$withval/include"
	ODBC_LIBS="-L$withval/lib -lodbc"
	AC_MSG_RESULT(yes)
    else
	AC_MSG_RESULT(no)
    fi
])
fi


#
#  Check for custom ODBC support
#
if test "$found_odbc" = "no"
then
AC_ARG_WITH(odbc-include, [dnl
AS_HELP_STRING([--with-odbc-inc=DIR],[Specify custom ODBC include directory])
  AS_HELP_STRING([],[(default=/usr/local/include)])
], [
    if test "$withval" = "yes"
    then
	withval=/usr/local/include
    fi

    AC_MSG_CHECKING([for sql.h])
    if test -f "$withval/sql.h"
    then
	ODBC_CFLAGS="-I$withval"
	AC_MSG_RESULT(yes)
    else
	AC_MSG_RESULT(no)
    fi
])


AC_ARG_WITH(odbc-lib, [dnl
AS_HELP_STRING([--with-odbc-lib=DIR],[Specify custom ODBC lib directory])
  AS_HELP_STRING([],[(default=/usr/local/lib)])dnl
], [
    if test "$withval" = "yes"
    then
	    withval=/usr/local/lib
    fi

    ac_save_LDFLAGS=$LDFLAGS
    test -n "$withval" && LDFLAGS="$LDFLAGS -L$withval"
    AC_CHECK_LIB(iodbc, SQLConnect, 
    [
	ODBC_LIBS="-L$withval -liodbc"; found_odbc=iODBC
    ],
    [
	AC_CHECK_LIB(odbc, SQLConnect,
	[
	    ODBC_LIBS="-L$withval -lodbc"; found_odbc=ODBC
	])
    ])
    LDFLAGS=$ac_save_LDFLAGS
])
fi


#
#  Maybe the libraries are in some default directory
#
if test "$found_odbc" = "no"
then
    AC_PATH_PROG(IODBC_CONFIG, iodbc-config, no)

    AC_MSG_CHECKING([for iODBC support])
    if test -x "$IODBC_CONFIG"
    then
	found_odbc=iODBC
	ODBC_CFLAGS=`$IODBC_CONFIG --cflags`
	ODBC_LIBS=`$IODBC_CONFIG --libs`
	AC_MSG_RESULT(yes)
    fi
fi 


#
#  Maybe the libraries are in some default directory
#
if test "$found_odbc" = "no"
then
    AC_CHECK_LIB(iodbc, SQLConnect, 
    [
	ODBC_LIBS="-liodbc"; found_odbc=iODBC
    ],
    [
	AC_CHECK_LIB(odbc, SQLConnect,
	[
	    ODBC_LIBS="-lodbc"; found_odbc=ODBC
	])
    ])
fi


#
#  Expand our compile flags
#
AC_SUBST(ODBC_CFLAGS)
AC_SUBST(ODBC_LIBS)

#
#  Check if SQLLEN, SQLULEN, SQLBIGINT, SQLUBIGINT are defined
#
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $ODBC_CFLAGS"
AC_CHECK_TYPE(SQLLEN, ,
	AC_DEFINE(SQLLEN, SQLINTEGER,
		[define to SQLINTEGER if not defined in the ODBC headers])
	AC_DEFINE(SQLULEN, SQLUINTEGER,
		[define to SQLUINTEGER if not defined in the ODBC headers]),
[
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
])


if test "x$with_virtuoso" != "xno" 
then
    AC_MSG_CHECKING([virtuoso storage dependencies])

    if test "$found_odbc" = "no"
    then
	AC_MSG_RESULT(no)
	AC_MSG_WARN([Disabled virtuoso storage:])
        AC_MSG_WARN([   could not find ODBC header files and/or libraries])
        virtuoso_storage_available=no
    else
	AC_MSG_RESULT(yes)
        storages_available="$storages_available virtuoso"
        virtuoso_storage_available=yes
    fi

fi

CPPFLAGS="$save_CPPFLAGS"

##########################################################################
#                                                                        #
# OpenLink Virtuoso storage support   ***END***                          #
#                                                                        #
##########################################################################




# This is needed because autoheader can't work out which computed
# symbols must be pulled from acconfig.h into config.h.in
if test "x" = "y"; then
  AC_DEFINE(STORAGE_FILE,   1, [Building file storage])
  AC_DEFINE(STORAGE_HASHES, 1, [Building hashes storage])
  AC_DEFINE(STORAGE_TREES,  1, [Building trees storage])
  AC_DEFINE(STORAGE_MEMORY, 1, [Building memory storage])
  AC_DEFINE(STORAGE_MYSQL,  1, [Building MySQL storage])
  AC_DEFINE(STORAGE_SQLITE, 1, [Building SQLite storage])
  AC_DEFINE(STORAGE_TSTORE, 1, [Building 3store storage])
  AC_DEFINE(STORAGE_POSTGRESQL, 1, [Building PostgreSQL storage])
  AC_DEFINE(STORAGE_VIRTUOSO, 1, [Building Virtuoso storage])
fi


AC_MSG_CHECKING(storages required)
AC_ARG_ENABLE(storages, [  --enable-storages=LIST  Use storages (default=all)], storages="$enableval") 
if test "X$storages" = Xall -o "X$storages" = X; then
  storages="$all_storages"
  AC_MSG_RESULT(all)
else
  AC_MSG_RESULT($storages)
fi

dnl absolutely ensuring memory is enabled
if echo $storages | grep memory >/dev/null; then
 :
else
  storages="memory $storages"
fi

for storage in $storages; do
  if eval test \$$storage'_storage_available' = yes; then
    eval $storage'_storage=yes'
    NAME=`echo $storage | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
    n=STORAGE_${NAME}
    AC_DEFINE_UNQUOTED($n)
    storages_enabled="$storages_enabled $storage"

    if echo $persistent_storages | grep "/$storage/" >/dev/null; then
      persistent_store=yes
    fi
  fi

done

AM_CONDITIONAL(STORAGE_FILE,   test $file_storage   = yes)
AM_CONDITIONAL(STORAGE_HASHES, test $hashes_storage = yes)
AM_CONDITIONAL(STORAGE_TREES,  test $trees_storage  = yes)
AM_CONDITIONAL(STORAGE_MEMORY, test $memory_storage = yes)
AM_CONDITIONAL(STORAGE_MYSQL,  test $mysql_storage  = yes)
AM_CONDITIONAL(STORAGE_SQLITE, test $sqlite_storage = yes)
AM_CONDITIONAL(STORAGE_TSTORE, test $tstore_storage = yes)
AM_CONDITIONAL(STORAGE_POSTGRESQL, test $postgresql_storage = yes)
AM_CONDITIONAL(STORAGE_VIRTUOSO,  test $virtuoso_storage  = yes)

if test $mysql_storage  = yes; then
  MYSQL_CPPFLAGS=`$MYSQL_CONFIG --cflags | sed -e "s/'//g" -e "s/-O[A-Za-z0-9]*//" `
  MYSQL_LIBS="$LIBRDF_LIBS $mysql_libs"
fi
if test $tstore_storage  = yes; then
  TSTORE_CPPFLAGS="`$TSTORE_CONFIG --cflags` `$PKG_CONFIG glib --cflags`"
  TSTORE_LIBS="`$TSTORE_CONFIG --libs` `$PKG_CONFIG glib --libs`"
fi
if test $postgresql_storage  = yes; then
  POSTGRESQL_CPPFLAGS="-I`$PG_CONFIG --includedir`"
  POSTGRESQL_LIBS="-L`$PG_CONFIG --libdir` -lpq"
fi
if test $virtuoso_storage  = yes; then
  VIRTUOSO_CPPFLAGS="$ODBC_CFLAGS"
  VIRTUOSO_LIBS="$ODBC_LIBS"
fi


if test "$persistent_store" = no; then
  AC_MSG_WARN(No persistant storage is available or enabled)
fi

AC_SUBST(MYSQL_CPPFLAGS)
AC_SUBST(MYSQL_LIBS)
AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LIBS)
AC_SUBST(TSTORE_CPPFLAGS)
AC_SUBST(TSTORE_LIBS)
AC_SUBST(POSTGRESQL_CPPFLAGS)
AC_SUBST(POSTGRESQL_LIBS)
AC_SUBST(VIRTUOSO_CPPFLAGS)
AC_SUBST(VIRTUOSO_LIBS)


if test "$modular" = yes; then
  LIBRDF_INTERNAL_CPPFLAGS="$LIBRDF_INTERNAL_CPPFLAGS $INCLTDL"
  LIBRDF_INTERNAL_LIBS="$LIBRDF_INTERNAL_LIBS $LIBLTDL"
  LIBRDF_INTERNAL_DEPS="$LIBRDF_INTERNAL_DEPS $LTDLDEPS"
  AC_DEFINE(MODULAR_LIBRDF, 1, [Build storage backends as modules])
else
  LIBRDF_CPPFLAGS="$LIBRDF_CPPFLAGS $MYSQL_CPPFLAGS $SQLITE_CFLAGS $TSTORE_CPPFLAGS $POSTGRESQL_CPPFLAGS $VIRTUOSO_CPPFLAGS"
  LIBRDF_LIBS="$LIBRDF_LIBS $MYSQL_LIBS $SQLITE_LIBS $TSTORE_LIBS $POSTGRESQL_LIBS $VIRTUOSO_LIBS"
fi

AM_CONDITIONAL(MODULAR_LIBRDF, test $modular = yes)

CPPFLAGS="$LIBRDF_CPPFLAGS"
LDFLAGS="$LIBRDF_LDFLAGS"
LIBS="$LIBRDF_LIBS"


dnl Check for use of threads

AC_ARG_WITH(threads, [  --with-threads          With POSIX thread support (default=no)], with_threads="$withval", with_threads="no")

AC_CHECK_HEADERS(pthread.h)

AC_MSG_CHECKING(if POSIX threads should be used)
if test $with_threads = "yes" ; then

  if test "$ac_cv_header_pthread_h" = yes ; then
    AC_DEFINE(WITH_THREADS, 1, [Use POSIX threads])
    LIBRDF_LIBS="$LIBRDF_LIBS -lpthread"
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
else
  AC_MSG_RESULT(no)
fi
  
LIBS=$LIBRDF_LIBS


# Maybe add some local digest modules
for module in $digest_modules; do
  module_u=`echo $module | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
  AC_MSG_CHECKING(for local $module digest module)
  base=rdf_digest_$module
  if test -r $srcdir/src/$base.c; then
    AC_MSG_RESULT(yes)

    n=HAVE_LOCAL_${module_u}_DIGEST
    AC_DEFINE_UNQUOTED($n)

    DIGEST_OBJS="$DIGEST_OBJS $base.lo"
    DIGEST_SRCS="$DIGEST_SRCS $base.c"
    digest_modules_available="$digest_modules_available $module"
  else
    AC_MSG_RESULT(no)
  fi
done

AC_SUBST(DIGEST_OBJS)
AC_SUBST(DIGEST_SRCS)

# This is needed because autoheader can't work out which computed
# symbols must be pulled from acconfig.h into config.h.in
if test "x" = "y"; then
  AC_DEFINE(HAVE_LOCAL_MD5_DIGEST, 1, [Have local MD5 digest])
  AC_DEFINE(HAVE_LOCAL_SHA1_DIGEST, 1, [Have local SHA1 digest])
  AC_DEFINE(HAVE_LOCAL_RIPEMD160_DIGEST, 1, [Have local RIPEMD160 digest])
fi



#LIBRDF_EXTERNAL_CPPFLAGS="$LIBRDF_EXTERNAL_CPPFLAGS $LIBRDF_CPPFLAGS"
LIBRDF_EXTERNAL_LIBS="$LIBRDF_EXTERNAL_LIBS $LIBRDF_LIBS"

# Everything except things with .pc files (raptor and rasqal)
# 3store uses it but is optional and thus tricky to specify this way
LIBRDF_PKGCONFIG_PRIVATE_LIBS=`echo $LIBRDF_LIBS | sed -e 's, ..top_build_prefix.libltdl/libltdlc.la, ,'`

# Make final changes to cflags
CFLAGS="-DLIBRDF_INTERNAL=1 $CFLAGS"

assert=yes
AC_ARG_ENABLE(assertions, [  --enable-assertions     Perform run time assertion checks (default yes).  ], assert=$enableval)
if test "$assert" = "yes"; then
  AC_DEFINE(LIBRDF_ASSERT, 1, [Run time assertion checks.])
fi

assert_messages=yes
AC_ARG_ENABLE(assert-messages, [  --enable-assert-messages  Print run time assertion check failure messages (default yes).  ], assert_messages=$enableval)	
if test "$assert_messages" = "yes"; then
  AC_DEFINE(LIBRDF_ASSERT_MESSAGES, 1, [Print run time assertion check failure messages.])
fi

debug_messages=no

AC_ARG_ENABLE(debug, [  --enable-debug          Enable debug messages (default no).  ], debug_messages=$enableval)	
if test "$debug_messages" = "yes"; then
  LIBRDF_CPPFLAGS="-g -DLIBRDF_DEBUG=1 $LIBRDF_CPPFLAGS"
fi

if test "$USE_MAINTAINER_MODE" = yes; then
  CPPFLAGS="$MAINTAINER_CPPFLAGS $CPPFLAGS"
fi

if test "$USE_MAINTAINER_MODE" = yes -a $release_version = no; then 
  dnl need to change quotes to allow square brackets
  changequote(<<, >>)dnl
  CFLAGS=`echo $CFLAGS | sed -e "s/-O[A-Za-z0-9]*//"`
  CXXFLAGS=`echo $CXXFLAGS | sed -e "s/-O[A-Za-z0-9]*//"`
  LIBRDF_CPPFLAGS=`echo $LIBRDF_CPPFLAGS | sed -e "s/-O[A-Za-z0-9]*//"`
  LIBRDF_EXTERNAL_CPPFLAGS=`echo $LIBRDF_EXTERNAL_CPPFLAGS | sed -e "s/-O[A-Za-z0-9]*//"`
  LIBRDF_INTERNAL_CPPFLAGS=`echo $LIBRDF_INTERNAL_CPPFLAGS | sed -e "s/-O[A-Za-z0-9]*//"`
  changequote([, ])dnl
fi


LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS"
LDFLAGS="$LDFLAGS"

LIBRDF_LIBTOOLLIBS=librdf.la

AC_SUBST(LIBRDF_EXTERNAL_CPPFLAGS)
AC_SUBST(LIBRDF_EXTERNAL_LIBS)
AC_SUBST(LIBRDF_DIRECT_LIBS)

AC_SUBST(LIBRDF_INTERNAL_CPPFLAGS)
AC_SUBST(LIBRDF_INTERNAL_LIBS)
AC_SUBST(LIBRDF_INTERNAL_DEPS)

AC_SUBST(LIBRDF_CPPFLAGS)
AC_SUBST(LIBRDF_LDFLAGS)
AC_SUBST(LIBRDF_LIBTOOLLIBS)
AC_SUBST(LIBRDF_PKGCONFIG_PRIVATE_LIBS)

AC_SUBST(MEM)
AC_SUBST(MEM_LIBS)

AC_SUBST(RECHO)
AC_SUBST(RECHO_N)
AC_SUBST(RECHO_C)

abs_top_srcdir=`cd $srcdir; pwd`
AC_SUBST(abs_top_srcdir)
abs_top_builddir=`pwd`
AC_SUBST(abs_top_builddir)

dnl automake 1.11
AM_SILENT_RULES([no])

AC_CONFIG_FILES([Makefile
redland.rdf
docs/Makefile
docs/version.xml
examples/Makefile
scripts/Makefile
src/Makefile
src/librdf.h
src/redland.spec
src/win32_rdf_config.h
src/win32/Makefile
utils/Makefile
demos/Makefile
data/Makefile
redland.pc])
AC_CONFIG_FILES([redland-config], [chmod +x redland-config])
AC_CONFIG_FILES([redland-src-config], [chmod +x redland-src-config])

dnl Check for gtk-doc and docbook
GTK_DOC_CHECK([1.3])

AC_OUTPUT

AC_MSG_RESULT([
Redland build summary:
])

if test "$modular" = yes; then
  AC_MSG_RESULT([  Using modular storage backends in $REDLAND_MODULE_PATH])
else
  AC_MSG_RESULT([  Using monolithic storage backends])
fi

AC_MSG_RESULT([
  Oracle Berkeley DB (BDB) : $bdb_available
  Triple stores available  : $storages_available
  Triple stores enabled    :$storages_enabled
  RDF parsers              :$rdf_parsers_available
  RDF query                : $rdf_query
  Content digests          :$digest_modules_available
])
