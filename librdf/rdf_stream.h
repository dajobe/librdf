/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_stream.h - RDF Stream interface and definitions
 *
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */


#ifndef LIBRDF_STREAM_H
#define LIBRDF_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif


struct librdf_stream_s {
  void *context;
  int is_end_stream;
  
  /* Used when mapping */
  librdf_statement* next; /* stores next statement */
  void *map_context;      /* context to pass on to map */
  
  int (*end_of_stream)(void*);
  librdf_statement* (*next_statement)(void*);
  void (*finished)(void*);

  librdf_statement* (*map)(void *context, librdf_statement* statement);
};



/* constructor */

librdf_stream* librdf_new_stream(void* context, int (*end_of_stream)(void*), librdf_statement* (*next_statement)(void*), void (*finished)(void*));

/* destructor */

void librdf_free_stream(librdf_stream* stream);

/* methods */
int librdf_stream_end(librdf_stream* stream);

librdf_statement* librdf_stream_next(librdf_stream* stream);

void librdf_stream_set_map(librdf_stream* stream, librdf_statement* (*map)(void* context, librdf_statement* statement), void* map_context);


#ifdef __cplusplus
}
#endif

#endif
