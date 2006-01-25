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

typedef enum {
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
} librdf_concepts_index;


/* NOTE: If the above list changes, edit the macros below and
 * librdf_concept_labels in rdf_concepts.c The above list is ordered
 * by simple 'sort' order 
 */


/* private implementation */
extern librdf_node* librdf_concept_resources[LIBRDF_CONCEPT_LAST+1];
extern librdf_uri* librdf_concept_uris[LIBRDF_CONCEPT_LAST+1];


/* public macros for the resources (librdf_node*) representing the concepts */

/**
 * LIBRDF_MS_Alt:
 *
 * RDF namespace concept librdf_node Alt
 */
#define LIBRDF_MS_Alt librdf_concept_resources[LIBRDF_CONCEPT_MS_Alt]

/**
 * LIBRDF_MS_Bag:
 *
 * RDF namespace concept librdf_node Bag
 */
#define LIBRDF_MS_Bag librdf_concept_resources[LIBRDF_CONCEPT_MS_Bag]

/**
 * LIBRDF_MS_Property:
 *
 * RDF namespace concept librdf_node Property
 */
#define LIBRDF_MS_Property librdf_concept_resources[LIBRDF_CONCEPT_MS_Property]

/**
 * LIBRDF_MS_Seq:
 *
 * RDF namespace concept librdf_node Seq
 */
#define LIBRDF_MS_Seq librdf_concept_resources[LIBRDF_CONCEPT_MS_Seq]

/**
 * LIBRDF_MS_Statement:
 *
 * RDF namespace concept librdf_node Statement
 */
#define LIBRDF_MS_Statement librdf_concept_resources[LIBRDF_CONCEPT_MS_Statement]

/**
 * LIBRDF_MS_object:
 *
 * RDF namespace concept librdf_node object
 */
#define LIBRDF_MS_object librdf_concept_resources[LIBRDF_CONCEPT_MS_object]

/**
 * LIBRDF_MS_predicate:
 *
 * RDF namespace concept librdf_node predicate
 */
#define LIBRDF_MS_predicate librdf_concept_resources[LIBRDF_CONCEPT_MS_predicate]

/**
 * LIBRDF_MS_subject:
 *
 * RDF namespace concept librdf_node subject
 */
#define LIBRDF_MS_subject librdf_concept_resources[LIBRDF_CONCEPT_MS_subject]

/**
 * LIBRDF_MS_type:
 *
 * RDF namespace concept librdf_node type
 */
#define LIBRDF_MS_type librdf_concept_resources[LIBRDF_CONCEPT_MS_type]

/**
 * LIBRDF_MS_value:
 *
 * RDF namespace concept librdf_node value
 */
#define LIBRDF_MS_value librdf_concept_resources[LIBRDF_CONCEPT_MS_value]

/**
 * LIBRDF_MS_li:
 *
 * RDF namespace concept librdf_node li
 */
#define LIBRDF_MS_li librdf_concept_resources[LIBRDF_CONCEPT_MS_li]

/**
 * LIBRDF_MS_RDF:
 *
 * RDF namespace concept librdf_node RDF
 */
#define LIBRDF_MS_RDF librdf_concept_resources[LIBRDF_CONCEPT_MS_RDF]

/**
 * LIBRDF_MS_Description:
 *
 * RDF namespace concept librdf_node Description
 */
#define LIBRDF_MS_Description librdf_concept_resources[LIBRDF_CONCEPT_MS_Description]

/**
 * LIBRDF_MS_aboutEach:
 *
 * RDF namespace concept librdf_node aboutEach
 */
#define LIBRDF_MS_aboutEach librdf_concept_resources[LIBRDF_CONCEPT_MS_aboutEach]

/**
 * LIBRDF_MS_aboutEachPrefix:
 *
 * RDF namespace concept librdf_node aboutEachPrefix
 */
#define LIBRDF_MS_aboutEachPrefix librdf_concept_resources[LIBRDF_CONCEPT_MS_aboutEachPrefix]

/**
 * LIBRDF_RS_nodeID:
 *
 * RDF namespace concept librdf_node nodeID
 */
#define LIBRDF_RS_nodeID librdf_concept_resources[LIBRDF_CONCEPT_RS_nodeID]

/**
 * LIBRDF_RS_List:
 *
 * RDF namespace concept librdf_node List
 */
#define LIBRDF_RS_List librdf_concept_resources[LIBRDF_CONCEPT_RS_List]

/**
 * LIBRDF_RS_first:
 *
 * RDF namespace concept librdf_node first
 */
#define LIBRDF_RS_first librdf_concept_resources[LIBRDF_CONCEPT_RS_first]

/**
 * LIBRDF_RS_rest:
 *
 * RDF namespace concept librdf_node rest
 */
#define LIBRDF_RS_rest librdf_concept_resources[LIBRDF_CONCEPT_RS_rest]

/**
 * LIBRDF_RS_nil:
 *
 * RDF namespace concept librdf_node nil
 */
#define LIBRDF_RS_nil librdf_concept_resources[LIBRDF_CONCEPT_RS_nil]

/**
 * LIBRDF_RS_XMLLiteral:
 *
 * RDF namespace concept librdf_node XMLLiteral
 */
#define LIBRDF_RS_XMLLiteral librdf_concept_resources[LIBRDF_CONCEPT_RS_XMLLiteral]


/**
 * LIBRDF_S_Class:
 *
 * RDFS namespace concept Class
 */
#define LIBRDF_S_Class librdf_concept_resources[LIBRDF_CONCEPT_S_Class]

/**
 * LIBRDF_S_ConstraintProperty:
 *
 * RDFS namespace concept ConstraintProperty
 */
#define LIBRDF_S_ConstraintProperty librdf_concept_resources[LIBRDF_CONCEPT_S_ConstraintProperty]

/**
 * LIBRDF_S_ConstraintResource:
 *
 * RDFS namespace concept ConstraintResource
 */
#define LIBRDF_S_ConstraintResource librdf_concept_resources[LIBRDF_CONCEPT_S_ConstraintResource]

/**
 * LIBRDF_S_Container:
 *
 * RDFS namespace concept Container
 */
#define LIBRDF_S_Container librdf_concept_resources[LIBRDF_CONCEPT_S_Container]

/**
 * LIBRDF_S_ContainerMembershipProperty:
 *
 * RDFS namespace concept ContainerMembershipProperty
 */
#define LIBRDF_S_ContainerMembershipProperty librdf_concept_resources[LIBRDF_CONCEPT_S_ContainerMembershipProperty]

/**
 * LIBRDF_S_Literal:
 *
 * RDFS namespace concept Literal
 */
#define LIBRDF_S_Literal librdf_concept_resources[LIBRDF_CONCEPT_S_Literal]

/**
 * LIBRDF_S_Resource:
 *
 * RDFS namespace concept Resource
 */
#define LIBRDF_S_Resource librdf_concept_resources[LIBRDF_CONCEPT_S_Resource]

/**
 * LIBRDF_S_comment:
 *
 * RDFS namespace concept comment
 */
#define LIBRDF_S_comment librdf_concept_resources[LIBRDF_CONCEPT_S_comment]

/**
 * LIBRDF_S_domain:
 *
 * RDFS namespace concept domain
 */
#define LIBRDF_S_domain librdf_concept_resources[LIBRDF_CONCEPT_S_domain]

/**
 * LIBRDF_S_isDefinedBy:
 *
 * RDFS namespace concept isDefinedBy
 */
#define LIBRDF_S_isDefinedBy librdf_concept_resources[LIBRDF_CONCEPT_S_isDefinedBy]

/**
 * LIBRDF_S_label:
 *
 * RDFS namespace concept label
 */
#define LIBRDF_S_label librdf_concept_resources[LIBRDF_CONCEPT_S_label]

/**
 * LIBRDF_S_range:
 *
 * RDFS namespace concept range
 */
#define LIBRDF_S_range librdf_concept_resources[LIBRDF_CONCEPT_S_range]

/**
 * LIBRDF_S_seeAlso:
 *
 * RDFS namespace concept seeAlso
 */
#define LIBRDF_S_seeAlso librdf_concept_resources[LIBRDF_CONCEPT_S_seeAlso]

/**
 * LIBRDF_S_subClassOf:
 *
 * RDFS namespace concept subClassOf
 */
#define LIBRDF_S_subClassOf librdf_concept_resources[LIBRDF_CONCEPT_S_subClassOf]

/**
 * LIBRDF_S_subPropertyOf:
 *
 * RDFS namespace concept subPropertyOf
 */
#define LIBRDF_S_subPropertyOf librdf_concept_resources[LIBRDF_CONCEPT_S_subPropertyOf]

/**
 * LIBRDF_S_subPropertyOf:
 *
 * RDFS namespace concept subPropertyOf
 */
#define LIBRDF_S_subPropertyOf librdf_concept_resources[LIBRDF_CONCEPT_S_subPropertyOf]



/* public macros for the URIs (librdf_uri*) representing the concepts */

/**
 * LIBRDF_MS_Alt_URI:
 *
 * RDF namespace concept URI Alt
 */
#define LIBRDF_MS_Alt_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Alt]

/**
 * LIBRDF_MS_Bag_URI:
 *
 * RDF namespace concept URI Bag
 */
#define LIBRDF_MS_Bag_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Bag]

/**
 * LIBRDF_MS_Property_URI:
 *
 * RDF namespace concept URI Property
 */
#define LIBRDF_MS_Property_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Property]

/**
 * LIBRDF_MS_Seq_URI:
 *
 * RDF namespace concept URI Seq
 */
#define LIBRDF_MS_Seq_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Seq]

/**
 * LIBRDF_MS_Statement_URI:
 *
 * RDF namespace concept URI Statement
 */
#define LIBRDF_MS_Statement_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Statement]

/**
 * LIBRDF_MS_object_URI:
 *
 * RDF namespace concept URI object
 */
#define LIBRDF_MS_object_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_object]

/**
 * LIBRDF_MS_predicate_URI:
 *
 * RDF namespace concept URI predicate
 */
#define LIBRDF_MS_predicate_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_predicate]

/**
 * LIBRDF_MS_subject_URI:
 *
 * RDF namespace concept URI subject
 */
#define LIBRDF_MS_subject_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_subject]

/**
 * LIBRDF_MS_type_URI:
 *
 * RDF namespace concept URI type
 */
#define LIBRDF_MS_type_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_type]

/**
 * LIBRDF_MS_value_URI:
 *
 * RDF namespace concept URI value
 */
#define LIBRDF_MS_value_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_value]

/**
 * LIBRDF_MS_li_URI:
 *
 * RDF namespace concept URI li
 */
#define LIBRDF_MS_li_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_li]

/**
 * LIBRDF_MS_RDF_URI:
 *
 * RDF namespace concept URI RDF
 */
#define LIBRDF_MS_RDF_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_RDF]

/**
 * LIBRDF_MS_Description_URI:
 *
 * RDF namespace concept URI Description
 */
#define LIBRDF_MS_Description_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_Description]

/**
 * LIBRDF_MS_aboutEach_URI:
 *
 * RDF namespace concept URI aboutEach
 */
#define LIBRDF_MS_aboutEach_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_aboutEach]

/**
 * LIBRDF_MS_aboutEachPrefix_URI:
 *
 * RDF namespace concept URI aboutEachPrefix
 */
#define LIBRDF_MS_aboutEachPrefix_URI librdf_concept_uris[LIBRDF_CONCEPT_MS_aboutEachPrefix]

/**
 * LIBRDF_RS_nodeID_URI:
 *
 * RDF namespace concept URI nodeID
 */
#define LIBRDF_RS_nodeID_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_nodeID]

/**
 * LIBRDF_RS_List_URI:
 *
 * RDF namespace concept URI List
 */
#define LIBRDF_RS_List_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_List]

/**
 * LIBRDF_RS_first_URI:
 *
 * RDF namespace concept URI first
 */
#define LIBRDF_RS_first_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_first]

/**
 * LIBRDF_RS_rest_URI:
 *
 * RDF namespace concept URI rest
 */
#define LIBRDF_RS_rest_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_rest]

/**
 * LIBRDF_RS_nil_URI:
 *
 * RDF namespace concept URI nil
 */
#define LIBRDF_RS_nil_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_nil]

/**
 * LIBRDF_RS_XMLLiteral_URI:
 *
 * RDF namespace concept URI XMLLiteral
 */
#define LIBRDF_RS_XMLLiteral_URI librdf_concept_uris[LIBRDF_CONCEPT_RS_XMLLiteral]


/**
 * LIBRDF_S_subPropertyOf_URI:
 *
 * RDFS namespace concept URI subPropertyOf
 */
#define LIBRDF_S_subPropertyOf_URI librdf_concept_uris[LIBRDF_CONCEPT_S_subPropertyOf]

/**
 * LIBRDF_S_subClassOf_URI:
 *
 * RDFS namespace concept URI subClassOf
 */
#define LIBRDF_S_subClassOf_URI librdf_concept_uris[LIBRDF_CONCEPT_S_subClassOf]

/**
 * LIBRDF_S_seeAlso_URI:
 *
 * RDFS namespace concept URI seeAlso
 */
#define LIBRDF_S_seeAlso_URI librdf_concept_uris[LIBRDF_CONCEPT_S_seeAlso]

/**
 * LIBRDF_S_range_URI:
 *
 * RDFS namespace concept URI range
 */
#define LIBRDF_S_range_URI librdf_concept_uris[LIBRDF_CONCEPT_S_range]

/**
 * LIBRDF_S_label_URI:
 *
 * RDFS namespace concept URI label
 */
#define LIBRDF_S_label_URI librdf_concept_uris[LIBRDF_CONCEPT_S_label]

/**
 * LIBRDF_S_isDefinedBy_URI:
 *
 * RDFS namespace concept URI isDefinedBy
 */
#define LIBRDF_S_isDefinedBy_URI librdf_concept_uris[LIBRDF_CONCEPT_S_isDefinedBy]

/**
 * LIBRDF_S_domain_URI:
 *
 * RDFS namespace concept URI domain
 */
#define LIBRDF_S_domain_URI librdf_concept_uris[LIBRDF_CONCEPT_S_domain]

/**
 * LIBRDF_S_comment_URI:
 *
 * RDFS namespace concept URI comment
 */
#define LIBRDF_S_comment_URI librdf_concept_uris[LIBRDF_CONCEPT_S_comment]

/**
 * LIBRDF_S_Resource_URI:
 *
 * RDFS namespace concept URI Resource
 */
#define LIBRDF_S_Resource_URI librdf_concept_uris[LIBRDF_CONCEPT_S_Resource]

/**
 * LIBRDF_S_Literal_URI:
 *
 * RDFS namespace concept URI Literal
 */
#define LIBRDF_S_Literal_URI librdf_concept_uris[LIBRDF_CONCEPT_S_Literal]

/**
 * LIBRDF_S_Container_URI:
 *
 * RDFS namespace concept URI Container
 */
#define LIBRDF_S_Container_URI librdf_concept_uris[LIBRDF_CONCEPT_S_Container]

/**
 * LIBRDF_S_ContainerMembershipProperty_URI:
 *
 * RDFS namespace concept URI ContainerMembershipProperty
 */
#define LIBRDF_S_ContainerMembershipProperty_URI librdf_concept_uris[LIBRDF_CONCEPT_S_ContainerMembershipProperty]

/**
 * LIBRDF_S_ConstraintResource_URI:
 *
 * RDFS namespace concept URI ConstraintResource
 */
#define LIBRDF_S_ConstraintResource_URI librdf_concept_uris[LIBRDF_CONCEPT_S_ConstraintResource]

/**
 * LIBRDF_S_ConstraintProperty_URI:
 *
 * RDFS namespace concept URI ConstraintProperty
 */
#define LIBRDF_S_ConstraintProperty_URI librdf_concept_uris[LIBRDF_CONCEPT_S_ConstraintProperty]

/**
 * LIBRDF_S_Class_URI:
 *
 * RDFS namespace concept URI Class
 */
#define LIBRDF_S_Class_URI librdf_concept_uris[LIBRDF_CONCEPT_S_Class]


/**
 * LIBRDF_URI_RDF_MS:
 *
 * #librdf_uri for <literal>rdf:</literal> namespace.  Copy with
 * librdf_new_uri_from_uri() before using.
 */
#define LIBRDF_URI_RDF_MS (librdf_concept_ms_namespace_uri)

/**
 * LIBRDF_URI_RDF_SCHEMA:
 *
 * #librdf_uri for <literal>rdfs:</literal> namespace.  Copy with
 * librdf_new_uri_from_uri() before using.
 */
#define LIBRDF_URI_RDF_SCHEMA (librdf_concept_schema_namespace_uri)

/* private implementation */
extern librdf_uri* librdf_concept_ms_namespace_uri;
extern librdf_uri* librdf_concept_schema_namespace_uri;


#ifdef __cplusplus
}
#endif

#endif
