/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_hashes.c - RDF Storage as Hashes Implementation
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

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_storage.h>
#include <rdf_storage_hashes.h>
#include <rdf_hash.h>
#include <rdf_statement.h>
#include <rdf_stream.h>

typedef struct 
{
  char *name;
  int key_fields; /* OR of LIBRDF_STATEMENT_* fields defined in rdf_statement.h */
  int value_fields; /* ditto */
} librdf_hash_descriptor;


static const librdf_hash_descriptor librdf_storage_hashes_descriptions[3]= {
  {"sp2o",
   LIBRDF_STATEMENT_SUBJECT|LIBRDF_STATEMENT_PREDICATE,
   LIBRDF_STATEMENT_OBJECT},  /* For 'get targets' */
  {"po2s",
   LIBRDF_STATEMENT_PREDICATE|LIBRDF_STATEMENT_OBJECT,
   LIBRDF_STATEMENT_SUBJECT},  /* For 'get sources' */
  {"so2p", 
   LIBRDF_STATEMENT_SUBJECT|LIBRDF_STATEMENT_OBJECT,
   LIBRDF_STATEMENT_PREDICATE}   /* For 'get arcs' */
};


/* FIXME: Used when want a hash to use to (de)serialise to get all content 
 * In future if the above hashes don't contain all the content, this
 * may have to be computed.
*/
#define ANY_OLD_HASH_INDEX 0


typedef struct
{
  /* from init() argument */
  char         *name;
  /* from options */
  char         *hash_type;
  char         *db_dir;
  /* internals */
  int           hash_count;   /* how many hashes are present? STATIC 3 - FIXME */
  const librdf_hash_descriptor *hash_descriptions; /* points to STATIC hash_descriptions above - FIXME */
  char* names[3];                 /* names for hash open method */
  librdf_hash *options;           /* options for hash open */
  librdf_hash* hashes[3];         /* here they are in a STATIC array */
} librdf_storage_hashes_context;



/* prototypes for local functions */
static int librdf_storage_hashes_init(librdf_storage* storage, char *name, librdf_hash* options);
static int librdf_storage_hashes_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_hashes_close(librdf_storage* storage);
static int librdf_storage_hashes_size(librdf_storage* storage);
static int librdf_storage_hashes_add_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_hashes_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_hashes_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_hashes_contains_statement(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_hashes_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_hashes_find_statements(librdf_storage* storage, librdf_statement* statement);

/* serialising implementing functions */
static int librdf_storage_hashes_serialise_end_of_stream(void* context);
static librdf_statement* librdf_storage_hashes_serialise_next_statement(void* context);
static void librdf_storage_hashes_serialise_finished(void* context);


static void librdf_storage_hashes_register_factory(librdf_storage_factory *factory);



/* functions implementing storage api */
static int
librdf_storage_hashes_init(librdf_storage* storage, char *name,
                           librdf_hash* options)
{
  librdf_storage_hashes_context *context=(librdf_storage_hashes_context*)storage->context;
  const librdf_hash_descriptor *desc;
  int i;
  int status;

  context->hash_type=librdf_hash_get(options, "hash-type");
  if(!context->hash_type)
    return 1;

  librdf_hash_delete(options, "hash-type", 9); /* FIXME strlen("hash-type") */
  
  context->db_dir=librdf_hash_get(options, "dir");
  if(context->db_dir)
    librdf_hash_delete(options, "dir", 3); /* FIXME strlen("dir") */
  
  
  /* FIXME: STATIC 3 hashes */
  context->hash_count=3;
  desc=context->hash_descriptions=librdf_storage_hashes_descriptions;

  context->options=options;
  
  status=0;
  for(i=0; i<context->hash_count; i++) {
    int len;
    char *full_name;
    
    len=strlen(desc[i].name) + 1 + strlen(name) + 1; /* "%s-%s\0" */
    if(context->db_dir)
      len+=strlen(context->db_dir) +1;
      
    full_name=(char*)LIBRDF_MALLOC(cstring, len);
    if(!full_name) {
      status=1;
      break;
    }

    /* FIXME: Implies Unix filenames */
    if(context->db_dir)
      sprintf(full_name, "%s/%s-%s", context->db_dir, name, desc[i].name);
    else
      sprintf(full_name, "%s-%s", name, desc[i].name);

    context->hashes[i]=librdf_new_hash(context->hash_type);

    context->names[i]=full_name;
  }


  if(status) {
    for(i=0; i<context->hash_count; i++) {
      if(context->hashes[i]) {
        librdf_free_hash(context->hashes[i]);
        context->hashes[i]=NULL;
      }
    }
  }

  if(context->options) {
    librdf_free_hash(context->options);
    context->options=NULL;
  }
  
  return status;
}


static void
librdf_storage_hashes_terminate(librdf_storage* storage)
{
  librdf_storage_hashes_context *context=(librdf_storage_hashes_context*)storage->context;
  int i;
  
  for(i=0; i<context->hash_count; i++) {
    if(context->hashes[i])
      librdf_free_hash(context->hashes[i]);
    if(context->names[i])
      LIBRDF_FREE(cstring,context->names[i]);
  }

  if(context->options)
    librdf_free_hash(context->options);

  if(context->hash_type)
    LIBRDF_FREE(cstring, context->hash_type);

  if(context->db_dir)
    LIBRDF_FREE(cstring, context->db_dir);

}


static int
librdf_storage_hashes_open(librdf_storage* storage, librdf_model* model)
{
  librdf_storage_hashes_context *context=(librdf_storage_hashes_context*)storage->context;
  int i;
  
  for(i=0; i<context->hash_count; i++) {
    librdf_hash *hash=context->hashes[i];

    if(librdf_hash_open(hash, context->names[i], 
                        0644, 1, 1, /* FIXME - mode, is_writable, is_new */
                        context->options)) {
      /* I still have my "Structured Fortran" book */
      int j;
      for (j=0; j<i; j++)
        librdf_hash_close(context->hashes[j]);
    
      return 1;
    }
  }

  return 0;
}


/**
 * librdf_storage_hashes_close:
 * @storage: 
 * 
 * Close the storage hashes storage, and free all content since there is no 
 * persistance.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_hashes_close(librdf_storage* storage)
{
  librdf_storage_hashes_context* context=(librdf_storage_hashes_context*)storage->context;
  int i;
  
  for(i=0; i<context->hash_count; i++) {
    if(context->hashes[i])
      librdf_hash_close(context->hashes[i]);
  }
  
  return 0;
}


static int
librdf_storage_hashes_size(librdf_storage* storage)
{
  /* FIXME - is this possbile to get cheaply from any disk hash ? */
  LIBRDF_FATAL1(librdf_storage_hashes_size, "Not implemented\n");
  return -1;
}


static int
librdf_storage_hashes_add_statement(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_hashes_context* context=(librdf_storage_hashes_context*)storage->context;
  int i;
  int status=0;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG1(librdf_storage_hashes_add_statement, "Adding statement: ");
  librdf_statement_print(statement, stderr);
  fputc('\n', stderr);
#endif  

  for(i=0; i<context->hash_count; i++) {
    char *key_buffer, *value_buffer;
    int key_len, value_len;

    /* ENCODE KEY */

    int fields=context->hash_descriptions[i].key_fields;
    
    key_len=librdf_statement_encode_parts(statement, NULL, 0, fields);
    if(!key_len)
      return 1;
    if(!(key_buffer=(char*)LIBRDF_MALLOC(data, key_len))) {
      status=1;
      break;
    }
       
    if(!librdf_statement_encode_parts(statement, key_buffer, key_len,
                                      fields)) {
      LIBRDF_FREE(data, key_buffer);
      status=1;
      break;
    }

    
    /* ENCODE VALUE */
    
    fields=context->hash_descriptions[i].value_fields;

    value_len=librdf_statement_encode_parts(statement, NULL, 0, fields);
    if(!value_len) {
      LIBRDF_FREE(data, key_buffer);
      status=1;
      break;
    }
    
    if(!(value_buffer=(char*)LIBRDF_MALLOC(data, value_len))) {
      LIBRDF_FREE(data, key_buffer);
      status=1;
      break;
    }

       
    if(!librdf_statement_encode_parts(statement, value_buffer, value_len,
                                      fields)) {
      LIBRDF_FREE(data, key_buffer);
      LIBRDF_FREE(data, value_buffer);
      status=1;
      break;
    }


#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG4(librdf_storage_hashes_add_statement, "Writing %s hash key %d bytes -> value %d bytes\n", context->hash_descriptions[i].name, key_len, value_len);
#endif

    /* Finally, store the sucker */
    status=librdf_hash_put(context->hashes[i], key_buffer, key_len,
                           value_buffer, value_len);
    
    LIBRDF_FREE(data, key_buffer);
    LIBRDF_FREE(data, value_buffer);

    if(status)
      break;
  }

  /* FIXME: if one of the above hash things fails, the add statement
   * operation is only partially complete (but reported as a failure)
   */

  /* Free it since it is stored and ównership passed in here */
  librdf_free_statement(statement);

  return status;
}


static int
librdf_storage_hashes_add_statements(librdf_storage* storage,
                                     librdf_stream* statement_stream)
{
  int status=0;

  while(!librdf_stream_end(statement_stream)) {
    librdf_statement* statement=librdf_stream_next(statement_stream);

    if(statement) {
      status=librdf_storage_hashes_add_statement(storage, statement);
      if(status)
        librdf_free_statement(statement);
    } else
      status=1;

    if(status)
      break;
  }

  librdf_free_stream(statement_stream);

  return status;
}


static int
librdf_storage_hashes_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  LIBRDF_FATAL1(librdf_storage_hashes_remove_statement, "Not implemented\n");
}


static int
librdf_storage_hashes_contains_statement(librdf_storage* storage, librdf_statement* statement)
{
  LIBRDF_FATAL1(librdf_storage_hashes_contains_statement, "Not implemented\n");
}



typedef struct {
  librdf_storage_hashes_context *hash_context;
  int index;
  librdf_iterator* iterator;
  librdf_hash_datum *key;
  librdf_hash_datum *value;
} librdf_storage_hashes_serialise_stream_context;


static librdf_stream*
librdf_storage_hashes_serialise(librdf_storage* storage)
{
  librdf_storage_hashes_context *context=(librdf_storage_hashes_context*)storage->context;
  librdf_storage_hashes_serialise_stream_context *scontext;
  librdf_hash *hash;
  librdf_stream *stream;
  
  scontext=(librdf_storage_hashes_serialise_stream_context*)LIBRDF_CALLOC(librdf_storage_hashes_serialise_stream_context, 1, sizeof(librdf_storage_hashes_serialise_stream_context));
  if(!scontext)
    return NULL;

  scontext->hash_context=context;
  scontext->index=ANY_OLD_HASH_INDEX;

  hash=context->hashes[scontext->index];

  scontext->key=librdf_new_hash_datum(NULL, 0);
  if(!scontext->key)
    return NULL;
  
  scontext->value=librdf_new_hash_datum(NULL, 0);
  if(!scontext->value) {
    librdf_free_hash_datum(scontext->key);
    return NULL;
  }

  scontext->iterator=librdf_hash_get_all(hash,
                                         scontext->key, scontext->value);
  if(!scontext->iterator) {
    librdf_storage_hashes_serialise_finished((void*)scontext);
    return NULL;
  }


  stream=librdf_new_stream((void*)scontext,
                           &librdf_storage_hashes_serialise_end_of_stream,
                           &librdf_storage_hashes_serialise_next_statement,
                           &librdf_storage_hashes_serialise_finished);
  if(!stream) {
    librdf_storage_hashes_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  

}


static int
librdf_storage_hashes_serialise_end_of_stream(void* context)
{
  librdf_storage_hashes_serialise_stream_context* scontext=(librdf_storage_hashes_serialise_stream_context*)context;

  return !librdf_iterator_have_elements(scontext->iterator);
}


static librdf_statement*
librdf_storage_hashes_serialise_next_statement(void* context)
{
  librdf_storage_hashes_serialise_stream_context* scontext=(librdf_storage_hashes_serialise_stream_context*)context;
  librdf_statement* statement;
  
  statement=librdf_new_statement();
  if(!statement)
    return NULL;

  if(!librdf_iterator_get_next(scontext->iterator))
    return NULL;

  /* decode key content */
  if(!librdf_statement_decode(statement, 
                              scontext->key->data, scontext->key->size)) {
    librdf_free_statement(statement);
    return NULL;
  }

  /* decode value content */
  if(!librdf_statement_decode(statement, 
                              scontext->value->data, scontext->value->size)) {
    librdf_free_statement(statement);
    return NULL;
  }

  return statement;
}


static void
librdf_storage_hashes_serialise_finished(void* context)
{
  librdf_storage_hashes_serialise_stream_context* scontext=(librdf_storage_hashes_serialise_stream_context*)context;
  
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

  LIBRDF_FREE(librdf_storage_hashes_serialise_stream_context, scontext);
}


static librdf_statement*
librdf_storage_hashes_find_map(void* context, librdf_statement* statement) 
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
 * librdf_storage_hashes_find_statements:
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
librdf_stream* librdf_storage_hashes_find_statements(librdf_storage* storage, librdf_statement* statement)
{
  librdf_stream* stream;

  stream=librdf_storage_hashes_serialise(storage);
  if(stream)
    librdf_stream_set_map(stream, &librdf_storage_hashes_find_map, (void*)statement);
  return stream;
}



/* local function to register hashes storage functions */

static void
librdf_storage_hashes_register_factory(librdf_storage_factory *factory) 
{
  factory->context_length     = sizeof(librdf_storage_hashes_context);
  
  factory->init               = librdf_storage_hashes_init;
  factory->terminate          = librdf_storage_hashes_terminate;
  factory->open               = librdf_storage_hashes_open;
  factory->close              = librdf_storage_hashes_close;
  factory->size               = librdf_storage_hashes_size;
  factory->add_statement      = librdf_storage_hashes_add_statement;
  factory->add_statements     = librdf_storage_hashes_add_statements;
  factory->remove_statement   = librdf_storage_hashes_remove_statement;
  factory->contains_statement = librdf_storage_hashes_contains_statement;
  factory->serialise          = librdf_storage_hashes_serialise;
  factory->find_statements    = librdf_storage_hashes_find_statements;
}


void
librdf_init_storage_hashes(void)
{
  librdf_storage_register_factory("hashes",
                                  &librdf_storage_hashes_register_factory);
}
