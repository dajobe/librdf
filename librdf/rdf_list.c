/*
 * RDF List Implementation
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

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h> /* for strncmp */
#endif

#include <rdf_config.h>
#include <rdf_list.h>
#include <rdf_iterator.h>


/* prototypes for local functions */
static rdf_list_node* rdf_list_find_node(rdf_list_node* list, void *data, rdf_list_node** prev);

static int rdf_list_iterator_have_elements(void* iterator);
static void* rdf_list_iterator_get_next(void* iterator);



/* helper functions */
static rdf_list_node*
rdf_list_find_node(rdf_list_node* list, void *data, rdf_list_node** prev) 
{
  rdf_list_node* node;
  
  if(prev)
    *prev=list;
  for(node=list; node; node=node->next) {
    if(node->data == data)
      break;
    if(prev)
      *prev=node;
  }
  return node;
}


rdf_list*
rdf_new_list(void)
{
  rdf_list* new_list;

  new_list=(rdf_list*)RDF_CALLOC(rdf_list, 1, sizeof(rdf_list));
  if(!new_list)
    return NULL;
  
  return new_list;
}


int
rdf_free_list(rdf_list* list) 
{
  rdf_list_node *node, *next;
  
  for(node=list->first; node; node=next) {
    next=node->next;
    RDF_FREE(rdf_list_node, node);
  }
  RDF_FREE(rdf_list, list);
  return 0;
}


int
rdf_list_add(rdf_list* list, void *data) 
{
  rdf_list_node* node;
  
  /* need new node */
  node=(rdf_list_node*)RDF_CALLOC(rdf_list_node, sizeof(rdf_list_node), 1);
  if(!node)
    return 1;
    
  node->data=data;
  node->next=list->first;
  list->first=node;
  list->length++;
  return 0;
}


int
rdf_list_remove(rdf_list* list, void *data) 
{
  rdf_list_node *node, *prev;

  node=rdf_list_find_node(list->first, data, &prev);
  if(!node) {
    /* not found */
    return 1;
  }
  if(node == list->first)
    list->first=node->next;
  else
    prev->next=node->next;

  /* free node */
  RDF_FREE(rdf_list_node, node);
  list->length--;
  return 0;
}


rdf_iterator*
rdf_list_get_iterator(rdf_list* list)
{
  /* Initialise walk */
  list->current=list->first;

  return rdf_new_iterator((void*)list, rdf_list_iterator_have_elements,
                          rdf_list_iterator_get_next);
  
}


static int
rdf_list_iterator_have_elements(void* iterator) 
{
  rdf_list *list=(rdf_list*)iterator;
  rdf_list_node *node;
  
  if(!list)
    return 0;
  node=list->current;
  
  return (node != NULL);
}


static void*
rdf_list_iterator_get_next(void* iterator) 
{
  rdf_list *list=(rdf_list*)iterator;
  rdf_list_node* node;

  if(!list || !list->current)
    return NULL;

  node= list->current=list->current->next;
  return (void*) node;
}
