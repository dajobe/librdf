/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_statement_common.c - RDF Triple (Statement) interface
 *
 * Copyright (C) 2000-2008, David Beckett http://www.dajobe.org/
 * Copyright (C) 2000-2005, University of Bristol, UK http://www.bristol.ac.uk/
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


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <redland.h>


#ifndef STANDALONE

/* class methods */

/**
 * librdf_init_statement:
 * @world: redland world object
 *
 * INTERNAL - Initialise the statement module.
 *
 **/
void
librdf_init_statement(librdf_world *world) 
{
}


/**
 * librdf_finish_statement:
 * @world: redland world object
 *
 * INTERNAL - Terminate the statement module.
 *
 **/
void
librdf_finish_statement(librdf_world *world) 
{
}



/**
 * librdf_statement_encode_parts2:
 * @world: redland world object
 * @statement: statement to serialise
 * @context_node: #librdf_node context node (can be NULL)
 * @buffer: the buffer to use
 * @length: buffer size
 * @fields: fields to encode
 *
 * Serialise parts of a statement into a buffer.
 * 
 * Encodes the given statement in the buffer, which must be of sufficient
 * size.  If buffer is NULL, no work is done but the size of buffer
 * required is returned.
 *
 * The fields values are or-ed combinations of:
 * #LIBRDF_STATEMENT_SUBJECT #LIBRDF_STATEMENT_PREDICATE
 * #LIBRDF_STATEMENT_OBJECT
 * or #LIBRDF_STATEMENT_ALL for subject,prdicate,object fields
 * 
 * If context_node is given, it is encoded also
 *
 * Return value: the number of bytes written or 0 on failure.
 **/
size_t
librdf_statement_encode_parts2(librdf_world* world,
                               librdf_statement* statement, 
                               librdf_node* context_node,
                               unsigned char *buffer, size_t length,
                               librdf_statement_part fields)
{
  size_t total_length=0;
  size_t node_len;
  unsigned char *p;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

  /* min size */
  if(buffer && length < 1)
    return 0;

  p=buffer;
  /* magic number 'x' */
  if(p) {
    *p++='x';
    length--;
  }
  total_length++;

  if((fields & LIBRDF_STATEMENT_SUBJECT) && statement->subject) {
    /* 's' + subject */
    if(p) {
      if(length < 1)
        return 0;
      *p++='s';
      length--;
    }
    total_length++;

    node_len=librdf_node_encode(statement->subject, p, length);
    if(!node_len)
      return 0;
    if(p) {
      p += node_len;
      length -= node_len;
    }
    
    
    total_length += node_len;
  }
  
  
  if((fields & LIBRDF_STATEMENT_PREDICATE) && statement->predicate) {
    /* 'p' + predicate */
    if(p) {
      if(length < 1)
        return 0;
      
      *p++='p';
      length--;
    }
    total_length++;

    node_len=librdf_node_encode(statement->predicate, p, length);
    if(!node_len)
      return 0;
    if(p) {
      p += node_len;
      length -= node_len;
    }
    
    total_length += node_len;
  }
  
  if((fields & LIBRDF_STATEMENT_OBJECT) && statement->object) {
    /* 'o' object */
    if(p) {
      if(length < 1)
        return 0;
      
      *p++='o';
      length--;
    }
    total_length++;

    node_len= librdf_node_encode(statement->object, p, length);
    if(!node_len)
      return 0;
    if(p) {
      p += node_len;
      length -= node_len;
    }

    total_length += node_len;
  }

  if(context_node) {
    /* 'o' object */
    if(p) {
      *p++='c';
      length--;
    }
    total_length++;

    node_len= librdf_node_encode(context_node, p, length);
    if(!node_len)
      return 0;

    /* p and length changes never needed to be calculated [clang] */
    /*
    if(p) {
      p += node_len;
      length -= node_len;
    }
    */

    total_length += node_len;
  }

  return total_length;
}

#endif
