/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - Repat RDF Parser implementation
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

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

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
  librdf_parser *parser;    /* Redland parser object */
} librdf_parser_repat_context;


/*
 * context used for the Repat parser to create &librdf_stream
 * of statements from parsing
 */
typedef struct {
  librdf_parser_repat_context* pcontext; /* parser context */
  librdf_uri* source_uri;    /* source URI */
  char *filename;            /* filename part of that */
  librdf_uri* base_uri;      /* base URI */
  FILE *fh;
  RDF_Parser repat;
  librdf_list* statements;  /* pending statements */
  int end_of_stream;        /* non 0 if stream finished */
  int in_literal;           /* non 0 if parseType literal */
  char *literal;            /* literal text being collected */
  int literal_length;
  librdf_statement *saved_statement; /* partial statement waiting
                                      * for XML literal content node
                                      * to be attached 
                                      * Only valid when in_literal !=0
                                      */
} librdf_parser_repat_stream_context;



/* repat callback handlers */

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
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)user_data;
  librdf_statement* statement=NULL;
  librdf_node *subject=NULL, *predicate=NULL, *object=NULL;
  librdf_world *world=scontext->pcontext->parser->world;

  /* got all statement parts now */
  statement=librdf_new_statement(world);
  if(!statement)
    return;

  switch(subject_type)
  {
    case RDF_SUBJECT_TYPE_URI:
      subject=librdf_new_node_from_normalised_uri_string(world,
                                                         subject_string,
                                                         scontext->source_uri,
                                                         scontext->base_uri);
      break;
    case RDF_SUBJECT_TYPE_DISTRIBUTED:
      subject=librdf_new_node_from_uri_string(world, subject_string);
      break;
    case RDF_SUBJECT_TYPE_PREFIX:
      subject=librdf_new_node_from_uri_string(world, subject_string);
      break;
    case RDF_SUBJECT_TYPE_ANONYMOUS:
      subject=librdf_new_node_from_uri_string(world, subject_string);
      break;
    default:
      LIBRDF_FATAL2(librdf_parser_repat_statement_handler, "Unknown subject type %d\n", subject_type);
  }
  if(!subject) {
    librdf_free_statement(statement);
    return;
  }
  librdf_statement_set_subject(statement,subject);

  
  if(!ordinal) {
    predicate=librdf_new_node_from_normalised_uri_string(world, 
                                                         predicate_string,
                                                         scontext->source_uri,
                                                         scontext->base_uri);
  } else {
    /* Generate an rdf:_<ordinal> predicate */
    const char *li_prefix="http://www.w3.org/1999/02/22-rdf-syntax-ns#_";
    char *ordinal_predicate_string;
    int o=ordinal;
    int len= 2 + strlen(li_prefix); /* 1-digit + string length + '\0' */

    /* increase len to hold width of integer */
    while((o /= 10)>0) 
      len++;
    ordinal_predicate_string=(char*)LIBRDF_MALLOC(cstring, len);
    if(predicate_string) {
      sprintf(ordinal_predicate_string, "%s%d", li_prefix, ordinal);
      predicate=librdf_new_node_from_uri_string(world, 
                                                ordinal_predicate_string);
      LIBRDF_FREE(cstring, ordinal_predicate_string);
    }
  }
  if(!predicate) {
    librdf_free_statement(statement);
    return;
  }
  librdf_statement_set_predicate(statement,predicate);

  
  switch( object_type )
  {
    case RDF_OBJECT_TYPE_RESOURCE:
      object=librdf_new_node_from_normalised_uri_string(world, 
                                                        object_string,
                                                        scontext->source_uri,
                                                        scontext->base_uri);
      break;
    case RDF_OBJECT_TYPE_LITERAL:
      if(scontext->literal) {
        LIBRDF_DEBUG2(librdf_parser_repat_end_element_handler,
                      "found literal text: '%s'\n", scontext->literal);
        object=librdf_new_node_from_literal(world, 
                                            scontext->literal, NULL, 0, 0);
        LIBRDF_FREE(cstring, scontext->literal);
        scontext->literal=NULL;
        scontext->literal_length=0;
      } else {
        if(!object_string)
          object_string="";
        object=librdf_new_node_from_literal(world, 
                                            object_string, xml_lang, 0, 0);
      }
      break;
    case RDF_OBJECT_TYPE_XML:
      /* A statement where the object is parseType=Literal and will
       * turn up later, so save the partial statement to add
       * in librdf_parser_repat_end_parse_type_literal_handler()
       */
      scontext->saved_statement=statement;
      return;

      break;
    default:
      LIBRDF_FATAL2(librdf_parser_repat_statement_handler, "Unknown object type %d\n", object_type);
  }
  if(!object) {
    librdf_free_statement(statement);
    return;
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
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)user_data;
  if(scontext->in_literal)
    LIBRDF_FATAL1(librdf_parser_repat_start_parse_type_literal_handler,
                 "started literal content while in literal content\n");
    
  scontext->in_literal=1;
  scontext->literal=NULL;
  scontext->literal_length=0;
}


static void 
librdf_parser_repat_end_parse_type_literal_handler( void* user_data )
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)user_data;
  librdf_statement* statement=scontext->saved_statement;
  librdf_world *world=scontext->pcontext->parser->world;

  if(statement) {
    librdf_node* object;

    /* create XML content literal string */

    /* FIXME: This does not care about namespaced-elements or
     * attributes which will appear in the literal string like this:
     *   'URI-of-element-namespace '^' qname
     * i.e. the literal is NOT legal an XML document or XML fragment.
     */
    object=librdf_new_node_from_literal(world, 
                                        scontext->literal, NULL, 0, 1);
    LIBRDF_FREE(cstring, scontext->literal);
    scontext->literal=NULL;
    scontext->literal_length=0;
 
    librdf_statement_set_object(statement,object);

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    fprintf(stderr, "librdf_parser_repat_end_parse_type_literal_handler: Adding statement: ");
    librdf_statement_print(statement, stderr);
    fputs("\n", stderr);
#endif

    librdf_list_add(scontext->statements, statement);
    
  } else {
    LIBRDF_FATAL1(librdf_parser_repat_end_element_handler,
                  "Found end of literal XML but no saved statement\n");
  }
  scontext->in_literal=0;
  scontext->saved_statement=NULL;
}


static void 
librdf_parser_repat_start_element_handler(void* user_data, 
                                          const XML_Char* name,
                                          const XML_Char** attributes)
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)user_data;
  char *buffer;
  int length, l;
  char *ptr;
  int i;

  length = 1 + strlen(name); /* <NAME */
  for(i=0; attributes[i];) {
    if(!i)
      length++; /* ' ' between attributes */
    length += strlen(attributes[i]) + 2; /* =" */
    i++;
    length += strlen(attributes[i]) + 1; /* " */
    i++;
  }
  length++; /* > */


  /* +1 here is for \0 at end */
  buffer=(char*)LIBRDF_MALLOC(cstring, scontext->literal_length + length + 1);
  if(!buffer) {
    librdf_parser_error(scontext->pcontext->parser, "Out of memory");
    return;
  }

  if(scontext->literal_length) {
    strncpy(buffer, scontext->literal, scontext->literal_length);
    LIBRDF_FREE(cstring, scontext->literal);
  }
  scontext->literal=buffer;

  ptr=buffer+scontext->literal_length; /* append */

  /* adjust stored length */
  scontext->literal_length += length;

  /* now write new stuff at end of literal buffer */
  *ptr++ = '<';
  l=strlen(name);
  strcpy(ptr, name);
  ptr += l;
   
  for(i=0; attributes[i];) {
    if(!i)
      *ptr++ =' ';
    
    l=strlen(attributes[i]);
    strcpy(ptr, attributes[i]);
    ptr += l;

    *ptr++ ='=';
    *ptr++ ='"';

    i++;

    l=strlen(attributes[i]);
    strcpy(ptr, attributes[i]);
    ptr += l;
    *ptr++ ='"';
    
    i++;
  }
  *ptr++ = '>';
  *ptr='\0';

  LIBRDF_DEBUG2(librdf_parser_repat_start_element_handler,
                "literal text now: '%s'\n", buffer);
}


static void
librdf_parser_repat_end_element_handler(void* user_data,
                                        const XML_Char* name)
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)user_data;
  char *buffer;
  int length, l;
  char *ptr;
  
  length = 3 + strlen(name); /* <NAME/> */

  /* +1 here is for \0 at end */
  buffer=(char*)LIBRDF_MALLOC(cstring, scontext->literal_length + length + 1);
  if(!buffer) {
    librdf_parser_error(scontext->pcontext->parser, "Out of memory");
    return;
  }

  if(scontext->literal_length) {
    strncpy(buffer, scontext->literal, scontext->literal_length);
    LIBRDF_FREE(cstring, scontext->literal);
  }
  scontext->literal=buffer;

  ptr=buffer+scontext->literal_length; /* append */

  /* adjust stored length */
  scontext->literal_length += length;

  /* now write new stuff at end of literal buffer */
  *ptr++ = '<';
  *ptr++ = '/';
  l=strlen(name);
  strcpy(ptr, name);
  ptr += l;
  *ptr++ = '>';
  *ptr='\0';

  LIBRDF_DEBUG2(librdf_parser_repat_end_element_handler,
                "literal text now: '%s'\n", buffer);
}


static void 
librdf_parser_repat_character_data_handler(void* user_data,
                                           const XML_Char* s,
                                           int len)
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)user_data;
  char *buffer;
  char *ptr;
  
  /* +1 here is for \0 at end */
  buffer=(char*)LIBRDF_MALLOC(cstring, scontext->literal_length + len + 1);
  if(!buffer) {
    librdf_parser_error(scontext->pcontext->parser, "Out of memory");
    return;
  }

  if(scontext->literal_length) {
    strncpy(buffer, scontext->literal, scontext->literal_length);
    LIBRDF_FREE(cstring, scontext->literal);
  }
  scontext->literal=buffer;

  ptr=buffer+scontext->literal_length; /* append */

  /* adjust stored length */
  scontext->literal_length += len;

  /* now write new stuff at end of literal buffer */
  strncpy(ptr, s, len);
  ptr += len;
  *ptr = '\0';

  LIBRDF_DEBUG2(librdf_parser_repat_character_data_handler,
                "literal text now: '%s'\n", buffer);
}


static void 
librdf_parser_repat_warning_handler(void* user_data, const XML_Char* msg)
{
  librdf_parser_repat_stream_context* scontext=(librdf_parser_repat_stream_context*)user_data;
  librdf_parser_warning(scontext->pcontext->parser, msg);
}


/**
 * librdf_parser_repat_init - Initialise the Repat RDF parser
 * @parser: the parser
 * @context: context
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_repat_init(librdf_parser *parser, void *context) 
{
  librdf_parser_repat_context* pcontext=(librdf_parser_repat_context*)context;
  pcontext->parser = parser;

  return 0;
}
  

/**
 * librdf_parser_repat_parse_as_stream - Retrieve the RDF/XML content at file and parse it into a librdf_stream
 * @context: serialisation context
 * @uri: URI of RDF content source
 * @base_uri: the base URI to use (or NULL if the same)
 * 
 * Return value: a new &librdf_stream or NULL if the parse failed.
 **/
static librdf_stream*
librdf_parser_repat_parse_file_as_stream(void *context,
                                         librdf_uri *uri, librdf_uri *base_uri) {
  librdf_parser_repat_context* pcontext=(librdf_parser_repat_context*)context;
  librdf_parser_repat_stream_context* scontext;
  librdf_stream* stream;
  const char* filename;
  librdf_world *world=pcontext->parser->world;

  scontext=(librdf_parser_repat_stream_context*)LIBRDF_CALLOC(librdf_parser_repat_stream_context, 1, sizeof(librdf_parser_repat_stream_context));
  if(!scontext)
    return NULL;

  scontext->pcontext=pcontext;
  
  scontext->statements=librdf_new_list(world);
  if(!scontext->statements) {
    librdf_parser_repat_serialise_finished((void*)scontext);
    return NULL;
  }

  scontext->source_uri=uri;
  scontext->base_uri=base_uri;

  filename=librdf_uri_as_filename(uri);
  if(!filename)
    return NULL;
  
  scontext->fh=fopen(filename, "r");
  if(!scontext->fh) {
    extern int errno;
    
    LIBRDF_DEBUG3(librdf_new_parser_repat, "Failed to open file '%s' - %s\n",
                  filename, strerror(errno));
    librdf_parser_repat_serialise_finished((void*)scontext);
    return(NULL);
  }

  scontext->repat = RDF_ParserCreate(NULL);

  RDF_SetUserData(scontext->repat, scontext);

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
  
  RDF_SetBase(scontext->repat, librdf_uri_as_string(base_uri));


  stream=librdf_new_stream(world,
                           (void*)scontext,
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
 * @base_uri: the base URI to use (or NULL if the same)
 * @model: &librdf_model
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_parser_repat_parse_file_into_model(void *context,
                                          librdf_uri *uri, 
                                          librdf_uri *base_uri,
                                          librdf_model *model) {
  librdf_stream* stream;
  
  stream=librdf_parser_repat_parse_file_as_stream(context,
                                                  uri, base_uri);
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
      librdf_parser_error(context->pcontext->parser,
                          "line %d - %s",
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
  if(count <= 0)
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
    if(count <= 0)
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
      while((statement=(librdf_statement*)librdf_list_pop(scontext->statements)))
        librdf_free_statement(statement);
      librdf_free_list(scontext->statements);
    }

    /* adjust stored length */
    if(scontext->literal)
      LIBRDF_FREE(cstring, scontext->literal);

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
  factory->parse_file_as_stream = librdf_parser_repat_parse_file_as_stream;
  factory->parse_file_into_model = librdf_parser_repat_parse_file_into_model;
}


/**
 * librdf_parser_repat_constructor - Initialise the Repat RDF parser module
 * @world: redland world object
 **/
void
librdf_parser_repat_constructor(librdf_world *world)
{
  librdf_parser_register_factory(world, "repat", NULL, NULL,
                                 &librdf_parser_repat_register_factory);
}
