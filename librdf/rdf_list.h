/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_list.h - RDF List Interface definition
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



#ifndef LIBRDF_LIST_H
#define LIBRDF_LIST_H

#include <rdf_iterator.h>

#ifdef __cplusplus
extern "C" {
#endif



/* private structure */
struct librdf_list_node_s
{
  struct librdf_list_node_s* next;
  void *data;
};
typedef struct librdf_list_node_s librdf_list_node;


struct librdf_list_s
{
  librdf_list_node* first;
  librdf_list_node* current;
  int length;
  int (*equals) (void* data1, void *data2);
};


librdf_list* librdf_new_list(void);
void librdf_free_list(librdf_list* list);

int librdf_list_add(librdf_list* list, void *data);
int librdf_list_remove(librdf_list* list, void *data);
void* librdf_list_pop(librdf_list* list);
int librdf_list_contains(librdf_list* list, void *data);
int librdf_list_size(librdf_list* list);

void librdf_list_set_equals(librdf_list* list, int (*equals) (void* data1, void *data2));

librdf_iterator* librdf_list_get_iterator(librdf_list* list);


#ifdef __cplusplus
}
#endif

#endif
