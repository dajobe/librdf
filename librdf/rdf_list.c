/*
 * rdf_list.c - RDF List Implementation
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

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h> /* for strncmp */
#endif

#define LIBRDF_INTERNAL 1
#include <rdf_config.h>
#include <rdf_list.h>
#include <rdf_iterator.h>


/* prototypes for local functions */
static librdf_list_node* librdf_list_find_node(librdf_list_node* list, void *data, librdf_list_node** prev);

static int librdf_list_iterator_have_elements(void* iterator);
static void* librdf_list_iterator_get_next(void* iterator);



/* helper functions */
static librdf_list_node*
librdf_list_find_node(librdf_list_node* list, void *data, librdf_list_node** prev) 
{
  librdf_list_node* node;
  
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


librdf_list*
librdf_new_list(void)
{
  librdf_list* new_list;

  new_list=(librdf_list*)LIBRDF_CALLOC(librdf_list, 1, sizeof(librdf_list));
  if(!new_list)
    return NULL;
  
  return new_list;
}


int
librdf_free_list(librdf_list* list) 
{
  librdf_list_node *node, *next;
  
  for(node=list->first; node; node=next) {
    next=node->next;
    LIBRDF_FREE(librdf_list_node, node);
  }
  LIBRDF_FREE(librdf_list, list);
  return 0;
}


int
librdf_list_add(librdf_list* list, void *data) 
{
  librdf_list_node* node;
  
  /* need new node */
  node=(librdf_list_node*)LIBRDF_CALLOC(librdf_list_node, sizeof(librdf_list_node), 1);
  if(!node)
    return 1;
    
  node->data=data;
  node->next=list->first;
  list->first=node;
  list->length++;
  return 0;
}


int
librdf_list_remove(librdf_list* list, void *data) 
{
  librdf_list_node *node, *prev;

  node=librdf_list_find_node(list->first, data, &prev);
  if(!node) {
    /* not found */
    return 1;
  }
  if(node == list->first)
    list->first=node->next;
  else
    prev->next=node->next;

  /* free node */
  LIBRDF_FREE(librdf_list_node, node);
  list->length--;
  return 0;
}


librdf_iterator*
librdf_list_get_iterator(librdf_list* list)
{
  /* Initialise walk */
  list->current=list->first;

  return librdf_new_iterator((void*)list, librdf_list_iterator_have_elements,
                          librdf_list_iterator_get_next);
  
}


static int
librdf_list_iterator_have_elements(void* iterator) 
{
  librdf_list *list=(librdf_list*)iterator;
  librdf_list_node *node;
  
  if(!list)
    return 0;
  node=list->current;
  
  return (node != NULL);
}


static void*
librdf_list_iterator_get_next(void* iterator) 
{
  librdf_list *list=(librdf_list*)iterator;
  librdf_list_node* node;

  if(!list || !list->current)
    return NULL;

  node= list->current=list->current->next;
  return (void*) node;
}
