/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_uri.h - Object Cache Definition
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



#ifndef LIBRDF_CACHE_H
#define LIBRDF_CACHE_H

/* This is an internal API */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct librdf_cache_s librdf_cache;

librdf_cache* librdf_new_cache(librdf_world* world, int capacity, int flush_percent, int flags);
void librdf_free_cache(librdf_cache* cache);
void* librdf_cache_get(librdf_cache *cache, void* key, size_t key_size, size_t* value_size_p);
int librdf_cache_set(librdf_cache *cache, void* key, size_t key_size, void* value, size_t value_size);
int librdf_cache_add(librdf_cache *cache, void* key, size_t key_size, void* value, size_t value_size);
int librdf_cache_delete(librdf_cache *cache, void* key, size_t key_size);
int librdf_cache_size(librdf_cache *cache);

#ifdef __cplusplus
}
#endif

#endif
