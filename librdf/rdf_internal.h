/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_internal.h - Redland RDF Application Framework internal API (header never shipped)
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


/* This file is never shipped outside the source tree and neither
 * are any of the included headers files.  It is only included
 * inside librdf.h when LIBRDF_INTERNAL is defined; when compiling
 * Redland.
 */

#ifndef RDF_INTERNAL_H
#define RDF_INTERNAL_H

/* error handling */
#ifdef LIBRDF_DEBUG
/* DEBUGGING TURNED ON */

/* Debugging messages */
#define LIBRDF_DEBUG1(msg) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, __func__); } while(0)
#define LIBRDF_DEBUG2(msg, arg1) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, __func__, arg1);} while(0)
#define LIBRDF_DEBUG3(msg, arg1, arg2) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, __func__, arg1, arg2);} while(0)
#define LIBRDF_DEBUG4(msg, arg1, arg2, arg3) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, __func__, arg1, arg2, arg3);} while(0)

#if defined(HAVE_DMALLOC_H) && defined(LIBRDF_MEMORY_DEBUG_DMALLOC)
void* librdf_system_malloc(size_t size);
void librdf_system_free(void *ptr);
#define SYSTEM_MALLOC(size)   librdf_system_malloc(size)
#define SYSTEM_FREE(ptr)   librdf_system_free(ptr)
#else
#define SYSTEM_MALLOC(size)   malloc(size)
#define SYSTEM_FREE(ptr)   free(ptr)
#endif

#define LIBRDF_ASSERT_DIE abort();

#else
/* DEBUGGING TURNED OFF */

/* No debugging messages */
#define LIBRDF_DEBUG1(msg)
#define LIBRDF_DEBUG2(msg, arg1)
#define LIBRDF_DEBUG3(msg, arg1, arg2)
#define LIBRDF_DEBUG4(msg, arg1, arg2, arg3)

#define SYSTEM_MALLOC(size)   malloc(size)
#define SYSTEM_FREE(ptr)   free(ptr)

#define LIBRDF_ASSERT_DIE

#endif


#ifdef LIBRDF_DISABLE_ASSERT_MESSAGES
#define LIBRDF_ASSERT_REPORT(line)
#else
#define LIBRDF_ASSERT_REPORT(msg) fprintf(stderr, "%s:%d: (%s) assertion failed: " msg "\n", __FILE__, __LINE__, __func__);
#endif


#ifdef LIBRDF_DISABLE_ASSERT

#define LIBRDF_ASSERT_RETURN(condition, msg, ret) 
#define LIBRDF_ASSERT_OBJECT_POINTER_RETURN(pointer, type)
#define LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(pointer, type, ret)

#else

#define LIBRDF_ASSERT_RETURN(condition, msg, ret) do { \
  if(condition) { \
    LIBRDF_ASSERT_REPORT(msg) \
    LIBRDF_ASSERT_DIE \
    return(ret); \
  } \
} while(0)

#define LIBRDF_ASSERT_OBJECT_POINTER_RETURN(pointer, type) do { \
  if(!pointer) { \
    LIBRDF_ASSERT_REPORT("object pointer of type " #type " is NULL.") \
    LIBRDF_ASSERT_DIE \
    return; \
  } \
} while(0)

#define LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(pointer, type, ret) do { \
  if(!pointer) { \
    LIBRDF_ASSERT_REPORT("object pointer of type " #type " is NULL.") \
    LIBRDF_ASSERT_DIE \
    return(ret); \
  } \
} while(0)

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
#define LIBRDF_FATAL1(world, facility, message) librdf_fatal(world, facility, __FILE__, __LINE__ , __func__, message)
#include <rdf_list.h>
#include <rdf_hash.h>
#include <rdf_digest.h>
#include <rdf_files.h>
#include <rdf_heuristics.h>

#endif
