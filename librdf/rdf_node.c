/*
 * rdf_node.c - RDF Node implementation
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
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */


#include <config.h>

#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h> /* for memcpy */
#endif

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#define LIBRDF_INTERNAL 1
#include <rdf_config.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_digest.h>


/* statics */
static librdf_digest_factory *librdf_node_digest_factory=NULL;

/**
 * librdf_init_node:
 * @factory: digest factory to use for digesting node content.
 * 
 * Initialise the node module.
 * 
 **/
void
librdf_init_node(librdf_digest_factory* factory) 
{
	librdf_node_digest_factory=factory;
}



/* class functions */

/* constructors */

/**
 * librdf_new_node:
 * 
 * Constructor: create a new &librdf_node object with a NULL URI.
 * 
 * Return value: a new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node(void)
{
	return librdf_new_node_from_uri_string((char*)NULL);
}

    

/**
 * librdf_new_node_from_uri_string:
 * @uri_string: string representing a URI or NULL
 * 
 * Constructor: create a new &librdf_node object with a given URI 
 * which can be NULL, and set later.

 * Return value: a new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_uri_string(char *uri_string) 
{
	librdf_node* new_node;
	librdf_uri *new_uri;
  
	new_node = (librdf_node*)LIBRDF_CALLOC(librdf_node, 1, 
					       sizeof(librdf_node));
	if(!new_node)
		return NULL;

	new_node->type = LIBRDF_NODE_TYPE_RESOURCE;
	/* not needed thanks to calloc */
	/* new_node->value.resource.uri = NULL; */

        /* return node now if not setting URI */
	if(!uri_string)
		return new_node;

	new_uri=librdf_new_uri(uri_string);
	if (!new_uri) {
		librdf_free_node(new_node);
		return NULL;
	}
	/* ownership of new_uri passes to node object */
	if(!librdf_node_set_uri(new_node, new_uri)) {
		librdf_free_node(new_node);
		return NULL;
	}
	return new_node;
}

    

/* Create a new Node and set the URI. */

/**
 * librdf_new_node_from_uri:
 * @uri: &rdf_uri object
 * 
 * Constructor: create a new &librdf_node object with a given URI

 * Return value: a new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_uri(librdf_uri *uri) 
{
        /* note: librdf_uri_as_string does not allocate string ... */
	char *uri_string=librdf_uri_as_string(uri); 

	librdf_node* new_node=librdf_new_node_from_uri_string(uri_string);

	/* ... thus no need for LIBRDF_FREE(uri_string); */
	return new_node;
}


/**
 * librdf_new_node_from_literal:
 * @string: literal string value
 * @xml_language: literal XML language (or NULL)
 * @is_wf_xml: non 0 if literal is XML
 * 
 * Constructor: create a new literal &librdf_node object
 * 
 * Return value: new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_literal(char *string, char *xml_language, int is_wf_xml) 
{
	librdf_node* new_node;
  
	new_node = (librdf_node*)LIBRDF_CALLOC(librdf_node, 1,
					       sizeof(librdf_node));
	if(!new_node)
		return NULL;

	/* set type */
	new_node->type=LIBRDF_NODE_TYPE_LITERAL;
	
	if (librdf_node_set_literal_value(new_node, string, xml_language,
					  is_wf_xml)) {
		librdf_free_node(new_node);
		return NULL;
	}
	
	return new_node;
}


/**
 * librdf_new_node_from_node:
 * @node: &librdf_node object to copy
 * 
 * Clone Constructor: create a new &librdf_node object from an existing
 * &librdf_node object
 * 
 * Return value: a new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_node(librdf_node *node) 
{
	librdf_node* new_node;
	librdf_uri *new_uri;
  
	new_node = (librdf_node*)LIBRDF_CALLOC(librdf_node, 1, 
					       sizeof(librdf_node));
	if(!new_node)
		return NULL;

	if(node->type == LIBRDF_NODE_TYPE_RESOURCE) {
		new_uri=librdf_new_uri_from_uri(node->value.resource.uri);
		if(!new_uri){
			LIBRDF_FREE(librdf_node, new_node);
			return NULL;
		}
		librdf_node_set_uri(new_node, new_uri);
	} else {
		/* must be a LIBRDF_NODE_TYPE_LITERAL */
		if (librdf_node_set_literal_value(node,
						  node->value.literal.string,
						  node->value.literal.xml_language,
						  node->value.literal.is_wf_xml)) {
			LIBRDF_FREE(librdf_node, new_node);
			return NULL;
		}
	}
	
	return new_node;
}


/**
 * librdf_free_node:
 * @node: &librdf_node object
 * 
 * Destructor: destroy an &librdf_node object
 **/
void
librdf_free_node(librdf_node *node) 
{
	if(node->type == LIBRDF_NODE_TYPE_RESOURCE) {
		if(node->value.resource.uri != NULL)
			librdf_free_uri(node->value.resource.uri);
	} else {
		if(node->value.literal.string != NULL)
			LIBRDF_FREE(cstring, node->value.literal.string);
		if(node->value.literal.xml_language != NULL)
			LIBRDF_FREE(cstring, node->value.literal.xml_language);
	}
	LIBRDF_FREE(librdf_node, node);
}


/* functions / methods */

/**
 * librdf_node_get_uri:
 * @node: the node object
 * 
 * Get the URI for a node object.
 *
 * Returns a pointer to the URI object held by the node, it must be
 * copied if it is wanted to be used by the caller.
 * 
 * Return value: URI object
 **/
librdf_uri*
librdf_node_get_uri(librdf_node* node) 
{
	return node->value.resource.uri;
}


/**
 * librdf_node_set_uri:
 * @node: the node object
 * @uri: the uri object
 * 
 * Set the URI of the node
 *
 * The URI passed in becomes owned by the node object and will be freed by
 * the node on destruction.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_node_set_uri(librdf_node* node, librdf_uri *uri)
{
	if(!uri)
		return 0;
	
	/* delete old URI */
	if(node->value.resource.uri)
		librdf_free_uri(node->value.resource.uri);
	
	/* set new one */
	node->value.resource.uri=uri;
	return 1;
}


/**
 * librdf_node_get_type:
 * @node: the node object
 * 
 * Get the type of the node
 * 
 * Return value: the node type
 **/
librdf_node_type
librdf_node_get_type(librdf_node* node) 
{
	return node->type;
}


/**
 * librdf_node_set_type:
 * @node: the node object
 * @type: the node type
 *
 * Presently the nodes can be of type LIBRDF_NODE_TYPE_RESOURCE or
 * LIBRDF_NODE_TYPE_LITERAL
 * 
 * Set the type of the node
 **/
void
librdf_node_set_type(librdf_node* node, librdf_node_type type)
{
	node->type=type;
}


/**
 * librdf_node_get_literal_value:
 * @node: the node object
 * 
 * Get the string literal value of the node
 * 
 * Returns a pointer to the literal value held by the node, it must be
 * copied if it is wanted to be used by the caller.
 *
 * Return value: the literal string or NULL if node is not a literal
 **/
char*
librdf_node_get_literal_value(librdf_node* node) 
{
	if(node->type != LIBRDF_NODE_TYPE_LITERAL)
		return NULL;
	return node->value.literal.string;
}


/**
 * librdf_node_get_literal_value_language:
 * @node: the node object
 * 
 * Returns a pointer to the literal language value held by the node, it must
 * be copied if it is wanted to be used by the caller.
 *
 * Return value: the XML language string or NULL if node is not a literal
 * or there is no XML language defined.
 **/
char*
librdf_node_get_literal_value_language(librdf_node* node) 
{
	if(node->type != LIBRDF_NODE_TYPE_LITERAL)
		return NULL;
	return node->value.literal.xml_language;
}


/**
 * librdf_node_set_literal_value:
 * @node: the node object
 * @value: pointer to the literal string value
 * @xml_language: pointer to the literal language (or NULL if not defined)
 * @is_wf_xml: non 0 if the value is Well Formed XML
 * 
 * Set the node literal value, optional language and if the literal is
 * well formed XML.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_node_set_literal_value(librdf_node* node, char* value,
			      char *xml_language,
                           int is_wf_xml) 
{
	int value_len;
	char *new_value;
	char *new_xml_language=NULL;
	
	/* only time the string literal length should ever be measured */
	value_len = node->value.literal.string_len = strlen(value);

	new_value=(char*)LIBRDF_MALLOC(cstring, value_len + 1);
	if(!new_value)
		return 1;
	memcpy(new_value, value, value_len);
	
	if(xml_language) {
		new_xml_language=(char*)LIBRDF_MALLOC(cstring, 
						      strlen(xml_language) + 1);
		if(!new_xml_language) {
			LIBRDF_FREE(cstring, value);
			return 1;
		}
		strcpy(new_xml_language, xml_language);
	}
	
	if(node->value.literal.string)
		LIBRDF_FREE(cstring, node->value.literal.string);
	node->value.literal.string=(char*)new_value;
	if(node->value.literal.xml_language)
		LIBRDF_FREE(cstring, node->value.literal.xml_language);
	node->value.literal.xml_language=new_xml_language;
	
	node->value.literal.is_wf_xml=is_wf_xml;
	
	return 0;
}


/**
 * librdf_node_to_string:
 * @node: the node object
 * 
 * Format the node as a string, which must be freed by the caller.
 * 
 * Return value: a string value representing the node or NULL on failure
 **/
char*
librdf_node_to_string(librdf_node* node) 
{
	char *uri_string;
	char *s;
	
	switch(node->type) {
	case LIBRDF_NODE_TYPE_RESOURCE:
		uri_string=librdf_uri_to_string(node->value.resource.uri);
		if(!uri_string)
			return NULL;
		s=(char*)LIBRDF_MALLOC(cstring, strlen(uri_string)+3);
		if(!s) {
			LIBRDF_FREE(cstring, uri_string);
			return NULL;
		}
		sprintf(s, "[%s]", uri_string);
		LIBRDF_FREE(cstring, uri_string);
		break;
	case LIBRDF_NODE_TYPE_LITERAL:
		s=(char*)LIBRDF_MALLOC(cstring,
				       node->value.literal.string_len + 1);
		if(!s)
			return NULL;
                /* use strcpy here to add \0 to end of literal string */
		strcpy(s, node->value.literal.string);
		break;
	default:
		LIBRDF_FATAL2(librdf_node_string, 
			      "Illegal node type %d seen\n", node->type);
	}
	return s;
}


/**
 * librdf_node_get_digest:
 * @node: the node object
 * 
 * Get a new digest representing the node content.
 * 
 * Return value: a new &librdf_digest object or NULL on failure
 **/
librdf_digest*
librdf_node_get_digest(librdf_node* node) 
{
	librdf_digest* d;
	char *s;
  
	if(node->type == LIBRDF_NODE_TYPE_RESOURCE) {
		d=librdf_uri_get_digest(node->value.resource.uri);
	} else {
		/* LIBRDF_NODE_TYPE_LITERAL */
		s=node->value.literal.string;
		d=librdf_new_digest(librdf_node_digest_factory);
		if(!d)
			return NULL;
		librdf_digest_init(d);
		librdf_digest_update(d, s, node->value.literal.string_len);
		librdf_digest_final(d);
	}
    
	return d;
}



#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
	librdf_node* node;
	char *hp_string1="http://www.ilrt.bristol.ac.uk/people/cmdjb/";
	char *hp_string2="http://purl.org/net/dajobe/";
	librdf_uri *uri, *uri2;
	
	char *program=argv[0];
	
	fprintf(stderr, "%s: Creating home page node from string\n", program);
	node=librdf_new_node_from_uri_string(hp_string1);
	
	fprintf(stderr, "%s: Home page URI is ", program);
	librdf_uri_print(librdf_node_get_uri(node), stderr);
	fputs("\n", stderr);

	fprintf(stderr, "%s: Creating URI from string '%s'\n", program, 
		hp_string2);
	uri=librdf_new_uri(hp_string2);
	fprintf(stderr, "%s: Setting node URI to new URI ", program);
	librdf_uri_print(uri, stderr);
	fputs("\n", stderr);
	
        /* now uri is owned by node - do not free */
	librdf_node_set_uri(node, uri);

	uri2=librdf_node_get_uri(node);
	fprintf(stderr, "%s: Node now has URI ", program);
	librdf_uri_print(uri2, stderr);
	fputs("\n", stderr);
	

	fprintf(stderr, "%s: Freeing node\n", program);
	librdf_free_node(node);
	
	librdf_memory_report(stderr);
	
	/* keep gcc -Wall happy */
	return(0);
}

#endif
