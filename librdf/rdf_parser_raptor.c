/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser using Raptor
 *
 * $Id$
 *
 * Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
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
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <librdf.h>

#define RAPTOR_IN_REDLAND
#include <raptor.h>


/* serialising implementing functions */
static int librdf_parser_raptor_serialise_end_of_stream(void* context);
static int librdf_parser_raptor_serialise_next_statement(void* context);
static void* librdf_parser_raptor_serialise_get_statement(void* context, int flags);
static void librdf_parser_raptor_serialise_finished(void* context);


typedef struct {
  librdf_parser *parser;        /* librdf parser object */
  librdf_hash *bnode_hash;      /* bnode id (raptor=>internal) map */
  char *parser_name;            /* raptor parser name to use */
} librdf_parser_raptor_context;


typedef struct {
  librdf_parser_raptor_context* pcontext; /* parser context */
  raptor_parser *rdf_parser;      /* source URI string (for raptor) */

  /* when reading from a file */
  FILE *fh;

  /* when storing into a model - librdf_parser_raptor_parse_uri_into_model */
  librdf_model *model;
  
  librdf_uri *source_uri;   /* source URI */
  librdf_uri *base_uri;     /* base URI */

  /* The set of statements pending is a sequence, with 'current'
   * as the first entry and any remaining ones held in 'statements'.
   * The latter are filled by the parser
   * sequence is empty := current=NULL and librdf_list_size(statements)=0
   */
  librdf_statement* current; /* current statement */
  librdf_list statements;   /* STATIC following statements after current */

  int errors;
  int warnings;
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
  pcontext->parser_name=pcontext->parser->factory->name;
  /* legacy name - see librdf_parser_raptor_constructor
   * from when there was just one parser
   */
  if(!strcmp(pcontext->parser_name, "raptor"))
    pcontext->parser_name="rdfxml";

  /* New in-memory hash for mapping bnode IDs */
  pcontext->bnode_hash=librdf_new_hash(parser->world, NULL);
  
  /* always succeeds ? */  
  return 0;
}
  

/**
 * librdf_parser_raptor_terminate - Terminate the raptor RDF parser
 * @context: context
 **/
static void
librdf_parser_raptor_terminate(void *context) 
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;

  if(pcontext->bnode_hash)
    librdf_free_hash(pcontext->bnode_hash);
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
    node=librdf_new_node_from_blank_identifier(world, (const unsigned char*)rstatement->subject);
  } else if (rstatement->subject_type == RAPTOR_IDENTIFIER_TYPE_RESOURCE) {
    node=librdf_new_node_from_normalised_uri_string(world,
                                                    librdf_uri_as_string((librdf_uri*)rstatement->subject),
                                                    scontext->source_uri,
                                                    scontext->base_uri);
  } else {
    LIBRDF_ERROR2(world, librdf_parser_raptor_new_statement_handler,
                  "Unknown Raptor subject identifier type %d", 
                  rstatement->subject_type);
    librdf_free_statement(statement);
    return;
  }
  
  librdf_statement_set_subject(statement, node);
  
  if(rstatement->predicate_type == RAPTOR_IDENTIFIER_TYPE_ORDINAL) {
    /* FIXME - but only a little
     * Do I really need to do log10(ordinal) [or /10 and count] + 1 ? 
     * See also librdf_heuristic_gen_name for some code to repurpose.
     */
    static char ordinal_buffer[100]; 
    int ordinal=*(int*)rstatement->predicate;
    sprintf(ordinal_buffer, "http://www.w3.org/1999/02/22-rdf-syntax-ns#_%d", ordinal);
    
    node=librdf_new_node_from_uri_string(world, (const unsigned char*)ordinal_buffer);
  } else if (rstatement->predicate_type == RAPTOR_IDENTIFIER_TYPE_PREDICATE) {
    node=librdf_new_node_from_normalised_uri_string(world,
                                                    librdf_uri_as_string((librdf_uri*)rstatement->predicate),
                                                    scontext->source_uri,
                                                    scontext->base_uri);
  } else {
    LIBRDF_ERROR2(world, librdf_parser_raptor_new_statement_handler,
                  "Unknown Raptor predicate identifier type %d", 
                  rstatement->predicate_type);
    librdf_free_statement(statement);
    return;
  }

  librdf_statement_set_predicate(statement, node);


  if(rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_LITERAL ||
     rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_XML_LITERAL) {
    int is_xml_literal = (rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_XML_LITERAL);
    librdf_uri *datatype_uri=(librdf_uri*)rstatement->object_literal_datatype;
    
    
    if(is_xml_literal)
      librdf_statement_set_object(statement,
                                 librdf_new_node_from_literal(world,
                                                              (const unsigned char*)rstatement->object,
                                                              (const char*)rstatement->object_literal_language,
                                                              is_xml_literal));
    else
      librdf_statement_set_object(statement,
                                  librdf_new_node_from_typed_literal(world,
                                                                     (const unsigned char*)rstatement->object,
                                                                     (const char*)rstatement->object_literal_language,
                                                                     datatype_uri));

  } else if(rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_ANONYMOUS) {
    node=librdf_new_node_from_blank_identifier(world, (const unsigned char*)rstatement->object);
    librdf_statement_set_object(statement, node);
  } else if(rstatement->object_type == RAPTOR_IDENTIFIER_TYPE_RESOURCE) {
    node=librdf_new_node_from_normalised_uri_string(world,
                                                    librdf_uri_as_string((librdf_uri*)rstatement->object),
                                                    scontext->source_uri,
                                                    scontext->base_uri);
    librdf_statement_set_object(statement, node);
  } else {
    LIBRDF_ERROR2(world, librdf_parser_raptor_new_statement_handler,
                  "Unknown Raptor object identifier type %d", 
                  rstatement->object_type);
    librdf_free_statement(statement);
    return;
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
  } else
    librdf_list_add(&scontext->statements, statement);
}


static void
librdf_parser_raptor_error_handler(void *data, raptor_locator *locator,
                                   const char *message) 
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)data;
  librdf_parser* parser=scontext->pcontext->parser;
  static const char *message_prefix=" - Raptor error - ";
  int prefix_len=strlen(message_prefix);
  int message_len=strlen(message);
  int locator_len=raptor_format_locator(NULL, 0, locator);
  char *buffer;

  scontext->errors++;
  raptor_parse_abort(scontext->rdf_parser);

  buffer=(char*)LIBRDF_MALLOC(cstring, locator_len+prefix_len+message_len+1);
  if(!buffer) {
    fprintf(stderr, "librdf_raptor_error_handler: Out of memory\n");
    return;
  }
  raptor_format_locator(buffer, locator_len, locator);
  strncpy(buffer+locator_len, message_prefix, prefix_len);
  strcpy(buffer+prefix_len+locator_len, message); /* want extra \0 - using strcpy */

  librdf_error(parser->world, "%s", buffer);
  LIBRDF_FREE(cstring, buffer);
}


static void
librdf_parser_raptor_warning_handler(void *data, raptor_locator *locator,
                                     const char *message) 
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)data;
  librdf_parser* parser=scontext->pcontext->parser;
  static const char *message_prefix=" - Raptor warning - ";
  int prefix_len=strlen(message_prefix);
  int message_len=strlen(message);
  int locator_len=raptor_format_locator(NULL, 0, locator);
  char *buffer;

  scontext->warnings++;

  buffer=(char*)LIBRDF_MALLOC(cstring, locator_len+prefix_len+message_len+1);
  if(!buffer) {
    fprintf(stderr, "librdf_raptor_warning_handler: Out of memory\n");
    return;
  }
  raptor_format_locator(buffer, locator_len, locator);
  strncpy(buffer+locator_len, message_prefix, prefix_len);
  strcpy(buffer+prefix_len+locator_len, message); /* want extra \0 - using strcpy */

  librdf_warning(parser->world, "%s", message);
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
  unsigned char buffer[RAPTOR_IO_BUFFER_LEN];
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


static unsigned char*
librdf_parser_raptor_generate_id_handler(void *user_data,
                                         raptor_genid_type type,
                                         unsigned char *user_bnodeid) 
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)user_data;
  if(user_bnodeid) {
    unsigned char *mapped_id=(unsigned char*)librdf_hash_get(pcontext->bnode_hash, (const char*)user_bnodeid);
    if(!mapped_id) {
      mapped_id=librdf_world_get_genid(pcontext->parser->world);
      if(librdf_hash_put_strings(pcontext->bnode_hash, (char*)user_bnodeid, (char*)mapped_id))
        return NULL;
    }
    /* FIXME if have raptor in sources, otherwise SYSTEM_FREE */
    LIBRDF_FREE(cstring, (char*)user_bnodeid);
    return mapped_id;
  }
  else
    return librdf_world_get_genid(pcontext->parser->world);
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

  rdf_parser=raptor_new_parser(pcontext->parser->factory->name);
  if(!rdf_parser)
    return NULL;

  raptor_set_statement_handler(rdf_parser, scontext, 
                               librdf_parser_raptor_new_statement_handler);
  
  raptor_set_error_handler(rdf_parser, scontext, 
                           librdf_parser_raptor_error_handler);
  raptor_set_warning_handler(rdf_parser, scontext,
                             librdf_parser_raptor_warning_handler);

  raptor_set_generate_id_handler(rdf_parser, pcontext,
                                 librdf_parser_raptor_generate_id_handler);
  
  
  scontext->rdf_parser=rdf_parser;

  scontext->pcontext=pcontext;
  scontext->source_uri = librdf_new_uri_from_uri(uri);
  if(!base_uri)
    base_uri=uri;
  scontext->base_uri = librdf_new_uri_from_uri(base_uri);

  filename=(char*)librdf_uri_to_filename(uri);
  if(!filename)
    return NULL;
  
  scontext->fh=fopen(filename, "r");
  if(!scontext->fh) {
    LIBRDF_DEBUG3(librdf_parser_raptor_parse_uri_as_stream, "Failed to open file '%s' - %s\n",
                  filename, strerror(errno));
    /* FIXME if have raptor in sources, otherwise SYSTEM_FREE */
    LIBRDF_FREE(cstring, filename);
    librdf_parser_raptor_serialise_finished((void*)scontext);
    return(NULL);
  }
  /* FIXME Only if have raptor in sources, otherwise SYSTEM_FREE */
  LIBRDF_FREE(cstring, filename);

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


static void
librdf_parser_raptor_parse_uri_as_stream_write_bytes_handler(raptor_www *www,
                                                             void *userdata,
                                                             const void *ptr,
                                                             size_t size,
                                                             size_t nmemb)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)userdata;
  int len=size*nmemb;

  if(raptor_parse_chunk(scontext->rdf_parser, (const unsigned char*)ptr, len, 0))
    raptor_www_abort(www, "Parsing failed");
}


/**
 * librdf_parser_raptor_parse_as_stream_common - Retrieve the content at URI/string and parse it into a librdf_stream
 * @context: parser context
 * @uri: &librdf_uri URI of ontent source
 * @string: or content string
 * @base_uri: &librdf_uri URI of the content location or NULL if the same
 *
 * Only one of uri or string may be given
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_as_stream_common(void *context, librdf_uri *uri,
                                            const unsigned char *string,
                                            librdf_uri *base_uri)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  librdf_parser_raptor_stream_context* scontext;
  raptor_parser *rdf_parser;
  librdf_stream *stream;
  
  if(uri && librdf_uri_is_file_uri(uri))
    return librdf_parser_raptor_parse_file_as_stream(context, uri, base_uri);


  scontext=(librdf_parser_raptor_stream_context*)LIBRDF_CALLOC(librdf_parser_raptor_stream_context, 1, sizeof(librdf_parser_raptor_stream_context));
  if(!scontext)
    return NULL;

  rdf_parser=raptor_new_parser(pcontext->parser_name);
  if(!rdf_parser) {
    LIBRDF_FREE(librdf_parser_raptor_stream_context, scontext);
    return NULL;
  }

  raptor_set_statement_handler(rdf_parser, scontext, 
                               librdf_parser_raptor_new_statement_handler);
  
  raptor_set_error_handler(rdf_parser, scontext, 
                           librdf_parser_raptor_error_handler);
  raptor_set_warning_handler(rdf_parser, scontext,
                             librdf_parser_raptor_warning_handler);

  raptor_set_generate_id_handler(rdf_parser, pcontext,
                                 librdf_parser_raptor_generate_id_handler);
  
  
  scontext->rdf_parser=rdf_parser;

  scontext->pcontext=pcontext;
  if(!base_uri)
    base_uri=uri;

  /* No base URI given, cannot proceed */
  if(!base_uri)
    return NULL;

  if(uri)
    scontext->source_uri = librdf_new_uri_from_uri(uri);
  else
    scontext->source_uri = librdf_new_uri_from_uri(base_uri);
  scontext->base_uri = librdf_new_uri_from_uri(base_uri);

  if(uri) {
    raptor_www *www=raptor_www_new();
    if(!www) {
      LIBRDF_FREE(librdf_parser_raptor_stream_context, scontext);
      raptor_free_parser(rdf_parser);
      return NULL;
    }
    
    raptor_www_set_write_bytes_handler(www, 
                                       librdf_parser_raptor_parse_uri_as_stream_write_bytes_handler,
                                       scontext);
    
    if(raptor_start_parse(rdf_parser, (raptor_uri*)base_uri)) {
      raptor_www_free(www);
      return NULL;
    }
    
    raptor_www_fetch(www, (raptor_uri*)uri);
    raptor_parse_chunk(rdf_parser, NULL, 0, 1);

    raptor_www_free(www);
  } else {
    if(raptor_start_parse(rdf_parser, (raptor_uri*)base_uri))
      return NULL;
    
    raptor_parse_chunk(rdf_parser, string, strlen((const char*)string), 1);
  }
  

  /* get first statement, else is empty */
  scontext->current=(librdf_statement*)librdf_list_pop(&scontext->statements);

  stream=librdf_new_stream(base_uri->world,
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
 * librdf_parser_raptor_parse_uri_as_stream - Retrieve the content at URI and parse it into a librdf_stream
 * @context: parser context
 * @uri: &librdf_uri URI of content source
 * @base_uri: &librdf_uri URI of the content location or NULL if the same
 *
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_uri_as_stream(void *context, librdf_uri *uri,
                                         librdf_uri *base_uri)
{
  return librdf_parser_raptor_parse_as_stream_common(context, 
                                                     uri, NULL, base_uri);
}


/**
 * librdf_parser_raptor_parse_string_as_stream - Parse the content in a string and return a librdf_stream
 * @context: parser context
 * @string: string content to parse
 * @base_uri: &librdf_uri URI of the content location or NULL if the same
 *
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_string_as_stream(void *context, 
                                            const unsigned char *string,
                                            librdf_uri *base_uri)
{
  return librdf_parser_raptor_parse_as_stream_common(context, 
                                                     NULL, string, base_uri);
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
librdf_parser_raptor_parse_into_model_common(void *context,
                                             librdf_uri *uri, 
                                             const unsigned char *string,
                                             librdf_uri *base_uri,
                                             librdf_model* model)
{
  int status=0;
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  librdf_parser_raptor_stream_context* scontext;
  raptor_parser *rdf_parser;
  
  scontext=(librdf_parser_raptor_stream_context*)LIBRDF_CALLOC(librdf_parser_raptor_stream_context, 1, sizeof(librdf_parser_raptor_stream_context));
  if(!scontext)
    return 1;

  rdf_parser=raptor_new_parser(pcontext->parser_name);
  if(!rdf_parser) {
    LIBRDF_FREE(librdf_parser_raptor_stream_context, scontext);
    return 1;
  }

  raptor_set_statement_handler(rdf_parser, scontext, 
                               librdf_parser_raptor_new_statement_handler);
  
  raptor_set_error_handler(rdf_parser, scontext, 
                           librdf_parser_raptor_error_handler);
  raptor_set_warning_handler(rdf_parser, scontext,
                             librdf_parser_raptor_warning_handler);

  raptor_set_generate_id_handler(rdf_parser, pcontext,
                                 librdf_parser_raptor_generate_id_handler);
  
  
  scontext->rdf_parser=rdf_parser;
  scontext->pcontext=pcontext;

  if(!base_uri)
    base_uri=uri;

  /* No base URI given, cannot proceed */
  if(!base_uri)
    return 1;
    
  if(uri)
    scontext->source_uri = librdf_new_uri_from_uri(uri);
  else
    scontext->source_uri = librdf_new_uri_from_uri(base_uri);

  scontext->base_uri = librdf_new_uri_from_uri(base_uri);

  /* direct into model */
  scontext->model = model;

  if(uri) {
    status=raptor_parse_uri(rdf_parser, (raptor_uri*)uri, (raptor_uri*)base_uri);
  } else {
    status=raptor_start_parse(rdf_parser, (raptor_uri*)base_uri);
    if(!status)
      status=raptor_parse_chunk(rdf_parser, string, strlen((const char*)string), 1);
  }
  

  librdf_parser_raptor_serialise_finished((void*)scontext);

  return status;
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
  return librdf_parser_raptor_parse_into_model_common(context,
                                                      uri, NULL,
                                                      base_uri, model);}


/**
 * librdf_parser_raptor_parse_string_into_model - Parse the RDF/XML content in a string and store it into a librdf_model
 * @context: parser context
 * @string: RDF/XML source
 * @base_uri: &librdf_uri URI of the content location
 * @model: &librdf_model of model
 *
 * Stores the statements found parsing string and stores in the given model.
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_raptor_parse_string_into_model(void *context, 
                                             const unsigned char *string,
                                             librdf_uri *base_uri,
                                             librdf_model* model)
{
  return librdf_parser_raptor_parse_into_model_common(context,
                                                      NULL, string,
                                                      base_uri, model);
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
      LIBRDF_ERROR2(scontext->pcontext->parser->world,
                    librdf_parser_raptor_serialise_get_statement,
                    "Unknown iterator method flag %d\n", flags);
      return NULL;
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
      raptor_free_parser(scontext->rdf_parser);

    if(scontext->current)
      librdf_free_statement(scontext->current);

    if(scontext->source_uri)
      librdf_free_uri(scontext->source_uri); 
 
    if(scontext->base_uri)
      librdf_free_uri(scontext->base_uri); 
 
    while((statement=(librdf_statement*)librdf_list_pop(&scontext->statements)))
      librdf_free_statement(statement);
    librdf_list_clear(&scontext->statements);

    LIBRDF_FREE(librdf_parser_raptor_context, scontext);
  }
}


static raptor_uri*
librdf_raptor_new_uri(void *context, const unsigned char *uri_string) 
{
  return (raptor_uri*)librdf_new_uri((librdf_world*)context, uri_string);
}

static raptor_uri*
librdf_raptor_new_uri_from_uri_local_name(void *context,
                                          raptor_uri *uri,
                                          const unsigned char *local_name)
{
   return (raptor_uri*)librdf_new_uri_from_uri_local_name((librdf_uri*)uri, local_name);
}

static raptor_uri*
librdf_raptor_new_uri_relative_to_base(void *context,
                                       raptor_uri *base_uri,
                                       const unsigned char *uri_string) 
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


static unsigned char*
librdf_raptor_uri_as_string(void *context, raptor_uri *uri)
{
  return librdf_uri_as_string((librdf_uri*)uri);
}


static unsigned char*
librdf_raptor_uri_as_counted_string(void *context, raptor_uri *uri, size_t *len_p)
{
  return librdf_uri_as_counted_string((librdf_uri*)uri, len_p);
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
  librdf_raptor_uri_as_counted_string,
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
  factory->terminate  = librdf_parser_raptor_terminate;
  factory->parse_uri_as_stream = librdf_parser_raptor_parse_uri_as_stream;
  factory->parse_uri_into_model = librdf_parser_raptor_parse_uri_into_model;
  factory->parse_string_as_stream = librdf_parser_raptor_parse_string_as_stream;
  factory->parse_string_into_model = librdf_parser_raptor_parse_string_into_model;
}


/**
 * librdf_parser_raptor_constructor - Initialise the raptor RDF parser module
 * @world: redland world object
 **/
void
librdf_parser_raptor_constructor(librdf_world *world)
{
  unsigned int i;
  raptor_init();

  raptor_uri_set_handler(&librdf_raptor_uri_handler, world);

  for(i=0; 1; i++) {
    const char *syntax_name=NULL;
    const char *mime_type=NULL;
    const unsigned char *uri_string=NULL;

    /* FIXME - after redland 0.9.15, depend on raptor 1.2.0 with
     * function raptor_syntaxes_enumerate available
     */
#if 0
    if(raptor_syntaxes_enumerate(i, &syntax_name, NULL, 
                                 &mime_type, &uri_string))
      break;
#else
    if(raptor_parsers_enumerate(i, &syntax_name, NULL))
      break;

    if(!strcmp(syntax_name, "rdfxml")) {
      mime_type="application/rdf+xml";
      uri_string=(const unsigned char*)"http://www.w3.org/TR/rdf-syntax-grammar";
      /* legacy name - see librdf_parser_raptor_init */
      librdf_parser_register_factory(world, "raptor", NULL, NULL,
                                     &librdf_parser_raptor_register_factory);
    } else if (!strcmp(syntax_name, "ntriples")) {
      mime_type="text/plain";
      uri_string=(const unsigned char*)"http://www.w3.org/TR/rdf-testcases/#ntriples";
    } else if (!strcmp(syntax_name, "ntriples-plus")) {
      uri_string=(const unsigned char*)"http://www.ilrt.bristol.ac.uk/discovery/2003/11/ntriplesplus/";
    }
#endif

    librdf_parser_register_factory(world, syntax_name, mime_type, uri_string,
                                   &librdf_parser_raptor_register_factory);
  }
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
