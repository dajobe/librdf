/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser using Raptor
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <librdf.h>

#define RAPTOR_IN_REDLAND
#include <raptor.h>
#include <ntriples.h>


/* serialising implementing functions */
static int librdf_parser_raptor_serialise_end_of_stream(void* context);
static int librdf_parser_raptor_serialise_next_statement(void* context);
static void* librdf_parser_raptor_serialise_get_statement(void* context, int flags);
static void librdf_parser_raptor_serialise_finished(void* context);


typedef struct {
  librdf_parser *parser;        /* librdf parser object */
  int is_ntriples;
} librdf_parser_raptor_context;


typedef struct {
  librdf_parser_raptor_context* pcontext; /* parser context */
  raptor_parser *rdf_parser;      /* source URI string (for raptor) */

  FILE *fh;
  
  librdf_uri *source_uri;   /* source URI */
  librdf_uri *base_uri;     /* base URI */

  /* The set of statements pending is a sequence, with 'current'
   * as the first entry and any remaining ones held in 'statements'.
   * The latter are filled by the repat parser
   * sequence is empty := current=NULL and librdf_list_size(statements)=0
   */
  librdf_statement* current; /* current statement */
  librdf_list statements;   /* STATIC following statements after current */
} librdf_parser_raptor_stream_context;



/**
 * librdf_parser_raptor_init - Initialise the raptor RDF parser
 * @parser: the parser
 * @context: context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_parser_raptor_init(librdf_parser *parser, void *context) 
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;

  pcontext->parser = parser;
  pcontext->is_ntriples=(pcontext->parser->factory->name && 
                         !strcmp(pcontext->parser->factory->name, "ntriples"));
  
  /* always succeeds ? */  
  return 0;
}
  

/*
 * librdf_parser_raptor_new_statement_handler - helper callback function for raptor RDF when a new triple is asserted
 * @context: context for callback
 * @statement: raptor_statement
 * 
 * Adds the statement to the list of statements.
 */
static void
librdf_parser_raptor_new_statement_handler (void *context,
                                            const raptor_statement *rstatement)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;
  librdf_node* node;
  librdf_statement* statement;
#ifdef LIBRDF_DEBUG
#if LIBRDF_DEBUG > 1
  char *s;
#endif
#endif
  librdf_world* world=scontext->pcontext->parser->world;

  statement=librdf_new_statement(world);
  if(!statement)
    return;

  if(rstatement->subject_type == RAPTOR_IDENTIFIER_TYPE_ANONYMOUS) {
    node=librdf_new_node_from_blank_identifier(world, rstatement->subject);
  } else if (rstatement->subject_type == RAPTOR_IDENTIFIER_TYPE_RESOURCE) {
    node=librdf_new_node_from_normalised_uri_string(world,
                                                    librdf_uri_as_string((librdf_uri*)rstatement->subject),
                                                    scontext->source_uri,
                                                    scontext->base_uri);
  } else
    LIBRDF_FATAL2(librdf_parser_raptor_new_statement_handler,"Unknown Raptor subject identifier type %d", rstatement->subject_type);
  
  librdf_statement_set_subject(statement, node);
  
  if(rstatement->predicate_type == RAPTOR_IDENTIFIER_TYPE_ORDINAL) {
    /* FIXME - but only a little
     * Do I really need to do log10(ordinal) [or /10 and count] + 1 ? 
     * See also librdf_heuristic_gen_name for some code to repurpose.
     */
    static char ordinal_buffer[100]; 
    int ordinal=*(int*)rstatement->predicate;
    sprintf(ordinal_buffer, "http://www.w3.org/1999/02/22-rdf-syntax-ns#_%d", ordinal);
    
    node=librdf_new_node_from_uri_string(world, ordinal_buffer);
  } else if (rstatement->predicate_type == RAPTOR_IDENTIFIER_TYPE_PREDICATE) {
    node=librdf_new_node_from_normalised_uri_string(world,
                                                    librdf_uri_as_string((librdf_uri*)rstatement->predicate),
                                                    scontext->source_uri,
                                                    scontext->base_uri);
  } else
    LIBRDF_FATAL2(librdf_parser_raptor_new_statement_handler,"Unknown Raptor predicate identifier type %d", rstatement->predicate_type);

  librdf_statement_set_predicate(statement, node);


  if(rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_LITERAL ||
     rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_XML_LITERAL) {
    int is_xml_literal = (rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_XML_LITERAL);
    
    librdf_statement_set_object(statement,
                                librdf_new_node_from_literal(world,
                                                             rstatement->object,
                                                             rstatement->object_literal_language,
                                                             is_xml_literal));
  } else if(rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_ANONYMOUS) {
    node=librdf_new_node_from_blank_identifier(world, rstatement->object);
    librdf_statement_set_object(statement, node);
  } else if(rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_RESOURCE) {
    node=librdf_new_node_from_normalised_uri_string(world,
                                                    librdf_uri_as_string((librdf_uri*)rstatement->object),
                                                    scontext->source_uri,
                                                    scontext->base_uri);
    librdf_statement_set_object(statement, node);
  } else
    LIBRDF_FATAL2(librdf_parser_raptor_new_statement_handler,"Unknown Raptor object identifier type %d", rstatement->object_type);


#ifdef LIBRDF_DEBUG
#if LIBRDF_DEBUG > 1
  s=librdf_statement_to_string(statement);
  fprintf(stderr, "Got new statement: %s\n", s);
  LIBRDF_FREE(cstring, s);
#endif
#endif

  librdf_list_add(&scontext->statements, statement);
}


static void
librdf_parser_raptor_error_handler(void *data, raptor_locator *locator,
                                   const char *message) 
{
  librdf_world *world=(librdf_world*)data;
  static const char *message_prefix=" - Raptor error - ";
  int prefix_len=strlen(message_prefix);
  int message_len=strlen(message);
  int locator_len=raptor_format_locator(NULL, 0, locator);
  char *buffer;

  buffer=(char*)LIBRDF_MALLOC(cstring, locator_len+prefix_len+message_len+1);
  if(!buffer) {
    fprintf(stderr, "librdf_raptor_error_handler: Out of memory\n");
    return;
  }
  raptor_format_locator(buffer, locator_len, locator);
  strncpy(buffer+locator_len, message_prefix, prefix_len);
  strcpy(buffer+prefix_len+locator_len, message); /* want extra \0 - using strcpy */

  librdf_error(world, buffer);
  LIBRDF_FREE(cstring, buffer);
}


static void
librdf_parser_raptor_warning_handler(void *data, raptor_locator *locator,
                                     const char *message) 
{
  librdf_world *world=(librdf_world*)data;
  static const char *message_prefix=" - Raptor warning - ";
  int prefix_len=strlen(message_prefix);
  int message_len=strlen(message);
  int locator_len=raptor_format_locator(NULL, 0, locator);
  char *buffer;

  buffer=(char*)LIBRDF_MALLOC(cstring, locator_len+prefix_len+message_len+1);
  if(!buffer) {
    fprintf(stderr, "librdf_raptor_warning_handler: Out of memory\n");
    return;
  }
  raptor_format_locator(buffer, locator_len, locator);
  strncpy(buffer+locator_len, message_prefix, prefix_len);
  strcpy(buffer+prefix_len+locator_len, message); /* want extra \0 - using strcpy */

  librdf_warning(world, message);
  LIBRDF_FREE(cstring, buffer);
}


/* FIXME: Yeah?  What about it? */
#define RAPTOR_IO_BUFFER_LEN 1024


/*
v * librdf_parser_raptor_get_next_statement - helper function to get the next statement
 * @context: serialisation context
 * 
 * Return value: >0 if a statement found, 0 at end of file, or <0 on error
 */
static int
librdf_parser_raptor_get_next_statement(librdf_parser_raptor_stream_context *context) {
  char buffer[RAPTOR_IO_BUFFER_LEN];
  int status=0;
  
  if(!context->fh)
    return 0;
  
  context->current=NULL;
  while(!feof(context->fh)) {
    int len=fread(buffer, 1, RAPTOR_IO_BUFFER_LEN, context->fh);
    int ret=raptor_parse_chunk(context->rdf_parser, buffer, len, 
                               (len < RAPTOR_IO_BUFFER_LEN));
    if(ret) {
      status=(-1);
      break; /* failed and done */
    }

    /* parsing found at least 1 statement, return */
    if(librdf_list_size(&context->statements)) {
      context->current=(librdf_statement*)librdf_list_pop(&context->statements);
      status=1;
      break;
    }

    if(len < RAPTOR_IO_BUFFER_LEN)
      break;
  }

  if(feof(context->fh) || status <1) {
    fclose(context->fh);
    context->fh=NULL;
  }
  
  return status;
}


/**
 * librdf_parser_raptor_parse_file_as_stream - Retrieve the RDF/XML content at URI and parse it into a librdf_stream
 * @context: parser context
 * @uri: &librdf_uri URI of RDF/XML content source
 * @base_uri: &librdf_uri URI of the content location or NULL if the same
 *
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_file_as_stream(void *context, librdf_uri *uri,
                                         librdf_uri *base_uri)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  librdf_parser_raptor_stream_context* scontext;
  librdf_stream *stream;
  char* filename;
  raptor_parser *rdf_parser;
  int rc;
  librdf_world *world=uri->world;
  
  scontext=(librdf_parser_raptor_stream_context*)LIBRDF_CALLOC(librdf_parser_raptor_stream_context, 1, sizeof(librdf_parser_raptor_stream_context));
  if(!scontext)
    return NULL;

  if(pcontext->is_ntriples)
    rdf_parser=raptor_new_parser("ntriples");
  else
    rdf_parser=raptor_new_parser("rdfxml");

  if(!rdf_parser)
    return NULL;

  raptor_set_statement_handler(rdf_parser, scontext, 
                               librdf_parser_raptor_new_statement_handler);
  
  /* raptor_set_feature(rdf_parser, RAPTOR_FEATURE_SCANNING, 1); */
  
  raptor_set_error_handler(rdf_parser, world, 
                           librdf_parser_raptor_error_handler);
  raptor_set_warning_handler(rdf_parser, world,
                             librdf_parser_raptor_warning_handler);
  
  scontext->rdf_parser=rdf_parser;

  scontext->pcontext=pcontext;
  scontext->source_uri = uri;
  if(!base_uri)
    base_uri=uri;
  scontext->base_uri = base_uri;

  filename=(char*)librdf_uri_to_filename(uri);
  if(!filename)
    return NULL;
  
  scontext->fh=fopen(filename, "r");
  if(!scontext->fh) {
#ifndef WIN32
    extern int errno;
#endif
    
    LIBRDF_DEBUG3(librdf_new_parser_repat, "Failed to open file '%s' - %s\n",
                  filename, strerror(errno));
    free(filename);
    librdf_parser_raptor_serialise_finished((void*)scontext);
    return(NULL);
  }
  free(filename); /* free since it is actually malloc()ed by libraptor */

  /* Start the parse */
  rc=raptor_start_parse(rdf_parser, (raptor_uri*)base_uri);

  /* start parsing; initialises scontext->statements, scontext->current */
  librdf_parser_raptor_get_next_statement(scontext);

  stream=librdf_new_stream(world,
                           (void*)scontext,
                           &librdf_parser_raptor_serialise_end_of_stream,
                           &librdf_parser_raptor_serialise_next_statement,
                           &librdf_parser_raptor_serialise_get_statement,
                           &librdf_parser_raptor_serialise_finished);
  if(!stream) {
    librdf_parser_raptor_serialise_finished((void*)scontext);
    return NULL;
  }

  return stream;  
}


/**
 * librdf_parser_raptor_parse_file_into_model - Retrieve the RDF/XML content at URI and store it into a librdf_model
 * @context: parser context
 * @uri: &librdf_uri URI of RDF/XML content source
 * @base_uri: &librdf_uri URI of the content location
 * @model: &librdf_model of model
 *
 * Retrieves all statements and stores them in the given model.
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_raptor_parse_file_into_model(void *context, librdf_uri *uri, 
                                          librdf_uri *base_uri,
                                          librdf_model* model)
{
  librdf_stream* stream;
  
  stream=librdf_parser_raptor_parse_file_as_stream(context, uri, base_uri);
  if(!stream)
    return 1;

  return librdf_model_add_statements(model, stream);
}


/**
 * librdf_parser_raptor_serialise_end_of_stream - Check for the end of the stream of statements from the raptor RDF parser
 * @context: the context passed in by &librdf_stream
 * 
 * Return value: non 0 at end of stream
 **/
static int
librdf_parser_raptor_serialise_end_of_stream(void* context)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;

  return (!scontext->current && !librdf_list_size(&scontext->statements));
}


/**
 * librdf_parser_raptor_serialise_next_statement - Move to the next librdf_statement in the stream of statements from the raptor RDF parse
 * @context: the context passed in by &librdf_stream
 * 
 * Return value: non 0 at end of stream
 **/
static int
librdf_parser_raptor_serialise_next_statement(void* context)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;

  librdf_free_statement(scontext->current);
  scontext->current=NULL;

  /* get another statement if there is one */
  while(!scontext->current) {
    scontext->current=(librdf_statement*)librdf_list_pop(&scontext->statements);
    if(scontext->current)
      break;
  
    /* else get a new one */

    /* 0 is end, <0 is error.  Either way stop */
    if(librdf_parser_raptor_get_next_statement(scontext) <=0)
      break;
  }

  return (scontext->current == NULL);
}


/**
 * librdf_parser_raptor_serialise_get_statement - Get the current librdf_statement from the stream of statements from the raptor RDF parse
 * @context: the context passed in by &librdf_stream
 * 
 * Return value: a new &librdf_statement or NULL on error or if no statements found.
 **/
static void*
librdf_parser_raptor_serialise_get_statement(void* context, int flags)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->current;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return NULL;
      
    default:
      abort();
  }

}


/**
 * librdf_parser_raptor_serialise_finished - Finish the serialisation of the statement stream from the raptor RDF parse
 * @context: the context passed in by &librdf_stream
 **/
static void
librdf_parser_raptor_serialise_finished(void* context)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;

  if(scontext) {
    librdf_statement* statement;

    if(scontext->rdf_parser)
      raptor_free(scontext->rdf_parser);

    if(scontext->current)
      librdf_free_statement(scontext->current);
  
    while((statement=(librdf_statement*)librdf_list_pop(&scontext->statements)))
      librdf_free_statement(statement);
    librdf_list_clear(&scontext->statements);

    LIBRDF_FREE(librdf_parser_raptor_context, scontext);
  }
}


static raptor_uri*
librdf_raptor_new_uri(void *context, const char *uri_string) 
{
  return (raptor_uri*)librdf_new_uri((librdf_world*)context, uri_string);
}

static raptor_uri*
librdf_raptor_new_uri_from_uri_local_name(void *context,
                                          raptor_uri *uri,
                                          const char *local_name)
{
   return (raptor_uri*)librdf_new_uri_from_uri_local_name((librdf_uri*)uri, local_name);
}

static raptor_uri*
librdf_raptor_new_uri_relative_to_base(void *context,
                                       raptor_uri *base_uri,
                                       const char *uri_string) 
{
  return (raptor_uri*)librdf_new_uri_relative_to_base((librdf_uri*)base_uri, uri_string);
}


static raptor_uri*
librdf_raptor_new_uri_for_rdf_concept(void *context, const char *name) 
{
  librdf_uri *uri;
  librdf_get_concept_by_name((librdf_world*)context, 1, name, &uri, NULL);
  return (raptor_uri*)librdf_new_uri_from_uri(uri);
}

static void
librdf_raptor_free_uri(void *context, raptor_uri *uri) 
{
  librdf_free_uri((librdf_uri*)uri);
}


static int
librdf_raptor_uri_equals(void *context, raptor_uri* uri1, raptor_uri* uri2)
{
  return librdf_uri_equals((librdf_uri*)uri1, (librdf_uri*)uri2);
}


static raptor_uri*
librdf_raptor_uri_copy(void *context, raptor_uri *uri)
{
  return (raptor_uri*)librdf_new_uri_from_uri((librdf_uri*)uri);
}


static char*
librdf_raptor_uri_as_string(void *context, raptor_uri *uri)
{
  return librdf_uri_as_string((librdf_uri*)uri);
}


static raptor_uri_handler librdf_raptor_uri_handler = {
  librdf_raptor_new_uri,
  librdf_raptor_new_uri_from_uri_local_name,
  librdf_raptor_new_uri_relative_to_base,
  librdf_raptor_new_uri_for_rdf_concept,
  librdf_raptor_free_uri,
  librdf_raptor_uri_equals,
  librdf_raptor_uri_copy,
  librdf_raptor_uri_as_string,
  1
};


/**
 * librdf_parser_raptor_register_factory - Register the raptor RDF parser with the RDF parser factory
 * @factory: factory
 * 
 **/
static void
librdf_parser_raptor_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_raptor_context);
  
  factory->init  = librdf_parser_raptor_init;
  factory->parse_file_as_stream = librdf_parser_raptor_parse_file_as_stream;
  factory->parse_file_into_model = librdf_parser_raptor_parse_file_into_model;
}


/**
 * librdf_parser_raptor_constructor - Initialise the raptor RDF parser module
 * @world: redland world object
 **/
void
librdf_parser_raptor_constructor(librdf_world *world)
{
  raptor_init();

  raptor_uri_set_handler(&librdf_raptor_uri_handler, world);

  librdf_parser_register_factory(world, "raptor", "application/rdf+xml", NULL,
                                 &librdf_parser_raptor_register_factory);
  librdf_parser_register_factory(world, "ntriples", "text/plain",
                                 "http://www.w3.org/TR/rdf-testcases/#ntriples",
                                 &librdf_parser_raptor_register_factory);
}


/**
 * librdf_parser_raptor_destructor - Terminate the raptor RDF parser module
 * @world: redland world object
 **/
void
librdf_parser_raptor_destructor(void)
{
  raptor_finish();
}
