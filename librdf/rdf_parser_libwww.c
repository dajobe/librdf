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

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#define LIBRDF_INTERNAL 1
#include <librdf.h>

#include <rdf_parser.h>
#include <rdf_node.h>
#include <rdf_stream.h>
#include <rdf_statement.h>

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

  scontext->next=librdf_new_statement();
  if(!scontext->next)
    return;
  
  librdf_statement_set_subject(scontext->next, 
                               librdf_new_node_from_uri_string(HTTriple_subject(t)));
  
  librdf_statement_set_predicate(scontext->next,
                                 librdf_new_node_from_uri_string(HTTriple_predicate(t)));
  
  /* FIXME: I have no idea if literal or resource */
  if(0) { /* if(object_is_literal) {*/
    librdf_statement_set_object(scontext->next,
                                librdf_new_node_from_literal(HTTriple_object(t), NULL, 0));
  } else {
    librdf_statement_set_object(scontext->next, 
                                librdf_new_node_from_uri_string(HTTriple_object(t)));
  }
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




/* This is a copy of client_profile from HT_Profile.c with one
   crucial change so that the default type conversions are not
   registered, and later on we can register something that assumes
   the content is RDF -- Dave Beckett <Dave.Beckett@bristol.ac.uk>
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

  /* Set up our RDF To Triple converter stream */
  HTFormat_addConversion("*/*", "www/present", HTRDFParser_new, 1.0, 0.0, 0.0);
  
  /* Set our new RDF parser instance callback */
  HTRDF_registerNewParserCallback (librdf_parser_libwww_new_handler, scontext);

  cwd= HTGetCurrentDirectoryURL();

  scontext->uri = HTParse(uri, cwd, PARSE_ALL);
  scontext->request = HTRequest_new();

  anchor = HTAnchor_findAddress(uri);
  
  /* Start the GET request */
  status = HTLoadAnchor(anchor, scontext->request);
  
  /* Go into the event loop... */
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
  return NULL;
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
