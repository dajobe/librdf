/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_stream.h - RDF Stream interface and definitions
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


#ifndef LIBRDF_STREAM_H
#define LIBRDF_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDF_INTERNAL

struct librdf_stream_s {
  librdf_world *world;
  void *context;
  int is_finished; /* 1 when have no more statements */
  
  /* Used when mapping */
  librdf_statement *current;
  void *map_context;      /* context to pass on to map */
  
  int (*is_end_method)(void*);
  int (*next_method)(void*);
  void* (*get_method)(void*, int); /* flags: type of get */
  void (*finished_method)(void*);

  librdf_statement* (*map)(void *context, librdf_statement* statement);
  void (*free_map)(void *context);
};

/* FIXME - should all short lists be enums */
#define LIBRDF_STREAM_GET_METHOD_GET_OBJECT  LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT  
#define LIBRDF_STREAM_GET_METHOD_GET_CONTEXT LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT
#endif

/* constructor */

librdf_stream* librdf_new_stream(librdf_world *world, void* context, int (*is_end_method)(void*), int (*next_method)(void*), void* (*get_method)(void*, int), void (*finished_method)(void*));

/* destructor */

void librdf_free_stream(librdf_stream* stream);

/* methods */
int librdf_stream_end(librdf_stream* stream);

int librdf_stream_next(librdf_stream* stream);
librdf_statement* librdf_stream_get_object(librdf_stream* stream);
void* librdf_stream_get_context(librdf_stream* stream);

void librdf_stream_set_map(librdf_stream* stream, librdf_statement* (*map)(void* context, librdf_statement* statement), void (*free_context)(void *map_context), void* map_context);

#ifdef LIBRDF_INTERNAL
librdf_stream* librdf_new_stream_from_node_iterator(librdf_iterator* iterator, librdf_statement* statement, unsigned int field);
#endif

void librdf_stream_print(librdf_stream *stream, FILE *fh);

#ifdef __cplusplus
}
#endif

#endif
