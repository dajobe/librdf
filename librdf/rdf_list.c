/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_list.c - RDF List Implementation
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


#include <rdf_config.h>

#include <sys/types.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h> /* for strncmp */
#endif

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_list.h>
#include <rdf_iterator.h>


/* prototypes for local functions */
static librdf_list_node* librdf_list_find_node(librdf_list* list, void *data, librdf_list_node** prev);

static int librdf_list_iterator_have_elements(void* iterator);
static void* librdf_list_iterator_get_next(void* iterator);



/* helper functions */
static librdf_list_node*
librdf_list_find_node(librdf_list* list, void *data,
		      librdf_list_node** prev) 
{
  librdf_list_node* node;
  
  if(prev)
    *prev=list->first;
  for(node=list->first; node; node=node->next) {
    if(list->equals) {
      if(list->equals(node->data, data))
        break;
    } else {
      if(node->data == data)
        break;
    }
    
    if(prev)
      *prev=node;
  }
  return node;
}


/**
 * librdf_new_list - Constructor - create a new librdf_list
 * 
 * Return value: a new &librdf_list or NULL on failure
 **/
librdf_list*
librdf_new_list(void)
{
  librdf_list* new_list;
  
  new_list=(librdf_list*)LIBRDF_CALLOC(librdf_list, 1, sizeof(librdf_list));
  if(!new_list)
    return NULL;
  
  return new_list;
}


/**
 * librdf_free_list - Destructor - destroy a librdf_list object
 * @list: &librdf_list object
 * 
 **/
void
librdf_free_list(librdf_list* list) 
{
  librdf_list_node *node, *next;
  
  for(node=list->first; node; node=next) {
    next=node->next;
    LIBRDF_FREE(librdf_list_node, node);
  }
  LIBRDF_FREE(librdf_list, list);
}


/**
 * librdf_list_add - add a data item to the start of a librdf_list
 * @list: &librdf_list object
 * @data: the data value
 * 
 * Equivalent to the list 'push' notion, thus if librdf_list_pop()
 * is called after this, it will return the value added here.
 *
 * Return value: non 0 on failure
 **/
int
librdf_list_add(librdf_list* list, void *data) 
{
  librdf_list_node* node;
  
  /* need new node */
  node=(librdf_list_node*)LIBRDF_CALLOC(librdf_list_node, 1,
                                        sizeof(librdf_list_node));
  if(!node)
    return 1;
  
  node->data=data;
  node->next=list->first;
  list->first=node;
  list->length++;
  return 0;
}


/**
 * librdf_list_remove - remove a data item from a librdf_list
 * @list: &librdf_list object
 * @data: the data item
 * 
 * The search is done using the 'equals' function which may be set
 * by librdf_list_set_equals() or by straight comparison of pointers
 * if not set.
 * 
 * Return value: non 0 on failure (not found or list empty)
 **/
int
librdf_list_remove(librdf_list* list, void *data) 
{
  librdf_list_node *node, *prev;
  
  node=librdf_list_find_node(list, data, &prev);
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


/**
 * librdf_list_pop - remove and return the data from the start of the list
 * @list: &librdf_list object
 * 
 * Return value: the data object or NULL if the list is empty
 **/
void*
librdf_list_pop(librdf_list* list)
{
  librdf_list_node *node;
  void *data;

  node=list->first;
  if(!node)
    return NULL;
     
  list->first=node->next;
  /* save data */
  data=node->data;

  /* free node */
  LIBRDF_FREE(librdf_list_node, node);

  list->length--;
  return data;
}


/**
 * librdf_list_contains - check for presence of data item in list
 * @list: &librdf_list object
 * @data: the data value
 * 
 * The search is done using the 'equals' function which may be set
 * by librdf_list_set_equals() or by straight comparison of pointers
 * if not set.
 * 
 * Return value: non 0 if item was found
 **/
int
librdf_list_contains(librdf_list* list, void *data) 
{
  librdf_list_node *node, *prev;
  
  node=librdf_list_find_node(list, data, &prev);
  return (node != NULL);
}


/**
 * librdf_list_size - return the length of the list
 * @list: &librdf_list object
 * 
 * Return value: length of the list
 **/
int
librdf_list_size(librdf_list* list) 
{
  return list->length;
}


/**
 * librdf_list_set_equals - set the equals function for the list
 * @list: &librdf_list object
 * @equals: the equals function
 * 
 * The function given is used when comparing items in the list
 * during searches such as those done in librdf_list_remove() or
 * librdf_list_contains().
 * 
 **/
void
librdf_list_set_equals(librdf_list* list, 
                       int (*equals) (void* data1, void *data2)) 
{
  list->equals=equals;
}



/**
 * librdf_list_get_iterator - get an iterator for the list
 * @list: &librdf_list object
 * 
 * Return value: a new &librdf_iterator object or NULL on failure
 **/
librdf_iterator*
librdf_list_get_iterator(librdf_list* list)
{
  /* Initialise walk */
  list->current=list->first;
  
  return librdf_new_iterator((void*)list,
                             librdf_list_iterator_have_elements,
                             librdf_list_iterator_get_next,
                             NULL);
  
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
  librdf_list_node *node;
  
  if(!list || !list->current)
    return NULL;
  
  node= list->current;
  list->current = list->current->next;
  return node->data;
}
