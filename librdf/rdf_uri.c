/*
 * rdf_uri.c - RDF URI Implementation
 *
 * $Source$
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


#include <config.h>

#include <stdio.h>

#define LIBRDF_INTERNAL 1
#include <rdf_config.h>

/* This define tells librdf_uri.h not to include this file again */
#define LIBRDF_INSIDE_LIBRDF_URI_C
#include <rdf_uri.h>


static librdf_digest_factory* librdf_uri_digest_factory;

/* class methods */
void librdf_init_uri(librdf_digest_factory* factory) 
{
  librdf_uri_digest_factory=factory;
}


#ifndef LIBRDF_URI_INLINE

/* constructors */
INLINE librdf_uri*
librdf_new_uri (char *uri_string) {
  char *new_uri=(char*)LIBRDF_MALLOC(librdf_uri, strlen(uri_string)+1);
  if(!new_uri)
    return 0;

  strcpy(new_uri, uri_string);
  return (librdf_uri*)new_uri;
}

INLINE librdf_uri*
librdf_new_uri_from_uri (librdf_uri* old_uri) {
  char *new_uri=(char*)LIBRDF_MALLOC(librdf_uri, strlen(old_uri)+1);
  if(!new_uri)
    return 0;

  strcpy(new_uri, old_uri);
  return (librdf_uri*)new_uri;
}

/* destructor */
INLINE void
librdf_free_uri (librdf_uri* uri) 
{
  LIBRDF_FREE(librdf_uri, uri);
}

/* methods */

/* note: does not allocate a new string */
INLINE char*
librdf_uri_as_string (librdf_uri *uri) 
{
  return (char*)uri;
}

librdf_digest*
librdf_uri_get_digest (librdf_uri* uri) 
{
  librdf_digest* d;

  d=librdf_new_digest(librdf_uri_digest_factory);
  if(!d)
    return NULL;

  librdf_digest_init(d);
  librdf_digest_update(d, (unsigned char*)uri, strlen(uri));
  librdf_digest_final(d);
  
  return d;
}


void
librdf_uri_print (librdf_uri* uri, FILE *fh) 
{
  fputs(uri, fh);
}


/* allocates a new string since this is a _to_ method */
char*
librdf_uri_to_string (librdf_uri* uri)
{
  char *s=(char*)LIBRDF_MALLOC(cstring, strlen(uri)+1);
  if(!s)
    return NULL;
  strcpy(s, uri);
  return s;
}


#endif
