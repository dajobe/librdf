/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_factory.h - Interface for an RDF Storage factory
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


/** Opaque instance handle. */
typedef void* librdf_storage_instance;


/** A Storage Factory */
struct librdf_storage_factory_s {
  /** Interface version */
  int version;

  /** Name (ID) of this storage, e.g. "megastore" (FIXME: this needs to become a URI) */
  char* name;

  /** Label of this storage, e.g. "Megastore Storage" */
  char* label;

  /* The rest of this structure is populated by the storage-specific register function */

  /** Create a new storage.
   * This method should create the required instance data and store it with
   * librdf_storage_set_instance so it can be used in other methods. */
  int (*init)(librdf_storage* storage, const char *name, librdf_hash* options);
  
  /** Copy a storage.
   * This is assumed to leave the new storage in the same state as an existing
   * storage after an init() method - i.e ready to use but closed.
   */
  int (*clone)(librdf_storage* new_storage, librdf_storage* old_storage);

  /** Destroy a storage.
   * This method is responsible for freeing all memory allocated in the init method. */
  void (*terminate)(librdf_storage* storage);
  
  /** Make storage be associated with model */
  int (*open)(librdf_storage* storage, librdf_model* model);
  
  /** Close storage/model context */
  int (*close)(librdf_storage* storage);
  
  /** Return the number of statements in the storage for model */
  int (*size)(librdf_storage* storage);
  
  /** Add a statement to the storage from the given model - OPTIONAL */
  int (*add_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /** Add a statement to the storage from the given model - OPTIONAL */
  int (*add_statements)(librdf_storage* storage, librdf_stream* statement_stream);
  
  /** Remove a statement from the storage - OPTIONAL */
  int (*remove_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /** Check if statement is in storage */
  int (*contains_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /** Check for [node, property, ?] */
  int (*has_arc_in)(librdf_storage *storage, librdf_node *node, librdf_node *property);
  
  /** Check for [?, property, node] */
  int (*has_arc_out)(librdf_storage *storage, librdf_node *node, librdf_node *property);
  
  /** Serialise the model in storage */
  librdf_stream* (*serialise)(librdf_storage* storage);
  
  /** Return a stream of triples matching a triple pattern */
  librdf_stream* (*find_statements)(librdf_storage* storage, librdf_statement* statement);

  /* OPTIONAL */
  librdf_stream* (*find_statements_with_options)(librdf_storage* storage,
      librdf_statement* statement, librdf_node* context_node, librdf_hash* options);

  /** Return a list of Nodes marching given arc, target */
  librdf_iterator* (*find_sources)(librdf_storage* storage, librdf_node *arc, librdf_node *target);

  /** Return a list of Nodes marching given source, target */
  librdf_iterator* (*find_arcs)(librdf_storage* storage, librdf_node *src, librdf_node *target);

  /* Return a list of Nodes marching given source, target */
  librdf_iterator* (*find_targets)(librdf_storage* storage, librdf_node *src, librdf_node *target);

  /** Return list of properties to a node (i.e. with node as the object) */
  librdf_iterator* (*get_arcs_in)(librdf_storage *storage, librdf_node *node);
  
  /** Return list of properties from a node (i.e. with node as the subject) */
  librdf_iterator* (*get_arcs_out)(librdf_storage *storage, librdf_node *node);

  /** Add a statement to the storage from the context - OPTIONAL
   * NOTE: if context is NULL, this MUST be equivalent to add_statement */
  int (*context_add_statement)(librdf_storage* storage, librdf_node* context,
                               librdf_statement *statement);
  
  /** Remove a statement from a context - OPTIONAL
   * NOTE: if context is NULL, this MUST be equivalent to remove_statement */
  int (*context_remove_statement)(librdf_storage* storage,
      librdf_node* context, librdf_statement *statement);

  /** Serialise statements in a context - OPTIONAL */
  librdf_stream* (*context_serialise)(librdf_storage* storage, librdf_node* context);

  /** Synchronise to underlying storage - OPTIONAL */
  int (*sync)(librdf_storage* storage);

  /** Add statements to a context - OPTIONAL
   * (rdf_storage will do this using context_add_statement if missing)
   * NOTE: if context is NULL, this MUST be equivalent to add_statements
   */
  int (*context_add_statements)(librdf_storage* storage, librdf_node* context, librdf_stream *stream);

  /** Remove statements from a context - OPTIONAL
   * (rdf_storage will do this using context_remove_statement if missing)
   */
  int (*context_remove_statements)(librdf_storage* storage, librdf_node* context);

  /** Search for statement in a context - OPTIONAL
   * (rdf_storage will do this using find_statements if missing)
   */
  librdf_stream* (*find_statements_in_context)(librdf_storage* storage,
      librdf_statement* statement, librdf_node* context_node);

  /** Return an iterator of context nodes in the store - OPTIONAL
   * (returning NULL)
   */
  librdf_iterator* (*get_contexts)(librdf_storage* storage);

  /** Get a feature - OPTIONAL */
  librdf_node* (*get_feature)(librdf_storage* storaage, librdf_uri* feature);
  
  /** Set a feature - OPTIONAL */
  int (*set_feature)(librdf_storage* storage, librdf_uri* feature, librdf_node* value);

  /** Begin a transaction - OPTIONAL */
  int (*transaction_start)(librdf_storage* storage);
  
  /** Begin a transaction with opaque data handle - OPTIONAL */
  int (*transaction_start_with_handle)(librdf_storage* storage, void* handle);
  
  /** Commit a transaction - OPTIONAL */
  int (*transaction_commit)(librdf_storage* storage);
  
  /** Rollback a transaction - OPTIONAL */
  int (*transaction_rollback)(librdf_storage* storage);

  /** Get opaque data handle passed to transaction_start_with_handle - OPTIONAL */
  void* (*transaction_get_handle)(librdf_storage* storage);
};


#ifdef __cplusplus
}
#endif

#endif
