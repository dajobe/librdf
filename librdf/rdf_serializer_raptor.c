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
#include <stdarg.h>

#include <librdf.h>


#define RAPTOR_IN_REDLAND
#include <raptor.h>
#include <ntriples.h>


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

  if(librdf_node_get_type(subject) == LIBRDF_NODE_TYPE_BLANK)
    fprintf(stream, "_:%s", librdf_node_get_blank_identifier(subject));
  else {
    fputc('<', stream);
    librdf_uri_print(librdf_node_get_uri(subject), stream);
    fputc('>', stream);
  }
  fputc(' ', stream);
  fputc('<', stream);
  librdf_uri_print(librdf_node_get_uri(predicate), stream);
  fputc('>', stream);
  fputc(' ', stream);

  switch(librdf_node_get_type(object)) {
    case LIBRDF_NODE_TYPE_LITERAL:
      fputc('"', stream);
      raptor_print_ntriples_string(stream, librdf_node_get_literal_value(object), '"');
      fputc('"', stream);
      if(librdf_node_get_literal_value_language(object))
        fprintf(stream, "@%s", librdf_node_get_literal_value_language(object));
      if(librdf_node_get_literal_value_is_wf_xml(object))
        fputs("^^<http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral>", stream);
      break;
    case LIBRDF_NODE_TYPE_BLANK:
      fprintf(stream, "_:%s", librdf_node_get_blank_identifier(object));
      break;
    default:
      /* must be URI */
      fputc('<', stream);
      librdf_uri_print(librdf_node_get_uri(object), stream);
      fputc('>', stream);
  }
  fputs(" .", stream);
}


static int
librdf_serializer_raptor_serialize_model(void *context,
                                         FILE *handle, librdf_uri* base_uri,
                                         librdf_model *model) 
{
  /* librdf_serializer_raptor_context* pcontext=(librdf_serializer_raptor_context*)context; */

  librdf_stream *stream=librdf_model_serialise(model);
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
                                     "http://www.w3.org/TR/rdf-testcases/#ntriples",
                                     &librdf_serializer_raptor_register_factory);
}
