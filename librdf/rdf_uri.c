/*
 * RDF URI Implementation
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>

#include <stdio.h>

#include <rdf_config.h>

/* This define tells rdf_uri.h not to include this file again */
#define RDF_INSIDE_RDF_URI_C
#include <rdf_uri.h>


static rdf_digest_factory* rdf_uri_digest_factory;

/* class methods */
void rdf_init_uri(rdf_digest_factory* factory) 
{
  rdf_uri_digest_factory=factory;
}


#ifndef RDF_URI_INLINE

/* constructors */
INLINE rdf_uri*
rdf_new_uri (char *uri_string) {
  char *new_uri=(char*)RDF_MALLOC(rdf_uri, strlen(uri_string)+1);
  if(!new_uri)
    return 0;

  strcpy(new_uri, uri_string);
  return (rdf_uri*)new_uri;
}

INLINE rdf_uri*
rdf_new_uri_from_uri (rdf_uri* old_uri) {
  char *new_uri=(char*)RDF_MALLOC(rdf_uri, strlen(old_uri)+1);
  if(!new_uri)
    return 0;

  strcpy(new_uri, old_uri);
  return (rdf_uri*)new_uri;
}

/* destructor */
INLINE void
rdf_free_uri (rdf_uri* uri) 
{
  RDF_FREE(rdf_uri, uri);
}

/* methods */

/* note: does not allocate a new string */
INLINE char*
rdf_uri_as_string (rdf_uri *uri) 
{
  return (char*)uri;
}

rdf_digest*
rdf_uri_get_digest (rdf_uri* uri) 
{
  rdf_digest* d;

  d=rdf_new_digest(rdf_uri_digest_factory);
  if(!d)
    return NULL;

  rdf_digest_init(d);
  rdf_digest_update(d, (unsigned char*)uri, strlen(uri));
  rdf_digest_final(d);
  
  return d;
}


void
rdf_uri_print (rdf_uri* uri, FILE *fh) 
{
  fputs(uri, fh);
}


/* allocates a new string since this is a _to_ method */
char*
rdf_uri_to_string (rdf_uri* uri)
{
  char *s=(char*)RDF_MALLOC(cstring, strlen(uri)+1);
  if(!s)
    return NULL;
  strcpy(s, uri);
  return s;
}


#endif
