/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_stream.c - RDF Statement Stream Implementation
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


#include <rdf_config.h>

#include <stdio.h>
#include <sys/types.h>

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_stream.h>


/* prototypes of local helper functions */
static librdf_statement* librdf_stream_get_next_mapped_statement(librdf_stream* stream);


librdf_stream*
librdf_new_stream(void* context,
		  int (*end_of_stream)(void*),
		  librdf_statement* (*next_statement)(void*),
		  void (*finished)(void*))
{
  librdf_stream* new_stream;
  
  new_stream=(librdf_stream*)LIBRDF_CALLOC(librdf_stream, 1, 
					   sizeof(librdf_stream));
  if(!new_stream)
    return NULL;


  new_stream->context=context;

  new_stream->end_of_stream=end_of_stream;
  new_stream->next_statement=next_statement;
  new_stream->finished=finished;

  new_stream->is_end_stream=0;
  
  return new_stream;
}


void
librdf_free_stream(librdf_stream* stream) 
{
  stream->finished(stream->context);
  LIBRDF_FREE(librdf_stream, stream);
}


/**
 * librdf_stream_get_next_mapped_element:
 * @stream: 
 * 
 * Helper function to get the next element subject to the user defined 
 * map function (as set by &librdf_stream_set_map ) or NULL
 * if the stream has ended.
 * 
 * Return value: the next
 **/
static librdf_statement*
librdf_stream_get_next_mapped_statement(librdf_stream* stream) 
{
  librdf_statement* statement=NULL;
  
  /* find next statement subject to map */
  while(!stream->end_of_stream(stream->context)) {
    statement=stream->next_statement(stream->context);
    /* apply the map to the statement  */
    statement=stream->map(stream->map_context, statement);
    /* found something, return it */
    if(statement)
      break;
  }
  return statement;
}


librdf_statement*
librdf_stream_next(librdf_stream* stream) 
{
  librdf_statement* statement;
  
  if(stream->is_end_stream)
    return NULL;

  /* simple case, no mapping so pass on */
  if(!stream->map)
    return stream->next_statement(stream->context);

  /* mapping from here */

  /* return stored element if there is one */
  if(stream->next) {
    statement=stream->next;
    stream->next=NULL;
    return statement;
  }

  /* else get a new one or NULL at end of list */
  stream->next=librdf_stream_get_next_mapped_statement(stream);
  if(!stream->next)
    stream->is_end_stream=1;
  
  return stream->next;
}


int
librdf_stream_end(librdf_stream* stream) 
{
  if(stream->is_end_stream)
    return 1;

  /* simple case, no mapping so pass on */
  if(!stream->map)
    return (stream->is_end_stream=stream->end_of_stream(stream->context));


  /* mapping from here */

  /* already have 1 stored item */
  if(stream->next)
    return 1;

  /* get next item subject to map or NULL if list ended */
  stream->next=librdf_stream_get_next_mapped_statement(stream);
  if(!stream->next)
    stream->is_end_stream=1;
  
  return (stream->next != NULL);
}


void
librdf_stream_set_map(librdf_stream* stream, 
		      librdf_statement* (*map)(void* context, librdf_statement* statement), 
		      void* map_context) 
{
  stream->map=map;
  stream->map_context=map_context;
}

