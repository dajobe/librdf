/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query.h - RDF Query Adaptor Factory and Query interfaces and definitions
 *
 * $Id$
 *
 * Copyright (C) 2002-2004 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.bris.ac.uk/
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
  
  /* perform the query on a model, returning results as a stream - OPTIONAL */
  librdf_stream* (*run_as_stream)(librdf_query* query, librdf_model* model);

  /* perform the query on a model, returning results as bindings  - OPTIONAL */
  int (*run_as_bindings)(librdf_query* query, librdf_model* model);

  /* get number of results (so far) - OPTIONAL */
  int (*get_result_count)(librdf_query *query);

  /* find out if binding results are exhausted - OPTIONAL */
  int (*results_finished)(librdf_query *query);

  /* get all binding names, values for current result - OPTIONAL */
  int (*get_result_bindings)(librdf_query *query, const char ***names, librdf_node **values);

  /* get one value for current result - OPTIONAL */
  librdf_node* (*get_result_binding_value)(librdf_query *query, int offset);

  /* get one name for current result - OPTIONAL */
  const char* (*get_result_binding_name)(librdf_query *query, int offset);

  /* get one value by name for current result - OPTIONAL */
  librdf_node* (*get_result_binding_by_name)(librdf_query *query, const char *name);

  /* get next result - OPTIONAL */
  int (*next_result)(librdf_query *query);
  
};


/* module init */
void librdf_init_query(librdf_world *world);

/* module terminate */
void librdf_finish_query(librdf_world *world);

/* class methods */
librdf_query_factory* librdf_get_query_factory(librdf_world *world, const char *name, librdf_uri* uri);

void librdf_query_triples_constructor(librdf_world *world);
void librdf_query_rasqal_constructor(librdf_world *world);

void librdf_query_rasqal_destructor(void);

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
REDLAND_API librdf_stream* librdf_query_run_as_stream(librdf_query* query, librdf_model *model);
REDLAND_API int librdf_query_run_as_bindings(librdf_query* query, librdf_model *model);
REDLAND_API int librdf_query_get_result_count(librdf_query *query);
REDLAND_API int librdf_query_results_finished(librdf_query *query);
REDLAND_API int librdf_query_get_result_bindings(librdf_query *query, const char ***names, librdf_node **values);
REDLAND_API librdf_node* librdf_query_get_result_binding_value(librdf_query *query, int offset);
REDLAND_API const char* librdf_query_get_result_binding_name(librdf_query *query, int offset);
REDLAND_API librdf_node* librdf_query_get_result_binding_by_name(librdf_query *query, const char *name);
REDLAND_API int librdf_query_next_result(librdf_query *query);
REDLAND_API int librdf_query_get_bindings_count(librdf_query *query);

#ifdef __cplusplus
}
#endif

#endif
