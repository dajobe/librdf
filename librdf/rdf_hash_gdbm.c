/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash_gdbm.c - RDF DB GDBM Interface Implementation
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

#include <gdbm.h>

/* for free() */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_hash.h>
#include <rdf_hash_gdbm.h>


typedef struct 
{
  GDBM_FILE gdbm_file;
  char* file_name;
  datum current_key;
} librdf_hash_gdbm_context;


/* prototypes for local functions */
static int librdf_hash_gdbm_open(void* context, char *identifier, void *mode,
                                 librdf_hash* options);
static int librdf_hash_gdbm_close(void* context);
static int librdf_hash_gdbm_get(void* context, librdf_hash_data *key,
                                librdf_hash_data *data, unsigned int flags);
static int librdf_hash_gdbm_put(void* context, librdf_hash_data *key,
                                librdf_hash_data *data, unsigned int flags);
static int librdf_hash_gdbm_exists(void* context, librdf_hash_data *key);
static int librdf_hash_gdbm_delete(void* context, librdf_hash_data *key);
static int librdf_hash_gdbm_get_seq(void* context, librdf_hash_data *key,
                                    librdf_hash_sequence_type type);
static int librdf_hash_gdbm_sync(void* context);
static int librdf_hash_gdbm_get_fd(void* context);

static void librdf_hash_gdbm_register_factory(librdf_hash_factory *factory);


/* functions implementing hash api */

/**
 * librdf_hash_gdbm_open - Open and maybe create a new GDBM hash
 * @context: GDBM hash context
 * @identifier: filename to use for GDBM file
 * @mode: GDBM access mode (currently unused)
 * @options:  librdf_hash of options (currently used)
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_open(void* context, char *identifier, void *mode,
                   librdf_hash* options)
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;
  GDBM_FILE gdbm;
  char *file;
  
  file=(char*)LIBRDF_MALLOC(cstring, strlen(identifier)+1);
  if(!file)
    return 1;
  strcpy(file, identifier);
  
  gdbm=gdbm_open(file, 512, GDBM_WRCREAT, 0644, 0);
  if(!gdbm) {
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
 * librdf_hash_gdbm_get - Retrieve a hash value for the given key
 * @context: GDBM hash context
 * @key: pointer to key to use
 * @data: pointer to data to return value
 * @flags: flags (not used at present)
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_get(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags) 
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;
  datum gdbm_data;
  datum gdbm_key;
  
  /* Initialise GDBM version of key */
  gdbm_key.dptr = (char*)key->data;
  gdbm_key.dsize = key->size;
  
  gdbm_data = gdbm_fetch(gdbm_context->gdbm_file, gdbm_key);
  if(!gdbm_data.dptr) {
    /* not found */
    data->data = NULL;
    return 0;
  }
  
  data->data = LIBRDF_MALLOC(gdbm_data, gdbm_data.dsize);
  if(!data->data) {
    free(gdbm_data.dptr);
    return 1;
  }
  memcpy(data->data, gdbm_data.dptr, gdbm_data.dsize);
  data->size = gdbm_data.dsize;
  
  /* always allocated by GDBM using system malloc */
  free(gdbm_data.dptr);
  return 0;
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
librdf_hash_gdbm_put(void* context, librdf_hash_data *key, librdf_hash_data *value, unsigned int flags) 
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
librdf_hash_gdbm_exists(void* context, librdf_hash_data *key) 
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
librdf_hash_gdbm_delete(void* context, librdf_hash_data *key) 
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
 * librdf_hash_gdbm_get_seq - Provide sequential access to the hash
 * @context: GDBM hash context
 * @key: pointer to the key
 * @type: type of operation
 * 
 * Valid operations are
 * LIBRDF_HASH_SEQUENCE_FIRST to get the first key in the sequence
 * LIBRDF_HASH_SEQUENCE_NEXT to get the next in sequence and
 * LIBRDF_HASH_SEQUENCE_CURRENT to get the current key (again).
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_gdbm_get_seq(void* context, librdf_hash_data *key, librdf_hash_sequence_type type) 
{
  librdf_hash_gdbm_context* gdbm_context=(librdf_hash_gdbm_context*)context;
  datum gdbm_key;
  
  if(type == LIBRDF_HASH_SEQUENCE_FIRST) {
    gdbm_key=gdbm_firstkey(gdbm_context->gdbm_file);

  } else if (type == LIBRDF_HASH_SEQUENCE_NEXT) {
    gdbm_key=gdbm_nextkey(gdbm_context->gdbm_file, gdbm_context->current_key);

  } else if (type == LIBRDF_HASH_SEQUENCE_CURRENT) {
    gdbm_key.dsize=gdbm_context->current_key.dsize;
    gdbm_key.dptr=gdbm_context->current_key.dptr;

  } else
    LIBRDF_FATAL2(librdf_hash_gdbm_get_seq, "Unknown type %d", type);
  
  if(!gdbm_key.dptr) {
    key->data=NULL;
    return 1;
  }

  key->data = LIBRDF_MALLOC(gdbm_data, gdbm_key.dsize);
  if(!key->data) {
    /* always allocated by GDBM using system malloc */
    if(type != LIBRDF_HASH_SEQUENCE_CURRENT)
      free(gdbm_key.dptr);
    return 1;
  }
  memcpy(key->data, gdbm_key.dptr, gdbm_key.dsize);
  key->size = gdbm_key.dsize;
  
  if(type != LIBRDF_HASH_SEQUENCE_CURRENT) {
    /* save new current key */
    if(gdbm_context->current_key.dsize)
      LIBRDF_FREE(gdbm_data, gdbm_context->current_key.dptr);
    gdbm_context->current_key.dsize=gdbm_key.dsize;
    gdbm_context->current_key.dptr=(char*)LIBRDF_MALLOC(gdbm_data,
                                                        gdbm_key.dsize);
    if(!gdbm_context->current_key.dptr)
      return 1;
    memcpy(gdbm_context->current_key.dptr, gdbm_key.dptr,
           gdbm_key.dsize);
    
    /* always allocated by GDBM using system malloc */
    free(gdbm_key.dptr);
  }
  
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
  
  factory->open    = librdf_hash_gdbm_open;
  factory->close   = librdf_hash_gdbm_close;
  factory->get     = librdf_hash_gdbm_get;
  factory->put     = librdf_hash_gdbm_put;
  factory->exists  = librdf_hash_gdbm_exists;
  factory->delete_key  = librdf_hash_gdbm_delete;
  factory->get_seq = librdf_hash_gdbm_get_seq;
  factory->sync    = librdf_hash_gdbm_sync;
  factory->get_fd  = librdf_hash_gdbm_get_fd;
}

/**
 * librdf_init_hash_gdbm - Initialise the GDBM hash module
 * 
 **/
void
librdf_init_hash_gdbm(void)
{
  librdf_hash_register_factory("GDBM", &librdf_hash_gdbm_register_factory);
}
