/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_virtuoso.c - RDF Storage using Virtuoso interface definition
 *
 * Copyright (C) 2008, Openlink Software http://www.openlinksw.com/
 *
 * Based in part on rdf_storage_mysql
 *
 * Copyright (C) 2000-2010, David Beckett http://www.dajobe.org/
 * Copyright (C) 2000-2005, University of Bristol, UK http://www.bristol.ac.uk/
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
#include <config-win.h>
#include <winsock.h>
#include <assert.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <sys/types.h>

#include <redland.h>
#include <rdf_types.h>

#if defined(__APPLE__)
/* Ignore /usr/include/sql.h deprecated warnings on OSX 10.8 */
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/* Virtuoso specific */
#include <sql.h>
#include <sqlext.h>

/*#define VIRTUOSO_STORAGE_DEBUG 1*/

#include <rdf_storage_virtuoso_internal.h>


static int librdf_storage_virtuoso_init(librdf_storage* storage, const char *name, librdf_hash* options);

static void librdf_storage_virtuoso_terminate(librdf_storage* storage);
static int librdf_storage_virtuoso_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_virtuoso_close(librdf_storage* storage);
static int librdf_storage_virtuoso_sync(librdf_storage* storage);
static int librdf_storage_virtuoso_size(librdf_storage* storage);
static int librdf_storage_virtuoso_add_statement(librdf_storage* storage,librdf_statement* statement);
static int librdf_storage_virtuoso_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_virtuoso_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_virtuoso_contains_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_virtuoso_context_contains_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_virtuoso_start_bulk(librdf_storage* storage);
static int librdf_storage_virtuoso_stop_bulk(librdf_storage* storage);

static librdf_stream* librdf_storage_virtuoso_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_virtuoso_find_statements(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_virtuoso_find_statements_with_options(librdf_storage* storage, librdf_statement* statement, librdf_node* context_node, librdf_hash* options);

/* context functions */
static int librdf_storage_virtuoso_context_add_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_virtuoso_context_add_statements(librdf_storage* storage, librdf_node* context_node, librdf_stream* statement_stream);
static int librdf_storage_virtuoso_context_remove_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_virtuoso_context_remove_statements(librdf_storage* storage, librdf_node* context_node);
static librdf_stream* librdf_storage_virtuoso_context_serialise(librdf_storage* storage, librdf_node* context_node);
static librdf_stream* librdf_storage_virtuoso_find_statements_in_context(librdf_storage* storage, librdf_statement* statement, librdf_node* context_node);
static librdf_iterator* librdf_storage_virtuoso_get_contexts(librdf_storage* storage);

static void librdf_storage_virtuoso_register_factory(librdf_storage_factory *factory);

static librdf_node* librdf_storage_virtuoso_get_feature(librdf_storage* storage, librdf_uri* feature);

static int librdf_storage_virtuoso_transaction_start(librdf_storage* storage);

static int librdf_storage_virtuoso_transaction_start_with_handle(librdf_storage* storage, void* handle);
static int librdf_storage_virtuoso_transaction_commit(librdf_storage* storage);

static int librdf_storage_virtuoso_transaction_rollback(librdf_storage* storage);

static void* librdf_storage_virtuoso_transaction_get_handle(librdf_storage* storage);

static int rdf_virtuoso_ODBC_Errors(const char *where, librdf_world *world, librdf_storage_virtuoso_connection *handle);
static int librdf_storage_virtuoso_context_add_statement_helper(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static void librdf_storage_virtuoso_release_handle(librdf_storage* storage, librdf_storage_virtuoso_connection *handle);

#ifdef MODULAR_LIBRDF
void librdf_storage_module_register_factory(librdf_world *world);
#endif


/*
 * Report any pending Virtuoso SQL errors to redland error logging mechanism
 */
static int
rdf_virtuoso_ODBC_Errors(const char *where, librdf_world *world,
                         librdf_storage_virtuoso_connection* handle)
{
  SQLCHAR buf[512];
  SQLCHAR sqlstate[15];

  while(SQLError(handle->henv, handle->hdbc, handle->hstmt, sqlstate, NULL,
                 buf, sizeof(buf), NULL) == SQL_SUCCESS) {
#ifdef VIRTUOSO_STORAGE_DEBUG
    fprintf(stderr, "%s ||%s, SQLSTATE=%s\n", where, buf, sqlstate);
#endif
    librdf_log(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Virtuoso %s failed [%s] %s", where, sqlstate, buf);
  }

  while(SQLError(handle->henv, handle->hdbc, SQL_NULL_HSTMT, sqlstate, NULL,
                 buf, sizeof(buf), NULL) == SQL_SUCCESS) {
#ifdef VIRTUOSO_STORAGE_DEBUG
    fprintf(stderr, "%s ||%s, SQLSTATE=%s\n", where, buf, sqlstate);
#endif
    librdf_log(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Virtuoso %s failed [%s] %s", where, sqlstate, buf);
  }

  while(SQLError(handle->henv, SQL_NULL_HDBC, SQL_NULL_HSTMT, sqlstate, NULL,
                 buf, sizeof(buf), NULL) == SQL_SUCCESS) {
#ifdef VIRTUOSO_STORAGE_DEBUG
    fprintf(stderr, "%s ||%s, SQLSTATE=%s\n", where, buf, sqlstate);
#endif
    librdf_log(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Virtuoso %s failed [%s] %s", where, sqlstate, buf);
  }

  return -1;
}


/*
 * vGetDataCHAR:
 * @world: redland world
 * @handle: virtoso storage connection handle
 * @col: column number
 * @is_null: pointer to NULL flag to set
 *
 * INTERNAL - Get the string value of the given column in current result row
 *
 * Return value: string value or NULL on failure or SQL NULL
 * value. SQL NULLness is distinguished from error by the *is_null
 * being non-0.
 */
static char*
vGetDataCHAR(librdf_world *world, librdf_storage_virtuoso_connection *handle,
             int col, int *is_null)
{
  int rc;
  SQLLEN len;
  SQLCHAR buf[255];

  *is_null = 0;

  rc = SQLGetData(handle->hstmt, LIBRDF_GOOD_CAST(SQLUSMALLINT, col),
                  SQL_C_CHAR, buf, 0, &len);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLGetData()", world, handle);
    return NULL;
  }

  if(len == SQL_NULL_DATA) {
    *is_null = 1;
    return NULL;
  } else {
    char *pLongData = NULL;
    SQLLEN bufsize = len + 4;

    pLongData = LIBRDF_MALLOC(char*, LIBRDF_GOOD_CAST(size_t, bufsize));
    if(pLongData == NULL) {
      librdf_log(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL, 
                 "Not enough memory to allocate resultset element");
      return NULL;
    }

    if(len == 0) {
      pLongData[0]='\0';
    } else {
      rc = SQLGetData(handle->hstmt, col, SQL_C_CHAR, pLongData, bufsize, &len);
      if(!SQL_SUCCEEDED(rc)) {
        LIBRDF_FREE(char*, pLongData);
        rdf_virtuoso_ODBC_Errors("SQLGetData()", world, handle);
        return NULL;
      }
    }
    return pLongData;
  }
}


/*
 * vGetDataINT:
 * @world: redland world
 * @handle: virtoso storage connection handle
 * @col: column number
 * @is_null: pointer to NULL flag to set
 * @val: pointer to store integer value
 *
 * INTERNAL - Get the integer value of the given column in current result row
 *
 * Return value: 0 on success or < 0 on failure.  SQL NULLness is
 * distinguished from error by the *is_null being non-0.
 */
static int
vGetDataINT(librdf_world *world, librdf_storage_virtuoso_connection *handle,
            int col, int *is_null, int *val)
{
  int rc;
  SQLLEN len;

  *is_null = 0;

  rc = SQLGetData(handle->hstmt, col, SQL_C_LONG, val, 0, &len);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLGetData()", world, handle);
    return -1;
  }

  if(len == SQL_NULL_DATA) {
    *is_null = 1;
    return 0;
  }

  return 0;
}


/*
 * rdf_lang2string:
 * @world: redland world
 * @handle: virtoso storage connection handle
 * @key: language ID key
 *
 * INTERNAL - turn an rdf_language ID into a language string
 *
 * Return value: language string or NULL on failure.
 */
static char*
rdf_lang2string(librdf_world *world,
                librdf_storage_virtuoso_connection *handle, short key)
{
  char *val = NULL;
  char query[]="select RL_ID from DB.DBA.RDF_LANGUAGE where RL_TWOBYTE=?";
  int rc;
  HSTMT hstmt;
  SQLLEN m_ind = 0;

  librdf_hash_datum hd_key, hd_value;
  librdf_hash_datum* old_value;

  hd_key.data = &key;
  hd_key.size = sizeof(short);

  old_value = librdf_hash_get_one(handle->h_lang, &hd_key);
  if(old_value)
    val = (char *)old_value->data;

  if(val)
    return val;

  hstmt = handle->hstmt;

  rc = SQLAllocHandle(SQL_HANDLE_STMT, handle->hdbc, &handle->hstmt);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLAllocHandle(hstmt)", world, handle);
    handle->hstmt = hstmt;
    return NULL;
  }

  rc = SQLBindParameter(handle->hstmt, 1, SQL_PARAM_INPUT, SQL_C_SSHORT,
                        SQL_SMALLINT, 0, 0, &key, 0, &m_ind);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLBindParameter()", world, handle);
    goto end;
  }

  rc = SQLExecDirect(handle->hstmt,(UCHAR *) query, SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", world, handle);
    goto end;
  }

  rc = SQLFetch(handle->hstmt);
  if(SQL_SUCCEEDED(rc)) {
    int is_null;
    val = vGetDataCHAR(world, handle, 1, &is_null);
    if(!val || is_null)
	goto end;
    hd_value.data = val;
    hd_value.size = strlen(val);
    librdf_hash_put(handle->h_lang, &hd_key, &hd_value);
  }

end:
  SQLCloseCursor(handle->hstmt);
  SQLFreeHandle(SQL_HANDLE_STMT, handle->hstmt);
  handle->hstmt = hstmt;

  return val;
}


/*
 * rdf_type2string:
 * @world: redland world
 * @handle: virtoso storage connection handle
 * @key: type ID key
 *
 * INTERNAL - turn an rdf_type ID into a URI string
 *
 * Return value: type URI string or NULL on failure.
 */
static char*
rdf_type2string(librdf_world *world,
                librdf_storage_virtuoso_connection *handle, short key)
{
  char *val = NULL;
  char query[] = "select RDT_QNAME from DB.DBA.RDF_DATATYPE where RDT_TWOBYTE=?";
  int rc;
  HSTMT hstmt;
  SQLLEN m_ind = 0;

  librdf_hash_datum hd_key, hd_value;
  librdf_hash_datum* old_value;

  hd_key.data = &key;
  hd_key.size = sizeof(short);

  old_value = librdf_hash_get_one(handle->h_type,&hd_key);
  if(old_value)
    val = (char*)old_value->data;

  if(val)
    return val;

  hstmt = handle->hstmt;

  rc = SQLAllocHandle(SQL_HANDLE_STMT, handle->hdbc, &handle->hstmt);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLAllocHandle(hstmt)", world, handle);
    handle->hstmt = hstmt;
    return NULL;
  }

  rc = SQLBindParameter(handle->hstmt, 1, SQL_PARAM_INPUT, SQL_C_SSHORT,
                        SQL_SMALLINT, 0, 0, &key, 0, &m_ind);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLBindParameter()", world, handle);
    goto end;
  }

  rc = SQLExecDirect(handle->hstmt,(UCHAR *) query, SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", world, handle);
    goto end;
  }

  rc = SQLFetch(handle->hstmt);
  if(SQL_SUCCEEDED(rc)) {
    int is_null;
    val = vGetDataCHAR(world, handle, 1, &is_null);
    if(!val || is_null)
	goto end;
    hd_value.data = val;
    hd_value.size = strlen(val);
    librdf_hash_put(handle->h_type, &hd_key, &hd_value);
  }

end:
  SQLCloseCursor(handle->hstmt);
  SQLFreeHandle(SQL_HANDLE_STMT, handle->hstmt);
  handle->hstmt = hstmt;

  return val;
}


/*
 * rdf2node:
 * @storage: storage object
 * @handle: virtoso storage connection handle
 * @col: column
 * @data: data at column
 *
 * INTERNAL - turn result data from a column cell into an #librdf_node
 *
 * Return value: node or NULL on failure.
 */
static librdf_node*
rdf2node(librdf_storage *storage, librdf_storage_virtuoso_connection *handle,
         int col, char *data)
{
  librdf_node *node = NULL;
#if 0
#ifdef SQL_DESC_COL_LITERAL_LANG
  SQLCHAR langbuf[100];
  SQLINTEGER lang_len = 0;
#endif
#ifdef SQL_DESC_COL_LITERAL_TYPE
  SQLCHAR typebuff[100];
  SQLINTEGER type_len = 0;
#endif
#else
  short l_lang;
  short l_type;
#endif
  SQLHDESC hdesc = NULL;
  librdf_uri *u_type = NULL;
  int dvtype = 0;
  int dv_dt_type = 0;
  int flag = 0;
  int rc;

  /* Get row descriptor for this statement */
  rc = SQLGetStmtAttr(handle->hstmt, SQL_ATTR_IMP_ROW_DESC, &hdesc,
                      SQL_IS_POINTER, NULL);
  if(!SQL_SUCCEEDED(rc))
      return NULL;

  /* Get datatype of column 'col' in row */
  rc = SQLGetDescField(hdesc, col, SQL_DESC_COL_DV_TYPE, &dvtype, 
                       SQL_IS_INTEGER, NULL);
  if(!SQL_SUCCEEDED(rc))
     return NULL;

  /* Try to get timestamp / date / time / datetime value for column 'col' */
  rc = SQLGetDescField(hdesc, col, SQL_DESC_COL_DT_DT_TYPE, &dv_dt_type,
                       SQL_IS_INTEGER, NULL);
  if(!SQL_SUCCEEDED(rc))
     return NULL;

  /* FIXME Should be converted to these:
   *   SQL_DESC_COL_LITERAL_LANG and SQL_DESC_COL_LITERAL_TYPE
   *
   * The docs says about SQL_DESC_COL_LITERAL_ATTR:
   *   "This call is deprecated in favor of using the
   *    SQL_DESC_COL_LITERAL_LANG and SQL_DESC_LITERAL_TYPE options
   *    of SQLGetDescField which caches these lookups to speed up
   *    describe operations."
   * -- http://docs.openlinksw.com/virtuoso/odbcimplementation.html
   *
   * instead of doing the lookup here then rdf_type2string() and
   * rdf_lang2string() on the decoded flags.
   */
#if 0
#ifdef SQL_DESC_COL_LITERAL_LANG
    rc = SQLGetDescField(hdesc, colNum, SQL_DESC_COL_LITERAL_LANG, langbuf,
                         sizeof(langbuf), &lang_len);
  if(!SQL_SUCCEEDED(rc))
     return NULL;
#endif
#ifdef SQL_DESC_COL_LITERAL_TYPE
    rc = SQLGetDescField(hdesc, colNum, SQL_DESC_COL_LITERAL_TYPE, typebuf,
                         sizeof(typebuff), &type_len);
  if(!SQL_SUCCEEDED(rc))
     return NULL;
#endif
#else
  /* Get RDF literal attributes flags */
  rc = SQLGetDescField(hdesc, col, SQL_DESC_COL_LITERAL_ATTR, &flag,
                       SQL_IS_INTEGER, NULL);
  if(!SQL_SUCCEEDED(rc))
     return NULL;

  /* literal language ID (16 bits) */
  l_lang = (short)((flag >> 16) & 0xFFFF);
  /* literal datatype ID (16 bits) */
  l_type = (short)(flag & 0xFFFF);
#endif

  rc = SQLGetDescField(hdesc, col, SQL_DESC_COL_BOX_FLAGS, &flag,
                       SQL_IS_INTEGER, NULL);
  if(!SQL_SUCCEEDED(rc))
     return NULL;

  switch(dvtype) {
    case VIRTUOSO_DV_STRING:
      if(flag) {
        if(!strncmp((char*)data, "_:", 2)) {
          node = librdf_new_node_from_blank_identifier(storage->world,
                                                       (const unsigned char*)data + 2);
        } else {
          node = librdf_new_node_from_uri_string(storage->world,
                                                 (const unsigned char*)data);
        }
      } else {
        if(!strncmp((char*)data, "nodeID://", 9)) {
          node = librdf_new_node_from_blank_identifier(storage->world,
                                                       (const unsigned char*)data + 9);
        } else {
          node = librdf_new_node_from_literal(storage->world,
                                              (const unsigned char *)data, NULL, 0);
        }
      }
      break;

    case VIRTUOSO_DV_RDF:
      if(1) {
	char *s_type = rdf_type2string(storage->world, handle, l_type);
	char *s_lang = rdf_lang2string(storage->world, handle, l_lang);

	if(s_type)
	  u_type = librdf_new_uri(storage->world,(unsigned char *)s_type);

	node = librdf_new_node_from_typed_literal(storage->world,
                                                  (const unsigned char *)data,
                                                  s_lang, 
                                                  u_type);
	break;
      }

    case VIRTUOSO_DV_LONG_INT: /* integer */
      u_type = librdf_new_uri(storage->world,
                              (unsigned char *)"http://www.w3.org/2001/XMLSchema#integer");
      
      node = librdf_new_node_from_typed_literal(storage->world,
                                                (const unsigned char *)data,
                                                NULL, 
                                                u_type);
      break;

    case VIRTUOSO_DV_SINGLE_FLOAT: /* float */
      u_type = librdf_new_uri(storage->world,
                              (unsigned char *)"http://www.w3.org/2001/XMLSchema#float");
      
      node = librdf_new_node_from_typed_literal(storage->world,
                                                (const unsigned char *)data, NULL,
                                                u_type);
	break;

    case VIRTUOSO_DV_DOUBLE_FLOAT: /* double */
      u_type = librdf_new_uri(storage->world, 
                              (unsigned char *)"http://www.w3.org/2001/XMLSchema#double");
      
      node = librdf_new_node_from_typed_literal(storage->world,
                                                (const unsigned char *)data, 
                                                NULL,
                                                u_type);
      break;

    case VIRTUOSO_DV_NUMERIC: /* decimal */
      u_type = librdf_new_uri(storage->world,
                              (unsigned char *)"http://www.w3.org/2001/XMLSchema#decimal");
      
      node = librdf_new_node_from_typed_literal(storage->world,
                                                (const unsigned char *)data,
                                                NULL,
                                                u_type);
      break;

    case VIRTUOSO_DV_TIMESTAMP: /* datetime */
    case VIRTUOSO_DV_DATE:
    case VIRTUOSO_DV_TIME:
    case VIRTUOSO_DV_DATETIME:
      switch(dv_dt_type) {
        case VIRTUOSO_DT_TYPE_DATE:
          u_type = librdf_new_uri(storage->world,
                                  (unsigned char *)"http://www.w3.org/2001/XMLSchema#date");
          break;
        case VIRTUOSO_DT_TYPE_TIME:
          u_type = librdf_new_uri(storage->world,
                                  (unsigned char *)"http://www.w3.org/2001/XMLSchema#time");
          break;
        default:
          u_type = librdf_new_uri(storage->world,
                                  (unsigned char *)"http://www.w3.org/2001/XMLSchema#dateTime");
          break;
      }
      node = librdf_new_node_from_typed_literal(storage->world,
                                                (const unsigned char *)data,
                                                NULL,
                                                u_type);
      break;

    case VIRTUOSO_DV_TIMESTAMP_OBJ: /* ? what is this */
      return NULL;
      break;

    case VIRTUOSO_DV_IRI_ID:
      node = librdf_new_node_from_literal(storage->world,
                                          (const unsigned char *)data,
                                          NULL,
                                          0);
      break;

    default:
      return NULL; /* printf("*unexpected result type %d*", dtp); */
  }

  return node;
}


static char*
librdf_storage_virtuoso_node2string(librdf_storage *storage, librdf_node *node)
{
  librdf_node_type type = librdf_node_get_type(node);
  size_t nodelen;
  char *ret = NULL;

  if(type == LIBRDF_NODE_TYPE_RESOURCE) {
    /* Get hash */
    char *uri = (char*)librdf_uri_as_counted_string(librdf_node_get_uri(node),
                                                    &nodelen);

    ret = LIBRDF_MALLOC(char*, nodelen + 3);
    if(!ret)
      goto end;

    strcpy(ret, "<");
    strcat(ret, uri);
    strcat(ret, ">");

  } else if(type == LIBRDF_NODE_TYPE_LITERAL) {
    /* Get hash */
    char *value, *datatype = 0;
    char *lang;
    librdf_uri *dt;
    size_t valuelen, langlen = 0, datatypelen = 0;

    value = (char *)librdf_node_get_literal_value_as_counted_string(node,
                                                                    &valuelen);
    lang = librdf_node_get_literal_value_language(node);
    if(lang)
      langlen = strlen(lang);
    dt = librdf_node_get_literal_value_datatype_uri(node);
    if(dt)
      datatype = (char *)librdf_uri_as_counted_string(dt, &datatypelen);
    if(datatype)
      datatypelen = strlen((const char*)datatype);

    /* Create composite node string for hash generation */
    ret = LIBRDF_MALLOC(char*, valuelen + langlen + datatypelen + 8);
    if(!ret)
      goto end;

    strcpy(ret, "\"");
    strcat(ret,(const char*)value);
    strcat(ret, "\"");
    if(lang && strlen(lang)) {
      strcat(ret, "@");
      strcat(ret, lang);
    }
    if(datatype) {
      strcat(ret, "^^<");
      strcat(ret,(const char*)datatype);
      strcat(ret, ">");
    }
  } else if(type == LIBRDF_NODE_TYPE_BLANK) {
    char *value = (char *)librdf_node_get_blank_identifier(node);

    ret = LIBRDF_MALLOC(char*, strlen(value) + 5);
    if(!ret)
      goto end;

#if 1
    strcpy(ret, "<_:");
    strcat(ret, value);
    strcat(ret, ">");
#else
    strcpy(ret, "_:");
    strcat(ret, value);
#endif
  }

end:
  return ret;
}


static char *
librdf_storage_virtuoso_icontext2string(librdf_storage *storage, 
                                        librdf_node *node)
{
  librdf_storage_virtuoso_instance* context;
  context = (librdf_storage_virtuoso_instance*)storage->instance;

  if(node)
    return(char *)librdf_uri_as_string(librdf_node_get_uri(node));
  else
    return context->model_name;
}


static char *
librdf_storage_virtuoso_context2string(librdf_storage *storage,
                                       librdf_node *node)
{
  librdf_storage_virtuoso_instance* context;
  char *ctxt_node = NULL;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

  if(node) {
    ctxt_node = librdf_storage_virtuoso_node2string(storage, node);
  } else {
    ctxt_node = LIBRDF_MALLOC(char*, strlen(context->model_name) + 4);
    
    if(!ctxt_node)
      return NULL;

    sprintf(ctxt_node, "<%s>", context->model_name);
  }
  return ctxt_node;
}


static char *
librdf_storage_virtuoso_fcontext2string(librdf_storage *storage,
                                        librdf_node *node)
{
  char *ctxt_node = NULL;

  if(node) {
    ctxt_node = librdf_storage_virtuoso_node2string(storage, node);
  } else {
    ctxt_node = LIBRDF_MALLOC(char*, 5);
    if(!ctxt_node)
      return NULL;

    strcpy(ctxt_node, "<?g>");
  }
  return ctxt_node;
}


/*
 * librdf_storage_virtuoso_init_connections:
 * @storage: the storage
 *
 * INTERNAL - Initialize Virtuoso connection pool.
 *
 * Return value: Non-zero on success.
 **/
static int
librdf_storage_virtuoso_init_connections(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance* context;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_init_connections \n");
#endif
  /* Reset connection pool */
  context->connections = NULL;
  context->connections_count = 0;

  return 1;
}


/*
 * librdf_storage_virtuoso_finish_connections:
 * @storage: the storage
 *
 * INTERNAL - Finish all connections in Virtuoso connection pool and free structures.
 *
 * Return value: None.
 **/
static void
librdf_storage_virtuoso_finish_connections(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance* context;
  librdf_storage_virtuoso_connection *handle;
  int i;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_finish_connections \n");
#endif
  /* Loop through connections and close */
  for(i = 0; i < context->connections_count; i++) {
    if(VIRTUOSO_CONNECTION_CLOSED != context->connections[i]->status) {
#ifdef LIBRDF_DEBUG_SQL
      LIBRDF_DEBUG2("virtuoso_close connection handle %p\n",
                    context->connections[i]->handle);
#endif
      handle=context->connections[i];
      if(handle->hstmt) {
	SQLCloseCursor(handle->hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, handle->hstmt);
      }

      if(handle->hdbc) {
	SQLDisconnect(handle->hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, handle->hdbc);
      }

      if(handle->henv) {
	SQLFreeHandle(SQL_HANDLE_ENV, handle->henv);
      }
    }
    LIBRDF_FREE(librdf_storage_virtuoso_connection*, context->connections[i]);
  }
  /* Free structure and reset */
  if(context->connections_count) {
    LIBRDF_FREE(librdf_storage_virtuoso_connection**, context->connections);
    context->connections = NULL;
    context->connections_count = 0;
  }
}


/*
 * librdf_storage_virtuoso_get_handle:
 * @storage: the storage
 *
 * INTERNAL - get a connection handle to the Virtuoso server
 *
 * This attempts to reuses any existing available pooled connection
 * otherwise creates a new connection to the server.
 *
 * Return value: Non-zero on succes.
 **/
static librdf_storage_virtuoso_connection *
librdf_storage_virtuoso_get_handle(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance* context;
  librdf_storage_virtuoso_connection* connection= NULL;
  int i;
  int rc;
  short buflen;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_connection \n");
#endif
  if(context->transaction_handle)
    return context->transaction_handle;

  /* Look for an open connection handle to return */
  for(i = 0; i < context->connections_count; i++) {
    if(VIRTUOSO_CONNECTION_OPEN == context->connections[i]->status) {
      context->connections[i]->status = VIRTUOSO_CONNECTION_BUSY;
      return context->connections[i];
    }
  }

  /* Look for a closed connection */
  for(i = 0; i < context->connections_count && !connection; i++) {
    if(VIRTUOSO_CONNECTION_CLOSED == context->connections[i]->status) {
      connection=context->connections[i];
      break;
    }
  }
  /* Expand connection pool if no closed connection was found */
  if(!connection) {
    /* Allocate new buffer with two extra slots */
    librdf_storage_virtuoso_connection** connections;
    connections = LIBRDF_CALLOC(librdf_storage_virtuoso_connection**,
                                LIBRDF_GOOD_CAST(size_t, context->connections_count + 2),
                                sizeof(librdf_storage_virtuoso_connection*));
    if(!connections)
      return NULL;

    connections[context->connections_count] =  LIBRDF_CALLOC(librdf_storage_virtuoso_connection*, 1, sizeof(librdf_storage_virtuoso_connection));
    if(!connections[context->connections_count]) {
      LIBRDF_FREE(librdf_storage_virtuoso_connection**, connections);
      return NULL;
    }

    connections[context->connections_count + 1] =  LIBRDF_CALLOC(librdf_storage_virtuoso_connection*, 1, sizeof(librdf_storage_virtuoso_connection));
    if(!connections[context->connections_count + 1]) {
      LIBRDF_FREE(librdf_storage_virtuoso_connection*, connections[context->connections_count]);
      LIBRDF_FREE(librdf_storage_virtuoso_connection**, connections);
      return NULL;
    }

    if(context->connections_count) {
      /* Copy old buffer to new */
      memcpy(connections, context->connections,
             sizeof(librdf_storage_virtuoso_connection*) * 
             LIBRDF_GOOD_CAST(size_t, context->connections_count));
      /* Free old buffer */
      LIBRDF_FREE(librdf_storage_virtuoso_connection**, context->connections);
    }

    /* Update buffer size and reset new connections */
    context->connections_count += 2;
    connection = connections[context->connections_count - 2];
    if(!connection) {
      LIBRDF_FREE(librdf_storage_virtuoso_connection**, connections);
      return NULL;
    }
    
    connection->status = VIRTUOSO_CONNECTION_CLOSED;
    connection->henv = NULL;
    connection->hdbc = NULL;
    connection->hstmt = NULL;

    connections[context->connections_count - 1]->status = VIRTUOSO_CONNECTION_CLOSED;
    connections[context->connections_count - 1]->henv = NULL;
    connections[context->connections_count - 1]->hdbc = NULL;
    connections[context->connections_count - 1]->hstmt = NULL;
    context->connections = connections;
  }

  /* Initialize closed Virtuoso connection handle */
  rc = SQLAllocHandle(SQL_HANDLE_ENV, NULL, &connection->henv);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLAllocHandle(henv)", storage->world,
                             connection);
    goto end;
  }

  SQLSetEnvAttr(connection->henv,
                SQL_ATTR_ODBC_VERSION,(SQLPOINTER) SQL_OV_ODBC3,
                SQL_IS_UINTEGER);

  rc = SQLAllocHandle(SQL_HANDLE_DBC, connection->henv, &connection->hdbc);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLAllocHandle(hdbc)", storage->world,
                             connection);
    goto end;
  }

  rc = SQLDriverConnect(connection->hdbc, 0,(UCHAR *) context->conn_str,
                        SQL_NTS, context->outdsn,
                        LIBRDF_VIRTUOSO_CONTEXT_DSN_SIZE,
                        &buflen, SQL_DRIVER_COMPLETE);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLConnect()", storage->world, connection);
    goto end;
  }

  rc = SQLAllocHandle(SQL_HANDLE_STMT, connection->hdbc, &connection->hstmt);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLAllocHandle(hstmt)", storage->world,
                             connection);
    goto end;
  }

  /* Update status and return */
  connection->h_lang = context->h_lang;
  connection->h_type = context->h_type;
  connection->v_release_connection = librdf_storage_virtuoso_release_handle;
  connection->v_rdf2node = rdf2node;
  connection->v_GetDataCHAR = vGetDataCHAR;
  connection->v_GetDataINT = vGetDataINT;
  connection->status = VIRTUOSO_CONNECTION_BUSY;
  return connection;

end:
  if(connection) {
    if(connection->hstmt) {
      SQLFreeHandle(SQL_HANDLE_STMT, connection->hstmt);
      connection->hstmt = NULL;
    }

    if(connection->hdbc) {
      SQLDisconnect(connection->hdbc);
      SQLFreeHandle(SQL_HANDLE_DBC, connection->hdbc);
      connection->hdbc = NULL;
    }

    if(connection->henv) {
      SQLFreeHandle(SQL_HANDLE_ENV, connection->henv);
      connection->henv = NULL;
    }
  }

  return NULL;
}


/*
 * librdf_storage_virtuoso_release_handle;
 * @storage: the storage
 * @handle: the Viruoso handle to release
 *
 * INTERNAL - Release a connection handle to Virtuoso server back to the pool
 *
 * Return value: None.
 **/
static void
librdf_storage_virtuoso_release_handle(librdf_storage* storage, 
                                       librdf_storage_virtuoso_connection *handle)
{
  librdf_storage_virtuoso_instance* context;
  int i;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

  if(handle == context->transaction_handle)
    return;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_release_handle %p \n", handle);
#endif
  /* Look for busy connection handle to drop */
  for(i = 0; i < context->connections_count; i++) {
    if(VIRTUOSO_CONNECTION_BUSY == context->connections[i]->status &&
       context->connections[i] == handle) {
      context->connections[i]->status = VIRTUOSO_CONNECTION_OPEN;
      return;
    }
  }

  librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL, 
             "Unable to find busy connection(in pool of %i connections)",
             context->connections_count);
}


/*
 * librdf_storage_virtuoso_init:
 * @storage: the storage
 * @name: model name
 * @options:  dsn, user, password, host, database, [bulk].
 *
 * INTERNAL - Create connection to database.
 *
 * The boolean bulk option can be set to true if optimized inserts(table
 * locks and temporary key disabling) is wanted. Note that this will block
 * all other access, and requires table locking and alter table privileges.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_virtuoso_init(librdf_storage* storage, const char *name,
                             librdf_hash* options)
{
  librdf_storage_virtuoso_instance* context;
  size_t len = 0;
  
  context = LIBRDF_CALLOC(librdf_storage_virtuoso_instance*, 
                          1, sizeof(*context));
  storage->instance = context;

  /* Must have connection parameters passed as options */
  if(!options)
    return 1;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_init \n");
#endif

  context->connections = NULL;
  context->connections_count = 0;
  context->storage = storage;
  context->password = librdf_hash_get_del(options, "password");
  context->user = librdf_hash_get_del(options, "user");
  context->dsn = librdf_hash_get_del(options, "dsn");
  context->host = librdf_hash_get_del(options, "host");
  context->database = librdf_hash_get_del(options, "database");
  context->charset = librdf_hash_get_del(options, "charset");

  context->h_lang = librdf_new_hash(storage->world, NULL);
  if(!context->h_lang)
    LIBRDF_FATAL1(storage->world, LIBRDF_FROM_STORAGE,
                  "Failed to create Virtuoso language hash from factory");

  if(librdf_hash_open(context->h_lang, NULL, 0, 1, 1, NULL))
    LIBRDF_FATAL1(storage->world, LIBRDF_FROM_STORAGE, 
                  "Failed to open Virtuoso language hash");

  context->h_type = librdf_new_hash(storage->world, NULL);
  if(!context->h_type)
    LIBRDF_FATAL1(storage->world, LIBRDF_FROM_STORAGE,
                  "Failed to create Virtuoso type hash from factory");

  if(librdf_hash_open(context->h_type, NULL, 0, 1, 1, NULL))
    LIBRDF_FATAL1(storage->world, LIBRDF_FROM_STORAGE,
                  "Failed to open Virtuoso type hash");

  if(!name)
    name="virt:DEFAULT";

  if(context->password)
    len += (strlen(context->password) + strlen("PWD=;"));
  if(context->user)
    len += (strlen(context->user) + strlen("UID=;"));
  if(context->dsn)
    len += (strlen(context->dsn) + strlen("DSN=;"));
  if(context->host)
    len += (strlen(context->host) + strlen("HOST=;"));
  if(context->database)
    len += (strlen(context->database) + strlen("DATABASE=;"));
  if(context->charset)
    len += (strlen(context->charset) + strlen("CHARSET=;"));

  context->conn_str = LIBRDF_MALLOC(char*, len + 16);
  if(!context->conn_str)
    return 1;

  context->model_name = LIBRDF_MALLOC(char*, strlen(name) + 1);
  if(!context->model_name)
    return 1;

  strcpy(context->model_name, name);

  /* Optimize loads? */
  context->bulk = (librdf_hash_get_as_boolean(options, "bulk") > 0);

  /* Truncate model? */
#if 0
/* ?? FIXME */
  if(!status && (librdf_hash_get_as_boolean(options, "new") > 0))
    status = librdf_storage_virtuoso_context_remove_statements(storage, NULL);
#endif

  if(!context->model_name || !context->dsn || !context->user ||
     !context->password)
    return 1;

  strcpy(context->conn_str, "");
  if(context->dsn) {
    strcat(context->conn_str, "DSN=");
    strcat(context->conn_str, context->dsn);
    strcat(context->conn_str, ";");
  }
  if(context->host) {
    strcat(context->conn_str, "HOST=");
    strcat(context->conn_str, context->host);
    strcat(context->conn_str, ";");
  }
  if(context->database) {
    strcat(context->conn_str, "DATABASE=");
    strcat(context->conn_str, context->database);
    strcat(context->conn_str, ";");
  }
  if(context->user) {
    strcat(context->conn_str, "UID=");
    strcat(context->conn_str, context->user);
    strcat(context->conn_str, ";");
  }
  if(context->password) {
    strcat(context->conn_str, "PWD=");
    strcat(context->conn_str, context->password);
    strcat(context->conn_str, ";");
  }
  if(context->charset) {
    strcat(context->conn_str, "CHARSET=");
    strcat(context->conn_str, context->charset);
    strcat(context->conn_str, ";");
  }

  /* Initialize Virtuoso connections */
  librdf_storage_virtuoso_init_connections(storage);

  return 0;
}


/*
 * librdf_storage_virtuoso_terminate:
 * @storage: the storage
 *
 * .
 *
 * Close the storage and database connections.
 *
 * Return value: None.
 **/
static void
librdf_storage_virtuoso_terminate(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance *context;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_terminate \n");
#endif
  librdf_storage_virtuoso_finish_connections(storage);

  if(context->password)
    LIBRDF_FREE(char*, (char*)context->password);

  if(context->user)
    LIBRDF_FREE(char*, (char*)context->user);

  if(context->model_name)
    LIBRDF_FREE(char*, (char*)context->model_name);

  if(context->dsn)
    LIBRDF_FREE(char*, (char*)context->dsn);

  if(context->database)
    LIBRDF_FREE(char*, (char*)context->database);

  if(context->charset)
    LIBRDF_FREE(char*, (char*)context->charset);

  if(context->host)
    LIBRDF_FREE(char*, (char*)context->host);

  if(context->conn_str)
    LIBRDF_FREE(char*, (char*)context->conn_str);

  if(context->transaction_handle)
    librdf_storage_virtuoso_transaction_rollback(storage);

  if(context->h_lang) {
    librdf_free_hash(context->h_lang);
    context->h_lang = NULL;
  }
  if(context->h_type) {
    librdf_free_hash(context->h_type);
    context->h_type = NULL;
  }
}


/*
 * librdf_storage_virtuoso_open:
 * @storage: the storage
 * @model: the model
 *
 * .
 *
 * Create or open model in database(nop).
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_virtuoso_open(librdf_storage* storage, librdf_model* model)
{
#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_open \n");
#endif
  return 0;
}


/*
 * librdf_storage_virtuoso_close:
 * @storage: the storage
 *
 * .
 *
 * Close model(nop).
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_virtuoso_close(librdf_storage* storage)
{
#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_close \n");
#endif

  librdf_storage_virtuoso_transaction_rollback(storage);

  return librdf_storage_virtuoso_sync(storage);
}


/*
 * librdf_storage_virtuoso_sync:
 * @storage: the storage
 *
 * Flush all tables, making sure they are saved on disk.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_virtuoso_sync(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance *context;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_sync \n");
#endif

  /* Make sure optimizing for bulk operations is stopped? */
  if(context->bulk)
    librdf_storage_virtuoso_stop_bulk(storage);

  return 0;
}


/*
 * librdf_storage_virtuoso_size:
 * @storage: the storage
 *
 * .
 *
 * Close model(nop).
 *
 * Return value: Negative on failure.
 **/
static int
librdf_storage_virtuoso_size(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance *context;
  char model_size[]="select count(*) from(sparql define input:storage \"\" select * from named <%s> where { graph ?g {?s ?p ?o}})f";
  char *query;
  int count = -1;
  int rc;
  librdf_storage_virtuoso_connection *handle;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_size \n");
#endif

  /* Get Virtuoso connection handle */
  handle = librdf_storage_virtuoso_get_handle(storage);
  if(!handle)
    return -1;

  /* Query for number of statements */
  query = LIBRDF_MALLOC(char*, 
                        strlen(model_size) + strlen(context->model_name) + 2);
  if(!query) {
    librdf_storage_virtuoso_release_handle(storage, handle);
    return -1;
  }

  sprintf(query, model_size, context->model_name);

#ifdef LIBRDF_DEBUG_SQL
  LIBRDF_DEBUG2("SQL: >>%s<<\n", query);
#endif

  rc = SQLExecDirect(handle->hstmt,(UCHAR *) query, SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, handle);
    count = -1;
    goto end;
  }

  rc = SQLFetch(handle->hstmt);
  if(SQL_SUCCEEDED(rc)) {
    int is_null;
    if(vGetDataINT(storage->world, handle, 1, &is_null, &count) == -1)
      count = -1;
  }
  SQLCloseCursor(handle->hstmt);

end:
  LIBRDF_FREE(char*, query);
  librdf_storage_virtuoso_release_handle(storage, handle);

  return count;
}


static int
librdf_storage_virtuoso_add_statement(librdf_storage* storage,
                                      librdf_statement* statement)
{
#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_add_statement \n");
#endif
  return librdf_storage_virtuoso_context_add_statement_helper(storage, NULL,
                                                              statement);
}


/*
 * librdf_storage_virtuoso_context_add_statement - Add a statement to a storage context
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to add
 *
 * Return value: non 0 on failure
 **/
static int
librdf_storage_virtuoso_context_add_statement(librdf_storage* storage,
                                              librdf_node* context_node,
                                              librdf_statement* statement)
{
#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_context_add_statements \n");
#endif
  return librdf_storage_virtuoso_context_add_statement_helper(storage,
                                                              context_node,
                                                              statement);
}


/*
 * librdf_storage_virtuoso_add_statements:
 * @storage: the storage
 * @statement_stream: the stream of statements
 *
 * .
 *
 * Add statements in stream to storage, without context.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_virtuoso_add_statements(librdf_storage* storage,
                                       librdf_stream* statement_stream)
{
  int helper = 0;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_add_statements \n");
#endif

  while(!helper && !librdf_stream_end(statement_stream)) {
    librdf_statement* statement = librdf_stream_get_object(statement_stream);
    helper = librdf_storage_virtuoso_context_add_statement_helper(storage,
                                                                  NULL,
                                                                  statement);
    librdf_stream_next(statement_stream);
  }

  return helper;
}


/*
 * librdf_storage_virtuoso_context_add_statements:
 * @storage: the storage
 * @statement_stream: the stream of statements
 *
 * .
 *
 * Add statements in stream to storage, without context.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_virtuoso_context_add_statements(librdf_storage* storage,
                                               librdf_node* context_node,
                                               librdf_stream* statement_stream)
{
  librdf_storage_virtuoso_instance* context;
  int helper = 0;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_context_add_statements \n");
#endif

  /* Optimize for bulk loads? */
  if(context->bulk) {
    if(librdf_storage_virtuoso_start_bulk(storage))
      return 1;
  }

  while(!helper && !librdf_stream_end(statement_stream)) {
    librdf_statement* statement = librdf_stream_get_object(statement_stream);

    helper = librdf_storage_virtuoso_context_add_statement_helper(storage,
                                                                  context_node,
                                                                  statement);
    librdf_stream_next(statement_stream);
  }

  /* Optimize for bulk loads? */
  if(context->bulk) {
    if(librdf_storage_virtuoso_stop_bulk(storage))
      return 1;
  }

  return helper;
}


static int
BindCtxt(librdf_storage* storage, librdf_storage_virtuoso_connection *handle, SQLUSMALLINT col, char *data, SQLLEN *ind)
{
  SQLUINTEGER ulen;
  int rc;

  *ind = SQL_NTS;
  ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(data));
  rc = SQLBindParameter(handle->hstmt, col, SQL_PARAM_INPUT, SQL_C_CHAR,
                        SQL_VARCHAR, ulen, 0,(SQLPOINTER)data, 0, ind);
  if(!SQL_SUCCEEDED(rc))
    goto error;
  return 0;

error:
  rdf_virtuoso_ODBC_Errors("SQLBindParameter()", storage->world, handle);
  return -1;
}


static int
BindSP(librdf_storage* storage, librdf_storage_virtuoso_connection *handle,
       SQLUSMALLINT col, librdf_node *node, char **data, SQLLEN *ind)
{
  librdf_node_type type = librdf_node_get_type(node);
  SQLUINTEGER ulen;
  int rc;

  *ind = SQL_NTS;
  if(type == LIBRDF_NODE_TYPE_RESOURCE) {
    char *uri = (char *)librdf_uri_as_string(librdf_node_get_uri(node));
    ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(uri));

    rc = SQLBindParameter(handle->hstmt, col, SQL_PARAM_INPUT, SQL_C_CHAR,
                          SQL_VARCHAR, ulen, 0, uri, 0, ind);
    if(!SQL_SUCCEEDED(rc))
      goto error;
  } else if(type == LIBRDF_NODE_TYPE_BLANK) {
    char *value = (char *)librdf_node_get_blank_identifier(node);
    char *bnode = NULL;

    bnode = LIBRDF_MALLOC(char*, strlen(value) + 5);
    if(!bnode)
      return -1;

    strcpy(bnode, "_:");
    strcat(bnode, value);
    *data = bnode;
    ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(bnode));

    rc = SQLBindParameter(handle->hstmt, col, SQL_PARAM_INPUT, SQL_C_CHAR,
                          SQL_VARCHAR, ulen, 0, bnode, 0, ind);
    if(!SQL_SUCCEEDED(rc))
      goto error;

  } else {
    return -1;
  }
  return 0;

error:
  rdf_virtuoso_ODBC_Errors("SQLBindParameter()", storage->world, handle);
  return -1;
}


static int
BindObject(librdf_storage* storage, librdf_storage_virtuoso_connection *handle,
           SQLUSMALLINT col, librdf_node *node, char **data, long *iData,
           SQLLEN *ind1, SQLLEN *ind2, SQLLEN *ind3)
{
  librdf_node_type type = librdf_node_get_type(node);
  SQLUINTEGER ulen;
  int rc;

  if(type == LIBRDF_NODE_TYPE_RESOURCE) {
     char *uri = (char *)librdf_uri_as_string(librdf_node_get_uri(node));
     *iData = 1;
     *ind1 = 0;
     rc = SQLBindParameter(handle->hstmt, col, SQL_PARAM_INPUT, SQL_C_SLONG,
                           SQL_INTEGER, 0, 0, iData, 0, ind1);
     if(!SQL_SUCCEEDED(rc))
       goto error;

     ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(uri));
     *ind2 = SQL_NTS;
     rc = SQLBindParameter(handle->hstmt, col+1, SQL_PARAM_INPUT, SQL_C_CHAR,
                           SQL_VARCHAR, ulen, 0, uri, 0, ind2);
     if(!SQL_SUCCEEDED(rc))
       goto error;
     *ind3 = SQL_NULL_DATA;
     rc = SQLBindParameter(handle->hstmt, col + 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                           SQL_VARCHAR, ulen, 0, NULL, 0, ind3);
     if(!SQL_SUCCEEDED(rc))
       goto error;

  } else if(type == LIBRDF_NODE_TYPE_BLANK) {
     char *value = (char *)librdf_node_get_blank_identifier(node);
     char *bnode = NULL;

     bnode = LIBRDF_MALLOC(char*, strlen(value) + 5);
     if(!bnode)
       return -1;

     *data = bnode;
     strcpy(bnode, "_:");
     strcat(bnode, value);

     *iData = 1;
     *ind1 = 0;
     rc = SQLBindParameter(handle->hstmt, col, SQL_PARAM_INPUT, SQL_C_SLONG,
                           SQL_INTEGER, 0, 0, iData, 0, ind1);
     if(!SQL_SUCCEEDED(rc))
       goto error;

     ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(bnode));
     *ind2 = SQL_NTS;
     rc = SQLBindParameter(handle->hstmt, col+1, SQL_PARAM_INPUT, SQL_C_CHAR,
                           SQL_VARCHAR, ulen, 0, bnode, 0, ind2);
     if(!SQL_SUCCEEDED(rc))
       goto error;
     *ind3 = SQL_NULL_DATA;
     rc = SQLBindParameter(handle->hstmt, col + 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                           SQL_VARCHAR, ulen, 0, NULL, 0, ind3);
     if(!SQL_SUCCEEDED(rc))
       goto error;

  } else if(type == LIBRDF_NODE_TYPE_LITERAL) {
     char *value, *datatype = 0;
     char *lang;
     librdf_uri *dt;

     value = (char *)librdf_node_get_literal_value(node);
     lang = librdf_node_get_literal_value_language(node);
     dt = librdf_node_get_literal_value_datatype_uri(node);
     if(lang) {
       *iData = 5;
       *ind1 = 0;
       rc = SQLBindParameter(handle->hstmt, col, SQL_PARAM_INPUT, SQL_C_SLONG,
                             SQL_INTEGER, 0, 0, iData, 0, ind1);
       if(!SQL_SUCCEEDED(rc))
         goto error;
       ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(value));
       *ind2 = SQL_NTS;
       rc = SQLBindParameter(handle->hstmt, col+1, SQL_PARAM_INPUT, SQL_C_CHAR,
                             SQL_VARCHAR, ulen, 0, value, 0, ind2);
       if(!SQL_SUCCEEDED(rc))
         goto error;
       ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(lang));
       *ind3 = SQL_NTS;
       rc = SQLBindParameter(handle->hstmt, col + 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                             SQL_VARCHAR, ulen, 0, lang, 0, ind3);
       if(!SQL_SUCCEEDED(rc))
         goto error;

     } else if(dt) {
       *iData = 4;
       *ind1 = 0;
       rc = SQLBindParameter(handle->hstmt, col, SQL_PARAM_INPUT, SQL_C_SLONG,
                             SQL_INTEGER, 0, 0, iData, 0, ind1);
       if(!SQL_SUCCEEDED(rc))
         goto error;
       ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(value));
       *ind2 = SQL_NTS;
       rc = SQLBindParameter(handle->hstmt, col+1, SQL_PARAM_INPUT, SQL_C_CHAR,
                             SQL_VARCHAR, ulen, 0, value, 0, ind2);
       if(!SQL_SUCCEEDED(rc))
         goto error;
       datatype = (char *)librdf_uri_as_string(dt);
       ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(datatype));
       *ind3 = SQL_NTS;
       rc = SQLBindParameter(handle->hstmt, col + 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                             SQL_VARCHAR, ulen, 0, datatype, 0, ind3);
       if(!SQL_SUCCEEDED(rc))
         goto error;

     } else {
       *iData = 3;
       *ind1 = 0;
       rc = SQLBindParameter(handle->hstmt, col, SQL_PARAM_INPUT, SQL_C_SLONG,
                             SQL_INTEGER, 0, 0, iData, 0, ind1);
       if(!SQL_SUCCEEDED(rc))
         goto error;
       ulen = LIBRDF_BAD_CAST(SQLUINTEGER, strlen(value));
       *ind2 = SQL_NTS;
       rc = SQLBindParameter(handle->hstmt, col+1, SQL_PARAM_INPUT, SQL_C_CHAR,
                             SQL_VARCHAR, ulen, 0, value, 0, ind2);
       if(!SQL_SUCCEEDED(rc))
         goto error;
       *ind3 = SQL_NULL_DATA;
       rc = SQLBindParameter(handle->hstmt, col + 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                             SQL_VARCHAR, 0, 0, NULL, 0, ind3);
       if(!SQL_SUCCEEDED(rc))
         goto error;
     }
  } else {
    return -1;
  }
  return 0;

error:
  rdf_virtuoso_ODBC_Errors("SQLBindParameter()", storage->world, handle);
  return -1;
}


/*
 * librdf_storage_virtuoso_context_add_statement_helper - Perform actual addition of a statement to a storage context
 * @storage: #librdf_storage object
 * @statement: #librdf_statement statement to add
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_virtuoso_context_add_statement_helper(librdf_storage* storage,
                                                     librdf_node* context_node,
                                                     librdf_statement* statement)
{
  const char *insert_statement="sparql define output:format '_JAVA_' insert into graph iri(\?\?) { `iri(\?\?)` `iri(\?\?)` `bif:__rdf_long_from_batch_params(\?\?,\?\?,\?\?)` }";
  librdf_storage_virtuoso_connection *handle = NULL;
  int rc;
  int ret = 0;
  char *subject = NULL;
  char *predicate = NULL;
  char *object = NULL;
  char *ctxt_node = NULL;
  librdf_node* nsubject = NULL;
  librdf_node* npredicate = NULL;
  librdf_node* nobject = NULL;
  SQLLEN ind, ind1, ind2;
  SQLLEN ind31, ind32, ind33;
  long iData;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_context_add_statement_helper \n");
#endif
  /* Get Virtuoso connection handle */
  handle = librdf_storage_virtuoso_get_handle(storage);
  if(!handle)
    return 1;

  ctxt_node = librdf_storage_virtuoso_icontext2string(storage, context_node);

  nsubject = librdf_statement_get_subject(statement);
  npredicate = librdf_statement_get_predicate(statement);
  nobject = librdf_statement_get_object(statement);

  if(!nsubject || !npredicate || !nobject || !ctxt_node) {
    ret = 1;
    goto end;
  }

  rc = BindCtxt(storage, handle, 1, ctxt_node, &ind);
  if(rc) {
    ret = 1;
    goto end;
  }

  rc = BindSP(storage, handle, 2, nsubject, &subject, &ind1);
  if(rc) {
    ret = 1;
    goto end;
  }
  rc = BindSP(storage, handle, 3, npredicate, &predicate, &ind2);
  if(rc) {
    ret = 1;
    goto end;
  }
  rc = BindObject(storage, handle, 4, nobject, &object, &iData, &ind31,
                  &ind32, &ind33);
  if(rc) {
    ret = 1;
    goto end;
  }

#ifdef VIRTUOSO_STORAGE_DEBUG
  printf("SQL: >>%s<<\n", insert_statement);
#endif
#ifdef LIBRDF_DEBUG_SQL
  LIBRDF_DEBUG2("SQL: >>%s<<\n", insert_statement);
#endif
  rc = SQLExecDirect(handle->hstmt,(SQLCHAR *)insert_statement, SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, handle);
    ret = -1;
    goto end;
  }

end:
  SQLFreeStmt(handle->hstmt, SQL_RESET_PARAMS);
  if(subject)
    LIBRDF_FREE(char*, subject);
  if(predicate)
    LIBRDF_FREE(char*, predicate);
  if(object)
    LIBRDF_FREE(char*, object);
  if(handle)
    librdf_storage_virtuoso_release_handle(storage, handle);

  return ret;
}


/*
 * librdf_storage_virtuoso_start_bulk - Prepare for bulk insert operation
 * @storage: the storage
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_virtuoso_start_bulk(librdf_storage* storage)
{
  return 1;
}


/*
 * librdf_storage_virtuoso_stop_bulk - End bulk insert operation
 * @storage: the storage
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_virtuoso_stop_bulk(librdf_storage* storage)
{
  return 1;
}


/*
 * librdf_storage_virtuoso_contains_statement:
 * @storage: the storage
 * @statement: a complete statement
 *
 * Test if a given complete statement is present in the model.
 *
 * Return value: Non-zero if the model contains the statement.
 **/
static int
librdf_storage_virtuoso_contains_statement(librdf_storage* storage,
                                           librdf_statement* statement)
{
  return librdf_storage_virtuoso_context_contains_statement(storage, NULL,
                                                            statement);
}


/*
 * librdf_storage_virtuoso_contains_statement:
 * @storage: the storage
 * @statement: a complete statement
 *
 * Test if a given complete statement is present in the model.
 *
 * Return value: Non-zero if the model contains the statement.
 **/
static int
librdf_storage_virtuoso_context_contains_statement(librdf_storage* storage,
                                                   librdf_node* context_node,
                                                   librdf_statement* statement)
{
  char find_statement[]="sparql define input:storage \"\" select * where {graph %s { %s %s %s }} limit 1";
  char *query = NULL;
  librdf_storage_virtuoso_connection *handle = NULL;
  int rc;
  int ret = 0;
  char *subject = NULL;
  char *predicate = NULL;
  char *object = NULL;
  char *ctxt_node = NULL;


#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_contains_statement \n");
#endif

  /* Get Virtuoso connection handle */
  handle = librdf_storage_virtuoso_get_handle(storage);
  if(!handle)
    return 0;

  subject = librdf_storage_virtuoso_node2string(storage,
                                                librdf_statement_get_subject(statement));
  predicate = librdf_storage_virtuoso_node2string(storage,
                                                  librdf_statement_get_predicate(statement));
  object = librdf_storage_virtuoso_node2string(storage,
                                               librdf_statement_get_object(statement));
  if(!subject || !predicate || !object) {
    ret = 0;
    goto end;
  }

  ctxt_node = librdf_storage_virtuoso_context2string(storage, context_node);
  if(!ctxt_node) {
    ret = 1;
    goto end;
  }

  query = LIBRDF_MALLOC(char*, strlen(find_statement)+ strlen(ctxt_node)+ strlen(subject) + strlen(predicate) + strlen(object) + 1);
  if(!query) {
    ret = 0;
    goto end;
  }

  sprintf(query, find_statement, ctxt_node, subject, predicate, object);

#ifdef VIRTUOSO_STORAGE_DEBUG
  printf("SQL: >>%s<<\n", query);
#endif
#ifdef LIBRDF_DEBUG_SQL
  LIBRDF_DEBUG2("SQL: >>%s<<\n", query);
#endif

  rc = SQLExecDirect(handle->hstmt,(SQLCHAR *)query, SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, handle);
    ret = 0;
    goto end;
  }

  rc = SQLFetch(handle->hstmt);
  if(SQL_SUCCEEDED(rc)) {
    ret = 1;
  } else if(rc == SQL_NO_DATA_FOUND) {
    ret = 0;
  }

  SQLCloseCursor(handle->hstmt);

end:
  if(query)
    LIBRDF_FREE(char*, query);
  if(ctxt_node)
    LIBRDF_FREE(char*, ctxt_node);
  if(subject)
    LIBRDF_FREE(char*, subject);
  if(predicate)
    LIBRDF_FREE(char*, predicate);
  if(object)
    LIBRDF_FREE(char*, object);
  if(handle)
    librdf_storage_virtuoso_release_handle(storage, handle);

  return ret;
}


/*
 * librdf_storage_virtuoso_remove_statement:
 * @storage: #librdf_storage object
 * @statement: #librdf_statement statement to remove
 *
 * Remove a statement from storage.
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_virtuoso_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  return librdf_storage_virtuoso_context_remove_statement(storage, NULL,
                                                          statement);
}


/*
 * librdf_storage_virtuoso_context_remove_statement - Remove a statement from a storage context
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to remove
 *
 * Remove a statement from storage.
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_virtuoso_context_remove_statement(librdf_storage* storage,
                                                 librdf_node* context_node,
                                                 librdf_statement* statement)
{
  const char *sdelete="sparql define output:format '_JAVA_' delete from graph iri(\?\?) {`iri(\?\?)` `iri(\?\?)` `bif:__rdf_long_from_batch_params(\?\?,\?\?,\?\?)`}";
  const char *sdelete_match="sparql delete from graph <%s> { %s %s %s } from <%s> where { %s %s %s }";
  const char *sdelete_graph="sparql clear graph iri(\?\?)";
  char *query = NULL;
  librdf_storage_virtuoso_connection *handle = NULL;
  int rc;
  int ret = 0;
  char *subject = NULL;
  char *predicate = NULL;
  char *object = NULL;
  char *ctxt_node = NULL;
  librdf_node* nsubject = NULL;
  librdf_node* npredicate = NULL;
  librdf_node* nobject = NULL;
  SQLLEN ind, ind1, ind2;
  SQLLEN ind31, ind32, ind33;
  long iData;


#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_context_remove_statement \n");
#endif

  /* Get Virtuoso connection handle */
  handle = librdf_storage_virtuoso_get_handle(storage);
  if(!handle)
    return 1;

  ctxt_node = librdf_storage_virtuoso_icontext2string(storage, context_node);
  if(!ctxt_node) {
    ret = 1;
    goto end;
  }

  nsubject = librdf_statement_get_subject(statement);
  npredicate = librdf_statement_get_predicate(statement);
  nobject = librdf_statement_get_object(statement);

  if(nsubject == NULL && npredicate == NULL && nobject == NULL && ctxt_node != NULL) {
    ind = SQL_NTS;
    rc = BindCtxt(storage, handle, 1, ctxt_node, &ind);
    if(rc) {
      ret = 1;
      goto end;
    }
    rc = SQLExecDirect(handle->hstmt,(SQLCHAR *)sdelete_graph, SQL_NTS);
    if(!SQL_SUCCEEDED(rc)) {
      rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, handle);
      ret = -1;
      goto end;
    }
  } else if(nsubject != NULL && npredicate != NULL && nobject != NULL &&
            ctxt_node != NULL) {
    ind = SQL_NTS;
    rc = BindCtxt(storage, handle, 1, ctxt_node, &ind);
    if(rc) {
      ret = 1;
      goto end;
    }
    ind1 = SQL_NTS;
    rc = BindSP(storage, handle, 2, nsubject, &subject, &ind1);
    if(rc) {
      ret = 1;
      goto end;
    }
    ind2 = SQL_NTS;
    rc = BindSP(storage, handle, 3, npredicate, &predicate, &ind2);
    if(rc) {
      ret = 1;
      goto end;
    }
    rc = BindObject(storage, handle, 4, nobject, &object, &iData, &ind31,
                    &ind32, &ind33);
    if(rc) {
      ret = 1;
      goto end;
    }
#ifdef VIRTUOSO_STORAGE_DEBUG
    printf("SQL: >>%s<<\n", sdelete);
#endif
    rc = SQLExecDirect(handle->hstmt,(SQLCHAR *)sdelete, SQL_NTS);
    if(!SQL_SUCCEEDED(rc)) {
      rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, handle);
      ret = -1;
      goto end;
    }
  } else {

    subject = librdf_storage_virtuoso_node2string(storage, nsubject);
    predicate = librdf_storage_virtuoso_node2string(storage, npredicate);
    object = librdf_storage_virtuoso_node2string(storage, nobject);

    if(subject && predicate && object)
      query = LIBRDF_MALLOC(char*, strlen(sdelete_match) +
                            strlen(ctxt_node)*2 + strlen(subject)*2 +
                            strlen(predicate)*2 + strlen(object)*2 +1);

    if(!query) {
      ret = 1;
      goto end;
    }

    sprintf(query, sdelete_match, ctxt_node, subject, predicate, object,
            ctxt_node, subject, predicate, object);

#ifdef VIRTUOSO_STORAGE_DEBUG
    printf("SQL: >>%s<<\n", query);
#endif
#ifdef LIBRDF_DEBUG_SQL
    LIBRDF_DEBUG2("SQL: >>%s<<\n", query);
#endif
    rc = SQLExecDirect(handle->hstmt,(SQLCHAR *)query, SQL_NTS);
    if(!SQL_SUCCEEDED(rc)) {
      rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, handle);
      ret = -1;
      goto end;
    }
  }

end:
  SQLFreeStmt(handle->hstmt, SQL_RESET_PARAMS);
  if(query)
    LIBRDF_FREE(char*, query);
  if(ctxt_node)
    LIBRDF_FREE(char*, ctxt_node);
  if(subject)
    LIBRDF_FREE(char*, subject);
  if(predicate)
    LIBRDF_FREE(char*, predicate);
  if(object)
    LIBRDF_FREE(char*, object);
  if(handle) {
    librdf_storage_virtuoso_release_handle(storage, handle);
  }

  return ret;
}


/*
 * librdf_storage_virtuoso_context_remove_statements - Remove all statement from a storage context
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * Remove statements from storage.
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_virtuoso_context_remove_statements(librdf_storage* storage,
                                                  librdf_node* context_node)
{
  const char *remove_statements="sparql clear graph iri(\?\?)";
  librdf_storage_virtuoso_connection *handle = NULL;
  int rc;
  int ret = 0;
  char *ctxt_node = NULL;
  SQLLEN ind = SQL_NTS;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_context_remove_statements \n");
#endif

  /* Get Virtuoso connection handle */
  handle = librdf_storage_virtuoso_get_handle(storage);
  if(!handle)
    return 1;

  ctxt_node = librdf_storage_virtuoso_context2string(storage, context_node);
  if(!ctxt_node) {
    ret = 1;
    goto end;
  }

  rc = BindCtxt(storage, handle, 1, ctxt_node, &ind);
  if(rc) {
    ret = 1;
    goto end;
  }


#ifdef VIRTUOSO_STORAGE_DEBUG
  printf("SQL: >>%s<<\n", remove_statements);
#endif
#ifdef LIBRDF_DEBUG_SQL
  LIBRDF_DEBUG2("SQL: >>%s<<\n", remove_statements);
#endif

  rc = SQLExecDirect(handle->hstmt,(SQLCHAR *)remove_statements, SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, handle);
    ret = -1;
    goto end;
  }

end:
  SQLFreeStmt(handle->hstmt, SQL_RESET_PARAMS);
  if(ctxt_node)
    LIBRDF_FREE(char*, ctxt_node);
  if(handle) {
    librdf_storage_virtuoso_release_handle(storage, handle);
  }

  return ret;
}


/*
 * librdf_storage_virtuoso_serialise - Return a stream of all statements in a storage
 * @storage: the storage
 *
 * Return a stream of all statements in a storage.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_virtuoso_serialise(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance* context;
  librdf_node *node;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

  node = librdf_new_node_from_uri_string(storage->world,(const unsigned char*)context->model_name);

  return librdf_storage_virtuoso_find_statements_in_context(storage,NULL,node);
}


/*
 * librdf_storage_virtuoso_context_serialise - List all statements in a storage context
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * Return a stream of all statements in a storage.
 *
 * Return value: #librdf_stream of statements or NULL on failure or context is empty
 **/
static librdf_stream*
librdf_storage_virtuoso_context_serialise(librdf_storage* storage, librdf_node* context_node)
{
  return librdf_storage_virtuoso_find_statements_in_context(storage, NULL,
                                                            context_node);
}


static int librdf_storage_virtuoso_find_statements_in_context_end_of_stream(void* context);
static int librdf_storage_virtuoso_find_statements_in_context_next_statement(void* context);
static void* librdf_storage_virtuoso_find_statements_in_context_get_statement(void* context, int flags);
static void librdf_storage_virtuoso_find_statements_in_context_finished(void* context);

/*
 * librdf_storage_virtuoso_find_statements - Find a graph of statements in storage.
 * @storage: the storage
 * @statement: the statement to match
 *
 * Return a stream of statements matching the given statement(or
 * all statements if NULL).  Parts(subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_virtuoso_find_statements(librdf_storage* storage,
                                        librdf_statement* statement)
{
  librdf_storage_virtuoso_instance* context;
  librdf_node *node;

  context = (librdf_storage_virtuoso_instance*)storage->instance;

  node = librdf_new_node_from_uri_string(storage->world,(const unsigned char*)context->model_name);

  return librdf_storage_virtuoso_find_statements_in_context(storage, statement,
                                                            node);
}


/*
 * librdf_storage_virtuoso_find_statements_in_context - Find a graph of statements in a storage context.
 * @storage: the storage
 * @statement: the statement to match
 * @context_node: the context to search
 *
 * Return a stream of statements matching the given statement(or
 * all statements if NULL).  Parts(subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_virtuoso_find_statements_in_context(librdf_storage* storage,
                                                   librdf_statement* statement,
                                                   librdf_node* context_node)
{
  char find_statement[]="sparql select * from %s where { %s %s %s }";
  char *query = NULL;
  librdf_storage_virtuoso_sos_context *sos = NULL;
  int rc = 0;
  librdf_node *subject = NULL, *predicate = NULL, *object = NULL;
  const char *s_subject = NULL;
  const char *s_predicate = NULL;
  const char *s_object = NULL;
  const char *ctxt_node = NULL;

  librdf_stream *stream = NULL;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_find_statements_in_context \n");
#endif
  /* Initialize sos context */
  sos = LIBRDF_CALLOC(librdf_storage_virtuoso_sos_context*, 1, sizeof(*sos));
  if(!sos)
    return NULL;

  sos->storage = storage;
  librdf_storage_add_reference(sos->storage);

  if(statement)
    sos->query_statement = librdf_new_statement_from_statement(statement);
  if(context_node)
    sos->query_context = librdf_new_node_from_node(context_node);

  sos->current_statement = NULL;
  sos->current_context = NULL;

  /* Get Vrtuoso connection handle */
  sos->handle = librdf_storage_virtuoso_get_handle(storage);
  if(!sos->handle) {
    librdf_storage_virtuoso_find_statements_in_context_finished((void*)sos);
    goto end;
  }

  if(statement) {
    subject = librdf_statement_get_subject(statement);
    predicate = librdf_statement_get_predicate(statement);
    object = librdf_statement_get_object(statement);
  }

  if(subject) {
    s_subject = librdf_storage_virtuoso_node2string(storage, subject);
    if(strlen(s_subject) == 0) {
      subject = NULL;
      LIBRDF_FREE(char*, (char*)s_subject);
    }
  }
  if(predicate) {
    s_predicate = librdf_storage_virtuoso_node2string(storage, predicate);
    if(strlen(s_predicate) == 0) {
      predicate = NULL;
      LIBRDF_FREE(char*, (char*)s_predicate);
    }
  }
  if(object) {
    s_object = librdf_storage_virtuoso_node2string(storage, object);
    if(strlen(s_object) == 0) {
      object = NULL;
      LIBRDF_FREE(char*, (char*)s_object);
    }
  }

  if(!subject)
    s_subject="?s";

  if(!predicate)
    s_predicate="?p";

  if(!object)
    s_object="?o";

  ctxt_node = librdf_storage_virtuoso_fcontext2string(storage, context_node);
  if(!ctxt_node)
    goto end;

  query = LIBRDF_MALLOC(char*, strlen(find_statement) + 
                               strlen(ctxt_node) + strlen(s_subject) +
                               strlen(s_predicate) + strlen(s_object)+1);
  if(!query) {
    librdf_storage_virtuoso_find_statements_in_context_finished((void*)sos);
    goto end;
  }

  sprintf(query, find_statement, ctxt_node, s_subject, s_predicate, s_object);

#ifdef VIRTUOSO_STORAGE_DEBUG
  printf("SQL: >>%s<<\n", query);
#endif
#ifdef LIBRDF_DEBUG_SQL
  LIBRDF_DEBUG2("SQL: >>%s<<\n", query);
#endif

  rc = SQLExecDirect(sos->handle->hstmt,(SQLCHAR *)query, SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, sos->handle);
    librdf_storage_virtuoso_find_statements_in_context_finished((void*)sos);
    goto end;
  }

  /* Get first statement, if any, and initialize stream */
  if(librdf_storage_virtuoso_find_statements_in_context_next_statement(sos) ) {
    librdf_storage_virtuoso_find_statements_in_context_finished((void*)sos);
    return librdf_new_empty_stream(storage->world);
  }

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_find_statements \n");
#endif
  stream = librdf_new_stream(storage->world,(void*)sos,
                           &librdf_storage_virtuoso_find_statements_in_context_end_of_stream,
                           &librdf_storage_virtuoso_find_statements_in_context_next_statement,
                           &librdf_storage_virtuoso_find_statements_in_context_get_statement,
                           &librdf_storage_virtuoso_find_statements_in_context_finished);

  if(!stream)
    librdf_storage_virtuoso_find_statements_in_context_finished((void*)sos);

end:
  if(query)
    LIBRDF_FREE(char*, (char*)query);
  if(ctxt_node)
    LIBRDF_FREE(char*, (char*)ctxt_node);
  if(subject)
    LIBRDF_FREE(char*, (char*)s_subject);
  if(predicate)
    LIBRDF_FREE(char*, (char*)s_predicate);
  if(object)
    LIBRDF_FREE(char*, (char*)s_object);

  return stream;
}


/*
 * librdf_storage_virtuoso_find_statements_with_options - Find a graph of statements in a storage context with options.
 * @storage: the storage
 * @statement: the statement to match
 * @context_node: the context to search
 * @options: #librdf_hash of match options or NULL
 *
 * Return a stream of statements matching the given statement(or
 * all statements if NULL).  Parts(subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_virtuoso_find_statements_with_options(librdf_storage* storage,
                                                     librdf_statement* statement,
                                                     librdf_node* context_node,
                                                     librdf_hash* options)
{
  return librdf_storage_virtuoso_find_statements_in_context(storage, statement,
                                                            context_node);
}


static int
librdf_storage_virtuoso_find_statements_in_context_end_of_stream(void* context)
{
  librdf_storage_virtuoso_sos_context* sos = (librdf_storage_virtuoso_sos_context*)context;

  return (sos->current_statement == NULL);
}


static int
librdf_storage_virtuoso_find_statements_in_context_next_statement(void* context)
{
  librdf_storage_virtuoso_sos_context* sos = (librdf_storage_virtuoso_sos_context*)context;
  librdf_node *subject = NULL, *predicate = NULL, *object = NULL;
  librdf_node *node;
  SQLUSMALLINT colNum;
  short numCols;
  int rc;

#ifdef VIRTUOSO_STORAGE_DEBUG
  printf("librdf_storage_virtuoso_find_statements_in_context_next_statement\n");
#endif

  rc = SQLNumResultCols(sos->handle->hstmt, &numCols);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLNumResultCols()", sos->storage->world,
                             sos->handle);
    return 1;
  }

  rc = SQLFetch(sos->handle->hstmt);
  if(rc == SQL_NO_DATA_FOUND) {

    if(sos->current_statement)
      librdf_free_statement(sos->current_statement);
    sos->current_statement = NULL;
    if(sos->current_context)
      librdf_free_node(sos->current_context);
    sos->current_context = NULL;
    return 0;
  }
  else if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    rdf_virtuoso_ODBC_Errors((char *)"SQLFetch", sos->storage->world,
                             sos->handle);
    return 1;
  }

  /* Get ready for context */
  if(sos->current_context)
    librdf_free_node(sos->current_context);

  sos->current_context = NULL;

  if(sos->query_statement) {
    subject = librdf_statement_get_subject(sos->query_statement);
    predicate = librdf_statement_get_predicate(sos->query_statement);
    object = librdf_statement_get_object(sos->query_statement);
  }

  /* Make sure we have a statement object to return */
  if(!sos->current_statement) {
    sos->current_statement = librdf_new_statement(sos->storage->world);
    if(!sos->current_statement)
      return 1;
  }

  librdf_statement_clear(sos->current_statement);

    /* Query without variables? */
  if(subject && predicate && object && sos->query_context) {
    librdf_statement_set_subject(sos->current_statement,librdf_new_node_from_node(subject));
    librdf_statement_set_predicate(sos->current_statement,librdf_new_node_from_node(predicate));
    librdf_statement_set_object(sos->current_statement,librdf_new_node_from_node(object));
    sos->current_context = librdf_new_node_from_node(sos->query_context);
  } else {
      char * data;
      int is_null;

      colNum = 1;

      if(sos->query_context) {
        sos->current_context = librdf_new_node_from_node(sos->query_context);
      } else {
        data = vGetDataCHAR(sos->storage->world, sos->handle, colNum, &is_null);
        if(!data || is_null)
          return 1;

        sos->current_context = rdf2node(sos->storage, sos->handle, colNum, data);
        LIBRDF_FREE(char*, (char*)data);
        if(!sos->current_context)
          return 1;

        colNum++;
      }

      if(subject) {
        librdf_statement_set_subject(sos->current_statement,librdf_new_node_from_node(subject));
      } else {

        data = vGetDataCHAR(sos->storage->world, sos->handle, colNum, &is_null);
        if(!data || is_null)
          return 1;

        node = rdf2node(sos->storage, sos->handle, colNum, data);
        LIBRDF_FREE(char*, (char*)data);
        if(!node)
          return 1;

        librdf_statement_set_subject(sos->current_statement, node);
        colNum++;
      }

      if(predicate) {
        librdf_statement_set_predicate(sos->current_statement,librdf_new_node_from_node(predicate));
      } else {
        data = vGetDataCHAR(sos->storage->world, sos->handle, colNum, &is_null);
        if(!data || is_null)
          return 1;

        node = rdf2node(sos->storage, sos->handle, colNum, data);
        LIBRDF_FREE(char*, (char*)data);
        if(!node)
          return 1;

        librdf_statement_set_predicate(sos->current_statement, node);
        colNum++;
      }

      if(object) {
        librdf_statement_set_object(sos->current_statement,librdf_new_node_from_node(object));
      } else {
        data = vGetDataCHAR(sos->storage->world, sos->handle, colNum, &is_null);
        if(!data || is_null)
          return 1;

        node = rdf2node(sos->storage, sos->handle, colNum, data);
        LIBRDF_FREE(char*, (char*)data);
        if(!node)
          return 1;

        librdf_statement_set_object(sos->current_statement, node);
      }
  }
  return 0;
}


static void*
librdf_storage_virtuoso_find_statements_in_context_get_statement(void* context,
                                                                 int flags)
{
  librdf_storage_virtuoso_sos_context* sos;
  sos = (librdf_storage_virtuoso_sos_context*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return sos->current_statement;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return sos->current_context;

    default:
      LIBRDF_DEBUG2("Unknown flags %d\n", flags);
      return NULL;
  }
}


static void
librdf_storage_virtuoso_find_statements_in_context_finished(void* context)
{
  librdf_storage_virtuoso_sos_context* sos;
  sos = (librdf_storage_virtuoso_sos_context*)context;

  if(sos->handle) {
    SQLCloseCursor(sos->handle->hstmt);
    librdf_storage_virtuoso_release_handle(sos->storage, sos->handle);
  }

  if(sos->current_statement)
    librdf_free_statement(sos->current_statement);

  if(sos->current_context)
    librdf_free_node(sos->current_context);

  if(sos->query_statement)
    librdf_free_statement(sos->query_statement);

  if(sos->query_context)
    librdf_free_node(sos->query_context);

  if(sos->storage)
    librdf_storage_remove_reference(sos->storage);

  LIBRDF_FREE(librdf_storage_virtuoso_sos_context, sos);
}


static librdf_node*
librdf_storage_virtuoso_get_feature(librdf_storage* storage,
                                    librdf_uri* feature)
{
  unsigned char *uri_string;

  if(!feature)
    return NULL;

  uri_string = librdf_uri_as_string(feature);

  if(!uri_string)
    return NULL;

  if(!strcmp((const char*)uri_string, LIBRDF_MODEL_FEATURE_CONTEXTS)) {
    unsigned char value[2];
    sprintf((char*)value, "%d", 1);

    return librdf_new_node_from_typed_literal(storage->world, value, NULL, NULL);
  }

  return NULL;
}


/* methods for iterator for contexts */
static int librdf_storage_virtuoso_get_contexts_end_of_iterator(void* context);
static int librdf_storage_virtuoso_get_contexts_next_context(void* context);
static void* librdf_storage_virtuoso_get_contexts_get_context(void* context, int flags);
static void librdf_storage_virtuoso_get_contexts_finished(void* context);


/*
 * librdf_storage_virtuoso_get_contexts:
 * @storage: the storage
 *
 * Return an iterator with the context nodes present in storage.
 *
 * Return value: a #librdf_iterator or NULL on failure
 **/
static librdf_iterator*
librdf_storage_virtuoso_get_contexts(librdf_storage* storage)
{
  librdf_storage_virtuoso_get_contexts_context* gccontext;
  char find_statement[]="DB.DBA.SPARQL_SELECT_KNOWN_GRAPHS()";
  int rc = 0;
  librdf_iterator *iterator = NULL;

  if(!storage)
    return NULL;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_get_contexts \n");
#endif
  /* Initialize get_contexts context */
  gccontext = LIBRDF_CALLOC(librdf_storage_virtuoso_get_contexts_context*, 1,
                            sizeof(*gccontext));
  if(!gccontext)
    return NULL;


  gccontext->storage = storage;
  librdf_storage_add_reference(gccontext->storage);

  gccontext->current_context = NULL;

  /* Get Virtuoso connection handle */
  gccontext->handle = librdf_storage_virtuoso_get_handle(storage);
  if(!gccontext->handle) {
    librdf_storage_virtuoso_get_contexts_finished((void*)gccontext);
    goto end;
  }

#ifdef VIRTUOSO_STORAGE_DEBUG
  printf("SQL: >>%s<<\n", find_statement);
#endif
#ifdef LIBRDF_DEBUG_SQL
  LIBRDF_DEBUG2("SQL: >>%s<<\n", find_statement);
#endif
  rc = SQLExecDirect(gccontext->handle->hstmt,(SQLCHAR *)find_statement,
                     SQL_NTS);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLExecDirect()", storage->world, gccontext->handle);
    librdf_storage_virtuoso_get_contexts_finished((void*)gccontext);
    goto end;
  }

  /* Get first statement, if any, and initialize stream */
  if(librdf_storage_virtuoso_get_contexts_next_context(gccontext) ||
     !gccontext->current_context) {
    librdf_storage_virtuoso_get_contexts_finished((void*)gccontext);
    return librdf_new_empty_iterator(storage->world);
  }

  iterator = librdf_new_iterator(storage->world, 
                                 (void*)gccontext,
                                 &librdf_storage_virtuoso_get_contexts_end_of_iterator,
                                 &librdf_storage_virtuoso_get_contexts_next_context,
                                 &librdf_storage_virtuoso_get_contexts_get_context,
                                 &librdf_storage_virtuoso_get_contexts_finished);

  if(!iterator)
    librdf_storage_virtuoso_get_contexts_finished((void*)gccontext);

end:

  return iterator;
}


static int
librdf_storage_virtuoso_get_contexts_end_of_iterator(void* context)
{
  librdf_storage_virtuoso_get_contexts_context* gccontext;

  gccontext = (librdf_storage_virtuoso_get_contexts_context*)context;

  return gccontext->current_context == NULL;
}


static int
librdf_storage_virtuoso_get_contexts_next_context(void* context)
{
  librdf_storage_virtuoso_get_contexts_context* gccontext;
  int rc;
  SQLUSMALLINT colNum;
  short numCols;
  char *data;
  int is_null;

  gccontext = (librdf_storage_virtuoso_get_contexts_context*)context;

  rc = SQLNumResultCols(gccontext->handle->hstmt, &numCols);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLNumResultCols()", gccontext->storage->world,
                             gccontext->handle);
    return 1;
  }

  rc = SQLFetch(gccontext->handle->hstmt);
  if(rc == SQL_NO_DATA_FOUND) {
    if(gccontext->current_context)
      librdf_free_node(gccontext->current_context);
    gccontext->current_context = NULL;
    return 0;
  } else if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    rdf_virtuoso_ODBC_Errors((char *)"SQLFetch", gccontext->storage->world,
                             gccontext->handle);
    return 1;
  }

  /* Free old context node, if allocated */
  if(gccontext->current_context)
    librdf_free_node(gccontext->current_context);

  colNum = 1;
  data = vGetDataCHAR(gccontext->storage->world, gccontext->handle, colNum,
                      &is_null);
  if(!data || is_null)
    return 1;

  gccontext->current_context = rdf2node(gccontext->storage, gccontext->handle,
                                        colNum, data);
  LIBRDF_FREE(char*, (char*)data);
  if(!gccontext->current_context)
    return 1;

  return 0;
}


static void*
librdf_storage_virtuoso_get_contexts_get_context(void* context, int flags)
{
  librdf_storage_virtuoso_get_contexts_context* gccontext;

  gccontext = (librdf_storage_virtuoso_get_contexts_context*)context;

  return gccontext->current_context;
}


static void
librdf_storage_virtuoso_get_contexts_finished(void* context)
{
  librdf_storage_virtuoso_get_contexts_context* gccontext;

  gccontext = (librdf_storage_virtuoso_get_contexts_context*)context;

  if(gccontext->handle) {
    SQLCloseCursor(gccontext->handle->hstmt);
    librdf_storage_virtuoso_release_handle(gccontext->storage,
                                           gccontext->handle);
  }

  if(gccontext->current_context)
    librdf_free_node(gccontext->current_context);

  if(gccontext->storage)
    librdf_storage_remove_reference(gccontext->storage);

  LIBRDF_FREE(librdf_storage_virtuoso_get_contexts_context, gccontext);
}


/*
 * librdf_storage_virtuoso_transaction_start:
 * @storage: the storage object
 *
 * Start a transaction
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_virtuoso_transaction_start(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance *context;
  int rc;

  context = (librdf_storage_virtuoso_instance *)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_transaction_start \n");
#endif

  if(context->transaction_handle) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Virtuoso transaction already started");
    return 1;
  }

  context->transaction_handle = librdf_storage_virtuoso_get_handle(storage);
  if(!context->transaction_handle)
    return 1;

  rc = SQLSetConnectAttr(context->transaction_handle->hdbc,
                         SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
  if(!SQL_SUCCEEDED(rc)) {
    rdf_virtuoso_ODBC_Errors("SQLSetConnectAttr(hdbc)", storage->world, 
                             context->transaction_handle);
    librdf_storage_virtuoso_release_handle(storage, context->transaction_handle);
    context->transaction_handle = NULL;
    return 1;
  }

  return 0;
}


/*
 * librdf_storage_virtuoso_transaction_start_with_handle:
 * @storage: the storage object
 * @handle: the transaction object
 *
 * Start a transaction using an existing external transaction object.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_virtuoso_transaction_start_with_handle(librdf_storage* storage,
                                                      void* handle)
{
#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_transaction_start_with_handle \n");
#endif
  return librdf_storage_virtuoso_transaction_start(storage);
}


/*
 * librdf_storage_virtuoso_transaction_commit:
 * @storage: the storage object
 *
 * Commit a transaction.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_virtuoso_transaction_commit(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance *context;
  int rc;

  context = (librdf_storage_virtuoso_instance *)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_transaction_commit \n");
#endif

  if(!context->transaction_handle)
    return 1;

  rc = SQLEndTran(SQL_HANDLE_DBC, context->transaction_handle->hdbc, SQL_COMMIT);
  if(!SQL_SUCCEEDED(rc))
    rdf_virtuoso_ODBC_Errors("SQLEndTran(hdbc,COMMIT)", storage->world,
                             context->transaction_handle);

  librdf_storage_virtuoso_release_handle(storage, context->transaction_handle);
  context->transaction_handle = NULL;

  return(SQL_SUCCEEDED(rc) ? 0 : 1);
}


/*
 * librdf_storage_virtuoso_transaction_rollback:
 * @storage: the storage object
 *
 * Rollback a transaction.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_virtuoso_transaction_rollback(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance *context;
  int rc;

  context = (librdf_storage_virtuoso_instance *)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_transaction_rollback \n");
#endif

  if(!context->transaction_handle)
    return 1;

  rc = SQLEndTran(SQL_HANDLE_DBC, context->transaction_handle->hdbc,
                  SQL_ROLLBACK);
  if(!SQL_SUCCEEDED(rc))
    rdf_virtuoso_ODBC_Errors("SQLEndTran(hdbc,ROLLBACK)", storage->world,
                             context->transaction_handle);

  librdf_storage_virtuoso_release_handle(storage, context->transaction_handle);
  context->transaction_handle = NULL;

  return(SQL_SUCCEEDED(rc) ? 0 : 1);
}


/*
 * librdf_storage_virtuoso_transaction_get_handle:
 * @storage: the storage object
 *
 * Get the current transaction handle.
 *
 * Return value: non-0 on failure
 **/
static void*
librdf_storage_virtuoso_transaction_get_handle(librdf_storage* storage)
{
  librdf_storage_virtuoso_instance *context;

  context = (librdf_storage_virtuoso_instance *)storage->instance;

#ifdef VIRTUOSO_STORAGE_DEBUG
  fprintf(stderr, "librdf_storage_virtuoso_transaction_get_handle \n");
#endif

  return context->transaction_handle;
}


/**
 * librdf_storage_virtuoso_supports_query:
 * @storage: #librdf_storage object
 * @query: #librdf_query query object
 *
 * Check if a storage system supports a query language.
 *
 * Not implemented.
 *
 * Return value: non-0 if the query is supported.
 **/
static int
librdf_storage_virtuoso_supports_query(librdf_storage* storage,
                                       librdf_query *query)
{
  librdf_uri *uri;

  uri = librdf_new_uri(storage->world,
                       (unsigned char *)"http://www.w3.org/TR/rdf-vsparql-query/");

  if(uri && query->factory->uri && librdf_uri_equals(query->factory->uri, uri)) {
    librdf_free_uri(uri);
    return 1;
  }

  librdf_free_uri(uri);

  if(!strcmp(query->factory->name, "vsparql"))
    return 1;
  else
    return 0;
}


/**
 * librdf_storage_query_execute:
 * @storage: #librdf_storage object
 * @query: #librdf_query query object
 *
 * Run the given query against the storage.
 *
 * Not implemented.
 *
 * Return value: #librdf_query_results or NULL on failure
 **/
static librdf_query_results*
librdf_storage_virtuoso_query_execute(librdf_storage* storage,
                                      librdf_query *query)
{
  librdf_query_virtuoso_context *qcontext;
  librdf_query_results* results = NULL;

  /* This storage only accepts query languages that it executes
   * ('vsparql') so we know the query context is a pointer to a
   * #librdf_query_virtuoso_context object as initialised by
   * librdf_query_virtuoso_init()
   */
  qcontext = (librdf_query_virtuoso_context*)query->context;

  qcontext->storage = storage;
  librdf_storage_add_reference(storage);

  /* get a server connection (type #librdf_storage_virtuoso_connection) */
  qcontext->vc = librdf_storage_virtuoso_get_handle(storage);

  if(query->factory->execute) {
    /* calls librdf_query_virtuoso_execute() with NULL model
     * having tied the query to this storage.
     */
    if((results = query->factory->execute(query, NULL)))
      librdf_query_add_query_result(query, results);
  }

  return results;

}


/* local function to register Virtuoso storage functions */
static void
librdf_storage_virtuoso_register_factory(librdf_storage_factory *factory)
{
  factory->version                              = LIBRDF_STORAGE_INTERFACE_VERSION;
  factory->init					= librdf_storage_virtuoso_init;
  factory->terminate				= librdf_storage_virtuoso_terminate;
  factory->open					= librdf_storage_virtuoso_open;
  factory->close				= librdf_storage_virtuoso_close;
  factory->sync					= librdf_storage_virtuoso_sync;
  factory->size					= librdf_storage_virtuoso_size;
  factory->add_statement			= librdf_storage_virtuoso_add_statement;
  factory->add_statements			= librdf_storage_virtuoso_add_statements;
  factory->remove_statement			= librdf_storage_virtuoso_remove_statement;
  factory->contains_statement			= librdf_storage_virtuoso_contains_statement;
  factory->serialise				= librdf_storage_virtuoso_serialise;
  factory->find_statements			= librdf_storage_virtuoso_find_statements;
  factory->find_statements_with_options		= librdf_storage_virtuoso_find_statements_with_options;
  factory->context_add_statement		= librdf_storage_virtuoso_context_add_statement;
  factory->context_add_statements		= librdf_storage_virtuoso_context_add_statements;
  factory->context_remove_statement		= librdf_storage_virtuoso_context_remove_statement;
  factory->context_remove_statements		= librdf_storage_virtuoso_context_remove_statements;
  factory->context_serialise			= librdf_storage_virtuoso_context_serialise;
  factory->find_statements_in_context		= librdf_storage_virtuoso_find_statements_in_context;
  factory->get_contexts				= librdf_storage_virtuoso_get_contexts;
  factory->get_feature				= librdf_storage_virtuoso_get_feature;
  factory->transaction_start			= librdf_storage_virtuoso_transaction_start;
  factory->transaction_start_with_handle	= librdf_storage_virtuoso_transaction_start_with_handle;
  factory->transaction_commit			= librdf_storage_virtuoso_transaction_commit;
  factory->transaction_rollback			= librdf_storage_virtuoso_transaction_rollback;
  factory->transaction_get_handle		= librdf_storage_virtuoso_transaction_get_handle;
  factory->supports_query			= librdf_storage_virtuoso_supports_query;
  factory->query_execute			= librdf_storage_virtuoso_query_execute;
}


#ifdef MODULAR_LIBRDF

/* Entry point for dynamically loaded storage module */
void
librdf_storage_module_register_factory(librdf_world *world)
{
  librdf_storage_register_factory(world, "virtuoso",
                                  "OpenLink Virtuoso Universal Server store",
                                  &librdf_storage_virtuoso_register_factory);
}

#else

/*
 * librdf_init_storage_virtuoso:
 * @world: world object
 * 
 * INTERNAL - initialise the storage_virtuoso module.
 **/
void
librdf_init_storage_virtuoso(librdf_world *world)
{
  librdf_storage_register_factory(world, "virtuoso",
                                  "OpenLink Virtuoso Universal Server store",
                                  &librdf_storage_virtuoso_register_factory);
}

#endif
