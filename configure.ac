dnl -*- Mode: sh -*-
dnl
dnl configure.in - top level autoconf file for Redland
dnl (Process this file with autoconf to produce a configure script.)
dnl
dnl $Id$
dnl
dnl Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
dnl Institute for Learning and Research Technology - http://www.ilrt.org/
dnl University of Bristol - http://www.bristol.ac.uk/
dnl 
dnl This package is Free Software or Open Source available under the
dnl following licenses (these are alternatives):
dnl   1. GNU Lesser General Public License (LGPL)
dnl   2. GNU General Public License (GPL)
dnl   3. Mozilla Public License (MPL)
dnl 
dnl See LICENSE.html or LICENSE.txt at the top of this package for the
dnl full license terms.
dnl 
dnl 


AC_INIT([Redland RDF Library], 0.9.13, redland-dev@lists.librdf.org, redland)
AC_PREREQ(2.53)
AC_CONFIG_SRCDIR(librdf/rdf_node.c)
AC_REVISION($Revision$)dnl

AM_INIT_AUTOMAKE(1.6)
AM_CONFIG_HEADER(librdf/rdf_config.h)
AM_MAINTAINER_MODE

AC_CANONICAL_HOST


LIBRDF_VERSION_MAJOR=0
LIBRDF_VERSION_MINOR=9
LIBRDF_VERSION_RELEASE=13
LIBRDF_VERSION=$LIBRDF_VERSION_MAJOR.$LIBRDF_VERSION_MINOR.$LIBRDF_VERSION_RELEASE
LIBRDF_VERSION_INFO=`expr $LIBRDF_VERSION_MAJOR + $LIBRDF_VERSION_MINOR`:$LIBRDF_VERSION_RELEASE:$LIBRDF_VERSION_MINOR

LIBRDF_VERSION_NUMBER=`expr $LIBRDF_VERSION_MAJOR \* 10000 + $LIBRDF_VERSION_MINOR \* 100 + $LIBRDF_VERSION_RELEASE`

AC_SUBST(LIBRDF_VERSION_MAJOR)
AC_SUBST(LIBRDF_VERSION_MINOR)
AC_SUBST(LIBRDF_VERSION_RELEASE)
AC_SUBST(LIBRDF_VERSION)
AC_SUBST(LIBRDF_VERSION_INFO)
AC_SUBST(LIBRDF_VERSION_NUMBER)
AC_DEFINE_UNQUOTED(LIBRDF_VERSION_MAJOR, $LIBRDF_VERSION_MAJOR, [Redland major version number])
AC_DEFINE_UNQUOTED(LIBRDF_VERSION_MINOR, $LIBRDF_VERSION_MINOR, [Redland minor version number])
AC_DEFINE_UNQUOTED(LIBRDF_VERSION_RELEASE, $LIBRDF_VERSION_RELEASE, [Redland release version number])

VERSION=${LIBRDF_VERSION}

AM_INIT_AUTOMAKE(redland, $VERSION)


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
AM_PROG_CC_STDC
AM_SANITY_CHECK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL

# Find a tar command for 'make dist'
AC_CHECK_PROGS(TAR, gnutar gtar tar)

AC_CHECK_PROGS(JAVA, java jre)
AC_CHECK_PROGS(JAVAC, javac)
AC_CHECK_PROGS(PYTHON, python)
AC_CHECK_PROGS(PERL, perl)
AC_CHECK_PROGS(PHP, php)
AC_CHECK_PROGS(TCL, tcl)
AC_CHECK_PROGS(RUBY, ruby)
AC_CHECK_PROGS(SWIG, swig)
# dotGNU
AC_CHECK_PROGS(ILRUN, ilrun)
AC_CHECK_PROGS(CSCC, cscc)
# Mono
AC_CHECK_PROGS(MONO, mono)
AC_CHECK_PROGS(MCS, mcs)

if test "X$PERL" != "X"; then
  PERLINSTALLSITEARCH=`$PERL -MConfig -e 'print $Config{installsitearch};'`
  PERLMAN3DIR=`$PERL -MConfig -e 'print $Config{man3dir};'`
else
  PERLINSTALLSITEARCH=none
  PERLMAN3DIR=none
fi
AC_SUBST(PERLINSTALLSITEARCH)
AC_SUBST(PERLMAN3DIR)

SWIG_MIN_VERSION=1.3.10

AC_MSG_CHECKING(SWIG support)
if test "X$SWIG" != "X"; then
  SWIG_VERSION=`$SWIG -version 2>&1 |sed -ne 's/^SWIG Version //p'`
  SWIG_VERSION_DEC=`echo $SWIG_VERSION | awk -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`

 if test $SWIG_VERSION_DEC -ge 010314; then
   SWIG_PYTHON_ARGS=-noproxy
 fi

  SWIG_MIN_VERSION_DEC=`echo $SWIG_MIN_VERSION | awk -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`

  if test $SWIG_VERSION_DEC -ge $SWIG_MIN_VERSION_DEC; then
    AC_MSG_RESULT($SWIG_VERSION - OK)
  else
    AC_MSG_RESULT($SWIG_VERSION - too old to generate language interfaces)
    if test -r $srcdir/perl/CORE_wrap.c ; then
      AC_MSG_WARN(Pre-generated language interface files are present)
      AC_MSG_WARN(If you want to change the interfaces you will need)
      AC_MSG_WARN(SWIG version $SWIG_MIN_VERSION from http://www.swig.org/)
    else
      AC_MSG_WARN(There are no pre-generated language interface files)
      AC_MSG_WARN(Redland language interfaces will NOT build.)
      AC_MSG_WARN(If you want to build them you will need)
      AC_MSG_WARN(SWIG version $SWIG_MIN_VERSION from http://www.swig.org/)
      AC_MSG_WARN(Alternatively copy the pre-generated interface)
      AC_MSG_WARN(files from a released version)
    fi
  fi
else
  AC_MSG_RESULT(not present - using pre-generated interface files)
fi

AC_SUBST(SWIG_PYTHON_ARGS)


languages=


AC_ARG_WITH(perl, [  --with-perl(=PERL)      Enable the Perl interface], with_perl="$withval", with_perl="no")
AC_MSG_CHECKING(Enable Perl API)
if test "X$with_perl" != "Xno"; then
  if test "X$with_perl" != "Xyes" -a "X$with_perl" != "Xauto"; then
    PERL=$with_perl
  fi
  with_perl=yes
  languages="$languages perl"
else
  with_perl=no
fi
AC_MSG_RESULT($with_perl)


AC_ARG_WITH(python, [  --with-python(=PYTHON)  Enable the Python interface], with_python="$withval", with_python="no")
AC_MSG_CHECKING(Enable Python API)
if test "X$with_python" != "Xno"; then
  if test "X$with_python" != "Xyes" -a "X$with_python" != "Xauto"; then
    PYTHON=$with_python
  fi
  with_python=yes
  languages="$languages python"
else
  with_python=no
fi
AC_MSG_RESULT($with_python)


AC_ARG_WITH(java, [  --with-java(=JAVA)      Enable the java interface], with_java="$withval", with_java="no")
AC_MSG_CHECKING(Enable Java API)
if test  "X$with_java" != "Xyes" -a "X$with_java" != "Xno"; then
  JAVA=$with_java
  with_java=yes
  languages="$languages java"
else
  with_java=no
fi
AC_MSG_RESULT($with_java)


AC_ARG_WITH(tcl, [  --with-tcl(=TCL)        Enable the Tcl interface], with_tcl="$withval", with_tcl="no")
AC_MSG_CHECKING(Enable Tcl API)
if test "X$with_tcl" != "Xyes" -a "X$with_tcl" != "Xno"; then
  TCL=$with_tcl
  with_tcl=yes
  languages="$languages tcl"
else
  with_tcl=no
fi
AC_MSG_RESULT($with_tcl)


AC_ARG_WITH(php, [  --with-php(=PHP)        Enable the PHP interface], with_php="$withval", with_php="no")
AC_MSG_CHECKING(Enable PHP API)
if test "X$with_php" != "Xyes" -a "X$with_php" != "Xno"; then
  PHP=$with_php
  with_php=yes
  languages="$languages php"
else
  with_php=no
fi
AC_MSG_RESULT($with_php)


AC_ARG_WITH(ruby, [  --with-ruby(=RUBY)      Enable the Ruby interface], with_ruby="$withval", with_ruby="no")
AC_MSG_CHECKING(Enable Ruby API)
if test "X$with_ruby" != "Xyes" -a "X$with_ruby" != "Xno"; then
  RUBY=$with_ruby
  with_ruby=yes
  languages="$languages ruby"
else
  with_ruby=no
fi
AC_MSG_RESULT($with_ruby)

with_ecma_cla=
dnl Pick an ECMA CLI implementation
AC_ARG_WITH(ecma-cli, [  --with-ecma-cli=SYSTEM  Enable ECMA CLI interface - mono, dotgnu (default=no)], ecma_cli_name="$withval", ecma_cli_name="no")


AC_MSG_CHECKING(ECMA CLI implementations available)
result=
if test "X$MONO" != X -a "X$MCS" != X; then
  result="$result Mono"
fi
if test "X$ILRUN" != X -a "X$CSCC" != X; then
  result="$result dotGNU"
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(ECMA CLI implementations to use)
with_ecma_cli=no
CLIRUN=
CSHARPC=
for name in $ecma_cli_name mono dotgnu no; do
  case $name in
    mono)
      if test "X$MONO" != X -a "X$MCS" != X; then
        CLIRUN=$MONO
        CSHARPC=$MCS
        ecma_cli=Mono
      fi
      ;;

    dotgnu)
      if test "X$ILRUN" != X -a "X$CSCC" != X; then
        CLIRUN=$ILRUN
        CSHARPC=$CSCC
        ecma_cli=dotGNU
      fi
      ;;

    no)
      ;;
    *)
      AC_MSG_ERROR(No such ECMA CLI implementation $name)
      ;;
esac
done
AC_MSG_RESULT($ecma_cli)
AC_SUBST(CLIRUN)
AC_SUBST(CSHARPC)

if test "$with_ecma_cli" != no; then
  languages="$languages csharp"
fi



AC_ARG_WITH(jdk, [  --with-jdk=DIR          Java JDK location []], jdk="$withval", jdk="")
AC_MSG_CHECKING(Java JDK includes)
JDK_INCLUDES=
if test "X$jdk" != X -a -r $jdk/include/jni.h; then
  inc1="$jdk/include"
  inc2=`find $inc1 -type d | grep -v "$inc1\$" | head -1`
  JDK_INCLUDES="-I$inc1"
  if test -r $inc2/jni_md.h; then
    JDK_INCLUDES="$JDK_INCLUDES -I$inc2"
  fi
fi
AC_SUBST(JDK_INCLUDES)
AC_MSG_RESULT($JDK_INCLUDES)


AC_CHECK_PROG(LIBWWW_CONFIG, libwww-config, libwww-config)

AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)


release_version=no
AC_ARG_ENABLE(release, [  --enable-release        Turn on optimizations.  ], \
 if test "$enableval" = "yes"; then \
	release_version=yes
fi;)	

if test $release_version = no; then 
  CFLAGS=`echo $CFLAGS | sed s/-O2//`
  CXXFLAGS=`echo $CXXFLAGS | sed s/-O2//`
  CPPFLAGS=`echo $CPPFLAGS | sed s/-O2//`
fi



dnl compiler checks
# if using gcc...
if test "$ac_cv_prog_gcc" = yes; then
  STANDARD_CFLAGS=
  MAINTAINER_CFLAGS="-Wall -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls"
else
  STANDARD_CFLAGS=
  MAINTAINER_CFLAGS=
fi


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

# do these first because somehow they get messed up by bdb checks
AC_CHECK_TYPE([byte u32])

AC_CHECK_SIZEOF(unsigned char, 1)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)
  
dnl Checks for libraries.

dnl Berkeley DB
AC_ARG_WITH(bdb, [  --with-bdb=DIR          Berkeley DB install area [/usr]], bdb_prefix="$withval", bdb_prefix="/usr") 
AC_ARG_WITH(bdb_lib, [  --with-bdb-lib=DIR      Berkeley DB lib directory [/usr/lib]], bdb_lib_dir="$withval", bdb_lib_dir="$bdb_prefix/lib") 
AC_ARG_WITH(bdb_include, [  --with-bdb-include=DIR  Berkeley DB include directory [/usr/include]], bdb_include_dir="$withval", bdb_include_dir="$bdb_prefix/include") 
AC_ARG_WITH(bdb_dbname, [  --with-bdb-dbname=NAME  Berkeley DB library name []], bdb_dbname="$withval", bdb_dbname="none")


AC_MSG_CHECKING(location of BDB libs)
AC_MSG_RESULT($bdb_lib_dir)
AC_MSG_CHECKING(location of BDB includes)
AC_MSG_RESULT($bdb_include_dir)

bdb_available=
if test "$bdb_prefix" != no; then
  nLDFLAGS=$LDFLAGS
  nCPPFLAGS=

  if test "$bdb_lib_dir" != /usr/lib; then
    nLDFLAGS="-L$bdb_lib_dir";
  fi

  if test "$bdb_name" != none; then
    # Yuck, yuck, yuck - can't they create a bdb-config program like GNOME?
    bdb_dbname=`cd $bdb_lib_dir; ls -1 libdb*.a 2>/dev/null |sed -e 's/^lib//' -e 's/\.a$//' | head -1`
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
  if test "$ac_cv_header_db_h" = yes ; then
    have_libdb=no

    AC_MSG_CHECKING(for BDB V1 dbopen)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [dbopen(NULL, 0, 0, DB_BTREE, NULL);],
		AC_DEFINE(HAVE_DBOPEN, 1, [BDB has dbopen method])
                if test $have_libdb = no; then
                  bdb_available="Version 1.x - not supported"
                  have_libdb=no
                fi
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB V2 db_open)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [db_open(NULL, DB_BTREE, 0, 0, NULL, NULL, NULL);],
		AC_DEFINE(HAVE_DB_OPEN, 1, [BDB has db_open method])
                if test $have_libdb = no; then
                  bdb_available="Version 2.x"
                  have_libdb=yes
                fi
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB V3/V4.x db_create)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [db_create(NULL, NULL, 0);],
		AC_DEFINE(HAVE_DB_CREATE, 1, [BDB has db_create method])
                bdb_available="Version 3.x/4.x"
                have_libdb=yes
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB V4.0 DB->open with 6 arguments)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB *bdb; bdb->open(bdb, NULL, NULL, DB_BTREE, 0, 0);],
		AC_DEFINE(HAVE_BDB_OPEN_6_ARGS, 1, [BDB has open method with 6 args])
                bdb_available="Version 4.0+"
                have_libdb=yes
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB V4.1+ DB->open with 7 arguments)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB *bdb; bdb->open(bdb, NULL, NULL, NULL, DB_BTREE, 0, 0);],
		AC_DEFINE(HAVE_BDB_OPEN_7_ARGS, 1, [BDB has open method with 7 args])
                bdb_available="Version 4.1+"
                have_libdb=yes
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(whether DB_TXN defined in db.h)
    dnl In BDB, DB_TXN is a pointer to a structure never defined
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB_TXN* ptr=(DB_TXN*)NULL],
		AC_DEFINE(HAVE_BDB_DB_TXN, 1, [BDB defines DB_TXN])
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))
    AC_MSG_CHECKING(whether DBC defined in db.h)
    AC_TRY_LINK([#include <db.h>], [size_t len=sizeof(DBC)],
		AC_DEFINE(HAVE_BDB_CURSOR, 1, [BDB defines DBC])
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))
    AC_MSG_CHECKING(number of arguments to db_cursor)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB* db; db->cursor(db, NULL, NULL, 0);],
		AC_DEFINE(HAVE_BDB_CURSOR_4_ARGS, 1, [BDB cursor method has 4 arguments])
		AC_MSG_RESULT(4),
		AC_MSG_RESULT(3))
  fi

  if test "$have_libdb" = yes; then
    LIBRDF_LIBS="-l$bdb_dbname $LIBRDF_LIBS"
    LIBRDF_LDFLAGS="$nLDFLAGS $LIBRDF_LDFLAGS"
    LIBRDF_CPPFLAGS="$nCPPFLAGS $LIBRDF_CPPFLAGS"
  fi

fi

CPPFLAGS="$LIBRDF_CPPFLAGS"
LDFLAGS="$LIBRDF_LDFLAGS"
LIBS="$LIBRDF_LIBS"


if test "$have_libdb" != yes; then
  AC_MSG_WARN(No persistant storage configured)
  bdb_available=Missing
fi


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(errno.h stdlib.h unistd.h string.h fcntl.h dmalloc.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(mkstemp mktemp tmpnam)

dnl Checks for URI resolvers
AC_ARG_WITH(libwww, [  --with-libwww           Use libwww [yes]], enable_libwww="$withval", enable_libwww="no") 

have_libwww=
AC_MSG_CHECKING(whether have working libwww)
if test "$enable_libwww" = yes ; then
  if test -n "$LIBWWW_CONFIG"; then
    AC_MSG_RESULT(yes)
    LIBRDF_LIBS="`$LIBWWW_CONFIG --libs` $LIBRDF_LIBS"
    LIBRDF_CPPFLAGS="`$LIBWWW_CONFIG --cflags` $LIBRDF_CPPFLAGS"
    AC_DEFINE(HAVE_LIBWWW, 1, [W3C libwww is present])
    have_libwww=yes
  else
    AC_MSG_RESULT(no)
  fi
else
  AC_MSG_RESULT(no)
fi

CPPFLAGS="$LIBRDF_CPPFLAGS"
LDFLAGS="$LIBRDF_LDFLAGS"
LIBS="$LIBRDF_LIBS"


dnl Checks for XML parsers

# At present this is for passing on to raptor and not used here
AC_ARG_WITH(xml-parser, [  --with-xml-parser=NAME  Use XML parser - expat, libxml [expat]], xml_parser="$withval", xml_parser="libxml") 




dnl Checks for RDF parsers
parser_modules="raptor"

AC_MSG_CHECKING(RDF parsers required)
AC_ARG_ENABLE(parsers, [  --enable-parsers=LIST   Use RDF parsers [raptor]], parser_modules="$enableval") 
if test "X$parser_modules" = "Xno" -o "X$parser_modules" = X; then
  parser_modules=
fi
AC_MSG_RESULT($parser_modules)

PARSER_OBJS=
PARSER_SRCS=


# This is needed because autoheader can't work out which computed
# symbols must be pulled from acconfig.h into config.h.in
if test "x" = "y"; then
  AC_DEFINE(HAVE_RAPTOR_RDF_PARSER, 1, [Have Raptor RDF parser])
fi


# Always have these
have_raptor=1


need_raptor=1
rdf_parsers_available=
for parser in $parser_modules; do
  uname=`echo $parser | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
  found=0
  original_parser=$parser

  case $parser in
    raptor)
      if test $have_raptor = 1 ; then
       	need_raptor=1
	found=1
      fi
        
      ;;
    *)
      AC_MSG_ERROR(No such parser $parser)
  esac

  if test $found = 1; then
    rdf_parsers_available="$rdf_parsers_available $original_parser"
  fi

  # Add objects, sources only if the parser was found
  if test $found = 1; then
    PARSER_OBJS="$PARSER_OBJS rdf_parser_$parser.lo"
    PARSER_SRCS="$PARSER_SRCS rdf_parser_$parser.c"

    if test $parser = raptor; then
      LIBRDF_INTERNAL_CPPFLAGS="-I\$(top_srcdir)/raptor $LIBRDF_INTERNAL_CPPFLAGS"
      LIBRDF_INTERNAL_LIBS="\$(top_builddir)/raptor/libraptor.la $LIBRDF_INTERNAL_LIBS"
    fi

    n=HAVE_${uname}_RDF_PARSER
    AC_DEFINE_UNQUOTED($n)
  fi
done



dnl Checks for languages
AC_CHECK_PROG(PHP_CONFIG, php-config, php-config)
if test "X$PHP" != X -a "X$PHP_CONFIG" != X ; then

  PHP_INCLUDES=`$PHP_CONFIG --includes`

  AC_SUBST(PHP_INCLUDES)
else
  # This allows 'make clean' in the python directory to work when
  # python isn't available
  PHP=:
fi

if test "X$PYTHON" != X; then

  dnl need to change quotes to allow square brackets
  changequote(<<, >>)dnl
  python_version=`$PYTHON -c 'import sys; print sys.version[:3]'`
  python_prefix=`$PYTHON -c 'import sys; print sys.prefix'`
  changequote([, ])dnl

  # What a crock
  PYTHON_INCLUDES="-I$python_prefix/include/python$python_version"
  AC_SUBST(PYTHON_INCLUDES)
else
  # This allows 'make clean' in the python directory to work when
  # python isn't available
  PYTHON=:
fi

if test "X$RUBY" != X; then

  dnl need to change quotes to allow square brackets
  changequote(<<, >>)dnl
  ruby_prefix=`$RUBY -rrbconfig -e "print Config::CONFIG['archdir']"`
  changequote([, ])dnl

  RUBY_INCLUDES="-I$ruby_prefix"
  AC_SUBST(RUBY_INCLUDES)
else
  # This allows 'make clean' in the python directory to work when
  # python isn't available
  RUBY=:
fi

AC_SUBST(PARSER_OBJS)
AC_SUBST(PARSER_SRCS)

if test "X$PARSER_OBJS" = X; then
  AC_MSG_WARN(No RDF parser configured)
fi



dnl Checks for modules
digest_modules="md5 sha1 ripemd160"

AC_MSG_CHECKING(digests wanted)
AC_ARG_ENABLE(digests, [  --enable-digests=LIST   Use digests [md5 sha1 ripemd160]], digest_modules="$enableval") 
AC_MSG_RESULT($digest_modules)

DIGEST_OBJS=
DIGEST_SRCS=

AC_ARG_WITH(openssl-digests, [  --with-openssl-digests  Use openssl digests [yes]], enable_openssl_digests="$withval", enable_openssl_digests="yes") 

# This is needed because autoheader can't work out which computed
# symbols must be pulled from acconfig.h into config.h.in
if test "x" = "y"; then
  AC_DEFINE(HAVE_OPENSSL_CRYPTO_MD5_DIGEST, 1, [Have openssl MD5 digest])
  AC_DEFINE(HAVE_OPENSSL_CRYPTO_SHA1_DIGEST, 1, [Have openssl SHA1 digest])
  AC_DEFINE(HAVE_OPENSSL_CRYPTO_RIPEMD160_DIGEST, 1, [Have openssl RIPEMD160 digest])
fi

digest_modules_available=
AC_MSG_CHECKING(whether to use openssl digests)
if test "$enable_openssl_digests" = yes ; then
  AC_MSG_RESULT(yes)
  AC_CHECK_HEADERS(openssl/crypto.h)
  if test "$ac_cv_header_openssl_crypto_h" = yes ; then
    AC_DEFINE(HAVE_OPENSSL_DIGESTS, 1, [Have openssl digests])
    new_digest_modules=
    LIBS="-lcrypto $LIBRDF_LIBS"
    have_libcrypto=no

    for module in $digest_modules; do
      func=`echo $module | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
      found=
      AC_MSG_CHECKING(for openssl $func digest module)
      AC_CACHE_VAL(ac_cv_lib_crypto_$func,
                   [AC_TRY_LINK(, $func(),
                   [eval "ac_cv_lib_crypto_$func=yes"],
                   [eval "ac_cv_lib_crypto_$func=no"])])
      if eval "test \"`echo '$ac_cv_lib_crypto_'$func`\" = yes"; then
	AC_MSG_RESULT(yes)
	n=HAVE_OPENSSL_CRYPTO_${func}_DIGEST
	AC_DEFINE_UNQUOTED($n)
        have_libcrypto=yes
        digest_modules_available="$digest_modules_available $module(openssl)"
      else
	AC_MSG_RESULT(no)
	new_digest_modules="${new_digest_modules} $module"
      fi
    done
    if test "$have_libcrypto" = yes; then
      LIBRDF_LIBS="-lcrypto $LIBRDF_LIBS"
    fi
    DIGEST_OBJS="$DIGEST_OBJS rdf_digest_openssl.lo"
    DIGEST_SRCS="$DIGEST_SRCS rdf_digest_openssl.c"
    digest_modules=$new_digest_modules
  fi
else
  AC_MSG_RESULT(no)
fi


dnl Check for use of threads

AC_ARG_WITH(threads, [  --with-threads          With POSIX thread support. default=no], with_threads="yes", with_threads="no")

AC_CHECK_HEADERS(pthread.h)

AC_MSG_CHECKING(if POSIX threads should be used)
if test $with_threads = "yes" ; then

  if test "$ac_cv_header_pthread_h" = yes ; then
    AC_DEFINE(WITH_THREADS, 1, [Use POSIX threads])
    LIBRDF_LIBS="-lpthread $LIBRDF_LIBS"
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
  if test -r $srcdir/librdf/$base.c; then
    AC_MSG_RESULT(yes);

    n=HAVE_LOCAL_${module_u}_DIGEST
    AC_DEFINE_UNQUOTED($n)

    DIGEST_OBJS="$DIGEST_OBJS $base.lo"
    DIGEST_SRCS="$DIGEST_SRCS $base.c"
    digest_modules_available="$digest_modules_available $module"
  else
    AC_MSG_RESULT(no);
  fi
done

AC_SUBST(DIGEST_OBJS)
AC_SUBST(DIGEST_SRCS)


HASH_OBJS=
HASH_SRCS=

AC_MSG_CHECKING(for bdb hash support)
if test "$have_libdb" = yes; then
  AC_MSG_RESULT(yes);
  AC_DEFINE(HAVE_BDB_HASH, 1, [Have BDB hash support])
  HASH_OBJS="$HASH_OBJS rdf_hash_bdb.lo"
  HASH_SRCS="$HASH_SRCS rdf_hash_bdb.c"
else
  AC_MSG_RESULT(no);
fi


AC_SUBST(HASH_OBJS)
AC_SUBST(HASH_SRCS)


dnl Checks for building sub-packages in sub directories
SD="docs $languages"


AC_SUBST(SD)


# Make final changes to cflags
CFLAGS="-DLIBRDF_INTERNAL=1 $CFLAGS"
MEM=
MEM_LIBS=
if test "$USE_MAINTAINER_MODE" = yes; then
  if test "$ac_cv_header_dmalloc_h" = yes; then
    MEM=-DLIBRDF_MEMORY_DEBUG_DMALLOC=1
    MEM_LIBS=-ldmalloc
  fi
  CFLAGS="-DLIBRDF_DEBUG=1 $CFLAGS"
fi
STANDARD_CFLAGS="$STANDARD_CFLAGS $CFLAGS"
if test "$USE_MAINTAINER_MODE" = yes; then
  CFLAGS="$MAINTAINER_CFLAGS $CFLAGS"
fi

LIBS=
CPPFLAGS=
LDFLAGS=

AC_SUBST(LIBRDF_INTERNAL_CPPFLAGS)
AC_SUBST(LIBRDF_INTERNAL_LIBS)

AC_SUBST(LIBRDF_CPPFLAGS)
AC_SUBST(LIBRDF_LDFLAGS)
AC_SUBST(LIBRDF_LIBS)

AC_SUBST(MEM)
AC_SUBST(MEM_LIBS)
AC_SUBST(STANDARD_CFLAGS)

abs_top_srcdir=`cd $srcdir; pwd`
AC_SUBST(abs_top_srcdir)
abs_top_builddir=`pwd`
AC_SUBST(abs_top_builddir)

AC_CONFIG_SUBDIRS(raptor)

AC_CONFIG_FILES([Makefile
docs/Makefile
librdf/Makefile
examples/Makefile
perl/Makefile
perl/t/Makefile
perl/lib/Makefile
perl/lib/RDF/Makefile
perl/lib/RDF/Redland/Makefile
php/Makefile
python/Makefile
python/test/Makefile
tcl/Makefile
java/Makefile
java/org/Makefile
java/org/librdf/Makefile
java/org/librdf/redland/Makefile
ruby/Makefile
csharp/Makefile
utils/Makefile
demos/Makefile])
AC_CONFIG_FILES([librdf/redland.spec])
AC_CONFIG_FILES([redland-config], [chmod +x redland-config])
AC_CONFIG_FILES([redland-src-config], [chmod +x redland-src-config])

AC_OUTPUT

AC_MSG_RESULT([
Redland build summary:
  Language interfaces:   $languages
  RDF parsers:           $rdf_parsers_available
  Hash digests:          $digest_modules_available
  Berkeley/Sleepycat DB:  $bdb_available
])
