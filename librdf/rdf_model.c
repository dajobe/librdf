/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_model.c - RDF Model implementation
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

#include <stdio.h>

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_list.h>
#include <rdf_node.h>
#include <rdf_statement.h>
#include <rdf_model.h>
#include <rdf_storage.h>
#include <rdf_stream.h>


/* class methods */
void
librdf_init_model(void)
{
  /* nothing to do here */
}

void
librdf_finish_model(void)
{
  /* nothing to do here */
}


/* constructors */

/* Create a new Model with storage */
librdf_model*
librdf_new_model(librdf_storage *storage, librdf_hash* options)
{
  librdf_model* model;
  
  model=(librdf_model*)LIBRDF_CALLOC(librdf_model, 1, sizeof(librdf_model));
  if(!model)
    return 0;
  
  if(storage && librdf_storage_open(storage, model)) {
    LIBRDF_FREE(librdf_model, model);
    return NULL;
  }
  
  model->storage=storage;
  
  return model;
}


/* Create a new Model from an existing Model - CLONE */
librdf_model*
librdf_new_model_from_model(librdf_model* model)
{
  abort();
  return NULL;
}


/* Create a new Model from a stream */
librdf_model*
librdf_new_model_from_stream(librdf_stream* stream)
{
  abort();
  return NULL;
}


/* destructor */
void
librdf_free_model(librdf_model *model)
{
  librdf_iterator* iterator;
  librdf_model* m;

  if(model->sub_models) {
    iterator=librdf_list_get_iterator(model->sub_models);
    if(iterator) {
      while(librdf_iterator_have_elements(iterator)) {
        m=(librdf_model*)librdf_iterator_get_next(iterator);
        if(m)
          librdf_free_model(m);
      }
      librdf_free_iterator(iterator);
    }
    librdf_free_list(model->sub_models);
  } else {
    if(model->storage)
      librdf_storage_close(model->storage);
  }
  LIBRDF_FREE(librdf_model, model);
}



/* functions / methods */
int
librdf_model_size(librdf_model* model)
{
  return librdf_storage_size(model->storage);
}


/* add statements */

/**
 * librdf_model_add_statement:
 * @model: model
 * @statement: statement
 * 
 * Add a statement to the model - after this, the statement becomes owned by
 * the model
 * 
 * Return value: non 0 on failure
 **/
int
librdf_model_add_statement(librdf_model* model, librdf_statement* statement)
{
  return librdf_storage_add_statement(model->storage, statement);
}


int
librdf_model_add_statements(librdf_model* model, librdf_stream* statement_stream)
{
  return librdf_storage_add_statements(model->storage, statement_stream);
}


int
librdf_model_add(librdf_model* model, librdf_node* subject, 
		 librdf_node* predicate, librdf_node* object)
{
  librdf_statement *statement;
  int result;
  
  statement=librdf_new_statement();
  if(!statement)
    return 1;

  librdf_statement_set_subject(statement, subject);
  librdf_statement_set_predicate(statement, predicate);
  librdf_statement_set_object(statement, object);

  result=librdf_model_add_statement(model, statement);
  if(result)
    librdf_free_statement(statement);
  
  return 0;
}


int
librdf_model_add_string_literal_statement(librdf_model* model, 
					  librdf_node* subject, 
					  librdf_node* predicate, char* string,
					  char *xml_language, int is_wf_xml)
{
  librdf_node* object;
  int result;
  
  object=librdf_new_node_from_literal(string, xml_language, is_wf_xml);
  if(!object)
    return 1;
  
  result=librdf_model_add(model, subject, predicate, object);
  if(result)
    librdf_free_node(object);
  
  return result;
}


/* remove statements */
int
librdf_model_remove_statement(librdf_model* model, librdf_statement* statement)
{
  return librdf_storage_remove_statement(model->storage, statement);
}


/* containment */
int
librdf_model_contains_statement(librdf_model* model, librdf_statement* statement)
{
  return librdf_storage_contains_statement(model->storage, statement);
}


/* serialise the entire model */
librdf_stream*
librdf_model_serialise(librdf_model* model)
{
  return librdf_storage_serialise(model->storage);
}


/* queries */
librdf_stream*
librdf_model_find_statements(librdf_model* model, 
                             librdf_statement* statement)
{
  return librdf_storage_find_statements(model->storage, statement);
}




/* submodels */
int
librdf_model_add_submodel(librdf_model* model, librdf_model* sub_model)
{
  librdf_list *l=model->sub_models;
  
  if(!l) {
    l=librdf_new_list();
    if(!l)
      return 1;
    model->sub_models=l;
  }
  
  if(!librdf_list_add(l, sub_model))
    return 1;
  
  return 0;
}



int
librdf_model_remove_submodel(librdf_model* model, librdf_model* sub_model)
{
  librdf_list *l=model->sub_models;
  
  if(!l)
    return 1;
  if(!librdf_list_remove(l, sub_model))
    return 1;
  
  return 0;
}


void
librdf_model_print(librdf_model *model, FILE *fh)
{
  librdf_stream* stream;
  librdf_statement* statement;
  char *s;
  
  stream=librdf_model_serialise(model);
  if(!stream)
    return;
  
  fputs("[[\n", fh);
  while(!librdf_stream_end(stream)) {
    statement=librdf_stream_next(stream);
    if(!statement)
      break;
    s=librdf_statement_to_string(statement);
    if(s) {
      fputs("  ", fh);
      fputs(s, fh);
      fputs("\n", fh);
      LIBRDF_FREE(cstring, s);
    }    
  }
  fputs("]]\n", fh);
  
  librdf_free_stream(stream);
}





#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_storage_factory* factory;
  librdf_storage* storage;
  librdf_model* model;
  librdf_statement *statement;
  char *program=argv[0];
  
  /* initialise statement and model modules */
  librdf_init_statement();
  librdf_init_storage();
  librdf_init_model();
  
  fprintf(stderr, "%s: Getting factory\n", program);
  factory=librdf_get_storage_factory(NULL);

  fprintf(stderr, "%s: Creating storage\n", program);
  storage=librdf_new_storage(factory, NULL);
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }
  fprintf(stderr, "%s: Creating model\n", program);
  model=librdf_new_model(storage, NULL);

  statement=librdf_new_statement();
  librdf_statement_set_subject(statement, librdf_new_node_from_uri_string("http://www.ilrt.bris.ac.uk/people/cmdjb/"));
  librdf_statement_set_predicate(statement, librdf_new_node_from_uri_string("http://purl.org/dc/elements/1.1/#Creator"));
  librdf_statement_set_object(statement, librdf_new_node_from_literal("Dave Beckett", NULL, 0));

  /* after this, statement becomes owned by model */
  librdf_model_add_statement(model, statement);

  fprintf(stderr, "%s: Printing model\n", program);
  librdf_model_print(model, stderr);
  
  fprintf(stderr, "%s: Freeing model\n", program);
  librdf_free_model(model);

  fprintf(stderr, "%s: Freeing storage\n", program);
  librdf_free_storage(storage);

  librdf_finish_model();
  librdf_finish_storage();
  librdf_finish_statement();
  
#ifdef LIBRDF_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}

#endif
