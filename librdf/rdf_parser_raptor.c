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

#include <librdf.h>

#include <rdf_parser.h>
#include <rdf_node.h>
#include <rdf_stream.h>
#include <rdf_statement.h>
#include <rdf_list.h>

#include <raptor.h>
#include <ntriples.h>


/* serialising implementing functions */
static int librdf_parser_raptor_serialise_end_of_stream(void* context);
static librdf_statement* librdf_parser_raptor_serialise_next_statement(void* context);
static void librdf_parser_raptor_serialise_finished(void* context);


/* implements parsing into stream or model */
static librdf_stream* librdf_parser_raptor_parse_common(void *context, librdf_uri *uri, librdf_uri* base_uri, librdf_model *model);


typedef struct {
  librdf_parser *parser;        /* librdf parser object */
  int is_ntriples;
} librdf_parser_raptor_context;


typedef struct {
  librdf_parser_raptor_context* pcontext; /* parser context */
  raptor_parser *rdf_parser;      /* source URI string (for raptor) */
  raptor_ntriples_parser *ntriples_parser; /* source URI string (for raptor) */
  librdf_statement* next;   /* next statement */
  librdf_model *model;      /* model to store in */
  librdf_list statements;  /* OR list to store statements (STATIC) */
  librdf_uri *source_uri;   /* source URI */
  librdf_uri *base_uri;     /* base URI */
  int end_of_stream;        /* non 0 if stream finished */
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
 * librdf_parser_raptor_make_node_from_anon - helper function to convert an id into a node
 * @scontext: parser strream context
 * @anon_string: anonymous ID string
 * 
 * Turns anon_string 'foo' into a resource node with URI baseURI#foo
 * 
 * Return value: librdf_node node for NULL on failure
 **/
static librdf_node*
librdf_parser_raptor_make_node_from_anon(librdf_parser_raptor_stream_context* scontext,
                                         const char *anon_string) 
{
  int len;
  char *buffer;
  librdf_node *node=NULL;

  len=strlen(anon_string);
  buffer=LIBRDF_MALLOC(cstring, len+2);
  if(buffer) {
    buffer[0]='#';
    strncpy(buffer+1, anon_string, len+1); /* copy \0 */
    node=librdf_new_node_from_uri_local_name(scontext->pcontext->parser->world,
                                             scontext->base_uri, buffer);
    LIBRDF_FREE(cstring, buffer);
  }
  return node;
}


/*
 * librdf_parser_raptor_new_statement_handler - helper callback function for raptor RDF when a new triple is asserted
 * @context: context for callback
 * @statement: raptor_statement
 * 
 * Adds the statement to the list of statements *in core* OR
 * to the model given during initialisation
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
    node=librdf_parser_raptor_make_node_from_anon(scontext, rstatement->subject);
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
                                                             NULL, 0, is_xml_literal));
  } else if(rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_ANONYMOUS) {
    node=librdf_parser_raptor_make_node_from_anon(scontext, rstatement->object);
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

  if(scontext->model) {
    librdf_model_add_statement(scontext->model, statement);
    librdf_free_statement(statement);
  } else {
    librdf_list_add(&scontext->statements, statement);
  }
}



/**
 * librdf_parser_raptor_parse_uri_as_stream - Retrieve the RDF/XML content at URI and parse it into a librdf_stream
 * @context: parser context
 * @uri: &librdf_uri URI of RDF/XML content source
 * @base_uri: &librdf_uri URI of the content location
 *
 * Retrieves all statements into memory in a list and emits them
 * when the URI content has been exhausted.  Use 
 * librdf_parser_raptor_parse_uri_into_model to update a model without
 * such a memory overhead.
 **/
static librdf_stream*
librdf_parser_raptor_parse_uri_as_stream(void *context, librdf_uri *uri,
                                         librdf_uri *base_uri)
{
  return librdf_parser_raptor_parse_common(context, uri, base_uri, NULL);
}


/**
 * librdf_parser_raptor_parse_uri_into_model - Retrieve the RDF/XML content at URI and store it into a librdf_model
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
librdf_parser_raptor_parse_uri_into_model(void *context, librdf_uri *uri, 
                                          librdf_uri *base_uri,
                                          librdf_model* model)
{
  void *status;

  status=(void*)librdf_parser_raptor_parse_common(context, 
                                                  uri, base_uri, model);
  
  return (status != NULL);
}


/**
 * librdf_parser_raptor_parse_common - Retrieve the RDF/XML content at URI and parse it into a librdf_stream or model
 * @context: parser context
 * @uri: &librdf_uri URI of RDF/XML content source
 * @base_uri: &librdf_uri URI of the content location
 * @model: &librdf_model of model
 *
 * Uses the raptor RDF routines to resolve RDF/XML content at a URI
 * and store it. If the model argument is not NULL, that is used
 * to store the data otherwise the data will be returned as a stream
 *
 * Return value:  If a model is given, the return value is NULL on success.
 * Otherwise the return value is a &librdf_stream or NULL on failure.
 **/
static librdf_stream*
librdf_parser_raptor_parse_common(void *context,
                                  librdf_uri *uri, librdf_uri *base_uri,
                                  librdf_model* model)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  librdf_parser_raptor_stream_context* scontext;
  librdf_stream *stream;
  raptor_parser *rdf_parser;
  raptor_ntriples_parser *ntriples_parser;
  int rc;
  librdf_world *world=uri->world;
  
  scontext=(librdf_parser_raptor_stream_context*)LIBRDF_CALLOC(librdf_parser_raptor_stream_context, 1, sizeof(librdf_parser_raptor_stream_context));
  if(!scontext)
    return NULL;

  if(pcontext->is_ntriples) {
    ntriples_parser=raptor_ntriples_new(world);
    if(!ntriples_parser)
      return NULL;
  
    raptor_ntriples_set_statement_handler(ntriples_parser, scontext, 
                                          librdf_parser_raptor_new_statement_handler);
    
    scontext->ntriples_parser=ntriples_parser;
  } else {
    rdf_parser=raptor_new(world);
    if(!rdf_parser)
      return NULL;
  
    raptor_set_statement_handler(rdf_parser, scontext, 
                                 librdf_parser_raptor_new_statement_handler);
    
    raptor_set_feature(rdf_parser, RAPTOR_FEATURE_SCANNING, 1);

    scontext->rdf_parser=rdf_parser;
  }
  

  scontext->pcontext=pcontext;
  scontext->source_uri = uri;
  scontext->base_uri = base_uri;

  scontext->model=model;

  stream=librdf_new_stream(world,
                           (void*)scontext,
                           &librdf_parser_raptor_serialise_end_of_stream,
                           &librdf_parser_raptor_serialise_next_statement,
                           &librdf_parser_raptor_serialise_finished);
  if(!stream) {
    librdf_parser_raptor_serialise_finished((void*)scontext);
    return NULL;
  }

  /* Do the work all in one go - should do incrementally - FIXME */
  if(pcontext->is_ntriples)
    rc=raptor_ntriples_parse_file(ntriples_parser, uri, base_uri);
  else
    rc=raptor_parse_file(rdf_parser, uri, base_uri);

  /* Above line does it all for adding to model */
  if(model) {
    librdf_free_stream(stream);
    return (librdf_stream*)rc;
  }
  
  return stream;  
}
  

/*
 * librdf_parser_raptor_get_next_statement - helper function to get the next librdf_statement
 * @context: 
 * 
 * Gets the next &librdf_statement from the raptor stream constructed from the
 * raptor RDF parser.
 * 
 * FIXME: Implementation currently stores all statements in memory in
 * a list and emits when the URI content has been exhausted.
 * 
 * Return value: a new &librdf_statement or NULL on error or if no statements found.
 *
 */
static librdf_statement*
librdf_parser_raptor_get_next_statement(librdf_parser_raptor_stream_context *context)
{
  context->next=(librdf_statement*)librdf_list_pop(&context->statements);

  if(!context->next)
    context->end_of_stream=1;

  return context->next;
}


/**
 * librdf_parser_raptor_serialise_end_of_stream - Check for the end of the stream of statements from the raptor RDF parser
 * @context: the context passed in by &librdf_stream
 * 
 * Uses helper function librdf_parser_raptor_get_next_statement() to try to
 * get at least one statement, to check for end of stream.
 * 
 * Return value: non 0 at end of stream
 **/
static int
librdf_parser_raptor_serialise_end_of_stream(void* context)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;

  if(scontext->end_of_stream)
    return 1;

  /* already have 1 stored item - not end yet */
  if(scontext->next)
    return 0;

  scontext->next=librdf_parser_raptor_get_next_statement(scontext);
  if(!scontext->next)
    scontext->end_of_stream=1;

  return (scontext->next == NULL);
}


/**
 * librdf_parser_raptor_serialise_next_statement - Get the next librdf_statement from the stream of statements from the raptor RDF parse
 * @context: the context passed in by &librdf_stream
 * 
 * Uses helper function librdf_parser_raptor_get_next_statement() to do the
 * work.
 *
 * Return value: a new &librdf_statement or NULL on error or if no statements found.
 **/
static librdf_statement*
librdf_parser_raptor_serialise_next_statement(void* context)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;
  librdf_statement* statement;

  if(scontext->end_of_stream)
    return NULL;

  /* return stored statement if there is one */
  if(scontext->next) {
    statement=scontext->next;
    scontext->next=NULL;
    return statement;
  }
  
  /* else get a new one or NULL at end */
  scontext->next=librdf_parser_raptor_get_next_statement(scontext);
  if(!scontext->next)
    scontext->end_of_stream=1;

  return scontext->next;
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

    if(scontext->ntriples_parser)
      raptor_ntriples_free(scontext->ntriples_parser);

    /* Empty static list of any remaining things */
    while((statement=(librdf_statement*)librdf_list_pop(&scontext->statements)))
      librdf_free_statement(statement);
    librdf_list_clear(&scontext->statements);

    if(scontext->next)
      librdf_free_statement(scontext->next);
    
    LIBRDF_FREE(librdf_parser_raptor_context, scontext);
  }
}


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
  factory->parse_uri_as_stream = librdf_parser_raptor_parse_uri_as_stream;
  factory->parse_uri_into_model = librdf_parser_raptor_parse_uri_into_model;
}


/**
 * librdf_parser_raptor_constructor - Initialise the raptor RDF parser module
 * @world: redland world object
 **/
void
librdf_parser_raptor_constructor(librdf_world *world)
{
  librdf_parser_register_factory(world, "raptor", NULL, NULL,
                                 &librdf_parser_raptor_register_factory);
  librdf_parser_register_factory(world, "ntriples", "text/plain",
                                 "http://www.w3.org/2001/sw/RDFCore/ntriples/",
                                 &librdf_parser_raptor_register_factory);
}
