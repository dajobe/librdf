/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * librdf.h - librdf main header
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


#ifndef LIBRDF_H
#define LIBRDF_H

#ifdef __cplusplus
extern "C" {
#endif


/* forward references to private structures */
typedef struct librdf_hash_s librdf_hash;
typedef struct librdf_digest_s librdf_digest;
/* typedef struct librdf_uri_s librdf_uri; */
typedef struct librdf_list_s librdf_list;
typedef struct librdf_iterator_s librdf_iterator;
typedef struct librdf_node_s librdf_node;
typedef struct librdf_statement_s librdf_statement;
typedef struct librdf_model_s librdf_model;
typedef struct librdf_storage_s librdf_storage;
typedef struct librdf_stream_s librdf_stream;
typedef struct librdf_context_s librdf_context;
typedef struct librdf_parser_s librdf_parser;


#ifdef HAVE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

#ifdef HAVE_INLINE
#define LIBRDF_URI_INLINE yes
#endif


/* error handling */
#ifdef LIBRDF_DEBUG
/* DEBUGGING TURNED ON */

void* librdf_malloc(char *file, int line, char *type, size_t size);
void* librdf_calloc(char *file, int line, char *type, size_t nmemb, size_t size);
void librdf_free(char *file, int line, char *type, void *ptr);

#define LIBRDF_MALLOC(type, size) librdf_malloc(__FILE__, __LINE__, #type, size)
#define LIBRDF_CALLOC(type, size, count) librdf_calloc(__FILE__, __LINE__, #type, size, count)
#define LIBRDF_FREE(type, ptr) librdf_free(__FILE__, __LINE__, #type, ptr)

void librdf_memory_report(FILE *fh);

/* Debugging messages */
#define LIBRDF_DEBUG1(function, msg) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function); } while(0)
#define LIBRDF_DEBUG2(function, msg, arg1) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1);} while(0)
#define LIBRDF_DEBUG3(function, msg, arg1, arg2) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2);} while(0)
#define LIBRDF_DEBUG4(function, msg, arg1, arg2, arg3) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2, arg3);} while(0)

#else

/* for the memory allocation functions */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


/* DEBUGGING TURNED OFF */

#define LIBRDF_MALLOC(type, size) malloc(size)
#define LIBRDF_CALLOC(type, size, count) calloc(size, count)
#define LIBRDF_FREE(type, ptr)   free(ptr)

/* Debugging messages */
#define LIBRDF_DEBUG1(function, msg)
#define LIBRDF_DEBUG2(function, msg, arg1)
#define LIBRDF_DEBUG3(function, msg, arg1, arg2)
#define LIBRDF_DEBUG4(function, msg, arg1, arg2, arg3)

#endif


/* Fatal errors - always happen */
#define LIBRDF_FATAL1(function, msg) do {fprintf(stderr, "%s:%d:%s: fatal error: " msg, __FILE__, __LINE__ , #function); abort();} while(0)
#define LIBRDF_FATAL2(function, msg,arg) do {fprintf(stderr, "%s:%d:%s: fatal error: " msg, __FILE__, __LINE__ , #function, arg); abort();} while(0)


/* public interfaces */

/* from rdf_init.c */
void librdf_init_world(char *digest_factory_name);
void librdf_destroy_world(void);


/* the rest should be automatically pulled from rdf_*.h headers */

#include <rdf_list.h>
#include <rdf_iterator.h>
#include <rdf_uri.h>
#include <rdf_hash.h>
#include <rdf_digest.h>
#include <rdf_node.h>
#include <rdf_statement.h>
#include <rdf_model.h>
#include <rdf_storage.h>
#include <rdf_stream.h>
#include <rdf_context.h>
#include <rdf_parser.h>


#ifdef __cplusplus
}
#endif

#endif
