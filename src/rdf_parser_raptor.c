/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser_raptor.c - RDF Parser using Raptor
 *
 * Copyright (C) 2000-2011, David Beckett http://www.dajobe.org/
 * Copyright (C) 2000-2004, University of Bristol, UK http://www.bristol.ac.uk/
 *
 * This package is Free Software and part of Redland http://librdf.org/
 *
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 *
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 *
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <redland.h>


/* serialising implementing functions */
static int librdf_parser_raptor_serialise_end_of_stream(void* context);
static int librdf_parser_raptor_serialise_next_statement(void* context);
static void* librdf_parser_raptor_serialise_get_statement(void* context, int flags);
static void librdf_parser_raptor_serialise_finished(void* context);


typedef struct {
  librdf_parser *parser;        /* librdf parser object */
  raptor_parser *rdf_parser;    /* source URI string (for raptor) */
  const char *parser_name;      /* raptor parser name to use */

  raptor_sequence* nspace_prefixes;
  raptor_sequence* nspace_uris;

  int errors;
  int warnings;

  raptor_www *www;              /* raptor stream */
  void *stream_context;         /* librdf_parser_raptor_stream_context* */
} librdf_parser_raptor_context;


typedef struct {
  librdf_parser_raptor_context* pcontext; /* parser context */

  /* when reading from a file */
  FILE *fh;
  /* when true, this FH is closed on finish */
  int close_fh;

  /* when finished */
  int finished;

  /* when storing into a model - librdf_parser_raptor_parse_uri_into_model */
  librdf_model *model;

  /* The set of statements pending is a sequence, with 'current'
   * as the first entry and any remaining ones held in 'statements'.
   * The latter are filled by the parser
   * sequence is empty := current=NULL and librdf_list_size(statements)=0
   */
  librdf_statement* current; /* current statement */
  librdf_list* statements;
} librdf_parser_raptor_stream_context;


static int
librdf_parser_raptor_relay_filter(void* user_data, raptor_uri* uri)
{
  librdf_parser* parser=(librdf_parser*)user_data;
  return parser->uri_filter(parser->uri_filter_user_data, (librdf_uri*)uri);
}


/**
 * librdf_parser_raptor_init:
 * @parser: the parser
 * @context: context
 *
 * Initialise the raptor RDF parser.
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
    pcontext->parser_name = "rdfxml";

  pcontext->rdf_parser = raptor_new_parser(parser->world->raptor_world_ptr,
                                           pcontext->parser_name);

  if(!pcontext->rdf_parser)
    return 1;

  librdf_raptor_reset_bnode_hash(parser->world);

  return 0;
}


/**
 * librdf_parser_raptor_terminate:
 * @context: context
 *
 * Terminate the raptor RDF parser.
 *
 **/
static void
librdf_parser_raptor_terminate(void *context)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;

  librdf_raptor_free_bnode_hash(pcontext->parser->world);
  
  if(pcontext->stream_context)
    librdf_parser_raptor_serialise_finished(pcontext->stream_context);

  if(pcontext->www)
    raptor_free_www(pcontext->www);

  if(pcontext->rdf_parser)
    raptor_free_parser(pcontext->rdf_parser);

  if(pcontext->nspace_prefixes)
    raptor_free_sequence(pcontext->nspace_prefixes);
  if(pcontext->nspace_uris)
    raptor_free_sequence(pcontext->nspace_uris);
}


/*
 * librdf_parser_raptor_new_statement_handler - helper callback function for raptor RDF when a new triple is asserted
 * @context: context for callback
 * @statement: raptor_statement
 *
 * Adds the statement to the list of statements.
 */
static void
librdf_parser_raptor_new_statement_handler(void *context,
                                           raptor_statement *rstatement)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;
  librdf_node* node;
  librdf_statement* statement;
  librdf_world* world=scontext->pcontext->parser->world;
  int rc;

  statement=librdf_new_statement(world);
  if(!statement)
    return;

  if(rstatement->subject->type == RAPTOR_TERM_TYPE_BLANK) {
    node = librdf_new_node_from_blank_identifier(world, (const unsigned char*)rstatement->subject->value.blank.string);
  } else if (rstatement->subject->type == RAPTOR_TERM_TYPE_URI) {
    node = librdf_new_node_from_uri(world, (librdf_uri*)rstatement->subject->value.uri);
  } else {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Unknown Raptor subject identifier type %d",
               rstatement->subject->type);
    librdf_free_statement(statement);
    return;
  }

  if(!node) {
    librdf_log(world,
               0, LIBRDF_LOG_FATAL, LIBRDF_FROM_PARSER, NULL,
               "Cannot create subject node");
    librdf_free_statement(statement);
    return;
  }

  librdf_statement_set_subject(statement, node);


  if(rstatement->predicate->type == RAPTOR_TERM_TYPE_URI) {
    node = librdf_new_node_from_uri(world, (librdf_uri*)rstatement->predicate->value.uri);
  } else {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Unknown Raptor predicate identifier type %d",
               rstatement->predicate->type);
    librdf_free_statement(statement);
    return;
  }

  if(!node) {
    librdf_log(world,
               0, LIBRDF_LOG_FATAL, LIBRDF_FROM_PARSER, NULL,
               "Cannot create predicate node");
    librdf_free_statement(statement);
    return;
  }

  librdf_statement_set_predicate(statement, node);

  if(rstatement->object->type == RAPTOR_TERM_TYPE_LITERAL) {
    node = librdf_new_node_from_typed_literal(world,
                                              rstatement->object->value.literal.string,
                                              (const char *)rstatement->object->value.literal.language,
                                              (librdf_uri*)rstatement->object->value.literal.datatype);
  } else if(rstatement->object->type == RAPTOR_TERM_TYPE_BLANK) {
    node = librdf_new_node_from_blank_identifier(world, rstatement->object->value.blank.string);
  } else if(rstatement->object->type == RAPTOR_TERM_TYPE_URI) {
    node = librdf_new_node_from_uri(world, (librdf_uri*)rstatement->object->value.uri);
  } else {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Unknown Raptor object identifier type %d",
               rstatement->object->type);
    librdf_free_statement(statement);
    return;
  }

  if(!node) {
    librdf_log(world,
               0, LIBRDF_LOG_FATAL, LIBRDF_FROM_PARSER, NULL,
               "Cannot create object node");
    librdf_free_statement(statement);
    return;
  }

  librdf_statement_set_object(statement, node);

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  if(1) {
    raptor_iostream *iostr;
    iostr = raptor_new_iostream_to_file_handle(world->raptor_world_ptr, stderr);
    librdf_statement_write(statement, iostr);
    raptor_free_iostream(iostr);
  }
#endif

  if(scontext->model) {
    rc=librdf_model_add_statement(scontext->model, statement);
    librdf_free_statement(statement);
  } else {
    rc=librdf_list_add(scontext->statements, statement);
    if(rc)
      librdf_free_statement(statement);
  }
  if(rc) {
    librdf_log(world,
               0, LIBRDF_LOG_FATAL, LIBRDF_FROM_PARSER, NULL,
               "Cannot add statement to model");
  }
}


/*
 * librdf_parser_raptor_namespace_handler - helper callback function for raptor RDF when a namespace is seen
 * @context: context for callback
 * @statement: raptor_statement
 *
 * Adds the statement to the list of statements.
 */
static void
librdf_parser_raptor_namespace_handler(void* user_data,
                                       raptor_namespace *nspace)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)user_data;
  const unsigned char* prefix;
  unsigned char* nprefix;
  size_t prefix_length;
  librdf_uri* uri;
  int i;

  uri=(librdf_uri*)raptor_namespace_get_uri(nspace);
  if(!uri)
    return;

  for(i=0; i < raptor_sequence_size(pcontext->nspace_uris); i++) {
    librdf_uri* u=(librdf_uri*)raptor_sequence_get_at(pcontext->nspace_uris, i);
    if(librdf_uri_equals(uri, u))
      return;
  }

  /* new namespace */
  uri=librdf_new_uri_from_uri(uri);
  raptor_sequence_push(pcontext->nspace_uris, uri);

  prefix=raptor_namespace_get_counted_prefix(nspace, &prefix_length);
  if(prefix) {
    nprefix = LIBRDF_MALLOC(unsigned char*, prefix_length + 1);
    /* FIXME: what if nprefix alloc failed? now just pushes NULL to sequence */
    if(nprefix)
      strncpy((char*)nprefix, (const char*)prefix, prefix_length+1);
  } else
    nprefix=NULL;
  raptor_sequence_push(pcontext->nspace_prefixes, nprefix);
}


/* FIXME: Yeah?  What about it? */
#define RAPTOR_IO_BUFFER_LEN 1024


/*
 * librdf_parser_raptor_get_next_statement - helper function to get the next statement
 * @context: serialisation context
 *
 * Return value: >0 if a statement found, 0 at end of file, or <0 on error
 */
static int
librdf_parser_raptor_get_next_statement(librdf_parser_raptor_stream_context *context) {
  unsigned char buffer[RAPTOR_IO_BUFFER_LEN];
  int status=0;

  if(context->finished || !context->fh)
    return 0;

  context->current=NULL;
  while(!feof(context->fh)) {
    size_t len;
    int ret;

    len = fread(buffer, 1, RAPTOR_IO_BUFFER_LEN, context->fh);
    ret = raptor_parser_parse_chunk(context->pcontext->rdf_parser, buffer, len,
                                    (len < RAPTOR_IO_BUFFER_LEN));

    if(ret) {
      status=(-1);
      break; /* failed and done */
    }

    /* parsing found at least 1 statement, return */
    if(librdf_list_size(context->statements)) {
      context->current=(librdf_statement*)librdf_list_pop(context->statements);
      status=1;
      break;
    }

    if(len < RAPTOR_IO_BUFFER_LEN)
      break;
  }

  if(feof(context->fh) || status <1)
    context->finished=1;

  return status;
}


/*
 * librdf_parser_raptor_parse_file_handle_as_stream:
 * @context: parser context
 * @fh: FILE* of content source
 * @close_fh: if true to fclose(fh) on finish
 * @base_uri: #librdf_uri URI of the content location
 *
 * Retrieve content from FILE* @fh and parse it into a #librdf_stream.
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_file_handle_as_stream(void *context,
                                                 FILE *fh, int close_fh,
                                                 librdf_uri *base_uri)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  librdf_parser_raptor_stream_context* scontext;
  librdf_stream *stream;
  int rc;
  int need_base_uri;
  const raptor_syntax_description *desc;

  librdf_world_open(pcontext->parser->world);

  desc = raptor_parser_get_description(pcontext->rdf_parser);
  if(!desc) {
    librdf_log(pcontext->parser->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Could not get description for %s parser",
               pcontext->parser_name);
    return NULL;
  }
  need_base_uri = desc->flags & RAPTOR_SYNTAX_NEED_BASE_URI;

  if(need_base_uri && !base_uri) {
    librdf_log(pcontext->parser->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Missing base URI for %s parser",
               pcontext->parser_name);
    return NULL;
  }


  pcontext->errors=0;
  pcontext->warnings=0;

  scontext = LIBRDF_CALLOC(librdf_parser_raptor_stream_context*, 1,
                           sizeof(*scontext));
  if(!scontext)
    goto oom;

  scontext->pcontext=pcontext;
  pcontext->stream_context=scontext;

  scontext->statements=librdf_new_list(pcontext->parser->world);
  if(!scontext->statements)
    goto oom;

  if(pcontext->nspace_prefixes)
    raptor_free_sequence(pcontext->nspace_prefixes);
  pcontext->nspace_prefixes=raptor_new_sequence(free, NULL);
  if(!pcontext->nspace_prefixes)
    goto oom;

  if(pcontext->nspace_uris)
    raptor_free_sequence(pcontext->nspace_uris);

  pcontext->nspace_uris = raptor_new_sequence((raptor_data_free_handler)librdf_free_uri, NULL);
  if(!pcontext->nspace_uris)
    goto oom;

  raptor_parser_set_statement_handler(pcontext->rdf_parser, scontext,
                                      librdf_parser_raptor_new_statement_handler);
  raptor_parser_set_namespace_handler(pcontext->rdf_parser, pcontext,
                                      librdf_parser_raptor_namespace_handler);


  scontext->fh=fh;
  scontext->close_fh=close_fh;

  if(pcontext->parser->uri_filter)
    raptor_parser_set_uri_filter(pcontext->rdf_parser,
                                 librdf_parser_raptor_relay_filter,
                                 pcontext->parser);

  /* Start the parse */
  stream = NULL;

  rc = raptor_parser_parse_start(pcontext->rdf_parser, (raptor_uri*)base_uri);
  if(!rc) {
    /* start parsing; initialises scontext->statements, scontext->current */
    librdf_parser_raptor_get_next_statement(scontext);

    stream=librdf_new_stream(pcontext->parser->world,
                             (void*)scontext,
                             &librdf_parser_raptor_serialise_end_of_stream,
                             &librdf_parser_raptor_serialise_next_statement,
                             &librdf_parser_raptor_serialise_get_statement,
                             &librdf_parser_raptor_serialise_finished);
  }
  if(!stream)
    goto oom;

  return stream;

  /* Clean up and report an error on OOM */
  oom:
  librdf_parser_raptor_serialise_finished((void*)scontext);
  librdf_log(pcontext->parser->world,
             0, LIBRDF_LOG_FATAL, LIBRDF_FROM_PARSER, NULL,
             "Out of memory");
  return NULL;
}


static void
librdf_parser_raptor_parse_uri_as_stream_write_bytes_handler(raptor_www *www,
                                                             void *userdata,
                                                             const void *ptr,
                                                             size_t size,
                                                             size_t nmemb)
{
  librdf_parser_raptor_stream_context* scontext = (librdf_parser_raptor_stream_context*)userdata;
  size_t len = size * nmemb;
  int rc;

  rc = raptor_parser_parse_chunk(scontext->pcontext->rdf_parser,
                                 (const unsigned char*)ptr, len, 0);

  if(rc)
    raptor_www_abort(www, "Parsing failed");
}


/**
 * librdf_parser_raptor_parse_as_stream_common:
 * @context: parser context
 * @uri: #librdf_uri URI of RDF content source
 * @string: or content string
 * @length: length of the string or 0 if not yet counted
 * @iostream: an iostream to parse
 * @base_uri: #librdf_uri URI of the content location or NULL if the same
 *
 * Retrieve the content at URI/string and parse it into a #librdf_stream.
 *
 * Precisely one of uri, string, and iostream must be non-NULL
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_as_stream_common(void *context, librdf_uri *uri,
                                            const unsigned char *string,
                                            size_t length,
                                            raptor_iostream *iostream,
                                            librdf_uri *base_uri)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  librdf_parser_raptor_stream_context* scontext;
  librdf_stream *stream;
  int need_base_uri;
  int status;
  const raptor_syntax_description *desc;

  if(!base_uri && uri)
    base_uri=uri;

  desc = raptor_parser_get_description(pcontext->rdf_parser);
  if(!desc) {
    librdf_log(pcontext->parser->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Could not get description for %s parser",
               pcontext->parser_name);
    return NULL;
  }
  need_base_uri = desc->flags & RAPTOR_SYNTAX_NEED_BASE_URI;

  if(need_base_uri && !base_uri) {
    librdf_log(pcontext->parser->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Missing base URI for %s parser",
               pcontext->parser_name);

    return NULL;
  }


  pcontext->errors=0;
  pcontext->warnings=0;

  if(uri && librdf_uri_is_file_uri(uri)) {
    char* filename=(char*)librdf_uri_to_filename(uri);
    FILE *fh;

    if(!filename)
      return NULL;

    fh=fopen(filename, "r");
    if(!fh) {
      librdf_log(pcontext->parser->world, 0, LIBRDF_LOG_ERROR,
                 LIBRDF_FROM_PARSER, NULL, "failed to open file '%s' - %s",
                 filename, strerror(errno));
      SYSTEM_FREE(filename);
      return NULL;
    }

    /* stream will close FH */
    stream=librdf_parser_raptor_parse_file_handle_as_stream(context, fh, 1,
                                                            base_uri);

    SYSTEM_FREE(filename);
    return stream;
  }


  scontext = LIBRDF_CALLOC(librdf_parser_raptor_stream_context*, 1,
                           sizeof(*scontext));
  if(!scontext)
    goto oom;

  scontext->pcontext=pcontext;
  pcontext->stream_context=scontext;

  scontext->statements=librdf_new_list(pcontext->parser->world);
  if(!scontext->statements)
    goto oom;

  if(pcontext->nspace_prefixes)
    raptor_free_sequence(pcontext->nspace_prefixes);
  pcontext->nspace_prefixes=raptor_new_sequence(free, NULL);
  if(!pcontext->nspace_prefixes)
    goto oom;

  if(pcontext->nspace_uris)
    raptor_free_sequence(pcontext->nspace_uris);

  pcontext->nspace_uris = raptor_new_sequence((raptor_data_free_handler)librdf_free_uri, NULL);
  if(!pcontext->nspace_uris)
    goto oom;

  raptor_parser_set_statement_handler(pcontext->rdf_parser, scontext,
                                      librdf_parser_raptor_new_statement_handler);
  raptor_parser_set_namespace_handler(pcontext->rdf_parser, pcontext,
                                      librdf_parser_raptor_namespace_handler);

  if(pcontext->parser->uri_filter)
    raptor_parser_set_uri_filter(pcontext->rdf_parser,
                                 librdf_parser_raptor_relay_filter,
                                 pcontext->parser);

  if(uri) {
    const char *accept_h;

    if(pcontext->www)
      raptor_free_www(pcontext->www);

    pcontext->www = raptor_new_www(pcontext->parser->world->raptor_world_ptr);
    if(!pcontext->www)
      goto oom;

    accept_h=raptor_parser_get_accept_header(pcontext->rdf_parser);
    if(accept_h) {
      raptor_www_set_http_accept(pcontext->www, accept_h);
      raptor_free_memory((void*)accept_h);
    }

    raptor_www_set_write_bytes_handler(pcontext->www,
                                       librdf_parser_raptor_parse_uri_as_stream_write_bytes_handler,
                                       scontext);

    status = raptor_parser_parse_start(pcontext->rdf_parser, (raptor_uri*)base_uri);
    if(status) {
      raptor_free_www(pcontext->www);

      pcontext->www = NULL;
      librdf_parser_raptor_serialise_finished((void*)scontext);
      return NULL;
    }

    raptor_www_fetch(pcontext->www, (raptor_uri*)uri);
    raptor_parser_parse_chunk(pcontext->rdf_parser, NULL, 0, 1);

    raptor_free_www(pcontext->www);

    pcontext->www = NULL;
  } else if (string) {
    status = raptor_parser_parse_start(pcontext->rdf_parser,
                                       (raptor_uri*)base_uri);
    if(status) {
      librdf_parser_raptor_serialise_finished((void*)scontext);
      return NULL;
    }

    if(!length)
      length = strlen((const char*)string);

    raptor_parser_parse_chunk(pcontext->rdf_parser, string, length, 1);
  } else if (iostream) {
    status = raptor_parser_parse_start(pcontext->rdf_parser, (raptor_uri*)base_uri);
    if(status) {
      librdf_parser_raptor_serialise_finished((void*)scontext);
      return NULL;
    }

    status = raptor_parser_parse_iostream(pcontext->rdf_parser,
                                          iostream,
                                          (raptor_uri*)base_uri);
    if(status) {
      librdf_parser_raptor_serialise_finished((void*)scontext);
      return NULL;
    }
  } else {
    /* All three of URI, string and iostream are null.  That's a coding error. */
    librdf_parser_raptor_serialise_finished((void*)scontext);
    return NULL;
  }


  /* get first statement, else is empty */
  scontext->current=(librdf_statement*)librdf_list_pop(scontext->statements);

  stream=librdf_new_stream(pcontext->parser->world,
                           (void*)scontext,
                           &librdf_parser_raptor_serialise_end_of_stream,
                           &librdf_parser_raptor_serialise_next_statement,
                           &librdf_parser_raptor_serialise_get_statement,
                           &librdf_parser_raptor_serialise_finished);
  if(!stream)
    goto oom;

  return stream;

  /* Clean up and report an error on OOM */
  oom:
  librdf_parser_raptor_serialise_finished((void*)scontext);
  librdf_log(pcontext->parser->world,
             0, LIBRDF_LOG_FATAL, LIBRDF_FROM_PARSER, NULL,
             "Out of memory");
  return NULL;
}


/**
 * librdf_parser_raptor_parse_uri_as_stream:
 * @context: parser context
 * @uri: #librdf_uri URI of content source
 * @base_uri: #librdf_uri URI of the content location or NULL if the same
 *
 * Retrieve the content at URI and parse it into a librdf_stream.
 *
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_uri_as_stream(void *context, librdf_uri *uri,
                                         librdf_uri *base_uri)
{
  return librdf_parser_raptor_parse_as_stream_common(context, uri,
                                                     NULL, 0,
                                                     0,
                                                     base_uri);
}


/**
 * librdf_parser_raptor_parse_string_as_stream:
 * @context: parser context
 * @string: string content to parse
 * @base_uri: #librdf_uri URI of the content location or NULL if the same
 *
 * Parse the content in a string and return a librdf_stream.
 *
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_string_as_stream(void *context,
                                            const unsigned char *string,
                                            librdf_uri *base_uri)
{
  return librdf_parser_raptor_parse_as_stream_common(context, NULL,
                                                     string, 0,
                                                     0,
                                                     base_uri);
}


/**
 * librdf_parser_raptor_parse_counted_string_as_stream:
 * @context: parser context
 * @string: string content to parse
 * @length: length of the string content (must be >0)
 * @base_uri: the base URI to use
 *
 * Parse a counted string of content to a librdf_stream of statements.
 *
 * Return value: #librdf_stream of statements or NULL
 **/
static librdf_stream*
librdf_parser_raptor_parse_counted_string_as_stream(void *context,
                                                    const unsigned char *string,
                                                    size_t length,
                                                    librdf_uri* base_uri)
{
  return librdf_parser_raptor_parse_as_stream_common(context, NULL,
                                                     string, length,
                                                     0,
                                                     base_uri);
}

/**
 * librdf_parser_raptor_parse_iostream_as_stream:
 * @context: parser context
 * @iostream: iostream content to parse
 * @base_uri: #librdf_uri URI of the content location or NULL if the same
 *
 * Parse the content in an iostream and return a librdf_stream.
 *
 *
 **/
static librdf_stream*
librdf_parser_raptor_parse_iostream_as_stream(void *context,
                                              raptor_iostream *iostream,
                                              librdf_uri *base_uri)
{
  return librdf_parser_raptor_parse_as_stream_common(context, NULL,
                                                     0, 0,
                                                     iostream,
                                                     base_uri);
}


/*
 * librdf_parser_raptor_parse_into_model_common:
 * @context: parser context
 * @uri: #librdf_uri URI of RDF content source, or NULL
 * @string: string content to parser, or NULL
 * @length: length of the string or 0 if not yet counted
 * @fh: FILE* content source, or NULL
 * @iostream: iostream content, or NULL
 * @base_uri: #librdf_uri URI of the content location or NULL
 * @model: #librdf_model of model
 *
 * Retrieve the RDF content at URI and store it into a librdf_model.
 *
 * Precisely one of uri, string, fh or iostream must be non-null.
 *
 * Parses the content at @uri, @string, @fh or @iostream and store it in the given model.
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_raptor_parse_into_model_common(void *context,
                                             librdf_uri *uri,
                                             const unsigned char *string,
                                             size_t length,
                                             FILE *fh,
                                             raptor_iostream *iostream,
                                             librdf_uri *base_uri,
                                             librdf_model* model)
{
  int status=0;
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  librdf_parser_raptor_stream_context* scontext;
  int need_base_uri;
  const raptor_syntax_description *desc;

  if(!base_uri)
    base_uri=uri;

  desc = raptor_parser_get_description(pcontext->rdf_parser);
  if(!desc) {
    librdf_log(pcontext->parser->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Could not get description for %s parser",
               pcontext->parser_name);
    return -1;
  }
  need_base_uri = desc->flags & RAPTOR_SYNTAX_NEED_BASE_URI;

  if(need_base_uri && !base_uri) {
    librdf_log(pcontext->parser->world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Missing base URI for %s parser",
               pcontext->parser_name);

    return 1;
  }

  pcontext->errors=0;
  pcontext->warnings=0;

  scontext = LIBRDF_CALLOC(librdf_parser_raptor_stream_context*, 1,
                           sizeof(*scontext));
  if(!scontext)
    goto oom;

  scontext->pcontext=pcontext;
  pcontext->stream_context=scontext;

  if(pcontext->nspace_prefixes)
    raptor_free_sequence(pcontext->nspace_prefixes);
  pcontext->nspace_prefixes=raptor_new_sequence(free, NULL);
  if(!pcontext->nspace_prefixes)
    goto oom;

  if(pcontext->nspace_uris)
    raptor_free_sequence(pcontext->nspace_uris);

  pcontext->nspace_uris = raptor_new_sequence((raptor_data_free_handler)librdf_free_uri, NULL);
  if(!pcontext->nspace_uris)
    goto oom;

  raptor_parser_set_statement_handler(pcontext->rdf_parser, scontext,
                                      librdf_parser_raptor_new_statement_handler);
  raptor_parser_set_namespace_handler(pcontext->rdf_parser, pcontext,
                                      librdf_parser_raptor_namespace_handler);

  /* direct into model */
  scontext->model=model;

  if(pcontext->parser->uri_filter)
    raptor_parser_set_uri_filter(pcontext->rdf_parser,
                                 librdf_parser_raptor_relay_filter,
                                 pcontext->parser);

  if(uri) {
    status = raptor_parser_parse_uri(pcontext->rdf_parser, (raptor_uri*)uri,
                                     (raptor_uri*)base_uri);
  } else if (string != NULL) {
    status = raptor_parser_parse_start(pcontext->rdf_parser, (raptor_uri*)base_uri);
    if(!status) {
      if(!length)
        length = strlen((const char*)string);
      status = raptor_parser_parse_chunk(pcontext->rdf_parser, string, length, 1);
    }
  } else if(fh) {
    status = raptor_parser_parse_file_stream(pcontext->rdf_parser, fh, NULL, (raptor_uri*)base_uri);
  } else if(iostream) {
    status = raptor_parser_parse_iostream(pcontext->rdf_parser, iostream,  (raptor_uri*)base_uri);
  } else {
    /* All four of URI, string, fh and iostream are null.  That's a coding error. */
    status = -1;
  }

  librdf_parser_raptor_serialise_finished((void*)scontext);

  return status;

  /* Clean up and report an error on OOM */
  oom:
  librdf_parser_raptor_serialise_finished((void*)scontext);
  librdf_log(pcontext->parser->world,
             0, LIBRDF_LOG_FATAL, LIBRDF_FROM_PARSER, NULL,
             "Out of memory");
  return -1;
}


/**
 * librdf_parser_raptor_parse_uri_into_model:
 * @context: parser context
 * @uri: #librdf_uri URI of RDF content source
 * @base_uri: #librdf_uri URI of the content location (or NULL if the same as @uri)
 * @model: #librdf_model of model
 *
 * Retrieve the RDF content at URI and store it into a librdf_model.
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
  return librdf_parser_raptor_parse_into_model_common(context, uri,
                                                      NULL, 0, NULL, NULL,
                                                      base_uri, model);}


/**
 * librdf_parser_raptor_parse_string_into_model:
 * @context: parser context
 * @string: content to parse
 * @base_uri: #librdf_uri URI of the content location
 * @model: #librdf_model of model
 *
 * Parse the RDF content in a string and store it into a librdf_model.
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
  return librdf_parser_raptor_parse_into_model_common(context, NULL,
                                                      string, 0, NULL, NULL,
                                                      base_uri, model);
}

/**
 * librdf_parser_raptor_parse_file_handle_into_model:
 * @context: parser context
 * @fh: FILE* of content source
 * @close_fh: non-0 to fclose(fh) on finish
 * @base_uri: #librdf_uri URI of the content location (or NULL)
 * @model: #librdf_model of model
 *
 * INTERNAL - Retrieve the RDF content from a FILE* handle and store it into a #librdf_model.
 *
 * Retrieves all statements and stores them in the given model.
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_raptor_parse_file_handle_into_model(void *context, FILE *fh,
                                                  int close_fh,
                                                  librdf_uri *base_uri,
                                                  librdf_model* model)
{
  int status=librdf_parser_raptor_parse_into_model_common(context, NULL,
                                                          NULL, 0, fh, NULL,
                                                          base_uri, model);

  if (close_fh)
    fclose(fh);

  return status;
}


/**
 * librdf_parser_raptor_parse_counted_string_into_model:
 * @context: parser context
 * @string: the content to parse
 * @length: length of content (must be >0)
 * @base_uri: the base URI to use
 * @model: the model to use
 *
 * INTERNAL - Parse a counted string of content into an #librdf_model.
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_raptor_parse_counted_string_into_model(void *context,
                                                     const unsigned char *string,
                                                     size_t length,
                                                     librdf_uri* base_uri,
                                                     librdf_model* model)
{
  return librdf_parser_raptor_parse_into_model_common(context, NULL,
                                                      string, length, NULL, NULL,
                                                      base_uri, model);
}


/**
 * librdf_parser_raptor_parse_iostream_into_model:
 * @context: parser context
 * @iostream: the content to parse
 * @base_uri: the base URI to use
 * @model: the model to use
 *
 * INTERNAL - Parse an iostream of content into an #librdf_model.
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_raptor_parse_iostream_into_model(void *context,
                                               raptor_iostream *iostream,
                                               librdf_uri* base_uri,
                                               librdf_model* model)
{
  return librdf_parser_raptor_parse_into_model_common(context, NULL,
                                                      NULL, 0, NULL, iostream,
                                                      base_uri, model);
}


/**
 * librdf_parser_raptor_serialise_end_of_stream:
 * @context: the context passed in by #librdf_stream
 *
 * Check for the end of the stream of statements from the raptor RDF parser.
 *
 * Return value: non 0 at end of stream
 **/
static int
librdf_parser_raptor_serialise_end_of_stream(void* context)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;

  return (!scontext->current && !librdf_list_size(scontext->statements));
}


/**
 * librdf_parser_raptor_serialise_next_statement:
 * @context: the context passed in by #librdf_stream
 *
 * Move to the next librdf_statement in the stream of statements from the raptor RDF parse.
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
    scontext->current=(librdf_statement*)librdf_list_pop(scontext->statements);
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
 * librdf_parser_raptor_serialise_get_statement:
 * @context: the context passed in by #librdf_stream
 * @flags: the context get method flags
 *
 * Get the current librdf_statement from the stream of statements from the raptor RDF parse.
 *
 * Return value: a new #librdf_statement or NULL on error or if no statements found.
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
      librdf_log(scontext->pcontext->parser->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
                 "Unknown iterator method flag %d", flags);
      return NULL;
  }

}


/**
 * librdf_parser_raptor_serialise_finished:
 * @context: the context passed in by #librdf_stream
 *
 * Finish the serialisation of the statement stream from the raptor RDF parse.
 *
 **/
static void
librdf_parser_raptor_serialise_finished(void* context)
{
  librdf_parser_raptor_stream_context* scontext=(librdf_parser_raptor_stream_context*)context;

  if(scontext) {
    librdf_statement* statement;
    librdf_world* world = scontext->pcontext->parser ? scontext->pcontext->parser->world : NULL;

    if(scontext->current)
      librdf_free_statement(scontext->current);

    if(scontext->statements) {
      while((statement=(librdf_statement*)librdf_list_pop(scontext->statements)))
        librdf_free_statement(statement);
      librdf_free_list(scontext->statements);
    }

    if(scontext->fh && scontext->close_fh)
      fclose(scontext->fh);

    if(scontext->pcontext)
      scontext->pcontext->stream_context = NULL;

    librdf_raptor_reset_bnode_hash(world);

    LIBRDF_FREE(librdf_parser_raptor_context, scontext);
  }
}


static librdf_node*
librdf_parser_raptor_get_feature(void* context, librdf_uri *feature)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  unsigned char intbuffer[20]; /* FIXME */
  unsigned char *uri_string;

  if(!feature)
    return NULL;

  uri_string=librdf_uri_as_string(feature);
  if(!uri_string)
    return NULL;

  if(!strcmp((const char*)uri_string, LIBRDF_PARSER_FEATURE_ERROR_COUNT)) {
    sprintf((char*)intbuffer, "%d", pcontext->errors);
    return librdf_new_node_from_typed_literal(pcontext->parser->world,
                                              intbuffer, NULL, NULL);
  } else if(!strcmp((const char*)uri_string, LIBRDF_PARSER_FEATURE_WARNING_COUNT)) {
    sprintf((char*)intbuffer, "%d", pcontext->warnings);
    return librdf_new_node_from_typed_literal(pcontext->parser->world,
                                              intbuffer, NULL, NULL);
  } else {
    /* raptor2: try a raptor option */
    raptor_option feature_i;
    feature_i = raptor_world_get_option_from_uri(pcontext->parser->world->raptor_world_ptr, (raptor_uri*)feature);

    if((int)feature_i >= 0) {
      int value;

      raptor_parser_get_option(pcontext->rdf_parser, feature_i, NULL, &value);
      sprintf((char*)intbuffer, "%d", value);
      return librdf_new_node_from_typed_literal(pcontext->parser->world,
                                                intbuffer, NULL, NULL);
    }
  }

  return NULL;
}


static int
librdf_parser_raptor_set_feature(void* context,
                                 librdf_uri *feature, librdf_node *value)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;
  raptor_option feature_i;
  const unsigned char* value_s;

  if(!feature)
    return 1;

  /* try a raptor feature */
  feature_i = raptor_world_get_option_from_uri(pcontext->parser->world->raptor_world_ptr, (raptor_uri*)feature);
  if((int)feature_i < 0)
    return 1;

  if(!librdf_node_is_literal(value))
    return 1;

  value_s=(const unsigned char*)librdf_node_get_literal_value(value);

  return raptor_parser_set_option(pcontext->rdf_parser, feature_i,
                                  (const char *)value_s, 0);
}


static char*
librdf_parser_raptor_get_accept_header(void* context)
{
  librdf_parser_raptor_context* pcontext = (librdf_parser_raptor_context*)context;
  const char* accept;
  char* r_accept;
  size_t length;

  accept = raptor_parser_get_accept_header(pcontext->rdf_parser);
  if(!accept)
    return NULL;
  length = strlen(accept);
  r_accept = (char *)librdf_alloc_memory(length + 1);
  strncpy(r_accept, accept, length+1);
  raptor_free_memory((void*)accept);

  return r_accept;
}


static const char*
librdf_parser_raptor_get_namespaces_seen_prefix(void* context, int offset)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;

  if(!pcontext->nspace_prefixes)
    return NULL;

  if(offset < 0 || offset > raptor_sequence_size(pcontext->nspace_prefixes))
    return NULL;

  return (const char*)raptor_sequence_get_at(pcontext->nspace_prefixes, offset);
}


static librdf_uri*
librdf_parser_raptor_get_namespaces_seen_uri(void* context, int offset)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;

  if(!pcontext->nspace_uris)
    return NULL;

  if(offset < 0 || offset > raptor_sequence_size(pcontext->nspace_uris))
    return NULL;

  return (librdf_uri*)raptor_sequence_get_at(pcontext->nspace_uris, offset);
}


static int
librdf_parser_raptor_get_namespaces_seen_count(void* context)
{
  librdf_parser_raptor_context* pcontext=(librdf_parser_raptor_context*)context;

  if(!pcontext->nspace_uris)
    return 0;

  return raptor_sequence_size(pcontext->nspace_uris);
}





/**
 * librdf_parser_raptor_register_factory:
 * @factory: factory
 *
 * Register the raptor RDF parser with the RDF parser factory.
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
  factory->parse_counted_string_as_stream = librdf_parser_raptor_parse_counted_string_as_stream;
  factory->parse_counted_string_into_model = librdf_parser_raptor_parse_counted_string_into_model;
  factory->parse_iostream_as_stream = librdf_parser_raptor_parse_iostream_as_stream;
  factory->parse_iostream_into_model = librdf_parser_raptor_parse_iostream_into_model;
  factory->parse_file_handle_as_stream = librdf_parser_raptor_parse_file_handle_as_stream;
  factory->parse_file_handle_into_model = librdf_parser_raptor_parse_file_handle_into_model;
  factory->get_feature = librdf_parser_raptor_get_feature;
  factory->set_feature = librdf_parser_raptor_set_feature;
  factory->get_accept_header = librdf_parser_raptor_get_accept_header;
  factory->get_namespaces_seen_prefix = librdf_parser_raptor_get_namespaces_seen_prefix;
  factory->get_namespaces_seen_uri = librdf_parser_raptor_get_namespaces_seen_uri;
  factory->get_namespaces_seen_count = librdf_parser_raptor_get_namespaces_seen_count;
}


/**
 * librdf_parser_raptor_constructor:
 * @world: redland world object
 *
 * Initialise the raptor RDF parser module.
 *
 **/
void
librdf_parser_raptor_constructor(librdf_world *world)
{
  unsigned int i;

  /* enumerate from parser 1, so the default parser 0 is done last */
  for(i = 1; 1; i++) {
    const char *syntax_name = NULL;
    const char *syntax_label = NULL;
    const char *mime_type = NULL;
    const unsigned char *uri_string = NULL;
    const raptor_syntax_description *desc;

    desc = raptor_world_get_parser_description(world->raptor_world_ptr, i);

    if(!desc) {
      /* reached the end of the parsers, now register the default one */
      i = 0;
      desc = raptor_world_get_parser_description(world->raptor_world_ptr, i);
      if(!desc) {
        librdf_log(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
                   "Failed to find any Raptor parsers - Raptor may not be initialised correctly");
        break;
      }
    }

    syntax_name = desc->names[0];
    syntax_label = desc->label;
    if(desc->mime_types)
      mime_type = desc->mime_types[0].mime_type;
    if(desc->uri_strings)
      uri_string = (const unsigned char *)desc->uri_strings[0];

    if(!strcmp(syntax_name, "rdfxml")) {
      /* legacy name - see librdf_parser_raptor_init */
      librdf_parser_register_factory(world, "raptor", NULL,
                                     mime_type, uri_string,
                                     &librdf_parser_raptor_register_factory);
    }

    librdf_parser_register_factory(world, syntax_name, syntax_label,
                                   mime_type, uri_string,
                                   &librdf_parser_raptor_register_factory);

    if(!i) /* registered default parser, end */
      break;
  }
}


/**
 * librdf_parser_raptor_destructor:
 * @world: redland world object
 *
 * Terminate the raptor RDF parser module.
 *
 **/
void
librdf_parser_raptor_destructor(void)
{
}
