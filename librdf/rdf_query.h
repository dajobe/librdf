/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query.h - RDF Query Adaptor Factory and Query interfaces and definitions
 *
 * $Id$
 *
 * Copyright (C) 2002 David Beckett - http://purl.org/net/dajobe/
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


#ifndef LIBRDF_QUERY_H
#define LIBRDF_QUERY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDF_INTERNAL

/** A query object */
struct librdf_query_s
{
  librdf_world *world;
  void *context;
  struct librdf_query_factory_s* factory;
};


/** A Query Factory */
struct librdf_query_factory_s {
  librdf_world *world;
  struct librdf_query_factory_s* next;
  char* name;       /* name of query language - required */
  librdf_uri *uri;  /* URI of query language - may be NULL */
  
  /* the rest of this structure is populated by the
   * query-specific register function */
  size_t context_length;
  
  /* create a new query */
  int (*init)(librdf_query* query, const char *name, librdf_uri *uri, const unsigned char *query_string);
  
  /* copy a query */
  /* clone is assumed to do leave the new query in the same state
   * after an init() method on an existing query - i.e ready to
   * use but closed.
   */
  int (*clone)(librdf_query* new_query, librdf_query* old_query);

  /* destroy a query */
  void (*terminate)(librdf_query* query);
  
  /* make query be started */
  int (*open)(librdf_query* query);
  
  /* close query/model context */
  int (*close)(librdf_query* query);
  
  /* perform the query on a model, returning results as a stream  */
  librdf_stream* (*run)(librdf_query* query, librdf_model* model);
  
};


#include <rdf_query_triples.h>

/* module init */
void librdf_init_query(librdf_world *world);

/* module terminate */
void librdf_finish_query(librdf_world *world);

/* class methods */
librdf_query_factory* librdf_get_query_factory(const char *name, librdf_uri* uri);

#endif

/* class methods */
REDLAND_API void librdf_query_register_factory(librdf_world *world, const char *name, librdf_uri* uri, void (*factory) (librdf_query_factory*));

/* constructor */
REDLAND_API librdf_query* librdf_new_query(librdf_world* world, const char *name, librdf_uri* uri, const unsigned char *query_string);
REDLAND_API librdf_query* librdf_new_query_from_query (librdf_query* old_query);
REDLAND_API librdf_query* librdf_new_query_from_factory(librdf_world* world, librdf_query_factory* factory, const char *name, librdf_uri* uri, const unsigned char* query_string);

/* destructor */
REDLAND_API void librdf_free_query(librdf_query *query);


/* methods */
REDLAND_API int librdf_query_open(librdf_query* query);
REDLAND_API int librdf_query_close(librdf_query* query);

REDLAND_API librdf_stream* librdf_query_run(librdf_query* query, librdf_model *model);

#ifdef __cplusplus
}
#endif

#endif
