/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_heuristics.c - Heuristic routines to guess things about RDF
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
#include <ctype.h>
#include <stdarg.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for strtol */
#endif

#ifdef HAVE_STRING_H
#include <string.h> /* for strncpy */
#endif

#include <librdf.h>
#include <rdf_heuristics.h>


/**
 * librdf_heuristic_object_is_literal - try to guess if an object string is a literal or a resource
 * @object: string object to guess type
 *
 * The guessing is done by assuming the object is a URL if it matches
 *   ^[isalnum()]+:[^isblank()]+$
 *
 * This will be fooled by literals of form 'thing:non-blank-thing' but
 * is good enough.
 * 
 * Return value: non 0 if object is probably a literal
 **/

int
librdf_heuristic_object_is_literal(char *object) 
{
  int object_is_literal=1; /* assume the worst */

  /* Find first non alphanumeric */
  for(;*object; object++)
    if(!isalnum(*object))
       break;

  /* Better be a ':' */
  if(*object && *object == ':') {
    /* check rest of string has no spaces */
    for(;*object; object++)
      if(isspace(*object))
        break;

    /* reached end, not a literal (by this heuristic) */
    if(!*object)
      object_is_literal=0;
  }
  
  return object_is_literal;
 
}


/**
 * librdf_heuristic_gen_name - Generate a new name from an existing name
 * @name: the name
 * 
 * Adds an integer or increases the integer at the end of the name
 * in order to generate a new one
 * 
 * Return value: a new name or NULL on failure
 **/
char *
librdf_heuristic_gen_name(char *name) 
{
  char *new_name;
  char *p=name;
  size_t len;
  size_t offset;
  long l=-1L;

  /* Move to last character of name */
  len=strlen(name);
  offset=len-1;
  p=name+offset;

  /* Move p to last non number char */
  if(isdigit(*p)) {
    while(isdigit(*p))
      p--;
    l=strtol(p+1, (char**)NULL, 10);
    offset=p-name;
  }
   
  if(l<0)
    l=0;
  l++;

  /* +1 to required length if no digit was found */
  if(offset == len-1) 
    len++;

  /* +1 to required length if an extra digit is needed -
   * number now ends in 0.  Note l is never 0. */
  if((l % 10) ==0) 
    len++;

  new_name=(char*)LIBRDF_MALLOC(cstring, len+1); /* +1 for \0 */
  strncpy(new_name, name, offset+2);
  sprintf(new_name+offset+1, "%ld", l);
  return new_name;
}


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  char *test_names[]={"test", "abc123", "99997", NULL};
  char *name;
  int n;
  
  char *program=argv[0];

  for(n=0; (name=test_names[n]); n++) {
    int i;
    
    fprintf(stdout, "%s: Generating 11 new names from '%s'\n", program, name);
  
    for(i=0; i<11; i++) {
      char *new_name;
      
      fprintf(stdout, "%s: Generated name from '%s' is ", program, name);
      new_name=librdf_heuristic_gen_name(name);
      if(!new_name) {
        fputs("failed\n", stderr);
        break;
      }
      fprintf(stdout, "'%s'\n", new_name);
      
      if(name != test_names[n])
        LIBRDF_FREE(cstring, name);
      /* copy them over */
      name=new_name;
    }

    if(name != test_names[n])
      LIBRDF_FREE(cstring, name);
  }

#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
 
  /* keep gcc -Wall happy */
  return(0);
}

#endif
