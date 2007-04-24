/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * touch-mtime.c - Copy file modification times
 *
 * Copyright (C) 2007, David Beckett http://purl.org/net/dajobe/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 */


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif


/* one prototype needed */
int main(int argc, char *argv[]);

static char *program=NULL;

int
main(int argc, char *argv[]) 
{
  char *p;
  struct stat sb;
#ifdef HAVE_UTIMES
  struct timeval times[2];
#else
  struct utimbuf utime;
#endif
  
  program=argv[0];
  if((p=strrchr(program, '/')))
    program=p+1;
  else if((p=strrchr(program, '\\')))
    program=p+1;
  argv[0]=program;

  if(argc != 3) {
    fprintf(stderr, "USAGE: %s REFERENCE-FILE FILE\n", program);
    exit(1);
  }
  
  if(stat(argv[1], &sb)) {
    fprintf(stderr, "%s: stat(%s) failed - %s\n", program, argv[1],
            strerror(errno));
    exit(1);
  }

#ifdef HAVE_UTIMES
    /* access time */
    times[0].tv_sec=sb.st_atime;
    times[0].tv_usec=0;
    /* mod time */
    times[1].tv_sec=sb.st_mtime;
    times[1].tv_usec=0;
    
    if(utimes(argv[2], times)) {
      fprintf(stderr, "%s: utimes(%s) failed - %s\n", program, argv[2],
              strerror(errno));
      exit(1);
    }
#else
    /* access time */
    utime.actime=sb.st_atime;
    /* mod time */
    utime.modtime=sb.st_mtime;

    if(utime(argv[2], &utime)) {
      fprintf(stderr, "%s: utime(%s) failed - %s\n", program, argv[2],
              strerror(errno));
      exit(1);
    }
#endif
  
  exit(0);
}

