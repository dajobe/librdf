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

#include </Users/dajobe/dev/redland/raptor/src/raptor.h>
#define RAPTOR_WORLD_DECLARED 1
#define RAPTOR_V2_AVAILABLE 1

#include <redland.h>

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
  librdf_world_open(world);

  return librdf_new_node_from_blank_identifier(world, (unsigned char*)NULL);
}


librdf_node*
librdf_new_node_from_uri_string(librdf_world *world,
                                const unsigned char *uri_string)
{
  raptor_uri* new_uri;
  
  librdf_world_open(world);

  new_uri = raptor_new_uri(world->raptor_world_ptr, uri_string);
  if(!new_uri)
    return NULL;
  
  return raptor_new_term_from_uri(world->raptor_world_ptr, new_uri);
}


librdf_node*
librdf_new_node_from_uri(librdf_world *world, librdf_uri *uri)
{
  return raptor_new_term_from_uri(world->raptor_world_ptr, uri);
}


librdf_node*
librdf_new_node_from_uri_local_name(librdf_world *world,
                                    librdf_uri *uri,
                                    const unsigned char *local_name)
{
  raptor_uri *new_uri;

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
  return NULL;
}


librdf_node*
librdf_new_node_from_literal(librdf_world *world,
                             const unsigned char *string,
                             const char *xml_language,
                             int is_wf_xml)
{
  return NULL;
}


librdf_node*
librdf_new_node_from_typed_literal(librdf_world *world,
                                   const unsigned char *value,
                                   const char *xml_language,
                                   librdf_uri *datatype_uri)
{
  return NULL;
}


librdf_node*
librdf_new_node_from_typed_counted_literal(librdf_world *world,
                                           const unsigned char *value,
                                           size_t value_len,
                                           const char *xml_language,
                                           size_t xml_language_len,
                                           librdf_uri *datatype_uri)
{
  return NULL;
}


librdf_node*
librdf_new_node_from_blank_identifier(librdf_world *world,
                                      const unsigned char *identifier)
{
  return NULL;
}


librdf_node*
librdf_new_node_from_node(librdf_node *node)
{
  return NULL;
}


void
librdf_free_node(librdf_node *node)
{
}


librdf_uri*
librdf_node_get_uri(librdf_node *node)
{
  return NULL;
}


librdf_node_type
librdf_node_get_type(librdf_node *node)
{
  return node->type;
}


unsigned char*
librdf_node_get_literal_value(librdf_node *node)
{
  return NULL;
}


unsigned char*
librdf_node_get_literal_value_as_counted_string(librdf_node *node,
                                                size_t *len_p)
{
  return NULL;
}


char*
librdf_node_get_literal_value_as_latin1(librdf_node *node)
{
  return NULL;
}


char*
librdf_node_get_literal_value_language(librdf_node *node)
{
  return NULL;
}


int
librdf_node_get_literal_value_is_wf_xml(librdf_node *node)
{
  return 0;
}


librdf_uri*
librdf_node_get_literal_value_datatype_uri(librdf_node *node)
{
  return NULL;
}


int
librdf_node_get_li_ordinal(librdf_node *node)
{
  return -1;
}


unsigned char *
librdf_node_get_blank_identifier(librdf_node *node)
{
  return NULL;
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
  return 0;
}


librdf_node*
librdf_node_decode(librdf_world *world, size_t *size_p,
                   unsigned char *buffer, size_t length)
{
  return NULL;
}


unsigned char*
librdf_node_to_string(librdf_node *node)
{
  return NULL;
}


unsigned char*
librdf_node_to_counted_string(librdf_node *node, size_t *len_p)
{
  return NULL;
}


void
librdf_node_print(librdf_node *node, FILE *fh)
{
}


int
librdf_node_equals(librdf_node *first_node, librdf_node *second_node)
{
  return 0;
}


librdf_iterator*
librdf_node_static_iterator_create(librdf_node **nodes, int size)
{
  return NULL;
}


/* for debugging */
const char*
librdf_node_get_type_as_string(int type)
{
  return NULL;
}
