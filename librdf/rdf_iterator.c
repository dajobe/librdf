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


librdf_iterator*
librdf_new_iterator(void* datum,
                 int (*have_elements)(void*),
                 void* (*get_next)(void*))
{
  librdf_iterator* new_iterator;
  
  new_iterator=(librdf_iterator*)LIBRDF_CALLOC(librdf_iterator, 1, sizeof(librdf_iterator));
  if(!new_iterator)
    return NULL;

  new_iterator->have_elements=have_elements;
  new_iterator->get_next=get_next;
  
  return new_iterator;
}

void
librdf_free_iterator(librdf_iterator* iterator) 
{
  LIBRDF_FREE(librdf_iterator, iterator);
}

