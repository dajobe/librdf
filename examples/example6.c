/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example6.c - Redland example code using model methods load and to_string
 *
 * $Id$
 *
 * Copyright (C) 2004 David Beckett - http://purl.org/net/dajobe/
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


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include <redland.h>

/* one prototype needed */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_world* world;
  librdf_storage *storage;
  librdf_model* model;
  librdf_uri* uri;
  unsigned char *string;
  
  librdf_world_open(world=librdf_new_world());

  model=librdf_new_model(world, storage=librdf_new_storage(world, "memory", NULL, NULL), NULL);

  uri=librdf_new_uri(world,"http://planetrdf.com/index.rdf");
  librdf_model_load(model, uri, NULL, NULL, NULL);

  string=librdf_model_to_string(model, uri, "ntriples", NULL, NULL);
  if(!string)
    printf("Failed to serialize model\n");
  else {
    printf("Made a %d byte string\n", strlen((char*)string));
    free(string);
  }

  librdf_free_uri(uri);
  
  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_free_world(world);

#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
