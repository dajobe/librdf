/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_concepts.c - Nodes representing concepts from RDF Model and Syntax 
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



#include <rdf_config.h>

#include <stdio.h>

#include <librdf.h>
#include <rdf_node.h>


librdf_uri* librdf_concept_uris[LIBRDF_CONCEPT_LAST+1];
librdf_node* librdf_concept_resources[LIBRDF_CONCEPT_LAST+1];


/* FIXME: All the stuff here and in rdf_concepts.h should be machine
 * generated from the schemas but there is a catch-22 here - can't do
 * it without representing it 
 */


/* TAKE CARE: Tokens != Labels */

/* Tokens used by the RDF world */
static const char* const librdf_concept_tokens[LIBRDF_CONCEPT_LAST+1]={
  /* RDF M&S */
  "Alt", "Bag", "Property", "Seq", "Statement", "object", "predicate", "subject", "type", "value", "li",
  "RDF", "Description",
  "aboutEach", "aboutEachPrefix",


  /* RDF S */
  "Class", "ConstraintProperty", "ConstraintResource", "Container", "ContainerMembershipProperty", "Literal", "Resource", "comment", "domain", "isDefinedBy", "label", "range", "seeAlso", "subClassOf", "subPropertyOf"
};


/* These are the ENGLISH labels from RDF Schema CR
 * FIXME - language issues 
 */
static const char* const librdf_concept_labels[LIBRDF_CONCEPT_LAST+1]={
  /* RDF M&S */
  "Alt", "Bag", "Property", "Sequence", "Statement", "object", "predicate", "subject", "type", "object", "li",
  "RDF", "Description",
  "aboutEach", "aboutEachPrefix",

  /* RDF S */
  "Class", "ConstraintProperty", "ConstraintResource", "Container", "ContainerMembershipProperty", "Literal", "Resource", "comment", "domain", "isDefinedBy", "label", "range", "seeAlso", "subClassOf", "subPropertyOf"
};



static const char * librdf_concept_ms_namespace="http://www.w3.org/1999/02/22-rdf-syntax-ns#";
static const char * librdf_concept_schema_namespace="http://www.w3.org/2000/01/rdf-schema#";

librdf_uri* librdf_concept_ms_namespace_uri = NULL;
librdf_uri* librdf_concept_schema_namespace_uri = NULL;



/**
 * librdf_init_concepts - Initialise the concepts module.
 * @factory: digest factory to use for digesting concepts content.
 * 
 **/
void
librdf_init_concepts(void)
{
  int i;


  /* Create the Unique URI objects */
  librdf_concept_ms_namespace_uri=librdf_new_uri(librdf_concept_ms_namespace);
  librdf_concept_schema_namespace_uri=librdf_new_uri(librdf_concept_schema_namespace);

  /* Create the M&S and Schema resource nodes */
  for (i=0; i< LIBRDF_CONCEPT_LAST; i++) {
    librdf_uri* ns_uri=(i < LIBRDF_CONCEPT_FIRST_S_ID) ? librdf_concept_ms_namespace_uri :
      librdf_concept_schema_namespace_uri;
    const char * token=librdf_concept_tokens[i];

    librdf_concept_resources[i]=librdf_new_node_from_uri_qname(ns_uri, token);
    if(!librdf_concept_resources[i])
      LIBRDF_FATAL1(librdf_init_concepts, "Failed to create Node from URI\n");

    /* keep shared copy of URI from node */
    librdf_concept_uris[i]=librdf_node_get_uri(librdf_concept_resources[i]);
  }
}


/**
 * librdf_finish_concepts - Terminate the librdf_concepts module
 **/
void
librdf_finish_concepts(void)
{
  int i;

  if(librdf_concept_ms_namespace_uri)
    librdf_free_uri(librdf_concept_ms_namespace_uri);
  if(librdf_concept_schema_namespace_uri)
    librdf_free_uri(librdf_concept_schema_namespace_uri);

  for (i=0; i< LIBRDF_CONCEPT_LAST; i++)
    /* deleted associated URI too */
    librdf_free_node(librdf_concept_resources[i]);
}





#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_init_digest();
  librdf_init_hash();
  librdf_init_uri(librdf_get_digest_factory(NULL), NULL);
  librdf_init_concepts();
  
  librdf_finish_concepts();
  librdf_finish_uri();
  librdf_finish_hash();
  librdf_finish_digest();


#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}

#endif
