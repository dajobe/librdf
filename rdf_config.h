/*
 * RDF System Configuration
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

#ifndef RDF_CONFIG_H
#define RDF_CONFIG_H

#ifdef HAVE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

#ifdef HAVE_INLINE
#define RDF_URI_INLINE yes
#endif

/* for the memory allocation functions below */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


#ifdef RDF_DEBUG
/* DEBUGGING TURNED ON */

#define RDF_MALLOC(type, size) malloc(size)
#define RDF_CALLOC(type, size, count) calloc(size, count)
#define RDF_FREE(type, ptr)   free(ptr)

/* Debugging messages */
#define RDF_DEBUG1(function, msg) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function); } while(0)
#define RDF_DEBUG2(function, msg, arg1) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1);} while(0)
#define RDF_DEBUG3(function, msg, arg1, arg2) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2);} while(0)
#define RDF_DEBUG4(function, msg, arg1, arg2, arg3) do {fprintf(stderr, "%s:%d:%s: " msg, __FILE__, __LINE__, #function, arg1, arg2, arg3);} while(0)

#else

/* DEBUGGING TURNED OFF */

/* Not RDF_DEBUG */
#define RDF_MALLOC(type, size) malloc(size)
#define RDF_CALLOC(type, size, count) calloc(size, count)
#define RDF_FREE(type, ptr)   free(ptr)

/* Debugging messages */
#define RDF_DEBUG1(function, msg)
#define RDF_DEBUG2(function, msg, arg1)
#define RDF_DEBUG3(function, msg, arg1, arg2)
#define RDF_DEBUG4(function, msg, arg1, arg2, arg3)

#endif


/* Fatal errors - always printed */
#define RDF_FATAL1(function, msg) do {fprintf(stderr, "%s:%d:%s: fatal error: " msg, __FILE__, __LINE__ , #function); exit(1);} while(0)
#define RDF_FATAL2(function, msg,arg) do {fprintf(stderr, "%s:%d:%s: fatal error: " msg, __FILE__, __LINE__ , #function, arg); exit(1);} while(0)

#endif

