/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_internal.h - Redland RDF Application Framework internal API (header never shipped)
 *
 * Copyright (C) 2000-2008, David Beckett http://www.dajobe.org/
 * Copyright (C) 2000-2004, University of Bristol, UK http://www.bristol.ac.uk/
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


/* This file is never shipped outside the source tree and neither
 * are any of the included headers files.  It is only included
 * inside librdf.h when LIBRDF_INTERNAL is defined; when compiling
 * Redland.
 */

#ifndef RDF_INTERNAL_H
#define RDF_INTERNAL_H

#ifdef __cplusplus
#define REDLAND_EXTERN_C extern "C"
#else
#define REDLAND_EXTERN_C
#endif

/* Can be over-ridden or undefined in a config.h file or -Ddefine */
#ifndef REDLAND_INLINE
#define REDLAND_INLINE inline
#endif

/* error handling */
#ifdef LIBRDF_DEBUG
/* DEBUGGING TURNED ON */

/* Debugging messages */
#define LIBRDF_DEBUG1(msg) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, __func__); } while(0)
#define LIBRDF_DEBUG2(msg, arg1) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, __func__, arg1);} while(0)
#define LIBRDF_DEBUG3(msg, arg1, arg2) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, __func__, arg1, arg2);} while(0)
#define LIBRDF_DEBUG4(msg, arg1, arg2, arg3) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, __func__, arg1, arg2, arg3);} while(0)

#define SYSTEM_MALLOC(size)   malloc(size)
#define SYSTEM_FREE(ptr)   free(ptr)

#ifndef LIBRDF_ASSERT_DIE
#define LIBRDF_ASSERT_DIE abort();
#endif

#else
/* DEBUGGING TURNED OFF */

/* No debugging messages */
#define LIBRDF_DEBUG1(msg)
#define LIBRDF_DEBUG2(msg, arg1)
#define LIBRDF_DEBUG3(msg, arg1, arg2)
#define LIBRDF_DEBUG4(msg, arg1, arg2, arg3)

#define SYSTEM_MALLOC(size)   malloc(size)
#define SYSTEM_FREE(ptr)   free(ptr)

#ifndef LIBRDF_ASSERT_DIE
#define LIBRDF_ASSERT_DIE
#endif

#endif


#ifdef LIBRDF_ASSERT_MESSAGES
#define LIBRDF_ASSERT_REPORT(msg) fprintf(stderr, "%s:%d: (%s) assertion failed: " msg "\n", __FILE__, __LINE__, __func__);
#else
#define LIBRDF_ASSERT_REPORT(line)
#endif


#ifdef LIBRDF_ASSERT

#define LIBRDF_ASSERT_CONDITION(condition) do { \
  if(!condition) { \
    LIBRDF_ASSERT_REPORT("assertion " #condition " failed.") \
    LIBRDF_ASSERT_DIE \
    return; \
  } \
} while(0)

#define LIBRDF_ASSERT_RETURN(condition, msg, ret) do { \
  if(condition) { \
    LIBRDF_ASSERT_REPORT(msg) \
    LIBRDF_ASSERT_DIE \
    return ret; \
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
    return ret; \
  } \
} while(0)

#else

#define LIBRDF_ASSERT_CONDITION(condition)
#define LIBRDF_ASSERT_RETURN(condition, msg, ret) 
#define LIBRDF_ASSERT_OBJECT_POINTER_RETURN(pointer, type)
#define LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(pointer, type, ret)

#endif


/* for the memory allocation functions */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#undef HAVE_STDLIB_H
#endif

#define LIBRDF_MALLOC(type, size) (type)malloc(size)
#define LIBRDF_CALLOC(type, count, size) (type)calloc(count, size)
#define LIBRDF_FREE(type, ptr)   free(ptr)

/* Fatal errors - always happen */
#define LIBRDF_FATAL1(world, facility, message) librdf_fatal(world, facility, __FILE__, __LINE__ , __func__, message)


/* Safe casts: widening a value */
#define LIBRDF_GOOD_CAST(t, v) (t)(v)

/* Unsafe casts: narrowing a value */
#define LIBRDF_BAD_CAST(t, v) (t)(v)


#include <rdf_list.h>
#include <rdf_files.h>
#include <rdf_heuristics.h>

#endif
