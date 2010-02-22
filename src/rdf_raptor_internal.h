/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_raptor_internal.h - librdf Raptor integration internals
 *
 * Copyright (C) 2008, David Beckett http://www.dajobe.org/
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



#ifndef LIBRDF_RAPTOR_INTERNAL_H
#define LIBRDF_RAPTOR_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

void librdf_init_raptor(librdf_world* world);
void librdf_finish_raptor(librdf_world* world);

/* raptor1 -> raptor2 migration */
/* use preprocessor macros to map raptor2 calls to raptor1 if raptor2 is not available */
#ifndef RAPTOR_V2_AVAILABLE

#define raptor_new_iostream_to_file_handle(world, fh) raptor_new_iostream_to_file_handle(fh)
#define raptor_new_iostream_to_string(world, string_p, len_p, malloc) raptor_new_iostream_to_string(string_p, len_p, malloc)
#define raptor_iostream_write_bytes(ptr, size, nmemb, iostr) raptor_iostream_write_bytes(iostr, ptr, size, nmemb)
#define raptor_iostream_write_byte(byte, iostr) raptor_iostream_write_byte(iostr, byte)
#define raptor_iostream_counted_string_write(string, len, iostr) raptor_iostream_write_counted_string(iostr, string, len)
#define raptor_iostream_string_write(string, iostr) raptor_iostream_write_string(iostr, string)

#define raptor_string_ntriples_write(str, len, delim, iostr) raptor_iostream_write_string_ntriples(iostr, str, len, delim)

#endif /* !RAPTOR_V2_AVAILABLE */


#ifdef __cplusplus
}
#endif

#endif
