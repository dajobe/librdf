/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_node.h - RDF Node definition
 *
 * $Id$
 *
 * Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology, University of Bristol.
 *
 *    This package is Free Software available under either of two licenses
 *    (see FAQS.html to see why):
 * 
 * 1. The GNU Lesser General Public License (LGPL)
 * 
 *    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
 *    full license text.
 *      _________________________________________________________________
 * 
 *      Copyright (C) 2000 David Beckett, Institute for Learning and
 *      Research Technology, University of Bristol. All Rights Reserved.
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public License
 *      as published by the Free Software Foundation; either version 2 of
 *      the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *      USA
 *      _________________________________________________________________
 * 
 *    NOTE - under Term 3 of the LGPL, you may choose to license the entire
 *    library under the GPL. See COPYING for the full license text.
 * 
 * 2. The Mozilla Public License
 * 
 *    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
 *    license text.
 * 
 *    Under MPL section 13. I declare that all of the Covered Code is
 *    Multiple Licensed:
 *      _________________________________________________________________
 * 
 *      The contents of this file are subject to the Mozilla Public License
 *      version 1.1 (the "License"); you may not use this file except in
 *      compliance with the License. You may obtain a copy of the License
 *      at http://www.mozilla.org/MPL/
 * 
 *      Software distributed under the License is distributed on an "AS IS"
 *      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 *      the License for the specific language governing rights and
 *      limitations under the License.
 * 
 *      The Initial Developer of the Original Code is David Beckett.
 *      Portions created by David Beckett are Copyright (C) 2000 David
 *      Beckett, Institute for Learning and Research Technology, University
 *      of Bristol. All Rights Reserved.
 * 
 *      Alternatively, the contents of this file may be used under the
 *      terms of the GNU Lesser General Public License, in which case the
 *      provisions of the LGPL License are applicable instead of those
 *      above. If you wish to allow use of your version of this file only
 *      under the terms of the LGPL License and not to allow others to use
 *      your version of this file under the MPL, indicate your decision by
 *      deleting the provisions above and replace them with the notice and
 *      other provisions required by the LGPL License. If you do not delete
 *      the provisions above, a recipient may use your version of this file
 *      under either the MPL or the LGPL License.
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
  LIBRDF_NODE_TYPE_PROPERTY  = LIBRDF_NODE_TYPE_RESOURCE,
  LIBRDF_NODE_TYPE_LITERAL   = 2,  /* rdf:Literal - has an XML string, language, XML space */
  LIBRDF_NODE_TYPE_STATEMENT = 3,  /* rdf:Statement - has a subject, predicate, object, is_stated */
  LIBRDF_NODE_TYPE_BAG       = 4,  /* rdf:Bag */
  LIBRDF_NODE_TYPE_SEQ       = 5,  /* rdf:Seq */
  LIBRDF_NODE_TYPE_ALT       = 6,  /* rdf:Alt */
  LIBRDF_NODE_TYPE_LI        = 7,  /* rdf:li and rdf:_ forms - has an integer */
  LIBRDF_NODE_TYPE_MODEL     = 8,  /* FIXME: no RDF M&S or Schema concept */

  LIBRDF_NODE_TYPE_LAST      = LIBRDF_NODE_TYPE_MODEL
} librdf_node_type;



/* literal xml:space values as defined in
 * http://www.w3.org/TR/REC-xml section 2.10 White Space Handling
 */

/* FIXME This should be defined publically */
typedef enum {
  LIBRDF_NODE_LITERAL_XML_SPACE_UNKNOWN = 0,  /* neither of the next two */
  LIBRDF_NODE_LITERAL_XML_SPACE_DEFAULT = 1,  /* xml:space="default" */
  LIBRDF_NODE_LITERAL_XML_SPACE_PRESERVE = 2, /* xml:space="preserve" */
} librdf_node_literal_xml_space;


struct librdf_node_s
{
  librdf_node_type type;
  union 
  {
    struct
    {
      /* rdf:Resource and rdf:Property-s have URIs */
      librdf_uri *uri;
    } resource;
    struct
    {
      /* literals have string values ... */
      char *string;
      int string_len;

      /* and for RDF the content may also be "Well Formed" XML 
       * such as when rdf:parseType="Literal" is used */
      int is_wf_xml;

      /* XML defines these additional attributes for literals */

      /* Language of literal (xml:lang) */
      char *xml_language;
      /* XML space preserving properties (xml:space) */
      librdf_node_literal_xml_space xml_space;
    } literal;
    struct 
    {
      /* rdf:Statement-s have s,p and o */
      librdf_node* subject;
      librdf_node* predicate;
      librdf_node* object;
    } statement;
    struct 
    {
      /* rdf:li and rdf:_-s have an ordinal */ 
      int ordinal;
    } li;
  } value;
};


#ifdef LIBRDF_INTERNAL

/* convienience macros - internal */
#define LIBRDF_NODE_STATEMENT_SUBJECT(s)   ((s)->value.statement.subject)
#define LIBRDF_NODE_STATEMENT_PREDICATE(s) ((s)->value.statement.predicate)
#define LIBRDF_NODE_STATEMENT_OBJECT(s)    ((s)->value.statement.object)

#endif


/* class methods */
void librdf_init_node(librdf_digest_factory* factory);
void librdf_finish_node(void);


/* initialising functions / constructors */

/* Create a new Node. */
librdf_node* librdf_new_node(void);

/* Create a new resource Node from URI string. */
librdf_node* librdf_new_node_from_uri_string(const char *string);

/* Create a new resource Node from URI object. */
librdf_node* librdf_new_node_from_uri(librdf_uri *uri);

/* Create a new resource Node from URI object with a qname */
librdf_node* librdf_new_node_from_uri_qname(librdf_uri *uri, const char *qname);

/* Create a new Node from literal string / language. */
librdf_node* librdf_new_node_from_literal(const char *string, const char *xml_language, int xml_space, int is_wf_xml);

/* Create a new Node from an existing Node - CLONE */
librdf_node* librdf_new_node_from_node(librdf_node *node);

/* destructor */
void librdf_free_node(librdf_node *r);



/* functions / methods */

librdf_uri* librdf_node_get_uri(librdf_node* node);
int librdf_node_set_uri(librdf_node* node, librdf_uri *uri);

librdf_node_type librdf_node_get_type(librdf_node* node);
void librdf_node_set_type(librdf_node* node, librdf_node_type type);

const char* librdf_node_get_type_as_string(int type);

char* librdf_node_get_literal_value(librdf_node* node);
char* librdf_node_get_literal_value_language(librdf_node* node);
int librdf_node_get_literal_value_xml_space(librdf_node* node);
int librdf_node_get_literal_value_is_wf_xml(librdf_node* node);
int librdf_node_set_literal_value(librdf_node* node, const char* value, const char *xml_language, int xml_space, int is_wf_xml);

int librdf_node_get_li_ordinal(librdf_node* node);
void librdf_node_set_li_ordinal(librdf_node* node, int ordinal);

librdf_digest* librdf_node_get_digest(librdf_node* node);

/* serialise / deserialise */
size_t librdf_node_encode(librdf_node* node, unsigned char *buffer, size_t length);
size_t librdf_node_decode(librdf_node* node, unsigned char *buffer, size_t length);


/* convert to a string */
char *librdf_node_to_string(librdf_node* node);

/* pretty print it */
void librdf_node_print(librdf_node* node, FILE *fh);


/* utility functions */
int librdf_node_equals(librdf_node* first_node, librdf_node* second_node);


#ifdef __cplusplus
}
#endif

#endif
