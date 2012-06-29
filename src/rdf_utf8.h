/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_utf8.h - RDF UTF8 / Unicode chars helper routines Definition
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



#ifndef LIBRDF_UTF8_H
#define LIBRDF_UTF8_H

#ifdef __cplusplus
extern "C" {
#endif

#include <raptor2.h>

/**
 * librdf_unichar:
 *
 * Unicode codepoint.
 *
 */
typedef raptor_unichar librdf_unichar;

REDLAND_API REDLAND_DEPRECATED
int librdf_unicode_char_to_utf8(librdf_unichar c, unsigned char *output, int length);
REDLAND_API REDLAND_DEPRECATED
int librdf_utf8_to_unicode_char(librdf_unichar *output, const unsigned char *input, int length);
REDLAND_API
unsigned char* librdf_latin1_to_utf8_2(const unsigned char *input, size_t length, size_t *output_length);
REDLAND_API
unsigned char* librdf_utf8_to_latin1_2(const unsigned char *input, size_t length, unsigned char discard, size_t *output_length);
REDLAND_API REDLAND_DEPRECATED
unsigned char* librdf_utf8_to_latin1(const unsigned char *input, int length, int *output_length);
REDLAND_API REDLAND_DEPRECATED
unsigned char* librdf_latin1_to_utf8(const unsigned char *input, int length, int *output_length);
REDLAND_API
void librdf_utf8_print(const unsigned char *input, int length, FILE *stream);



#ifdef __cplusplus
}
#endif

#endif
