/*
 * rdf_model.c - RDF Model implementation
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

#include <stdio.h>

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#define LIBRDF_INTERNAL 1
#include <rdf_config.h>
#include <rdf_model.h>


/* class methods */
void
librdf_init_model(void)
{
}


/* constructors */

/* Create a new Model */
librdf_model*
librdf_new_model(void)
{
  return NULL;
}


/* Create a new Model from an existing Model - CLONE */
librdf_model*
librdf_new_model_from_model(librdf_model* model)
{
  return NULL;
}

/* destructor */
void
librdf_free_model(librdf_model *model)
{
}




/* functions / methods */
int
librdf_model_size(librdf_model* model)
{
  return 0;
}



int
librdf_model_add(librdf_model* model, librdf_node* subject, librdf_node* property, librdf_node* object)
{
  return 0;
}


int
librdf_model_add_string_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* property, char* string)
{
  return 0;
}


int
librdf_model_add_statement(librdf_model* model, librdf_statement* statement)
{
  return 0;
}


int
librdf_model_remove_statement(librdf_model* model, librdf_statement* statement)
{
  return 0;
}


int
librdf_model_contains_statement(librdf_model* model, librdf_statement* statement)
{
  return 0;
}


/* returns an interator that returns all statements */
librdf_iterator*
librdf_model_get_all_statements(librdf_model* model)
{
  return NULL;
}



/* any of subject, property or object can be NULL */
/* returns count of number of matching statements */
int
librdf_model_find(librdf_model* model, librdf_node* subject, librdf_node* property, librdf_node* object)
{
  return 0;
}


/* returns iterator object that returns matching librdf_statements */
librdf_iterator*
librdf_model_find_(librdf_model* model, librdf_node* subject, librdf_node* property, librdf_node* object)
{
  return NULL;
}



/* get/set the model source URI */
librdf_uri*
librdf_model_get_source_uri(librdf_model* model)
{
  return NULL;
}


int
librdf_model_set_source_uri(librdf_model* model, librdf_uri *uri)
{
  return 0;
}


int
librdf_model_add_submodel(librdf_model* model, librdf_model* sub_model) 
{
  return 0;
}

  
int
librdf_model_remove_submodel(librdf_model* model, librdf_model* sub_model) 
{
  return 0;
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_model* model;
  char *program=argv[0];
  
  /* initialise model module */
  librdf_init_model();

  fprintf(stderr, "%s: Creating model\n", program);
  model=librdf_new_model();

  fprintf(stderr, "%s: Freeing model\n", program);
  librdf_free_model(model);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
