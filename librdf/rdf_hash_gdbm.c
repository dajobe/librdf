/*
 * RDF DB GDBM Interface Implementation
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>

#include <sys/types.h>

#include <stdio.h>

#include <gdbm.h>


#include <rdf_config.h>
#include <rdf_hash.h>
#include <rdf_hash_gdbm.h>


typedef struct 
{
  GDBM_FILE gdbm_file;
  datum current_key;
} rdf_hash_gdbm_context;


/* prototypes for local functions */
static int rdf_hash_gdbm_open(void* context, char *identifier, void *mode, void *options);
static int rdf_hash_gdbm_close(void* context);
static int rdf_hash_gdbm_get(void* context, rdf_hash_data *key, rdf_hash_data *data, unsigned int flags);
static int rdf_hash_gdbm_put(void* context, rdf_hash_data *key, rdf_hash_data *data, unsigned int flags);
static int rdf_hash_gdbm_delete(void* context, rdf_hash_data *key);
static int rdf_hash_gdbm_get_seq(void* context, rdf_hash_data *key, unsigned int flags);
static int rdf_hash_gdbm_sync(void* context);
static int rdf_hash_gdbm_get_fd(void* context);

static void rdf_hash_gdbm_register_factory(rdf_hash_factory *factory);


/* functions implementing hash api */

static int
rdf_hash_gdbm_open(void* context, char *identifier, void *mode, void *options) 
{
  rdf_hash_gdbm_context* gdbm_context=(rdf_hash_gdbm_context*)context;
  GDBM_FILE gdbm;

  gdbm=gdbm_open(identifier, 512, GDBM_WRCREAT, 0644, 0);
  if(!gdbm)
    return 1;

  gdbm_context->gdbm_file=gdbm;
  return 0;
}

static int
rdf_hash_gdbm_close(void* context) 
{
  rdf_hash_gdbm_context* gdbm_context=(rdf_hash_gdbm_context*)context;
  gdbm_close(gdbm_context->gdbm_file);
  return 0;
}


static int
rdf_hash_gdbm_get(void* context, rdf_hash_data *key, rdf_hash_data *data, unsigned int flags) 
{
  rdf_hash_gdbm_context* gdbm_context=(rdf_hash_gdbm_context*)context;
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
  
  data->data = RDF_MALLOC(gdbm_data, gdbm_data.dsize);
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


static int
rdf_hash_gdbm_put(void* context, rdf_hash_data *key, rdf_hash_data *value, unsigned int flags) 
{
  rdf_hash_gdbm_context* gdbm_context=(rdf_hash_gdbm_context*)context;
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


static int
rdf_hash_gdbm_delete(void* context, rdf_hash_data *key) 
{
  rdf_hash_gdbm_context* gdbm_context=(rdf_hash_gdbm_context*)context;
  datum gdbm_key;

  /* Initialise GDBM version of key */
  gdbm_key.dptr = (char*)key->data;
  gdbm_key.dsize = key->size;
  
  gdbm_delete(gdbm_context->gdbm_file, gdbm_key);
  return 0;
}


static int
rdf_hash_gdbm_get_seq(void* context, rdf_hash_data *key, unsigned int flags) 
{
  rdf_hash_gdbm_context* gdbm_context=(rdf_hash_gdbm_context*)context;
  datum gdbm_key;

  if(flags == RDF_HASH_FLAGS_FIRST) {
    gdbm_key=gdbm_firstkey(gdbm_context->gdbm_file);
  } else if (flags == RDF_HASH_FLAGS_NEXT) {
    gdbm_key=gdbm_nextkey(gdbm_context->gdbm_file, gdbm_context->current_key);
  } else { /* RDF_HASH_FLAGS_CURRENT */
    gdbm_key.dsize=gdbm_context->current_key.dsize;
    gdbm_key.dptr=gdbm_context->current_key.dptr;
  }
  
  
  key->data = RDF_MALLOC(gdbm_data, gdbm_key.dsize);
  if(!key->data) {
    /* always allocated by GDBM using system malloc */
    if(flags != RDF_HASH_FLAGS_CURRENT)
      free(gdbm_key.dptr);
    return 1;
  }
  memcpy(key->data, gdbm_key.dptr, gdbm_key.dsize);
  key->size = gdbm_key.dsize;

  if(flags != RDF_HASH_FLAGS_CURRENT) {
    /* save new current key */
    if(gdbm_context->current_key.dsize)
      RDF_FREE(gdbm_data, gdbm_context->current_key.dptr);
    gdbm_context->current_key.dsize=gdbm_key.dsize;
    gdbm_context->current_key.dptr=(char*)RDF_MALLOC(gdbm_data, gdbm_key.dsize);
    if(!gdbm_context->current_key.dptr)
      return 1;
    memcpy(gdbm_context->current_key.dptr, gdbm_key.dptr, gdbm_key.dsize);
    
    /* always allocated by GDBM using system malloc */
    free(gdbm_key.dptr);
  }

  return 0;
}

static int
rdf_hash_gdbm_sync(void* context) 
{
  rdf_hash_gdbm_context* gdbm_context=(rdf_hash_gdbm_context*)context;
  gdbm_sync(gdbm_context->gdbm_file);
  return 0;
}

static int
rdf_hash_gdbm_get_fd(void* context) 
{
  rdf_hash_gdbm_context* gdbm_context=(rdf_hash_gdbm_context*)context;
  int fd;

  fd=gdbm_fdesc(gdbm_context->gdbm_file);
  return fd;
}


/* local function to register GDBM hash functions */

static void
rdf_hash_gdbm_register_factory(rdf_hash_factory *factory) 
{
  factory->context_length = sizeof(rdf_hash_gdbm_context);

  factory->open    = rdf_hash_gdbm_open;
  factory->close   = rdf_hash_gdbm_close;
  factory->get     = rdf_hash_gdbm_get;
  factory->put     = rdf_hash_gdbm_put;
  factory->delete_key  = rdf_hash_gdbm_delete;
  factory->get_seq = rdf_hash_gdbm_get_seq;
  factory->sync    = rdf_hash_gdbm_sync;
  factory->get_fd  = rdf_hash_gdbm_get_fd;
}

void
init_rdf_hash_gdbm(void)
{
  rdf_hash_register_factory("GDBM", &rdf_hash_gdbm_register_factory);
}
