/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_serializer_raptor.c - RDF Serializers via Raptor
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

#include <librdf.h>


#define RAPTOR_IN_REDLAND
#include <raptor.h>


typedef struct {
  librdf_serializer *serializer;        /* librdf serializer object */
} librdf_serializer_raptor_context;


/**
 * librdf_serializer_raptor_init - Initialise the N-Triples RDF serializer
 * @serializer: the serializer
 * @context: context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_serializer_raptor_init(librdf_serializer *serializer, void *context) 
{
  librdf_serializer_raptor_context* pcontext=(librdf_serializer_raptor_context*)context;

  pcontext->serializer = serializer;

  return 0;
}


static void
librdf_serializer_print_statement_as_ntriple(librdf_statement * statement,
                                             FILE *stream) 
{
  librdf_node *subject=librdf_statement_get_subject(statement);
  librdf_node *predicate=librdf_statement_get_predicate(statement);
  librdf_node *object=librdf_statement_get_object(statement);
  char *lang=NULL;
  librdf_uri *dt_uri=NULL;
  
  if(librdf_node_is_blank(subject))
    fprintf(stream, "_:%s", librdf_node_get_blank_identifier(subject));
  else if(librdf_node_is_resource(subject)) {
    /* Must be a URI */
    fputc('<', stream);
    raptor_print_ntriples_string(stream, librdf_uri_as_string(librdf_node_get_uri(subject)), '\0');
    fputc('>', stream);
  } else {
    LIBRDF_ERROR2(statement->world, "Do not know how to print triple subject type %d\n", librdf_node_get_type(subject));
    return;
  }

  if(!librdf_node_is_resource(predicate)) {
    LIBRDF_ERROR2(statement->world, "Do not know how to print triple predicate type %d\n", librdf_node_get_type(predicate));
    return;
  }

  fputc(' ', stream);
  fputc('<', stream);
  raptor_print_ntriples_string(stream, librdf_uri_as_string(librdf_node_get_uri(predicate)), '\0');
  fputc('>', stream);
  fputc(' ', stream);

  switch(librdf_node_get_type(object)) {
    case LIBRDF_NODE_TYPE_LITERAL:
      fputc('"', stream);
      raptor_print_ntriples_string(stream, librdf_node_get_literal_value(object), '"');
      fputc('"', stream);
      lang=librdf_node_get_literal_value_language(object);
      dt_uri=librdf_node_get_literal_value_datatype_uri(object);
      if(lang) {
        fputc('@', stream);
        fputs(lang, stream);
      }
      if(dt_uri) {
        fputs("^^<", stream); 
        raptor_print_ntriples_string(stream, librdf_uri_as_string(dt_uri), '\0');
        fputc('>', stream);
      }
      break;
    case LIBRDF_NODE_TYPE_BLANK:
      fputs("_:", stream);
      fputs((const char*)librdf_node_get_blank_identifier(object), stream);
      break;
    case LIBRDF_NODE_TYPE_RESOURCE:
      fputc('<', stream);
      raptor_print_ntriples_string(stream, librdf_uri_as_string(librdf_node_get_uri(object)), '\0');
      fputc('>', stream);
      break;
    default:
      LIBRDF_ERROR2(statement->world, "Do not know how to print triple object type %d\n", librdf_node_get_type(object));
      return;
  }
  fputs(" .", stream);
}


static int
librdf_serializer_raptor_serialize_model(void *context,
                                         FILE *handle, librdf_uri* base_uri,
                                         librdf_model *model) 
{
  /* librdf_serializer_raptor_context* pcontext=(librdf_serializer_raptor_context*)context; */

  librdf_stream *stream=librdf_model_as_stream(model);
  if(!stream)
    return 1;
  while(!librdf_stream_end(stream)) {
    librdf_statement *statement=librdf_stream_get_object(stream);
    librdf_serializer_print_statement_as_ntriple(statement, handle);
    fputc('\n', handle);
    librdf_stream_next(stream);
  }
  librdf_free_stream(stream);
  return 0;
}



/**
 * librdf_serializer_raptor_register_factory - Register the N-riples serializer with the RDF serializer factory
 * @factory: factory
 * 
 **/
static void
librdf_serializer_raptor_register_factory(librdf_serializer_factory *factory) 
{
  factory->context_length = sizeof(librdf_serializer_raptor_context);
  
  factory->init  = librdf_serializer_raptor_init;
  factory->serialize_model = librdf_serializer_raptor_serialize_model;
}


/**
 * librdf_serializer_raptor_constructor - Initialise the raptor RDF serializer module
 * @world: redland world object
 **/
void
librdf_serializer_raptor_constructor(librdf_world *world)
{
  librdf_serializer_register_factory(world, "ntriples", "text/plain",
                                     (const unsigned char*)"http://www.w3.org/TR/rdf-testcases/#ntriples",
                                     &librdf_serializer_raptor_register_factory);
}
