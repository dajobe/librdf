/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage.c - RDF Storage Implementation
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
#include <ctype.h>
#include <sys/types.h>

#define LIBRDF_INTERNAL 1
#include <librdf.h>

#include <rdf_storage.h>
#include <rdf_storage_list.h>


/* prototypes for helper functions */
static void librdf_delete_storage_factories(void);


/**
 * librdf_init_storage - Initialise the librdf_storage module
 * 
 * Initialises and registers all
 * compiled storage modules.  Must be called before using any of the storage
 * factory functions such as librdf_get_storage_factory()
 **/
void
librdf_init_storage(void) 
{
  /* Always have storage list implementation available */
  librdf_init_storage_list();
}


/**
 * librdf_finish_storage - Terminate the librdf_storage module
 **/
void
librdf_finish_storage(void) 
{
  librdf_delete_storage_factories();
}


/* statics */

/* list of storage factories */
static librdf_storage_factory* storages;


/* helper functions */


/*
 * librdf_delete_storage_factories - helper function to delete all the registered storage factories
 */
static void
librdf_delete_storage_factories(void)
{
  librdf_storage_factory *factory, *next;
  
  for(factory=storages; factory; factory=next) {
    next=factory->next;
    LIBRDF_FREE(librdf_storage_factory, factory->name);
    LIBRDF_FREE(librdf_storage_factory, factory);
  }
}


/* class methods */

/**
 * librdf_storage_register_factory - Register a storage factory
 * @name: the storage factory name
 * @factory: pointer to function to call to register the factory
 * 
 **/
void
librdf_storage_register_factory(const char *name,
				void (*factory) (librdf_storage_factory*)) 
{
  librdf_storage_factory *storage, *h;
  char *name_copy;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_storage_register_factory,
                "Received registration for storage %s\n", name);
#endif
  
  storage=(librdf_storage_factory*)LIBRDF_CALLOC(librdf_storage_factory, 1,
                                                 sizeof(librdf_storage_factory));
  if(!storage)
    LIBRDF_FATAL1(librdf_storage_register_factory, "Out of memory\n");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, strlen(name)+1, 1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_storage, storage);
    LIBRDF_FATAL1(librdf_storage_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  storage->name=name_copy;
        
  for(h = storages; h; h = h->next ) {
    if(!strcmp(h->name, name_copy)) {
      LIBRDF_FATAL2(librdf_storage_register_factory,
                    "storage %s already registered\n", h->name);
    }
  }
  
  /* Call the storage registration function on the new object */
  (*factory)(storage);
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_storage_register_factory, "%s has context size %d\n",
                name, storage->context_length);
#endif
  
  storage->next = storages;
  storages = storage;
}


/**
 * librdf_get_storage_factory - Get a storage factory by name
 * @name: the factory name or NULL for the default factory
 * 
 * Return value: the factory object or NULL if there is no such factory
 **/
librdf_storage_factory*
librdf_get_storage_factory (const char *name) 
{
  librdf_storage_factory *factory;

  /* return 1st storage if no particular one wanted - why? */
  if(!name) {
    factory=storages;
    if(!factory) {
      LIBRDF_DEBUG1(librdf_get_storage_factory, 
                    "No (default) storages registered\n");
      return NULL;
    }
  } else {
    for(factory=storages; factory; factory=factory->next) {
      if(!strcmp(factory->name, name)) {
        break;
      }
    }
    /* else FACTORY name not found */
    if(!factory) {
      LIBRDF_DEBUG2(librdf_get_storage_factory,
                    "No storage with name %s found\n",
                    name);
      return NULL;
    }
  }
        
  return factory;
}



/**
 * librdf_new_storage - Constructor - create a new librdf_storage object
 * @name: the factory name
 * @options_string: options to initialise storage
 *
 * The options are encoded as described in librdf_hash_from_string()
 * and can be NULL if none are required.
 *
 * Return value: a new &librdf_storage object or NULL on failure
 */
librdf_storage*
librdf_new_storage (char *name, char *options_string) {
  librdf_storage_factory* factory;
  librdf_hash* options_hash;
  librdf_storage *storage;
  
  factory=librdf_get_storage_factory(name);
  if(!factory)
    return NULL;

  options_hash=librdf_new_hash(NULL);
  if(!options_hash)
    return NULL;
  
  if(librdf_hash_from_string(options_hash, options_string)) {
    librdf_free_hash(options_hash);
    return NULL;
  }

  storage=librdf_new_storage_from_factory(factory, options_hash);
  librdf_free_hash(options_hash);
  return storage;
}


/**
 * librdf_new_storage_from_factory - Constructor - create a new librdf_storage object
 * @factory: the factory to use to construct the storage
 * @options: &librdf_hash of options to initialise storage
 *
 * Return value: a new &librdf_storage object or NULL on failure
 */
librdf_storage*
librdf_new_storage_from_factory (librdf_storage_factory* factory,
                                 librdf_hash* options) {
  librdf_storage* storage;

  if(!factory) {
    LIBRDF_DEBUG1(librdf_new_storage, "No factory given\n");
    return NULL;
  }
  
  storage=(librdf_storage*)LIBRDF_CALLOC(librdf_storage, 1,
                                         sizeof(librdf_storage));
  if(!storage)
    return NULL;
  
  storage->context=(char*)LIBRDF_CALLOC(librdf_storage_context, 1,
                                        factory->context_length);
  if(!storage->context) {
    librdf_free_storage(storage);
    return NULL;
  }
  
  storage->factory=factory;
  
  if(factory->init(storage, options)) {
    librdf_free_storage (storage);
    return NULL;
  }
  
  return storage;
}


/**
 * librdf_free_storage - Destructor - destroy a librdf_storage object
 * @storage: &librdf_storage object
 * 
 **/
void
librdf_free_storage (librdf_storage* storage) 
{
  if(storage->context)
    LIBRDF_FREE(librdf_storage_context, storage->context);
  LIBRDF_FREE(librdf_storage, storage);
}


/* methods */

/**
 * librdf_storage_open - Start a model / storage association
 * @storage: &librdf_storage object
 * @model: model stored
 *
 * This is ended with librdf_storage_close()
 * 
 * Return value: non 0 on failure
 **/
int
librdf_storage_open(librdf_storage* storage, librdf_model* model) 
{
  return storage->factory->open(storage, model);
}


/**
 * librdf_storage_close - End a model / storage association
 * @storage: &librdf_storage object
 * 
 * Return value: non 0 on failure
 **/
int
librdf_storage_close(librdf_storage* storage)
{
  return storage->factory->close(storage);
}


/**
 * librdf_storage_size - Get the number of statements stored
 * @storage: &librdf_storage object
 * 
 * Return value: The number of statements
 **/
int
librdf_storage_size(librdf_storage* storage) 
{
  return storage->factory->size(storage);
}


/**
 * librdf_storage_add_statement - Add a statement to a storage
 * @storage: &librdf_storage object
 * @statement: &librdf_statement statement to add
 * 
 * Return value: non 0 on failure
 **/
int
librdf_storage_add_statement(librdf_storage* storage,
                             librdf_statement* statement) 
{
  return storage->factory->add_statement(storage, statement);
}


/**
 * librdf_storage_add_statements - Add a stream of statements to the storage
 * @storage: &librdf_storage object
 * @statement_stream: &librdf_stream of statements
 * 
 * Return value: non 0 on failure
 **/
int
librdf_storage_add_statements(librdf_storage* storage,
                              librdf_stream* statement_stream) 
{
  return storage->factory->add_statements(storage, statement_stream);
}


/**
 * librdf_storage_remove_statement - Remove a statement from the storage
 * @storage: &librdf_storage object
 * @statement: &librdf_statement statement to remove
 * 
 * Return value: non 0 on failure
 **/
int
librdf_storage_remove_statement(librdf_storage* storage, 
                                librdf_statement* statement) 
{
  return storage->factory->remove_statement(storage, statement);
}


/**
 * librdf_storage_contains_statement - Test if a given statement is present in the storage
 * @storage: &librdf_storage object
 * @statement: &librdf_statement statement to check
 *
 * Return value: non 0 if the storage contains the statement
 **/
int
librdf_storage_contains_statement(librdf_storage* storage,
                                  librdf_statement* statement) 
{
  return storage->factory->contains_statement(storage, statement);
}


/**
 * librdf_storage_serialise - Serialise the storage as a librdf_stream of statemetns
 * @storage: &librdf_storage object
 * 
 * Return value: &librdf_stream of statements or NULL on failure
 **/
librdf_stream*
librdf_storage_serialise(librdf_storage* storage) 
{
  return storage->factory->serialise(storage);
}


/**
 * librdf_storage_find_statements - search the storage for matching statements
 * @storage: &librdf_storage object
 * @statement: &librdf_statement partial statement to find
 * 
 * Searches the storage for a (partial) statement - see librdf_statement_match() - and return a stream matching &librdf_statement.
 * 
 * Return value:  &librdf_stream of matching statements (may be empty) or NULL on failure
 **/
librdf_stream*
librdf_storage_find_statements(librdf_storage* storage,
                               librdf_statement* statement) 
{
  return storage->factory->find_statements(storage, statement);
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_storage* storage;
  char *program=argv[0];
  
  /* initialise hash, model and storage modules */
  librdf_init_hash();
  librdf_init_storage();
  librdf_init_model();
  
  fprintf(stderr, "%s: Creating storage\n", program);
  storage=librdf_new_storage(NULL, NULL);
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }

  
  fprintf(stderr, "%s: Opening storage\n", program);
  if(librdf_storage_open(storage, NULL)) {
    fprintf(stderr, "%s: Failed to open storage\n", program);
    return(1);
  }


  /* Can do nothing here since need model and storage working */

  fprintf(stderr, "%s: Closing storage\n", program);
  librdf_storage_close(storage);

  fprintf(stderr, "%s: Freeing storage\n", program);
  librdf_free_storage(storage);
  

  /* finish model and storage modules */
  librdf_finish_model();
  librdf_finish_storage();
  librdf_finish_hash();
  
  
#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
