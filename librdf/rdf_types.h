/*
 * RDF data types used by some bit-twiddling routines
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


#ifndef RDF_TYPES_H
#define RDF_TYPES_H

#include <sys/types.h>
#include <config.h>

#if u16 == MISSING
  #undef u16
  #if SIZEOF_UNSIGNED_SHORT == 2
    typedef unsigned short u16;
  #elif SIZEOF_UNSIGNED_INT == 2
    typedef unsigned int u16;
  #else
    #error u16 type not defined
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


#if byte == MISSING
  #undef byte
  #if SIZEOF_UNSIGNED_CHAR == 1
    typedef unsigned char byte;
  #else
    #error byte type not defined
  #endif
#endif


#endif
