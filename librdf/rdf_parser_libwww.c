/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser using libwww RDF implementation
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

/* libwww includes */
#undef PACKAGE
#undef VERSION
#include "WWWLib.h"
#include "WWWInit.h"
#include "WWWXML.h"


/* serialising implementing functions */
static int librdf_parser_libwww_serialise_end_of_stream(void* context);
static librdf_statement* librdf_parser_libwww_serialise_next_statement(void* context);
static void librdf_parser_libwww_serialise_finished(void* context);


/* implements parsing into stream or model */
static librdf_stream* librdf_parser_libwww_parse_common(void *context, librdf_uri *uri, librdf_uri* base_uri, librdf_model *model);


/* not used at present */
typedef struct {
  librdf_parser *parser;    /* Redland parser object */
} librdf_parser_libwww_context;


typedef struct {
  librdf_parser_libwww_context* pcontext; /* parser context */
  char* uri;                /* source URI string (for libwww) */
  HTRDF* parser;            /* libwww RDF parser */
  HTRequest* request;       /* request for URI */
  librdf_statement* next;   /* next statement */
  librdf_model *model;      /* model to store in */
  librdf_list *statements;  /* OR list to store statements */
  librdf_uri *source_uri;   /* source URI */
  librdf_uri *base_uri;     /* base URI */
  int request_done;         /* non 0 if request done */
  int end_of_stream;        /* non 0 if stream finished */
} librdf_parser_libwww_stream_context;



/**
 * librdf_parser_libwww_init - Initialise the libwww RDF parser
 * @parser: the parser
 * @context: context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_parser_libwww_init(librdf_parser *parser, void *context) 
{
  librdf_parser_libwww_context* pcontext=(librdf_parser_libwww_context*)context;

  pcontext->parser = parser;
  
  /* always succeeds ? */  
  return 0;
}
  

/*
 * librdf_parser_libwww_new_triple_handler - helper callback function for libwww RDF when a new triple is asserted
 * @rdfp: libwww &HTRDF reference
 * @t: libwww &HTTriple triple
 * @context: context for callback
 * 
 * Adds the statement to the list of statements *in core* OR
 * to the model given during initialisation
 *
 * Registered in librdf_parser_libwww_new_handler().
 */
static void
librdf_parser_libwww_new_triple_handler (HTRDF *rdfp, HTTriple *t, 
                                         void *context) 
{
  librdf_parser_libwww_stream_context* scontext=(librdf_parser_libwww_stream_context*)context;
  librdf_node* node;
  librdf_statement* statement;
  char *object;
#ifdef LIBRDF_DEBUG
#if LIBRDF_DEBUG > 1
  char *s;
#endif
#endif
  librdf_world *world=scontext->pcontext->parser->world;
  
  statement=librdf_new_statement(world);
  if(!statement)
    return;
  
  node=librdf_new_node_from_normalised_uri_string(world,
                                                  HTTriple_subject(t),
                                                  scontext->source_uri,
                                                  scontext->base_uri);
  librdf_statement_set_subject(statement, node);
  
  node=librdf_new_node_from_normalised_uri_string(world,
                                                  HTTriple_predicate(t),
                                                  scontext->source_uri,
                                                  scontext->base_uri);
  librdf_statement_set_predicate(statement, node);


  object=HTTriple_object(t);
  if(librdf_heuristic_object_is_literal(object))
    librdf_statement_set_object(statement,
                                librdf_new_node_from_literal(world, object, NULL, 0, 0));
  else {
    node=librdf_new_node_from_normalised_uri_string(world,
                                                    object,
                                                    scontext->source_uri,
                                                    scontext->base_uri);
    librdf_statement_set_object(statement, node);
  }


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
    librdf_list_add(scontext->statements, statement);
  }
}


/*
 * librdf_parser_libwww_new_handler - helper callback function for libwww RDF when a new parse is started on a libwww HTStream
 * @me: libwww &HTStream that is being parsed (ignored)
 * @request: libwww &HTRequest that generated the call (ignored)
 * @target_format: libwww &HTFormat target output format (ignored)
 * @target_stream: libwww &HTStream target output stream (ignored)
 * @rdfparser: libwww &HTRDF parser
 * @context: context for callback
 *
 * Registered in librdf_parser_libwww_parse_uri_as_stream().
 */
static void
librdf_parser_libwww_new_handler (HTStream *		me,
                                  HTRequest *		request,
                                  HTFormat 		target_format,
                                  HTStream *		target_stream,
                                  HTRDF *    	        rdfparser,
                                  void *         	context)
{
  librdf_parser_libwww_stream_context* scontext=(librdf_parser_libwww_stream_context*)context;

  if (rdfparser) {
    /* Remember the new RDF instance */
    scontext->parser = rdfparser;
    
    /* Register own triple callback just to follow what is going on */
    HTRDF_registerNewTripleCallback (rdfparser, 
                                     librdf_parser_libwww_new_triple_handler, 
                                     scontext);
  }
}


/*
 * librdf_parser_libwww_terminate_handler - helper callback function for libwww when request terminates
 * @request: libwww &HTRequest that generated the call
 * @response: libwww &HTResponse (ignored)
 * @param: context for callback
 * @status: status of termination (ignored)
 * 
 * Used so we can shut down and release resources.
 * 
 * Return value: 0 (does not matter since at termination)
 */
static int
librdf_parser_libwww_terminate_handler (HTRequest * request,
					HTResponse * response,
					void * param, int status)
{
  librdf_parser_libwww_stream_context* scontext=(librdf_parser_libwww_stream_context*)param;

  /* deleted by libwww */
  scontext->parser=NULL;

  /* finish event loop */
  HTEventList_stopLoop();
}



/*
 * librdf_parser_libwww_client_profile - helper function initialise libwww
 * @AppName: application name
 * @AppVersion: application version
 * 
 * This is a copy of client_profile() from libwww HT_Profile.c with one
 * change so that the default type conversions are not
 * registered, and later on we can register something that assumes
 * the content is RDF.
 *
 * FIXME: Probably don't need anywhere near all of this initialisation
 *
 * FIXME: this should be done once per use of libwww, not per libwww parser.
 */
static void
librdf_parser_libwww_client_profile (const char * AppName, 
                                     const char * AppVersion)
{
  HTList *converters = NULL;
  HTList * transfer_encodings = NULL;
  HTList * content_encodings = NULL;
  
  /* If the Library is not already initialized then do it */
  if (!HTLib_isInitialized()) HTLibInit(AppName, AppVersion);
  
  /* Register the default set of messages and dialog functions */
  
  /* was: HTAlertInit(); */
  HTAlert_add(HTError_print, HT_A_MESSAGE);
  HTAlert_add(HTConfirm, HT_A_CONFIRM);
  HTAlert_add(HTPrompt, HT_A_PROMPT);
  HTAlert_add(HTPromptPassword, HT_A_SECRET);
  HTAlert_add(HTPromptUsernameAndPassword, HT_A_USER_PW);
  
  
  HTAlert_setInteractive(YES);
  
  if (!converters) converters = HTList_new();
  if (!transfer_encodings) transfer_encodings = HTList_new();
  if (!content_encodings) content_encodings = HTList_new();
  
  /* Register the default set of transport protocols */
  HTTransportInit();
  
  /* Register the default set of application protocol modules */
  HTProtocolInit();
  
  /* Initialize suffix bindings for local files */
  HTBind_init();
  
  /* Set max number of sockets we want open simultanously */ 
  HTNet_setMaxSocket(32);
  
  /* Register the default set of BEFORE and AFTER filters */
  HTNetInit();
  
  /* Set up the default set of Authentication schemes */
  HTAAInit();
  
  /* Get any proxy or gateway environment variables */
  HTProxy_getEnvVar();
  
  /* Register the default set of converters */
  HTConverterInit(converters);
  
  /* Set the convertes as global converters for all requests */
  HTFormat_setConversion(converters);
  
  /* Register the default set of transfer encoders and decoders */
  HTTransferEncoderInit(transfer_encodings);
  HTFormat_setTransferCoding(transfer_encodings);
  
  /* Register the default set of content encoders and decoders */
  HTContentEncoderInit(content_encodings);
  if (HTList_count(content_encodings) > 0)
    HTFormat_setContentCoding(content_encodings);
  else {
    HTList_delete(content_encodings);
    content_encodings = NULL;
  }
  
  /* Register the default set of MIME header parsers */
  HTMIMEInit();
  
  /* Register the default set of file suffix bindings */
  HTFileInit();
  
  /* Register the default set of Icons for directory listings */
  HTIconInit(NULL);
}


#ifdef LIBRDF_DEBUG
#if LIBRDF_DEBUG > 1
static int
tracer (const char * fmt, va_list pArgs)
{
  return (vfprintf(stderr, fmt, pArgs));
}
#endif
#endif


/**
 * librdf_parser_libwww_parse_uri_as_stream - Retrieve the RDF/XML content at URI and parse it into a librdf_stream
 * @context: parser context
 * @uri: &librdf_uri URI of RDF/XML content source
 * @base_uri: &librdf_uri URI of the content location
 *
 * Retrieves all statements into memory in a list and emits them
 * when the URI content has been exhausted.  Use 
 * librdf_parser_libwww_parse_uri_into_model to update a model without
 * such a memory overhead.
 **/
static librdf_stream*
librdf_parser_libwww_parse_uri_as_stream(void *context, librdf_uri *uri,
                                         librdf_uri *base_uri)
{
  return librdf_parser_libwww_parse_common(context, uri, base_uri, NULL);
}


/**
 * librdf_parser_libwww_parse_uri_into_model - Retrieve the RDF/XML content at URI and store it into a librdf_model
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
librdf_parser_libwww_parse_uri_into_model(void *context, librdf_uri *uri, 
                                          librdf_uri *base_uri,
                                          librdf_model* model)
{
  void *status;

  status=(void*)librdf_parser_libwww_parse_common(context, 
                                                  uri, base_uri, model);
  
  return (status != NULL);
}


/**
 * librdf_parser_libwww_parse_common - Retrieve the RDF/XML content at URI and parse it into a librdf_stream or model
 * @context: parser context
 * @uri: &librdf_uri URI of RDF/XML content source
 * @base_uri: &librdf_uri URI of the content location
 * @model: &librdf_model of model
 *
 * Uses the libwww RDF routines to resolve RDF/XML content at a URI
 * and store it. If the model argument is not NULL, that is used
 * to store the data otherwise the data will be returned as a stream
 *
 * Return value:  If a model is given, the return value is NULL on success.
 * Otherwise the return value is a &librdf_stream or NULL on failure.
 **/
static librdf_stream*
librdf_parser_libwww_parse_common(void *context,
                                  librdf_uri *uri, librdf_uri *base_uri,
                                  librdf_model* model)
{
  librdf_parser_libwww_context* pcontext=(librdf_parser_libwww_context*)context;
  librdf_parser_libwww_stream_context* scontext;
  char * cwd;
  HTAnchor *anchor;
  BOOL status = NO;
  librdf_stream *stream;
  librdf_world *world=uri->world;
  
  scontext=(librdf_parser_libwww_stream_context*)LIBRDF_CALLOC(librdf_parser_libwww_stream_context, 1, sizeof(librdf_parser_libwww_stream_context));
  if(!scontext)
    return NULL;

  scontext->pcontext=pcontext;
  
  librdf_parser_libwww_client_profile("librdf_parser_libwwww", "1.0");

  HTEventInit();

#ifdef LIBRDF_DEBUG
#if LIBRDF_DEBUG > 1
  HTTrace_setCallback(tracer);
  HTSetTraceMessageMask("*");
#endif
#endif

  /* Set up our RDF To Triple converter stream */
  HTFormat_addConversion("*/*", "www/present", HTRDFParser_new, 1.0, 0.0, 0.0);
  
  /* Set our new RDF parser instance callback */
  HTRDF_registerNewParserCallback (librdf_parser_libwww_new_handler, scontext);

  /* Add a terminate handler */
  HTNet_addAfter(librdf_parser_libwww_terminate_handler, NULL, scontext,
		 HT_ALL, HT_FILTER_LAST);

  /* Set the timeout for long we are going to wait for a response */
  HTHost_setEventTimeout(10000);

  cwd= HTGetCurrentDirectoryURL();

  scontext->source_uri = uri;
  scontext->uri = HTParse(librdf_uri_as_string(uri), cwd, PARSE_ALL);
  HT_FREE(cwd);
  scontext->request = HTRequest_new();

  if(!base_uri)
    base_uri=uri;
  scontext->base_uri = base_uri;

  anchor = HTAnchor_findAddress(librdf_uri_as_string(uri));
  if(!anchor) {
    librdf_parser_libwww_serialise_finished((void*)scontext);
    return NULL;
  }
  
  /* Start the GET request */
  status = HTLoadAnchor(anchor, scontext->request);

  if (status == NO) {
    librdf_parser_libwww_serialise_finished((void*)scontext);
    return NULL;
  }

  scontext->model=model;
  
  if(model) {
    HTEventList_loop(scontext->request);
    scontext->request_done=1;
    librdf_parser_libwww_serialise_finished((void*)scontext);
    return (librdf_stream*)1;
  }
  
  stream=librdf_new_stream(world,
                           (void*)scontext,
                           &librdf_parser_libwww_serialise_end_of_stream,
                           &librdf_parser_libwww_serialise_next_statement,
                           &librdf_parser_libwww_serialise_finished);
  if(!stream) {
    librdf_parser_libwww_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}
  

/*
 * librdf_parser_libwww_get_next_statement - helper function to get the next librdf_statement
 * @context: 
 * 
 * Gets the next &librdf_statement from the libwww stream constructed from the
 * libwww RDF parser.
 * 
 * FIXME: Implementation currently stores all statements in memory in
 * a list and emits when the URI content has been exhausted.
 * 
 * Return value: a new &librdf_statement or NULL on error or if no statements found.
 *
 */
static librdf_statement*
librdf_parser_libwww_get_next_statement(librdf_parser_libwww_stream_context *context)
{
  librdf_world *world=context->pcontext->parser->world;

  if(!context->request_done) {
    context->statements=librdf_new_list(world);
    if(!context->statements)
      return NULL;
    HTEventList_loop(context->request);
    context->request_done=1;
  }
  
  context->next=(librdf_statement*)librdf_list_pop(context->statements);

  if(!context->next)
    context->end_of_stream=1;

  return context->next;
}


/**
 * librdf_parser_libwww_serialise_end_of_stream - Check for the end of the stream of statements from the libwww RDF parser
 * @context: the context passed in by &librdf_stream
 * 
 * Uses helper function librdf_parser_libwww_get_next_statement() to try to
 * get at least one statement, to check for end of stream.
 * 
 * Return value: non 0 at end of stream
 **/
static int
librdf_parser_libwww_serialise_end_of_stream(void* context)
{
  librdf_parser_libwww_stream_context* scontext=(librdf_parser_libwww_stream_context*)context;

  if(scontext->end_of_stream)
    return 1;

  /* already have 1 stored item - not end yet */
  if(scontext->next)
    return 0;

  scontext->next=librdf_parser_libwww_get_next_statement(scontext);
  if(!scontext->next)
    scontext->end_of_stream=1;

  return (scontext->next == NULL);
}


/**
 * librdf_parser_libwww_serialise_next_statement - Get the next librdf_statement from the stream of statements from the libwww RDF parse
 * @context: the context passed in by &librdf_stream
 * 
 * Uses helper function librdf_parser_libwww_get_next_statement() to do the
 * work.
 *
 * Return value: a new &librdf_statement or NULL on error or if no statements found.
 **/
static librdf_statement*
librdf_parser_libwww_serialise_next_statement(void* context)
{
  librdf_parser_libwww_stream_context* scontext=(librdf_parser_libwww_stream_context*)context;
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
  scontext->next=librdf_parser_libwww_get_next_statement(scontext);
  if(!scontext->next)
    scontext->end_of_stream=1;

  return scontext->next;
}


/**
 * librdf_parser_libwww_serialise_finished - Finish the serialisation of the statement stream from the libwww RDF parse
 * @context: the context passed in by &librdf_stream
 **/
static void
librdf_parser_libwww_serialise_finished(void* context)
{
  librdf_parser_libwww_stream_context* scontext=(librdf_parser_libwww_stream_context*)context;
  librdf_parser_libwww_context* pcontext=scontext->pcontext;

  if(scontext) {
    if(scontext->request)
      HTRequest_delete(scontext->request);

    if(scontext->parser)
      HTRDF_delete(scontext->parser);

    if(scontext->statements) {
      librdf_statement* statement;
      while((statement=(librdf_statement*)librdf_list_pop(scontext->statements)))
        librdf_free_statement(statement);
      librdf_free_list(scontext->statements);
    }

    if(scontext->next)
      librdf_free_statement(scontext->next);
    
    /* terminate libwww */
    HTProfile_delete();

    LIBRDF_FREE(librdf_parser_libwww_context, scontext);
  }
}


/**
 * librdf_parser_libwww_register_factory - Register the libwww RDF parser with the RDF parser factory
 * @factory: factory
 * 
 **/
static void
librdf_parser_libwww_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_libwww_context);
  
  factory->init  = librdf_parser_libwww_init;
  factory->parse_uri_as_stream = librdf_parser_libwww_parse_uri_as_stream;
  factory->parse_uri_into_model = librdf_parser_libwww_parse_uri_into_model;
}


/**
 * librdf_parser_libwww_constructor - Initialise the libwww RDF parser module
 * @world: redland world object
 **/
void
librdf_parser_libwww_constructor(librdf_world *world)
{
  librdf_parser_register_factory(world, "libwww", NULL, NULL,
                                 &librdf_parser_libwww_register_factory);
}
