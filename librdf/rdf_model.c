/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_model.c - RDF Model implementation
 *
 * $Id$
 *
 * Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
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


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit()  */
#endif

#include <librdf.h>
#include <raptor.h>

#ifndef STANDALONE
/* prototypes for helper functions */
static void librdf_delete_model_factories(void);


/**
 * librdf_init_model - Initialise librdf_model class
 * @world: redland world object
 **/
void
librdf_init_model(librdf_world *world)
{
  /* Always have model storage implementation available */
  librdf_init_model_storage(world);
}


/**
 * librdf_finish_model - Terminate librdf_model class
 * @world: redland world object
 **/
void
librdf_finish_model(librdf_world *world)
{
  librdf_delete_model_factories();
}


/* statics */

/* list of storage factories */
static librdf_model_factory* models;


/*
 * librdf_delete_model_factories - helper function to delete all the registered model factories
 */
static void
librdf_delete_model_factories(void)
{
  librdf_model_factory *factory, *next;
  
  for(factory=models; factory; factory=next) {
    next=factory->next;
    LIBRDF_FREE(librdf_model_factory, factory->name);
    LIBRDF_FREE(librdf_model_factory, factory);
  }
  models=NULL;
}


/*
 * librdf_model_supports_contexts - helper function to determine if this model supports contexts
 **/
static inline int
librdf_model_supports_contexts(librdf_model* model) {
  return model->supports_contexts;
}



/* class methods */

/**
 * librdf_model_register_factory - Register a model factory
 * @world: redland world object
 * @name: the model factory name
 * @factory: pointer to function to call to register the factory
 * 
 **/
void
librdf_model_register_factory(librdf_world *world, const char *name,
                              void (*factory) (librdf_model_factory*)) 
{
  librdf_model_factory *model, *h;
  char *name_copy;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2("Received registration for model %s\n", name);
#endif
  
  model=(librdf_model_factory*)LIBRDF_CALLOC(librdf_model_factory, 1,
                                             sizeof(librdf_model_factory));
  if(!model)
    LIBRDF_FATAL1(world, LIBRDF_FROM_MODEL, "Out of memory");

  name_copy=(char*)LIBRDF_CALLOC(cstring, strlen(name)+1, 1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_model, model);
    LIBRDF_FATAL1(world, LIBRDF_FROM_MODEL, "Out of memory");
  }
  strcpy(name_copy, name);
  model->name=name_copy;
        
  for(h = models; h; h = h->next ) {
    if(!strcmp(h->name, name_copy)) {
      LIBRDF_FREE(cstring, name_copy);
      librdf_log(world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_MODEL, NULL,
                 "model %s already registered", h->name);
      return;
    }
  }
  
  /* Call the model registration function on the new object */
  (*factory)(model);
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3("%s has context size %d\n", name, model->context_length);
#endif
  
  model->next = models;
  models = model;
}


/**
 * librdf_get_model_factory - Get a model factory by name
 * @name: the factory name or NULL for the default factory
 * 
 * Return value: the factory object or NULL if there is no such factory
 **/
librdf_model_factory*
librdf_get_model_factory (const char *name) 
{
  librdf_model_factory *factory;

  /* return 1st model if no particular one wanted - why? */
  if(!name) {
    factory=models;
    if(!factory) {
      LIBRDF_DEBUG1("No (default) models registered\n");
      return NULL;
    }
  } else {
    for(factory=models; factory; factory=factory->next) {
      if(!strcmp(factory->name, name)) {
        break;
      }
    }
    /* else FACTORY name not found */
    if(!factory) {
      LIBRDF_DEBUG2("No model with name %s found\n", name);
      return NULL;
    }
  }
        
  return factory;
}




/**
 * librdf_new_model - Constructor - create a new storage librdf_model object
 * @world: redland world object
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

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

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
 * librdf_new_model_with_options - Constructor - Create a new librdf_model with storage
 * @world: redland world object
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
  librdf_model *model;
  librdf_uri *uri;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  if(!storage)
    return NULL;
  
  model=(librdf_model*)LIBRDF_CALLOC(librdf_model, 1, sizeof(librdf_model));
  if(!model)
    return NULL;
  
  model->world=world;

  model->factory=librdf_get_model_factory("storage");
  if(!model->factory) {
    LIBRDF_FREE(librdf_model, model);
    return NULL;
  }
    
  model->context=LIBRDF_MALLOC(data, model->factory->context_length);

  if(model->context && model->factory->create(model, storage, options)) {
    LIBRDF_FREE(data, model->context);
    LIBRDF_FREE(librdf_model, model);
    return NULL;
  }

  uri=librdf_new_uri(world, (const unsigned char*)LIBRDF_MODEL_FEATURE_CONTEXTS);
  if(uri) {
    librdf_node *node=librdf_model_get_feature(model, uri);
    if(node) {
      model->supports_contexts=atoi((const char*)librdf_node_get_literal_value(node));
      librdf_free_node(node);
    }
    librdf_free_uri(uri);
  }

  model->usage=1;

  return model;
}


/**
 * librdf_new_model_from_model - Copy constructor - create a new librdf_model from an existing one
 * @model: the existing &librdf_model
 * 
 * Creates a new model as a copy of the existing model in the same
 * storage context.
 * 
 * Return value: a new &librdf_model or NULL on failure
 **/
librdf_model*
librdf_new_model_from_model(librdf_model* model)
{
  librdf_model *new_model;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);

  new_model=model->factory->clone(model);
  new_model->supports_contexts=model->supports_contexts;
  new_model->usage=1;
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

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(model, librdf_model);

  if(--model->usage)
    return;
  
  if(model->sub_models) {
    iterator=librdf_list_get_iterator(model->sub_models);
    if(iterator) {
      while(!librdf_iterator_end(iterator)) {
        m=(librdf_model*)librdf_iterator_get_object(iterator);
        if(m)
          librdf_free_model(m);
        librdf_iterator_next(iterator);
      }
      librdf_free_iterator(iterator);
    }
    librdf_free_list(model->sub_models);
  } else {
    model->factory->destroy(model);
  }
  LIBRDF_FREE(data, model->context);

  LIBRDF_FREE(librdf_model, model);
}


void
librdf_model_add_reference(librdf_model *model)
{
  model->usage++;
}

void
librdf_model_remove_reference(librdf_model *model)
{
  model->usage--;
}


/* methods */

/**
 * librdf_model_size - get the number of statements in the model
 * @model: &librdf_model object
 * 
 * WARNING: Not all underlying stores can return the size of the graph
 * In which case the return value will be negative.
 *
 * Return value: the number of statements or <0 if not possible
 **/
int
librdf_model_size(librdf_model* model)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, -1);

  return model->factory->size(model);
}


/**
 * librdf_model_add_statement - Add a statement to the model
 * @model: model object
 * @statement: statement object
 * 
 * The passed-in statement is copied when added to the model, not
 * shared with the model.  It must be a complete statement - all
 * of subject, predicate, object parts must be present.
 *
 * Return value: non 0 on failure
 **/
int
librdf_model_add_statement(librdf_model* model, librdf_statement* statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  if(!librdf_statement_is_complete(statement))
    return 1;
  
  return model->factory->add_statement(model, statement);
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement_stream, librdf_statement, 1);

  return model->factory->add_statements(model, statement_stream);
}


/**
 * librdf_model_add - Create and add a new statement about a resource to the model
 * @model: model object
 * @subject: &librdf_node of subject
 * @predicate: &librdf_node of predicate
 * @object: &librdf_node of object (literal or resource)
 * 
 * After this method, the &librdf_node objects become owned by the model.
 * All of subject, predicate and object must be non-NULL.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_model_add(librdf_model* model, librdf_node* subject, 
		 librdf_node* predicate, librdf_node* object)
{
  librdf_statement *statement;
  int result;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(subject, librdf_node, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(predicate, librdf_node, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(object, librdf_node, 1);

  if(!subject || !predicate || !object)
    return 1;

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
 * librdf_model_add_typed_literal_statement - Create and add a new statement about a typed literal to the model
 * @model: model object
 * @subject: &librdf_node of subject
 * @predicate: &librdf_node of predicate
 * @literal: string literal content
 * @xml_language: language of literal
 * @datatype_uri: datatype &librdf_uri
 * 
 * After this method, the &librdf_node subject and predicate become
 * owned by the model.
 * 
 * The language can be set to NULL if not used.
 * All of subject, predicate and literal must be non-NULL.
 *
 * Return value: non 0 on failure
 **/
int
librdf_model_add_typed_literal_statement(librdf_model* model, 
                                         librdf_node* subject, 
                                         librdf_node* predicate, 
                                         const unsigned char* literal,
                                         char *xml_language,
                                         librdf_uri *datatype_uri)
{
  librdf_node* object;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(subject, librdf_node, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(predicate, librdf_node, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(literal, string, 1);

  if(!subject || !predicate || !literal)
    return 1;

  object=librdf_new_node_from_typed_literal(model->world,
                                            literal, xml_language, 
                                            datatype_uri);
  if(!object)
    return 1;
  
  return librdf_model_add(model, subject, predicate, object);
}


/**
 * librdf_model_add_string_literal_statement - Create and add a new statement about a literal to the model
 * @model: model object
 * @subject: &librdf_node of subject
 * @predicate: &librdf_node of predicate
 * @literal: string literal conten
 * @xml_language: language of literal
 * @is_wf_xml: literal is XML
 * 
 * The language can be set to NULL if not used.
 * All of subject, predicate and literal must be non-NULL.
 *
 * 0.9.12: xml_space argument deleted
 *
 * Return value: non 0 on failure
 **/
int
librdf_model_add_string_literal_statement(librdf_model* model, 
					  librdf_node* subject, 
					  librdf_node* predicate, 
                                          const unsigned char* literal,
					  char *xml_language,
                                          int is_wf_xml)
{
  librdf_node* object;
  int result;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(subject, librdf_node, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(predicate, librdf_node, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(literal, string, 1);

  if(!subject || !predicate || !literal)
    return 1;

  object=librdf_new_node_from_literal(model->world,
                                      literal, xml_language, 
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
 * It must be a complete statement - all of subject, predicate, object
 * parts must be present.
 *
 * Return value: non 0 on failure
 **/
int
librdf_model_remove_statement(librdf_model* model, librdf_statement* statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  if(!librdf_statement_is_complete(statement))
    return 1;

  return model->factory->remove_statement(model, statement);
}


/**
 * librdf_model_contains_statement - Check for a statement in the model
 * @model: the model object
 * @statement: the statement
 * 
 * It must be a complete statement - all of subject, predicate, object
 * parts must be present.  Use librdf_model_find_statements to search
 * for partial statement matches.
 *
 * WARNING: librdf_model_contains_statement may not work correctly
 * with stores using contexts.  In this case, a search using
 * librdf_model_find_statements for a non-empty list will
 * return the correct result.
 *
 * Return value: non 0 if the model contains the statement
 **/
int
librdf_model_contains_statement(librdf_model* model, librdf_statement* statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

  if(!librdf_statement_is_complete(statement))
    return 1;

  return model->factory->contains_statement(model, statement);
}


/**
 * librdf_model_as_stream - list the model contents as a stream of statements
 * @model: the model object
 * 
 * Return value: a &librdf_stream or NULL on failure
 **/
librdf_stream*
librdf_model_as_stream(librdf_model* model)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);

  return model->factory->serialise(model);
}


/**
 * librdf_model_serialise - serialise the entire model as a stream (DEPRECATED)
 * @model: the model object
 * 
 * DEPRECATED to reduce confusion with the librdf_serializer class.
 * Please use librdf_model_as_stream.
 *
 * Return value: a &librdf_stream or NULL on failure
 **/
librdf_stream*
librdf_model_serialise(librdf_model* model)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);

  return model->factory->serialise(model);
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  return model->factory->find_statements(model, statement);
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(arc, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(target, librdf_node, NULL);

  return model->factory->get_sources(model, arc, target);
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(source, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(target, librdf_node, NULL);

  return model->factory->get_arcs(model, source, target);
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(source, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(arc, librdf_node, NULL);

  return model->factory->get_targets(model, source, arc);
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
 * Return value:  a new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_model_get_source(librdf_model *model,
                        librdf_node *arc, librdf_node *target) 
{
  librdf_iterator *iterator;
  librdf_node *node;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(arc, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(target, librdf_node, NULL);

  iterator=librdf_model_get_sources(model, arc, target);
  if(!iterator)
    return NULL;
  
  node=(librdf_node*)librdf_iterator_get_object(iterator);
  if(node)
    node=librdf_new_node_from_node(node);
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
 * Return value:  a new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_model_get_arc(librdf_model *model,
                     librdf_node *source, librdf_node *target) 
{
  librdf_iterator *iterator;
  librdf_node *node;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(source, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(target, librdf_node, NULL);

  iterator=librdf_model_get_arcs(model, source, target);
  if(!iterator)
    return NULL;
  
  node=(librdf_node*)librdf_iterator_get_object(iterator);
  if(node)
    node=librdf_new_node_from_node(node);
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
 * Return value:  a new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_model_get_target(librdf_model *model,
                        librdf_node *source, librdf_node *arc) 
{
  librdf_iterator *iterator;
  librdf_node *node;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(source, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(arc, librdf_node, NULL);

  iterator=librdf_model_get_targets(model, source, arc);
  if(!iterator)
    return NULL;
  
  node=(librdf_node*)librdf_iterator_get_object(iterator);
  if(node)
    node=librdf_new_node_from_node(node);
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
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(sub_model, librdf_model, 1);

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
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(sub_model, librdf_model, 1);

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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  return model->factory->get_arcs_in(model, node);
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  return model->factory->get_arcs_out(model, node);
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(property, librdf_node, 0);

  return model->factory->has_arc_in(model, node, property);
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(property, librdf_node, 0);

  return model->factory->has_arc_out(model, node, property);
}




/**
 * librdf_model_print - print the model
 * @model: the model object
 * @fh: the FILE stream to print to
 * 
 * This method is for debugging and the format of the output should
 * not be relied on.
 **/
void
librdf_model_print(librdf_model *model, FILE *fh)
{
  librdf_stream* stream;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(model, librdf_model);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(fh, FILE*);

  stream=librdf_model_as_stream(model);
  if(!stream)
    return;
  fputs("[[\n", fh);
  librdf_stream_print(stream, fh);
  fputs("]]\n", fh);
  librdf_free_stream(stream);
}


/**
 * librdf_model_context_add_statement - Add a statement to a model with a context
 * @model: &librdf_model object
 * @context: &librdf_node context
 * @statement: &librdf_statement statement object
 * 
 * It must be a complete statement - all
 * of subject, predicate, object parts must be present.
 *
 * If @context is NULL, this is equivalent to librdf_model_add_statement
 *
 * Return value: Non 0 on failure
 **/
int
librdf_model_context_add_statement(librdf_model* model, 
                                   librdf_node* context,
                                   librdf_statement* statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  if(!librdf_statement_is_complete(statement))
    return 1;

  if(!librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  return model->factory->context_add_statement(model, context, statement);
}



/**
 * librdf_model_context_add_statements - Add statements to a model with a context
 * @model: &librdf_model object
 * @context: &librdf_node context
 * @stream: &librdf_stream stream object
 * 
 * If @context is NULL, this is equivalent to librdf_model_add_statements
 *
 * Return value: Non 0 on failure
 **/
int
librdf_model_context_add_statements(librdf_model* model, 
                                    librdf_node* context,
                                    librdf_stream* stream) 
{
  int status=0;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(stream, librdf_stream, 1);

  if(!stream)
    return 1;

  if(!librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  if(model->factory->context_add_statements)
    return model->factory->context_add_statements(model, context, stream);

  while(!librdf_stream_end(stream)) {
    librdf_statement* statement=librdf_stream_get_object(stream);
    if(!statement)
      break;
    status=librdf_model_context_add_statement(model, context, statement);
    if(status)
      break;
    librdf_stream_next(stream);
  }

  return status;
}



/**
 * librdf_model_context_remove_statement - Remove a statement from a model in a context
 * @model: &librdf_model object
 * @context: &librdf_uri context
 * @statement: &librdf_statement statement
 * 
 * It must be a complete statement - all of subject, predicate, object
 * parts must be present.
 *
 * If @context is NULL, this is equivalent to librdf_model_remove_statement
 *
 * Return value: Non 0 on failure
 **/
int
librdf_model_context_remove_statement(librdf_model* model,
                                      librdf_node* context,
                                      librdf_statement* statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  if(!librdf_statement_is_complete(statement))
    return 1;

  if(!librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  return model->factory->context_remove_statement(model, context, statement);
}


/**
 * librdf_model_context_remove_statements - Remove statements from a model with the given context
 * @model: &librdf_model object
 * @context: &librdf_uri context
 * 
 * Return value: Non 0 on failure
 **/
int
librdf_model_context_remove_statements(librdf_model* model,
                                       librdf_node* context) 
{
  librdf_stream *stream;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, librdf_node, 1);

  if(!librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  if(model->factory->context_remove_statements)
    return model->factory->context_remove_statements(model, context);

  stream=librdf_model_context_as_stream(model, context);
  if(!stream)
    return 1;

  while(!librdf_stream_end(stream)) {
    librdf_statement *statement=librdf_stream_get_object(stream);
    if(!statement)
      break;
    librdf_model_context_remove_statement(model, context, statement);
    librdf_stream_next(stream);
  }
  librdf_free_stream(stream);  
  return 0;
}


/**
 * librdf_model_context_as_stream - list all statements in a model context
 * @model: &librdf_model object
 * @context: &librdf_uri context
 * 
 * Return value: &librdf_stream of statements or NULL on failure
 **/
librdf_stream*
librdf_model_context_as_stream(librdf_model* model, librdf_node* context) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, librdf_node, NULL);

  if(!librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  return model->factory->context_serialize(model, context);
}


/**
 * librdf_model_context_serialize - List all statements in a model context
 * @model: &librdf_model object
 * @context: &librdf_uri context
 * 
 * DEPRECATED to reduce confusion with the librdf_serializer class.
 * Please use librdf_model_context_as_stream.
 *
 * Return value: &librdf_stream of statements or NULL on failure
 **/
librdf_stream*
librdf_model_context_serialize(librdf_model* model, librdf_node* context) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, librdf_node, NULL);

  if(!librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  return model->factory->context_serialize(model, context);
}


/**
 * librdf_model_query_execute - Execute a query against the model
 * @model: &librdf_model object
 * @query: &librdf_query object
 * 
 * Run the given query against the model and return a &librdf_stream of
 * matching &librdf_statement objects
 * 
 * Return value: &librdf_query_results or NULL on failure
 **/
librdf_query_results*
librdf_model_query_execute(librdf_model* model, librdf_query* query) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, NULL);

  return model->factory->query_execute(model, query);
}


/**
 * librdf_model_sync - Synchronise the model to the model implementation
 * @model: &librdf_model object
 * 
 **/
void
librdf_model_sync(librdf_model* model) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(model, librdf_model);

  if(model->factory->sync)
    model->factory->sync(model);
}


/**
 * librdf_model_get_storage - return the storage of this model
 * @model: &librdf_model object
 * 
 * Note: this can only return one storage, so model implementations
 * that have multiple &librdf_storage internally may chose not to
 * implement this.
 *
 * Return value:  &librdf_storage or NULL if this has no store
 **/
librdf_storage*
librdf_model_get_storage(librdf_model *model)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);

  if(model->factory->get_storage)
    return model->factory->get_storage(model);
  else
    return NULL;
}


/**
 * librdf_model_find_statements_in_context - search the model for matching statements in a given context
 * @model: &librdf_model object
 * @statement: &librdf_statement partial statement to find
 * @context_node: context &librdf_node (or NULL)
 * 
 * Searches the model for a (partial) statement as described in
 * librdf_statement_match() in the given context and returns a
 * &librdf_stream of matching &librdf_statement objects.  If
 * context is NULL, this is equivalent to librdf_model_find_statements.
 * 
 * Return value: &librdf_stream of matching statements (may be empty) or NULL on failure
 **/
librdf_stream*
librdf_model_find_statements_in_context(librdf_model* model, librdf_statement* statement, librdf_node* context_node) 
{
  librdf_stream *stream;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  if(!librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  if(model->factory->find_statements_in_context)
    return model->factory->find_statements_in_context(model, statement, context_node);

  statement=librdf_new_statement_from_statement(statement);
  if(!statement)
    return NULL;

  stream=librdf_model_context_as_stream(model, context_node);
  if(!stream) {
    librdf_free_statement(statement);
    return NULL;
  }

  librdf_stream_add_map(stream,
                        &librdf_stream_statement_find_map,
                        (librdf_stream_map_free_context_handler)&librdf_free_statement, (void*)statement);

  return stream;
}


/**
 * librdf_model_get_contexts - return the list of contexts in the graph
 * @model: &librdf_model object
 * 
 * Returns an iterator of &librdf_node context nodes for each
 * context in the graph.
 *
 * Return value: &librdf_iterator of context nodes or NULL on failure or if contexts are not supported
 **/
librdf_iterator*
librdf_model_get_contexts(librdf_model* model) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);

  if(!librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  if(model->factory->get_contexts)
    return model->factory->get_contexts(model);
  else
    return NULL;
}


/**
 * librdf_model_get_feature - get the value of a graph feature 
 * @model: &librdf_model object
 * @feature: &librdf_uri feature property
 * 
 * Return value: &librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
librdf_node*
librdf_model_get_feature(librdf_model* model, librdf_uri* feature)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, NULL);

  if(model->factory->get_feature)
    return model->factory->get_feature(model, feature);
  return NULL;
}


/**
 * librdf_model_set_feature - set the value of a graph feature
 * @model: &librdf_model object
 * @feature: &librdf_uri feature property
 * @value: &librdf_node feature property value
 * 
 * Return value: non 0 on failure (negative if no such feature)
 **/
int
librdf_model_set_feature(librdf_model* model, librdf_uri* feature,
                         librdf_node* value)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, -1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, -1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(value, librdf_node, -1);

  if(model->factory->set_feature)
    return model->factory->set_feature(model, feature, value);
  return -1;
}


/**
 * librdf_model_find_statements_with_options - search the model for matching statements with match options
 * @model: &librdf_model object
 * @statement: &librdf_statement partial statement to find
 * @context_node: &librdf_node context node or NULL.
 * @options: &librdf_hash of matching options or NULL
 * 
 * Searches the model for a (partial) statement as described in
 * librdf_statement_match() and returns a &librdf_stream of
 * matching &librdf_statement objects.
 * 
 * If options is given then the match is made according to
 * the given options.  If options is NULL, this is equivalent
 * to librdf_model_find_statements_in_context.
 * 
 * Return value:  &librdf_stream of matching statements (may be empty) or NULL on failure
 **/
librdf_stream*
librdf_model_find_statements_with_options(librdf_model* model,
                                          librdf_statement* statement,
                                          librdf_node* context_node,
                                          librdf_hash* options) 
{
  if(context_node && !librdf_model_supports_contexts(model))
    librdf_log(model->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_MODEL, NULL,
               "Model does not support contexts");

  if(model->factory->find_statements_with_options)
    return model->factory->find_statements_with_options(model, statement, context_node, options);
  else
    return librdf_model_find_statements_in_context(model, statement, context_node);
}


/**
 * librdf_model_load - Load content from a URI into the model
 * @model: &librdf_model object
 * @uri: the URI to read the content
 * @name: the name of the parser (or NULL)
 * @mime_type: the MIME type of the syntax (NULL if not used)
 * @type_uri: URI identifying the syntax (NULL if not used)
 *
 * If the name field is NULL, the library will try to guess
 * the parser to use from the uri, mime_type and type_uri fields.
 * This is done via the raptor_guess_parser_name function.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_model_load(librdf_model* model, librdf_uri *uri,
                  const char *name, const char *mime_type, 
                  librdf_uri *type_uri)
{
  int rc=0;
  librdf_parser* parser;

  if(name && !*name)
    name=NULL;
  if(mime_type && !*mime_type)
    mime_type=NULL;

  if(!name)
    name=raptor_guess_parser_name((raptor_uri*)type_uri, mime_type,
                                  NULL, 0, librdf_uri_as_string(uri));
  parser=librdf_new_parser(model->world, name, NULL, NULL);
  if(!parser)
    return 1;

  rc=librdf_parser_parse_into_model(parser, uri, NULL, model);
  librdf_free_parser(parser);
  return rc;
}


#endif


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);

#define EX1_CONTENT \
"<?xml version=\"1.0\"?>\n" \
"<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n" \
"         xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n" \
"  <rdf:Description rdf:about=\"http://purl.org/net/dajobe/\">\n" \
"    <dc:title>Dave Beckett's Home Page</dc:title>\n" \
"    <dc:creator>Dave Beckett</dc:creator>\n" \
"    <dc:description>The generic home page of Dave Beckett.</dc:description>\n" \
"  </rdf:Description>\n" \
"</rdf:RDF>"

#define EX2_CONTENT \
"<?xml version=\"1.0\"?>\n" \
"<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n" \
"         xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n" \
"  <rdf:Description rdf:about=\"http://purl.org/net/dajobe/\">\n" \
"    <dc:title>Dave Beckett's Home Page</dc:title>\n" \
"    <dc:creator>Dave Beckett</dc:creator>\n" \
"    <dc:description>I do development-based research on RDF, metadata and web searching.</dc:description>\n" \
"    <dc:rights>Copyright &#169; 2002 Dave Beckett</dc:rights>\n" \
"  </rdf:Description>\n" \
"</rdf:RDF>"


int
main(int argc, char *argv[]) 
{
  librdf_storage* storage;
  librdf_model* model;
  librdf_statement *statement;
  librdf_parser* parser;
  librdf_stream* stream;
  const char *parser_name="raptor";
  #define URI_STRING_COUNT 2
  const unsigned char *file_uri_strings[URI_STRING_COUNT]={(const unsigned char*)"http://example.org/test1.rdf", (const unsigned char*)"http://example.org/test2.rdf"};
  const unsigned char *file_content[URI_STRING_COUNT]={(const unsigned char*)EX1_CONTENT, (const unsigned char*)EX2_CONTENT};
  librdf_uri* uris[URI_STRING_COUNT];
  librdf_node* nodes[URI_STRING_COUNT];
  int i;
  char *program=argv[0];
  /* initialise dependent modules - all of them! */
  librdf_world *world=librdf_new_world();
  librdf_iterator* iterator;
  librdf_node *n1, *n2;
  int count;
  int expected_count;

  librdf_world_open(world);
  
  fprintf(stderr, "%s: Creating storage\n", program);
  if(1) {
    /* test contexts in memory */
    storage=librdf_new_storage(world, NULL, NULL, "contexts='yes'");
  } else {
    /* test contexts on disk */
    storage=librdf_new_storage(world, "hashes", "test", "hash-type='bdb',dir='.',write='yes',new='yes',contexts='yes'");
  }
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }
  fprintf(stderr, "%s: Creating model\n", program);
  model=librdf_new_model(world, storage, NULL);
  if(!model) {
    fprintf(stderr, "%s: Failed to create new model\n", program);
    return(1);
  }

  statement=librdf_new_statement(world);
  /* after this, nodes become owned by model */
  librdf_statement_set_subject(statement, librdf_new_node_from_uri_string(world, (const unsigned char*)"http://www.ilrt.bris.ac.uk/people/cmdjb/"));
  librdf_statement_set_predicate(statement, librdf_new_node_from_uri_string(world, (const unsigned char*)"http://purl.org/dc/elements/1.1/creator"));

  if(!librdf_model_add_statement(model, statement)) {
    fprintf(stderr, "%s: librdf_model_add_statement unexpectedly succeeded adding a partial statement\n", program);
    return(1);
  }

  librdf_statement_set_object(statement, librdf_new_node_from_literal(world, (const unsigned char*)"Dave Beckett", NULL, 0));

  librdf_model_add_statement(model, statement);
  librdf_free_statement(statement);

  fprintf(stderr, "%s: Printing model\n", program);
  librdf_model_print(model, stderr);
  
  parser=librdf_new_parser(world, parser_name, NULL, NULL);
  if(!parser) {
    fprintf(stderr, "%s: Failed to create new parser type %s\n", program,
            parser_name);
    exit(1);
  }

  for (i=0; i<URI_STRING_COUNT; i++) {
    uris[i]=librdf_new_uri(world, file_uri_strings[i]);
    nodes[i]=librdf_new_node_from_uri_string(world, file_uri_strings[i]);

    fprintf(stderr, "%s: Adding content from %s into statement context\n", program,
            librdf_uri_as_string(uris[i]));
    if(!(stream=librdf_parser_parse_string_as_stream(parser, 
                                                     file_content[i], uris[i]))) {
      fprintf(stderr, "%s: Failed to parse RDF from %s as stream\n", program,
              librdf_uri_as_string(uris[i]));
      exit(1);
    }
    librdf_model_context_add_statements(model, nodes[i], stream);
    librdf_free_stream(stream);

    fprintf(stderr, "%s: Printing model\n", program);
    librdf_model_print(model, stderr);
  }


  fprintf(stderr, "%s: Freeing Parser\n", program);
  librdf_free_parser(parser);


  /* sync - probably a NOP */
  librdf_model_sync(model);


  /* sources */
  n1=librdf_new_node_from_uri_string(world, (const unsigned char*)"http://purl.org/dc/elements/1.1/creator");
  n2=librdf_new_node_from_literal(world, (const unsigned char*)"Dave Beckett", NULL, 0);

  fprintf(stderr, "%s: Looking for sources of arc=", program);
  librdf_node_print(n1, stderr);
  fputs(" target=", stderr);
  librdf_node_print(n2, stderr);
  fputs("\n", stderr);

  iterator=librdf_model_get_sources(model, n1, n2);
  if(!iterator) {
    fprintf(stderr, "%s: librdf_model_get_sources failed\n", program);
    exit(1);
  }

  expected_count=3;
  for(count=0; !librdf_iterator_end(iterator); librdf_iterator_next(iterator), count++) {
    librdf_node* n=(librdf_node*)librdf_iterator_get_object(iterator);
    fputs("  ", stderr);
    librdf_node_print(n, stderr);
    fputs("\n", stderr);
  }
  librdf_free_iterator(iterator);
  if(count != expected_count) {
    fprintf(stderr, "%s: librdf_model_get_sources returned %d nodes, expected %d\n", program, count, expected_count);
    exit(1);
  }
  librdf_free_node(n1);
  librdf_free_node(n2);
  

  /* targets */
  n1=librdf_new_node_from_uri_string(world, (const unsigned char*)"http://purl.org/net/dajobe/");
  n2=librdf_new_node_from_uri_string(world, (const unsigned char*)"http://purl.org/dc/elements/1.1/description");

  fprintf(stderr, "%s: Looking for targets of source=", program);
  librdf_node_print(n1, stderr);
  fputs(" arc=", stderr);
  librdf_node_print(n2, stderr);
  fputs("\n", stderr);

  iterator=librdf_model_get_targets(model, n1, n2);
  if(!iterator) {
    fprintf(stderr, "%s: librdf_model_get_targets failed\n", program);
    exit(1);
  }

  expected_count=2;
  for(count=0; !librdf_iterator_end(iterator); librdf_iterator_next(iterator), count++) {
    librdf_node* n=(librdf_node*)librdf_iterator_get_object(iterator);
    fputs("  ", stderr);
    librdf_node_print(n, stderr);
    fputs("\n", stderr);
  }
  librdf_free_iterator(iterator);
  if(count != expected_count) {
    fprintf(stderr, "%s: librdf_model_get_targets returned %d nodes, expected %d\n", program, count, expected_count);
    exit(1);
  }
  librdf_free_node(n1);
  librdf_free_node(n2);
  

  /* arcs */
  n1=librdf_new_node_from_uri_string(world, (const unsigned char*)"http://purl.org/net/dajobe/");
  n2=librdf_new_node_from_literal(world, (const unsigned char*)"Dave Beckett", NULL, 0);

  fprintf(stderr, "%s: Looking for arcs of source=", program);
  librdf_node_print(n1, stderr);
  fputs(" target=", stderr);
  librdf_node_print(n2, stderr);
  fputs("\n", stderr);

  iterator=librdf_model_get_arcs(model, n1, n2);
  if(!iterator) {
    fprintf(stderr, "%s: librdf_model_get_arcs failed\n", program);
    exit(1);
  }

  expected_count=2;
  for(count=0; !librdf_iterator_end(iterator); librdf_iterator_next(iterator), count++) {
    librdf_node* n=(librdf_node*)librdf_iterator_get_object(iterator);
    fputs("  ", stderr);
    librdf_node_print(n, stderr);
    fputs("\n", stderr);
  }
  librdf_free_iterator(iterator);
  if(count != expected_count) {
    fprintf(stderr, "%s: librdf_model_get_arcs returned %d nodes, expected %d\n", program, count, expected_count);
    exit(1);
  }
  librdf_free_node(n1);
  librdf_free_node(n2);


  fprintf(stderr, "%s: Listing contexts\n", program);
  iterator=librdf_model_get_contexts(model);
  if(!iterator) {
    fprintf(stderr, "%s: librdf_model_get_contexts failed (optional method)\n", program);
  } else {
    expected_count=2;
    for(count=0; !librdf_iterator_end(iterator); librdf_iterator_next(iterator), count++) {
      librdf_node* n=(librdf_node*)librdf_iterator_get_object(iterator);
      fputs("  ", stderr);
      librdf_node_print(n, stderr);
      fputs("\n", stderr);
    }
    librdf_free_iterator(iterator);
    if(count != expected_count) {
      fprintf(stderr, "%s: librdf_model_get_contexts returned %d context nodes, expected %d\n", program, count, expected_count);
      exit(1);
    }
  }

  

  for (i=0; i<URI_STRING_COUNT; i++) {
    fprintf(stderr, "%s: Removing statement context %s\n", program, 
            librdf_uri_as_string(uris[i]));
    librdf_model_context_remove_statements(model, nodes[i]);

    fprintf(stderr, "%s: Printing model\n", program);
    librdf_model_print(model, stderr);
  }


  fprintf(stderr, "%s: Freeing URIs and Nodes\n", program);
  for (i=0; i<URI_STRING_COUNT; i++) {
    librdf_free_uri(uris[i]);
    librdf_free_node(nodes[i]);
  }
  
  fprintf(stderr, "%s: Freeing model\n", program);
  librdf_free_model(model);

  fprintf(stderr, "%s: Freeing storage\n", program);
  librdf_free_storage(storage);

  librdf_free_world(world);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
