/*
 * rdf_node.h - RDF Node definition
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */



#ifndef LIBRDF_NODE_H
#define LIBRDF_NODE_H

#include <rdf_uri.h>
#include <rdf_digest.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Node types */
typedef enum {
	LIBRDF_NODE_TYPE_RESOURCE, /* A resource (& property) - has a URI */
	LIBRDF_NODE_TYPE_LITERAL   /* A literal - an XML string + language */
} librdf_node_type;


typedef struct 
{
	librdf_node_type type;
	union
	{
		union
		{
			/* resources have URIs */
			librdf_uri *uri;
		} resource;
		struct
		{
			/* literals have string values ... */
			char *string;
			int string_len;
			/* .. and optional XML Language */
			char *xml_language;
			/* and may be "Well Formed XML" */
			int is_wf_xml;
		} literal;
	} value;
}
librdf_node;


/* class methods */
void librdf_init_node(librdf_digest_factory* factory);


/* initialising functions / constructors */

/* Create a new Node. */
librdf_node* librdf_new_node(void);

/* Create a new resource Node from URI string. */
librdf_node* librdf_new_node_from_uri_string(char *string);

/* Create a new resource Node from URI object. */
librdf_node* librdf_new_node_from_uri(librdf_uri *uri);

/* Create a new Node from literal string / language. */
librdf_node* librdf_new_node_from_literal(char *string, char *xml_language, int is_wf_xml);

/* Create a new Node from an existing Node - CLONE */
librdf_node* librdf_new_node_from_node(librdf_node *node);

/* destructor */
void librdf_free_node(librdf_node *r);



/* functions / methods */

librdf_uri* librdf_node_get_uri(librdf_node* node);
int librdf_node_set_uri(librdf_node* node, librdf_uri *uri);

librdf_node_type librdf_node_get_type(librdf_node* node);
void librdf_node_set_type(librdf_node* node, librdf_node_type type);

char* librdf_node_get_literal_value(librdf_node* node);
char* librdf_node_get_literal_value_language(librdf_node* node);
int librdf_node_set_literal_value(librdf_node* node, char* value, char *xml_language, int is_wf_xml);

librdf_digest* librdf_node_get_digest(librdf_node* node);

char *librdf_node_to_string(librdf_node* node);


/* utility functions */
int librdf_node_equals(librdf_node* first_node, librdf_node* second_node);


#ifdef __cplusplus
}
#endif

#endif
