/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash_memory.c - RDF Hash In Memory Implementation
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
#include <rdf_hash.h>
#include <rdf_hash_memory.h>


/* private structures */
struct librdf_hash_memory_node_value_s
{
  struct librdf_hash_memory_node_value_s* next;
  char *value;
  size_t value_len;
};
typedef struct librdf_hash_memory_node_value_s librdf_hash_memory_node_value;


struct librdf_hash_memory_node_s
{
  struct librdf_hash_memory_node_s* next;
  char *key;
  size_t key_len;
  unsigned long hash_key;
  librdf_hash_memory_node_value *values;
  int values_count;
};
typedef struct librdf_hash_memory_node_s librdf_hash_memory_node;


typedef struct
{
  /* the hash object */
  librdf_hash* hash;
  /* An array pointing to a list of nodes (buckets) */
  librdf_hash_memory_node** nodes;
  /* this many buckets used */
  int size;
  /* this many keys */
  int keys;
  /* this many values */
  int values;
  /* total array size */
  int capacity;

  /* array load factor expressed out of 1000.
   * Always true: (size/capacity * 1000) < load_factor,
   * or in the code: size * 1000 < load_factor * capacity
   */
  int load_factor;
} librdf_hash_memory_context;


/* statics */

/* default load_factor out of 1000 */
static int librdf_hash_default_load_factor=750;

/* starting capacity - MUST BE POWER OF 2 */
static int librdf_hash_initial_capacity=8;


/* prototypes for local functions */
static unsigned long librdf_hash_memory_crc32 (const unsigned char *s, unsigned int len);
static librdf_hash_memory_node* librdf_hash_memory_find_node(librdf_hash_memory_context* hash, char *key, size_t key_len, int *bucket, librdf_hash_memory_node** prev);
static void librdf_free_hash_memory_node(librdf_hash_memory_node* node);
static int librdf_hash_memory_expand_size(librdf_hash_memory_context* hash);

/* Implementing the hash cursor */
static int librdf_hash_memory_cursor_init(void *cursor_context, void *hash_context);
static int librdf_hash_memory_cursor_get(void* context, librdf_hash_datum* key, librdf_hash_datum* value, unsigned int flags);
static void librdf_hash_memory_cursor_finish(void* context);




/* functions implementing the API */

static int librdf_hash_memory_create(librdf_hash* new_hash, void* context);
static int librdf_hash_memory_destroy(void* context);
static int librdf_hash_memory_open(void* context, char *identifier, int mode, int is_writable, int is_new, librdf_hash* options);
static int librdf_hash_memory_close(void* context);
static int librdf_hash_memory_clone(librdf_hash* new_hash, void *new_context, char *new_identifier, void* old_context);
static int librdf_hash_memory_put(void* context, librdf_hash_datum *key, librdf_hash_datum *data);
static int librdf_hash_memory_exists(void* context, librdf_hash_datum *key);
static int librdf_hash_memory_delete(void* context, librdf_hash_datum *key);
static int librdf_hash_memory_sync(void* context);
static int librdf_hash_memory_get_fd(void* context);

static void librdf_hash_memory_register_factory(librdf_hash_factory *factory);


/* helper functions */

/* Return a CRC of the contents of the buffer. */
unsigned long
librdf_hash_memory_crc32 (const unsigned char *s, unsigned int len)
{
  unsigned int i;
  unsigned long crc32val;
  
  crc32val = 0;
  for (i=0;  i< len;  i++) {
    crc32val = (crc32val << 3) ^ s[i];
  }
  return crc32val;
}




static librdf_hash_memory_node*
librdf_hash_memory_find_node(librdf_hash_memory_context* hash, 
			     char *key, size_t key_len,
			     int *user_bucket,
			     librdf_hash_memory_node** prev) 
{
  librdf_hash_memory_node* node;
  int bucket;
  int hash_key;

  /* empty hash */
  if(!hash->capacity)
    return NULL;
  
  hash_key=librdf_hash_memory_crc32((unsigned char*)key, key_len);

  if(prev)
    *prev=NULL;

  /* find slot in table */
  bucket=hash_key & (hash->capacity - 1);
  if(user_bucket)
    *user_bucket=bucket;

  /* check if there is a list present */ 
  node=hash->nodes[bucket];
  if(!node)
    /* no list there */
    return NULL;
    
  /* walk the list */
  while(node) {
    if(key_len == node->key_len && !memcmp(key, node->key, key_len))
      break;
    if(prev)
      *prev=node;
    node=node->next;
  }

  return node;
}


static void
librdf_free_hash_memory_node(librdf_hash_memory_node* node) 
{
  if(node->key)
    LIBRDF_FREE(cstring, node->key);
  if(node->values) {
    librdf_hash_memory_node_value *vnode, *next;

    /* Empty the list of values */
    for(vnode=node->values; vnode; vnode=next) {
      next=vnode->next;
      if(vnode->value)
        LIBRDF_FREE(cstring, vnode->value);
      LIBRDF_FREE(librdf_hash_memory_node_value, vnode);
    }
    LIBRDF_FREE(librdf_hash_memory_node, node);
  }
}


static int
librdf_hash_memory_expand_size(librdf_hash_memory_context* hash) {
  int required_capacity=0;
  librdf_hash_memory_node **new_nodes;
  int i;

  if (hash->capacity) {
    /* big enough */
    if((1000 * hash->size) < (hash->load_factor * hash->capacity))
      return 0;
    /* grow hash (keeping it a power of two) */
    required_capacity=hash->capacity << 1;
  } else {
    required_capacity=librdf_hash_initial_capacity;
  }

  /* allocate new table */
  new_nodes=(librdf_hash_memory_node**)LIBRDF_CALLOC(librdf_hash_memory_nodes, 
						     required_capacity,
						     sizeof(librdf_hash_memory_node*));
  if(!new_nodes)
    return 1;


  /* it is a new hash empty hash - we are done */
  if(!hash->size) {
    hash->capacity=required_capacity;
    hash->nodes=new_nodes;
    return 0;
  }


  for(i=0; i<hash->capacity; i++) {
    librdf_hash_memory_node *node=hash->nodes[i];
      
    /* walk all attached nodes */
    while(node) {
      librdf_hash_memory_node *next;
      int bucket;

      next=node->next;
      /* find slot in new table */
      bucket=node->hash_key & (required_capacity - 1);
      node->next=new_nodes[bucket];
      new_nodes[bucket]=node;

      node=next;
    }
  }

  /* now free old table */
  LIBRDF_FREE(librdf_hash_memory_nodes, hash->nodes);

  /* attach new one */
  hash->capacity=required_capacity;
  hash->nodes=new_nodes;

  return 0;
}



/* functions implementing hash api */

/**
 * librdf_hash_memory_create - Create a new memory hash
 * @hash: &librdf_hash hash
 * @context: memory hash contxt
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_create(librdf_hash* hash, void* context) 
{
  librdf_hash_memory_context* hcontext=(librdf_hash_memory_context*)context;

  hcontext->hash=hash;
  hcontext->load_factor=librdf_hash_default_load_factor;
  return librdf_hash_memory_expand_size(hcontext);
}


/**
 * librdf_hash_memory_destroy - Destroy a memory hash
 * @context: memory hash context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_destroy(void* context) 
{
  librdf_hash_memory_context* hcontext=(librdf_hash_memory_context*)context;

  if(hcontext->nodes) {
    int i;
  
    for(i=0; i<hcontext->capacity; i++) {
      librdf_hash_memory_node *node=hcontext->nodes[i];
      
      /* this entry is used */
      if(node) {
	librdf_hash_memory_node *next;
	/* free all attached nodes */
	while(node) {
	  next=node->next;
	  librdf_free_hash_memory_node(node);
	  node=next;
	}
      }
    }
    LIBRDF_FREE(librdf_hash_memory_nodes, hcontext->nodes);
  }

  return 0;
}


/**
 * librdf_hash_memory_open - Open memory hash with given parameters
 * @context: memory hash context
 * @identifier: identifier - not used
 * @mode: access mode - not used
 * @is_writable: is hash writable? - not used
 * @is_new: is hash new? - not used
 * @options: &librdf_hash of options - not used
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_open(void* context, char *identifier,
                        int mode, int is_writable, int is_new,
                        librdf_hash* options) 
{
  /* NOP */
  return 0;
}


/**
 * librdf_hash_memory_close - Close the hash
 * @context: memory hash context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_close(void* context) 
{
  /* NOP */
  return 0;
}


static int
librdf_hash_memory_clone(librdf_hash *hash, void* context, char *new_identifer,
                         void *old_context) 
{
  librdf_hash_memory_context* hcontext=(librdf_hash_memory_context*)context;
  librdf_hash_memory_context* old_hcontext=(librdf_hash_memory_context*)old_context;
  librdf_hash_datum *key, *value;
  librdf_iterator *iterator;
  int status=0;
  
  /* copy data fields that might change */
  hcontext->hash=hash;
  hcontext->load_factor=old_hcontext->load_factor;

  /* Don't need to deal with new_identifier - not used for memory hashes */

  /* Use higher level functions to iterator this data
   * on the other hand, maybe this is a good idea since that
   * code is tested and works
   */

  key=librdf_new_hash_datum(NULL, 0);
  value=librdf_new_hash_datum(NULL, 0);

  iterator=librdf_hash_get_all(old_hcontext->hash, key, value);
  while(librdf_iterator_have_elements(iterator)) {
    librdf_iterator_get_next(iterator);

    if(librdf_hash_memory_put(hcontext, key, value)) {
      status=1;
      break;
    }
  }
  if(iterator)
    librdf_free_iterator(iterator);

  librdf_free_hash_datum(value);
  librdf_free_hash_datum(key);

  return status;
}



typedef struct {
  librdf_hash_memory_context* hash;
  void *last_key;
  void *last_value;
  int current_bucket;
  librdf_hash_memory_node* current_node;
  librdf_hash_memory_node_value *current_value;
} librdf_hash_memory_cursor_context;



/**
 * librdf_hash_memory_cursor_init - Initialise a new hash cursor
 * @cursor_context: hash cursor context
 * @hash_context: hash to operate over
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_memory_cursor_init(void *cursor_context, void *hash_context) 
{
  librdf_hash_memory_cursor_context *cursor=(librdf_hash_memory_cursor_context*)cursor_context;

  cursor->hash = (librdf_hash_memory_context*)hash_context;
  return 0;
}


/**
 * librdf_hash_memory_cursor_get - Retrieve a hash value for the given key
 * @context: memory hash cursor context
 * @key: pointer to key to use
 * @value: pointer to value to use
 * @flags: flags
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_cursor_get(void* context, 
                              librdf_hash_datum *key,
                              librdf_hash_datum *value,
                              unsigned int flags)
{
  librdf_hash_memory_cursor_context *cursor=(librdf_hash_memory_cursor_context*)context;
  librdf_hash_memory_node_value *vnode=NULL;
  librdf_hash_memory_node *node;
  

  /* Free previous key and values */
  if(cursor->last_key) {
    LIBRDF_FREE(cstring, cursor->last_key);
    cursor->last_key=NULL;
  }
    
  if(cursor->last_value) {
    LIBRDF_FREE(cstring, cursor->last_value);
    cursor->last_value=NULL;
  }


  /* First step, make sure cursor->current_node points to a valid node,
     if possible */

  /* Move to start of hash if necessary  */
  if(flags == LIBRDF_HASH_CURSOR_FIRST) {
    int i;
    
    cursor->current_node=NULL;
    /* find first used bucket (with keys) */
    cursor->current_bucket=0;

    for(i=0; i< cursor->hash->capacity; i++)
      if((cursor->current_node=cursor->hash->nodes[i])) {
        cursor->current_bucket=i;
        break;
      }

    if(cursor->current_node)
      cursor->current_value=cursor->current_node->values;
  }

  /* If still have no current node, try to find it from the key */
  if(!cursor->current_node && key && key->data) {
    cursor->current_node=librdf_hash_memory_find_node(cursor->hash,
                                                      (char*)key->data,
                                                      key->size,
                                                      NULL, NULL);
    if(cursor->current_node)
      cursor->current_value=cursor->current_node->values;
  }


  /* If still have no node, failed */
  if(!cursor->current_node)
    return 1;

  /* Check for end of values */

  switch(flags) {
    case LIBRDF_HASH_CURSOR_SET:
      /* If key does not exist, failed above, so test if there are values */

      /* FALLTHROUGH */
    case LIBRDF_HASH_CURSOR_NEXT_VALUE:
      /* If want values and have reached end of values list, end */
      if(!cursor->current_value)
        return 1;
      break;
      
    case LIBRDF_HASH_CURSOR_FIRST:
    case LIBRDF_HASH_CURSOR_NEXT:
      /* If have reached last bucket, end */
      if(cursor->current_bucket >= cursor->hash->capacity)
        return 1;
      
      break;
    default:
      abort();
  }
  

  /* Ok, there is data, retrieve it */

  switch(flags) {
    case LIBRDF_HASH_CURSOR_SET:

      /* FALLTHROUGH */
    case LIBRDF_HASH_CURSOR_NEXT_VALUE:
      vnode=cursor->current_value;
      
      /* copy value */
      value->data=vnode->value;
      value->size=vnode->value_len;
      
      /* move on */
      cursor->current_value=vnode->next;
      break;
      
    case LIBRDF_HASH_CURSOR_FIRST:
    case LIBRDF_HASH_CURSOR_NEXT:
      node=cursor->current_node;

      /* copy key */
      cursor->last_key = key->data = LIBRDF_MALLOC(cstring, node->key_len);
      if(!key->data)
        return 1;
  
      memcpy(key->data, node->key, node->key_len);
      key->size=node->key_len;

      /* if want values, walk through them */
      if(value) {
        vnode=cursor->current_value;
        
        /* copy value */
        cursor->last_value = value->data = LIBRDF_MALLOC(cstring,
                                                         vnode->value_len);
        if(!value->data)
          return 1;
    
        memcpy(value->data, vnode->value, vnode->value_len);
        value->size=vnode->value_len;

        /* move on */
        cursor->current_value=vnode->next;
        
        /* stop here if there are more values, otherwise need next
         * key & values so drop through and move to the next node
         */
        if(cursor->current_value)
          break;
      }
      
      /* move on to next node in current bucket */
      if(!(node=cursor->current_node->next)) {
        int i;
        
        /* end of list - move to next used bucket */
        for(i=cursor->current_bucket+1; i< cursor->hash->capacity; i++)
          if((node=cursor->hash->nodes[i])) {
            cursor->current_bucket=i;
            break;
          }
        
      }
      
      if((cursor->current_node=node))
        cursor->current_value=node->values;
      
      break;
    default:
      abort();
  }
  

  return 0;
}


/**
 * librdf_hash_memory_cursor_finished - Finish the serialisation of the hash memory get
 * @context: hash memory get iterator context
 **/
static void
librdf_hash_memory_cursor_finish(void* context)
{
  librdf_hash_memory_cursor_context *cursor=(librdf_hash_memory_cursor_context*)context;

  if(cursor->last_key)
    LIBRDF_FREE(cstring, cursor->last_key);
    
  if(cursor->last_value)
    LIBRDF_FREE(cstring, cursor->last_value);
}


/**
 * librdf_hash_memory_put: - Store a key/value pair in the hash
 * @context: memory hash context
 * @key: pointer to key to store
 * @value: pointer to value to store
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_put(void* context, librdf_hash_datum *key, 
		       librdf_hash_datum *value) 
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_node *node;
  librdf_hash_memory_node_value *vnode;
  int hash_key;
  char *new_key=NULL;
  char *new_value;
  int bucket= (-1);
  int is_new_node;

  /* ensure there is enough space in the hash */
  if (librdf_hash_memory_expand_size(hash))
    return 1;
  
  /* find node for key */
  node=librdf_hash_memory_find_node(hash,
				    (char*)key->data, key->size,
				    NULL, NULL);

  is_new_node=(node == NULL);
  
  /* not found - new key */
  if(is_new_node) {
    hash_key=librdf_hash_memory_crc32((unsigned char*)key->data, key->size);

    bucket=hash_key & (hash->capacity - 1);

    /* allocate new node */
    node=(librdf_hash_memory_node*)LIBRDF_CALLOC(librdf_hash_memory_node, 1,
                                                 sizeof(librdf_hash_memory_node));
    if(!node)
      return 1;

    node->hash_key=hash_key;
    
    /* allocate key for new node */
    new_key=(char*)LIBRDF_MALLOC(cstring, key->size);
    if(!new_key) {
      LIBRDF_FREE(librdf_hash_memory_node, node);
      return 1;
    }

    /* copy new key */
    memcpy(new_key, key->data, key->size);
    node->key=new_key;
    node->key_len=key->size;
  }
  
  
  /* always allocate new value */
  new_value=(char*)LIBRDF_MALLOC(cstring, value->size);
  if(!new_value) {
    if(is_new_node) {
      LIBRDF_FREE(cstring, new_key);
      LIBRDF_FREE(librdf_hash_memory_node, node);
    }
    return 1;
  }

  /* always allocate new librdf_hash_memory_node_value */
  vnode=(librdf_hash_memory_node_value*)LIBRDF_CALLOC(librdf_hash_memory_node_value, 1, sizeof(librdf_hash_memory_node_value));
  if(!vnode) {
    LIBRDF_FREE(cstring, new_value);
    if(is_new_node) {
      LIBRDF_FREE(cstring, new_key);
      LIBRDF_FREE(librdf_hash_memory_node, node);
    }
    return 1;
  }

  /* if we get here, all allocations succeeded */


  /* put new value node in list */
  vnode->next=node->values;
  node->values=vnode;

  /* note that in counter */
  node->values_count++;
 
  /* copy new value */
  memcpy(new_value, value->data, value->size);
  vnode->value=new_value;
  vnode->value_len=value->size;


  /* now update buckets and hash counts */
  if(is_new_node) {
    node->next=hash->nodes[bucket];
    hash->nodes[bucket]=node;
  
    hash->keys++;
  }
  

  hash->values++;

  /* Only increase bucket count use when previous value was NULL */
  if(!node->next)
    hash->size++;

  return 0;
}


/**
 * librdf_hash_memory_exists - Test the existence of a key in the hash
 * @context: memory hash context
 * @key: pointer to key to store
 * 
 * Return value: non 0 if the key exists in the hash
 **/
static int
librdf_hash_memory_exists(void* context, librdf_hash_datum *key) 
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_node* node;
  
  node=librdf_hash_memory_find_node(hash,
				    (char*)key->data, key->size,
				    NULL, NULL);
  return (node != NULL);
}



/**
 * librdf_hash_memory_delete: - Delete a key and all its values from the hash
 * @context: memory hash context
 * @key: pointer to key to delete
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_delete(void* context, librdf_hash_datum *key) 
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_node *node, *prev;
  int bucket;
  
  node=librdf_hash_memory_find_node(hash, 
				    (char*)key->data, key->size,
				    &bucket, &prev);
  /* not found anywhere */
  if(!node)
    return 1;

  /* search list from here */
  if(!prev) {
    /* is at start of list, so delete from there */
    if(!(hash->nodes[bucket]=node->next))
      /* hash bucket occupancy is one less if bucket is now empty */
      hash->size--;
  } else
    prev->next=node->next;

  /* update hash counts */
  hash->keys--;
  hash->values-= node->values_count;
  
  /* free node */
  librdf_free_hash_memory_node(node);
  return 0;
}


/**
 * librdf_hash_memory_sync - Flush the hash to disk
 * @context: memory hash context
 * 
 * Not used
 * 
 * Return value: 0
 **/
static int
librdf_hash_memory_sync(void* context) 
{
  /* Not applicable */
  return 0;
}


/**
 * librdf_hash_memory_get_fd - Get the file descriptor representing the hash
 * @context: memory hash context
 * 
 * Not used
 * 
 * Return value: -1
 **/
static int
librdf_hash_memory_get_fd(void* context) 
{
  /* Not applicable */
  return -1;
}


/* local function to register memory hash functions */

/**
 * librdf_hash_memory_register_factory - Register the memory hash module with the hash factory
 * @factory: hash factory prototype
 * 
 **/
static void
librdf_hash_memory_register_factory(librdf_hash_factory *factory) 
{
  factory->context_length = sizeof(librdf_hash_memory_context);
  factory->cursor_context_length = sizeof(librdf_hash_memory_cursor_context);
  
  factory->create  = librdf_hash_memory_create;
  factory->destroy = librdf_hash_memory_destroy;

  factory->open    = librdf_hash_memory_open;
  factory->close   = librdf_hash_memory_close;
  factory->clone   = librdf_hash_memory_clone;

  factory->put     = librdf_hash_memory_put;
  factory->exists  = librdf_hash_memory_exists;
  factory->delete_key  = librdf_hash_memory_delete;
  factory->sync    = librdf_hash_memory_sync;
  factory->get_fd  = librdf_hash_memory_get_fd;

  factory->cursor_init   = librdf_hash_memory_cursor_init;
  factory->cursor_get    = librdf_hash_memory_cursor_get;
  factory->cursor_finish = librdf_hash_memory_cursor_finish;
}

/**
 * librdf_init_hash_memory - Initialise the memory hash module
 * @default_load_factor: Default hash load factor (0-1000)
 * 
 * Initialises the memory hash module and sets the default hash load factor.
 *
 * The recommended and current default value is 0.75, i.e. 750/1000.  
 * To use the default value (whatever it is) use a value less than 0.
 **/
void
librdf_init_hash_memory(int default_load_factor)
{
  if(default_load_factor > 0 && default_load_factor < 1000)
    librdf_hash_default_load_factor=default_load_factor;

  librdf_hash_register_factory("memory", &librdf_hash_memory_register_factory);
}
