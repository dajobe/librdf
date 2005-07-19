/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_concepts.h - Definitions of RDF concept URIs and nodes
 *
 * $Id$
 *
 * Copyright (C) 2000-2004, David Beckett http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology http://www.ilrt.bristol.ac.uk/
 * University of Bristol, UK http://www.bristol.ac.uk/
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


#ifndef LIBRDF_CONCEPTS_H
#define LIBRDF_CONCEPTS_H

#ifdef LIBRDF_INTERNAL
#include <rdf_concepts_internal.h>
#endif

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
  LIBRDF_CONCEPT_MS_li,

  LIBRDF_CONCEPT_MS_RDF,
  LIBRDF_CONCEPT_MS_Description,

  LIBRDF_CONCEPT_MS_aboutEach,
  LIBRDF_CONCEPT_MS_aboutEachPrefix,

  LIBRDF_CONCEPT_RS_nodeID,
  LIBRDF_CONCEPT_RS_List,
  LIBRDF_CONCEPT_RS_first,
  LIBRDF_CONCEPT_RS_rest,
  LIBRDF_CONCEPT_RS_nil,
  LIBRDF_CONCEPT_RS_XMLLiteral,

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
#define LIBRDF_MS_li librdf_concept_resources[LIBRDF_CONCEPT_MS_li]

#define LIBRDF_MS_RDF librdf_concept_resources[LIBRDF_CONCEPT_MS_RDF]
#define LIBRDF_MS_Description librdf_concept_resources[LIBRDF_CONCEPT_MS_Description]

#define LIBRDF_MS_aboutEach librdf_concept_resources[LIBRDF_CONCEPT_MS_aboutEach]
#define LIBRDF_MS_aboutEachPrefix librdf_concept_resources[LIBRDF_CONCEPT_MS_aboutEachPrefix]

#define LIBRDF_RS_nodeID librdf_concept_resources[LIBRDF_CONCEPT_RS_nodeID]
#define LIBRDF_RS_List librdf_concept_resources[LIBRDF_CONCEPT_RS_List]
#define LIBRDF_RS_first librdf_concept_resources[LIBRDF_CONCEPT_RS_first]
#define LIBRDF_RS_rest librdf_concept_resources[LIBRDF_CONCEPT_RS_rest]
#define LIBRDF_RS_nil librdf_concept_resources[LIBRDF_CONCEPT_RS_nil]
#define LIBRDF_RS_XMLLiteral librdf_concept_resources[LIBRDF_CONCEPT_RS_XMLLiteral]


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
#define LIBRDF_S_subPropertyOf librdf_concept_resources[LIBRDF_CONCEPT_S_subPropertyOf]



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
#define LIBRDF_MS_li_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_li]

#define LIBRDF_MS_RDF_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_RDF]
#define LIBRDF_MS_Description_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Description]

#define LIBRDF_MS_aboutEach_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_aboutEach]
#define LIBRDF_MS_aboutEachPrefix_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_aboutEachPrefix]

#define LIBRDF_RS_nodeID_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_nodeID]
#define LIBRDF_RS_List_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_List]
#define LIBRDF_RS_first_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_first]
#define LIBRDF_RS_rest_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_rest]
#define LIBRDF_RS_nil_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_nil]
#define LIBRDF_RS_XMLLiteral_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_XMLLiteral]


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

/* public macros */
#define LIBRDF_URI_RDF_MS (&librdf_concept_ms_namespace_uri)
#define LIBRDF_URI_RDF_SCHEMA (&librdf_concept_schema_namespace_uri)

/* private implementation */
extern librdf_uri* librdf_concept_ms_namespace_uri;
extern librdf_uri* librdf_concept_schema_namespace_uri;


#ifdef __cplusplus
}
#endif

#endif
