/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_iterator.h - RDF Iterator definition
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
