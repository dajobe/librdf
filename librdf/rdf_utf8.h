/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_utf8.h - RDF UTF8 / Unicode chars helper routines Definition
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



#ifndef LIBRDF_UTF8_H
#define LIBRDF_UTF8_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rdf_types.h>


typedef u32 librdf_unichar;

int librdf_unicode_char_to_utf8(librdf_unichar c, byte *output, int length);
int librdf_utf8_to_unicode_char(librdf_unichar *output, const byte *input, int length);
byte* librdf_utf8_to_latin1(const byte *input, int length, int *output_length);
byte* librdf_latin1_to_utf8(const byte *input, int length, int *output_length);
void librdf_utf8_print(const byte *input, int length, FILE *stream);



#ifdef __cplusplus
}
#endif

#endif
