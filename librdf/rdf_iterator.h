/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_iterator.h - RDF Iterator definition
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.org/
 * University of Bristol - http://www.bristol.ac.uk/
 * 
 * This package is Free Software or Open Source available under the
 * following licenses (these are alternatives):
 *   1. GNU Lesser General Public License (LGPL)
 *   2. GNU General Public License (GPL)
 *   3. Mozilla Public License (MPL)
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * full license terms.
 * 
 * 
 */



#ifndef LIBRDF_ITERATOR_H
#define LIBRDF_ITERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDF_INTERNAL

/* used in map_list below */
typedef struct {
  void *context; /* context to pass on to map */
  void* (*fn)(void *context, void *element);
} librdf_iterator_map;

struct librdf_iterator_s {
  librdf_world *world;
  void *context;
  int is_finished; /* 1 when have no more elements */
  int is_updated; /* 1 when we know there is a current item */

  /* Used when mapping */
  void *current;            /* stores current element */
  librdf_list *map_list; /* non-empty means there is a list of maps */
  
  int (*is_end_method)(void*);
  int (*next_method)(void*);
  void* (*get_method)(void*, int); /* flags: type of get */
  void (*finished_method)(void*);
};

/* FIXME - should all short lists be enums */
#define LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT  0
#define LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT 1
#define LIBRDF_ITERATOR_GET_METHOD_GET_KEY     2
#define LIBRDF_ITERATOR_GET_METHOD_GET_VALUE   3

#endif

REDLAND_API librdf_iterator* librdf_new_iterator(librdf_world *world, void *context, int (*is_end_method)(void*), int (*next_method)(void*), void* (*get_method)(void*, int), void (*finished_method)(void*));

REDLAND_API void librdf_free_iterator(librdf_iterator*);

REDLAND_API int librdf_iterator_end(librdf_iterator* iterator);
REDLAND_API int librdf_iterator_have_elements(librdf_iterator* iterator);

REDLAND_API int librdf_iterator_finished(librdf_iterator* iterator);

REDLAND_API int librdf_iterator_next(librdf_iterator* iterator);
REDLAND_API void* librdf_iterator_get_object(librdf_iterator* iterator);
REDLAND_API void* librdf_iterator_get_context(librdf_iterator* iterator);
REDLAND_API void* librdf_iterator_get_key(librdf_iterator* iterator);
REDLAND_API void* librdf_iterator_get_value(librdf_iterator* iterator);

REDLAND_API int librdf_iterator_add_map(librdf_iterator* iterator, void* (*fn)(void *context, void *item), void *context);

#ifdef __cplusplus
}
#endif

#endif
