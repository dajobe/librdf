/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash_gdbm.c - RDF DB GDBM Interface Implementation
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
#include <stdarg.h>

#include <sys/types.h>

#include <gdbm.h>

/* for free() */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


#include <librdf.h>
#include <rdf_hash.h>
#include <rdf_hash_gdbm.h>



typedef struct 
{
  librdf_hash *hash;
  int mode;
  int is_writable;
  int is_new;
  /* for GDBM only */
  GDBM_FILE gdbm_file;
  char* file_name;
  datum current_key;
} librdf_hash_gdbm_context;


/* Implementing the hash cursor */
static int librdf_hash_gdbm_cursor_init(void *cursor_context, void *hash_context);
static int librdf_hash_gdbm_cursor_get(void *context, librdf_hash_datum* key, librdf_hash_datum* value, unsigned int flags);
static void librdf_hash_gdbm_cursor_finish(void* context);


/* prototypes for local functions */
static int librdf_hash_gdbm_create(librdf_hash* hash, void* context);
static int librdf_hash_gdbm_destroy(void* context);
static int librdf_hash_gdbm_open(void* context, char *identifier, int mode, int is_writable, int is_new, librdf_hash* options);
static int librdf_hash_gdbm_close(void* context);
static int librdf_hash_gdbm_put(void* context, librdf_hash_datum *key,
                                librdf_hash_datum *data);
static int librdf_hash_gdbm_exists(void* context, librdf_hash_datum *key);
static int librdf_hash_gdbm_delete(void* context, librdf_hash_datum *key);
static int librdf_hash_gdbm_sync(void* context);
static int librdf_hash_gdbm_get_fd(void* context);

static void librdf_hash_gdbm_register_factory(librdf_hash_factory *factory);


/* functions implementing hash api */

/**
 * librdf_hash_gdbm_create - Create a new GDBM hash
 * @hash: &librdf_hash hash
 * @context: GDBM hash context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_create(librdf_hash* hash, void* context)
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;

  gdbm_context->hash=hash;
  return 0;
}


/**
 * librdf_hash_gdbm_destroy - Destroy a GDBM hash
 * @hash: &librdf_hash hash
 * @context: GDBM hash context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_destroy(void* context)
{
  /* NOP */
  return 0;
}


/**
 * librdf_hash_gdbm_open - Open and maybe create a new GDBM hash
 * @context: GDBM hash context
 * @identifier: filename to use for GDBM file
 * @mode: file creation mode
 * @is_writable: is hash writable?
 * @is_new: is hash new?
 * @options: hash options (currently unused)
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_open(void* context, char *identifier, 
                     int mode, int is_writable, int is_new,
                     librdf_hash* options)
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;
  GDBM_FILE gdbm;
  char *file;
  int flags;
  
  file=(char*)LIBRDF_MALLOC(cstring, strlen(identifier)+1);
  if(!file)
    return 1;
  strcpy(file, identifier);
  
  /* NOTE: If the options parameter is ever used here, the data must be
   * copied into a private part of the context so that the clone
   * method can access them
   */
  gdbm_context->mode=mode;
  gdbm_context->is_writable=is_writable;
  gdbm_context->is_new=is_new;

  flags=(is_writable) ? GDBM_WRITER : GDBM_READER;
  if(is_new)
    flags|=GDBM_WRCREAT;
  
  gdbm=gdbm_open(file, 512, flags, mode, 0);
  if(!gdbm) {
    LIBRDF_DEBUG3(librdf_hash_gdbm_open,
		"GBDM open of %s failed - %s\n", file, 
                  gdbm_strerror(gdbm_errno));
    LIBRDF_FREE(cstring, file);
    return 1;
  }
  
  gdbm_context->gdbm_file=gdbm;
  gdbm_context->file_name=file;
  return 0;
}


/**
 * librdf_hash_gdbm_close - Close the hash
 * @context: GDBM hash context
 * 
 * Finish the association between the rdf hash and the GDBM file (does
 * not delete the file)
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_close(void* context) 
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;

  gdbm_close(gdbm_context->gdbm_file);
  if(gdbm_context->current_key.dsize)
    LIBRDF_FREE(gdbm_data, gdbm_context->current_key.dptr);
  LIBRDF_FREE(cstring, gdbm_context->file_name);
  return 0;
}



/**
 * librdf_hash_gdbm_clone - Clone the GDBM hash
 * @hash: new &librdf_hash that this implements
 * @context: new GDBM hash context
 * @new_identifier: new identifier for this hash
 * @old_context: old GDBM hash context
 * 
 * Clones the existing GDBM hash into the new one with the
 * new identifier.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_clone(librdf_hash *hash, void* context, char *new_identifier,
                       void *old_context) 
{
  librdf_hash_gdbm_context* hcontext=(librdf_hash_gdbm_context*)context;
  librdf_hash_gdbm_context* old_hcontext=(librdf_hash_gdbm_context*)old_context;
  librdf_hash_datum *key, *value;
  librdf_iterator *iterator;
  int status=0;
  
  /* copy data fields that might change */
  hcontext->hash=hash;

  /* Note: The options are not used at present, so no need to make a copy 
   */
  if(librdf_hash_gdbm_open(context, new_identifier,
                           old_hcontext->mode, old_hcontext->is_writable,
                           old_hcontext->is_new, NULL))
    return 1;


  /* Use higher level functions to iterator this data
   * on the other hand, maybe this is a good idea since that
   * code is tested and works
   */

  key=librdf_new_hash_datum(NULL, 0);
  value=librdf_new_hash_datum(NULL, 0);

  iterator=librdf_hash_get_all(old_hcontext->hash, key, value);
  while(!librdf_iterator_end(iterator)) {
    librdf_hash_datum* k= librdf_iterator_get_key(iterator);
    librdf_hash_datum* v= librdf_iterator_get_value(iterator);

    if(librdf_hash_gdbm_put(hcontext, k, v)) {
      status=1;
      break;
    }
    librdf_iterator_next(iterator);
  }
  if(iterator)
    librdf_free_iterator(iterator);

  librdf_free_hash_datum(value);
  librdf_free_hash_datum(key);

  return status;
}


/**
 * librdf_hash_gdbm_values_count - Get the number of values in the hash
 * @context: GDBM hash context
 * 
 * Return value: number of values in the hash or <0 on failure
 **/
static int
librdf_hash_gdbm_values_count(void *context) 
{
  /* Does not seem to be possible to implement */
  return -1;
}


typedef struct {
  librdf_hash_gdbm_context* hash;
  datum current_key;
} librdf_hash_gdbm_cursor_context;



/**
 * librdf_hash_gdbm_cursor_init - 
 **/
static int
librdf_hash_gdbm_cursor_init(void *cursor_context, void *hash_context)
{
  librdf_hash_gdbm_cursor_context *cursor=(librdf_hash_gdbm_cursor_context*)cursor_context;

  cursor->hash=(librdf_hash_gdbm_context*)hash_context;

  return 0;
}


/**
 * librdf_hash_gdbm_cursor_get - Retrieve a hash value for the given key
 * @context: GDBM hash cursor context
 * @key: pointer to key to use
 * @value: pointer to value to use
 * @flags: flags
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_cursor_get(void* context, 
                           librdf_hash_datum *key, librdf_hash_datum *value,
                           unsigned int flags)
{
  librdf_hash_gdbm_cursor_context *cursor=(librdf_hash_gdbm_cursor_context*)context;
  datum gdbm_key;
  datum gdbm_value;


  /* Always initialise key */
  if (flags != LIBRDF_HASH_CURSOR_FIRST) {
    cursor->current_key.dptr = (char*)key->data;
    cursor->current_key.dsize = key->size;
  }
  
  switch(flags) {
  case LIBRDF_HASH_CURSOR_FIRST:
    gdbm_key=cursor->current_key=gdbm_firstkey(cursor->hash->gdbm_file);
    break;

  case LIBRDF_HASH_CURSOR_NEXT:
  case LIBRDF_HASH_CURSOR_NEXT_VALUE:
    /* FIXME - GDBM cannot distinguish these */
    gdbm_key=gdbm_nextkey(cursor->hash->gdbm_file, cursor->current_key);

  default:
    abort();
  }
    
  if(!gdbm_key.dptr) {
    key->data=NULL;
    return 1;
  }

  /* want value? */
  if(value) {
    gdbm_value = gdbm_fetch(cursor->hash->gdbm_file, gdbm_key);
    if(!gdbm_value.dptr) {
      /* not found */
      free(gdbm_key.dptr);
      key->data = NULL;
      return 0;
    }
  
    value->data = LIBRDF_MALLOC(gdbm_data, gdbm_value.dsize);
    if(!value->data) {
      free(gdbm_key.dptr);
      free(gdbm_value.dptr);
      return 1;
    }
    memcpy(value->data, gdbm_value.dptr, gdbm_value.dsize);
    value->size = gdbm_value.dsize;
  
  /* always allocated by GDBM using system malloc */
    free(gdbm_value.dptr);
  }
  

  key->data = LIBRDF_MALLOC(gdbm_data, gdbm_key.dsize);
  if(!key->data) {
    free(gdbm_key.dptr);
    if(value) {
      LIBRDF_FREE(gdbm_data, value->data);
      value->data=NULL;
    }
    return 1;
  }

  memcpy(key->data, gdbm_key.dptr, gdbm_key.dsize);
  key->size = gdbm_key.dsize;
  
  
  /* save new current key */
  if(cursor->current_key.dsize)
    LIBRDF_FREE(gdbm_data, cursor->current_key.dptr);
  cursor->current_key.dsize=gdbm_key.dsize;
  cursor->current_key.dptr=(char*)LIBRDF_MALLOC(gdbm_data, gdbm_key.dsize);
  if(!cursor->current_key.dptr)
    return 1;

  memcpy(cursor->current_key.dptr, gdbm_key.dptr, gdbm_key.dsize);
  
  /* always allocated by GDBM using system malloc */
  free(gdbm_key.dptr);

  return 0;
}


/**
 * librdf_hash_gdbm_cursor_finished - Finish the serialisation of the hash gdbm get
 * @context: GDBM hash cursor context
 **/
static void
librdf_hash_gdbm_cursor_finish(void* context)
{
#if 0
  librdf_hash_gdbm_cursor_context* cursor=(librdf_hash_gdbm_cursor_context*)context;
#endif
}

/**
 * librdf_hash_gdbm_put - Store a key/value pair in the hash
 * @context: GDBM hash context
 * @key: pointer to key to store
 * @value: pointer to value to store
 * @flags: flags (not used at present)
 *
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_put(void* context, librdf_hash_datum *key, librdf_hash_datum *value) 
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;
  datum gdbm_data;
  datum gdbm_key;
  
  /* Initialise GDBM version of key */
  gdbm_key.dptr = (char*)key->data;
  gdbm_key.dsize = key->size;
  
  /* Initialise GDBM version of data */
  gdbm_data.dptr = (char*)value->data;
  gdbm_data.dsize = value->size;
  
  /* flags can be GDBM_INSERT or GDBM_REPLACE */
  gdbm_store(gdbm_context->gdbm_file, gdbm_key, gdbm_data, GDBM_REPLACE);
  return 0;
}


/**
 * librdf_hash_gdbm_exists - Test the existence of a key in the hash
 * @context: GDBM hash context
 * @key: pointer to key to store
 * 
 * Return value: non 0 if the key exists in the hash
 **/
static int
librdf_hash_gdbm_exists(void* context, librdf_hash_datum *key) 
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;
  datum gdbm_key;
  
  /* Initialise GDBM version of key */
  gdbm_key.dptr = (char*)key->data;
  gdbm_key.dsize = key->size;
  
  return gdbm_exists(gdbm_context->gdbm_file, gdbm_key);
}


/**
 * librdf_hash_gdbm_delete - Delete a key/value pair from the hash
 * @context: GDBM hash context
 * @key: pointer to key to delete
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_delete(void* context, librdf_hash_datum *key) 
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;
  datum gdbm_key;
  
  /* Initialise GDBM version of key */
  gdbm_key.dptr = (char*)key->data;
  gdbm_key.dsize = key->size;
  
  gdbm_delete(gdbm_context->gdbm_file, gdbm_key);
  return 0;
}


/**
 * librdf_hash_gdbm_sync - Flush the hash to disk
 * @context: GDBM hash context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_sync(void* context) 
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;

  gdbm_sync(gdbm_context->gdbm_file);
  return 0;
}


/**
 * librdf_hash_gdbm_get_fd - Get the file descriptor representing the hash
 * @context: GDBM hash context
 * 
 * Return value: file descriptor
 **/
static int
librdf_hash_gdbm_get_fd(void* context) 
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;
  int fd;
  
  fd=gdbm_fdesc(gdbm_context->gdbm_file);
  return fd;
}


/* local function to register GDBM hash functions */

/**
 * librdf_hash_gdbm_register_factory - Register the GDBM hash module with the hash factory
 * @factory: hash factory to fill in with GDBM-specific values.
 * 
 **/
static void
librdf_hash_gdbm_register_factory(librdf_hash_factory *factory) 
{
  factory->context_length = sizeof(librdf_hash_gdbm_context);
  factory->cursor_context_length = sizeof(librdf_hash_gdbm_cursor_context);
  
  factory->create  = librdf_hash_gdbm_create;
  factory->destroy = librdf_hash_gdbm_destroy;

  factory->open    = librdf_hash_gdbm_open;
  factory->close   = librdf_hash_gdbm_close;
  factory->clone   = librdf_hash_gdbm_clone;

  factory->values_count = librdf_hash_gdbm_values_count;

  factory->put     = librdf_hash_gdbm_put;
  factory->exists  = librdf_hash_gdbm_exists;
  factory->delete_key  = librdf_hash_gdbm_delete;
  factory->sync    = librdf_hash_gdbm_sync;
  factory->get_fd  = librdf_hash_gdbm_get_fd;

  factory->cursor_init   = librdf_hash_gdbm_cursor_init;
  factory->cursor_get    = librdf_hash_gdbm_cursor_get;
  factory->cursor_finish = librdf_hash_gdbm_cursor_finish;
}

/**
 * librdf_init_hash_gdbm - Initialise the GDBM hash module
 * @world: redland world object
 **/
void
librdf_init_hash_gdbm(librdf_world *world)
{
  librdf_hash_register_factory(world,
                               "gdbm", &librdf_hash_gdbm_register_factory);
}
