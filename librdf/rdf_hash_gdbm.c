/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash_gdbm.c - RDF DB GDBM Interface Implementation
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


/* Implementing the hash cursor */
static int librdf_hash_gdbm_cursor_init(void *cursor_context, void *hash_context);
static int librdf_hash_gdbm_cursor_get(void *context, librdf_hash_datum* key, librdf_hash_datum* value, unsigned int flags);
static void librdf_hash_gdbm_cursor_finish(void* context);


/* prototypes for local functions */
static int librdf_hash_gdbm_open(void* context, char *identifier, void *mode,
                                 librdf_hash* options);
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
  
  factory->open    = librdf_hash_gdbm_open;
  factory->close   = librdf_hash_gdbm_close;
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
 * 
 **/
void
librdf_init_hash_gdbm(void)
{
  librdf_hash_register_factory("gdbm", &librdf_hash_gdbm_register_factory);
}
