/*
 * RDF Iterator Implementation
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>

#include <sys/types.h>

#include <rdf_config.h>
#include <rdf_iterator.h>


rdf_iterator*
rdf_new_iterator(void* datum,
                 int (*have_elements)(void*),
                 void* (*get_next)(void*))
{
  rdf_iterator* new_iterator;
  
  new_iterator=(rdf_iterator*)RDF_CALLOC(rdf_iterator, 1, sizeof(rdf_iterator));
  if(!new_iterator)
    return NULL;

  new_iterator->have_elements=have_elements;
  new_iterator->get_next=get_next;
  
  return new_iterator;
}

void
rdf_free_iterator(rdf_iterator* iterator) 
{
  RDF_FREE(rdf_iterator, iterator);
}

