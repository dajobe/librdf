/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage.h - RDF Storage Factory and Storage interfaces and definitions
 *
 * $Id$
 *
 * Copyright (C) 2000-2004, David Beckett http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology http://www.ilrt.bristol.ac.uk/
 * University of Bristol, UK http://www.bristol.ac.uk/
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
 * 
 * 
 */


#ifndef LIBRDF_STORAGE_H
#define LIBRDF_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDF_INTERNAL

/** A storage object */
struct librdf_storage_s
{
  librdf_world *world;

  /* usage count of this instance
   * Used by other redland classes such as model, iterator, stream
   * via  librdf_storage_add_reference librdf_storage_remove_reference
   * The usage count of storage after construction is 1.
   */
  int usage;
  
  librdf_model *model;
  void *context;
  int index_contexts;
  struct librdf_storage_factory_s* factory;
};


/** A Storage Factory */
struct librdf_storage_factory_s {
  librdf_world *world;
  struct librdf_storage_factory_s* next;
  char* name;
  char* label;
  
  /* the rest of this structure is populated by the
     storage-specific register function */
  size_t context_length;
  
  /* create a new storage */
  int (*init)(librdf_storage* storage, char *name, librdf_hash* options);
  
  /* copy a storage */
  /* clone is assumed to do leave the new storage in the same state
   * after an init() method on an existing storage - i.e ready to
   * use but closed.
   */
  int (*clone)(librdf_storage* new_storage, librdf_storage* old_storage);

  /* destroy a storage */
  void (*terminate)(librdf_storage* storage);
  
  /* make storage be associated with model */
  int (*open)(librdf_storage* storage, librdf_model* model);
  
  /* close storage/model context */
  int (*close)(librdf_storage* storage);
  
  /* return the number of statements in the storage for model */
  int (*size)(librdf_storage* storage);
  
  /* add a statement to the storage from the given model - OPTIONAL */
  int (*add_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* add a statement to the storage from the given model - OPTIONAL */
  int (*add_statements)(librdf_storage* storage, librdf_stream* statement_stream);
  
  /* remove a statement from the storage - OPTIONAL */
  int (*remove_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* check if statement in storage  */
  int (*contains_statement)(librdf_storage* storage, librdf_statement* statement);
  /* check for [node, property, ?] */
  int (*has_arc_in)(librdf_storage *storage, librdf_node *node, librdf_node *property);
  /* check for [?, property, node] */
  int (*has_arc_out)(librdf_storage *storage, librdf_node *node, librdf_node *property);

  
  /* serialise the model in storage  */
  librdf_stream* (*serialise)(librdf_storage* storage);
  
  /* serialise the results of a query */
  librdf_stream* (*find_statements)(librdf_storage* storage, librdf_statement* statement);
  /* OPTIONAL */
  librdf_stream* (*find_statements_with_options)(librdf_storage* storage, librdf_statement* statement, librdf_node* context_node, librdf_hash* options);

  /* return a list of Nodes marching given arc, target */
  librdf_iterator* (*find_sources)(librdf_storage* storage, librdf_node *arc, librdf_node *target);

  /* return a list of Nodes marching given source, target */
  librdf_iterator* (*find_arcs)(librdf_storage* storage, librdf_node *source, librdf_node *target);

  /* return a list of Nodes marching given source, target */
  librdf_iterator* (*find_targets)(librdf_storage* storage, librdf_node *source, librdf_node *target);

  /* return list of properties to/from a node */
  librdf_iterator* (*get_arcs_in)(librdf_storage *storage, librdf_node *node);
  librdf_iterator* (*get_arcs_out)(librdf_storage *storage, librdf_node *node);


  /* add a statement to the storage from the context - OPTIONAL */
  /* NOTE: if context is NULL, this MUST be equivalent to add_statement */
  int (*context_add_statement)(librdf_storage* storage, librdf_node* context, librdf_statement *statement);
  
  /* remove a statement from the context - OPTIONAL */
  /* NOTE: if context is NULL, this MUST be equivalent to remove_statement */
  int (*context_remove_statement)(librdf_storage* storage, librdf_node* context, librdf_statement *statement);

  /* list statements in a context - OPTIONAL */
  librdf_stream* (*context_serialise)(librdf_storage* storage, librdf_node* context);

  /* synchronise to underlying storage - OPTIONAL */
  int (*sync)(librdf_storage* storage);

  /* add statements to the context - OPTIONAL (rdf_storage will do it
   * using context_add_statement if missing)
   * NOTE: if context is NULL, this MUST be equivalent to add_statements
  */
  int (*context_add_statements)(librdf_storage* storage, librdf_node* context, librdf_stream *stream);

  /* remove statements from the context - OPTIONAL (rdf_storage will do it
   * using context_remove_statement if missing)
   */
  int (*context_remove_statements)(librdf_storage* storage, librdf_node* context);

  /* search for statement in a context - OPTIONAL (rdf_storage will do
   * it using find_statements if missing)
   */
  librdf_stream* (*find_statements_in_context)(librdf_storage* storage, librdf_statement* statement, librdf_node* context_node);

  /* return an iterator of context nodes in the store - OPTIONAL
   * (returning NULL)
   */
  librdf_iterator* (*get_contexts)(librdf_storage* storage);

  /* features - OPTIONAL */
  librdf_node* (*get_feature)(librdf_storage* storaage, librdf_uri* feature);
  int (*set_feature)(librdf_storage* storage, librdf_uri* feature, librdf_node* value);

};

#include <rdf_storage_list.h>
#include <rdf_storage_hashes.h>
void librdf_init_storage_file(librdf_world *world);

#ifdef HAVE_MYSQL
#include <rdf_storage_mysql.h>
#endif

#ifdef HAVE_TSTORE
#include <rdf_storage_tstore.h>
#endif


/* module init */
void librdf_init_storage(librdf_world *world);

/* module terminate */
void librdf_finish_storage(librdf_world *world);

/* class methods */
librdf_storage_factory* librdf_get_storage_factory(const char *name);

void librdf_storage_add_reference(librdf_storage *storage);
void librdf_storage_remove_reference(librdf_storage *storage);

#ifdef HAVE_SQLITE
void librdf_init_storage_sqlite(librdf_world *world);
#endif

#endif


/* class methods */
REDLAND_API void librdf_storage_register_factory(librdf_world *world, const char *name, const char *label, void (*factory) (librdf_storage_factory*));

REDLAND_API int librdf_storage_enumerate(const unsigned int counter, const char **name, const char **label);


/* constructor */
REDLAND_API librdf_storage* librdf_new_storage(librdf_world *world, char *storage_name, char *name, char *options_string);
REDLAND_API librdf_storage* librdf_new_storage_with_options(librdf_world *world, char *storage_name, char *name, librdf_hash *options);
REDLAND_API librdf_storage* librdf_new_storage_from_storage(librdf_storage* old_storage);
REDLAND_API librdf_storage* librdf_new_storage_from_factory(librdf_world *world, librdf_storage_factory* factory, char *name, librdf_hash* options);

/* destructor */
REDLAND_API void librdf_free_storage(librdf_storage *storage);


/* methods */
REDLAND_API int librdf_storage_open(librdf_storage* storage, librdf_model *model);
REDLAND_API int librdf_storage_close(librdf_storage* storage);
REDLAND_API int librdf_storage_get(librdf_storage* storage, void *key, size_t key_len, void **value, size_t* value_len, unsigned int flags);

REDLAND_API int librdf_storage_size(librdf_storage* storage);

REDLAND_API int librdf_storage_add_statement(librdf_storage* storage, librdf_statement* statement);
REDLAND_API int librdf_storage_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
REDLAND_API int librdf_storage_remove_statement(librdf_storage* storage, librdf_statement* statement);
REDLAND_API int librdf_storage_contains_statement(librdf_storage* storage, librdf_statement* statement);
REDLAND_API librdf_stream* librdf_storage_serialise(librdf_storage* storage);
REDLAND_API librdf_stream* librdf_storage_find_statements(librdf_storage* storage, librdf_statement* statement);
REDLAND_API librdf_stream* librdf_storage_find_statements_with_options(librdf_storage* storage, librdf_statement* statement, librdf_node* context_node, librdf_hash* options);
REDLAND_API librdf_iterator* librdf_storage_get_sources(librdf_storage *storage, librdf_node *arc, librdf_node *target);
REDLAND_API librdf_iterator* librdf_storage_get_arcs(librdf_storage *storage, librdf_node *source, librdf_node *target);
REDLAND_API librdf_iterator* librdf_storage_get_targets(librdf_storage *storage, librdf_node *source, librdf_node *arc);


/* return list of properties to/from a node */
REDLAND_API librdf_iterator* librdf_storage_get_arcs_in(librdf_storage *storage, librdf_node *node);
REDLAND_API librdf_iterator* librdf_storage_get_arcs_out(librdf_storage *storage, librdf_node *node);

/* check for [node, property, ?] */
REDLAND_API int librdf_storage_has_arc_in(librdf_storage *storage, librdf_node *node, librdf_node *property);
/* check for [?, property, node] */
REDLAND_API int librdf_storage_has_arc_out(librdf_storage *storage, librdf_node *node, librdf_node *property);

/* context methods */
REDLAND_API int librdf_storage_context_add_statement(librdf_storage* storage, librdf_node* context, librdf_statement* statement);
REDLAND_API int librdf_storage_context_add_statements(librdf_storage* storage, librdf_node* context, librdf_stream* stream);
REDLAND_API int librdf_storage_context_remove_statement(librdf_storage* storage, librdf_node* context, librdf_statement* statement);
REDLAND_API int librdf_storage_context_remove_statements(librdf_storage* storage, librdf_node* context);
REDLAND_API librdf_stream* librdf_storage_context_as_stream(librdf_storage* storage, librdf_node* context);
REDLAND_API REDLAND_DEPRECATED librdf_stream* librdf_storage_context_serialise(librdf_storage* storage, librdf_node* context);
  
/* querying methods */
REDLAND_API int librdf_storage_supports_query(librdf_storage* storage, librdf_query *query);
REDLAND_API librdf_query_results* librdf_storage_query_execute(librdf_storage* storage, librdf_query *query);

/* synchronise a storage to the backing store */
REDLAND_API int librdf_storage_sync(librdf_storage *storage);

/* find statements in a given context */
REDLAND_API librdf_stream* librdf_storage_find_statements_in_context(librdf_storage* storage, librdf_statement* statement, librdf_node* context_node);

REDLAND_API librdf_iterator* librdf_storage_get_contexts(librdf_storage* storage);

/* features */
REDLAND_API librdf_node* librdf_storage_get_feature(librdf_storage* storage, librdf_uri* feature);
REDLAND_API int librdf_storage_set_feature(librdf_storage* storage, librdf_uri* feature, librdf_node* value);

#ifdef __cplusplus
}
#endif

#endif
