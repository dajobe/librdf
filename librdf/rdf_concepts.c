/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_concepts.c - Nodes representing concepts from RDF Model and Syntax 
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
