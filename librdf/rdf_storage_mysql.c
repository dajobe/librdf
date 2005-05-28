/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_mysql.c - RDF Storage in MySQL DB interface definition.
 *
 * $Id$
 *
 * Based in part on rdf_storage_list and rdf_storage_parka.
 *
 * Copyright (C) 2003-2005 Morten Frederiksen - http://purl.org/net/morten/
 *
 * and
 *
 * Copyright (C) 2000-2005, David Beckett http://purl.org/net/dajobe/
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
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif
#include <sys/types.h>

#include <redland.h>
#include <rdf_types.h>

#include <mysql.h>
#include <mysqld_error.h>

typedef enum {
  /* Status of individual MySQL connections */
  LIBRDF_STORAGE_MYSQL_CONNECTION_CLOSED = 0,
  LIBRDF_STORAGE_MYSQL_CONNECTION_OPEN = 1,
  LIBRDF_STORAGE_MYSQL_CONNECTION_BUSY = 2
} librdf_storage_mysql_connection_status;

typedef struct {
  /* A MySQL connection */
  librdf_storage_mysql_connection_status status;
  MYSQL *handle;
} librdf_storage_mysql_connection;

typedef struct {
  /* MySQL connection parameters */
  const char *host;
  int port;
  const char *database;
  const char *user;
  const char *password;

  /* Array of virtual MySQL connections */
  librdf_storage_mysql_connection *connections;
  int connections_count;

  /* hash of model name in the database (table Models, column ID) */
  u64 model;

  /* if inserts should be optimized by locking and index optimizations */
  int bulk;

  /* if a table with merged models should be maintained */
  int merge;

  /* digest object for node hashes */
  librdf_digest *digest;

} librdf_storage_mysql_context;

/* prototypes for local functions */
static int librdf_storage_mysql_init(librdf_storage* storage, char *name,
                                     librdf_hash* options);
static int librdf_storage_mysql_merge(librdf_storage* storage);
static void librdf_storage_mysql_terminate(librdf_storage* storage);
static int librdf_storage_mysql_open(librdf_storage* storage,
                                     librdf_model* model);
static int librdf_storage_mysql_close(librdf_storage* storage);
static int librdf_storage_mysql_sync(librdf_storage* storage);
static int librdf_storage_mysql_size(librdf_storage* storage);
static int librdf_storage_mysql_add_statement(librdf_storage* storage,
                                              librdf_statement* statement);
static int librdf_storage_mysql_add_statements(librdf_storage* storage,
                                               librdf_stream* statement_stream);
static int librdf_storage_mysql_remove_statement(librdf_storage* storage,
                                                 librdf_statement* statement);
static int librdf_storage_mysql_contains_statement(librdf_storage* storage,
                                                   librdf_statement* statement);
static librdf_stream*
       librdf_storage_mysql_serialise(librdf_storage* storage);
static librdf_stream*
       librdf_storage_mysql_find_statements(librdf_storage* storage,
                                            librdf_statement* statement);
static librdf_stream*
       librdf_storage_mysql_find_statements_with_options(librdf_storage* storage,
                                                         librdf_statement* statement,
                                                         librdf_node* context_node,
                                                         librdf_hash* options);

/* context functions */
static int librdf_storage_mysql_context_add_statement(librdf_storage* storage,
                                                      librdf_node* context_node,
                                                      librdf_statement* statement);
static int librdf_storage_mysql_context_add_statements(librdf_storage* storage,
                                                       librdf_node* context_node,
                                                       librdf_stream* statement_stream);
static int librdf_storage_mysql_context_remove_statement(librdf_storage* storage,
                                                         librdf_node* context_node,
                                                         librdf_statement* statement);
static int librdf_storage_mysql_context_remove_statements(librdf_storage* storage,
                                                          librdf_node* context_node);
static librdf_stream*
       librdf_storage_mysql_context_serialise(librdf_storage* storage,
                                              librdf_node* context_node);
static librdf_stream* librdf_storage_mysql_find_statements_in_context(librdf_storage* storage,
                                               librdf_statement* statement,
                                               librdf_node* context_node);
static librdf_iterator* librdf_storage_mysql_get_contexts(librdf_storage* storage);

static void librdf_storage_mysql_register_factory(librdf_storage_factory *factory);

/* "private" helper definitions */
typedef struct {
  librdf_storage *storage;
  librdf_statement *current_statement;
  librdf_node *current_context;
  librdf_statement *query_statement;
  librdf_node *query_context;
  MYSQL *handle;
  MYSQL_RES *results;
  int is_literal_match;
} librdf_storage_mysql_sos_context;

typedef struct {
  librdf_storage *storage;
  librdf_node *current_context;
  MYSQL *handle;
  MYSQL_RES *results;
} librdf_storage_mysql_get_contexts_context;

static u64 librdf_storage_mysql_hash(librdf_storage* storage, char *type,
                                     char *string, int length);
static u64 librdf_storage_mysql_node_hash(librdf_storage* storage,
                                          librdf_node* node,int add);
static int librdf_storage_mysql_start_bulk(librdf_storage* storage);
static int librdf_storage_mysql_stop_bulk(librdf_storage* storage);
static int librdf_storage_mysql_context_add_statement_helper(librdf_storage* storage,
                                                             u64 ctxt,
                                                             librdf_statement* statement);
static int librdf_storage_mysql_find_statements_in_context_augment_query(char **query, char *addition);

/* methods for stream of statements */
static int librdf_storage_mysql_find_statements_in_context_end_of_stream(void* context);
static int librdf_storage_mysql_find_statements_in_context_next_statement(void* context);
static void* librdf_storage_mysql_find_statements_in_context_get_statement(void* context, int flags);
static void librdf_storage_mysql_find_statements_in_context_finished(void* context);

/* methods for iterator for contexts */
static int librdf_storage_mysql_get_contexts_end_of_iterator(void* context);
static int librdf_storage_mysql_get_contexts_next_context(void* context);
static void* librdf_storage_mysql_get_contexts_get_context(void* context, int flags);
static void librdf_storage_mysql_get_contexts_finished(void* context);


/* functions implementing storage api */

/*
 * librdf_storage_mysql_hash - Find hash value of string.
 * @storage: the storage
 * @type: character type of node to hash ("R", "L" or "B")
 * @string: a string to get hash for
 * @length: length of string
 *
 * Find hash value of string.
 *
 * Return value: Non-zero on succes.
 **/
static u64
librdf_storage_mysql_hash(librdf_storage* storage, char *type,
                          char *string, int length)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  u64 hash;
  byte* digest;
  uint i;

  /* (Re)initialize digest object */
  librdf_digest_init(context->digest);
  
  /* Update digest with data */
  if(type)
    librdf_digest_update(context->digest, (unsigned char*)type, 1);
  librdf_digest_update(context->digest, (unsigned char*)string, length);
  librdf_digest_final(context->digest);
  
  /* Copy first 8 bytes of digest into unsigned 64bit hash
   * using a method portable across big/little endianness
   *
   * Fixes Issue#0000023 - http://bugs.librdf.org/mantis/view.php?id=23
   */
  digest = (byte*) librdf_digest_get_digest(context->digest);
  hash = 0;
  for(i=0; i<8; i++)
    hash += ((u64) digest[i]) << (i*8);
  
  return hash;
}


/*
 * librdf_storage_mysql_init_connections - Initialize MySQL connection pool.
 * @storage: the storage
 *
 * Return value: Non-zero on success.
 **/
static int
librdf_storage_mysql_init_connections(librdf_storage* storage)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;

  /* Reset connection pool */
  context->connections=NULL;
  context->connections_count=0;
  return 0;
}


/*
 * librdf_storage_mysql_finish_connections - Finish all connections in MySQL connection pool and free structures.
 * @storage: the storage
 *
 * Return value: None.
 **/
static void
librdf_storage_mysql_finish_connections(librdf_storage* storage)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  int i;

  /* Loop through connections and close */
  for(i=0; i < context->connections_count; i++) {
    if(LIBRDF_STORAGE_MYSQL_CONNECTION_CLOSED != context->connections[i].status)
      mysql_close(context->connections[i].handle);
  }
  /* Free structure and reset */
  if (context->connections_count) {
    LIBRDF_FREE(librdf_storage_mysql_connection*, context->connections);
    context->connections=NULL;
    context->connections_count=0;
  }
}

/*
 * librdf_storage_mysql_get_handle - get a connection handle to the MySQL server
 * @storage: the storage
 *
 * This attempts to reuses any existing available pooled connection
 * otherwise creates a new connection to the server.
 *
 * Return value: Non-zero on succes.
 **/
static MYSQL*
librdf_storage_mysql_get_handle(librdf_storage* storage)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  librdf_storage_mysql_connection* connection= NULL;
  int i;

  /* Look for an open connection handle to return */
  for(i=0; i < context->connections_count; i++) {
    if(LIBRDF_STORAGE_MYSQL_CONNECTION_OPEN == context->connections[i].status) {
      context->connections[i].status=LIBRDF_STORAGE_MYSQL_CONNECTION_BUSY;
      return context->connections[i].handle;
    }
  }

  /* Look for a closed connection */
  for(i=0; i < context->connections_count && !connection; i++) {
    if(LIBRDF_STORAGE_MYSQL_CONNECTION_CLOSED == context->connections[i].status) {
      connection=&context->connections[i];
      break;
    }
  }
  /* Expand connection pool if no closed connection was found */
  if (!connection) {
    /* Allocate new buffer with two extra slots */
    librdf_storage_mysql_connection* connections;
    if(!(connections=(librdf_storage_mysql_connection*)
        LIBRDF_CALLOC(librdf_storage_mysql_connection,
                      context->connections_count+2,
                      sizeof(librdf_storage_mysql_connection))))
      return NULL;

    if (context->connections_count) {
      /* Copy old buffer to new */
      memcpy(connections, context->connections, sizeof(librdf_storage_mysql_connection)*context->connections_count);
      /* Free old buffer */
      LIBRDF_FREE(librdf_storage_mysql_connection*, context->connections);
    }

    /* Update buffer size and reset new connections */
    context->connections_count+=2;
    connection=&connections[context->connections_count-2];
    connection->status=LIBRDF_STORAGE_MYSQL_CONNECTION_CLOSED;
    connection->handle=NULL;
    connections[context->connections_count-1].status=LIBRDF_STORAGE_MYSQL_CONNECTION_CLOSED;
    connections[context->connections_count-1].handle=NULL;
    context->connections=connections;
  }

  /* Initialize closed MySQL connection handle */
  connection->handle=mysql_init(connection->handle);

  /* Create connection to database for handle */
  if(!mysql_real_connect(connection->handle,
                         context->host, context->user, context->password,
                         context->database, context->port, NULL, 0)) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Connection to MySQL database %s:%d name %s as user %s failed: %s",
               context->host, context->port, context->database,
               context->user, mysql_error(connection->handle));
    return NULL;
  }
  /* Update status and return */
  connection->status=LIBRDF_STORAGE_MYSQL_CONNECTION_BUSY;
  return connection->handle;
}


/*
 * librdf_storage_mysql_release_handle - Release a connection handle to MySQL server back to the pool
 * @storage: the storage
 * @handle: the MySQL handle to release
 *
 * Return value: None.
 **/
static void
librdf_storage_mysql_release_handle(librdf_storage* storage, MYSQL *handle)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  int i;

  /* Look for busy connection handle to drop */
  for(i=0; i < context->connections_count; i++) {
    if(LIBRDF_STORAGE_MYSQL_CONNECTION_BUSY == context->connections[i].status &&
       context->connections[i].handle == handle) {
      context->connections[i].status=LIBRDF_STORAGE_MYSQL_CONNECTION_OPEN;
      return;
    }
  }
  librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
             "Unable to find busy connection (in pool of %i connections) to drop for MySQL server thread: %lu",
             context->connections_count, mysql_thread_id(handle));
}


/**
 * librdf_storage_mysql_init:
 * @storage: the storage
 * @name: model name
 * @options: host, port, database, user, password [, new] [, bulk] [, merge].
 *
 * Create connection to database.  Defaults to port 3306 if not given.
 *
 * The boolean bulk option can be set to true if optimized inserts (table
 * locks and temporary key disabling) is wanted. Note that this will block
 * all other access, and requires table locking and alter table privileges.
 *
 * The boolean merge option can be set to true if a merged "view" of all
 * models should be maintained. This "view" will be a table with TYPE=MERGE.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_mysql_init(librdf_storage* storage, char *name,
                          librdf_hash* options)
{
  librdf_storage_mysql_context *context=(librdf_storage_mysql_context*)storage->context;
  const char create_table_statements[]="\
  CREATE TABLE IF NOT EXISTS Statements%llu (\
  Subject bigint unsigned NOT NULL,\
  Predicate bigint unsigned NOT NULL,\
  Object bigint unsigned NOT NULL,\
  Context bigint unsigned NOT NULL,\
  KEY Context (Context),\
  KEY SubjectPredicate (Subject,Predicate),\
  KEY PredicateObject (Predicate,Object),\
  KEY ObjectSubject (Object,Subject)\
) TYPE=MyISAM DELAY_KEY_WRITE=1 MAX_ROWS=100000000 AVG_ROW_LENGTH=33";
  const char create_table_literals[]="\
  CREATE TABLE IF NOT EXISTS Literals (\
  ID bigint unsigned NOT NULL,\
  Value longtext NOT NULL,\
  Language text NOT NULL,\
  Datatype text NOT NULL,\
  PRIMARY KEY ID (ID),\
  FULLTEXT KEY Value (Value)\
) TYPE=MyISAM DELAY_KEY_WRITE=1 MAX_ROWS=100000000 AVG_ROW_LENGTH=44";
  const char create_table_resources[]="\
  CREATE TABLE IF NOT EXISTS Resources (\
  ID bigint unsigned NOT NULL,\
  URI text NOT NULL,\
  PRIMARY KEY ID (ID)\
) TYPE=MyISAM DELAY_KEY_WRITE=1 MAX_ROWS=100000000 AVG_ROW_LENGTH=63";
  const char create_table_bnodes[]="\
  CREATE TABLE IF NOT EXISTS Bnodes (\
  ID bigint unsigned NOT NULL,\
  Name text NOT NULL,\
  PRIMARY KEY ID (ID)\
) TYPE=MyISAM DELAY_KEY_WRITE=1 MAX_ROWS=100000000 AVG_ROW_LENGTH=33";
  const char create_table_models[]="\
  CREATE TABLE IF NOT EXISTS Models (\
  ID bigint unsigned NOT NULL,\
  Name text NOT NULL,\
  PRIMARY KEY ID (ID)\
) TYPE=MyISAM DELAY_KEY_WRITE=1";
  const char create_model[]="INSERT INTO Models (ID,Name) VALUES (%llu,'%s')";
  const char check_model[]="SELECT HIGH_PRIORITY 1 FROM Models WHERE ID=%llu AND Name='%s'";
  int status=0;
  char *escaped_name=NULL;
  char *query=NULL;
  MYSQL_RES *res;
  MYSQL *handle;

  /* Must have connection parameters passed as options */
  if(!options)
    return 1;

  /* Create digest */
  if(!(context->digest=librdf_new_digest(storage->world,"MD5")))
    return 1;

  /* Save hash of model name */
  context->model=librdf_storage_mysql_hash(storage, NULL, name, strlen(name));

  /* Save connection parameters */
  context->host=librdf_hash_get_del(options, "host");
  context->port=librdf_hash_get_as_long(options, "port");
  if(context->port < 0)
    context->port=3306; /* default mysql port */
  context->database=librdf_hash_get_del(options, "database");
  context->user=librdf_hash_get_del(options, "user");
  context->password=librdf_hash_get_del(options, "password");

  if(!context->host || !context->database || !context->user || !context->port
     || !context->password)
    return 1;

  /* Maintain merge table? */
  context->merge=(librdf_hash_get_as_boolean(options, "merge")>0);

  /* Initialize MySQL connections */
  librdf_storage_mysql_init_connections(storage);

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 1;

  /* Create tables, if new and not existing */
  if(!status && (librdf_hash_get_as_boolean(options, "new")>0)) {
    if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(create_table_statements)+
                                    20))) {
      status=1;
    } else {
      sprintf(query, create_table_statements, context->model);
      if(mysql_real_query(handle, query, strlen(query)) ||
         mysql_real_query(handle, create_table_literals,
                          strlen(create_table_literals)) ||
         mysql_real_query(handle, create_table_resources,
                          strlen(create_table_resources)) ||
         mysql_real_query(handle, create_table_bnodes,
                          strlen(create_table_bnodes)) ||
         mysql_real_query(handle, create_table_models,
                          strlen(create_table_models))) {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "MySQL table creation failed: %s",
                   mysql_error(handle));
        status=-1;
      }
      LIBRDF_FREE(cstring, query);
    }
  }

  /* Create model if new and not existing, or check for existence */
  if(!status) {
    if(!(escaped_name=(char*)LIBRDF_MALLOC(cstring,strlen(name)*2+1)))
      status=1;
    mysql_real_escape_string(handle, escaped_name,
                             (const char*)name, strlen(name));
  }
  if(!status && (librdf_hash_get_as_boolean(options, "new")>0)) {
    /* Create new model */
    if(!(query=(char*)LIBRDF_MALLOC(cstring,strlen(create_model)+20+
                                    strlen(escaped_name)+1)))
      status=1;
    sprintf(query, create_model, context->model, name);
    if(!status && mysql_real_query(handle, query, strlen(query)) &&
       mysql_errno(handle) != ER_DUP_ENTRY) {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "MySQL insert into Models table failed: %s",
                 mysql_error(handle));
      status=-1;
    }
    /* Maintain merge table? */
    if(!status && context->merge)
      status=librdf_storage_mysql_merge(storage);
  } else if(!status) {
    /* Check for model existence */
    if(!(query=(char*)LIBRDF_MALLOC(cstring,strlen(check_model)+20+
                                    strlen(escaped_name)+1)))
      status=1;
    sprintf(query, check_model, context->model, name);
    res=NULL;
    if(!status && (mysql_real_query(handle, query, strlen(query)) ||
                   !(res=mysql_store_result(handle)))) {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "MySQL select from Models table failed: %s",
                 mysql_error(handle));
      status=-1;
    }
    if(!status && !(mysql_fetch_row(res))) {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown model: %s",name);
      status=1;
    }
    if(res)
      mysql_free_result(res);
  }
  if(query)
    LIBRDF_FREE(cstring, query);
  if(escaped_name)
    LIBRDF_FREE(cstring, escaped_name);

  /* Optimize loads? */
  context->bulk=(librdf_hash_get_as_boolean(options, "bulk")>0);

  /* Truncate model? */
  if(!status && (librdf_hash_get_as_boolean(options, "new")>0))
    status=librdf_storage_mysql_context_remove_statements(storage, NULL);

  /* Unused options: write (always...) */
  librdf_free_hash(options);

  librdf_storage_mysql_release_handle(storage, handle);
  if(status)
    librdf_storage_mysql_terminate(storage);

  return status;
}

/*
 * librdf_storage_mysql_merge - (re)create merged "view" of all models
 * @storage: the storage
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_mysql_merge(librdf_storage* storage)
{
  const char get_models[]="SELECT ID FROM Models";
  const char drop_table_statements[]="DROP TABLE IF EXISTS Statements";
  const char create_table_statements[]="\
  CREATE TABLE Statements (\
  Subject bigint unsigned NOT NULL,\
  Predicate bigint unsigned NOT NULL,\
  Object bigint unsigned NOT NULL,\
  Context bigint unsigned NOT NULL,\
  KEY Context (Context),\
  KEY SubjectPredicate (Subject,Predicate),\
  KEY PredicateObject (Predicate,Object),\
  KEY ObjectSubject (Object,Subject)\
) TYPE=MERGE INSERT_METHOD=NO UNION=(";
  char *query=NULL;
  MYSQL_RES *res;
  MYSQL_ROW row;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 1;

  /* Query for list of models. */
  if(mysql_real_query(handle, get_models, strlen(get_models)) ||
     !(res=mysql_store_result(handle))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL query for model list failed: %s",
               mysql_error(handle));
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  /* Allocate space for merge table generation query. */
  if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(create_table_statements)+
                                  mysql_num_rows(res)*31+2))) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 1;
  }
  /* Generate CSV list of models. */
  strcpy(query,create_table_statements);
  while((row=mysql_fetch_row(res))) {
    strcat(query,"Statements");
    strcat(query,row[0]);
    strcat(query,",");
  }
  mysql_free_result(res);
  query[strlen(query)-1]=')';

  /* Drop and create merge table. */
  if(mysql_real_query(handle, drop_table_statements,
                      strlen(drop_table_statements)) ||
     mysql_real_query(handle, query, strlen(query))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL merge table creation failed: %s",
               mysql_error(handle));
    LIBRDF_FREE(cstring, query);
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  LIBRDF_FREE(cstring, query);
  librdf_storage_mysql_release_handle(storage, handle);

  return 0;
}

/**
 * librdf_storage_mysql_terminate:
 * @storage: the storage
 *
 * Close the storage and database connections.
 *
 * Return value: None.
 **/
static void
librdf_storage_mysql_terminate(librdf_storage* storage)
{
  librdf_storage_mysql_context *context=(librdf_storage_mysql_context*)storage->context;

  librdf_storage_mysql_finish_connections(storage);

  if(context->password)
    LIBRDF_FREE(cstring,(char *)context->password);

  if(context->user)
    LIBRDF_FREE(cstring,(char *)context->user);

  if(context->database)
    LIBRDF_FREE(cstring,(char *)context->database);

  if(context->host)
    LIBRDF_FREE(cstring,(char *)context->host);

  if(context->digest)
    librdf_free_digest(context->digest);
}

/**
 * librdf_storage_mysql_open:
 * @storage: the storage
 * @model: the model
 *
 * Create or open model in database (nop).
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_mysql_open(librdf_storage* storage, librdf_model* model)
{
  return 0;
}

/**
 * librdf_storage_mysql_close:
 * @storage: the storage
 *
 * Close model (nop).
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_mysql_close(librdf_storage* storage)
{
  return librdf_storage_mysql_sync(storage);
}

/**
 * librdf_storage_mysql_sync - Flush all tables, making sure they are saved on disk.
 * @storage: the storage
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_mysql_sync(librdf_storage* storage)
{
  librdf_storage_mysql_context *context=(librdf_storage_mysql_context*)storage->context;

  /* Make sure optimizing for bulk operations is stopped? */
  if(context->bulk)
    librdf_storage_mysql_stop_bulk(storage);

  return 0;
}

/**
 * librdf_storage_mysql_size:
 * @storage: the storage
 *
 * Close model (nop).
 *
 * Return value: Negative on failure.
 **/
static int
librdf_storage_mysql_size(librdf_storage* storage)
{
  librdf_storage_mysql_context *context=(librdf_storage_mysql_context*)storage->context;
  char model_size[]="SELECT HIGH_PRIORITY COUNT(*) FROM Statements%llu";
  char *query;
  MYSQL_RES *res;
  MYSQL_ROW row;
  int count;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return -1;

  /* Query for number of statements */
  if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(model_size)+21))) {
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  sprintf(query, model_size, context->model);
  if(mysql_real_query(handle, query, strlen(query)) ||
     !(res=mysql_store_result(handle)) ||
     !(row=mysql_fetch_row(res))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL query for model size failed: %s",
               mysql_error(handle));
    LIBRDF_FREE(cstring,query);
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  count=atol(row[0]);
  mysql_free_result(res);
  LIBRDF_FREE(cstring,query);
  librdf_storage_mysql_release_handle(storage, handle);

  return count;
}


static int
librdf_storage_mysql_add_statement(librdf_storage* storage,
                                   librdf_statement* statement)
{
  /* Do not add duplicate statements */
  if(librdf_storage_mysql_contains_statement(storage, statement))
    return 0;

  return librdf_storage_mysql_context_add_statement_helper(storage, 0,
                                                           statement);
}


/**
 * librdf_storage_mysql_add_statements:
 * @storage: the storage
 * @statement_stream: the stream of statements
 *
 * Add statements in stream to storage, without context.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_mysql_add_statements(librdf_storage* storage,
                                    librdf_stream* statement_stream)
{
  int helper=0;

  while(!helper && !librdf_stream_end(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);
    /* Do not add duplicate statements */
    if(!librdf_storage_mysql_contains_statement(storage, statement))
      helper=librdf_storage_mysql_context_add_statement_helper(storage, 0,
                                                               statement);
    librdf_stream_next(statement_stream);
  };

  return helper;
}

/*
 * librdf_storage_mysql_node_hash - Create hash value for node
 * @storage: the storage
 * @node: a node to get hash for (and possibly create in database)
 * @add: whether to add the node to the database
 *
 * Return value: Non-zero on succes.
 **/
static u64
librdf_storage_mysql_node_hash(librdf_storage* storage,
                               librdf_node* node,
                               int add)
{
  librdf_node_type type=librdf_node_get_type(node);
  u64 hash;
  size_t nodelen;
  char *query;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 0;

  if(type==LIBRDF_NODE_TYPE_RESOURCE) {
    /* Get hash */
    unsigned char *uri=librdf_uri_as_counted_string(librdf_node_get_uri(node), &nodelen);
    hash=librdf_storage_mysql_hash(storage, "R", (char*)uri, nodelen);

    if(add) {
      char create_resource[]="INSERT INTO Resources (ID,URI) VALUES (%llu,'%s')";
      /* Escape URI for db query */
      char *escaped_uri;
      if(!(escaped_uri=(char*)LIBRDF_MALLOC(cstring, nodelen*2+1))) {
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      mysql_real_escape_string(handle, escaped_uri,
                               (const char*)uri, nodelen);

      /* Create new resource, ignore if existing */
      if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(create_resource)+
                                      20+nodelen+1))) {
        LIBRDF_FREE(cstring,escaped_uri);
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      sprintf(query, create_resource, hash, escaped_uri);
      LIBRDF_FREE(cstring,escaped_uri);
      if(mysql_real_query(handle, query, strlen(query)) &&
         mysql_errno(handle) != ER_DUP_ENTRY) {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "MySQL insert into Resources failed with error %s", mysql_error(handle));
        LIBRDF_FREE(cstring,query);
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      LIBRDF_FREE(cstring,query);
    }

  } else if(type==LIBRDF_NODE_TYPE_LITERAL) {
    /* Get hash */
    unsigned char *value, *datatype=0;
    char *lang, *nodestring;
    librdf_uri *dt;
    size_t valuelen, langlen=0, datatypelen=0;

    value=librdf_node_get_literal_value_as_counted_string(node,&valuelen);
    lang=librdf_node_get_literal_value_language(node);
    if(lang)
      langlen=strlen(lang);
    dt=librdf_node_get_literal_value_datatype_uri(node);
    if(dt)
      datatype=librdf_uri_as_counted_string(dt,&datatypelen);
    if(datatype)
      datatypelen=strlen((const char*)datatype);

    /* Create composite node string for hash generation */
    if(!(nodestring=(char*)LIBRDF_MALLOC(cstring, valuelen+langlen+datatypelen+3))) {
      librdf_storage_mysql_release_handle(storage, handle);
      return 0;
    }
    strcpy(nodestring, (const char*)value);
    strcat(nodestring, "<");
    if(lang)
      strcat(nodestring, lang);
    strcat(nodestring, ">");
    if(datatype)
      strcat(nodestring, (const char*)datatype);
    nodelen=valuelen+langlen+datatypelen+2;
    hash=librdf_storage_mysql_hash(storage, "L", nodestring, nodelen);
    LIBRDF_FREE(cstring,nodestring);

    if(add) {
      char create_literal[]="INSERT INTO Literals (ID,Value,Language,Datatype) VALUES (%llu,'%s','%s','%s')";
      /* Escape value, lang and datatype for db query */
      char *escaped_value, *escaped_lang, *escaped_datatype;
      if(!(escaped_value=(char*)LIBRDF_MALLOC(cstring, valuelen*2+1)) ||
          !(escaped_lang=(char*)LIBRDF_MALLOC(cstring, langlen*2+1)) ||
          !(escaped_datatype=(char*)LIBRDF_MALLOC(cstring, datatypelen*2+1))) {
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      mysql_real_escape_string(handle, escaped_value,
                               (const char*)value, valuelen);
      if(lang)
        mysql_real_escape_string(handle, escaped_lang,
                                 (const char*)lang, langlen);
      else
        strcpy(escaped_lang,"");
      if(datatype)
        mysql_real_escape_string(handle, escaped_datatype,
                                 (const char*)datatype, datatypelen);
      else
        strcpy(escaped_datatype,"");

      /* Create new literal, ignore if existing */
      if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(create_literal)+
                                      strlen(escaped_value)+
                                      strlen(escaped_lang)+
                                      strlen(escaped_datatype)+21))) {
        LIBRDF_FREE(cstring,escaped_value);
        LIBRDF_FREE(cstring,escaped_lang);
        LIBRDF_FREE(cstring,escaped_datatype);
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      sprintf(query, create_literal, hash, escaped_value, escaped_lang, escaped_datatype);
      LIBRDF_FREE(cstring,escaped_value);
      LIBRDF_FREE(cstring,escaped_lang);
      LIBRDF_FREE(cstring,escaped_datatype);
      if(mysql_real_query(handle, query, strlen(query)) &&
         mysql_errno(handle) != ER_DUP_ENTRY) {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "MySQL insert into Literals failed: %s",
                   mysql_error(handle));
        LIBRDF_FREE(cstring,query);
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      LIBRDF_FREE(cstring,query);
    }

  } else if(type==LIBRDF_NODE_TYPE_BLANK) {
    /* Get hash */
    unsigned char *name=librdf_node_get_blank_identifier(node);
    nodelen=strlen((const char*)name);
    hash=librdf_storage_mysql_hash(storage, "B", (char*)name, nodelen);

    if(add) {
      char create_bnode[]="INSERT INTO Bnodes (ID,Name) VALUES (%llu,'%s')";
      /* Escape name for db query */
      char *escaped_name;
      if(!(escaped_name=(char*)LIBRDF_MALLOC(cstring, nodelen*2+1))) {
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      mysql_real_escape_string(handle, escaped_name,
                               (const char*)name, nodelen);

      /* Create new bnode, ignore if existing */
      if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(create_bnode)+
                                      strlen(escaped_name)+21))) {
        LIBRDF_FREE(cstring,escaped_name);
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      sprintf(query, create_bnode, hash, escaped_name);
      LIBRDF_FREE(cstring,escaped_name);
      if(mysql_real_query(handle, query, strlen(query)) &&
         mysql_errno(handle) != ER_DUP_ENTRY) {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "MySQL insert into Bnodes failed: %s",
                   mysql_error(handle));
        LIBRDF_FREE(cstring,query);
        librdf_storage_mysql_release_handle(storage, handle);
        return 0;
      }
      LIBRDF_FREE(cstring,query);
    }

  } else {
    librdf_storage_mysql_release_handle(storage, handle);
    return 0;
  }

  librdf_storage_mysql_release_handle(storage, handle);
  return hash;
}


/*
 * librdf_storage_mysql_start_bulk - Prepare for bulk insert operation
 * @storage: the storage
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_mysql_start_bulk(librdf_storage* storage)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  char disable_statement_keys[]="ALTER TABLE Statements%llu DISABLE KEYS";
  char disable_literal_keys[]="ALTER TABLE Literals DISABLE KEYS";
  char lock_tables[]="LOCK TABLES Statements%llu WRITE, Resources WRITE, Bnodes WRITE, Literals WRITE";
  char lock_tables_extra[]=", Statements WRITE";
  char *query=NULL;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 1;

  if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(disable_statement_keys)+21))) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 1;
  }
  sprintf(query, disable_statement_keys, context->model);
  if(mysql_real_query(handle, query, strlen(query))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL statement key disabling failed: %s",
               mysql_error(handle));
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  LIBRDF_FREE(cstring,query);
  if(mysql_real_query(handle, disable_literal_keys,
                      strlen(disable_literal_keys))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL literal key disabling failed: %s",
               mysql_error(handle));
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }

  if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(lock_tables)+
                                  strlen(lock_tables_extra)+21))) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 1;
  }
  sprintf(query, lock_tables, context->model);
  if(context->merge)
    strcat(query, lock_tables_extra);
  if(mysql_real_query(handle, query, strlen(query))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL table locking failed: %s",
               mysql_error(handle));
    LIBRDF_FREE(cstring,query);
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  LIBRDF_FREE(cstring,query);
  librdf_storage_mysql_release_handle(storage, handle);

  return 0;
}


/*
 * librdf_storage_mysql_stop_bulk - End bulk insert operation
 * @storage: the storage
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_mysql_stop_bulk(librdf_storage* storage)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  char enable_statement_keys[]="ALTER TABLE Statements%llu ENABLE KEYS";
  char enable_literal_keys[]="ALTER TABLE Literals ENABLE KEYS";
  char unlock_tables[]="UNLOCK TABLES";
  char flush_statements[]="FLUSH TABLE Statements";
  char *query=NULL;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 1;

  if(mysql_real_query(handle, unlock_tables,
                      strlen(unlock_tables))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL table unlocking failed: %s",
               mysql_error(handle));
    librdf_storage_mysql_release_handle(storage, handle);
    return 1;
  }

  if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(enable_statement_keys)+21))) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 1;
  }
  sprintf(query, enable_statement_keys, context->model);
  if(mysql_real_query(handle, query, strlen(query))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL statement key re-enabling failed: %s",
               mysql_error(handle));
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  LIBRDF_FREE(cstring,query);
  if(mysql_real_query(handle, enable_literal_keys,
                      strlen(enable_literal_keys))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL literal key re-enabling failed: %s",
               mysql_error(handle));
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }

  if(context->merge && mysql_real_query(handle, flush_statements,
                      strlen(flush_statements))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL table flush failed: %s",
               mysql_error(handle));
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }

  librdf_storage_mysql_release_handle(storage, handle);
  return 0;
}


/**
 * librdf_storage_mysql_context_add_statements:
 * @storage: the storage
 * @context_node: &librdf_node object
 * @statement_stream: the stream of statements
 *
 * Add statements in stream to storage, with context.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_mysql_context_add_statements(librdf_storage* storage,
                                            librdf_node* context_node,
                                            librdf_stream* statement_stream)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  u64 ctxt=0;
  int helper=0;

  /* Optimize for bulk loads? */
  if(context->bulk) {
    if(librdf_storage_mysql_start_bulk(storage))
      return 1;
  }
  
  /* Find hash for context, creating if necessary */
  if(context_node) {
    ctxt=librdf_storage_mysql_node_hash(storage,context_node,1);
    if(!ctxt)
      return 1;
  }

  while(!helper && !librdf_stream_end(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);
    helper=librdf_storage_mysql_context_add_statement_helper(storage, ctxt,
                                                             statement);
    librdf_stream_next(statement_stream);
  };

  return helper;
}


/**
 * librdf_storage_mysql_context_add_statement - Add a statement to a storage context
 * @storage: &librdf_storage object
 * @context_node: &librdf_node object
 * @statement: &librdf_statement statement to add
 *
 * Return value: non 0 on failure
 **/
static int
librdf_storage_mysql_context_add_statement(librdf_storage* storage,
                                          librdf_node* context_node,
                                          librdf_statement* statement)
{
  u64 ctxt=0;

  /* Find hash for context, creating if necessary */
  if(context_node) {
    ctxt=librdf_storage_mysql_node_hash(storage,context_node,1);
    if(!ctxt)
      return 1;
  };

  return librdf_storage_mysql_context_add_statement_helper(storage, ctxt,
                                                           statement);
}


/*
 * librdf_storage_mysql_context_add_statement_helper - Perform actual addition of a statement to a storage context
 * @storage: &librdf_storage object
 * @ctxt: u64 context hash
 * @statement: &librdf_statement statement to add
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_mysql_context_add_statement_helper(librdf_storage* storage,
                                          u64 ctxt, librdf_statement* statement)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  char insert_statement[]="INSERT INTO Statements%llu (Subject,Predicate,Object,Context) VALUES (%llu,%llu,%llu,%llu)";
  u64 subject, predicate, object;
  char *query;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 1;

  /* Find hashes for nodes, creating if necessary */
  subject=librdf_storage_mysql_node_hash(storage,
                                         librdf_statement_get_subject(statement),1);
  predicate=librdf_storage_mysql_node_hash(storage,
                                           librdf_statement_get_predicate(statement),1);
  object=librdf_storage_mysql_node_hash(storage,
                                        librdf_statement_get_object(statement),1);
  if(!subject || !predicate || !object) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 1;
  }

  /* Add statement to storage */
  if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(insert_statement)+101))) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 1;
  }
  sprintf(query, insert_statement, context->model, subject, predicate, object, ctxt);
  if(mysql_real_query(handle, query, strlen(query))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL insert into Statements failed: %s",
               mysql_error(handle));
    LIBRDF_FREE(cstring,query);
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  LIBRDF_FREE(cstring,query);
  librdf_storage_mysql_release_handle(storage, handle);

  return 0;
}


/**
 * librdf_storage_mysql_contains_statement - Test if a given complete statement is present in the model
 * @storage: the storage
 * @statement: a complete statement
 *
 * Return value: Non-zero if the model contains the statement.
 **/
static int
librdf_storage_mysql_contains_statement(librdf_storage* storage,
                                        librdf_statement* statement)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  char find_statement[]="SELECT HIGH_PRIORITY 1 FROM Statements%llu WHERE Subject=%llu AND Predicate=%llu AND Object=%llu limit 1";
  u64 subject, predicate, object;
  char *query;
  MYSQL_RES *res;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 0;

  /* Find hashes for nodes */
  subject=librdf_storage_mysql_node_hash(storage,
                                         librdf_statement_get_subject(statement),0);
  predicate=librdf_storage_mysql_node_hash(storage,
                                           librdf_statement_get_predicate(statement),0);
  object=librdf_storage_mysql_node_hash(storage,
                                        librdf_statement_get_object(statement),0);
  if(!subject || !predicate || !object) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 0;
  }

  /* Check for statement */
  if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(find_statement)+81))) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 0;
  }
  sprintf(query, find_statement, context->model, subject, predicate, object);
  if(mysql_real_query(handle, query, strlen(query))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL query for statement failed: %s",
               mysql_error(handle));
    LIBRDF_FREE(cstring,query);
    librdf_storage_mysql_release_handle(storage, handle);
    return 0;
  }
  LIBRDF_FREE(cstring, query);
  if(!(res=mysql_store_result(handle)) ||
     !(mysql_fetch_row(res))) {
    if(res)
      mysql_free_result(res);
    librdf_storage_mysql_release_handle(storage, handle);
    return 0;
  }
  if(res)
    mysql_free_result(res);
  librdf_storage_mysql_release_handle(storage, handle);

  return 1;
}


/**
 * librdf_storage_mysql_remove_statement - Remove a statement from storage
 * @storage: &librdf_storage object
 * @statement: &librdf_statement statement to remove
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_mysql_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  return librdf_storage_mysql_context_remove_statement(storage,NULL,statement);
}


/**
 * librdf_storage_mysql_context_remove_statement - Remove a statement from a storage context
 * @storage: &librdf_storage object
 * @context_node: &librdf_node object
 * @statement: &librdf_statement statement to remove
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_mysql_context_remove_statement(librdf_storage* storage,
                                             librdf_node* context_node,
                                             librdf_statement* statement)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  char delete_statement[]="DELETE FROM Statements%llu WHERE Subject=%llu AND Predicate=%llu AND Object=%llu";
  char delete_statement_with_context[]="DELETE FROM Statements%llu WHERE Subject=%llu AND Predicate=%llu AND Object=%llu AND Context=%llu";
  u64 subject, predicate, object, ctxt=0;
  char *query;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 1;

  /* Find hashes for nodes */
  subject=librdf_storage_mysql_node_hash(storage,
                                         librdf_statement_get_subject(statement),0);
  predicate=librdf_storage_mysql_node_hash(storage,
                                           librdf_statement_get_predicate(statement),0);
  object=librdf_storage_mysql_node_hash(storage,
                                        librdf_statement_get_object(statement),0);
  if(context_node) {
    ctxt=librdf_storage_mysql_node_hash(storage,context_node,0);
    if(!ctxt) {
      librdf_storage_mysql_release_handle(storage, handle);
      return 1;
    }
  }
  if(!subject || !predicate || !object || (context_node && !ctxt)) {
    librdf_storage_mysql_release_handle(storage, handle);
    return 1;
  }

  /* Remove statement(s) from storage */
  if(context_node) {
    if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(delete_statement_with_context)+101))) {
      librdf_storage_mysql_release_handle(storage, handle);
      return 1;
    }
    sprintf(query, delete_statement_with_context, context->model, subject,
            predicate, object, ctxt);
  } else {
    if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(delete_statement)+81))) {
      librdf_storage_mysql_release_handle(storage, handle);
      return 1;
    }
    sprintf(query, delete_statement, context->model, subject, predicate,
            object);
  }
  if(mysql_real_query(handle, query, strlen(query))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL delete from Statements failed: %s",
               mysql_error(handle));
    LIBRDF_FREE(cstring,query);
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  LIBRDF_FREE(cstring,query);
  librdf_storage_mysql_release_handle(storage, handle);

  return 0;
}


/**
 * librdf_storage_mysql_context_remove_statements - Remove all statement from a storage context
 * @storage: &librdf_storage object
 * @context_node: &librdf_node object
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_mysql_context_remove_statements(librdf_storage* storage,
                                               librdf_node* context_node)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  char delete_context[]="DELETE FROM Statements%llu WHERE Context=%llu";
  char delete_model[]="DELETE FROM Statements%llu";
  char flush_statements[]="FLUSH TABLE Statements";
  u64 ctxt=0;
  char *query;
  MYSQL *handle;

  /* Get MySQL connection handle */
  handle=librdf_storage_mysql_get_handle(storage);
  if(!handle)
    return 1;

  /* Find hash for context */
  if(context_node) {
    ctxt=librdf_storage_mysql_node_hash(storage,context_node,0);
    if(!ctxt) {
      librdf_storage_mysql_release_handle(storage, handle);
      return 1;
    }
  }

  /* Remove statement(s) from storage */
  if(context_node) {
    if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(delete_context)+61))) {
      librdf_storage_mysql_release_handle(storage, handle);
      return 1;
    }
    sprintf(query, delete_context, context->model, ctxt);
  } else {
    if(!(query=(char*)LIBRDF_MALLOC(cstring, strlen(delete_model)+21))) {
      librdf_storage_mysql_release_handle(storage, handle);
      return 1;
    }
    sprintf(query, delete_model, context->model);
  }
  if(mysql_real_query(handle,query,strlen(query))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL delete of context from Statements failed: %s",
               mysql_error(handle));
    LIBRDF_FREE(cstring,query);
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  LIBRDF_FREE(cstring,query);

  /* Flush merge table when using delete without where... */
  if(context->merge && !context_node
      && mysql_real_query(handle, flush_statements,
                                   strlen(flush_statements))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL table flush failed: %s",
               mysql_error(handle));
    librdf_storage_mysql_release_handle(storage, handle);
    return -1;
  }
  librdf_storage_mysql_release_handle(storage, handle);

  return 0;
}


/**
 * librdf_storage_mysql_serialise - Return a stream of all statements in a storage
 * @storage: the storage
 *
 * Return a stream of all statements in a storage.
 *
 * Return value: a &librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_mysql_serialise(librdf_storage* storage)
{
  return librdf_storage_mysql_find_statements_in_context(storage,NULL,NULL);
}


/**
 * librdf_storage_mysql_find_statements - Find a graph of statements in storage.
 * @storage: the storage
 * @statement: the statement to match
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a &librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_mysql_find_statements(librdf_storage* storage,
                                     librdf_statement* statement)
{
  return librdf_storage_mysql_find_statements_in_context(storage,statement,NULL);
}


/**
 * librdf_storage_mysql_context_serialise - List all statements in a storage context
 * @storage: &librdf_storage object
 * @context_node: &librdf_node object
 *
 * Return value: &librdf_stream of statements or NULL on failure or context is empty
 **/
static librdf_stream*
librdf_storage_mysql_context_serialise(librdf_storage* storage,
                                       librdf_node* context_node)
{
  return librdf_storage_mysql_find_statements_in_context(storage,NULL,context_node);
}


/**
 * librdf_storage_mysql_find_statements_in_context - Find a graph of statements in a storage context.
 * @storage: the storage
 * @statement: the statement to match
 * @context_node: the context to search
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a &librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_mysql_find_statements_in_context(librdf_storage* storage, librdf_statement* statement,
                         librdf_node* context_node)
{
  return librdf_storage_mysql_find_statements_with_options(storage, statement, context_node, NULL);
}


/**
 * librdf_storage_mysql_find_statements_with_options - Find a graph of statements in a storage context with options.
 * @storage: the storage
 * @statement: the statement to match
 * @context_node: the context to search
 * @options: &librdf_hash of match options or NULL
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a &librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_mysql_find_statements_with_options(librdf_storage* storage, 
                                                  librdf_statement* statement,
                                                  librdf_node* context_node,
                                                  librdf_hash* options)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  librdf_storage_mysql_sos_context* sos;
  librdf_node *subject=NULL, *predicate=NULL, *object=NULL;
  char *query;
  char tmp[64];
  char where[256];
  char joins[640];
  librdf_stream *stream;

  /* Initialize sos context */
  if(!(sos=(librdf_storage_mysql_sos_context*)
      LIBRDF_CALLOC(librdf_storage_mysql_sos_context,1,
                    sizeof(librdf_storage_mysql_sos_context))))
    return NULL;
  sos->storage=storage;
  librdf_storage_add_reference(sos->storage);

  if(statement)
    sos->query_statement=librdf_new_statement_from_statement(statement);
  if(context_node)
    sos->query_context=librdf_new_node_from_node(context_node);
  sos->current_statement=NULL;
  sos->current_context=NULL;
  sos->results=NULL;

  if(options) {
    sos->is_literal_match=librdf_hash_get_as_boolean(options, "match-substring");
  }

  /* Get MySQL connection handle */
  sos->handle=librdf_storage_mysql_get_handle(storage);
  if(!sos->handle) {
    librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }

  /* Construct query */
  if(!(query=(char*)LIBRDF_MALLOC(cstring, 21))) {
    librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }
  strcpy(query, "SELECT HIGH_PRIORITY");
  *where='\0';
  if(sos->is_literal_match)
    sprintf(joins, " FROM Literals AS L LEFT JOIN Statements%llu as S ON L.ID=S.Object",
            context->model);
  else
    sprintf(joins, " FROM Statements%llu AS S", context->model);

  if(statement) {
    subject=librdf_statement_get_subject(statement);
    predicate=librdf_statement_get_predicate(statement);
    object=librdf_statement_get_object(statement);
  }

  /* Subject */
  if(statement && subject) {
    sprintf(tmp, "S.Subject=%llu",
            librdf_storage_mysql_node_hash(storage,subject,0));
    if(!strlen(where))
      strcat(where, " WHERE ");
    else
      strcat(where, " AND ");
    strcat(where, tmp);
  } else {
    if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, " SubjectR.URI AS SuR, SubjectB.Name AS SuB")) {
      librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
    strcat(joins," LEFT JOIN Resources AS SubjectR ON S.Subject=SubjectR.ID");
    strcat(joins," LEFT JOIN Bnodes AS SubjectB ON S.Subject=SubjectB.ID");
  }

  /* Predicate */
  if(statement && predicate) {
    sprintf(tmp, "S.Predicate=%llu",
            librdf_storage_mysql_node_hash(storage, predicate, 0));
    if(!strlen(where))
      strcat(where, " WHERE ");
    else
      strcat(where, " AND ");
    strcat(where, tmp);
  } else {
    if(!statement || !subject) {
      if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, ",")) {
        librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
        return NULL;
      }
    }
    if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, " PredicateR.URI AS PrR")) {
      librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
    strcat(joins," LEFT JOIN Resources AS PredicateR ON S.Predicate=PredicateR.ID");
  }

  /* Object */
  if(statement && object) {
    if(!sos->is_literal_match) {
      sprintf(tmp,"S.Object=%llu",
              librdf_storage_mysql_node_hash(storage, object, 0));
      if(!strlen(where))
        strcat(where, " WHERE ");
      else
        strcat(where, " AND ");
      strcat(where, tmp);
    } else {
      /* MATCH literal, not hash_id */
      if(!statement || !subject || !predicate) {
        if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, ",")) {
          librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
          return NULL;
        }
      }
      if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, " ObjectR.URI AS ObR, ObjectB.Name AS ObB, ObjectL.Value AS ObV, ObjectL.Language AS ObL, ObjectL.Datatype AS ObD")) {
        librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
        return NULL;
      }
      strcat(joins," LEFT JOIN Resources AS ObjectR ON S.Object=ObjectR.ID");
      strcat(joins," LEFT JOIN Bnodes AS ObjectB ON S.Object=ObjectB.ID");
      strcat(joins," LEFT JOIN Literals AS ObjectL ON S.Object=ObjectL.ID");

      sprintf(tmp, "MATCH(L.Value) AGAINST ('%s')",
              librdf_node_get_literal_value(object));

      /* NOTE:  This is NOT USED but could be if FULLTEXT wasn't enabled */
      /*
        sprintf(tmp, " L.Value LIKE '%%%s%%'",
                librdf_node_get_literal_value(object));
       */
      if(!strlen(where))
        strcat(where, " WHERE ");
      else
        strcat(where, " AND ");
      strcat(where, tmp);
    }
  } else {
    if(!statement || !subject || !predicate) {
      if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, ",")) {
        librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
        return NULL;
      }
    }
    if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, " ObjectR.URI AS ObR, ObjectB.Name AS ObB, ObjectL.Value AS ObV, ObjectL.Language AS ObL, ObjectL.Datatype AS ObD")) {
      librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
    strcat(joins," LEFT JOIN Resources AS ObjectR ON S.Object=ObjectR.ID");
    strcat(joins," LEFT JOIN Bnodes AS ObjectB ON S.Object=ObjectB.ID");
    strcat(joins," LEFT JOIN Literals AS ObjectL ON S.Object=ObjectL.ID");
  }

  /* Context */
  if(context_node) {
    sprintf(tmp,"S.Context=%llu",
            librdf_storage_mysql_node_hash(storage,context_node,0));
    if(!strlen(where))
      strcat(where, " WHERE ");
    else
      strcat(where, " AND ");
    strcat(where, tmp);
  } else {
    if(!statement || !subject || !predicate || !object) {
      if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, ",")) {
        librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
        return NULL;
      }
    }
    if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, " ContextR.URI AS CoR, ContextB.Name AS CoB, ContextL.Value AS CoV, ContextL.Language AS CoL, ContextL.Datatype AS CoD")) {
      librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
    strcat(joins," LEFT JOIN Resources AS ContextR ON S.Context=ContextR.ID");
    strcat(joins," LEFT JOIN Bnodes AS ContextB ON S.Context=ContextB.ID");
    strcat(joins," LEFT JOIN Literals AS ContextL ON S.Context=ContextL.ID");
  }

  /* Query without variables? */
  if(statement && subject && predicate && object && context_node) {
    if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, " 1")) {
      librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
  }

  /* Complete query string */
  if(librdf_storage_mysql_find_statements_in_context_augment_query(&query, joins) ||
      librdf_storage_mysql_find_statements_in_context_augment_query(&query, where)) {
    librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }

  /* Start query... */
  if(mysql_real_query(sos->handle, query, strlen(query)) ||
     !(sos->results=mysql_use_result(sos->handle))) {
    librdf_log(sos->storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL query failed: %s",
               mysql_error(sos->handle));
    librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
    return NULL;
  };
  LIBRDF_FREE(cstring, query);

  /* Get first statement, if any, and initialize stream */
  if(librdf_storage_mysql_find_statements_in_context_next_statement(sos) ||
      !sos->current_statement) {
    librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
    return librdf_new_empty_stream(storage->world);
  }

  stream=librdf_new_stream(storage->world,(void*)sos,
                           &librdf_storage_mysql_find_statements_in_context_end_of_stream,
                           &librdf_storage_mysql_find_statements_in_context_next_statement,
                           &librdf_storage_mysql_find_statements_in_context_get_statement,
                           &librdf_storage_mysql_find_statements_in_context_finished);
  if(!stream) {
    librdf_storage_mysql_find_statements_in_context_finished((void*)sos);
    return NULL;
  };

  return stream;
}


static int
librdf_storage_mysql_find_statements_in_context_augment_query(char **query, char *addition)
{
  char *newquery;

  /* Augment existing query, returning 0 on success. */
  if(!(newquery=(char*)LIBRDF_MALLOC(cstring, strlen(*query)+strlen(addition)+1)))
      return 1;
  strcpy(newquery,*query);
  strcat(newquery,addition);
  LIBRDF_FREE(cstring, *query);
  *query=newquery;

  return 0;
}


static int
librdf_storage_mysql_find_statements_in_context_end_of_stream(void* context)
{
  librdf_storage_mysql_sos_context* sos=(librdf_storage_mysql_sos_context*)context;

  return sos->current_statement==NULL;
}


static int
librdf_storage_mysql_find_statements_in_context_next_statement(void* context)
{
  librdf_storage_mysql_sos_context* sos=(librdf_storage_mysql_sos_context*)context;
  MYSQL_ROW row;
  librdf_node *subject=NULL, *predicate=NULL, *object=NULL;
  librdf_node *node;

  /* Get next statement */
  row=mysql_fetch_row(sos->results);
  if(row) {
    /* Get ready for context */
    if(sos->current_context)
      librdf_free_node(sos->current_context);
    sos->current_context=NULL;
    /* Is this a query with statement parts? */
    if(sos->query_statement) {
      subject=librdf_statement_get_subject(sos->query_statement);
      predicate=librdf_statement_get_predicate(sos->query_statement);
      if(sos->is_literal_match)
        object=NULL;
      else
        object=librdf_statement_get_object(sos->query_statement);
    }
    /* Make sure we have a statement object to return */
    if(!sos->current_statement) {
      if(!(sos->current_statement=librdf_new_statement(sos->storage->world)))
        return 1;

    }
    librdf_statement_clear(sos->current_statement);
    /* Query without variables? */
    if(subject && predicate && object && sos->query_context) {
      librdf_statement_set_subject(sos->current_statement,librdf_new_node_from_node(subject));
      librdf_statement_set_predicate(sos->current_statement,librdf_new_node_from_node(predicate));
      librdf_statement_set_object(sos->current_statement,librdf_new_node_from_node(object));
      sos->current_context=librdf_new_node_from_node(sos->query_context);
    } else {
      /* Turn row parts into statement and context */
      int part=0;
      /* Subject - constant or from row? */
      if(subject) {
        librdf_statement_set_subject(sos->current_statement,librdf_new_node_from_node(subject));
      } else {
        /* Resource or Bnode? */
        if(row[part]) {
          if(!(node=librdf_new_node_from_uri_string(sos->storage->world,
                                                     (const unsigned char*)row[part])))
            return 1;
        } else if(row[part+1]) {
          if(!(node=librdf_new_node_from_blank_identifier(sos->storage->world,
                                                           (const unsigned char*)row[part+1])))
            return 1;
        } else
          return 1;

        librdf_statement_set_subject(sos->current_statement,node);
        part+=2;
      }
      /* Predicate - constant or from row? */
      if(predicate) {
        librdf_statement_set_predicate(sos->current_statement,librdf_new_node_from_node(predicate));
      } else {
        /* Resource? */
        if(row[part]) {
          if(!(node=librdf_new_node_from_uri_string(sos->storage->world,
                                                     (const unsigned char*)row[part])))
            return 1;
        } else
          return 1;

        librdf_statement_set_predicate(sos->current_statement,node);
        part+=1;
      }
      /* Object - constant or from row? */
      if(object) {
        librdf_statement_set_object(sos->current_statement,librdf_new_node_from_node(object));
      } else {
        /* Resource, Bnode or Literal? */
        if(row[part]) {
          if(!(node=librdf_new_node_from_uri_string(sos->storage->world,
                                                     (const unsigned char*)row[part])))
            return 1;
        } else if(row[part+1]) {
          if(!(node=librdf_new_node_from_blank_identifier(sos->storage->world,
                                                           (const unsigned char*)row[part+1])))
            return 1;
        } else if(row[part+2]) {
          /* Typed literal? */
          librdf_uri *datatype=NULL;
          if(row[part+4] && strlen(row[part+4]))
            datatype=librdf_new_uri(sos->storage->world,
                                    (const unsigned char*)row[part+4]);
          if(!(node=librdf_new_node_from_typed_literal(sos->storage->world,
                                                        (const unsigned char*)row[part+2],
                                                        row[part+3],
                                                        datatype)))
            return 1;
        } else
          return 1;

        librdf_statement_set_object(sos->current_statement,node);
        part+=5;
      }
      /* Context - constant or from row? */
      if(sos->query_context) {
        sos->current_context=librdf_new_node_from_node(sos->query_context);
      } else {
        /* Resource, Bnode or Literal? */
        if(row[part]) {
          if(!(node=librdf_new_node_from_uri_string(sos->storage->world,
                                                     (const unsigned char*)row[part])))
            return 1;
        } else if(row[part+1]) {
          if(!(node=librdf_new_node_from_blank_identifier(sos->storage->world,
                                                           (const unsigned char*)row[part+1])))
            return 1;
        } else if(row[part+2]) {
          /* Typed literal? */
          librdf_uri *datatype=NULL;
          if(row[part+4] && strlen(row[part+4]))
            datatype=librdf_new_uri(sos->storage->world,
                                    (const unsigned char*)row[part+4]);
          if(!(node=librdf_new_node_from_typed_literal(sos->storage->world,
                                                        (const unsigned char*)row[part+2],
                                                        row[part+3],
                                                        datatype)))
            return 1;
        } else
          /* no context */
          node=NULL;
        sos->current_context=node;
      }
    }
  } else {
    if(sos->current_statement)
      librdf_free_statement(sos->current_statement);
    sos->current_statement=NULL;
    if(sos->current_context)
      librdf_free_node(sos->current_context);
    sos->current_context=NULL;
  }

  return 0;
}


static void*
librdf_storage_mysql_find_statements_in_context_get_statement(void* context, int flags)
{
  librdf_storage_mysql_sos_context* sos=(librdf_storage_mysql_sos_context*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return sos->current_statement;
    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return sos->current_context;
    default:
      abort();
  }
}


static void
librdf_storage_mysql_find_statements_in_context_finished(void* context)
{
  librdf_storage_mysql_sos_context* sos=(librdf_storage_mysql_sos_context*)context;

  if(sos->results)
    mysql_free_result(sos->results);

  if(sos->handle)
    librdf_storage_mysql_release_handle(sos->storage, sos->handle);

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

  LIBRDF_FREE(librdf_storage_mysql_sos_context, sos);
}


/**
 * librdf_storage_mysql_get_contexts:
 * @storage: the storage
 *
 * Return an iterator with the context nodes present in storage.
 *
 * Return value: a &librdf_iterator or NULL on failure
 **/
static librdf_iterator*
librdf_storage_mysql_get_contexts(librdf_storage* storage)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  librdf_storage_mysql_get_contexts_context* gccontext;
  const char select_contexts[]="\
SELECT HIGH_PRIORITY DISTINCT R.URI AS CoR, B.Name AS CoB, \
L.Value AS CoV, L.Language AS CoL, L.Datatype AS CoD \
FROM Statements%llu as S \
LEFT JOIN Resources AS R ON S.Context=R.ID \
LEFT JOIN Bnodes AS B ON S.Context=B.ID \
LEFT JOIN Literals AS L ON S.Context=L.ID";
  char *query;
  librdf_iterator *iterator;

  /* Initialize get_contexts context */
  if(!(gccontext=(librdf_storage_mysql_get_contexts_context*)
      LIBRDF_CALLOC(librdf_storage_mysql_get_contexts_context,1,
                    sizeof(librdf_storage_mysql_get_contexts_context))))
    return NULL;
  gccontext->storage=storage;
  librdf_storage_add_reference(gccontext->storage);

  gccontext->current_context=NULL;
  gccontext->results=NULL;

  /* Get MySQL connection handle */
  gccontext->handle=librdf_storage_mysql_get_handle(storage);
  if(!gccontext->handle) {
    librdf_storage_mysql_get_contexts_finished((void*)gccontext);
    return NULL;
  }

  /* Construct query */
  if(!(query=(char*)LIBRDF_MALLOC(cstring,strlen(select_contexts)+21))) {
    librdf_storage_mysql_get_contexts_finished((void*)gccontext);
    return NULL;
  }
  sprintf(query, select_contexts, context->model);

  /* Start query... */
  if(mysql_real_query(gccontext->handle, query, strlen(query)) ||
     !(gccontext->results=mysql_use_result(gccontext->handle))) {
    librdf_log(gccontext->storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "MySQL query failed: %s",
               mysql_error(gccontext->handle));
    librdf_storage_mysql_get_contexts_finished((void*)gccontext);
    return NULL;
  }
  LIBRDF_FREE(cstring, query);

  /* Get first context, if any, and initialize iterator */
  if(librdf_storage_mysql_get_contexts_next_context(gccontext) ||
      !gccontext->current_context) {
    librdf_storage_mysql_get_contexts_finished((void*)gccontext);
    return librdf_new_empty_iterator(storage->world);
  }

  iterator=librdf_new_iterator(storage->world,(void*)gccontext,
                               &librdf_storage_mysql_get_contexts_end_of_iterator,
                               &librdf_storage_mysql_get_contexts_next_context,
                               &librdf_storage_mysql_get_contexts_get_context,
                               &librdf_storage_mysql_get_contexts_finished);
  if(!iterator)
    librdf_storage_mysql_get_contexts_finished((void*)gccontext);

  return iterator;
}


static int
librdf_storage_mysql_get_contexts_end_of_iterator(void* context)
{
  librdf_storage_mysql_get_contexts_context* gccontext=(librdf_storage_mysql_get_contexts_context*)context;

  return gccontext->current_context==NULL;
}


static int
librdf_storage_mysql_get_contexts_next_context(void* context)
{
  librdf_storage_mysql_get_contexts_context* gccontext=(librdf_storage_mysql_get_contexts_context*)context;
  MYSQL_ROW row;
  librdf_node *node;

  /* Get next statement */
  row=mysql_fetch_row(gccontext->results);
  if(row) {
    /* Free old context node, if allocated */
    if(gccontext->current_context)
      librdf_free_node(gccontext->current_context);
    /* Resource, Bnode or Literal? */
    if(row[0]) {
      if(!(node=librdf_new_node_from_uri_string(gccontext->storage->world,
                                                 (const unsigned char*)row[0])))
        return 1;
    } else if(row[1]) {
      if(!(node=librdf_new_node_from_blank_identifier(gccontext->storage->world,
                                                       (const unsigned char*)row[1])))
        return 1;
    } else if(row[2]) {
      /* Typed literal? */
      librdf_uri *datatype=NULL;
      if(row[4] && strlen(row[4]))
        datatype=librdf_new_uri(gccontext->storage->world,
                                (const unsigned char*)row[4]);
      if(!(node=librdf_new_node_from_typed_literal(gccontext->storage->world,
                                                    (const unsigned char*)row[2],
                                                    row[3],
                                                    datatype)))
        return 1;
    } else
      return 1;

    gccontext->current_context=node;
  } else {
    if(gccontext->current_context)
      librdf_free_node(gccontext->current_context);
    gccontext->current_context=NULL;
  }

  return 0;
}


static void*
librdf_storage_mysql_get_contexts_get_context(void* context, int flags)
{
  librdf_storage_mysql_get_contexts_context* gccontext=(librdf_storage_mysql_get_contexts_context*)context;

  return gccontext->current_context;
}


static void
librdf_storage_mysql_get_contexts_finished(void* context)
{
  librdf_storage_mysql_get_contexts_context* gccontext=(librdf_storage_mysql_get_contexts_context*)context;

  if(gccontext->results)
    mysql_free_result(gccontext->results);

  if(gccontext->handle)
    librdf_storage_mysql_release_handle(gccontext->storage, gccontext->handle);

  if(gccontext->current_context)
    librdf_free_node(gccontext->current_context);

  if(gccontext->storage)
    librdf_storage_remove_reference(gccontext->storage);

  LIBRDF_FREE(librdf_storage_mysql_get_contexts_context, gccontext);

}


/**
 * librdf_storage_mysql_get_feature - get the value of a storage feature
 * @storage: &librdf_storage object
 * @feature: &librdf_uri feature property
 * 
 * Return value: &librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
static librdf_node*
librdf_storage_mysql_get_feature(librdf_storage* storage, librdf_uri* feature)
{
  unsigned char *uri_string;

  if(!feature)
    return NULL;

  uri_string=librdf_uri_as_string(feature);
  if(!uri_string)
    return NULL;
  
  if(!strcmp((const char*)uri_string, (const char*)LIBRDF_MODEL_FEATURE_CONTEXTS)) {
    /* Always have contexts */
    static const unsigned char value[2]="1";

    return librdf_new_node_from_typed_literal(storage->world,
                                              value,
                                              NULL, NULL);
  }

  return NULL;
}




/* local function to register MySQL storage functions */
static void
librdf_storage_mysql_register_factory(librdf_storage_factory *factory)
{
  factory->context_length     = sizeof(librdf_storage_mysql_context);
  factory->init               = librdf_storage_mysql_init;
  factory->terminate          = librdf_storage_mysql_terminate;
  factory->open               = librdf_storage_mysql_open;
  factory->close              = librdf_storage_mysql_close;
  factory->sync               = librdf_storage_mysql_sync;
  factory->size               = librdf_storage_mysql_size;
  factory->add_statement      = librdf_storage_mysql_add_statement;
  factory->add_statements     = librdf_storage_mysql_add_statements;
  factory->remove_statement   = librdf_storage_mysql_remove_statement;
  factory->contains_statement = librdf_storage_mysql_contains_statement;
  factory->serialise          = librdf_storage_mysql_serialise;
  factory->find_statements    = librdf_storage_mysql_find_statements;
  factory->find_statements_with_options    = librdf_storage_mysql_find_statements_with_options;
  factory->context_add_statement      = librdf_storage_mysql_context_add_statement;
  factory->context_add_statements     = librdf_storage_mysql_context_add_statements;
  factory->context_remove_statement   = librdf_storage_mysql_context_remove_statement;
  factory->context_remove_statements  = librdf_storage_mysql_context_remove_statements;
  factory->context_serialise          = librdf_storage_mysql_context_serialise;
  factory->find_statements_in_context = librdf_storage_mysql_find_statements_in_context;
  factory->get_contexts               = librdf_storage_mysql_get_contexts;
  factory->get_feature                = librdf_storage_mysql_get_feature;
}


void
librdf_init_storage_mysql(librdf_world *world)
{
  librdf_storage_register_factory(world, "mysql", "MySQL database store",
                                  &librdf_storage_mysql_register_factory);
}


#ifdef NEED_MYSQL_ERRBUFF
char **errbuff=NULL;
#endif
