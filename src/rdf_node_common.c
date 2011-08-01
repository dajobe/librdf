/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_node_common.c - RDF Node / Term class common code
 *
 * Copyright (C) 2010, David Beckett http://www.dajobe.org/
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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <redland.h>


/* iterator over a static array of nodes; - mostly for testing */
static int librdf_node_static_iterator_is_end(void* iterator);
static int librdf_node_static_iterator_next_method(void* iterator);
static void* librdf_node_static_iterator_get_method(void* iterator, int flags);
static void librdf_node_static_iterator_finished(void* iterator);

typedef struct {
  librdf_world *world;
  librdf_node** nodes; /* static array of nodes; shared */
  int size;            /* size of above array */
  int current;         /* index into above array */
} librdf_node_static_iterator_context;


static int
librdf_node_static_iterator_is_end(void* iterator)
{
  librdf_node_static_iterator_context* context;

  context = (librdf_node_static_iterator_context*)iterator;

  return (context->current > context->size-1);
}


static int
librdf_node_static_iterator_next_method(void* iterator) 
{
  librdf_node_static_iterator_context* context;

  context = (librdf_node_static_iterator_context*)iterator;

  if(context->current > context->size-1)
    return 1;

  context->current++;
  return 0;
}


static void*
librdf_node_static_iterator_get_method(void* iterator, int flags) 
{
  librdf_node_static_iterator_context* context;

  context = (librdf_node_static_iterator_context*)iterator;
  
  if(context->current > context->size-1)
    return NULL;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
       return (void*)context->nodes[context->current];

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return NULL;

    default:
      librdf_log(context->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_NODE, NULL,
                 "Unknown iterator method flag %d", flags);
      return NULL;
  }
}


static void
librdf_node_static_iterator_finished(void* iterator) 
{
  librdf_node_static_iterator_context* context;

  context = (librdf_node_static_iterator_context*)iterator;
  LIBRDF_FREE(librdf_node_static_iterator_context, context);
}


/**
 * librdf_node_new_static_node_iterator:
 * @world: world object
 * @nodes: static array of #librdf_node objects
 * @size: size of array
 *
 * Create an iterator over an array of nodes.
 * 
 * This creates an iterator for an existing static array of librdf_node
 * objects.  It is mostly intended for testing iterator code.
 * 
 * Return value: a #librdf_iterator serialization of the nodes or NULL on failure
 **/
librdf_iterator*
librdf_node_new_static_node_iterator(librdf_world* world, librdf_node** nodes,
                                     int size)
{
  librdf_node_static_iterator_context* context;
  librdf_iterator* iterator;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(nodes, librdf_node**, NULL);

  context = LIBRDF_CALLOC(librdf_node_static_iterator_context*, 1,
                          sizeof(*context));
  if(!context)
    return NULL;

  context->nodes = nodes;
  context->size = size;
  context->current = 0;

  iterator = librdf_new_iterator(world,
                                 (void*)context,
                                 librdf_node_static_iterator_is_end,
                                 librdf_node_static_iterator_next_method,
                                 librdf_node_static_iterator_get_method,
                                 librdf_node_static_iterator_finished);
  if(!iterator)
    librdf_node_static_iterator_finished(context);

  return iterator;
}
