/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash_bdb.c - RDF hash Berkeley DB Interface Implementation
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

#include <sys/types.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
/* for memset */
#include <string.h>
#endif
/* for the memory allocation functions */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


#ifdef HAVE_DB_H
#include <db.h>
#endif

/* these three are alternatives: */
/* BDB V3 */
#ifdef HAVE_DB_CREATE
#define BDB_CLOSE_2_ARGS ?
#define BDB_FD_2_ARGS ?

#else

/* BDB V2 */
#ifdef HAVE_DB_OPEN
#define BDB_CLOSE_2_ARGS 1
#define BDB_FD_2_ARGS 1

#else

/* BDB V1 - NOT WORKING */
#ifdef HAVE_DBOPEN
/* for O_ flags */
#include <fcntl.h>
#undef BDB_CLOSE_2_ARGS
#undef BDB_FD_2_ARGS

#else

ERROR - no idea how to use Berkeley DB

#endif
#endif
#endif


#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_hash.h>
#include <rdf_hash_bdb.h>


typedef struct 
{
  DB* db;
#ifdef HAVE_BDB_CURSOR
  DBC* cursor;
#endif
	char* file_name;
} librdf_hash_bdb_context;


/* prototypes for local functions */
static int librdf_hash_bdb_open(void* context, char *identifier, void *mode, librdf_hash* options);
static int librdf_hash_bdb_close(void* context);
static int librdf_hash_bdb_get(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags);
static int librdf_hash_bdb_put(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags);
static int librdf_hash_bdb_exists(void* context, librdf_hash_data *key);
static int librdf_hash_bdb_delete(void* context, librdf_hash_data *key);
static int librdf_hash_bdb_get_seq(void* context, librdf_hash_data *key, librdf_hash_sequence_type type);
static int librdf_hash_bdb_sync(void* context);
static int librdf_hash_bdb_get_fd(void* context);

static void librdf_hash_bdb_register_factory(librdf_hash_factory *factory);


/* functions implementing hash api */

/**
 * librdf_hash_bdb_open:
 * @context: BerkeleyDB hash context
 * @identifier: filename to use for BerkeleyDB file
 * @mode: access mode (currently unused)
 * @options: hash options (currently unused)
 * 
 * Open and maybe create a BerkeleyDB hash.
 * 
 * Return value: non 0 on failure.
 **/
static int
librdf_hash_bdb_open(void* context, char *identifier, void *mode,
                     librdf_hash* options) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* bdb;
  char *file;
  int ret;
  
  file=(char*)LIBRDF_MALLOC(cstring, strlen(identifier)+1);
  if(!file)
    return 1;
  strcpy(file, identifier);
	
#ifdef HAVE_DB_CREATE
  if((ret=db_create(&bdb, NULL, 0)) != 0)
    return 1;
  if((ret=bdb->open(bdb, file, NULL, DB_HASH, DB_CREATE, 0644)) != 0) {
    LIBRDF_FREE(cstring, file);
    return 1;
  }
#else
#ifdef HAVE_DB_OPEN
  /* db_open() on my system is prototyped as:
     const char *name, DBTYPE, u_int32_t flags, int mode, DB_ENV*, DB_INFO*, DB** */
  if((ret=db_open(file, DB_HASH, DB_CREATE, 0644, NULL, NULL, &bdb)) != 0) {
    LIBRDF_FREE(cstring, file);
    return 1;
  }
#else
#ifdef HAVE_DBOPEN
  /* dbopen() on my system is prototyped as:
    const char *file, int flags, int mode, DBTYPE, const void *openinfo
  */
  if((bdb=dbopen(file, O_RDWR, 0644, DB_HASH, NULL)) != 0) {
    LIBRDF_FREE(cstring, file);
    return 1;
  }
  ret=0;
#else
ERROR - no idea how to use Berkeley DB
#endif
#endif
#endif

  bdb_context->db=bdb;
 bdb_context->file_name=file;
 return 0;
}


/**
 * librdf_hash_bdb_close:
 * @context: BerkeleyDB hash context
 * 
 * Finish the association between the rdf hash and the GDBM file (does
 * not delete the file)
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_close(void* context) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  int ret;
  
#ifdef BDB_CLOSE_2_ARGS
  ret=db->close(db, 0);
#else
  ret=db->close(db);
#endif
  LIBRDF_FREE(cstring, bdb_context->file_name);
  return ret;
}


/**
 * librdf_hash_bdb_get:
 * @context: BerkeleyDB hash context
 * @key: pointer to key to use
 * @data: pointer to data to return value
 * @flags: flags (not used at present)
 * 
 * Retrieve a hash value for the given key
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_get(void* context, librdf_hash_data *key,
                    librdf_hash_data *data, unsigned int flags) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  DBT bdb_data;
  DBT bdb_key;
  int ret;
  
  /* docs say you must zero DBT's before use */
  memset(&bdb_data, 0, sizeof(DBT));
  memset(&bdb_key, 0, sizeof(DBT));
  
  /* Initialise BDB version of key */
  bdb_key.data = (char*)key->data;
  bdb_key.size = key->size;
  
#ifdef DB_DBT_MALLOC
  /* BDB V2 or later? */
  bdb_data.flags=DB_DBT_MALLOC;
#endif
  
  
#ifdef HAVE_BDB_DB_TXN
  if((ret=db->get(db, NULL, &bdb_key, &bdb_data, 0)) == DB_NOTFOUND) {
#else
  if((ret=db->get(db, &bdb_key, &bdb_data, 0))) {
#endif
    /* not found */
    data->data = NULL;
    return ret;
  }
  
  data->data = LIBRDF_MALLOC(bdb_data, bdb_data.size);
  if(!data->data) {
    free(bdb_data.data);
    return 1;
  }
  memcpy(data->data, bdb_data.data, bdb_data.size);
  data->size = bdb_data.size;
  
  /* always allocated by BDB using system malloc */
  free(bdb_data.data);
  return 0;
}
	

/**
 * librdf_hash_bdb_put:
 * @context: BerkeleyDB hash context
 * @key: pointer to key to store
 * @value: pointer to value to store
 * @flags: flags (not used at present)
 * 
 * Store a key/value pair in the hash
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_put(void* context, librdf_hash_data *key, 
                    librdf_hash_data *value, unsigned int flags) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  DBT bdb_data;
  DBT bdb_key;
  int ret;

  /* docs say you must zero DBT's before use */
  memset(&bdb_data, 0, sizeof(DBT));
  memset(&bdb_key, 0, sizeof(DBT));
  
  /* Initialise BDB version of key */
  bdb_key.data = (char*)key->data;
  bdb_key.size = key->size;
  
  /* Initialise BDB version of data */
  bdb_data.data = (char*)value->data;
  bdb_data.size = value->size;
  
  /* flags can be R_CURSOR, R_IAFTER, R_IBEFORE, R_NOOVERWRITE, R_SETCURSOR */
#ifdef HAVE_BDB_DB_TXN
  return (ret=db->put(db, NULL, &bdb_key, &bdb_data, 0));
#else
  return (ret=db->put(db, &bdb_key, &bdb_data, 0));
#endif
}


/**
 * librdf_hash_bdb_exists:
 * @context: BerkeleyDB hash context
 * @key: pointer to key to store
 * 
 * Test the existence of a key in the hash.
 * 
 * Return value: non 0 if the key exists in the hash
 **/
static int
librdf_hash_bdb_exists(void* context, librdf_hash_data *key) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  DBT bdb_key;
  DBT bdb_data;
  int ret;

  /* docs say you must zero DBT's before use */
  memset(&bdb_key, 0, sizeof(DBT));
  memset(&bdb_data, 0, sizeof(DBT));
  
  /* Initialise BDB version of key */
  bdb_key.data = (char*)key->data;
  bdb_key.size = key->size;
	
#ifdef HAVE_BDB_DB_TXN
  if((ret=db->get(db, NULL, &bdb_key, &bdb_data, 0)) == DB_NOTFOUND) {
#else
  if((ret=db->get(db, &bdb_key, &bdb_data, 0)) != 0) {
#endif
    /* not found */
    return 0;
  }
  
  /* always allocated by BDB using system malloc */
  free(bdb_data.data);
  return 1;
}


/**
 * librdf_hash_bdb_delete:
 * @context: BerkeleyDB hash context
 * @key: pointer to key to delete
 * 
 * Delete a key and associate value from the hash.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_delete(void* context, librdf_hash_data *key) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  DBT bdb_key;
  int ret;
  
  /* Initialise BDB version of key */
  bdb_key.data = (char*)key->data;
  bdb_key.size = key->size;
  
#ifdef HAVE_BDB_DB_TXN
  return (ret=db->del(db, NULL, &bdb_key, 0));
#else
  return (ret=db->del(db, &bdb_key, 0));
#endif
}


/**
 * librdf_hash_bdb_get_seq:
 * @context: BerkeleyDB hash context
 * @key: pointer to the key
 * @type: type of operation
 * 
 * Start/get keys in a sequence from the hash.  Valid operations are
 * LIBRDF_HASH_SEQUENCE_FIRST to get the first key in the sequence
 * LIBRDF_HASH_SEQUENCE_NEXT to get the next in sequence and
 * LIBRDF_HASH_SEQUENCE_CURRENT to get the current key (again).
 * 
 * Return value: 
 **/
static int
librdf_hash_bdb_get_seq(void* context, librdf_hash_data *key, 
                        librdf_hash_sequence_type type) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  DBT bdb_key;
  DBT bdb_data;
#ifdef HAVE_BDB_CURSOR
  DBC* bdb_cursor;
#endif
  int ret;

  /* docs say you must zero DBT's before use */
  memset(&bdb_key, 0, sizeof(DBT));
  memset(&bdb_data, 0, sizeof(DBT));
#ifdef DB_DBT_MALLOC
  /* BDB V2 or later? */
  bdb_key.flags=DB_DBT_MALLOC;
  bdb_data.flags=DB_DBT_MALLOC;
#endif
  
  
  if(type == LIBRDF_HASH_SEQUENCE_FIRST) {
#ifdef HAVE_BDB_CURSOR
    ret=db->cursor(db, NULL, &bdb_context->cursor);
    if(!ret) {
      bdb_cursor=bdb_context->cursor;
      ret=bdb_cursor->c_get(bdb_cursor, &bdb_key, &bdb_data, DB_FIRST);
    }

#else
    ret=db->seq(db, &bdb_key, &bdb_data, R_FIRST);
#endif
  } else if (type == LIBRDF_HASH_SEQUENCE_NEXT) {
#ifdef HAVE_BDB_CURSOR
    bdb_cursor=bdb_context->cursor;
    ret=bdb_cursor->c_get(bdb_cursor, &bdb_key, &bdb_data, DB_NEXT);
#else
    ret=db->seq(db, &bdb_key, &bdb_data, R_NEXT);
#endif
  } else { /* LIBRDF_HASH_SEQUENCE_CURRENT */
#ifdef HAVE_BDB_CURSOR
    bdb_cursor=bdb_context->cursor;
    ret=bdb_cursor->c_get(bdb_cursor, &bdb_key, &bdb_data, DB_CURRENT);
#else
#ifdef R_CURRENT
    ret=db->seq(db, &bdb_key, &bdb_data, R_CURRENT);
#else
    /* BDB V1 does not define R_CURRENT - just use fail - FIXME */
    return 1;
#endif
#endif
  }
  

  if(ret) {
    key->data=NULL;
    return ret;
  }
  
  key->data = LIBRDF_MALLOC(bdb_data, bdb_key.size);
  if(!key->data) {
    /* always allocated by BDB using system malloc */
    free(bdb_key.data);
    free(bdb_data.data);
    return 1;
  }
  memcpy(key->data, bdb_key.data, bdb_key.size);
  key->size = bdb_key.size;
  
  /* always allocated by BDB using system malloc */
  free(bdb_key.data);
  free(bdb_data.data);
  
  return 0;
}


/**
 * librdf_hash_bdb_sync:
 * @context: BerkeleyDB hash context
 * 
 * Flush the database to disk.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_sync(void* context) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  int ret;

  return (ret=db->sync(db, 0));
}


/**
 * librdf_hash_bdb_get_fd:
 * @context: BerkeleyDB hash context
 * 
 * Get the file description representing the file for the database, for
 * use in file locking.
 * 
 * Return value: the file descriptor
 **/
static int
librdf_hash_bdb_get_fd(void* context) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  int fd;
  int ret;
  
#ifdef BDB_FD_2_ARGS
  ret=db->fd(db, &fd);
#else
  ret=0;
  fd=db->fd(db);
#endif
  return fd;
}


/* local function to register BDB hash functions */

/**
 * librdf_hash_bdb_register_factory:
 * @factory: hash factory prototype
 * 
 * Register the BerkeleyDB hash module with the hash factory
 **/
static void
librdf_hash_bdb_register_factory(librdf_hash_factory *factory) 
{
  factory->context_length = sizeof(librdf_hash_bdb_context);
  
  factory->open    = librdf_hash_bdb_open;
  factory->close   = librdf_hash_bdb_close;
  factory->get     = librdf_hash_bdb_get;
  factory->put     = librdf_hash_bdb_put;
  factory->exists  = librdf_hash_bdb_exists;
  factory->delete_key  = librdf_hash_bdb_delete;
  factory->get_seq = librdf_hash_bdb_get_seq;
  factory->sync    = librdf_hash_bdb_sync;
  factory->get_fd  = librdf_hash_bdb_get_fd;
}


/**
 * librdf_init_hash_bdb:
 * 
 * Initialise the BerkeleyDB hash module.
 **/
void
librdf_init_hash_bdb(void)
{
  librdf_hash_register_factory("BDB", &librdf_hash_bdb_register_factory);
}
