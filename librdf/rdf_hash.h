/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash.h - RDF Hash Factory and Hash interfaces and definitions
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


#ifndef LIBRDF_HASH_H
#define LIBRDF_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDF_INTERNAL

/** data type used to describe hash key and data */
struct librdf_hash_datum_s
{
  librdf_world *world;
  void *data;
  size_t size;
  /* used internally to build lists of these  */
  struct librdf_hash_datum_s *next;
};
typedef struct librdf_hash_datum_s librdf_hash_datum;

/* constructor / destructor for above */
librdf_hash_datum* librdf_new_hash_datum(librdf_world *world, void *data, size_t size);
void librdf_free_hash_datum(librdf_hash_datum *ptr);
  


/** A hash object */
struct librdf_hash_s
{
  librdf_world* world;
  char* identifier; /* as passed in during open(), used by clone() */
  void* context;
  int   is_open;
  struct librdf_hash_factory_s* factory;
};


/** A Hash Factory */
struct librdf_hash_factory_s {
  struct librdf_hash_factory_s* next;
  char* name;

  /* the rest of this structure is populated by the
     hash-specific register function */
  size_t context_length;

  /* size of the cursor context */
  size_t cursor_context_length;

  /* clone an existing storage */
  int (*clone)(librdf_hash* new_hash, void* new_context, char* new_name, void* old_context);

  /* create / destroy a hash implementation */
  int (*create)(librdf_hash* hash, void* context);
  int (*destroy)(void* context);

  /* open/create hash with identifier and options  */
  int (*open)(void* context, char *identifier, int mode, int is_writable, int is_new, librdf_hash* options);
  /* end hash association */
  int (*close)(void* context);

  /* hoe many values? */
  int (*values_count)(void* context);

  /* insert key/value pairs according to flags */
  int (*put)(void* context, librdf_hash_datum *key, librdf_hash_datum *data);

  /* returns true if key exists in hash, without returning value */
  int (*exists)(void* context, librdf_hash_datum *key, librdf_hash_datum *value);

  int (*delete_key)(void* context, librdf_hash_datum *key);
  int (*delete_key_value)(void* context, librdf_hash_datum *key, librdf_hash_datum *value);

  /* flush any cached information to disk */
  int (*sync)(void* context);

  /* get the file descriptor for the hash, if it is file based (for locking) */
  int (*get_fd)(void* context);

  /* create a cursor and operate on it */
  int (*cursor_init)(void *cursor_context, void* hash_context);
  int (*cursor_get)(void *cursor, librdf_hash_datum *key, librdf_hash_datum *value, unsigned int flags);
  void (*cursor_finish)(void *context);
};
typedef struct librdf_hash_factory_s librdf_hash_factory;

#endif

/* hash cursor_get method flags */
#define LIBRDF_HASH_CURSOR_SET 0
#define LIBRDF_HASH_CURSOR_NEXT_VALUE 1
#define LIBRDF_HASH_CURSOR_FIRST 2
#define LIBRDF_HASH_CURSOR_NEXT 3


/* factory class methods */
void librdf_hash_register_factory(librdf_world *world, const char *name, void (*factory) (librdf_hash_factory*));
librdf_hash_factory* librdf_get_hash_factory(librdf_world *world, const char *name);


/* module init */
void librdf_init_hash(librdf_world *world);

/* module terminate */
void librdf_finish_hash(librdf_world *world);

/* constructors */
librdf_hash* librdf_new_hash(librdf_world *world, char *name);
librdf_hash* librdf_new_hash_from_factory(librdf_world *world, librdf_hash_factory* factory);
librdf_hash* librdf_new_hash_from_hash (librdf_hash* old_hash);

/* destructor */
void librdf_free_hash(librdf_hash *hash);


/* methods */

/* open/create hash with identifier and options  */
int librdf_hash_open(librdf_hash* hash, char *identifier, int mode, int is_writable, int is_new, librdf_hash* options);
/* end hash association */
int librdf_hash_close(librdf_hash* hash);

/* how many values */
int librdf_hash_values_count(librdf_hash* hash);

/* retrieve one value for a given hash key either as char or hash datum */
char* librdf_hash_get(librdf_hash* hash, char *key);
librdf_hash_datum* librdf_hash_get_one(librdf_hash* hash, librdf_hash_datum *key);

/* retrieve one value for key and delete from hash all other values */
char* librdf_hash_get_del(librdf_hash* hash, char *key);

/* retrieve all values for a given hash key according to flags */
librdf_iterator* librdf_hash_get_all(librdf_hash* hash, librdf_hash_datum *key, librdf_hash_datum *value);

/* insert a key/value pair */
int librdf_hash_put(librdf_hash* hash, librdf_hash_datum *key, librdf_hash_datum *value);
int librdf_hash_put_strings(librdf_hash* hash, const char *key, const char *value);

  /* returns true if key exists in hash, without returning value */
int librdf_hash_exists(librdf_hash* hash, librdf_hash_datum *key, librdf_hash_datum *value);

int librdf_hash_delete(librdf_hash* hash, librdf_hash_datum *key, librdf_hash_datum *value);
int librdf_hash_delete_all(librdf_hash* hash, librdf_hash_datum *key);
librdf_iterator* librdf_hash_keys(librdf_hash* hash, librdf_hash_datum *key);
/* flush any cached information to disk */
int librdf_hash_sync(librdf_hash* hash);
/* get the file descriptor for the hash, if it is file based (for locking) */
int librdf_hash_get_fd(librdf_hash* hash);

/* extra methods */
void librdf_hash_print(librdf_hash* hash, FILE *fh);
void librdf_hash_print_keys(librdf_hash* hash, FILE *fh);
void librdf_hash_print_values(librdf_hash* hash, char *key, FILE *fh);

/* import a hash from a string representation */
int librdf_hash_from_string (librdf_hash* hash, const char *string);

/* import a hash from an array of strings */
int librdf_hash_from_array_of_strings (librdf_hash* hash, char *array[]);

/* lookup a hash key and decode value as a boolean */
int librdf_hash_get_as_boolean (librdf_hash* hash, char *key);

/* lookup a hash key and decode value as a long */
long librdf_hash_get_as_long (librdf_hash* hash, char *key);


/* cursor methods from rdf_hash_cursor.c */

librdf_hash_cursor* librdf_new_hash_cursor (librdf_hash* hash);
void librdf_free_hash_cursor (librdf_hash_cursor* cursor);
int librdf_hash_cursor_set(librdf_hash_cursor *cursor, librdf_hash_datum *key,librdf_hash_datum *value);
int librdf_hash_cursor_get_next_value(librdf_hash_cursor *cursor, librdf_hash_datum *key,librdf_hash_datum *value);
int librdf_hash_cursor_get_first(librdf_hash_cursor *cursor, librdf_hash_datum *key, librdf_hash_datum *value);
int librdf_hash_cursor_get_next(librdf_hash_cursor *cursor, librdf_hash_datum *key, librdf_hash_datum *value);




#ifdef __cplusplus
}
#endif

#endif
