/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query.c - RDF Query Results
 *
 * $Id$
 *
 * Copyright (C) 2004-2004, David Beckett http://purl.org/net/dajobe/
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


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <redland.h>
#include <rdf_query.h>


/**
 * librdf_query_results_as_stream - Return the query results as statements
 * @query_results: &librdf_query_results object
 * @model: model to operate query on
 * 
 * Runs the query against the (previously registered) model
 * and returns a &librdf_stream of
 * matching &librdf_statement objects.
 * 
 * NOTE: Not Implemented.  Depends on rasqal_query_results class
 * 
 * Return value:  &librdf_stream of matching statements (may be empty) or NULL on failure
 **/
librdf_stream*
librdf_query_results_as_stream(librdf_query_results* query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  if(query_results->query->factory->results_as_stream)
    return query_results->query->factory->results_as_stream(query_results);
  else
    return NULL;
}




/**
 * librdf_query_get_result_count - Get number of bindings so far
 * @query_results: &librdf_query_results query results
 * 
 * Return value: number of bindings found so far
 **/
int
librdf_query_results_get_count(librdf_query_results *query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, 1);

  if(query_results->query->factory->results_get_count)
    return query_results->query->factory->results_get_count(query_results);
  else
    return 1;
}


/**
 * librdf_query_results_next - Move to the next result
 * @query_results: &librdf_query_results query results
 * 
 * Return value: non-0 if failed or results exhausted
 **/
int
librdf_query_results_next(librdf_query_results *query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, 1);

  if(query_results->query->factory->results_next)
    return query_results->query->factory->results_next(query_results);
  else
    return 1;
}


/**
 * librdf_query_results_finished - Find out if binding results are exhausted
 * @query_results: &librdf_query_results query results
 * 
 * Return value: non-0 if results are finished or query failed
 **/
int
librdf_query_results_finished(librdf_query_results *query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, 1);

  if(query_results->query->factory->results_finished)
    return query_results->query->factory->results_finished(query_results);
  else
    return 1;
}


/**
 * librdf_query_results_get_bindings - Get all binding names, values for current result
 * @query_results: &librdf_query_results query results
 * @names: pointer to an array of binding names (or NULL)
 * @values: pointer to an array of binding value &librdf_node (or NULL)
 * 
 * If names is not NULL, it is set to the address of a shared array
 * of names of the bindings (an output parameter).  These names
 * are shared and must not be freed by the caller
 *
 * If values is not NULL, it is used as an array to store pointers
 * to the librdf_node* of the results.  These nodes must be freed
 * by the caller.  The size of the array is determined by the
 * number of names of bindings, returned by
 * librdf_query_get_bindings_count dynamically or
 * will be known in advanced if hard-coded into the query string.
 * 
 * Example
 *
 * const char **names=NULL;
 * librdf_node* values[10];
 * 
 * if(librdf_query_results_get_bindings(results, &names, values))
 *   ...
 *
 * Return value: non-0 if the assignment failed
 **/
int
librdf_query_results_get_bindings(librdf_query_results *query_results, 
                                  const char ***names, librdf_node **values)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, 1);

  if(query_results->query->factory->results_get_bindings)
    return query_results->query->factory->results_get_bindings(query_results, names, values);
  else
    return 1;
}


/**
 * librdf_query_results_get_binding_value - Get one binding value for the current result
 * @query_results: &librdf_query_results query results
 * @offset: offset of binding name into array of known names
 * 
 * Return value: a new &librdf_node binding value or NULL on failure
 **/
librdf_node*
librdf_query_results_get_binding_value(librdf_query_results *query_results,
                                       int offset)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  if(query_results->query->factory->results_get_binding_value)
    return query_results->query->factory->results_get_binding_value(query_results, offset);
  else
    return NULL;
}


/**
 * librdf_query_results_get_binding_name - Get binding name for the current result
 * @query_results: &librdf_query_results query results
 * @offset: offset of binding name into array of known names
 * 
 * Return value: a pointer to a shared copy of the binding name or NULL on failure
 **/
const char*
librdf_query_results_get_binding_name(librdf_query_results *query_results, int offset)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  if(query_results->query->factory->results_get_binding_name)
    return query_results->query->factory->results_get_binding_name(query_results, offset);
  else
    return NULL;
}


/**
 * librdf_query_results_get_binding_value_by_name - Get one binding value for a given name in the current result
 * @query_results: &librdf_query_results query results
 * @name: variable name
 * 
 * Return value: a new &librdf_node binding value or NULL on failure
 **/
librdf_node*
librdf_query_results_get_binding_value_by_name(librdf_query_results *query_results, const char *name)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  if(query_results->query->factory->results_get_binding_value_by_name)
    return query_results->query->factory->results_get_binding_value_by_name(query_results, name);
  else
    return NULL;
}


/**
 * librdf_query_results_get_bindings_count - Get the number of bound variables in the result
 * @query_results: &librdf_query_results query results
 * 
 * Return value: <0 if failed or results exhausted
 **/
int
librdf_query_results_get_bindings_count(librdf_query_results *query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, 1);

  if(query_results->query->factory->results_get_bindings_count)
    return query_results->query->factory->results_get_bindings_count(query_results);
  else
    return -1;
}


/**
 * librdf_free_query_results - Destructor - destroy a librdf_query_results object
 * @query_results: &librdf_query_results object
 * 
 **/
void
librdf_free_query_results(librdf_query_results* query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(query_results, librdf_query_results);

  if(query_results->query->factory->free_results)
    query_results->query->factory->free_results(query_results);

  librdf_query_remove_query_result(query_results->query, query_results);

  LIBRDF_FREE(librdf_query_results, query_results);
}
