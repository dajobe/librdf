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
 * librdf_init_storage:
 * 
 * Initialise the rdf_storage module - initialises and registers all
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
 * librdf_finish_storage:
 * 
 * Close down the rdf_storage module
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


/**
 * librdf_delete_storage_factories:
 * 
 * Delete all the registered storage factories
 **/
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
 * librdf_storage_register_factory:
 * @name: the storage factory name
 * @factory: pointer to function to call to register the factory
 * 
 * Register a storage factory
 **/
void
librdf_storage_register_factory(const char *name,
				void (*factory) (librdf_storage_factory*)) 
{
  librdf_storage_factory *storage, *h;
  char *name_copy;
  
  LIBRDF_DEBUG2(librdf_storage_register_factory,
                "Received registration for storage %s\n", name);
  
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
  
  LIBRDF_DEBUG3(librdf_storage_register_factory, "%s has context size %d\n",
                name, storage->context_length);
  
  storage->next = storages;
  storages = storage;
}


/**
 * librdf_get_storage_factory:
 * @name: the factory name or NULL for the default factory
 * 
 * Get a storage factory for a given storage name
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
 * librdf_new_storage:
 * @factory: the factory to use to construct the storage
 *
 * Constructor: create a new &librdf_storage object
 *
 * Return value: a new &librdf_storage object or NULL on failure
 */
librdf_storage*
librdf_new_storage (librdf_storage_factory* factory,
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
 * librdf_free_storage:
 * @storage: &librdf_storage object
 * 
 * Destructor: destroy &librdf_storage object
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
 * librdf_storage_open:
 * @storage: &librdf_storage object
 * @model: model stored
 *
 * Start storage association, ended with librdf_storage_close()
 * 
 * Return value: non 0 on failure
 **/
int
librdf_storage_open(librdf_storage* storage, librdf_model* model) 
{
  return storage->factory->open(storage, model);
}


/**
 * librdf_storage_close:
 * @storage: &librdf_storage object
 * 
 * End storage association
 * 
 * Return value: non 0 on failure
 **/
int
librdf_storage_close(librdf_storage* storage)
{
  return storage->factory->close(storage);
}


/**
 * librdf_storage_size:
 * @storage: &librdf_storage object
 * 
 * Get the number of statements in the storage.
 * 
 * Return value: The number of statements
 **/
int
librdf_storage_size(librdf_storage* storage) 
{
  return storage->factory->size(storage);
}


/**
 * librdf_storage_add_statement:
 * @storage: &librdf_storage object
 * @statement: &librdf_statement statement to add
 * 
 * Add a statement to the storage.
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
 * librdf_storage_add_statements:
 * @storage: &librdf_storage object
 * @statement_stream: &librdf_stream of statements
 * 
 * Add a stream of statements to the storage.
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
 * librdf_storage_remove_statement:
 * @storage: &librdf_storage object
 * @statement: &librdf_statement statement to remove
 * 
 * Remove a statement from the storage.
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
 * librdf_storage_contains_statement:
 * @storage: &librdf_storage object
 * @statement: &librdf_statement statement to check
 * 
 * Test if a given statement is present in the storage.
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
 * librdf_storage_serialise:
 * @storage: &librdf_storage object
 * 
 * Serialise the storage as a stream of &librdf_statements.
 * 
 * Return value: &librdf_stream of statements or NULL on failure
 **/
librdf_stream*
librdf_storage_serialise(librdf_storage* storage) 
{
  return storage->factory->serialise(storage);
}


/**
 * librdf_storage_find_statements:
 * @storage: &librdf_storage object
 * @statement: &librdf_statement partial statement to find
 * 
 * Search the storage for a (partial) statement and return a stream
 * of matching &librdf_statement.
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
  librdf_storage_factory* factory;
  librdf_storage* storage;
  char *program=argv[0];
  
  /* initialise model and storage modules */
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
  
  
#ifdef LIBRDF_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
