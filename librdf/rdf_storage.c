/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage.c - RDF Storage Implementation
 *
 * $Id$
 *
 * Copyright (C) 2000-2004 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.bristol.ac.uk/
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
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <librdf.h>
#include <rdf_storage.h>
#include <rdf_storage_hashes.h>
#include <rdf_storage_list.h>


#ifndef STANDALONE

/* prototypes for helper functions */
static void librdf_delete_storage_factories(void);


/* prototypes for functions implementing get_sources, arcs, targets
 * librdf_iterator via conversion from a librdf_stream of librdf_statement
 */
static int librdf_storage_stream_to_node_iterator_is_end(void* iterator);
static int librdf_storage_stream_to_node_iterator_next_method(void* iterator);
static void* librdf_storage_stream_to_node_iterator_get_method(void* iterator, int flags);
static void librdf_storage_stream_to_node_iterator_finished(void* iterator);

/* helper function for creating iterators for get sources, targets, arcs */
static librdf_iterator* librdf_storage_node_stream_to_node_create(librdf_storage* storage, librdf_node* node1, librdf_node *node2, librdf_statement_part want);


/**
 * librdf_init_storage - Initialise the librdf_storage module
 * @world: redland world object
 * 
 * Initialises and registers all
 * compiled storage modules.  Must be called before using any of the storage
 * factory functions such as librdf_get_storage_factory()
 **/
void
librdf_init_storage(librdf_world *world)
{
#ifdef HAVE_MYSQL
  librdf_init_storage_mysql(world);
#endif
#ifdef HAVE_TSTORE
  librdf_init_storage_tstore(world);
#endif
  /* Always have storage list, hashes, file implementations available */
  librdf_init_storage_file(world);
  librdf_init_storage_hashes(world);
  librdf_init_storage_list(world);
}


/**
 * librdf_finish_storage - Terminate the librdf_storage module
 * @world: redland world object
 **/
void
librdf_finish_storage(librdf_world *world) 
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
    LIBRDF_FREE(librdf_storage_factory, factory->label);
    LIBRDF_FREE(librdf_storage_factory, factory);
  }
  storages=NULL;
}


/* class methods */

/**
 * librdf_storage_register_factory - Register a storage factory
 * @world: redland world object
 * @name: the storage factory name
 * @label: the storage factory label
 * @factory: pointer to function to call to register the factory
 * 
 **/
void
librdf_storage_register_factory(librdf_world* world,
                                const char *name, const char *label,
				void (*factory) (librdf_storage_factory*)) 
{
  librdf_storage_factory *storage, *h;
  char *name_copy;
  char *label_copy;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2("Received registration for storage %s\n", name);
#endif
  
  storage=(librdf_storage_factory*)LIBRDF_CALLOC(librdf_storage_factory, 1,
                                                 sizeof(librdf_storage_factory));
  if(!storage)
    LIBRDF_FATAL1(world, LIBRDF_FROM_STORAGE, "Out of memory");

  name_copy=(char*)LIBRDF_CALLOC(cstring, strlen(name)+1, 1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_storage, storage);
    LIBRDF_FATAL1(world, LIBRDF_FROM_STORAGE, "Out of memory");
  }
  strcpy(name_copy, name);
  storage->name=name_copy;
        
  for(h = storages; h; h = h->next ) {
    if(!strcmp(h->name, name_copy)) {
      LIBRDF_FREE(cstring, name_copy); 
      LIBRDF_FREE(librdf_storage, storage);
      librdf_log(world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "storage %s already registered\n", h->name);
      return;
    }
  }
  
  label_copy=(char*)LIBRDF_CALLOC(cstring, strlen(label)+1, 1);
  if(!label_copy) {
    LIBRDF_FREE(librdf_storage, storage);
    LIBRDF_FATAL1(world, LIBRDF_FROM_STORAGE, "Out of memory");
  }
  strcpy(label_copy, label);
  storage->label=label_copy;
        
  /* Call the storage registration function on the new object */
  (*factory)(storage);
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3("%s has context size %d\n", name, storage->context_length);
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
      LIBRDF_DEBUG1("No (default) storages registered\n");
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
      LIBRDF_DEBUG2("No storage with name %s found\n", name);
      return NULL;
    }
  }
        
  return factory;
}


/**
 * librdf_storage_enumerate - Get information on storages
 * @counter: index into the list of storages
 * @name: pointer to store the name of the storage (or NULL)
 * @label: pointer to store syntax readable label (or NULL)
 * 
 * Return value: non 0 on failure of if counter is out of range
 **/
int
librdf_storage_enumerate(const unsigned int counter,
                         const char **name, const char **label)
{
  unsigned int i;
  librdf_storage_factory *factory=storages;

  if(!factory || counter < 0)
    return 1;

  for(i=0; factory && i<=counter ; i++, factory=factory->next) {
    if(i == counter) {
      if(name)
        *name=factory->name;
      if(label)
        *label=factory->label;
      return 0;
    }
  }
        
  return 1;
}


/**
 * librdf_new_storage - Constructor - create a new librdf_storage object
 * @world: redland world object
 * @storage_name: the storage factory name
 * @name: an identifier for the storage
 * @options_string: options to initialise storage
 *
 * The options are encoded as described in librdf_hash_from_string()
 * and can be NULL if none are required.
 *
 * Return value: a new &librdf_storage object or NULL on failure
 */
librdf_storage*
librdf_new_storage (librdf_world *world, 
                    char *storage_name, char *name, 
                    char *options_string) {
  librdf_storage_factory* factory;
  librdf_hash* options_hash;
  
  factory=librdf_get_storage_factory(storage_name);
  if(!factory)
    return NULL;

  options_hash=librdf_new_hash(world, NULL);
  if(!options_hash)
    return NULL;

  if(librdf_hash_open(options_hash, NULL, 0, 1, 1, NULL)) {
    librdf_free_hash(options_hash);
    return NULL;
  }
  
  if(librdf_hash_from_string(options_hash, options_string)) {
    librdf_free_hash(options_hash);
    return NULL;
  }

  return librdf_new_storage_from_factory(world, factory, name, options_hash);
}


/**
 * librdf_new_storage_with_options - Constructor - create a new librdf_storage object
 * @world: redland world object
 * @storage_name: the storage factory name
 * @name: an identifier for the storage
 * @options: &librdf_hash of options to use
 *
 * The options can be NULL if none are required.
 *
 * Return value: a new &librdf_storage object or NULL on failure
 */
librdf_storage*
librdf_new_storage_with_options (librdf_world *world, 
                                 char *storage_name, char *name, 
                                 librdf_hash *options) {
  librdf_storage_factory* factory;
  librdf_hash* options_hash;
  
  factory=librdf_get_storage_factory(storage_name);
  if(!factory)
    return NULL;

  options_hash=librdf_new_hash_from_hash(options);
  if(!options_hash)
    return NULL;

  if(librdf_hash_open(options_hash, NULL, 0, 1, 1, NULL)) {
    librdf_free_hash(options_hash);
    return NULL;
  }
  
  return librdf_new_storage_from_factory(world, factory, name, options_hash);
}


/**
 * librdf_new_storage_from_storage - Copy constructor - create a new librdf_storage object from an existing one
 * @old_storage: the existing storage &librdf_storage to use
 *
 * Should create a new storage in the same context as the existing one
 * as appropriate for the storage.  For example, in a RDBMS storage
 * it would be a new database, or in on disk it would be a new
 * set of files.  This will mean automatically generating
 * a new identifier for the storage, maybe based on the existing
 * storage identifier.
 *
 * Return value: a new &librdf_storage object or NULL on failure
 */
librdf_storage*
librdf_new_storage_from_storage(librdf_storage* old_storage) 
{
  librdf_storage* new_storage;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(old_storage, librdf_storage, NULL);

  if(!old_storage->factory->clone) {
    librdf_log(old_storage->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "clone method not implemented for storage factory %s", old_storage->factory->name);
    return NULL;
  }

  new_storage=(librdf_storage*)LIBRDF_CALLOC(librdf_storage, 1,
                                             sizeof(librdf_storage));
  if(!new_storage)
    return NULL;
  
  new_storage->context=(char*)LIBRDF_CALLOC(librdf_storage_context, 1,
                                            old_storage->factory->context_length);
  if(!new_storage->context) {
    librdf_free_storage(new_storage);
    return NULL;
  }

  new_storage->world=old_storage->world;

  /* do this now so librdf_free_storage won't call new factory on
   * partially copied storage 
   */
  new_storage->factory=old_storage->factory;

  /* clone is assumed to do leave the new storage in the same state
   * after an init() method on an existing storage - i.e ready to
   * use but closed.
   */
  if(old_storage->factory->clone(new_storage, old_storage)) {
    librdf_free_storage(new_storage);
    return NULL;
  }

  new_storage->usage=1;
  
  return new_storage;
}


/**
 * librdf_new_storage_from_factory - Constructor - create a new librdf_storage object
 * @world: redland world object
 * @factory: the factory to use to construct the storage
 * @name: name to use for storage
 * @options: &librdf_hash of options to initialise storage
 *
 * If the options are present, they become owned by the storage
 * and should no longer be used.
 *
 * Return value: a new &librdf_storage object or NULL on failure
 */
librdf_storage*
librdf_new_storage_from_factory (librdf_world *world,
                                 librdf_storage_factory* factory,
                                 char *name,
                                 librdf_hash* options) {
  librdf_storage* storage;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(factory, librdf_storage_factory, NULL);

  if(!factory) {
    librdf_free_hash(options);
    return NULL;
  }
  
  storage=(librdf_storage*)LIBRDF_CALLOC(librdf_storage, 1,
                                         sizeof(librdf_storage));
  if(!storage) {
    librdf_free_hash(options);
    return NULL;
  }
  

  storage->world=world;
  
  storage->context=(char*)LIBRDF_CALLOC(librdf_storage_context, 1,
                                        factory->context_length);
  if(!storage->context) {
    librdf_free_hash(options);
    librdf_free_storage(storage);
    return NULL;
  }
  
  storage->factory=factory;

  if(factory->init(storage, name, options)) {
    librdf_free_storage(storage);
    return NULL;
  }
  
  storage->usage=1;
  
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(storage, librdf_storage);

  if(--storage->usage)
    return;

  if(storage->factory)
    storage->factory->terminate(storage);

  if(storage->context)
    LIBRDF_FREE(librdf_storage_context, storage->context);
  LIBRDF_FREE(librdf_storage, storage);
}


void
librdf_storage_add_reference(librdf_storage *storage)
{
  storage->usage++;
}

void
librdf_storage_remove_reference(librdf_storage *storage)
{
  storage->usage--;
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  return storage->factory->close(storage);
}


/**
 * librdf_storage_size - Get the number of statements stored
 * @storage: &librdf_storage object
 * 
 * Return value: The number of statements or < 0 if cannot be determined
 **/
int
librdf_storage_size(librdf_storage* storage) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, -1);

  return storage->factory->size(storage);
}


/**
 * librdf_storage_add_statement - Add a statement to a storage
 * @storage: &librdf_storage object
 * @statement: &librdf_statement statement to add
 * 
 * The passed-in statement is copied when added to the store, not
 * shared with the store.
 *
 * Return value: non 0 on failure
 **/
int
librdf_storage_add_statement(librdf_storage* storage,
                             librdf_statement* statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  if(storage->factory->add_statement)
    return storage->factory->add_statement(storage, statement);
  return 1;
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
  int status=0;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement_stream, librdf_stream, 1);

  if(storage->factory->add_statements)
    return storage->factory->add_statements(storage, statement_stream);

  while(!librdf_stream_end(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);

    if(statement)
      status=librdf_storage_add_statement(storage, statement);
    else
      status=1;

    if(status)
      break;

    librdf_stream_next(statement_stream);
  }
  
  return status;
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  if(storage->factory->remove_statement)
    return storage->factory->remove_statement(storage, statement);
  return 1;
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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

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
 * Searches the storage for a (partial) statement as described in
 * librdf_statement_match() and returns a &librdf_stream of
 * matching &librdf_statement objects.
 * 
 * Return value:  &librdf_stream of matching statements (may be empty) or NULL on failure
 **/
librdf_stream*
librdf_storage_find_statements(librdf_storage* storage,
                               librdf_statement* statement) 
{
  librdf_node *subject, *predicate, *object;
  librdf_iterator *iterator;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  subject=librdf_statement_get_subject(statement);
  predicate=librdf_statement_get_predicate(statement);
  object=librdf_statement_get_object(statement);

  /* try to pick the most efficient storage back end */

  /* only subject/source field blank -> use find_sources */
  if(!subject && predicate && object &&
     storage->factory->find_sources) {
    iterator=storage->factory->find_sources(storage, predicate, object);
    if(iterator)
      return librdf_new_stream_from_node_iterator(iterator, statement,
                                                  LIBRDF_STATEMENT_SUBJECT);
    return NULL;
  }
  
  /* only predicate/arc field blank -> use find_arcs */
  if(subject && !predicate && object &&
     storage->factory->find_arcs) {
    iterator=storage->factory->find_arcs(storage, subject, object);
    if(iterator)
      return librdf_new_stream_from_node_iterator(iterator, statement,
                                                  LIBRDF_STATEMENT_PREDICATE);
    return NULL;
  }
  
  /* only object/target field blank -> use find_targets */
  if(subject && predicate && !object &&
     storage->factory->find_targets) {
    iterator=storage->factory->find_targets(storage, subject, predicate);
    if(iterator)
      return librdf_new_stream_from_node_iterator(iterator, statement,
                                                  LIBRDF_STATEMENT_OBJECT);
    return NULL;
  }
  
  return storage->factory->find_statements(storage, statement);
}


typedef struct {
  librdf_storage *storage;
  librdf_stream *stream;
  librdf_statement *partial_statement;
  librdf_statement_part want;
} librdf_storage_stream_to_node_iterator_context;


static int
librdf_storage_stream_to_node_iterator_is_end(void* iterator)
{
  librdf_storage_stream_to_node_iterator_context* context=(librdf_storage_stream_to_node_iterator_context*)iterator;

  return librdf_stream_end(context->stream);
}


static int
librdf_storage_stream_to_node_iterator_next_method(void* iterator) 
{
  librdf_storage_stream_to_node_iterator_context* context=(librdf_storage_stream_to_node_iterator_context*)iterator;

  return librdf_stream_next(context->stream);
}


static void*
librdf_storage_stream_to_node_iterator_get_method(void* iterator, int flags) 
{
  librdf_storage_stream_to_node_iterator_context* context=(librdf_storage_stream_to_node_iterator_context*)iterator;
  librdf_node* node;
  librdf_statement* statement=librdf_stream_get_object(context->stream);

  if(!statement)
    return NULL;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:

      switch(context->want) {
        case LIBRDF_STATEMENT_SUBJECT: /* SOURCES (subjects) */
          node=librdf_statement_get_subject(statement);
          break;

        case LIBRDF_STATEMENT_PREDICATE: /* ARCS (predicates) */
          node=librdf_statement_get_predicate(statement);
          break;

        case LIBRDF_STATEMENT_OBJECT: /* TARGETS (objects) */
          node=librdf_statement_get_object(statement);
          break;

        default: /* error */
          librdf_log(statement->world,
                     0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                     "Unknown statement part %d\n", context->want);
          node=NULL;
      }
      break;
      
    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      node=(librdf_node*)librdf_stream_get_context(context->stream);
      break;
      
    default:
      librdf_log(statement->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d\n", flags);
      node=NULL;
  }
  
  return (void*)node;
}


static void
librdf_storage_stream_to_node_iterator_finished(void* iterator) 
{
  librdf_storage_stream_to_node_iterator_context* context=(librdf_storage_stream_to_node_iterator_context*)iterator;
  librdf_statement *partial_statement=context->partial_statement;

  /* make sure librdf_free_statement() doesn't free anything here */
  if(partial_statement) {
    librdf_statement_set_subject(partial_statement, NULL);
    librdf_statement_set_predicate(partial_statement, NULL);
    librdf_statement_set_object(partial_statement, NULL);

    librdf_free_statement(partial_statement);
  }

  if(context->stream)
    librdf_free_stream(context->stream);

  if(context->storage)
    librdf_storage_remove_reference(context->storage);
  
  LIBRDF_FREE(librdf_storage_stream_to_node_iterator_context, context);
}


/*
 * librdf_storage_node_stream_to_node_create - Create a stream for get sources, targets or arcs methods using find_statements method
 * @storage: the storage object to use
 * @node1: the first node to encode in the key
 * @node2: the second node to encode in the key (or NULL if not needed)
 * @want: the field required from the statement
 * 
 * Return value: a new &librdf_iterator or NULL on failure
 **/
static librdf_iterator*
librdf_storage_node_stream_to_node_create(librdf_storage* storage,
                                          librdf_node *node1,
                                          librdf_node *node2,
                                          librdf_statement_part want)
{
  librdf_statement *partial_statement;
  librdf_stream *stream;
  librdf_storage_stream_to_node_iterator_context* context;
  librdf_iterator *iterator;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node1, librdf_node, NULL);

  partial_statement=librdf_new_statement(storage->world);
  if(!partial_statement)
    return NULL;
  
  context=(librdf_storage_stream_to_node_iterator_context*)LIBRDF_CALLOC(librdf_storage_stream_to_node_iterator_context, 1, sizeof(librdf_storage_stream_to_node_iterator_context));
  if(!context) {
    librdf_free_statement(partial_statement);
    return NULL;
  }

  switch(want) {
    case LIBRDF_STATEMENT_SUBJECT:
      librdf_statement_set_predicate(partial_statement, node1);
      librdf_statement_set_object(partial_statement, node2);
      break;
    case LIBRDF_STATEMENT_PREDICATE:
      librdf_statement_set_subject(partial_statement, node1);
      librdf_statement_set_object(partial_statement, node2);
      break;
    case LIBRDF_STATEMENT_OBJECT:
      librdf_statement_set_subject(partial_statement, node1);
      librdf_statement_set_predicate(partial_statement, node2);
      break;
    default:
      librdf_free_statement(partial_statement);
      librdf_log(storage->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Illegal statement part %d seen\n", want);
      return NULL;
  }
  
  stream=storage->factory->find_statements(storage, partial_statement);
  if(!stream) {
    librdf_storage_stream_to_node_iterator_finished(context);
    return NULL;
  }
  
  /* initialise context */
  context->partial_statement=partial_statement;
  context->stream=stream;
  context->want=want;

  context->storage=storage;
  librdf_storage_add_reference(context->storage);

  iterator=librdf_new_iterator(storage->world,
                               (void*)context,
                               librdf_storage_stream_to_node_iterator_is_end,
                               librdf_storage_stream_to_node_iterator_next_method,
                               librdf_storage_stream_to_node_iterator_get_method,
                               librdf_storage_stream_to_node_iterator_finished);
  if(!iterator)
    librdf_storage_stream_to_node_iterator_finished(context);

  return iterator;
}


/**
 * librdf_storage_get_sources - return the sources (subjects) of arc in an RDF graph given arc (predicate) and target (object)
 * @storage: &librdf_storage object
 * @arc: &librdf_node arc
 * @target: &librdf_node target
 * 
 * Searches the storage for arcs matching the given arc and target
 * and returns a list of the source &librdf_node objects as an iterator
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_storage_get_sources(librdf_storage *storage,
                           librdf_node *arc, librdf_node *target) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(arc, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(target, librdf_node, NULL);

  if (storage->factory->find_sources)
    return storage->factory->find_sources(storage, arc, target);

  return librdf_storage_node_stream_to_node_create(storage, arc, target,
                                                   LIBRDF_STATEMENT_SUBJECT);
}


/**
 * librdf_storage_get_arcs - return the arcs (predicates) of an arc in an RDF graph given source (subject) and target (object)
 * @storage: &librdf_storage object
 * @source: &librdf_node source
 * @target: &librdf_node target
 * 
 * Searches the storage for arcs matching the given source and target
 * and returns a list of the arc &librdf_node objects as an iterator
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_storage_get_arcs(librdf_storage *storage,
                        librdf_node *source, librdf_node *target) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(source, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(target, librdf_node, NULL);

  if (storage->factory->find_arcs)
    return storage->factory->find_arcs(storage, source, target);

  return librdf_storage_node_stream_to_node_create(storage, source, target,
                                                   LIBRDF_STATEMENT_PREDICATE);
}


/**
 * librdf_storage_get_targets - return the targets (objects) of an arc in an RDF graph given source (subject) and arc (predicate)
 * @storage: &librdf_storage object
 * @source: &librdf_node source
 * @arc: &librdf_node arc
 * 
 * Searches the storage for targets matching the given source and arc
 * and returns a list of the source &librdf_node objects as an iterator
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_storage_get_targets(librdf_storage *storage,
                           librdf_node *source, librdf_node *arc) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(source, librdf_node, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(arc, librdf_node, NULL);

  if (storage->factory->find_targets)
    return storage->factory->find_targets(storage, source, arc);

  return librdf_storage_node_stream_to_node_create(storage, source, arc,
                                                   LIBRDF_STATEMENT_OBJECT);
}


/**
 * librdf_storage_get_arcs_in - return the properties pointing to the given resource
 * @storage: &librdf_storage object
 * @node: &librdf_node resource node
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_storage_get_arcs_in(librdf_storage *storage, librdf_node *node) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if (storage->factory->get_arcs_in)
    return storage->factory->get_arcs_in(storage, node);

  return librdf_storage_node_stream_to_node_create(storage, NULL, node,
                                                   LIBRDF_STATEMENT_PREDICATE);
}


/**
 * librdf_storage_get_arcs_out - return the properties pointing from the given resource
 * @storage: &librdf_storage object
 * @node: &librdf_node resource node
 * 
 * Return value:  &librdf_iterator of &librdf_node objects (may be empty) or NULL on failure
 **/
librdf_iterator*
librdf_storage_get_arcs_out(librdf_storage *storage, librdf_node *node) 
{
  if (storage->factory->get_arcs_out)
    return storage->factory->get_arcs_out(storage, node);
  return librdf_storage_node_stream_to_node_create(storage, node, NULL,
                                                   LIBRDF_STATEMENT_PREDICATE);
}


/**
 * librdf_storage_has_arc_in - check if a node has a given property pointing to it
 * @storage: &librdf_storage object
 * @node: &librdf_node resource node
 * @property: &librdf_node property node
 * 
 * Return value: non 0 if arc property does point to the resource node
 **/
int
librdf_storage_has_arc_in(librdf_storage *storage, librdf_node *node,
                          librdf_node *property) 
{
  librdf_iterator *iterator;
  int status;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(property, librdf_node, 0);

  if (storage->factory->has_arc_in)
    return storage->factory->has_arc_in(storage, node, property);
  
  iterator=librdf_storage_get_sources(storage, property, node);
  if(!iterator)
    return 0;

  /* a non-empty list of sources is success */
  status=!librdf_iterator_end(iterator);
  librdf_free_iterator(iterator);

  return status;
}


/**
 * librdf_storage_has_arc_out - check if a node has a given property pointing from it
 * @storage: &librdf_storage object
 * @node: &librdf_node resource node
 * @property: &librdf_node property node
 * 
 * Return value: non 0 if arc property does point from the resource node
 **/
int
librdf_storage_has_arc_out(librdf_storage *storage, librdf_node *node, 
                           librdf_node *property) 
{
  librdf_iterator *iterator;
  int status;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(property, librdf_node, 0);

  if (storage->factory->has_arc_out)
    return storage->factory->has_arc_out(storage, node, property);
  
  iterator=librdf_storage_get_targets(storage, node, property);
  if(!iterator)
    return 0;

  /* a non-empty list of targets is success */
  status=!librdf_iterator_end(iterator);
  librdf_free_iterator(iterator);

  return status;
}



/**
 * librdf_storage_context_add_statement - Add a statement to a storage in a context
 * @storage: &librdf_storage object
 * @context: &librdf_node context node
 * @statement: &librdf_statement statement to add
 * 
 * If @context is NULL, this is equivalent to librdf_storage_add_statement
 *
 * Return value: non 0 on failure
 **/
int
librdf_storage_context_add_statement(librdf_storage* storage,
                                     librdf_node* context,
                                     librdf_statement* statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, librdf_statement, 1);

  if(storage->factory->context_add_statement)
    return storage->factory->context_add_statement(storage, context, statement);
  return 1;
}


/**
 * librdf_storage_context_add_statements - Add statements to a storage with a context
 * @storage: &librdf_storage object
 * @context: &librdf_node context
 * @stream: &librdf_stream stream object
 * 
 * If @context is NULL, this is equivalent to librdf_storage_add_statements
 *
 * Return value: Non 0 on failure
 **/
int
librdf_storage_context_add_statements(librdf_storage* storage, 
                                      librdf_node* context,
                                      librdf_stream* stream) 
{
  int status=0;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, librdf_statement, 1);

  if(storage->factory->context_add_statements)
    return storage->factory->context_add_statements(storage, context, stream);

  if(!storage->factory->context_add_statement)
    return 1;
  
  if(!stream)
    return 1;

  while(!librdf_stream_end(stream)) {
    librdf_statement* statement=librdf_stream_get_object(stream);
    if(!statement)
      break;
    status=librdf_storage_context_add_statement(storage, context, statement);
    if(status)
      break;
    librdf_stream_next(stream);
  }

  return status;
}



/**
 * librdf_storage_context_remove_statement - Remove a statement from a storage in a context
 * @storage: &librdf_storage object
 * @context: &librdf_node context node
 * @statement: &librdf_statement statement to remove
 * 
 * If @context is NULL, this is equivalent to librdf_storage_remove_statement
 *
 * Return value: non 0 on failure
 **/
int
librdf_storage_context_remove_statement(librdf_storage* storage, 
                                        librdf_node* context,
                                        librdf_statement* statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, librdf_statement, 1);

  if(!storage->factory->context_remove_statement)
    return 1;
  
  return storage->factory->context_remove_statement(storage, context, statement);
}


/**
 * librdf_storage_context_remove_statements - Remove statements from a storage with the given context
 * @storage: &librdf_storage object
 * @context: &librdf_uri context
 * 
 * If @context is NULL, this is equivalent to librdf_storage_remove_statements
 *
 * Return value: Non 0 on failure
 **/
int
librdf_storage_context_remove_statements(librdf_storage* storage,
                                         librdf_node* context) 
{
  librdf_stream *stream;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  if(storage->factory->context_remove_statements)
    return storage->factory->context_remove_statements(storage, context);
  
  if(!storage->factory->context_remove_statement)
    return 1;
  
  stream=librdf_storage_context_as_stream(storage, context);
  if(!stream)
    return 1;

  while(!librdf_stream_end(stream)) {
    librdf_statement *statement=librdf_stream_get_object(stream);
    if(!statement)
      break;
    librdf_storage_context_remove_statement(storage, context, statement);
    librdf_stream_next(stream);
  }
  librdf_free_stream(stream);  
  return 0;
}


/**
 * librdf_storage_context_as_stream - List all statements in a storage context
 * @storage: &librdf_storage object
 * @context: &librdf_node context node
 * 
 * Return value: &librdf_stream of statements or NULL on failure or context is empty
 **/
librdf_stream*
librdf_storage_context_as_stream(librdf_storage* storage, librdf_node* context)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  return storage->factory->context_serialise(storage, context);
}


/**
 * librdf_storage_context_serialise - List all statements in a storage context (DEPRECATED)
 * @storage: &librdf_storage object
 * @context: &librdf_node context node
 * 
 * DEPRECATED to reduce confusion with the librdf_serializer class.
 * Please use librdf_storage_context_as_stream.
 *
 * Return value: &librdf_stream of statements or NULL on failure or context is empty
 **/
librdf_stream*
librdf_storage_context_serialise(librdf_storage* storage,
                                 librdf_node* context)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  return librdf_storage_context_as_stream(storage, context);
}


int
librdf_storage_supports_query(librdf_storage* storage, librdf_query *query)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, 0);

  /* FIXME - no storage system supports a query language at present */
  return 0;
}


librdf_query_results*
librdf_storage_query_execute(librdf_storage* storage, librdf_query *query) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(query, librdf_query, NULL);

  /* FIXME - no storage system supports querying yet */
  return NULL;
}


void
librdf_storage_sync(librdf_storage* storage) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(storage, librdf_storage);

  if(storage->factory->sync)
    storage->factory->sync(storage);
}


/**
 * librdf_storage_find_statements_in_context - search the storage for matching statements in a given context
 * @storage: &librdf_storage object
 * @statement: &librdf_statement partial statement to find
 * @context_node: context &librdf_node (or NULL)
 * 
 * Searches the storage for a (partial) statement as described in
 * librdf_statement_match() in the given context and returns a
 * &librdf_stream of matching &librdf_statement objects.  If
 * context is NULL, this is equivalent to librdf_storage_find_statements.
 * 
 * Return value: &librdf_stream of matching statements (may be empty) or NULL on failure
 **/
librdf_stream*
librdf_storage_find_statements_in_context(librdf_storage* storage, librdf_statement* statement, librdf_node* context_node) 
{
  librdf_stream *stream;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  if(storage->factory->find_statements_in_context)
    return storage->factory->find_statements_in_context(storage, statement, context_node);

  statement=librdf_new_statement_from_statement(statement);
  if(!statement)
    return NULL;

  stream=librdf_storage_context_as_stream(storage, context_node);
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
 * librdf_storage_get_contexts - return the list of contexts in the store
 * @storage: &librdf_storage object
 * 
 * Returns an iterator of &librdf_node context nodes for each
 * context in the store.
 *
 * Return value: &librdf_iterator of context nodes or NULL on failure or if contexts are not supported
 **/
librdf_iterator*
librdf_storage_get_contexts(librdf_storage* storage) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  if(storage->factory->get_contexts)
    return storage->factory->get_contexts(storage);
  else
    return NULL;
}



/**
 * librdf_storage_get_feature - get the value of a storage feature
 * @storage: &librdf_storage object
 * @feature: &librdf_uri feature property
 * 
 * Return value: &librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
librdf_node*
librdf_storage_get_feature(librdf_storage* storage, librdf_uri* feature)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, NULL);

  if(storage->factory->get_feature)
    return storage->factory->get_feature(storage, feature);
  return NULL;
}


/**
 * librdf_storage_set_feature - set the value of a storage feature
 * @storage: &librdf_storage object
 * @feature: &librdf_uri feature property
 * @value: &librdf_node feature property value
 * 
 * Return value: non 0 on failure (negative if no such feature)
 **/
int
librdf_storage_set_feature(librdf_storage* storage, librdf_uri* feature,
                           librdf_node* value)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, -1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, -1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(value, librdf_node, -1);

  if(storage->factory->set_feature)
    return storage->factory->set_feature(storage, feature, value);
  return -1;
}


/**
 * librdf_storage_find_statements_with_options - search the storage for matching statements with match options
 * @storage: &librdf_storage object
 * @statement: &librdf_statement partial statement to find
 * @context_node: &librdf_node context node or NULL.
 * @options: &librdf_hash of matching options or NULL
 * 
 * Searches the storage for a (partial) statement as described in
 * librdf_statement_match() and returns a &librdf_stream of
 * matching &librdf_statement objects.
 * 
 * If options is given then the match is made according to
 * the given options.  If options is NULL, this is equivalent
 * to librdf_storage_find_statements_in_context.
 * 
 * Return value:  &librdf_stream of matching statements (may be empty) or NULL on failure
 **/
librdf_stream*
librdf_storage_find_statements_with_options(librdf_storage* storage,
                                            librdf_statement* statement,
                                            librdf_node* context_node,
                                            librdf_hash* options) 
{
  if(storage->factory->find_statements_with_options)
    return storage->factory->find_statements_with_options(storage, statement, context_node, options);
  else
    return librdf_storage_find_statements_in_context(storage, statement, context_node);
}


#endif


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_storage* storage;
  char *program=argv[0];
  librdf_world *world;
  
  world=librdf_new_world();
  
  /* initialise hash, model and storage modules */
  librdf_init_hash(world);
  librdf_init_storage(world);
  librdf_init_model(world);
  
  fprintf(stdout, "%s: Creating storage\n", program);
  storage=librdf_new_storage(world, NULL, "test", NULL);
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }

  
  fprintf(stdout, "%s: Opening storage\n", program);
  if(librdf_storage_open(storage, NULL)) {
    fprintf(stderr, "%s: Failed to open storage\n", program);
    return(1);
  }


  /* Can do nothing here since need model and storage working */

  fprintf(stdout, "%s: Closing storage\n", program);
  librdf_storage_close(storage);

  fprintf(stdout, "%s: Freeing storage\n", program);
  librdf_free_storage(storage);
  

  /* finish model and storage modules */
  librdf_finish_model(world);
  librdf_finish_storage(world);
  librdf_finish_hash(world);

  LIBRDF_FREE(librdf_world, world);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
