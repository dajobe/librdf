/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_memory.c - RDF Memory Debugging Implementation
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

#ifdef LIBRDF_MEMORY_DEBUG
#include <stdio.h>

#define LIBRDF_INTERNAL
#include <redland.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


/* In theory these could be changed to use a custom allocator */
#define os_malloc malloc
#define os_calloc calloc
#define os_free free


struct librdf_memory_s
{
  struct librdf_memory_s* next;

  char *file;
  int line;
  char *type;
  size_t size;
  void *addr;
};
typedef struct librdf_memory_s librdf_memory;


/* statics */
librdf_memory* Rdf_Memory_List=NULL;

/* prototypes for helper functions */
static librdf_memory* librdf_memory_find_node(librdf_memory* list, void *addr, librdf_memory** prev);
static void librdf_free_memory(librdf_memory* node);
static librdf_memory* librdf_add_memory(char *file, int line, char *type, size_t size, void *addr);


/* helper functions */
static librdf_memory*
librdf_memory_find_node(librdf_memory* list, void *addr, librdf_memory** prev) 
{
  librdf_memory* node;
  
  if(prev)
    *prev=list;
  for(node=list; node; node=node->next) {
    if(addr == node->addr)
      break;
    if(prev)
      *prev=node;
  }
  return node;
}


static void
librdf_free_memory(librdf_memory* node) 
{
  os_free(node);
}


static librdf_memory*
librdf_add_memory(char *file, int line, char *type,
               size_t size, void *addr) 
{
  librdf_memory* node;
  
  node=librdf_memory_find_node(Rdf_Memory_List, addr, NULL);

  if(node) {
    /* duplicated node added! */
    fprintf(stderr, "librdf_add_memory: Duplicate memory address %p added\n", addr);
    fprintf(stderr, "%s:%d: prev - %d bytes for type %s\n",
            node->file, node->line, node->size, node->type);
    fprintf(stderr, "%s:%d: this - %d bytes for type %s\n",
            file, line, size, type);
    abort();
    /* never returns actually */
  }

    
  /* need new node */
  node=(librdf_memory*)os_calloc(sizeof(librdf_memory), 1);
  if(!node)
    return NULL;


  node->file=file; /* pointer to static string, no need to free */
  node->line=line;
  node->type=type; /* pointer to static string, no need to free */
  node->size=size;
  node->addr=addr;


  /* only now touch list */
  node->next=Rdf_Memory_List;
  Rdf_Memory_List=node;

  return node;
}


void*
librdf_malloc(char *file, int line, char *type, size_t size) 
{
  void *addr=os_malloc(size);
  librdf_memory* node;
  
  if(!addr)
    return NULL;
  node=librdf_add_memory(file, line, type, size, addr);
#if defined(LIBRDF_MEMORY_DEBUG) && LIBRDF_MEMORY_DEBUG > 1
  fprintf(stderr, "%s:%d: malloced %d bytes for type %s at %p\n",
          node->file, node->line, node->size, node->type, node->addr);
#endif
  return addr;
}


void*
librdf_calloc(char *file, int line, char *type, size_t nmemb, size_t size) 
{
  void *addr=os_calloc(nmemb, size);
  librdf_memory* node;
  
  if(!addr)
    return NULL;
  node=librdf_add_memory(file, line, type, nmemb*size, addr);
#if defined(LIBRDF_MEMORY_DEBUG) && LIBRDF_MEMORY_DEBUG > 1
  fprintf(stderr, "%s:%d: calloced %d bytes for type %s at %p\n",
          node->file, node->line, node->size, node->type, node->addr);
#endif
  return addr;
}


void
librdf_free(char *file, int line, char *type, void *addr) 
{
  librdf_memory *node, *prev;

  node=librdf_memory_find_node(Rdf_Memory_List, addr, &prev);
  if(!node) {
    fprintf(stderr, "%s:%d: free of type %s addr %p never allocated\n",
            file, line, type, addr);
    abort();
    return;
  }
  
#if defined(LIBRDF_MEMORY_DEBUG) && LIBRDF_MEMORY_DEBUG > 1
  fprintf(stderr,
	  "%s:%d: freeing %d bytes for type %s at %p allocated at %s:%d\n",
	  file,line, node->size, node->type, node->addr,
	  node->file, node->line);
#endif

  if(node == Rdf_Memory_List)
    Rdf_Memory_List=node->next;
  else
    prev->next=node->next;

  /* free node */
  librdf_free_memory(node);

  return;
}


/* fh is actually a FILE* */
void
librdf_memory_report(FILE *fh)
{
  librdf_memory *node, *next;
  
  if(!Rdf_Memory_List)
    return;
  
  fprintf((FILE*)fh, "ALLOCATED MEMORY REPORT\n");
  fprintf((FILE*)fh, "-----------------------\n");
  for(node=Rdf_Memory_List; node; node=next) {
    next=node->next;
    fprintf((FILE*)fh, "%s:%d: %d bytes allocated for type %s at %p\n",
            node->file, node->line, node->size, node->type, node->addr);
    librdf_free_memory(node);
  }
  fprintf((FILE*)fh, "-----------------------\n");
}
#endif
