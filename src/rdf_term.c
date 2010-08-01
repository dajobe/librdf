/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_term.c - RDF Term (RDF URI, Literal, Blank Node) Interface
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
#include <stdlib.h> /* for abort() as used in errors */
#endif

#define RAPTOR_WORLD_DECLARED 1
#define RAPTOR_V2_AVAILABLE 1

#include <redland.h>
/* needed for utf8 functions and definition of 'byte' */
#include <rdf_utf8.h>

/**
 * librdf_init_node:
 * @world: redland world object
 *
 * INTERNAL - Initialise the node module.
 * 
 **/
void
librdf_init_node(librdf_world* world)
{
}


/**
 * librdf_finish_node:
 * @world: redland world object
 *
 * INTERNAL - Terminate the node module.
 *
 **/
void
librdf_finish_node(librdf_world* world)
{
}


/* constructors */

librdf_node*
librdf_new_node(librdf_world *world)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  return librdf_new_node_from_blank_identifier(world, (unsigned char*)NULL);
}


librdf_node*
librdf_new_node_from_uri_string(librdf_world *world,
                                const unsigned char *uri_string)
{
  raptor_uri* new_uri;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  new_uri = raptor_new_uri(world->raptor_world_ptr, uri_string);
  if(!new_uri)
    return NULL;
  
  return raptor_new_term_from_uri(world->raptor_world_ptr, new_uri);
}


librdf_node*
librdf_new_node_from_uri(librdf_world *world, librdf_uri *uri)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  return raptor_new_term_from_uri(world->raptor_world_ptr, uri);
}


librdf_node*
librdf_new_node_from_uri_local_name(librdf_world *world,
                                    librdf_uri *uri,
                                    const unsigned char *local_name)
{
  raptor_uri *new_uri;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(uri, raptor_uri, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(local_name, string, NULL);

  new_uri = raptor_new_uri_from_uri_local_name(world->raptor_world_ptr,
                                               uri, local_name);
  if(!new_uri)
    return NULL;

  return raptor_new_term_from_uri(world->raptor_world_ptr, new_uri);
}


librdf_node*
librdf_new_node_from_normalised_uri_string(librdf_world *world,
                                           const unsigned char *uri_string,
                                           librdf_uri *source_uri,
                                           librdf_uri *base_uri)
{
  librdf_uri* new_uri;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  new_uri = librdf_new_uri_normalised_to_base(uri_string,
                                              source_uri, base_uri);
  if(!new_uri)
    return NULL;

  return raptor_new_term_from_uri(world->raptor_world_ptr, new_uri);
}


librdf_node*
librdf_new_node_from_literal(librdf_world *world,
                             const unsigned char *string,
                             const char *xml_language,
                             int is_wf_xml)
{
  librdf_uri* datatype_uri;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  datatype_uri = (is_wf_xml ?  LIBRDF_RS_XMLLiteral_URI(world) : NULL);

  return raptor_new_term_from_literal(world->raptor_world_ptr,
                                      string, datatype_uri,
                                      (const unsigned char*)xml_language);
}


librdf_node*
librdf_new_node_from_typed_literal(librdf_world *world,
                                   const unsigned char *value,
                                   const char *xml_language,
                                   librdf_uri *datatype_uri)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  return raptor_new_term_from_literal(world->raptor_world_ptr,
                                      value, datatype_uri,
                                      (const unsigned char*)xml_language);
}


librdf_node*
librdf_new_node_from_typed_counted_literal(librdf_world *world,
                                           const unsigned char *value,
                                           size_t value_len,
                                           const char *xml_language,
                                           size_t xml_language_len,
                                           librdf_uri *datatype_uri)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  return raptor_new_term_from_counted_literal(world->raptor_world_ptr,
                                              value, value_len,
                                              datatype_uri,
                                              (const unsigned char*)xml_language,
                                              (unsigned char)xml_language_len);
}


librdf_node*
librdf_new_node_from_counted_blank_identifier(librdf_world *world,
                                              const unsigned char *identifier,
                                              size_t identifier_len)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  return raptor_new_term_from_counted_blank(world->raptor_world_ptr,
                                            identifier, identifier_len);
}


librdf_node*
librdf_new_node_from_blank_identifier(librdf_world *world,
                                      const unsigned char *identifier)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  return raptor_new_term_from_blank(world->raptor_world_ptr,
                                    identifier);
}


librdf_node*
librdf_new_node_from_node(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  return raptor_term_copy(node);
}


void
librdf_free_node(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(node, librdf_node);

  raptor_free_term(node);
}


librdf_uri*
librdf_node_get_uri(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_URI)
    return NULL;
  
  return node->value.uri;
}


librdf_node_type
librdf_node_get_type(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, LIBRDF_NODE_TYPE_UNKNOWN);

  return (librdf_node_type)node->type;
}


unsigned char*
librdf_node_get_literal_value(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  return node->value.literal.string;
}


unsigned char*
librdf_node_get_literal_value_as_counted_string(librdf_node *node,
                                                size_t *len_p)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  if(len_p)
    *len_p = node->value.literal.string_len;

  return node->value.literal.string;
}


char*
librdf_node_get_literal_value_as_latin1(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  if(!node->value.literal.string)
    return NULL;
  
  return (char*)librdf_utf8_to_latin1((const byte*)node->value.literal.string,
                                      node->value.literal.string_len, NULL);
}


char*
librdf_node_get_literal_value_language(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  return (char*)node->value.literal.language;
}


int
librdf_node_get_literal_value_is_wf_xml(librdf_node *node)
{
  raptor_uri* rdf_xml_literal_uri;
  int rc;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);

  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return 0;
  
  if(!node->value.literal.datatype)
    return 0;

  rdf_xml_literal_uri = raptor_new_uri_for_rdf_concept(node->world,
                                                       (const unsigned char *)"XMLLiteral");
  
  rc = librdf_uri_equals(node->value.literal.datatype, rdf_xml_literal_uri);
  raptor_free_uri(rdf_xml_literal_uri);

  return rc;
}


librdf_uri*
librdf_node_get_literal_value_datatype_uri(librdf_node *node)
{
  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  return node->value.literal.datatype;
}


int
librdf_node_get_li_ordinal(librdf_node *node)
{
  unsigned char *uri_string;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);

  if(node->type != RAPTOR_TERM_TYPE_URI)
    return -1;

  uri_string = raptor_uri_as_string(node->value.uri); 
  if(strncmp((const char*)uri_string,
             (const char*)"http://www.w3.org/1999/02/22-rdf-syntax-ns#_", 44))
    return -1;
  
  return atoi((const char*)uri_string+44);
}


unsigned char*
librdf_node_get_blank_identifier(librdf_node *node)
{
  return node->value.blank.string;
}


unsigned char*
librdf_node_get_counted_blank_identifier(librdf_node* node, size_t* len_p)
{
  if(len_p)
    *len_p = node->value.blank.string_len;
  return node->value.blank.string;
}

int
librdf_node_is_resource(librdf_node *node)
{
  return (node->type == RAPTOR_TERM_TYPE_URI);
}


int
librdf_node_is_literal(librdf_node *node)
{
  return (node->type == RAPTOR_TERM_TYPE_LITERAL);
}


int
librdf_node_is_blank(librdf_node *node)
{
  return (node->type == RAPTOR_TERM_TYPE_BLANK);
}


size_t
librdf_node_encode(librdf_node *node,
                   unsigned char *buffer, size_t length)
{
  size_t total_length = 0;
  unsigned char *string;
  size_t string_length;
  unsigned char language_length = 0;
  unsigned char *datatype_uri_string = NULL;
  size_t datatype_uri_length = 0;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);

  switch(node->type) {
    case RAPTOR_TERM_TYPE_URI:
      string = (unsigned char*)librdf_uri_as_counted_string(node->value.uri,
                                                            &string_length);
      
      total_length = 3 + string_length + 1; /* +1 for \0 at end */
      
      if(length && total_length > length)
        return 0;    

      if(string_length > 0xFFFF)
        return 0;

      if(buffer) {
        buffer[0] = 'R';
        buffer[1] = (string_length & 0xff00) >> 8;
        buffer[2] = (string_length & 0x00ff);
        memcpy((char*)buffer + 3, string, string_length + 1);
      }
      break;
      
    case RAPTOR_TERM_TYPE_LITERAL:
      string = (unsigned char*)node->value.literal.string;
      string_length = node->value.literal.string_len;
      if(node->value.literal.language)
        language_length = node->value.literal.language_len;

      if(node->value.literal.datatype) {
        datatype_uri_string = librdf_uri_as_counted_string(node->value.literal.datatype, &datatype_uri_length);
      }
      
      total_length = 6 + string_length + 1; /* +1 for \0 at end */
      if(string_length > 0xFFFF) /* for long literal - type 'N' */
        total_length += 2;

      if(language_length)
        total_length += language_length + 1;

      if(datatype_uri_length)
        total_length += datatype_uri_length + 1;
      
      if(length && total_length > length)
        return 0;    

      if(datatype_uri_length > 0xFFFF)
        return 0;


      if(buffer) {
        if(string_length > 0xFFFF) {
          /* long literal type N (string length > 0x10000) */
          buffer[0] = 'N';
          buffer[1] = (string_length & 0xff000000) >> 24;
          buffer[2] = (string_length & 0x00ff0000) >> 16;
          buffer[3] = (string_length & 0x0000ff00) >> 8;
          buffer[4] = (string_length & 0x000000ff);
          buffer[5] = (datatype_uri_length & 0xff00) >> 8;
          buffer[6] = (datatype_uri_length & 0x00ff);
          buffer[7] = (language_length & 0x00ff);
          buffer += 8;
        } else {
          /* short literal type M (string length <= 0xFFFF) */
          buffer[0] = 'M';
          buffer[1] = (string_length & 0xff00) >> 8;
          buffer[2] = (string_length & 0x00ff);
          buffer[3] = (datatype_uri_length & 0xff00) >> 8;
          buffer[4] = (datatype_uri_length & 0x00ff);
          buffer[5] = (language_length & 0x00ff);
          buffer += 6;
        }
        memcpy(buffer, string, string_length + 1);
        buffer += string_length + 1;

        if(datatype_uri_length) {
          memcpy(buffer, datatype_uri_string, datatype_uri_length + 1);
          buffer += datatype_uri_length + 1;
        }

        if(language_length)
          memcpy(buffer, node->value.literal.language, language_length + 1);
      } /* end if buffer */

      break;
      
    case RAPTOR_TERM_TYPE_BLANK:
      string = (unsigned char*)node->value.blank.string;
      string_length = node->value.blank.string_len;
      
      total_length = 3 + string_length + 1; /* +1 for \0 at end */
      
      if(length && total_length > length)
        return 0;    
      
      if(string_length > 0xFFFF)
        return 0;

      if(buffer) {
        buffer[0] = 'B';
        buffer[1] = (string_length & 0xff00) >> 8;
        buffer[2] = (string_length & 0x00ff);
        memcpy((char*)buffer + 3, string, string_length + 1);
      }
      break;
      
    case RAPTOR_TERM_TYPE_UNKNOWN:
    default:
      return 0;
  }
  
  return total_length;
}


librdf_node*
librdf_node_decode(librdf_world *world, size_t *size_p,
                   unsigned char *buffer, size_t length)
{
  int is_wf_xml;
  size_t string_length;
  size_t total_length;
  unsigned char language_length;
  unsigned char *datatype_uri_string = NULL;
  size_t datatype_uri_length;
  librdf_uri* datatype_uri = NULL;
  unsigned char *language = NULL;
  int status = 0;
  librdf_node* node = NULL;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  /* absolute minimum - first byte is type */
  if (length < 1)
    return NULL;

  total_length = 0;
  switch(buffer[0]) {
    case 'R': /* URI / Resource */
      /* min */
      if(length < 3)
        return NULL;

      string_length = (buffer[1] << 8) | buffer[2];
      total_length = 3 + string_length + 1;
      
      node = librdf_new_node_from_uri_string(world, buffer + 3);

      break;

    case 'L': /* Old encoding form for Literal */
      /* min */
      if(length < 6)
        return NULL;
      
      is_wf_xml = (buffer[1] & 0xf0)>>8;
      string_length = (buffer[2] << 8) | buffer[3];
      language_length = buffer[5];

      total_length = 6 + string_length + 1; /* +1 for \0 at end */
      if(language_length) {
        language = buffer + total_length;
        total_length += language_length + 1;
      }
      
      node = librdf_new_node_from_typed_counted_literal(world,
                                                        buffer + 6,
                                                        string_length,
                                                        (const char*)language,
                                                        language_length,
                                                        is_wf_xml ? LIBRDF_RS_XMLLiteral_URI(world) : NULL);
    
    break;

    case 'M': /* Literal for Redland 0.9.12+ */
      /* min */
      if(length < 6)
        return NULL;
      
      string_length = (buffer[1] << 8) | buffer[2];
      datatype_uri_length = (buffer[3] << 8) | buffer[4];
      language_length =buffer[5];

      total_length = 6 + string_length + 1; /* +1 for \0 at end */
      if(datatype_uri_length) {
        datatype_uri_string = buffer + total_length;
        total_length += datatype_uri_length + 1;
      }
      if(language_length) {
        language = buffer + total_length;
        total_length += language_length + 1;
      }

      if(datatype_uri_string)
        datatype_uri = librdf_new_uri(world, datatype_uri_string);
      
      node = librdf_new_node_from_typed_counted_literal(world,
                                                        buffer + 6,
                                                        string_length,
                                                        (const char*)language,
                                                        language_length,
                                                        datatype_uri);
      if(datatype_uri)
        librdf_free_uri(datatype_uri);
      
      if(status)
        return NULL;
      
    break;

    case 'N': /* Literal for redland 1.0.5+ (long literal) */
      /* min */
      if(length < 8)
        return NULL;
      
      string_length = (buffer[1] << 24) | (buffer[2] << 16) | (buffer[3] << 8) | buffer[4];
      datatype_uri_length = (buffer[5] << 8) | buffer[6];
      language_length = buffer[7];

      total_length = 8 + string_length + 1; /* +1 for \0 at end */
      if(datatype_uri_length) {
        datatype_uri_string = buffer + total_length;
        total_length += datatype_uri_length + 1;
      }
      if(language_length) {
        language = buffer + total_length;
        total_length += language_length + 1;
      }

      if(datatype_uri_string)
        datatype_uri = librdf_new_uri(world, datatype_uri_string);
      
      node = librdf_new_node_from_typed_counted_literal(world,
                                                        buffer + 8,
                                                        string_length,
                                                        (const char*)language,
                                                        language_length,
                                                        datatype_uri);
      if(datatype_uri)
        librdf_free_uri(datatype_uri);
      
      if(status)
        return NULL;
      
    break;

    case 'B': /* RAPTOR_TERM_TYPE_BLANK */
      /* min */
      if(length < 3)
        return NULL;
      
      string_length = (buffer[1] << 8) | buffer[2];

      total_length = 3 + string_length + 1; /* +1 for \0 at end */
      
      node = librdf_new_node_from_blank_identifier(world, buffer+3);
    
    break;

  default:
    return NULL;
  }
  
  if(size_p)
    *size_p = total_length;

  return node;
}


unsigned char*
librdf_node_to_string(librdf_node *node)
{
  raptor_iostream* iostr;
  unsigned char *s;
  int rc;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  iostr = raptor_new_iostream_to_string(node->world,
                                        (void**)&s, NULL, malloc);
  if(!iostr)
    return NULL;
  
  rc = librdf_node_write(node, iostr);
  raptor_free_iostream(iostr);
  if(rc) {
    free(s);
    s = NULL;
  }

  return s;
}


unsigned char*
librdf_node_to_counted_string(librdf_node *node, size_t *len_p)
{
  raptor_iostream* iostr;
  unsigned char *s;
  int rc;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  iostr = raptor_new_iostream_to_string(node->world,
                                        (void**)&s, len_p, malloc);
  if(!iostr)
    return NULL;
  
  rc = librdf_node_write(node, iostr);
  raptor_free_iostream(iostr);

  if(rc) {
    free(s);
    s = NULL;
  }

  return s;
}


void
librdf_node_print(librdf_node *node, FILE *fh)
{
  raptor_iostream *iostr;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(node, librdf_node);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(fh, FILE*);

  if(!node)
    return;

  iostr = raptor_new_iostream_to_file_handle(node->world, fh);
  if(!iostr)
    return;
  
  librdf_node_write(node, iostr);

  raptor_free_iostream(iostr);
}


int
librdf_node_equals(librdf_node *first_node, librdf_node *second_node)
{
  return raptor_term_equals(first_node, second_node);
}


/* Deprecated. Always fails - use librdf_node_new_static_node_iterator()  */
librdf_iterator*
librdf_node_static_iterator_create(librdf_node **nodes, int size)
{
  return NULL;
}


/* for debugging */
const char*
librdf_node_get_type_as_string(int type)
{
  /* FIXME */
  return NULL;
}


/**
 * librdf_node_write:
 * @node: the node
 * @iostream: iostream to write to
 *
 * Write the node to an iostream
 * 
 * This method is for debugging and the format of the output should
 * not be relied on.
 *
 * Return value: non-0 on failure
 **/
int
librdf_node_write(librdf_node* node, raptor_iostream *iostr)
{
  const unsigned char* term;
  size_t len;

#define NULL_STRING_LENGTH 6
  static const unsigned char * const null_string = (const unsigned char *)"(null)";

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(iostr, raptor_iostream, 1);

  if(!node) {
    raptor_iostream_counted_string_write(null_string, NULL_STRING_LENGTH, iostr);
    return 0;
  }

  switch(node->type) {
    case RAPTOR_TERM_TYPE_LITERAL:
      raptor_iostream_write_byte('"', iostr);
      raptor_string_ntriples_write(node->value.literal.string,
                                   node->value.literal.string_len,
                                   '"',
                                   iostr);
      raptor_iostream_write_byte('"', iostr);
      if(node->value.literal.language) {
        raptor_iostream_write_byte('@', iostr);
        raptor_iostream_string_write(node->value.literal.language, iostr);
      }
      if(node->value.literal.datatype) {
        raptor_iostream_counted_string_write("^^<", 3, iostr);
        term = librdf_uri_as_counted_string(node->value.literal.datatype,
                                            &len);
        raptor_string_ntriples_write(term, len, '>', iostr);
        raptor_iostream_write_byte('>', iostr);
      }

      break;
      
    case RAPTOR_TERM_TYPE_BLANK:
      raptor_iostream_counted_string_write("_:", 2, iostr);
      term = (unsigned char*)node->value.blank.string;
      len = node->value.blank.string_len;
      raptor_iostream_counted_string_write(term, len, iostr);
      break;
      
    case RAPTOR_TERM_TYPE_URI:
      raptor_iostream_write_byte('<', iostr);
      term = librdf_uri_as_counted_string(node->value.uri, &len);
      raptor_string_ntriples_write(term, len, '>', iostr);
      raptor_iostream_write_byte('>', iostr);
      break;
      
    case RAPTOR_TERM_TYPE_UNKNOWN:
    default:
      /*LIBRDF_FATAL1(node->world, LIBRDF_FROM_NODE, "Unknown node type");*/
      return 1;
  }

  return 0;
}
