/*
 * RDF Node definition
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */


#ifndef RDF_NODE_H
#define RDF_NODE_H

#include <rdf_uri.h>
#include <rdf_digest.h>
/* A resource (also a property) - has a URI */
#define RDF_NODE_TYPE_RESOURCE 0
/* A literal - an XML string + language */
#define RDF_NODE_TYPE_LITERAL  1

typedef struct 
{
  int type;
  union
  {
    union
    {
      /* resources have URIs */
      rdf_uri *uri;
    } resource;
    struct
    {
      /* literals have string values and maybe an XML language */
      char *string;
      char *xml_language; /* yuck */
    } literal;
  } value;
}
rdf_node;


/* class methods */
void rdf_init_node(rdf_digest_factory* factory);


/* initialising functions / constructors */

/* Create a new Node. */
rdf_node* rdf_new_node(void);

/* Create a new resource Node from URI string. */
rdf_node* rdf_new_node_from_uri_string(char *string);

/* Create a new resource Node from URI object. */
rdf_node* rdf_new_node_from_uri(rdf_uri *uri);

/* Create a new Node from literal string / language. */
rdf_node* rdf_new_node_from_literal(char *string, char *xml_language);

/* Create a new Node from an existing Node - CLONE */
rdf_node* rdf_new_node_from_node(rdf_node *node);

/* destructor */
void rdf_free_node(rdf_node *r);



/* functions / methods */

rdf_uri* rdf_node_get_uri(rdf_node* node);
int rdf_node_set_uri(rdf_node* node, rdf_uri *uri);

int rdf_node_get_type(rdf_node* node);
void rdf_node_set_type(rdf_node* node, int type);

char* rdf_node_get_literal_value(rdf_node* node);
char* rdf_node_get_literal_value_language(rdf_node* node);
int rdf_node_set_literal_value(rdf_node* node, char* value, char *xml_language);

rdf_digest* rdf_node_get_digest(rdf_node* node);

char *rdf_node_to_string(rdf_node* node);


/* utility functions */
int rdf_node_equals(rdf_node* first_node, rdf_node* second_node);


#endif
