/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_iterator.h - RDF Iterator definition
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



#ifndef LIBRDF_ITERATOR_H
#define LIBRDF_ITERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

struct librdf_iterator_s {
  void *context;
  int have_more_elements; /* 0 when have no more elements */

  /* Used when mapping */
  void *next;        /* stores next element */
  void *map_context; /* context to pass on to map */
  
  int (*have_elements)(void*);
  void* (*get_next)(void*);
  void (*finished)(void*);
  void* (*map)(void *context, void *element);
};


librdf_iterator* librdf_new_iterator(void *context, int (*have_elements)(void*), void* (*get_next)(void*), void (*finished)(void*));

void librdf_free_iterator(librdf_iterator*);

int librdf_iterator_have_elements(librdf_iterator* iterator);

void* librdf_iterator_get_next(librdf_iterator* iterator);

void librdf_iterator_set_map(librdf_iterator* iterator, void* (*map)(void *context, void *item), void *map_context);

#ifdef __cplusplus
}
#endif

#endif
