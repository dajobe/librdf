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
 * librdf_new_iterator:
 * @context: context to pass to the iterator functions
 * @have_elements: function to call to see if there are more elements
 * @get_next: function to get the next element
 * @finished: function to destroy the iterator context (or NULL if not needed)
 * 
 * Constructor: create a new &librdf_iterator object.
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
 * librdf_free_iterator:
 * @iterator: 
 * 
 * 
 **/
void
librdf_free_iterator(librdf_iterator* iterator) 
{
  if(iterator->finished)
    iterator->finished(iterator->context);
  LIBRDF_FREE(librdf_iterator, iterator);
}



/**
 * librdf_iterator_get_next_mapped_element:
 * @iterator: 
 * 
 * Helper function to get the next element subject to the user defined 
 * map function (as set by &librdf_iterator_set_map ) or NULL
 * if the iterator has ended.
 * 
 * Return value: the next element
 **/
static void*
librdf_iterator_get_next_mapped_element(librdf_iterator* iterator) 
{
  void *element=NULL;
  
  /* find next element subject to map */
  while(iterator->have_elements(iterator->context)) {
    element=iterator->get_next(iterator->context);
    /* apply the map to the element  */
    element=iterator->map(iterator->map_context, element);
    /* found something, return it */
    if(element)
      break;
  }
  return element;
}


int
librdf_iterator_have_elements(librdf_iterator* iterator) 
{
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
 * librdf_iterator_get_next:
 * @iterator: the iterator
 * 
 * Get the next iterator element
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
 * librdf_iterator_set_map:
 * @iterator: the iterator
 * @map: the function to operate
 * 
 * Set the iterator map mapping function to operate over the iterator.  The 
 * mapping function should return non 0 to return the element to the caller 
 * of the iterator.
 **/
void
librdf_iterator_set_map(librdf_iterator* iterator, 
                        void* (*map)(void *context, void *element),
                        void *map_context)
{
  iterator->map=map;
  iterator->map_context=map_context;
}
