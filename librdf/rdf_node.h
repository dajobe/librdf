/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_node.h - RDF Node definition
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



#ifndef LIBRDF_NODE_H
#define LIBRDF_NODE_H

#include <rdf_uri.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Node types */

/* DEPENDENCY: If this list is changed, the librdf_node_type_names definition in rdf_node.c
 * must be updated to match 
*/
typedef enum {
  LIBRDF_NODE_TYPE_UNKNOWN   = 0,  /* To catch uninitialised nodes */
  LIBRDF_NODE_TYPE_RESOURCE  = 1,  /* rdf:Resource (& rdf:Property) - has a URI */
  LIBRDF_NODE_TYPE_LITERAL   = 2,  /* rdf:Literal - has an XML string, language, XML space */
  LIBRDF_NODE_TYPE_RESERVED1 = 3,  /* Do not use */
  LIBRDF_NODE_TYPE_BLANK     = 4,  /* blank node has an identifier string */
  LIBRDF_NODE_TYPE_LAST      = LIBRDF_NODE_TYPE_BLANK
} librdf_node_type;



/* literal xml:space values as defined in
 * http://www.w3.org/TR/REC-xml section 2.10 White Space Handling
 */


#ifdef LIBRDF_INTERNAL

struct librdf_node_s
{
  librdf_world *world;
  librdf_node_type type;
  int usage;
  union 
  {
    struct
    {
      /* rdf:Resource and rdf:Property-s have URIs */
      librdf_uri *uri;
    } resource;
    struct
    {
      /* literals are UTF-8 string values ... */
      unsigned char *string;
      unsigned int string_len;

      /* datatype URI or null */
      librdf_uri* datatype_uri;

      /* XML defines these additional attributes for literals */

      /* Language of literal (xml:lang) */
      char *xml_language;

      /* Hash key & size */
      unsigned char *key;
      size_t size;
    } literal;
    struct 
    {
      /* blank nodes have an identifier */
      unsigned char *identifier;
      int identifier_len;
    } blank;
  } value;
};


/* initialising functions / constructors */

/* class methods */
void librdf_init_node(librdf_world* world);
void librdf_finish_node(librdf_world* world);

#ifdef LIBRDF_DEBUG
const char* librdf_node_get_type_as_string(int type);
#endif

/* exported public in error but never used */
librdf_digest* librdf_node_get_digest(librdf_node* node);

#endif


/* Create a new Node. */
REDLAND_API librdf_node* librdf_new_node(librdf_world* world);

/* Create a new resource Node from URI string. */
REDLAND_API librdf_node* librdf_new_node_from_uri_string(librdf_world* world, const unsigned char *string);

/* Create a new resource Node from URI object. */
REDLAND_API librdf_node* librdf_new_node_from_uri(librdf_world* world, librdf_uri *uri);

/* Create a new resource Node from URI object with a local_name */
REDLAND_API librdf_node* librdf_new_node_from_uri_local_name(librdf_world* world, librdf_uri *uri, const unsigned char *local_name);

/* Create a new resource Node from URI string renormalised to a new base */
REDLAND_API librdf_node* librdf_new_node_from_normalised_uri_string(librdf_world* world, const unsigned char *uri_string, librdf_uri *source_uri, librdf_uri *base_uri);

/* Create a new Node from literal string / language. */
REDLAND_API librdf_node* librdf_new_node_from_literal(librdf_world* world, const unsigned char *string, const char *xml_language, int is_wf_xml);

/* Create a new Node from a typed literal string / language. */
REDLAND_API librdf_node* librdf_new_node_from_typed_literal(librdf_world *world, const unsigned char *string, const char *xml_language, librdf_uri* datatype_uri);

/* Create a new Node from blank node identifier. */
REDLAND_API librdf_node* librdf_new_node_from_blank_identifier(librdf_world* world, const unsigned char *identifier);

/* Create a new Node from an existing Node - CLONE */
REDLAND_API librdf_node* librdf_new_node_from_node(librdf_node *node);

/* Init a statically allocated node */
REDLAND_API void librdf_node_init(librdf_world *world, librdf_node *node);

/* destructor */
REDLAND_API void librdf_free_node(librdf_node *r);



/* functions / methods */

REDLAND_API librdf_uri* librdf_node_get_uri(librdf_node* node);

REDLAND_API librdf_node_type librdf_node_get_type(librdf_node* node);

REDLAND_API unsigned char* librdf_node_get_literal_value(librdf_node* node);
REDLAND_API unsigned char* librdf_node_get_literal_value_as_counted_string(librdf_node* node, size_t* len_p);
REDLAND_API char* librdf_node_get_literal_value_as_latin1(librdf_node* node);
REDLAND_API char* librdf_node_get_literal_value_language(librdf_node* node);
REDLAND_API int librdf_node_get_literal_value_is_wf_xml(librdf_node* node);
REDLAND_API librdf_uri* librdf_node_get_literal_value_datatype_uri(librdf_node* node);

REDLAND_API int librdf_node_get_li_ordinal(librdf_node* node);

REDLAND_API unsigned char *librdf_node_get_blank_identifier(librdf_node* node);
REDLAND_API int librdf_node_is_resource(librdf_node* node);
REDLAND_API int librdf_node_is_literal(librdf_node* node);
REDLAND_API int librdf_node_is_blank(librdf_node* node);

/* serialise / deserialise */
REDLAND_API size_t librdf_node_encode(librdf_node* node, unsigned char *buffer, size_t length);
REDLAND_API librdf_node* librdf_node_decode(librdf_world *world, size_t* len_p, unsigned char *buffer, size_t length);


/* convert to a string */
REDLAND_API unsigned char *librdf_node_to_string(librdf_node* node);
REDLAND_API unsigned char* librdf_node_to_counted_string(librdf_node* node, size_t* len_p);

/* pretty print it */
REDLAND_API void librdf_node_print(librdf_node* node, FILE *fh);


/* utility functions */
REDLAND_API int librdf_node_equals(librdf_node* first_node, librdf_node* second_node);


/* create an iterator for a static array of nodes */
REDLAND_API librdf_iterator* librdf_node_static_iterator_create(librdf_node** nodes, int size);


#ifdef __cplusplus
}
#endif

#endif
