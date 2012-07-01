/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query.c - RDF Query Results and Query Results Formatter classes
 *
 * Copyright (C) 2004-2010, David Beckett http://www.dajobe.org/
 * Copyright (C) 2004-2005, University of Bristol, UK http://www.bristol.ac.uk/
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
#include <stdlib.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#endif

#include <redland.h>
#include <rdf_query.h>


/**
 * librdf_query_results_get_count:
 * @query_results: #librdf_query_results query results
 *
 * Get number of bindings so far.
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
 * librdf_query_results_next:
 * @query_results: #librdf_query_results query results
 *
 * Move to the next result.
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
 * librdf_query_results_finished:
 * @query_results: #librdf_query_results query results
 *
 * Find out if binding results are exhausted.
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
 * librdf_query_results_get_bindings:
 * @query_results: #librdf_query_results query results
 * @names: pointer to an array of binding names (or NULL)
 * @values: pointer to an array of binding value #librdf_node (or NULL)
 *
 * Get all binding names, values for current result.
 * 
 * If names is not NULL, it is set to the address of a shared array
 * of names of the bindings (an output parameter).  These names
 * are shared and must not be freed by the caller
 *
 * If values is not NULL, it is used as an array to store pointers
 * to the librdf_node* of the results.  These nodes must be freed
 * by the caller.  The size of the array is determined by the
 * number of names of bindings, returned by
 * librdf_query_results_get_bindings_count dynamically or
 * will be known in advanced if hard-coded into the query string.
 * 
 * Example
 *
 * const char **names=NULL;
 * librdf_node* values[10];
 * 
 * if(librdf_query_results_get_bindings(results, &amp;names, values))
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
 * librdf_query_results_get_binding_value:
 * @query_results: #librdf_query_results query results
 * @offset: offset of binding name into array of known names
 *
 * Get one binding value for the current result.
 * 
 * Return value: a new #librdf_node binding value or NULL on failure
 **/
librdf_node*
librdf_query_results_get_binding_value(librdf_query_results *query_results,
                                       int offset)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  /* FIXME - offset really should be an unsigned int */
  if(offset < 0)
    return NULL;

  if(query_results->query->factory->results_get_binding_value)
    return query_results->query->factory->results_get_binding_value(query_results, offset);
  else
    return NULL;
}


/**
 * librdf_query_results_get_binding_name:
 * @query_results: #librdf_query_results query results
 * @offset: offset of binding name into array of known names
 *
 * Get binding name for the current result.
 * 
 * Return value: a pointer to a shared copy of the binding name or NULL on failure
 **/
const char*
librdf_query_results_get_binding_name(librdf_query_results *query_results, int offset)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  /* FIXME - offset really should be an unsigned int */
  if(offset < 0)
    return NULL;

  if(query_results->query->factory->results_get_binding_name)
    return query_results->query->factory->results_get_binding_name(query_results, offset);
  else
    return NULL;
}


/**
 * librdf_query_results_get_binding_value_by_name:
 * @query_results: #librdf_query_results query results
 * @name: variable name
 *
 * Get one binding value for a given name in the current result.
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
 * librdf_query_results_get_bindings_count:
 * @query_results: #librdf_query_results query results
 *
 * Get the number of bound variables in the result.
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
 * librdf_free_query_results:
 * @query_results: #librdf_query_results object
 *
 * Destructor - destroy a #librdf_query_results object.
 * 
 **/
void
librdf_free_query_results(librdf_query_results* query_results)
{
  if(!query_results)
    return;
  
  if(query_results->query->factory->free_results)
    query_results->query->factory->free_results(query_results);

  librdf_query_remove_query_result(query_results->query, query_results);

  LIBRDF_FREE(librdf_query_results, query_results);
}


/**
 * librdf_query_results_to_counted_string2:
 * @query_results: #librdf_query_results object
 * @name: name of syntax to format to
 * @mime_type: mime type of syntax to format to (or NULL)
 * @format_uri: URI of syntax to format to (or NULL)
 * @base_uri: Base URI of output formatted syntax (or NULL)
 * @length_p: Pointer to where to store length of string (or NULL)
 *
 * Turn a query results into a string.
 * 
 * A query results format can be named, have a mime type, or
 * identified by a URI, all of which are optional.  The default
 * query results format will be used if @name, @mime_type and
 * @format_uri are all NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * The @base_uri may be used for as the base URI the generated
 * syntax, depending on the format.
 *
 * The returned string must be freed by the caller using
 * librdf_free_memory().
 *
 * Return value: new string value or NULL on failure
 **/
unsigned char*
librdf_query_results_to_counted_string2(librdf_query_results *query_results,
                                        const char *name,
                                        const char *mime_type,
                                        librdf_uri *format_uri,
                                        librdf_uri *base_uri,
                                        size_t *length_p)
{
  librdf_query_results_formatter *formatter;
  void *string=NULL;
  size_t string_length=0;
  raptor_iostream *iostr;
  int error=0;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  iostr = raptor_new_iostream_to_string(query_results->query->world->raptor_world_ptr,
                                        &string, &string_length, malloc);
  if(!iostr)
    return NULL;
              
  formatter = librdf_new_query_results_formatter2(query_results,
                                                  name, mime_type,
                                                  format_uri);
  if(!formatter) {
    error=1;
    goto tidy;
  }

  if(librdf_query_results_formatter_write(iostr, formatter,
                                          query_results, base_uri))
    error=1;

  librdf_free_query_results_formatter(formatter);

 tidy:
  raptor_free_iostream(iostr);

  /* string is available only after the iostream is finished
   * - clean it up here on error */
  if(error) {
    if(string) {
      raptor_free_memory(string);
      string=NULL;
    }
  }
  else if(length_p)
    *length_p = string_length;
  
  return (unsigned char *)string;
}


#ifndef REDLAND_DISABLE_DEPRECATED
/**
 * librdf_query_results_to_counted_string:
 * @query_results: #librdf_query_results object
 * @format_uri: URI of syntax to format to (or NULL)
 * @base_uri: Base URI of output formatted syntax (or NULL)
 * @length_p: Pointer to where to store length of string (or NULL)
 *
 * Turn a query results into a string.
 * 
 * The default query results format will be used if @format_uri is
 * NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * The @base_uri may be used for as the base URI the generated
 * syntax, depending on the format.
 *
 * The returned string must be freed by the caller using
 * librdf_free_memory().
 *
 * @deprecated: Use librdf_query_results_to_counted_string2() with extra
 * name and mime-type args.
 *
 * Return value: new string value or NULL on failure
 **/
unsigned char*
librdf_query_results_to_counted_string(librdf_query_results *query_results,
                                       librdf_uri *format_uri,
                                       librdf_uri *base_uri,
                                       size_t *length_p)
{
  return librdf_query_results_to_counted_string2(query_results,
                                                 NULL /* name */,
                                                 NULL /* mime type */,
                                                 format_uri,
                                                 base_uri,
                                                 length_p);
}
#endif


/**
 * librdf_query_results_to_string2:
 * @query_results: #librdf_query_results object
 * @name: format name
 * @mime_type: format mime type (or NULL)
 * @format_uri: URI of syntax to format to (or NULL)
 * @base_uri: Base URI of output formatted syntax (or NULL)
 *
 * Turn a query results into a string.
 * 
 * A query results format can be named, have a mime type, or
 * identified by a URI, all of which are optional.  The default
 * query results format will be used if @name, @mime_type and @format_uri
 * are all NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * The @base_uri may be used for as the base URI the generated
 * syntax, depending on the format.
 *
 * The returned string must be freed by the caller using
 * librdf_free_memory().
 *
 * Return value: new string value or NULL on failure
 **/
unsigned char*
librdf_query_results_to_string2(librdf_query_results *query_results,
                                const char* name,
                                const char* mime_type,
                                librdf_uri *format_uri,
                                librdf_uri *base_uri)
{

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  return librdf_query_results_to_counted_string2(query_results,
                                                 name, mime_type, format_uri,
                                                 base_uri, NULL);
}


#ifndef REDLAND_DISABLE_DEPRECATED
/**
 * librdf_query_results_to_string:
 * @query_results: #librdf_query_results object
 * @format_uri: URI of syntax to format to
 * @base_uri: Base URI of output formatted syntax (or NULL)
 *
 * Turn a query results into a string.
 * 
 * A query results format can be named, have a mime type, or
 * identified by a URI, all of which are optional.  The default
 * query results format will be used if @format_uri is NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * The @base_uri may be used for as the base URI the generated
 * syntax, depending on the format.
 *
 * The returned string must be freed by the caller using
 * librdf_free_memory().
 *
 * @Deprecated: use librdf_query_results_to_string2() with extra name
 * and mime_type args.
 *
 * Return value: new string value or NULL on failure
 **/
unsigned char*
librdf_query_results_to_string(librdf_query_results *query_results,
                               librdf_uri *format_uri,
                               librdf_uri *base_uri)
{

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, librdf_query_results, NULL);

  return librdf_query_results_to_string2(query_results,
                                         NULL, NULL, format_uri,
                                         base_uri);
}
#endif


/**
 * librdf_query_results_to_file_handle2:
 * @query_results: #librdf_query_results object
 * @handle: file handle to write to
 * @name: result format name (or NULL)
 * @mime_type: result mime type (or NULL)
 * @format_uri: URI of syntax to format to (or NULL)
 * @base_uri: Base URI of output formatted syntax 
 *
 * Write a query results to a FILE*.
 * 
 * A query results format can be named, have a mime type, or
 * identified by a URI, all of which are optional.  The default
 * query results format will be used if @name, @mime_type and @format_uri
 * are all NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * The @base_uri may be used for as the base URI the generated syntax, depending
 * on the format.
 *
 * Return value: non 0 on failure
 **/
int
librdf_query_results_to_file_handle2(librdf_query_results *query_results, 
                                     FILE *handle, 
                                     const char *name,
                                     const char *mime_type,
                                     librdf_uri *format_uri,
                                     librdf_uri *base_uri)
{
  raptor_iostream *iostr;
  librdf_query_results_formatter *formatter;
  int status;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(handle, FILE*, 1);


  iostr = raptor_new_iostream_to_file_handle(query_results->query->world->raptor_world_ptr,
                                             handle);
  if(!iostr)
    return 1;

  formatter = librdf_new_query_results_formatter2(query_results,
                                                  name, mime_type,
                                                  format_uri);
  if(!formatter) {
    raptor_free_iostream(iostr);
    return 1;
  }

  status = librdf_query_results_formatter_write(iostr, formatter,
                                                query_results, base_uri);

  librdf_free_query_results_formatter(formatter);

  raptor_free_iostream(iostr);

  return status;
}


#ifndef REDLAND_DISABLE_DEPRECATED
/**
 * librdf_query_results_to_file_handle:
 * @query_results: #librdf_query_results object
 * @handle: file handle to write to
 * @format_uri: URI of syntax to format to
 * @base_uri: Base URI of output formatted syntax (or NULL)
 *
 * Write a query results to a FILE*.
 * 
 * A query results format can be named, have a mime type, or
 * identified by a URI, all of which are optional.  The default
 * query results format will be used if @format_uri is NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * The @base_uri may be used for as the base URI the generated
 * syntax, depending on the format.
 *
 * @Deprecated: use librdf_query_results_to_file_handle() with extra
 * name and mime_type args.
 *
 * Return value: non 0 on failure
 **/
int
librdf_query_results_to_file_handle(librdf_query_results *query_results, 
                                    FILE *handle, 
                                    librdf_uri *format_uri,
                                    librdf_uri *base_uri)
{
  return librdf_query_results_to_file_handle2(query_results, 
                                              handle,
                                              NULL /* name */,
                                              NULL /* mime type */,
                                              format_uri,
                                              base_uri);
}
#endif


/**
 * librdf_query_results_to_file2:
 * @query_results: #librdf_query_results object
 * @name: filename to write to
 * @mime_type: mime type (or NULL)
 * @format_uri: URI of syntax to format to (or NULL)
 * @base_uri: Base URI of output formatted syntax (or NULL)
 *
 * Write a query results to a file.
 * 
 * A query results format can be named, have a mime type, or
 * identified by a URI, all of which are optional.  The default
 * query results format will be used if @name, @mime_type and @format_uri
 * are all NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * The @base_uri may be used for as the base URI the generated
 * syntax, depending on the format.
 *
 * Return value: non 0 on failure
 **/
int
librdf_query_results_to_file2(librdf_query_results *query_results, 
                             const char *name,
                             const char *mime_type,
                             librdf_uri *format_uri,
                             librdf_uri *base_uri)
{
  FILE* fh;
  int status;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(name, string, 1);

  fh = fopen(name, "w+");
  if(!fh) {
    librdf_log(query_results->query->world, 0, LIBRDF_LOG_ERROR, 
               LIBRDF_FROM_QUERY, NULL, 
               "failed to open file '%s' for writing - %s",
               name, strerror(errno));
    return 1;
  }

  status = librdf_query_results_to_file_handle2(query_results, fh, 
                                                name, mime_type,
                                                format_uri, base_uri);
  fclose(fh);
  return status;
}


#ifndef REDLAND_DISABLE_DEPRECATED
/**
 * librdf_query_results_to_file:
 * @query_results: #librdf_query_results object
 * @name: filename to write to
 * @format_uri: URI of syntax to format to
 * @base_uri: Base URI of output formatted syntax (or NULL)
 *
 * Write a query results to a file.
 * 
 * A query results format can be named, have a mime type, or
 * identified by a URI, all of which are optional.  The default
 * query results format will be used if @format_uri is NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * The @base_uri may be used for as the base URI the generated
 * syntax, depending on the format.
 *
 * @Deprecated: use librdf_query_results_to_file2() with extra mime_type
 * arg.
 *
 * Return value: non 0 on failure
 **/
int
librdf_query_results_to_file(librdf_query_results *query_results, 
                             const char *name,
                             librdf_uri *format_uri,
                             librdf_uri *base_uri)
{
  return librdf_query_results_to_file2(query_results,
                                       name,
                                       NULL /* mime type */,
                                       format_uri,
                                       base_uri);
}
#endif


/**
 * librdf_query_results_is_bindings:
 * @query_results: #librdf_query_results object
 *
 * Test if librdf_query_results is variable bindings format.
 * 
 * Return value: non-0 if true
 **/
int
librdf_query_results_is_bindings(librdf_query_results* query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_is_bindings)
    return query_results->query->factory->results_is_bindings(query_results);
  else
    return -1;
}
  

/**
 * librdf_query_results_is_boolean:
 * @query_results: #librdf_query_results object
 *
 * Test if librdf_query_results is boolean format.
 * 
 * If this function returns true, the result can be retrieved by
 * librdf_query_results_get_boolean().
 *
 * Return value: non-0 if true
 **/
int
librdf_query_results_is_boolean(librdf_query_results* query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_is_boolean)
    return query_results->query->factory->results_is_boolean(query_results);
  else
    return -1;
}


/**
 * librdf_query_results_is_graph:
 * @query_results: #librdf_query_results object
 *
 * Test if librdf_query_results is RDF graph format.
 * 
 * Return value: non-0 if true
 **/
int
librdf_query_results_is_graph(librdf_query_results* query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_is_graph)
    return query_results->query->factory->results_is_graph(query_results);
  else
    return -1;
}


/**
 * librdf_query_results_is_syntax:
 * @query_results: #librdf_query_results object
 *
 * Test if librdf_query_results is a syntax.
 *
 * If this function returns true, the ONLY result available
 * from this query is a syntax that can be serialized using
 * one of the #query_result_formatter class methods or with
 * librdf_query_results_to_counted_string(), librdf_query_results_to_string(),
 * librdf_query_results_to_file_handle() or librdf_query_results_to_file()
 * 
 * Return value: non-0 if true
 **/
int
librdf_query_results_is_syntax(librdf_query_results* query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_is_syntax)
    return query_results->query->factory->results_is_syntax(query_results);
  else
    return -1;
}


/**
 * librdf_query_results_get_boolean:
 * @query_results: #librdf_query_results query_results
 *
 * Get boolean query result.
 *
 * The return value is only meaningful if this is a boolean
 * query result - see librdf_query_results_is_boolean()
 *
 * Return value: boolean query result - >0 is true, 0 is false, <0 on error or finished
 */
int
librdf_query_results_get_boolean(librdf_query_results* query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, -1);

  if(query_results->query->factory->results_get_boolean)
    return query_results->query->factory->results_get_boolean(query_results);
  else
    return -1;
}


/**
 * librdf_query_results_as_stream:
 * @query_results: #librdf_query_results query_results
 *
 * Get a query result as an RDF graph in #librdf_stream form
 *
 * The return value is only meaningful if this is an RDF graph
 * query result - see librdf_query_results_is_graph().
 *
 * Return value: a new #librdf_stream result or NULL on error
 */
librdf_stream*
librdf_query_results_as_stream(librdf_query_results* query_results)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query_results, query_results, NULL);

  if(query_results->query->factory->results_as_stream)
    return query_results->query->factory->results_as_stream(query_results);
  else
    return NULL;
}


/**
 * librdf_new_query_results_formatter2:
 * @query_results: #librdf_query_results query_results
 * @name: the query results format name (or NULL)
 * @mime_type: the query results format mime type (or NULL)
 * @uri: #librdf_uri query results format uri (or NULL)
 *
 * Constructor - create a new librdf_query_results_formatter object by identified format.
 *
 * A query results format can be named, have a mime type, or
 * identified by a URI, all of which are optional.  The default query
 * results format will be used if @name, @mime_type and @uri are all
 * NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * Return value: a new #librdf_query_results_formatter object or NULL on failure
 */
librdf_query_results_formatter*
librdf_new_query_results_formatter2(librdf_query_results* query_results,
                                    const char *name, const char *mime_type,
                                    librdf_uri* uri)
{
  if(query_results->query->factory->new_results_formatter)
    return query_results->query->factory->new_results_formatter(query_results, name, mime_type, uri);
  else
    return NULL;
}


#ifndef REDLAND_DISABLE_DEPRECATED
/**
 * librdf_new_query_results_formatter:
 * @query_results: #librdf_query_results query_results
 * @name: the query results format name (or NULL)
 * @uri: #librdf_uri query results format uri (or NULL)
 *
 * Constructor - create a new librdf_query_results_formatter object by identified format.
 *
 * A query results format can be named or identified by a URI, both
 * of which are optional.  The default query results format will be used
 * if @name and @uri are both NULL.
 *
 * librdf_query_results_formats_enumerate() returns information on
 * the known query results names, labels and URIs.
 *
 * @Deprecated: for librdf_new_query_results_formatter2() with the
 * name, mime_type and format_uri args.
 *
 * Return value: a new #librdf_query_results_formatter object or NULL on failure
 */
librdf_query_results_formatter*
librdf_new_query_results_formatter(librdf_query_results* query_results,
                                   const char *name, librdf_uri* uri)
{
  if(query_results->query->factory->new_results_formatter)
    return query_results->query->factory->new_results_formatter(query_results, name, NULL, uri);
  else
    return NULL;
}
#endif


#ifndef REDLAND_DISABLE_DEPRECATED
/**
 * librdf_new_query_results_formatter_by_mime_type:
 * @query_results: #librdf_query_results query_results
 * @mime_type: mime type name
 *
 * Constructor - create a new librdf_query_results_formatter object by mime type.
 *
 * A query results format generates a syntax with a mime type which
 * may be requested with this constructor.

 * Note that there may be several formatters that generate the same
 * MIME Type (such as SPARQL XML results format drafts) and in thot
 * case the librdf_new_query_results_formatter() constructor allows
 * selecting of a specific one by name or URI.
 *
 * @Deprecated: for librdf_new_query_results_formatter2() with the
 * name, mime_type and format_uri args.
 *
 * Return value: a new #librdf_query_results_formatter object or NULL on failure
 */
librdf_query_results_formatter*
librdf_new_query_results_formatter_by_mime_type(librdf_query_results* query_results,
                                                const char *mime_type)
{
  if(query_results->query->factory->new_results_formatter)
    return query_results->query->factory->new_results_formatter(query_results, NULL, mime_type, NULL);
  else
    return NULL;
}
#endif


/**
 * librdf_free_query_results_formatter:
 * @formatter: #librdf_query_results_formatter object
 * 
 * Destructor - destroy a #librdf_query_results_formatter object.
 **/
void
librdf_free_query_results_formatter(librdf_query_results_formatter* formatter) 
{
  if(!formatter)
    return;
  
  if(formatter->query_results->query->factory->free_results_formatter)
    formatter->query_results->query->factory->free_results_formatter(formatter);
}


/**
 * librdf_query_results_formatter_write:
 * @iostr: #raptor_iostream to write the query to
 * @formatter: #librdf_query_results_formatter object
 * @query_results: #librdf_query_results query results format
 * @base_uri: #librdf_uri base URI of the output format
 *
 * Write the query results using the given formatter to an iostream
 * 
 * Note that after calling this method, the query results will be
 * empty and librdf_query_results_finished() will return true (non-0)
 *
 * See librdf_query_results_formats_enumerate() to get the
 * list of syntax URIs and their description. 
 *
 * Return value: non-0 on failure
 **/
int
librdf_query_results_formatter_write(raptor_iostream *iostr,
                                     librdf_query_results_formatter* formatter,
                                     librdf_query_results* query_results,
                                     librdf_uri *base_uri)
{
  if(query_results->query->factory->results_formatter_write)
    return query_results->query->factory->results_formatter_write(iostr,
                                                                  formatter,
                                                                  query_results,
                                                                  base_uri);
  else
    return 1;
}


/**
 * librdf_query_results_formats_check:
 * @world: #librdf_world
 * @name: the query results format name (or NULL)
 * @uri: #librdf_uri query results format uri (or NULL)
 * @mime_type: mime type name
 * 
 * Check if a query results formatter exists for the requested format.
 * 
 * Return value: non-0 if a formatter exists.
 **/
int
librdf_query_results_formats_check(librdf_world* world, 
                                   const char *name, librdf_uri* uri,
                                   const char *mime_type)
{
  int flags = RASQAL_QUERY_RESULTS_FORMAT_FLAG_READER;
  
  librdf_world_open(world);

  /* FIXME - this should use some kind of registration but for now
   * it is safe to assume Rasqal does it all
   */
  return rasqal_query_results_formats_check(world->rasqal_world_ptr,
                                            name, (raptor_uri*)uri, mime_type,
                                            flags);
}


/**
 * librdf_query_results_formats_enumerate:
 * @world: #librdf_world
 * @counter: index into the list of query result syntaxes
 * @name: pointer to store the name of the query result syntax (or NULL)
 * @label: pointer to store query result syntax readable label (or NULL)
 * @uri_string: pointer to store query result syntax URI string (or NULL)
 * @mime_type: pointer to store query result syntax mime type string (or NULL)
 *
 * Get information on query result syntaxes.
 * 
 * All returned strings are shared and must be copied if needed to be
 * used dynamically.
 * 
 * @Deprecated: use librdf_query_results_formats_get_description() to
 * return more information in a static structure.
 *
 * Return value: non 0 on failure of if counter is out of range
 */
int
librdf_query_results_formats_enumerate(librdf_world* world,
                                       const unsigned int counter,
                                       const char **name,
                                       const char **label,
                                       const unsigned char **uri_string,
                                       const char **mime_type)
{
  const raptor_syntax_description *desc;

  librdf_world_open(world);

  /* FIXME - this should use some kind of registration but for now
   * it is safe to assume Rasqal does it all
   */
  desc = rasqal_world_get_query_results_format_description(world->rasqal_world_ptr, counter);
  if(!desc)
    return -1;
  
  /* First element of names, uri_strings, mime_types arrays is always
   * the main one 
   */
  if(name && desc->names)
    *name = desc->names[0];

  if(label)
    *label = desc->label;

  if(uri_string && desc->uri_strings)
    *uri_string = (const unsigned char *)desc->uri_strings[0];

  if(mime_type && desc->mime_types)
    *mime_type = desc->mime_types[0].mime_type;

  return 0;
}


/**
 * librdf_query_results_formats_get_description:
 * @world: world object
 * @counter: index into the list of query results formats
 *
 * Get query result formats descriptive syntax information
 * 
 * Return value: description or NULL if counter is out of range
 **/
const raptor_syntax_description*
librdf_query_results_formats_get_description(librdf_world* world, 
                                             unsigned int counter)
{
  librdf_world_open(world);

  return rasqal_world_get_query_results_format_description(world->rasqal_world_ptr,
                                                           counter);
}
