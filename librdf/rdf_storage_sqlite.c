/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_sqlite.c - RDF Storage using SQLite implementation
 *
 * $Id$
 *
 * Copyright (C) 2004, David Beckett http://purl.org/net/dajobe/
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

#include <sqlite.h>

typedef struct
{
  librdf_storage *storage;
  
  sqlite *db;

  int is_new;
  
  /* If this is non-0, contexts are being used */
  int index_contexts;

  char *name;
  size_t name_len;  
} librdf_storage_sqlite_context;



/* prototypes for local functions */
static int librdf_storage_sqlite_init(librdf_storage* storage, char *name, librdf_hash* options);
static int librdf_storage_sqlite_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_sqlite_close(librdf_storage* storage);
static int librdf_storage_sqlite_size(librdf_storage* storage);
static int librdf_storage_sqlite_add_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_sqlite_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_sqlite_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_sqlite_contains_statement(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_sqlite_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_sqlite_find_statements(librdf_storage* storage, librdf_statement* statement);

/* serialising implementing functions */
static int librdf_storage_sqlite_serialise_end_of_stream(void* context);
static int librdf_storage_sqlite_serialise_next_statement(void* context);
static void* librdf_storage_sqlite_serialise_get_statement(void* context, int flags);
static void librdf_storage_sqlite_serialise_finished(void* context);

/* find_statements implementing functions */
static int librdf_storage_sqlite_find_statements_end_of_stream(void* context);
static int librdf_storage_sqlite_find_statements_next_statement(void* context);
static void* librdf_storage_sqlite_find_statements_get_statement(void* context, int flags);
static void librdf_storage_sqlite_find_statements_finished(void* context);

/* context functions */
static int librdf_storage_sqlite_context_add_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_sqlite_context_remove_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static librdf_stream* librdf_storage_sqlite_context_serialise(librdf_storage* storage, librdf_node* context_node);

/* context sqlite statement stream methods */
static int librdf_storage_sqlite_context_serialise_end_of_stream(void* context);
static int librdf_storage_sqlite_context_serialise_next_statement(void* context);
static void* librdf_storage_sqlite_context_serialise_get_statement(void* context, int flags);
static void librdf_storage_sqlite_context_serialise_finished(void* context);

/* helper functions for contexts */

static librdf_iterator* librdf_storage_sqlite_get_contexts(librdf_storage* storage);

/* get_context iterator functions */
static int librdf_storage_sqlite_get_contexts_is_end(void* iterator);
static int librdf_storage_sqlite_get_contexts_next_method(void* iterator);
static void* librdf_storage_sqlite_get_contexts_get_method(void* iterator, int);
static void librdf_storage_sqlite_get_contexts_finished(void* iterator);


static void librdf_storage_sqlite_register_factory(librdf_storage_factory *factory);



/* functions implementing storage api */
static int
librdf_storage_sqlite_init(librdf_storage* storage, char *name,
                           librdf_hash* options)
{
  librdf_storage_sqlite_context *context=(librdf_storage_sqlite_context*)storage->context;
  char *name_copy;
  int index_contexts=0;
  int is_new;
  
  if(!name)
    return 1;
  
  if((index_contexts=librdf_hash_get_as_boolean(options, "contexts"))<0)
    index_contexts=0; /* default is no contexts */

  context->storage=storage;
  context->index_contexts=index_contexts;

  context->name_len=strlen(name);
  name_copy=(char*)LIBRDF_MALLOC(cstring, context->name_len+1);
  if(!name_copy)
    return 1;
  strncpy(name_copy, name, context->name_len+1);
  context->name=name_copy;
  
  if((is_new=librdf_hash_get_as_boolean(options, "new"))<0)
    context->is_new=1; /* default is NOT NEW */

  /* no more options, might as well free them now */
  if(options)
    librdf_free_hash(options);

  return 0;
}


static void
librdf_storage_sqlite_terminate(librdf_storage* storage)
{
  librdf_storage_sqlite_context *context=(librdf_storage_sqlite_context*)storage->context;

  if(context->name)
    LIBRDF_FREE(cstring, context->name);
}


static int
librdf_storage_sqlite_open(librdf_storage* storage, librdf_model* model)
{
  librdf_storage_sqlite_context *context=(librdf_storage_sqlite_context*)storage->context;
  int mode=0;
  char *errmsg=NULL;
  
  context->db=sqlite_open(context->name, mode, &errmsg);
  if(!context->db) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s exec failed, %s", 
               context->name, errmsg);
    free(errmsg);
    return 1;
  }
  
  if(context->is_new) {
    int status;

    status=sqlite_exec(context->db,
                       "DROP TABLE triples;",
                       NULL, /* no callback */
                       NULL, /* arg */
                       &errmsg);
    /* don't care if this fails */

    status=sqlite_exec(context->db,
                       "CREATE TABLE triples (subject, subjectType, predicate, object, objectType, context);",
                       NULL, /* no callback */
                       NULL, /* arg */
                       &errmsg);
    if(status != SQLITE_OK) {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s sqlite_compile failed, %s (%d)", 
                 context->name, errmsg, status);
      sqlite_close(context->db);
      return 1;
    }

    status=sqlite_exec(context->db,
                       "CREATE INDEX sp-index ON triples (subject, subjectType, predicate);",
                       NULL, /* no callback */
                       NULL, /* arg */
                       &errmsg);
    if(status != SQLITE_OK) {
      librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s sqlite_compile failed, %s (%d)", 
                 context->name, errmsg, status);
      sqlite_close(context->db);
      return 1;
    }


  }
  
  return 0;
}


/**
 * librdf_storage_sqlite_close - Close the sqlite storage
 * @storage: the storage
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_sqlite_close(librdf_storage* storage)
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  int status=0;
  
  if(context->db) {
    sqlite_close(context->db);
    context->db=NULL;
  }

  return status;
}


static int
librdf_storage_sqlite_get_1int_callback(void *arg,
                                        int argc, char **argv,
                                        char **columnNames) {
  int* count_p=(int*)arg;
  
  if(argc == 1) {
    *count_p++;
  }
  return 0;
}


static int
librdf_storage_sqlite_size(librdf_storage* storage)
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  char *errmsg;
  int count=0;
  int status;
  
  /* FIXME - is this legal */
  status=sqlite_exec(context->db,
                     "SELECT COUNT(*) FROM triples",
                     librdf_storage_sqlite_get_1int_callback,
                     &count, &errmsg);
  
  if(status !=SQLITE_OK) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s exec failed, %s (%d)", 
               context->name, errmsg, status);
    sqlite_freemem(errmsg);
    return -1;
  }

  return count;
}


static int
librdf_storage_sqlite_add_statement(librdf_storage* storage, librdf_statement* statement)
{
  /* Do not add duplicate statements */
  if(librdf_storage_sqlite_contains_statement(storage, statement))
    return 0;

  return librdf_storage_sqlite_context_add_statement(storage, NULL, statement);
}


static int
librdf_storage_sqlite_add_statements(librdf_storage* storage,
                                   librdf_stream* statement_stream)
{
  /* librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context; */
  int status=0;

  for(; !librdf_stream_end(statement_stream);
      librdf_stream_next(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);

    if(!statement) {
      status=1;
      break;
    }

    /* Do not add duplicate statements */
    if(librdf_storage_sqlite_contains_statement(storage, statement))
      continue;

    /* FIXME - add a statement */
    
    /* "INSERT INTO triples (subject, subjectType, predicate, object, objectType, context) VALUES ('s', 'st', 'p', 'o', 'ot', 'c');" */
    
  }
  
  return status;
}


static int
librdf_storage_sqlite_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  return librdf_storage_sqlite_context_remove_statement(storage, NULL, statement);
}


static char *
sqlite_string_escape(char *raw, size_t raw_len, size_t *len_p) 
{
  int escapes=0;
  char *p;
  char *escaped;
  int len;

  for(p=raw; *p; p++) {
    if(*p == '\'')
      escapes++;
  }

  len+= raw_len+1;
  escaped=(char*)LIBRDF_MALLOC(cstring, len);

  p=escaped;
  while(*raw) {
    if(*raw == '\'') {
      *p++='\\';
    }
    *p++=*raw++;
  }

  if(len_p)
    *len_p=len;
  
  return escaped;
}


static const char* sqlite_node_types[3]={"r", "l", "c"};

static char*
librdf_node_to_sqlite_string(librdf_node *node, size_t *output_len,
                             const char **type) {
  size_t len;
  unsigned char *s;

  switch(librdf_node_get_type(node)) {
    case LIBRDF_NODE_TYPE_RESOURCE:
      s=librdf_uri_as_counted_string(librdf_node_get_uri(node), &len);
      if(!s)
        return NULL;

      s=sqlite_string_escape(s, len, output_len);
      *type=sqlite_node_types[0];
      break;

    case LIBRDF_NODE_TYPE_LITERAL:
      s=librdf_node_get_literal_value_as_counted_string(node, &len);
      if(!s)
        return NULL;

#if 0
      if(node->value.literal.xml_language) {
        language_len=strlen(node->value.literal.xml_language);
        len+=1+language_len;
      }

      if(node->value.literal.datatype_uri) {
        datatype_uri_string=librdf_uri_to_counted_string(node->value.literal.datatype_uri, &datatype_len);
        len+=4+datatype_len;
      }
#endif

      s=sqlite_string_escape(s, len, output_len);
      *type=sqlite_node_types[1];
      break;

    case LIBRDF_NODE_TYPE_BLANK:
      s=librdf_node_get_blank_identifier(node);
      s=sqlite_string_escape(s, len, output_len);
      *type=sqlite_node_types[2];
      break;

    default:
      librdf_log(node->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Do not know how to encode node type %d\n", node->type);
    return NULL;
  }

  return s;
}


static int
librdf_storage_sqlite_contains_statement(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  int status;
  char *errmsg=NULL;
  int count=0;
  
  if(context->index_contexts) {

    /* When we have contexts, we have to use find_statements for contains
     * since we do not know what context node may be stored for a statement
     */
    librdf_stream *stream=librdf_storage_sqlite_find_statements(storage, statement);

    if(!stream)
      return 0;
    /* librdf_stream_end returns 0 if have more, non-0 at end */
    status=!librdf_stream_end(stream);
    /* convert to 0 if at end (not found) and non-zero otherwise (found) */
    librdf_free_stream(stream);
    return status;
  }

  /* FIXME - find a statement */

  status=sqlite_exec_printf(context->db,
                            "SELECT subject, subjectType, predicate, object, objectType, context FROM triples WHERE subject='%s' AND subjectType='%s' AND predicate='%s' AND object='%s' and objectType='%s';",
                            librdf_storage_sqlite_get_1int_callback,
                            &count,
                            &errmsg);
  /* "SELECT COUNT(*) FROM triples where (s, p, o)" */
  if(status != SQLITE_OK) {
    librdf_log(storage->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s sqlite_finalize failed, %s (%d)", 
               context->name, errmsg, status);
    sqlite_freemem(errmsg);
    return 0;
  }

  return (count > 0);
}


typedef struct {
  librdf_storage *storage;
  librdf_storage_sqlite_context* sqlite_context;

  int finished;

  int index_contexts;

  librdf_statement *statement;
  librdf_node* context;

  sqlite_vm *vm;
  /* OUT from vm: */
  const char *pzTail;
  sqlite_vm *ppVm;
  char *pzErrMsg;
} librdf_storage_sqlite_serialise_stream_context;


static librdf_stream*
librdf_storage_sqlite_serialise(librdf_storage* storage)
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  librdf_storage_sqlite_serialise_stream_context* scontext;
  librdf_stream* stream;
  int status;
  
  scontext=(librdf_storage_sqlite_serialise_stream_context*)LIBRDF_CALLOC(librdf_storage_sqlite_serialise_stream_context, 1, sizeof(librdf_storage_sqlite_serialise_stream_context));
  if(!scontext)
    return NULL;

  scontext->index_contexts=context->index_contexts;
  scontext->sqlite_context=context;

  status=sqlite_compile(context->db,
                        "SELECT subject, subjectType, predicate, object, objectType, context FROM triples;",
                        &scontext->pzTail, &scontext->ppVm,
                        &scontext->pzErrMsg);
  if(status != SQLITE_OK) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s sqlite_compile failed, %s (%d)", 
               context->name, scontext->pzErrMsg, status);

    librdf_storage_sqlite_serialise_finished((void*)scontext);
    return NULL;
  }
    
  
  scontext->storage=storage;
  librdf_storage_add_reference(scontext->storage);

  stream=librdf_new_stream(storage->world,
                           (void*)scontext,
                           &librdf_storage_sqlite_serialise_end_of_stream,
                           &librdf_storage_sqlite_serialise_next_statement,
                           &librdf_storage_sqlite_serialise_get_statement,
                           &librdf_storage_sqlite_serialise_finished);
  if(!stream) {
    librdf_storage_sqlite_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}



static int
librdf_storage_sqlite_get_next_common(librdf_storage_sqlite_context* scontext,
                                      sqlite_vm *vm) {
  int status=SQLITE_BUSY;
  int pN;
  const char **pazValue;   /* Column data */
  const char **pazColName; /* Column names and datatypes */
  int result=0;
  
  /*
   * Each invocation of sqlite_step returns an integer code that
   * indicates what happened during that step. This code may be
   * SQLITE_BUSY, SQLITE_ROW, SQLITE_DONE, SQLITE_ERROR, or
   * SQLITE_MISUSE.
  */
  do {
    status=sqlite_step(vm, &pN, &pazValue, &pazColName);
    if(status == SQLITE_BUSY) {
      /* FIXME - how to handle busy? */
      status=SQLITE_ERROR;
      continue;
    }
    break;
  } while(1);

  if(status == SQLITE_ROW) {
    /* FIXME - turn row data into scontext->statement, scontext->context */
  }

  if(status != SQLITE_ROW)
    result=1;

  if(status == SQLITE_ERROR) {
    char *errmsg=NULL;
    
    status=sqlite_finalize(vm, &errmsg);
    if(status != SQLITE_OK) {
      librdf_log(scontext->storage->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s sqlite_finalize failed, %s (%d)", 
                 scontext->name, errmsg, status);
      sqlite_freemem(errmsg);
    }
    result= -1;
  }
  
  return result;
}



static int
librdf_storage_sqlite_serialise_end_of_stream(void* context)
{
  librdf_storage_sqlite_serialise_stream_context* scontext=(librdf_storage_sqlite_serialise_stream_context*)context;
  int result;
  
  if(scontext->finished)
    return 1;
  
  result=librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                               scontext->vm);
  if(result) {
    /* error or finished */
    if(result<0)
      scontext->vm=NULL;
    scontext->finished=1;
  }

  return scontext->finished;
}


static int
librdf_storage_sqlite_serialise_next_statement(void* context)
{
  librdf_storage_sqlite_serialise_stream_context* scontext=(librdf_storage_sqlite_serialise_stream_context*)context;
  int result;
  
  result=librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                               scontext->vm);
  if(result) {
    /* error or finished */
    if(result<0)
      scontext->vm=NULL;
    scontext->finished=1;
  }

  return result;
}


static void*
librdf_storage_sqlite_serialise_get_statement(void* context, int flags)
{
  librdf_storage_sqlite_serialise_stream_context* scontext=(librdf_storage_sqlite_serialise_stream_context*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->statement;
    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->context;
    default:
      librdf_log(scontext->storage->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d\n", flags);
      return NULL;
  }
}


static void
librdf_storage_sqlite_serialise_finished(void* context)
{
  librdf_storage_sqlite_serialise_stream_context* scontext=(librdf_storage_sqlite_serialise_stream_context*)context;

  if(scontext->vm) {
    char *errmsg=NULL;

    int status=sqlite_finalize(scontext->vm, &errmsg);
    if(status != SQLITE_OK) {
      librdf_log(scontext->storage->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s sqlite_finalize failed, %s (%d)", 
                 scontext->sqlite_context->name, errmsg, status);
      sqlite_freemem(errmsg);
    }
  }

  if(scontext->pzErrMsg) {
    sqlite_freemem(scontext->pzErrMsg);
    scontext->pzErrMsg=NULL;
  }

  if(scontext->storage)
    librdf_storage_remove_reference(scontext->storage);

  LIBRDF_FREE(librdf_storage_sqlite_serialise_stream_context, scontext);
}


typedef struct {
  librdf_storage *storage;
  librdf_storage_sqlite_context* sqlite_context;

  int finished;

  int index_contexts;

  librdf_statement *statement;
  librdf_node* context;

  sqlite_vm *vm;
  /* OUT from vm: */
  const char *pzTail;
  sqlite_vm *ppVm;
  char *pzErrMsg;
} librdf_storage_sqlite_find_statements_stream_context;


/**
 * librdf_storage_sqlite_find_statements:
 * @storage: the storage
 * @statement: the statement to match
 * 
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 * Uses &librdf_statement_match to do the matching.
 * 
 * Return value: a &librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_sqlite_find_statements(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  librdf_storage_sqlite_find_statements_stream_context* scontext;
  librdf_stream* stream;
  char query[2000];
  int status;
  char *subject_string, *predicate_string, *object_string, *context_string;
  const char *subject_type, *object_type;
  
  scontext=(librdf_storage_sqlite_find_statements_stream_context*)LIBRDF_CALLOC(librdf_storage_sqlite_find_statements_stream_context, 1, sizeof(librdf_storage_sqlite_find_statements_stream_context));
  if(!scontext)
    return NULL;

  scontext->index_contexts=context->index_contexts;
  scontext->sqlite_context=context;


  statement=librdf_new_statement_from_statement(statement);
  if(!statement)
    return NULL;

  subject_string=librdf_node_to_sqlite_string(statement->subject, NULL,
                                              &subject_type);
  predicate_string=librdf_node_to_sqlite_string(statement->object, NULL,
                                                NULL);
  object_string=librdf_node_to_sqlite_string(statement->object, NULL,
                                             &object_type);
  context_string="c";
  
  sprintf(query, "SELECT subject, subjectType, predicate, object, objectType, context FROM triples WHERE subject='%s' AND subjectType='%s' AND predicate='%s' AND object='%s' AND objectType='%s' AND context='%s';",
          subject_string, subject_type,
          predicate_string,
          object_string, object_type,
          context_string);

  LIBRDF_FREE(cstring, subject_string);
  LIBRDF_FREE(cstring, predicate_string);
  LIBRDF_FREE(cstring, object_string);
  
  status=sqlite_compile(context->db,
                        query,
                        &scontext->pzTail, &scontext->ppVm,
                        &scontext->pzErrMsg);
  if(status != SQLITE_OK) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s sqlite_compile failed, %s (%d)", 
               context->name, scontext->pzErrMsg, status);

    librdf_storage_sqlite_serialise_finished((void*)scontext);
    return NULL;
  }
    
  
  scontext->storage=storage;
  librdf_storage_add_reference(scontext->storage);

  stream=librdf_new_stream(storage->world,
                           (void*)scontext,
                           &librdf_storage_sqlite_find_statements_end_of_stream,
                           &librdf_storage_sqlite_find_statements_next_statement,
                           &librdf_storage_sqlite_find_statements_get_statement,
                           &librdf_storage_sqlite_find_statements_finished);
  if(!stream) {
    librdf_storage_sqlite_find_statements_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_storage_sqlite_find_statements_end_of_stream(void* context)
{
  librdf_storage_sqlite_find_statements_stream_context* scontext=(librdf_storage_sqlite_find_statements_stream_context*)context;
  int result;
  
  if(scontext->finished)
    return 1;
  
  result=librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                               scontext->vm);
  if(result) {
    /* error or finished */
    if(result<0)
      scontext->vm=NULL;
    scontext->finished=1;
  }

  return scontext->finished;
}


static int
librdf_storage_sqlite_find_statements_next_statement(void* context)
{
  librdf_storage_sqlite_find_statements_stream_context* scontext=(librdf_storage_sqlite_find_statements_stream_context*)context;
  int result;
  
  result=librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                               scontext->vm);
  if(result) {
    /* error or finished */
    if(result<0)
      scontext->vm=NULL;
    scontext->finished=1;
  }

  return result;
}


static void*
librdf_storage_sqlite_find_statements_get_statement(void* context, int flags)
{
  librdf_storage_sqlite_find_statements_stream_context* scontext=(librdf_storage_sqlite_find_statements_stream_context*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->statement;
    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->context;
    default:
      librdf_log(scontext->storage->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d\n", flags);
      return NULL;
  }
}


static void
librdf_storage_sqlite_find_statements_finished(void* context)
{
  librdf_storage_sqlite_find_statements_stream_context* scontext=(librdf_storage_sqlite_find_statements_stream_context*)context;

  if(scontext->vm) {
    char *errmsg=NULL;

    int status=sqlite_finalize(scontext->vm, &errmsg);
    if(status != SQLITE_OK) {
      librdf_log(scontext->storage->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s sqlite_finalize failed, %s (%d)", 
                 scontext->sqlite_context->name, errmsg, status);
      sqlite_freemem(errmsg);
    }
  }

  if(scontext->pzErrMsg) {
    sqlite_freemem(scontext->pzErrMsg);
    scontext->pzErrMsg=NULL;
  }

  if(scontext->storage)
    librdf_storage_remove_reference(scontext->storage);

  LIBRDF_FREE(librdf_storage_sqlite_find_statements_stream_context, scontext);
}


/**
 * librdf_storage_sqlite_context_add_statement - Add a statement to a storage context
 * @storage: &librdf_storage object
 * @context_node: &librdf_node object
 * @statement: &librdf_statement statement to add
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_sqlite_context_add_statement(librdf_storage* storage,
                                          librdf_node* context_node,
                                          librdf_statement* statement) 
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  int status=0;

  if(context_node && !context->index_contexts) {
    librdf_log(storage->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_STORAGE, NULL,
               "Storage was created without context support");
    return 1;
  }
  
  /* FIXME Store statement + node in the storage_sqlite */

  if(!context->index_contexts || !context_node)
    return 0;
  
  /* Store (context => statement) in the context hash */

  return status;
}


/**
 * librdf_storage_sqlite_context_remove_statement - Remove a statement from a storage context
 * @storage: &librdf_storage object
 * @context_node: &librdf_node object
 * @statement: &librdf_statement statement to remove
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_sqlite_context_remove_statement(librdf_storage* storage, 
                                               librdf_node* context_node,
                                               librdf_statement* statement) 
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  int status;

  if(context_node && !context->index_contexts) {
    librdf_log(storage->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_STORAGE, NULL,
               "Storage was created without context support");
    return 1;
  }
  
  /* FIXME - remove statement */

  return status;
}


typedef struct {
  librdf_storage *storage;
  librdf_statement current; /* static, shared statement */
  librdf_node *context_node;
  char *context_node_data;
} librdf_storage_sqlite_context_serialise_stream_context;


/**
 * librdf_storage_sqlite_context_serialise - Sqlite all statements in a storage context
 * @storage: &librdf_storage object
 * @context_node: &librdf_node object
 * 
 * Return value: &librdf_stream of statements or NULL on failure or context is empty
 **/
static librdf_stream*
librdf_storage_sqlite_context_serialise(librdf_storage* storage,
                                      librdf_node* context_node) 
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  librdf_storage_sqlite_context_serialise_stream_context* scontext;
  librdf_stream* stream;

  if(!context->index_contexts) {
    librdf_log(storage->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_STORAGE, NULL,
               "Storage was created without context support");
    return NULL;
  }
  
  scontext=(librdf_storage_sqlite_context_serialise_stream_context*)LIBRDF_CALLOC(librdf_storage_sqlite_context_serialise_stream_context, 1, sizeof(librdf_storage_sqlite_context_serialise_stream_context));
  if(!scontext)
    return NULL;

  librdf_statement_init(storage->world, &scontext->current);

  scontext->context_node=librdf_new_node_from_node(context_node);



  scontext->storage=storage;
  librdf_storage_add_reference(scontext->storage);

  stream=librdf_new_stream(storage->world,
                           (void*)scontext,
                           &librdf_storage_sqlite_context_serialise_end_of_stream,
                           &librdf_storage_sqlite_context_serialise_next_statement,
                           &librdf_storage_sqlite_context_serialise_get_statement,
                           &librdf_storage_sqlite_context_serialise_finished);
  if(!stream) {
    librdf_storage_sqlite_context_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_storage_sqlite_context_serialise_end_of_stream(void* context)
{
  /* librdf_storage_sqlite_context_serialise_stream_context* scontext=(librdf_storage_sqlite_context_serialise_stream_context*)context; */

  /* FIXME */
  return 1;
}


static int
librdf_storage_sqlite_context_serialise_next_statement(void* context)
{
  /* librdf_storage_sqlite_context_serialise_stream_context* scontext=(librdf_storage_sqlite_context_serialise_stream_context*)context; */

  return 1;
}


static void*
librdf_storage_sqlite_context_serialise_get_statement(void* context, int flags)
{
  librdf_storage_sqlite_context_serialise_stream_context* scontext=(librdf_storage_sqlite_context_serialise_stream_context*)context;
  
  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      librdf_statement_clear(&scontext->current);

      /* FIXME decode content */

      return &scontext->current;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->context_node;

    default:
      librdf_log(scontext->storage->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d\n", flags);
      return NULL;
  }
  
}


static void
librdf_storage_sqlite_context_serialise_finished(void* context)
{
  librdf_storage_sqlite_context_serialise_stream_context* scontext=(librdf_storage_sqlite_context_serialise_stream_context*)context;
  
  if(scontext->context_node)
    librdf_free_node(scontext->context_node);
  
  if(scontext->context_node_data)
    LIBRDF_FREE(cstring, scontext->context_node_data);

  librdf_statement_clear(&scontext->current);

  if(scontext->storage)
    librdf_storage_remove_reference(scontext->storage);

  LIBRDF_FREE(librdf_storage_sqlite_context_serialise_stream_context, scontext);
}



typedef struct {
  librdf_storage *storage;
  librdf_iterator *iterator;
  librdf_hash_datum *key;
  librdf_node *current;
} librdf_storage_sqlite_get_contexts_iterator_context;



static int
librdf_storage_sqlite_get_contexts_is_end(void* iterator)
{
  librdf_storage_sqlite_get_contexts_iterator_context* icontext=(librdf_storage_sqlite_get_contexts_iterator_context*)iterator;

  return librdf_iterator_end(icontext->iterator);
}


static int
librdf_storage_sqlite_get_contexts_next_method(void* iterator) 
{
  librdf_storage_sqlite_get_contexts_iterator_context* icontext=(librdf_storage_sqlite_get_contexts_iterator_context*)iterator;

  return librdf_iterator_next(icontext->iterator);
}


static void*
librdf_storage_sqlite_get_contexts_get_method(void* iterator, int flags) 
{
  librdf_storage_sqlite_get_contexts_iterator_context* icontext=(librdf_storage_sqlite_get_contexts_iterator_context*)iterator;
  void *result=NULL;
  librdf_hash_datum* k;
  
  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      if(!(k=(librdf_hash_datum*)librdf_iterator_get_key(icontext->iterator)))
        return NULL;

      if(icontext->current)
        librdf_free_node(icontext->current);

      /* decode value content */
      icontext->current=librdf_node_decode(icontext->storage->world, NULL,
                                           (unsigned char*)k->data, k->size);
      result=icontext->current;
      break;

    case LIBRDF_ITERATOR_GET_METHOD_GET_KEY:
    case LIBRDF_ITERATOR_GET_METHOD_GET_VALUE:
      result=NULL;
      break;
      
    default:
      librdf_log(icontext->iterator->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d\n", flags);
      result=NULL;
      break;
  }

  return result;
}


static void
librdf_storage_sqlite_get_contexts_finished(void* iterator) 
{
  librdf_storage_sqlite_get_contexts_iterator_context* icontext=(librdf_storage_sqlite_get_contexts_iterator_context*)iterator;

  if(icontext->iterator)
    librdf_free_iterator(icontext->iterator);

  librdf_free_hash_datum(icontext->key);
  
  if(icontext->current)
    librdf_free_node(icontext->current);

  if(icontext->storage)
    librdf_storage_remove_reference(icontext->storage);
  
  LIBRDF_FREE(librdf_storage_sqlite_get_contexts_iterator_context, icontext);
}


/**
 * librdf_storage_sqlite_context_get_contexts - Sqlite all context nodes in a storage
 * @storage: &librdf_storage object
 * 
 * Return value: &librdf_iterator of context_nodes or NULL on failure or no contexts
 **/
static librdf_iterator*
librdf_storage_sqlite_get_contexts(librdf_storage* storage) 
{
  librdf_storage_sqlite_context* context=(librdf_storage_sqlite_context*)storage->context;
  librdf_storage_sqlite_get_contexts_iterator_context* icontext;
  librdf_iterator* iterator;

  if(!context->index_contexts) {
    librdf_log(storage->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_STORAGE, NULL,
               "Storage was created without context support");
    return NULL;
  }
  
  icontext=(librdf_storage_sqlite_get_contexts_iterator_context*)LIBRDF_CALLOC(librdf_storage_sqlite_get_contexts_iterator_context, 1, sizeof(librdf_storage_sqlite_get_contexts_iterator_context));
  if(!icontext)
    return NULL;

  icontext->key=librdf_new_hash_datum(storage->world, NULL, 0);
  if(!icontext->key)
    return NULL;
  
  icontext->storage=storage;
  librdf_storage_add_reference(icontext->storage);
  
  iterator=librdf_new_iterator(storage->world,
                               (void*)icontext,
                               &librdf_storage_sqlite_get_contexts_is_end,
                               &librdf_storage_sqlite_get_contexts_next_method,
                               &librdf_storage_sqlite_get_contexts_get_method,
                               &librdf_storage_sqlite_get_contexts_finished);
  if(!iterator) {
    librdf_storage_sqlite_get_contexts_finished((void*)icontext);
    return NULL;
  }
  
  return iterator;  
}



/**
 * librdf_storage_sqlite_get_feature - get the value of a storage feature
 * @storage: &librdf_storage object
 * @feature: &librdf_uri feature property
 * 
 * Return value: &librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
static librdf_node*
librdf_storage_sqlite_get_feature(librdf_storage* storage, librdf_uri* feature)
{
  librdf_storage_sqlite_context* scontext=(librdf_storage_sqlite_context*)storage->context;
  unsigned char *uri_string;

  if(!feature)
    return NULL;

  uri_string=librdf_uri_as_string(feature);
  if(!uri_string)
    return NULL;
  
  if(!strcmp((const char*)uri_string, LIBRDF_MODEL_FEATURE_CONTEXTS)) {
    unsigned char value[2];

    sprintf((char*)value, "%d", (scontext->index_contexts != 0));
    return librdf_new_node_from_typed_literal(storage->world,
                                              value, NULL, NULL);
  }

  return NULL;
}


/* local function to register sqlite storage functions */

static void
librdf_storage_sqlite_register_factory(librdf_storage_factory *factory) 
{
  factory->context_length     = sizeof(librdf_storage_sqlite_context);
  
  factory->init               = librdf_storage_sqlite_init;
  factory->terminate          = librdf_storage_sqlite_terminate;
  factory->open               = librdf_storage_sqlite_open;
  factory->close              = librdf_storage_sqlite_close;
  factory->size               = librdf_storage_sqlite_size;
  factory->add_statement      = librdf_storage_sqlite_add_statement;
  factory->add_statements     = librdf_storage_sqlite_add_statements;
  factory->remove_statement   = librdf_storage_sqlite_remove_statement;
  factory->contains_statement = librdf_storage_sqlite_contains_statement;
  factory->serialise          = librdf_storage_sqlite_serialise;
  factory->find_statements    = librdf_storage_sqlite_find_statements;
  factory->context_add_statement    = librdf_storage_sqlite_context_add_statement;
  factory->context_remove_statement = librdf_storage_sqlite_context_remove_statement;
  factory->context_serialise        = librdf_storage_sqlite_context_serialise;
  factory->get_contexts             = librdf_storage_sqlite_get_contexts;
  factory->get_feature              = librdf_storage_sqlite_get_feature;
}


void
librdf_init_storage_sqlite(librdf_world *world)
{
  librdf_storage_register_factory(world, "sqllite", "SQLite",
                                  &librdf_storage_sqlite_register_factory);
}
