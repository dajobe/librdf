/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query.c - RDF Query Adaptors Implementation
 *
 * $Id$
 *
 * Copyright (C) 2002 David Beckett - http://purl.org/net/dajobe/
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
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <librdf.h>

#include <rdf_query.h>
#include <rdf_query_triples.h>
#include <rdf_uri.h>


/* prototypes for helper functions */
static void librdf_delete_query_factories(void);


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
  /* Always have query triple implementations available */
  librdf_init_query_triples();
}


/**
 * librdf_finish_query - Terminate the librdf_query module
 * @world: redland world object
 **/
void
librdf_finish_query(librdf_world *world) 
{
  librdf_delete_query_factories();
}


/* statics */

/* list of query adaptor factories */
static librdf_query_factory* query_factories;


/* helper functions */


/*
 * librdf_delete_query_factories - helper function to delete all the registered query factories
 */
static void
librdf_delete_query_factories(void)
{
  librdf_query_factory *factory, *next;
  
  for(factory=query_factories; factory; factory=next) {
    next=factory->next;
    LIBRDF_FREE(librdf_query_factory, factory->name);
    if(factory->uri)
      librdf_free_uri(factory->uri);
    LIBRDF_FREE(librdf_query_factory, factory);
  }
  query_factories=NULL;
}


/* class methods */

/**
 * librdf_query_register_factory - Register a query factory
 * @name: the query language name
 * @uri: the query language URI (or NULL if none)
 * @factory: pointer to function to call to register the factory
 * 
 **/
void
librdf_query_register_factory(const char *name,
                              librdf_uri *uri,
                              void (*factory) (librdf_query_factory*)) 
{
  librdf_query_factory *query, *h;
  char *name_copy;
  int name_length;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_query_register_factory,
                "Received registration for query name %s URI %s\n", name, 
                librdf_uri_as_string(uri));
#endif
  
  query=(librdf_query_factory*)LIBRDF_CALLOC(librdf_query_factory, 1,
                                             sizeof(librdf_query_factory));
  if(!query)
    LIBRDF_FATAL1(librdf_query_register_factory, "Out of memory\n");

  name_length=strlen(name);
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, name_length+1, 1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_query, query);
    LIBRDF_FATAL1(librdf_query_register_factory, "Out of memory\n");
  }
  query->name=strcpy(name_copy, name);
  if(uri) {
    query->uri=librdf_new_uri_from_uri(uri);
    if(!query->uri) {
      LIBRDF_FREE(cstring, name_copy); 
      LIBRDF_FREE(librdf_query, query);
      LIBRDF_FATAL1(librdf_query_register_factory, "Out of memory\n");
    }
  }
        
  for(h = query_factories; h; h = h->next ) {
    if(!strcmp(h->name, name_copy)) {
      LIBRDF_FATAL2(librdf_query_register_factory,
                    "query %s already registered\n", h->name);
    }
  }
  
  /* Call the query registration function on the new object */
  (*factory)(query);
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_query_register_factory, "%s has context size %d\n",
                name, query->context_length);
#endif
  
  query->next = query_factories;
  query_factories = query;
}


/**
 * librdf_get_query_factory - Get a query factory by name
 * @name: the factory name or NULL for the default factory
 * @uri: the factory URI or NULL for the default factory
 * 
 * Return value: the factory object or NULL if there is no such factory
 **/
librdf_query_factory*
librdf_get_query_factory (const char *name, librdf_uri *uri) 
{
  librdf_query_factory *factory;

  /* return 1st query if no particular one wanted - why? */
  if(!name && !uri) {
    factory=query_factories;
    if(!factory) {
      LIBRDF_DEBUG1(librdf_get_query_factory, 
                    "No (default) query factories registered\n");
      return NULL;
    }
  } else {
    for(factory=query_factories; factory; factory=factory->next) {
      if(name && !strcmp(factory->name, name)) {
        break;
      }
      if(uri && factory->uri && !librdf_uri_equals(factory->uri, uri)) {
        break;
      }
    }
    /* else FACTORY name not found */
    if(!factory) {
      LIBRDF_DEBUG3(librdf_get_query_factory,
                    "No query factory with name %s uri %s found\n",
                    name, librdf_uri_as_string(uri));
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
                  const char *query_string) {
  librdf_query_factory* factory;

  factory=librdf_get_query_factory(name, uri);
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

  /* FIXME: fail if clone is not supported by this query (factory) */
  if(!old_query->factory->clone) {
    LIBRDF_FATAL2(librdf_new_query_from_query, "clone not implemented for query factory type %s", old_query->factory->name);
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
                               const char *query_string) {
  librdf_query* query;

  if(!factory) {
    LIBRDF_DEBUG1(librdf_new_query, "No factory given\n");
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
  if(query->factory)
    query->factory->terminate(query);

  if(query->context)
    LIBRDF_FREE(librdf_query_context, query->context);
  LIBRDF_FREE(librdf_query, query);
}


/* methods */

/**
 * librdf_query_open - Start a query
 * @query: &librdf_query object
 *
 * This is ended with librdf_query_close()
 * 
 * Return value: non 0 on failure
 **/
int
librdf_query_open(librdf_query* query)
{
  return query->factory->open(query);
}


/**
 * librdf_query_close - End a model / query association
 * @query: &librdf_query object
 * 
 * Return value: non 0 on failure
 **/
int
librdf_query_close(librdf_query* query)
{
  return query->factory->close(query);
}


/**
 * librdf_query_run - Run the query on a model giving matching statements
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
librdf_query_run(librdf_query* query, librdf_model* model)
{
  return query->factory->run(query, model);
}


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_query* query;
  char *program=argv[0];
  librdf_world *world;
  
  world=librdf_new_world();
  
  /* initialise hash, model and query modules */
  librdf_init_hash(world);
  librdf_init_uri(world);
  librdf_init_query(world);
  librdf_init_model(world);
  
  fprintf(stdout, "%s: Creating query\n", program);
  query=librdf_new_query(world, "triples", NULL, "[http://example.org] \"literal\" -");
  if(!query) {
    fprintf(stderr, "%s: Failed to create new query\n", program);
    return(1);
  }

  
  fprintf(stdout, "%s: Opening query\n", program);
  if(librdf_query_open(query)) {
    fprintf(stderr, "%s: Failed to open query\n", program);
    return(1);
  }


  /* Can do nothing here since need model and query working */

  fprintf(stdout, "%s: Closing query\n", program);
  librdf_query_close(query);

  fprintf(stdout, "%s: Freeing query\n", program);
  librdf_free_query(query);
  

  /* finish model and query modules */
  librdf_finish_model(world);
  librdf_finish_query(world);
  librdf_finish_uri(world);
  librdf_finish_hash(world);

  LIBRDF_FREE(librdf_world, world);
  
#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
