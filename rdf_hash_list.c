/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash_list.c - RDF Hash List Interface Implementation
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
#include <rdf_hash_list.h>


/* private structure */
struct librdf_hash_list_node_s
{
  struct librdf_hash_list_node_s* next;
  char *key;
  size_t key_len;
  char *value;
  size_t value_len;
};
typedef struct librdf_hash_list_node_s librdf_hash_list_node;

typedef struct
{
  librdf_hash_list_node* first;
  librdf_hash_list_node* current_key;
} librdf_hash_list_context;


/* prototypes for local functions */
static librdf_hash_list_node* librdf_hash_list_find_node(librdf_hash_list_node* list, char *key, size_t key_len, librdf_hash_list_node** prev);
static void librdf_free_hash_list_node(librdf_hash_list_node* node);

static int librdf_hash_list_open(void* context, char *identifier, void *mode, librdf_hash* options);
static int librdf_hash_list_close(void* context);
static int librdf_hash_list_get(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags);
static int librdf_hash_list_put(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags);
static int librdf_hash_list_exists(void* context, librdf_hash_data *key);
static int librdf_hash_list_delete(void* context, librdf_hash_data *key);
static int librdf_hash_list_get_seq(void* context, librdf_hash_data *key, librdf_hash_sequence_type type);
static int librdf_hash_list_sync(void* context);
static int librdf_hash_list_get_fd(void* context);

static void librdf_hash_list_register_factory(librdf_hash_factory *factory);


/* helper functions */
static librdf_hash_list_node*
librdf_hash_list_find_node(librdf_hash_list_node* list, 
                           char *key, size_t key_len,
                           librdf_hash_list_node** prev) 
{
  librdf_hash_list_node* node;
  
  if(prev)
    *prev=list;
  for(node=list; node; node=node->next) {
    if(key_len == node->key_len &&
       !memcmp(key, node->key, key_len))
      break;
    if(prev)
      *prev=node;
  }
  return node;
}


static void
librdf_free_hash_list_node(librdf_hash_list_node* node) 
{
  if(node->key)
    LIBRDF_FREE(cstring, node->key);
  if(node->value)
    LIBRDF_FREE(cstring, node->value);
  LIBRDF_FREE(librdf_hash_list_node, node);
}


/* functions implementing hash api */

static int
librdf_hash_list_open(void* context, char *identifier, void *mode, 
                      librdf_hash* options) 
{
  /* nop */
  return 0;
}

static int
librdf_hash_list_close(void* context) 
{
  librdf_hash_list_context* list_context=(librdf_hash_list_context*)context;
  librdf_hash_list_node *node, *next;
  
  for(node=list_context->first; node; node=next) {
    next=node->next;
    librdf_free_hash_list_node(node);
  }
  return 0;
}


static int
librdf_hash_list_get(void* context, librdf_hash_data *key,
		     librdf_hash_data *data, unsigned int flags) 
{
  librdf_hash_list_context* list_context=(librdf_hash_list_context*)context;
  librdf_hash_list_node* node;
  
  node=librdf_hash_list_find_node(list_context->first, (char*)key->data,
                                  key->size, NULL);
  if(!node) {
    /* not found */
    data->data = NULL;
    return 0;
  }

  data->data = LIBRDF_MALLOC(hash_list_data, node->value_len);
  if (!data->data)
    return 1;

  memcpy(data->data, node->value, node->value_len);
  data->size = node->value_len;
  
  return 0;
}


static int
librdf_hash_list_put(void* context, librdf_hash_data *key, 
		     librdf_hash_data *value, unsigned int flags) 
{
  librdf_hash_list_context* list_context=(librdf_hash_list_context*)context;
  librdf_hash_list_node* node;
  int is_new_node;
  char *new_key;
  char *new_value;
  
  node=librdf_hash_list_find_node(list_context->first,
                                  (char*)key->data, key->size, NULL);
  
  is_new_node=(node == NULL);
  
  if(is_new_node) {
    /* allocate new node */
    node=(librdf_hash_list_node*)LIBRDF_CALLOC(librdf_hash_list_node, 1,
                                               sizeof(librdf_hash_list_node));
    if(!node)
      return 1;
    
    /* allocate key for new node */
    new_key=(char*)LIBRDF_MALLOC(cstring, key->size);
    if(!new_key) {
      LIBRDF_FREE(librdf_hash_list_node, node);
      return 1;
    }
  }
  
  /* always allocate new value */
  new_value=(char*)LIBRDF_MALLOC(cstring, value->size);
  if(!new_value) {
    if(is_new_node) {
      LIBRDF_FREE(cstring, new_key);
      LIBRDF_FREE(librdf_hash_list_node, node);
    }
    return 1;
  }
  
  /* at this point, all mallocs worked and list hasn't been touched */
  if(is_new_node) {
    /* new node - copy new key */
    memcpy(new_key, key->data, key->size);
    node->key=new_key;
    node->key_len=key->size;
  } else {
    /* old node - delete old value */
    LIBRDF_FREE(cstring, node->value);
  }
  
  /* always copy new value */
  memcpy(new_value, value->data, value->size);
  node->value=new_value;
  node->value_len=value->size;
  
  if(is_new_node) {
    /* only now touch list */
    node->next=list_context->first;
    list_context->first=node;
  }
  return 0;
}


static int
librdf_hash_list_exists(void* context, librdf_hash_data *key) 
{
  librdf_hash_list_context* list_context=(librdf_hash_list_context*)context;
  librdf_hash_list_node* node;
  
  node=librdf_hash_list_find_node(list_context->first,
                                  (char*)key->data, key->size, NULL);
  return (node != NULL);
}



static int
librdf_hash_list_delete(void* context, librdf_hash_data *key) 
{
  librdf_hash_list_context* list_context=(librdf_hash_list_context*)context;
  librdf_hash_list_node *node, *prev;
  
  node=librdf_hash_list_find_node(list_context->first, 
                                  (char*)key->data, key->size, &prev);
  /* not found */
  if(!node)
    return 1;

  if(node == list_context->first)
    list_context->first=node->next;
  else
    prev->next=node->next;
  
  /* free node */
  librdf_free_hash_list_node(node);
  return 0;
}


static int
librdf_hash_list_get_seq(void* context, librdf_hash_data *key, 
			 librdf_hash_sequence_type type) 
{
  librdf_hash_list_context* list_context=(librdf_hash_list_context*)context;
  librdf_hash_list_node* node;
  
  if(type == LIBRDF_HASH_SEQUENCE_FIRST) {
    node=list_context->first;
  } else if (type == LIBRDF_HASH_SEQUENCE_NEXT) {
    node=list_context->current_key ? list_context->current_key->next : NULL;
  } else { /* LIBRDF_HASH_SEQUENCE_CURRENT */
    node=list_context->current_key;
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
  
  /* save new current key */
  list_context->current_key=node;
  
  return 0;
}


static int
librdf_hash_list_sync(void* context) 
{
  /* Not applicable */
  return 0;
}


static int
librdf_hash_list_get_fd(void* context) 
{
  /* Not applicable */
  return 0;
}


/* local function to register LIST hash functions */

static void
librdf_hash_list_register_factory(librdf_hash_factory *factory) 
{
  factory->context_length = sizeof(librdf_hash_list_context);
  
  factory->open    = librdf_hash_list_open;
  factory->close   = librdf_hash_list_close;
  factory->get     = librdf_hash_list_get;
  factory->put     = librdf_hash_list_put;
  factory->exists  = librdf_hash_list_exists;
  factory->delete_key  = librdf_hash_list_delete;
  factory->get_seq = librdf_hash_list_get_seq;
  factory->sync    = librdf_hash_list_sync;
  factory->get_fd  = librdf_hash_list_get_fd;
}

void
librdf_init_hash_list(void)
{
  librdf_hash_register_factory("LIST", &librdf_hash_list_register_factory);
}
