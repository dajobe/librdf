/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_mysql.c - RDF Storage in MySQL DB interface definition.
 *
 * $Id$
 *
 * Based on rdf_storage_list and rdf_storage_parka.
 *
 * Copyright (C) 2003 Morten Frederiksen - http://purl.org/net/morten/
 *
 * and
 *
 * Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.org/
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

#include <rdf_config.h>

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif
#include <sys/types.h>

#include <librdf.h>

#include <mysql.h>


typedef struct
{
  /* MySQL connection parameters; excluding password */
  const char *host;
  const char *user;
  const char *database;
  int port;

  /* non zero when MYSQL connection is open */
  int connection_open;
  MYSQL connection;

  /* model name in the database (table Models, column Name) */
  char *name;

  /* model ID in the database (table Models, column ID) */
  int id;

} librdf_storage_mysql_context;

/* prototypes for local functions */
static int librdf_storage_mysql_init(librdf_storage* storage, char *name, librdf_hash* options);
static void librdf_storage_mysql_terminate(librdf_storage* storage);
static int librdf_storage_mysql_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_mysql_close(librdf_storage* storage);
static int librdf_storage_mysql_size(librdf_storage* storage);
static int librdf_storage_mysql_add_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_mysql_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_mysql_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_mysql_contains_statement(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_mysql_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_mysql_find_statements(librdf_storage* storage, librdf_statement* statement);

/* context functions */
static int librdf_storage_mysql_context_add_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_mysql_context_add_statements(librdf_storage* storage, librdf_node* context_node, librdf_stream* statement_stream);
static int librdf_storage_mysql_context_remove_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_mysql_context_remove_statements(librdf_storage* storage, librdf_node* context_node);
static librdf_stream* librdf_storage_mysql_context_serialise(librdf_storage* storage, librdf_node* context_node);

/* context list statement stream methods */
static int librdf_storage_mysql_context_serialise_end_of_stream(void* context);
static int librdf_storage_mysql_context_serialise_next_statement(void* context);
static void* librdf_storage_mysql_context_serialise_get_statement(void* context, int flags);
static void librdf_storage_mysql_context_serialise_finished(void* context);

static void librdf_storage_mysql_register_factory(librdf_storage_factory *factory);

/* "private" functions */
static char* librdf_storage_mysql_node_hash(librdf_storage* storage,librdf_node* node,int add);
static int librdf_storage_mysql_query_add_node_clause(librdf_storage* storage,librdf_node* node,char* query,char* name);
static int librdf_storage_mysql_context_add_statement_helper(librdf_storage* storage, char *ctxt,librdf_statement* statement);

typedef struct {
  char query[920]; /* FIXME */
  unsigned int *stmts;
  int count;
  int one_context;
  librdf_storage *storage;
  librdf_statement *current;
  librdf_node *current_context_node;
} librdf_storage_mysql_context_serialise_stream_context;


/* functions implementing storage api */

/**
 * librdf_storage_mysql_init:
 * @storage: the storage
 * @name: model name
 * @options: host, port, database, user, password.
 *
 * Create connection to database.
 *
 * Return value: Non-zero on failure (negative values are MySQL errors).
 **/
static int
librdf_storage_mysql_init(librdf_storage* storage, char *name,
                          librdf_hash* options)
{
  librdf_storage_mysql_context *context=(librdf_storage_mysql_context*)storage->context;
  char *password=NULL;
  int status=0;
  
  if(!options)
    return 1;

  /* Save model name */
  context->name=(char*)LIBRDF_MALLOC(cstring,strlen(name)+1);
  if(!context->name)
    return 1;
  strcpy(context->name,name);
  
  context->host=librdf_hash_get_del(options, "host");
  context->database=librdf_hash_get_del(options, "database");
  context->user=librdf_hash_get_del(options, "user");
  context->port=librdf_hash_get_as_long(options, "port");

  /* NOTE: not saved */
  password=librdf_hash_get_del(options, "password");

  if(!context->host || !context->database || !context->user || !context->port
     || !password)
    return 1;

  /* Initialize MySQL connection */
  mysql_init(&context->connection);
  
  /* Create connection to database */
  if(!mysql_real_connect(&context->connection,
                         context->host, context->user, password,
                         context->database, context->port, NULL,0)) {
    librdf_error(storage->world, "Connection to MySQL database %s:%d name %s as user %s  failed with error %d - %s",context->host,context->port, context->database, context->user, mysql_errno(&context->connection), mysql_error(&context->connection));
    status=0-mysql_errno(&context->connection);
    goto cleanup;
  };
  
  context->connection_open=1;

 cleanup:
  
  /* Unused options: new, write */
  librdf_free_hash(options);

  /* Never stored */
  if(password)
    LIBRDF_FREE(cstring,password);

  if(status)
    librdf_storage_mysql_terminate(storage);
  
  return status;
}

/**
 * librdf_storage_mysql_terminate:
 * @storage: the storage
 *
 * Close the storage and database connection.
 *
 * Return value: None.
 **/
static void
librdf_storage_mysql_terminate(librdf_storage* storage)
{
  librdf_storage_mysql_context *context=(librdf_storage_mysql_context*)storage->context;
  
  if(context->connection_open)
    mysql_close(&context->connection);
  
  if(context->name)
    LIBRDF_FREE(cstring,context->name);

  if(context->host)
    LIBRDF_FREE(cstring,(char*)context->host);
  
  if(context->database)
    LIBRDF_FREE(cstring,(char*)context->database);
  
  if(context->user)
    LIBRDF_FREE(cstring,(char*)context->user);
  
}

/**
 * librdf_storage_mysql_open:
 * @storage: the storage
 * @model: the model
 *
 * Create or open model in database.
 *
 * Return value: Non-zero on failure (negative values are MySQL errors).
 **/
static int
librdf_storage_mysql_open(librdf_storage* storage, librdf_model* model)
{
  librdf_storage_mysql_context *context=(librdf_storage_mysql_context*)storage->context;
  char *query;
  MYSQL_RES *res;
  MYSQL_ROW row;

  /* Create model if not existing */
  query=(char*)LIBRDF_MALLOC(cstring,strlen(context->name)+36);
  if(!query)
    return 1;

  sprintf(query, "select ID from Models where Name='%s'",context->name);
  if(mysql_real_query(&context->connection,query,strlen(query)) ||
     !(res=mysql_store_result(&context->connection))) {
    LIBRDF_ERROR3(storage->world, librdf_storage_mysql_open,
                  "Query for model ID %s failed with error %s", 
                  context->name, mysql_error(&context->connection));
    return 0-mysql_errno(&context->connection);
  };

  if(!mysql_num_rows(res)) {
    LIBRDF_FREE(cstring,query);
    query=(char*)LIBRDF_MALLOC(cstring,strlen(context->name)+38);
    if(!query)
      return 1;

    sprintf(query,"insert into Models (Name) values ('%s')",context->name);
    if(mysql_real_query(&context->connection,query,strlen(query))) {
      LIBRDF_ERROR2(storage->world, librdf_storage_mysql_open,
                    "Insert into Models table failed with error %s",
                    mysql_error(&context->connection));
      return 0-mysql_errno(&context->connection);
    };
    context->id=mysql_insert_id(&context->connection);
  } else {
    if(!(row=mysql_fetch_row(res))) {
      LIBRDF_ERROR2(storage->world, librdf_storage_mysql_open,
                    "Fetch of model ID failed with error %s",
                    mysql_error(&context->connection));
      return 0-mysql_errno(&context->connection);
    };
    context->id=atol(row[0]);
  };

  mysql_free_result(res);
  LIBRDF_FREE(cstring,query);

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
  char *query;
  MYSQL_RES *res;
  MYSQL_ROW row;
  int count;
  
  /* Query for number of statements */
  query=(char*)LIBRDF_MALLOC(cstring,80);
  if(!query)
    return -1;

  sprintf(query, "select count(*) from Statements where Model=%u", 
          context->id);
  if(mysql_real_query(&context->connection,query,strlen(query)) ||
     !(res=mysql_store_result(&context->connection)) ||
     !(row=mysql_fetch_row(res))) {
    LIBRDF_ERROR2(storage->world, librdf_storage_mysql_size,
                  "Query for model size failed with error %s",
                  mysql_error(&context->connection));
    return 0-mysql_errno(&context->connection);
  };

  count=atol(row[0]);
  mysql_free_result(res);
  LIBRDF_FREE(cstring,query);
  return count;
}


static int
librdf_storage_mysql_add_statement(librdf_storage* storage, 
                                   librdf_statement* statement)
{
  return librdf_storage_mysql_context_add_statement_helper(storage, NULL,
                                                           statement);
}


/**
 * librdf_storage_mysql_add_statements:
 * @storage: the storage
 * @stream: the stream of statements
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
  
  while(!librdf_stream_end(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);
    helper=librdf_storage_mysql_context_add_statement_helper(storage,NULL,statement);
    if(helper)
      return helper;

    librdf_stream_next(statement_stream);
  };
  
  return helper;
}


static char *
librdf_storage_mysql_node_hash(librdf_storage* storage,
                               librdf_node* node,
                               int add)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  librdf_node_type type=librdf_node_get_type(node);
  librdf_digest* digest;
  char *id;
  unsigned long long int *id1;
  unsigned long long int *id2;
  size_t nodelen;
  char *query;
  
  /* Prepare digest */
  if(!(digest=librdf_new_digest(storage->world,"MD5")))
    return 0;

  librdf_digest_init(digest);

  if(!(id=(char*)LIBRDF_MALLOC(cstring,16)))
    return 0;

  id1=(unsigned long long int*)id;
  id2=(unsigned long long int*)&id[8];
  
  if(type==LIBRDF_NODE_TYPE_RESOURCE) {
    unsigned char *uri=librdf_uri_as_counted_string(librdf_node_get_uri(node), &nodelen);
    
    /* Create digest */
    librdf_digest_update(digest, (unsigned char*)"R", 1);
    librdf_digest_update(digest, uri, nodelen);
    librdf_digest_final(digest);
    memcpy(id,librdf_digest_get_digest(digest),16);
    
    if(add) {
      /* Escape URI for db query */
      char *escuri=(char*)LIBRDF_MALLOC(cstring, nodelen*2+1);
      if(!escuri)
        return 0;

      mysql_real_escape_string(&context->connection, escuri, (const char*)uri, nodelen);
      nodelen=strlen(escuri);
      
      /* Create new resource, ignore if existing */
      if(!(query=(char*)LIBRDF_MALLOC(cstring, nodelen+110)))
        return 0;

      sprintf(query,
              "insert into Resources (ID1,ID2,URI) values (%llu,%llu,'%s')",
              *id1,*id2,escuri);
      if(mysql_real_query(&context->connection,query,strlen(query)) &&
         mysql_errno(&context->connection) != 1062) {
        LIBRDF_ERROR2(storage->world, librdf_storage_mysql_node_hash,
                      "Insert into Resources failed with error %s",
                      mysql_error(&context->connection));
        return 0;
      }
      LIBRDF_FREE(cstring,query);
      LIBRDF_FREE(cstring,escuri);
    }
  } else if(type==LIBRDF_NODE_TYPE_LITERAL) {
    unsigned char *value=librdf_node_get_literal_value(node);
    char *lang=librdf_node_get_literal_value_language(node);
    unsigned char *datatype=librdf_node_get_literal_value_datatype_uri(node) ? librdf_uri_as_string(librdf_node_get_literal_value_datatype_uri(node)) : 0;
    int valuelen=strlen((const char*)value);
    int langlen=lang ? strlen(lang) : 0;
    int datatypelen=datatype ? strlen((const char*)datatype) : 0;
    nodelen=valuelen+langlen+datatypelen;
    
    /* Create digest */
    librdf_digest_update(digest, (unsigned char*)"L", 1);
    librdf_digest_update(digest,value,valuelen);
    if(lang)
      librdf_digest_update(digest,(unsigned char*)lang,langlen);

    if(datatype)
      librdf_digest_update(digest,datatype,datatypelen);

    librdf_digest_final(digest);
    memcpy(id,librdf_digest_get_digest(digest),16);
    
    if(add) {
      /* Escape URI for db query */
      char *escvalue=(char*)LIBRDF_MALLOC(cstring,valuelen*2+1);
      char *esclang=lang?(char*)LIBRDF_MALLOC(cstring,langlen*2+1):0;
      char *escdatatype=datatype?(char*)LIBRDF_MALLOC(cstring,datatypelen*2+1):0;
      mysql_real_escape_string(&context->connection,escvalue,(const char*)value,valuelen);
      if(lang)
        mysql_real_escape_string(&context->connection,esclang,lang,langlen);
      if(datatype)
        mysql_real_escape_string(&context->connection,escdatatype,(const char*)datatype,datatypelen);
      valuelen=strlen(escvalue);
      langlen=lang?strlen(esclang):0;
      datatypelen=datatype?strlen(escdatatype):0;
      nodelen=valuelen+langlen+datatypelen;
      
      /* Create new literal, ignore if existing */
      if(!(query=(char*)LIBRDF_MALLOC(cstring, nodelen+135)))
        return 0;

      sprintf(query,"insert into Literals (ID1,ID2,Value,Language,Datatype) values (%llu,%llu,'%s','%s','%s')",*id1,*id2,escvalue,lang?esclang:"",datatype?escdatatype:"");
      if(mysql_real_query(&context->connection,query,strlen(query)) &&
         mysql_errno(&context->connection) != 1062) {
        LIBRDF_ERROR2(storage->world, librdf_storage_mysql_node_hash,
                      "Insert into Literals failed with error %s",
                      mysql_error(&context->connection));
        return 0;
      }
      LIBRDF_FREE(cstring,query);
      LIBRDF_FREE(cstring,escvalue);
      LIBRDF_FREE(cstring,esclang);
      LIBRDF_FREE(cstring,escdatatype);
    }
  } else if(type==LIBRDF_NODE_TYPE_BLANK) {
    unsigned char *name=librdf_node_get_blank_identifier(node);
    nodelen=strlen((const char*)name);
    
    /* Create digest */
    librdf_digest_update(digest, (unsigned char*)"B", 1);
    librdf_digest_update(digest, (unsigned char*)name, nodelen);
    librdf_digest_final(digest);
    memcpy(id,librdf_digest_get_digest(digest),16);
    
    if(add) {
      /* Escape name for db query */
      char *escname=(char*)LIBRDF_MALLOC(cstring, nodelen*2+1);
      if(!escname)
        return 0;

      mysql_real_escape_string(&context->connection, escname, (const char*)name, nodelen);
      nodelen=strlen(escname);
      
      /* Create new bnode, ignore if existing */
      if(!(query=(char*)LIBRDF_MALLOC(cstring, nodelen+107)))
        return 0;

      sprintf(query,"insert into Bnodes (ID1,ID2,Name) values (%llu,%llu,'%s')",*id1,*id2,escname);
      if(mysql_real_query(&context->connection,query,strlen(query)) &&
         mysql_errno(&context->connection) != 1062) {
        LIBRDF_ERROR2(storage->world, librdf_storage_mysql_node_hash,
                      "Insert into Bnodes failed with errror %s",
                      mysql_error(&context->connection));
        return 0;
      }
      LIBRDF_FREE(cstring,query);
      LIBRDF_FREE(cstring,escname);
    }
  } else
    return 0;

  librdf_free_digest(digest);
  return id;
}


static int
librdf_storage_mysql_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  return librdf_storage_mysql_context_remove_statement(storage,NULL,statement);
}


/**
 * librdf_storage_mysql_contains_statement:
 * @storage: the storage
 * @stream: a complete statement
 *
 * Test if a given complete statement is present in the model.
 *
 * Return value: Non-zero if the model contains the statement.
 **/
static int
librdf_storage_mysql_contains_statement(librdf_storage* storage, 
                                        librdf_statement* statement)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  char *query;
  char *subject,*predicate,*object;
  unsigned long long int *subject1,*subject2,*predicate1,*predicate2;
  unsigned long long int *object1,*object2;
  MYSQL_RES *res;
  
  /* Find IDs for nodes */
  subject=librdf_storage_mysql_node_hash(storage,
                                         librdf_statement_get_subject(statement),0);
  predicate=librdf_storage_mysql_node_hash(storage,
                                           librdf_statement_get_predicate(statement),0);
  object=librdf_storage_mysql_node_hash(storage,
                                        librdf_statement_get_object(statement),0);
  if(!subject || !predicate || !object)
    return 0;

  subject1=(unsigned long long int*)subject;
  subject2=(unsigned long long int*)&subject[8];
  predicate1=(unsigned long long int*)predicate;
  predicate2=(unsigned long long int*)&predicate[8];
  object1=(unsigned long long int*)object;
  object2=(unsigned long long int*)&object[8];
  
  /* Check for statement */
  if(!(query=(char*)LIBRDF_MALLOC(cstring,275)))
    return 0;

  sprintf(query,
          "select 1 from Statements where Subject1=%llu and Subject2=%llu and Predicate1=%llu and Predicate2=%llu and Object1=%llu and Object2=%llu and Model=%u limit 1",
          *subject1,*subject2,
          *predicate1,*predicate2,
          *object1,*object2,
          context->id);
  if(mysql_real_query(&context->connection,query,strlen(query))) {
    LIBRDF_ERROR2(storage->world, librdf_storage_mysql_contains_statement,
                  "Query for statement failed with error %s",
                  mysql_error(&context->connection));
    return 0;
  };

  if(!(res=mysql_store_result(&context->connection))) {
    LIBRDF_ERROR2(storage->world, librdf_storage_mysql_contains_statement,
                  "Store of statement failed with error %s",
                  mysql_error(&context->connection));
    return 0;
  };

  if(!(mysql_fetch_row(res)))
    return 0;

  mysql_free_result(res);
  LIBRDF_FREE(cstring,query);
  LIBRDF_FREE(cstring,subject);
  LIBRDF_FREE(cstring,predicate);
  LIBRDF_FREE(cstring,object);

  return 1;
}


static librdf_stream*
librdf_storage_mysql_serialise(librdf_storage* storage)
{
  return librdf_storage_mysql_context_serialise(storage,NULL);
}


/**
 * librdf_storage_mysql_find_statements:
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
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  librdf_storage_mysql_context_serialise_stream_context* scontext;
  MYSQL_RES *res;
  MYSQL_ROW row;
  librdf_stream *stream;
  librdf_node *node;
  
  /* Initialize "context" */
  scontext=(librdf_storage_mysql_context_serialise_stream_context*)
    LIBRDF_CALLOC(librdf_storage_mysql_context_serialise_stream_context,1,sizeof(librdf_storage_mysql_context_serialise_stream_context));
  if(!scontext)
    return NULL;

  scontext->storage=storage;
  scontext->one_context=0;

  scontext->current=librdf_new_statement(storage->world);
  if(!scontext->current)
    return NULL;

  scontext->current_context_node=NULL;
  scontext->stmts=NULL;
  scontext->count=0;
  
  /* Start query */
  sprintf(scontext->query,
          "select ID from Statements as T where Model=%u", context->id);

  /* Subject? */
  if(statement && 
     (node=librdf_statement_get_subject(statement)) &&
     librdf_storage_mysql_query_add_node_clause(storage, node, 
                                                scontext->query, "Subject")) {
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return NULL;
  };

  /* Predicate? */
  if(statement &&
     (node=librdf_statement_get_predicate(statement)) &&
     librdf_storage_mysql_query_add_node_clause(storage, node,
                                                scontext->query, "Predicate")) {
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return NULL;
  };

  /* Object? */
  if(statement &&
     (node=librdf_statement_get_object(statement)) &&
     librdf_storage_mysql_query_add_node_clause(storage, node,
                                                scontext->query, "Object")) {
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return NULL;
  };
  
  /* Perform query... */
  if(mysql_real_query(&context->connection,scontext->query,strlen(scontext->query)) ||
     !(res=mysql_store_result(&context->connection))) {
    LIBRDF_ERROR2(storage->world, librdf_storage_mysql_find_statements,
                  "Query failed with error %s",
                  mysql_error(&context->connection));
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return NULL;
  };

  if((scontext->count=mysql_num_rows(res))) {
    scontext->stmts=(unsigned int*)LIBRDF_CALLOC(unsigned int,sizeof(unsigned int),scontext->count);
    if(!scontext->stmts) {
      mysql_free_result(res);
      librdf_storage_mysql_context_serialise_finished((void*)scontext);
      return NULL;
    };

    while ((row=mysql_fetch_row(res)))
      scontext->stmts[--scontext->count]=atoi(row[0]);
    
    if(scontext->count!=0) {
      LIBRDF_ERROR2(storage->world, librdf_storage_mysql_find_statements,
                    "Fetch of context query failed with error %s",
                    mysql_error(&context->connection));
      mysql_free_result(res);
      librdf_storage_mysql_context_serialise_finished((void*)scontext);
      return NULL;
    };
    scontext->count=mysql_num_rows(res);
  };
  mysql_free_result(res);

  librdf_storage_mysql_context_serialise_next_statement(scontext);
  stream=librdf_new_stream(storage->world,(void*)scontext,
                           &librdf_storage_mysql_context_serialise_end_of_stream,
                           &librdf_storage_mysql_context_serialise_next_statement,
                           &librdf_storage_mysql_context_serialise_get_statement,
                           &librdf_storage_mysql_context_serialise_finished);
  if(!stream) {
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return NULL;
  };

  return stream;
}


static int
librdf_storage_mysql_query_add_node_clause(librdf_storage* storage,
                                           librdf_node* node,
                                           char* query,
                                           char* name)
{
  char *q;
  unsigned long long int *hash1,*hash2;
  char *hash=librdf_storage_mysql_node_hash(storage, node, 0);
  
  if(!hash)
    return 1;

  hash1=(unsigned long long int*)hash;
  hash2=(unsigned long long int*)&hash[8];
  if(!(q=(char*)LIBRDF_MALLOC(cstring,80))) {
    LIBRDF_FREE(cstring,hash);
    return 2;
  };

  sprintf(q, " and T.%s1=%llu and T.%s2=%llu", name, *hash1, name, *hash2);
  strcat(query, q);
  LIBRDF_FREE(cstring,q);
  LIBRDF_FREE(cstring,hash);

  return 0;
}


/**
 * librdf_storage_mysql_context_add_statements:
 * @storage: the storage
 * @context_node: &librdf_node object
 * @stream: the stream of statements
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
  char *ctxt=NULL;
  int helper=0;
  
  /* Find ID for context, creating if necessary */
  if(context_node) {
    ctxt=librdf_storage_mysql_node_hash(storage,context_node,1);
    if(!ctxt)
      return 1;
  };

  while(!librdf_stream_end(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);
    helper=librdf_storage_mysql_context_add_statement_helper(storage, ctxt,
                                                             statement);
    if(helper)
      return helper;
    librdf_stream_next(statement_stream);
  };

  if(context_node)
    LIBRDF_FREE(cstring,ctxt);
  
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
  char *ctxt=NULL;
  int helper=0;
  
  /* Find ID for context, creating if necessary */
  if(context_node) {
    ctxt=librdf_storage_mysql_node_hash(storage,context_node,1);
    if(!ctxt)
      return 1;
  };

  helper=librdf_storage_mysql_context_add_statement_helper(storage, ctxt,
                                                           statement);
  if(context_node)
    LIBRDF_FREE(cstring,ctxt);
  
  return helper;
}


static int
librdf_storage_mysql_context_add_statement_helper(librdf_storage* storage,
                                          char *ctxt,
                                          librdf_statement* statement)
{
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  char *query;
  char *subject,*predicate,*object;
  unsigned long long int *subject1,*subject2,*predicate1,*predicate2;
  unsigned long long int *object1,*object2,*ctxt1,*ctxt2;
  
  /* Find IDs for nodes, creating if necessary */
  subject=librdf_storage_mysql_node_hash(storage,
                                         librdf_statement_get_subject(statement),1);
  predicate=librdf_storage_mysql_node_hash(storage,
                                           librdf_statement_get_predicate(statement),1);
  object=librdf_storage_mysql_node_hash(storage,
                                        librdf_statement_get_object(statement),1);
  if(!subject || !predicate || !object)
    return 1;

  subject1=(unsigned long long int*)subject;
  subject2=(unsigned long long int*)&subject[8];
  predicate1=(unsigned long long int*)predicate;
  predicate2=(unsigned long long int*)&predicate[8];
  object1=(unsigned long long int*)object;
  object2=(unsigned long long int*)&object[8];

  if(ctxt) {
    ctxt1=(unsigned long long int*)ctxt;
    ctxt2=(unsigned long long int*)&ctxt[8];
  }
  
  /* Add statement to storage */
  if(!(query=(char*)LIBRDF_MALLOC(cstring,318)))
    return 1;

  sprintf(query,
          "insert into Statements (Subject1,Subject2,Predicate1,Predicate2,Object1,Object2,Context1,Context2,Model) values (%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u)",
          *subject1, *subject2,
          *predicate1, *predicate2,
          *object1, *object2,
          ctxt? *ctxt1 : 0,
          ctxt? *ctxt2 : 0,
          context->id);
  if(mysql_real_query(&context->connection,query,strlen(query))) {
    LIBRDF_ERROR2(storage->world, 
                  librdf_storage_mysql_context_add_statement_helper,
                  "Insert into Statements failed with error %s",
                  mysql_error(&context->connection));
    return 0-mysql_errno(&context->connection);
  };

  LIBRDF_FREE(cstring,query);
  LIBRDF_FREE(cstring,subject);
  LIBRDF_FREE(cstring,predicate);
  LIBRDF_FREE(cstring,object);
  
  return 0;
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
  char *query;
  char *subject,*predicate,*object,*ctxt;
  unsigned long long int *subject1,*subject2,*predicate1,*predicate2;
  unsigned long long int *object1,*object2,*ctxt1,*ctxt2;
  
  /* Find IDs for nodes */
  subject=librdf_storage_mysql_node_hash(storage,
                                         librdf_statement_get_subject(statement),0);
  predicate=librdf_storage_mysql_node_hash(storage,
                                           librdf_statement_get_predicate(statement),0);
  object=librdf_storage_mysql_node_hash(storage,
                                        librdf_statement_get_object(statement),0);
  ctxt=context_node ? librdf_storage_mysql_node_hash(storage, context_node,0) : 0;
  if(!subject || !predicate || !object || (context_node && !ctxt))
    return 1;

  subject1=(unsigned long long int*)subject;
  subject2=(unsigned long long int*)&subject[8];
  predicate1=(unsigned long long int*)predicate;
  predicate2=(unsigned long long int*)&predicate[8];
  object1=(unsigned long long int*)object;
  object2=(unsigned long long int*)&object[8];

  if(context_node) {
    ctxt1=(unsigned long long int*)ctxt;
    ctxt2=(unsigned long long int*)&ctxt[8];
  }

  /* Remove statement(s) from storage */
  if(!(query=(char*)LIBRDF_MALLOC(cstring,337)))
    return 1;

  if(context_node)
    sprintf(query,"delete from Statements where Subject1=%llu and Subject2=%llu and Predicate1=%llu and Predicate2=%llu and Object1=%llu and Object2=%llu and Context1=%llu and Context2=%llu and Model=%u",*subject1,*subject2,*predicate1,*predicate2,*object1,*object2,*ctxt1,*ctxt2,context->id);
  else
    sprintf(query,"delete from Statements where Subject1=%llu and Subject2=%llu and Predicate1=%llu and Predicate2=%llu and Object1=%llu and Object2=%llu and Model=%u",*subject1,*subject2,*predicate1,*predicate2,*object1,*object2,context->id);

  if(mysql_real_query(&context->connection,query,strlen(query))) {
    LIBRDF_ERROR2(storage->world, 
                  librdf_storage_mysql_context_remove_statement,
                  "Delete from Statements failed with error %s",
                  mysql_error(&context->connection));
    return 0-mysql_errno(&context->connection);
  };

  LIBRDF_FREE(cstring,query);
  LIBRDF_FREE(cstring,subject);
  LIBRDF_FREE(cstring,predicate);
  LIBRDF_FREE(cstring,object);
  if(context_node)
    LIBRDF_FREE(cstring,ctxt);

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
  char *ctxt;
  unsigned long long int *ctxt1,*ctxt2;
  char *query=(char*)LIBRDF_MALLOC(cstring,116);
  
  /* Remove statement(s) from storage */

  if(!query)
    return 1;
  
  /* Find ID for context node? */
  if(context_node) {
    ctxt=librdf_storage_mysql_node_hash(storage,context_node,0);
    if(!ctxt)
      return 1;

    ctxt1=(unsigned long long int*)ctxt;
    ctxt2=(unsigned long long int*)&ctxt[8];
    sprintf(query,
            "delete from Statements where Context1=%llu and Context2=%llu and Model=%u",
            *ctxt1, *ctxt2, context->id);
    LIBRDF_FREE(cstring,ctxt);
  } else
    sprintf(query, "delete from Statements where Model=%u", context->id);

  if(mysql_real_query(&context->connection,query,strlen(query))) {
    LIBRDF_ERROR2(storage->world, 
                  librdf_storage_mysql_context_remove_statements,
                  "Delete of context from Statements failed with error %s",
                  mysql_error(&context->connection));
    return 0-mysql_errno(&context->connection);
  };
  LIBRDF_FREE(cstring,query);

 return 0;
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
  librdf_storage_mysql_context* context=(librdf_storage_mysql_context*)storage->context;
  librdf_storage_mysql_context_serialise_stream_context* scontext;
  MYSQL_RES *res;
  MYSQL_ROW row;
  librdf_stream* stream;
  
  /* Initialize "context" */
  scontext=(librdf_storage_mysql_context_serialise_stream_context*)
    LIBRDF_CALLOC(librdf_storage_mysql_context_serialise_stream_context,1,sizeof(librdf_storage_mysql_context_serialise_stream_context));
  if(!scontext)
    return NULL;

  scontext->storage=storage;
  scontext->one_context=0;
  if(!(scontext->current=librdf_new_statement(storage->world)))
    return NULL;
  scontext->current_context_node=NULL;
  scontext->stmts=NULL;
  scontext->count=0;
  
  /* Start query */
  sprintf(scontext->query, "select ID from Statements where Model=%u",
          context->id);
  if(context_node) {
    if(librdf_storage_mysql_query_add_node_clause(storage,context_node,
                                                  scontext->query, 
                                                  "Context")) {
      librdf_storage_mysql_context_serialise_finished((void*)scontext);
      return NULL;
    };

    scontext->one_context=1;
    scontext->current_context_node=librdf_new_node_from_node(context_node);
  };

  if(mysql_real_query(&context->connection, scontext->query, 
                      strlen(scontext->query))) {
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return NULL;
  };

  if(!(res=mysql_store_result(&context->connection))) {
    LIBRDF_ERROR2(storage->world, librdf_storage_mysql_context_serialise,
                  "Context serialise query failed with error %s",
                  mysql_error(&context->connection));
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return NULL;
  };

  if((scontext->count=mysql_num_rows(res))) {
    if(!(scontext->stmts=(unsigned int*)LIBRDF_CALLOC(unsigned int, sizeof(unsigned int),
                                       scontext->count))) {
      mysql_free_result(res);
      librdf_storage_mysql_context_serialise_finished((void*)scontext);
      return NULL;
    };

    while ((row=mysql_fetch_row(res)))
      scontext->stmts[--scontext->count]=atoi(row[0]);

    if(scontext->count!=0) {
      LIBRDF_ERROR2(storage->world, librdf_storage_mysql_context_serialise,
                    "Fetch of context serialise query failed with error %s",
                    mysql_error(&context->connection));
      mysql_free_result(res);
      librdf_storage_mysql_context_serialise_finished((void*)scontext);
      return NULL;
    };
    scontext->count=mysql_num_rows(res);
  };

  mysql_free_result(res);
  librdf_storage_mysql_context_serialise_next_statement(scontext);
  stream=librdf_new_stream(storage->world,(void*)scontext,
                           &librdf_storage_mysql_context_serialise_end_of_stream,
                           &librdf_storage_mysql_context_serialise_next_statement,
                           &librdf_storage_mysql_context_serialise_get_statement,
                           &librdf_storage_mysql_context_serialise_finished);
  if(!stream) {
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return NULL;
  };

  return stream;
}


static int
librdf_storage_mysql_context_serialise_end_of_stream(void* context)
{
  librdf_storage_mysql_context_serialise_stream_context* scontext=(librdf_storage_mysql_context_serialise_stream_context*)context;
  
  return scontext->count < 0;
}


static int
librdf_storage_mysql_context_serialise_next_statement(void* context)
{
  librdf_storage_mysql_context_serialise_stream_context* scontext=(librdf_storage_mysql_context_serialise_stream_context*)context;
  librdf_storage_mysql_context *sc=(librdf_storage_mysql_context*)scontext->storage->context;
  MYSQL_RES *res;
  MYSQL_ROW row;
  librdf_node *s,*p,*o;
  
  scontext->count--;
  if(scontext->count<0)
    return 1;

  LIBRDF_DEBUG2(librdf_storage_mysql_context_serialise_next_statement,
                "retrieving statement #%u\n",
                scontext->stmts[scontext->count]);

  if(scontext->one_context)
    sprintf(scontext->query,
"select \
SuR.URI as SUR,SuB.Name as SUB,\
PrR.URI as PRR,ObR.URI as OBR,ObB.Name as OBB,\
ObL.Value as OBV,ObL.Language as OBL,ObL.Datatype as OBD \
from Statements as T \
left join Resources as SuR on T.Subject1=SuR.ID1 and T.Subject2=SuR.ID2 \
left join Bnodes as SuB on T.Subject1=SuB.ID1 and T.Subject2=SuB.ID2 \
left join Resources as PrR on T.Predicate1=PrR.ID1 and T.Predicate2=PrR.ID2 \
left join Resources as ObR on T.Object1=ObR.ID1 and T.Object2=ObR.ID2 \
left join Bnodes as ObB on T.Object1=ObB.ID1 and T.Object2=ObB.ID2 \
left join Literals as ObL on T.Object1=ObL.ID1 and T.Object2=ObL.ID2 \
where T.ID=%u",
            scontext->stmts[scontext->count]);
  else
    sprintf(scontext->query,
"select \
SuR.URI as SUR,SuB.Name as SUB,\
PrR.URI as PRR,ObR.URI as OBR,ObB.Name as OBB,\
ObL.Value as OBV,ObL.Language as OBL,ObL.Datatype as OBD,\
CoR.URI as COR,CoB.Name as COB,\
CoL.Value as COV,CoL.Language as COL,CoL.Datatype as CoD \
from Statements as T \
left join Resources as SuR on T.Subject1=SuR.ID1 and T.Subject2=SuR.ID2 \
left join Bnodes as SuB on T.Subject1=SuB.ID1 and T.Subject2=SuB.ID2 \
left join Resources as PrR on T.Predicate1=PrR.ID1 and T.Predicate2=PrR.ID2 \
left join Resources as ObR on T.Object1=ObR.ID1 and T.Object2=ObR.ID2 \
left join Bnodes as ObB on T.Object1=ObB.ID1 and T.Object2=ObB.ID2 \
left join Literals as ObL on T.Object1=ObL.ID1 and T.Object2=ObL.ID2 \
left join Resources as CoR on T.Context1=CoR.ID1 and T.Context2=CoR.ID2 \
left join Bnodes as CoB on T.Context1=CoB.ID1 and T.Context2=CoB.ID2 \
left join Literals as CoL on T.Context1=CoL.ID1 and T.Context2=CoL.ID2 \
where T.ID=%u",
            scontext->stmts[scontext->count]);

  if(mysql_real_query(&sc->connection,
                      scontext->query, strlen(scontext->query))) {
      LIBRDF_ERROR2(scontext->storage->world, 
                    librdf_storage_mysql_context_serialise_next_statement,
                    "Statement retrieval query failed with error %s",
                    mysql_error(&sc->connection));
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return 0;
  };
  
  if(!(res=mysql_store_result(&sc->connection)) ||
     !(row=mysql_fetch_row(res))) {
    LIBRDF_ERROR2(scontext->storage->world,
                  librdf_storage_mysql_context_serialise_next_statement,
                  "Store of retrieved statement failed with error %s",
                  mysql_error(&sc->connection));
    librdf_storage_mysql_context_serialise_finished((void*)scontext);
    return 0;
  };
  
  /* Turn row into statement and context */
  librdf_statement_clear(scontext->current);
  
  /* Subject. */
  if((!row[0] && !row[1]) ||
     !(s=row[0] ? librdf_new_node_from_uri_string(scontext->storage->world,(const unsigned char*)row[0])
                : librdf_new_node_from_blank_identifier(scontext->storage->world, (const unsigned char*)row[1]))
    )
    return 0;
  librdf_statement_set_subject(scontext->current,s);

  /* Predicate. */
  if(!row[2] ||
     !(p=librdf_new_node_from_uri_string(scontext->storage->world,(const unsigned char*)row[2])))
    return 0;
  librdf_statement_set_predicate(scontext->current,p);
  
  /* Object. */
  if((!row[3] && !row[4] && !row[5]) ||
     !(o=row[3] ? librdf_new_node_from_uri_string(scontext->storage->world,(const unsigned char*)row[3])
                : row[4] ? librdf_new_node_from_blank_identifier(scontext->storage->world,(const unsigned char*)row[4])
                         : librdf_new_node_from_typed_literal(scontext->storage->world,(const unsigned char*)row[5],row[6], row[7] && strlen(row[7])?librdf_new_uri(scontext->storage->world,(const unsigned char*)row[7]):NULL)
      )
    )
    return 0;
  librdf_statement_set_object(scontext->current,o);
  
  /* Context? */
  if(!scontext->one_context) {
    if(scontext->current_context_node)
      librdf_free_node(scontext->current_context_node);

    scontext->current_context_node=row[8]? librdf_new_node_from_uri_string(scontext->storage->world,(const unsigned char*)row[8]): row[9]? librdf_new_node_from_blank_identifier(scontext->storage->world,(const unsigned char*)row[9]): row[10]? librdf_new_node_from_typed_literal(scontext->storage->world,(const unsigned char*)row[10],row[11], row[12] && strlen(row[12])? librdf_new_uri(scontext->storage->world,(const unsigned char*)row[12]): NULL): NULL;
  };

  return 0;
}


static void*
librdf_storage_mysql_context_serialise_get_statement(void* context, int flags)
{
  librdf_storage_mysql_context_serialise_stream_context* scontext=(librdf_storage_mysql_context_serialise_stream_context*)context;
  
  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->current;
    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->current_context_node;
    default:
      abort();
  }
}


static void
librdf_storage_mysql_context_serialise_finished(void* context)
{
  librdf_storage_mysql_context_serialise_stream_context* scontext=(librdf_storage_mysql_context_serialise_stream_context*)context;
  
  if(scontext->stmts)
    LIBRDF_FREE(unsigned int,scontext->stmts);

  if(scontext->current)
    librdf_free_statement(scontext->current);

  if(scontext->current_context_node)
    librdf_free_node(scontext->current_context_node);

  LIBRDF_FREE(librdf_storage_mysql_context_serialise_stream_context, scontext);
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
  factory->size               = librdf_storage_mysql_size;
  factory->add_statement      = librdf_storage_mysql_add_statement;
  factory->add_statements     = librdf_storage_mysql_add_statements;
  factory->remove_statement   = librdf_storage_mysql_remove_statement;
  factory->contains_statement = librdf_storage_mysql_contains_statement;
  factory->serialise          = librdf_storage_mysql_serialise;
  factory->find_statements    = librdf_storage_mysql_find_statements;
  factory->context_add_statement     = librdf_storage_mysql_context_add_statement;
  factory->context_add_statements    = librdf_storage_mysql_context_add_statements;
  factory->context_remove_statement  = librdf_storage_mysql_context_remove_statement;
  factory->context_remove_statements = librdf_storage_mysql_context_remove_statements;
  factory->context_serialise         = librdf_storage_mysql_context_serialise;
}


void
librdf_init_storage_mysql(void)
{
  librdf_storage_register_factory("mysql",
                                  &librdf_storage_mysql_register_factory);
}
