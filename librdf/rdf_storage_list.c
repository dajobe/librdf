/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_list.c - RDF Storage in memory as a list implementation
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
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
#include <sys/types.h>

#include <librdf.h>


typedef struct
{
  librdf_list* list;
  librdf_hash* groups;
  
} librdf_storage_list_context;


/* prototypes for local functions */
static int librdf_storage_list_init(librdf_storage* storage, char *name, librdf_hash* options);
static int librdf_storage_list_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_list_close(librdf_storage* storage);
static int librdf_storage_list_size(librdf_storage* storage);
static int librdf_storage_list_add_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_list_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_list_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_list_contains_statement(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_list_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_list_find_statements(librdf_storage* storage, librdf_statement* statement);

/* serialising implementing functions */
static int librdf_storage_list_serialise_end_of_stream(void* context);
static int librdf_storage_list_serialise_next_statement(void* context);
static void* librdf_storage_list_serialise_get_statement(void* context, int flags);
static void librdf_storage_list_serialise_finished(void* context);

/* group functions */
static int librdf_storage_list_group_add_statement(librdf_storage* storage, librdf_uri* group_uri, librdf_statement* statement);
static int librdf_storage_list_group_remove_statement(librdf_storage* storage, librdf_uri* group_uri, librdf_statement* statement);
static librdf_stream* librdf_storage_list_group_serialise(librdf_storage* storage, librdf_uri* group_uri);

/* group list statement stream methods */
static int librdf_storage_list_group_serialise_end_of_stream(void* context);
static int librdf_storage_list_group_serialise_next_statement(void* context);
static void* librdf_storage_list_group_serialise_get_statement(void* context, int flags);
static void librdf_storage_list_group_serialise_finished(void* context);



static void librdf_storage_list_register_factory(librdf_storage_factory *factory);



/* functions implementing storage api */
static int
librdf_storage_list_init(librdf_storage* storage, char *name,
                         librdf_hash* options)
{
  /* do not need options, might as well free them now */
  if(options)
    librdf_free_hash(options);

  return 0;
}


static void
librdf_storage_list_terminate(librdf_storage* storage)
{
  /* nop */  
}




static int
librdf_storage_list_open(librdf_storage* storage, librdf_model* model)
{
  librdf_storage_list_context *context=(librdf_storage_list_context*)storage->context;

  context->list=librdf_new_list(storage->world);
  if(!context->list)
    return 1;

  librdf_list_set_equals(context->list, 
                         (int (*)(void*, void*))&librdf_statement_equals);

  /* create a new memory hash */
  context->groups=librdf_new_hash(storage->world, NULL);
  if(librdf_hash_open(context->groups, NULL, 0, 1, 1, NULL)) {
    librdf_free_list(context->list);
    context->list=NULL;
    return 1;
  }

  return 0;
}


/**
 * librdf_storage_list_close:
 * @storage: the storage
 * 
 * Close the storage list storage, and free all content since there is no 
 * persistance.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_list_close(librdf_storage* storage)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;
  int status=0;
  librdf_iterator* iterator;
  librdf_statement* statement;
  
  if(context->list) {
    iterator=librdf_list_get_iterator(context->list);
    status=(iterator != 0);
    if(iterator) {
      while(!librdf_iterator_end(iterator)) {
        statement=(librdf_statement*)librdf_iterator_get_object(iterator);
        if(statement)
          librdf_free_statement(statement);
        librdf_iterator_next(iterator);
      }
      librdf_free_iterator(iterator);
    }
    librdf_free_list(context->list);

    context->list=NULL;
  }

  if(context->groups) {
    librdf_free_hash(context->groups);
    context->groups=NULL;
  }
  
  return status;
}


static int
librdf_storage_list_size(librdf_storage* storage)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;

  return librdf_list_size(context->list);
}


static int
librdf_storage_list_add_statement(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;
  librdf_statement* statement2;
  int status;

  statement2=librdf_new_statement_from_statement(statement);
  if(!statement2)
    return 1;
  
  status=librdf_list_add(context->list, statement2);
  if(status)
    librdf_free_statement(statement2);
  return status;
}


static int
librdf_storage_list_add_statements(librdf_storage* storage,
                                   librdf_stream* statement_stream)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;
  librdf_statement* statement;
  int status=0;

  while(!librdf_stream_end(statement_stream)) {
    statement=librdf_stream_get_object(statement_stream);

    if(statement) {
      /* copy shared statement */
      statement=librdf_new_statement_from_statement(statement);
      if(!statement) {
        status=1;
        break;
      }
      librdf_list_add(context->list, statement);
    } else
      status=1;
    librdf_stream_next(statement_stream);
  }
  librdf_free_stream(statement_stream);
  
  return status;
}


static int
librdf_storage_list_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;

  /* get actual stored statement */
  statement=librdf_list_remove(context->list, statement);
  if(!statement)
    return 1;
  
  librdf_free_statement(statement);
  return 0;
}


static int
librdf_storage_list_contains_statement(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;

  return librdf_list_contains(context->list, statement);
}


static librdf_stream*
librdf_storage_list_serialise(librdf_storage* storage)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;
  librdf_iterator* iterator;
  librdf_stream* stream;
  
  iterator=librdf_list_get_iterator(context->list);
  if(!iterator) {
    librdf_storage_list_serialise_finished((void*)iterator);
    return NULL;
  }
    
  
  stream=librdf_new_stream(storage->world,
                           (void*)iterator,
                           &librdf_storage_list_serialise_end_of_stream,
                           &librdf_storage_list_serialise_next_statement,
                           &librdf_storage_list_serialise_get_statement,
                           &librdf_storage_list_serialise_finished);
  if(!stream) {
    librdf_storage_list_serialise_finished((void*)iterator);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_storage_list_serialise_end_of_stream(void* context)
{
  librdf_iterator* iterator=(librdf_iterator*)context;

  return librdf_iterator_end(iterator);

}

static int
librdf_storage_list_serialise_next_statement(void* context)
{
  librdf_iterator* iterator=(librdf_iterator*)context;

  return librdf_iterator_next(iterator);
}


static void*
librdf_storage_list_serialise_get_statement(void* context, int flags)
{
  librdf_iterator* iterator=(librdf_iterator*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return (librdf_statement*)librdf_iterator_get_object(iterator);
    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return librdf_iterator_get_context(iterator);
    default:
      abort();
  }
}


static void
librdf_storage_list_serialise_finished(void* context)
{
  librdf_iterator* iterator=(librdf_iterator*)context;
  if(iterator)
    librdf_free_iterator(iterator);
}


static librdf_statement*
librdf_storage_list_find_map(void* context, librdf_statement* statement) 
{
  librdf_statement* partial_statement=(librdf_statement*)context;

  /* any statement matches when no partial statement is given */
  if(!partial_statement)
    return statement;
  
  if (librdf_statement_match(statement, partial_statement)) {
    return statement;
  }

  /* not suitable */
  return NULL;
}



/**
 * librdf_storage_list_find_statements:
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
static
librdf_stream* librdf_storage_list_find_statements(librdf_storage* storage, librdf_statement* statement)
{
  librdf_stream* stream;

  statement=librdf_new_statement_from_statement(statement);
  if(!statement)
    return NULL;
  
  stream=librdf_storage_list_serialise(storage);
  if(stream)
    librdf_stream_set_map(stream, &librdf_storage_list_find_map,
                          (librdf_stream_map_free_context_handler)&librdf_free_statement, (void*)statement);
  return stream;
}


/**
 * librdf_storage_list_group_add_statement - Add a statement to a storage group
 * @storage: &librdf_storage object
 * @group_uri: &librdf_uri object
 * @statement: &librdf_statement statement to add
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_list_group_add_statement(librdf_storage* storage,
                                        librdf_uri* group_uri,
                                        librdf_statement* statement) 
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;
  librdf_hash_datum key, value; /* on stack - not allocated */
  int size;
  int status;
  
  key.data=(char*)librdf_uri_as_string(group_uri);
  key.size=strlen(key.data);

  size=librdf_statement_encode(statement, NULL, 0);

  value.data=(char*)LIBRDF_MALLOC(cstring, size);
  value.size=librdf_statement_encode(statement, (unsigned char*)value.data, size);

  status=librdf_hash_put(context->groups, &key, &value);
  LIBRDF_FREE(data, value.data);

  return status;
}


/**
 * librdf_storage_list_group_remove_statement - Remove a statement from a storage group
 * @storage: &librdf_storage object
 * @group_uri: &librdf_uri object
 * @statement: &librdf_statement statement to remove
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_list_group_remove_statement(librdf_storage* storage, 
                                           librdf_uri* group_uri,
                                           librdf_statement* statement) 
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;
  librdf_hash_datum key, value; /* on stack - not allocated */
  int size;
  int status;
  
  key.data=(char*)librdf_uri_as_string(group_uri);
  key.size=strlen(key.data);

  size=librdf_statement_encode(statement, NULL, 0);

  value.data=(char*)LIBRDF_MALLOC(cstring, size);
  value.size=librdf_statement_encode(statement, (unsigned char*)value.data, size);

  status=librdf_hash_delete(context->groups, &key, &value);
  LIBRDF_FREE(data, value.data);
  
  return status;
}


typedef struct {
  librdf_iterator* iterator;
  librdf_hash_datum *key;
  librdf_hash_datum *value;
  librdf_statement current; /* static, shared statement */
} librdf_storage_list_group_serialise_stream_context;


/**
 * librdf_storage_list_group_serialise - List all statements in a storage group
 * @storage: &librdf_storage object
 * @group_uri: &librdf_uri object
 * 
 * Return value: &librdf_stream of statements or NULL on failure or group is empty
 **/
static librdf_stream*
librdf_storage_list_group_serialise(librdf_storage* storage,
                                    librdf_uri* group_uri) 
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;
  librdf_storage_list_group_serialise_stream_context* scontext;
  librdf_stream* stream;

  scontext=(librdf_storage_list_group_serialise_stream_context*)LIBRDF_CALLOC(librdf_storage_list_group_serialise_stream_context, 1, sizeof(librdf_storage_list_group_serialise_stream_context));
  if(!scontext)
    return NULL;

  librdf_statement_init(storage->world, &scontext->current);

  scontext->key=librdf_new_hash_datum(storage->world, NULL, 0);
  if(!scontext->key)
    return NULL;
  
  scontext->value=librdf_new_hash_datum(storage->world, NULL, 0);
  if(!scontext->value) {
    librdf_free_hash_datum(scontext->key);
    return NULL;
  }

  scontext->key->data=librdf_uri_as_string(group_uri);
  scontext->key->size=strlen(scontext->key->data);

  scontext->iterator=librdf_hash_get_all(context->groups, 
                                         scontext->key, scontext->value);
  if(!scontext->iterator) {
    librdf_storage_list_group_serialise_finished(scontext);
    return NULL;
  }


  stream=librdf_new_stream(storage->world,
                           (void*)scontext,
                           &librdf_storage_list_group_serialise_end_of_stream,
                           &librdf_storage_list_group_serialise_next_statement,
                           &librdf_storage_list_group_serialise_get_statement,
                           &librdf_storage_list_group_serialise_finished);
  if(!stream) {
    librdf_storage_list_group_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_storage_list_group_serialise_end_of_stream(void* context)
{
  librdf_storage_list_group_serialise_stream_context* scontext=(librdf_storage_list_group_serialise_stream_context*)context;

  return librdf_iterator_end(scontext->iterator);
}


static int
librdf_storage_list_group_serialise_next_statement(void* context)
{
  librdf_storage_list_group_serialise_stream_context* scontext=(librdf_storage_list_group_serialise_stream_context*)context;

  return librdf_iterator_next(scontext->iterator);
}


static void*
librdf_storage_list_group_serialise_get_statement(void* context, int flags)
{
  librdf_storage_list_group_serialise_stream_context* scontext=(librdf_storage_list_group_serialise_stream_context*)context;
  librdf_hash_datum* v;
  
  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      if(!(v=librdf_iterator_get_value(scontext->iterator)))
        return NULL;

      /* decode value content */
      if(!librdf_statement_decode(&scontext->current,
                                  (unsigned char*)v->data, v->size)) {
        return NULL;
      }

      return &scontext->current;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return librdf_iterator_get_context(scontext->iterator);
    default:
      abort();
  }
  
}


static void
librdf_storage_list_group_serialise_finished(void* context)
{
  librdf_storage_list_group_serialise_stream_context* scontext=(librdf_storage_list_group_serialise_stream_context*)context;
  
  if(scontext->iterator)
    librdf_free_iterator(scontext->iterator);

  if(scontext->key) {
    scontext->key->data=NULL;
    librdf_free_hash_datum(scontext->key);
  }
  if(scontext->value) {
    scontext->value->data=NULL;
    librdf_free_hash_datum(scontext->value);
  }

  librdf_statement_clear(&scontext->current);

  LIBRDF_FREE(librdf_storage_list_group_serialise_stream_context, scontext);
}



/* local function to register list storage functions */

static void
librdf_storage_list_register_factory(librdf_storage_factory *factory) 
{
  factory->context_length     = sizeof(librdf_storage_list_context);
  
  factory->init               = librdf_storage_list_init;
  factory->terminate          = librdf_storage_list_terminate;
  factory->open               = librdf_storage_list_open;
  factory->close              = librdf_storage_list_close;
  factory->size               = librdf_storage_list_size;
  factory->add_statement      = librdf_storage_list_add_statement;
  factory->add_statements     = librdf_storage_list_add_statements;
  factory->remove_statement   = librdf_storage_list_remove_statement;
  factory->contains_statement = librdf_storage_list_contains_statement;
  factory->serialise          = librdf_storage_list_serialise;
  factory->find_statements    = librdf_storage_list_find_statements;
  factory->group_add_statement    = librdf_storage_list_group_add_statement;
  factory->group_remove_statement = librdf_storage_list_group_remove_statement;
  factory->group_serialise        = librdf_storage_list_group_serialise;
}


void
librdf_init_storage_list(void)
{
  librdf_storage_register_factory("memory",
                                  &librdf_storage_list_register_factory);
}
