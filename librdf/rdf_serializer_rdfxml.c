/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_serializer_rdfxml.c - Dumb RDF/XML Serializer
 *
 * $Id$
 *
 * Copyright (C) 2002 David Beckett - http://purl.org/net/dajobe/
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
#include <stdarg.h>

#include <ctype.h>

#include <librdf.h>


typedef struct {
  librdf_serializer *serializer;
} librdf_serializer_rdfxml_context;


/**
 * librdf_serializer_rdfxml_init - Initialise the N-Triples RDF serializer
 * @serializer: the serializer
 * @context: context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_serializer_rdfxml_init(librdf_serializer *serializer, void *context) 
{
  librdf_serializer_rdfxml_context* pcontext=(librdf_serializer_rdfxml_context*)context;

  pcontext->serializer = serializer;

  return 0;
}


/* check name is OK XML Name - starts with alpha, ends with alnum */
static int
rdf_serializer_rdfxml_ok_xml_name(char *name) 
{
  if(!isalpha(*name) && *name != '_')
    return 0;
  name++;
  while(*name) {
    if(!isalnum(*name))
      return 0;
    name++;
  }
  return 1;
}


static void
librdf_serializer_print_statement_as_rdfxml(librdf_serializer_rdfxml_context *context,
                                            librdf_statement * statement,
                                            FILE *stream) 
{
  librdf_node* nodes[3];
  char* uris[3];
  char *name=NULL;  /* where to split predicate name */
  char *rdf_ns_uri=librdf_uri_as_string(librdf_concept_ms_namespace_uri);
  int rdf_ns_uri_len=strlen(rdf_ns_uri);
  int name_is_rdf_ns=0;
  int i;
  char* nsprefix="ns0";
  librdf_uri *duri;
  
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
          break;
        }
        
        p= uris[i] + strlen(uris[i])-1;
        while(p >= uris[i]) {
          if(rdf_serializer_rdfxml_ok_xml_name(p))
            name=p;
          p--;
        }

        if(!name) {
          librdf_serializer_warning(context->serializer, "Cannot split predicate URI %s into an XML qname - skipping statement", uris[1]);
          return;
        }

      }

    }
  }


  fputs("  <rdf:Description", stream);

  /* subject */
  if(librdf_node_get_type(nodes[0]) == LIBRDF_NODE_TYPE_BLANK)
    fprintf(stream, " rdf:nodeID=\"%s\"",
            librdf_node_get_blank_identifier(nodes[0]));
  else
    fprintf(stream, " rdf:about=\"%s\"", uris[0]);

  fputs(">\n", stream);

  fputs("    <", stream);
  fputs(nsprefix, stream);
  fputc(':', stream);
  fputs(name, stream);

  if(!name_is_rdf_ns) {
    fputs(" xmlns:", stream);
    fputs(nsprefix, stream);
    fputs("=\"", stream);
    fwrite(uris[1], 1, (name-uris[1]), stream);
    fputc('"', stream);
  }
  fputc('>', stream);

  switch(librdf_node_get_type(nodes[2])) {
    case LIBRDF_NODE_TYPE_LITERAL:
      if(librdf_node_get_literal_value_language(nodes[2]))
        fprintf(stream, " xml:lang=\"%s\"",
                librdf_node_get_literal_value_language(nodes[2]));
      duri=librdf_node_get_literal_value_datatype_uri(nodes[2]);
      if(duri)
        fprintf(stream, " rdf:datatype=\"%s\"", librdf_uri_as_string(duri));
      if(librdf_node_get_literal_value_is_wf_xml(nodes[2]))
        fputs(" rdf:parseType=\"Literal\"", stream);

      fputs(librdf_node_get_literal_value(nodes[2]), stream);

      fputs("</", stream);
      fputs(nsprefix, stream);
      fputc(':', stream);
      fputs(name, stream);
      fputc('>', stream);
      break;
    case LIBRDF_NODE_TYPE_BLANK:
      fputs(" rdf:nodeID=\"", stream);
      fputs(librdf_node_get_blank_identifier(nodes[2]), stream);
      fputs("/>", stream);
      break;
    default:
      /* must be URI */
      fprintf(stream, " rdf:resource=\"%s\"/>", uris[2]);
  }

  fputc('\n', stream);

  fputs("  </rdf:Description>\n", stream);
}


static int
librdf_serializer_rdfxml_serialize_model(void *context,
                                         FILE *handle, librdf_uri* base_uri,
                                         librdf_model *model) 
{
  librdf_serializer_rdfxml_context* scontext=(librdf_serializer_rdfxml_context*)context;

  librdf_stream *stream=librdf_model_serialise(model);
  if(!stream)
    return 1;
  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", handle);
  fputs("<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n", handle);
  while(!librdf_stream_end(stream)) {
    librdf_statement *statement=librdf_stream_get_object(stream);
    librdf_serializer_print_statement_as_rdfxml(scontext, statement, handle);
    fputc('\n', handle);
    librdf_stream_next(stream);
  }
  librdf_free_stream(stream);
  fputs("</rdf:RDF>\n", handle);
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
