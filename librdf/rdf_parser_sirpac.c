/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser SiRPAC (via pipe) implementation
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



/* FIXME: Yeah?  What about it? */
#define LINE_BUFFER_LEN 1024



/* serialising implementing functions */
static int librdf_parser_sirpac_serialise_end_of_stream(void* context);
static librdf_statement* librdf_parser_sirpac_serialise_next_statement(void* context);
static void librdf_parser_sirpac_serialise_finished(void* context);


/*
 * SiRPAC parser context- not used at present
 */
typedef struct {
  char dummy;
} librdf_parser_sirpac_context;


/*
 * context used for the SiRPAC parser to create &librdf_stream
 * of statements from parsing the output of the java command
 */
typedef struct {
  librdf_uri* uri;          /* source */
  char *command;            /* command invoked ... */
  FILE *fh;                 /* file handle to pipe to above command */
  librdf_statement* next;   /* next statement */
  int end_of_stream;        /* non 0 if stream finished */
} librdf_parser_sirpac_stream_context;



static int
librdf_parser_sirparc_init(void *context) {
  /* always succeeds ? */
  return 0;
}
  

/**
 * librdf_parser_sirpac_get_next_statement
 * @context: serialisation context
 * @uri: URI of RDF content
 * 
 * Parse the RDF content at the given URI and return it as a stream of
 * &librdf_statement objects.
 * 
 * Return value: a new &librdf_stream or NULL if the parse failed.
 **/
static librdf_stream*
librdf_parser_sirparc_parse_from_uri(void *context, librdf_uri *uri) {
  /* Note: not yet used */
/*  librdf_parser_sirpac_context* pcontext=(librdf_parser_sirpac_context*)context; */
  librdf_parser_sirpac_stream_context* scontext;
  librdf_stream* stream;
  int command_len;
  char *command;
  FILE *fh;
  static const char *command_format_string="%s -classpath %s org.w3c.rdf.examples.ListStatements %s 2>&1";
  char *uri_string;

  scontext=(librdf_parser_sirpac_stream_context*)LIBRDF_CALLOC(librdf_parser_sirpac_stream_context, 1, sizeof(librdf_parser_sirpac_stream_context));
  if(!scontext)
    return NULL;

  scontext->uri=uri;
  
  uri_string=librdf_uri_as_string(uri);

  /* strlen(format_string) is 3 chars too long (%s * 3) and 1 char
   * too short (\0 at end of new string), so take 2 chars off length 
   */
  command_len=strlen(command_format_string) + 
              strlen(JAVA_COMMAND) +
              strlen(RDF_JAVA_API_JAR) +
              strlen(uri_string) -2;

  command=(char*)LIBRDF_MALLOC(cstring, command_len);
  if(!command) {
    librdf_parser_sirpac_serialise_finished((void*)context);
    return NULL;
  }
  sprintf(command, command_format_string, JAVA_COMMAND, RDF_JAVA_API_JAR, 
	  uri_string);
  scontext->command=command;

  LIBRDF_DEBUG2(librdf_parser_sirparc_parse_from_uri, "Running command '%s'\n", command);

  fh=popen(command, "r");
  if(!fh) {
    LIBRDF_DEBUG2(librdf_new_parser_sirpac, "Failed to create pipe to '%s'", command);
    librdf_parser_sirpac_serialise_finished((void*)context);
    return(NULL);
  }
  scontext->fh=fh;
  

  stream=librdf_new_stream((void*)scontext,
                           &librdf_parser_sirpac_serialise_end_of_stream,
                           &librdf_parser_sirpac_serialise_next_statement,
                           &librdf_parser_sirpac_serialise_finished);
  if(!stream) {
    librdf_parser_sirpac_serialise_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}
  

/**
 * librdf_parser_sirpac_get_next_statement
 * @context: serialisation context
 * 
 * Decode the output of the Java command to get the next statement
 * 
 * Return value: the next &librdf_statement or NULL at end of stream.
 **/
static librdf_statement*
librdf_parser_sirpac_get_next_statement(librdf_parser_sirpac_stream_context *context) {
  librdf_statement* statement=NULL;
  char buffer[LINE_BUFFER_LEN];
  
  /* SiRPAC format:
  triple("URL-of-predicate","URL-of-subject","URI-of-object/literal-string").

  Rdf-api-java (org.w3c.rdf.examples.ListStatements) format:

  Statement triple("URL-of-subject", "URL-of-predicate", literal("string")) has digest URI uuid:rdf:SHA-1-6a3288e73db731bee798bf039f22a74dd7366888
    OR
  Statement triple("URL-of-subject", "URL-of-predicate", "URL-of-object") has digest URI uuid:rdf:SHA-1-6a3288e73db731bee798bf039f22a74dd7366888

  */


  while(!feof(context->fh)) {
    char *p, *s, *o, *e;
    int object_is_literal=0;
    
    statement=NULL;

    if(!fgets(buffer, LINE_BUFFER_LEN, context->fh))
      continue;

    if(!(s=strstr(buffer, "triple(\"")))
      continue;

    /* subject found after >>triple("<< */
    s+=8;

    if(!(p=strstr(s, "\", \"")))
      continue;
    *p='\0';
    p+=4;
    
    /* predicate found after >>", "<< */

    if(!(o=strstr(p, "\", ")))
      continue;
    *o='\0';
    o+=3;

    /* object found after >>", << (with optional >>"<< if resource object)*/
    if(*o == '"')
      o++;


    if(!(e=strstr(o, "\")")))
      continue;
    /* zap end */
    *e='\0';

    if(!strncmp(o, "literal(\"", 9)) {
      o+=9;
      object_is_literal=1;
    }


    /* got all statement parts now */
    statement=librdf_new_statement();
    if(!statement)
      break;
    
    librdf_statement_set_subject(statement, 
                                 librdf_new_node_from_uri_string(s));

    librdf_statement_set_predicate(statement,
                                   librdf_new_node_from_uri_string(p));

    if(object_is_literal) {
      librdf_statement_set_object(statement,
                                  librdf_new_node_from_literal(o, NULL, 0, 0));
    } else {
      librdf_statement_set_object(statement, 
                                  librdf_new_node_from_uri_string(o));
    }

    /* found a statement, return it */
    break;
  }

  return statement;
}


/**
 * librdf_parser_sirpac_serialise_end_of_stream:
 * @context: serialisation context
 * 
 * Test for the end of the stream of statements from the RDF SiRPAC
 * parser serialisation.
 * 
 * Return value: non 0 at end of stream.
 **/
static int
librdf_parser_sirpac_serialise_end_of_stream(void* context)
{
  librdf_parser_sirpac_stream_context* scontext=(librdf_parser_sirpac_stream_context*)context;

  if(scontext->end_of_stream)
    return 1;

  /* already have 1 stored item - not end yet */
  if(scontext->next)
    return 0;

  scontext->next=librdf_parser_sirpac_get_next_statement(scontext);
  if(!scontext->next)
    scontext->end_of_stream=1;

  return (scontext->next == NULL);
}


/**
 * librdf_parser_sirpac_serialise_next_statement:
 * @context: serialisation context
 * 
 * Get next statement in RDF SiRPAC &librdf_stream serialisation
 * 
 * Return value: next &librdf_statement or NULL at end of stream.
 **/
static librdf_statement*
librdf_parser_sirpac_serialise_next_statement(void* context)
{
  librdf_parser_sirpac_stream_context* scontext=(librdf_parser_sirpac_stream_context*)context;
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
  scontext->next=librdf_parser_sirpac_get_next_statement(scontext);
  if(!scontext->next)
    scontext->end_of_stream=1;

  return scontext->next;
}


/**
 * librdf_parser_sirpac_serialise_finished:
 * @context: serialisation context
 * 
 * Finish SiRPAC RDF parser &librdf_stream serialisation
 **/
static void
librdf_parser_sirpac_serialise_finished(void* context)
{
  librdf_parser_sirpac_stream_context* scontext=(librdf_parser_sirpac_stream_context*)context;

  if(scontext) {
    if(scontext->fh) {
      char buffer[LINE_BUFFER_LEN];
      /* throw away any remaining data, to prevent EPIPE signal */
      while(!feof(scontext->fh)) {
	fgets(buffer, LINE_BUFFER_LEN, scontext->fh);
      }
      fclose(scontext->fh);
      scontext->fh=NULL;
    }

    if(scontext->command)
      LIBRDF_FREE(cstring, scontext->command);

    LIBRDF_FREE(librdf_parser_sirpac_context, scontext);
  }
}


/**
 * librdf_parser_sirpac_register_factory:
 * @factory: prototype rdf parser factory
 * 
 * Register the SiRPAC RDF parser with the RDF parser factory.
 **/
static void
librdf_parser_sirpac_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_sirpac_context);
  
  factory->init  = librdf_parser_sirparc_init;
  factory->parse_from_uri = librdf_parser_sirparc_parse_from_uri;
}


/**
 * librdf_parser_sirpac_constructor:
 * @void: 
 * 
 * Initialise the SiRPAC RDF parser module
 **/
void
librdf_parser_sirpac_constructor(void)
{
  librdf_parser_register_factory("SiRPAC", &librdf_parser_sirpac_register_factory);
}
