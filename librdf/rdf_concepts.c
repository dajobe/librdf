/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_concepts.c - Nodes representing concepts from RDF Model and Syntax 
 *
 * $Id$
 *
 * Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
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



#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <redland.h>
#include <rdf_node.h>

#ifndef STANDALONE

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
  /* all new in RDF/XML revised */
  "nodeID",
  "List", "first", "rest", "nil", 
  "XMLLiteral",

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
  /* all new in RDF/XML revised */
  "nodeID",
  "List", "first", "rest", "nil",
  "XMLLiteral",

  /* RDF S */
  "Class", "ConstraintProperty", "ConstraintResource", "Container", "ContainerMembershipProperty", "Literal", "Resource", "comment", "domain", "isDefinedBy", "label", "range", "seeAlso", "subClassOf", "subPropertyOf"
};



static const unsigned char * librdf_concept_ms_namespace=(const unsigned char *)"http://www.w3.org/1999/02/22-rdf-syntax-ns#";
static const unsigned char * librdf_concept_schema_namespace=(const unsigned char *)"http://www.w3.org/2000/01/rdf-schema#";

librdf_uri* librdf_concept_ms_namespace_uri = NULL;
librdf_uri* librdf_concept_schema_namespace_uri = NULL;



/**
 * librdf_init_concepts - Initialise the concepts module.
 * @world: redland world object
 * 
 **/
void
librdf_init_concepts(librdf_world *world)
{
  int i;


  /* Create the Unique URI objects */
  librdf_concept_ms_namespace_uri=librdf_new_uri(world, librdf_concept_ms_namespace);
  librdf_concept_schema_namespace_uri=librdf_new_uri(world, librdf_concept_schema_namespace);

  /* Create the M&S and Schema resource nodes */
  for (i=0; i< LIBRDF_CONCEPT_LAST; i++) {
    librdf_uri* ns_uri=(i < LIBRDF_CONCEPT_FIRST_S_ID) ? librdf_concept_ms_namespace_uri :
      librdf_concept_schema_namespace_uri;
    const unsigned char * token=(const unsigned char *)librdf_concept_tokens[i];

    librdf_concept_resources[i]=librdf_new_node_from_uri_local_name(world, ns_uri, token);
    if(!librdf_concept_resources[i])
      LIBRDF_FATAL1(world, LIBRDF_FROM_CONCEPTS, "Failed to create Node from URI\n");

    /* keep shared copy of URI from node */
    librdf_concept_uris[i]=librdf_node_get_uri(librdf_concept_resources[i]);
  }
}


/**
 * librdf_get_concept_by_name - get Redland uri and/or node objects for RDF concepts
 * @world: redland world object
 * @is_ms: non zero if name is a RDF M&S concept (else is RDF schema)
 * @name: the name to look up
 * @uri_p: pointer to variable to hold &librdf_uri of concept or NULL if not required
 * @node_p: pointer to variable to hold &librdf_node of concept or NULL if not required
 * 
 * Allows the dynamic look-up of an RDF concept by the local_name of
 * the concept in either the RDF M&S or RDF Schema namespace.  Returns
 * the &librdf_uri and/or &librdf_node found as required.
 **/
void
librdf_get_concept_by_name(librdf_world *world, int is_ms,
                           const char *name,
                           librdf_uri **uri_p, librdf_node **node_p)
{
  int i;
  int start=is_ms ? 0 : LIBRDF_CONCEPT_FIRST_S_ID;
  int last=is_ms ? LIBRDF_CONCEPT_FIRST_S_ID : LIBRDF_CONCEPT_LAST;

  for (i=start; i< last; i++)
    if(!strcmp(librdf_concept_tokens[i], name)) {
      if(uri_p)
        *uri_p=librdf_concept_uris[i];
      if(node_p)
        *node_p=librdf_concept_resources[i];
    }
}


/**
 * librdf_finish_concepts - Terminate the librdf_concepts module
 * @world: redland world object
 **/
void
librdf_finish_concepts(librdf_world *world)
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

#endif


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_world *world;
  
  world=librdf_new_world();
  librdf_world_init_mutex(world);
  
  librdf_init_digest(world);
  librdf_init_hash(world);
  librdf_init_uri(world);
  librdf_init_node(world);
  librdf_init_concepts(world);
  
  librdf_finish_concepts(world);
  librdf_finish_node(world);
  librdf_finish_uri(world);
  librdf_finish_hash(world);
  librdf_finish_digest(world);

  LIBRDF_FREE(librdf_world, world);

  /* keep gcc -Wall happy */
  return(0);
}

#endif
