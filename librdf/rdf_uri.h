/*
 * RDF URI Definition / Implementation (if inline)
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


#ifndef RDF_URI_H
#define RDF_URI_H

#include <rdf_config.h>

#include <rdf_digest.h>

typedef char rdf_uri;

/* class methods */
void init_rdf_uri(rdf_digest_factory *factory);

/* constructors */
rdf_uri* new_rdf_uri (char *string);
/* Create a new URI from an existing URI - CLONE */
rdf_uri* new_rdf_uri_from_uri (rdf_uri* uri);

/* destructor */
void free_rdf_uri(rdf_uri *uri);

/* methods */
char* rdf_uri_as_string (rdf_uri *uri);
rdf_digest* rdf_uri_get_digest (rdf_uri *uri);
void rdf_uri_print (rdf_uri* uri, FILE *fh);
char* rdf_uri_to_string (rdf_uri* uri);


#if defined(RDF_URI_INLINE) && !defined(RDF_INSIDE_RDF_URI_C)
/* Please inline the functions */
#undef RDF_URI_INLINE
#include <rdf_uri.c>
#define RDF_URI_INLINE yes
#endif

#endif
