/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_iterator.c - RDF Iterator Implementation
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


#include <rdf_config.h>

#include <stdio.h>
#include <stdarg.h>

#include <librdf.h>


/* prototypes of local helper functions */
static void* librdf_iterator_get_next_mapped_element(librdf_iterator* iterator);


/**
 * librdf_new_iterator - Constructor - create a new librdf_iterator object
 * @world: redland world object
 * @context: context to pass to the iterator functions
 * @is_end: function to call to see if the iteration has ended
 * @get_next: function to get the next element
 * @finished: function to destroy the iterator context (or NULL if not needed)
 * 
 * Return value: a new &librdf_iterator object or NULL on failure
**/
librdf_iterator*
librdf_new_iterator(librdf_world *world,
                    void* context,
		    int (*is_end)(void*),
		    int (*next_method)(void*),
		    void* (*get_method)(void*, int),
		    void (*finished)(void*))
{
  librdf_iterator* new_iterator;
  
  new_iterator=(librdf_iterator*)LIBRDF_CALLOC(librdf_iterator, 1, 
                                               sizeof(librdf_iterator));
  if(!new_iterator)
    return NULL;
  
  new_iterator->world=world;

  new_iterator->context=context;
  
  new_iterator->is_end=is_end;
  new_iterator->next_method=next_method;
  new_iterator->get_method=get_method;
  new_iterator->finished=finished;

  /* Not needed, calloc ensures this */
  /* new_iterator->is_finished=0; */
  
  return new_iterator;
}


/* helper function for deleting list map */
static void
librdf_iterator_free_iterator_map(void *list_data, void *user_data) 
{
  LIBRDF_FREE(librdf_iterator_map, (librdf_iterator_map*)list_data);
}

  

/**
 * librdf_free_iterator - Destructor - destroy a librdf_iterator object
 * @iterator: the &librdf_iterator object
 * 
 **/
void
librdf_free_iterator(librdf_iterator* iterator) 
{
  if(!iterator)
    return;
  
  if(iterator->finished)
    iterator->finished(iterator->context);

  if(iterator->map_list) {
    librdf_list_foreach(iterator->map_list,
                        librdf_iterator_free_iterator_map, NULL);
    librdf_free_list(iterator->map_list);
  }
  
  LIBRDF_FREE(librdf_iterator, iterator);
}



/*
 * librdf_iterator_get_next_mapped_element - Get next element with filtering
 * @iterator: the &librdf_iterator object
 * 
 * Helper function to get the next element subject to the user defined 
 * map function (as set by &librdf_iterator_set_map ) or NULL
 * if the iterator has ended.
 * 
 * Return value: the next element
 */
static void*
librdf_iterator_get_next_mapped_element(librdf_iterator* iterator) 
{
  void *element=NULL;
  
  /* find next element subject to map */
  while(!iterator->is_end(iterator->context)) {
    librdf_iterator* map_iterator; /* Iterator over iterator->map_list librdf_list */
    element=iterator->get_method(iterator->context, 
                                 LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT);
    if(!element)
      break;

    map_iterator=librdf_list_get_iterator(iterator->map_list);
    if(!map_iterator)
      break;
    
    while(!librdf_iterator_end(map_iterator)) {
      librdf_iterator_map *map=(librdf_iterator_map*)librdf_iterator_next(map_iterator);
      if(!map)
        break;
      
      /* apply the map to the element  */
      element=map->fn(element, map->context);
      if(!element)
        break;
    }
    librdf_free_iterator(map_iterator);
    

    /* found something, return it */
    if(element)
      break;
  }
  return element;
}


/**
 * librdf_iterator_have_elements - Test if the iterator has finished
 * @iterator: the &librdf_iterator object
 * 
 * DEPRECATED - use !librdf_iterator_end(iterator)
 *
 * Return value: 0 if the iterator has finished
 **/
int
librdf_iterator_have_elements(librdf_iterator* iterator) 
{
  return !librdf_iterator_end(iterator);
}


/**
 * librdf_iterator_end - Test if the iterator has finished
 * @iterator: the &librdf_iterator object
 * 
 * Return value: non 0 if the iterator has finished
 **/
int
librdf_iterator_end(librdf_iterator* iterator) 
{
  if(!iterator)
    return 1;

  if(iterator->is_finished)
    return 1;

  /* simple case, no mapping so pass on */
  if(!iterator->map_list)
    return (iterator->is_finished=iterator->is_end(iterator->context));


  /* mapping from here */

  /* already have 1 stored item */
  if(iterator->current)
    return 0;

  /* get next item subject to map or NULL if list ended */
  iterator->current=librdf_iterator_get_next_mapped_element(iterator);
  if(!iterator->current)
    iterator->is_finished=1;
  
  return (iterator->current == NULL);
}


/**
 * librdf_iterator_next - Move to the next iterator element
 * @iterator: the &librdf_iterator object
 *
 * Return value: non 0 if the iterator has finished
 **/
int
librdf_iterator_next(librdf_iterator* iterator)
{
  if(iterator->is_finished)
    return 1;

  iterator->is_finished=iterator->next_method(iterator->context);

  if(!iterator->is_finished && iterator->map_list) {
    /* mapping */
    iterator->current=librdf_iterator_get_next_mapped_element(iterator);

    if(!iterator->current)
      iterator->is_finished=1;
  }
  
  return iterator->is_finished;
}


/**
 * librdf_iterator_get_object - Get the current object from the iterator
 * @iterator: the &librdf_iterator object
 *
 * Return value: The next element or NULL if the iterator has finished.
 **/
void*
librdf_iterator_get_object(librdf_iterator* iterator)
{
  if(iterator->is_finished)
    return NULL;

  return iterator->get_method(iterator->context, 
                              LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT);
}


/**
 * librdf_iterator_get_object - Get the context of the current object on the iterator
 * @iterator: the &librdf_iterator object
 *
 * Return value: The context or NULL if the iterator has finished.
 **/
void*
librdf_iterator_get_context(librdf_iterator* iterator) 
{
  if(iterator->is_finished)
    return NULL;

  return iterator->get_method(iterator->context, 
                              LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT);
}



/**
 * librdf_iterator_get_key - Get the key of the current object on the iterator
 * @iterator: the &librdf_iterator object
 *
 * Return value: The context or NULL if the iterator has finished.
 **/
void*
librdf_iterator_get_key(librdf_iterator* iterator) 
{
  if(iterator->is_finished)
    return NULL;

  return iterator->get_method(iterator->context, 
                              LIBRDF_ITERATOR_GET_METHOD_GET_KEY);
}



/**
 * librdf_iterator_get_value - Get the value of the current object on the iterator
 * @iterator: the &librdf_iterator object
 *
 * Return value: The context or NULL if the iterator has finished.
 **/
void*
librdf_iterator_get_value(librdf_iterator* iterator) 
{
  if(iterator->is_finished)
    return NULL;

  return iterator->get_method(iterator->context, 
                              LIBRDF_ITERATOR_GET_METHOD_GET_VALUE);
}



/**
 * librdf_iterator_add_map - Add a librdf_iterator mapping function
 * @iterator: the iterator
 * @fn: the function to operate
 * @context: the context to pass to the map function
 * 
 * Adds an iterator mapping function which operates over the iterator to
 * select which elements are returned; it will be applied as soon as
 * this method is called.
 *
 * Several mapping functions can be added and they are applied in
 * the order given
 *
 * The mapping function should return non 0 to allow the element to be
 * returned.
 *
 * Return value: Non 0 on failure
 **/
int
librdf_iterator_add_map(librdf_iterator* iterator, 
                        void* (*fn)(void *context, void *element),
                        void *context)
{
  librdf_iterator_map *map;
  
  if(!iterator->map_list) {
    iterator->map_list=librdf_new_list(iterator->world);
    if(!iterator->map_list)
      return 1;
  }

  map=(librdf_iterator_map*)LIBRDF_CALLOC(librdf_iterator_map, sizeof(librdf_iterator_map), 1);
  if(!map)
    return 1;

  map->fn=fn;
  map->context=context;

  if(librdf_list_add(iterator->map_list, map)) {
    LIBRDF_FREE(librdf_iterator_map, map);
    return 1;
  }
  
  return 0;
}


void*
librdf_iterator_map_remove_duplicate_nodes(void *item, void *user_data) 
{
  librdf_node *node=(librdf_node *)item;
  static const char *null_string="NULL";
  char *s;

  s=node ? librdf_node_to_string(node) : (char*)null_string;
  fprintf(stderr, "librdf_iterator_remove_duplicate_nodes: node %s and user_data %p\n", s, user_data);
  if(s != null_string)
    LIBRDF_FREE(cstring, s);
  return item;
}
