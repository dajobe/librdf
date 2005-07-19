/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query.c - RDF Query Results
 *
 * $Id$
 *
 * Copyright (C) 2004-2005, David Beckett http://purl.org/net/dajobe/
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
 * librdf_query_get_result_count - Get number of bindings so far
 * @query_results: #librdf_query_results query results
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
 * @query_results: #librdf_query_results query results
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
 * @query_results: #librdf_query_results query results
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
 * @query_results: #librdf_query_results query results
 * @names: pointer to an array of binding names (or NULL)
 * @values: pointer to an array of binding value #librdf_node (or NULL)
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
 * @query_results: #librdf_query_results query results
 * @offset: offset of binding name into array of known names
 * 
 * Return value: a new #librdf_node binding value or NULL on failure
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
 * @query_results: #librdf_query_results query results
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
 * @query_results: #librdf_query_results query results
 * @name: variable name
 * 
 * Return value: a new #librdf_node binding value or NULL on failure
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
 * @query_results: #librdf_query_results query results
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
 * @query_results: #librdf_query_results object
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


/**
 * librdf_query_results_to_counted_string - Turn a query results into a string
 * @query_results: #librdf_query_results object
 * @format_uri: URI of syntax to format to
 * @base_uri: Base URI of output formatted syntax  or NULL
 * @length_p: Pointer to where to store length of string or NULL
 * 
 * Values of format_uri currently supported (via Rasqal) are:
 *  http://www.w3.org/TR/2004/WD-rdf-sparql-XMLres-20041221/
 *
 * The base URI may be used for the generated syntax, depending
 * on the format.
 *
 * The returned string must be freed by the caller
 *
 * Return value: new string value or NULL on failure
 **/
unsigned char*
librdf_query_results_to_counted_string(librdf_query_results *query_results,
                                       librdf_uri *format_uri,
                                       librdf_uri *base_uri,
                                       size_t *length_p) {
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  if(query_results->query->factory->results_to_counted_string)
    return query_results->query->factory->results_to_counted_string(query_results, format_uri, base_uri, length_p);
  else
    return NULL;
}


/**
 * librdf_query_results_to_string - Turn a query results into a string
 * @query_results: #librdf_query_results object
 * @format_uri: URI of syntax to format to
 * @base_uri: Base URI of output formatted syntax 
 * 
 * See librdf_query_results_to_counted_string for information on the
 * format_uri and base_uri parameters.
 *
 * The returned string must be freed by the caller
 *
 * Return value: new string value or NULL on failure
 **/
unsigned char*
librdf_query_results_to_string(librdf_query_results *query_results,
                               librdf_uri *format_uri,
                               librdf_uri *base_uri) {

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  return librdf_query_results_to_counted_string(query_results, 
                                                format_uri, base_uri, NULL);
}


/**
 * librdf_query_results_to_file_handle - Write a query results to a FILE*
 * @query_results: #librdf_query_results object
 * @handle: file handle to write to
 * @format_uri: URI of syntax to format to
 * @base_uri: Base URI of output formatted syntax 
 * 
 * See librdf_query_results_to_counted_string for information on the
 * format_uri and base_uri parameters.
 *
 * Return value: non 0 on failure
 **/
int
librdf_query_results_to_file_handle(librdf_query_results *query_results, 
                                    FILE *handle, 
                                    librdf_uri *format_uri,
                                    librdf_uri *base_uri) {
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(handle, FILE*, 1);

  if(query_results->query->factory->results_to_file_handle)
    return query_results->query->factory->results_to_file_handle(query_results,
                                                                 handle,
                                                                 format_uri, 
                                                                 base_uri);
  else
    return 1;
}


/**
 * librdf_query_results_to_file_handle - Write a query results to a file
 * @query_results: #librdf_query_results object
 * @name: filename to write to
 * @format_uri: URI of syntax to format to
 * @base_uri: Base URI of output formatted syntax 
 * 
 * See librdf_query_results_to_counted_string for information on the
 * format_uri and base_uri parameters.
 *
 * Return value: non 0 on failure
 **/
int
librdf_query_results_to_file(librdf_query_results *query_results, 
                             const char *name,
                             librdf_uri *format_uri,
                             librdf_uri *base_uri) {
  FILE* fh;
  int status;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(name, string, 1);

  fh=fopen(name, "w+");
  if(!fh)
    return 1;

  status=librdf_query_results_to_file_handle(query_results, fh, 
                                             format_uri, base_uri);
  fclose(fh);
  return status;
}


/**
 * librdf_query_results_is_bindings - test if librdf_query_results is variable bindings format
 * @query_results: #librdf_query_results object
 * 
 * Return value: non-0 if true
 **/
int
librdf_query_results_is_bindings(librdf_query_results* query_results) {
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_is_bindings)
    return query_results->query->factory->results_is_bindings(query_results);
  else
    return -1;
}
  

/**
 * librdf_query_results_is_boolean - test if librdf_query_results is boolean format
 * @query_results: #librdf_query_results object
 * 
 * Return value: non-0 if true
 **/
int
librdf_query_results_is_boolean(librdf_query_results* query_results) {
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_is_boolean)
    return query_results->query->factory->results_is_boolean(query_results);
  else
    return -1;
}


/**
 * librdf_query_results_is_graph - test if librdf_query_results is RDF graph format
 * @query_results: #librdf_query_results object
 * 
 * Return value: non-0 if true
 **/
int
librdf_query_results_is_graph(librdf_query_results* query_results) {
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_is_graph)
    return query_results->query->factory->results_is_graph(query_results);
  else
    return -1;
}


/**
 * librdf_query_results_get_boolean - Get boolean query result
 * @query_results: #librdf_query_results query_results
 *
 * The return value is only meaningful if this is a boolean
 * query result - see #librdf_query_results_is_boolean
 *
 * Return value: boolean query result - >0 is true, 0 is false, <0 on error or finished
 */
int
librdf_query_results_get_boolean(librdf_query_results* query_results) {
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_get_boolean)
    return query_results->query->factory->results_get_boolean(query_results);
  else
    return -1;
}


/**
 * librdf_query_results_as_stream - Get RDF graph query result
 * @query_results: #librdf_query_results query_results
 *
 * The return value is only meaningful if this is an RDF graph
 * query result - see #librdf_query_results_is_graph
 *
 * Return value: RDF graph query result or NULL on error
 */
librdf_stream*
librdf_query_results_as_stream(librdf_query_results* query_results) {
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, NULL);

  if(query_results->query->factory->results_as_stream)
    return query_results->query->factory->results_as_stream(query_results);
  else
    return NULL;
}
