/*
 * RDF Hash Factory and Hash interfaces and definitions
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef RDF_HASH_H
#define RDF_HASH_H


/** data type used to describe hash key and data */
typedef struct 
{
  void *data;
  size_t size;
} rdf_hash_data;


/* Return the current hash key/value pair in a sequence
 * @see rdf_hash_get_seq
 */
#define RDF_HASH_FLAGS_CURRENT 0

/* Return first hash key/value pair in a sequence
 * @see rdf_hash_get_seq
 */
#define RDF_HASH_FLAGS_FIRST 1

/* Return next hash key/value pair in a sequence
 * @see rdf_hash_get_seq
 */
#define RDF_HASH_FLAGS_NEXT 2

/** A Hash Factory */
struct rdf_hash_factory_s {
  struct rdf_hash_factory_s* next;
  char* name;

  /* the rest of this structure is populated by the
     hash-specific register function */
  size_t context_length;

  /* open/create hash with identifier and options  */
  int (*open)(void* context, char *identifier, void *mode, void *options);
  /* end hash association */
  int (*close)(void* context);

  /* retrieve / insert key/data pairs according to flags */
  int (*get)(void* context, rdf_hash_data *key, rdf_hash_data *data, unsigned int flags);
  int (*put)(void* context, rdf_hash_data *key, rdf_hash_data *data, unsigned int flags);

  int (*delete_key)(void* context, rdf_hash_data *key);
  /* retrieve a key/data pair via cursor-based/sequential access */
  int (*get_seq)(void* context, rdf_hash_data *key, unsigned int flags);
  /* flush any cached information to disk */
  int (*sync)(void* context);
  /* get the file descriptor for the hash, if it is file based (for locking) */
  int (*get_fd)(void* context);
};

typedef struct rdf_hash_factory_s rdf_hash_factory;

/** A hash object */
typedef struct
{
  char *context;
  rdf_hash_factory* factory;
} rdf_hash;



/* module init */
void init_rdf_hash(void);


/* class methods */
void rdf_hash_register_factory(const char *name,
                               void (*factory) (rdf_hash_factory*)
                               );
rdf_hash_factory* get_rdf_hash_factory(const char *name);

/* constructor */
rdf_hash* new_rdf_hash(rdf_hash_factory* factory);

/* destructor */
void free_rdf_hash(rdf_hash *hash);


/* methods */

/* open/create hash with identifier and options  */
int rdf_hash_open(rdf_hash* hash, char *identifier, void *mode, void *options);
/* end hash association */
int rdf_hash_close(rdf_hash* hash);

/* retrieve / insert key/data pairs according to flags */
int rdf_hash_get(rdf_hash* hash, void *key, size_t key_len, void **value, size_t *value_len, unsigned int flags);
int rdf_hash_put(rdf_hash* hash, void *key, size_t key_len, void *value, size_t value_len, unsigned int flags);

int rdf_hash_delete(rdf_hash* hash, void *key, size_t key_len);
/* retrieve a key/data pair via cursor-based/sequential access */
int rdf_hash_get_seq(rdf_hash* hash, void **key, size_t* key_len, unsigned int flags);
/* flush any cached information to disk */
int rdf_hash_sync(rdf_hash* hash);
/* get the file descriptor for the hash, if it is file based (for locking) */
int rdf_hash_get_fd(rdf_hash* hash);

/* extra methods */
void rdf_hash_print(rdf_hash* hash, FILE *fh);
int rdf_hash_first(rdf_hash* hash, void** key, size_t* key_len);
int rdf_hash_next(rdf_hash* hash, void** key, size_t* key_len);

#endif

