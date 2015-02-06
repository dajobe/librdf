/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_sqlite.c - RDF Storage using SQLite implementation
 *
 * Copyright (C) 2004-2010, David Beckett http://www.dajobe.org/
 * Copyright (C) 2004-2005, University of Bristol, UK http://www.bristol.ac.uk/
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


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

#include <sqlite3.h>

#include <redland.h>
#include <rdf_storage.h>


static const char* const sqlite_synchronous_flags[4] = {
  "off", "normal", "full", NULL
};

typedef struct librdf_storage_sqlite_query librdf_storage_sqlite_query;

struct librdf_storage_sqlite_query
{
  unsigned char *query;
  librdf_storage_sqlite_query *next;
};

typedef struct
{
  librdf_storage *storage;

  sqlite3 *db;

  int is_new;
  
  char *name;
  size_t name_len;  

  int synchronous; /* -1 (not set), 0+ index into sqlite_synchronous_flags */

  int in_stream;
  librdf_storage_sqlite_query *in_stream_queries;

  int in_transaction;
} librdf_storage_sqlite_instance;



/* prototypes for local functions */
static int librdf_storage_sqlite_init(librdf_storage* storage, const char *name, librdf_hash* options);
static int librdf_storage_sqlite_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_sqlite_close(librdf_storage* storage);
static int librdf_storage_sqlite_size(librdf_storage* storage);
static int librdf_storage_sqlite_add_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_sqlite_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_sqlite_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_sqlite_contains_statement(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_sqlite_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_sqlite_find_statements(librdf_storage* storage, librdf_statement* statement);

/* serialising implementing functions */
static int librdf_storage_sqlite_serialise_end_of_stream(void* context);
static int librdf_storage_sqlite_serialise_next_statement(void* context);
static void* librdf_storage_sqlite_serialise_get_statement(void* context, int flags);
static void librdf_storage_sqlite_serialise_finished(void* context);

/* find_statements implementing functions */
static int librdf_storage_sqlite_find_statements_end_of_stream(void* context);
static int librdf_storage_sqlite_find_statements_next_statement(void* context);
static void* librdf_storage_sqlite_find_statements_get_statement(void* context, int flags);
static void librdf_storage_sqlite_find_statements_finished(void* context);

/* context functions */
static int librdf_storage_sqlite_context_add_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_sqlite_context_remove_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_sqlite_context_contains_statement(librdf_storage* storage, librdf_node* context, librdf_statement* statement);
static librdf_stream* librdf_storage_sqlite_context_serialise(librdf_storage* storage, librdf_node* context_node);

/* context sqlite statement stream methods */
static int librdf_storage_sqlite_context_serialise_end_of_stream(void* context);
static int librdf_storage_sqlite_context_serialise_next_statement(void* context);
static void* librdf_storage_sqlite_context_serialise_get_statement(void* context, int flags);
static void librdf_storage_sqlite_context_serialise_finished(void* context);

/* helper functions for contexts */

static librdf_iterator* librdf_storage_sqlite_get_contexts(librdf_storage* storage);

/* get_context iterator functions */
static int librdf_storage_sqlite_get_contexts_is_end(void* iterator);
static int librdf_storage_sqlite_get_contexts_next_method(void* iterator);
static void* librdf_storage_sqlite_get_contexts_get_method(void* iterator, int);
static void librdf_storage_sqlite_get_contexts_finished(void* iterator);

/* transactions */
static int librdf_storage_sqlite_transaction_start(librdf_storage *storage);
static int librdf_storage_sqlite_transaction_commit(librdf_storage *storage);
static int librdf_storage_sqlite_transaction_rollback(librdf_storage *storage);

static void librdf_storage_sqlite_query_flush(librdf_storage *storage);

static void librdf_storage_sqlite_register_factory(librdf_storage_factory *factory);
#ifdef MODULAR_LIBRDF
void librdf_storage_module_register_factory(librdf_world *world);
#endif


/* functions implementing storage api */
static int
librdf_storage_sqlite_init(librdf_storage* storage, const char *name,
                           librdf_hash* options)
{
  char *name_copy;
  char* synchronous;
  librdf_storage_sqlite_instance* context;
  
  if(!name) {
    if(options)
      librdf_free_hash(options);
    return 1;
  }
  
  context = LIBRDF_CALLOC(librdf_storage_sqlite_instance*, 1, sizeof(*context));
  if(!context) {
    if(options)
      librdf_free_hash(options);
    return 1;
  }

  librdf_storage_set_instance(storage, context);
  
  context->storage = storage;

  context->name_len = strlen(name);
  name_copy = LIBRDF_MALLOC(char*, context->name_len + 1);
  if(!name_copy) {
    if(options)
      librdf_free_hash(options);
    return 1;
  }

  strncpy(name_copy, name, context->name_len + 1);
  context->name = name_copy;
  
  if(librdf_hash_get_as_boolean(options, "new")>0)
    context->is_new = 1; /* default is NOT NEW */

  /* Redland default is "PRAGMA synchronous normal" */
  context->synchronous = 1;

  if((synchronous = librdf_hash_get(options, "synchronous"))) {
    int i;
    
    for(i = 0; sqlite_synchronous_flags[i]; i++) {
      if(!strcmp(synchronous, sqlite_synchronous_flags[i])) {
        context->synchronous = i;
        break;
      }
    }
    
    LIBRDF_FREE(char*, synchronous);

  }
  

  /* no more options, might as well free them now */
  if(options)
    librdf_free_hash(options);

  return 0;
}


static void
librdf_storage_sqlite_terminate(librdf_storage* storage)
{
  librdf_storage_sqlite_instance* context;

  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);
  
  if (context == NULL)
    return;

  if(context->name)
    LIBRDF_FREE(char*, context->name);
  
  LIBRDF_FREE(librdf_storage_sqlite_terminate, librdf_storage_get_instance(storage));
}


typedef struct 
{
  const char *name;
  const char *schema;
  const char *columns; /* Excluding key column, always called id */
} table_info;


#define NTABLES 4

/*
 * INTEGER PRIMARY KEY columns can be used to implement the
 * equivalent of AUTOINCREMENT. If you try to insert a NULL into an
 * INTEGER PRIMARY KEY column, the column will actually be filled
 * with a integer that is one greater than the largest key already in
 * the table. Or if the largest key is 2147483647, then the column
 * will be filled with a random integer. Either way, the INTEGER
 * PRIMARY KEY column will be assigned a unique integer. You can
 * retrieve this integer using the sqlite3_last_insert_rowid() API
 * function or using the last_insert_rowid() SQL function in a
 * subsequent SELECT statement.
 */

typedef enum {
  TABLE_URIS,
  TABLE_BLANKS,
  TABLE_LITERALS,
  TABLE_TRIPLES
}  sqlite_table_numbers;

static const table_info sqlite_tables[NTABLES]={
  { "uris",     "id INTEGER PRIMARY KEY, uri TEXT", "uri" },
  { "blanks",   "id INTEGER PRIMARY KEY, blank TEXT", "blank" },
  { "literals", "id INTEGER PRIMARY KEY, text TEXT, language TEXT, datatype INTEGER", "text, language, datatype" },
  { "triples",  "subjectUri INTEGER, subjectBlank INTEGER, predicateUri INTEGER, objectUri INTEGER, objectBlank INTEGER, objectLiteral INTEGER, contextUri INTEGER", "subjectUri, subjectBlank, predicateUri, objectUri, objectBlank, objectLiteral, contextUri"  },
};


typedef enum {
  TRIPLE_SUBJECT  =0,
  TRIPLE_PREDICATE=1,
  TRIPLE_OBJECT   =2,
  TRIPLE_CONTEXT  =3,
} triple_part;

typedef enum {
  TRIPLE_URI    =0,
  TRIPLE_BLANK  =1,
  TRIPLE_LITERAL=2,
  TRIPLE_NONE   =3,
} triple_node_type;

static const char * const triples_fields[4][3] = {
  { "subjectUri",   "subjectBlank", NULL },
  { "predicateUri", NULL,           NULL },
  { "objectUri",    "objectBlank",  "objectLiteral" },
  { "contextUri",   NULL,           NULL }
};


static int
librdf_storage_sqlite_get_1int_callback(void *arg,
                                        int argc, char **argv,
                                        char **columnNames)
{
  int* count_p = (int*)arg;
  
  if(argc == 1) {
    *count_p = argv[0] ? atoi(argv[0]) : 0;
  }
  return 0;
}


static unsigned char *
sqlite_string_escape(const unsigned char *raw, size_t raw_len, size_t *len_p) 
{
  size_t escapes = 0;
  unsigned char *p;
  unsigned char *escaped;
  size_t len;

  for(p = (unsigned char*)raw, len = raw_len; len > 0; p++, len--) {
    if(*p == '\'')
      escapes++;
  }

  len = raw_len + escapes + 2; /* for '' */
  escaped = LIBRDF_MALLOC(unsigned char*, len + 1);
  if(!escaped)
    return NULL;

  p = escaped;
  *p++ = '\'';
  while(raw_len > 0) {
    if(*raw == '\'') {
      *p++ = '\'';
    }
    *p++ = *raw++;
    raw_len--;
  }
  *p++ = '\'';
  *p = '\0';

  if(len_p)
    *len_p = len;
  
  return escaped;
}


static int
librdf_storage_sqlite_exec(librdf_storage* storage, 
                           unsigned char *request,
                           sqlite3_callback callback, void *arg,
                           int fail_ok)
{
  librdf_storage_sqlite_instance* context;
  int status=SQLITE_OK;
  char *errmsg = NULL;

  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  /* sqlite crashes if passed in a NULL sql string */
  if(!request)
    return 1;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
  LIBRDF_DEBUG2("SQLite exec '%s'\n", request);
#endif
  
  status = sqlite3_exec(context->db, (const char*)request, callback, arg,
                       &errmsg);
  if(fail_ok)
    status = SQLITE_OK;
  
  if(status != SQLITE_OK) {
    if(status == SQLITE_LOCKED && !callback && context->in_stream) {
      librdf_storage_sqlite_query *query;
      /* error message from sqlite3_exec needs to be freed on both sqlite 2 and 3 */
      if(errmsg)
        sqlite3_free(errmsg);


      query = LIBRDF_CALLOC(librdf_storage_sqlite_query*, 1, sizeof(*query));
      if(!query)
        return 1;

      query->query = LIBRDF_MALLOC(unsigned char*, strlen((char *)request) + 1);
      if(!query->query) {
        LIBRDF_FREE(librdf_storage_sqlite_query, query);
        return 1;
      }

      strcpy((char*)query->query, (char *)request);

      if(!context->in_stream_queries)
        context->in_stream_queries = query;
      else {
        librdf_storage_sqlite_query *q = context->in_stream_queries;

        while(q->next)
          q = q->next;

        q->next = query;
      }

      status = SQLITE_OK;

    } else {
      librdf_log(librdf_storage_get_world(storage), 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s SQL exec '%s' failed - %s (%d)",
                 context->name, request, errmsg, status);
      /* error message from sqlite3_exec needs to be freed on both sqlite 2 and 3 */
      if(errmsg)
        sqlite3_free(errmsg);
    }
  }

  return (status != SQLITE_OK);
}


static int
librdf_storage_sqlite_set_helper(librdf_storage *storage,
                                 int table, 
                                 const unsigned char *values,
                                 size_t values_len) 
{
  librdf_storage_sqlite_instance* context;
  int rc;
  raptor_stringbuffer *sb;
  unsigned char *request;

  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  sb = raptor_new_stringbuffer();
  if(!sb)
    return -1;

  raptor_stringbuffer_append_string(sb, 
                                    (const unsigned char*)"INSERT INTO ", 1);
  raptor_stringbuffer_append_string(sb,  
                                    (const unsigned char*)sqlite_tables[table].name, 1);
  raptor_stringbuffer_append_counted_string(sb,  
                                    (const unsigned char*)" (id, ", 6, 1);
  raptor_stringbuffer_append_string(sb,  
                                    (const unsigned char*)sqlite_tables[table].columns, 1);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (const unsigned char*) ") VALUES(NULL, ", 15, 1);
  raptor_stringbuffer_append_counted_string(sb, values, values_len, 1);
  raptor_stringbuffer_append_counted_string(sb,  
                                            (const unsigned char*)");", 2, 1);
  request=raptor_stringbuffer_as_string(sb);

  rc=librdf_storage_sqlite_exec(storage,
                                request,
                                NULL, /* no callback */
                                NULL, /* arg */
                                0);

  raptor_free_stringbuffer(sb);

  if(rc)
    return -1;

  return LIBRDF_BAD_CAST(int, sqlite3_last_insert_rowid(context->db));
}


static int
librdf_storage_sqlite_get_helper(librdf_storage *storage,
                                 int table, 
                                 const unsigned char *expression) 
{
  int id = -1;
  int rc;
  raptor_stringbuffer *sb;
  unsigned char *request;

  sb = raptor_new_stringbuffer();
  if(!sb)
    return -1;

  raptor_stringbuffer_append_string(sb,  
                                    (const unsigned char*)"SELECT id FROM ", 1);
  raptor_stringbuffer_append_string(sb,  
                                    (const unsigned char*)sqlite_tables[table].name, 1);
  raptor_stringbuffer_append_counted_string(sb,  
                                            (const unsigned char*)" WHERE ", 7, 1);
  raptor_stringbuffer_append_string(sb,  
                                    (const unsigned char*)expression, 1);
  raptor_stringbuffer_append_counted_string(sb,  
                                            (const unsigned char*)";", 1, 1);
  request=raptor_stringbuffer_as_string(sb);

  rc = librdf_storage_sqlite_exec(storage,
                                  request,
                                  librdf_storage_sqlite_get_1int_callback,
                                  &id,
                                  0);

  raptor_free_stringbuffer(sb);

  if(rc)
    return -1;

  return id;
}


static int
librdf_storage_sqlite_uri_helper(librdf_storage* storage,
                                 librdf_uri* uri,
                                 int add_new) 
{
  const unsigned char *uri_string;
  size_t uri_len;
  unsigned char *expression = NULL;
  unsigned char *uri_e;
  size_t uri_e_len;
  int id = -1;
  static const char * const field="uri";

  uri_string = librdf_uri_as_counted_string(uri, &uri_len);
  uri_e = sqlite_string_escape(uri_string, uri_len, &uri_e_len);
  if(!uri_e)
    goto tidy;
  
  expression = LIBRDF_MALLOC(unsigned char*, strlen(field) + 3 + uri_e_len + 1);
  if(!expression)
    goto tidy;

  sprintf((char*)expression, "%s = %s", field, uri_e);
  id = librdf_storage_sqlite_get_helper(storage, TABLE_URIS, expression);
  if(id >= 0)
    goto tidy;

  if(add_new)  
    id = librdf_storage_sqlite_set_helper(storage, TABLE_URIS, uri_e,
                                          uri_e_len);

  tidy:
  if(expression)
    LIBRDF_FREE(char*, expression);
  if(uri_e)
    LIBRDF_FREE(char*, uri_e);

  return id;
}


static int
librdf_storage_sqlite_blank_helper(librdf_storage* storage,
                                   const unsigned char *blank,
                                   int add_new)
{
  size_t blank_len;
  unsigned char *expression = NULL;
  unsigned char *blank_e;
  size_t blank_e_len;
  int id = -1;
  static const char * const field="blank";

  blank_len = strlen((const char*)blank);
  blank_e = sqlite_string_escape(blank, blank_len, &blank_e_len);
  if(!blank_e)
    goto tidy;
  
  expression = LIBRDF_MALLOC(unsigned char*,
                             strlen(field) + 3 + blank_e_len + 1);
  if(!expression)
    goto tidy;

  sprintf((char*)expression, "%s = %s", field, blank_e);
  id = librdf_storage_sqlite_get_helper(storage, TABLE_BLANKS, expression);
  if(id >= 0)
    goto tidy;

  if(add_new)
    id = librdf_storage_sqlite_set_helper(storage, TABLE_BLANKS, blank_e, 
                                          blank_e_len);

  tidy:
  if(expression)
    LIBRDF_FREE(char*, expression);
  if(blank_e)
    LIBRDF_FREE(char*, blank_e);

  return id;
}


static int
librdf_storage_sqlite_literal_helper(librdf_storage* storage,
                                     const unsigned char *value,
                                     size_t value_len,
                                     const char *language,
                                     librdf_uri *datatype,
                                     int add_new) 
{
  int id = -1;
  size_t len;
  unsigned char *value_e;
  size_t value_e_len;
  unsigned char *language_e = NULL;
  size_t language_e_len = 0;
  int datatype_id = -1;
  raptor_stringbuffer *sb = NULL;
  unsigned char *expression;

  value_e = sqlite_string_escape(value, value_len, &value_e_len);
  if(!value_e)
    goto tidy;

  sb = raptor_new_stringbuffer();
  if(!sb)
    goto tidy;

  raptor_stringbuffer_append_counted_string(sb,  
                                            (const unsigned char*)"text = ",
                                            7, 1);
  raptor_stringbuffer_append_counted_string(sb, value_e, value_e_len, 1);
  raptor_stringbuffer_append_counted_string(sb, (const unsigned char*)" ", 1, 1);

  if(language) {
    len = strlen(language);
    language_e = sqlite_string_escape((unsigned const char*)language, len,
                                      &language_e_len);
    if(!language_e)
      goto tidy;

    raptor_stringbuffer_append_string(sb, (const unsigned char*)"AND language = ", 1);
    raptor_stringbuffer_append_counted_string(sb, language_e, language_e_len, 1);
  } else
    raptor_stringbuffer_append_string(sb, (const unsigned char*)"AND language IS NULL ", 1);

  if(datatype) {
    datatype_id = librdf_storage_sqlite_uri_helper(storage, datatype, add_new);
    raptor_stringbuffer_append_string(sb, (const unsigned char*)"AND datatype = ", 1);
    raptor_stringbuffer_append_decimal(sb, datatype_id);
  } else
    raptor_stringbuffer_append_string(sb, (const unsigned char*)"AND datatype IS NULL ", 1);
  
  expression = raptor_stringbuffer_as_string(sb);
  id = librdf_storage_sqlite_get_helper(storage, TABLE_LITERALS, expression);
  
  if(id >= 0 || !add_new)
    goto tidy;
  
  raptor_free_stringbuffer(sb);
  sb = raptor_new_stringbuffer();
  if(!sb) {
    id = -1;
    goto tidy;
  }

  raptor_stringbuffer_append_counted_string(sb, value_e, value_e_len, 1);

  raptor_stringbuffer_append_counted_string(sb, (const unsigned char*)", ", 2, 1);
  if(language_e)
    raptor_stringbuffer_append_counted_string(sb,language_e, language_e_len, 1);
  else
    raptor_stringbuffer_append_counted_string(sb, (const unsigned char*)"NULL", 4, 1);

  raptor_stringbuffer_append_counted_string(sb, (const unsigned char*)", ", 2, 1);
  if(datatype)
    raptor_stringbuffer_append_decimal(sb, datatype_id);
  else
    raptor_stringbuffer_append_counted_string(sb, (const unsigned char*)"NULL", 4, 1);

  expression = raptor_stringbuffer_as_string(sb);
  id = librdf_storage_sqlite_set_helper(storage, TABLE_LITERALS, expression,
                                      raptor_stringbuffer_length(sb));

  tidy:
  if(sb)
    raptor_free_stringbuffer(sb);
  if(value_e)
    LIBRDF_FREE(char*, value_e);
  if(language_e)
    LIBRDF_FREE(char*, language_e);
  
  return id;
}


static int
librdf_storage_sqlite_node_helper(librdf_storage* storage,
                                  librdf_node* node,
                                  int* id_p,
                                  triple_node_type *node_type_p,
                                  int add_new) 
{
  int id;
  triple_node_type node_type;
  unsigned char *value;
  size_t value_len;

  if(!node)
    return 1;
  
  switch(librdf_node_get_type(node)) {
    case LIBRDF_NODE_TYPE_RESOURCE:
      id = librdf_storage_sqlite_uri_helper(storage,
                                            librdf_node_get_uri(node),
                                            add_new);
      if(id < 0 && add_new)
        return 1;

      node_type = TRIPLE_URI;
      break;

    case LIBRDF_NODE_TYPE_LITERAL:
      value = librdf_node_get_literal_value_as_counted_string(node, &value_len);
      id = librdf_storage_sqlite_literal_helper(storage,
                                                value, value_len,
                                                librdf_node_get_literal_value_language(node),
                                                librdf_node_get_literal_value_datatype_uri(node),
                                                add_new);
      if(id < 0 && add_new)
        return 1;

      node_type = TRIPLE_LITERAL;
      break;

    case LIBRDF_NODE_TYPE_BLANK:
      id = librdf_storage_sqlite_blank_helper(storage,
                                              librdf_node_get_blank_identifier(node),
                                              add_new);
      if(id < 0 && add_new)
        return 1;

      node_type = TRIPLE_BLANK;
      break;

    case LIBRDF_NODE_TYPE_UNKNOWN:
    default:
      librdf_log(librdf_storage_get_world(storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Do not know how to store node type %d", node->type);
    return 1;
  }

  if(id_p)
    *id_p = id;
  if(node_type_p)
    *node_type_p = node_type;
  
  return 0;
}

                                        

static int
librdf_storage_sqlite_statement_helper(librdf_storage* storage,
                                       librdf_statement* statement,
                                       librdf_node* context_node,
                                       triple_node_type node_types[4],
                                       int node_ids[4],
                                       const unsigned char* fields[4],
                                       int add_new) 
{
  librdf_node* nodes[4];
  int i;
  
  nodes[0] = statement ? librdf_statement_get_subject(statement) : NULL;
  nodes[1] = statement ? librdf_statement_get_predicate(statement) : NULL;
  nodes[2] = statement ? librdf_statement_get_object(statement) : NULL;
  nodes[3] = context_node;
    
  for(i = 0; i < 4; i++) {
    if(!nodes[i]) {
      fields[i] = NULL;
      node_ids[i] = -1;
      node_types[i] = TRIPLE_NONE;
      continue;
    }
    
    if(librdf_storage_sqlite_node_helper(storage,
                                         nodes[i],
                                         &node_ids[i],
                                         &node_types[i],
                                         add_new))
      return 1;

    fields[i] = (const unsigned char*)triples_fields[i][node_types[i]];
  }

  return 0;
}


static int
librdf_storage_sqlite_open(librdf_storage* storage, librdf_model* model)
{
  librdf_storage_sqlite_instance* context;
  int rc = SQLITE_OK;
  char *errmsg = NULL;
  int db_file_exists = 0;
  
  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  if(!access((const char*)context->name, F_OK))
    db_file_exists = 1;

  if(context->is_new && db_file_exists)
    unlink(context->name);

  context->db = NULL;
  rc = sqlite3_open(context->name, &context->db);
  if(rc != SQLITE_OK)
    errmsg = (char*)sqlite3_errmsg(context->db);

  if(rc != SQLITE_OK) {
    librdf_log(librdf_storage_get_world(storage), 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s open failed - %s", 
               context->name, errmsg);
    librdf_storage_sqlite_close(storage);
    return 1;
  }

  
  if(context->synchronous >= 0) {
    raptor_stringbuffer *sb;
    unsigned char *request;

    sb = raptor_new_stringbuffer();
    if(!sb) {
      librdf_storage_sqlite_close(storage);
      return 1;
    }

    raptor_stringbuffer_append_string(sb, 
                                      (const unsigned char*)"PRAGMA synchronous=", 1);
    raptor_stringbuffer_append_string(sb, 
                                      (const unsigned char*)sqlite_synchronous_flags[context->synchronous], 1);
    raptor_stringbuffer_append_counted_string(sb, (const unsigned char*)";", 1, 1);
    
    request = raptor_stringbuffer_as_string(sb);

    rc = librdf_storage_sqlite_exec(storage, 
                                    request,
                                    NULL, NULL, 0);
    raptor_free_stringbuffer(sb);
    if(rc) {
      librdf_storage_sqlite_close(storage);
      return 1;
    }
  }

  
  if(context->is_new) {
    int i;
    unsigned char request[200];
    int begin;

    begin = librdf_storage_sqlite_transaction_start(storage);

    for(i = 0; i < NTABLES; i++) {

#if 0
      sprintf((char*)request, "DROP TABLE %s;", sqlite_tables[i].name);
      librdf_storage_sqlite_exec(storage,
                                 request,
                                 NULL, /* no callback */
                                 NULL, /* arg */
                                 1); /* don't care if this fails */
#endif

      sprintf((char*)request, "CREATE TABLE %s (%s);",
              sqlite_tables[i].name, sqlite_tables[i].schema);
      
      if(librdf_storage_sqlite_exec(storage,
                                    request,
                                    NULL, /* no callback */
                                    NULL, /* arg */
                                    0)) {
        if(!begin)
          librdf_storage_sqlite_transaction_rollback(storage);
        librdf_storage_sqlite_close(storage);
        return 1;
      }

    } /* end drop/create table loop */

    strcpy((char*)request, 
           "CREATE INDEX spindex ON triples (subjectUri, subjectBlank, predicateUri);");
    if(librdf_storage_sqlite_exec(storage,
                                  request,
                                  NULL, /* no callback */
                                  NULL, /* arg */
                                  0)) {
      if(!begin)
        librdf_storage_sqlite_transaction_rollback(storage);
      librdf_storage_sqlite_close(storage);
      return 1;
    }
    
    strcpy((char*)request, 
           "CREATE INDEX uriindex ON uris (uri);");
    if(librdf_storage_sqlite_exec(storage,
                                  request,
                                  NULL, /* no callback */
                                  NULL, /* arg */
                                  0)) {
      if(!begin)
        librdf_storage_sqlite_transaction_rollback(storage);
      librdf_storage_sqlite_close(storage);
      return 1;
    }
    
    if(!begin)
      librdf_storage_sqlite_transaction_commit(storage);    
  } /* end if is new */

  return 0;
}


/**
 * librdf_storage_sqlite_close:
 * @storage: the storage
 *
 * Close the sqlite storage.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_sqlite_close(librdf_storage* storage)
{
  librdf_storage_sqlite_instance* context;
  int status = 0;
  
  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  if(context->db) {
    sqlite3_close(context->db);
    context->db = NULL;
  }

  return status;
}


static int
librdf_storage_sqlite_size(librdf_storage* storage)
{
  int count = 0;
  
  if(librdf_storage_sqlite_exec(storage,
                                (unsigned char*)"SELECT COUNT(*) FROM triples;",
                                librdf_storage_sqlite_get_1int_callback,
                                &count,
                                0))
    return -1;

  return count;
}


static int
librdf_storage_sqlite_add_statement(librdf_storage* storage, 
                                    librdf_statement* statement)
{
  return librdf_storage_sqlite_context_add_statement(storage, NULL, statement);
}


static int
librdf_storage_sqlite_add_statements(librdf_storage* storage,
                                     librdf_stream* statement_stream)
{
  /*librdf_storage_sqlite_instance* context;*/
  int status = 0;
  int begin;

  /*context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);*/

  /* returns non-0 if a transaction is already active */
  begin = librdf_storage_sqlite_transaction_start(storage);

  for(; !librdf_stream_end(statement_stream);
      librdf_stream_next(statement_stream)) {
    librdf_statement* statement;
    librdf_node* context_node;
    triple_node_type node_types[4];
    int node_ids[4];
    const unsigned char* fields[4];
    raptor_stringbuffer *sb;
    int i;
    unsigned char* request;
    int rc;
    int max = 3;
    
    statement = librdf_stream_get_object(statement_stream);
    context_node = librdf_stream_get_context2(statement_stream);

    if(!statement) {
      status = 1;
      break;
    }

    /* Do not add duplicate statements */
    if(librdf_storage_sqlite_context_contains_statement(storage, context_node, statement))
      continue;

    if(librdf_storage_sqlite_statement_helper(storage,
                                              statement,
                                              context_node,
                                              node_types, node_ids, fields,
                                              1)) {
      if(!begin)
        librdf_storage_sqlite_transaction_rollback(storage);
      return -1;
    }
    
    if(context_node)
      max++;
    
    /* FIXME no context field used */
    sb = raptor_new_stringbuffer();
    if(!sb) {
      if(!begin)
        librdf_storage_sqlite_transaction_rollback(storage);
      return -1;
    }

    raptor_stringbuffer_append_string(sb,
                                      (unsigned char*)"INSERT INTO ", 1);
    raptor_stringbuffer_append_string(sb, 
                                      (unsigned char*)sqlite_tables[TABLE_TRIPLES].name, 1);
    raptor_stringbuffer_append_counted_string(sb, 
                                              (unsigned char*)" ( ", 3, 1);
    for(i = 0; i < max; i++) {
      raptor_stringbuffer_append_string(sb, fields[i], 1);
      if(i < (max-1))
        raptor_stringbuffer_append_counted_string(sb, 
                                                  (unsigned char*)", ", 2, 1);
    }
    
    raptor_stringbuffer_append_counted_string(sb, 
                                              (unsigned char*)") VALUES(", 9, 1);
    for(i = 0; i < max; i++) {
      raptor_stringbuffer_append_decimal(sb, node_ids[i]);
      if(i < (max-1))
        raptor_stringbuffer_append_counted_string(sb, 
                                                  (unsigned char*)", ", 2, 1);
    }
    raptor_stringbuffer_append_counted_string(sb, 
                                              (unsigned char*)");", 2, 1);
    
    request = raptor_stringbuffer_as_string(sb);

    rc = librdf_storage_sqlite_exec(storage,
                                    request,
                                    NULL, /* no callback */
                                    NULL, /* arg */
                                    0);

    raptor_free_stringbuffer(sb);

    if(rc) {
      if(!begin)
        librdf_storage_sqlite_transaction_rollback(storage);
      return 1;
    }

  }

  if(!begin)
    librdf_storage_sqlite_transaction_commit(storage);
  
  return status;
}


static int
librdf_storage_sqlite_remove_statement(librdf_storage* storage,
                                       librdf_statement* statement)
{
  return librdf_storage_sqlite_context_remove_statement(storage, NULL, 
                                                        statement);
}


static int
librdf_storage_sqlite_statement_operator_helper(librdf_storage* storage, 
                                                librdf_statement* statement,
                                                librdf_node* context_node,
                                                raptor_stringbuffer* sb,
                                                int add_new)
{
  /* librdf_storage_sqlite_instance* context; */
  triple_node_type node_types[4];
  int node_ids[4];
  const unsigned char* fields[4];
  int i;
  int need_and = 0;
  int max=3;
  
  /* context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage); */

  if(context_node)
    max++;
  
  if(librdf_storage_sqlite_statement_helper(storage,
                                            statement,
                                            context_node, 
                                            node_types, node_ids, fields,
                                            add_new))
    return 1;
  
  raptor_stringbuffer_append_counted_string(sb,
                                            (const unsigned char*)" FROM ", 6, 1);
  raptor_stringbuffer_append_string(sb,  
                                    (const unsigned char*)sqlite_tables[TABLE_TRIPLES].name, 1);
  raptor_stringbuffer_append_counted_string(sb,  
                                            (const unsigned char*)" WHERE ", 7, 1);
  
  for(i = 0; i < max; i++) {
    if(need_and)
      raptor_stringbuffer_append_counted_string(sb, 
                                                (unsigned char*)" AND ", 5, 1);
    raptor_stringbuffer_append_string(sb, fields[i], 1);
    raptor_stringbuffer_append_counted_string(sb,  
                                              (const unsigned char*)"=", 1, 1);
    raptor_stringbuffer_append_decimal(sb, node_ids[i]);
    
    need_and = 1;
  }
    
  return 0;
}


static int
librdf_storage_sqlite_contains_statement(librdf_storage* storage, 
                                         librdf_statement* statement)
{
  return librdf_storage_sqlite_context_contains_statement(storage, NULL, statement);
}


static int
librdf_storage_sqlite_context_contains_statement(librdf_storage* storage,
                                                 librdf_node* context_node,
                                                 librdf_statement* statement)
{
  raptor_stringbuffer *sb;
  unsigned char *request;
  int count = 0;
  int rc, begin;

  sb = raptor_new_stringbuffer();
  if(!sb)
    return -1;

  /* returns non-0 if a transaction is already active */
  begin = librdf_storage_sqlite_transaction_start(storage);

  raptor_stringbuffer_append_string(sb, 
                                    (const unsigned char*)"SELECT 1",
                                    1);

  if(librdf_storage_sqlite_statement_operator_helper(storage, statement, 
                                                     context_node, sb, 0)) {
    if(!begin)
      librdf_storage_sqlite_transaction_rollback(storage);
    raptor_free_stringbuffer(sb);
    return -1;
  }

  raptor_stringbuffer_append_string(sb, (const unsigned char*)" LIMIT 1;", 1);
  request = raptor_stringbuffer_as_string(sb);
  
  rc = librdf_storage_sqlite_exec(storage,
                                  request,
                                  librdf_storage_sqlite_get_1int_callback,
                                  &count,
                                  0);
  
  raptor_free_stringbuffer(sb);

  if(!begin)
    librdf_storage_transaction_commit(storage);

  if(rc)  
    return -1;

  return (count > 0);
}


static void
sqlite_construct_select_helper(raptor_stringbuffer* sb) 
{
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)"SELECT\n", 7, 1);

  /* If this order is changed MUST CHANGE order in 
   * librdf_storage_sqlite_get_next_common 
   */
  raptor_stringbuffer_append_string(sb, (unsigned char*)
"  SubjectURIs.uri     AS subjectUri,\n\
  SubjectBlanks.blank AS subjectBlank,\n\
  PredicateURIs.uri   AS predicateUri,\n\
  ObjectURIs.uri      AS objectUri,\n\
  ObjectBlanks.blank  AS objectBlank,\n\
  ObjectLiterals.text AS objectLiteralText,\n\
  ObjectLiterals.language AS objectLiteralLanguage,\n\
  ObjectLiterals.datatype AS objectLiteralDatatype,\n\
  ObjectDatatypeURIs.uri  AS objectLiteralDatatypeUri,\n\
  ContextURIs.uri         AS contextUri\n",
                                    1);
  
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)"FROM ", 5, 1);
  raptor_stringbuffer_append_string(sb, 
                                    (unsigned char*)sqlite_tables[TABLE_TRIPLES].name, 1);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)" AS T\n", 6, 1);
  
  raptor_stringbuffer_append_string(sb, (unsigned char*)
"  LEFT JOIN uris     AS SubjectURIs    ON SubjectURIs.id    = T.subjectUri\n\
  LEFT JOIN blanks   AS SubjectBlanks  ON SubjectBlanks.id  = T.subjectBlank\n\
  LEFT JOIN uris     AS PredicateURIs  ON PredicateURIs.id  = T.predicateUri\n\
  LEFT JOIN uris     AS ObjectURIs     ON ObjectURIs.id     = T.objectUri\n\
  LEFT JOIN blanks   AS ObjectBlanks   ON ObjectBlanks.id   = T.objectBlank\n\
  LEFT JOIN literals AS ObjectLiterals ON ObjectLiterals.id = T.objectLiteral\n\
  LEFT JOIN uris     AS ObjectDatatypeURIs ON ObjectDatatypeURIs.id = objectLiteralDatatype\n\
  LEFT JOIN uris     AS ContextURIs    ON ContextURIs.id     = T.contextUri\n",
                                    1);
}


typedef struct {
  librdf_storage *storage;
  librdf_storage_sqlite_instance* sqlite_context;

  int finished;

  librdf_statement *statement;
  librdf_node* context;

  /* OUT from sqlite3_prepare (V3) or sqlite_compile (V2) */
  sqlite3_stmt *vm;
  const char *zTail;
} librdf_storage_sqlite_serialise_stream_context;


static librdf_stream*
librdf_storage_sqlite_serialise(librdf_storage* storage)
{
  librdf_storage_sqlite_instance* context;
  librdf_storage_sqlite_serialise_stream_context* scontext;
  librdf_stream* stream;
  int status;
  char *errmsg = NULL;
  raptor_stringbuffer *sb;
  unsigned char *request;
  
  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  scontext = LIBRDF_CALLOC(librdf_storage_sqlite_serialise_stream_context*,
                           1, sizeof(*scontext));
  if(!scontext)
    return NULL;

  scontext->storage = storage;
  librdf_storage_add_reference(scontext->storage);

  scontext->sqlite_context = context;
  context->in_stream++;

  sb = raptor_new_stringbuffer();
  if(!sb) {
    librdf_storage_sqlite_serialise_finished((void*)scontext);
    return NULL;
  }

  sqlite_construct_select_helper(sb);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)";", 1, 1);

  request = raptor_stringbuffer_as_string(sb);
  if(!request) {
    raptor_free_stringbuffer(sb);
    librdf_storage_sqlite_serialise_finished((void*)scontext);
    return NULL;
  }

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
  LIBRDF_DEBUG2("SQLite prepare '%s'\n", request);
#endif

  status = sqlite3_prepare(context->db,
                           (const char*)request,
                           LIBRDF_GOOD_CAST(int, raptor_stringbuffer_length(sb)),
                           &scontext->vm,
                           &scontext->zTail);
  if(status != SQLITE_OK)
    errmsg = (char*)sqlite3_errmsg(context->db);

  raptor_free_stringbuffer(sb);

  if(status != SQLITE_OK) {
    librdf_log(librdf_storage_get_world(storage), 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s SQL compile failed - %s (%d)", 
               context->name, errmsg, status);

    librdf_storage_sqlite_serialise_finished((void*)scontext);
    return NULL;
  }
  
  stream = librdf_new_stream(librdf_storage_get_world(storage),
                             (void*)scontext,
                             &librdf_storage_sqlite_serialise_end_of_stream,
                             &librdf_storage_sqlite_serialise_next_statement,
                             &librdf_storage_sqlite_serialise_get_statement,
                             &librdf_storage_sqlite_serialise_finished);
  if(!stream) {
    librdf_storage_sqlite_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_storage_sqlite_get_next_common(librdf_storage_sqlite_instance* scontext,
                                      sqlite3_stmt *vm,
                                      librdf_statement **statement,
                                      librdf_node **context_node)
{
  int status = SQLITE_BUSY;
  int result = 0;
  
  /*
   * Each invocation of sqlite_step returns an integer code that
   * indicates what happened during that step. This code may be
   * SQLITE_BUSY, SQLITE_ROW, SQLITE_DONE, SQLITE_ERROR, or
   * SQLITE_MISUSE.
  */
  do {
    status = sqlite3_step(vm);
    if(status == SQLITE_BUSY) {
      /* FIXME - how to handle busy? */
      continue;
    }
    break;
  } while(1);

  if(status == SQLITE_ROW) {
    /* FIXME - turn row data into statement, scontext->context */
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
    int i;
#endif
    librdf_node* node;
    const unsigned char *uri_string, *blank;
    
/*
 * MUST MATCH fields order in query in librdf_storage_sqlite_serialise
 *
 i  field name (all TEXT unless given)
 0  subjectUri
 1  subjectBlank
 2  predicateUri
 3  objectUri
 4  objectBlank
 5  objectLiteralText
 6  objectLiteralLanguage
 7  objectLiteralDatatype (INTEGER)
 8  objectLiteralDatatypeUri
 9  contextUri
*/

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
    for(i = 0; i < sqlite3_column_count(vm); i++)
      fprintf(stderr, "%s, ", sqlite3_column_name(vm, i));
    fputc('\n', stderr);

    for(i = 0; i < sqlite3_column_count(vm); i++) {
      if(i == 7)
        fprintf(stderr, "%d, ", sqlite3_column_int(vm, i));
      else
        fprintf(stderr, "%s, ", sqlite3_column_text(vm, i));
    }
    fputc('\n', stderr);
#endif

    if(!*statement) {
      if(!(*statement = librdf_new_statement(librdf_storage_get_world(scontext->storage))))
        return 1;
    }

    librdf_statement_clear(*statement);

    /* subject */
    uri_string = sqlite3_column_text(vm, 0);
    if(uri_string)
      node = librdf_new_node_from_uri_string(librdf_storage_get_world(scontext->storage),
                                             uri_string);
    else {
      blank = sqlite3_column_text(vm, 1);
      node = librdf_new_node_from_blank_identifier(librdf_storage_get_world(scontext->storage),
                                                   blank);
    }
    if(!node)
      /* finished on error */
      return 1;

    librdf_statement_set_subject(*statement, node);


    uri_string = sqlite3_column_text(vm, 2);
    node = librdf_new_node_from_uri_string(librdf_storage_get_world(scontext->storage),
                                           uri_string);
    if(!node)
      /* finished on error */
      return 1;

    librdf_statement_set_predicate(*statement, node);

    uri_string = sqlite3_column_text(vm, 3);
    blank = sqlite3_column_text(vm, 4);
    if(uri_string)
      node = librdf_new_node_from_uri_string(librdf_storage_get_world(scontext->storage),
                                             uri_string);
    else if(blank)
      node = librdf_new_node_from_blank_identifier(librdf_storage_get_world(scontext->storage),
                                                   blank);
    else {
      const unsigned char *literal = sqlite3_column_text(vm, 5);
      const unsigned char *language = sqlite3_column_text(vm, 6);
      librdf_uri *datatype = NULL;
      
      /* int datatype_id= sqlite3_column_int(vm, 7); */
      uri_string = sqlite3_column_text(vm, 8);
      if(uri_string) {
        datatype = librdf_new_uri(librdf_storage_get_world(scontext->storage), uri_string);
        if(!datatype)
          /* finished on error */
          return 1;
      }
      
      node = librdf_new_node_from_typed_literal(librdf_storage_get_world(scontext->storage),
                                                literal, 
                                                (const char*)language,
                                                datatype);
      if(datatype)
        librdf_free_uri(datatype);
      
    }
    if(!node)
      /* finished on error */
      return 1;

    librdf_statement_set_object(*statement, node);

    uri_string = sqlite3_column_text(vm, 9);
    if(uri_string) {
      node = librdf_new_node_from_uri_string(librdf_storage_get_world(scontext->storage),
                                             uri_string);
      if(!node)
        /* finished on error */
        return 1;

      if(*context_node)
        librdf_free_node(*context_node);
      *context_node = node;
    }
  }

  if(status != SQLITE_ROW)
    result = 1;

  if(status == SQLITE_ERROR) {
    char *errmsg = NULL;

    status = sqlite3_finalize(vm);
    if(status != SQLITE_OK)
      errmsg = (char*)sqlite3_errmsg(scontext->db);

    if(status != SQLITE_OK) {
      librdf_log(librdf_storage_get_world(scontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s finalize failed - %s (%d)", 
                 scontext->name, errmsg, status);
    }
    result = -1;
  }
  
  return result;
}



static int
librdf_storage_sqlite_serialise_end_of_stream(void* context)
{
  librdf_storage_sqlite_serialise_stream_context* scontext;

  scontext = (librdf_storage_sqlite_serialise_stream_context*)context;
  
  if(scontext->finished)
    return 1;
  
  if(scontext->statement == NULL) {
    int result;

    result = librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                                   scontext->vm,
                                                   &scontext->statement,
                                                   &scontext->context);
    if(result) {
      /* error or finished */
      if(result < 0)
        scontext->vm = NULL;
      scontext->finished = 1;
    }
  }

  return scontext->finished;
}


static int
librdf_storage_sqlite_serialise_next_statement(void* context)
{
  librdf_storage_sqlite_serialise_stream_context* scontext;
  int result;
  
  scontext = (librdf_storage_sqlite_serialise_stream_context*)context;
  if(scontext->finished)
    return 1;
  
  result = librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                                 scontext->vm,
                                                 &scontext->statement,
                                                 &scontext->context);
  if(result) {
    /* error or finished */
    if(result < 0)
      scontext->vm = NULL;
    scontext->finished = 1;
  }

  return result;
}


static void*
librdf_storage_sqlite_serialise_get_statement(void* context, int flags)
{
  librdf_storage_sqlite_serialise_stream_context* scontext;

  scontext  = (librdf_storage_sqlite_serialise_stream_context*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->statement;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->context;

    default:
      librdf_log(librdf_storage_get_world(scontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d", flags);
      return NULL;
  }
}


static void
librdf_storage_sqlite_serialise_finished(void* context)
{
  librdf_storage_sqlite_serialise_stream_context* scontext;

  scontext = (librdf_storage_sqlite_serialise_stream_context*)context;

  if(scontext->vm) {
    char *errmsg = NULL;
    int status;
    
    status = sqlite3_finalize(scontext->vm);
    if(status != SQLITE_OK)
      errmsg = (char*)sqlite3_errmsg(scontext->sqlite_context->db);

    if(status != SQLITE_OK) {
      librdf_log(librdf_storage_get_world(scontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s finalize failed - %s (%d)", 
                 scontext->sqlite_context->name, errmsg, status);
    }
  }

  if(scontext->storage)
    librdf_storage_remove_reference(scontext->storage);

  if(scontext->statement)
    librdf_free_statement(scontext->statement);

  if(scontext->context)
    librdf_free_node(scontext->context);

  scontext->sqlite_context->in_stream--;
  if(!scontext->sqlite_context->in_stream)
    librdf_storage_sqlite_query_flush(scontext->storage);

  LIBRDF_FREE(librdf_storage_sqlite_serialise_stream_context, scontext);
}


typedef struct {
  librdf_storage *storage;
  librdf_storage_sqlite_instance* sqlite_context;

  int finished;

  librdf_statement *query_statement;

  librdf_statement *statement;
  librdf_node* context;

  /* OUT from sqlite3_prepare (V3) or sqlite_compile (V2) */
  sqlite3_stmt *vm;
  const char *zTail;
} librdf_storage_sqlite_find_statements_stream_context;


/**
 * librdf_storage_sqlite_find_statements:
 * @storage: the storage
 * @statement: the statement to match
 *
 * .
 * 
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 * Uses #librdf_statement_match to do the matching.
 * 
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_sqlite_find_statements(librdf_storage* storage,
                                      librdf_statement* statement)
{
  librdf_storage_sqlite_instance* context;
  librdf_storage_sqlite_find_statements_stream_context* scontext;
  librdf_stream* stream;
  unsigned char* request;
  int status;
  triple_node_type node_types[4];
  int node_ids[4];
  const unsigned char* fields[4];
  char *errmsg = NULL;
  raptor_stringbuffer *sb;
  int need_where = 1;
  int need_and = 0;
  int i;
  
  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  scontext = LIBRDF_CALLOC(librdf_storage_sqlite_find_statements_stream_context*,
                           1, sizeof(*scontext));
  if(!scontext)
    return NULL;

  scontext->storage = storage;
  librdf_storage_add_reference(scontext->storage);

  scontext->sqlite_context = context;
  context->in_stream++;

  scontext->query_statement = librdf_new_statement_from_statement(statement);
  if(!scontext->query_statement) {
    librdf_storage_sqlite_find_statements_finished((void*)scontext);
    return NULL;
  }

  if(librdf_storage_sqlite_statement_helper(storage,
                                            statement,
                                            NULL, 
                                            node_types, node_ids, fields,
                                            0)) {
    librdf_storage_sqlite_find_statements_finished((void*)scontext);
    return NULL;
  }

  sb = raptor_new_stringbuffer();
  if(!sb) {
    librdf_storage_sqlite_find_statements_finished((void*)scontext);
    return NULL;
  }

  sqlite_construct_select_helper(sb);

  for(i = 0; i < 3; i++) {
    if(node_types[i] == TRIPLE_NONE)
      continue;
    
    if(need_where) {
      raptor_stringbuffer_append_counted_string(sb, 
                                                (unsigned char*)" WHERE ", 7, 1);
      need_where = 0;
      need_and = 1;
    } else if(need_and)
      raptor_stringbuffer_append_counted_string(sb, 
                                                (unsigned char*)" AND ", 5, 1);
    raptor_stringbuffer_append_counted_string(sb, 
                                              (unsigned char*)"T.", 2, 1);
    raptor_stringbuffer_append_string(sb, fields[i], 1);
    raptor_stringbuffer_append_counted_string(sb, 
                                              (unsigned char*)"=", 1, 1);
    raptor_stringbuffer_append_decimal(sb, node_ids[i]);
    raptor_stringbuffer_append_counted_string(sb, 
                                              (unsigned char*)"\n", 1, 1);
  }
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)";", 1, 1);
  
  request = raptor_stringbuffer_as_string(sb);
  if(!request) {
    raptor_free_stringbuffer(sb);
    librdf_storage_sqlite_find_statements_finished((void*)scontext);
    return NULL;
  }

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
  LIBRDF_DEBUG2("SQLite prepare '%s'\n", request);
#endif

  status = sqlite3_prepare(context->db,
                           (const char*)request,
                           LIBRDF_GOOD_CAST(int, raptor_stringbuffer_length(sb)),
                           &scontext->vm,
                           &scontext->zTail);
  if(status != SQLITE_OK)
    errmsg = (char*)sqlite3_errmsg(context->db);

  raptor_free_stringbuffer(sb);

  if(status != SQLITE_OK) {
    librdf_log(librdf_storage_get_world(storage), 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s SQL compile '%s' failed - %s (%d)", 
               context->name, request, errmsg, status);

    librdf_storage_sqlite_find_statements_finished((void*)scontext);
    return NULL;
  }
  
  stream = librdf_new_stream(librdf_storage_get_world(storage),
                             (void*)scontext,
                             &librdf_storage_sqlite_find_statements_end_of_stream,
                             &librdf_storage_sqlite_find_statements_next_statement,
                             &librdf_storage_sqlite_find_statements_get_statement,
                             &librdf_storage_sqlite_find_statements_finished);
  if(!stream) {
    librdf_storage_sqlite_find_statements_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_storage_sqlite_find_statements_end_of_stream(void* context)
{
  librdf_storage_sqlite_find_statements_stream_context* scontext;

  scontext = (librdf_storage_sqlite_find_statements_stream_context*)context;
  
  if(scontext->finished)
    return 1;
  
  if(scontext->statement == NULL) {
    int result;
    result = librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                                   scontext->vm,
                                                   &scontext->statement,
                                                   &scontext->context);
    if(result) {
      /* error or finished */
      if(result < 0)
        scontext->vm = NULL;
      scontext->finished = 1;
    }
  }
  

  return scontext->finished;
}


static int
librdf_storage_sqlite_find_statements_next_statement(void* context)
{
  librdf_storage_sqlite_find_statements_stream_context* scontext;
  int result;
  
  scontext = (librdf_storage_sqlite_find_statements_stream_context*)context;

  if(scontext->finished)
    return 1;
  
  result = librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                                 scontext->vm,
                                                 &scontext->statement,
                                                 &scontext->context);
  if(result) {
    /* error or finished */
    if(result < 0)
      scontext->vm = NULL;
    scontext->finished = 1;
  }

  return result;
}


static void*
librdf_storage_sqlite_find_statements_get_statement(void* context, int flags)
{
  librdf_storage_sqlite_find_statements_stream_context* scontext;

  scontext = (librdf_storage_sqlite_find_statements_stream_context*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->statement;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->context;

    default:
      librdf_log(librdf_storage_get_world(scontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d", flags);
      return NULL;
  }
}


static void
librdf_storage_sqlite_find_statements_finished(void* context)
{
  librdf_storage_sqlite_find_statements_stream_context* scontext;

  scontext  = (librdf_storage_sqlite_find_statements_stream_context*)context;

  if(scontext->vm) {
    char *errmsg = NULL;
    int status;
    
    status = sqlite3_finalize(scontext->vm);
    if(status != SQLITE_OK)
      errmsg = (char*)sqlite3_errmsg(scontext->sqlite_context->db);

    if(status != SQLITE_OK) {
      librdf_log(librdf_storage_get_world(scontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s finalize failed - %s (%d)", 
                 scontext->sqlite_context->name, errmsg, status);
    }
  }

  if(scontext->storage)
    librdf_storage_remove_reference(scontext->storage);

  if(scontext->query_statement)
    librdf_free_statement(scontext->query_statement);

  if(scontext->statement)
    librdf_free_statement(scontext->statement);

  if(scontext->context)
    librdf_free_node(scontext->context);

  scontext->sqlite_context->in_stream--;
  if(!scontext->sqlite_context->in_stream)
    librdf_storage_sqlite_query_flush(scontext->storage);

  LIBRDF_FREE(librdf_storage_sqlite_find_statements_stream_context, scontext);
}


/**
 * librdf_storage_sqlite_context_add_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to add
 *
 * Add a statement to a storage context.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_sqlite_context_add_statement(librdf_storage* storage,
                                            librdf_node* context_node,
                                            librdf_statement* statement) 
{
  /* librdf_storage_sqlite_instance* context; */
  triple_node_type node_types[4];
  int node_ids[4];
  const unsigned char* fields[4];
  raptor_stringbuffer *sb;
  unsigned char* request;
  int i;
  int rc, begin;
  int max=3;

  /* Do not add duplicate statements */
  rc = librdf_storage_sqlite_context_contains_statement(storage, context_node, statement);
  if(rc != 0)
    return rc < 0 ? rc : 0; /* return error or 'found' */

  /* context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage); */

  sb = raptor_new_stringbuffer();
  if(!sb)
    return -1;

  /* returns non-0 if transaction is already active */
  begin = librdf_storage_sqlite_transaction_start(storage);

  if(librdf_storage_sqlite_statement_helper(storage,
                                            statement,
                                            context_node,
                                            node_types, node_ids, fields,
                                            1)) {

    if(!begin)
      librdf_storage_sqlite_transaction_rollback(storage);
    raptor_free_stringbuffer(sb);
    return -1;
  }
  
  if(context_node)
    max++;

  raptor_stringbuffer_append_string(sb, 
                                    (unsigned char*)"INSERT INTO ", 1);
  raptor_stringbuffer_append_string(sb, 
                                    (unsigned char*)sqlite_tables[TABLE_TRIPLES].name, 1);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)" ( ", 3, 1);
  for(i = 0; i < max; i++) {
    raptor_stringbuffer_append_string(sb, fields[i], 1);
    if(i < (max-1))
      raptor_stringbuffer_append_counted_string(sb, 
                                                (unsigned char*)", ", 2, 1);
  }
  
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)") VALUES(", 9, 1);
  for(i = 0; i < max; i++) {
    raptor_stringbuffer_append_decimal(sb, node_ids[i]);
    if(i < (max-1))
      raptor_stringbuffer_append_counted_string(sb, 
                                                (unsigned char*)", ", 2, 1);
  }
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)");", 2, 1);
  
  request = raptor_stringbuffer_as_string(sb);
  
  rc = librdf_storage_sqlite_exec(storage,
                                  request,
                                  NULL, /* no callback */
                                  NULL, /* arg */
                                  0);
  
  raptor_free_stringbuffer(sb);
  
  if(rc) {
    if(!begin)
      librdf_storage_transaction_rollback(storage);
    return rc;
  }

  if(!begin)
    librdf_storage_transaction_commit(storage);

  return 0;
}


/**
 * librdf_storage_sqlite_context_remove_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to remove
 *
 * Remove a statement from a storage context.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_sqlite_context_remove_statement(librdf_storage* storage, 
                                               librdf_node* context_node,
                                               librdf_statement* statement) 
{
  /* librdf_storage_sqlite_instance* context; */
  int rc;
  raptor_stringbuffer *sb;
  unsigned char *request;

  /* context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage); */

  sb = raptor_new_stringbuffer();
  if(!sb)
    return -1;

  raptor_stringbuffer_append_string(sb, (const unsigned char*)"DELETE", 1);
  if(librdf_storage_sqlite_statement_operator_helper(storage, statement,
                                                     context_node, sb, 0)) {
    raptor_free_stringbuffer(sb);
    return -1;
  }

  raptor_stringbuffer_append_counted_string(sb,
                                            (const unsigned char*)";", 1, 1);
 
  request = raptor_stringbuffer_as_string(sb);
  
  rc = librdf_storage_sqlite_exec(storage,
                                  request,
                                  NULL,
                                  NULL,
                                  0);
  
  raptor_free_stringbuffer(sb);

  return rc;
}


static  int
librdf_storage_sqlite_context_remove_statements(librdf_storage* storage, 
                                                librdf_node* context_node)
{
  triple_node_type node_types[4];
  int node_ids[4];
  const unsigned char* fields[4];
  raptor_stringbuffer *sb;
  unsigned char *request;
  int rc = 0;
  
  
  if(librdf_storage_sqlite_statement_helper(storage,
                                            NULL,
                                            context_node,
                                            node_types, node_ids, fields, 0))
    return -1;
    
  sb = raptor_new_stringbuffer();
  if(!sb)
    return -1;

  raptor_stringbuffer_append_counted_string(sb,
                                    (unsigned char*)"DELETE FROM ", 12, 1);
  raptor_stringbuffer_append_string(sb, 
                                    (unsigned char*)sqlite_tables[TABLE_TRIPLES].name, 1);

  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)" WHERE ", 7, 1);

  raptor_stringbuffer_append_string(sb, fields[TRIPLE_CONTEXT], 1);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)"=", 1, 1);
  raptor_stringbuffer_append_decimal(sb, node_ids[TRIPLE_CONTEXT]);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)"\n", 1, 1);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)";", 1, 1);
  
  request = raptor_stringbuffer_as_string(sb);

  rc = librdf_storage_sqlite_exec(storage,
                                  request,
                                  NULL, /* no callback */
                                  NULL, /* arg */
                                  0);

  raptor_free_stringbuffer(sb);

  if(rc)
    return -1;

  return 0;
}


typedef struct {
  librdf_storage *storage;
  librdf_storage_sqlite_instance* sqlite_context;

  int finished;

  librdf_node *context_node;

  librdf_statement *statement;
  librdf_node* context;

  /* OUT from sqlite3_prepare (V3) or sqlite_compile (V2) */
  sqlite3_stmt *vm;
  const char *zTail;
} librdf_storage_sqlite_context_serialise_stream_context;


/**
 * librdf_storage_sqlite_context_serialise:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * Sqlite all statements in a storage context.
 * 
 * Return value: #librdf_stream of statements or NULL on failure or context is empty
 **/
static librdf_stream*
librdf_storage_sqlite_context_serialise(librdf_storage* storage,
                                        librdf_node* context_node) 
{
  librdf_storage_sqlite_instance* context;
  librdf_storage_sqlite_context_serialise_stream_context* scontext;
  librdf_stream* stream;
  int status;
  char *errmsg = NULL;
  triple_node_type node_types[4];
  int node_ids[4];
  const unsigned char* fields[4];
  raptor_stringbuffer *sb;
  unsigned char *request;

  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  scontext = LIBRDF_CALLOC(librdf_storage_sqlite_context_serialise_stream_context*,
                           1, sizeof(*scontext));
  if(!scontext)
    return NULL;

  scontext->storage = storage;
  librdf_storage_add_reference(scontext->storage);

  scontext->sqlite_context = context;
  context->in_stream++;

  scontext->context_node = librdf_new_node_from_node(context_node);

  if(librdf_storage_sqlite_statement_helper(storage,
                                            NULL,
                                            scontext->context_node,
                                            node_types, node_ids, fields,
                                            0)) {
    librdf_storage_sqlite_context_serialise_finished((void*)scontext);
    return NULL;
  }

  sb = raptor_new_stringbuffer();
  if(!sb) {
    librdf_storage_sqlite_context_serialise_finished((void*)scontext);
    return NULL;
  }

  sqlite_construct_select_helper(sb);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)" WHERE ", 7, 1);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)"T.", 2, 1);
  raptor_stringbuffer_append_string(sb, fields[TRIPLE_CONTEXT], 1);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)"=", 1, 1);
  raptor_stringbuffer_append_decimal(sb, node_ids[TRIPLE_CONTEXT]);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)"\n", 1, 1);
  raptor_stringbuffer_append_counted_string(sb, 
                                            (unsigned char*)";", 1, 1);
  
  request = raptor_stringbuffer_as_string(sb);
  if(!request) {
    raptor_free_stringbuffer(sb);
    librdf_storage_sqlite_context_serialise_finished((void*)scontext);
    return NULL;
  }

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
  LIBRDF_DEBUG2("SQLite prepare '%s'\n", request);
#endif

  status = sqlite3_prepare(context->db,
                           (const char*)request,
                           LIBRDF_GOOD_CAST(int, raptor_stringbuffer_length(sb)),
                           &scontext->vm,
                           &scontext->zTail);
  if(status != SQLITE_OK)
    errmsg = (char*)sqlite3_errmsg(context->db);

  raptor_free_stringbuffer(sb);

  if(status != SQLITE_OK) {
    librdf_log(librdf_storage_get_world(storage), 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s SQL compile failed - %s (%d)", 
               context->name, errmsg, status);

    librdf_storage_sqlite_context_serialise_finished((void*)scontext);
    return NULL;
  }

  stream = librdf_new_stream(librdf_storage_get_world(storage),
                             (void*)scontext,
                             &librdf_storage_sqlite_context_serialise_end_of_stream,
                             &librdf_storage_sqlite_context_serialise_next_statement,
                             &librdf_storage_sqlite_context_serialise_get_statement,
                             &librdf_storage_sqlite_context_serialise_finished);
  if(!stream) {
    librdf_storage_sqlite_context_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_storage_sqlite_context_serialise_end_of_stream(void* context)
{
  librdf_storage_sqlite_context_serialise_stream_context* scontext;

  scontext = (librdf_storage_sqlite_context_serialise_stream_context*)context;
  
  if(scontext->finished)
    return 1;
  
  if(scontext->statement == NULL) {
    int result;

    result = librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                                   scontext->vm,
                                                   &scontext->statement,
                                                   &scontext->context);
    if(result) {
      /* error or finished */
      if(result < 0)
        scontext->vm = NULL;
      scontext->finished = 1;
    }
  }
  

  return scontext->finished;
}


static int
librdf_storage_sqlite_context_serialise_next_statement(void* context)
{
  librdf_storage_sqlite_context_serialise_stream_context* scontext;
  int result;
  
  scontext = (librdf_storage_sqlite_context_serialise_stream_context*)context;

  if(scontext->finished)
    return 1;
  
  result = librdf_storage_sqlite_get_next_common(scontext->sqlite_context,
                                                 scontext->vm,
                                                 &scontext->statement,
                                                 &scontext->context);
  if(result) {
    /* error or finished */
    if(result < 0)
      scontext->vm = NULL;
    scontext->finished = 1;
  }

  return result;
}


static void*
librdf_storage_sqlite_context_serialise_get_statement(void* context, int flags)
{
  librdf_storage_sqlite_context_serialise_stream_context* scontext;

  scontext = (librdf_storage_sqlite_context_serialise_stream_context*)context;
  
  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->statement;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->context;

    default:
      librdf_log(librdf_storage_get_world(scontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d", flags);
      return NULL;
  }
}


static void
librdf_storage_sqlite_context_serialise_finished(void* context)
{
  librdf_storage_sqlite_context_serialise_stream_context* scontext;

  scontext = (librdf_storage_sqlite_context_serialise_stream_context*)context;

  if(scontext->vm) {
    char *errmsg = NULL;
    int status;
    
    status = sqlite3_finalize(scontext->vm);
    if(status != SQLITE_OK)
      errmsg = (char*)sqlite3_errmsg(scontext->sqlite_context->db);

    if(status != SQLITE_OK) {
      librdf_log(librdf_storage_get_world(scontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s finalize failed - %s (%d)", 
                 scontext->sqlite_context->name, errmsg, status);
    }
  }

  if(scontext->storage)
    librdf_storage_remove_reference(scontext->storage);

  if(scontext->statement)
    librdf_free_statement(scontext->statement);

  if(scontext->context)
    librdf_free_node(scontext->context);

  if(scontext->context_node)
    librdf_free_node(scontext->context_node);

  scontext->sqlite_context->in_stream--;
  if(!scontext->sqlite_context->in_stream)
    librdf_storage_sqlite_query_flush(scontext->storage);

  LIBRDF_FREE(librdf_storage_sqlite_context_serialise_stream_context, scontext);
}



typedef struct {
  librdf_storage *storage;
  librdf_storage_sqlite_instance* sqlite_context;

  int finished;
  
  librdf_node *current;

  /* OUT from sqlite3_prepare (V3) or sqlite_compile (V2) */
  sqlite3_stmt *vm;
  const char *zTail;
} librdf_storage_sqlite_get_contexts_iterator_context;



static int
librdf_storage_sqlite_get_next_context_common(librdf_storage_sqlite_instance* scontext,
                                              sqlite3_stmt *vm,
                                              librdf_node **context_node)
{
  int status = SQLITE_BUSY;
  int result = 0;
  
  /*
   * Each invocation of sqlite_step returns an integer code that
   * indicates what happened during that step. This code may be
   * SQLITE_BUSY, SQLITE_ROW, SQLITE_DONE, SQLITE_ERROR, or
   * SQLITE_MISUSE.
  */
  do {
    status = sqlite3_step(vm);
    if(status == SQLITE_BUSY) {
      /* FIXME - how to handle busy? */
      continue;
    }
    break;
  } while(1);

  if(status == SQLITE_ROW) {
    /* Turns row data into scontext->context */
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
    int i;
#endif
    const unsigned char *uri_string;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
    for(i = 0; i < sqlite3_column_count(vm); i++)
      fprintf(stderr, "%s, ", sqlite3_column_name(vm, i));
    fputc('\n', stderr);

    for(i = 0; i < sqlite3_column_count(vm); i++) {
      if(i == 7)
        fprintf(stderr, "%d, ", sqlite3_column_int(vm, i));
      else
        fprintf(stderr, "%s, ", sqlite3_column_text(vm, i));
    }
    fputc('\n', stderr);
#endif

    uri_string = sqlite3_column_text(vm, 0);
    if(uri_string) {
      librdf_node *node;
      node = librdf_new_node_from_uri_string(librdf_storage_get_world(scontext->storage),
                                             uri_string);
      if(!node)
        /* finished on error */
        return 1;

      if(*context_node)
        librdf_free_node(*context_node);
      *context_node = node;
    }
  }

  if(status != SQLITE_ROW)
    result = 1;

  if(status == SQLITE_ERROR) {
    char *errmsg = NULL;

    status = sqlite3_finalize(vm);
    if(status != SQLITE_OK)
      errmsg = (char*)sqlite3_errmsg(scontext->db);

    if(status != SQLITE_OK) {
      librdf_log(librdf_storage_get_world(scontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s finalize failed - %s (%d)", 
                 scontext->name, errmsg, status);
    }
    result = -1;
  }
  
  return result;
}



static int
librdf_storage_sqlite_get_contexts_is_end(void* iterator)
{
  librdf_storage_sqlite_get_contexts_iterator_context* icontext;

  icontext = (librdf_storage_sqlite_get_contexts_iterator_context*)iterator;
  
  if(icontext->finished)
    return 1;

  if(!icontext->current) {
    int result;
    
    result = librdf_storage_sqlite_get_next_context_common(icontext->sqlite_context,
                                                         icontext->vm,
                                                         &icontext->current);
    if(result) {
      /* error or finished */
      if(result < 0)
        icontext->vm = NULL;
      icontext->finished = 1;
    }
  }
  

  return icontext->finished;
}


static int
librdf_storage_sqlite_get_contexts_next_method(void* iterator) 
{
  librdf_storage_sqlite_get_contexts_iterator_context* icontext;
  int result;
  
  icontext = (librdf_storage_sqlite_get_contexts_iterator_context*)iterator;

  if(icontext->finished)
    return 1;

  result = librdf_storage_sqlite_get_next_context_common(icontext->sqlite_context,
                                                         icontext->vm,
                                                         &icontext->current);
  if(result) {
    /* error or finished */
    if(result < 0)
      icontext->vm = NULL;
    icontext->finished = 1;
  }

  return result;
}


static void*
librdf_storage_sqlite_get_contexts_get_method(void* iterator, int flags) 
{
  librdf_storage_sqlite_get_contexts_iterator_context* icontext;
  void *result = NULL;
  
  icontext = (librdf_storage_sqlite_get_contexts_iterator_context*)iterator;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return icontext->current;
      break;

    case LIBRDF_ITERATOR_GET_METHOD_GET_KEY:
    case LIBRDF_ITERATOR_GET_METHOD_GET_VALUE:
      result = NULL;
      break;
      
    default:
      librdf_log(librdf_storage_get_world(icontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "Unknown iterator method flag %d", flags);
      result = NULL;
      break;
  }

  return result;
}


static void
librdf_storage_sqlite_get_contexts_finished(void* iterator) 
{
  librdf_storage_sqlite_get_contexts_iterator_context* icontext;

  icontext = (librdf_storage_sqlite_get_contexts_iterator_context*)iterator;

  if(icontext->vm) {
    char *errmsg = NULL;
    int status;
    
    status = sqlite3_finalize(icontext->vm);
    if(status != SQLITE_OK)
      errmsg = (char*)sqlite3_errmsg(icontext->sqlite_context->db);

    if(status != SQLITE_OK) {
      librdf_log(librdf_storage_get_world(icontext->storage),
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
                 "SQLite database %s finalize failed - %s (%d)", 
                 icontext->sqlite_context->name, errmsg, status);
    }
  }

  if(icontext->storage)
    librdf_storage_remove_reference(icontext->storage);

  if(icontext->current)
    librdf_free_node(icontext->current);

  LIBRDF_FREE(librdf_storage_sqlite_get_contexts_iterator_context, icontext);
}


/**
 * librdf_storage_sqlite_context_get_contexts:
 * @storage: #librdf_storage object
 *
 * Sqlite all context nodes in a storage.
 * 
 * Return value: #librdf_iterator of context_nodes or NULL on failure or no contexts
 **/
static librdf_iterator*
librdf_storage_sqlite_get_contexts(librdf_storage* storage) 
{
  librdf_storage_sqlite_instance* context;
  librdf_storage_sqlite_get_contexts_iterator_context* icontext;
  int status;
  char *errmsg = NULL;
  raptor_stringbuffer *sb;
  unsigned char *request;
  librdf_iterator* iterator;

  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  icontext = LIBRDF_CALLOC(librdf_storage_sqlite_get_contexts_iterator_context*,
                           1, sizeof(*icontext));
  if(!icontext)
    return NULL;

  icontext->sqlite_context = context;

  sb = raptor_new_stringbuffer();
  if(!sb) {
    LIBRDF_FREE(librdf_storage_sqlite_get_contexts_iterator_context, icontext);
    return NULL;
  }

  raptor_stringbuffer_append_string(sb, (unsigned char*)
                                    "SELECT DISTINCT uris.uri", 1);
  raptor_stringbuffer_append_counted_string(sb,
                                            (const unsigned char*)" FROM ", 6, 1);
  raptor_stringbuffer_append_string(sb,  
                                    (const unsigned char*)sqlite_tables[TABLE_TRIPLES].name, 1);
  raptor_stringbuffer_append_string(sb,
                                    (const unsigned char*)" LEFT JOIN uris ON uris.id = contextUri WHERE contextUri NOT NULL;", 1);

  request = raptor_stringbuffer_as_string(sb);
  if(!request) {
    raptor_free_stringbuffer(sb);
    LIBRDF_FREE(librdf_storage_sqlite_get_contexts_iterator_context, icontext);
    return NULL;
  }

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 2
  LIBRDF_DEBUG2("SQLite prepare '%s'\n", request);
#endif

  status = sqlite3_prepare(context->db,
                           (const char*)request,
                           LIBRDF_GOOD_CAST(int, raptor_stringbuffer_length(sb)),
                           &icontext->vm,
                           &icontext->zTail);
  if(status != SQLITE_OK)
    errmsg = (char*)sqlite3_errmsg(context->db);

  raptor_free_stringbuffer(sb);

  if(status != SQLITE_OK) {
    librdf_log(librdf_storage_get_world(storage), 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "SQLite database %s SQL compile failed - %s (%d)", 
               context->name, errmsg, status);

    librdf_storage_sqlite_get_contexts_finished((void*)icontext);
    return NULL;
  }
    
  icontext->storage = storage;
  librdf_storage_add_reference(icontext->storage);

  iterator = librdf_new_iterator(librdf_storage_get_world(storage),
                                 (void*)icontext,
                                 &librdf_storage_sqlite_get_contexts_is_end,
                                 &librdf_storage_sqlite_get_contexts_next_method,
                                 &librdf_storage_sqlite_get_contexts_get_method,
                                 &librdf_storage_sqlite_get_contexts_finished);
  if(!iterator)
    librdf_storage_sqlite_get_contexts_finished(icontext);
  return iterator;
}



/**
 * librdf_storage_sqlite_get_feature:
 * @storage: #librdf_storage object
 * @feature: #librdf_uri feature property
 *
 * Get the value of a storage feature.
 * 
 * Return value: #librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
static librdf_node*
librdf_storage_sqlite_get_feature(librdf_storage* storage, librdf_uri* feature)
{
  /* librdf_storage_sqlite_instance* scontext; */
  unsigned char *uri_string;

  /* scontext = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage); */

  if(!feature)
    return NULL;

  uri_string = librdf_uri_as_string(feature);
  if(!uri_string)
    return NULL;
  
  if(!strcmp((const char*)uri_string, LIBRDF_MODEL_FEATURE_CONTEXTS)) {
    return librdf_new_node_from_typed_literal(librdf_storage_get_world(storage),
                                              (const unsigned char*)"1",
                                              NULL, NULL);
  }

  return NULL;
}


/**
 * librdf_storage_sqlite_transaction_start:
 * @storage: #librdf_storage object
 *
 * Start a new transaction unless one is already active.
 * 
 * Return value: 0 if transaction successfully started, non-0 on error
 * (including a transaction already active)
 **/
static int
librdf_storage_sqlite_transaction_start(librdf_storage *storage)
{
  librdf_storage_sqlite_instance* context;
  int rc;
  
  context = (librdf_storage_sqlite_instance* )librdf_storage_get_instance(storage);

  if(context->in_transaction)
    return 1;

  rc = librdf_storage_sqlite_exec(storage,
                                  (unsigned char *)"BEGIN IMMEDIATE;",
                                  NULL, NULL, 0);
  if(!rc)
    context->in_transaction = 1;      
  
  return rc;
}


/**
 * librdf_storage_sqlite_transaction_commit:
 * @storage: #librdf_storage object
 *
 * Commit an active transaction.
 * 
 * Return value: 0 if transaction successfully committed, non-0 on error
 * (including no transaction active)
 **/
static int
librdf_storage_sqlite_transaction_commit(librdf_storage *storage)
{
  librdf_storage_sqlite_instance* context;
  int rc;

  context = (librdf_storage_sqlite_instance* )librdf_storage_get_instance(storage);

  if(!context->in_transaction)
    return 1;
    
  rc = librdf_storage_sqlite_exec(storage,
                                  (unsigned char *)"END;",
                                  NULL, NULL, 0);
  if(!rc)
    context->in_transaction = 0;

  return rc;
}


/**
 * librdf_storage_sqlite_transaction_rollback:
 * @storage: #librdf_storage object
 *
 * Roll back an active transaction.
 * 
 * Return value: 0 if transaction successfully committed, non-0 on error
 * (including no transaction active)
 **/
static int
librdf_storage_sqlite_transaction_rollback(librdf_storage *storage)
{
  librdf_storage_sqlite_instance* context;
  int rc;

  context = (librdf_storage_sqlite_instance* )librdf_storage_get_instance(storage);

  if(!context->in_transaction)
    return 1;

  rc = librdf_storage_sqlite_exec(storage,
                                  (unsigned char *)"ROLLBACK;",
                                  NULL, NULL, 0);
  if(!rc)
    context->in_transaction = 0;

  return rc;
}


static void
librdf_storage_sqlite_query_flush(librdf_storage *storage)
{
  librdf_storage_sqlite_query *query;
  librdf_storage_sqlite_instance* context;
  int begin;

  if(!storage)
    return;

  context = (librdf_storage_sqlite_instance*)librdf_storage_get_instance(storage);

  if(!context->in_stream_queries)
    return;

  /* returns non-0 if a transaction is already active */  
  begin = librdf_storage_sqlite_transaction_start(storage);

  while(context->in_stream_queries) {
    query = context->in_stream_queries;
    context->in_stream_queries = query->next;

    librdf_storage_sqlite_exec(storage, query->query, NULL, NULL, 0);

    LIBRDF_FREE(char*, query->query);
    LIBRDF_FREE(librdf_storage_sqlite_query, query);
  }

  if(!begin)
    librdf_storage_sqlite_transaction_commit(storage);
}


/** Local entry point for dynamically loaded storage module */
static void
librdf_storage_sqlite_register_factory(librdf_storage_factory *factory) 
{
  LIBRDF_ASSERT_CONDITION(!strcmp(factory->name, "sqlite"));

  factory->version            = LIBRDF_STORAGE_INTERFACE_VERSION;
  factory->init               = librdf_storage_sqlite_init;
  factory->terminate          = librdf_storage_sqlite_terminate;
  factory->open               = librdf_storage_sqlite_open;
  factory->close              = librdf_storage_sqlite_close;
  factory->size               = librdf_storage_sqlite_size;
  factory->add_statement      = librdf_storage_sqlite_add_statement;
  factory->add_statements     = librdf_storage_sqlite_add_statements;
  factory->remove_statement   = librdf_storage_sqlite_remove_statement;
  factory->contains_statement = librdf_storage_sqlite_contains_statement;
  factory->serialise          = librdf_storage_sqlite_serialise;
  factory->find_statements    = librdf_storage_sqlite_find_statements;
  factory->context_add_statement    = librdf_storage_sqlite_context_add_statement;
  factory->context_remove_statement = librdf_storage_sqlite_context_remove_statement;
  factory->context_remove_statements = librdf_storage_sqlite_context_remove_statements;
  factory->context_serialise        = librdf_storage_sqlite_context_serialise;
  factory->get_contexts             = librdf_storage_sqlite_get_contexts;
  factory->get_feature              = librdf_storage_sqlite_get_feature;
  factory->transaction_start        = librdf_storage_sqlite_transaction_start;
  factory->transaction_commit       = librdf_storage_sqlite_transaction_commit;
  factory->transaction_rollback     = librdf_storage_sqlite_transaction_rollback;
}

#ifdef MODULAR_LIBRDF

/** Entry point for dynamically loaded storage module */
void
librdf_storage_module_register_factory(librdf_world *world)
{
  librdf_storage_register_factory(world, "sqlite", "SQLite",
                                  &librdf_storage_sqlite_register_factory);
}

#else

/*
 * librdf_init_storage_sqlite:
 * @world: world object
 *
 * INTERNAL - Initialise the built-in storage_sqlite module.
 */
void
librdf_init_storage_sqlite(librdf_world *world)
{
  librdf_storage_register_factory(world, "sqlite", "SQLite",
                                  &librdf_storage_sqlite_register_factory);
}

#endif
