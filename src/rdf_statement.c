/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_raptor_statement.c - RDF Statement Class using raptor_statement
 *
 * Copyright (C) 2010-2011, David Beckett http://www.dajobe.org/
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


static size_t
librdf_statement_encode_parts_internal(librdf_statement* statement, 
                                       librdf_node* context_node,
                                       unsigned char *buffer, size_t length,
                                       librdf_statement_part fields);

/* class methods */


/**
 * librdf_new_statement:
 * @world: redland world object
 *
 * Constructor - create a new empty #librdf_statement.
 * 
 * Return value: a new #librdf_statement or NULL on failure
 **/
librdf_statement*
librdf_new_statement(librdf_world *world) 
{
  librdf_world_open(world);

  return raptor_new_statement(world->raptor_world_ptr);
}


/**
 * librdf_new_statement_from_statement:
 * @statement: #librdf_statement to copy
 *
 * Copy constructor - create a new librdf_statement from an existing librdf_statement.
 * Creates a deep copy - changes to original statement nodes are not reflected in the copy.
 * 
 * Return value: a new #librdf_statement with copy or NULL on failure
 **/
librdf_statement*
librdf_new_statement_from_statement(librdf_statement* statement)
{
  raptor_term *subject = NULL;
  raptor_term *predicate = NULL;
  raptor_term *object = NULL;
  raptor_term *graph = NULL;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  if(!statement)
    return NULL;

  subject = raptor_term_copy(statement->subject);
  if(statement->subject && !subject)
    goto err;

  predicate = raptor_term_copy(statement->predicate);
  if(statement->predicate && !predicate)
    goto err;

  object = raptor_term_copy(statement->object);
  if(statement->object && !object)
    goto err;

  graph = raptor_term_copy(statement->graph);
  if(statement->graph && !graph)
    goto err;

  return raptor_new_statement_from_nodes(statement->world, subject, predicate, object, graph);

 err:
  if(graph)
    raptor_free_term(graph);
  if(object)
    raptor_free_term(object);
  if(predicate)
    raptor_free_term(predicate);
  if(subject)
    raptor_free_term(subject);
  return NULL;
}


/**
 * librdf_new_statement_from_statement2:
 * @statement: #librdf_statement to copy
 *
 * Copy constructor - create a new librdf_statement from an existing librdf_statement.
 * Creates a shallow copy - changes to original statement nodes are reflected in the copy.
 * 
 * Return value: a new #librdf_statement with copy or NULL on failure
 **/
librdf_statement*
librdf_new_statement_from_statement2(librdf_statement* statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  if(!statement)
    return NULL;
  
  return raptor_statement_copy(statement);
}


/**
 * librdf_new_statement_from_nodes:
 * @world: redland world object
 * @subject: #librdf_node
 * @predicate: #librdf_node
 * @object: #librdf_node
 *
 * Constructor - create a new #librdf_statement from existing #librdf_node objects.
 * 
 * The node objects become owned by the new statement (or freed on error).
 *
 * Return value: a new #librdf_statement with copy or NULL on failure
 **/
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


/**
 * librdf_statement_init:
 * @world: redland world object
 * @statement: #librdf_statement object
 *
 * Initialise a statically declared librdf_statement.
 * 
 * This MUST be called on a statically declared librdf_statement
 * to initialise it properly.  It is the responsibility of the
 * user of the statically allocated librdf_statement to deal
 * with deallocation of any statement parts (subject, predicate, object).
 **/
void
librdf_statement_init(librdf_world *world, librdf_statement *statement)
{
  librdf_world_open(world);

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  raptor_statement_init(statement, world->raptor_world_ptr);
}


/**
 * librdf_statement_clear:
 * @statement: #librdf_statement object
 *
 * Empty a librdf_statement of nodes.
 * 
 **/
void
librdf_statement_clear(librdf_statement *statement)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  raptor_statement_clear(statement);
}


/**
 * librdf_free_statement:
 * @statement: #librdf_statement object
 *
 * Destructor - destroy a #librdf_statement.
 * 
 **/
void
librdf_free_statement(librdf_statement* statement)
{
  if(!statement)
    return;
  
  raptor_free_statement(statement);
}



/* methods */

/**
 * librdf_statement_get_subject:
 * @statement: #librdf_statement object
 *
 * Get the statement subject.
 * 
 * This method returns a SHARED pointer to the subject which must
 * be copied by the caller if needed.
 * 
 * Return value: a pointer to the #librdf_node of the statement subject - 
 **/
librdf_node*
librdf_statement_get_subject(librdf_statement *statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  return statement->subject;
}


/**
 * librdf_statement_set_subject:
 * @statement: #librdf_statement object
 * @node: #librdf_node of subject
 *
 * Set the statement subject.
 * 
 * The subject passed in becomes owned by
 * the statement object and must not be used by the caller after this call.
 **/
void
librdf_statement_set_subject(librdf_statement *statement, librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  statement->subject = node;
}


/**
 * librdf_statement_get_predicate:
 * @statement: #librdf_statement object
 *
 * Get the statement predicate.
 * 
 * This method returns a SHARED pointer to the predicate which must
 * be copied by the caller if needed.
 * 
 * Return value: a pointer to the #librdf_node of the statement predicate - 
 **/
librdf_node*
librdf_statement_get_predicate(librdf_statement *statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  return statement->predicate;
}


/**
 * librdf_statement_set_predicate:
 * @statement: #librdf_statement object
 * @node: #librdf_node of predicate
 *
 * Set the statement predicate.
 *
 * The predicate passed in becomes owned by
 * the statement object and must not be used by the caller after this call.
 **/
void
librdf_statement_set_predicate(librdf_statement *statement, librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  statement->predicate = node;
}


/**
 * librdf_statement_get_object:
 * @statement: #librdf_statement object
 *
 * Get the statement object.
 * 
 * This method returns a SHARED pointer to the object which must
 * be copied by the caller if needed.
 * 
 * Return value: a pointer to the #librdf_node of the statement object - 
 **/
librdf_node*
librdf_statement_get_object(librdf_statement *statement) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, NULL);

  return statement->object;
}


/**
 * librdf_statement_set_object:
 * @statement: #librdf_statement object
 * @node: #librdf_node of object
 *
 * Set the statement object.
 * 
 * The object passed in becomes owned by
 * the statement object and must not be used by the caller after this call.
 **/
void
librdf_statement_set_object(librdf_statement *statement, librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(statement, librdf_statement);

  statement->object = node;
}


/**
 * librdf_statement_is_complete:
 * @statement: #librdf_statement object
 *
 * Check if statement is a complete and legal RDF triple.
 *
 * Checks that all subject, predicate, object fields are present
 * and they have the allowed node types.
 * 
 * Return value: non 0 if the statement is complete and legal
 **/
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
/**
 * librdf_statement_to_string:
 * @statement: the statement
 *
 * Format the librdf_statement as a string.
 * 
 * Formats the statement as a newly allocate string that must be freed by
 * the caller.
 * 
 * @Deprecated: Use librdf_statement_write() to write to
 * #raptor_iostream which can be made to write to a string.  Use a
 * #librdf_serializer to write proper syntax formats.
 *
 * Return value: the string or NULL on failure.
 **/
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
    raptor_free_memory(s);
    s = NULL;
  }

  return s;
}
#endif


/**
 * librdf_statement_write:
 * @statement: the statement
 * @iostr: raptor iostream to write to
 *
 * Write the statement to an iostream
 * 
 * This method is for debugging and the format of the output should
 * not be relied on.
 *
 * Return value: non-0 on failure
 **/
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



/**
 * librdf_statement_print:
 * @statement: the statement
 * @fh: file handle
 *
 * Pretty print the statement to a file descriptor.
 * 
 * This method is for debugging and the format of the output should
 * not be relied on.
 * 
 **/
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



/**
 * librdf_statement_equals:
 * @statement1: first #librdf_statement
 * @statement2: second #librdf_statement
 *
 * Check if two statements are equal.
 * 
 * Return value: non 0 if statements are equal
 **/
int
librdf_statement_equals(librdf_statement* statement1, 
                        librdf_statement* statement2)
{
  return raptor_statement_equals(statement1, statement2);
}


/**
 * librdf_statement_match:
 * @statement: statement
 * @partial_statement: statement with possible empty parts
 *
 * Match a statement against a 'partial' statement.
 * 
 * A partial statement is where some parts of the statement -
 * subject, predicate or object can be empty (NULL).
 * Empty parts match against any value, parts with values
 * must match exactly.  Node matching is done via librdf_node_equals()
 * 
 * Return value: non 0 on match
 **/
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


/**
 * librdf_statement_encode2:
 * @world: redland world
 * @statement: the statement to serialise
 * @buffer: the buffer to use
 * @length: buffer size
 *
 * Serialise a statement into a buffer.
 * 
 * Encodes the given statement in the buffer, which must be of sufficient
 * size.  If buffer is NULL, no work is done but the size of buffer
 * required is returned.
 * 
 * Return value: the number of bytes written or 0 on failure.
 **/
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

/**
 * librdf_statement_encode:
 * @statement: the statement to serialise
 * @buffer: the buffer to use
 * @length: buffer size
 *
 * Serialise a statement into a buffer.
 * 
 * Encodes the given statement in the buffer, which must be of sufficient
 * size.  If buffer is NULL, no work is done but the size of buffer
 * required is returned.
 * 
 * @Deprecated: Use librdf_statement_encode2()
 *
 * Return value: the number of bytes written or 0 on failure.
 **/
size_t
librdf_statement_encode(librdf_statement* statement, 
                        unsigned char *buffer, 
                        size_t length)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

  return librdf_statement_encode_parts_internal(statement, NULL,
                                                buffer, length,
                                                LIBRDF_STATEMENT_ALL);
}


static size_t
librdf_statement_encode_parts_internal(librdf_statement* statement, 
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


/**
 * librdf_statement_encode_parts:
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
 * @Deprecated: This will no longer be a public API
 * 
 * Return value: the number of bytes written or 0 on failure.
 **/
size_t
librdf_statement_encode_parts(librdf_statement* statement, 
                              librdf_node* context_node,
                              unsigned char *buffer, size_t length,
                              librdf_statement_part fields)
{
  return librdf_statement_encode_parts_internal(statement, context_node,
                                                buffer, length, fields);
}


/**
 * librdf_statement_decode:
 * @statement: the statement to deserialise into
 * @buffer: the buffer to use
 * @length: buffer size
 *
 * Decodes a statement from a buffer. (ALWAYS FAILS)
 * 
 * Used to decode the serialised statement as created by
 * librdf_statement_encode() from the given buffer.
 *
 * @Deprecated: Replaced by librdf_statement_decode2() which works.
 * 
 * Return value: 0 signifying failure
 **/
size_t
librdf_statement_decode(librdf_statement* statement, 
                        unsigned char *buffer, size_t length)
{
  return 0;
}


/**
 * librdf_statement_decode_parts:
 * @statement: the statement to deserialise into
 * @context_node: pointer to #librdf_node context_node to deserialise into
 * @buffer: the buffer to use
 * @length: buffer size
 *
 * Decodes a statement + context node from a buffer. (ALWAYS FAILS)
 * 
 * Used to decode the serialised statement as created by
 * librdf_statement_encode() from the given buffer.
 *
 * @Deprecated: Replaced by librdf_statement_decode2() which works.
 * 
 * Return value: 0 signifying failure
 **/
size_t
librdf_statement_decode_parts(librdf_statement* statement, 
                              librdf_node** context_node,
                              unsigned char *buffer, size_t length)
{
  return 0;
}


/**
 * librdf_statement_decode2:
 * @world: redland world
 * @statement: the statement to deserialise into
 * @context_node: pointer to #librdf_node context_node to deserialise into
 * @buffer: the buffer to use
 * @length: buffer size
 *
 * Decodes a statement + context node from a buffer.
 * 
 * Decodes the serialised statement (as created by librdf_statement_encode() )
 * from the given buffer.  If a context node is found and context_node is
 * not NULL, a pointer to the new #librdf_node is stored in *context_node.
 * 
 * Return value: number of bytes used or 0 on failure (bad encoding, allocation failure)
 **/
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
    
    if(!(node = librdf_node_decode(world, &node_len, p, length)))
      return 0;

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
      /* FIXME - report this or not? */
      return 0;
    }
  }

  return total_length;
}

#endif


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_statement *statement, *statement2;
  int size, size2;
  const char *program=librdf_basename((const char*)argv[0]);
  char *s, *buffer;
  librdf_world *world;
  raptor_iostream *iostr;

  world=librdf_new_world();
  librdf_world_open(world);

  iostr = raptor_new_iostream_to_file_handle(world->raptor_world_ptr, stdout);

  fprintf(stdout, "%s: Creating statement\n", program);
  statement=librdf_new_statement(world);

  fprintf(stdout, "%s: Empty statement: ", program);
  librdf_statement_write(statement, iostr);
  fputs("\n", stdout);

  librdf_statement_set_subject(statement, librdf_new_node_from_uri_string(world, (const unsigned char*)"http://purl.org/net/dajobe/"));
  librdf_statement_set_predicate(statement, librdf_new_node_from_uri_string(world, (const unsigned char*)"http://purl.org/dc/elements/1.1/#Creator"));
  librdf_statement_set_object(statement, librdf_new_node_from_literal(world, (const unsigned char*)"Dave Beckett", NULL, 0));

  fprintf(stdout, "%s: Resulting statement: ", program);
  librdf_statement_write(statement, iostr);
  fputs("\n", stdout);

  size = librdf_statement_encode2(world, statement, NULL, 0);
  fprintf(stdout, "%s: Encoding statement requires %d bytes\n", program, size);
  buffer = LIBRDF_MALLOC(char*, size);

  fprintf(stdout, "%s: Encoding statement in buffer\n", program);
  size2 = librdf_statement_encode2(world, statement,
                                   (unsigned char*)buffer, size);
  if(size2 != size) {
    fprintf(stdout, "%s: Encoding statement used %d bytes, expected it to use %d\n", program, size2, size);
    return(1);
  }
  
    
  fprintf(stdout, "%s: Creating new statement\n", program);
  statement2=librdf_new_statement(world);

  fprintf(stdout, "%s: Decoding statement from buffer\n", program);
  if(!librdf_statement_decode2(world, statement2, NULL,
                               (unsigned char*)buffer, size)) {
    fprintf(stdout, "%s: Decoding statement failed\n", program);
    return(1);
  }
  LIBRDF_FREE(char*, buffer);
   
  fprintf(stdout, "%s: New statement is: ", program);
  librdf_statement_write(statement, iostr);
  fputs("\n", stdout);
 
  
  fprintf(stdout, "%s: Freeing statements\n", program);
  librdf_free_statement(statement2);
  librdf_free_statement(statement);

  raptor_free_iostream(iostr);

  librdf_free_world(world);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
