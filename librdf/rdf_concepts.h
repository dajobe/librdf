/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_concepts.h - Definitions of concepts from RDF Model and Syntax
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


#ifndef LIBRDF_CONCEPTS_H
#define LIBRDF_CONCEPTS_H

#ifdef __cplusplus
extern "C" {
#endif


/* Private tokens for the concepts we 'know' about */

enum {
  /* RDF Model & Syntax concepts defined in prose at
   *   http://www.w3.org/1999/02/22-rdf-syntax-ns
   * and in RDF Schema form at
   *   http://www.w3.org/2000/01/rdf-schema
   */
  LIBRDF_CONCEPT_MS_Alt,
  LIBRDF_CONCEPT_MS_Bag,
  LIBRDF_CONCEPT_MS_Property,
  LIBRDF_CONCEPT_MS_Seq,
  LIBRDF_CONCEPT_MS_Statement,
  LIBRDF_CONCEPT_MS_object,
  LIBRDF_CONCEPT_MS_predicate,
  LIBRDF_CONCEPT_MS_subject,
  LIBRDF_CONCEPT_MS_type,
  LIBRDF_CONCEPT_MS_value,

  LIBRDF_CONCEPT_SYNTAX_aboutEach,
  LIBRDF_CONCEPT_SYNTAX_aboutEachPrefix,

  /* RDF Schema concepts defined in prose at
   *   http://www.w3.org/TR/2000/CR-rdf-schema-20000327/
   * and in RDF Schema form at 
   *   http://www.w3.org/2000/01/rdf-schema
   */
  LIBRDF_CONCEPT_S_Class,
  LIBRDF_CONCEPT_S_ConstraintProperty,
  LIBRDF_CONCEPT_S_ConstraintResource,
  LIBRDF_CONCEPT_S_Container,
  LIBRDF_CONCEPT_S_ContainerMembershipProperty,
  LIBRDF_CONCEPT_S_Literal,
  LIBRDF_CONCEPT_S_Resource,
  LIBRDF_CONCEPT_S_comment,
  LIBRDF_CONCEPT_S_domain,
  LIBRDF_CONCEPT_S_isDefinedBy,
  LIBRDF_CONCEPT_S_label,
  LIBRDF_CONCEPT_S_range,
  LIBRDF_CONCEPT_S_seeAlso,
  LIBRDF_CONCEPT_S_subClassOf,
  LIBRDF_CONCEPT_S_subPropertyOf,

  /* first entry from schema namespace */
  LIBRDF_CONCEPT_FIRST_S_ID = LIBRDF_CONCEPT_S_Class,

  LIBRDF_CONCEPT_LAST = LIBRDF_CONCEPT_S_subPropertyOf,
};

/* If the above list changes, edit the macros below and
 * librdf_concept_labels in rdf_concepts.c 
 * The above list is ordered by simple 'sort' order */


/* private implementation */
extern librdf_node* librdf_concept_resources[LIBRDF_CONCEPT_LAST+1];
extern librdf_uri* librdf_concept_uris[LIBRDF_CONCEPT_LAST+1];

/* public macros for the resources (librdf_node*) representing the concepts */
#define LIBRDF_MS_Alt librdf_concept_resources[LIBRDF_CONCEPT_MS_Alt]
#define LIBRDF_MS_Bag librdf_concept_resources[LIBRDF_CONCEPT_MS_Bag]
#define LIBRDF_MS_Property librdf_concept_resources[LIBRDF_CONCEPT_MS_Property]
#define LIBRDF_MS_Seq librdf_concept_resources[LIBRDF_CONCEPT_MS_Seq]
#define LIBRDF_MS_Statement librdf_concept_resources[LIBRDF_CONCEPT_MS_Statement]
#define LIBRDF_MS_object librdf_concept_resources[LIBRDF_CONCEPT_MS_object]
#define LIBRDF_MS_predicate librdf_concept_resources[LIBRDF_CONCEPT_MS_predicate]
#define LIBRDF_MS_subject librdf_concept_resources[LIBRDF_CONCEPT_MS_subject]
#define LIBRDF_MS_type librdf_concept_resources[LIBRDF_CONCEPT_MS_type]
#define LIBRDF_MS_value librdf_concept_resources[LIBRDF_CONCEPT_MS_value]


#define LIBRDF_S_Class librdf_concept_resources[LIBRDF_CONCEPT_S_Class]
#define LIBRDF_S_ConstraintProperty librdf_concept_resources[LIBRDF_CONCEPT_S_ConstraintProperty]
#define LIBRDF_S_ConstraintResource librdf_concept_resources[LIBRDF_CONCEPT_S_ConstraintResource]
#define LIBRDF_S_Container librdf_concept_resources[LIBRDF_CONCEPT_S_Container]
#define LIBRDF_S_ContainerMembershipProperty librdf_concept_resources[LIBRDF_CONCEPT_S_ContainerMembershipProperty]
#define LIBRDF_S_Literal librdf_concept_resources[LIBRDF_CONCEPT_S_Literal]
#define LIBRDF_S_Resource librdf_concept_resources[LIBRDF_CONCEPT_S_Resource]
#define LIBRDF_S_comment librdf_concept_resources[LIBRDF_CONCEPT_S_comment]
#define LIBRDF_S_domain librdf_concept_resources[LIBRDF_CONCEPT_S_domain]
#define LIBRDF_S_isDefinedBy librdf_concept_resources[LIBRDF_CONCEPT_S_isDefinedBy]
#define LIBRDF_S_label librdf_concept_resources[LIBRDF_CONCEPT_S_label]
#define LIBRDF_S_range librdf_concept_resources[LIBRDF_CONCEPT_S_range]
#define LIBRDF_S_seeAlso librdf_concept_resources[LIBRDF_CONCEPT_S_seeAlso]
#define LIBRDF_S_subClassOf librdf_concept_resources[LIBRDF_CONCEPT_S_subClassOf]
#define LIBRDF_S_subPropertyOf librdf_concept_resources[LIBRDF_CONCEPT_S_subPropertyOf]

#define LIBRDF_SYNTAX_aboutEach librdf_concept_resources[LIBRDF_CONCEPT_SYNTAX_aboutEach]
#define LIBRDF_SYNTAX_aboutEachPrefix librdf_concept_resources[LIBRDF_CONCEPT_SYNTAX_aboutEachPrefix]


/* public macros for the URIs (librdf_uri*) representing the concepts */
#define LIBRDF_MS_Alt_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Alt]
#define LIBRDF_MS_Bag_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Bag]
#define LIBRDF_MS_Property_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Property]
#define LIBRDF_MS_Seq_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Seq]
#define LIBRDF_MS_Statement_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Statement]
#define LIBRDF_MS_object_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_object]
#define LIBRDF_MS_predicate_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_predicate]
#define LIBRDF_MS_subject_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_subject]
#define LIBRDF_MS_type_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_type]
#define LIBRDF_MS_value_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_value]


#define LIBRDF_S_subPropertyOf_URI librdf_concept_uris[LIBRDF_CONCEPT_S_subPropertyOf]
#define LIBRDF_S_subClassOf_URI librdf_concept_uris[LIBRDF_CONCEPT_S_subClassOf]
#define LIBRDF_S_seeAlso_URI librdf_concept_uris[LIBRDF_CONCEPT_S_seeAlso]
#define LIBRDF_S_range_URI librdf_concept_uris[LIBRDF_CONCEPT_S_range]
#define LIBRDF_S_label_URI librdf_concept_uris[LIBRDF_CONCEPT_S_label]
#define LIBRDF_S_isDefinedBy_URI librdf_concept_uris[LIBRDF_CONCEPT_S_isDefinedBy]
#define LIBRDF_S_domain_URI librdf_concept_uris[LIBRDF_CONCEPT_S_domain]
#define LIBRDF_S_comment_URI librdf_concept_uris[LIBRDF_CONCEPT_S_comment]
#define LIBRDF_S_Resource_URI librdf_concept_uris[LIBRDF_CONCEPT_S_Resource]
#define LIBRDF_S_Literal_URI librdf_concept_uris[LIBRDF_CONCEPT_S_Literal]
#define LIBRDF_S_Container_URI librdf_concept_uris[LIBRDF_CONCEPT_S_Container]
#define LIBRDF_S_ContainerMembershipProperty_URI librdf_concept_uris[LIBRDF_CONCEPT_S_ContainerMembershipProperty]
#define LIBRDF_S_ConstraintResource_URI librdf_concept_uris[LIBRDF_CONCEPT_S_ConstraintResource]
#define LIBRDF_S_ConstraintProperty_URI librdf_concept_uris[LIBRDF_CONCEPT_S_ConstraintProperty]
#define LIBRDF_S_Class_URI librdf_concept_uris[LIBRDF_CONCEPT_S_Class]

#define LIBRDF_SYNTAX_aboutEach_URI librdf_concept_uris[LIBRDF_CONCEPT_SYNTAX_aboutEach]
#define LIBRDF_SYNTAX_aboutEachPrefix_URI librdf_concept_uris[LIBRDF_CONCEPT_SYNTAX_aboutEachPrefix]


/* private implementation */
extern librdf_uri* librdf_concept_ms_namespace_uri;
extern librdf_uri* librdf_concept_schema_namespace_uri;

/* public macros */
#define LIBRDF_URI_RDF_MS (&librdf_concept_ms_namespace_uri)
#define LIBRDF_URI_RDF_SCHEMA (&librdf_concept_schema_namespace_uri)


/* class methods */
void librdf_init_concepts(void);
void librdf_finish_concepts(void);




#ifdef __cplusplus
}
#endif

#endif
