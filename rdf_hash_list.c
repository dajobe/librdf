/*
 * RDF Hash List Interface Implementation
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
#ifdef HAVE_STRING_H
#include <string.h> /* for strncmp */
#endif

#include <rdf_config.h>
#include <rdf_hash.h>
#include <rdf_hash_list.h>


struct rdf_hash_list_node_s
{
  struct rdf_hash_list_node_s* next;
  char *key;
  size_t key_len;
  char *value;
  size_t value_len;
};
typedef struct rdf_hash_list_node_s rdf_hash_list_node;

typedef struct
{
  rdf_hash_list_node* first;
  rdf_hash_list_node* current_key;
} rdf_hash_list_context;


/* prototypes for local functions */
static rdf_hash_list_node* rdf_hash_list_find_node(rdf_hash_list_node* list, char *key, size_t key_len, rdf_hash_list_node** prev);
static void free_rdf_hash_list_node(rdf_hash_list_node* node);

static int rdf_hash_list_open(void* context, char *identifier, void *mode, void *options);
static int rdf_hash_list_close(void* context);
static int rdf_hash_list_get(void* context, rdf_hash_data *key, rdf_hash_data *data, unsigned int flags);
static int rdf_hash_list_put(void* context, rdf_hash_data *key, rdf_hash_data *data, unsigned int flags);
static int rdf_hash_list_delete(void* context, rdf_hash_data *key);
static int rdf_hash_list_get_seq(void* context, rdf_hash_data *key, unsigned int flags);
static int rdf_hash_list_sync(void* context);
static int rdf_hash_list_get_fd(void* context);

static void rdf_hash_list_register_factory(rdf_hash_factory *factory);


/* helper functions */
static rdf_hash_list_node*
rdf_hash_list_find_node(rdf_hash_list_node* list, char *key, size_t key_len,
                        rdf_hash_list_node** prev) 
{
  rdf_hash_list_node* node;
  
  if(prev)
    *prev=list;
  for(node=list; node; node=node->next) {
    if(key_len == node->key_len && !strncmp(key, node->key, key_len))
      break;
    if(prev)
      *prev=node;
  }
  return node;
}

static void
free_rdf_hash_list_node(rdf_hash_list_node* node) 
  {
  if(node->key)
    RDF_FREE(cstring, node->key);
  if(node->value)
    RDF_FREE(cstring, node->value);
  RDF_FREE(rdf_hash_list_node, node);
}


/* functions implementing hash api */

static int
rdf_hash_list_open(void* context, char *identifier, void *mode, void *options) 
{
  /* nop */
  return 0;
}

static int
rdf_hash_list_close(void* context) 
{
  rdf_hash_list_context* list_context=(rdf_hash_list_context*)context;
  rdf_hash_list_node *node, *next;
  
  for(node=list_context->first; node; node=next) {
    next=node->next;
    free_rdf_hash_list_node(node);
  }
  return 0;
}


static int
rdf_hash_list_get(void* context, rdf_hash_data *key, rdf_hash_data *data, unsigned int flags) 
{
  rdf_hash_list_context* list_context=(rdf_hash_list_context*)context;
  rdf_hash_list_node* node;

  node=rdf_hash_list_find_node(list_context->first, (char*)key->data, key->size, NULL);
  if(!node) {
    /* not found */
    data->data = NULL;
    return 0;
  }
  data->data = RDF_MALLOC(hash_list_data, node->value_len);
  if(!data->data) {
    return 1;
  }
  memcpy(data->data, node->value, node->value_len);
  data->size = node->value_len;

  return 0;
}


static int
rdf_hash_list_put(void* context, rdf_hash_data *key, rdf_hash_data *value, unsigned int flags) 
{
  rdf_hash_list_context* list_context=(rdf_hash_list_context*)context;
  rdf_hash_list_node* node;
  int is_new_node;
  char *new_key;
  char *new_value;
  
  node=rdf_hash_list_find_node(list_context->first, (char*)key->data, key->size, NULL);

  is_new_node=(node == NULL);

  if(is_new_node) {
    /* need new node */
    node=(rdf_hash_list_node*)RDF_CALLOC(rdf_hash_list_node, sizeof(rdf_hash_list_node), 1);
    if(!node)
      return 1;
  }

  /* allocate key only for a new node */
  if(is_new_node) {
    new_key=(char*)RDF_MALLOC(cstring, key->size);
    if(!new_key) {
      RDF_FREE(rdf_hash_list_node, node);
      return 1;
    }
  }
    
  new_value=(char*)RDF_MALLOC(cstring, value->size);
  if(!new_value) {
    RDF_FREE(cstring, new_key);
    if(is_new_node)
      RDF_FREE(rdf_hash_list_node, node);
    return 1;
  }

  /* at this point, all mallocs worked and list hasn't been touched*/
  if(is_new_node) {
    memcpy(new_key, key->data, key->size);
    node->key=new_key;
    node->key_len=key->size;
  } else {
    /* only now delete old data */
    RDF_FREE(cstring, node->value);
  }
  
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
rdf_hash_list_delete(void* context, rdf_hash_data *key) 
{
  rdf_hash_list_context* list_context=(rdf_hash_list_context*)context;
  rdf_hash_list_node *node, *prev;

  node=rdf_hash_list_find_node(list_context->first, (char*)key->data, key->size, &prev);
  if(!node) {
    /* not found */
    return 1;
  }
  if(node == list_context->first)
    list_context->first=node->next;
  else
    prev->next=node->next;

  /* free node */
  free_rdf_hash_list_node(node);
  return 0;
}


static int
rdf_hash_list_get_seq(void* context, rdf_hash_data *key, unsigned int flags) 
{
  rdf_hash_list_context* list_context=(rdf_hash_list_context*)context;
  rdf_hash_list_node* node;

  if(flags == RDF_HASH_FLAGS_FIRST) {
    node=list_context->first;
  } else if (flags == RDF_HASH_FLAGS_NEXT) {
    node=list_context->current_key ? list_context->current_key->next : NULL;
  } else { /* RDF_HASH_FLAGS_CURRENT */
    node=list_context->current_key;
  }

  if(!node) {
    key->data = NULL;
  } else {
    key->data = RDF_MALLOC(cstring, node->key_len);
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
rdf_hash_list_sync(void* context) 
{
  /* Not applicable */
  return 0;
}

static int
rdf_hash_list_get_fd(void* context) 
{
  /* Not applicable */
  return 0;
}


/* local function to register LIST hash functions */

static void
rdf_hash_list_register_factory(rdf_hash_factory *factory) 
{
  factory->context_length = sizeof(rdf_hash_list_context);

  factory->open    = rdf_hash_list_open;
  factory->close   = rdf_hash_list_close;
  factory->get     = rdf_hash_list_get;
  factory->put     = rdf_hash_list_put;
  factory->delete_key  = rdf_hash_list_delete;
  factory->get_seq = rdf_hash_list_get_seq;
  factory->sync    = rdf_hash_list_sync;
  factory->get_fd  = rdf_hash_list_get_fd;
}

void
init_rdf_hash_list(void)
{
  rdf_hash_register_factory("LIST", &rdf_hash_list_register_factory);
}
