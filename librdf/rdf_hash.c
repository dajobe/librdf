/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash.c - RDF Hash Implementation
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

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for strtol */
#endif

#ifdef HAVE_STRING_H
#include <string.h> /* for strncmp */
#endif


#define LIBRDF_INTERNAL 1
#include <redland.h>

#include <rdf_hash.h>
#ifdef HAVE_GDBM_HASH
#include <rdf_hash_gdbm.h>
#endif
#ifdef HAVE_BDB_HASH
#include <rdf_hash_bdb.h>
#endif
#include <rdf_hash_memory.h>


/* prototypes for helper functions */
static void librdf_delete_hash_factories(void);

static void librdf_init_hash_datums(void);
static void librdf_free_hash_datums(void);


/* prototypes for iterator for getting all keys and values */
static int librdf_hash_get_all_iterator_have_elements(void* iterator);
static void* librdf_hash_get_all_iterator_get_next(void* iterator);
static void librdf_hash_get_all_iterator_finished(void* iterator);

/* prototypes for iterator for getting all keys */
static int librdf_hash_keys_iterator_have_elements(void* iterator);
static void* librdf_hash_keys_iterator_get_next(void* iterator);
static void librdf_hash_keys_iterator_finished(void* iterator);




/**
 * librdf_init_hash - Initialise the librdf_hash module
 *
 * Initialises and registers all
 * compiled hash modules.  Must be called before using any of the hash
 * factory functions such as librdf_get_hash_factory()
 **/
void
librdf_init_hash(void) 
{
  /* Init hash datum cache */
  librdf_init_hash_datums();
#if 0  
#ifdef HAVE_GDBM_HASH
  librdf_init_hash_gdbm();
#endif
#endif
#ifdef HAVE_BDB_HASH
  librdf_init_hash_bdb();
#endif
  /* Always have hash in memory implementation available */
  librdf_init_hash_memory(-1); /* use default load factor */
}


/**
 * librdf_finish_hash - Terminate the librdf_hash module
 **/
void
librdf_finish_hash(void) 
{
  librdf_delete_hash_factories();
  librdf_free_hash_datums();
}


/* statics */

/* list of hash factories */
static librdf_hash_factory* hashes;


/* helper functions */
static void
librdf_delete_hash_factories(void)
{
  librdf_hash_factory *factory, *next;
  
  for(factory=hashes; factory; factory=next) {
    next=factory->next;
    LIBRDF_FREE(librdf_hash_factory, factory->name);
    LIBRDF_FREE(librdf_hash_factory, factory);
  }
}



/* hash datums structures */

/* a list of free librdf_hash_datums is kept */
static librdf_hash_datum* hash_datums_list=NULL;


static void
librdf_init_hash_datums(void)
{
  hash_datums_list=NULL;
}


static void
librdf_free_hash_datums(void)
{
  librdf_hash_datum *datum, *next;
  
  for(datum=hash_datums_list; datum; datum=next) {
    next=datum->next;
    LIBRDF_FREE(librdf_hash_datum, datum);
  }
}


/**
 * librdf_new_hash_datum - Constructor - Create a new hash datum object
 * @data: data to store
 * @size: size of data
 * 
 * Return value: New &librdf_hash_datum object or NULL on failure
 **/
librdf_hash_datum*
librdf_new_hash_datum(void *data, size_t size)
{
  librdf_hash_datum *datum;

  /* get one from free list, or allocate new one */ 
  if((datum=hash_datums_list)) {
    hash_datums_list=datum->next;
  } else 
    datum=(librdf_hash_datum*)LIBRDF_CALLOC(librdf_hash_datum, 1, sizeof(librdf_hash_datum));

  if(datum) {
    datum->data=data;
    datum->size=size;
  }
  return datum;
}


/**
 * librdf_free_hash_datum - Destructor - destroy a hash datum object
 * @datum: hash datum object
 **/
void
librdf_free_hash_datum(librdf_hash_datum *datum) 
{
  if(datum->data)
    LIBRDF_FREE(cstring, datum->data);
  datum->next=hash_datums_list;
  hash_datums_list=datum;
}


/* class methods */

/**
 * librdf_hash_register_factory - Register a hash factory
 * @name: the hash factory name
 * @factory: pointer to function to call to register the factory
 * 
 **/
void
librdf_hash_register_factory(const char *name,
                             void (*factory) (librdf_hash_factory*)) 
{
  librdf_hash_factory *hash, *h;
  char *name_copy;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_hash_register_factory,
		"Received registration for hash %s\n", name);
#endif
  
  hash=(librdf_hash_factory*)LIBRDF_CALLOC(librdf_hash_factory, 1,
                                           sizeof(librdf_hash_factory));
  if(!hash)
    LIBRDF_FATAL1(librdf_hash_register_factory, "Out of memory\n");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, strlen(name)+1, 1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_hash, hash);
    LIBRDF_FATAL1(librdf_hash_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  hash->name=name_copy;
  
  for(h = hashes; h; h = h->next ) {
    if(!strcmp(h->name, name_copy)) {
      LIBRDF_FATAL2(librdf_hash_register_factory,
		    "hash %s already registered\n", h->name);
    }
  }
  
  /* Call the hash registration function on the new object */
  (*factory)(hash);
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_hash_register_factory, "%s has context size %d\n",
		name, hash->context_length);
#endif
  
  hash->next = hashes;
  hashes = hash;
}


/**
 * librdf_get_hash_factory - Get a hash factory by name
 * @name: the factory name or NULL for the default factory
 * 
 * FIXME: several bits of code assume the default hash factory is
 * in memory.
 *
 * Return value: the factory object or NULL if there is no such factory
 **/
librdf_hash_factory*
librdf_get_hash_factory(const char *name) 
{
  librdf_hash_factory *factory;

  /* return 1st hash if no particular one wanted - why? */
  if(!name) {
    factory=hashes;
    if(!factory) {
      LIBRDF_DEBUG1(librdf_get_hash_factory,
		    "No (default) hashes registered\n");
      return NULL;
    }
  } else {
    for(factory=hashes; factory; factory=factory->next) {
      if(!strcmp(factory->name, name)) {
	break;
      }
    }
    /* else FACTORY name not found */
    if(!factory)
      return NULL;
  }
  
  return factory;
}



/**
 * librdf_new_hash -  Constructor - create a new librdf_hash object
 * @name: factory name
 *
 * Return value: a new &librdf_hash object or NULL on failure
 */
librdf_hash*
librdf_new_hash(char* name) {
  librdf_hash_factory *factory;

  factory=librdf_get_hash_factory(name);
  if(!factory)
    return NULL;

  return librdf_new_hash_from_factory(factory);
}


/**
 * librdf_new_hash_from_factory -  Constructor - create a new librdf_hash object from a factory
 * @factory: the factory to use to construct the hash
 *
 * Return value: a new &librdf_hash object or NULL on failure
 */
librdf_hash*
librdf_new_hash_from_factory (librdf_hash_factory* factory) {
  librdf_hash* h;

  h=(librdf_hash*)LIBRDF_CALLOC(librdf_hash, sizeof(librdf_hash), 1);
  if(!h)
    return NULL;
  
  h->context=(char*)LIBRDF_CALLOC(librdf_hash_context, 1,
                                  factory->context_length);
  if(!h->context) {
    librdf_free_hash(h);
    return NULL;
  }
  
  h->factory=factory;
  
  return h;
}


/**
 * librdf_free_hash - Destructor - destroy a librdf_hash object
 *
 * @hash: hash object
 **/
void
librdf_free_hash (librdf_hash* hash) 
{
  if(hash->context) {
    if(hash->is_open)
      hash->factory->close(hash->context);
    LIBRDF_FREE(librdf_hash_context, hash->context);
  }
  LIBRDF_FREE(librdf_hash, hash);
}


/* methods */

/**
 * librdf_hash_open - Start a hash association 
 * @hash: hash object
 * @identifier: indentifier for the hash factory - usually a URI or file name
 * @mode: hash access mode
 * @is_writable: is hash writable?
 * @is_new: is hash new?
 * @options: a hash of options for the hash factory or NULL if there are none.
 * 
 * This method opens and/or creates a new hash with any resources it
 * needs.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_open(librdf_hash* hash, char *identifier,
                 int mode, int is_writable, int is_new,
                 librdf_hash* options) 
{
  int status;

  status=hash->factory->open(hash->context, identifier, 
                             mode, is_writable, is_new, 
                             options);
  if(!status)
    hash->is_open=1;
  return status;
}


/**
 * librdf_hash_close - End a hash association
 * @hash: hash object
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_close(librdf_hash* hash)
{
  hash->is_open=0;
  return hash->factory->close(hash->context);
}


/**
 * librdf_hash_get - Retrieve one value from hash for a given key as string
 * @hash: hash object
 * @key: pointer to key
 * 
 * The value returned is from newly allocated memory which the
 * caller must free.
 * 
 * Return value: the value or NULL on failure
 **/
char*
librdf_hash_get(librdf_hash* hash, char *key)
{
  librdf_hash_datum *hd_key, *hd_value;
  char *value=NULL;

  hd_key=librdf_new_hash_datum(key, strlen(key));
  if(!hd_key)
    return NULL;

  hd_value=librdf_hash_get_one(hash, hd_key);

  if(hd_value) {
    if(hd_value->data) {
      value=LIBRDF_MALLOC(cstring, hd_value->size+1);
      if(value) {
        /* Copy into new null terminated string for userland */
        memcpy(value, hd_value->data, hd_value->size);
        value[hd_value->size]='\0';
      }
    }
    librdf_free_hash_datum(hd_value);
  }

  /* don't free user key */
  hd_key->data=NULL;
  librdf_free_hash_datum(hd_key);

  return value;
}


/**
 * librdf_hash_get_one - Retrieve one value from hash for a given key
 * @hash: hash object
 * @key: pointer to key
 * 
 * The value returned is from newly allocated memory which the
 * caller must free.
 * 
 * Return value: the value or NULL on failure
 **/
librdf_hash_datum*
librdf_hash_get_one(librdf_hash* hash, librdf_hash_datum *key)
{
  librdf_hash_datum *value;
  librdf_hash_cursor *cursor;
  int status;
  char *new_value;
  
  value=librdf_new_hash_datum(NULL, 0);
  if(!value)
    return NULL;

  cursor=librdf_new_hash_cursor(hash);
  if(!cursor) {
    librdf_free_hash_datum(value);
    return NULL;
  }

  status=librdf_hash_cursor_get_next(cursor, key, value);
  if(!status) {
    /* value->data will point to SHARED area, so copy it */
    new_value=LIBRDF_MALLOC(cstring, value->size);
    if(new_value) {
      memcpy(new_value, value->data, value->size);
      value->data=new_value;
    } else {
      status=1;
      value->data=NULL;
    }
  }

  /* this deletes the data behind the datum */
  librdf_free_hash_cursor(cursor);

  if(status) {
    librdf_free_hash_datum(value);
    return NULL;
  }
  
  return value;
}


typedef struct {
  librdf_hash* hash;
  librdf_hash_cursor* cursor;
  librdf_hash_datum *key;
  librdf_hash_datum *value;

  librdf_hash_datum *next_key; /* not used if one_key set */
  librdf_hash_datum *next_value;
  int have_elements;
  int one_key;
} librdf_hash_get_all_iterator_context;



/**
 * librdf_hash_get_all - Retrieve all values from hash for a given key
 * @hash: hash object
 * @key: pointer to key
 * @value: pointer to value
 * 
 * The iterator returns &librdf_hash_datum objects containingvalue returned is from newly allocated memory which the
 * caller must free.
 * 
 * Return value: non 0 on failure
 **/
librdf_iterator*
librdf_hash_get_all(librdf_hash* hash, 
                    librdf_hash_datum *key, librdf_hash_datum *value)
{
  librdf_hash_get_all_iterator_context* context;
  int status;
  
  context=(librdf_hash_get_all_iterator_context*)LIBRDF_CALLOC(librdf_hash_get_all_iterator_context, 1, sizeof(librdf_hash_get_all_iterator_context));
  if(!context)
    return NULL;

  if(!(context->cursor=librdf_new_hash_cursor(hash))) {
    librdf_hash_get_all_iterator_finished(context);
    return NULL;
  }

  if(key->data)
    context->one_key=1;

  if(!context->one_key)
    if(!(context->next_key=librdf_new_hash_datum(NULL, 0))) {
      librdf_hash_get_all_iterator_finished(context);
      return NULL;
    }

  if(value)
    if(!(context->next_value=librdf_new_hash_datum(NULL, 0))) {
      librdf_hash_get_all_iterator_finished(context);
      return NULL;
    }

  context->hash=hash;
  context->key=key;
  context->value=value;

  if(context->one_key)
    status=librdf_hash_cursor_set(context->cursor, context->key, 
                                  context->next_value);
  else
    status=librdf_hash_cursor_get_first(context->cursor, context->next_key, 
                                        context->next_value);

  context->have_elements=(status == 0);
  
  return librdf_new_iterator((void*)context,
                             librdf_hash_get_all_iterator_have_elements,
                             librdf_hash_get_all_iterator_get_next,
                             librdf_hash_get_all_iterator_finished);
}


static int
librdf_hash_get_all_iterator_have_elements(void* iterator)
{
  librdf_hash_get_all_iterator_context* context=(librdf_hash_get_all_iterator_context*)iterator;
  int status;
  
  if(!context->have_elements)
    return 0;

  /* have key (if not one_key) or value */
  if((!context->one_key && context->next_key->data) ||
     context->next_value->data)
    return 1;
  
  /* no stored data, so check for it */
  if(context->one_key)
    status=librdf_hash_cursor_get_next_value(context->cursor,
                                             context->key,
                                             context->next_value);
  else
    status=librdf_hash_cursor_get_next(context->cursor, 
                                       context->next_key, 
                                       context->next_value);

  if(status)
    context->have_elements=0;
  
  return context->have_elements;
}


static void*
librdf_hash_get_all_iterator_get_next(void* iterator) 
{
  librdf_hash_get_all_iterator_context* context=(librdf_hash_get_all_iterator_context*)iterator;

  if(!context->have_elements)
    return NULL;
  
  /* check stored data - have key (if not one_key) or value? */
  if((!context->one_key && context->next_key->data) ||
     context->next_value->data) {

    if(!context->one_key) {
      context->key->data=context->next_key->data;
      context->key->size=context->next_key->size;
    }

    if(context->value) {
      context->value->data=context->next_value->data;
      context->value->size=context->next_value->size;
    }

    if(!context->one_key)
      context->next_key->data=NULL;
    
    context->next_value->data=NULL;

  } else {
    int status;
    
    /* no stored data, so check for it */
    if(context->one_key)
      status=librdf_hash_cursor_get_next_value(context->cursor, 
                                               context->next_key,
                                               context->next_value);
    else
      status=librdf_hash_cursor_get_next(context->cursor, context->next_key, 
                                         context->next_value);
    
    if(status)
      context->have_elements=0;
  }
  

  return (void*)(context->have_elements != 0);
}


static void
librdf_hash_get_all_iterator_finished(void* iterator) 
{
  librdf_hash_get_all_iterator_context* context=(librdf_hash_get_all_iterator_context*)iterator;

  if(context->cursor)
    librdf_free_hash_cursor(context->cursor);

  if(context->next_key) {
    context->next_key->data=NULL;
    if(context->next_key)
      librdf_free_hash_datum(context->next_key);
  }
  
  if(context->next_value) {
    context->next_value->data=NULL;
    if(context->next_value)
      librdf_free_hash_datum(context->next_value);
  }

  context->key->data=NULL;

  if(context->value)
    context->value->data=NULL;

  LIBRDF_FREE(librdf_hash_get_all_iterator_context, context);
}


/**
 * librdf_hash_put - Insert key/value pairs into the hash according to flags
 * @hash: hash object
 * @key: pointer to key 
 * @key_len: length of key in bytes
 * @value: pointer to the value
 * @value_len: length of the value in bytes
 * 
 * The key and values are copied into the hash; the original pointers
 * can be deleted.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_put(librdf_hash* hash, void *key, size_t key_len,
                void *value, size_t value_len)
{
  librdf_hash_datum hd_key, hd_value;
        
  /* copy pointers and lengths into librdf_hash_datum structures */
  hd_key.data=key; hd_key.size=key_len;
  hd_value.data=value; hd_value.size=value_len;
        
  /* call generic routine using librdf_hash_datum structs */
  return hash->factory->put(hash->context, &hd_key, &hd_value);
}


/**
 * librdf_hash_exists - Check if a given key is in the hash
 * @hash: hash object
 * @key: pointer to the key
 * @key_len: length of key in bytes
 * 
 * Return value: non 0 if the key is present in the hash
 **/
int
librdf_hash_exists(librdf_hash* hash, void *key, size_t key_len) 
{
  librdf_hash_datum hd_key;
  /* copy key pointers and lengths into librdf_hash_datum structures */
  hd_key.data=key; hd_key.size=key_len;
        
  return hash->factory->exists(hash->context, &hd_key);
}


/**
 * librdf_hash_delete - Delete a key/value pair from the hash
 * @hash: hash object
 * @key: pointer to key
 * @key_len: length of key in bytes
 * 
 * Return value: non 0 on failure (including pair not present)
 **/
int
librdf_hash_delete(librdf_hash* hash, void *key, size_t key_len)
{
  librdf_hash_datum hd_key;

  /* copy key pointers and lengths into librdf_hash_datum structures */
  hd_key.data=key; hd_key.size=key_len;

  return hash->factory->delete_key(hash->context, &hd_key);
}


typedef struct {
  librdf_hash* hash;
  librdf_hash_cursor* cursor;
  librdf_hash_datum *key;

  librdf_hash_datum *next_key;
  int have_elements;
} librdf_hash_keys_iterator_context;



/**
 * librdf_hash_keys - Get the hash keys
 * @hash: hash object
 * @key: pointer to key
 * 
 * The iterator returns &librdf_hash_datum objects containingvalue returned is from newly allocated memory which the
 * caller must free.
 * 
 * Return value: &librdf_iterator serialisation of keys or NULL on failure
 **/
librdf_iterator*
librdf_hash_keys(librdf_hash* hash, librdf_hash_datum *key)
{
  librdf_hash_keys_iterator_context* context;
  int status;
  
  context=(librdf_hash_keys_iterator_context*)LIBRDF_CALLOC(librdf_hash_keys_iterator_context, 1, sizeof(librdf_hash_keys_iterator_context));
  if(!context)
    return NULL;


  if(!(context->cursor=librdf_new_hash_cursor(hash))) {
    librdf_hash_keys_iterator_finished(context);
    return NULL;
  }

  if(!(context->next_key=librdf_new_hash_datum(NULL, 0))) {
    librdf_hash_keys_iterator_finished(context);
    return NULL;
  }

  context->hash=hash;
  context->key=key;
 
  status=librdf_hash_cursor_get_first(context->cursor, context->next_key, 
                                      NULL);
  context->have_elements=(status == 0);
  
  return librdf_new_iterator((void*)context,
                             librdf_hash_keys_iterator_have_elements,
                             librdf_hash_keys_iterator_get_next,
                             librdf_hash_keys_iterator_finished);
}


static int
librdf_hash_keys_iterator_have_elements(void* iterator)
{
  librdf_hash_keys_iterator_context* context=(librdf_hash_keys_iterator_context*)iterator;
  
  if(!context->have_elements)
    return 0;

  /* have key */
  if(context->next_key->data)
    return 1;
  
  /* no stored data, so check for it */
  if(librdf_hash_cursor_get_next(context->cursor, context->next_key, NULL))
    context->have_elements=0;
  
  return context->have_elements;
}


static void*
librdf_hash_keys_iterator_get_next(void* iterator) 
{
  librdf_hash_keys_iterator_context* context=(librdf_hash_keys_iterator_context*)iterator;

  if(!context->have_elements)
    return NULL;
  
  /* check stored data */
  if(context->next_key->data) {
    context->key->data=context->next_key->data;
    context->key->size=context->next_key->size;

    context->next_key->data=NULL;
  } else {
    /* no stored data, so check for it */
    if(librdf_hash_cursor_get_next(context->cursor, context->next_key, 
                                   NULL))
      context->have_elements=0;
  }

  return (void*)(context->have_elements != 0);
}


static void
librdf_hash_keys_iterator_finished(void* iterator) 
{
  librdf_hash_keys_iterator_context* context=(librdf_hash_keys_iterator_context*)iterator;

  if(context->cursor)
    librdf_free_hash_cursor(context->cursor);

  context->next_key->data=NULL;
  if(context->next_key)
    librdf_free_hash_datum(context->next_key);

  context->key->data=NULL;

  LIBRDF_FREE(librdf_hash_keys_iterator_context, context);
}


/**
 * librdf_hash_sync - Flush any cached information to disk if appropriate
 * @hash: hash object
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_sync(librdf_hash* hash)
{
  return hash->factory->sync(hash->context);
}


/**
 * librdf_hash_get_fd - Get the file descriptor for the hash
 * @hash: hash object
 * 
 * This returns the file descriptor if it is file based for
 * use with file locking.
 * 
 * Return value: the file descriptor
 **/
int
librdf_hash_get_fd(librdf_hash* hash)
{
  return hash->factory->get_fd(hash->context);
}


/**
 * librdf_hash_print - pretty print the hash to a file descriptor
 * @hash: the hash
 * @fh: file handle
 **/
void
librdf_hash_print(librdf_hash* hash, FILE *fh) 
{
  librdf_iterator* iterator;
  librdf_hash_datum *key, *value;
  
  fprintf(fh, "%s hash: {\n", hash->factory->name);

  key=librdf_new_hash_datum(NULL, 0);
  value=librdf_new_hash_datum(NULL, 0);

  iterator=librdf_hash_get_all(hash, key, value);
  while(librdf_iterator_have_elements(iterator)) {
    librdf_iterator_get_next(iterator);
    
    fputs("  '", fh);
    fwrite(key->data, key->size, 1, fh);
    fputs("'=>'", fh);
    fwrite(value->data, value->size, 1, fh);
    fputs("'\n", fh);
      
  }
  if(iterator)
    librdf_free_iterator(iterator);

  librdf_free_hash_datum(value);
  librdf_free_hash_datum(key);

  fputc('}', fh);
}


/**
 * librdf_hash_print_keys - pretty print the keys to a file descriptor
 * @hash: the hash
 * @fh: file handle
 **/
void
librdf_hash_print_keys(librdf_hash* hash, FILE *fh) 
{
  librdf_iterator* iterator;
  librdf_hash_datum *key;
  
  fputs("{\n", fh);

  key=librdf_new_hash_datum(NULL, 0);

  iterator=librdf_hash_keys(hash, key);
  while(librdf_iterator_have_elements(iterator)) {
    librdf_iterator_get_next(iterator);
    
    fputs("  '", fh);
    fwrite(key->data, key->size, 1, fh);
    fputs("'\n", fh);
  }
  if(iterator)
    librdf_free_iterator(iterator);

  librdf_free_hash_datum(key);

  fputc('}', fh);
}


/**
 * librdf_hash_print_values - pretty print the values of one key to a file descriptor
 * @hash: the hash
 * @key_string: the key as a string
 * @fh: file handle
 **/
void
librdf_hash_print_values(librdf_hash* hash, char *key_string, FILE *fh)
{
  librdf_hash_datum *key, *value;
  librdf_iterator* iterator;
  int first=1;
  
  key=librdf_new_hash_datum(key_string, strlen(key_string));
  if(!key)
    return;
  
  value=librdf_new_hash_datum(NULL, 0);
  if(!value) {
    key->data=NULL;
    librdf_free_hash_datum(key);
    return;
  }
  
  iterator=librdf_hash_get_all(hash, key, value);
  fputc('(', fh);
  while(librdf_iterator_have_elements(iterator)) {
    librdf_iterator_get_next(iterator);
    if(!first)
      fputs(", ", fh);
      
    fputc('\'', fh);
    fwrite(value->data, value->size, 1, fh);
    fputc('\'', fh);
    first=0;
  }
  fputc(')', fh);
  librdf_free_iterator(iterator);

  key->data=NULL;
  librdf_free_hash_datum(key);

  librdf_free_hash_datum(value);
}



/* private enum */
typedef enum {
  HFS_PARSE_STATE_INIT = 0,
  HFS_PARSE_STATE_KEY = 1,
  HFS_PARSE_STATE_SEP = 2,
  HFS_PARSE_STATE_EQ = 3,
  HFS_PARSE_STATE_VALUE = 4
} librdf_hfs_parse_state;



/**
 * librdf_hash_from_string - Initialise a hash from a string
 * @hash: hash object
 * @string: hash encoded as a string
 * 
 * The string format is something like:
 * key1='value1',key2='value2', key3='\'quoted value\''
 *
 * The 's are required and whitespace can appear around the = and ,s
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_from_string (librdf_hash* hash, char *string) 
{
  char *p;
  char *key;
  size_t key_len;
  char *value;
  size_t value_len;
  int backslashes;
  librdf_hfs_parse_state state;
  int real_value_len;
  char *new_value;
  int i;
  char *to;

  if(!string)
    return 0;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_hash_from_string, "Parsing >>%s<<\n", string);
#endif

  p=string;
  key=NULL; key_len=0;
  value=NULL; value_len=0;
  backslashes=0;
  state=HFS_PARSE_STATE_INIT;
  while(*p) {

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG3(librdf_hash_from_string,
                  "state %d at %s\n", state, p);
#endif

    switch(state){
      /* start of config - before key */
      case HFS_PARSE_STATE_INIT:
        while(*p && (isspace((int)*p) || *p == ','))
          p++;
        if(!*p)
          break;

        /* fall through to next state */
        state=HFS_PARSE_STATE_KEY;
        
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG3(librdf_hash_from_string,
                  "state %d at %s\n", state, p);
#endif

        /* start of key */
      case HFS_PARSE_STATE_KEY:
        key=p;
        while(*p && (isalnum((int)*p) || *p == '_' || *p == '-'))
          p++;
        if(!*p)
          break;
        key_len=p-key;
        
        /* if 1st char is not space or alpha, move on */
        if(!key_len) {
          p++;
          state=HFS_PARSE_STATE_INIT;
          break;
        }
        
        state=HFS_PARSE_STATE_SEP;
        /* fall through to next state */
      
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG3(librdf_hash_from_string,
                  "state %d at %s\n", state, p);
#endif

        /* got key, now skipping spaces */
      case HFS_PARSE_STATE_SEP:
        while(*p && isspace((int)*p))
          p++;
        if(!*p)
          break;
        /* expecting = now */
        if(*p != '=') {
          p++;
          state=HFS_PARSE_STATE_INIT;
          break;
        }
        p++;
        state=HFS_PARSE_STATE_EQ;
        /* fall through to next state */
        
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG3(librdf_hash_from_string,
                  "state %d at %s\n", state, p);
#endif

        /* got key\s+= now skipping spaces " */
      case HFS_PARSE_STATE_EQ:
        while(*p && isspace((int)*p))
          p++;
        if(!*p)
          break;
        /* expecting ' now */
        if(*p != '\'') {
          p++;
          state=HFS_PARSE_STATE_INIT;
          break;
        }
        p++;
        state=HFS_PARSE_STATE_VALUE;
        /* fall through to next state */
        
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG3(librdf_hash_from_string,
                  "state %d at %s\n", state, p);
#endif

        /* got key\s+=\s+" now reading value */
      case HFS_PARSE_STATE_VALUE:
        value=p;
        backslashes=0;
        while(*p) {
          if(*p == '\\')
            /* backslashes are removed during value copy later */
            backslashes++; /* reduces real length */
          else if (*p == '\'')
            break;

          p++;
        }
        if(!*p)
          return 1;
        
        /* ' at end of value found */
        value_len=p-value;
        real_value_len=value_len-backslashes;
        new_value=(char*)LIBRDF_MALLOC(cstring, real_value_len+1);
        if(!new_value)
          return 1;
        for(i=0, to=new_value; i<(int)value_len; i++){
          if(value[i]=='\\')
            i++;
          *to++=value[i];
        }
        *to='\0';

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
        LIBRDF_DEBUG3(librdf_hash_from_string,
                      "decoded key >>%s<< (true) value >>%s<<\n", key, new_value);
#endif
        
        librdf_hash_put(hash, key, key_len, new_value, real_value_len);
        
        LIBRDF_FREE(cstring, new_value);
        
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
        LIBRDF_DEBUG1(librdf_hash_from_string,
                      "after decoding ");
        librdf_hash_print (hash, stderr) ;
        fputc('\n', stderr);
#endif
        state=HFS_PARSE_STATE_INIT;
        p++;

        break;
        
      default:
        LIBRDF_FATAL2(librdf_hash_from_string, "No such state %d", state);
    }
  }
  return 0;
}


/**
 * librdf_hash_from_array_of_strings - Initialise a hash from an array of strings
 * @hash: hash object
 * @array: address of the start of the array of char* pointers
 * 
 * Return value: 
 **/
int
librdf_hash_from_array_of_strings (librdf_hash* hash, char **array) 
{
  int i;
  char *key;
  char *value;
  
  for(i=0; (key=array[i]); i+=2) {
    value=array[i+1];
    if(!value)
      LIBRDF_FATAL2(librdf_hash_from_array_of_strings,
                    "Array contains an odd number of strings - %d", i);
    librdf_hash_put(hash, key, strlen(key), value, strlen(value));
  }
  return 0;
}


/**
 * librdf_hash_get_as_boolean - lookup a hash key and decode value as a boolean
 * @hash: &librdf_hash object
 * @key: key string to look up
 * 
 * Return value: >0 (for true), 0 (for false) or <0 (for key not found or not known boolean value)
 **/
int
librdf_hash_get_as_boolean (librdf_hash* hash, char *key) 
{
  int bvalue= (-1);
  char *value;

  value=librdf_hash_get(hash, key);
  if(!value)
    /* does not exist - fail */
    return -1;

  switch(strlen(value)) {
  case 2: /* try 'no' */
    if(*value=='n' && value[1]=='o')
      bvalue=0;
    break;
  case 3: /* try 'yes' */
    if(*value=='y' && value[1]=='e' && value[2]=='s')
      bvalue=1;
    break;
  case 4: /* try 'true' */
    if(*value=='t' && value[1]=='r' && value[2]=='u' && value[3]=='e')
      bvalue=1;
    break;
  case 5: /* try 'false' */
    if(!strncmp(value, "false", 5))
      bvalue=1;
    break;
  /* no need for default, bvalue is set above */
  }

  LIBRDF_FREE(cstring, value);

  return bvalue;
}


/**
 * librdf_hash_get_as_long - lookup a hash key and decode value as a long
 * @hash: &librdf_hash object
 * @key: key string to look up
 * 
 * Return value: >0 (for success), <0 (for key not found or not known boolean value)
 **/
long
librdf_hash_get_as_long (librdf_hash* hash, char *key) 
{
  int lvalue;
  char *value;
  char *end_ptr;
  
  value=librdf_hash_get(hash, key);
  if(!value)
    /* does not exist - fail */
    return -1;

  /* Using special base 0 which allows decimal, hex and octal */
  lvalue=strtol(value, &end_ptr, 0);

  /* nothing found, return error */
  if(end_ptr == value)
    lvalue= (-1);

  LIBRDF_FREE(cstring, value);
  return lvalue;
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_hash *h, *h2;
  char *test_hash_types[]={"gdbm", "bdb", "memory", NULL};
  char *test_hash_values[]={"colour","yellow", /* Made in UK, can you guess? */
			    "age", "new",
			    "size", "large",
                            "colour", "green",
			    "fruit", "banana",
                            "colour", "yellow",
			    NULL, NULL};
  char *test_duplicate_key="colour";
  char *test_hash_array[]={"shape", "cube",
			   "sides", "six",
			   "colours", "red",
			   "colours", "yellow",
			   "creator", "rubik",
			   NULL};
  char *test_hash_delete_key="size";
  int i,j;
  char *type;
  char *key, *value;
  char *program=argv[0];
  
  
  /* initialise hash module */
  librdf_init_hash();
  
  if(argc ==2) {
    type=argv[1];
    h=librdf_new_hash(NULL);
    if(!h) {
      fprintf(stderr, "%s: Failed to create new hash type '%s'\n",
	      program, type);
      return(0);
    }
    
    librdf_hash_open(h, "test", 0644, 1, 1, NULL);
    librdf_hash_from_string(h, argv[1]);
    fprintf(stdout, "%s: resulting ", program);    
    librdf_hash_print(h, stdout);
    fprintf(stdout, "\n");
    librdf_hash_close(h);
    librdf_free_hash(h);
    return(0);
  }
  
  
  for(i=0; (type=test_hash_types[i]); i++) {
    fprintf(stdout, "%s: Trying to create new %s hash\n", program, type);
    h=librdf_new_hash(type);
    if(!h) {
      fprintf(stderr, "%s: Failed to create new hash type '%s'\n", program, type);
      continue;
    }

    if(librdf_hash_open(h, "test", 0644, 1, 1, NULL)) {
      fprintf(stderr, "%s: Failed to open new hash type '%s'\n", program, type);
      continue;
    }
    
    
    for(j=0; test_hash_values[j]; j+=2) {
      key=test_hash_values[j];
      value=test_hash_values[j+1];
      fprintf(stdout, "%s: Adding key/value pair: %s=%s\n", program,
	      key, value);
      
      librdf_hash_put(h, key, strlen(key), value, strlen(value));
      
      fprintf(stdout, "%s: resulting ", program);    
      librdf_hash_print(h, stdout);
      fprintf(stdout, "\n");
    }
    
    fprintf(stdout, "%s: Deleting key '%s'\n", program, test_hash_delete_key);
    librdf_hash_delete(h, test_hash_delete_key, strlen(test_hash_delete_key));
    
    fprintf(stdout, "%s: resulting ", program);    
    librdf_hash_print(h, stdout);
    fprintf(stdout, "\n");
    
    fprintf(stdout, "%s: resulting %s hash keys: ", program, type);
    librdf_hash_print_keys(h, stdout);
    fprintf(stdout, "\n");

    fprintf(stdout, "%s: all values of key '%s'=", program, test_duplicate_key);
    librdf_hash_print_values(h, test_duplicate_key, stdout);
    fprintf(stdout, "\n");

    librdf_hash_close(h);
      
    fprintf(stdout, "%s: Freeing hash\n", program);
    librdf_free_hash(h);
  }
  fprintf(stdout, "%s: Getting default hash factory\n", program);
  h2=librdf_new_hash(NULL);
  if(!h2) {
    fprintf(stderr, "%s: Failed to create new hash from default factory\n", program);
    return(0);
  }

  fprintf(stdout, "%s: Initialising hash from array of strings\n", program);
  if(librdf_hash_from_array_of_strings(h2, test_hash_array)) {
    fprintf(stderr, "%s: Failed to init hash from array of strings\n", program);
    return(0);
  }
  
  fprintf(stdout, "%s: resulting hash ", program);    
  librdf_hash_print(h2, stdout);
  fprintf(stdout, "\n");

  fprintf(stdout, "%s: resulting hash keys: ", program);    
  librdf_hash_print_keys(h2, stdout);
  fprintf(stdout, "\n");

  librdf_hash_close(h2);
  
  fprintf(stdout, "%s: Freeing hash\n", program);
  librdf_free_hash(h2);
  
  /* finish hash module */
  librdf_finish_hash();
  
#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
