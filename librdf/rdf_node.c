/*
 * RDF Node implementation
 *   RDF:Resource
 *   RDF:Property
 *   (object) - RDF:Literal / RDF:Resource
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

#include <config.h>

#include <stdio.h>

#ifdef STANDALONE
#define RDF_DEBUG 1
#endif

#include <rdf_config.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_digest.h>


/* statics */
static rdf_digest_factory *rdf_node_digest_factory=NULL;

void rdf_init_node(rdf_digest_factory* factory) 
{
  rdf_node_digest_factory=factory;
}



/* class functions */

/* constructors */

/* Create a new Node with NULL URI. */
rdf_node*
rdf_new_node(void)
{
  return rdf_new_node_from_uri_string((char*)NULL);
}

    

/* Create a new Node and set the URI (can be NULL). */
rdf_node*
rdf_new_node_from_uri_string(char *uri_string) 
{
  rdf_node* new_node;
  rdf_uri *new_uri;
  
  new_node = (rdf_node*)RDF_CALLOC(rdf_node, 1, sizeof(rdf_node));
  if(!new_node)
    return NULL;

  /* set type (actually not needed since calloc above sets type to 0) */
  new_node->type = RDF_NODE_TYPE_RESOURCE;
  
  new_node->value.resource.uri = NULL;
  if(uri_string != NULL) {
    new_uri=rdf_new_uri(uri_string);
    if (!new_uri) {
      rdf_free_node(new_node);
      return NULL;
    }
    if(!rdf_node_set_uri(new_node, new_uri)) {
      rdf_free_node(new_node);
      return NULL;
    }
  }
  return new_node;
}

    

/* Create a new Node and set the URI. */
rdf_node*
rdf_new_node_from_uri(rdf_uri *uri) 
{
  char *uri_string=rdf_uri_as_string(uri); /* note: does not allocate string */
  rdf_node* new_node=rdf_new_node_from_uri_string(uri_string);
  /* thus no need for RDF_FREE(uri_string); */
  return new_node;
}


/* Create a new literal Node. */
rdf_node*
rdf_new_node_from_literal(char *string, char *xml_language) 
{
  rdf_node* new_node;
  
  new_node = (rdf_node*)RDF_CALLOC(rdf_node, 1, sizeof(rdf_node));
  if(!new_node)
    return NULL;

  /* set type */
  new_node->type=RDF_NODE_TYPE_LITERAL;

  if (rdf_node_set_literal_value(new_node, string, xml_language)) {
    rdf_free_node(new_node);
    return NULL;
  }

  return new_node;
}


/* Create a new Node from an existing Node - CLONE */
rdf_node*
rdf_new_node_from_node(rdf_node *node) 
{
  rdf_node* new_node;
  rdf_uri *new_uri;
  
  new_node = (rdf_node*)RDF_CALLOC(rdf_node, 1, sizeof(rdf_node));
  if(!new_node)
    return NULL;

  if(node->type == RDF_NODE_TYPE_RESOURCE) {
    new_uri=rdf_new_uri_from_uri(node->value.resource.uri);
    if(!new_uri){
      RDF_FREE(rdf_node, new_node);
      return NULL;
    }
    rdf_node_set_uri(new_node, new_uri);
  } else {
    /* must be a RDF_NODE_TYPE_LITERAL */
    if (rdf_node_set_literal_value(node,
                                   node->value.literal.string,
                                   node->value.literal.xml_language)) {
      RDF_FREE(rdf_node, new_node);
      return NULL;
    }
  }

  return new_node;
}


/* destructor */
void
rdf_free_node(rdf_node *r) 
{
  if(r->type == RDF_NODE_TYPE_RESOURCE) {
    if(r->value.resource.uri != NULL)
      rdf_free_uri(r->value.resource.uri);
  } else {
    if(r->value.literal.string != NULL)
      RDF_FREE(cstring, r->value.literal.string);
    if(r->value.literal.xml_language != NULL)
      RDF_FREE(cstring, r->value.literal.xml_language);
  }
  RDF_FREE(rdf_node, r);
}


/* functions / methods */

/* returns pointer to *shared* copy of URI - must copy if used elsewhere */
rdf_uri*
rdf_node_get_uri(rdf_node* node) 
{
  return node->value.resource.uri;
}


/* Set the URI of the node */
int
rdf_node_set_uri(rdf_node* node, rdf_uri *uri)
{
  rdf_uri* new_uri=rdf_new_uri_from_uri(uri);
  if(!new_uri)
    return 0;

  /* delete old URI */
  if(node->value.resource.uri)
    rdf_free_uri(node->value.resource.uri);

  /* set new one */
  node->value.resource.uri=new_uri;
  return 1;
}



/* Get the type of the node */
int
rdf_node_get_type(rdf_node* node) 
{
  return node->type;
}


/* Set the type of the node */
void
rdf_node_set_type(rdf_node* node, int type)
{
  node->type=type;
}


/* Get the literal value of the node */
char*
rdf_node_get_literal_value(rdf_node* node) 
{
  if(node->type != RDF_NODE_TYPE_LITERAL)
    return NULL;
  return node->value.literal.string;
}

char*
rdf_node_get_literal_value_language(rdf_node* node) 
{
  if(node->type != RDF_NODE_TYPE_LITERAL)
    return NULL;
  return node->value.literal.xml_language;
}


int
rdf_node_set_literal_value(rdf_node* node, char* value, char *xml_language) 
{
  char *new_value;
  char *new_xml_language=NULL;

  new_value=(char*)RDF_MALLOC(cstring, strlen(value)+1);
  if(!new_value)
    return 1;
  strcpy(new_value, value);
  
  if(xml_language) {
    new_xml_language=(char*)RDF_MALLOC(cstring, strlen(xml_language)+1);
    if(!new_xml_language) {
      RDF_FREE(cstring, value);
      return 1;
    }
    strcpy(new_xml_language, xml_language);
  }

  if(node->value.literal.string)
    RDF_FREE(cstring, node->value.literal.string);
  node->value.literal.string=(char*)new_value;
  if(node->value.literal.xml_language)
    RDF_FREE(cstring, node->value.literal.xml_language);
  node->value.literal.xml_language=new_xml_language;

  return 0;
}


/* allocates a new string since this is a _to_ method */
char*
rdf_node_to_string(rdf_node* node) 
{
  char *uri_string;
  char *s;

  switch(node->type) {
    case RDF_NODE_TYPE_RESOURCE:
      uri_string=rdf_uri_to_string(node->value.resource.uri);
      if(!uri_string)
        return NULL;
      s=(char*)RDF_MALLOC(cstring, strlen(uri_string)+3);
      if(!s) {
        RDF_FREE(cstring, uri_string);
        return NULL;
      }
      sprintf(s, "[%s]", uri_string);
      break;
    case RDF_NODE_TYPE_LITERAL:
      s=(char*)RDF_MALLOC(cstring, strlen(node->value.literal.string)+1);
      if(!s)
        return NULL;
      strcpy(s, node->value.literal.string);
      break;
    default:
      RDF_FATAL2(rdf_node_string, "Illegal node type %d seen\n", node->type);
  }
  return s;
}


rdf_digest*
rdf_node_get_digest(rdf_node* node) 
{
  return rdf_uri_get_digest(node->value.resource.uri);
}



#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  rdf_node* node;
  char *hp_string1="http://www.ilrt.bristol.ac.uk/people/cmdjb/";
  char *hp_string2="http://purl.org/net/dajobe/";
  rdf_uri *uri, *uri2;

  char *program=argv[0];
  
  fprintf(stderr, "%s: Creating home page node from string\n", program);
  node=rdf_new_node_from_uri_string(hp_string1);

  fprintf(stderr, "%s: Home page URI is ", program);
  rdf_uri_print(rdf_node_get_uri(node), stderr);
  fputs("\n", stderr);

  fprintf(stderr, "%s: Creating URI from string '%s'\n", program, hp_string2);
  uri=rdf_new_uri(hp_string2);
  fprintf(stderr, "%s: Setting node URI to new URI ", program);
  rdf_uri_print(uri, stderr);
  fputs("\n", stderr);
  
  
  rdf_node_set_uri(node, uri);

  uri2=rdf_node_get_uri(node);
  fprintf(stderr, "%s: Node now has URI ", program);
  rdf_uri_print(uri2, stderr);
  fputs("\n", stderr);
  

  fprintf(stderr, "%s: Freeing URI\n", program);
  rdf_free_uri(uri);

  fprintf(stderr, "%s: Freeing node\n", program);
  rdf_free_node(node);

  /* keep gcc -Wall happy */
  return(0);
}

#endif
