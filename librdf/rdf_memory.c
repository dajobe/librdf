/*
 * rdf_memory.c - RDF Memory Debugging Implementation
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



#include <stdio.h>
#include <stdlib.h>

#define LIBRDF_DEBUG 1

#define LIBRDF_INTERNAL
#include <rdf_config.h>

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


static librdf_memory* librdf_memory_find_node(librdf_memory* list, void *addr, librdf_memory** prev);


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
  free(node);
}


static int
librdf_add_memory(char *file, int line, char *type,
               size_t size, void *addr) 
{
  librdf_memory* node;
  
  node=librdf_memory_find_node(Rdf_Memory_List, addr, NULL);

  if(node) {
    /* duplicated node added! */
    fprintf(stderr, "librdf_add_memory: Duplicate memory address %p added\n", addr);
    abort();
  }
  
    
  /* need new node */
  node=(librdf_memory*)calloc(sizeof(librdf_memory), 1);
  if(!node)
    return 1;


  node->file=file; /* pointer to static string, no need to free */
  node->line=line;
  node->type=type; /* pointer to static string, no need to free */
  node->size=size;
  node->addr=addr;


  /* only now touch list */
  node->next=Rdf_Memory_List;
  Rdf_Memory_List=node;

  return 0;
}


/* memory allocation with tracking */

void* librdf_malloc(char *file, int line, char *type, size_t size) 
{
  void *addr=malloc(size);
  if(!addr)
    return NULL;
  librdf_add_memory(file, line, type, size, addr);
  return addr;
}


void* librdf_calloc(char *file, int line, char *type, size_t nmemb, size_t size) 
{
  void *addr=calloc(nmemb, size);
  if(!addr)
    return NULL;
  librdf_add_memory(file, line, type, nmemb*size, addr);
  return addr;
}


void librdf_free(char *file, int line, char *type, void *addr) 
{
  librdf_memory *node, *prev;

  node=librdf_memory_find_node(Rdf_Memory_List, addr, &prev);
  if(!node) {
    fprintf(stderr, "%s:%d: free of type %s addr %p never allocated\n",
            file, line, type, addr);
    return;
  }
  
  if(node == Rdf_Memory_List)
    Rdf_Memory_List=node->next;
  else
    prev->next=node->next;

  /* free node */
  librdf_free_memory(node);

  return;
}


void
librdf_memory_report(FILE *fh)
{
  librdf_memory *node, *next;

  if(!Rdf_Memory_List)
    return;

  fprintf(stderr, "ALLOCATED MEMORY REPORT\n");
  fprintf(stderr, "-----------------------\n");
  for(node=Rdf_Memory_List; node; node=next) {
    next=node->next;
    fprintf(fh, "%s:%d: %d bytes allocated for type %s\n",
            node->file, node->line, node->size, node->type);
    librdf_free_memory(node);
  }
  fprintf(stderr, "-----------------------\n");
}
