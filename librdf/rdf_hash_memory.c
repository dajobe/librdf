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
struct librdf_hash_memory_node_s
{
  struct librdf_hash_memory_node_s* next;
  char *key;
  size_t key_len;
  char *value;
  size_t value_len;
  unsigned long hash_key;
};
typedef struct librdf_hash_memory_node_s librdf_hash_memory_node;


typedef struct
{
  /* An array pointing to a list of nodes */
  librdf_hash_memory_node** nodes;
  /* Currently this many array entries used */
  int size;
  /* Total array length */
  int capacity;
  /* array load factor expressed out of 1000.
   * Always true: (size/capacity * 1000) < load_factor,
   * or in the code: size * 1000 < load_factor * capacity
   */
  int load_factor;
  /* used for get_seq */
  int current_bucket;
  librdf_hash_memory_node* current_node;
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

/* functions implementing the API */

static int librdf_hash_memory_open(void* context, char *identifier, void *mode, librdf_hash* options);
static int librdf_hash_memory_close(void* context);
static int librdf_hash_memory_get(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags);
static int librdf_hash_memory_put(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags);
static int librdf_hash_memory_exists(void* context, librdf_hash_data *key);
static int librdf_hash_memory_delete(void* context, librdf_hash_data *key);
static int librdf_hash_memory_get_seq(void* context, librdf_hash_data *key, librdf_hash_sequence_type type);
static int librdf_hash_memory_sync(void* context);
static int librdf_hash_memory_get_fd(void* context);

static void librdf_hash_memory_register_factory(librdf_hash_factory *factory);


/* helper functions */


/* Copyright for this function */

  /* ============================================================= */
  /*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or       */
  /*  code or tables extracted from it, as desired without restriction.     */
  /*                                                                        */
  /*  First, the polynomial itself and its table of feedback terms.  The    */
  /*  polynomial is                                                         */
  /*  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0   */
  /*                                                                        */
  /*  Note that we take it "backwards" and put the highest-order term in    */
  /*  the lowest-order bit.  The X^32 term is "implied"; the LSB is the     */
  /*  X^31 term, etc.  The X^0 term (usually shown as "+1") results in      */
  /*  the MSB being 1.                                                      */
  /*                                                                        */
  /*  Note that the usual hardware shift register implementation, which     */
  /*  is what we're using (we're merely optimizing it by doing eight-bit    */
  /*  chunks at a time) shifts bits into the lowest-order term.  In our     */
  /*  implementation, that means shifting towards the right.  Why do we     */
  /*  do it this way?  Because the calculated CRC must be transmitted in    */
  /*  order from highest-order term to lowest-order term.  UARTs transmit   */
  /*  characters in order from LSB to MSB.  By storing the CRC this way,    */
  /*  we hand it to the UART in the order low-byte to high-byte; the UART   */
  /*  sends each low-bit to hight-bit; and the result is transmission bit   */
  /*  by bit from highest- to lowest-order term without requiring any bit   */
  /*  shuffling on our part.  Reception works similarly.                    */
  /*                                                                        */
  /*  The feedback terms table consists of 256, 32-bit entries.  Notes:     */
  /*                                                                        */
  /*      The table can be generated at runtime if desired; code to do so   */
  /*      is shown later.  It might not be obvious, but the feedback        */
  /*      terms simply represent the results of eight shift/xor opera-      */
  /*      tions for all combinations of data and CRC register values.       */
  /*                                                                        */
  /*      The values must be right-shifted by eight bits by the "updcrc"    */
  /*      logic; the shift must be unsigned (bring in zeroes).  On some     */
  /*      hardware you could probably optimize the shift in assembler by    */
  /*      using byte-swap instructions.                                     */
  /*      polynomial $edb88320                                              */
  /*                                                                        */
  /*  --------------------------------------------------------------------  */

static unsigned long librdf_hash_memory_crc32_tab[] = {
      0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
      0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
      0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
      0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
      0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
      0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
      0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
      0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
      0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
      0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
      0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
      0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
      0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
      0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
      0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
      0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
      0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
      0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
      0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
      0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
      0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
      0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
      0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
      0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
      0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
      0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
      0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
      0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
      0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
      0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
      0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
      0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
      0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
      0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
      0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
      0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
      0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
      0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
      0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
      0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
      0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
      0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
      0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
      0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
      0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
      0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
      0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
      0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
      0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
      0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
      0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
      0x2d02ef8dL
   };

/* Return a 32-bit CRC of the contents of the buffer. */

unsigned long
librdf_hash_memory_crc32 (const unsigned char *s, unsigned int len)
{
  register unsigned int i;
  register unsigned long crc32val;
  
  crc32val = 0;
  for (i=0;  i< len;  i++) {
    crc32val = librdf_hash_memory_crc32_tab[(crc32val ^ s[i]) & 0xff] ^
               (crc32val >> 8);
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

  hash_key=librdf_hash_memory_crc32(key, key_len);

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
  if(node->value)
    LIBRDF_FREE(cstring, node->value);
  LIBRDF_FREE(librdf_hash_memory_node, node);
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
 * librdf_hash_memory_open:
 * @context: memory hash contxt
 * @identifier: not used
 * @mode: access mode (currently unused)
 * @options: &librdf_hash of options (currently unused)
 * 
 * Open a new memory hash.
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
 * librdf_hash_memory_close:
 * @context: memory hash context
 * 
 * Finish the association between the rdf hash and the memory hash.
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


/**
 * librdf_hash_memory_get:
 * @context: memory hash context
 * @key: pointer to key to use
 * @data: pointer to data to return value
 * @flags: (not used at present)
 * 
 * Retrieve a hash value for the given key
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_get(void* context, librdf_hash_data *key,
		       librdf_hash_data *data, unsigned int flags) 
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_node* node;

  node=librdf_hash_memory_find_node(hash,
				    (char*)key->data, key->size,
				    NULL, NULL);
  if(!node) {
    /* not found */
    data->data = NULL;
    return 0;
  }

  data->data = LIBRDF_MALLOC(hash_memory_data, node->value_len);
  if (!data->data)
    return 1;

  memcpy(data->data, node->value, node->value_len);
  data->size = node->value_len;
  
  return 0;
}


/**
 * librdf_hash_memory_put:
 * @context: memory hash context
 * @key: pointer to key to store
 * @value: pointer to value to store
 * @flags: flags (not used at present)
 * 
 * Store a key/value pair in the hash
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_put(void* context, librdf_hash_data *key, 
		       librdf_hash_data *value, unsigned int flags) 
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_node *node;
  int hash_key;
  int bucket;
  char *new_key;
  char *new_value;

  /* ensure there is enough space in the hash */
  if (librdf_hash_memory_expand_size(hash))
    return 1;
  
  hash_key=librdf_hash_memory_crc32(key->data, key->size);

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
  
  /* allocate new value */
  new_value=(char*)LIBRDF_MALLOC(cstring, value->size);
  if(!new_value) {
    LIBRDF_FREE(cstring, new_key);
    LIBRDF_FREE(librdf_hash_memory_node, node);
    return 1;
  }
  
  /* at this point, all mallocs worked and hash hasn't been touched */
  /* copy new key */
  memcpy(new_key, key->data, key->size);
  node->key=new_key;
  node->key_len=key->size;
  
  /* copy new value */
  memcpy(new_value, value->data, value->size);
  node->value=new_value;
  node->value_len=value->size;

 
  /* only now touch hash */
  node->next=hash->nodes[bucket];
  hash->nodes[bucket]=node;

  hash->size++;

  return 0;
}


/**
 * librdf_hash_memory_exists:
 * @context: memory hash context
 * @key: pointer to key to store
 * 
 * Test the existence of a key in the hash.
 * 
 * Return value: non 0 if the key exists in the hash
 **/
static int
librdf_hash_memory_exists(void* context, librdf_hash_data *key) 
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_node* node;
  
  node=librdf_hash_memory_find_node(hash,
				    (char*)key->data, key->size,
				    NULL, NULL);
  return (node != NULL);
}



/**
 * librdf_hash_memory_delete:
 * @context: memory hash context
 * @key: pointer to key to delete
 * 
 * Delete a key and associated value from the hash.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_delete(void* context, librdf_hash_data *key) 
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
  if(!prev)
    /* is at start of list, so delete from there */
    hash->nodes[bucket]=node->next;
  else
    prev->next=node->next;
  
  /* free node */
  librdf_free_hash_memory_node(node);
  return 0;
}


/**
 * librdf_hash_memory_get_seq:
 * @context: memory hash context
 * @key: pointer to the key
 * @type: type of operation
 * 
 * Start/get keys in a sequence from the hash.  Valid operations are
 * LIBRDF_HASH_SEQUENCE_FIRST to get the first key in the sequence
 * LIBRDF_HASH_SEQUENCE_NEXT to get the next in sequence and
 * LIBRDF_HASH_SEQUENCE_CURRENT to get the current key (again).
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_memory_get_seq(void* context, librdf_hash_data *key, 
			   librdf_hash_sequence_type type) 
{
  librdf_hash_memory_context* hash=(librdf_hash_memory_context*)context;
  librdf_hash_memory_node* node=NULL;
  
  if(type == LIBRDF_HASH_SEQUENCE_FIRST) {
    int i;
    /* find first used bucket */
    for(i=0; i< hash->capacity; i++)
      if((node=hash->nodes[i])) {
	hash->current_bucket=i;
	break;
      }
  } else if (type == LIBRDF_HASH_SEQUENCE_NEXT) {
    /* move on to next node in current bucket */
    if(!(node=hash->current_node->next)) {
      int i;
      /* end of list - find next used bucket */
      for(i=hash->current_bucket+1; i< hash->capacity; i++)
	if((node=hash->nodes[i])) {
	  hash->current_bucket=i;
	  break;
	}
    }
  } else { /* LIBRDF_HASH_SEQUENCE_CURRENT */
    node=hash->current_node;
  }
  
  if(!node) {
    key->data = NULL;
  } else {
    key->data = LIBRDF_MALLOC(cstring, node->key_len);
    if(!key->data)
      return 1;
    memcpy(key->data, node->key, node->key_len);
    key->size = node->key_len;
  }
  
  /* save new current node */
  hash->current_node=node;
  
  return 0;
}


/**
 * librdf_hash_memory_sync:
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
 * librdf_hash_memory_get_fd:
 * @context: memory hash context
 * 
 * Not used
 * 
 * Return value: 0
 **/
static int
librdf_hash_memory_get_fd(void* context) 
{
  /* Not applicable */
  return 0;
}


/* local function to register memory hash functions */

/**
 * librdf_hash_memory_register_factory:
 * @factory: hash factory prototype
 * 
 * Register the memory hash module with the hash factory
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
  factory->get_seq = librdf_hash_memory_get_seq;
  factory->sync    = librdf_hash_memory_sync;
  factory->get_fd  = librdf_hash_memory_get_fd;
}

/**
 * librdf_init_hash_memory:
 * 
 * Initialise the memory hash module.
 **/
void
librdf_init_hash_memory(int default_load_factor)
{
  if(default_load_factor >= 0 && default_load_factor < 1000)
    librdf_hash_default_load_factor=default_load_factor;

  librdf_hash_register_factory("memory", &librdf_hash_memory_register_factory);
}
