/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_iterator.c - RDF Iterator Implementation
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

#include <stdio.h>

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_iterator.h>


/* prototypes of local helper functions */
static void* librdf_iterator_get_next_mapped_element(librdf_iterator* iterator);


/**
 * librdf_new_iterator - Constructor - create a new librdf_iterator object
 * @context: context to pass to the iterator functions
 * @have_elements: function to call to see if there are more elements
 * @get_next: function to get the next element
 * @finished: function to destroy the iterator context (or NULL if not needed)
 * 
 * Return value: a new &librdf_iterator object or NULL on failure
**/
librdf_iterator*
librdf_new_iterator(void* context,
		    int (*have_elements)(void*),
		    void* (*get_next)(void*),
		    void (*finished)(void*))
{
  librdf_iterator* new_iterator;
  
  new_iterator=(librdf_iterator*)LIBRDF_CALLOC(librdf_iterator, 1, 
                                               sizeof(librdf_iterator));
  if(!new_iterator)
    return NULL;
  
  new_iterator->context=context;
  
  new_iterator->have_elements=have_elements;
  new_iterator->get_next=get_next;
  new_iterator->finished=finished;

  /* Not strictly true, but we will test this later */
  new_iterator->have_more_elements=1;
  
  return new_iterator;
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
  while(iterator->have_elements(iterator->context)) {
    element=iterator->get_next(iterator->context);
    if(!element)
      break;
    
    /* apply the map to the element  */
    element=iterator->map(iterator->map_context, element);
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
 * Return value: 0 if the iterator has finished
 **/
int
librdf_iterator_have_elements(librdf_iterator* iterator) 
{
  if(!iterator)
    return 0;

  if(!iterator->have_more_elements)
    return 0;

  /* simple case, no mapping so pass on */
  if(!iterator->map)
    return (iterator->have_more_elements=iterator->have_elements(iterator->context));


  /* mapping from here */

  /* already have 1 stored item */
  if(iterator->next)
    return 1;

  /* get next item subject to map or NULL if list ended */
  iterator->next=librdf_iterator_get_next_mapped_element(iterator);
  if(!iterator->next)
    iterator->have_more_elements=0;
  
  return (iterator->next != NULL);
}


/**
 * librdf_iterator_get_next - Get the next iterator element
 * @iterator: the &librdf_iterator object
 *
 * Return value: The next element or NULL if the iterator has finished.
 **/
void*
librdf_iterator_get_next(librdf_iterator* iterator) 
{
  void *element;
  
  if(!iterator->have_more_elements)
    return NULL;

  /* simple case, no mapping so pass on */
  if(!iterator->map)
    return iterator->get_next(iterator->context);

  /* mapping from here */

  /* return stored element if there is one */
  if(iterator->next) {
    element=iterator->next;
    iterator->next=NULL;
    return element;
  }

  /* else get a new one or NULL at end of list */
  iterator->next=librdf_iterator_get_next_mapped_element(iterator);
  if(!iterator->next)
    iterator->have_more_elements=0;
  
  return iterator->next;
}


/**
 * librdf_iterator_set_map - Set the librdf_iterator mapping function
 * @iterator: the iterator
 * @map: the function to operate
 * @map_context: the context to pass to the map function
 * 
 * Sets the iterator mapping function which operates over the iterator to
 * select which elements are returned; it will be applied as soon as
 * this method is called.
 *
 * The mapping function should return non 0 to allow the element to be
 * returned.
 **/
void
librdf_iterator_set_map(librdf_iterator* iterator, 
                        void* (*map)(void *context, void *element),
                        void *map_context)
{
  iterator->map=map;
  iterator->map_context=map_context;
}
