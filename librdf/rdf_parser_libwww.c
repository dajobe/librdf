/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser using libwww RDF implementation
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
#include <string.h>

#define LIBRDF_INTERNAL 1
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


/* not used at present */
typedef struct {
  char *dummy;
} librdf_parser_libwww_context;


typedef struct {
  char* uri;                /* source */
  HTRDF* parser;            /* libwww RDF parser */
  HTRequest* request;       /* request for URI */
  librdf_statement* next;   /* next statement */
  librdf_list *statements;  /* list containing all statements */
  int request_done;         /* non 0 if request done */
  int end_of_stream;        /* non 0 if stream finished */
} librdf_parser_libwww_stream_context;



static int
librdf_parser_libwww_init(void *context) {
  /* always succeeds ? */
  return 0;
}
  

static void
librdf_parser_libwwW_new_triple_handler (HTRDF *rdfp, HTTriple *t, 
                                         void *context) 
{
  librdf_parser_libwww_stream_context* scontext=(librdf_parser_libwww_stream_context*)context;
  librdf_statement* statement;
  int object_is_literal;
  char *s;

  statement=librdf_new_statement();
  if(!statement)
    return;
  
  librdf_statement_set_subject(statement, 
                               librdf_new_node_from_uri_string(HTTriple_subject(t)));
  
  librdf_statement_set_predicate(statement,
                                 librdf_new_node_from_uri_string(HTTriple_predicate(t)));


  /* FIXME: I have no idea if the object points to a literal or a resource */
  
  object_is_literal=1; /* assume the worst */

  /* Try to guess if resource / literal from string by assuming it 
   * is a URL if it matches ^[isalnum()]+:[^isblank()]+$
   * This will be fooled by literals of form 'thing:non-blank-thing'
   */
  for(s=HTTriple_object(t); *s; s++) {
    /* Find first non alphanumeric */
    if(!isalnum(*s)) {
      /* Better be a ':' */
      if(*s == ':') {
	/* check rest of string has no spaces */
	for(;*s; s++)
	  if(isblank(*s))
	    break;
	/* reached end, thus probably not a literal */
	if(!*s)
	  object_is_literal=0;
	break;
      }
    }
  }

  if(object_is_literal) {
    librdf_statement_set_object(statement,
                                librdf_new_node_from_literal(HTTriple_object(t), NULL, 0));
  } else {
    librdf_statement_set_object(statement, 
                                librdf_new_node_from_uri_string(HTTriple_object(t)));
  }

#ifdef LIBRDF_DEBUG
#if LIBRDF_DEBUG > 1
  s=librdf_statement_to_string(statement);
  fprintf(stderr, "Got new statement: %s\n", s);
  LIBRDF_FREE(cstring, s);
#endif
#endif

  librdf_list_add(scontext->statements, statement);
}


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
                                     librdf_parser_libwwW_new_triple_handler, 
                                     scontext);
  }
}


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



/* This is a copy of client_profile from HT_Profile.c with one
 * change so that the default type conversions are not
 * registered, and later on we can register something that assumes
 * the content is RDF.
 *
 * Probably don't need anywhere near all of this initialisation
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


static librdf_stream*
librdf_parser_libwww_parse_from_uri(void *context, librdf_uri *uri) {
  /* Note: not yet used */
/*  librdf_parser_libwww_context* pcontext=(librdf_parser_libwww_context*)context; */
  librdf_parser_libwww_stream_context* scontext;
  char * cwd;
  HTAnchor *anchor;
  BOOL status = NO;
  librdf_stream *stream;
  
  scontext=(librdf_parser_libwww_stream_context*)LIBRDF_CALLOC(librdf_parser_libwww_stream_context, 1, sizeof(librdf_parser_libwww_stream_context));
  if(!scontext)
    return NULL;

  
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

  scontext->uri = HTParse(uri, cwd, PARSE_ALL);
  HT_FREE(cwd);
  scontext->request = HTRequest_new();

  anchor = HTAnchor_findAddress(uri);
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

  
  stream=librdf_new_stream((void*)scontext,
                           &librdf_parser_libwww_serialise_end_of_stream,
                           &librdf_parser_libwww_serialise_next_statement,
                           &librdf_parser_libwww_serialise_finished);
  if(!stream) {
    librdf_parser_libwww_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}
  

static librdf_statement*
librdf_parser_libwww_get_next_statement(librdf_parser_libwww_stream_context *context) {

  if(!context->request_done) {
    context->statements=librdf_new_list();
    if(!context->statements)
      return NULL;
    HTEventList_loop(context->request);
    context->request_done=1;
  }
  
  context->next=librdf_list_pop(context->statements);

  if(!context->next)
    context->end_of_stream=1;

  return context->next;
}


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


static void
librdf_parser_libwww_serialise_finished(void* context)
{
  librdf_parser_libwww_stream_context* scontext=(librdf_parser_libwww_stream_context*)context;

  if(scontext) {
    if(scontext->request)
      HTRequest_delete(scontext->request);

    if(scontext->parser)
      HTRDF_delete(scontext->parser);

    if(scontext->statements)
      librdf_free_list(scontext->statements);

    /* Terminate libwww */
    HTProfile_delete();

    LIBRDF_FREE(librdf_parser_libwww_context, scontext);
  }
}


static void
librdf_parser_libwww_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_libwww_context);
  
  factory->init  = librdf_parser_libwww_init;
  factory->parse_from_uri = librdf_parser_libwww_parse_from_uri;
}


/**
 * librdf_parser_libwww_constructor:
 * @void: 
 * 
 * Initialise the Libwww parser module
 **/
void
librdf_parser_libwww_constructor(void)
{
  librdf_parser_register_factory("Libwww", &librdf_parser_libwww_register_factory);
}
