/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_model.c - RDF Model implementation
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

#include <stdio.h>

#include <librdf.h>
#include <rdf_list.h>
#include <rdf_node.h>
#include <rdf_statement.h>
#include <rdf_model.h>
#include <rdf_storage.h>
#include <rdf_stream.h>


/**
 * librdf_init_model - Initialise librdf_model class
 **/
void
librdf_init_model(librdf_world *world)
{
  /* nothing to do here */
}


/**
 * librdf_finish_model - Terminate librdf_model class
 **/
void
librdf_finish_model(librdf_world *world)
{
  /* nothing to do here */
}


/**
 * librdf_new_model - Constructor - create a new librdf_storage object
 * @storage: &librdf_storage to use
 * @options_string: options to initialise model
 *
 * The options are encoded as described in librdf_hash_from_string()
 * and can be NULL if none are required.
 *
 * Return value: a new &librdf_model object or NULL on failure
 */
librdf_model*
librdf_new_model (librdf_world *world,
                  librdf_storage *storage, char *options_string) {
  librdf_hash* options_hash;
  librdf_model *model;

  if(!storage)
    return NULL;
  
  options_hash=librdf_new_hash(world, NULL);
  if(!options_hash)
    return NULL;
  
  if(librdf_hash_from_string(options_hash, options_string)) {
    librdf_free_hash(options_hash);
    return NULL;
  }

  model=librdf_new_model_with_options(world, storage, options_hash);
  librdf_free_hash(options_hash);
  return model;
}


/**
 * librdf_new_model - Constructor - Create a new librdf_model with storage
 * @storage: &librdf_storage storage to use
 * @options: &librdf_hash of options to use
 * 
 * Options are presently not used.
 *
 * Return value: a new &librdf_model object or NULL on failure
 **/
librdf_model*
librdf_new_model_with_options(librdf_world *world,
                              librdf_storage *storage, librdf_hash* options)
{
  librdf_model* model;

  if(!storage)
    return NULL;
  
  model=(librdf_model*)LIBRDF_CALLOC(librdf_model, 1, sizeof(librdf_model));
  if(!model)
    return NULL;
  
  model->world=world;

  if(storage && librdf_storage_open(storage, model)) {
    LIBRDF_FREE(librdf_model, model);
    return NULL;
  }
  
  model->storage=storage;
  
  return model;
}


/**
 * librdf_new_model_from_model - Copy constructor - create a new librdf_model from an existing one
 * @old_model: the existing &librdf_model
 * 
 * Creates a new model as a copy of the existing model in the same
 * storage context.
 * 
 * Return value: a new &librdf_model or NULL on failure
 **/
librdf_model*
librdf_new_model_from_model(librdf_model* old_model)
{
  librdf_storage *new_storage;
  librdf_model *new_model;
  
  new_storage=librdf_new_storage_from_storage(old_model->storage);
  if(!new_storage)
    return NULL;

  new_model=librdf_new_model_with_options(old_model->world, new_storage, NULL);
  if(!new_model)
    librdf_free_storage(new_storage);

  return new_model;
}


/**
 * librdf_free_model - Destructor - Destroy a librdf_model object
 * @model: &librdf_model model to destroy
 * 
 **/
void
librdf_free_model(librdf_model *model)
{
  librdf_iterator* iterator;
  librdf_model* m;

  if(model->sub_models) {
    iterator=librdf_list_get_iterator(model->sub_models);
    if(iterator) {
      while(!librdf_iterator_end(iterator)) {
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



/**
 * librdf_model_size - get the number of statements in the model
 * @model: &librdf_model object
 * 
 * Return value: the number of statements
 **/
int
librdf_model_size(librdf_model* model)
{
  return librdf_storage_size(model->storage);
}


/**
 * librdf_model_add_statement - Add a statement to the model
 * @model: model object
 * @statement: statement object
 * 
 * Return value: non 0 on failure
 **/
int
librdf_model_add_statement(librdf_model* model, librdf_statement* statement)
{
  return librdf_storage_add_statement(model->storage, statement);
}


/**
 * librdf_model_add_statements - Add a stream of statements to the model
 * @model: model object
 * @statement_stream: stream of statements to use
 * 
 * Return value: non 0 on failure
 **/
int
librdf_model_add_statements(librdf_model* model, librdf_stream* statement_stream)
{
  return librdf_storage_add_statements(model->storage, statement_stream);
}


/**
 * librdf_model_add - Create and add a new statement about a resource to the model
 * @model: model object
 * @subject: &librdf_node of subject
 * @predicate: &librdf_node of predicate
 * @object: &librdf_node of object (literal or resource)
 * 
 * After this method, the &librdf_node objects become owned by the model.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_model_add(librdf_model* model, librdf_node* subject, 
		 librdf_node* predicate, librdf_node* object)
{
  librdf_statement *statement;
  int result;
  
  statement=librdf_new_statement(model->world);
  if(!statement)
    return 1;

  librdf_statement_set_subject(statement, subject);
  librdf_statement_set_predicate(statement, predicate);
  librdf_statement_set_object(statement, object);

  result=librdf_model_add_statement(model, statement);
  librdf_free_statement(statement);
  
  return result;
}


/**
 * librdf_model_add_string_literal_statement - Create and add a new statement about a literal to the model
 * @model: model object
 * @subject: &librdf_node of subject
 * @predicate: &librdf_node of predicate
 * @string: string literal conten
 * @xml_language: language of literal
 * @xml_space: XML space properties
 * @is_wf_xml: literal is XML
 * 
 * The language can be set to NULL if not used.
 *
 * The XML space property can be set to three values: 0 - not known,
 * 1 - default or 2 - preserve space.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_model_add_string_literal_statement(librdf_model* model, 
					  librdf_node* subject, 
					  librdf_node* predicate, char* string,
					  char *xml_language, int xml_space,
                                          int is_wf_xml)
{
  librdf_node* object;
  int result;
  
  object=librdf_new_node_from_literal(model->world,
                                      string, xml_language, xml_space, 
                                      is_wf_xml);
  if(!object)
    return 1;
  
  result=librdf_model_add(model, subject, predicate, object);
  if(result)
    librdf_free_node(object);
  
  return result;
}


/**
 * librdf_model_remove_statement - Remove a known statement from the model
 * @model: the model object
 * @statement: the statement
 *
 * Return value: non 0 on failure
 **/
int
librdf_model_remove_statement(librdf_model* model, librdf_statement* statement)
{
  return librdf_storage_remove_statement(model->storage, statement);
}


/**
 * librdf_model_contains_statement - Check for a statement in the model
 * @model: the model object
 * @statement: the statement
 * 
 * Return value: non 0 if the model contains the statement
 **/
int
librdf_model_contains_statement(librdf_model* model, librdf_statement* statement)
{
  return librdf_storage_contains_statement(model->storage, statement);
}


/**
 * librdf_model_serialise - serialise the entire model as a stream
 * @model: the model object
 * 
 * Return value: a &librdf_stream or NULL on failure
 **/
librdf_stream*
librdf_model_serialise(librdf_model* model)
{
  return librdf_storage_serialise(model->storage);
}


/**
 * librdf_model_find_statements - find matching statements in the model
 * @model: the model object
 * @statement: the partial statement to match
 * 
 * The partial statement is a statement where the subject, predicate
 * and/or object can take the value NULL which indicates a match with
 * any value in the model
 * 
 * Return value: a &librdf_stream of statements (can be empty) or NULL
 * on failure.
 **/
librdf_stream*
librdf_model_find_statements(librdf_model* model, 
                             librdf_statement* statement)
{
  return librdf_storage_find_statements(model->storage, statement);
}


/**
 * librdf_model_get_sources - return the sources (subjects) of arc in an RDF graph given arc (predicate) and target (object)
 * @model: &librdf_model object
 * @arc: &librdf_node arc
 * @target: &librdf_node target
 * 
 * Searches the model for arcs matching the given arc and target
 * and returns a list of the source &librdf_node objects as an iterator
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_model_get_sources(librdf_model *model,
                         librdf_node *arc, librdf_node *target) 
{
  return librdf_storage_get_sources(model->storage, arc, target);
}


/**
 * librdf_model_get_arcs - return the arcs (predicates) of an arc in an RDF graph given source (subject) and target (object)
 * @model: &librdf_model object
 * @source: &librdf_node source
 * @target: &librdf_node target
 * 
 * Searches the model for arcs matching the given source and target
 * and returns a list of the arc &librdf_node objects as an iterator
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_model_get_arcs(librdf_model *model,
                      librdf_node *source, librdf_node *target) 
{
  return librdf_storage_get_arcs(model->storage, source, target);
}


/**
 * librdf_model_get_targets - return the targets (objects) of an arc in an RDF graph given source (subject) and arc (predicate)
 * @model: &librdf_model object
 * @source: &librdf_node source
 * @arc: &librdf_node arc
 * 
 * Searches the model for targets matching the given source and arc
 * and returns a list of the source &librdf_node objects as an iterator
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_model_get_targets(librdf_model *model,
                         librdf_node *source, librdf_node *arc) 
{
  return librdf_storage_get_targets(model->storage, source, arc);
}


/**
 * librdf_model_get_source - return one source (subject) of arc in an RDF graph given arc (predicate) and target (object)
 * @model: &librdf_model object
 * @arc: &librdf_node arc
 * @target: &librdf_node target
 * 
 * Searches the model for arcs matching the given arc and target
 * and returns one &librdf_node object
 * 
 * Return value:  &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_model_get_source(librdf_model *model,
                        librdf_node *arc, librdf_node *target) 
{
  librdf_iterator *iterator=librdf_storage_get_sources(model->storage, 
                                                       arc, target);
  librdf_node *node=(librdf_node*)librdf_iterator_get_next(iterator);
  librdf_free_iterator(iterator);
  return node;
}


/**
 * librdf_model_get_arc - return one arc (predicate) of an arc in an RDF graph given source (subject) and target (object)
 * @model: &librdf_model object
 * @source: &librdf_node source
 * @target: &librdf_node target
 * 
 * Searches the model for arcs matching the given source and target
 * and returns one &librdf_node object
 * 
 * Return value:  &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_model_get_arc(librdf_model *model,
                     librdf_node *source, librdf_node *target) 
{
  librdf_iterator *iterator=librdf_storage_get_arcs(model->storage, 
                                                    source, target);
  librdf_node *node=(librdf_node*)librdf_iterator_get_next(iterator);
  librdf_free_iterator(iterator);
  return node;
}


/**
 * librdf_model_get_target - return one target (object) of an arc in an RDF graph given source (subject) and arc (predicate)
 * @model: &librdf_model object
 * @source: &librdf_node source
 * @arc: &librdf_node arc
 * 
 * Searches the model for targets matching the given source and arc
 * and returns one &librdf_node object
 * 
 * Return value:  &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_model_get_target(librdf_model *model,
                        librdf_node *source, librdf_node *arc) 
{
  librdf_iterator *iterator=librdf_storage_get_targets(model->storage, 
                                                       source, arc);
  librdf_node *node=(librdf_node*)librdf_iterator_get_next(iterator);
  librdf_free_iterator(iterator);
  return node;
}


/**
 * librdf_model_add_submodel - add a sub-model to the model
 * @model: the model object
 * @sub_model: the sub model to add
 * 
 * FIXME: Not tested
 * 
 * Return value: non 0 on failure
 **/
int
librdf_model_add_submodel(librdf_model* model, librdf_model* sub_model)
{
  librdf_list *l=model->sub_models;
  
  if(!l) {
    l=librdf_new_list(model->world);
    if(!l)
      return 1;
    model->sub_models=l;
  }
  
  if(!librdf_list_add(l, sub_model))
    return 1;
  
  return 0;
}



/**
 * librdf_model_remove_submodel - remove a sub-model from the model
 * @model: the model object
 * @sub_model: the sub model to remove
 * 
 * FIXME: Not tested
 * 
 * Return value: non 0 on failure
 **/
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



/**
 * librdf_model_get_arcs_in - return the properties pointing to the given resource
 * @model: &librdf_model object
 * @node: &librdf_node resource node
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_model_get_arcs_in(librdf_model *model, librdf_node *node) 
{
  return librdf_storage_get_arcs_in(model->storage, node);
}


/**
 * librdf_model_get_arcs_out - return the properties pointing from the given resource
 * @model: &librdf_model object
 * @node: &librdf_node resource node
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_model_get_arcs_out(librdf_model *model, librdf_node *node) 
{
  return librdf_storage_get_arcs_out(model->storage, node);
}


/**
 * librdf_model_has_arc_in - check if a node has a given property pointing to it
 * @model: &librdf_model object
 * @node: &librdf_node resource node
 * @property: &librdf_node property node
 * 
 * Return value: non 0 if arc property does point to the resource node
 **/
int
librdf_model_has_arc_in(librdf_model *model, librdf_node *node, 
                        librdf_node *property) 
{
  return librdf_storage_has_arc_in(model->storage, node, property);
}


/**
 * librdf_model_has_arc_out - check if a node has a given property pointing from it
 * @model: &librdf_model object
 * @node: &librdf_node resource node
 * @property: &librdf_node property node
 * 
 * Return value: non 0 if arc property does point from the resource node
 **/
int
librdf_model_has_arc_out(librdf_model *model, librdf_node *node,
                         librdf_node *property) 
{
  return librdf_storage_has_arc_out(model->storage, node, property);
}




/**
 * librdf_model_print - print the model
 * @model: the model object
 * @fh: the FILE stream to print to
 * 
 **/
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
    librdf_free_statement(statement);
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
  librdf_storage* storage;
  librdf_model* model;
  librdf_statement *statement;
  char *program=argv[0];
  
  /* initialise dependent modules - all of them! */
  librdf_world *world=librdf_new_world();
  librdf_world_open(world);
  
  fprintf(stderr, "%s: Creating storage\n", program);
  storage=librdf_new_storage(world, NULL, NULL, NULL);
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }
  fprintf(stderr, "%s: Creating model\n", program);
  model=librdf_new_model(world, storage, NULL);

  statement=librdf_new_statement(world);
  /* after this, nodes become owned by model */
  librdf_statement_set_subject(statement, librdf_new_node_from_uri_string(world, "http://www.ilrt.bris.ac.uk/people/cmdjb/"));
  librdf_statement_set_predicate(statement, librdf_new_node_from_uri_string(world, "http://purl.org/dc/elements/1.1/#Creator"));
  librdf_statement_set_object(statement, librdf_new_node_from_literal(world, "Dave Beckett", NULL, 0, 0));

  librdf_model_add_statement(model, statement);
  librdf_free_statement(statement);

  fprintf(stderr, "%s: Printing model\n", program);
  librdf_model_print(model, stderr);
  
  fprintf(stderr, "%s: Freeing model\n", program);
  librdf_free_model(model);

  fprintf(stderr, "%s: Freeing storage\n", program);
  librdf_free_storage(storage);

  librdf_free_world(world);
  
#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}

#endif
