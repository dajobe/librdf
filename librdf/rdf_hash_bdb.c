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
  char* file_name;
} librdf_hash_bdb_context;


/* Implementing the hash cursor */
static int librdf_hash_bdb_cursor_init(void *cursor_context, void *hash_context);
static int librdf_hash_bdb_cursor_get(void *context, librdf_hash_datum* key, librdf_hash_datum* value, unsigned int flags);
static void librdf_hash_bdb_cursor_finish(void* context);


/* prototypes for local functions */
static int librdf_hash_bdb_open(void* context, char *identifier, void *mode, librdf_hash* options);
static int librdf_hash_bdb_close(void* context);
static int librdf_hash_bdb_put(void* context, librdf_hash_datum *key, librdf_hash_datum *data, unsigned int flags);
static int librdf_hash_bdb_exists(void* context, librdf_hash_datum *key);
static int librdf_hash_bdb_delete(void* context, librdf_hash_datum *key);
static int librdf_hash_bdb_sync(void* context);
static int librdf_hash_bdb_get_fd(void* context);

static void librdf_hash_bdb_register_factory(librdf_hash_factory *factory);


/* functions implementing hash api */

/**
 * librdf_hash_bdb_open - Open and maybe create a BerkeleyDB hash
 * @context: BerkeleyDB hash context
 * @identifier: filename to use for BerkeleyDB file
 * @mode: access mode (currently unused)
 * @options: hash options (currently unused)
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
  /* V3 prototype:
   * int db_create(DB **dbp, DB_ENV *dbenv, u_int32_t flags);
   */
  if((ret=db_create(&bdb, NULL, 0))) {
    LIBRDF_DEBUG2(librdf_hash_bdb_open, "Failed to create BDB context - %d\n", ret);
    return 1;
  }
  
  if((ret=bdb->set_flags(bdb, DB_DUP))) {
    LIBRDF_DEBUG2(librdf_hash_bdb_open, "Failed to set BDB duplicate flag - %d\n", ret);
    return 1;
  }
  
  /* V3 prototype:
   * int DB->open(DB *db, const char *file, const char *database,
   *              DBTYPE type, u_int32_t flags, int mode);
   */
  if((ret=bdb->open(bdb, file, NULL, DB_BTREE, DB_CREATE, 0644))) {
    LIBRDF_FREE(cstring, file);
    return 1;
  }
#else
#ifdef HAVE_DB_OPEN
  /* V2 prototype:
   * int db_open(const char *file, DBTYPE type, u_int32_t flags,
   *             int mode, DB_ENV *dbenv, DB_INFO *dbinfo, DB **dbpp);
   */
  if((ret=db_open(file, DB_BTREE, DB_CREATE | DB_DUP, 0644, NULL, NULL, &bdb))) {
    LIBRDF_DEBUG2(librdf_hash_bdb_open, "BDB db_open failed - %d\n", ret);
    LIBRDF_FREE(cstring, file);
    return 1;
  }
#else
#ifdef HAVE_DBOPEN
  /* V1 prototype:
    const char *file, int flags, int mode, DBTYPE, const void *openinfo
  */
  if((bdb=dbopen(file, O_CREAT | O_RDWR | R_DUP, 0644, DB_BTREE, NULL)) == 0) {
    LIBRDF_DEBUG2(librdf_hash_bdb_open, "BDB dbopen failed - %d\n", ret);
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
 * librdf_hash_bdb_close - Close the hash
 * @context: BerkeleyDB hash context
 * 
 * Finish the association between the rdf hash and the BDB file (does
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
  /* V2/V3 */
  ret=db->close(db, 0);
#else
  /* V1 */
  ret=db->close(db);
#endif
  LIBRDF_FREE(cstring, bdb_context->file_name);
  return ret;
}


typedef struct {
  librdf_hash_bdb_context* hash;
  void *last_key;
  void *last_value;
#ifdef HAVE_BDB_CURSOR
  DBC* cursor;
#endif
} librdf_hash_bdb_cursor_context;


/**
 * librdf_hash_bdb_cursor_init - Initialise a new bdb cursor
 * @cursor_context: hash cursor context
 * @hash_context: hash to operate over
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_cursor_init(void *cursor_context, void *hash_context)
{
  librdf_hash_bdb_cursor_context *cursor=(librdf_hash_bdb_cursor_context*)cursor_context;
#ifdef HAVE_BDB_CURSOR
  DB* db;
#endif

  cursor->hash=(librdf_hash_bdb_context*)hash_context;

#ifdef HAVE_BDB_CURSOR
  /* V2/V3 prototype:
   * int DB->cursor(DB *db, DB_TXN *txnid, DBC **cursorp, u_int32_t flags);
   */
  db=cursor->hash->db;
  if(db->cursor(db, NULL, &cursor->cursor, 0))
    return 0;
#endif
  return 0;
}


/**
 * librdf_hash_bdb_cursor_get - Retrieve a hash value for the given key
 * @context: BerkeleyDB hash cursor context
 * @key: pointer to key to use
 * @value: pointer to value to use
 * @flags: flags
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_cursor_get(void* context, 
                           librdf_hash_datum *key, librdf_hash_datum *value,
                           unsigned int flags)
{
  librdf_hash_bdb_cursor_context *cursor=(librdf_hash_bdb_cursor_context*)context;
#ifdef HAVE_BDB_CURSOR
  DBC *bdb_cursor=cursor->cursor;
#endif
  DBT bdb_key;
  DBT bdb_value;
  int ret;

  /* Free old data */
  /* Don't free last key when moving by value, need to check it later */
  if(cursor->last_key && flags != LIBRDF_HASH_CURSOR_NEXT_VALUE) {
    LIBRDF_FREE(cstring, cursor->last_key);
    cursor->last_key=NULL;
  }
    
  if(cursor->last_value) {
    LIBRDF_FREE(cstring, cursor->last_value);
    cursor->last_value=NULL;
  }

  /* docs say you must zero DBT's before use */
  memset(&bdb_key, 0, sizeof(DBT));
  memset(&bdb_value, 0, sizeof(DBT));

  /* Always initialise BDB version of key */
  bdb_key.data = (char*)key->data;
  bdb_key.size = key->size;
  
#ifdef DB_DBT_MALLOC
  /* BDB V2 or later? */
  bdb_key.flags=DB_DBT_MALLOC;   /* Return in malloc() allocated memory */
  bdb_value.flags=DB_DBT_MALLOC;
#endif
  
  switch(flags) {
    case LIBRDF_HASH_CURSOR_SET:

#ifdef HAVE_BDB_CURSOR
      /* V2/V3 prototype:
       * int DBcursor->c_get(DBC *cursor, DBT *key, DBT *data, u_int32_t flags);
       */
      ret=bdb_cursor->c_get(bdb_cursor, &bdb_key, &bdb_value, DB_SET);
#else
      /* V1 */
      ret=db->seq(db, &bdb_key, &bdb_value, 0);
#endif
      break;
      
    case LIBRDF_HASH_CURSOR_FIRST:
#ifdef HAVE_BDB_CURSOR
      /* V2/V3 prototype:
       * int DBcursor->c_get(DBC *cursor, DBT *key, DBT *data, u_int32_t flags);
       */
      ret=bdb_cursor->c_get(bdb_cursor, &bdb_key, &bdb_value, DB_FIRST);
#else
      /* V1 */
      ret=db->seq(db, &bdb_key, &bdb_value, R_FIRST);
#endif
      break;
      
    case LIBRDF_HASH_CURSOR_NEXT_VALUE:
#ifdef HAVE_BDB_CURSOR
      /* V2/V3 */
      ret=bdb_cursor->c_get(bdb_cursor, &bdb_key, &bdb_value, DB_NEXT);
#else
      /* V1 */
      ret=db->seq(db, &bdb_key, &bdb_value, R_NEXT);
#endif
      
      /* If succeeded and key has changed, end */
      if(!ret && cursor->last_key &&
         memcmp(cursor->last_key, bdb_key.data, bdb_key.size)) {
        
        /* always allocated by BDB using system malloc */
        free(bdb_key.data);
        free(bdb_value.data);

        ret=DB_NOTFOUND;
      }
      

      /* always tidy up this */
      if(cursor->last_key) {
        LIBRDF_FREE(cstring, cursor->last_key);
        cursor->last_key=NULL;
      }

      break;
      
    case LIBRDF_HASH_CURSOR_NEXT:
#ifdef HAVE_BDB_CURSOR
      /* V2/V3 */
      ret=bdb_cursor->c_get(bdb_cursor, &bdb_key, &bdb_value,
                            (value) ? DB_NEXT : DB_NEXT_NODUP);
#else
      /* V1 */
      ret=db->seq(db, &bdb_key, &bdb_value, R_NEXT);
#endif
      break;
      
    default:
      abort();
  }
  

  if(ret) {
    if(ret != DB_NOTFOUND)
      LIBRDF_DEBUG2(librdf_hash_bdb_cursor_get, "BDB cursor error - %d\n", ret);
    key->data=NULL;
    return ret;
  }
  
  cursor->last_key = key->data = LIBRDF_MALLOC(cstring, bdb_key.size);
  if(!key->data) {
    /* always allocated by BDB using system malloc */
    if(flags != LIBRDF_HASH_CURSOR_SET)
      free(bdb_key.data);
    free(bdb_value.data);
    return 1;
  }
  
  memcpy(key->data, bdb_key.data, bdb_key.size);
  key->size = bdb_key.size;

  if(value) {
    cursor->last_value = value->data = LIBRDF_MALLOC(cstring, bdb_value.size);
    if(!value->data) {
      /* always allocated by BDB using system malloc */
      if(flags != LIBRDF_HASH_CURSOR_SET)
        free(bdb_key.data);
      free(bdb_value.data);
      return 1;
    }
    
    memcpy(value->data, bdb_value.data, bdb_value.size);
    value->size = bdb_value.size;
  }

  /* always allocated by BDB using system malloc */
  if(flags != LIBRDF_HASH_CURSOR_SET)
    free(bdb_key.data);
  free(bdb_value.data);

  return 0;
}


/**
 * librdf_hash_bdb_cursor_finished - Finish the serialisation of the hash bdb get
 * @context: BerkeleyDB hash cursor context
 **/
static void
librdf_hash_bdb_cursor_finish(void* context)
{
  librdf_hash_bdb_cursor_context* cursor=(librdf_hash_bdb_cursor_context*)context;

#ifdef HAVE_BDB_CURSOR
  /* BDB V2/V3 */
  if(cursor->cursor)
    cursor->cursor->c_close(cursor->cursor);
#endif
  if(cursor->last_key)
    LIBRDF_FREE(cstring, cursor->last_key);
    
  if(cursor->last_value)
    LIBRDF_FREE(cstring, cursor->last_value);
}


/**
 * librdf_hash_bdb_put - Store a key/value pair in the hash
 * @context: BerkeleyDB hash context
 * @key: pointer to key to store
 * @value: pointer to value to store
 * @flags: flags (not used at present)
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_put(void* context, librdf_hash_datum *key, 
                    librdf_hash_datum *value, unsigned int flags) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  DBT bdb_value;
  DBT bdb_key;
  int ret;

  /* docs say you must zero DBT's before use */
  memset(&bdb_value, 0, sizeof(DBT));
  memset(&bdb_key, 0, sizeof(DBT));
  
  /* Initialise BDB version of key */
  bdb_key.data = (char*)key->data;
  bdb_key.size = key->size;
  
  /* Initialise BDB version of data */
  bdb_value.data = (char*)value->data;
  bdb_value.size = value->size;
  
#ifdef HAVE_BDB_DB_TXN
  /* V2/V3 prototype:
   * int DB->put(DB *db, DB_TXN *txnid, DBT *key, DBT *data, u_int32_t flags); 
   */
  ret=db->put(db, NULL, &bdb_key, &bdb_value, 0);
#else
  /* V1 */
  ret=db->put(db, &bdb_key, &bdb_value, 0);
#endif
  if(ret)
    LIBRDF_DEBUG2(librdf_hash_bdb_put, "BDB put failed - %d\n", ret);

  return (ret != 0);
}


/**
 * librdf_hash_bdb_exists - Test the existence of a key in the hash
 * @context: BerkeleyDB hash context
 * @key: pointer to key to store
 * 
 * Return value: non 0 if the key exists in the hash
 **/
static int
librdf_hash_bdb_exists(void* context, librdf_hash_datum *key) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  DBT bdb_key;
  DBT bdb_value;
  int ret;

  /* docs say you must zero DBT's before use */
  memset(&bdb_key, 0, sizeof(DBT));
  memset(&bdb_value, 0, sizeof(DBT));
  
  /* Initialise BDB version of key */
  bdb_key.data = (char*)key->data;
  bdb_key.size = key->size;
	
#ifdef HAVE_BDB_DB_TXN
  /* V2/V3 */
  if((ret=db->get(db, NULL, &bdb_key, &bdb_value, 0)) == DB_NOTFOUND) {
#else
  /* V1 */
  if((ret=db->get(db, &bdb_key, &bdb_value, 0)) != 0) {
#endif
    /* not found */
    return 0;
  }
  
  /* always allocated by BDB using system malloc */
  free(bdb_value.data);

  return 1;
}


/**
 * librdf_hash_bdb_delete - Delete a key/value pair from the hash
 * @context: BerkeleyDB hash context
 * @key: pointer to key to delete
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_hash_bdb_delete(void* context, librdf_hash_datum *key) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* bdb=bdb_context->db;
  DBT bdb_key;
  int ret;

  memset(&bdb_key, 0, sizeof(DBT));

  /* Initialise BDB version of key */
  bdb_key.data = (char*)key->data;
  bdb_key.size = key->size;
  
#ifdef HAVE_BDB_DB_TXN
  /* V2/V3 */
  ret=bdb->del(bdb, NULL, &bdb_key, 0);
#else
  /* V1 */
  ret=bdb->del(bdb, &bdb_key, 0);
#endif
  if(ret)
    LIBRDF_DEBUG3(librdf_hash_bdb_delete, "BDB del failed - %d - %s\n", ret,
                  db_strerror(ret));

  return (ret != 0);
}


/**
 * librdf_hash_bdb_sync - Flush the hash to disk
 * @context: BerkeleyDB hash context
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
 * librdf_hash_bdb_get_fd - Get the file description representing the hash
 * @context: BerkeleyDB hash context
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
 * librdf_hash_bdb_register_factory - Register the BerkeleyDB hash module with the hash factory
 * @factory: hash factory prototype
 * 
 **/
static void
librdf_hash_bdb_register_factory(librdf_hash_factory *factory) 
{
  factory->context_length = sizeof(librdf_hash_bdb_context);
  factory->cursor_context_length = sizeof(librdf_hash_bdb_cursor_context);
  
  factory->open    = librdf_hash_bdb_open;
  factory->close   = librdf_hash_bdb_close;
  factory->put     = librdf_hash_bdb_put;
  factory->exists  = librdf_hash_bdb_exists;
  factory->delete_key  = librdf_hash_bdb_delete;
  factory->sync    = librdf_hash_bdb_sync;
  factory->get_fd  = librdf_hash_bdb_get_fd;

  factory->cursor_init   = librdf_hash_bdb_cursor_init;
  factory->cursor_get    = librdf_hash_bdb_cursor_get;
  factory->cursor_finish = librdf_hash_bdb_cursor_finish;
}


/**
 * librdf_init_hash_bdb - Initialise the BerkeleyDB hash module
 **/
void
librdf_init_hash_bdb(void)
{
  librdf_hash_register_factory("bdb", &librdf_hash_bdb_register_factory);
}
