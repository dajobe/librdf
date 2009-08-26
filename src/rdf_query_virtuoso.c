/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query_virtuoso.c - RDF Query with Virtuoso
 *
 * Copyright (C) 2008, Openlink Software http://www.openlinksw.com/
 *
 * Based in part on rasqal_result_formats.c and rasqal_sparql_xml.c
 * (see NOTICE later in code)
 *
 * This package is Free Software and part of Redland http://librdf.org/
 *
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License(LGPL) V2.1 or any newer version
 *   2. GNU General Public License(GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 *
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 *
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
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
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <ctype.h>

#include <redland.h>

#include <sql.h>
#include <sqlext.h>

/*#define VIRTUOSO_STORAGE_DEBUG 1 */

#include <rdf_storage_virtuoso_internal.h>

#define RASQAL_INTERNAL
#include <rasqal_internal.h>
#undef RASQAL_INTERNAL

static librdf_query_results_formatter* virtuoso_new_results_formatter(librdf_query_results* query_results, const char *name, librdf_uri* uri, const char *mime_type);

static int librdf_query_virtuoso_results_next(librdf_query_results *query_results);

static const char *strexpect(const char *keyword, const char *source)
{
  while(isspace(*source))
    source++;

  while(*keyword &&(toupper(*keyword) == toupper(*source))) {
    keyword++;
    source++;
  }

  if(*keyword)
    return NULL;

  if(*source == '\0')
    return source;

  if(isspace(*source)) {
    while(isspace(*source))
      source++;
    return source;
  }

  return NULL;
}


static int
rdf_virtuoso_ODBC_Errors(const char *where, librdf_world *world, librdf_storage_virtuoso_connection* handle)
{
  SQLCHAR buf[512];
  SQLCHAR sqlstate[15];

  while(SQLError(handle->henv, handle->hdbc, handle->hstmt, sqlstate, NULL, buf, sizeof(buf), NULL) == SQL_SUCCESS) {
#ifdef VIRTUOSO_STORAGE_DEBUG
    fprintf(stderr, "%s ||%s, SQLSTATE=%s\n", where, buf, sqlstate);
#endif
    librdf_log(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL, "Virtuoso %s failed [%s] %s", where, sqlstate, buf);
  }

  while(SQLError(handle->henv, handle->hdbc, SQL_NULL_HSTMT, sqlstate, NULL, buf, sizeof(buf), NULL) == SQL_SUCCESS) {
#ifdef VIRTUOSO_STORAGE_DEBUG
    fprintf(stderr, "%s ||%s, SQLSTATE=%s\n", where, buf, sqlstate);
#endif
    librdf_log(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL, "Virtuoso %s failed [%s] %s", where, sqlstate, buf);
  }

  while(SQLError(handle->henv, SQL_NULL_HDBC, SQL_NULL_HSTMT, sqlstate, NULL, buf, sizeof(buf), NULL) == SQL_SUCCESS) {
#ifdef VIRTUOSO_STORAGE_DEBUG
    fprintf(stderr, "%s ||%s, SQLSTATE=%s\n", where, buf, sqlstate);
#endif
    librdf_log(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL, "Virtuoso %s failed [%s] %s", where, sqlstate, buf);
  }

  return -1;
}



/* functions implementing query api */


static void
virtuoso_free_result(librdf_query* query)
{
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  short i;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "virtuoso_free_result \n");
#endif
  if(context->colNames) {
    for(i=0; i < context->numCols; i++) {
      if(context->colNames[i])
        LIBRDF_FREE(cstring, (char *)context->colNames[i]);
    }
    LIBRDF_FREE(cstrings, (char **)context->colNames);
  }
  context->colNames=NULL;

  if(context->colValues) {
    for(i=0; i < context->numCols; i++) {
      if(context->colValues[i]) {
        librdf_free_node(context->colValues[i]);
      }
    }
    LIBRDF_FREE(librdf_node*, context->colValues);
  }
  context->colValues=NULL;
}


static int
librdf_query_virtuoso_init(librdf_query* query, const char *name, librdf_uri* uri, const unsigned char* query_string, librdf_uri *base_uri)
{
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  int len;
  unsigned char *query_string_copy;
  char *seps={(char *)" \t\n\r\f"};
  char *token;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_init \n");
#endif
  context->query=query;
  context->language=context->query->factory->name;
  context->offset=0;
  context->colNames=NULL;
  context->colValues=0;
  context->numCols=0;
  context->failed=0;
  context->eof=1;
  context->row_count=0;
  context->result_type=VQUERY_RESULTS_UNKNOWN;

  len=strlen((const char*)query_string);
  query_string_copy=(unsigned char*)LIBRDF_MALLOC(cstring, len+1);
  if(!query_string_copy)
    return 1;
  strcpy((char*)query_string_copy, (const char*)query_string);

  token=strtok((char*)query_string_copy, seps);

  while(token != NULL) {
    if(strexpect("SELECT", (const char*)token)) {
      context->result_type=VQUERY_RESULTS_BINDINGS;
      break;
    } else if(strexpect("ASK", (const char*)token)) {
      context->result_type=VQUERY_RESULTS_BOOLEAN;
      break;
    } else if(strexpect("CONSTRUCT", (const char*)token)) {
      context->result_type=VQUERY_RESULTS_GRAPH | VQUERY_RESULTS_BINDINGS;
      break;
    } else if(strexpect("DESCRIBE", (const char*)token)) {
      context->result_type=VQUERY_RESULTS_GRAPH | VQUERY_RESULTS_BINDINGS;
      break;
    }
    token=strtok(NULL, seps);
  }

  strcpy((char*)query_string_copy, (const char*)query_string);
  context->query_string=query_string_copy;
  if(base_uri)
    context->uri=librdf_new_uri_from_uri(base_uri);

  return 0;
}


static void
librdf_query_virtuoso_terminate(librdf_query* query)
{
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_terminate \n");
#endif
  virtuoso_free_result(query);
  SQLCloseCursor(context->vc->hstmt);

  if(context->query_string)
    LIBRDF_FREE(cstring, context->query_string);

  if(context->uri)
    librdf_free_uri(context->uri);

  if(context->vc)
    context->vc->v_release_connection(context->storage, context->vc);

  if(context->storage)
    librdf_storage_remove_reference(context->storage);
}


static librdf_query_results*
librdf_query_virtuoso_execute(librdf_query* query, librdf_model* model)
{
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  librdf_query_results* results;
  int rc=0;
  SQLUSMALLINT icol;
  char pref[]="sparql define output:format '_JAVA_' ";
  char *cmd=NULL;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_execute \n");
#endif
  context->model=model;
  context->numCols=0;
  context->failed=0;
  context->eof=1;
  context->row_count=0;
  context->limit= -1;
  context->offset= -1;
  virtuoso_free_result(query);
  SQLCloseCursor(context->vc->hstmt);

  if(!(cmd=(char*)LIBRDF_MALLOC(cstring, strlen(pref)+	strlen((char *)context->query_string) +1))) {
      goto error;
    }
  strcpy(cmd, pref);
  strcat(cmd, (char *)context->query_string);

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "SQL>>%s\n", cmd);
#endif
  rc=SQLExecDirect(context->vc->hstmt, (SQLCHAR *)cmd, SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    context->result_type=VQUERY_RESULTS_SYNTAX;
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", context->storage->world, context->vc);
    goto error;
  }

  rc=SQLNumResultCols(context->vc->hstmt, &context->numCols);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLNumResultCols()", context->storage->world, context->vc);
    goto error;
  }

  if(context->numCols > 0) {
    context->colNames =(char**)LIBRDF_CALLOC(cstrings, context->numCols+1, sizeof(char*));
    if(!context->colNames) {
      goto error;
    }

    context->colValues =(librdf_node **)LIBRDF_CALLOC(librdf_node**, context->numCols+1, sizeof(librdf_node*));
    if(!context->colValues) {
      goto error;
    }

    for(icol=1; icol<= context->numCols; icol++) {
      SQLSMALLINT namelen;
      SQLCHAR name[255];

      rc=SQLColAttributes(context->vc->hstmt, icol, SQL_COLUMN_LABEL, name, sizeof(name), &namelen, NULL);
      if(!SQL_SUCCEEDED(rc)) {
        rdf_virtuoso_ODBC_Errors("SQLColAttributes()", context->storage->world, context->vc);
        goto error;
      }

      context->colNames[icol-1] =(char*)LIBRDF_CALLOC(cstring, 1, namelen+1);
      if(!context->colNames[icol-1])
        goto error;

      strcpy(context->colNames[icol-1], (char *)name);
    }

    context->colNames[context->numCols]=NULL;
    context->result_type |= VQUERY_RESULTS_BINDINGS;
    context->eof=0;
  }

  results=(librdf_query_results*)LIBRDF_MALLOC(librdf_query_results, sizeof(librdf_query_results));
  if(!results) {
    SQLCloseCursor(context->vc->hstmt);
  } else {
    results->query=query;
  }

  rc=librdf_query_virtuoso_results_next(results);
  if(rc == 2) /* ERROR */
    goto error;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_execute OK\n");
#endif
  if(cmd)
    LIBRDF_FREE(cstring, (char *)cmd);

  return results;

error:
  if(cmd)
    LIBRDF_FREE(cstring, (char *)cmd);
  context->failed=1;
  virtuoso_free_result(query);
  return NULL;
}


static int
librdf_query_virtuoso_get_limit(librdf_query* query)
{
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  return context->limit;
}


static int
librdf_query_virtuoso_set_limit(librdf_query* query, int limit)
{
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  context->limit=limit;
  return 0;
}


static int
librdf_query_virtuoso_get_offset(librdf_query* query)
{
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  return context->offset;
}


static int
librdf_query_virtuoso_set_offset(librdf_query* query, int offset)
{
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  context->offset=offset;
  return 0;
}


/**
 * librdf_query_results_get_count:
 * @query_results: #librdf_query_results query results
 *
 * Get number of bindings so far.
 *
 * Return value: number of bindings found so far
 **/
static int
librdf_query_virtuoso_results_get_count(librdf_query_results *query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_get_count \n");
#endif
  if(!query)
    return -1;

  context =(librdf_query_virtuoso_context*)query->context;

  if(context->failed || context->numCols <= 0)
    return -1;

  if(context->numCols <= 0)
    return -1;

  return context->row_count;
}


/**
 * librdf_query_results_next:
 * @query_results: #librdf_query_results query results
 *
 * Move to the next result.
 *
 * Return value: non-0 if failed or results exhausted
 **/
static int
librdf_query_virtuoso_results_next(librdf_query_results *query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  int rc;
  short col;
  short numCols=context->numCols;
  librdf_node *node;
  char *data;
  int is_null;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_next \n");
#endif
  if(context->failed || context->eof)
    return 1;

  for(col=0; col < numCols; col++)
    if(context->colValues[col]) {
      librdf_free_node(context->colValues[col]);
      context->colValues[col]=NULL;
    }

  rc=SQLFetch(context->vc->hstmt);
  if(rc == SQL_NO_DATA_FOUND) {
    context->eof=1;
    return 1;
  } else if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    rdf_virtuoso_ODBC_Errors((char *)"SQLFetch", context->storage->world, context->vc);
    return 2;
  }

  for(col=1; col <= context->numCols; col++) {
    data=context->vc->v_GetDataCHAR(context->storage->world, context->vc, col, &is_null);
    if(!data && !is_null)
      return 2;

    if(!data || is_null) {
      node=NULL;
    } else {
      node=context->vc->v_rdf2node(context->storage, context->vc, col, data);
      LIBRDF_FREE(cstring, (char *) data);
      if(!node)
	return 2;
    }
    context->colValues[col-1]=node;
  }

  context->row_count++;
  return 0;
}


/**
 * librdf_query_results_finished:
 * @query_results: #librdf_query_results query results
 *
 * Find out if binding results are exhausted.
 *
 * Return value: non-0 if results are finished or query failed
 **/
static int
librdf_query_virtuoso_results_finished(librdf_query_results *query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_finished \n");
#endif
  if(context->failed || context->eof)
    return 1;

  return 0;
}


/**
 * librdf_query_results_get_bindings:
 * @query_results: #librdf_query_results query results
 * @names: pointer to an array of binding names(or NULL)
 * @values: pointer to an array of binding value #librdf_node(or NULL)
 *
 * Get all binding names, values for current result.
 *
 * If names is not NULL, it is set to the address of a shared array
 * of names of the bindings(an output parameter).  These names
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
 * if(librdf_query_results_get_bindings(results, &amp;names, values))
 *   ...
 *
 * Return value: non-0 if the assignment failed
 **/
static int
librdf_query_virtuoso_results_get_bindings(librdf_query_results *query_results, const char ***names, librdf_node **values)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  short col;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_get_bindings \n");
#endif
  if(context->failed || context->numCols <= 0)
    return 1;

  if(names)
    *names =(const char**)context->colNames;

  if(values && !context->eof) {

    for(col=0; col < context->numCols; col++) {
      values[col]=context->colValues[col];
      context->colValues[col]=NULL;
    }
  }

 return 0;
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
static librdf_node*
librdf_query_virtuoso_results_get_binding_value(librdf_query_results *query_results, int offset)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  librdf_node *retVal;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_get_binding_value \n");
#endif
  if(context->failed || context->numCols <= 0)
    return NULL;

  if(offset < 0 || offset > context->numCols-1 || !context->colValues)
    return NULL;

  retVal=context->colValues[offset];
  context->colValues[offset]=NULL;
  return retVal;
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
static const char*
librdf_query_virtuoso_results_get_binding_name(librdf_query_results *query_results, int offset)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_get_binding_name \n");
#endif
  if(context->failed || context->numCols <= 0)
    return NULL;

  if(offset < 0 || offset > context->numCols-1 || !context->colNames)
    return NULL;

  return context->colNames[offset];
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
static librdf_node*
librdf_query_virtuoso_results_get_binding_value_by_name(librdf_query_results *query_results, const char *name)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  short col;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_get_binding_value_by_name \n");
#endif
  if(context->failed || context->numCols <= 0)
    return NULL;

  if(!context->colNames || !context->colValues)
    return NULL;

  for(col=0; col < context->numCols; col++) {
    if(!strcmp((const char*)name, (const char*)context->colNames[col])) {
      return context->colValues[col];
      break;
    }
  }

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
static int
librdf_query_virtuoso_results_get_bindings_count(librdf_query_results *query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_get_bindings_count \n");
#endif
  if(context->failed || context->numCols <= 0)
    return -1;

  if(!context->colNames || !context->colValues)
    return -1;

  return context->numCols;
}


/**
 * librdf_free_query_results:
 * @query_results: #librdf_query_results object
 *
 * Destructor - destroy a #librdf_query_results object.
 *
 **/
static void
librdf_query_virtuoso_free_results(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_free_results \n");
#endif
  if(!context->failed && context->numCols) {
    SQLCloseCursor(context->vc->hstmt);
  }

  virtuoso_free_result(query);
  context->eof=1;
  context->numCols=0;
  context->row_count=0;
  context->result_type=VQUERY_RESULTS_UNKNOWN;
}


/**
 * librdf_query_results_is_bindings:
 * @query_results: #librdf_query_results object
 *
 * Test if librdf_query_results is variable bindings format.
 *
 * Return value: non-0 if true
 **/
static int
librdf_query_virtuoso_results_is_bindings(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_is_bindings \n");
#endif
  if(context->numCols <= 0)
    return 0;

  return(context->result_type & VQUERY_RESULTS_BINDINGS);
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
static int
librdf_query_virtuoso_results_is_boolean(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_is_boolean \n");
#endif
  if(context->numCols <= 0)
    return 0;

  return(context->result_type & VQUERY_RESULTS_BOOLEAN);
}


/**
 * librdf_query_results_is_graph:
 * @query_results: #librdf_query_results object
 *
 * Test if librdf_query_results is RDF graph format.
 *
 * Return value: non-0 if true
 **/
static int
librdf_query_virtuoso_results_is_graph(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_is_graph \n");
#endif
  if(context->numCols <= 0)
    return 0;

  return(context->result_type & VQUERY_RESULTS_GRAPH);
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
static int
librdf_query_virtuoso_results_is_syntax(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_is_syntax \n");
#endif
  if(context->numCols <= 0)
    return 0;

  return(context->result_type & VQUERY_RESULTS_SYNTAX);
}


/**
 * librdf_query_results_get_boolean:
 * @query_results: #librdf_query_results query_results
 *
 * Get boolean query result.
 *
 * The return value is only meaningful if this is a boolean
 * query result - see #librdf_query_results_is_boolean
 *
 * Return value: boolean query result - >0 is true, 0 is false, <0 on error or finished
 */
static int
librdf_query_virtuoso_results_get_boolean(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  int rc;
  int data;
  int is_null;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_get_boolean \n");
#endif
  if(context->failed || context->numCols <= 0)
    return -1;

  rc=SQLFetch(context->vc->hstmt);
  if(rc == SQL_NO_DATA_FOUND) {
    context->eof=1;
    return 0;
  } else if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    rdf_virtuoso_ODBC_Errors((char *)"SQLFetch", context->storage->world, context->vc);
    return -1;
  }

  rc=context->vc->v_GetDataINT(context->storage->world, context->vc, 1, &is_null, &data);

  context->eof=1;

  if(rc == -1)
    return -1;

  return data;
}


typedef struct {
  librdf_query *query;

  librdf_query_virtuoso_context* qcontext; /* query context */

  librdf_statement* statement; /* current statement */
  librdf_node *graph;
  int finished;
  short numCols;
} librdf_query_virtuoso_stream_context;


static int
librdf_query_virtuoso_query_results_end_of_stream(void* context)
{
  librdf_query_virtuoso_stream_context* scontext=(librdf_query_virtuoso_stream_context*)context;

  return scontext->finished;
}


static int
librdf_query_virtuoso_query_results_update_statement(void* context)
{
  librdf_query_virtuoso_stream_context* scontext=(librdf_query_virtuoso_stream_context*)context;
  librdf_query_virtuoso_context *qcontext=scontext->qcontext;
  librdf_world* world=scontext->query->world;
  librdf_node* node;
  short colNum;
  char *data;
  int is_null;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_query_results_update_statement \n");
#endif
  scontext->statement=librdf_new_statement(world);
  if(!scontext->statement)
    return 1;

  if(scontext->graph) {
    librdf_free_node(scontext->graph);
    scontext->graph=NULL;
  }

  if(!(qcontext->result_type & VQUERY_RESULTS_GRAPH))
    goto fail;

  colNum=1;
  if(colNum > scontext->numCols)
    goto fail;

  if(scontext->numCols > 3) {
    data=qcontext->vc->v_GetDataCHAR(world, qcontext->vc, colNum, &is_null);
    if(!data || is_null)
      goto fail;
    node=qcontext->vc->v_rdf2node(qcontext->storage, qcontext->vc, colNum, data);
    LIBRDF_FREE(cstring, (char *)data);
    if(!node)
      goto fail;
    scontext->graph=node;
    colNum++;
  }

  data=qcontext->vc->v_GetDataCHAR(world, qcontext->vc, colNum, &is_null);
  if(!data || is_null)
    goto fail;
  node=qcontext->vc->v_rdf2node(qcontext->storage, qcontext->vc, colNum, data);
  LIBRDF_FREE(cstring, (char *)data);
  if(!node)
    goto fail;

  librdf_statement_set_subject(scontext->statement, node);
  colNum++;

  if(colNum > scontext->numCols)
    goto fail;

  data=qcontext->vc->v_GetDataCHAR(world, qcontext->vc, colNum, &is_null);
  if(!data || is_null)
    goto fail;
  node=qcontext->vc->v_rdf2node(qcontext->storage, qcontext->vc, colNum, data);
  LIBRDF_FREE(cstring, (char *)data);
  if(!node)
    goto fail;

  librdf_statement_set_predicate(scontext->statement, node);
  colNum++;

  if(colNum > scontext->numCols)
    goto fail;

  data=qcontext->vc->v_GetDataCHAR(world, qcontext->vc, colNum, &is_null);
  if(!data || is_null)
    goto fail;
  node=qcontext->vc->v_rdf2node(qcontext->storage, qcontext->vc, colNum, data);
  LIBRDF_FREE(cstring, (char *)data);
  if(!node)
    goto fail;

  librdf_statement_set_object(scontext->statement, node);
  return 0; /* success */

fail:
  librdf_free_statement(scontext->statement);
  scontext->statement=NULL;
  return 1;
}


static int
librdf_query_virtuoso_query_results_next_statement(void* context)
{
  librdf_query_virtuoso_stream_context* scontext=(librdf_query_virtuoso_stream_context*)context;
  librdf_query_virtuoso_context *qcontext=scontext->qcontext;
  librdf_world* world=scontext->query->world;
  int rc;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_query_results_next_statement \n");
#endif
  if(scontext->finished)
    return 1;

  if(scontext->statement) {
    librdf_free_statement(scontext->statement);
    scontext->statement=NULL;
  }

  rc=SQLFetch(qcontext->vc->hstmt);
  if(rc == SQL_NO_DATA_FOUND) {
    scontext->finished=1;
  } else if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    rdf_virtuoso_ODBC_Errors((char *)"SQLFetch", world, qcontext->vc);
    scontext->finished=1;
  }

  if(!scontext->finished)
    librdf_query_virtuoso_query_results_update_statement(scontext);

  return scontext->finished;
}


static void*
librdf_query_virtuoso_query_results_get_statement(void* context, int flags)
{
  librdf_query_virtuoso_stream_context* scontext=(librdf_query_virtuoso_stream_context*)context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_query_results_get_statement \n");
#endif
  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->statement;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->graph;

    default:
      librdf_log(scontext->query->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL, "Unknown iterator method flag %d", flags);
      return NULL;
  }
}


static void
librdf_query_virtuoso_query_results_finished(void* context)
{
  librdf_query_virtuoso_stream_context* scontext=(librdf_query_virtuoso_stream_context*)context;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_query_results_finished \n");
#endif
  if(scontext) {
    if(scontext->statement)
      librdf_free_statement(scontext->statement);

    if(scontext->graph)
      librdf_free_node(scontext->graph);

    LIBRDF_FREE(librdf_query_virtuoso_stream_context, scontext);
  }
}


/**
 * librdf_query_results_as_stream:
 * @query_results: #librdf_query_results query_results
 *
 * Get a query result as an RDF graph in #librdf_stream form
 *
 * The return value is only meaningful if this is an RDF graph
 * query result - see #librdf_query_results_is_graph
 *
 * Return value: a new #librdf_stream result or NULL on error
 */
static librdf_stream*
librdf_query_virtuoso_results_as_stream(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;
  librdf_query_virtuoso_stream_context* scontext;
  librdf_stream *stream;
  int col;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_as_stream \n");
#endif
  if(context->failed || context->numCols < 3 || context->eof)
    return NULL;

  if((context->result_type & VQUERY_RESULTS_GRAPH) == 0)
    return NULL;

  scontext=(librdf_query_virtuoso_stream_context*)LIBRDF_CALLOC(librdf_query_virtuoso_stream_context, 1, sizeof(librdf_query_virtuoso_stream_context));
  if(!scontext)
    return NULL;

  scontext->query=query;
  scontext->qcontext=context;
  scontext->numCols=context->numCols;
  scontext->statement= librdf_new_statement(query->world);
  if(!scontext->statement)
    return NULL;

  col=0;
  if(scontext->numCols > 3) {
    scontext->graph=context->colValues[col];
    context->colValues[col]=NULL;
    col++;
  }

  librdf_statement_set_subject(scontext->statement, context->colValues[col]);
  context->colValues[col]=NULL;
  col++;
  if(col > scontext->numCols)
    goto fail;

  librdf_statement_set_predicate(scontext->statement, context->colValues[col]);
  context->colValues[col]=NULL;
  col++;
  if(col > scontext->numCols)
    goto fail;

  librdf_statement_set_object(scontext->statement, context->colValues[col]);
  context->colValues[col]=NULL;
  col++;
  if(col > scontext->numCols)
    goto fail;

  stream=librdf_new_stream(query->world,
                          (void*)scontext,
                           &librdf_query_virtuoso_query_results_end_of_stream,
                           &librdf_query_virtuoso_query_results_next_statement,
                           &librdf_query_virtuoso_query_results_get_statement,
                           &librdf_query_virtuoso_query_results_finished);
  if(!stream) {
    librdf_query_virtuoso_query_results_finished((void*)scontext);
    return NULL;
  }

  return stream;

fail:
  librdf_free_statement(scontext->statement);
  scontext->statement=NULL;
  librdf_query_virtuoso_query_results_finished((void*)scontext);
  return NULL;
}



/**
 * librdf_new_query_results_formatter:
 * @query_results: #librdf_query_results query_results
 * @name: the query results format name(or NULL)
 * @uri: #librdf_uri query results format uri(or NULL)
 *
 * Constructor - create a new librdf_query_results_formatter object by identified format.
 *
 * A query results format can be named or identified by a URI, both
 * of which are optional.  The default query results format will be used
 * if both are NULL.  librdf_query_results_formats_enumerate() returns
 * information on the known query results names, labels and URIs.
 *
 * Return value: a new #librdf_query_results_formatter object or NULL on failure
 */
static librdf_query_results_formatter*
librdf_query_virtuoso_new_results_formatter(librdf_query_results* query_results, const char *name, librdf_uri* uri)
{
#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_new_results_formatter \n");
#endif
  return virtuoso_new_results_formatter(query_results, name, uri, NULL);
}


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
 * MIME Type(such as SPARQL XML results format drafts) and in thot
 * case the librdf_new_query_results_formatter() constructor allows
 * selecting of a specific one by name or URI.
 *
 * Return value: a new #librdf_query_results_formatter object or NULL on failure
 */
static librdf_query_results_formatter*
librdf_query_virtuoso_new_results_formatter_by_mime_type(librdf_query_results* query_results, const char *mime_type)
{
#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_new_results_formatter_by_mime_type \n");
#endif
  return virtuoso_new_results_formatter(query_results, NULL, NULL, mime_type);
}


/**
 * librdf_free_query_results_formatter:
 * @formatter: #librdf_query_results_formatter object
 *
 * Destructor - destroy a #librdf_query_results_formatter object.
 **/
static void
librdf_query_virtuoso_free_results_formatter(librdf_query_results_formatter* qrf)
{
#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_free_results_formatter \n");
#endif

  if(qrf->formatter) {
    RASQAL_FREE(rasqal_query_results_format_factory, qrf->formatter->factory);
    rasqal_free_query_results_formatter(qrf->formatter);
  }
  LIBRDF_FREE(librdf_query_results, qrf);
}


/**
 * librdf_query_results_formatter_write:
 * @iostr: #raptor_iostream to write the query to
 * @formatter: #librdf_query_results_formatter object
 * @results: #librdf_query_results query results format
 * @base_uri: #librdf_uri base URI of the output format
 *
 * Write the query results using the given formatter to an iostream
 *
 * See librdf_query_results_formats_enumerate() to get the
 * list of syntax URIs and their description.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_query_virtuoso_results_formatter_write(raptor_iostream *iostr, librdf_query_results_formatter* qrf, librdf_query_results* query_results, librdf_uri *base_uri)
{

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_query_virtuoso_results_formatter_write \n");
#endif
  if(!qrf->formatter->factory->writer)
     return 1;
  return qrf->formatter->factory->writer(iostr, (rasqal_query_results *)query_results, (raptor_uri *)base_uri);
}


/* local function to register list query functions */


static void
librdf_query_virtuoso_register_factory(librdf_query_factory *factory)
{
  factory->context_length			= sizeof(librdf_query_virtuoso_context);

  factory->init					= librdf_query_virtuoso_init;
  factory->terminate				= librdf_query_virtuoso_terminate;
  factory->execute				= librdf_query_virtuoso_execute;
  factory->get_limit				= librdf_query_virtuoso_get_limit;
  factory->set_limit				= librdf_query_virtuoso_set_limit;
  factory->get_offset				= librdf_query_virtuoso_get_offset;
  factory->set_offset				= librdf_query_virtuoso_set_offset;

  factory->results_get_count			= librdf_query_virtuoso_results_get_count;
  factory->results_next				= librdf_query_virtuoso_results_next;
  factory->results_finished			= librdf_query_virtuoso_results_finished;
  factory->results_get_bindings			= librdf_query_virtuoso_results_get_bindings;
  factory->results_get_binding_value		= librdf_query_virtuoso_results_get_binding_value;
  factory->results_get_binding_name		= librdf_query_virtuoso_results_get_binding_name;
  factory->results_get_binding_value_by_name	= librdf_query_virtuoso_results_get_binding_value_by_name;

  factory->results_get_bindings_count		= librdf_query_virtuoso_results_get_bindings_count;
  factory->free_results				= librdf_query_virtuoso_free_results;
  factory->results_is_bindings			= librdf_query_virtuoso_results_is_bindings;
  factory->results_is_boolean			= librdf_query_virtuoso_results_is_boolean;
  factory->results_is_graph			= librdf_query_virtuoso_results_is_graph;
  factory->results_is_syntax			= librdf_query_virtuoso_results_is_syntax;
  factory->results_get_boolean			= librdf_query_virtuoso_results_get_boolean;
  factory->results_as_stream			= librdf_query_virtuoso_results_as_stream;

  factory->new_results_formatter		= librdf_query_virtuoso_new_results_formatter;
  factory->new_results_formatter_by_mime_type	= librdf_query_virtuoso_new_results_formatter_by_mime_type;
  factory->free_results_formatter		= librdf_query_virtuoso_free_results_formatter;
  factory->results_formatter_write		= librdf_query_virtuoso_results_formatter_write;
}


void
librdf_init_query_virtuoso(librdf_world *world)
{
  librdf_query_register_factory(world, "vsparql", (const unsigned char*)"http://www.w3.org/TR/rdf-vsparql-query/", &librdf_query_virtuoso_register_factory);
}



/*
 * NOTICE: The following code was duplicated from
 *
 *   rasqal_result_formats.c
 *   Copyright (C) 2003-2008, David Beckett http://www.dajobe.org/
 *   Copyright (C) 2003-2005, University of Bristol, UK http://www.bristol.ac.uk/
 *
 *   rasqal_sparql_xml.c
 *   Copyright (C) 2007-2008, David Beckett http://www.dajobe.org/
 *
 * and modified to use the librdf_ interfaces rather than the internal rasqal
 * structures.
 *
 * This code should be abstracted to avoid such duplication on future interfaces
 */
static void
virtuoso_query_simple_error(void* user_data, const char *message, ...)
{
  librdf_query* query=(librdf_query*)user_data;
  librdf_query_virtuoso_context *context=(librdf_query_virtuoso_context*)query->context;

  va_list arguments;

  va_start(arguments, message);

  context->failed=1;
  librdf_log(query->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL, message, arguments);
  va_end(arguments);
}


static int
virtuoso_query_results_write_sparql_xml(raptor_iostream *iostr, librdf_query_results* results, raptor_uri *base_uri)
{
  int rc=1;
  librdf_query* query=results->query;
#ifndef RAPTOR_V2_AVAILABLE
  const raptor_uri_handler *uri_handler;
  void *uri_context;
#endif
  raptor_xml_writer* xml_writer=NULL;
  raptor_namespace *res_ns=NULL;
  raptor_namespace_stack *nstack=NULL;
  raptor_xml_element *sparql_element=NULL;
  raptor_xml_element *results_element=NULL;
  raptor_xml_element *result_element=NULL;
  raptor_xml_element *element1=NULL;
  raptor_xml_element *binding_element=NULL;
  raptor_xml_element *variable_element=NULL;
  raptor_qname **attrs=NULL;
  int i;

  if(!librdf_query_results_is_bindings(results) && !librdf_query_results_is_boolean(results)) {
    librdf_log(query->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL, "Can only write XML format v3 for variable binding and boolean results");
    return 1;
  }

#ifdef RAPTOR_V2_AVAILABLE
  nstack=raptor_new_namespaces_v2(query->world->raptor_world_ptr, (raptor_simple_message_handler)virtuoso_query_simple_error, query, 1);
#else
  raptor_uri_get_handler(&uri_handler, &uri_context);
  nstack=raptor_new_namespaces(uri_handler, uri_context, (raptor_simple_message_handler)virtuoso_query_simple_error, query, 1);
#endif
  if(!nstack)
    return 1;

#ifdef RAPTOR_V2_AVAILABLE
  xml_writer=raptor_new_xml_writer_v2(query->world->raptor_world_ptr, nstack, iostr, (raptor_simple_message_handler)virtuoso_query_simple_error, query, 1);
#else
  xml_writer=raptor_new_xml_writer(nstack, uri_handler, uri_context, iostr, (raptor_simple_message_handler)virtuoso_query_simple_error, query, 1);
#endif
  if(!xml_writer)
    goto tidy;

  res_ns=raptor_new_namespace(nstack, NULL, (const unsigned char*)"http://www.w3.org/2005/sparql-results#", 0);
  if(!res_ns)
    goto tidy;

  sparql_element=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"sparql", NULL, base_uri);
  if(!sparql_element)
    goto tidy;

  if(librdf_query_results_is_bindings(results)) {
#if 0
    /* FIXME - consider when to write the XSD.  Need the XSD URI too. */
    raptor_namespace* xsi_ns;
    xsi_ns=raptor_new_namespace(nstack, (const unsigned char*)"xsi", (const unsigned char*)"http://www.w3.org/2001/XMLSchema-instance", 0);
    raptor_xml_element_declare_namespace(sparql_element, xsi_ns);

    attrs=(raptor_qname **)raptor_alloc_memory(sizeof(raptor_qname*));
    attrs[0]=raptor_new_qname_from_namespace_local_name(xsi_ns, (const unsigned char*)"schemaLocation", (const unsigned char*)"http://www.w3.org/2001/sw/DataAccess/rf1/result2.xsd");
    raptor_xml_element_set_attributes(sparql_element, attrs, 1);
#endif
  }

  raptor_xml_writer_start_element(xml_writer, sparql_element);
  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

  /*   <head> */
  element1=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"head", NULL, base_uri);
  if(!element1)
    goto tidy;

  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"  ", 2);
  raptor_xml_writer_start_element(xml_writer, element1);
  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

  if(librdf_query_results_is_bindings(results)) {
    for(i=0; 1; i++) {
      const unsigned char *name;
      name=(const unsigned char*)librdf_query_results_get_binding_name(results, i);
      if(!name)
        break;

      /*     <variable name="x"/> */
      variable_element=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"variable", NULL, base_uri);
      if(!variable_element)
        goto tidy;

      attrs=(raptor_qname **)raptor_alloc_memory(sizeof(raptor_qname*));
      if(!attrs)
        goto tidy;
#ifdef RAPTOR_V2_AVAILABLE
      attrs[0]=raptor_new_qname_from_namespace_local_name_v2(query->world->raptor_world_ptr, res_ns, (const unsigned char*)"name", (const unsigned char*)name); /* attribute value */
#else
      attrs[0]=raptor_new_qname_from_namespace_local_name(res_ns, (const unsigned char*)"name", (const unsigned char*)name); /* attribute value */
#endif
      if(!attrs[0]) {
        raptor_free_memory((void*)attrs);
        goto tidy;
      }

      raptor_xml_element_set_attributes(variable_element, attrs, 1);

      raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"    ", 4);
      raptor_xml_writer_empty_element(xml_writer, variable_element);
      raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

      raptor_free_xml_element(variable_element);
      variable_element=NULL;
    }
  }

  /* FIXME - could add <link> inside <head> */


  /*   </head> */
  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"  ", 2);
  raptor_xml_writer_end_element(xml_writer, element1);
  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

  raptor_free_xml_element(element1);
  element1=NULL;


  /* Boolean Results */
  if(librdf_query_results_is_boolean(results)) {
    result_element=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"boolean", NULL, base_uri);
    if(!result_element)
      goto tidy;

    raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"  ", 2);
    raptor_xml_writer_start_element(xml_writer, result_element);
    if(librdf_query_results_get_boolean(results))
      raptor_xml_writer_raw(xml_writer, RASQAL_XSD_BOOLEAN_TRUE);
    else
      raptor_xml_writer_raw(xml_writer, RASQAL_XSD_BOOLEAN_FALSE);
    raptor_xml_writer_end_element(xml_writer, result_element);
    raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

    goto results3done;
  }


  /* Variable Binding Results */

  /*   <results> */
  results_element=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"results", NULL, base_uri);
  if(!results_element)
    goto tidy;

  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"  ", 2);
  raptor_xml_writer_start_element(xml_writer, results_element);
  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

  /* declare result element for later multiple use */
  result_element=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"result", NULL, base_uri);
  if(!result_element)
    goto tidy;

  while(!librdf_query_results_finished(results)) {
    /*     <result> */
    raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"    ", 4);
    raptor_xml_writer_start_element(xml_writer, result_element);
    raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

    for(i=0; i<librdf_query_results_get_bindings_count(results); i++) {
      const char *name=librdf_query_results_get_binding_name(results, i);
      librdf_node *node=librdf_query_results_get_binding_value(results, i);

      /*       <binding> */
      binding_element=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"binding", NULL, base_uri);
      if(!binding_element)
        goto tidy;

      attrs=(raptor_qname **)raptor_alloc_memory(sizeof(raptor_qname*));
      if(!attrs)
        goto tidy;
#ifdef RAPTOR_V2_AVAILABLE
      attrs[0]=raptor_new_qname_from_namespace_local_name_v2(query->world->raptor_world_ptr, res_ns, (const unsigned char*)"name", (const unsigned char *) name);

#else
      attrs[0]=raptor_new_qname_from_namespace_local_name(res_ns, (const unsigned char*)"name", (const unsigned char *) name);
#endif
      if(!attrs[0]) {
        raptor_free_memory((void*)attrs);
        goto tidy;
      }

      raptor_xml_element_set_attributes(binding_element, attrs, 1);

      raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"      ", 6);
      raptor_xml_writer_start_element(xml_writer, binding_element);

      if(!node) {
        element1=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"unbound", NULL, base_uri);
        if(!element1)
          goto tidy;
        raptor_xml_writer_empty_element(xml_writer, element1);

      } else switch(librdf_node_get_type(node)) {
        case LIBRDF_NODE_TYPE_RESOURCE:
          element1=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"uri", NULL, base_uri);
          if(!element1)
            goto tidy;

          raptor_xml_writer_start_element(xml_writer, element1);
          raptor_xml_writer_cdata(xml_writer, (const unsigned char*)librdf_uri_as_string(librdf_node_get_uri(node)));
          raptor_xml_writer_end_element(xml_writer, element1);
          break;

        case LIBRDF_NODE_TYPE_BLANK:
          element1=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"bnode", NULL, base_uri);
          if(!element1)
            goto tidy;

          raptor_xml_writer_start_element(xml_writer, element1);
          raptor_xml_writer_cdata(xml_writer, (const unsigned char*)librdf_node_get_blank_identifier(node));
          raptor_xml_writer_end_element(xml_writer, element1);
          break;

        case LIBRDF_NODE_TYPE_LITERAL:
          element1=raptor_new_xml_element_from_namespace_local_name(res_ns, (const unsigned char*)"literal", NULL, base_uri);
          if(!element1)
            goto tidy;
          {
	    char *value, *datatype=NULL;
	    char *lang=NULL;
	    librdf_uri *dt;
	    size_t valuelen;

	    value=(char *)librdf_node_get_literal_value_as_counted_string(node, &valuelen);
	    lang=librdf_node_get_literal_value_language(node);
	    dt=librdf_node_get_literal_value_datatype_uri(node);
	    if(dt)
      	      datatype=(char *)librdf_uri_as_string(dt);

            if(lang || dt) {
              attrs=(raptor_qname **)raptor_alloc_memory(sizeof(raptor_qname*));
              if(!attrs)
                goto tidy;

              if(lang)
                attrs[0]=raptor_new_qname(nstack, (const unsigned char*)"xml:lang", (const unsigned char*)lang, (raptor_simple_message_handler)virtuoso_query_simple_error, query);
              else
#ifdef RAPTOR_V2_AVAILABLE
                attrs[0]=raptor_new_qname_from_namespace_local_name_v2(query->world->raptor_world_ptr, res_ns, (const unsigned char*)"datatype", (const unsigned char*)datatype);
#else
                attrs[0]=raptor_new_qname_from_namespace_local_name(res_ns, (const unsigned char*)"datatype", (const unsigned char*)datatype);
#endif
              if(!attrs[0]) {
                raptor_free_memory((void*)attrs);
                goto tidy;
              }

              raptor_xml_element_set_attributes(element1, attrs, 1);
            }

            raptor_xml_writer_start_element(xml_writer, element1);
            raptor_xml_writer_cdata_counted(xml_writer, (const unsigned char*)value, valuelen);
            raptor_xml_writer_end_element(xml_writer, element1);
          }
          break;

        case LIBRDF_NODE_TYPE_UNKNOWN:
        default:
          virtuoso_query_simple_error(query, "Cannot turn literal type %d into XML", librdf_node_get_type(node));
      }

      if(element1) {
        raptor_free_xml_element(element1);
        element1=NULL;
      }

      /*       </binding> */
      raptor_xml_writer_end_element(xml_writer, binding_element);
      raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);
      raptor_free_xml_element(binding_element);
      binding_element=NULL;
      librdf_free_node(node);
    }

    raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"    ", 4);
    raptor_xml_writer_end_element(xml_writer, result_element);
    raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);
    librdf_query_results_next(results);
  }

  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"  ", 2);
  raptor_xml_writer_end_element(xml_writer, results_element);
  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

results3done:
  rc=0;
  raptor_xml_writer_end_element(xml_writer, sparql_element);
  raptor_xml_writer_raw_counted(xml_writer, (const unsigned char*)"\n", 1);

tidy:
  if(element1)
    raptor_free_xml_element(element1);
  if(variable_element)
    raptor_free_xml_element(variable_element);
  if(binding_element)
    raptor_free_xml_element(binding_element);
  if(result_element)
    raptor_free_xml_element(result_element);
  if(results_element)
    raptor_free_xml_element(results_element);
  if(sparql_element)
    raptor_free_xml_element(sparql_element);
  if(res_ns)
    raptor_free_namespace(res_ns);
  if(xml_writer)
    raptor_free_xml_writer(xml_writer);
  if(nstack)
    raptor_free_namespaces(nstack);
  return rc;
}


static void
virtuoso_iostream_write_json_boolean(raptor_iostream* iostr, const char* name, int json_bool)
{
  raptor_iostream_write_byte(iostr, '\"');
  raptor_iostream_write_string(iostr, name);
  raptor_iostream_write_counted_string(iostr, "\" : ",4);

  if(json_bool)
    raptor_iostream_write_counted_string(iostr, "true", 4);
  else
    raptor_iostream_write_counted_string(iostr, "false", 5);

}


static int
virtuoso_query_results_write_json1(raptor_iostream *iostr, librdf_query_results* results, raptor_uri *base_uri)
{
  librdf_query* query=results->query;
  int i;
  int row_comma;
  int column_comma=0;

  if(!librdf_query_results_is_bindings(results) && !librdf_query_results_is_boolean(results)) {
    virtuoso_query_simple_error(query, "Can only write JSON format for variable binding and boolean results");
    return 1;
  }

  raptor_iostream_write_counted_string(iostr, "{\n", 2);
  /* Header */
  raptor_iostream_write_counted_string(iostr, "  \"head\": {\n", 12);

  if(librdf_query_results_is_bindings(results)) {
    raptor_iostream_write_counted_string(iostr, "    \"vars\": [ ", 14);
    for(i=0; 1; i++) {
      const char *name;

      name=librdf_query_results_get_binding_name(results, i);
      if(!name)
        break;

      /*     'x', */
      if(i > 0)
        raptor_iostream_write_counted_string(iostr, ", ", 2);
      raptor_iostream_write_byte(iostr, '\"');
      raptor_iostream_write_string(iostr, name);
      raptor_iostream_write_byte(iostr, '\"');
    }
    raptor_iostream_write_counted_string(iostr, " ]\n", 3);
  }

  /* FIXME - could add link inside 'head': */
  /*   End Header */
  raptor_iostream_write_counted_string(iostr, "  },\n", 5);
  /* Boolean Results */
  if(librdf_query_results_is_boolean(results)) {
    raptor_iostream_write_counted_string(iostr, "  ", 2);
    virtuoso_iostream_write_json_boolean(iostr, "boolean", librdf_query_results_get_boolean(results));
    goto results3done;
  }

  /* Variable Binding Results */
  raptor_iostream_write_counted_string(iostr, "  \"results\": {\n", 15);
#if 1
  raptor_iostream_write_counted_string(iostr, "    \"ordered\" : false,\n", 23);
#else
  /*FIXME*/
  rasqal_iostream_write_json_boolean(iostr, "ordered", (rasqal_query_get_order_condition(query, 0) != NULL));
#endif

#if 1
  raptor_iostream_write_counted_string(iostr, "    \"distinct\" : false,\n", 24);
#else
  /*FIXME*/
  rasqal_iostream_write_json_boolean(iostr, "distinct", rasqal_query_get_distinct(query));
#endif

  raptor_iostream_write_counted_string(iostr, "    \"bindings\" : [\n", 19);
  row_comma=0;
  while(!librdf_query_results_finished(results)) {
    if(row_comma)
      raptor_iostream_write_counted_string(iostr, ",\n", 2);

    /* Result row */
    raptor_iostream_write_counted_string(iostr, "      {\n", 8);
    column_comma=0;
    for(i=0; i<librdf_query_results_get_bindings_count(results); i++) {
      const char *name=librdf_query_results_get_binding_name(results, i);
      librdf_node *node=librdf_query_results_get_binding_value(results, i);

      if(column_comma)
        raptor_iostream_write_counted_string(iostr, ",\n", 2);

      /*       <binding> */
      raptor_iostream_write_counted_string(iostr, "        \"", 9);
      raptor_iostream_write_string(iostr, name);
      raptor_iostream_write_counted_string(iostr, "\" : { ", 6);

      if(!node) {
        raptor_iostream_write_string(iostr, "\"type\": \"unbound\", \"value\": null");
      } else switch(librdf_node_get_type(node)) {
        const unsigned char* str;
        size_t len;
        char *lang;
        librdf_uri *dt;

        case LIBRDF_NODE_TYPE_RESOURCE:
          raptor_iostream_write_string(iostr, "\"type\": \"uri\", \"value\": \"");
          str=(const unsigned char*)librdf_uri_as_counted_string(librdf_node_get_uri(node), &len);
          raptor_iostream_write_string_ntriples(iostr, str, len, '"');
          raptor_iostream_write_byte(iostr, '"');
          break;

        case LIBRDF_NODE_TYPE_BLANK:
          str=(const unsigned char*)librdf_node_get_blank_identifier(node);

          raptor_iostream_write_string(iostr, "\"type\": \"bnode\", \"value\": \"");
          raptor_iostream_write_string_ntriples(iostr, str, strlen((char *)str), '"');
          raptor_iostream_write_byte(iostr, '"');
          break;

        case LIBRDF_NODE_TYPE_LITERAL:
          str=(const unsigned char *)librdf_node_get_literal_value_as_counted_string(node,&len);
          raptor_iostream_write_string(iostr, "\"type\": \"literal\", \"value\": \"");
          raptor_iostream_write_string_ntriples(iostr, str, len, '"');
          raptor_iostream_write_byte(iostr, '"');

          lang=librdf_node_get_literal_value_language(node);
          if(lang) {
            raptor_iostream_write_string(iostr, ",\n      \"xml:lang\" : \"");
            raptor_iostream_write_string(iostr, (const unsigned char*)lang);
            raptor_iostream_write_byte(iostr, '"');
          }

          dt=librdf_node_get_literal_value_datatype_uri(node);
          if(dt) {
            raptor_iostream_write_string(iostr, ",\n      \"datatype\" : \"");
            str=(const unsigned char*)librdf_uri_as_counted_string(dt, &len);
            raptor_iostream_write_string_ntriples(iostr, str, len, '"');
            raptor_iostream_write_byte(iostr, '"');
          }

          break;

        case LIBRDF_NODE_TYPE_UNKNOWN:
        default:
          virtuoso_query_simple_error(query, "Cannot turn literal type %d into XML", librdf_node_get_type(node));
      }

      /* End Binding */
      raptor_iostream_write_counted_string(iostr, " }", 2);
      column_comma=1;
      librdf_free_node(node);

    }

    /* End Result Row */
    raptor_iostream_write_counted_string(iostr, "\n      }", 8);
    row_comma=1;
    librdf_query_results_next(results);
  }

  raptor_iostream_write_counted_string(iostr, "\n    ]\n  }", 10);
results3done:

  /* end sparql */
  raptor_iostream_write_counted_string(iostr, "\n}\n", 3);

  return 0;
}


static librdf_query_results_formatter*
virtuoso_new_results_formatter(librdf_query_results* query_results, const char *name, librdf_uri* uri, const char *mime_type)
{
  rasqal_query_results_format_factory* factory;
  rasqal_query_results_formatter* formatter;
  librdf_query_results_formatter* qrf;
  rasqal_query_results_formatter_func writer_fn=(rasqal_query_results_formatter_func)virtuoso_query_results_write_sparql_xml;
  int is_json=0;


  if(name) {
     if(!strcmp(name, "json")) {
        writer_fn =(rasqal_query_results_formatter_func)virtuoso_query_results_write_json1;
        is_json=1;
     }
  } else if(uri) {
     char *s_uri =(char *)librdf_uri_as_string(uri);
     if(!strcmp(s_uri, "http://www.w3.org/2001/sw/DataAccess/json-sparql/")) {
        writer_fn =(rasqal_query_results_formatter_func)virtuoso_query_results_write_json1;
        is_json=1;
     } else if(!strcmp(s_uri, "http://www.mindswap.org/%7Ekendall/sparql-results-json/")) {
        writer_fn =(rasqal_query_results_formatter_func)virtuoso_query_results_write_json1;
        is_json=1;
     }
  } else if(mime_type)  {
     if(!strcmp(name, "text/json")) {
        writer_fn =(rasqal_query_results_formatter_func)virtuoso_query_results_write_json1;
        is_json=1;
     }
  }

  factory=(rasqal_query_results_format_factory*)RASQAL_CALLOC(query_results_format_factory, 1, sizeof(rasqal_query_results_format_factory));
  if(!factory)
    return NULL;

  factory->name=NULL;
  factory->label=NULL;
  factory->uri_string=NULL;
  factory->writer=writer_fn;
  factory->reader=NULL;
  factory->get_rowsource=NULL;
  factory->mime_type=NULL;

  formatter=(rasqal_query_results_formatter*)RASQAL_CALLOC(rasqal_query_results_formatter, 1, sizeof(rasqal_query_results_formatter));
  if(!formatter) {
    RASQAL_FREE(query_results_format_factory, factory);
    return NULL;
  }

  formatter->factory=factory;
  formatter->mime_type=NULL;

  qrf=(librdf_query_results_formatter*)LIBRDF_MALLOC(query_results_formatter, sizeof(librdf_query_results_formatter));
  if(!qrf) {
    RASQAL_FREE(query_results_format_factory, factory);
    rasqal_free_query_results_formatter(formatter);
    return NULL;
  }

  qrf->query_results=query_results;
  qrf->formatter=formatter;
  return qrf;
}
