/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_heuristics.c - Heuristic routines to guess things about RDF
 *
 * $Id$
 *
 * Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology, University of Bristol.
 *
 *    This package is Free Software available under either of two licenses
 *    (see FAQS.html to see why):
 * 
 * 1. The GNU Lesser General Public License (LGPL)
 * 
 *    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
 *    full license text.
 *      _________________________________________________________________
 * 
 *      Copyright (C) 2000 David Beckett, Institute for Learning and
 *      Research Technology, University of Bristol. All Rights Reserved.
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public License
 *      as published by the Free Software Foundation; either version 2 of
 *      the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *      USA
 *      _________________________________________________________________
 * 
 *    NOTE - under Term 3 of the LGPL, you may choose to license the entire
 *    library under the GPL. See COPYING for the full license text.
 * 
 * 2. The Mozilla Public License
 * 
 *    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
 *    license text.
 * 
 *    Under MPL section 13. I declare that all of the Covered Code is
 *    Multiple Licensed:
 *      _________________________________________________________________
 * 
 *      The contents of this file are subject to the Mozilla Public License
 *      version 1.1 (the "License"); you may not use this file except in
 *      compliance with the License. You may obtain a copy of the License
 *      at http://www.mozilla.org/MPL/
 * 
 *      Software distributed under the License is distributed on an "AS IS"
 *      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 *      the License for the specific language governing rights and
 *      limitations under the License.
 * 
 *      The Initial Developer of the Original Code is David Beckett.
 *      Portions created by David Beckett are Copyright (C) 2000 David
 *      Beckett, Institute for Learning and Research Technology, University
 *      of Bristol. All Rights Reserved.
 * 
 *      Alternatively, the contents of this file may be used under the
 *      terms of the GNU Lesser General Public License, in which case the
 *      provisions of the LGPL License are applicable instead of those
 *      above. If you wish to allow use of your version of this file only
 *      under the terms of the LGPL License and not to allow others to use
 *      your version of this file under the MPL, indicate your decision by
 *      deleting the provisions above and replace them with the notice and
 *      other provisions required by the LGPL License. If you do not delete
 *      the provisions above, a recipient may use your version of this file
 *      under either the MPL or the LGPL License.
 */


#include <rdf_config.h>

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for strtol */
#endif

#ifdef HAVE_STRING_H
#include <string.h> /* for strncpy */
#endif

#define LIBRDF_INTERNAL 1
#include <librdf.h>


/**
 * librdf_heuristic_object_is_literal - try to guess if an object string is a literal or a resource
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
  int offset;
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
