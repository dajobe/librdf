/*
 * rdf_iterator.c - RDF Iterator Implementation
 *
 * $Source$
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


#include <config.h>

#include <sys/types.h>

#define LIBRDF_INTERNAL 1
#include <rdf_config.h>
#include <rdf_iterator.h>


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




int
librdf_iterator_have_elements(librdf_iterator* iterator) 
{
	return iterator->have_elements(iterator->context);
}


void*
librdf_iterator_get_next(librdf_iterator* iterator) 
{
	return iterator->get_next(iterator->context);
}

