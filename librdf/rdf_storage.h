/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage.h - RDF Storage Factory and Storage interfaces and definitions
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
  librdf_model *model;
  void *context;
  struct librdf_storage_factory_s* factory;
};


/** A Storage Factory */
struct librdf_storage_factory_s {
  librdf_world *world;
  struct librdf_storage_factory_s* next;
  char* name;
  
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
  
  /* add a statement to the storage from the given model */
  int (*add_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* add a statement to the storage from the given model */
  int (*add_statements)(librdf_storage* storage, librdf_stream* statement_stream);
  
  /* remove a statement from the storage  */
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

  /* return a list of Nodes marching given arc, target */
  librdf_iterator* (*find_sources)(librdf_storage* storage, librdf_node *arc, librdf_node *target);

  /* return a list of Nodes marching given source, target */
  librdf_iterator* (*find_arcs)(librdf_storage* storage, librdf_node *source, librdf_node *target);

  /* return a list of Nodes marching given source, target */
  librdf_iterator* (*find_targets)(librdf_storage* storage, librdf_node *source, librdf_node *target);

  /* return list of properties to/from a node */
  librdf_iterator* (*get_arcs_in)(librdf_storage *storage, librdf_node *node);
  librdf_iterator* (*get_arcs_out)(librdf_storage *storage, librdf_node *node);


  /* add a statement to the storage from the group */
  int (*group_add_statement)(librdf_storage* storage, librdf_uri* group, librdf_statement *statement);
  
  /* remove a statement from the group  */
  int (*group_remove_statement)(librdf_storage* storage, librdf_uri* group, librdf_statement *statement);

  /* list statements in the group  */
  librdf_stream* (*group_serialise)(librdf_storage* storage, librdf_uri* group);

};

#include <rdf_storage_list.h>
#include <rdf_storage_hashes.h>

#endif


/* module init */
void librdf_init_storage(librdf_world *world);

/* module terminate */
void librdf_finish_storage(librdf_world *world);

/* class methods */
void librdf_storage_register_factory(const char *name, void (*factory) (librdf_storage_factory*));
librdf_storage_factory* librdf_get_storage_factory(const char *name);

/* constructor */
librdf_storage* librdf_new_storage(librdf_world *world, char *storage_name, char *name, char *options_string);
librdf_storage* librdf_new_storage_from_storage (librdf_storage* old_storage);
librdf_storage* librdf_new_storage_from_factory(librdf_world *world, librdf_storage_factory* factory, char *name, librdf_hash* options);

/* destructor */
void librdf_free_storage(librdf_storage *storage);


/* methods */
int librdf_storage_open(librdf_storage* storage, librdf_model *model);
int librdf_storage_close(librdf_storage* storage);
int librdf_storage_get(librdf_storage* storage, void *key, size_t key_len, void **value, size_t* value_len, unsigned int flags);

int librdf_storage_size(librdf_storage* storage);
int librdf_storage_add_statement(librdf_storage* storage, librdf_statement* statement);
int librdf_storage_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
int librdf_storage_remove_statement(librdf_storage* storage, librdf_statement* statement);
int librdf_storage_contains_statement(librdf_storage* storage, librdf_statement* statement);
librdf_stream* librdf_storage_serialise(librdf_storage* storage);
librdf_stream* librdf_storage_find_statements(librdf_storage* storage, librdf_statement* statement);
librdf_iterator* librdf_storage_get_sources(librdf_storage *storage, librdf_node *arc, librdf_node *target);
librdf_iterator* librdf_storage_get_arcs(librdf_storage *storage, librdf_node *source, librdf_node *target);
librdf_iterator* librdf_storage_get_targets(librdf_storage *storage, librdf_node *source, librdf_node *arc);


/* return list of properties to/from a node */
librdf_iterator* librdf_storage_get_arcs_in(librdf_storage *storage, librdf_node *node);
librdf_iterator* librdf_storage_get_arcs_out(librdf_storage *storage, librdf_node *node);

/* check for [node, property, ?] */
int librdf_storage_has_arc_in(librdf_storage *storage, librdf_node *node, librdf_node *property);
/* check for [?, property, node] */
int librdf_storage_has_arc_out(librdf_storage *storage, librdf_node *node, librdf_node *property);

/* group methods */
int librdf_storage_group_add_statement(librdf_storage* storage, librdf_uri* group_uri, librdf_statement* statement);
int librdf_storage_group_remove_statement(librdf_storage* storage, librdf_uri* group_uri, librdf_statement* statement);
librdf_stream* librdf_storage_group_serialise(librdf_storage* storage, librdf_uri* group_uri);

/* querying methods */
int librdf_storage_supports_query(librdf_storage* storage, librdf_query *query);
librdf_stream* librdf_storage_query(librdf_storage* storage, librdf_query *query);


#ifdef __cplusplus
}
#endif

#endif
