/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query.c - RDF Query Adaptors Implementation
 *
 * $Id$
 *
 * Copyright (C) 2002-2004 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.bris.ac.uk/
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
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <librdf.h>
#include <rdf_query.h>


#ifndef STANDALONE

/* prototypes for helper functions */
static void librdf_delete_query_factories(librdf_world *world);


/**
 * librdf_init_query - Initialise the librdf_query module
 * @world: redland world object
 * 
 * Initialises and registers all
 * compiled query modules.  Must be called before using any of the query
 * factory functions such as librdf_get_query_factory()
 **/
void
librdf_init_query(librdf_world *world) 
{
  /* Always have query triple, rasqal implementations available */
  librdf_query_triples_constructor(world);
  librdf_query_rasqal_constructor(world);
}


/**
 * librdf_finish_query - Terminate the librdf_query module
 * @world: redland world object
 **/
void
librdf_finish_query(librdf_world *world) 
{
  librdf_query_rasqal_destructor();
  librdf_delete_query_factories(world);
}



/* helper functions */


/*
 * librdf_delete_query_factories - helper function to delete all the registered query factories
 */
static void
librdf_delete_query_factories(librdf_world *world)
{
  librdf_query_factory *factory, *next;
  
  for(factory=world->query_factories; factory; factory=next) {
    next=factory->next;
    LIBRDF_FREE(librdf_query_factory, factory->name);
    if(factory->uri)
      librdf_free_uri(factory->uri);
    LIBRDF_FREE(librdf_query_factory, factory);
  }
  world->query_factories=NULL;
}


/* class methods */

/**
 * librdf_query_register_factory - Register a query factory
 * @world: redland world object
 * @name: the query language name
 * @uri: the query language URI (or NULL if none)
 * @factory: pointer to function to call to register the factory
 * 
 **/
void
librdf_query_register_factory(librdf_world *world, const char *name,
                              librdf_uri *uri,
                              void (*factory) (librdf_query_factory*)) 
{
  librdf_query_factory *query, *h;
  char *name_copy;
  int name_length;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3("Received registration for query name %s URI %s\n", name, librdf_uri_as_string(uri));
#endif
  
  query=(librdf_query_factory*)LIBRDF_CALLOC(librdf_query_factory, 1,
                                             sizeof(librdf_query_factory));
  if(!query)
    LIBRDF_FATAL1(world, LIBRDF_FROM_QUERY, "Out of memory");

  name_length=strlen(name);
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, name_length+1, 1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_query, query);
    LIBRDF_FATAL1(world, LIBRDF_FROM_QUERY, "Out of memory");
  }
  query->name=strcpy(name_copy, name);
  if(uri) {
    query->uri=librdf_new_uri_from_uri(uri);
    if(!query->uri) {
      LIBRDF_FREE(cstring, name_copy); 
      LIBRDF_FREE(librdf_query, query);
      LIBRDF_FATAL1(world, LIBRDF_FROM_QUERY, "Out of memory");
    }
  }
        
  for(h = world->query_factories; h; h = h->next ) {
    if(!strcmp(h->name, name_copy)) {
      LIBRDF_FREE(cstring, name_copy); 
      LIBRDF_FREE(librdf_query, query);
      librdf_log(world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL,
                 "query language %s already registered\n", h->name);
      return;
    }
  }
  
  /* Call the query registration function on the new object */
  (*factory)(query);
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3("%s has context size %d\n", name, query->context_length);
#endif
  
  query->next = world->query_factories;
  world->query_factories = query;
}


/**
 * librdf_get_query_factory - Get a query factory by name
 * @world: redland world object
 * @name: the factory name or NULL for the default factory
 * @uri: the factory URI or NULL for the default factory
 * 
 * Return value: the factory object or NULL if there is no such factory
 **/
librdf_query_factory*
librdf_get_query_factory(librdf_world *world, 
                         const char *name, librdf_uri *uri) 
{
  librdf_query_factory *factory;

  /* return 1st query if no particular one wanted - why? */
  if(!name && !uri) {
    factory=world->query_factories;
    if(!factory) {
      LIBRDF_DEBUG1("No (default) query factories registered\n");
      return NULL;
    }
  } else {
    for(factory=world->query_factories; factory; factory=factory->next) {
      if(name && !strcmp(factory->name, name)) {
        break;
      }
      if(uri && factory->uri && !librdf_uri_equals(factory->uri, uri)) {
        break;
      }
    }
    /* else FACTORY name not found */
    if(!factory) {
      LIBRDF_DEBUG3("No query language with name '%s' uri %s found\n", 
                    name, (uri ? (char*)librdf_uri_as_string(uri) : "NULL"));
      return NULL;
    }
  }
        
  return factory;
}



/**
 * librdf_new_query - Constructor - create a new librdf_query object
 * @world: redland world object
 * @name: the query language name
 * @uri: the query language URI (or NULL)
 * @query_string: the query string
 *
 * Return value: a new &librdf_query object or NULL on failure
 */
librdf_query*
librdf_new_query (librdf_world *world,
                  const char *name, librdf_uri *uri,
                  const unsigned char *query_string) {
  librdf_query_factory* factory;

  factory=librdf_get_query_factory(world, name, uri);
  if(!factory)
    return NULL;

  return librdf_new_query_from_factory(world, factory, name, uri, query_string);
}


/**
 * librdf_new_query_from_query - Copy constructor - create a new librdf_query object from an existing one
 * @old_query: the existing query &librdf_query to use
 *
 * Should create a new query in the same context as the existing one
 * as appropriate.
 *
 * Return value: a new &librdf_query object or NULL on failure
 */
librdf_query*
librdf_new_query_from_query(librdf_query* old_query) 
{
  librdf_query* new_query;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(old_query, librdf_query, NULL);

  /* FIXME: fail if clone is not supported by this query (factory) */
  if(!old_query->factory->clone) {
    LIBRDF_FATAL1(old_query->world, LIBRDF_FROM_QUERY, "clone not implemented for query factory");
    return NULL;
  }

  new_query=(librdf_query*)LIBRDF_CALLOC(librdf_query, 1, 
                                         sizeof(librdf_query));
  if(!new_query)
    return NULL;
  
  new_query->context=(char*)LIBRDF_CALLOC(librdf_query_context, 1,
                                          old_query->factory->context_length);
  if(!new_query->context) {
    librdf_free_query(new_query);
    return NULL;
  }

  new_query->world=old_query->world;

  /* do this now so librdf_free_query won't call new factory on
   * partially copied query
   */
  new_query->factory=old_query->factory;

  /* clone is assumed to do leave the new query in the same state
   * after an init() method on an existing query - i.e ready to use
   */
  if(old_query->factory->clone(new_query, old_query)) {
    librdf_free_query(new_query);
    return NULL;
  }

  return new_query;
}


/**
 * librdf_new_query_from_factory - Constructor - create a new librdf_query object
 * @world: redland world object
 * @factory: the factory to use to construct the query
 * @name: query language name
 * @uri: query language URI (or NULL)
 * @query_string: the query string
 *
 * Return value: a new &librdf_query object or NULL on failure
 */
librdf_query*
librdf_new_query_from_factory (librdf_world *world,
                               librdf_query_factory* factory,
                               const char *name, librdf_uri *uri,
                               const unsigned char *query_string) {
  librdf_query* query;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(factory, librdf_query_factory, NULL);

  if(!factory) {
    LIBRDF_DEBUG1("No query factory given\n");
    return NULL;
  }
  
  query=(librdf_query*)LIBRDF_CALLOC(librdf_query, 1, sizeof(librdf_query));
  if(!query)
    return NULL;

  query->world=world;

  query->context=(char*)LIBRDF_CALLOC(librdf_query_context, 1,
                                      factory->context_length);
  if(!query->context) {
    librdf_free_query(query);
    return NULL;
  }
  
  query->factory=factory;
  
  if(factory->init(query, name, uri, query_string)) {
    librdf_free_query(query);
    return NULL;
  }
  
  return query;
}


/**
 * librdf_free_query - Destructor - destroy a librdf_query object
 * @query: &librdf_query object
 * 
 **/
void
librdf_free_query (librdf_query* query) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(query, librdf_query);

  if(query->factory)
    query->factory->terminate(query);

  if(query->context)
    LIBRDF_FREE(librdf_query_context, query->context);
  LIBRDF_FREE(librdf_query, query);
}


/* methods */


/**
 * librdf_query_run_as_stream - Run the query on a model giving matching statements
 * @query: &librdf_query object
 * @model: model to operate query on
 * 
 * Runs the query against the (previously registered) model
 * and returns a &librdf_stream of
 * matching &librdf_statement objects.
 * 
 * Return value:  &librdf_stream of matching statements (may be empty) or NULL on failure
 **/
librdf_stream*
librdf_query_run_as_stream(librdf_query* query, librdf_model* model)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_model, NULL);

  if(query->factory->run_as_stream)
    return query->factory->run_as_stream(query, model);
  else
    return NULL;
}


/**
 * librdf_query_run_as_bindings - Run the query on a model giving variable bindings
 * @query: &librdf_query object
 * @model: model to operate query on
 * 
 * Runs the query against the (previously registered) model
 * 
 * Return value:  non-0 on failure
 **/
int
librdf_query_run_as_bindings(librdf_query* query, librdf_model* model)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_model, 1);

  if(query->factory->run_as_bindings)
    return query->factory->run_as_bindings(query, model);
  else
    return 1;
}


/**
 * librdf_query_get_result_count - Get number of bindings so far
 * @query: &librdf_query query
 * 
 * Return value: number of bindings found so far
 **/
int
librdf_query_get_result_count(librdf_query *query)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, 1);

  if(query->factory->get_result_count)
    return query->factory->get_result_count(query);
  else
    return 1;
}


/**
 * librdf_query_results_finished - Find out if binding results are exhausted
 * @query: &librdf_query query
 * 
 * Return value: non-0 if results are finished or query failed
 **/
int
librdf_query_results_finished(librdf_query *query)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, 1);

  if(query->factory->results_finished)
    return query->factory->results_finished(query);
  else
    return 1;
}


/**
 * librdf_query_get_result_bindings - Get all binding names, values for current result
 * @query: &librdf_query query
 * @names: pointer to an array of binding names (or NULL)
 * @values: pointer to an array of binding value &librdf_node (or NULL)
 * 
 * If names is not NULL, it is set to the address of a shared array
 * of names of the bindings (an output parameter).  These names
 * are shared and must not be freed by the caller
 *
 * If values is not NULL, it is used as an array to store pointers
 * to the librdf_node* of the results.  These nodes must be freed
 * by the caller.  The size of the array is determined by the
 * number of names of bindings, returned by
 * librdf_query_get_bindings_count dynamically or
 * will be known in advanced if hard-coded into the query string.
 * 
 * Return value: non-0 if the assignment failed
 **/
int
librdf_query_get_result_bindings(librdf_query *query, 
                                 const char ***names, librdf_node **values)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, 1);

  if(query->factory->get_result_bindings)
    return query->factory->get_result_bindings(query, names, values);
  else
    return 1;
}


/**
 * librdf_query_get_result_binding_value - Get one binding value for the current result
 * @query: &librdf_query query
 * @offset: offset of binding name into array of known names
 * 
 * Return value: a new &librdf_node binding value or NULL on failure
 **/
librdf_node*
librdf_query_get_result_binding_value(librdf_query *query, int offset)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, NULL);

  if(query->factory->get_result_binding_value)
    return query->factory->get_result_binding_value(query, offset);
  else
    return NULL;
}


/**
 * librdf_query_get_result_binding_name - Get binding name for the current result
 * @query: &librdf_query query
 * @offset: offset of binding name into array of known names
 * 
 * Return value: a pointer to a shared copy of the binding name or NULL on failure
 **/
const char*
librdf_query_get_result_binding_name(librdf_query *query, int offset)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, NULL);

  if(query->factory->get_result_binding_name)
    return query->factory->get_result_binding_name(query, offset);
  else
    return NULL;
}


/**
 * librdf_query_get_result_binding_value_by_name - Get one binding value for a given name in the current result
 * @query: &librdf_query query
 * @name: variable name
 * 
 * Return value: a new &librdf_node binding value or NULL on failure
 **/
librdf_node*
librdf_query_get_result_binding_value_by_name(librdf_query *query, const char *name)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, NULL);

  if(query->factory->get_result_binding_value_by_name)
    return query->factory->get_result_binding_value_by_name(query, name);
  else
    return NULL;
}


/**
 * librdf_query_next_result - Move to the next result
 * @query: &librdf_query query
 * 
 * Return value: non-0 if failed or results exhausted
 **/
int
librdf_query_next_result(librdf_query *query)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, 1);

  if(query->factory->next_result)
    return query->factory->next_result(query);
  else
    return 1;
}


/**
 * librdf_query_get_bindings_count - Get the number of bound variables in the result
 * @query: &librdf_query query
 * 
 * Return value: <0 if failed or results exhausted
 **/
int
librdf_query_get_bindings_count(librdf_query *query)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, 1);

  if(query->factory->get_bindings_count)
    return query->factory->get_bindings_count(query);
  else
    return -1;
}

#endif



/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


#define DATA "@prefix ex: <http://example.org/> .\
ex:fido a ex:Dog ;\
        ex:label \"Fido\" .\
"
#define DATA_LANGUAGE "turtle"
#define DATA_BASE_URI "http://example.org/"
#define QUERY_STRING "select ?x where (?x rdf:type ?y)";
#define QUERY_LANGUAGE "rdql"
#define VARIABLES_COUNT 1

int
main(int argc, char *argv[]) 
{
  librdf_query* query;
  librdf_model* model;
  librdf_storage* storage;
  librdf_parser* parser;
  librdf_uri *uri;
  char *program=argv[0];
  librdf_world *world;

  char *query_string=QUERY_STRING;

  world=librdf_new_world();
  librdf_world_open(world);

  /* create model and storage */
  storage=librdf_new_storage(world, NULL, NULL, NULL);
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

  /* read the example data in */
  uri=librdf_new_uri(world, DATA_BASE_URI);
  parser=librdf_new_parser(world, DATA_LANGUAGE, NULL, NULL);
  librdf_parser_parse_string_into_model(parser, DATA, uri, model);
  librdf_free_parser(parser);
  librdf_free_uri(uri);


  fprintf(stdout, "%s: Creating query\n", program);
  query=librdf_new_query(world, QUERY_LANGUAGE, NULL, query_string);
  if(!query) {
    fprintf(stderr, "%s: Failed to create new query\n", program);
    return(1);
  }

  /* do the query */
  if(librdf_model_query_as_bindings(model, query)) {
    fprintf(stderr, "%s: Query of model with '%s' failed\n", 
            program, query_string);
    return 1;
  }

  /* print the results */
  while(!librdf_query_results_finished(query)) {
    const char **names=NULL;
    librdf_node* values[VARIABLES_COUNT];
    
    if(librdf_query_get_result_bindings(query, &names, values))
      break;
    
    fputs("result: [", stdout);
    if(names) {
      int i;
      
      for(i=0; names[i]; i++) {
        fprintf(stdout, "%s=", names[i]);
        if(values[i]) {
          librdf_node_print(values[i], stdout);
          librdf_free_node(values[i]);
        } else
          fputs("NULL", stdout);
        if(names[i+1])
          fputs(", ", stdout);
      }
    }
    fputs("]\n", stdout);
    
    librdf_query_next_result(query);
  }
  
  fprintf(stdout, "%s: Query returned %d results\n", program, 
          librdf_query_get_result_count(query));

  fprintf(stdout, "%s: Freeing query\n", program);
  librdf_free_query(query);

  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_free_world(world);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
