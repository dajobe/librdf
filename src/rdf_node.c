/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_term.c - RDF Term (RDF URI, Literal, Blank Node) Interface
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
/* needed for utf8 functions and definition of 'byte' */
#include <rdf_utf8.h>



#ifndef STANDALONE

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

/**
 * librdf_new_node:
 * @world: redland world object
 *
 * Constructor - create a new #librdf_node object with a private identifier.
 * 
 * Calls librdf_new_node_from_blank_identifier(world, NULL) to
 * construct a new redland blank node identifier and make a
 * new librdf_node object for it.
 *
 * Return value: a new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node(librdf_world *world)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  return librdf_new_node_from_blank_identifier(world, (unsigned char*)NULL);
}


/**
 * librdf_new_node_from_uri_string:
 * @world: redland world object
 * @uri_string: string representing a URI
 *
 * Constructor - create a new #librdf_node object from a URI string.
 * 
 * Return value: a new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_uri_string(librdf_world *world,
                                const unsigned char *uri_string)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  return raptor_new_term_from_uri_string(world->raptor_world_ptr, uri_string);
}


/**
 * librdf_new_node_from_counted_uri_string:
 * @world: redland world object
 * @uri_string: string representing a URI
 * @len: length of string
 *
 * Constructor - create a new #librdf_node object from a counted URI string.
 * 
 * Return value: a new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_counted_uri_string(librdf_world *world, 
                                        const unsigned char *uri_string,
                                        size_t len) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  return raptor_new_term_from_counted_uri_string(world->raptor_world_ptr, 
                                                 uri_string, len);
}


/* Create a new (Resource) Node and set the URI. */

/**
 * librdf_new_node_from_uri:
 * @world: redland world object
 * @uri: #librdf_uri object
 *
 * Constructor - create a new resource #librdf_node object with a given URI.
 *
 * Return value: a new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_uri(librdf_world *world, librdf_uri *uri)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  return raptor_new_term_from_uri(world->raptor_world_ptr, uri);
}


/**
 * librdf_new_node_from_uri_local_name:
 * @world: redland world object
 * @uri: #librdf_uri object
 * @local_name: local name to append to URI
 *
 * Constructor - create a new resource #librdf_node object with a given URI and local name.
 *
 * Return value: a new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_uri_local_name(librdf_world *world,
                                    librdf_uri *uri,
                                    const unsigned char *local_name)
{
  raptor_uri *new_uri;
  librdf_node* node;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);

  librdf_world_open(world);

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(uri, raptor_uri, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(local_name, string, NULL);

  new_uri = raptor_new_uri_from_uri_local_name(world->raptor_world_ptr,
                                               uri, local_name);
  if(!new_uri)
    return NULL;

  node = raptor_new_term_from_uri(world->raptor_world_ptr, new_uri);
  raptor_free_uri(new_uri);
  return node;
}


/**
 * librdf_new_node_from_normalised_uri_string:
 * @world: redland world object
 * @uri_string: UTF-8 encoded string representing a URI
 * @source_uri: source URI
 * @base_uri: base URI
 *
 * Constructor - create a new #librdf_node object from a UTF-8 encoded URI string normalised to a new base URI.
 * 
 * Return value: a new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_normalised_uri_string(librdf_world *world,
                                           const unsigned char *uri_string,
                                           librdf_uri *source_uri,
                                           librdf_uri *base_uri)
{
  librdf_uri* new_uri;
  librdf_node* node;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  new_uri = librdf_new_uri_normalised_to_base(uri_string,
                                              source_uri, base_uri);
  if(!new_uri)
    return NULL;

  node = raptor_new_term_from_uri(world->raptor_world_ptr, new_uri);
  raptor_free_uri(new_uri);
  return node;
}


#define LIBRDF_XSD_BOOLEAN_TRUE (const unsigned char*)"true"
#define LIBRDF_XSD_BOOLEAN_TRUE_LEN 4
#define LIBRDF_XSD_BOOLEAN_FALSE (const unsigned char*)"false"
#define LIBRDF_XSD_BOOLEAN_FALSE_LEN 5

static int
librdf_xsd_boolean_value_from_string(const unsigned char* string,
                                     unsigned int len)
{
  int integer = 0;

  /* FIXME
   * Strictly only {true, false, 1, 0} are allowed according to
   * http://www.w3.org/TR/xmlschema-2/#boolean
   */
  if((len == LIBRDF_XSD_BOOLEAN_TRUE_LEN &&
     (!strcmp(LIBRDF_GOOD_CAST(const char*, string), "true") ||
      !strcmp(LIBRDF_GOOD_CAST(const char*, string), "TRUE"))
      )
     ||
     (len == 1 &&
      !strcmp(LIBRDF_GOOD_CAST(const char*, string), "1")
      )
     )
     integer = 1;

  return integer;
}


/*
 * librdf_node_normalize:
 * @world: world
 * @node: node
 *
 * INTERNAL - Normalize the literal of datatyped literals to canonical form
 *
 * Currently handles xsd:boolean. 
 *
 * FIXME: This should be in Raptor or Rasqal since
 * librdf_xsd_boolean_value_from_string and the canonicalization was
 * ripped out of Rasqal.
 *
 * Return value: new node (or same one if no action was taken)
 */
static librdf_node*
librdf_node_normalize(librdf_world* world, librdf_node* node)
{
  if(!node)
    return NULL;
  
  if(node->value.literal.datatype) {
    librdf_uri* dt_uri;

    dt_uri = librdf_new_uri_from_uri_local_name(world->xsd_namespace_uri,
                                                LIBRDF_GOOD_CAST(const unsigned char*, "boolean"));
    if(raptor_uri_equals(node->value.literal.datatype, dt_uri)) {
      const unsigned char* value;
      size_t value_len;
      int bvalue;

      bvalue = librdf_xsd_boolean_value_from_string(node->value.literal.string,
                                                    node->value.literal.string_len);

      value = bvalue ? LIBRDF_XSD_BOOLEAN_TRUE :
                       LIBRDF_XSD_BOOLEAN_FALSE;
      value_len = bvalue ? LIBRDF_XSD_BOOLEAN_TRUE_LEN :
                           LIBRDF_XSD_BOOLEAN_FALSE_LEN;

      if(node->value.literal.string_len != value_len ||
         strcmp(LIBRDF_GOOD_CAST(const char*, node->value.literal.string),
                LIBRDF_GOOD_CAST(const char*, value))) {
        /* If literal is not canonical, replace the node */
        librdf_free_node(node);
        node = NULL;

        /* Have to use Raptor constructor here since
         * librdf_new_node_from_typed_counted_literal() calls this
         */
        node = raptor_new_term_from_counted_literal(world->raptor_world_ptr,
                                                    value, value_len,
                                                    dt_uri,
                                                    (const unsigned char*)NULL,
                                                    (unsigned char)0);
      }
    }

    if(dt_uri)
      librdf_free_uri(dt_uri);
  }

  return node;
}


/**
 * librdf_new_node_from_literal:
 * @world: redland world object
 * @string: literal UTF-8 encoded string value
 * @xml_language: literal XML language (or NULL, empty string)
 * @is_wf_xml: non 0 if literal is XML
 *
 * Constructor - create a new literal #librdf_node object.
 * 
 * 0.9.12: xml_space argument deleted
 *
 * An @xml_language cannot be used when @is_wf_xml is non-0. If both
 * are given, NULL is returned.  If @xml_language is the empty string,
 * it is the equivalent to NULL.
 *
 * Return value: new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_literal(librdf_world *world,
                             const unsigned char *string,
                             const char *xml_language,
                             int is_wf_xml)
{
  librdf_uri* datatype_uri;
  librdf_node* n;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  datatype_uri = (is_wf_xml ?  LIBRDF_RS_XMLLiteral_URI(world) : NULL);

  n = raptor_new_term_from_literal(world->raptor_world_ptr,
                                   string, datatype_uri,
                                   (const unsigned char*)xml_language);
  return librdf_node_normalize(world, n);
}


/**
 * librdf_new_node_from_typed_literal:
 * @world: redland world object
 * @value: literal UTF-8 encoded string value
 * @xml_language: literal XML language (or NULL, empty string)
 * @datatype_uri: URI of typed literal datatype or NULL
 *
 * Constructor - create a new typed literal #librdf_node object.
 * 
 * Only one of @xml_language or @datatype_uri may be given.  If both
 * are given, NULL is returned.  If @xml_language is the empty string,
 * it is the equivalent to NULL.
 *
 * Return value: new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_typed_literal(librdf_world *world,
                                   const unsigned char *value,
                                   const char *xml_language,
                                   librdf_uri *datatype_uri)
{
  librdf_node* n;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  n = raptor_new_term_from_literal(world->raptor_world_ptr,
                                   value, datatype_uri,
                                   (const unsigned char*)xml_language);
  return librdf_node_normalize(world, n);
}


/**
 * librdf_new_node_from_typed_counted_literal:
 * @world: redland world object
 * @value: literal UTF-8 encoded string value
 * @value_len: literal string value length
 * @xml_language: literal XML language (or NULL, empty string)
 * @xml_language_len: literal XML language length (not used if @xml_language is NULL)
 * @datatype_uri: URI of typed literal datatype or NULL
 *
 * Constructor - create a new typed literal #librdf_node object.
 * 
 * Takes copies of the passed in @value, @datatype_uri and @xml_language.
 *
 * Only one of @xml_language or @datatype_uri may be given.  If both
 * are given, NULL is returned.  If @xml_language is the empty string,
 * it is the equivalent to NULL.
 *
 * Return value: new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_typed_counted_literal(librdf_world *world,
                                           const unsigned char *value,
                                           size_t value_len,
                                           const char *xml_language,
                                           size_t xml_language_len,
                                           librdf_uri *datatype_uri)
{
  librdf_node* n;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  n = raptor_new_term_from_counted_literal(world->raptor_world_ptr,
                                           value, value_len,
                                           datatype_uri,
                                           (const unsigned char*)xml_language,
                                           (unsigned char)xml_language_len);
  return librdf_node_normalize(world, n);
}


/**
 * librdf_new_node_from_counted_blank_identifier:
 * @world: redland world object
 * @identifier: UTF-8 encoded blank node identifier or NULL
 * @identifier_len: length of @identifier
 *
 * Constructor - create a new blank node #librdf_node object from a blank node counted length identifier.
 *
 * If no @identifier string is given (NULL) this creates a new
 * internal identifier and uses it.
 * 
 * Return value: new #librdf_node object or NULL on failure
 **/
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


/**
 * librdf_new_node_from_blank_identifier:
 * @world: redland world object
 * @identifier: UTF-8 encoded blank node identifier or NULL
 *
 * Constructor - create a new blank node #librdf_node object from a blank node identifier.
 *
 * If no @identifier string is given (NULL) this creates a new
 * internal identifier and uses it.
 * 
 * Return value: new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_blank_identifier(librdf_world *world,
                                      const unsigned char *identifier)
{
  const unsigned char *blank = identifier;
  librdf_node* node = NULL;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  
  librdf_world_open(world);

  if(!identifier)
    blank = librdf_world_get_genid(world);
  
  node = raptor_new_term_from_blank(world->raptor_world_ptr, blank);

  if(!identifier)
    LIBRDF_FREE(char*, (char*)blank);

  return node;
}


/**
 * librdf_new_node_from_node:
 * @node: #librdf_node object to copy
 *
 * Copy constructor - create a new librdf_node object from an existing librdf_node object.
 * 
 * Return value: a new #librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_node(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  return raptor_term_copy(node);
}


/**
 * librdf_free_node:
 * @node: #librdf_node object
 *
 * Destructor - destroy an #librdf_node object.
 * 
 **/
void
librdf_free_node(librdf_node *node)
{
  if(!node)
    return;

  raptor_free_term(node);
}


/* functions / methods */

/**
 * librdf_node_get_uri:
 * @node: the node object
 *
 * Get the URI for a node object.
 *
 * Returns a pointer to the URI object held by the node, it must be
 * copied if it is wanted to be used by the caller.
 * 
 * Return value: URI object or NULL if node has no URI.
 **/
librdf_uri*
librdf_node_get_uri(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_URI)
    return NULL;
  
  return node->value.uri;
}


/**
 * librdf_node_get_type:
 * @node: the node object
 *
 * Get the type of the node.
 *
 * See also librdf_node_is_resource(), librdf_node_is_literal() and
 * librdf_node_is_blank() for testing individual types.
 * 
 * Return value: the node type
 **/
librdf_node_type
librdf_node_get_type(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, LIBRDF_NODE_TYPE_UNKNOWN);

  return (librdf_node_type)node->type;
}


/**
 * librdf_node_get_literal_value:
 * @node: the node object
 *
 * Get the literal value of the node as a UTF-8 encoded string.
 * 
 * Returns a pointer to the UTF-8 encoded literal value held by the
 * node, it must be copied if it is wanted to be used by the caller.
 *
 * Return value: the UTF-8 encoded literal string or NULL if node is not a literal
 **/
unsigned char*
librdf_node_get_literal_value(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  return node->value.literal.string;
}


/**
 * librdf_node_get_literal_value_as_counted_string:
 * @node: the node object
 * @len_p: pointer to location to store the string length (or NULL)
 *
 * Get the literal value of the node as a counted UTF-8 encoded string.
 * 
 * Returns a pointer to the UTF-8 encoded literal string value held
 * by the node, it must be copied if it is wanted to be used by the
 * caller.
 *
 * Return value: the UTF-8 encoded literal string or NULL if node is not a literal
 **/
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


/**
 * librdf_node_get_literal_value_as_latin1:
 * @node: the node object
 *
 * Get the string literal value of the node as ISO Latin-1.
 * 
 * Returns a newly allocated string containing the conversion of the
 * node literal value held by the node into ISO Latin-1.  Discards
 * characters outside the U+0000 to U+00FF range (inclusive).
 *
 * Return value: the Latin-1 literal string or NULL if node is not a literal
 **/
char*
librdf_node_get_literal_value_as_latin1(librdf_node *node)
{
  size_t slen;
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  if(!node->value.literal.string)
    return NULL;
  
  slen = LIBRDF_GOOD_CAST(size_t, node->value.literal.string_len);
  return (char*)librdf_utf8_to_latin1_2((const unsigned char*)node->value.literal.string,
                                        slen, '\0', NULL);
}


/**
 * librdf_node_get_literal_value_language:
 * @node: the node object
 *
 * Get the XML language of the node.
 * 
 * Returns a pointer to the literal language value held by the node,
 * it must be copied if it is wanted to be used by the caller.
 * Language strings are ASCII, not UTF-8 encoded Unicode.
 *
 * Return value: the XML language string or NULL if node is not a literal
 * or there is no XML language defined.
 **/
char*
librdf_node_get_literal_value_language(librdf_node *node)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, NULL);

  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  return (char*)node->value.literal.language;
}


/**
 * librdf_node_get_literal_value_is_wf_xml:
 * @node: the node object
 *
 * Get the XML well-formness property of the node.
 * 
 * Return value: 0 if the XML literal is NOT well formed XML content, or the node is not a literal
 **/
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


/**
 * librdf_node_get_literal_value_datatype_uri:
 * @node: the node object
 *
 * Get the typed literal datatype URI of the literal node.
 * 
 * Return value: shared URI of the datatyped literal or NULL if the node is not a literal, or has no datatype URI
 **/
librdf_uri*
librdf_node_get_literal_value_datatype_uri(librdf_node *node)
{
  if(node->type != RAPTOR_TERM_TYPE_LITERAL)
    return NULL;
  
  return node->value.literal.datatype;
}


/**
 * librdf_node_get_li_ordinal:
 * @node: the node object
 *
 * Get the node li object ordinal value.
 *
 * Return value: the li ordinal value or < 1 on failure
 **/
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


/**
 * librdf_node_get_blank_identifier:
 * @node: the node object
 *
 * Get the blank node identifier as a UTF-8 encoded string.
 *
 * Return value: the UTF-8 encoded blank node identifier value or NULL on failure
 **/
unsigned char*
librdf_node_get_blank_identifier(librdf_node *node)
{
  return node->value.blank.string;
}


/**
 * librdf_node_get_counted_blank_identifier:
 * @node: the node object
 * @len_p: pointer to variable to store length (or NULL)
 *
 * Get the blank node identifier as a counted UTF-8 encoded string.
 *
 * Return value: the UTF-8 encoded blank node identifier value or NULL on failure
 **/
unsigned char*
librdf_node_get_counted_blank_identifier(librdf_node* node, size_t* len_p)
{
  if(len_p)
    *len_p = node->value.blank.string_len;
  return node->value.blank.string;
}


/**
 * librdf_node_is_resource:
 * @node: the node object
 *
 * Check node is a resource.
 * 
 * Return value: non-zero if the node is a resource (URI)
 **/
int
librdf_node_is_resource(librdf_node *node)
{
  return (node->type == RAPTOR_TERM_TYPE_URI);
}


/**
 * librdf_node_is_literal:
 * @node: the node object
 *
 * Check node is a literal.
 * 
 * Return value: non-zero if the node is a literal
 **/
int
librdf_node_is_literal(librdf_node *node)
{
  return (node->type == RAPTOR_TERM_TYPE_LITERAL);
}


/**
 * librdf_node_is_blank:
 * @node: the node object
 *
 * Check node is a blank nodeID.
 * 
 * Return value: non-zero if the node is a blank nodeID
 **/
int
librdf_node_is_blank(librdf_node *node)
{
  return (node->type == RAPTOR_TERM_TYPE_BLANK);
}


/**
 * librdf_node_encode:
 * @node: the node to serialise
 * @buffer: the buffer to use
 * @length: buffer size
 *
 * Serialise a node into a buffer.
 * 
 * Encodes the given node in the buffer, which must be of sufficient
 * size.  If buffer is NULL, no work is done but the size of buffer
 * required is returned.
 * 
 * If the node cannot be encoded due to restrictions of the encoding
 * format, a redland error is generated
 *
 * Return value: the number of bytes written or 0 on failure.
 **/
size_t
librdf_node_encode(librdf_node *node,
                   unsigned char *buffer, size_t length)
{
  size_t total_length = 0;
  unsigned char *string;
  size_t string_length;
  size_t language_length = 0;
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
        language_length = LIBRDF_GOOD_CAST(size_t, node->value.literal.language_len);

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


/**
 * librdf_node_decode:
 * @world: librdf_world
 * @size_p: pointer to bytes used or NULL
 * @buffer: the buffer to use
 * @length: buffer size
 *
 * Deserialise a node from a buffer.
 * 
 * Decodes the serialised node (as created by librdf_node_encode() )
 * from the given buffer.
 * 
 * Return value: new node or NULL on failure (bad encoding, allocation failure)
 **/
librdf_node*
librdf_node_decode(librdf_world *world, size_t *size_p,
                   unsigned char *buffer, size_t length)
{
  int is_wf_xml;
  size_t string_length;
  size_t total_length;
  size_t language_length;
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

  switch(buffer[0]) {
    case 'R': /* URI / Resource */
      /* min */
      if(length < 3)
        return NULL;

      string_length = LIBRDF_GOOD_CAST(size_t, (buffer[1] << 8) | buffer[2]);
      total_length = 3 + string_length + 1;
      
      node = librdf_new_node_from_uri_string(world, buffer + 3);

      break;

    case 'L': /* Old encoding form for Literal */
      /* min */
      if(length < 6)
        return NULL;
      
      is_wf_xml = (buffer[1] & 0xf0)>>8;
      string_length = LIBRDF_GOOD_CAST(size_t, (buffer[2] << 8) | buffer[3]);
      language_length = LIBRDF_GOOD_CAST(size_t, buffer[5]);

      total_length = 6 + string_length + 1; /* +1 for \0 at end */
      if(language_length) {
        language = buffer + total_length;
        total_length += language_length + 1;
      }
      
      node = librdf_new_node_from_typed_counted_literal(world,
                                                        buffer + 6,
                                                        string_length,
                                                        (const char*)language,
                                                        LIBRDF_GOOD_CAST(unsigned char, language_length),
                                                        is_wf_xml ? LIBRDF_RS_XMLLiteral_URI(world) : NULL);
    
    break;

    case 'M': /* Literal for Redland 0.9.12+ */
      /* min */
      if(length < 6)
        return NULL;
      
      string_length = LIBRDF_GOOD_CAST(size_t, (buffer[1] << 8) | buffer[2]);
      datatype_uri_length = LIBRDF_GOOD_CAST(size_t, (buffer[3] << 8) | buffer[4]);
      language_length = buffer[5];

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
                                                        LIBRDF_GOOD_CAST(unsigned char, language_length),
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
      
      string_length = LIBRDF_GOOD_CAST(size_t, (buffer[1] << 24) | (buffer[2] << 16) | (buffer[3] << 8) | buffer[4]);
      datatype_uri_length = LIBRDF_GOOD_CAST(size_t, (buffer[5] << 8) | buffer[6]);
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
                                                        LIBRDF_GOOD_CAST(size_t, language_length),
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
      
      string_length = LIBRDF_GOOD_CAST(size_t, (buffer[1] << 8) | buffer[2]);

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


#ifndef REDLAND_DISABLE_DEPRECATED
/**
 * librdf_node_to_string:
 * @node: the node object
 *
 * Format the node as a string in a debugging format.
 * 
 * Note a new string is allocated which must be freed by the caller.
 * 
 * @Deprecated: Use librdf_node_write() to write to #raptor_iostream
 * which can be made to write to a string.  Use a #librdf_serializer
 * to write proper syntax formats.
 *
 * Return value: a string value representing the node or NULL on failure
 **/
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
    raptor_free_memory(s);
    s = NULL;
  }

  return s;
}
#endif


#ifndef REDLAND_DISABLE_DEPRECATED
/**
 * librdf_node_to_counted_string:
 * @node: the node object
 * @len_p: pointer to location to store length
 *
 * Format the node as a counted string in a debugging format.
 * 
 * Note a new string is allocated which must be freed by the caller.
 *
 * @Deprecated: Use librdf_node_write() to write to #raptor_iostream
 * which can be made to write to a string.  Use a #librdf_serializer
 * to write proper syntax formats.
 *
 * Return value: a string value representing the node or NULL on failure
 **/
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
    raptor_free_memory(s);
    s = NULL;
  }

  return s;
}
#endif


/**
 * librdf_node_print:
 * @node: the node
 * @fh: file handle
 *
 * Pretty print the node to a file descriptor.
 * 
 * This method is for debugging and the format of the output should
 * not be relied on.
 * 
 **/
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


/**
 * librdf_node_equals:
 * @first_node: first #librdf_node node
 * @second_node: second #librdf_node node
 *
 * Compare two librdf_node objects for equality.
 * 
 * Note - for literal nodes, XML language, XML space and well-formness are 
 * presently ignored in the comparison.
 * 
 * Return value: non 0 if nodes are equal.  0 if not-equal or failure
 **/
int
librdf_node_equals(librdf_node *first_node, librdf_node *second_node)
{
  return raptor_term_equals(first_node, second_node);
}


/**
 * librdf_node_static_iterator_create:
 * @nodes: static array of #librdf_node objects
 * @size: size of array
 *
 * Create an iterator over an array of nodes (ALWAYS FAILS)
 * 
 * This legacy method used to create an iterator for an existing
 * static array of librdf_node objects.  It was intended for testing
 * iterator code.
 * 
 * @deprecated: always returns NULL. Use
 * librdf_node_new_static_node_iterator()
 *
 * Return value: NULL
 **/
librdf_iterator*
librdf_node_static_iterator_create(librdf_node **nodes, int size)
{
  return NULL;
}


/**
 * librdf_node_write:
 * @node: the node
 * @iostr: iostream to write to
 *
 * Write the node to an iostream in N-Triples format.
 * 
 * This method can be used to write a node in a relatively
 * readable format.  To write more compact formats use a
 * serializer to pick a syntax and serialize triples to it.
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

#endif /* STANDALONE */


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


static void
dump_node_as_C(FILE* fh, const char* var, void *buffer, int size) {
  int i;
  unsigned char* p=(unsigned char*)buffer;
  
  fprintf(fh, "const unsigned char %s[%d] = {", var, size);
  for (i=0; i < size; i++) {
    if(i)
      fputs(", ", fh);
    fprintf(fh, "0x%02x", p[i]);
  }
  fputs("};\n", fh);
}

  
static int
check_node(const char* program, const unsigned char *expected, 
           void *buffer, size_t size) {
  unsigned int i;
  for(i=0; i< size; i++) {
    unsigned char c=((unsigned char*)buffer)[i];
    if(c != expected[i]) {
      fprintf(stderr, "%s: Encoding node byte %d: 0x%02x expected 0x%02x\n",
              program, i, c, expected[i]);
      return(1);
    }
  }
  return(0);
}


static const char *hp_string1="http://purl.org/net/dajobe/";
static const char *hp_string2="http://purl.org/net/dajobe/";
static const char *lit_string="Dave Beckett";
static const char *genid="genid42";
static const char *datatype_lit_string="Datatyped literal value";
static const char *datatype_uri_string="http://example.org/datatypeURI";

/* Node Encoded (type R) version of hp_string1 */
static const unsigned char hp_uri_encoded[31] = {0x52, 0x00, 0x1b, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x70, 0x75, 0x72, 0x6c, 0x2e, 0x6f, 0x72, 0x67, 0x2f, 0x6e, 0x65, 0x74, 0x2f, 0x64, 0x61, 0x6a, 0x6f, 0x62, 0x65, 0x2f, 0x00};

/* Node Encoded (type M) version of typed literal with literal value
 * datatype_lit_string and datatype URI datatype_uri_string */
static const unsigned char datatyped_literal_M_encoded[61] = {0x4d, 0x00, 0x17, 0x00, 0x1e, 0x00, 0x44, 0x61, 0x74, 0x61, 0x74, 0x79, 0x70, 0x65, 0x64, 0x20, 0x6c, 0x69, 0x74, 0x65, 0x72, 0x61, 0x6c, 0x20, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x00, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x6f, 0x72, 0x67, 0x2f, 0x64, 0x61, 0x74, 0x61, 0x74, 0x79, 0x70, 0x65, 0x55, 0x52, 0x49, 0x00};

/* Node Encoded (type N) version of big 100,000-length literal
 * (just the first 32 bytes, the rest are 0x58 'X')
 */
const unsigned char big_literal_N_encoded[32] = {0x4e, 0x00, 0x01, 0x86, 0xa0, 0x00, 0x00, 0x00, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58};


int
main(int argc, char *argv[]) 
{
  librdf_node *node, *node2, *node3, *node4, *node5, *node6, *node7, *node8, *node9;
  librdf_uri *uri, *uri2;
  int size, size2;
  unsigned char *buffer;
  librdf_world *world;
  size_t big_literal_length;
  unsigned char *big_literal;
  unsigned int i;
  
  const char *program=librdf_basename((const char*)argv[0]);
	
  world=librdf_new_world();
  librdf_world_open(world);

  fprintf(stderr, "%s: Creating home page node from string\n", program);
  node=librdf_new_node_from_uri_string(world, (const unsigned char*)hp_string1);
  if(!node) {
    fprintf(stderr, "%s: librdf_new_node_from_uri_string failed\n", program);
    return(1);
  }
  
  fprintf(stdout, "%s: Home page URI is ", program);
  librdf_uri_print(librdf_node_get_uri(node), stdout);
  fputc('\n', stdout);
  
  fprintf(stdout, "%s: Creating URI from string '%s'\n", program, 
          hp_string2);
  uri=librdf_new_uri(world, (const unsigned char*)hp_string2);
  fprintf(stdout, "%s: Setting node URI to new URI ", program);
  librdf_uri_print(uri, stdout);
  fputc('\n', stdout);
  librdf_free_uri(uri);
  
  fprintf(stdout, "%s: Node is: ", program);
  librdf_node_print(node, stdout);
  fputc('\n', stdout);

  size=librdf_node_encode(node, NULL, 0);
  fprintf(stdout, "%s: Encoding node requires %d bytes\n", program, size);
  buffer = LIBRDF_MALLOC(unsigned char*, size);

  fprintf(stdout, "%s: Encoding node in buffer\n", program);
  size2=librdf_node_encode(node, buffer, size);
  if(size2 != size) {
    fprintf(stderr, "%s: Encoding node used %d bytes, expected it to use %d\n", program, size2, size);
    return(1);
  }

  if(0)
    dump_node_as_C(stdout, "hp_uri_encoded", buffer, size);
  if(check_node(program, hp_uri_encoded, buffer, size))
    return(1);
  
    
  fprintf(stdout, "%s: Creating new node\n", program);

  fprintf(stdout, "%s: Decoding node from buffer\n", program);
  if(!(node2=librdf_node_decode(world, NULL, buffer, size))) {
    fprintf(stderr, "%s: Decoding node failed\n", program);
    return(1);
  }
  LIBRDF_FREE(char*, buffer);
   
  fprintf(stdout, "%s: New node is: ", program);
  librdf_node_print(node2, stdout);
  fputc('\n', stdout);
 
  
  fprintf(stdout, "%s: Creating new literal string node\n", program);
  node3=librdf_new_node_from_literal(world, (const unsigned char*)lit_string, NULL, 0);
  if(!node3) {
    fprintf(stderr, "%s: librdf_new_node_from_literal failed\n", program);
    return(1);
  }

  buffer=(unsigned char*)librdf_node_get_literal_value_as_latin1(node3);
  if(!buffer) {
    fprintf(stderr, "%s: Failed to get literal string value as Latin-1\n", program);
    return(1);
  }
  fprintf(stdout, "%s: Node literal string value (Latin-1) is: '%s'\n",
          program, buffer);
  LIBRDF_FREE(char*, buffer);
  
  fprintf(stdout, "%s: Creating new blank node with identifier %s\n", program, genid);
  node4=librdf_new_node_from_blank_identifier(world, (const unsigned char*)genid);
  if(!node4) {
    fprintf(stderr, "%s: librdf_new_node_from_blank_identifier failed\n", program);
    return(1);
  }

  buffer=librdf_node_get_blank_identifier(node4);
  if(!buffer) {
    fprintf(stderr, "%s: Failed to get blank node identifier\n", program);
    return(1);
  }
  fprintf(stdout, "%s: Node identifier is: '%s'\n", program, buffer);
  
  node5=librdf_new_node_from_node(node4);
  if(!node5) {
    fprintf(stderr, "%s: Failed to make new blank node from old one\n", program);
    return(1);
  } 

  buffer=librdf_node_get_blank_identifier(node5);
  if(!buffer) {
    fprintf(stderr, "%s: Failed to get copied blank node identifier\n", program);
    return(1);
  }
  fprintf(stdout, "%s: Copied node identifier is: '%s'\n", program, buffer);

  fprintf(stdout, "%s: Creating a new blank node with a generated identifier\n", program);
  node6=librdf_new_node(world);
  if(!node6) {
    fprintf(stderr, "%s: librdf_new_node failed\n", program);
    return(1);
  }

  buffer=librdf_node_get_blank_identifier(node6);
  if(!buffer) {
    fprintf(stderr, "%s: Failed to get blank node identifier\n", program);
    return(1);
  }
  fprintf(stdout, "%s: Generated node identifier is: '%s'\n", program, buffer);

  uri2=librdf_new_uri(world, (const unsigned char*)datatype_uri_string);
  node7=librdf_new_node_from_typed_literal(world,
                                           (const unsigned char*)datatype_lit_string,
                                           NULL, uri2);
  librdf_free_uri(uri2);

  size=librdf_node_encode(node7, NULL, 0);
  fprintf(stdout, "%s: Encoding typed node requires %d bytes\n", program, size);
  buffer = LIBRDF_MALLOC(unsigned char*, size);

  fprintf(stdout, "%s: Encoding typed node in buffer\n", program);
  size2=librdf_node_encode(node7, (unsigned char*)buffer, size);
  if(size2 != size) {
    fprintf(stderr, "%s: Encoding typed node used %d bytes, expected it to use %d\n", program, size2, size);
    return(1);
  }

  if(0)
    dump_node_as_C(stdout, "datatyped_literal_M_encoded", buffer, size);
  if(check_node(program, datatyped_literal_M_encoded, buffer, size))
    return(1);
  
  fprintf(stdout, "%s: Decoding typed node from buffer\n", program);
  if(!(node8=librdf_node_decode(world, NULL, (unsigned char*)buffer, size))) {
    fprintf(stderr, "%s: Decoding typed node failed\n", program);
    return(1);
  }
  LIBRDF_FREE(char*, buffer);
   
  if(librdf_new_node_from_typed_literal(world, 
                                        (const unsigned char*)"Datatyped literal value",
                                        "en-GB", uri2)) {
    fprintf(stderr, "%s: Unexpected success allowing a datatyped literal with a language\n", program);
    return(1);
  }
    
  if(librdf_new_node_from_literal(world, 
                                  (const unsigned char*)"XML literal value",
                                  "en-GB", 1)) {
    fprintf(stderr, "%s: Unexpected success allowing an XML literal with a language\n", program);
    return(1);
  }
    
  big_literal_length=100000;
  big_literal = LIBRDF_MALLOC(unsigned char*, big_literal_length + 1);
  for(i=0; i<big_literal_length; i++)
     big_literal[i]='X';

  node9=librdf_new_node_from_typed_counted_literal(world,
                                                   big_literal, big_literal_length,
                                                   NULL, 0, NULL);
  if(!node9) {
    fprintf(stderr, "%s: Failed to make big %d byte literal\n", program,
            (int)big_literal_length);
    return(1);
  }
  LIBRDF_FREE(char*, big_literal);

  size=librdf_node_encode(node9, NULL, 0);
  fprintf(stdout, "%s: Encoding big literal node requires %d bytes\n", program, size);
  buffer = LIBRDF_MALLOC(unsigned char*, size);
  fprintf(stdout, "%s: Encoding big literal node in buffer\n", program);
  size2=librdf_node_encode(node9, (unsigned char*)buffer, size);
  if(size2 != size) {
    fprintf(stderr, "%s: Encoding big literal node used %d bytes, expected it to use %d\n", program, size2, size);
    return(1);
  }

  /* Just check first 32 bytes */
  if(0)
    dump_node_as_C(stdout, "big_literal_N_encoded", buffer, 32);
  if(check_node(program, big_literal_N_encoded, buffer, 32))
    return(1);
  LIBRDF_FREE(char*, buffer);
    

  fprintf(stdout, "%s: Freeing nodes\n", program);
  librdf_free_node(node9);
  librdf_free_node(node8);
  librdf_free_node(node7);
  librdf_free_node(node6);
  librdf_free_node(node5);
  librdf_free_node(node4);
  librdf_free_node(node3);
  librdf_free_node(node2);
  librdf_free_node(node);

  librdf_free_world(world);

  /* keep gcc -Wall happy */
  return(0);
}

#endif
