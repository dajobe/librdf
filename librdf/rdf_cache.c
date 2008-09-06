/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_cache.c - Object Cache interface
 *
 * Copyright (C) 2008, David Beckett http://www.dajobe.org/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 * 
 */


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h> /* for isalnum */
#ifdef WITH_THREADS
#include <pthread.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <redland.h>


#ifndef STANDALONE


#define DEFAULT_FLUSH_PERCENT 20

typedef struct
{
  /* these are shared - the librdf_hash owns these */
  void* key;
  size_t key_size;
  void* value;
  size_t value_size;

  int id;
  int usage;
} librdf_cache_node;

typedef struct
{
  int id;
  int usage;
} librdf_cache_hist_node;

struct librdf_cache_s
{
  librdf_world *world;
  int size;
  int capacity;
  int flush_count;
  int flags;
  librdf_hash* hash;
  librdf_cache_node* nodes;
  librdf_cache_hist_node* hists;
};


/**
 * librdf_new_cache:
 * @world: redland world object
 * @capacity: cache maximum number of objects
 * @flush_percent: percentage (1-100) to remove when cache is full
 *
 * Constructor - create a new #librdf_cache object
 * 
 * A new cache is constructed from the parameters.
 *
 * If capacity is 0, the cache is not limited in size; no cache
 * ejection will be done.
 *
 * If flush_percent is out of the range 1-100, it is set to a
 * default value.  if set to 100, it means all objects will be
 * removed when the cache is full.
 *
 * Return value: a new #librdf_cache object or NULL on failure
 **/
librdf_cache*
librdf_new_cache(librdf_world* world, int capacity, int flush_percent,
                 int flags)
{
  librdf_cache* new_cache=NULL;

  if(!world || capacity < 0)
    return NULL;

#ifdef WITH_THREADS
  pthread_mutex_lock(world->mutex);
#endif
  
  new_cache=(librdf_cache*)LIBRDF_CALLOC(librdf_cache, 1, sizeof(librdf_cache));
  if(!new_cache)
    goto unlock;

  if(flush_percent < 1 || flush_percent > 100)
    flush_percent= DEFAULT_FLUSH_PERCENT;
  
  new_cache->world=world;
  new_cache->capacity=capacity;
  new_cache->size=0; /* empty */
  if(capacity)
    new_cache->flush_count= (capacity * flush_percent) / 100;
  new_cache->flags=flags;

  new_cache->hash=librdf_new_hash(world, NULL);
  if(!new_cache->hash) {
    LIBRDF_FREE(librdf_cache, new_cache);
    new_cache=NULL;
    LIBRDF_FATAL1(world, LIBRDF_FROM_URI, "Failed to create cache hash");
    goto unlock;
  }
  if(librdf_hash_open(new_cache->hash, NULL, 0, 1, 1, NULL)) {
    librdf_free_cache(new_cache);
    new_cache=NULL;
    goto unlock;
  }

  /* allocate static nodes and histogram nodes if capacity is fixed */
  if(capacity) {
    new_cache->nodes=(librdf_cache_node*)LIBRDF_CALLOC(array, capacity, sizeof(librdf_cache_node));
    if(!new_cache->nodes) {
      librdf_free_cache(new_cache);
      new_cache=NULL;
      goto unlock;
    }
  
    new_cache->hists=(librdf_cache_hist_node*)LIBRDF_CALLOC(array, capacity, 
                                   sizeof(librdf_cache_hist_node));
    if(!new_cache->hists) {
      librdf_free_cache(new_cache);
      new_cache=NULL;
      goto unlock;
    }
  }

 unlock:
#ifdef WITH_THREADS
  pthread_mutex_unlock(world->mutex);
#endif

  return new_cache;
}


/**
 * librdf_free_cache:
 * @cache: #librdf_cache object
 *
 * Destructor - destroy a #librdf_cache object.
 * 
 **/
void
librdf_free_cache(librdf_cache* cache) 
{
#ifdef WITH_THREADS
  pthread_mutex_lock(cache->world->mutex);
#endif

  if(cache->hash) {
    librdf_hash_close(cache->hash);
    librdf_free_hash(cache->hash);
  }

  if(cache->nodes)
    LIBRDF_FREE(array, cache->nodes);

  if(cache->hists)
    LIBRDF_FREE(array, cache->hists);

  LIBRDF_FREE(librdf_cache, cache);

#ifdef WITH_THREADS
  pthread_mutex_unlock(cache->world->mutex);
#endif
};


static int
librdf_hist_node_compare(const void* a_p, const void* b_p) 
{
  return ((librdf_cache_hist_node*)a_p)->usage - 
         ((librdf_cache_hist_node*)b_p)->usage;
}



static int
librdf_cache_cleanup(librdf_cache *cache)
{
  int i;
  int largest_ejected_usage;
  
  /* never cleanup if capacity is not limited */
  if(!cache->capacity)
    return 0;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3("Running cache cleanup - cache size is %d out of capacity %d\n",
                cache->size, cache->capacity);
#endif


  for(i=0; i < cache->size; i++) {
    cache->hists[i].id= cache->nodes[i].id;
    cache->hists[i].usage= cache->nodes[i].usage;
  }

  qsort(cache->hists, cache->size, sizeof(librdf_cache_hist_node),
        librdf_hist_node_compare);

  largest_ejected_usage=cache->nodes[cache->hists[cache->flush_count].id].usage;

  for(i=0; i < cache->flush_count; i++) {
    librdf_cache_node* node=&cache->nodes[cache->hists[i].id];
    /* this will zero out *node */
    librdf_cache_delete(cache, node->key, node->key_size);
  }

  /* adjust usage after cleanup */
  for(i=0; i < cache->size; i++) {
    if(cache->nodes[i].usage >= largest_ejected_usage)
      cache->nodes[i].usage -= largest_ejected_usage;
    else
      cache->nodes[i].usage=0;
  }
  
  return 0;
}


#define SET_FLAG_OVERWRITE 1

static int
librdf_cache_set_common(librdf_cache *cache,
                        void* key, size_t key_size,
                        void* value, size_t value_size,
                        void** existing_value_p, int flags)
{
  void* new_object;
  librdf_hash_datum key_hd, value_hd; /* on stack - not allocated */
  librdf_hash_datum *old_value;
  int i;
  int id= -1;
  librdf_cache_node* node;
  int rc=0;
  
  if(!key || !value || !key_size || !value_size)
    return -1;

#ifdef WITH_THREADS
  pthread_mutex_lock(cache->world->mutex);
#endif
  
  key_hd.data=key;
  key_hd.size=key_size;

  if(!(flags & SET_FLAG_OVERWRITE)) {
    /* if existing object found in hash, return it */
    if((old_value=librdf_hash_get_one(cache->hash, &key_hd))) {
      node=*(librdf_cache_node**)old_value->data;
      new_object=node->value;
      
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG2("Found existing object %p in hash\n", new_object);
#endif
      
      librdf_free_hash_datum(old_value);
      /* value already present */
      rc=1;
      goto unlock;
    }
  }
  

  /* otherwise store it */
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG1("Need to add new object to cache\n");
#endif

  if(!cache->capacity) {
    /* dynamic cache capacity */
    node=(librdf_cache_node*)LIBRDF_CALLOC(librdf_cache_node, 1, sizeof(librdf_cache_node));
    if(!node) {
      rc=1;
      goto unlock;
    }
    id= -1;
  } else {
    /* fixed cache capacity so eject some (static) nodes */
    if(cache->size == cache->capacity) {
      if(librdf_cache_cleanup(cache)) {
        rc=1;
        goto unlock;
      }
    }
    
    for(i=0; i < cache->capacity; i++) {
      node=&cache->nodes[i];
      if(!node->value) {
        id=i;
        break;
      }
    }
  }
  
  node->key=key;
  node->key_size=key_size;
  node->value=value;
  node->value_size=value_size;
  node->id=id;
  node->usage=0;
  
  value_hd.data=&node; value_hd.size=sizeof(node);
  
  /* store in hash: key => (librdf_cache_node*) */
  if(librdf_hash_put(cache->hash, &key_hd, &value_hd)) {
    memset(&node, '\0', sizeof(*node));
    LIBRDF_FREE(void, new_object);
    rc= -1;
  }

  /* succeeded */
  cache->size++;
  
 unlock:
#ifdef WITH_THREADS
  pthread_mutex_unlock(cache->world->mutex);
#endif

  return rc;
}


/**
 * librdf_cache_set:
 * @cache: redland cache object
 * @key: key data
 * @key_size: key data size
 * @value: value data
 * @value_size: value data size
 *
 * Store an item to the cache with given key and value
 *
 * Overwrites any existing value and returns it.
 * 
 * Return value: non-0 on failure
 **/
int
librdf_cache_set(librdf_cache *cache,
                 void* key, size_t key_size,
                 void* value, size_t value_size)
{
  int rc=librdf_cache_set_common(cache, key, key_size, value, value_size,
                                 NULL, SET_FLAG_OVERWRITE);
  return (rc != 0);
}



/**
 * librdf_cache_add:
 * @cache: redland cache object
 * @key: key data
 * @key_size: key data size
 * @value: value data
 * @value_size: value data size
 *
 * Store an item to the cache with given key and value
 *
 * Fails if any value exists for the key
 * 
 * Return value: non-0 on failure
 **/
int
librdf_cache_add(librdf_cache *cache,
                 void* key, size_t key_size,
                 void* value, size_t value_size)
{
  int rc=librdf_cache_set_common(cache, key, key_size, value, value_size,
                                 NULL, 0);
  return (rc != 0);
}



/**
 * librdf_cache_get:
 * @cache: redland cache object
 * @key: key data
 * @key_size: key data size
 * @value_size_p: pointer to store value size or NULL
 *
 * Get an item from the cache with given key
 * 
 * Return value: value pointer on success or NULL or failure/not found
 **/
void*
librdf_cache_get(librdf_cache *cache, void* key, size_t key_size,
                 size_t* value_size_p)
{
  librdf_cache_node* an_object=NULL;
  librdf_hash_datum key_hd; /* on stack - not allocated */
  librdf_hash_datum *value;

  if(!key || !key_size)
    return NULL;

#ifdef WITH_THREADS
  pthread_mutex_lock(cache->world->mutex);
#endif
  
  key_hd.data=key;
  key_hd.size=key_size;

  /* if existing object found in hash, return it */
  value=librdf_hash_get_one(cache->hash, &key_hd);
  if(value) {
    librdf_cache_node* node=*(librdf_cache_node**)value->data;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG4("Found object %p in cache of size %d usage %d\n", 
                  node->value, (int)node->value_size, node->usage);
#endif

    /* no need to track usage for dynamic nodes */
    if(cache->capacity)
      node->usage++;
    
    an_object=(librdf_cache_node*)node->value;
    if(value_size_p)
      *value_size_p=node->value_size;

    librdf_free_hash_datum(value);
    /* value present */
    goto unlock;
  }

 unlock:
#ifdef WITH_THREADS
  pthread_mutex_unlock(cache->world->mutex);
#endif

  return an_object;
}


/**
 * librdf_cache_delete:
 * @cache: redland cache object
 * @key: key data
 * @key_size: key data size
 *
 * Delte an item from the cache with given key
 * 
 * Return value: non-0 on failure
 **/
int
librdf_cache_delete(librdf_cache *cache, void* key, size_t key_size)
{
  librdf_hash_datum key_hd; /* on stack - not allocated */
  int rc=0;
  
  if(!key || !key_size)
    return -1;

#ifdef WITH_THREADS
  pthread_mutex_lock(cache->world->mutex);
#endif
  
  key_hd.data=key;
  key_hd.size=key_size;

  if(librdf_hash_delete_all(cache->hash, &key_hd)) {
    rc=1;
    goto unlock;
  }

  /* succeeded */
  cache->size--;
  
 unlock:
#ifdef WITH_THREADS
  pthread_mutex_unlock(cache->world->mutex);
#endif

  return rc;
}


/**
 * librdf_cache_delete:
 * @cache: redland cache object
 *
 * Get size of cache
 * 
 * Return value: cache siez
 **/
int
librdf_cache_size(librdf_cache *cache)
{
  return cache->size;
}

#endif


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  const char *program=librdf_basename((const char*)argv[0]);
  librdf_world *world=NULL;
  librdf_cache *cache=NULL;
  int failures=0;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  const char *test_id_params[]={ "fixed size 5, 70% eject", "unlimited" };
#endif
  const int test_cache_counts[]={4, 0, 1, 2, 0, 0, 0};
  const char *test_cache_values[]={
    "colour","yellow", /* 4: should stay in cache */
    "age", "new",
    "size", "large",
    "width", "small",
    "fruit", "banana",
    "parity", "even",
    "shape", "square",
    NULL, NULL};
  const char *test_cache_delete_key="shape";
  int test_id;
  
  world=librdf_new_world();
  if(!world) {
    fprintf(stderr, "%s: Failed to open world\n", program);
    failures++;
    goto tidy;
  }
  librdf_world_open(world);


  for(test_id=0; test_id < 2; test_id++) {
    librdf_hash_datum hd_key, hd_value; /* on stack */
    int j;
    
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    fprintf(stderr, "%s: Testing cache with parameters: %s\n", program,
              test_id_params[test_id]);
#endif
    if(test_id == 0)
      cache=librdf_new_cache(world, 5, 70, 0); /* fixed size 5, 70% eject */
    else
      cache=librdf_new_cache(world, 0, 0, 0); /* unlimited size */

    if(!cache) {
      fprintf(stderr, "%s: Failed to create cache for test #%d\n", program,
              test_id);
      failures++;
      goto tidy;
    }


    for(j=0; test_cache_values[j]; j+=2) {
      void* value=NULL;
      size_t value_size=0;
      char* expected_value=(char*)test_cache_values[j+1];
      int count;
      int test_cache_count=test_cache_counts[j/2];

      hd_key.data=(char*)test_cache_values[j];
      hd_value.data=(char*)test_cache_values[j+1];

      hd_key.size=strlen((char*)hd_key.data);
      hd_value.size=strlen((char*)hd_value.data); 

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      fprintf(stderr, "%s: Adding key/value pair: %s=%s\n", program,
              (char*)hd_key.data, (char*)hd_value.data);
#endif
      if(librdf_cache_set(cache, hd_key.data, hd_key.size,
                          hd_value.data, hd_value.size)) {
        fprintf(stderr, "%s: Adding key/value pair: %s=%s failed\n", program,
                (char*)hd_key.data, (char*)hd_value.data);
        failures++;
        break;
      }

  #if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      fprintf(stderr, "%s: cache size %d\n", program, librdf_cache_size(cache));
  #endif

      for(count=0; count < test_cache_count; count++) {
        value=librdf_cache_get(cache, hd_key.data, hd_key.size, &value_size);
        if(!value) {
          fprintf(stderr, "%s: Failed to get value\n", program);
          failures++;
          break;
        }
        if(strcmp(value, expected_value)) {
          fprintf(stderr, "%s: librdf_cache_get returned '%s' expected '%s'\n",
                  program, (char*)value, expected_value);
          failures++;
          break;
        }
      }
      if(failures)
        break;
    }
    if(failures)
      goto tidy;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    fprintf(stderr, "%s: Deleting key '%s'\n", program, test_cache_delete_key);
#endif
    hd_key.data=(char*)test_cache_delete_key;
    hd_key.size=strlen((char*)hd_key.data);
    if(librdf_cache_delete(cache, hd_key.data, hd_key.size)) {
      fprintf(stderr, "%s: Deleting key '%s' failed\n", program,
              test_cache_delete_key);
      failures++;
      goto tidy;
    }

  }

  tidy:
  if(cache)
    librdf_free_cache(cache);
  
  if(world)
    librdf_free_world(world);

  return failures;
}

#endif
