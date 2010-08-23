/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_raptor_statement.c - RDF Statement Class using raptor_statement
 *
 * Copyright (C) 2010, David Beckett http://www.dajobe.org/
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


/* class methods */


librdf_statement*
librdf_new_statement(librdf_world *world) 
{
  librdf_world_open(world);

  return raptor_new_statement(world->raptor_world_ptr);
}


librdf_statement*
librdf_new_statement_from_statement(librdf_statement* statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  if(!statement)
    return NULL;
  
  return raptor_statement_copy(statement);
}


librdf_statement*
librdf_new_statement_from_nodes(librdf_world *world, 
                                librdf_node* subject,
                                librdf_node* predicate,
                                librdf_node* object)
{
  librdf_world_open(world);

  return raptor_new_statement_from_nodes(world->raptor_world_ptr,
                                         subject, predicate, object, NULL);
}


void
librdf_statement_init(librdf_world *world, librdf_statement *statement)
{
  librdf_world_open(world);

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  raptor_statement_init(statement, world->raptor_world_ptr);
}


void
librdf_statement_clear(librdf_statement *statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  raptor_statement_clear(statement);
}


void
librdf_free_statement(librdf_statement* statement)
{
  raptor_free_statement(statement);
}



/* methods */

librdf_node*
librdf_statement_get_subject(librdf_statement *statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  return statement->subject;
}


void
librdf_statement_set_subject(librdf_statement *statement, librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  statement->subject = node;
}


librdf_node*
librdf_statement_get_predicate(librdf_statement *statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  return statement->predicate;
}


void
librdf_statement_set_predicate(librdf_statement *statement, librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  statement->predicate = node;
}


librdf_node*
librdf_statement_get_object(librdf_statement *statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  return statement->object;
}


void
librdf_statement_set_object(librdf_statement *statement, librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  statement->object = node;
}


int
librdf_statement_is_complete(librdf_statement *statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

  if(!statement->subject ||
     (!librdf_node_is_resource(statement->subject) && 
      !librdf_node_is_blank(statement->subject)))
    return 0;

  if(!statement->predicate || !librdf_node_is_resource(statement->predicate))
     return 0;

  if(!statement->object)
    return 0;

  return 1;
}


#ifndef REDLAND_DISABLE_DEPRECATED
unsigned char *
librdf_statement_to_string(librdf_statement *statement)
{
  raptor_iostream* iostr;
  unsigned char *s;
  int rc;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  iostr = raptor_new_iostream_to_string(statement->world,
                                        (void**)&s, NULL, malloc);
  if(!iostr)
    return NULL;
  
  rc = librdf_statement_write(statement, iostr);
  raptor_free_iostream(iostr);
  if(rc) {
    free(s);
    s = NULL;
  }

  return s;
}
#endif


int
librdf_statement_write(librdf_statement *statement, raptor_iostream *iostr) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  if(!statement)
    return 1;
  
  if(librdf_node_write(statement->subject, iostr))
    return 1;

  raptor_iostream_write_byte(' ', iostr);
  if(librdf_node_write(statement->predicate, iostr))
    return 1;

  raptor_iostream_write_byte(' ', iostr);
  if(librdf_node_write(statement->object, iostr))
    return 1;

  return 0;
}



void
librdf_statement_print(librdf_statement *statement, FILE *fh) 
{
  raptor_iostream *iostr;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  if(!statement)
    return;
  
  iostr = raptor_new_iostream_to_file_handle(statement->world, fh);
  if(!iostr)
    return;
  
  librdf_statement_write(statement, iostr);

  raptor_free_iostream(iostr);
}



int
librdf_statement_equals(librdf_statement* statement1, 
                        librdf_statement* statement2)
{
  return raptor_statement_equals(statement1, statement2);
}


int
librdf_statement_match(librdf_statement* statement, 
                       librdf_statement* partial_statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(partial_statement, librdf_statement, 0);

  if(partial_statement->subject &&
     !raptor_term_equals(statement->subject, partial_statement->subject))
      return 0;

  if(partial_statement->predicate &&
     !raptor_term_equals(statement->predicate, partial_statement->predicate))
      return 0;

  if(partial_statement->object &&
     !raptor_term_equals(statement->object, partial_statement->object))
      return 0;

  return 1;
}


size_t
librdf_statement_encode2(librdf_world *world,
                         librdf_statement* statement, 
                         unsigned char *buffer, 
                         size_t length)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

  return librdf_statement_encode_parts2(world, statement, NULL,
                                        buffer, length,
                                        LIBRDF_STATEMENT_ALL);
}

size_t
librdf_statement_encode(librdf_statement* statement, 
                        unsigned char *buffer, 
                        size_t length)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

  return librdf_statement_encode_parts(statement, NULL,
                                       buffer, length,
                                       LIBRDF_STATEMENT_ALL);
}


size_t
librdf_statement_encode_parts(librdf_statement* statement, 
                              librdf_node* context_node,
                              unsigned char *buffer, size_t length,
                              librdf_statement_part fields)
{
  size_t total_length = 0;
  size_t node_len;
  unsigned char *p;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

  /* min size */
  if(buffer && length < 1)
    return 0;

  p = buffer;
  /* magic number 'x' */
  if(p) {
    *p++ = 'x';
    length--;
  }
  total_length++;

  if((fields & LIBRDF_STATEMENT_SUBJECT) && statement->subject) {
    /* 's' + subject */
    if(p) {
      if(length < 1)
        return 0;
      *p++ = 's';
      length--;
    }
    total_length++;

    node_len = librdf_node_encode(statement->subject, p, length);
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
      
      *p++ = 'p';
      length--;
    }
    total_length++;

    node_len = librdf_node_encode(statement->predicate, p, length);
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
      
      *p++ = 'o';
      length--;
    }
    total_length++;

    node_len = librdf_node_encode(statement->object, p, length);
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
      *p++ = 'c';
      length--;
    }
    total_length++;

    node_len = librdf_node_encode(context_node, p, length);
    if(!node_len)
      return 0;
    if(p) {
      p += node_len;
      length -= node_len;
    }

    total_length += node_len;
  }

  return total_length;
}


size_t
librdf_statement_decode(librdf_statement* statement, 
                        unsigned char *buffer, size_t length)
{
  return 0;
}


size_t
librdf_statement_decode2(librdf_world* world,
                         librdf_statement* statement,
                         librdf_node** context_node,
                         unsigned char *buffer, size_t length)
{
  unsigned char *p;
  librdf_node* node;
  unsigned char type;
  size_t total_length = 0;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG2("Decoding buffer of %d bytes\n", length);
#endif


  /* absolute minimum - first byte is type */
  if(length < 1)
    return 0;

  p = buffer;
  if(*p++ != 'x')
    return 0;
  length--;
  total_length++;
  
  
  while(length > 0) {
    size_t node_len;
    
    type = *p++;
    length--;
    total_length++;

    if(!length)
      return 0;
    
#ifdef LIBRDF_USE_RAPTOR_STATEMENT
    if(!(node = librdf_node_decode(world, &node_len, p, length)))
      return 0;
#else
    if(!(node = librdf_node_decode(statement->world, &node_len, p, length)))
      return 0;
#endif

    p += node_len;
    length -= node_len;
    total_length += node_len;
    
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG3("Found type %c (%d bytes)\n", type, node_len);
#endif
  
    switch(type) {
    case 's': /* subject */
      statement->subject = node;
      break;
      
    case 'p': /* predicate */
      statement->predicate = node;
      break;
      
    case 'o': /* object */
      statement->object = node;
      break;

    case 'c': /* context */
      if(context_node)
        *context_node = node;
      else
        librdf_free_node(node);
      break;

    default:
#ifdef LIBRDF_USE_RAPTOR_STATEMENT
      /* FIXME - report this or not? */
#else
      librdf_log(statement->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STATEMENT, NULL,
                 "Illegal statement encoding '%c' seen", p[-1]);
#endif
      return 0;
    }
  }

  return total_length;
}
