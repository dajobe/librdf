/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser using Rapier
 *
 * $Id$
 *
 * Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology, University of Bristol.
 *
 *    This package is Free Software available under either of two licenses
 *    (see FAQS.html to see why):
 * 
 * 1. The GNU Lesser General Public License (LGPL)
 * 
 *    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
 *    full license text.
 *      _________________________________________________________________
 * 
 *      Copyright (C) 2000 David Beckett, Institute for Learning and
 *      Research Technology, University of Bristol. All Rights Reserved.
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public License
 *      as published by the Free Software Foundation; either version 2 of
 *      the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *      USA
 *      _________________________________________________________________
 * 
 *    NOTE - under Term 3 of the LGPL, you may choose to license the entire
 *    library under the GPL. See COPYING for the full license text.
 * 
 * 2. The Mozilla Public License
 * 
 *    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
 *    license text.
 * 
 *    Under MPL section 13. I declare that all of the Covered Code is
 *    Multiple Licensed:
 *      _________________________________________________________________
 * 
 *      The contents of this file are subject to the Mozilla Public License
 *      version 1.1 (the "License"); you may not use this file except in
 *      compliance with the License. You may obtain a copy of the License
 *      at http://www.mozilla.org/MPL/
 * 
 *      Software distributed under the License is distributed on an "AS IS"
 *      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 *      the License for the specific language governing rights and
 *      limitations under the License.
 * 
 *      The Initial Developer of the Original Code is David Beckett.
 *      Portions created by David Beckett are Copyright (C) 2000 David
 *      Beckett, Institute for Learning and Research Technology, University
 *      of Bristol. All Rights Reserved.
 * 
 *      Alternatively, the contents of this file may be used under the
 *      terms of the GNU Lesser General Public License, in which case the
 *      provisions of the LGPL License are applicable instead of those
 *      above. If you wish to allow use of your version of this file only
 *      under the terms of the LGPL License and not to allow others to use
 *      your version of this file under the MPL, indicate your decision by
 *      deleting the provisions above and replace them with the notice and
 *      other provisions required by the LGPL License. If you do not delete
 *      the provisions above, a recipient may use your version of this file
 *      under either the MPL or the LGPL License.
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

#include <rapier.h>


/* serialising implementing functions */
static int librdf_parser_rapier_serialise_end_of_stream(void* context);
static librdf_statement* librdf_parser_rapier_serialise_next_statement(void* context);
static void librdf_parser_rapier_serialise_finished(void* context);


/* implements parsing into stream or model */
static librdf_stream* librdf_parser_rapier_parse_common(void *context, librdf_uri *uri, librdf_uri* base_uri, librdf_model *model);


/* not used at present */
typedef struct {
  librdf_parser *parser;        /* librdf parser object */
  rapier_parser *rdf_parser;    /* Rapier parser object */
} librdf_parser_rapier_context;


typedef struct {
  librdf_parser_rapier_context* pcontext; /* parser context */
  rapier_parser *rdf_parser;      /* source URI string (for rapier) */
  librdf_statement* next;   /* next statement */
  librdf_model *model;      /* model to store in */
  librdf_list *statements;  /* OR list to store statements */
  librdf_uri *source_uri;   /* source URI */
  librdf_uri *base_uri;     /* base URI */
  int end_of_stream;        /* non 0 if stream finished */
} librdf_parser_rapier_stream_context;



/**
 * librdf_parser_rapier_init - Initialise the rapier RDF parser
 * @parser: the parser
 * @context: context
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_parser_rapier_init(librdf_parser *parser, void *context) 
{
  librdf_parser_rapier_context* pcontext=(librdf_parser_rapier_context*)context;

  pcontext->parser = parser;
  
  /* always succeeds ? */  
  return 0;
}
  

/*
 * librdf_parser_rapier_new_statement_handler - helper callback function for rapier RDF when a new triple is asserted
 * @context: context for callback
 * @statement: rapier_statement
 * 
 * Adds the statement to the list of statements *in core* OR
 * to the model given during initialisation
 */
static void
librdf_parser_rapier_new_statement_handler (void *context,
                                            const rapier_statement *rstatement)
{
  librdf_parser_rapier_stream_context* scontext=(librdf_parser_rapier_stream_context*)context;
  librdf_node* node;
  librdf_statement* statement;
  char *object;
#ifdef LIBRDF_DEBUG
#if LIBRDF_DEBUG > 1
  char *s;
#endif
#endif

  statement=librdf_new_statement();
  if(!statement)
    return;
  
  node=librdf_new_node_from_normalised_uri_string(rstatement->subject,
                                                  scontext->source_uri,
                                                  scontext->base_uri);
  librdf_statement_set_subject(statement, node);
  
  node=librdf_new_node_from_normalised_uri_string(rstatement->predicate,
                                                  scontext->source_uri,
                                                  scontext->base_uri);
  librdf_statement_set_predicate(statement, node);


  if(rstatement->object_type == RAPIER_OBJECT_TYPE_LITERAL)
    librdf_statement_set_object(statement,
                                librdf_new_node_from_literal(rstatement->object,
                                                             NULL, 0, 0));
  else {
    node=librdf_new_node_from_normalised_uri_string(rstatement->object,
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



/**
 * librdf_parser_rapier_parse_uri_as_stream - Retrieve the RDF/XML content at URI and parse it into a librdf_stream
 * @context: parser context
 * @uri: &librdf_uri URI of RDF/XML content source
 * @base_uri: &librdf_uri URI of the content location
 *
 * Retrieves all statements into memory in a list and emits them
 * when the URI content has been exhausted.  Use 
 * librdf_parser_rapier_parse_uri_into_model to update a model without
 * such a memory overhead.
 **/
static librdf_stream*
librdf_parser_rapier_parse_uri_as_stream(void *context, librdf_uri *uri,
                                         librdf_uri *base_uri)
{
  return librdf_parser_rapier_parse_common(context, uri, base_uri, NULL);
}


/**
 * librdf_parser_rapier_parse_uri_into_model - Retrieve the RDF/XML content at URI and store it into a librdf_model
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
librdf_parser_rapier_parse_uri_into_model(void *context, librdf_uri *uri, 
                                          librdf_uri *base_uri,
                                          librdf_model* model)
{
  void *status;

  status=(void*)librdf_parser_rapier_parse_common(context, 
                                                  uri, base_uri, model);
  
  return (status == NULL);
}


/**
 * librdf_parser_rapier_parse_common - Retrieve the RDF/XML content at URI and parse it into a librdf_stream or model
 * @context: parser context
 * @uri: &librdf_uri URI of RDF/XML content source
 * @base_uri: &librdf_uri URI of the content location
 * @model: &librdf_model of model
 *
 * Uses the rapier RDF routines to resolve RDF/XML content at a URI
 * and store it. If the model argument is not NULL, that is used
 * to store the data otherwise the data will be returned as a stream
 *
 * Return value:  If a model is given, the return value is NULL on success.
 * Otherwise the return value is a &librdf_stream or NULL on failure.
 **/
static librdf_stream*
librdf_parser_rapier_parse_common(void *context,
                                  librdf_uri *uri, librdf_uri *base_uri,
                                  librdf_model* model)
{
  librdf_parser_rapier_context* pcontext=(librdf_parser_rapier_context*)context;
  librdf_parser_rapier_stream_context* scontext;
  librdf_stream *stream;
  rapier_parser *rdf_parser;
  int rc;
  
  scontext=(librdf_parser_rapier_stream_context*)LIBRDF_CALLOC(librdf_parser_rapier_stream_context, 1, sizeof(librdf_parser_rapier_stream_context));
  if(!scontext)
    return NULL;

  rdf_parser=rapier_new(base_uri);
  if(!rdf_parser)
    return NULL;
  
  rapier_set_statement_handler(rdf_parser, scontext, 
                               librdf_parser_rapier_new_statement_handler);

  scontext->rdf_parser=rdf_parser;

  scontext->pcontext=pcontext;
  scontext->source_uri = uri;
  scontext->base_uri = base_uri;

  scontext->model=model;

  stream=librdf_new_stream((void*)scontext,
                           &librdf_parser_rapier_serialise_end_of_stream,
                           &librdf_parser_rapier_serialise_next_statement,
                           &librdf_parser_rapier_serialise_finished);
  if(!stream) {
    librdf_parser_rapier_serialise_finished((void*)scontext);
    return NULL;
  }

  /* Do the work all in one go - should do incrementally - FIXME */
  rc=rapier_parse_file(rdf_parser, uri, base_uri);

  /* Above line does it all for adding to model */
  if(model) {
    librdf_parser_rapier_serialise_finished((void*)scontext);
    return (librdf_stream*)rc;
  }
  
  return stream;  
}
  

/*
 * librdf_parser_rapier_get_next_statement - helper function to get the next librdf_statement
 * @context: 
 * 
 * Gets the next &librdf_statement from the rapier stream constructed from the
 * rapier RDF parser.
 * 
 * FIXME: Implementation currently stores all statements in memory in
 * a list and emits when the URI content has been exhausted.
 * 
 * Return value: a new &librdf_statement or NULL on error or if no statements found.
 *
 */
static librdf_statement*
librdf_parser_rapier_get_next_statement(librdf_parser_rapier_stream_context *context)
{
  context->next=(librdf_statement*)librdf_list_pop(context->statements);

  if(!context->next)
    context->end_of_stream=1;

  return context->next;
}


/**
 * librdf_parser_rapier_serialise_end_of_stream - Check for the end of the stream of statements from the rapier RDF parser
 * @context: the context passed in by &librdf_stream
 * 
 * Uses helper function librdf_parser_rapier_get_next_statement() to try to
 * get at least one statement, to check for end of stream.
 * 
 * Return value: non 0 at end of stream
 **/
static int
librdf_parser_rapier_serialise_end_of_stream(void* context)
{
  librdf_parser_rapier_stream_context* scontext=(librdf_parser_rapier_stream_context*)context;

  if(scontext->end_of_stream)
    return 1;

  /* already have 1 stored item - not end yet */
  if(scontext->next)
    return 0;

  scontext->next=librdf_parser_rapier_get_next_statement(scontext);
  if(!scontext->next)
    scontext->end_of_stream=1;

  return (scontext->next == NULL);
}


/**
 * librdf_parser_rapier_serialise_next_statement - Get the next librdf_statement from the stream of statements from the rapier RDF parse
 * @context: the context passed in by &librdf_stream
 * 
 * Uses helper function librdf_parser_rapier_get_next_statement() to do the
 * work.
 *
 * Return value: a new &librdf_statement or NULL on error or if no statements found.
 **/
static librdf_statement*
librdf_parser_rapier_serialise_next_statement(void* context)
{
  librdf_parser_rapier_stream_context* scontext=(librdf_parser_rapier_stream_context*)context;
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
  scontext->next=librdf_parser_rapier_get_next_statement(scontext);
  if(!scontext->next)
    scontext->end_of_stream=1;

  return scontext->next;
}


/**
 * librdf_parser_rapier_serialise_finished - Finish the serialisation of the statement stream from the rapier RDF parse
 * @context: the context passed in by &librdf_stream
 **/
static void
librdf_parser_rapier_serialise_finished(void* context)
{
  librdf_parser_rapier_stream_context* scontext=(librdf_parser_rapier_stream_context*)context;
  librdf_parser_rapier_context* pcontext=scontext->pcontext;

  if(scontext) {

    if(scontext->rdf_parser)
      rapier_free(scontext->rdf_parser);

    if(scontext->statements) {
      librdf_statement* statement;
      while((statement=(librdf_statement*)librdf_list_pop(scontext->statements)))
        librdf_free_statement(statement);
      librdf_free_list(scontext->statements);
    }

    if(scontext->next)
      librdf_free_statement(scontext->next);
    
    LIBRDF_FREE(librdf_parser_rapier_context, scontext);
  }
}


/**
 * librdf_parser_rapier_register_factory - Register the rapier RDF parser with the RDF parser factory
 * @factory: factory
 * 
 **/
static void
librdf_parser_rapier_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_rapier_context);
  
  factory->init  = librdf_parser_rapier_init;
  factory->parse_uri_as_stream = librdf_parser_rapier_parse_uri_as_stream;
  factory->parse_uri_into_model = librdf_parser_rapier_parse_uri_into_model;
}


/**
 * librdf_parser_rapier_constructor - Initialise the rapier RDF parser module
 **/
void
librdf_parser_rapier_constructor(void)
{
  librdf_parser_register_factory("rapier", NULL, NULL,
                                 &librdf_parser_rapier_register_factory);
}
