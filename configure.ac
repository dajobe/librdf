dnl -*- Mode: sh -*-
dnl
dnl configure.in - top level autoconf file for Redland
dnl (Process this file with autoconf to produce a configure script.)
dnl
dnl $Id$
dnl
dnl Copyright (C) 2000-2004 David Beckett - http://purl.org/net/dajobe/
dnl Institute for Learning and Research Technology - http://www.ilrt.bris.ac.uk/
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


AC_INIT([Redland RDF Application Framework], 0.9.17, redland-dev@lists.librdf.org, redland)
AC_PREREQ(2.53)
AC_CONFIG_SRCDIR(librdf/rdf_node.c)
AC_REVISION($Revision$)dnl

AM_INIT_AUTOMAKE([1.7 check-news std-options -Wobsolete -Wportability -Wsyntax -Wunsupported])
AM_CONFIG_HEADER(librdf/rdf_config.h)
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
LIBRDF_VERSION_DECIMAL=$version_decimal
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

# OSX Fink
if test -d /sw/bin ; then
  PATH=$PATH:/sw/bin
fi

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
AC_CHECK_PROGS(TCL, tclsh)
AC_CHECK_PROGS(RUBY, ruby)
AC_CHECK_PROGS(SWIG, swig)
# dotGNU
AC_CHECK_PROGS(ILRUN, ilrun)
AC_CHECK_PROGS(CSCC, cscc)
# Mono
AC_CHECK_PROGS(MONO, mono)
AC_CHECK_PROGS(MCS, mcs)
# Gnome
AC_CHECK_PROGS(PKG_CONFIG, pkg-config)

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
  SWIG_VERSION_DEC=`echo $SWIG_VERSION | $AWK -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`

 if test $SWIG_VERSION_DEC -ge 010314; then
   SWIG_PYTHON_ARGS=-noproxy
 fi

  SWIG_MIN_VERSION_DEC=`echo $SWIG_MIN_VERSION | $AWK -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`

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
languages_available=


AC_ARG_WITH(perl, [  --with-perl(=PERL)      Enable the Perl interface], with_perl="$withval", with_perl="no")
AC_MSG_CHECKING(Enable Perl API)
if test "X$with_perl" != Xno ; then
  if test "X$with_perl" != Xyes; then
    PERL=$with_perl
  fi
  with_perl=yes
fi

PERL_VERSION=`$PERL -MConfig -e 'print $Config{version}' 2>/dev/null`
if test "X$PERL_VERSION" != X; then
  languages_available="$languages_available perl($PERL_VERSION)"
  if test $with_perl = yes; then
    languages="$languages perl"
  fi
fi
AC_MSG_RESULT($with_perl)
AC_SUBST(PERL_VERSION)


AC_ARG_WITH(python, [  --with-python(=PYTHON)  Enable the Python interface], with_python="$withval", with_python="no")
AC_MSG_CHECKING(Enable Python API)
if test "X$with_python" != Xno; then
  if test "X$with_python" != Xyes; then
    PYTHON=$with_python
  fi
  with_python=yes
fi

dnl need to change quotes to allow square brackets
changequote(<<, >>)dnl
PYTHON_VERSION=`$PYTHON -c 'import sys; print sys.version[:3]' 2>/dev/null`
changequote([, ])dnl
if test "X$PYTHON_VERSION" != X; then
  languages_available="$languages_available python($PYTHON_VERSION)"
  if test $with_python = yes; then
    languages="$languages python"
  fi
fi
AC_MSG_RESULT($with_python)
AC_SUBST(PYTHON_VERSION)


AC_ARG_WITH(java, [  --with-java(=JAVA)      Enable the java interface], with_java="$withval", with_java="no")
AC_MSG_CHECKING(Enable Java API)
if test "X$with_java" != Xno ; then
  if test "X$with_java" != Xyes; then
    JAVA=$with_java
  fi
  with_java=yes
fi

dnl need to change quotes to allow square brackets
changequote(<<, >>)dnl
JAVA_VERSION=`$JAVA -version 2>&1 | $SED -ne 's/java version "\([^"]*\)".*/\1/p' 2>/dev/null`
changequote([, ])dnl
if test "X$JAVA_VERSION" != X; then
  languages_available="$languages_available java($JAVA_VERSION)"
  if test $with_java = yes; then
    languages="$languages java"
  fi
fi
AC_MSG_RESULT($with_java)
AC_SUBST(JAVA_VERSION)


AC_ARG_WITH(tcl, [  --with-tcl(=TCL)        Enable the Tcl interface], with_tcl="$withval", with_tcl="no")
AC_MSG_CHECKING(Enable Tcl API)
if test "X$with_tcl" != Xno ; then
  if test "X$with_tcl" != Xyes; then
    TCL=$with_tcl
  fi
  with_tcl=yes
fi

changequote(<<, >>)dnl
TCL_VERSION=`echo 'puts -nonewline [info tclversion]' | $TCL 2>/dev/null`
changequote([, ])dnl
if test "X$TCL_VERSION" != X; then
  languages_available="$languages_available tcl($TCL_VERSION)"
  if test $with_tcl = yes; then
    languages="$languages tcl"
  fi
fi
AC_MSG_RESULT($with_tcl)
AC_SUBST(TCL_VERSION)

AC_ARG_WITH(tcl_include, [  --with-tcl-include=DIR  Tcl include directory], tcl_include="$withval", tcl_include="")
AC_MSG_CHECKING(TCL include)
TCL_INCLUDES=
if test "X$TCL_VERSION" != X; then
  if test "X$tcl_include" = X ; then
    for dir in /usr/include/tcl$TCL_VERSION /usr/local/include/tcl$TCL_VERSION; do
      if test -r $dir/tcl.h; then
	TCL_INCLUDES="-I$dir"
	break
      fi
    done
  elif test "X$tcl_include" != X -a -r $tcl_include/tcl.h; then
    TCL_INCLUDES="-I$tcl_include"
  fi
fi
AC_MSG_RESULT($TCL_INCLUDES)
AC_SUBST(TCL_INCLUDES)


AC_ARG_WITH(php, [  --with-php(=PHP)        Enable the PHP interface], with_php="$withval", with_php="no")
AC_MSG_CHECKING(Enable PHP API)
if test "X$with_php" != Xno ; then
  if test "X$with_php" != Xyes; then
    PHP=$with_php
  fi
  with_php=yes
fi

PHP_VERSION=`php-config --version 2>/dev/null`
if test "X$PHP_VERSION" != X; then
  languages_available="$languages_available php($PHP_VERSION)"
  if test $with_php = yes; then
    languages="$languages php"
  fi
fi
AC_MSG_RESULT($with_php)
AC_SUBST(PHP_VERSION)


AC_ARG_WITH(ruby, [  --with-ruby(=RUBY)      Enable the Ruby interface], with_ruby="$withval", with_ruby="no")
AC_MSG_CHECKING(Enable Ruby API)
if test "X$with_ruby" != Xno ; then
  if test "X$with_ruby" != Xyes; then
    RUBY=$with_ruby
  fi
  with_ruby=yes
fi

RUBY_VERSION=`$RUBY --version 2>/dev/null | $AWK '{print $2}'`
if test "X$RUBY_VERSION" != X; then
  languages_available="$languages_available ruby($RUBY_VERSION)"
  if test $with_ruby = yes; then
    languages="$languages ruby"
  fi
fi
AC_MSG_RESULT($with_ruby)
AC_SUBST(RUBY_VERSION)



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



AC_ARG_WITH(jdk, [  --with-jdk=DIR          Java JDK location (default=auto)], jdk="$withval", jdk="")
AC_MSG_CHECKING(Java JDK includes)
if test "X$JAVA_VERSION" != X; then
  if test "X$jdk" = X ; then
    for jdkdir in `ls -1dr /usr/local/jdk* 2>/dev/null` /usr/j2se `ls -1dr /usr/lib/java* 2>/dev/null` /System/Library/Frameworks/JavaVM.framework /usr/local/packages/j2sdk; do
      if test ! -d $jdkdir; then
        continue
      fi
      for idir in include Headers; do
        if test -r $jdkdir/$idir/jni.h ; then
          jdk=$jdkdir
          jdkinc=$jdkdir/$idir
          break 2
        fi
      done
    done
  else
    jdkinc=$jdkdir/include
  fi

  if test "X$jdk" != X -a -r $jdkinc/jni.h; then
    inc2=`find $jdkinc -type d | grep -v "$jdkinc\$" | head -1`
    JDK_INCLUDES="-I$jdkinc"
    if test -r $inc2/jni_md.h; then
      JDK_INCLUDES="$JDK_INCLUDES -I$inc2"
    fi
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

# Libraries linked externally also may be internal - presently just
# raptor and rasqal but in future may include others
# Currently only substituted into redland-src-config.
# -Dfoo -Idir
LIBRDF_EXTERNAL_CPPFLAGS=
# -Ldir -llib
LIBRDF_EXTERNAL_LIBS=

# do these first because somehow they get messed up by bdb checks
AC_CHECK_TYPE([byte u32 u64])

AC_CHECK_SIZEOF(unsigned char, 1)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)
AC_CHECK_SIZEOF(unsigned long long, 8)
  
dnl Checks for libraries.


STORAGE_OBJS=

store_modules="memory"
persistent_store=no


dnl Berkeley DB
AC_ARG_WITH(bdb, [  --with-bdb=DIR          Berkeley DB install area (default=/usr)], bdb_prefix="$withval", bdb_prefix="none") 
AC_ARG_WITH(bdb_lib, [  --with-bdb-lib=DIR      Berkeley DB lib directory (default=/usr/lib)], bdb_lib_dir="$withval", bdb_lib_dir="none") 
AC_ARG_WITH(bdb_include, [  --with-bdb-include=DIR  Berkeley DB include directory (default=/usr/include)], bdb_include_dir="$withval", bdb_include_dir="none") 
AC_ARG_WITH(bdb_dbname, [  --with-bdb-dbname=NAME  Berkeley DB library name (auto)], bdb_dbname="$withval", bdb_dbname="none")

bdb_version=unknown

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
  # OSX fink
  if test -d /sw/include -a -d /sw/lib ; then
    extraprefix=/sw
  fi

  # Sigh!  And this still might not be good enough.
  for bdbc_version in 4.2 4.1 4.0 4 3.3 3.2 3.1 3 2; do
    for bdbc_prefix in $extraprefix /usr/local /usr; do
      bdbc_major=`echo $bdbc_version | sed -e 's/^\(.\).*$/\1/'`;
      bdbc_version2=`echo $bdbc_version | sed -e 's/^\(.\).\(.\)$/\1\2/'`

      bdbc_libdir=$bdbc_prefix/lib
      for bdbc_incdir in $bdbc_prefix/include/db$bdbc_version $bdbc_prefix/include/db$bdbc_version2 $bdbc_prefix/include/db$bdbc_major $bdbc_prefix/include ; do
        if test -r $bdbc_incdir/db.h ; then
          header_bdbc_version=`sed -ne 's/^.*DB_VERSION_STRING.*Berkeley DB \(...\).*/\1/p' $bdbc_incdir/db.h`
          if test "X$header_bdbc_version" = "X$bdbc_version" ; then 
            for bdbc_name in db-$bdbc_version db$bdbc_version db-$bdbc_version2 db$bdbc_version2; do
              if test -r $bdbc_libdir/lib$bdbc_name.a -o -r $bdbc_libdir/lib$bdbc_name.so -o -r $bdbc_libdir/lib$bdbc_name.dylib; then
  	        bdb_version=$bdbc_version
  	        bdb_prefix=$bdbc_prefix
  	        bdb_include_dir=$bdbc_incdir
	        bdb_lib_dir=$bdbc_libdir
	        bdb_dbname=$bdbc_name
                break 4
              fi
            done
          fi
        fi

      done
     done
   done

fi

AC_MSG_CHECKING(BDB version)
AC_MSG_RESULT($bdb_version)
AC_MSG_CHECKING(location of BDB libs)
AC_MSG_RESULT($bdb_lib_dir)
AC_MSG_CHECKING(location of BDB includes)
AC_MSG_RESULT($bdb_include_dir)

bdb_available=Missing
if test "$bdb_prefix" != no; then
  nLDFLAGS=$LDFLAGS
  nCPPFLAGS=

  if test "$bdb_lib_dir" != /usr/lib; then
    nLDFLAGS="-L$bdb_lib_dir";
  fi

  if test "$bdb_dbname" = none; then
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
                have_libdb=yes
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB V3/V4.x db_create)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [db_create(NULL, NULL, 0);],
		AC_DEFINE(HAVE_DB_CREATE, 1, [BDB has db_create method])
                have_libdb=yes
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB V4.0 DB->open with 6 arguments)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB *bdb; bdb->open(bdb, NULL, NULL, DB_BTREE, 0, 0);],
		AC_DEFINE(HAVE_BDB_OPEN_6_ARGS, 1, [BDB has open method with 6 args])
                have_libdb=yes
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB DB->close with 2 arguments)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB *bdb; bdb->close(bdb, 0);],
		AC_DEFINE(HAVE_BDB_CLOSE_2_ARGS, 1, [BDB has close method with 2 args])
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB DB->fd with 2 arguments)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB *bdb; bdb->fd(bdb, 0);],
		AC_DEFINE(HAVE_BDB_FD_2_ARGS, 1, [BDB has fd method with 2 args])
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB DB->set_flags)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB *bdb; bdb->set_flags(bdb, 0);],
		AC_DEFINE(HAVE_BDB_SET_FLAGS, 1, [BDB has set_flags method])
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

    AC_MSG_CHECKING(for BDB V4.1+ DB->open with 7 arguments)
    AC_TRY_LINK([#include <stdio.h>
                 #include <db.h>], [DB *bdb; bdb->open(bdb, NULL, NULL, NULL, DB_BTREE, 0, 0);],
		AC_DEFINE(HAVE_BDB_OPEN_7_ARGS, 1, [BDB has open method with 7 args])
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
    bdb_available="Version $bdb_version (library $bdb_dbname in $bdb_lib_dir)"
    LIBRDF_LIBS="-l$bdb_dbname $LIBRDF_LIBS"
    LIBRDF_LDFLAGS="$nLDFLAGS $LIBRDF_LDFLAGS"
    LIBRDF_CPPFLAGS="$nCPPFLAGS $LIBRDF_CPPFLAGS"
  fi

fi

CPPFLAGS="$LIBRDF_CPPFLAGS"
LDFLAGS="$LIBRDF_LDFLAGS"
LIBS="$LIBRDF_LIBS"


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(errno.h stdlib.h unistd.h string.h fcntl.h dmalloc.h time.h sys/time.h getopt.h)
AC_HEADER_TIME

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(getopt getopt_long mkstemp mktemp tmpnam gettimeofday getenv)

RDFPROC_EXTRA_OBJS=
if test $ac_cv_func_getopt = no -a $ac_cv_func_getopt_long = no; then
  RDFPROC_EXTRA_OBJS='getopt.$(OBJEXT)'
  AC_SUBST([RDFPROC_EXTRA_OBJS])
fi

dnl Checks for URI resolvers
AC_ARG_WITH(libwww, [  --with-libwww           Use libwww (default=auto)], enable_libwww="$withval", enable_libwww="no") 

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
AC_ARG_WITH(xml-parser, [  --with-xml-parser=NAME  Use XML parser - expat, libxml (default=libxml)], xml_parser="$withval", xml_parser="libxml") 




dnl Checks for RDF parsers
parser_modules=raptor
rdf_parsers_available=

# raptor is REQUIRED despite the checking here
AC_CHECK_PROG(RAPTOR_CONFIG, raptor-config, raptor-config)
AC_ARG_WITH(raptor, [  --with-raptor=SOURCE    Pick raptor source - system/internal (default=auto)], with_raptor="$withval", with_raptor="auto")

if test "X$RAPTOR_CONFIG" == "X"; then
  with_raptor=internal
else
  RAPTOR_VERSION=`$RAPTOR_CONFIG --version 2>/dev/null`
  RAPTOR_MIN_VERSION=1.3.0

  # FIXME - could use raptor-config --decimal-version at some point
  RAPTOR_VERSION_DEC=`echo $RAPTOR_VERSION | $AWK -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`
  RAPTOR_MIN_VERSION_DEC=`echo $RAPTOR_MIN_VERSION | $AWK -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`

  if test "X$with_raptor" = Xauto; then
    if test "X$RAPTOR_VERSION" -a $RAPTOR_VERSION_DEC -ge $RAPTOR_MIN_VERSION_DEC; then
      with_raptor=system
    else
      with_raptor=internal
    fi
  elif test $with_raptor = system; then
    if test $RAPTOR_VERSION_DEC -lt $RAPTOR_MIN_VERSION_DEC; then
      AC_MSG_WARN(System raptor $RAPTOR_VERSION is too old - need $RAPTOR_MIN_VERSION)
      AC_MSG_WARN(Proceeding anyway since --with-raptor=system was given)
      AC_MSG_WARN(To use the internal raptor use --with-raptor=internal)
      AC_MSG_WARN(or omit any --with-raptor argument.)
    fi
  fi
fi

AC_MSG_CHECKING(raptor library source)
if test $with_raptor = system; then
  AC_MSG_RESULT(system $RAPTOR_VERSION)
  rdf_parsers_available="$parser_available raptor(system $RAPTOR_VERSION)"
else
  with_raptor=internal
  AC_MSG_RESULT(internal)
  rdf_parsers_available="$parser_available raptor(internal)"
fi
AC_SUBST(with_raptor)

AC_MSG_CHECKING(RDF parsers required)
AC_ARG_ENABLE(parsers, [  --enable-parsers=LIST   Use RDF parsers (default=raptor)], parser_modules="$enableval") 
if test "X$parser_modules" = Xno -o "X$parser_modules" = X; then
  parser_modules=
  AC_MSG_RESULT($parser_modules)
  AC_MSG_WARN(No parsers are enabled, however libraptor is still used internally)
else
  AC_MSG_RESULT($parser_modules)
fi

PARSER_OBJS=


# This is needed because autoheader can't work out which computed
# symbols must be pulled from acconfig.h into config.h.in
if test "x" = "y"; then
  AC_DEFINE(HAVE_RAPTOR_RDF_PARSER, 1, [Have Raptor RDF parser])
fi


# Always have these
raptor_dir=raptor

need_raptor=1
for parser in $parser_modules; do
  uname=`echo $parser | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
  found=0
  original_parser=$parser

  case $parser in
    raptor)
      need_raptor=1
      found=1
      ;;
    *)
      AC_MSG_ERROR(No such parser $parser)
  esac

  # Add objects, sources only if the parser was found
  if test $found = 1; then
    PARSER_OBJS="$PARSER_OBJS rdf_parser_$parser.lo"

    if test $parser = raptor; then
      if test $with_raptor = internal; then
        LIBRDF_INTERNAL_CPPFLAGS="-I\$(top_srcdir)/raptor $LIBRDF_INTERNAL_CPPFLAGS"
        LIBRDF_INTERNAL_LIBS="\$(top_builddir)/raptor/libraptor.la $LIBRDF_INTERNAL_LIBS"
      else
        LIBRDF_EXTERNAL_CPPFLAGS="`$RAPTOR_CONFIG --cflags` $LIBRDF_EXTERNAL_CPPFLAGS"
        LIBRDF_EXTERNAL_LIBS="`$RAPTOR_CONFIG --libs` $LIBRDF_EXTERNAL_LIBS"
        raptor_dir=
      fi
    fi

    n=HAVE_${uname}_RDF_PARSER
    AC_DEFINE_UNQUOTED($n)
  fi
done


# rasqal is REQUIRED despite the checking here
AC_CHECK_PROG(RASQAL_CONFIG, rasqal-config, rasqal-config)
AC_ARG_WITH(rasqal, [  --with-rasqal=SOURCE    Pick rasqal source - system/internal (default=auto)], with_rasqal="$withval", with_rasqal="auto")

if test "X$RAPTOR_CONFIG" == "X"; then
  with_rasqal=internal
else
  RASQAL_VERSION=`$RASQAL_CONFIG --version 2>/dev/null`
  RASQAL_MIN_VERSION=0.2.0

  RASQAL_VERSION_DEC=`$RASQAL_CONFIG --version-decimal`
  RASQAL_MIN_VERSION_DEC=`echo $RASQAL_MIN_VERSION | $AWK -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`

  if test "X$with_rasqal" = Xauto; then
    if test "X$RASQAL_VERSION" -a $RASQAL_VERSION_DEC -ge $RASQAL_MIN_VERSION_DEC; then
      with_rasqal=system
    else
      with_rasqal=internal
    fi
  elif test $with_rasqal = system; then
    if test $RASQAL_VERSION_DEC -lt $RASQAL_MIN_VERSION_DEC; then
      AC_MSG_WARN(System rasqal $RASQAL_VERSION is too old - need $RASQAL_MIN_VERSION)
      AC_MSG_WARN(Proceeding anyway since --with-rasqal=system was given)
      AC_MSG_WARN(To use the internal rasqal use --with-rasqal=internal)
      AC_MSG_WARN(or omit any --with-rasqal argument.)
    fi
  fi
fi

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
  AC_MSG_CHECKING(Python includes)
  python_prefix=`$PYTHON -c 'import sys; print sys.prefix' 2>/dev/null`
  python_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
  PYTHON_INCLUDES="-I${python_prefix}/include/python${PYTHON_VERSION}"
  if test "$python_prefix" != "$python_exec_prefix"; then
    PYTHON_INCLUDES="$PYTHON_INCLUDES -I${python_exec_prefix}/include/python${PYTHON_VERSION}"
  fi
  AC_SUBST(PYTHON_INCLUDES)
  AC_MSG_RESULT($PYTHON_INCLUDES)

  AC_MSG_CHECKING(Python libs)
  PYTHON_LIB="${python_prefix}/lib/python${PYTHON_VERSION}/site-packages"
  AC_SUBST(PYTHON_LIB)
  AC_MSG_RESULT($PYTHON_LIB)
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

if test "X$PARSER_OBJS" = X; then
  AC_MSG_WARN(No RDF parser configured)
fi



dnl Checks for modules
digest_modules="md5 sha1 ripemd160"

AC_MSG_CHECKING(digests wanted)
AC_ARG_ENABLE(digests, [  --enable-digests=LIST   Use digests (default=md5 sha1 ripemd160)], digest_modules="$enableval") 
AC_MSG_RESULT($digest_modules)

DIGEST_OBJS=
DIGEST_SRCS=

AC_ARG_WITH(openssl-digests, [  --with-openssl-digests  Use openssl digests (default=yes)], enable_openssl_digests="$withval", enable_openssl_digests="yes") 

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
  LIBRDF_CPPFLAGS="$LIBRDF_CPPFLAGS `$TSTORE_CONFIG --cflags` `$PKG_CONFIG glib --cflags`"
  LIBRDF_LIBS="$LIBRDF_LIBS `$TSTORE_CONFIG --libs` `$PKG_CONFIG glib --libs`"

  AC_DEFINE(HAVE_TSTORE, 1, [Have 3store available])

  STORAGE_OBJS="$STORAGE_OBJS rdf_storage_tstore.lo"
  store_modules="$store_modules 3store($tstore_version)"
  persistent_store=yes
  AC_MSG_RESULT(yes - version $tstore_version)
else
  AC_MSG_RESULT(no)
fi


AC_ARG_WITH(mysql, [  --with-mysql(=CONFIG|yes|no) Location of MySQL 4 mysql_config (default=auto)], mysql_config="$withval", mysql_config="")
if test "X$mysql_config" != "Xno"; then
  if test "X$mysql_config" != "Xyes" ; then
    MYSQL_CONFIG=$mysql_config
  fi
  mysql_config=yes
fi
if test $mysql_config = yes -a "X$MYSQL_CONFIG" = "X" ; then
  AC_CHECK_PROGS(MYSQL_CONFIG, mysql_config)
fi

libmysql_min_version=3.23.58
# Also tested on 4.0.14

if test "X$MYSQL_CONFIG" != X; then
  LIBS="$LIBRDF_LIBS `$MYSQL_CONFIG --libs|sed -e "s/'//g"`"
  CPPFLAGS="$LIBRDF_CPPFLAGS `$MYSQL_CONFIG --cflags|sed -e "s/'//g"`"
  AC_MSG_CHECKING(for mysql library)
  LIBMYSQL_VERSION=`$MYSQL_CONFIG --version`
  libmysql_version_dec=`echo $LIBMYSQL_VERSION | awk -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`
  libmysql_min_version_dec=`echo $libmysql_min_version | awk -F. '{printf("%d\n", 10000*$1 + 100*$2 + $3)};'`
  AC_MSG_RESULT(yes - version $LIBMYSQL_VERSION)
  if test $libmysql_version_dec -lt $libmysql_min_version_dec; then
    AC_MSG_WARN(Using libmysql $LIBMYSQL_VERSION is unsupported - $libmysql_min_version or newer recommended.)
  else
    LIBRDF_LIBS="$LIBRDF_LIBS `$MYSQL_CONFIG --libs|sed -e "s/'//g"`"
    LIBRDF_CPPFLAGS="$LIBRDF_CPPFLAGS `$MYSQL_CONFIG --cflags|sed -e "s/'//g"`"

    AC_DEFINE(HAVE_MYSQL,1,[MySQL libraries are present])
    have_libmysql=yes

    STORAGE_OBJS="$STORAGE_OBJS rdf_storage_mysql.lo"
    store_modules="$store_modules mysql($LIBMYSQL_VERSION)"
    persistent_store=yes
  fi
fi

AC_SUBST(STORAGE_OBJS)


CPPFLAGS="$LIBRDF_CPPFLAGS"
LDFLAGS="$LIBRDF_LDFLAGS"
LIBS="$LIBRDF_LIBS"


dnl Check for use of threads

AC_ARG_WITH(threads, [  --with-threads          With POSIX thread support (default=no)], with_threads="yes", with_threads="no")

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

# This is needed because autoheader can't work out which computed
# symbols must be pulled from acconfig.h into config.h.in
if test "x" = "y"; then
  AC_DEFINE(HAVE_LOCAL_MD5_DIGEST, 1, [Have local MD5 digest])
  AC_DEFINE(HAVE_LOCAL_SHA1_DIGEST, 1, [Have local SHA1 digest])
  AC_DEFINE(HAVE_LOCAL_RIPEMD160_DIGEST, 1, [Have local RIPEMD160 digest])
fi


HASH_OBJS=
HASH_SRCS=

store_modules="$store_modules hashes(memory)"
AC_MSG_CHECKING(for bdb hash support)
if test "$have_libdb" = yes; then
  AC_MSG_RESULT(yes);
  AC_DEFINE(HAVE_BDB_HASH, 1, [Have BDB hash support])
  HASH_OBJS="$HASH_OBJS rdf_hash_bdb.lo"
  HASH_SRCS="$HASH_SRCS rdf_hash_bdb.c"
  store_modules="$store_modules hashes(bdb $bdb_version)"
  persistent_store=yes
else
  AC_MSG_RESULT(no);
fi


AC_SUBST(HASH_OBJS)
AC_SUBST(HASH_SRCS)


if test "$persistent_store" = no; then
  AC_MSG_WARN(No persistant storage available)
fi


dnl RDF query
if test $with_rasqal = internal; then
  LIBRDF_INTERNAL_CPPFLAGS="-I\$(top_srcdir)/rasqal $LIBRDF_INTERNAL_CPPFLAGS"
  LIBRDF_INTERNAL_LIBS="\$(top_builddir)/rasqal/librasqal.la $LIBRDF_INTERNAL_LIBS"
else
  LIBRDF_EXTERNAL_CPPFLAGS="`$RASQAL_CONFIG --cflags` $LIBRDF_EXTERNAL_CPPFLAGS"
  LIBRDF_EXTERNAL_LIBS="`$RASQAL_CONFIG --libs` $LIBRDF_EXTERNAL_LIBS"
  rasqal_dir=
fi



dnl Checks for building sub-packages in sub directories
dnl before librdf (pre-requisites)
SD_PRE="$raptor_dir $rasqal_dir"
dnl after librdf (use it)
SD_POST="$languages"

AC_SUBST(SD_PRE)
AC_SUBST(SD_POST)


# Make final changes to cflags
CFLAGS="-DLIBRDF_INTERNAL=1 $CFLAGS"
MEM=
MEM_LIBS=
AC_ARG_WITH(dmalloc, [  --with-dmalloc          Use dmalloc debugging library (default=auto)], use_dmalloc="$withval", use_dmalloc="auto") 
AC_MSG_CHECKING(using dmalloc library)
if test "$USE_MAINTAINER_MODE" = yes; then
  if test "$ac_cv_header_dmalloc_h" = yes; then
    if test "X$use_dmalloc" = Xauto; then
      use_dmalloc=yes
    fi
  else
    use_dmalloc=no
  fi
  CFLAGS="-DLIBRDF_DEBUG=1 $CFLAGS"
else
  use_dmalloc=no
fi
AC_MSG_RESULT($use_dmalloc);
if test $use_dmalloc = yes; then
  MEM=-DLIBRDF_MEMORY_DEBUG_DMALLOC=1
  MEM_LIBS=-ldmalloc
fi

AC_ARG_ENABLE(assert, [  --disable-assert        Turn off run time assertion checks.  ], \
if test "$enableval" = "yes"; then \
  AC_DEFINE(LIBRDF_DISABLE_ASSERT, 1, [Disable run time assertion checks.])
fi;)	

AC_ARG_ENABLE(assert-messages, [  --disable-assert-messages  Turn off run time assertion check messages.  ], \
if test "$enableval" = "yes"; then \
  AC_DEFINE(LIBRDF_DISABLE_ASSERT_MESSAGES, 1, [Disable run time assertion check messages.])
fi;)	


STANDARD_CFLAGS="$STANDARD_CFLAGS $CFLAGS"
if test "$USE_MAINTAINER_MODE" = yes; then
  CFLAGS="$MAINTAINER_CFLAGS $CFLAGS"
fi

LIBS=
CPPFLAGS=
LDFLAGS=

LIBRDF_LIBTOOLLIBS=librdf.la

AC_SUBST(LIBRDF_EXTERNAL_CPPFLAGS)
AC_SUBST(LIBRDF_EXTERNAL_LIBS)

AC_SUBST(LIBRDF_INTERNAL_CPPFLAGS)
AC_SUBST(LIBRDF_INTERNAL_LIBS)

AC_SUBST(LIBRDF_CPPFLAGS)
AC_SUBST(LIBRDF_LDFLAGS)
AC_SUBST(LIBRDF_LIBS)
AC_SUBST(LIBRDF_LIBTOOLLIBS)

AC_SUBST(MEM)
AC_SUBST(MEM_LIBS)
AC_SUBST(STANDARD_CFLAGS)

abs_top_srcdir=`cd $srcdir; pwd`
AC_SUBST(abs_top_srcdir)
abs_top_builddir=`pwd`
AC_SUBST(abs_top_builddir)

AC_CONFIG_SUBDIRS([raptor rasqal])

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
demos/Makefile
librdf/redland.pc])
AC_CONFIG_FILES([librdf/redland.spec])
AC_CONFIG_FILES([redland-config], [chmod +x redland-config])
AC_CONFIG_FILES([redland-src-config], [chmod +x redland-src-config])

AC_OUTPUT

AC_MSG_RESULT([
Redland build summary:
  Berkeley/Sleepycat DB:  $bdb_available
  Languages available:   $languages_available
  Language APIs built:   $languages
  Triple stores:          $store_modules
  RDF parsers:           $rdf_parsers_available
  Content digests:       $digest_modules_available
])
