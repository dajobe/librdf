/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_list.c - RDF List Implementation
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


#include <rdf_config.h>

#include <sys/types.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h> /* for strncmp */
#endif

#include <librdf.h>
#include <rdf_list.h>
#include <rdf_iterator.h>


/* prototypes for local functions */
static librdf_list_node* librdf_list_find_node(librdf_list* list, void *data);

static int librdf_list_iterator_is_end(void* iterator);
static void* librdf_list_iterator_get_next(void* iterator);
static void librdf_list_iterator_finished(void* iterator);



/* helper functions */
static librdf_list_node*
librdf_list_find_node(librdf_list* list, void *data)
{
  librdf_list_node* node;
  
  for(node=list->first; node; node=node->next) {
    if(list->equals) {
      if(list->equals(node->data, data))
        break;
    } else {
      if(node->data == data)
        break;
    }
  }
  return node;
}


/**
 * librdf_new_list - Constructor - create a new librdf_list
 * @world: redland world object
 * 
 * Return value: a new &librdf_list or NULL on failure
 **/
librdf_list*
librdf_new_list(librdf_world *world)
{
  librdf_list* new_list;
  
  new_list=(librdf_list*)LIBRDF_CALLOC(librdf_list, 1, sizeof(librdf_list));
  if(!new_list)
    return NULL;
  
  new_list->world=world;
  
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
  librdf_list_clear(list);
  LIBRDF_FREE(librdf_list, list);
}


/**
 * librdf_list_clear - empty an librdf_list
 * @list: &librdf_list object
 * 
 **/
void
librdf_list_clear(librdf_list* list) 
{
  librdf_list_node *node, *next;
  
  for(node=list->first; node; node=next) {
    next=node->next;
    LIBRDF_FREE(librdf_list_node, node);
  }
}


/**
 * librdf_list_add - add a data item to the end of a librdf_list
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

  /* if there is a list, connect the new node to the last node  */
  if(list->last) {
    node->prev=list->last;
    list->last->next=node;
  }

  /* make this node the last node always */
  list->last=node;
  
  /* if there is no list at all, make this the first to */
  if(!list->first)
    list->first=node;

  /* node->next = NULL implicitly */

  list->length++;
  return 0;
}


/**
 * librdf_list_unshift - add a data item to the start of a librdf_list
 * @list: &librdf_list object
 * @data: the data value
 * 
 * if librdf_list_shift() is called after this, it will return the value
 * added here.
 *
 * Return value: non 0 on failure
 **/
int
librdf_list_unshift(librdf_list* list, void *data) 
{
  librdf_list_node* node;
  
  /* need new node */
  node=(librdf_list_node*)LIBRDF_CALLOC(librdf_list_node, 1,
                                        sizeof(librdf_list_node));
  if(!node)
    return 1;
  
  node->data=data;

  /* if there is a list, connect the new node to the first node  */
  if(list->first) {
    node->next=list->first;
    list->first->prev=node;
  }

  /* make this node the first node always */
  list->first=node;
  
  /* if there is no list at all, make this the last too */
  if(!list->last)
    list->last=node;

  /* node->next = NULL implicitly */

  list->length++;
  return 0;
}


/**
 * librdf_list_remove - remove a data item from an librdf_list
 * @list: &librdf_list object
 * @data: the data item
 * 
 * The search is done using the 'equals' function which may be set
 * by librdf_list_set_equals() or by straight comparison of pointers
 * if not set.
 * 
 * Return value: the data stored or NULL on failure (not found or list empty)
 **/
void *
librdf_list_remove(librdf_list* list, void *data) 
{
  librdf_list_node *node;
  
  node=librdf_list_find_node(list, data);
  if(!node)
    /* not found */
    return NULL;

  if(node == list->first)
    list->first=node->next;
  if(node->prev)
    node->prev->next=node->next;

  if(node == list->last)
    list->last=node->prev;
  if(node->next)
    node->next->prev=node->prev;

  /* retrieve actual stored data */
  data=node->data;
  
  /* free node */
  LIBRDF_FREE(librdf_list_node, node);
  list->length--;

  return data;
}


/**
 * librdf_list_shift - remove and return the data at the start of the list
 * @list: &librdf_list object
 * 
 * Return value: the data object or NULL if the list is empty
 **/
void*
librdf_list_shift(librdf_list* list)
{
  librdf_list_node *node;
  void *data;

  node=list->first;
  if(!node)
    return NULL;
     
  list->first=node->next;

  if(list->first)
    /* if list not empty, fix pointers */
    list->first->prev=NULL;
  else
    /* list is now empty, zap last pointer */
    list->last=NULL;
  
  /* save data */
  data=node->data;

  /* free node */
  LIBRDF_FREE(librdf_list_node, node);

  list->length--;
  return data;
}


/**
 * librdf_list_pop - remove and return the data at the end of the list
 * @list: &librdf_list object
 * 
 * Return value: the data object or NULL if the list is empty
 **/
void*
librdf_list_pop(librdf_list* list)
{
  librdf_list_node *node;
  void *data;

  node=list->last;
  if(!node)
    return NULL;
     
  list->last=node->prev;

  if(list->last)
    /* if list not empty, fix pointers */
    list->last->next=NULL;
  else
    /* list is now empty, zap last pointer */
    list->first=NULL;
  
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
  librdf_list_node *node;
  
  node=librdf_list_find_node(list, data);
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



typedef struct {
  librdf_list *list;
  librdf_list_node *current;
} librdf_list_iterator_context;


/**
 * librdf_list_get_iterator - get an iterator for the list
 * @list: &librdf_list object
 * 
 * Return value: a new &librdf_iterator object or NULL on failure
 **/
librdf_iterator*
librdf_list_get_iterator(librdf_list* list)
{
  librdf_list_iterator_context* context;

  context=(librdf_list_iterator_context*)LIBRDF_CALLOC(librdf_list_iterator_context, 1, sizeof(librdf_list_iterator_context));
  if(!context)
    return NULL;

  context->list=list;
  context->current=list->first;
  
  return librdf_new_iterator(list->world, 
                             (void*)context,
                             librdf_list_iterator_is_end,
                             librdf_list_iterator_get_next,
                             librdf_list_iterator_finished);
  
}


static int
librdf_list_iterator_is_end(void* iterator) 
{
  librdf_list_iterator_context* context=(librdf_list_iterator_context*)iterator;
  librdf_list_node *node=context->current;
  
  return (node == NULL);
}


static void*
librdf_list_iterator_get_next(void* iterator) 
{
  librdf_list_iterator_context* context=(librdf_list_iterator_context*)iterator;
  librdf_list_node *node=context->current;
  
  if(!node)
    return NULL;
  
  context->current = node->next;
  return node->data;
}

static void
librdf_list_iterator_finished(void* iterator)
{
  librdf_list_iterator_context* context=(librdf_list_iterator_context*)iterator;
  LIBRDF_FREE(librdf_list_iterator_context, context);
}



/**
 * librdf_list_foreach - apply a function for each data item in a librdf_list
 * @list: &librdf_list object
 * @fn: pointer to function to apply that takes data pointer and user data parameters
 * @user_data: user data for applied function 
 * 
 **/
void
librdf_list_foreach(librdf_list* list, void (*fn)(void *, void *),
                    void *user_data) 
{
  librdf_list_node *node, *next;
  
  for(node=list->first; node; node=next) {
    next=node->next;
    (*fn)(node->data, user_data);
  }
}


