/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_stream.h - RDF Stream interface and definitions
 *
 * $Id$
 *
 * Copyright (C) 2000-2004, David Beckett http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology http://www.ilrt.bristol.ac.uk/
 * University of Bristol, UK http://www.bristol.ac.uk/
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


#ifndef LIBRDF_STREAM_H
#define LIBRDF_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef librdf_statement* (*librdf_stream_map_handler)(librdf_stream *stream, void *map_context, librdf_statement *item);
typedef void (*librdf_stream_map_free_context_handler)(void *map_context);

#ifdef LIBRDF_INTERNAL

/* used in map_list below */
typedef struct {
  void *context; /* context to pass on to map */
  librdf_stream_map_handler fn;
  librdf_stream_map_free_context_handler free_context;
} librdf_stream_map;

struct librdf_stream_s {
  librdf_world *world;
  void *context;
  int is_finished; /* 1 when have no more statements */
  int is_updated; /* 1 when we know there is a current item */
  
  /* Used when mapping */
  librdf_statement *current;
  librdf_list *map_list; /* non-empty means there is a list of maps */
  
  int (*is_end_method)(void*);
  int (*next_method)(void*);
  void* (*get_method)(void*, int); /* flags: type of get */
  void (*finished_method)(void*);
};

/* FIXME - should all short lists be enums */
#define LIBRDF_STREAM_GET_METHOD_GET_OBJECT  LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT  
#define LIBRDF_STREAM_GET_METHOD_GET_CONTEXT LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT

librdf_statement* librdf_stream_statement_find_map(librdf_stream *stream, void* context, librdf_statement* statement);

#endif

/* constructor */

REDLAND_API librdf_stream* librdf_new_stream(librdf_world *world, void* context, int (*is_end_method)(void*), int (*next_method)(void*), void* (*get_method)(void*, int), void (*finished_method)(void*));
REDLAND_API librdf_stream* librdf_new_stream_from_node_iterator(librdf_iterator* iterator, librdf_statement* statement, librdf_statement_part field);

/* destructor */

REDLAND_API void librdf_free_stream(librdf_stream* stream);

/* methods */
REDLAND_API int librdf_stream_end(librdf_stream* stream);

REDLAND_API int librdf_stream_next(librdf_stream* stream);
REDLAND_API librdf_statement* librdf_stream_get_object(librdf_stream* stream);
REDLAND_API void* librdf_stream_get_context(librdf_stream* stream);

REDLAND_API int librdf_stream_add_map(librdf_stream* stream, librdf_stream_map_handler map_function, librdf_stream_map_free_context_handler free_context, void *map_context);

REDLAND_API void librdf_stream_print(librdf_stream *stream, FILE *fh);

#ifdef __cplusplus
}
#endif

#endif
