/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - Repat RDF Parser implementation
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
#include <stdarg.h>

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#define LIBRDF_INTERNAL 1
#include <librdf.h>

#include <rdf_parser.h>
#include <rdf_node.h>
#include <rdf_stream.h>
#include <rdf_statement.h>

#include <repat/rdfparse.h>



/* serialising implementing functions */
static int librdf_parser_repat_serialise_end_of_stream(void* context);
static librdf_statement* librdf_parser_repat_serialise_next_statement(void* context);
static void librdf_parser_repat_serialise_finished(void* context);


/* repat callback prototypes */
static void librdf_parser_repat_statement_handler(void* user_data, RDF_SubjectType subject_type, const XML_Char* subject, const XML_Char* predicate, int ordinal, RDF_ObjectType  object_type, const XML_Char* object, const XML_Char* xml_lang);
static void librdf_parser_repat_start_parse_type_literal_handler( void* user_data);
static void librdf_parser_repat_end_parse_type_literal_handler( void* user_data);
static void librdf_parser_repat_start_element_handler(void* user_data, const XML_Char* name,  const XML_Char** attributes);
static void librdf_parser_repat_end_element_handler(void* user_data, const XML_Char* name);
static void librdf_parser_repat_character_data_handler(void* user_data, const XML_Char* s, int len);
static void librdf_parser_repat_warning_handler(void* user_data, const XML_Char* warning);


/*
 * Repat parser context- not used at present
 */
typedef struct {
  char dummy;
} librdf_parser_repat_context;


/*
 * context used for the Repat parser to create &librdf_stream
 * of statements from parsing
 */
typedef struct {
  librdf_uri* uri;          /* source */
  FILE *fh;
  RDF_Parser repat;
  librdf_list* statements;  /* pending statements */
  int end_of_stream;        /* non 0 if stream finished */
  char *literal;            /* literal text being collected */
} librdf_parser_repat_stream_context;



/* repat callback handlers */

/* FIXME: Repat should allow per parse user data to be stored */
static void *librdf_parser_repat__user_data;

static void 
librdf_parser_repat_statement_handler(void* user_data,
                                      RDF_SubjectType subject_type, 
                                      const XML_Char* subject_string,
                                      const XML_Char* predicate_string,
                                      int ordinal,
                                      RDF_ObjectType object_type,
                                      const XML_Char* object_string,
                                      const XML_Char* xml_lang)
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)librdf_parser_repat__user_data;
  librdf_statement* statement=NULL;
  librdf_node *subject, *predicate, *object;

  /* got all statement parts now */
  statement=librdf_new_statement();
  if(!statement)
    return;

  switch(subject_type)
  {
    case RDF_SUBJECT_TYPE_URI:
      subject=librdf_new_node_from_uri_string(subject_string);
      break;
    case RDF_SUBJECT_TYPE_DISTRIBUTED:
      subject=librdf_new_node_from_uri_string(subject_string);
      break;
    case RDF_SUBJECT_TYPE_PREFIX:
      subject=librdf_new_node_from_uri_string(subject_string);
      break;
    case RDF_SUBJECT_TYPE_ANONYMOUS:
      subject=librdf_new_node_from_uri_string(subject_string);
      break;
    default:
      /* FIXME - warn of unknown subject type */
      abort();
  }
  librdf_statement_set_subject(statement,subject);

  
  if(!ordinal) {
    predicate=librdf_new_node_from_uri_string(predicate_string);
  } else {
    /* FIXME - an ordinal predicate li:<ordinal> should be generated */
    predicate=librdf_new_node_from_uri_string(predicate_string);
  }
  librdf_statement_set_predicate(statement,predicate);

  
  switch( object_type )
  {
    case RDF_OBJECT_TYPE_RESOURCE:
      object=librdf_new_node_from_uri_string(object_string);
      break;
    case RDF_OBJECT_TYPE_LITERAL:
      object=librdf_new_node_from_literal(object_string, xml_lang, 0, 0);
      break;
    case RDF_OBJECT_TYPE_XML:
      /* FIXME - content is XML literal (parseType="Literal") and should be
       * taken from collected literal text in parse context 
       */
      abort();
      object=librdf_new_node_from_literal(scontext->literal, 0, 0, 1);
      LIBRDF_FREE(cstring, scontext->literal);
      scontext->literal=NULL;
      break;
    default:
      /* FIXME - warn of unknown object type */
      abort();
  }
  
  librdf_statement_set_object(statement,object);

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  fprintf(stderr, "librdf_parser_repat_statement_handler: Adding statement: ");
  librdf_statement_print(statement, stderr);
  fputs("\n", stderr);
#endif

  librdf_list_add(scontext->statements, statement);
}


static void 
librdf_parser_repat_start_parse_type_literal_handler( void* user_data )
{
  printf( "start parse type literal\n" );
}


static void 
librdf_parser_repat_end_parse_type_literal_handler( void* user_data )
{
  printf( "end parse type literal\n" );
}


static void 
librdf_parser_repat_start_element_handler(void* user_data, 
                                          const XML_Char* name,
                                          const XML_Char** attributes)
{
  printf( "start element: %s\n", name );
}


static void
librdf_parser_repat_end_element_handler(void* user_data,
                                        const XML_Char* name)
{
  printf( "end element: %s\n", name );
}


static void 
librdf_parser_repat_character_data_handler(void* user_data,
                                           const XML_Char* s,
                                           int len)
{
  int i;
  
  printf( "text: " );
  
  for( i = 0; i < len; ++i )
  {
    switch( s[ i ] )
    {
      case 0x09:
        printf( "&#x9;" );
        break;
      case 0x0A:
        printf( "&#xA;" );
        break;
      default:
        printf( "%c", s[ i ] );
    }
  }
  
  printf( "\n" );
}


static void 
librdf_parser_repat_warning_handler(void* user_data, const XML_Char* msg)
{
  fprintf(stderr, "repat RDF parser warning - %s\n", msg);
}


/**
 * librdf_parser_repat_init - Initialise the Repat RDF parser
 * @context: context
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_repat_init(void *context) 
{
  return 0;
}
  

/**
 * librdf_parser_repat_parse_as_stream - Retrieve the RDF/XML content at URI and parse it into a librdf_stream
 * @context: serialisation context
 * @uri: URI of RDF content
 * 
 * FIXME: No error reporting provided 
 *
 * Return value: a new &librdf_stream or NULL if the parse failed.
 **/
static librdf_stream*
librdf_parser_repat_parse_as_stream(void *context, librdf_uri *uri) {
  /* Note: not yet used */
/*  librdf_parser_repat_context* pcontext=(librdf_parser_repat_context*)context; */
  librdf_parser_repat_stream_context* scontext;
  librdf_stream* stream;
  char *filename;
  char *uri_string;

  scontext=(librdf_parser_repat_stream_context*)LIBRDF_CALLOC(librdf_parser_repat_stream_context, 1, sizeof(librdf_parser_repat_stream_context));
  if(!scontext)
    return NULL;

  scontext->statements=librdf_new_list();
  if(!scontext->statements) {
    librdf_parser_repat_serialise_finished((void*)scontext);
    return NULL;
  }

  scontext->uri=uri;
  
  if(strncmp(librdf_uri_as_string(uri), "file:", 5)) {
    fprintf(stderr, "librdf_parser_repat_parse_as_stream: parser cannot handle non file: URI %s\n",
            librdf_uri_as_string(uri));
    librdf_parser_repat_serialise_finished((void*)scontext);
    return NULL;
  }
  
  uri_string=librdf_uri_as_string(uri);
  filename=uri_string+5; /* FIXME - copy this */

  scontext->fh=fopen(filename, "r");
  if(!scontext->fh) {
    LIBRDF_DEBUG2(librdf_new_parser_repat, "Failed to open file URI 'file:%s'\n", uri_string);
    librdf_parser_repat_serialise_finished((void*)scontext);
    return(NULL);
  }

  scontext->repat = RDF_ParserCreate(NULL);

  /* FIXME - Repat should allow user data per parse */
  librdf_parser_repat__user_data=scontext;

  RDF_SetStatementHandler(scontext->repat, 
                          librdf_parser_repat_statement_handler );
  
  RDF_SetParseTypeLiteralHandler(scontext->repat, 
                                 librdf_parser_repat_start_parse_type_literal_handler,
                                 librdf_parser_repat_end_parse_type_literal_handler );
  
  RDF_SetElementHandler(scontext->repat, 
                        librdf_parser_repat_start_element_handler, 
                        librdf_parser_repat_end_element_handler );
  
  RDF_SetCharacterDataHandler(scontext->repat,
                              librdf_parser_repat_character_data_handler );
  
  RDF_SetWarningHandler(scontext->repat, 
                        librdf_parser_repat_warning_handler);
  
  RDF_SetBase(scontext->repat, uri_string);


  stream=librdf_new_stream((void*)scontext,
                           &librdf_parser_repat_serialise_end_of_stream,
                           &librdf_parser_repat_serialise_next_statement,
                           &librdf_parser_repat_serialise_finished);
  if(!stream) {
    librdf_parser_repat_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


/**
 * librdf_parser_repat_parse_into_model - Retrieve the RDF/XML content at URI and store in a librdf_model
 * @context: serialisation context
 * @uri: URI of RDF content
 * @model: &librdf_model
 * 
 * FIXME: No error reporting provided 
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_repat_parse_into_model(void *context, librdf_uri *uri,
                                       librdf_model *model) {
  librdf_stream* stream;
  
  stream=librdf_parser_repat_parse_as_stream(context, uri);
  if(!stream)
    return 1;

  return librdf_model_add_statements(model, stream);
}


/* FIXME: Yeah?  What about it? */
#define LINE_BUFFER_LEN 1024


/*
 * librdf_parser_repat_get_next_statement - helper function to get the next statement
 * @context: serialisation context
 * 
 * Return value: the number of &librdf_statement-s found or 0
 */
static int
librdf_parser_repat_get_next_statement(librdf_parser_repat_stream_context *context) {
  char buffer[LINE_BUFFER_LEN];

  if(!context->fh)
    return 0;
  
  while(!feof(context->fh)) {
    int len;
    int ret;
    int count;
    
    len=fread(buffer, 1, LINE_BUFFER_LEN, context->fh);

    ret=RDF_Parse(context->repat, buffer, len, (len == 0));
    if(!len)
      return 0; /* done */

    if(!ret) {
      fprintf(stderr, "repat RDF parser failed at line %d - %s\n",
              XML_GetCurrentLineNumber( RDF_GetXmlParser(context->repat)),
              XML_ErrorString(XML_GetErrorCode(RDF_GetXmlParser(context->repat))));
      return -1; /* failed and done */
    }

    /* parsing found at least 1 statement, return */
    count=librdf_list_size(context->statements);
    if(count)
      return count;
  }


  if(feof(context->fh)) {
    fclose(context->fh);
    context->fh=NULL;
  }
  
  return 0;
}


/**
 * librdf_parser_repat_serialise_end_of_stream - Check for the end of the stream of statements from the Repat parser
 * @context: serialisation context
 * 
 * Return value: non 0 at end of stream.
 **/
static int
librdf_parser_repat_serialise_end_of_stream(void* context)
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)context;
  int count;
  
  if(scontext->end_of_stream)
    return 1;

  /* have at least 1 stored item - not end yet */
  count=librdf_list_size(scontext->statements);
  if(count > 0)
    return 0;

  count=librdf_parser_repat_get_next_statement(scontext);
  if(!count)
    scontext->end_of_stream=1;

  return (count == 0);
}


/**
 * librdf_parser_repat_serialise_next_statement - Get the next librdf_statement from the stream of statements from the Repat RDF parser
 * @context: serialisation context
 * 
 * Return value: next &librdf_statement or NULL at end of stream.
 **/
static librdf_statement*
librdf_parser_repat_serialise_next_statement(void* context)
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)context;
  librdf_statement* statement=NULL;

  /* get another statement if there is one */
  while(!scontext->end_of_stream) {
    int count;
    
    statement=(librdf_statement*)librdf_list_pop(scontext->statements);
    if(statement)
      return statement;
  
    /* else get a new one or NULL at end */
    count=librdf_parser_repat_get_next_statement(scontext);
    if(!count)
      scontext->end_of_stream=1;
  }

  return statement;
}


/**
 * librdf_parser_repat_serialise_finished - Finish the serialisation of the statement stream from the Repat RDF parse
 * @context: serialisation context
 **/
static void
librdf_parser_repat_serialise_finished(void* context)
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)context;

  if(scontext) {
    librdf_statement* statement;

    if(scontext->fh)
      fclose(scontext->fh);

    if(scontext->repat)
      RDF_ParserFree(scontext->repat);

    if(scontext->statements) {
      while((statement=librdf_list_pop(scontext->statements)))
        librdf_free_statement(statement);
      librdf_free_list(scontext->statements);
    }

    LIBRDF_FREE(librdf_parser_repat_context, scontext);
  }
}


/**
 * librdf_parser_repat_register_factory - Register the Repat RDF parser with the RDF parser factory
 * @factory: prototype rdf parser factory
 **/
static void
librdf_parser_repat_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_repat_context);
  
  factory->init  = librdf_parser_repat_init;
  factory->parse_as_stream = librdf_parser_repat_parse_as_stream;
  factory->parse_into_model = librdf_parser_repat_parse_into_model;
}


/**
 * librdf_parser_repat_constructor - Initialise the Repat RDF parser module
 **/
void
librdf_parser_repat_constructor(void)
{
  librdf_parser_register_factory("repat", &librdf_parser_repat_register_factory);
}
