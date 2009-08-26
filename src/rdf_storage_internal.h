/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_internal.h - Internal RDF Storage definitions
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
 * 
 * 
 */


#ifndef LIBRDF_STORAGE_INTERNAL_H
#define LIBRDF_STORAGE_INTERNAL_H

#include "rdf_storage_module.h"

#ifdef __cplusplus
extern "C" {
#endif

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
  void *instance;
  int index_contexts;
  struct librdf_storage_factory_s* factory;
};

void librdf_init_storage_list(librdf_world *world);

void librdf_init_storage_hashes(librdf_world *world);

void librdf_init_storage_trees(librdf_world *world);

void librdf_init_storage_file(librdf_world *world);

#ifdef STORAGE_MYSQL
void librdf_init_storage_mysql(librdf_world *world);
#endif

#ifdef STORAGE_VIRTUOSO
void librdf_init_storage_virtuoso(librdf_world *world);
#endif

#ifdef STORAGE_POSTGRESQL
void librdf_init_storage_postgresql(librdf_world *world);
#endif

#ifdef STORAGE_TSTORE
void librdf_init_storage_tstore(librdf_world *world);
#endif

#ifdef STORAGE_SQLITE
void librdf_init_storage_sqlite(librdf_world *world);
#endif


/* module init */
void librdf_init_storage(librdf_world *world);

/* module terminate */
void librdf_finish_storage(librdf_world *world);

/* class methods */
librdf_storage_factory* librdf_get_storage_factory(librdf_world* world, const char *name);


/* rdf_storage_sql.c */
typedef struct  
{
  char* filename;
  
  const char** predicate_uri_strings;
  int predicates_count;

  /* array of char* with NULL at end - size predicates_count */
  char** values;
} librdf_sql_config;

librdf_sql_config* librdf_new_sql_config(librdf_world* world, const char *storage_name, const char* layout, const char* config_dir, const char** predicate_uri_strings);
librdf_sql_config* librdf_new_sql_config_for_storage(librdf_storage* storage, const char* layout, const char* dir);
void librdf_free_sql_config(librdf_sql_config* config);

typedef enum {
  DBCONFIG_CREATE_TABLE_STATEMENTS,
  DBCONFIG_CREATE_TABLE_LITERALS,
  DBCONFIG_CREATE_TABLE_RESOURCES,
  DBCONFIG_CREATE_TABLE_BNODES,
  DBCONFIG_CREATE_TABLE_MODELS,
  DBCONFIG_CREATE_TABLE_LAST = DBCONFIG_CREATE_TABLE_MODELS
} librdf_dbconfig;

extern const char* librdf_storage_sql_dbconfig_predicates[DBCONFIG_CREATE_TABLE_LAST+2];



#ifdef __cplusplus
}
#endif

#endif
