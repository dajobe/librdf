/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_heuristics.c - Heuristic routines used by librdf to guess things
 *
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */


#include <rdf_config.h>

#include <stdio.h>
#include <ctype.h>

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
