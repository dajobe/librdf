/*
 * rdf_list.h - RDF List Interface definition
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



#ifndef LIBRDF_LIST_H
#define LIBRDF_LIST_H

#include <rdf_iterator.h>

#ifdef __cplusplus
extern "C" {
#endif

struct librdf_list_node_s
{
  struct librdf_list_node_s* next;
  void *data;
};
typedef struct librdf_list_node_s librdf_list_node;


typedef struct
{
  librdf_list_node* first;
  librdf_list_node* current;
  int length;
} librdf_list;


librdf_list* librdf_new_list(void);
int librdf_free_list(librdf_list* list);

int librdf_list_add(librdf_list* list, void *data);
int librdf_list_remove(librdf_list* list, void *data);
librdf_iterator* librdf_list_get_iterator(librdf_list* list);


#ifdef __cplusplus
}
#endif

#endif
