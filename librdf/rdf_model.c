/*
 * RDF Model implementation
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

#include <stdio.h>

#ifdef STANDALONE
#define RDF_DEBUG 1
#endif

#include <rdf_config.h>
#include <rdf_model.h>


/* class methods */
void
rdf_init_model(void)
{
}


/* constructors */

/* Create a new Model */
rdf_model*
rdf_new_model(void)
{
  return NULL;
}


/* Create a new Model from an existing Model - CLONE */
rdf_model*
rdf_new_model_from_model(rdf_model* model)
{
  return NULL;
}

/* destructor */
void
rdf_free_model(rdf_model *model)
{
}




/* functions / methods */
int
rdf_model_size(rdf_model* model)
{
  return 0;
}



int
rdf_model_add(rdf_model* model, rdf_node* subject, rdf_node* property, rdf_node* object)
{
  return 0;
}


int
rdf_model_add_string_literal_statement(rdf_model* model, rdf_node* subject, rdf_node* property, char* string)
{
  return 0;
}


int
rdf_model_add_statement(rdf_model* model, rdf_statement* statement)
{
  return 0;
}


int
rdf_model_remove_statement(rdf_model* model, rdf_statement* statement)
{
  return 0;
}


int
rdf_model_contains_statement(rdf_model* model, rdf_statement* statement)
{
  return 0;
}


/* returns an interator that returns all statements */
rdf_iterator*
rdf_model_get_all_statements(rdf_model* model)
{
  return NULL;
}



/* any of subject, property or object can be NULL */
/* returns count of number of matching statements */
int
rdf_model_find(rdf_model* model, rdf_node* subject, rdf_node* property, rdf_node* object)
{
  return 0;
}


/* returns iterator object that returns matching rdf_statements */
rdf_iterator*
rdf_model_find_(rdf_model* model, rdf_node* subject, rdf_node* property, rdf_node* object)
{
  return NULL;
}



/* get/set the model source URI */
rdf_uri*
rdf_model_get_source_uri(rdf_model* model)
{
  return NULL;
}


int
rdf_model_set_source_uri(rdf_model* model, rdf_uri *uri)
{
  return 0;
}


int
rdf_model_add_submodel(rdf_model* model, rdf_model* sub_model) 
{
  return 0;
}

  
int
rdf_model_remove_submodel(rdf_model* model, rdf_model* sub_model) 
{
  return 0;
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  rdf_model* model;
  char *program=argv[0];
  
  /* initialise model module */
  rdf_init_model();

  fprintf(stderr, "%s: Creating model\n", program);
  model=rdf_new_model();

  fprintf(stderr, "%s: Freeing model\n", program);
  rdf_free_model(model);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
