/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_files.c - RDF File and directory handling utilities
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.org/
 * University of Bristol - http://www.bristol.ac.uk/
 * 
 * This package is Free Software or Open Source available under the
 * following licenses (these are alternatives):
 *   1. GNU Lesser General Public License (LGPL)
 *   2. GNU General Public License (GPL)
 *   3. Mozilla Public License (MPL)
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * full license terms.
 * 
 * 
 */


#include <rdf_config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for mktemp(), mkstemp(), getenv() */
#endif
#ifdef HAVE_MKSTEMP
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* for close(), unlink() */
#endif
#endif

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#include <librdf.h>


char *
librdf_files_temporary_file_name(void) 
{
#if defined(HAVE_MKSTEMP) || defined(HAVE_MKTEMP)
  const char *tmp_dir;
  size_t length;
  char *name;
  static const char * const file_template="librdf_tmp_XXXXXX"; /* FIXME */
#ifdef HAVE_MKSTEMP
  int fd;
#endif

  /* FIXME: unix dependencies */
  tmp_dir=getenv("TMPDIR");
  if(!tmp_dir)
    tmp_dir="/tmp";

  length=strlen(tmp_dir) + strlen(file_template) + 2; /* 2: / sep and \/0 */
  
  name=(char*)LIBRDF_MALLOC(cstring, length);
  if(!name)
    return NULL;

  /* FIXME: unix dependency - file/dir separator */
  sprintf(name, "%s/%s", tmp_dir, file_template);
  
#ifdef HAVE_MKSTEMP
  /* Proritise mkstemp() since GNU libc says: Never use mktemp(). */
  fd=mkstemp(name);
  if(fd<0) {
    LIBRDF_FREE(cstring, name);
    return NULL;
  }
  close(fd);
  unlink(name);

  return name;  
#else
  return mktemp(name);
#endif

#else
#ifdef HAVE_TMPNAM
  /* GNU libc says: Never use this function. Use mkstemp(3) instead. */
  char *name;
  char *new_name;

  name=tmpnam(NULL); /* NULL ensures statically allocated */
  new_name=(char*)LIBRDF_MALLOC(cstring, strlen(name)+1);
  if(!new_name)
    return NULL;
  strcpy(new_name, name);

  return name;
#else /* not tmpnam(), mkstemp() or mktemp() */
HELP
#endif
#endif
}


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  char *program=argv[0];
  char *name;
  
  name=librdf_files_temporary_file_name();
  if(!name)
    fprintf(stderr, "%s: Failed to get temporary file anme\n", program);
  else {
    fprintf(stdout, "%s: Got temporary file name %s\n", program, name);
    LIBRDF_FREE(cstring, name);
  }

  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
