/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * redland.h - Redland RDF Application Framework main header
 *
 * $Id$
 *
 * Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
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


#ifndef LIBRDF_H
#define LIBRDF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#  ifdef REDLAND_INTERNAL
#    define REDLAND_API _declspec(dllexport)
#  else
#    define REDLAND_API _declspec(dllimport)
#  endif
#else
#  define REDLAND_API
#endif

/* Use gcc 3.1+ feature to allow marking of deprecated API calls.
 * This gives a warning during compiling.
 */
#if ( __GNUC__ == 3 && __GNUC_MINOR__ > 0 ) || __GNUC__ > 3
#ifdef __APPLE_CC__
/* OSX gcc cpp-precomp is broken */
#define REDLAND_DEPRECATED
#else
#define REDLAND_DEPRECATED __attribute__((deprecated))
#endif
#else
#define REDLAND_DEPRECATED
#endif


/* forward references to private structures */
typedef struct librdf_world_s librdf_world;
typedef struct librdf_hash_s librdf_hash;
typedef struct librdf_hash_cursor_s librdf_hash_cursor;
typedef struct librdf_digest_s librdf_digest;
typedef struct librdf_digest_factory_s librdf_digest_factory;
typedef struct librdf_uri_s librdf_uri;
typedef struct librdf_list_s librdf_list;
typedef struct librdf_iterator_s librdf_iterator;
typedef struct librdf_node_s librdf_node;
typedef struct librdf_statement_s librdf_statement;
typedef struct librdf_model_s librdf_model;
typedef struct librdf_model_factory_s librdf_model_factory;
typedef struct librdf_storage_s librdf_storage;
typedef struct librdf_storage_factory_s librdf_storage_factory;
typedef struct librdf_stream_s librdf_stream;
typedef struct librdf_parser_s librdf_parser;
typedef struct librdf_parser_factory_s librdf_parser_factory;
typedef struct librdf_query_s librdf_query;
typedef struct librdf_query_factory_s librdf_query_factory;
typedef struct librdf_serializer_s librdf_serializer;
typedef struct librdf_serializer_factory_s librdf_serializer_factory;

/* Public statics */
extern const char * const librdf_short_copyright_string;
extern const char * const librdf_copyright_string;
extern const char * const librdf_version_string;
extern const unsigned int librdf_version_major;
extern const unsigned int librdf_version_minor;
extern const unsigned int librdf_version_release;
extern const unsigned int librdf_version_decimal;

/* error handling */
#ifdef LIBRDF_DEBUG
/* DEBUGGING TURNED ON */

/* Debugging messages */
#define LIBRDF_DEBUG1(function, msg) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function); } while(0)
#define LIBRDF_DEBUG2(function, msg, arg1) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1);} while(0)
#define LIBRDF_DEBUG3(function, msg, arg1, arg2) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2);} while(0)
#define LIBRDF_DEBUG4(function, msg, arg1, arg2, arg3) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2, arg3);} while(0)

#define LIBRDF_ERROR1(world, function, msg) do {fprintf(stderr, "%s:%d:%s: error: " msg, __FILE__, __LINE__ , #function); abort();} while(0)
#define LIBRDF_ERROR2(world, function, msg,arg) do {fprintf(stderr, "%s:%d:%s: error: " msg, __FILE__, __LINE__ , #function, arg); abort();} while(0)

#if defined(HAVE_DMALLOC_H) && defined(LIBRDF_MEMORY_DEBUG_DMALLOC)
void* librdf_system_malloc(size_t size);
void librdf_system_free(void *ptr);
#define SYSTEM_MALLOC(size)   librdf_system_malloc(size)
#define SYSTEM_FREE(ptr)   librdf_system_free(ptr)
#else
#define SYSTEM_MALLOC(size)   malloc(size)
#define SYSTEM_FREE(ptr)   free(ptr)
#endif


#else
/* DEBUGGING TURNED OFF */

/* No debugging messages */
#define LIBRDF_DEBUG1(function, msg)
#define LIBRDF_DEBUG2(function, msg, arg1)
#define LIBRDF_DEBUG3(function, msg, arg1, arg2)
#define LIBRDF_DEBUG4(function, msg, arg1, arg2, arg3)

#define LIBRDF_ERROR1(world, function, msg) librdf_error(world, "%s:%d:%s: error: " msg, __FILE__, __LINE__ , #function)
#define LIBRDF_ERROR2(world, function, msg, arg) librdf_error(world, "%s:%d:%s: error: " msg, __FILE__, __LINE__ , #function, arg)

#define SYSTEM_MALLOC(size)   malloc(size)
#define SYSTEM_FREE(ptr)   free(ptr)

#endif


/* for the memory allocation functions */
#if defined(HAVE_DMALLOC_H) && defined(LIBRDF_MEMORY_DEBUG_DMALLOC)
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#undef HAVE_STDLIB_H
#endif
#include <dmalloc.h>
#endif


#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#undef HAVE_STDLIB_H
#endif

#define LIBRDF_MALLOC(type, size) malloc(size)
#define LIBRDF_CALLOC(type, size, count) calloc(size, count)
#define LIBRDF_FREE(type, ptr)   free(ptr)


/* Fatal errors - always happen */
#define LIBRDF_FATAL1(world, function, msg) do {fprintf(stderr, "%s:%d:%s: fatal error: " msg "\n", __FILE__, __LINE__ , #function); abort();} while(0)
#define LIBRDF_FATAL2(world, function, msg, arg) do {fprintf(stderr, "%s:%d:%s: fatal error: " msg "\n", __FILE__, __LINE__ , #function, arg); abort();} while(0)


/* Required for va_list in error handler function registrations
 * which are in the public API
 */
#include <stdarg.h>


/* these includes should be replaced with automatically pulled
 * definitions from rdf_*.h headers 
 */

/* internal interfaces  */
#ifdef LIBRDF_INTERNAL

#include <rdf_list.h>
#include <rdf_hash.h>
#include <rdf_digest.h>
#include <rdf_files.h>
#include <rdf_heuristics.h>

#ifdef NEED_EXPAT_SOURCE
/* Define correct header define for internal expat sources since this
 * is determined after configure has done a header hunt
 */
#undef HAVE_EXPAT_H
#undef HAVE_XMLPARSE_H
/* Must change this when expat in source tree is changed to be newer one */
#define HAVE_XMLPARSE_H 1
#endif

#endif

/* public interfaces */
#include <rdf_init.h>
#include <rdf_iterator.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_concepts.h>
#include <rdf_statement.h>
#include <rdf_model.h>
#include <rdf_storage.h>
#include <rdf_parser.h>
#include <rdf_serializer.h>
#include <rdf_stream.h>
#include <rdf_query.h>


#ifdef __cplusplus
}
#endif

#endif
