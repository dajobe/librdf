/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_list.c - RDF Storage in memory as a list implementation
 *
 * $Id$
 *
 * Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology, University of Bristol.
 *
 *    This package is Free Software available under either of two licenses
 *    (see FAQS.html to see why):
 * 
 * 1. The GNU Lesser General Public License (LGPL)
 * 
 *    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
 *    full license text.
 *      _________________________________________________________________
 * 
 *      Copyright (C) 2000 David Beckett, Institute for Learning and
 *      Research Technology, University of Bristol. All Rights Reserved.
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public License
 *      as published by the Free Software Foundation; either version 2 of
 *      the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *      USA
 *      _________________________________________________________________
 * 
 *    NOTE - under Term 3 of the LGPL, you may choose to license the entire
 *    library under the GPL. See COPYING for the full license text.
 * 
 * 2. The Mozilla Public License
 * 
 *    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
 *    license text.
 * 
 *    Under MPL section 13. I declare that all of the Covered Code is
 *    Multiple Licensed:
 *      _________________________________________________________________
 * 
 *      The contents of this file are subject to the Mozilla Public License
 *      version 1.1 (the "License"); you may not use this file except in
 *      compliance with the License. You may obtain a copy of the License
 *      at http://www.mozilla.org/MPL/
 * 
 *      Software distributed under the License is distributed on an "AS IS"
 *      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 *      the License for the specific language governing rights and
 *      limitations under the License.
 * 
 *      The Initial Developer of the Original Code is David Beckett.
 *      Portions created by David Beckett are Copyright (C) 2000 David
 *      Beckett, Institute for Learning and Research Technology, University
 *      of Bristol. All Rights Reserved.
 * 
 *      Alternatively, the contents of this file may be used under the
 *      terms of the GNU Lesser General Public License, in which case the
 *      provisions of the LGPL License are applicable instead of those
 *      above. If you wish to allow use of your version of this file only
 *      under the terms of the LGPL License and not to allow others to use
 *      your version of this file under the MPL, indicate your decision by
 *      deleting the provisions above and replace them with the notice and
 *      other provisions required by the LGPL License. If you do not delete
 *      the provisions above, a recipient may use your version of this file
 *      under either the MPL or the LGPL License.
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
