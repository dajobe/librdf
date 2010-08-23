/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * librdf_storage_module.h - Interface for a Redland storage module
 *
 * Copyright (C) 2000-2008, David Beckett http://www.dajobe.org/
 * Copyright (C) 2000-2005, University of Bristol, UK http://www.bristol.ac.uk/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 */

#ifndef LIBRDF_STORAGE_FACTORY_H
#define LIBRDF_STORAGE_FACTORY_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * librdf_storage_instance:
 *
 * Opaque storage module instance handle.
 *
 * For use with a storage module and the librdf_storage_get_instance()
 * and librdf_storage_set_instance() functions.  The instance handle
 * should be set in the #librdf_storage_factory init factory method.
 */
typedef void* librdf_storage_instance;


/**
 * LIBRDF_STORAGE_MIN_INTERFACE_VERSION:
 *
 * Oldest support librdf storage module interface version.
 *
 */
#define LIBRDF_STORAGE_MIN_INTERFACE_VERSION 1

/**
 * LIBRDF_STORAGE_MAX_INTERFACE_VERSION:
 *
 * Newest supported librdf storage module interface version.
 *
 */
#define LIBRDF_STORAGE_MAX_INTERFACE_VERSION 1

/**
 * LIBRDF_STORAGE_INTERFACE_VERSION:
 *
 * Default librdf storage module interface version.
 *
 */
#define LIBRDF_STORAGE_INTERFACE_VERSION LIBRDF_STORAGE_MAX_INTERFACE_VERSION

/**
 * librdf_storage_factory:
 * @version: Interface version.  Only version 1 is defined.
 * @name: Name (ID) of this storage, e.g. "megastore" 
 * @label: Label of this storage, e.g. "Megastore Storage"
 * @init: Create a new storage.
 *   This method should create the required instance data and store it with
 *   librdf_storage_set_instance() so it can be used in other methods.
 * @clone: Copy a storage.
 *   This is assumed to leave the new storage in the same state as an existing
 *   storage after an init() method - i.e ready to use but closed.
 * @terminate: Destroy a storage.
 *   This method is responsible for freeing all memory allocated in the init method.
 * @open: Make storage be associated with model
 * @close: Close storage/model context
 * @size: Return the number of statements in the storage for model
 * @add_statement: Add a statement to the storage from the given model. OPTIONAL
 * @add_statements: Add a statement to the storage from the given model. OPTIONAL
 * @remove_statement: Remove a statement from the storage. OPTIONAL
 * @contains_statement: Check if statement is in storage
 * @has_arc_in: Check for [node, property, ?]
 * @has_arc_out: Check for [?, property, node]
 * @serialise: Serialise the model in storage
 * @find_statements: Return a stream of triples matching a triple pattern
 * @find_statements_with_options: Return a stream of triples matching a triple pattern with some options.  OPTIONAL
 * @find_sources: Return a list of Nodes marching given arc, target
 * @find_arcs: Return a list of Nodes marching given source, target
 * @find_targets: Return a list of Nodes marching given source, target
 * @get_arcs_in:  Return list of properties to a node (i.e. with node as the object)
 * @get_arcs_out: Return list of properties from a node (i.e. with node as the subject)
 * @context_add_statement: Add a statement to the storage from the context.
 *    NOTE: If context is NULL, this MUST be equivalent to @add_statement. OPTIONAL.
 * @context_remove_statement: Remove a statement from a context.
 *    NOTE: if context is NULL, this MUST be equivalent to remove_statement. OPTIONAL.
 * @context_serialise: Serialise statements in a context. OPTIONAL
 * @sync: Synchronise to underlying storage. OPTIONAL
 * @context_add_statements: Add statements to a context. storage core will do this using context_add_statement if missing. 
 *    NOTE: If context is NULL, this MUST be equivalent to add_statements. OPTIONAL
 * @context_remove_statements: Remove statements from a context. storage core will do this using context_remove_statement if missing). OPTIONAL
 * @find_statements_in_context: Search for statement in a context. storage core will do this using find_statements if missing. OPTIONAL
 * @get_contexts: Return an iterator of context nodes. Returns NULL if contexts not supported. OPTIONAL
 * @get_feature: Get a feature. OPTIONAL
 * @set_feature: Set a feature. OPTIONAL
 * @transaction_start: Begin a transaction. OPTIONAL
 * @transaction_start_with_handle: Begin a transaction with opaque data handle. OPTIONAL
 * @transaction_commit: Commit a transaction. OPTIONAL
 * @transaction_rollback: Rollback a transaction. OPTIONAL
 * @transaction_get_handle: Get opaque data handle passed to transaction_start_with_handle. OPTIONAL
 * 
 * A Storage Factory
 */
struct librdf_storage_factory_s {
  /* Interface version */
  int version;

  /* Name (ID) of this storage */
  char* name;

  /* Label of this storage */
  char* label;

  /* The rest of this structure is populated by the storage-specific
   * register function 
   */

  /* Create a new storage. */
  int (*init)(librdf_storage* storage, const char *name, librdf_hash* options);
  
  /* Copy a storage. */
  int (*clone)(librdf_storage* new_storage, librdf_storage* old_storage);

  /* Destroy a storage. */
  void (*terminate)(librdf_storage* storage);
  
  /* Make storage be associated with model */
  int (*open)(librdf_storage* storage, librdf_model* model);
  
  /* Close storage/model context */
  int (*close)(librdf_storage* storage);
  
  /* Return the number of statements in the storage for model */
  int (*size)(librdf_storage* storage);
  
  /* Add a statement to the storage from the given model */
  int (*add_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* Add a statement to the storage from the given model */
  int (*add_statements)(librdf_storage* storage, librdf_stream* statement_stream);
  
  /* Remove a statement from the storage */
  int (*remove_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* Check if statement is in storage */
  int (*contains_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* Check for [node, property, ?] */
  int (*has_arc_in)(librdf_storage *storage, librdf_node *node, librdf_node *property);
  
  /* Check for [?, property, node] */
  int (*has_arc_out)(librdf_storage *storage, librdf_node *node, librdf_node *property);
  
  /* Serialise the model in storage */
  librdf_stream* (*serialise)(librdf_storage* storage);
  
  /* Return a stream of triples matching a triple pattern */
  librdf_stream* (*find_statements)(librdf_storage* storage, librdf_statement* statement);

  /* Return a stream of triples matching a triple pattern with some options. */
  librdf_stream* (*find_statements_with_options)(librdf_storage* storage, librdf_statement* statement, librdf_node* context_node, librdf_hash* options);

  /* Return a list of Nodes marching given arc, target */
  librdf_iterator* (*find_sources)(librdf_storage* storage, librdf_node *arc, librdf_node *target);

  /* Return a list of Nodes marching given source, target */
  librdf_iterator* (*find_arcs)(librdf_storage* storage, librdf_node *src, librdf_node *target);

  /* Return a list of Nodes marching given source, target */
  librdf_iterator* (*find_targets)(librdf_storage* storage, librdf_node *src, librdf_node *target);

  /** Return list of properties to a node (i.e. with node as the object) */
  librdf_iterator* (*get_arcs_in)(librdf_storage *storage, librdf_node *node);
  
  /* Return list of properties from a node (i.e. with node as the subject) */
  librdf_iterator* (*get_arcs_out)(librdf_storage *storage, librdf_node *node);

  /* Add a statement to the storage from the context */
  int (*context_add_statement)(librdf_storage* storage, librdf_node* context, librdf_statement *statement);
  
  /* Remove a statement from a context */
  int (*context_remove_statement)(librdf_storage* storage, librdf_node* context, librdf_statement *statement);

  /* Serialise statements in a context */
  librdf_stream* (*context_serialise)(librdf_storage* storage, librdf_node* context);

  /* Synchronise to underlying storage */
  int (*sync)(librdf_storage* storage);

  /* Add statements to a context */
  int (*context_add_statements)(librdf_storage* storage, librdf_node* context, librdf_stream *stream);

  /* Remove statements from a context */
  int (*context_remove_statements)(librdf_storage* storage, librdf_node* context);

  /* Search for statement in a context */
  librdf_stream* (*find_statements_in_context)(librdf_storage* storage,
      librdf_statement* statement, librdf_node* context_node);

  /* Return an iterator of context nodes */
  librdf_iterator* (*get_contexts)(librdf_storage* storage);

  /* Get a feature */
  librdf_node* (*get_feature)(librdf_storage* storaage, librdf_uri* feature);
  
  /* Set a feature */
  int (*set_feature)(librdf_storage* storage, librdf_uri* feature, librdf_node* value);

  /* Begin a transaction */
  int (*transaction_start)(librdf_storage* storage);
  
  /* Begin a transaction with opaque data handle */
  int (*transaction_start_with_handle)(librdf_storage* storage, void* handle);
  
  /* Commit a transaction */
  int (*transaction_commit)(librdf_storage* storage);
  
  /* Rollback a transaction */
  int (*transaction_rollback)(librdf_storage* storage);

  /* Get opaque data handle passed to transaction_start_with_handle */
  void* (*transaction_get_handle)(librdf_storage* storage);

  /** Storage engine supports querying - OPTIONAL */
  int (*supports_query)(librdf_storage* storage, librdf_query *query);

  /** Storage engine returns query results - OPTIONAL */
  librdf_query_results* (*query_execute)(librdf_storage* storage, librdf_query *query);
};


/**
 * librdf_storage_module_register_function:
 * @world: world object
 *
 * Registration function for storage
 *
 * A storage module must define and export a function named of this
 * type with function name "librdf_storage_module_register_factory".
 *
 * This function will be called by Redland and must call
 * librdf_storage_register_factory() to register whatever storage
 * backends are implemented in the module.
 */
typedef void (*librdf_storage_module_register_function)(librdf_world *world);


#ifdef __cplusplus
}
#endif

#endif

