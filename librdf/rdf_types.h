/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_types.h - RDF data types used by some bit-twiddling routines
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
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



#ifndef LIBRDF_TYPES_H
#define LIBRDF_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#if u64 == MISSING
  #undef u64
  #if SIZEOF_UNSIGNED_INT == 8
    typedef unsigned int u64;
  #elif SIZEOF_UNSIGNED_LONG == 8
    typedef unsigned long u64;
  #elif SIZEOF_UNSIGNED_LONG_LONG == 8
    typedef unsigned long long u64;
  #else
    #error u64 type not defined
  #endif
#endif

#if u32 == MISSING
  #undef u32
  #if SIZEOF_UNSIGNED_INT == 4
    typedef unsigned int u32;
  #elif SIZEOF_UNSIGNED_LONG == 4
    typedef unsigned long u32;
  #else
    #error u32 type not defined
  #endif
#endif


#if byte == MISSING
  #undef byte
  #if SIZEOF_UNSIGNED_CHAR == 1
    typedef unsigned char byte;
  #else
    #error byte type not defined
  #endif
#endif


#ifdef __cplusplus
}
#endif

#endif
