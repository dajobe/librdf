/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_node.c - RDF Node implementation
 *   RDF:Resource
 *   RDF:Property
 *   (object) - RDF:Literal / RDF:Resource
 *
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


#include <rdf_config.h>

#include <stdio.h>

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_digest.h>


/* statics */
static librdf_digest_factory *librdf_node_digest_factory=NULL;

/**
 * librdf_init_node - Initialise the node module.
 * @factory: digest factory to use for digesting node content.
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
 * librdf_new_node - Constructor - create a new librdf_node object with a NULL URI.
 * 
 * Return value: a new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node(void)
{
  return librdf_new_node_from_uri_string((char*)NULL);
}

    

/**
 * librdf_new_node_from_uri_string - Constructor - create a new librdf_node object from a URI string
 * @uri_string: string representing a URI
 * 
 * The URI can be NULL, and can be set later.
 *
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
  if(librdf_node_set_uri(new_node, new_uri)) {
    librdf_free_node(new_node);
    return NULL;
  }
  return new_node;
}

    

/* Create a new Node and set the URI. */

/**
 * librdf_new_node_from_uri - Constructor - create a new librdf_node object with a given URI
 * @uri: &rdf_uri object
 *
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
 * librdf_new_node_from_literal -  Constructor - create a new literal librdf_node object
 * @string: literal string value
 * @xml_language: literal XML language (or NULL)
 * @xml_space: XML space properties (0 if unknown)
 * @is_wf_xml: non 0 if literal is XML
 * 
 * Return value: new &librdf_node object or NULL on failure
 **/
librdf_node*
librdf_new_node_from_literal(char *string, char *xml_language, 
                             int xml_space, int is_wf_xml) 
{
  librdf_node* new_node;
  
  new_node = (librdf_node*)LIBRDF_CALLOC(librdf_node, 1,
                                         sizeof(librdf_node));
  if(!new_node)
    return NULL;
  
  /* set type */
  new_node->type=LIBRDF_NODE_TYPE_LITERAL;
  
  if (librdf_node_set_literal_value(new_node, string, xml_language,
                                    xml_space, is_wf_xml)) {
    librdf_free_node(new_node);
    return NULL;
  }
  
  return new_node;
}


/**
 * librdf_new_node_from_node - Copy constructor - create a new librdf_node object from an existing  librdf_node object
 * @node: &librdf_node object to copy
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
  
  new_node->type=node->type;

  if(node->type == LIBRDF_NODE_TYPE_RESOURCE) {
    new_uri=librdf_new_uri_from_uri(node->value.resource.uri);
    if(!new_uri){
      LIBRDF_FREE(librdf_node, new_node);
      return NULL;
    }
    if(librdf_node_set_uri(new_node, new_uri)) {
      librdf_free_uri(new_uri);
      LIBRDF_FREE(librdf_node, new_node);
      return NULL;
    } 
  } else {
    /* must be a LIBRDF_NODE_TYPE_LITERAL */
    if (librdf_node_set_literal_value(new_node,
                                      node->value.literal.string,
                                      node->value.literal.xml_language,
                                      node->value.literal.xml_space,
                                      node->value.literal.is_wf_xml)) {
      LIBRDF_FREE(librdf_node, new_node);
      return NULL;
    }
  }
  
  return new_node;
}


/**
 * librdf_free_node - Destructor - destroy an librdf_node object
 * @node: &librdf_node object
 * 
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
 * librdf_node_get_uri - Get the URI for a node object
 * @node: the node object
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
 * librdf_node_set_uri -  Set the URI of the node
 * @node: the node object
 * @uri: the uri object
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
    return 1;
  
  /* delete old URI */
  if(node->value.resource.uri)
    librdf_free_uri(node->value.resource.uri);
  
  /* set new one */
  node->value.resource.uri=uri;
  return 0;
}


/**
 * librdf_node_get_type - Get the type of the node
 * @node: the node object
 * 
 * Return value: the node type
 **/
librdf_node_type
librdf_node_get_type(librdf_node* node) 
{
  return node->type;
}


/**
 * librdf_node_set_type - Set the type of the node
 * @node: the node object
 * @type: the node type
 *
 * Presently the nodes can be of type LIBRDF_NODE_TYPE_RESOURCE or
 * LIBRDF_NODE_TYPE_LITERAL
 **/
void
librdf_node_set_type(librdf_node* node, librdf_node_type type)
{
  node->type=type;
}


/**
 * librdf_node_get_literal_value - Get the string literal value of the node
 * @node: the node object
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
 * librdf_node_get_literal_value_language - Get the XML language of the node
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
 * librdf_node_get_literal_value_is_wf_xml - Get the XML well-formness property of the node
 * @node: the node object
 * 
 * Return value: 0 if the XML literal is NOT well formed XML content, or the node is not a literal
 **/
int
librdf_node_get_literal_value_is_wf_xml(librdf_node* node) 
{
  if(node->type != LIBRDF_NODE_TYPE_LITERAL)
    return 0;
  return node->value.literal.is_wf_xml;
}


/**
 * librdf_node_get_literal_value_xml_space - Get the XML space property of the node
 * @node: the node object
 * 
 * See librdf_node_set_literal_value() for the legal xml:space values.
 *
 * Return value: the xml:space property of the literal or -1 if the node is not a literal
 **/
int
librdf_node_get_literal_value_xml_space(librdf_node* node) 
{
  if(node->type != LIBRDF_NODE_TYPE_LITERAL)
    return -1;
  return node->value.literal.xml_space;
}


/**
 * librdf_node_set_literal_value - Set the node literal value with options
 * @node: the node object
 * @value: pointer to the literal string value
 * @xml_language: pointer to the literal language (or NULL if not defined)
 * @xml_space: XML space properties (0 if unknown)
 * @is_wf_xml: non 0 if the value is Well Formed XML
 * 
 * Sets the node literal value, optional language, XML space
 * properties and if the literal is well formed XML.
 *
 * The space property can take three values: 0 - unknown, 1 - use default
 * method (xml:space="default") or 2 - preserve space (xml:space="preserve").
 * 
 * Return value: non 0 on failure
 **/
int
librdf_node_set_literal_value(librdf_node* node, char* value,
			      char *xml_language, int xml_space,
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
  strcpy(new_value, value);
  
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

  /* FIXME: whatever next, parameter validation? */
  if(xml_space < LIBRDF_NODE_LITERAL_XML_SPACE_DEFAULT ||
     xml_space > LIBRDF_NODE_LITERAL_XML_SPACE_PRESERVE )
    xml_space=LIBRDF_NODE_LITERAL_XML_SPACE_UNKNOWN;
  
  node->value.literal.xml_space=(librdf_node_literal_xml_space)xml_space;
  node->value.literal.is_wf_xml=is_wf_xml;
  
  return 0;
}


/**
 * librdf_node_to_string - Format the node as a string
 * @node: the node object
 * 
 * Note a new string is allocated which must be freed by the caller.
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
    s=(char*)LIBRDF_MALLOC(cstring, node->value.literal.string_len + 1);
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
 * librdf_node_print - pretty print the node to a file descriptor
 * @node: the node
 * @fh: file handle
 **/
void
librdf_node_print(librdf_node* node, FILE *fh)
{
  char* s;

  if(!node)
    return;
  
  s=librdf_node_to_string(node);
  if(!s)
    return;
  fputs(s, fh);
  LIBRDF_FREE(cstring, s);
}



/**
 * librdf_node_get_digest - Get a digest representing a librdf_node
 * @node: the node object
 * 
 * A new digest object is created which must be freed by the caller.
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
    d=librdf_new_digest_from_factory(librdf_node_digest_factory);
    if(!d)
      return NULL;

    librdf_digest_init(d);
    librdf_digest_update(d, (unsigned char*)s, node->value.literal.string_len);
    librdf_digest_final(d);
  }
  
  return d;
}


/**
 * librdf_node_equals - Compare two librdf_node objects for equality
 * @first_node: first &librdf_node node
 * @second_node: second &librdf_node node
 * 
 * Note - for literal nodes, XML language, XML space and well-formness are 
 * presently ignored in the comparison.
 * 
 * Return value: non 0 if nodes are equal
 **/
int
librdf_node_equals(librdf_node* first_node, librdf_node* second_node) 
{
  int status;
  
  if(first_node->type != first_node->type)
    return 0;
  
  if(first_node->type == LIBRDF_NODE_TYPE_RESOURCE) {
    return librdf_uri_equals(first_node->value.resource.uri,
                             second_node->value.resource.uri);
  } else {
    if(first_node->value.literal.string_len != second_node->value.literal.string_len)
      return 0;

    status=strcmp(first_node->value.literal.string,
                  second_node->value.literal.string);
    if(!status)
      return 1;

    /* FIXME: compare xml_language, xml_space and is_wf_xml ?  Probably not */
  }

  return 0;
}



/**
 * librdf_node_encode - Serialise a node into a buffer
 * @node: the node to serialise
 * @buffer: the buffer to use
 * @length: buffer size
 * 
 * Encodes the given node in the buffer, which must be of sufficient
 * size.  If buffer is NULL, no work is done but the size of buffer
 * required is returned.
 * 
 * Return value: the number of bytes written or 0 on failure.
 **/
int
librdf_node_encode(librdf_node* node, char *buffer, size_t length)
{
  int total_length=0;
  char *string;
  int string_length;
  
  switch(node->type) {
  case LIBRDF_NODE_TYPE_RESOURCE:
    string=librdf_uri_as_string(node->value.resource.uri);
    string_length=strlen(string);

    total_length=3 + string_length + 1; /* +1 for \0 at end */

    if(length && total_length > length)
      return 0;    
    
    if(buffer) {
      buffer[0]='R';
      buffer[1]=(string_length & 0xff00) >> 8;
      buffer[2]=(string_length & 0x00ff);
      strcpy(buffer+3, string);
    }
    break;

  case LIBRDF_NODE_TYPE_LITERAL:
    string=node->value.literal.string;
    string_length=node->value.literal.string_len;

    total_length=6 + string_length + 1; /* +1 for \0 at end */
    
    if(length && total_length > length)
      return 0;    
    
    if(buffer) {
      buffer[0]='L';
      buffer[1]=(node->value.literal.is_wf_xml & 0xf) << 8 |
                (node->value.literal.xml_space & 0xf);
      buffer[2]=(string_length & 0xff00) >> 8;
      buffer[3]=(string_length & 0x00ff);
      buffer[4]=0; /* FIXME - xml:language */
      buffer[5]=0;
      strcpy(buffer+6, string);
    }
    break;

  default:
    LIBRDF_FATAL2(librdf_node_encode, 
                  "Illegal node type %d seen\n", node->type);

  }
  
  return total_length;
}


/**
 * librdf_node_decode - Deserialise a node from a buffer
 * @node: the node to deserialise into
 * @buffer: the buffer to use
 * @length: buffer size
 * 
 * Decodes the serialised node (as created by librdf_node_encode() )
 * from the given buffer.
 * 
 * Return value: non 0 on failure (bad encoding, allocation failure)
 **/
int
librdf_node_decode(librdf_node* node, char *buffer, size_t length)
{
  int is_wf_xml;
  int xml_space;
  int string_length;
  librdf_uri* new_uri;

  /* absolute minimum - first byte is type */
  if (length < 1)
    return 1;

  switch(buffer[0]) {
    case 'R': /* LIBRDF_NODE_TYPE_RESOURCE */
      /* min */
      if(length < 3)
        return 1;
      
      string_length=(buffer[1] << 8) | buffer[2];
      
      /* Now initialise fields */
      node->type = LIBRDF_NODE_TYPE_RESOURCE;
      new_uri=librdf_new_uri(buffer+3);
      if (!new_uri)
        return 1;
      
      if(librdf_node_set_uri(node, new_uri)) {
        librdf_free_uri(new_uri);
        return 1;
      }
      
      break;

    case 'L': /* LIBRDF_NODE_TYPE_LITERAL */
      /* min */
      if(length < 6)
        return 1;
      
      is_wf_xml=(buffer[1] & 0xf0)>>8;
      xml_space=(buffer[1] & 0x0f)>>8;
      string_length=(buffer[2] << 8) | buffer[3];
      
      /* FIXME - xml:language length in 4, 5 */
      
      /* Now initialise fields */
      node->type = LIBRDF_NODE_TYPE_LITERAL;
      if (librdf_node_set_literal_value(node, buffer+6,
                                        NULL, /* xml:language */
                                        xml_space, is_wf_xml))
        return 1;
    
    break;

  default:
    LIBRDF_FATAL2(librdf_node_decode, "Illegal node encoding %d seen\n",
                  buffer[0]);

  }
  
  return 0;
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_node *node, *node2;
  char *hp_string1="http://www.ilrt.bristol.ac.uk/people/cmdjb/";
  char *hp_string2="http://purl.org/net/dajobe/";
  librdf_uri *uri, *uri2;
  int size, size2;
  char *buffer;
  
  char *program=argv[0];
	
  librdf_init_digest();
  librdf_init_hash();
  librdf_init_uri(librdf_get_digest_factory(NULL), NULL);

  fprintf(stdout, "%s: Creating home page node from string\n", program);
  node=librdf_new_node_from_uri_string(hp_string1);
  
  fprintf(stdout, "%s: Home page URI is ", program);
  librdf_uri_print(librdf_node_get_uri(node), stdout);
  fputs("\n", stdout);
  
  fprintf(stdout, "%s: Creating URI from string '%s'\n", program, 
          hp_string2);
  uri=librdf_new_uri(hp_string2);
  fprintf(stdout, "%s: Setting node URI to new URI ", program);
  librdf_uri_print(uri, stdout);
  fputs("\n", stdout);
  
  /* now uri is owned by node - do not free */
  librdf_node_set_uri(node, uri);
  
  uri2=librdf_node_get_uri(node);
  fprintf(stdout, "%s: Node now has URI ", program);
  librdf_uri_print(uri2, stdout);
  fputs("\n", stdout);


  fprintf(stdout, "%s: Node is: ", program);
  librdf_node_print(node, stdout);
  fputs("\n", stdout);

  size=librdf_node_encode(node, NULL, 0);
  fprintf(stdout, "%s: Encoding node requires %d bytes\n", program, size);
  buffer=(char*)LIBRDF_MALLOC(cstring, size);

  fprintf(stdout, "%s: Encoding node in buffer\n", program);
  size2=librdf_node_encode(node, buffer, size);
  if(size2 != size) {
    fprintf(stderr, "%s: Encoding node used %d bytes, expected it to use %d\n", program, size2, size);
    return(1);
  }
  
    
  fprintf(stdout, "%s: Creating new node\n", program);
  node2=librdf_new_node();

  fprintf(stdout, "%s: Decoding node from buffer\n", program);
  if(librdf_node_decode(node2, buffer, size)) {
    fprintf(stderr, "%s: Decoding node failed\n", program);
    return(1);
  }
  LIBRDF_FREE(cstring, buffer);
   
  fprintf(stdout, "%s: New node is: ", program);
  librdf_node_print(node2, stdout);
  fputs("\n", stdout);
 
  
  
  fprintf(stdout, "%s: Freeing nodes\n", program);
  librdf_free_node(node2);
  librdf_free_node(node);
  
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
