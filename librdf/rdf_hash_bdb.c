/*
 * rdf_hash_bdb.c - RDF hash Berkeley DB Interface Implementation
 *
 * $Source$
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


#include <config.h>

#include <sys/types.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
/* for memset */
#include <string.h>
#endif

#ifdef HAVE_DB_H
#include <db.h>
#endif

/* these three are alternatives: */
/* BDB V3 */
#ifdef HAVE_DB_CREATE
#define BDB_CLOSE_2_ARGS ?
#define BDB_FD_2_ARGS ?

#endif

/* BDB V2 */
#ifdef HAVE_DB_OPEN
#define BDB_CLOSE_2_ARGS 1
#define BDB_FD_2_ARGS 1

#endif

/* BDB V1 - NO CODE OR TESTS WRITTEN FOR DB 1.x */
#if 0
#define BDB_CLOSE_2_ARGS ?
#define BDB_FD_2_ARGS ?
#endif


#define LIBRDF_INTERNAL 1
#include <rdf_config.h>
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

static int
librdf_hash_bdb_open(void* context, char *identifier, void *mode, librdf_hash* options) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* bdb;
  char *file;
  int ret;
  
  file=(char*)LIBRDF_MALLOC(cstring, strlen(identifier)+6);
  if(!file)
    return 1;
  sprintf(file, "%s.db", identifier);
  remove(file);

#ifdef HAVE_DB_CREATE
  if((ret=db_create(&bdb, NULL, 0)) != 0)
    return 1;
  if((ret=bdb->open(bdb, file, NULL, DB_HASH, DB_CREATE, 0644)) != 0) {
    LIBRDF_FREE(cstring, file);
    return 1;
  }
#else
  /* db_open() on my system is prototyped as:
     const char *name, DBTYPE, u_int32_t flags, int mode, DB_ENV*, DB_INFO*, DB** */
  if((ret=db_open(file, DB_HASH, DB_CREATE, 0644, NULL, NULL, &bdb)) != 0) {
    LIBRDF_FREE(cstring, file);
    return 1;
  }
#endif

  bdb_context->db=bdb;
  bdb_context->file_name=file;
  return 0;
}

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


static int
librdf_hash_bdb_get(void* context, librdf_hash_data *key, librdf_hash_data *data, unsigned int flags) 
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

  bdb_data.flags=DB_DBT_MALLOC;

  
#ifdef HAVE_BDB_DB_TXN
  if((ret=db->get(db, NULL, &bdb_key, &bdb_data, 0)) == DB_NOTFOUND) {
#else
  if(ret=db->get(db, &bdb_key, &bdb_data, 0)) {
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


static int
librdf_hash_bdb_put(void* context, librdf_hash_data *key, librdf_hash_data *value, unsigned int flags) 
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


static int
librdf_hash_bdb_get_seq(void* context, librdf_hash_data *key, librdf_hash_sequence_type type) 
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
  bdb_key.flags=DB_DBT_MALLOC;
  bdb_data.flags=DB_DBT_MALLOC;
  
  
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
    ret=db->seq(db, &bdb_key, &bdb_data, R_CURRENT);
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

static int
librdf_hash_bdb_sync(void* context) 
{
  librdf_hash_bdb_context* bdb_context=(librdf_hash_bdb_context*)context;
  DB* db=bdb_context->db;
  int ret;

  return (ret=db->sync(db, 0));
}


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

void
librdf_init_hash_bdb(void)
{
  librdf_hash_register_factory("BDB", &librdf_hash_bdb_register_factory);
}
