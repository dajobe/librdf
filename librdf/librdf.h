/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * redland.h - Redland RDF Application Framework public API
 *
 * $Id$
 *
 * Copyright (C) 2000-2004 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.bris.ac.uk/
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

#include <stdio.h>

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


/* Public typedefs (references to private structures) */
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
typedef struct librdf_query_results_s librdf_query_results;
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

/* Required for va_list in error handler function registrations
 * which are in the public API
 */
#include <stdarg.h>


/* internal interfaces  */
#ifdef LIBRDF_INTERNAL
#include <rdf_internal.h>
#endif

/* public interfaces */

/* FIXME: Should be replaced with automatically pulled
 * definitions from the listed rdf_*.h header files.
 */
#include <rdf_log.h>
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
