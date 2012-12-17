/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_postgresql.c - RDF Storage in PostgreSQL DB interface definition.
 *
 * Based in part on rdf_storage_mysql.
 *
 * Copyright (C) 2003-2005 Shi Wenzhong - email to shiwenzhong@hz.cn
 * Copyright (C) 2000-2009, David Beckett http://www.dajobe.org/
 * Copyright (C) 2000-2005, University of Bristol, UK http://www.bristol.ac.uk/
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
#include <stdlib.h>
#endif
#include <sys/types.h>

#include <redland.h>
#include <rdf_types.h>

#include <libpq-fe.h>

typedef enum {
  /* Status of individual postgresql connections */
  LIBRDF_STORAGE_POSTGRESQL_CONNECTION_CLOSED = 0,
  LIBRDF_STORAGE_POSTGRESQL_CONNECTION_OPEN = 1,
  LIBRDF_STORAGE_POSTGRESQL_CONNECTION_BUSY = 2
} librdf_storage_postgresql_connection_status;

typedef struct {
  /* A postgresql connection */
  librdf_storage_postgresql_connection_status status;
  PGconn *handle;
} librdf_storage_postgresql_connection;

typedef struct {
  /* postgresql connection parameters */
  char *host;
  char *port;
  char *dbname;
  char *user;
  const char *password;

  /* Array of virtual postgresql connections */
  librdf_storage_postgresql_connection *connections;
  int connections_count;

  /* hash of model name in the database (table Models, column ID) */
  u64 model;

  /* if inserts should be optimized by locking and index optimizations */
  int bulk;

  /* if a table with merged models should be maintained */
  int merge;

  /* digest object for node hashes */
  librdf_digest *digest;

  PGconn* transaction_handle;

} librdf_storage_postgresql_instance;

/* prototypes for local functions */
static int librdf_storage_postgresql_init(librdf_storage* storage, const char *name,
                                          librdf_hash* options);
static int librdf_storage_postgresql_merge(librdf_storage* storage);
static void librdf_storage_postgresql_terminate(librdf_storage* storage);
static int librdf_storage_postgresql_open(librdf_storage* storage,
                                          librdf_model* model);
static int librdf_storage_postgresql_close(librdf_storage* storage);
static int librdf_storage_postgresql_sync(librdf_storage* storage);
static int librdf_storage_postgresql_size(librdf_storage* storage);
static int librdf_storage_postgresql_add_statement(librdf_storage* storage,
                                              librdf_statement* statement);
static int librdf_storage_postgresql_add_statements(librdf_storage* storage,
                                               librdf_stream* statement_stream);
static int librdf_storage_postgresql_remove_statement(librdf_storage* storage,
                                                 librdf_statement* statement);
static int librdf_storage_postgresql_contains_statement(librdf_storage* storage,
                                                   librdf_statement* statement);


librdf_stream* librdf_storage_postgresql_serialise(librdf_storage* storage);

static librdf_stream* librdf_storage_postgresql_find_statements(librdf_storage* storage,
                                                                librdf_statement* statement);
static librdf_stream* librdf_storage_postgresql_find_statements_with_options(librdf_storage* storage,
                                                                             librdf_statement* statement,
                                                                             librdf_node* context_node,
                                                                             librdf_hash* options);

/* context functions */
static int librdf_storage_postgresql_context_add_statement(librdf_storage* storage,
                                                           librdf_node* context_node,
                                                           librdf_statement* statement);
static int librdf_storage_postgresql_context_add_statements(librdf_storage* storage,
                                                            librdf_node* context_node,
                                                            librdf_stream* statement_stream);
static int librdf_storage_postgresql_context_remove_statement(librdf_storage* storage,
                                                              librdf_node* context_node,
                                                              librdf_statement* statement);
static int librdf_storage_postgresql_context_remove_statements(librdf_storage* storage,
                                                               librdf_node* context_node);
static librdf_stream*
       librdf_storage_postgresql_context_serialise(librdf_storage* storage,
                                                   librdf_node* context_node);
static librdf_stream* librdf_storage_postgresql_find_statements_in_context(librdf_storage* storage,
                                                                           librdf_statement* statement,
                                                                           librdf_node* context_node);
static librdf_iterator* librdf_storage_postgresql_get_contexts(librdf_storage* storage);

static void librdf_storage_postgresql_register_factory(librdf_storage_factory *factory);
#ifdef MODULAR_LIBRDF
void librdf_storage_module_register_factory(librdf_world *world);
#endif

/* "private" helper definitions */
typedef struct {
  librdf_storage *storage;
  librdf_statement *current_statement;
  librdf_node *current_context;
  librdf_statement *query_statement;
  librdf_node *query_context;
  PGconn *handle;
  PGresult *results;
  int current_rowno;
  char **row;
  int is_literal_match;
} librdf_storage_postgresql_sos_context;

typedef struct {
  librdf_storage *storage;
  librdf_node *current_context;
  PGconn *handle;
  PGresult *results;
  int current_rowno;
  char **row;
} librdf_storage_postgresql_get_contexts_context;

static u64 librdf_storage_postgresql_hash(librdf_storage* storage,
                                          const char *type,
                                          const char *string, size_t length);
static u64 librdf_storage_postgresql_node_hash(librdf_storage* storage,
                                               librdf_node* node, int add);
static int librdf_storage_postgresql_start_bulk(librdf_storage* storage);
static int librdf_storage_postgresql_stop_bulk(librdf_storage* storage);
static int librdf_storage_postgresql_context_add_statement_helper(librdf_storage* storage,
                                                                  u64 ctxt,
                                                                  librdf_statement* statement);
static int librdf_storage_postgresql_find_statements_in_context_augment_query(char **query, const char *addition);

/* methods for stream of statements */
static int librdf_storage_postgresql_find_statements_in_context_end_of_stream(void* context);
static int librdf_storage_postgresql_find_statements_in_context_next_statement(void* context);
static void* librdf_storage_postgresql_find_statements_in_context_get_statement(void* context, int flags);
static void librdf_storage_postgresql_find_statements_in_context_finished(void* context);

/* methods for iterator for contexts */
static int librdf_storage_postgresql_get_contexts_end_of_iterator(void* context);
static int librdf_storage_postgresql_get_contexts_next_context(void* context);
static void* librdf_storage_postgresql_get_contexts_get_context(void* context, int flags);
static void librdf_storage_postgresql_get_contexts_finished(void* context);



static int librdf_storage_postgresql_transaction_rollback(librdf_storage* storage);



/* functions implementing storage api */


/*
 * librdf_storage_postgresql_hash - Find hash value of string.
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
librdf_storage_postgresql_hash(librdf_storage* storage, const char *type,
                               const char *string, size_t length)
{
  librdf_storage_postgresql_instance* context;
  u64 hash;
  byte* digest;
  int i;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(string, char*, 0);

  context = (librdf_storage_postgresql_instance*)storage->instance;

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
 * librdf_storage_postgresql_init_connections - Initialize postgresql connection pool.
 * @storage: the storage
 *
 * Return value: Non-zero on success.
 **/
static int
librdf_storage_postgresql_init_connections(librdf_storage* storage)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);

  /* Reset connection pool */
  context->connections=NULL;
  context->connections_count=0;
  return 0;
}


/*
 * librdf_storage_postgresql_finish_connections - Finish all connections in postgresql connection pool and free structures.
 * @storage: the storage
 *
 * Return value: None.
 **/
static void
librdf_storage_postgresql_finish_connections(librdf_storage* storage)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  int i;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(storage, librdf_storage);

  /* Loop through connections and close */
  for(i=0; i < context->connections_count; i++) {
    if(LIBRDF_STORAGE_POSTGRESQL_CONNECTION_CLOSED != context->connections[i].status)
      PQfinish(context->connections[i].handle);
  }
  /* Free structure and reset */
  if (context->connections_count) {
    LIBRDF_FREE(librdf_storage_postgresql_connection*, context->connections);
    context->connections=NULL;
    context->connections_count=0;
  }
}


/*
 * librdf_storage_postgresql_get_handle:
 * @storage: the storage
 *
 * INTERNAL - get a connection handle to the postgresql server
 *
 * This attempts to reuses any existing available pooled connection
 * otherwise creates a new connection to the server.
 *
 * Return value: Non-zero on succes.
 **/
static PGconn*
librdf_storage_postgresql_get_handle(librdf_storage* storage)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  librdf_storage_postgresql_connection* connection= NULL;
  int i;
  const int pool_increment = 2;
  char coninfo_template[] = "host=%s port=%s dbname=%s user=%s password=%s";
  size_t coninfo_size;
  char *conninfo;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  if(context->transaction_handle)
    return context->transaction_handle;

  /* Look for an open connection handle to return */
  for(i=0; i < context->connections_count; i++) {
    if(LIBRDF_STORAGE_POSTGRESQL_CONNECTION_OPEN == context->connections[i].status) {
      context->connections[i].status=LIBRDF_STORAGE_POSTGRESQL_CONNECTION_BUSY;
      return context->connections[i].handle;
    }
  }

  /* Look for a closed connection */
  for(i=0; i < context->connections_count && !connection; i++) {
    if(LIBRDF_STORAGE_POSTGRESQL_CONNECTION_CLOSED == context->connections[i].status) {
      connection=&context->connections[i];
      break;
    }
  }
  /* Expand connection pool if no closed connection was found */
  if (!connection) {
    /* Allocate new buffer with two extra slots */
    int new_pool_size = context->connections_count + pool_increment;
    librdf_storage_postgresql_connection* connections;
    connections = LIBRDF_CALLOC(librdf_storage_postgresql_connection*,
                                LIBRDF_GOOD_CAST(size_t, new_pool_size),
                                sizeof(librdf_storage_postgresql_connection));
    if(!connections)
      return NULL;

    if(context->connections_count) {
      /* Copy old buffer to new */
      memcpy(connections, context->connections,
             sizeof(librdf_storage_postgresql_connection) * LIBRDF_GOOD_CAST(size_t, context->connections_count));
      /* Free old buffer */
      LIBRDF_FREE(librdf_storage_postgresql_connection*, context->connections);
    }

    /* Initialize expanded pool */
    context->connections = connections;
    connection = &(context->connections[context->connections_count]);
    while (context->connections_count < new_pool_size) {
      context->connections[context->connections_count].status = LIBRDF_STORAGE_POSTGRESQL_CONNECTION_CLOSED;
      context->connections[context->connections_count].handle = NULL;
      context->connections_count++;
    }
  }

  /* Initialize closed postgresql connection handle */
  coninfo_size = strlen(coninfo_template)
    + strlen(context->host)
    + strlen(context->port)
    + strlen(context->dbname)
    + strlen(context->user)
    + strlen(context->password);
  conninfo = LIBRDF_MALLOC(char*,coninfo_size);
  if(conninfo) {
    sprintf(conninfo,coninfo_template,context->host,context->port,context->dbname,context->user,context->password);
    connection->handle=PQconnectdb(conninfo);
    if(connection->handle) {
    	if( PQstatus(connection->handle) == CONNECTION_OK ) {
        connection->status=LIBRDF_STORAGE_POSTGRESQL_CONNECTION_BUSY;
      } else {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "Connection to postgresql database %s:%s name %s as user %s failed: %s",
                   context->host, context->port, context->dbname,
                   context->user, PQerrorMessage(connection->handle));
        PQfinish(connection->handle);
        connection->handle=NULL;
      }
    }
    LIBRDF_FREE(char*, conninfo);
  }

  return connection->handle;
}


/*
 * librdf_storage_postgresql_release_handle:
 * @storage: the storage
 * @handle: the postgresql handle to release
 *
 * INTERNAL - Release a connection handle to postgresql server back to the pool
 *
 * Return value: None.
 **/
static void
librdf_storage_postgresql_release_handle(librdf_storage* storage, PGconn *handle)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  int i;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(storage, librdf_storage);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(handle, PGconn*);

  /* Look for busy connection handle to drop */
  for(i=0; i < context->connections_count; i++) {
    if(LIBRDF_STORAGE_POSTGRESQL_CONNECTION_BUSY == context->connections[i].status &&
       context->connections[i].handle == handle) {
      context->connections[i].status=LIBRDF_STORAGE_POSTGRESQL_CONNECTION_OPEN;
      return;
    }
  }
  librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
             "Unable to find busy connection (in pool of %i connections) to drop for postgresql server thread: %d",
             context->connections_count, PQbackendPID(handle));
}


/*
 * librdf_storage_postgresql_init:
 * @storage: the storage
 * @name: model name
 * @options: host, port, database, user, password [, new] [, bulk] [, merge].
 *
 * INTERNAL - Create connection to database.  Defaults to port 5432 if not given.
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
librdf_storage_postgresql_init(librdf_storage* storage, const char *name,
                               librdf_hash* options)
{
  librdf_storage_postgresql_instance *context=(librdf_storage_postgresql_instance*)storage->instance;
  const char create_table_statements[]="\
  CREATE TABLE Statements" UINT64_T_FMT " (\
  Subject numeric(20) NOT NULL,\
  Predicate numeric(20) NOT NULL,\
  Object numeric(20) NOT NULL,\
  Context numeric(20) NOT NULL\
) ";
  const char create_table_literals[]="\
  CREATE TABLE Literals (\
  ID numeric(20) NOT NULL,\
  Value text NOT NULL,\
  Language text NOT NULL,\
  Datatype text NOT NULL,\
  PRIMARY KEY (ID)\
) ";
  const char create_table_resources[]="\
  CREATE TABLE Resources (\
  ID numeric(20) NOT NULL,\
  URI text NOT NULL,\
  PRIMARY KEY (ID)\
) ";
  const char create_table_bnodes[]="\
  CREATE TABLE Bnodes (\
  ID numeric(20) NOT NULL,\
  Name text NOT NULL,\
  PRIMARY KEY (ID)\
) ";
  const char create_table_models[]="\
  CREATE TABLE Models (\
  ID numeric(20) NOT NULL,\
  Name text NOT NULL,\
  PRIMARY KEY (ID)\
) ";

  const char *create_tables[] = {
    create_table_statements,
    create_table_literals,
    create_table_resources,
    create_table_bnodes,
    create_table_models,
    NULL,
  };


  const char create_model[]="INSERT INTO Models (ID,Name) VALUES (" UINT64_T_FMT ",'%s')";
  const char check_model[]="SELECT 1 FROM Models WHERE ID=" UINT64_T_FMT " AND Name='%s'";
  int status=0;
  char *escaped_name=NULL;
  char *query=NULL;
  PGresult *res=NULL;
  PGconn *handle;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(name, char*, 1);

  /* Must have connection parameters passed as options */
  if(!options)
    return 1;

  context = LIBRDF_CALLOC(librdf_storage_postgresql_instance*, 1,
                          sizeof(*context));
  if(!context) {
    librdf_free_hash(options);
    return 1;
  }

  librdf_storage_set_instance(storage, context);


  /* Create digest */
  if(!(context->digest=librdf_new_digest(storage->world,"MD5"))) {
    librdf_free_hash(options);
    return 1;
  }

  /* Save hash of model name */
  context->model = librdf_storage_postgresql_hash(storage, NULL, name,
                                                  strlen(name));

  /* Save connection parameters */
  context->host = librdf_hash_get(options, "host");
  if(!context->host) {
     context->host = LIBRDF_MALLOC(char*, 10);
     strcpy(context->host,"localhost");
  }

  context->port = librdf_hash_get(options, "port");
  if(!context->port) {
     context->port = LIBRDF_MALLOC(char*, 10);
     strcpy(context->port,"5432"); /* default postgresql port */
  }

  context->dbname = librdf_hash_get(options, "database");
  if(!context->dbname)
    context->dbname = librdf_hash_get(options, "dbname");

  context->user = librdf_hash_get(options, "user");
  if(context->user && !context->dbname) {
     context->dbname = LIBRDF_MALLOC(char*, strlen(context->user) + 1);
     strcpy(context->dbname, context->user); /* default dbname=user */
  }

  context->password = librdf_hash_get(options, "password");

  if(!context->host || !context->dbname || !context->user || !context->port
     || !context->password) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "%s storage requires database/dbname, user and password in options", 
               storage->factory->name);
    librdf_free_hash(options);
    return 1;
  }

  /* Maintain merge table? */
  context->merge=(librdf_hash_get_as_boolean(options, "merge")>0);

  /* Initialize postgresql connections */
  librdf_storage_postgresql_init_connections(storage);

  /* Get postgresql connection handle */
  handle=librdf_storage_postgresql_get_handle(storage);
  if(!handle) {
    librdf_free_hash(options);
    return 1;
  }

  /* Create tables, if new and not existing */
  if(!status && (librdf_hash_get_as_boolean(options, "new")>0))
  {
    query = LIBRDF_MALLOC(char*, strlen(create_table_statements) + (20*1) + 1);
    if(! query) {
      status=1;
    } else {
      int i;
      sprintf(query, create_table_statements, context->model);
      create_tables[0] = query;
      for (i = 0; !status && NULL != create_tables[i]; i++) {
        PGresult *res2 = PQexec(handle, create_tables[i]);
        if (res2) {
          if (PQresultStatus(res2) != PGRES_COMMAND_OK) {
            if (0 != strncmp("42P07", PQresultErrorField(res2, PG_DIAG_SQLSTATE), strlen("42P07"))) {
              librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                         "postgresql table creation failed: %s",
                         PQresultErrorMessage(res2));
              status = -1;
            }
          }
          PQclear(res2);
        } else {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "postgresql table creation failed: %s",
                     PQerrorMessage(handle));
          status = -1;
        }
      }
      LIBRDF_FREE(char*, query);
      query=NULL;
    }
  }

  /* Create model if new and not existing, or check for existence */
  if(!status) {
    escaped_name = LIBRDF_MALLOC(char*, strlen(name) * 2 + 1);
    if(!escaped_name)
      status=1;
    else {
      int error = 0;
      PQescapeStringConn(handle, escaped_name,
                         (const char*)name, strlen(name),
                         &error);
      if(error) {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "postgresql escapeStringConn() failed with error %s",
                   PQerrorMessage(handle));
        status = 1;
      }
    }
  }
  if(!status && (librdf_hash_get_as_boolean(options, "new")>0)) {
    /* Create new model */
    query = LIBRDF_MALLOC(char*,
                          strlen(create_model) + 20 + strlen(escaped_name) + 1);
    if(!query)
      status=1;
    else {
      sprintf(query, create_model, context->model, escaped_name);
      if((res=PQexec(handle, query))) {
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
          if (0 != strncmp("23505", PQresultErrorField(res, PG_DIAG_SQLSTATE), strlen("23505"))) {
            librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                       "postgresql table creation failed with error %s",
                       PQresultErrorMessage(res));
            status = -1;
          }
        }
        PQclear(res);
      } else {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "postgresql insert into Models table failed: %s",
                   PQerrorMessage(handle));
        status=-1;
      }
      LIBRDF_FREE(char*, query);
      query=NULL;
    }
    /* Maintain merge table? */
    if(!status && context->merge)
      status=librdf_storage_postgresql_merge(storage);
  } else if(!status) {
    /* Check for model existence */
    query = LIBRDF_MALLOC(char*,
                          strlen(check_model) + 20 + strlen(escaped_name) + 1);
    if(!query)
      status=1;
    else {
      sprintf(query, check_model, context->model, name);
      res=NULL;
      if((res=PQexec(handle, query))) {
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "postgresql insert into Models table failed: %s",
                     PQresultErrorMessage(res));
          status=-1;
        }
        if(!status && !(PQntuples(res))) {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "postgresql failed to find model '%s'", name);
          status=1;
        }
        PQclear(res);
      } else {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "postgresql select from Models table failed: %s",
                   PQerrorMessage(handle));
        status=-1;
      }
      LIBRDF_FREE(char*, query);
      query=NULL;
    }
  }
  if(escaped_name)
    LIBRDF_FREE(char*, escaped_name);

  /* Optimize loads? */
  context->bulk=(librdf_hash_get_as_boolean(options, "bulk")>0);

  /* Truncate model? */
   if(!status && (librdf_hash_get_as_boolean(options, "new")>0))
    status=librdf_storage_postgresql_context_remove_statements(storage, NULL);

  /* Unused options: write (always...) */
  librdf_free_hash(options);

  librdf_storage_postgresql_release_handle(storage, handle);

  return status;
}


/*
 * librdf_storage_postgresql_merge:
 * @storage: the storage
 *
 * INTERNAL - (re)create merged "view" of all models
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_postgresql_merge(librdf_storage* storage)
{
  const char get_models[]="SELECT ID FROM Models";
  const char drop_table_statements[]="DROP TABLE Statements";
  const char insert_statements[]="INSERT INTO statements SELECT * FROM ";
  const char create_table_statements[]="\
  CREATE TABLE Statements (\
  Subject numeric(20) NOT NULL,\
  Predicate numeric(20) NOT NULL,\
  Object numeric(20) NOT NULL,\
  Context numeric(20) NOT NULL\
) ";
  char *query=NULL;
  PGresult *res;
  int i;
  PGconn *handle;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  /* Get postgresql connection handle */
  handle=librdf_storage_postgresql_get_handle(storage);
  if(!handle)
    return 1;

  /* Drop and create merge table. */
  if(! PQexec(handle, drop_table_statements) ||
     ! PQexec(handle, create_table_statements)) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql merge table creation failed: %s",
               PQerrorMessage(handle));
    librdf_storage_postgresql_release_handle(storage, handle);
    return -1;
  }

  /* Query for list of models. */
  if(!(res=PQexec(handle, get_models))) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql query for model list failed: %s",
               PQresultErrorMessage(res));
    librdf_storage_postgresql_release_handle(storage, handle);
    return -1;
   }

  /* Allocate space for merge table generation query. */
  query = LIBRDF_MALLOC(char*, strlen(insert_statements) + 50);
  if(!query) {
    PQclear(res);
    librdf_storage_postgresql_release_handle(storage, handle);
    return 1;
  }

  /* Generate CSV list of models. */
  for(i=0;i<PQntuples(res);i++) {
    strcpy(query,insert_statements);
    strcat(query,"Statements");
    strcat(query,PQgetvalue(res,i,0));
    if(! PQexec(handle, query)) {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql merge table insert failed: %s",
               PQerrorMessage(handle));
        LIBRDF_FREE(char*, query);
        PQclear(res);
        librdf_storage_postgresql_release_handle(storage, handle);
        return -1;
     }
  }

  LIBRDF_FREE(char*, query);
  PQclear(res);
  librdf_storage_postgresql_release_handle(storage, handle);

  return 0;
}


/*
 * librdf_storage_postgresql_terminate:
 * @storage: the storage
 *
 * INTERNAL - Close the storage and database connections.
 *
 * Return value: None.
 **/
static void
librdf_storage_postgresql_terminate(librdf_storage* storage)
{
  librdf_storage_postgresql_instance *context=(librdf_storage_postgresql_instance*)storage->instance;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(storage, librdf_storage);

  librdf_storage_postgresql_finish_connections(storage);

  if(context->password)
    LIBRDF_FREE(char*, (char*)context->password);

  if(context->user)
    LIBRDF_FREE(char*, (char*)context->user);

  if(context->dbname)
    LIBRDF_FREE(char*, (char*)context->dbname);

  if(context->port)
    LIBRDF_FREE(char*, (char*)context->port);

  if(context->host)
    LIBRDF_FREE(char*, (char*)context->host);

  if(context->digest)
    librdf_free_digest(context->digest);

  if(context->transaction_handle)
    librdf_storage_postgresql_transaction_rollback(storage);

  LIBRDF_FREE(librdf_storage_postgresql_instance, storage->instance);
}


/*
 * librdf_storage_postgresql_open:
 * @storage: the storage
 * @model: the model
 *
 * INTERNAL - Create or open model in database (nop).
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_postgresql_open(librdf_storage* storage, librdf_model* model)
{
  return 0;
}


/*
 * librdf_storage_postgresql_close:
 * @storage: the storage
 *
 * INTERNAL - Close model (nop).
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_postgresql_close(librdf_storage* storage)
{
  librdf_storage_postgresql_transaction_rollback(storage);

  return librdf_storage_postgresql_sync(storage);
}


/*
 * librdf_storage_postgresql_sync
 * @storage: the storage
 *
 * INTERNAL - Flush all tables, making sure they are saved on disk.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_postgresql_sync(librdf_storage* storage)
{
  librdf_storage_postgresql_instance *context=(librdf_storage_postgresql_instance*)storage->instance;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  /* Make sure optimizing for bulk operations is stopped? */
  if(context->bulk)
    librdf_storage_postgresql_stop_bulk(storage);

  return 0;
}


/*
 * librdf_storage_postgresql_size:
 * @storage: the storage
 *
 * INTERNAL - Close model (nop).
 *
 * Return value: Negative on failure.
 **/
static int
librdf_storage_postgresql_size(librdf_storage* storage)
{
  librdf_storage_postgresql_instance *context=(librdf_storage_postgresql_instance*)storage->instance;
  char model_size[]="SELECT COUNT(*) FROM Statements" UINT64_T_FMT;
  char *query;
  PGresult *res;
  long count;
  PGconn *handle;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, -1);

  /* Get postgresql connection handle */
  handle=librdf_storage_postgresql_get_handle(storage);
  if(!handle)
    return -1;

  /* Query for number of statements */
  query = LIBRDF_MALLOC(char*, strlen(model_size) + 20 + 1);
  if(!query) {
    librdf_storage_postgresql_release_handle(storage, handle);
    return -1;
  }
  sprintf(query, model_size, context->model);
  if(!(res=PQexec(handle, query)) || !(PQntuples(res))) {
    if (res) {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "postgresql query for model size failed: %s",
                 PQresultErrorMessage(res));
      PQclear(res);
    }
    else {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "postgresql query for model size failed: %s",
                 PQerrorMessage(handle));
    }
    LIBRDF_FREE(char*, query);
    librdf_storage_postgresql_release_handle(storage, handle);
    return -1;
  }
  count = atol(PQgetvalue(res,0,0));
  PQclear(res);
  LIBRDF_FREE(char*, query);
  librdf_storage_postgresql_release_handle(storage, handle);

  return LIBRDF_BAD_CAST(int, count);
}


static int
librdf_storage_postgresql_add_statement(librdf_storage* storage,
                                   librdf_statement* statement)
{
  /* Do not add duplicate statements */
  if(librdf_storage_postgresql_contains_statement(storage, statement))
    return 0;

  return librdf_storage_postgresql_context_add_statement_helper(storage, 0,
                                                           statement);
}


/*
 * librdf_storage_postgresql_add_statements:
 * @storage: the storage
 * @statement_stream: the stream of statements
 *
 * INTERNAL - Add statements in stream to storage, without context.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_postgresql_add_statements(librdf_storage* storage,
                                    librdf_stream* statement_stream)
{
  return librdf_storage_postgresql_context_add_statements(storage, NULL,
                                                     statement_stream);
}

/*
 * librdf_storage_postgresql_node_hash - Create hash value for node
 * @storage: the storage
 * @node: a node to get hash for (and possibly create in database)
 * @add: whether to add the node to the database
 *
 * Return value: Non-zero on succes.
 **/
static u64
librdf_storage_postgresql_node_hash(librdf_storage* storage,
                               librdf_node* node,
                               int add)
{
  librdf_node_type type=librdf_node_get_type(node);
  u64 hash;
  size_t nodelen;
  char *query;
  PGconn *handle;
  PGresult *res;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);

  /* Get postgresql connection handle */
  handle=librdf_storage_postgresql_get_handle(storage);
  if(!handle)
    return 0;

  if(type==LIBRDF_NODE_TYPE_RESOURCE) {
    /* Get hash */
    unsigned char *uri=librdf_uri_as_counted_string(librdf_node_get_uri(node), &nodelen);
    hash = librdf_storage_postgresql_hash(storage, "R", (char*)uri, nodelen);

    if(add) {
      char create_resource[]="INSERT INTO Resources (ID,URI) VALUES (" UINT64_T_FMT ",'%s')";
      int add_status = 0;
      char *escaped_uri;

      escaped_uri = LIBRDF_MALLOC(char*, nodelen * 2 + 1);
      if(escaped_uri) {
        int error = 0;
        PQescapeStringConn(handle, escaped_uri,
                           (const char*)uri, nodelen,
                           &error);
        if(error) {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "postgresql escapeStringConn() failed: %s",
                     PQerrorMessage(handle));
        }

        query = LIBRDF_MALLOC(char*, strlen(create_resource) + 20 + nodelen + 1);
        if(query) {
          sprintf(query, create_resource, hash, escaped_uri);
          if((res=PQexec(handle, query))) {
            if(PQresultStatus(res) == PGRES_COMMAND_OK) {
              add_status = 1;
            } else {
              if (0 == strncmp("23505", PQresultErrorField(res, PG_DIAG_SQLSTATE), strlen("23505"))) {
                /* Don't care about unique key viloations */
                add_status = 1;
              } else {
                librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                           "postgresql insert into Resources failed: %s",
                           PQresultErrorMessage(res));
              }
            }
            PQclear(res);
          } else {
            librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                       "postgresql insert into Resources failed");
          }
          LIBRDF_FREE(char*, query);
        }
        LIBRDF_FREE(char*, escaped_uri);
      }
      if(!add_status) {
        librdf_storage_postgresql_release_handle(storage, handle);
        return 0;
      }
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
    nodestring = LIBRDF_MALLOC(char*, valuelen + langlen + datatypelen + 3);
    if(!nodestring) {
      librdf_storage_postgresql_release_handle(storage, handle);
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
    hash = librdf_storage_postgresql_hash(storage, "L", nodestring, nodelen);
    LIBRDF_FREE(char*, nodestring);

    if(add) {
      char create_literal[]="INSERT INTO Literals (ID,Value,Language,Datatype) VALUES (" UINT64_T_FMT ",'%s','%s','%s')";
      int add_status = 0;
      char *escaped_value, *escaped_lang, *escaped_datatype;

      escaped_value = LIBRDF_MALLOC(char*, valuelen * 2 + 1);
      if(escaped_value) {
        int error = 0;
        PQescapeStringConn(handle, escaped_value,
                           (const char*)value, valuelen,
                           &error);
        if(error) {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "postgresql escapeStringConn() failed: %s",
                     PQerrorMessage(handle));
        }

        escaped_lang = LIBRDF_MALLOC(char*, langlen * 2 + 1);
        if(escaped_lang) {
          if(lang) {
            PQescapeStringConn(handle, escaped_lang,
                               (const char*)lang, langlen,
                               &error);
            if(error) {
              librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                         "postgresql escapeStringConn() failed: %s",
                         PQerrorMessage(handle));
            }
          } else
            strcpy(escaped_lang,"");

          escaped_datatype = LIBRDF_MALLOC(char*, datatypelen * 2 + 1);
          if(escaped_datatype) {
            if(datatype) {
              PQescapeStringConn(handle, escaped_datatype,
                                 (const char*)datatype, datatypelen,
                                 &error);
              if(error) {
                librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                           "postgresql escapeStringConn() failed: %s",
                           PQerrorMessage(handle));
              }
            } else
              strcpy(escaped_datatype,"");

            query = LIBRDF_MALLOC(char*,
                                  strlen(create_literal) +
                                  strlen(escaped_value) +
                                  strlen(escaped_lang) +
                                  strlen(escaped_datatype) + 21);
            if(query) {
              sprintf(query, create_literal, hash, escaped_value, escaped_lang, escaped_datatype);
              if((res=PQexec(handle, query))) {
                if(PQresultStatus(res) == PGRES_COMMAND_OK) {
                  add_status = 1;
                } else {
                  if (0 == strncmp("23505", PQresultErrorField(res, PG_DIAG_SQLSTATE), strlen("23505"))) {
                    /* Don't care about unique key viloations */
                    add_status = 1;
                  } else {
                    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                               "postgresql insert into Resources failed: %s",
                               PQresultErrorMessage(res));
                  }
                }
                PQclear(res);
              } else {
                librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                           "postgresql insert into Resources failed");
              }
              LIBRDF_FREE(char*, query);
            }
            LIBRDF_FREE(char*, escaped_datatype);
          }
          LIBRDF_FREE(char*, escaped_lang);
        }
        LIBRDF_FREE(char*, escaped_value);
      }
      if(!add_status) {
        librdf_storage_postgresql_release_handle(storage, handle);
        return 0;
      }
    }

  } else if(type==LIBRDF_NODE_TYPE_BLANK) {
    /* Get hash */
    unsigned char *name = librdf_node_get_blank_identifier(node);
    nodelen = strlen((const char*)name);
    hash = librdf_storage_postgresql_hash(storage, "B", (char*)name, nodelen);

    if(add) {
      char create_bnode[]="INSERT INTO Bnodes (ID,Name) VALUES (" UINT64_T_FMT ",'%s')";
      int add_status = 0;
      char *escaped_name;

      escaped_name = LIBRDF_MALLOC(char*, nodelen * 2 + 1);
      if(escaped_name) {
        int error = 0;
        PQescapeStringConn(handle, escaped_name,
                           (const char*)name, nodelen,
                           &error);
        if(error) {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "postgresql escapeStringConn() failed: %s",
                     PQerrorMessage(handle));
        }

        query = LIBRDF_MALLOC(char*, strlen(create_bnode) + 20 + nodelen + 1);
        if(query) {
          sprintf(query, create_bnode, hash, escaped_name);
          if((res=PQexec(handle, query))) {
            if(PQresultStatus(res) == PGRES_COMMAND_OK) {
              add_status = 1;
            } else {
              if (0 == strncmp("23505", PQresultErrorField(res, PG_DIAG_SQLSTATE), strlen("23505"))) {
                /* Don't care about unique key viloations */
                add_status = 1;
              } else {
                librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                           "postgresql insert into Resources failed: %s",
                           PQresultErrorMessage(res));
              }
            }
            PQclear(res);
          } else {
            librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                       "postgresql insert into Bnodes failed");
          }
          LIBRDF_FREE(char*, query);
        }
        LIBRDF_FREE(char*, escaped_name);
      }
      if(!add_status) {
        librdf_storage_postgresql_release_handle(storage, handle);
        return 0;
      }
    }
  } else {
    /* Some node type we don't know about? */
    librdf_storage_postgresql_release_handle(storage, handle);
    return 0;
  }

  librdf_storage_postgresql_release_handle(storage, handle);

  return hash;
}


/*
 * librdf_storage_postgresql_start_bulk:
 * @storage: the storage
 *
 * INTERNAL - Prepare for bulk insert operation
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_postgresql_start_bulk(librdf_storage* storage)
{

  return 1;
}


/*
 * librdf_storage_postgresql_stop_bulk:
 * @storage: the storage
 *
 * INTERNAL - End bulk insert operation
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_postgresql_stop_bulk(librdf_storage* storage)
{
  return 1;
}


/*
 * librdf_storage_postgresql_context_add_statements:
 * @storage: the storage
 * @context_node: #librdf_node object
 * @statement_stream: the stream of statements
 *
 * INTERNAL - Add statements in stream to storage, with context.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_postgresql_context_add_statements(librdf_storage* storage,
                                            librdf_node* context_node,
                                            librdf_stream* statement_stream)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  u64 ctxt=0;
  int helper=0;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement_stream, librdf_stream, 1);

  /* Optimize for bulk loads? */
  if(context->bulk) {
    if(librdf_storage_postgresql_start_bulk(storage))
      return 1;
  }

  /* Find hash for context, creating if necessary */
  if(context_node) {
    ctxt=librdf_storage_postgresql_node_hash(storage,context_node,1);
    if(!ctxt)
      return 1;
  }

  while(!helper && !librdf_stream_end(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);
    if(!context->bulk) {
      /* Do not add duplicate statements
       * but do not check for this when in bulk mode.
       */
      if(librdf_storage_postgresql_contains_statement(storage, statement)) {
        librdf_stream_next(statement_stream);
        continue;
      }
    }

    helper=librdf_storage_postgresql_context_add_statement_helper(storage, ctxt,
                                                             statement);
    librdf_stream_next(statement_stream);
  }

  return helper;
}


/*
 * librdf_storage_postgresql_context_add_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to add
 *
 * INTERNAL - Add a statement to a storage context
 *
 * Return value: non 0 on failure
 **/
static int
librdf_storage_postgresql_context_add_statement(librdf_storage* storage,
                                          librdf_node* context_node,
                                          librdf_statement* statement)
{
  u64 ctxt=0;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  /* Find hash for context, creating if necessary */
  if(context_node) {
    ctxt=librdf_storage_postgresql_node_hash(storage,context_node,1);
    if(!ctxt)
      return 1;
  }

  return librdf_storage_postgresql_context_add_statement_helper(storage, ctxt,
                                                           statement);
}


/*
 * librdf_storage_postgresql_context_add_statement_helper
 * @storage: #librdf_storage object
 * @ctxt: u64 context hash
 * @statement: #librdf_statement statement to add
 *
 * INTERNAL - Perform actual addition of a statement to a storage context
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_postgresql_context_add_statement_helper(librdf_storage* storage,
                                          u64 ctxt, librdf_statement* statement)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  char insert_statement[]="INSERT INTO Statements" UINT64_T_FMT " (Subject,Predicate,Object,Context) VALUES (" UINT64_T_FMT "," UINT64_T_FMT "," UINT64_T_FMT "," UINT64_T_FMT ")";
  u64 subject, predicate, object;
  PGconn *handle;
  int status = 1;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  /* Get postgresql connection handle */
  if ((handle=librdf_storage_postgresql_get_handle(storage))) {

    /* Find hashes for nodes, creating if necessary */
    subject=librdf_storage_postgresql_node_hash(storage,
                                           librdf_statement_get_subject(statement),1);
    predicate=librdf_storage_postgresql_node_hash(storage,
                                             librdf_statement_get_predicate(statement),1);
    object=librdf_storage_postgresql_node_hash(storage,
                                          librdf_statement_get_object(statement),1);
    if(subject && predicate && object) {
      char *query;
      PGresult *res;

      query = LIBRDF_MALLOC(char*, strlen(insert_statement) + (20 * 5) + 1);
      if(query) {
        sprintf(query, insert_statement, context->model, subject, predicate, object, ctxt);
        if((res=PQexec(handle, query))) {
          if(PQresultStatus(res) == PGRES_COMMAND_OK) {
            status = 0;
          } else {
            librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                       "postgresql insert into Statements failed: %s",
                       PQresultErrorMessage(res));
          }
          PQclear(res);
        } else {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "postgresql insert into Statements failed: %s",
                     PQerrorMessage(handle));
        }
        LIBRDF_FREE(char*, query);
      }
    }
    librdf_storage_postgresql_release_handle(storage, handle);
  }

  return status;
}


/*
 * librdf_storage_postgresql_contains_statement:
 * @storage: the storage
 * @statement: a complete statement
 *
 * INTERNAL - Test if a given complete statement is present in the model
 *
 * Return value: Non-zero if the model contains the statement.
 **/
static int
librdf_storage_postgresql_contains_statement(librdf_storage* storage,
                                             librdf_statement* statement)
{
  librdf_storage_postgresql_instance* context = (librdf_storage_postgresql_instance*)storage->instance;
  char find_statement[]="SELECT 1 FROM Statements" UINT64_T_FMT " WHERE Subject=" UINT64_T_FMT " AND Predicate=" UINT64_T_FMT " AND Object=" UINT64_T_FMT " limit 1";
  u64 subject, predicate, object;
  PGconn *handle;
  int status = 0;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

  /* Get postgresql connection handle */
  if ((handle=librdf_storage_postgresql_get_handle(storage))) {

    /* Find hashes for nodes */
    subject=librdf_storage_postgresql_node_hash(storage,
                                           librdf_statement_get_subject(statement),0);
    predicate=librdf_storage_postgresql_node_hash(storage,
                                             librdf_statement_get_predicate(statement),0);
    object=librdf_storage_postgresql_node_hash(storage,
                                          librdf_statement_get_object(statement),0);

    if(subject && predicate && object) {
      char *query;
      size_t len = strlen(find_statement)+(20*4)+1;
      query = LIBRDF_MALLOC(char*, len);
      if(query) {
        PGresult *res;
        snprintf(query, len, find_statement, context->model,
                 subject, predicate, object);
        if((res=PQexec(handle, query))) {
          if(PQresultStatus(res) == PGRES_TUPLES_OK) {
            if(PQntuples(res)) {
              status = 1;
            }
          } else {
            librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                       "postgresql insert into Statements failed: %s",
                       PQresultErrorMessage(res));
          }
          PQclear(res);
        }
        LIBRDF_FREE(char*, query);
      }
    }
    librdf_storage_postgresql_release_handle(storage, handle);
  }

  return status;
}


/*
 * librdf_storage_postgresql_remove_statement:
 * @storage: #librdf_storage object
 * @statement: #librdf_statement statement to remove
 *
 * INTERNAL - Remove a statement from storage
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_postgresql_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  return librdf_storage_postgresql_context_remove_statement(storage,NULL,statement);
}


/*
 * librdf_storage_postgresql_context_remove_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to remove
 *
 * INTERNAL - Remove a statement from a storage context
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_postgresql_context_remove_statement(librdf_storage* storage,
                                             librdf_node* context_node,
                                             librdf_statement* statement)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  char delete_statement[]="DELETE FROM Statements" UINT64_T_FMT " WHERE Subject=" UINT64_T_FMT " AND Predicate=" UINT64_T_FMT " AND Object=" UINT64_T_FMT;
  char delete_statement_with_context[]="DELETE FROM Statements" UINT64_T_FMT " WHERE Subject=" UINT64_T_FMT " AND Predicate=" UINT64_T_FMT " AND Object=" UINT64_T_FMT " AND Context=" UINT64_T_FMT;
  u64 subject, predicate, object, ctxt=0;
  PGconn *handle=NULL;
  int status = 1;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  if((handle=librdf_storage_postgresql_get_handle(storage))) {

    /* Find hashes for nodes */
    subject=librdf_storage_postgresql_node_hash(storage,
                                           librdf_statement_get_subject(statement),0);
    predicate=librdf_storage_postgresql_node_hash(storage,
                                             librdf_statement_get_predicate(statement),0);
    object=librdf_storage_postgresql_node_hash(storage,
                                          librdf_statement_get_object(statement),0);

    if (subject && predicate && object) {
      char *query=NULL;
      if(context_node) {
        ctxt=librdf_storage_postgresql_node_hash(storage,context_node,0);
        if(ctxt) {
          query = LIBRDF_MALLOC(char*,
                                strlen(delete_statement_with_context) + (20 * 5) + 1);
  if(query) {
            sprintf(query, delete_statement_with_context, context->model, subject, predicate, object, ctxt);
          }
        }
      } else {
        query = LIBRDF_MALLOC(char*, strlen(delete_statement) + (20 * 4) + 1);
        if(query) {
          sprintf(query, delete_statement, context->model, subject, predicate, object);
        }
      }
      if(query) {
        PGresult *res=NULL;
        if((res=PQexec(handle, query))) {
          if(PQresultStatus(res) == PGRES_COMMAND_OK) {
            status = 0;
          } else {
            librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                       "postgresql delete from Statements failed: %s",
                       PQresultErrorMessage(res));
          }
          PQclear(res);
        } else {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                   "postgresql delete from Statements failed");
        }

        LIBRDF_FREE(char*, query);
      }
    }

    librdf_storage_postgresql_release_handle(storage, handle);
  }

  return status;
}


/*
 * librdf_storage_postgresql_context_remove_statements:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * INTERNAL - Remove all statement from a storage context
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_postgresql_context_remove_statements(librdf_storage* storage,
                                               librdf_node* context_node)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  char delete_context[]="DELETE FROM Statements" UINT64_T_FMT " WHERE Context=" UINT64_T_FMT;
  char delete_model[]="DELETE FROM Statements" UINT64_T_FMT;
  u64 ctxt=0;
  PGconn *handle=NULL;
  int status = 1;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  if((handle=librdf_storage_postgresql_get_handle(storage))) {
    char *query=NULL;
    if(context_node) {
      ctxt=librdf_storage_postgresql_node_hash(storage,context_node,0);
      if(ctxt) {
        query = LIBRDF_MALLOC(char*, strlen(delete_context) + (20 * 2) + 1);
        if(query) {
          sprintf(query, delete_context, context->model, ctxt);
        }
      }
    } else {
      query = LIBRDF_MALLOC(char*, strlen(delete_model) + (20) + 1);
      if(query) {
        sprintf(query, delete_model, context->model);
      }
    }
    if(query) {
      PGresult *res=NULL;
      if((res=PQexec(handle, query))) {
        if(PQresultStatus(res) == PGRES_COMMAND_OK) {
          status = 0;
        } else {
          librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "postgresql delete from Statements failed: %s",
                     PQresultErrorMessage(res));
        }
        PQclear(res);
      } else {
        librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "postgresql delete from Statements failed");
      }

      LIBRDF_FREE(char*, query);
    }

    librdf_storage_postgresql_release_handle(storage, handle);
  }

  return status;
}


/*
 * librdf_storage_postgresql_serialise:
 * @storage: the storage
 *
 * INTERNAL - Return a stream of all statements in a storage.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
librdf_stream* librdf_storage_postgresql_serialise(librdf_storage* storage)
{
  return librdf_storage_postgresql_find_statements_in_context(storage,NULL,NULL);
}


/*
 * librdf_storage_postgresql_find_statements:
 * @storage: the storage
 * @statement: the statement to match
 *
 * INTERNAL - Find a graph of statements in storage.
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_postgresql_find_statements(librdf_storage* storage,
                                     librdf_statement* statement)
{
  return librdf_storage_postgresql_find_statements_in_context(storage,statement,NULL);
}


/*
 * librdf_storage_postgresql_context_serialise:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * INTERNAL - List all statements in a storage context
 *
 * Return value: #librdf_stream of statements or NULL on failure or context is empty
 **/
static librdf_stream*
librdf_storage_postgresql_context_serialise(librdf_storage* storage,
                                       librdf_node* context_node)
{
  return librdf_storage_postgresql_find_statements_in_context(storage,NULL,context_node);
}


/*
 * librdf_storage_postgresql_find_statements_in_context:
 * @storage: the storage
 * @statement: the statement to match
 * @context_node: the context to search
 *
 * INTERNAL - Find a graph of statements in a storage context.
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_postgresql_find_statements_in_context(librdf_storage* storage, librdf_statement* statement,librdf_node* context_node)
{
  return librdf_storage_postgresql_find_statements_with_options(storage, statement, context_node, NULL);
}


/*
 * librdf_storage_postgresql_find_statements_with_options:
 * @storage: the storage
 * @statement: the statement to match
 * @context_node: the context to search
 * @options: #librdf_hash of match options or NULL
 *
 * INTERNAL - Find a graph of statements in a storage context with options.
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_postgresql_find_statements_with_options(librdf_storage* storage,
                                                  librdf_statement* statement,
                                                  librdf_node* context_node,
                                                  librdf_hash* options)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  librdf_storage_postgresql_sos_context* sos;
  librdf_node *subject=NULL, *predicate=NULL, *object=NULL;
  char *query;
  char tmp[64];
  char where[256];
  char joins[640];
  librdf_stream *stream;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  /* Initialize sos context */
  sos = LIBRDF_CALLOC(librdf_storage_postgresql_sos_context*, 1, sizeof(*sos));
  if(!sos)
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

  /* Get postgresql connection handle */
  sos->handle=librdf_storage_postgresql_get_handle(storage);
  if(!sos->handle) {
    librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }

  /* Construct query */
  query = LIBRDF_MALLOC(char*, 21);
  if(!query) {
    librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }
  strcpy(query, "SELECT ");
  *where='\0';
  if(sos->is_literal_match)
    sprintf(joins, " FROM Literals AS L LEFT JOIN Statements" UINT64_T_FMT " as S ON L.ID=S.Object",
            context->model);
  else
    sprintf(joins, " FROM Statements" UINT64_T_FMT " AS S", context->model);

  if(statement) {
    subject=librdf_statement_get_subject(statement);
    predicate=librdf_statement_get_predicate(statement);
    object=librdf_statement_get_object(statement);
  }

  /* Subject */
  if(statement && subject) {
    sprintf(tmp, "S.Subject=" UINT64_T_FMT "",
            librdf_storage_postgresql_node_hash(storage,subject,0));
    if(!strlen(where))
      strcat(where, " WHERE ");
    else
      strcat(where, " AND ");
    strcat(where, tmp);
  } else {
    if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, " SubjectR.URI AS SuR, SubjectB.Name AS SuB")) {
      librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
    strcat(joins," LEFT JOIN Resources AS SubjectR ON S.Subject=SubjectR.ID");
    strcat(joins," LEFT JOIN Bnodes AS SubjectB ON S.Subject=SubjectB.ID");
  }

  /* Predicate */
  if(statement && predicate) {
    sprintf(tmp, "S.Predicate=" UINT64_T_FMT "",
            librdf_storage_postgresql_node_hash(storage, predicate, 0));
    if(!strlen(where))
      strcat(where, " WHERE ");
    else
      strcat(where, " AND ");
    strcat(where, tmp);
  } else {
    if(!statement || !subject) {
      if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, ",")) {
        librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
        return NULL;
      }
    }
    if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, " PredicateR.URI AS PrR")) {
      librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
    strcat(joins," LEFT JOIN Resources AS PredicateR ON S.Predicate=PredicateR.ID");
  }

  /* Object */
  if(statement && object) {
    if(!sos->is_literal_match) {
      sprintf(tmp,"S.Object=" UINT64_T_FMT "",
              librdf_storage_postgresql_node_hash(storage, object, 0));
      if(!strlen(where))
        strcat(where, " WHERE ");
      else
        strcat(where, " AND ");
      strcat(where, tmp);
    } else {
      /* MATCH literal, not hash_id */
      if(!statement || !subject || !predicate) {
        if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, ",")) {
          librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
          return NULL;
        }
      }
      if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, " ObjectR.URI AS ObR, ObjectB.Name AS ObB, ObjectL.Value AS ObV, ObjectL.Language AS ObL, ObjectL.Datatype AS ObD")) {
        librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
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
      if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, ",")) {
        librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
        return NULL;
      }
    }
    if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, " ObjectR.URI AS ObR, ObjectB.Name AS ObB, ObjectL.Value AS ObV, ObjectL.Language AS ObL, ObjectL.Datatype AS ObD")) {
      librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
    strcat(joins," LEFT JOIN Resources AS ObjectR ON S.Object=ObjectR.ID");
    strcat(joins," LEFT JOIN Bnodes AS ObjectB ON S.Object=ObjectB.ID");
    strcat(joins," LEFT JOIN Literals AS ObjectL ON S.Object=ObjectL.ID");
  }

  /* Context */
  if(context_node) {
    sprintf(tmp,"S.Context=" UINT64_T_FMT "",
            librdf_storage_postgresql_node_hash(storage,context_node,0));
    if(!strlen(where))
      strcat(where, " WHERE ");
    else
      strcat(where, " AND ");
    strcat(where, tmp);
  } else {
    if(!statement || !subject || !predicate || !object) {
      if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, ",")) {
        librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
        return NULL;
      }
    }

    if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, " ContextR.URI AS CoR, ContextB.Name AS CoB, ContextL.Value AS CoV, ContextL.Language AS CoL, ContextL.Datatype AS CoD")) {
      librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
    strcat(joins," LEFT JOIN Resources AS ContextR ON S.Context=ContextR.ID");
    strcat(joins," LEFT JOIN Bnodes AS ContextB ON S.Context=ContextB.ID");
    strcat(joins," LEFT JOIN Literals AS ContextL ON S.Context=ContextL.ID");
  }

  /* Query without variables? */
  if(statement && subject && predicate && object && context_node) {
    if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, " 1")) {
      librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
  }

  /* Complete query string */
  if(librdf_storage_postgresql_find_statements_in_context_augment_query(&query, joins) ||
      librdf_storage_postgresql_find_statements_in_context_augment_query(&query, where)) {
    librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }


  /* Start query... */
  sos->results=PQexec(sos->handle, query);
  LIBRDF_FREE(char*, query);
  if (sos->results) {
    if (PQresultStatus(sos->results) != PGRES_TUPLES_OK) {
      librdf_log(sos->storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "postgresql query failed: %s",
                 PQresultErrorMessage(sos->results));
      librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
      return NULL;
    }
  } else {
    librdf_log(sos->storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql query failed: %s",
               PQerrorMessage(sos->handle));
    librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }

  sos->current_rowno=0;
  sos->row = LIBRDF_CALLOC(char**, LIBRDF_GOOD_CAST(size_t, PQnfields(sos->results) + 1), sizeof(char*));
  if(!sos->row) {
    librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }

  /* Get first statement, if any, and initialize stream */
  if(librdf_storage_postgresql_find_statements_in_context_next_statement(sos) ) {
    librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
    return librdf_new_empty_stream(storage->world);
  }

  stream=librdf_new_stream(storage->world,(void*)sos,
                           &librdf_storage_postgresql_find_statements_in_context_end_of_stream,
                           &librdf_storage_postgresql_find_statements_in_context_next_statement,
                           &librdf_storage_postgresql_find_statements_in_context_get_statement,
                           &librdf_storage_postgresql_find_statements_in_context_finished);
  if(!stream) {
    librdf_storage_postgresql_find_statements_in_context_finished((void*)sos);
    return NULL;
  }

  return stream;
}


static int
librdf_storage_postgresql_find_statements_in_context_augment_query(char **query, const char *addition)
{
  char *newquery;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, char, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(addition, char, 1);

  /* Augment existing query, returning 0 on success. */
  newquery = LIBRDF_MALLOC(char*, strlen(*query) + strlen(addition) + 1);
  if(!newquery)
      return 1;
  strcpy(newquery,*query);
  strcat(newquery,addition);
  LIBRDF_FREE(char*, *query);
  *query=newquery;

  return 0;
}


static int
librdf_storage_postgresql_find_statements_in_context_end_of_stream(void* context)
{
  librdf_storage_postgresql_sos_context* sos=(librdf_storage_postgresql_sos_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, 1);

  return sos->current_statement==NULL;
}


static int
librdf_storage_postgresql_find_statements_in_context_next_statement(void* context)
{
  librdf_storage_postgresql_sos_context* sos=(librdf_storage_postgresql_sos_context*)context;
  librdf_node *subject=NULL, *predicate=NULL, *object=NULL;
  librdf_node *node;
  char **row=sos->row;
  int i;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, 1);

  if( sos->current_rowno < PQntuples(sos->results) ) {
     for(i=0;i<PQnfields(sos->results);i++) {
       if(PQgetlength(sos->results,sos->current_rowno,i) > 0 ) {
         /* FIXME: why is this not copied? */
             /*
           row[i] = LIBRDF_MALLOC(char*, PQgetlength(sos->results,sos->current_rowno, i) + 1)
           if(!row[i])
              return 1;
           strcpy(row[i], PQgetvalue(sos->results,sos->current_rowno, i));
             */
           row[i]=PQgetvalue(sos->results,sos->current_rowno,i);

        }
      else
           row[i]=NULL;
     }
    sos->current_rowno=sos->current_rowno+1;

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
librdf_storage_postgresql_find_statements_in_context_get_statement(void* context, int flags)
{
  librdf_storage_postgresql_sos_context* sos=(librdf_storage_postgresql_sos_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, NULL);

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
librdf_storage_postgresql_find_statements_in_context_finished(void* context)
{
  librdf_storage_postgresql_sos_context* sos=(librdf_storage_postgresql_sos_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(context, void);

  if( sos->row )
     LIBRDF_FREE(char*, sos->row);


  if(sos->results)
    PQclear(sos->results);

  if(sos->handle)
    librdf_storage_postgresql_release_handle(sos->storage, sos->handle);

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

  LIBRDF_FREE(librdf_storage_postgresql_sos_context, sos);

}


/*
 * librdf_storage_postgresql_get_contexts:
 * @storage: the storage
 *
 * INTERNAL - Return an iterator with the context nodes present in storage.
 *
 * Return value: a #librdf_iterator or NULL on failure
 **/
static librdf_iterator*
librdf_storage_postgresql_get_contexts(librdf_storage* storage)
{
  librdf_storage_postgresql_instance* context=(librdf_storage_postgresql_instance*)storage->instance;
  librdf_storage_postgresql_get_contexts_context* gccontext;
  const char select_contexts[]="\
SELECT DISTINCT R.URI AS CoR, B.Name AS CoB, \
L.Value AS CoV, L.Language AS CoL, L.Datatype AS CoD \
FROM Statements" UINT64_T_FMT " as S \
LEFT JOIN Resources AS R ON S.Context=R.ID \
LEFT JOIN Bnodes AS B ON S.Context=B.ID \
LEFT JOIN Literals AS L ON S.Context=L.ID";
  char *query;
  librdf_iterator *iterator;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  /* Initialize get_contexts context */
  gccontext = LIBRDF_CALLOC(librdf_storage_postgresql_get_contexts_context*, 1,
                            sizeof(*gccontext));
  if(!gccontext)
    return NULL;
  gccontext->storage=storage;
  librdf_storage_add_reference(gccontext->storage);

  gccontext->current_context=NULL;
  gccontext->results=NULL;

  /* Get postgresql connection handle */
  gccontext->handle=librdf_storage_postgresql_get_handle(storage);
  if(!gccontext->handle) {
    librdf_storage_postgresql_get_contexts_finished((void*)gccontext);
    return NULL;
  }

  /* Construct query */
  query = LIBRDF_MALLOC(char*, strlen(select_contexts) + 21);
  if(!query) {
    librdf_storage_postgresql_get_contexts_finished((void*)gccontext);
    return NULL;
  }
  sprintf(query, select_contexts, context->model);

  /* Start query... */
  gccontext->results=PQexec(gccontext->handle, query);
  LIBRDF_FREE(char*, query);
  if (gccontext->results) {
    if (PQresultStatus(gccontext->results) != PGRES_TUPLES_OK) {
      librdf_log(gccontext->storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "postgresql query failed: %s",
                 PQresultErrorMessage(gccontext->results));
      librdf_storage_postgresql_get_contexts_finished((void*)gccontext);
      return NULL;
    }
  } else {
    librdf_log(gccontext->storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql query failed: %s",
               PQerrorMessage(gccontext->handle));
    librdf_storage_postgresql_get_contexts_finished((void*)gccontext);
    return NULL;
  }

  gccontext->current_rowno=0;
  gccontext->row = LIBRDF_CALLOC(char**, LIBRDF_GOOD_CAST(size_t, PQnfields(gccontext->results) + 1), sizeof(char*));
  if(!gccontext->row) {
    librdf_storage_postgresql_get_contexts_finished((void*)gccontext);
    return NULL;
  }

  /* Get first context, if any, and initialize iterator */
  if(librdf_storage_postgresql_get_contexts_next_context(gccontext) ||
      !gccontext->current_context) {
    librdf_storage_postgresql_get_contexts_finished((void*)gccontext);
    return librdf_new_empty_iterator(storage->world);
  }

  iterator=librdf_new_iterator(storage->world,(void*)gccontext,
                               &librdf_storage_postgresql_get_contexts_end_of_iterator,
                               &librdf_storage_postgresql_get_contexts_next_context,
                               &librdf_storage_postgresql_get_contexts_get_context,
                               &librdf_storage_postgresql_get_contexts_finished);
  if(!iterator)
    librdf_storage_postgresql_get_contexts_finished(gccontext);
  return iterator;
}


static int
librdf_storage_postgresql_get_contexts_end_of_iterator(void* context)
{
  librdf_storage_postgresql_get_contexts_context* gccontext=(librdf_storage_postgresql_get_contexts_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, 1);

  return gccontext->current_context==NULL;
}


static int
librdf_storage_postgresql_get_contexts_next_context(void* context)
{
  librdf_storage_postgresql_get_contexts_context* gccontext=(librdf_storage_postgresql_get_contexts_context*)context;
  librdf_node *node;
  char **row=gccontext->row;
  int i;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, 1);

  if( gccontext->current_rowno < PQntuples(gccontext->results) ) {
     for(i=0;i<PQnfields(gccontext->results);i++) {
       if(PQgetlength(gccontext->results,gccontext->current_rowno,i) > 0 ) {
         /* FIXME: why is this not copied? */
             /*
           row[i] = LIBRDF_MALLOC(char*, PQgetlength(gccontext->results, gccontext->current_rowno, i) + 1)
           if(!row[i)
              return 1;
           strcpy(row[i], PQgetvalue(gccontext->results, gccontext->current_rowno,i));
             */
           row[i]=PQgetvalue(gccontext->results,gccontext->current_rowno,i);
        }
      else
           row[i]=NULL;
     }
    gccontext->current_rowno=gccontext->current_rowno+1;

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
librdf_storage_postgresql_get_contexts_get_context(void* context, int flags)
{
  librdf_storage_postgresql_get_contexts_context* gccontext=(librdf_storage_postgresql_get_contexts_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, NULL);

  return gccontext->current_context;
}


#if 0
/* FIXME: why is this not used ? */
static void
librdf_storage_postgresql_free_gccontext_row(void* context)
{
  librdf_storage_postgresql_get_contexts_context* gccontext=(librdf_storage_postgresql_get_contexts_context*)context;

/*
  for(i=0;i<PQnfields(gccontext->results);i++)
     if( gccontext->row[i] )
         LIBRDF_FREE(char*, gccontext->row[i]);
*/
}
#endif


static void
librdf_storage_postgresql_get_contexts_finished(void* context)
{
  librdf_storage_postgresql_get_contexts_context* gccontext=(librdf_storage_postgresql_get_contexts_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(context, void);

  if( gccontext->row )
      LIBRDF_FREE(char*, gccontext->row);

  if(gccontext->results)
    PQclear(gccontext->results);

  if(gccontext->handle)
    librdf_storage_postgresql_release_handle(gccontext->storage, gccontext->handle);

  if(gccontext->current_context)
    librdf_free_node(gccontext->current_context);

  if(gccontext->storage)
    librdf_storage_remove_reference(gccontext->storage);

  LIBRDF_FREE(librdf_storage_postgresql_get_contexts_context, gccontext);

}


/*
 * librdf_storage_postgresql_get_feature:
 * @storage: #librdf_storage object
 * @feature: #librdf_uri feature property
 *
 * INTERNAL - get the value of a storage feature
 *
 * Return value: #librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
static librdf_node*
librdf_storage_postgresql_get_feature(librdf_storage* storage, librdf_uri* feature)
{
  unsigned char *uri_string;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, NULL);

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




/*
 * librdf_storage_postgresql_transaction_start:
 * @storage: the storage object
 *
 * INTERNAL - Start a transaction
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_postgresql_transaction_start(librdf_storage* storage)
{
  librdf_storage_postgresql_instance *context=(librdf_storage_postgresql_instance*)storage->instance;
  const char query[]="START TRANSACTION";
  int status = 1;
  PGresult *res;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  if(context->transaction_handle) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql transaction already started");
    return status;
  }

  context->transaction_handle=librdf_storage_postgresql_get_handle(storage);
  if(!context->transaction_handle) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Failed to establish transaction handle");
    return status;
  }

  res = PQexec(context->transaction_handle, query);
  if (res) {
    if (PQresultStatus(res) == PGRES_COMMAND_OK) {
      status = 0;
    } else {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "postgresql query failed: %s",
                 PQresultErrorMessage(res));
    }
    PQclear(res);
  } else {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql query failed: %s",
               PQerrorMessage(context->transaction_handle));
  }

  if (0 != status) {
    librdf_storage_postgresql_release_handle(storage, context->transaction_handle);
    context->transaction_handle=NULL;
  }

  return status;
}


/*
 * librdf_storage_postgresql_transaction_start_with_handle:
 * @storage: the storage object
 * @handle: the transaction object
 *
 * INTERNAL - Start a transaction using an existing external transaction object.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_postgresql_transaction_start_with_handle(librdf_storage* storage,
                                                   void* handle)
{
  return librdf_storage_postgresql_transaction_start(storage);
}


/*
 * librdf_storage_postgresql_transaction_commit:
 * @storage: the storage object
 *
 * INTERNAL - Commit a transaction.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_postgresql_transaction_commit(librdf_storage* storage)
{
  librdf_storage_postgresql_instance *context=(librdf_storage_postgresql_instance*)storage->instance;
  const char query[]="COMMIT TRANSACTION";
  int status = 1;
  PGresult *res;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  if(!context->transaction_handle)
    return status;

  res = PQexec(context->transaction_handle, query);
  if (res) {
    if (PQresultStatus(res) == PGRES_COMMAND_OK) {
      status = 0;
    } else {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "postgresql commit query failed: %s",
                 PQresultErrorMessage(res));
    }
    PQclear(res);
  } else {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql commit query failed: %s",
               PQerrorMessage(context->transaction_handle));
  }

  librdf_storage_postgresql_release_handle(storage, context->transaction_handle);
  context->transaction_handle=NULL;

  return status;
}


/*
 * librdf_storage_postgresql_transaction_rollback:
 * @storage: the storage object
 *
 * INTERNAL - Rollback a transaction.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_postgresql_transaction_rollback(librdf_storage* storage)
{
  librdf_storage_postgresql_instance *context=(librdf_storage_postgresql_instance*)storage->instance;
  const char query[]="ROLLBACK TRANSACTION";
  int status = 1;
  PGresult *res;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  if(!context->transaction_handle)
    return status;

  res = PQexec(context->transaction_handle, query);
  if (res) {
    if (PQresultStatus(res) == PGRES_COMMAND_OK) {
      status = 0;
    } else {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "postgresql commit query failed: %s",
                 PQresultErrorMessage(res));
    }
    PQclear(res);
  } else {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "postgresql commit query failed: %s",
               PQerrorMessage(context->transaction_handle));
  }

  librdf_storage_postgresql_release_handle(storage, context->transaction_handle);
  context->transaction_handle=NULL;

  return status;
}


/*
 * librdf_storage_postgresql_transaction_get_handle:
 * @storage: the storage object
 *
 * INTERNAL - Get the current transaction handle.
 *
 * Return value: non-0 on success
 **/
static void*
librdf_storage_postgresql_transaction_get_handle(librdf_storage* storage)
{
  librdf_storage_postgresql_instance *context=(librdf_storage_postgresql_instance*)storage->instance;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  return context->transaction_handle;
}


/* local function to register postgresql storage functions */
static void
librdf_storage_postgresql_register_factory(librdf_storage_factory *factory)
{
  LIBRDF_ASSERT_CONDITION(!strcmp(factory->name, "postgresql"));

  factory->version            = LIBRDF_STORAGE_INTERFACE_VERSION;
  factory->init               = librdf_storage_postgresql_init;
  factory->terminate          = librdf_storage_postgresql_terminate;
  factory->open               = librdf_storage_postgresql_open;
  factory->close              = librdf_storage_postgresql_close;
  factory->sync               = librdf_storage_postgresql_sync;
  factory->size               = librdf_storage_postgresql_size;
  factory->add_statement      = librdf_storage_postgresql_add_statement;
  factory->add_statements     = librdf_storage_postgresql_add_statements;
  factory->remove_statement   = librdf_storage_postgresql_remove_statement;
  factory->contains_statement = librdf_storage_postgresql_contains_statement;
  factory->serialise          = librdf_storage_postgresql_serialise;
  factory->find_statements    = librdf_storage_postgresql_find_statements;
  factory->find_statements_with_options    = librdf_storage_postgresql_find_statements_with_options;
  factory->context_add_statement      = librdf_storage_postgresql_context_add_statement;
  factory->context_add_statements     = librdf_storage_postgresql_context_add_statements;
  factory->context_remove_statement   = librdf_storage_postgresql_context_remove_statement;
  factory->context_remove_statements  = librdf_storage_postgresql_context_remove_statements;
  factory->context_serialise          = librdf_storage_postgresql_context_serialise;
  factory->find_statements_in_context = librdf_storage_postgresql_find_statements_in_context;
  factory->get_contexts               = librdf_storage_postgresql_get_contexts;
  factory->get_feature                = librdf_storage_postgresql_get_feature;

  factory->transaction_start             = librdf_storage_postgresql_transaction_start;
  factory->transaction_start_with_handle = librdf_storage_postgresql_transaction_start_with_handle;
  factory->transaction_commit            = librdf_storage_postgresql_transaction_commit;
  factory->transaction_rollback          = librdf_storage_postgresql_transaction_rollback;
  factory->transaction_get_handle        = librdf_storage_postgresql_transaction_get_handle;
}

#ifdef MODULAR_LIBRDF

/* Entry point for dynamically loaded storage module */
void
librdf_storage_module_register_factory(librdf_world *world)
{
  librdf_storage_register_factory(world, "postgresql",
                                  "PostgreSQL database store",
                                  &librdf_storage_postgresql_register_factory);
}

#else

/*
 * librdf_init_storage_postgresql:
 * @world: world object
 *
 * INTERNAL - Initialise the built-in storage_postgresql module.
 */
void
librdf_init_storage_postgresql(librdf_world *world)
{
  librdf_storage_register_factory(world, "postgresql", 
                                  "PostgreSQL database store",
                                  &librdf_storage_postgresql_register_factory);
}

#endif


