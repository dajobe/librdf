/*
 * RDF Iterator definition
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


#ifndef RDF_ITERATOR_H
#define RDF_ITERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  void *datum;
  int (*have_elements)(void*);
  void* (*get_next)(void*);
} rdf_iterator;


rdf_iterator* rdf_new_iterator(void *datum,
                               int (*have_elements)(void*),
                               void* (*get_next)(void*));
void rdf_free_iterator(rdf_iterator*);


#ifdef __cplusplus
}
#endif

#endif
