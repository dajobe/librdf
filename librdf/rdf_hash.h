/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash.h - RDF Hash Factory and Hash interfaces and definitions
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


#ifndef LIBRDF_HASH_H
#define LIBRDF_HASH_H

#ifdef __cplusplus
extern "C" {
#endif


/** private data type used to describe hash key and data */
typedef struct 
{
  void *data;
  size_t size;
} librdf_hash_data;


/* Return various hash key/value pairs in a sequence
 * @see librdf_hash_get_seq
 */
typedef enum {
  LIBRDF_HASH_SEQUENCE_CURRENT,  /* current hash key/value pair in a sequence */
  LIBRDF_HASH_SEQUENCE_FIRST,    /* first hash key/value pair in a sequence */
  LIBRDF_HASH_SEQUENCE_NEXT      /* next hash key/value pair in a sequence */
} librdf_hash_sequence_type;

  
/** A hash object */
struct librdf_hash_s
{
  char *context;
  struct librdf_hash_factory_s* factory;
};


/** A Hash Factory */
struct librdf_hash_factory_s {
  struct librdf_hash_factory_s* next;
  char* name;

  /* the rest of this structure is populated by the
     hash-specific register function */
  size_t context_length;

  /* open/create hash with identifier and options  */
  int (*open)(void* context, char *identifier, void *mode, librdf_hash* options);
  /* end hash association */
  int (*close)(void* context);

  /* retrieve / insert key/data pairs according to flags */
  int (*get)(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags);
  int (*put)(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags);

  /* returns true if key exists in hash, without returning value */
  int (*exists)(void* context, librdf_hash_data *key);

  int (*delete_key)(void* context, librdf_hash_data *key);
  /* retrieve a key/data pair via cursor-based/sequential access */
  int (*get_seq)(void* context, librdf_hash_data *key, librdf_hash_sequence_type type);
  /* flush any cached information to disk */
  int (*sync)(void* context);
  /* get the file descriptor for the hash, if it is file based (for locking) */
  int (*get_fd)(void* context);
};

typedef struct librdf_hash_factory_s librdf_hash_factory;



/* factory class methods */
void librdf_hash_register_factory(const char *name, void (*factory) (librdf_hash_factory*));
librdf_hash_factory* librdf_get_hash_factory(const char *name);


/* module init */
void librdf_init_hash(void);

/* module terminate */
void librdf_finish_hash(void);

/* constructor */
librdf_hash* librdf_new_hash(librdf_hash_factory* factory);

/* destructor */
void librdf_free_hash(librdf_hash *hash);


/* methods */

/* open/create hash with identifier and options  */
int librdf_hash_open(librdf_hash* hash, char *identifier, void *mode, librdf_hash* options);
/* end hash association */
int librdf_hash_close(librdf_hash* hash);

/* retrieve / insert key/data pairs according to flags */
int librdf_hash_get(librdf_hash* hash, void *key, size_t key_len, void **value, size_t *value_len, unsigned int flags);
int librdf_hash_put(librdf_hash* hash, void *key, size_t key_len, void *value, size_t value_len, unsigned int flags);

  /* returns true if key exists in hash, without returning value */
int librdf_hash_exists(librdf_hash* hash, void *key, size_t key_len);

int librdf_hash_delete(librdf_hash* hash, void *key, size_t key_len);
/* retrieve a key/data pair via cursor-based/sequential access */
int librdf_hash_get_seq(librdf_hash* hash, void **key, size_t* key_len, librdf_hash_sequence_type type);
/* flush any cached information to disk */
int librdf_hash_sync(librdf_hash* hash);
/* get the file descriptor for the hash, if it is file based (for locking) */
int librdf_hash_get_fd(librdf_hash* hash);

/* extra methods */
void librdf_hash_print(librdf_hash* hash, FILE *fh);
int librdf_hash_first(librdf_hash* hash, void** key, size_t* key_len);
int librdf_hash_next(librdf_hash* hash, void** key, size_t* key_len);

/* import a hash from a string representation */
int librdf_hash_from_string (librdf_hash* hash, char *string);

/* import a hash from an array of strings */
int librdf_hash_from_array_of_strings (librdf_hash* hash, char *array[]);


#ifdef __cplusplus
}
#endif

#endif
