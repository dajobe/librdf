/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_serializer_rdfxml.c - Dumb RDF/XML Serializer
 *
 * $Id$
 *
 * Copyright (C) 2002-2003 David Beckett - http://purl.org/net/dajobe/
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
#include <string.h>
#include <ctype.h>

#include <librdf.h>
#include <raptor.h>


typedef struct {
  librdf_serializer *serializer;

  int depth;
  raptor_namespace_stack *nstack;
} librdf_serializer_rdfxml_context;


/* local prototypes */
static void librdf_serializer_rdfxml_raptor_error_handler(void *data, const char *message, ...);


/**
 * librdf_serializer_rdfxml_init - Initialise the RDF/XML serializer
 * @serializer: the serializer
 * @context: context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_serializer_rdfxml_init(librdf_serializer *serializer, void *context) 
{
  librdf_world *world=serializer->world;
  librdf_serializer_rdfxml_context* pcontext=(librdf_serializer_rdfxml_context*)context;
  raptor_uri_handler *uri_handler;
  void *uri_context;
  
  pcontext->serializer = serializer;

  raptor_uri_get_handler(&uri_handler, &uri_context);
  pcontext->nstack=raptor_namespaces_new(uri_handler, uri_context,
                                         librdf_serializer_rdfxml_raptor_error_handler, world,
                                         1);

  pcontext->depth=0;
  
  return 0;
}


/**
 * librdf_serializer_rdfxml_terminate - Terminate the RDF/XML serializer
 * @serializer: the serializer
 * @context: context
 * 
 * Return value: non 0 on failure
 **/
static void
librdf_serializer_rdfxml_terminate(void *context) 
{
  librdf_serializer_rdfxml_context* pcontext=(librdf_serializer_rdfxml_context*)context;

  raptor_namespaces_free(pcontext->nstack);
}


/**
 * rdf_serializer_rdfxml_ok_xml_name - check name is OK XML Name
 * @name: XML name to check
 * 
 * An XML name starts with alpha or _, continues with alnum or _ - .
 * 
 * Return value: non-zero if is a legal XML name
 **/
static int
rdf_serializer_rdfxml_ok_xml_name(char *name) 
{
  if(!isalpha(*name) && *name != '_')
    return 0;
  name++;
  while(*name) {
    if(!isalnum(*name)  && *name != '_' && *name != '-'  && *name != '.')
      return 0;
    name++;
  }
  return 1;
}


/**
 * rdf_serializer_rdfxml_print_as_xml_content - Print the given string XML-escaped for element content
 * @p: Content string
 * @handle: FILE* to print to
 * 
 **/
static void
rdf_serializer_rdfxml_print_as_xml_content(char *p, FILE *handle) 
{
  while(*p) {
    if(*p == '&')
      fputs("&amp;", handle);
    else if (*p == '<')
      fputs("&lt;", handle);
    else if (*p == '>')
      fputs("&gt;", handle);
    else if (*p > 0x7e)
      fprintf(handle, "&#%d;", *p);
    else
      fputc(*p, handle);
    p++;
  }
}


static void
librdf_serializer_rdfxml_raptor_error_handler(void *data, const char *message, ...) 
{
  librdf_world* world=(librdf_world*)data;
  static const char *message_prefix=" - Raptor serializing error - ";
  int prefix_len=strlen(message_prefix);
  int message_len=strlen(message);
  char *buffer;
  va_list arguments;

  va_start(arguments, message);

  buffer=(char*)LIBRDF_MALLOC(cstring, prefix_len+message_len+1);
  if(!buffer) {
    fprintf(stderr, "librdf_serializer_rdfxml_raptor_error_handler: Out of memory\n");
    return;
  }
  strncpy(buffer, message_prefix, prefix_len);
  strcpy(buffer+prefix_len, message); /* want extra \0 - using strcpy */

  librdf_error_varargs(world, buffer, arguments);
  LIBRDF_FREE(cstring, buffer);

  va_end(arguments);
}



/**
 * rdf_serializer_rdfxml_print_xml_attribute - Print the attribute/value as an XML attribute, XML escaped
 * @world: &librdf_world world
 * @attr: attribute name
 * @value: attribute value
 * @handle: FILE* to print to
 * 
 **/
static void
rdf_serializer_rdfxml_print_xml_attribute(librdf_world *world,
                                          char *attr, char *value,
                                          FILE *handle) 
{
  size_t attr_len;
  size_t len;
  size_t escaped_len;
  unsigned char *buffer;
  unsigned char *p;
  
  attr_len=strlen(attr);
  len=strlen(value);

  escaped_len=raptor_xml_escape_string(value, len,
                                       NULL, 0, '"',
                                       librdf_serializer_rdfxml_raptor_error_handler, world);

  buffer=(char*)LIBRDF_MALLOC(cstring, 1 + attr_len + 2 + escaped_len + 1 +1);
  if(!buffer)
    return;
  p=buffer;
  *p++=' ';
  strncpy(p, attr, attr_len);
  p+= attr_len;
  *p++='=';
  *p++='"';
  raptor_xml_escape_string(value, len,
                           p, escaped_len, '"',
                           librdf_serializer_rdfxml_raptor_error_handler, world);
  p+= escaped_len;
  *p++='"';
  *p++='\0';
  
  fputs(buffer, handle);
  LIBRDF_FREE(cstring, buffer);
}


/**
 * librdf_serializer_print_statement_as_rdfxml - Print the given statement as RDF/XML
 * @context: serializer context
 * @statement: &librdf_statement to print
 * @handle: FILE* to print to
 * 
 **/
static void
librdf_serializer_print_statement_as_rdfxml(librdf_serializer_rdfxml_context *context,
                                            librdf_statement * statement,
                                            FILE *handle) 
{
  librdf_world* world=statement->world;
  librdf_node* nodes[3];
  char* uris[3];
  char *name=NULL;  /* where to split predicate name */
  char *rdf_ns_uri=librdf_uri_as_string(librdf_concept_ms_namespace_uri);
  int rdf_ns_uri_len=strlen(rdf_ns_uri);
  int name_is_rdf_ns=0;
  int i;
  char* nsprefix="ns0";
  char *content;
  
  nodes[0]=librdf_statement_get_subject(statement);
  nodes[1]=librdf_statement_get_predicate(statement);
  nodes[2]=librdf_statement_get_object(statement);

  for (i=0; i<3; i++) {
    librdf_node* n=nodes[i];
    
    if(librdf_node_get_type(n) == LIBRDF_NODE_TYPE_RESOURCE) {
      uris[i]=librdf_uri_as_string(librdf_node_get_uri(n));

      if(i == 1) {
        char *p;

        if(!strncmp(uris[i], rdf_ns_uri, rdf_ns_uri_len)) {
          name=uris[i] + rdf_ns_uri_len;
          name_is_rdf_ns=1;
          nsprefix="rdf";
          continue;
        }
        
        p= uris[i] + strlen(uris[i])-1;
        while(p >= uris[i]) {
          if(rdf_serializer_rdfxml_ok_xml_name(p))
            name=p;
          else if(name)
            /* this char isn't in name, but got name earlier, so stop */
            break;
          p--;
        }

        if(!name) {
          librdf_serializer_warning(context->serializer, "Cannot split predicate URI %s into an XML qname - skipping statement", uris[1]);
          return;
        }

      }

    }
  }

  fputs("  <rdf:Description", handle);


  /* subject */
  if(librdf_node_get_type(nodes[0]) == LIBRDF_NODE_TYPE_BLANK)
    rdf_serializer_rdfxml_print_xml_attribute(world, "rdf:nodeID", 
                                              librdf_node_get_blank_identifier(nodes[0]),
                                              handle);
  else
    rdf_serializer_rdfxml_print_xml_attribute(world, "rdf:about", 
                                              uris[0], handle);

  fputs(">\n", handle);

  fputs("    <", handle);
  fputs(nsprefix, handle);
  fputc(':', handle);
  fputs(name, handle);

  if(!name_is_rdf_ns) {
    size_t len=name-uris[1];
    size_t escaped_len;
    unsigned char *buffer;
    unsigned char *p;

    fputs(" xmlns:", handle);
    fputs(nsprefix, handle);
    fputc('=', handle);

    escaped_len=raptor_xml_escape_string(uris[1], len,
                                         NULL, 0, '"', 
                                         librdf_serializer_rdfxml_raptor_error_handler, world);
    /* " + string + " + \0 */
    buffer=(char*)LIBRDF_MALLOC(cstring, 1 + escaped_len + 1 + 1);
    if(!buffer)
      return;

    p=buffer;
    *p++='"';
    raptor_xml_escape_string(uris[1], len, 
                             p, escaped_len, '"', 
                             librdf_serializer_rdfxml_raptor_error_handler, world);
    p+= escaped_len;
    *p++='"';
    *p='\0';

    fputs(buffer, handle);
    LIBRDF_FREE(cstring, buffer);
  }

  switch(librdf_node_get_type(nodes[2])) {
    case LIBRDF_NODE_TYPE_LITERAL:
      if(librdf_node_get_literal_value_language(nodes[2]))
        rdf_serializer_rdfxml_print_xml_attribute(world, "xml:lang",
                                                  librdf_node_get_literal_value_language(nodes[2]),
                                                  handle);

      content=librdf_node_get_literal_value(nodes[2]);
      
      if(librdf_node_get_literal_value_is_wf_xml(nodes[2])) {
        fputs(" rdf:parseType=\"Literal\">", handle);
        /* Print without escaping XML */
        fputs(content, handle);
      } else {
        librdf_uri *duri=librdf_node_get_literal_value_datatype_uri(nodes[2]);
        if(duri)
          rdf_serializer_rdfxml_print_xml_attribute(world, "rdf:datatype",
                                                    librdf_uri_as_string(duri),
                                                    handle);

        fputc('>', handle);

        rdf_serializer_rdfxml_print_as_xml_content(content, handle);
      }

      fputs("</", handle);
      fputs(nsprefix, handle);
      fputc(':', handle);
      fputs(name, handle);
      fputc('>', handle);
      break;
    case LIBRDF_NODE_TYPE_BLANK:
      rdf_serializer_rdfxml_print_xml_attribute(world, "rdf:nodeID",
                                                librdf_node_get_blank_identifier(nodes[2]), handle);
      fputs("/>", handle);
      break;

    case LIBRDF_NODE_TYPE_RESOURCE:
      /* must be URI */
      rdf_serializer_rdfxml_print_xml_attribute(world, "rdf:resource",
                                                uris[2], handle);
      fputs("/>", handle);
      break;
      
    default:
      LIBRDF_ERROR2(statement->world, 
                    librdf_serializer_print_statement_as_rdfxml,
                    "Do not know how to serialize node type %d\n", 
                    librdf_node_get_type(nodes[2]));
      return;
  }

  fputc('\n', handle);

  fputs("  </rdf:Description>\n", handle);
}


/**
 * librdf_serializer_rdfxml_serialize_model - Print the given model as RDF/XML
 * @context: serializer context
 * @handle: FILE* to print to
 * @base_uri: base URI of model
 * @model: &librdf_model to print
 * 
 * Formats the model as complete RDF/XML document.
 * 
 * Return value: non-zero on failure
 **/
static int
librdf_serializer_rdfxml_serialize_model(void *context,
                                         FILE *handle, librdf_uri* base_uri,
                                         librdf_model *model) 
{
  librdf_serializer_rdfxml_context* pcontext=(librdf_serializer_rdfxml_context*)context;
  raptor_namespace *ns;
  unsigned char *buffer;
  librdf_stream *stream=librdf_model_as_stream(model);

  if(!stream)
    return 1;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", handle);

  pcontext->depth++;
  ns=raptor_namespace_new(pcontext->nstack,
                          "rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#",
                          pcontext->depth);

  raptor_namespaces_start_namespace(pcontext->nstack, ns);

  buffer=raptor_namespaces_format(ns, NULL);

  fputs("<rdf:RDF ", handle);
  fputs(buffer, handle);
  SYSTEM_FREE(buffer);
  fputs(">\n", handle);

  while(!librdf_stream_end(stream)) {
    librdf_statement *statement=librdf_stream_get_object(stream);
    librdf_serializer_print_statement_as_rdfxml(pcontext, statement, handle);
    fputc('\n', handle);
    librdf_stream_next(stream);
  }
  librdf_free_stream(stream);

  fputs("</rdf:RDF>\n", handle);

  raptor_namespaces_end_for_depth(pcontext->nstack, pcontext->depth);
  pcontext->depth--;

  return 0;
}



/**
 * librdf_serializer_rdfxml_register_factory - Register the N-riples serializer with the RDF serializer factory
 * @factory: factory
 * 
 **/
static void
librdf_serializer_rdfxml_register_factory(librdf_serializer_factory *factory) 
{
  factory->context_length = sizeof(librdf_serializer_rdfxml_context);
  
  factory->init  = librdf_serializer_rdfxml_init;
  factory->terminate  = librdf_serializer_rdfxml_terminate;
  factory->serialize_model = librdf_serializer_rdfxml_serialize_model;
}


/**
 * librdf_serializer_rdfxml_constructor - Initialise the rdfxml RDF serializer module
 * @world: redland world object
 **/
void
librdf_serializer_rdfxml_constructor(librdf_world *world)
{
  librdf_serializer_register_factory(world, "rdfxml", "application/rdf+xml",
                                     "http://www.w3.org/TR/rdf-syntax-grammar/",
                                     &librdf_serializer_rdfxml_register_factory);
}
