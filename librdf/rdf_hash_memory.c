/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash_memory.c - RDF Hash In Memory Implementation
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

/* Implementing hash keys iterator */
static librdf_hash_memory_node* librdf_hash_memory_keys_iterator_get_next_datum(void *context);
static int librdf_hash_memory_keys_iterator_have_elements(void* context);
static void* librdf_hash_memory_keys_iterator_get_next(void* context);
static void librdf_hash_memory_keys_iterator_finished(void* context);

/* Implementing the hash get iterator */
static int librdf_hash_memory_get_iterator_have_elements(void* context);
static void* librdf_hash_memory_get_iterator_get_next(void* context);
static void librdf_hash_memory_get_iterator_finished(void* context);




/* functions implementing the API */

static int librdf_hash_memory_open(void* context, char *identifier, void *mode, librdf_hash* options);
static int librdf_hash_memory_close(void* context);
static librdf_iterator* librdf_hash_memory_get(void* context, librdf_hash_datum *key, unsigned int flags);
static int librdf_hash_memory_put(void* context, librdf_hash_datum *key, librdf_hash_datum *data, unsigned int flags);
static int librdf_hash_memory_exists(void* context, librdf_hash_datum *key);
static int librdf_hash_memory_delete(void* context, librdf_hash_datum *key);
static librdf_iterator* librdf_hash_memory_keys(void* context);
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
 * librdf_hash_memory_open - Open a new memory hash
 * @context: memory hash contxt
 * @identifier: not used
 * @mode: access mode (currently unused)
 * @options: &librdf_hash of options (currently unused)
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_open(void* context, char *identifier, void *mode, 
                      librdf_hash* options) 
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;

  hash->load_factor=librdf_hash_default_load_factor;
  return librdf_hash_memory_expand_size(hash);
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
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;

  if(hash->nodes) {
    int i;
  
    for(i=0; i<hash->capacity; i++) {
      librdf_hash_memory_node *node=hash->nodes[i];
      
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
    LIBRDF_FREE(librdf_hash_memory_node_array, hash->nodes);
  }
  return 0;
}


typedef struct {
  librdf_hash_memory_context *hash;
  librdf_hash_memory_node_value *values;
} librdf_hash_memory_get_iterator_context;



/**
 * librdf_hash_memory_get - Retrieve a hash value for the given key
 * @context: memory hash context
 * @key: pointer to key to use
 * @data: pointer to data to return value
 * @flags: (not used at present)
 * 
 * Return value: non 0 on failure
 **/
static librdf_iterator*
librdf_hash_memory_get(void* context, librdf_hash_datum *key,
		       unsigned int flags) 
{
  librdf_hash_memory_context *hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_get_iterator_context *gcontext;
  librdf_hash_memory_node *node;

  /* find node for key */
  node=librdf_hash_memory_find_node(hash,
				    (char*)key->data, key->size,
				    NULL, NULL);
  /* not found */
  if(!node)
    return NULL;

  gcontext=(librdf_hash_memory_get_iterator_context*)LIBRDF_CALLOC(librdf_hash_memory_get_iterator_context, 1, sizeof(librdf_hash_memory_get_iterator_context));
  if(!gcontext)
    return NULL;
  
  gcontext->hash = hash;
  gcontext->values=node->values;

  return librdf_new_iterator((void*)gcontext,
                             librdf_hash_memory_get_iterator_have_elements,
                             librdf_hash_memory_get_iterator_get_next,
                             librdf_hash_memory_get_iterator_finished);
}


/**
 * librdf_hash_memory_get_iterator_have_elements - Check for the end of the iterator of statements from the SiRPARC parser
 * @context: iteration context
 * 
 * Return value: non 0 at end of iterator.
 **/
static int
librdf_hash_memory_get_iterator_have_elements(void* context)
{
  librdf_hash_memory_get_iterator_context* gcontext=(librdf_hash_memory_get_iterator_context*)context;

  return (gcontext->values != NULL);
}


/**
 * librdf_hash_memory_get_iterator_get_next - Get the next value from the iterator over key values
 * @context: hash memory get iterator context
 * 
 * Return value: next value or NULL at end of iterator.
 **/
static void*
librdf_hash_memory_get_iterator_get_next(void* context)
{
  librdf_hash_memory_get_iterator_context* gcontext=(librdf_hash_memory_get_iterator_context*)context;
  librdf_hash_memory_node_value *vnode;
  librdf_hash_datum *value;
  void *data;

  if(!(vnode=gcontext->values))
    return NULL;

  data=LIBRDF_MALLOC(cstring, vnode->value_len);
  if(!data)
    return NULL;
  memcpy(data, vnode->value, vnode->value_len);

  value=librdf_hash_datum_new(data, vnode->value_len);
  if(!value) {
    if(data)
      LIBRDF_FREE(cstring, data);
    return NULL;
  }
  
  gcontext->values=vnode->next;

  return (void*)value;
}


/**
 * librdf_hash_memory_get_iterator_finished - Finish the serialisation of the hash memory get
 * @context: hash memory get iterator context
 **/
static void
librdf_hash_memory_get_iterator_finished(void* context)
{
  librdf_hash_memory_get_iterator_context* gcontext=(librdf_hash_memory_get_iterator_context*)context;

  LIBRDF_FREE(librdf_hash_memory_get_iterator_context, gcontext);
}


/**
 * librdf_hash_memory_put: - Store a key/value pair in the hash
 * @context: memory hash context
 * @key: pointer to key to store
 * @value: pointer to value to store
 * @flags: flags (not used at present)
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_put(void* context, librdf_hash_datum *key, 
		       librdf_hash_datum *value, unsigned int flags) 
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



typedef struct {
  librdf_hash_memory_context* hash;
  int current_bucket;
  librdf_hash_memory_node* current_node;
  int have_elements;
} librdf_hash_memory_keys_iterator_context;


/**
 * librdf_hash_memory_keys - Get the hash keys
 * @context: memory hash context
 * 
 * Return value: &librdf_iterator serialisation of keys or NULL on failure
 **/
static librdf_iterator*
librdf_hash_memory_keys(void* context)
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_node* node=NULL;
  librdf_hash_memory_keys_iterator_context* scontext;
  int i;

  scontext=(librdf_hash_memory_keys_iterator_context*)LIBRDF_CALLOC(librdf_hash_memory_keys_iterator_context, 1, sizeof(librdf_hash_memory_keys_iterator_context));
  if(!scontext)
    return NULL;
  
  scontext->hash = hash;

  /* find first used bucket */
  scontext->current_bucket=0;
  for(i=0; i< hash->capacity; i++)
    if((node=hash->nodes[i])) {
      scontext->current_bucket=i;
      break;
    }

  /* if current bucket is now 0, iterator is OK but empty */
  scontext->have_elements= (!scontext) ? 0 : 1;

  scontext->current_node=node;

  return librdf_new_iterator((void*)scontext,
                             librdf_hash_memory_keys_iterator_have_elements,
                             librdf_hash_memory_keys_iterator_get_next,
                             librdf_hash_memory_keys_iterator_finished);
}


static librdf_hash_memory_node*
librdf_hash_memory_keys_iterator_get_next_datum(void *context) 
{
  librdf_hash_memory_keys_iterator_context* scontext=(librdf_hash_memory_keys_iterator_context*)context;
  librdf_hash_memory_node* node;

  if(!scontext->have_elements)
    return NULL;
  
  /* move on to next node in current bucket */
  if(!(node=scontext->current_node->next)) {
    int i;

    /* end of list - find next used bucket */
    for(i=scontext->current_bucket+1; i< scontext->hash->capacity; i++)
      if((node=scontext->hash->nodes[i])) {
        scontext->current_bucket=i;
        break;
      }

    if(!node)
      scontext->have_elements=0;
  }

  /* save new current node */
  scontext->current_node=node;
  
  return node;
}


/**
 * librdf_hash_memory_keys_iterator_have_elements - Check for the end of the iterator of statements from the SiRPARC parser
 * @context: hash memory keys iterator context
 * 
 * Return value: non 0 at end of iterator.
 **/
static int
librdf_hash_memory_keys_iterator_have_elements(void* context)
{
  librdf_hash_memory_keys_iterator_context* scontext=(librdf_hash_memory_keys_iterator_context*)context;

  if(!scontext->have_elements)
    return 0;

  /* already have 1 stored item - not end yet */
  if(scontext->current_node)
    return 1;

  /* get next datum / check for end */
  if(!librdf_hash_memory_keys_iterator_get_next_datum(scontext))
    scontext->have_elements=0;

  return scontext->have_elements;
}


/**
 * librdf_hash_memory_keys_iterator_get_next - Get the next eleemnt from the iterator
 * @context: hash memory keys iterator context
 * 
 * Return value: next value or NULL at end of iterator.
 **/
static void*
librdf_hash_memory_keys_iterator_get_next(void* context)
{
  librdf_hash_memory_keys_iterator_context* scontext=(librdf_hash_memory_keys_iterator_context*)context;
  librdf_hash_memory_node* node;
  librdf_hash_datum *key;
  void *data=NULL;

  if(!scontext->have_elements)
    return NULL;

  node=scontext->current_node;
  
  data=LIBRDF_MALLOC(cstring, node->key_len);
  if(!data)
    return NULL;
  memcpy(data, node->key, node->key_len);

  key=librdf_hash_datum_new(data, node->key_len);
  if(!key) {
    if(data)
      LIBRDF_FREE(cstring, data);
    return NULL;
  }

  /* get next datum */
  librdf_hash_memory_keys_iterator_get_next_datum(scontext);

  return (void*)key;
}


/**
 * librdf_hash_memory_keys_iterator_finished - Finish the serialisation of the hash memory keys
 * @context: hash memory keys iterator context
 **/
static void
librdf_hash_memory_keys_iterator_finished(void* context)
{
  librdf_hash_memory_keys_iterator_context* scontext=(librdf_hash_memory_keys_iterator_context*)context;

  LIBRDF_FREE(librdf_hash_memory_keys_iterator_context, scontext);
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
  
  factory->open    = librdf_hash_memory_open;
  factory->close   = librdf_hash_memory_close;
  factory->get     = librdf_hash_memory_get;
  factory->put     = librdf_hash_memory_put;
  factory->exists  = librdf_hash_memory_exists;
  factory->delete_key  = librdf_hash_memory_delete;
  factory->keys    = librdf_hash_memory_keys;
  factory->sync    = librdf_hash_memory_sync;
  factory->get_fd  = librdf_hash_memory_get_fd;
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
