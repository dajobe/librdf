/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_list.c - RDF List Implementation
 *
 * $Id$
 *
 * Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology, University of Bristol.
 *
 *    This package is Free Software available under either of two licenses
 *    (see FAQS.html to see why):
 * 
 * 1. The GNU Lesser General Public License (LGPL)
 * 
 *    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
 *    full license text.
 *      _________________________________________________________________
 * 
 *      Copyright (C) 2000 David Beckett, Institute for Learning and
 *      Research Technology, University of Bristol. All Rights Reserved.
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public License
 *      as published by the Free Software Foundation; either version 2 of
 *      the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *      USA
 *      _________________________________________________________________
 * 
 *    NOTE - under Term 3 of the LGPL, you may choose to license the entire
 *    library under the GPL. See COPYING for the full license text.
 * 
 * 2. The Mozilla Public License
 * 
 *    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
 *    license text.
 * 
 *    Under MPL section 13. I declare that all of the Covered Code is
 *    Multiple Licensed:
 *      _________________________________________________________________
 * 
 *      The contents of this file are subject to the Mozilla Public License
 *      version 1.1 (the "License"); you may not use this file except in
 *      compliance with the License. You may obtain a copy of the License
 *      at http://www.mozilla.org/MPL/
 * 
 *      Software distributed under the License is distributed on an "AS IS"
 *      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 *      the License for the specific language governing rights and
 *      limitations under the License.
 * 
 *      The Initial Developer of the Original Code is David Beckett.
 *      Portions created by David Beckett are Copyright (C) 2000 David
 *      Beckett, Institute for Learning and Research Technology, University
 *      of Bristol. All Rights Reserved.
 * 
 *      Alternatively, the contents of this file may be used under the
 *      terms of the GNU Lesser General Public License, in which case the
 *      provisions of the LGPL License are applicable instead of those
 *      above. If you wish to allow use of your version of this file only
 *      under the terms of the LGPL License and not to allow others to use
 *      your version of this file under the MPL, indicate your decision by
 *      deleting the provisions above and replace them with the notice and
 *      other provisions required by the LGPL License. If you do not delete
 *      the provisions above, a recipient may use your version of this file
 *      under either the MPL or the LGPL License.
 */


#include <rdf_config.h>

#include <sys/types.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h> /* for strncmp */
#endif

#define LIBRDF_INTERNAL 1
#include <redland.h>
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
