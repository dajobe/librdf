/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_serializer_raptor.c - RDF Serializers via Raptor
 *
 * $Id$
 *
 * Copyright (C) 2002-2004 David Beckett - http://purl.org/net/dajobe/
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


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <redland.h>


typedef struct {
  librdf_serializer *serializer;        /* librdf serializer object */
  raptor_serializer *rdf_serializer;    /* raptor serializer object */
  char *serializer_name;                /* raptor serializer name to use */

  int errors;
  int warnings;
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
  librdf_serializer_raptor_context* scontext=(librdf_serializer_raptor_context*)context;

  scontext->serializer = serializer;
  scontext->serializer_name=scontext->serializer->factory->name;

  scontext->rdf_serializer=raptor_new_serializer(scontext->serializer_name);
  if(!scontext->rdf_serializer)
    return 1;

  return 0;
}


/**
 * librdf_serializer_raptor_terminate - Terminate the raptor RDF serializer
 * @context: context
 **/
static void
librdf_serializer_raptor_terminate(void *context) 
{
  librdf_serializer_raptor_context* scontext=(librdf_serializer_raptor_context*)context;
  
  if(scontext->rdf_serializer)
    raptor_free_serializer(scontext->rdf_serializer);
}
  

static int
librdf_serializer_raptor_serialize_statement(raptor_serializer *rserializer,
                                             librdf_statement* statement)
{
  raptor_statement rstatement;
  librdf_node *subject=librdf_statement_get_subject(statement);
  librdf_node *predicate=librdf_statement_get_predicate(statement);
  librdf_node *object=librdf_statement_get_object(statement);

  if(librdf_node_is_blank(subject)) {
    rstatement.subject=librdf_node_get_blank_identifier(subject);
    rstatement.subject_type=RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
  } else if(librdf_node_is_resource(subject)) {
    rstatement.subject=(raptor_uri*)librdf_node_get_uri(subject);
    rstatement.subject_type=RAPTOR_IDENTIFIER_TYPE_RESOURCE;
    /* or  RAPTOR_IDENTIFIER_TYPE_ORDINAL ? */
  } else {
    librdf_log(statement->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_SERIALIZER, NULL,
               "Do not know how to serialize triple subject type %d\n", librdf_node_get_type(subject));
    return 1;
  }

  if(!librdf_node_is_resource(predicate)) {
    librdf_log(statement->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_SERIALIZER, NULL,
               "Do not know how to print triple predicate type %d\n", librdf_node_get_type(predicate));
    return 1;
  }

  rstatement.predicate=(raptor_uri*)librdf_node_get_uri(predicate);
  rstatement.predicate_type=RAPTOR_IDENTIFIER_TYPE_RESOURCE;

  rstatement.object_literal_language=NULL;
  rstatement.object_literal_datatype=NULL;
  switch(librdf_node_get_type(object)) {
    case LIBRDF_NODE_TYPE_LITERAL:
      rstatement.object=librdf_node_get_literal_value(object);
      rstatement.object_type=RAPTOR_IDENTIFIER_TYPE_LITERAL;
      
      /* or RAPTOR_IDENTIFIER_TYPE_XML_LITERAL ? */
      rstatement.object_literal_language=(const unsigned char*)librdf_node_get_literal_value_language(object);
      rstatement.object_literal_datatype=(raptor_uri*)librdf_node_get_literal_value_datatype_uri(object);
      break;

    case LIBRDF_NODE_TYPE_BLANK:
      rstatement.object=librdf_node_get_blank_identifier(object);
      rstatement.object_type=RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
      break;

    case LIBRDF_NODE_TYPE_RESOURCE:
      /* or  RAPTOR_IDENTIFIER_TYPE_ORDINAL ? */
      rstatement.object=librdf_node_get_uri(object);
      rstatement.object_type=RAPTOR_IDENTIFIER_TYPE_RESOURCE;
      break;
    default:
      librdf_log(statement->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_SERIALIZER, NULL,
                 "Do not know how to print triple object type %d\n", librdf_node_get_type(object));
      return 1;
  }

  return raptor_serialize_statement(rserializer, &rstatement);
}


static void
librdf_serializer_raptor_error_handler(void *data, raptor_locator *locator,
                                       const char *message) 
{
  librdf_serializer_raptor_context* scontext=(librdf_serializer_raptor_context*)data;
  scontext->errors++;

  librdf_log_simple(scontext->serializer->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_SERIALIZER, locator, message);
}


static void
librdf_serializer_raptor_warning_handler(void *data, raptor_locator *locator,
                                         const char *message) 
{
  librdf_serializer_raptor_context* scontext=(librdf_serializer_raptor_context*)data;
  scontext->warnings++;

  librdf_log_simple(scontext->serializer->world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_SERIALIZER, locator, message);
}


static int
librdf_serializer_raptor_serialize_model_to_file_handle(void *context,
                                                        FILE *handle, 
                                                        librdf_uri* base_uri,
                                                        librdf_model *model) 
{
  librdf_serializer_raptor_context* scontext=(librdf_serializer_raptor_context*)context;
  int rc=0;
  
  librdf_stream *stream=librdf_model_as_stream(model);
  if(!stream)
    return 1;

  /* start the serialize */
  rc=raptor_serialize_start_to_file_handle(scontext->rdf_serializer, 
                                           (raptor_uri*)base_uri, handle);
  if(rc)
    return 1;

  scontext->errors=0;
  scontext->warnings=0;
  raptor_serializer_set_error_handler(scontext->rdf_serializer, scontext, 
                                      librdf_serializer_raptor_error_handler);
  raptor_serializer_set_warning_handler(scontext->rdf_serializer, scontext, 
                                        librdf_serializer_raptor_warning_handler);
    
  while(!librdf_stream_end(stream)) {
    librdf_statement *statement=librdf_stream_get_object(stream);
    librdf_serializer_raptor_serialize_statement(scontext->rdf_serializer, 
                                                 statement);
    librdf_stream_next(stream);
  }
  librdf_free_stream(stream);
  raptor_serialize_end(scontext->rdf_serializer);

  return 0;
}



static unsigned char*
librdf_serializer_raptor_serialize_model_to_counted_string(void *context,
                                                           librdf_uri* base_uri,
                                                           librdf_model *model,
                                                           size_t* length_p)
{
  librdf_serializer_raptor_context* scontext=(librdf_serializer_raptor_context*)context;
  raptor_iostream *iostr;
  unsigned char *string=NULL;
  size_t string_length=0;
  int rc=0;
  
  librdf_stream *stream=librdf_model_as_stream(model);
  if(!stream)
    return NULL;

  /* start the serialize */
  iostr=raptor_new_iostream_to_string((void**)&string, &string_length,
                                      malloc);
  if(!iostr) {
    librdf_free_stream(stream);    
    return NULL;
  }
    
  rc=raptor_serialize_start(scontext->rdf_serializer, 
                            (raptor_uri*)base_uri, iostr);

  if(rc) {
    librdf_free_stream(stream);    
    raptor_free_iostream(iostr);
    return NULL;
  }
    
  scontext->errors=0;
  scontext->warnings=0;
  raptor_serializer_set_error_handler(scontext->rdf_serializer, scontext, 
                                      librdf_serializer_raptor_error_handler);
  raptor_serializer_set_warning_handler(scontext->rdf_serializer, scontext, 
                                        librdf_serializer_raptor_warning_handler);
    
  while(!librdf_stream_end(stream)) {
    librdf_statement *statement=librdf_stream_get_object(stream);
    librdf_serializer_raptor_serialize_statement(scontext->rdf_serializer, 
                                                 statement);
    librdf_stream_next(stream);
  }
  librdf_free_stream(stream);
  raptor_serialize_end(scontext->rdf_serializer);

  if(length_p)
    *length_p=string_length;
  
  return string;
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
  factory->terminate = librdf_serializer_raptor_terminate;

  factory->serialize_model_to_file_handle = librdf_serializer_raptor_serialize_model_to_file_handle;
  factory->serialize_model_to_counted_string = librdf_serializer_raptor_serialize_model_to_counted_string;
}


/**
 * librdf_serializer_raptor_constructor - Initialise the raptor RDF serializer module
 * @world: redland world object
 **/
void
librdf_serializer_raptor_constructor(librdf_world *world)
{
  int i;
  
  /* enumerate from serializer 1, so the default serializer 0 is done last */
  for(i=1; 1; i++) {
    const char *syntax_name=NULL;
    const char *mime_type=NULL;
    const unsigned char *uri_string=NULL;

    if(raptor_serializers_enumerate(i, &syntax_name, NULL, 
                                    &mime_type, &uri_string)) {
      /* reached the end of the serializers, now register the default one */
      i=0;
      raptor_serializers_enumerate(i, &syntax_name, NULL,
                                   &mime_type, &uri_string);
    }
    
    librdf_serializer_register_factory(world, syntax_name, mime_type, 
                                       uri_string,
                                       &librdf_serializer_raptor_register_factory);

    if(!i) /* registered default serializer, end */
      break;
  }
}
