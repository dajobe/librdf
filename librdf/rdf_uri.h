/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_uri.h - RDF URI Definition
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



#ifndef LIBRDF_URI_H
#define LIBRDF_URI_H

#include <rdf_digest.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef LIBRDF_INTERNAL

struct librdf_uri_s
{
  librdf_world *world;
  char *string;
  int string_length; /* useful for fast comparisons (that fail) */
  int usage;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  int max_usage;
#endif
};

#endif

/* class methods */
void librdf_init_uri(librdf_world *world);
void librdf_finish_uri(librdf_world *world);

/* constructors */
librdf_uri* librdf_new_uri (librdf_world *world, const char *string);
/* Create a new URI from an existing URI - CLONE */
librdf_uri* librdf_new_uri_from_uri (librdf_uri* uri);
/* Create a new URI from an existing URI and qname */
librdf_uri* librdf_new_uri_from_uri_qname (librdf_uri* uri, const char *qname);

/* destructor */
void librdf_free_uri(librdf_uri *uri);

/* methods */
char* librdf_uri_as_string (librdf_uri *uri);
librdf_digest* librdf_uri_get_digest (librdf_uri *uri);
void librdf_uri_print (librdf_uri* uri, FILE *fh);
char* librdf_uri_to_string (librdf_uri* uri);
int librdf_uri_equals(librdf_uri* first_uri, librdf_uri* second_uri);
int librdf_uri_is_file_uri(librdf_uri* uri);
const char* librdf_uri_as_filename(librdf_uri* uri);
librdf_uri* librdf_new_uri_normalised_to_base(const char *uri_string, librdf_uri* source_uri, librdf_uri* base_uri);
librdf_uri* librdf_new_uri_relative_to_base(librdf_uri* base_uri, const char *uri_string);


#ifdef __cplusplus
}
#endif

#endif
