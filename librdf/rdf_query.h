/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query.h - RDF Query Adaptor Factory and Query interfaces and definitions
 *
 * $Id$
 *
 * Copyright (C) 2002-2004, David Beckett http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology http://www.ilrt.bristol.ac.uk/
 * University of Bristol, UK http://www.bristol.ac.uk/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
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
  int usage;
  void *context;
  struct librdf_query_factory_s* factory;

  /* list of all the results for this query */
  librdf_query_results* results;
};


struct librdf_query_results_s
{
  /* query that this was executed over */
  librdf_query* query;

  /* next query result */
  librdf_query_results* next;
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
  int (*init)(librdf_query* query, const char *name, librdf_uri *uri, const unsigned char *query_string, librdf_uri *base_uri);
  
  /* copy a query */
  /* clone is assumed to do leave the new query in the same state
   * after an init() method on an existing query - i.e ready to
   * use but closed.
   */
  int (*clone)(librdf_query* new_query, librdf_query* old_query);

  /* destroy a query */
  void (*terminate)(librdf_query* query);
  
  /* perform the query on a model */
  librdf_query_results* (*execute)(librdf_query* query, librdf_model* model);

  /* get the query results as a stream - OPTIONAL */
  librdf_stream* (*results_as_stream)(librdf_query_results* query_results);

  /* get number of results (so far) - OPTIONAL */
  int (*results_get_count)(librdf_query_results* query_results);

  /* get next result - OPTIONAL */
  int (*results_next)(librdf_query_results* query_results);

  /* find out if binding results are exhausted - OPTIONAL */
  int (*results_finished)(librdf_query_results* query_results);

  /* get all binding names, values for current result - OPTIONAL */
  int (*results_get_bindings)(librdf_query_results* query_results, const char ***names, librdf_node **values);

  /* get one value for current result - OPTIONAL */
  librdf_node* (*results_get_binding_value)(librdf_query_results* query_results, int offset);

  /* get one name for current result - OPTIONAL */
  const char* (*results_get_binding_name)(librdf_query_results* query_results, int offset);

  /* get one value by name for current result - OPTIONAL */
  librdf_node* (*results_get_binding_value_by_name)(librdf_query_results* query_results, const char *name);

  /* get number of bound variables in the result - OPTIONAL */
  int (*results_get_bindings_count)(librdf_query_results* query_results);

  /* tidy up query results - OPTIONAL */
  void (*free_results)(librdf_query_results* query_results);

  /* turn the query results into a new string */
  unsigned char* (*results_to_counted_string)(librdf_query_results *query_results, librdf_uri *format_uri, librdf_uri *base_uri, size_t *length_p);

  /* write the query results to an existing file handle */
  int (*results_to_file_handle)(librdf_query_results *query_results, FILE *handle, librdf_uri *format_uri, librdf_uri *base_uri);
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

void librdf_query_add_query_result(librdf_query *query, librdf_query_results* query_results);
void librdf_query_remove_query_result(librdf_query *query, librdf_query_results* query_results);

#endif

/* class methods */
REDLAND_API void librdf_query_register_factory(librdf_world *world, const char *name, const unsigned char *uri_string, void (*factory) (librdf_query_factory*));

/* constructor */
REDLAND_API librdf_query* librdf_new_query(librdf_world* world, const char *name, librdf_uri* uri, const unsigned char *query_string, librdf_uri* base_uri);
REDLAND_API librdf_query* librdf_new_query_from_query (librdf_query* old_query);
REDLAND_API librdf_query* librdf_new_query_from_factory(librdf_world* world, librdf_query_factory* factory, const char *name, librdf_uri* uri, const unsigned char* query_string, librdf_uri* base_uri);

/* destructor */
REDLAND_API void librdf_free_query(librdf_query *query);


/* methods */
REDLAND_API librdf_query_results* librdf_query_execute(librdf_query* query, librdf_model *model);

REDLAND_API librdf_stream* librdf_query_results_as_stream(librdf_query_results* query_results);

REDLAND_API int librdf_query_results_get_count(librdf_query_results* query_results);
REDLAND_API int librdf_query_results_next(librdf_query_results* query_results);
REDLAND_API int librdf_query_results_finished(librdf_query_results* query_results);

REDLAND_API int librdf_query_results_get_bindings(librdf_query_results* query_results, const char ***names, librdf_node **values);
REDLAND_API librdf_node* librdf_query_results_get_binding_value(librdf_query_results* query_results, int offset);
REDLAND_API const char* librdf_query_results_get_binding_name(librdf_query_results* query_results, int offset);
REDLAND_API librdf_node* librdf_query_results_get_binding_value_by_name(librdf_query_results* query_results, const char *name);
REDLAND_API int librdf_query_results_get_bindings_count(librdf_query_results* query_results);
REDLAND_API unsigned char* librdf_query_results_to_counted_string(librdf_query_results *query_results, librdf_uri *format_uri, librdf_uri *base_uri, size_t *length_p);
REDLAND_API unsigned char* librdf_query_results_to_string(librdf_query_results *query_results, librdf_uri *format_uri, librdf_uri *base_uri);
REDLAND_API int librdf_query_results_to_file_handle(librdf_query_results *query_results, FILE *handle, librdf_uri *format_uri, librdf_uri *base_uri);
REDLAND_API int librdf_query_results_to_file(librdf_query_results *query_results, const char *name, librdf_uri *format_uri, librdf_uri *base_uri);

REDLAND_API void librdf_free_query_results(librdf_query_results* query_results);

#ifdef __cplusplus
}
#endif

#endif
