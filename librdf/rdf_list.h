/*
 * RDF List Interface definition
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


#ifndef RDF_LIST_H
#define RDF_LIST_H

#include <rdf_iterator.h>

struct rdf_list_node_s
{
  struct rdf_list_node_s* next;
  void *data;
};
typedef struct rdf_list_node_s rdf_list_node;


typedef struct
{
  rdf_list_node* first;
  rdf_list_node* current;
  int length;
} rdf_list;


rdf_list* rdf_new_list(void);
int rdf_free_list(rdf_list* list);

int rdf_list_add(rdf_list* list, void *data);
int rdf_list_remove(rdf_list* list, void *data);
rdf_iterator* rdf_list_get_iterator(rdf_list* list);


#endif
