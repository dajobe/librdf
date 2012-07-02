/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_virtuoso_internal.h - RDF Storage using Virtuoso interface definition
 *
 * Copyright (C) 2008, Openlink Software http://www.openlinksw.com/
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
 */
#ifndef _RDF_STORAGE_VIRTUOSO_INTERNAL_H
#define _RDF_STORAGE_VIRTUOSO_INTERNAL_H

typedef enum {
  VIRTUOSO_CONNECTION_CLOSED = 0,
  VIRTUOSO_CONNECTION_OPEN = 1,
  VIRTUOSO_CONNECTION_BUSY = 2
} librdf_storage_virtuoso_connection_status;


typedef struct librdf_storage_virtuoso_connection_s  librdf_storage_virtuoso_connection;


struct librdf_storage_virtuoso_connection_s {
   /* A ODBC connection */
   librdf_storage_virtuoso_connection_status status;
   HENV henv;
   HDBC hdbc;
   HSTMT hstmt;
   short numCols;

  librdf_hash *h_lang;
  librdf_hash *h_type;

  void (*v_release_connection)(librdf_storage* storage, librdf_storage_virtuoso_connection *handle);
  librdf_node* (*v_rdf2node)(librdf_storage *storage, librdf_storage_virtuoso_connection *handle, int col, char *data);
  char* (*v_GetDataCHAR)(librdf_world *world, librdf_storage_virtuoso_connection *handle, int col, int *is_null);
  int (*v_GetDataINT)(librdf_world *world, librdf_storage_virtuoso_connection *handle, int col, int *is_null, int *val);
};


typedef int vquery_results_type;

#define VQUERY_RESULTS_UNKNOWN   0
#define VQUERY_RESULTS_BINDINGS  1
#define VQUERY_RESULTS_BOOLEAN   2
#define VQUERY_RESULTS_GRAPH     4
#define VQUERY_RESULTS_SYNTAX    8


typedef struct
{
  librdf_query *query;        /* librdf query object */
  librdf_model *model;
  char *language;            /* rasqal query language name to use */
  unsigned char *query_string;
  librdf_uri *uri;           /* base URI or NULL */

  librdf_storage_virtuoso_connection *vc;
  librdf_storage *storage;
  int failed;
  int eof;
  short numCols;
  short offset;
  int limit;
  vquery_results_type result_type;
  int row_count;

  char **colNames;
  librdf_node **colValues;
} librdf_query_virtuoso_context;


#define LIBRDF_VIRTUOSO_CONTEXT_DSN_SIZE 4096

typedef struct {
  /* Virtuoso connection parameters */
  librdf_storage *storage;
  librdf_node *current;

  librdf_storage_virtuoso_connection **connections;
  int connections_count;

  char *model_name;
  char *user;
  char *password;
  char *dsn;
  char *host;
  char *database;
  char *charset;
  char *conn_str;

  /* if inserts should be optimized by locking and index optimizations */
  int bulk;
  int merge;

  librdf_hash* h_lang;
  librdf_hash* h_type;

  librdf_world *world;

  librdf_storage_virtuoso_connection *transaction_handle;

  /* for output connection DSN from SQLDriverConnect() as called by
   * librdf_storage_virtuoso_get_handle() 
   */
  UCHAR outdsn[LIBRDF_VIRTUOSO_CONTEXT_DSN_SIZE];
} librdf_storage_virtuoso_instance;


typedef struct {
  librdf_storage *storage;
  librdf_statement *current_statement;
  librdf_statement *query_statement;
  librdf_storage_virtuoso_connection *handle;

  librdf_node *query_context;
  librdf_node *current_context;

} librdf_storage_virtuoso_sos_context;


typedef struct {
  librdf_storage *storage;
  librdf_node *current_context;
  librdf_storage_virtuoso_connection *handle;

} librdf_storage_virtuoso_get_contexts_context;



/******************* Virtuoso ODBC Extensions *******************/

/* See
 * http://docs.openlinksw.com/virtuoso/odbcimplementation.html#virtodbcsparql
 */


/*
 *  ODBC extensions for SQLGetDescField
 */
#define SQL_DESC_COL_DV_TYPE		1057L
#define SQL_DESC_COL_DT_DT_TYPE		1058L
#define SQL_DESC_COL_LITERAL_ATTR	1059L
#define SQL_DESC_COL_BOX_FLAGS		1060L
#define SQL_DESC_COL_LITERAL_LANG       1061L
#define SQL_DESC_COL_LITERAL_TYPE       1062L

/*
 *  Virtuoso - ODBC SQL_DESC_COL_DV_TYPE
 */
#define VIRTUOSO_DV_DATE		129
#define VIRTUOSO_DV_DATETIME		211
#define VIRTUOSO_DV_DOUBLE_FLOAT	191
#define VIRTUOSO_DV_IRI_ID		243
#define VIRTUOSO_DV_LONG_INT		189
#define VIRTUOSO_DV_NUMERIC		219
#define VIRTUOSO_DV_RDF			246
#define VIRTUOSO_DV_SINGLE_FLOAT	190
#define VIRTUOSO_DV_STRING		182
#define VIRTUOSO_DV_TIME		210
#define VIRTUOSO_DV_TIMESTAMP		128
#define VIRTUOSO_DV_TIMESTAMP_OBJ	208


/*
 *  Virtuoso - ODBC SQL_DESC_COL_DT_DT_TYPE
 */
#define VIRTUOSO_DT_TYPE_DATETIME	1
#define VIRTUOSO_DT_TYPE_DATE		2
#define VIRTUOSO_DT_TYPE_TIME		3


/*
 *  Virtuoso - ODBC SQL_DESC_COL_BOX_FLAGS
 */
#define VIRTUOSO_BF_IRI			0x1
#define VIRTUOSO_BF_UTF8                0x2
#define VIRTUOSO_BF_DEFAULT_ENC         0x4

#endif
