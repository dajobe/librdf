/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example3.c - Redland example code creating model, statement and storing it in 10 lines
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


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include <redland.h>

/* one prototype needed */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_world* world;
  librdf_storage *storage;
  librdf_model* model;
  librdf_statement* statement;
  
  world=librdf_new_world();
  librdf_world_open(world);

  model=librdf_new_model(world, storage=librdf_new_storage(world, "hashes", "test", "hash-type='bdb',dir='.'"), NULL);

  librdf_model_add_statement(model, 
                             statement=librdf_new_statement_from_nodes(world, librdf_new_node_from_uri_string(world, "http://purl.org/net/dajobe/"),
                                                             librdf_new_node_from_uri_string(world, "http://purl.org/dc/elements/1.1/creator"),
                                                             librdf_new_node_from_literal(world, "Dave Beckett", NULL, 0)
                                                             )
                             );

  librdf_free_statement(statement);

  librdf_model_print(model, stdout);
  
  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_free_world(world);

#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
