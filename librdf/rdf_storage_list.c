/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_list.c - RDF Storage List Interface Implementation
 *
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */


#include <rdf_config.h>

#include <sys/types.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h> /* for memcmp */
#endif

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_storage.h>
#include <rdf_storage_list.h>
#include <rdf_list.h>
#include <rdf_statement.h>
#include <rdf_stream.h>


typedef struct
{
  librdf_list* list;
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
static librdf_statement* librdf_storage_list_serialise_next_statement(void* context);
static void librdf_storage_list_serialise_finished(void* context);


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

  context->list=librdf_new_list();
  if(!context->list)
    return 1;

  librdf_list_set_equals(context->list, 
                         (int (*)(void*, void*))&librdf_statement_equals);
  return 0;
}


/**
 * librdf_storage_list_close:
 * @storage: 
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
      while(librdf_iterator_have_elements(iterator)) {
        statement=(librdf_statement*)librdf_iterator_get_next(iterator);
        if(statement)
          librdf_free_statement(statement);
      }
      librdf_free_iterator(iterator);
    }
    librdf_free_list(context->list);

    context->list=NULL;
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

  return librdf_list_add(context->list, statement);
}


static int
librdf_storage_list_add_statements(librdf_storage* storage,
                                   librdf_stream* statement_stream)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;
  librdf_statement* statement;
  int status=0;

  while(!librdf_stream_end(statement_stream)) {
    statement=librdf_stream_next(statement_stream);
    if(statement)
      librdf_list_add(context->list, statement);
    else
      status=1;
  }
  librdf_free_stream(statement_stream);
  
  return status;
}


static int
librdf_storage_list_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_list_context* context=(librdf_storage_list_context*)storage->context;

  return librdf_list_remove(context->list, statement);
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
    
  
  stream=librdf_new_stream((void*)iterator,
                           &librdf_storage_list_serialise_end_of_stream,
                           &librdf_storage_list_serialise_next_statement,
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

  return !librdf_iterator_have_elements(iterator);

}

static librdf_statement*
librdf_storage_list_serialise_next_statement(void* context)
{
  librdf_iterator* iterator=(librdf_iterator*)context;
  librdf_statement* statement=(librdf_statement*)librdf_iterator_get_next(iterator);
  if(!statement)
    return NULL;
  
  return librdf_new_statement_from_statement(statement);
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

  /* discard */
  librdf_free_statement(statement);
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

  stream=librdf_storage_list_serialise(storage);
  if(stream)
    librdf_stream_set_map(stream, &librdf_storage_list_find_map,
                          (void*)statement);
  return stream;
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
}


void
librdf_init_storage_list(void)
{
  librdf_storage_register_factory("memory",
                                  &librdf_storage_list_register_factory);
}
