/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_hashes.c - RDF Storage as Hashes Implementation
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

#include <sys/types.h>

#include <stdio.h>

#include <librdf.h>
#include <rdf_storage.h>
#include <rdf_storage_hashes.h>
#include <rdf_hash.h>
#include <rdf_statement.h>
#include <rdf_stream.h>
#include <rdf_heuristics.h>

typedef struct 
{
  char *name;
  int key_fields; /* OR of LIBRDF_STATEMENT_* fields defined in rdf_statement.h */
  int value_fields; /* ditto */
} librdf_hash_descriptor;


#ifdef LIBRDF_STORAGE_HASH_GROUPS
/* FIXME: STATIC */
#define NUMBER_OF_HASHES 4
#else
#define NUMBER_OF_HASHES 3
#endif

static const librdf_hash_descriptor librdf_storage_hashes_descriptions[NUMBER_OF_HASHES]= {
  {"sp2o",
   LIBRDF_STATEMENT_SUBJECT|LIBRDF_STATEMENT_PREDICATE,
   LIBRDF_STATEMENT_OBJECT},  /* For 'get targets' */
  {"po2s",
   LIBRDF_STATEMENT_PREDICATE|LIBRDF_STATEMENT_OBJECT,
   LIBRDF_STATEMENT_SUBJECT},  /* For 'get sources' */
  {"so2p", 
   LIBRDF_STATEMENT_SUBJECT|LIBRDF_STATEMENT_OBJECT,
   LIBRDF_STATEMENT_PREDICATE}  /* For 'get arcs' */
#ifdef LIBRDF_STORAGE_HASH_GROUPS
  ,{"groups",
   0L, /* for groups - do not touch when storing statements! */
   0L}
#endif
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
  /* from options decoded from options hash during init() */
  char         *hash_type;
  char         *db_dir;
  int           mode;
  int           is_writable;
  int           is_new;
  librdf_hash  *options;     /* remaining options for hash open method */

  /* internals */
  int           hash_count; /* how many hashes are present? STATIC */
  librdf_hash* hashes[NUMBER_OF_HASHES]; /* here they are in a STATIC array */
  const librdf_hash_descriptor *hash_descriptions; /* points to STATIC hash_descriptions above - FIXME */
  char* names[NUMBER_OF_HASHES];       /* hash names for hash open method */

  int sources_index;
  int arcs_index;
  int targets_index;

#ifdef LIBRDF_STORAGE_HASH_GROUPS
  int groups_index;
#endif
} librdf_storage_hashes_context;



/* helper function for implementing init and clone methods */
static int librdf_storage_hashes_init_common(librdf_storage* storage, char *name, char *hash_type, char *db_dir, int mode, int is_writable, int is_new, librdf_hash* options);


/* prototypes for local functions */
static int librdf_storage_hashes_init(librdf_storage* storage, char *name, librdf_hash* options);
static void librdf_storage_hashes_terminate(librdf_storage* storage);
static int librdf_storage_hashes_clone(librdf_storage* new_storage, librdf_storage* old_storage);
static int librdf_storage_hashes_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_hashes_close(librdf_storage* storage);
static int librdf_storage_hashes_size(librdf_storage* storage);
static int librdf_storage_hashes_add_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_hashes_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_hashes_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_hashes_contains_statement(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_hashes_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_hashes_find_statements(librdf_storage* storage, librdf_statement* statement);
static librdf_iterator* librdf_storage_hashes_find_sources(librdf_storage* storage, librdf_node* arc, librdf_node *target);
static librdf_iterator* librdf_storage_hashes_find_arcs(librdf_storage* storage, librdf_node* source, librdf_node *target);
static librdf_iterator* librdf_storage_hashes_find_targets(librdf_storage* storage, librdf_node* source, librdf_node *arc);

/* serialising implementing functions */
static int librdf_storage_hashes_serialise_end_of_stream(void* context);
static librdf_statement* librdf_storage_hashes_serialise_next_statement(void* context);
static void librdf_storage_hashes_serialise_finished(void* context);

/* group functions */
static int librdf_storage_hashes_group_add_statement(librdf_storage* storage, librdf_uri* group_uri, librdf_statement* statement);
static int librdf_storage_hashes_group_remove_statement(librdf_storage* storage, librdf_uri* group_uri, librdf_statement* statement);
static librdf_stream* librdf_storage_hashes_group_serialise(librdf_storage* storage, librdf_uri* group_uri);

/* group list statement stream methods */
static int librdf_storage_hashes_group_serialise_end_of_stream(void* context);
static librdf_statement* librdf_storage_hashes_group_serialise_next_statement(void* context);
static void librdf_storage_hashes_group_serialise_finished(void* context);


static void librdf_storage_hashes_register_factory(librdf_storage_factory *factory);


/* node iterator implementing functions for get sources, targets, arcs methods */
static int librdf_storage_hashes_node_iterator_is_end(void* iterator);
static void* librdf_storage_hashes_node_iterator_get_next(void* iterator);
static void librdf_storage_hashes_node_iterator_finished(void* iterator);
/* common initialisation code for creating get sources, targets, arcs iterators */
static librdf_iterator* librdf_storage_hashes_node_iterator_create(librdf_storage* storage, librdf_node* node1, librdf_node *node2, int hash_index, int want);



/* helper function for implementing init and clone methods */

static int
librdf_storage_hashes_init_common(librdf_storage* storage, char *name,
                                  char *hash_type, char *db_dir,
                                  int mode, int is_writable, int is_new,
                                  librdf_hash* options)
{
  librdf_storage_hashes_context *context=(librdf_storage_hashes_context*)storage->context;
  const librdf_hash_descriptor *desc;
  int i;
  int status;
  
  context->hash_type=hash_type;
  context->db_dir=db_dir;

  context->mode=mode;
  context->is_writable=is_writable;
  context->is_new=is_new;

  context->hash_count=NUMBER_OF_HASHES;
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

    context->hashes[i]=librdf_new_hash(storage->world, context->hash_type);
    if(!context->hashes[i]) {
      status=1;
      break;
    }

    context->names[i]=full_name;
  }

  /* find indexes for get targets, sources and arcs */
  context->sources_index= -1;
  context->arcs_index= -1;
  context->targets_index= -1;
#ifdef LIBRDF_STORAGE_HASH_GROUPS
  /* and index for groups (no key or value fields) */
  context->groups_index= -1;
#endif
  for(i=0; i<context->hash_count; i++) {
    int key_fields=context->hash_descriptions[i].key_fields;
    int value_fields=context->hash_descriptions[i].value_fields;
    
    if(key_fields == (LIBRDF_STATEMENT_SUBJECT|LIBRDF_STATEMENT_PREDICATE) &&
       value_fields == LIBRDF_STATEMENT_OBJECT) {
      context->targets_index=i;
    } else if(key_fields == (LIBRDF_STATEMENT_PREDICATE|LIBRDF_STATEMENT_OBJECT) &&
              value_fields == LIBRDF_STATEMENT_SUBJECT) {
      context->sources_index=i;
    } else if(key_fields == (LIBRDF_STATEMENT_SUBJECT|LIBRDF_STATEMENT_OBJECT) &&
              value_fields == LIBRDF_STATEMENT_PREDICATE) {
      context->arcs_index=i;
    } else if(!key_fields || !value_fields) {
#ifdef LIBRDF_STORAGE_HASH_GROUPS
       context->groups_index=i;
#endif
    }
  }


  if(status) {
    for(i=0; i<context->hash_count; i++) {
      if(context->hashes[i]) {
        librdf_free_hash(context->hashes[i]);
        context->hashes[i]=NULL;
      }
    }
  }
  
  /* on success or failure - don't need the passed in options */
  if(context->options) {
    librdf_free_hash(context->options);
    context->options=NULL;
  }
  
  return status;
}


/* functions implementing storage api */
static int
librdf_storage_hashes_init(librdf_storage* storage, char *name,
                           librdf_hash* options)
{
  char *hash_type, *db_dir;
  int mode, is_writable, is_new;
  
  if(!options)
    return 1;

  hash_type=librdf_hash_get_del(options, "hash-type");
  if(!hash_type)
    return 1;

  db_dir=librdf_hash_get_del(options, "dir");
  
  if((mode=librdf_hash_get_as_long(options, "mode"))<0)
    mode=0644; /* default mode */
  
  if((is_writable=librdf_hash_get_as_boolean(options, "write"))<0)
    is_writable=1; /* default is WRITABLE */
  
  if((is_new=librdf_hash_get_as_boolean(options, "new"))<0)
    is_new=0; /* default is NOT NEW */
  

  return librdf_storage_hashes_init_common(storage, name, 
                                           hash_type, db_dir,
                                           mode, is_writable, is_new, 
                                           options);
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
librdf_storage_hashes_clone(librdf_storage* new_storage, librdf_storage* old_storage)
{
  librdf_storage_hashes_context *old_context=(librdf_storage_hashes_context*)old_storage->context;
  librdf_storage_hashes_context *new_context=(librdf_storage_hashes_context*)new_storage->context;
  librdf_hash *options;
  
  new_context->name=librdf_heuristic_gen_name(old_context->name);
  if(!new_context->name)
    return 1;

  /* This is always a copy of an in-memory hash */
  options=librdf_new_hash_from_hash(old_context->options);
  if(!options) {
    LIBRDF_FREE(cstring, new_context->name);
    return 1;
  }

  if(librdf_storage_hashes_init_common(new_storage, new_context->name,
                                       old_context->hash_type,
                                       old_context->db_dir,
                                       old_context->mode,
                                       old_context->is_writable,
                                       old_context->is_new,
                                       options)) {
    librdf_free_hash(options);
    LIBRDF_FREE(cstring, new_context->name);
    return 1;
  }

  return 0;
}
 

static int
librdf_storage_hashes_open(librdf_storage* storage, librdf_model* model)
{
  librdf_storage_hashes_context *context=(librdf_storage_hashes_context*)storage->context;
  int i;
  
  for(i=0; i<context->hash_count; i++) {
    librdf_hash *hash=context->hashes[i];

    if(!hash ||
       librdf_hash_open(hash, context->names[i], 
                        context->mode, context->is_writable, context->is_new,
                        context->options)) {
      /* I still have my "Structured Fortran" book */
      int j;
      for (j=0; j<i; j++) {
        librdf_hash_close(context->hashes[j]);
        context->hashes[j]=NULL;
      }
      
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
  librdf_storage_hashes_context* context=(librdf_storage_hashes_context*)storage->context;
  librdf_hash* any_hash=context->hashes[ANY_OLD_HASH_INDEX];

  if(!any_hash)
    return -1;

  return librdf_hash_values_count(any_hash);
}


static int
librdf_storage_hashes_add_remove_statement(librdf_storage* storage, 
                                           librdf_statement* statement,
                                           int is_addition)
{
  librdf_storage_hashes_context* context=(librdf_storage_hashes_context*)storage->context;
  int i;
  int status=0;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  if(is_addition)
    LIBRDF_DEBUG1(librdf_storage_hashes_add_remove_statement, "Adding statement: ");
  else
    LIBRDF_DEBUG1(librdf_storage_hashes_add_remove_statement, "Removing statement: ");
  librdf_statement_print(statement, stderr);
  fputc('\n', stderr);
#endif  

  for(i=0; i<context->hash_count; i++) {
    librdf_hash_datum hd_key, hd_value; /* on stack */
    unsigned char *key_buffer, *value_buffer;
    int key_len, value_len;

    /* ENCODE KEY */

    int fields=context->hash_descriptions[i].key_fields;
    if(!fields)
      continue;
    
    key_len=librdf_statement_encode_parts(statement, NULL, 0, fields);
    if(!key_len)
      return 1;
    if(!(key_buffer=(unsigned char*)LIBRDF_MALLOC(data, key_len))) {
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
    if(!fields)
      continue;
    
    value_len=librdf_statement_encode_parts(statement, NULL, 0, fields);
    if(!value_len) {
      LIBRDF_FREE(data, key_buffer);
      status=1;
      break;
    }
    
    if(!(value_buffer=(unsigned char*)LIBRDF_MALLOC(data, value_len))) {
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
    LIBRDF_DEBUG4(librdf_storage_hashes_add_statement, "Using %s hash key %d bytes -> value %d bytes\n", context->hash_descriptions[i].name, key_len, value_len);
#endif

    /* Finally, store / remove the sucker */
    hd_key.data=key_buffer; hd_key.size=key_len;
    hd_value.data=value_buffer; hd_value.size=value_len;
    
    if(is_addition)
      status=librdf_hash_put(context->hashes[i], &hd_key, &hd_value);
    else
      status=librdf_hash_delete(context->hashes[i], &hd_key, &hd_value);
    
    LIBRDF_FREE(data, key_buffer);
    LIBRDF_FREE(data, value_buffer);

    if(status)
      break;
  }

  return status;
}


static int
librdf_storage_hashes_add_statement(librdf_storage* storage, librdf_statement* statement)
{
  return librdf_storage_hashes_add_remove_statement(storage, statement, 1);
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
  return librdf_storage_hashes_add_remove_statement(storage, statement, 0);
}


static int
librdf_storage_hashes_contains_statement(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_hashes_context* context=(librdf_storage_hashes_context*)storage->context;
  librdf_hash_datum hd_key, hd_value; /* on stack */
  unsigned char *key_buffer, *value_buffer;
  int key_len, value_len;
  int hash_index=ANY_OLD_HASH_INDEX;
  int fields;
  int status;
  
  /* ENCODE KEY */
  fields=context->hash_descriptions[hash_index].key_fields;
  key_len=librdf_statement_encode_parts(statement, NULL, 0, fields);
  if(!key_len)
    return 1;
  if(!(key_buffer=(unsigned char*)LIBRDF_MALLOC(data, key_len)))
    return 1;
       
  if(!librdf_statement_encode_parts(statement, key_buffer, key_len,
                                    fields)) {
    LIBRDF_FREE(data, key_buffer);
    return 1;
  }

  /* ENCODE VALUE */
  fields=context->hash_descriptions[hash_index].value_fields;
  value_len=librdf_statement_encode_parts(statement, NULL, 0, fields);
  if(!value_len) {
    LIBRDF_FREE(data, key_buffer);
    return 1;
  }
    
  if(!(value_buffer=(unsigned char*)LIBRDF_MALLOC(data, value_len))) {
    LIBRDF_FREE(data, key_buffer);
    return 1;
  }

       
  if(!librdf_statement_encode_parts(statement, value_buffer, value_len,
                                    fields)) {
    LIBRDF_FREE(data, key_buffer);
    LIBRDF_FREE(data, value_buffer);
    return 1;
  }


#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG4(librdf_storage_hashes_contains_statement, "Using %s hash key %d bytes -> value %d bytes\n", context->hash_descriptions[hash_index].name, key_len, value_len);
#endif

  hd_key.data=key_buffer; hd_key.size=key_len;
  hd_value.data=value_buffer; hd_value.size=value_len;
  status=librdf_hash_exists(context->hashes[hash_index], &hd_key, &hd_value);
  
  LIBRDF_FREE(data, key_buffer);
  LIBRDF_FREE(data, value_buffer);

  /* DO NOT free statement, ownership was not passed in */
  return status;
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

  scontext->key=librdf_new_hash_datum(storage->world, NULL, 0);
  if(!scontext->key)
    return NULL;
  
  scontext->value=librdf_new_hash_datum(storage->world, NULL, 0);
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


  stream=librdf_new_stream(storage->world,
                           (void*)scontext,
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

  return librdf_iterator_end(scontext->iterator);
}


static librdf_statement*
librdf_storage_hashes_serialise_next_statement(void* context)
{
  librdf_storage_hashes_serialise_stream_context* scontext=(librdf_storage_hashes_serialise_stream_context*)context;
  librdf_statement* statement;
  
  statement=librdf_new_statement(scontext->iterator->world);
  if(!statement)
    return NULL;

  if(!librdf_iterator_get_next(scontext->iterator))
    return NULL;

  /* decode key content */
  if(!librdf_statement_decode(statement, 
                              (unsigned char*)scontext->key->data, 
                              scontext->key->size)) {
    librdf_free_statement(statement);
    return NULL;
  }

  /* decode value content */
  if(!librdf_statement_decode(statement, 
                              (unsigned char*)scontext->value->data,
                              scontext->value->size)) {
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


typedef struct {
  librdf_storage* storage;   /* (shared) pointer to storage */
  int hash_index;            /* index of hash in storage list of hashes */
  librdf_iterator* iterator; /* owned iterator over above hash */
  int want;                  /* part of decoded statement to return */
  librdf_statement statement; /* NOTE: stored here, never allocated */
  librdf_hash_datum key;
  librdf_hash_datum value;
} librdf_storage_hashes_node_iterator_context;


static int
librdf_storage_hashes_node_iterator_is_end(void* iterator)
{
  librdf_storage_hashes_node_iterator_context* context=(librdf_storage_hashes_node_iterator_context*)iterator;

  return librdf_iterator_end(context->iterator);
}


static void*
librdf_storage_hashes_node_iterator_get_next(void* iterator) 
{
  librdf_storage_hashes_node_iterator_context* context=(librdf_storage_hashes_node_iterator_context*)iterator;
  librdf_node* node;

  if(librdf_iterator_end(context->iterator))
    return NULL;

  /* this side effects the already initialised librdf_hash_datum
   * objects in the context.  It guarantees to return a value
   * if there are elements.
   */
  librdf_iterator_get_next(context->iterator);

  if(!librdf_statement_decode(&context->statement, 
                              (unsigned char*)context->value.data,
                              context->value.size))
    return NULL;

  switch(context->want) {
    case LIBRDF_STATEMENT_SUBJECT: /* SOURCES (subjects) */
      node=librdf_statement_get_subject(&context->statement);
      librdf_statement_set_subject(&context->statement, NULL);
      break;
      
    case LIBRDF_STATEMENT_PREDICATE: /* ARCS (predicates) */
      node=librdf_statement_get_predicate(&context->statement);
      librdf_statement_set_predicate(&context->statement, NULL);
      break;
      
    case LIBRDF_STATEMENT_OBJECT: /* TARGETS (objects) */
      node=librdf_statement_get_object(&context->statement);
      librdf_statement_set_object(&context->statement, NULL);
      break;
      
    default: /* error */
      LIBRDF_FATAL2(librdf_storage_hashes_node_iterator_get_next,
                    "Illegal statement part %d seen\n", context->want);
  }
  
  return (void*)node;
}


static void
librdf_storage_hashes_node_iterator_finished(void* iterator) 
{
  librdf_storage_hashes_node_iterator_context* icontext=(librdf_storage_hashes_node_iterator_context*)iterator;

  if(icontext->iterator)
    librdf_free_iterator(icontext->iterator);

  LIBRDF_FREE(librdf_storage_hashes_node_iterator_context, icontext);
}


/*
 * librdf_storage_hashes_node_iterator_create - Create a node iterator for get sources, targets or arcs methods
 * @storage: the storage hashes object to iterate
 * @node1: the first node to encode in the key
 * @node2: the second node to encode in the key
 * @hash_index: the index of the hash to iterate over
 * @want: the field required from the hash value
 * 
 * Return value: a new &librdf_iterator or NULL on failure
 **/
static librdf_iterator*
librdf_storage_hashes_node_iterator_create(librdf_storage* storage, 
                                           librdf_node* node1,
                                           librdf_node *node2,
                                           int hash_index,
                                           int want) 
{
  librdf_storage_hashes_context *scontext=(librdf_storage_hashes_context*)storage->context;
  librdf_storage_hashes_node_iterator_context* icontext;
  librdf_iterator *iterator;
  librdf_hash *hash;
  int fields;
  unsigned char *key_buffer;
  
  icontext=(librdf_storage_hashes_node_iterator_context*)LIBRDF_CALLOC(librdf_storage_hashes_node_iterator_context, 1, sizeof(librdf_storage_hashes_node_iterator_context));
  if(!icontext)
    return NULL;

  icontext->storage=storage;
  icontext->hash_index=hash_index;
  icontext->want=want;

  librdf_statement_init(storage->world, &icontext->statement);

  hash=scontext->hashes[icontext->hash_index];

  /* set the fields in the static statement contained in the context */
  switch(icontext->want) {
    case LIBRDF_STATEMENT_SUBJECT: /* SOURCES (subjects) */
      librdf_statement_set_predicate(&icontext->statement, node1);
      librdf_statement_set_object(&icontext->statement, node2);
      break;
      
    case LIBRDF_STATEMENT_PREDICATE: /* PREDICATES (arcs) */
      librdf_statement_set_subject(&icontext->statement, node1);
      librdf_statement_set_object(&icontext->statement, node2);
      break;
      
    case LIBRDF_STATEMENT_OBJECT: /* OBJECTS (targets) */
      librdf_statement_set_subject(&icontext->statement, node1);
      librdf_statement_set_predicate(&icontext->statement, node2);
      break;
      
    default: /* error */
      LIBRDF_FATAL2(librdf_storage_hashes_node_iterator_create,
                    "Illegal statement part %d seen\n", icontext->want);
  }


  /* ENCODE KEY */
  fields=scontext->hash_descriptions[hash_index].key_fields;
  icontext->key.size=librdf_statement_encode_parts(&icontext->statement, 
                                                   NULL, 0, fields);
  if(!icontext->key.size) {
    LIBRDF_FREE(librdf_storage_hashes_node_iterator_context, icontext);
    return NULL;
  }
  if(!(key_buffer=(unsigned char*)LIBRDF_MALLOC(data, icontext->key.size))) {
    LIBRDF_FREE(librdf_storage_hashes_node_iterator_context, icontext);
    return NULL;
  }
       
  if(!librdf_statement_encode_parts(&icontext->statement, 
                                    key_buffer, icontext->key.size,
                                    fields)) {
    LIBRDF_FREE(data, key_buffer);
    librdf_storage_hashes_node_iterator_finished(icontext);
    return NULL;
  }

    
  icontext->key.data=key_buffer;

  icontext->iterator=librdf_hash_get_all(hash, &icontext->key, &icontext->value);
  if(!icontext->iterator) {
    LIBRDF_FREE(data, key_buffer);
    librdf_storage_hashes_node_iterator_finished(icontext);
    return NULL;
  }

  LIBRDF_FREE(data, key_buffer);


  iterator=librdf_new_iterator(storage->world,
                               (void*)icontext,
                               librdf_storage_hashes_node_iterator_is_end,
                               librdf_storage_hashes_node_iterator_get_next,
                               librdf_storage_hashes_node_iterator_finished);
  if(!iterator)
    librdf_storage_hashes_node_iterator_finished(icontext);

  return iterator;
}


static librdf_iterator*
librdf_storage_hashes_find_sources(librdf_storage* storage, 
                                   librdf_node* arc, librdf_node *target) 
{
  librdf_storage_hashes_context *scontext=(librdf_storage_hashes_context*)storage->context;
  return librdf_storage_hashes_node_iterator_create(storage, arc, target,
                                                    scontext->sources_index,
                                                    LIBRDF_STATEMENT_SUBJECT);
}


static librdf_iterator*
librdf_storage_hashes_find_arcs(librdf_storage* storage,
                                librdf_node* source, librdf_node *target) 
{
  librdf_storage_hashes_context *scontext=(librdf_storage_hashes_context*)storage->context;
  return librdf_storage_hashes_node_iterator_create(storage, source, target,
                                                    scontext->arcs_index,
                                                    LIBRDF_STATEMENT_PREDICATE);
}

static librdf_iterator*
librdf_storage_hashes_find_targets(librdf_storage* storage,
                                   librdf_node* source, librdf_node *arc) 
{
  librdf_storage_hashes_context *scontext=(librdf_storage_hashes_context*)storage->context;
  return librdf_storage_hashes_node_iterator_create(storage, source, arc,
                                                    scontext->targets_index,
                                                    LIBRDF_STATEMENT_OBJECT);
}

#ifdef LIBRDF_STORAGE_HASH_GROUPS
/**
 * librdf_storage_hashes_group_add_statement - Add a statement to a storage group
 * @storage: &librdf_storage object
 * @group_uri: &librdf_uri object
 * @statement: &librdf_statement statement to add
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_hashes_group_add_statement(librdf_storage* storage,
                                          librdf_uri* group_uri,
                                          librdf_statement* statement) 
{
  librdf_storage_hashes_context* context=(librdf_storage_hashes_context*)storage->context;
  librdf_hash_datum key, value; /* on stack - not allocated */
  int size;
  int status;
  
  key.data=(char*)librdf_uri_as_string(group_uri);
  key.size=strlen(key.data);

  size=librdf_statement_encode(statement, NULL, 0);

  value.data=(char*)LIBRDF_MALLOC(cstring, size);
  value.size=librdf_statement_encode(statement, (unsigned char*)value.data, size);

  status=librdf_hash_put(context->hashes[context->groups_index], &key, &value);
  LIBRDF_FREE(data, value.data);

  return status;
}


/**
 * librdf_storage_hashes_group_remove_statement - Remove a statement from a storage group
 * @storage: &librdf_storage object
 * @group_uri: &librdf_uri object
 * @statement: &librdf_statement statement to remove
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_hashes_group_remove_statement(librdf_storage* storage, 
                                             librdf_uri* group_uri,
                                             librdf_statement* statement) 
{
  librdf_storage_hashes_context* context=(librdf_storage_hashes_context*)storage->context;
  librdf_hash_datum key, value; /* on stack - not allocated */
  int size;
  int status;
  
  key.data=(char*)librdf_uri_as_string(group_uri);
  key.size=strlen(key.data);

  size=librdf_statement_encode(statement, NULL, 0);

  value.data=(char*)LIBRDF_MALLOC(cstring, size);
  value.size=librdf_statement_encode(statement, (unsigned char*)value.data, size);

  status=librdf_hash_delete(context->hashes[context->groups_index], &key, &value);
  LIBRDF_FREE(data, value.data);
  
  return status;
}


typedef struct {
  librdf_iterator* iterator;
  librdf_hash_datum *key;
  librdf_hash_datum *value;
} librdf_storage_hashes_group_serialise_stream_context;


/**
 * librdf_storage_hashes_group_serialise - List all statements in a storage group
 * @storage: &librdf_storage object
 * @group_uri: &librdf_uri object
 * 
 * Return value: &librdf_stream of statements or NULL on failure or group is empty
 **/
static librdf_stream*
librdf_storage_hashes_group_serialise(librdf_storage* storage,
                                      librdf_uri* group_uri) 
{
  librdf_storage_hashes_context* context=(librdf_storage_hashes_context*)storage->context;
  librdf_storage_hashes_group_serialise_stream_context* scontext;
  librdf_stream* stream;

  scontext=(librdf_storage_hashes_group_serialise_stream_context*)LIBRDF_CALLOC(librdf_storage_hashes_group_serialise_stream_context, 1, sizeof(librdf_storage_hashes_group_serialise_stream_context));
  if(!scontext)
    return NULL;

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

  scontext->iterator=librdf_hash_get_all(context->hashes[context->groups_index], 
                                         scontext->key, scontext->value);
  if(!scontext->iterator) {
    librdf_storage_hashes_group_serialise_finished(scontext);
    return NULL;
  }


  stream=librdf_new_stream(storage->world,
                           (void*)scontext,
                           &librdf_storage_hashes_group_serialise_end_of_stream,
                           &librdf_storage_hashes_group_serialise_next_statement,
                           &librdf_storage_hashes_group_serialise_finished);
  if(!stream) {
    librdf_storage_hashes_group_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_storage_hashes_group_serialise_end_of_stream(void* context)
{
  librdf_storage_hashes_group_serialise_stream_context* scontext=(librdf_storage_hashes_group_serialise_stream_context*)context;

  return librdf_iterator_end(scontext->iterator);
}


static librdf_statement*
librdf_storage_hashes_group_serialise_next_statement(void* context)
{
  librdf_storage_hashes_group_serialise_stream_context* scontext=(librdf_storage_hashes_group_serialise_stream_context*)context;
  librdf_statement* statement;
  
  statement=librdf_new_statement(scontext->iterator->world);
  if(!statement)
    return NULL;

  if(!librdf_iterator_get_next(scontext->iterator))
    return NULL;

  /* decode value content */
  if(!librdf_statement_decode(statement, 
                              (unsigned char*)scontext->value->data,
                              scontext->value->size)) {
    librdf_free_statement(statement);
    return NULL;
  }

  return statement;
}


static void
librdf_storage_hashes_group_serialise_finished(void* context)
{
  librdf_storage_hashes_group_serialise_stream_context* scontext=(librdf_storage_hashes_group_serialise_stream_context*)context;
  
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

  LIBRDF_FREE(librdf_storage_hashes_group_serialise_stream_context, scontext);
}


#else
/* !LIBRDF_STORAGE_HASH_GROUPS */
static int
librdf_storage_hashes_group_add_statement(librdf_storage* storage,
                                          librdf_uri* group_uri,
                                          librdf_statement* statement) 
{
 return 0;
}
static int
librdf_storage_hashes_group_remove_statement(librdf_storage* storage, 
                                             librdf_uri* group_uri,
                                             librdf_statement* statement) 
{
 return 0;
}

static librdf_stream*
librdf_storage_hashes_group_serialise(librdf_storage* storage,
                                      librdf_uri* group_uri) 
{
 return NULL;
}

#endif
/* end !LIBRDF_STORAGE_HASH_GROUPS */


/* local function to register hashes storage functions */

static void
librdf_storage_hashes_register_factory(librdf_storage_factory *factory) 
{
  factory->context_length     = sizeof(librdf_storage_hashes_context);
  
  factory->init               = librdf_storage_hashes_init;
  factory->clone              = librdf_storage_hashes_clone;
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
  factory->find_sources       = librdf_storage_hashes_find_sources;
  factory->find_arcs          = librdf_storage_hashes_find_arcs;
  factory->find_targets       = librdf_storage_hashes_find_targets;

  factory->group_add_statement    = librdf_storage_hashes_group_add_statement;
  factory->group_remove_statement = librdf_storage_hashes_group_remove_statement;
  factory->group_serialise        = librdf_storage_hashes_group_serialise;
}


void
librdf_init_storage_hashes(void)
{
  librdf_storage_register_factory("hashes",
                                  &librdf_storage_hashes_register_factory);
}
