/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser SiRPAC (via pipe) implementation
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

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#include <librdf.h>

#include <rdf_parser.h>
#include <rdf_node.h>
#include <rdf_stream.h>
#include <rdf_statement.h>
#include <rdf_concepts.h>



/* FIXME: Yeah?  What about it? */
#define LINE_BUFFER_LEN 1024



/* serialising implementing functions */
static int librdf_parser_sirpac_serialise_end_of_stream(void* context);
static librdf_statement* librdf_parser_sirpac_serialise_next_statement(void* context);
static void librdf_parser_sirpac_serialise_finished(void* context);


/*
 * SiRPAC parser context
 */
typedef struct {
  librdf_parser *parser;
  int sirpac_flavour;                /* 0 = W3C 1 = Stanford */
  const char *command_format_string; /* command to run with 2-%s for arg, URI */
  int feature_aboutEach;             /* non 0 if rdf:aboutEach supported */
  int feature_aboutEachPrefix;       /* non 0 if rdf:aboutEachPrefix supported */
} librdf_parser_sirpac_context;


/*
 * context used for the SiRPAC parser to create &librdf_stream
 * of statements from parsing the output of the java command
 */
typedef struct {
  librdf_parser_sirpac_context *pcontext;
  librdf_uri* uri;          /* source */
  librdf_uri* base_uri;     /* base URI */
  char *command;            /* command invoked ... */
  FILE *fh;                 /* file handle to pipe to above command */
  librdf_statement* next;   /* next statement */
  int end_of_stream;        /* non 0 if stream finished */
} librdf_parser_sirpac_stream_context;



#ifdef JAVA_SIRPACSTANFORD_JAR
#ifdef JAVA_SAX_JAR
static const char *librdf_parser_sirpac_stanford_command_format_string= JAVA_COMMAND " -classpath " JAVA_CLASS_DIR ":" JAVA_SIRPACSTANFORD_JAR ":" JAVA_SAX_JAR" -Dorg.xml.sax.parser=" JAVA_SAX_CLASS " PrintParser %s %s";
#else
static const char *librdf_parser_sirpac_stanford_command_format_string= JAVA_COMMAND " -classpath " JAVA_CLASS_DIR ":" JAVA_SIRPACSTANFORD_JAR " -Dorg.xml.sax.parser=" JAVA_SAX_CLASS " PrintParser %s %s";
#endif

/**
 * librdf_parser_sirpac_stanford_init - Initialise the SiRPAC (Stanford) RDF parser
 * @parser: the parser
 * @context: context
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_sirpac_stanford_init(librdf_parser* parser, void *context) 
{
  librdf_parser_sirpac_context* pcontext=(librdf_parser_sirpac_context*)context;
  pcontext->parser = parser;
  pcontext->sirpac_flavour=1;
  pcontext->command_format_string=librdf_parser_sirpac_stanford_command_format_string;
  return 0;
}

#endif


#ifdef JAVA_SIRPACW3C_JAR
static const char *librdf_parser_sirpac_w3c_command_format_string=JAVA_COMMAND " -classpath " JAVA_CLASS_DIR ":" JAVA_SIRPACW3C_JAR ":" JAVA_SAX_JAR " -Dorg.xml.sax.parser=" JAVA_SAX_CLASS " PrintParser %s %s";

/**
 * librdf_parser_sirpac_w3c_init - Initialise the SiRPAC (W3C) RDF parser
 * @parser: the parser
 * @context: context
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_sirpac_w3c_init(librdf_parser* parser, void *context) 
{
  librdf_parser_sirpac_context* pcontext=(librdf_parser_sirpac_context*)context;
  pcontext->parser = parser;
  pcontext->sirpac_flavour=0;
  pcontext->command_format_string=librdf_parser_sirpac_w3c_command_format_string;
  return 0;
}
#endif
  

/**
 * librdf_parser_sirpac_parse_uri_as_stream - Retrieve the RDF/XML content at URI and parse it into a librdf_stream
 * @context: serialisation context
 * @uri: URI of RDF content
 * @base_uri: the base URI to use (or NULL if the same)
 * 
 * Return value: a new &librdf_stream or NULL if the parse failed.
 **/
static librdf_stream*
librdf_parser_sirpac_parse_uri_as_stream(void *context, librdf_uri *uri,
                                         librdf_uri *base_uri) {
  librdf_parser_sirpac_context* pcontext=(librdf_parser_sirpac_context*)context;
  librdf_parser_sirpac_stream_context* scontext;
  librdf_stream* stream;
  int command_len;
  char *command;
  FILE *fh;
  char *uri_string;
  char *streaming_arg;

  scontext=(librdf_parser_sirpac_stream_context*)LIBRDF_CALLOC(librdf_parser_sirpac_stream_context, 1, sizeof(librdf_parser_sirpac_stream_context));
  if(!scontext)
    return NULL;
  
  scontext->pcontext=pcontext;

  scontext->uri=uri;
  scontext->base_uri=base_uri;
  
  uri_string=librdf_uri_as_string(uri);

  streaming_arg=(!scontext->pcontext->feature_aboutEach &&
                 !scontext->pcontext->feature_aboutEachPrefix) ?
                 "--streaming" : "--static";

  /* strlen(format_string) is 2 chars too long because 2x %s and 1 char
   * too short because of the \0 at the end of the new string, so take
   * 1 char off the length.
   */
  command_len=strlen(scontext->pcontext->command_format_string) +
              strlen(streaming_arg) + strlen(uri_string) -1;

  command=(char*)LIBRDF_MALLOC(cstring, command_len);
  if(!command) {
    librdf_parser_sirpac_serialise_finished((void*)context);
    return NULL;
  }
  
  /* can use streaming if aboutEach and aboutEachPrefix are not wanted */

  sprintf(command, scontext->pcontext->command_format_string, 
          streaming_arg, uri_string);
  scontext->command=command;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_parser_sirpac_parse_uri_as_stream, "Running command '%s'\n", command);
#endif

  fh=popen(command, "r");
  if(!fh) {
    librdf_parser_error(pcontext->parser,
                        "Failed to create pipe to SiRPAC command '%s'\n", command);
    librdf_parser_sirpac_serialise_finished((void*)scontext);
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
 * librdf_parser_sirpac_parse_uri_into_model - Retrieve the RDF/XML content at URI and store in a librdf_model
 * @context: serialisation context
 * @uri: URI of RDF content
 * @base_uri: the base URI to use (or NULL if the same)
 * @model: &librdf_model
 * 
 * FIXME: No error reporting provided 
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_sirpac_parse_uri_into_model(void *context, librdf_uri *uri,
                                          librdf_uri *base_uri, 
                                          librdf_model *model) {
  librdf_stream* stream;
  
  stream=librdf_parser_sirpac_parse_uri_as_stream(context,
                                                  uri, base_uri);
  if(!stream)
    return 1;

  return librdf_model_add_statements(model, stream);
}


/**
 * librdf_parser_sirpac_get_next_statement - helper function to decode the output of the Java command to get the next statement
 * @context: serialisation context
 * 
 * Return value: the next &librdf_statement or NULL at end of stream.
 */
static librdf_statement*
librdf_parser_sirpac_get_next_statement(librdf_parser_sirpac_stream_context *context) {
  librdf_statement* statement=NULL;
  char buffer[LINE_BUFFER_LEN];
  char *literal_buffer=NULL;
  int literal_buffer_length=0;
  
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

    if(!strncmp(o, "literal(\"", 9)) {
      o+=9;
      object_is_literal=1;
    }

    if(e=strstr(o, "\")")) {
      /* found end of object - terminate it */
      *e='\0';
    } else {
      /* Ignore statements with URI objects that dont live on one line */
      if(!object_is_literal)
        continue;

      /* otherwise build a multi-line literal */
      e=NULL;
      
      literal_buffer_length=strlen(o);
      literal_buffer=(char*)LIBRDF_MALLOC(cstring, literal_buffer_length+1);
      if(!literal_buffer) {
        o=NULL;
        break;
      }
      strcpy(literal_buffer, o);

      /* Read more lines for rest of literal */
      while(!feof(context->fh)) {
        char *p, *literal_buffer2;
        char literal_line_buffer[LINE_BUFFER_LEN];
        int len;
        
        if(!fgets(literal_line_buffer, LINE_BUFFER_LEN, context->fh))
          break;
        
        if(p=strstr(literal_line_buffer, "\")")) {
          len=(p-literal_line_buffer);
        } else {
          len=strlen(literal_line_buffer);
        }

        literal_buffer2=(char*)LIBRDF_MALLOC(cstring, 
                                             literal_buffer_length+len+1);
        if(!literal_buffer2) {
          LIBRDF_FREE(cstring, literal_buffer);
          literal_buffer=NULL;
          break;
        }
        strncpy(literal_buffer2,
                literal_buffer, literal_buffer_length);
        strncpy(literal_buffer2+literal_buffer_length, 
                literal_line_buffer, len);
        literal_buffer_length+=len;
        literal_buffer2[literal_buffer_length]='\0';
        LIBRDF_FREE(cstring, literal_buffer);

        literal_buffer=literal_buffer2;

        /* found end so stop */
        if(p)
          break;
      }
    }

    if(!o && !literal_buffer)
      break;

    /* got all statement parts now */
    statement=librdf_new_statement();
    if(!statement)
      break;
    
    librdf_statement_set_subject(statement, 
                                 librdf_new_node_from_uri_string(s));

    librdf_statement_set_predicate(statement,
                                   librdf_new_node_from_uri_string(p));

    if(object_is_literal) {
      if(literal_buffer)
        o=literal_buffer;
      librdf_statement_set_object(statement,
                                  librdf_new_node_from_literal(o, NULL, 0, 0));
      if(literal_buffer)
        LIBRDF_FREE(cstring, literal_buffer);
    } else {
      librdf_statement_set_object(statement, 
                                  librdf_new_node_from_uri_string(o));
    }

    /* found a statement, return it */
    break;
  }


  if(feof(context->fh)) {
    int status=(pclose(context->fh)& 0xff00)>>8;

    if(status)
      librdf_parser_error(context->pcontext->parser,
                          "SiRPAC command '%s' exited with status %d\n",
                          context->command, status);
    context->fh=NULL;
  }
  
  return statement;
}


/**
 * librdf_parser_sirpac_serialise_end_of_stream - Check for the end of the stream of statements from the Sirpac parser
 * @context: serialisation context
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
 * librdf_parser_sirpac_serialise_next_statement - Get the next librdf_statement from the stream of statements from the Sirpac RDF parse
 * @context: serialisation context
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
 * librdf_parser_sirpac_serialise_finished - Finish the serialisation of the statement stream from the SiRPAC RDF parse
 * @context: serialisation context
 **/
static void
librdf_parser_sirpac_serialise_finished(void* context)
{
  librdf_parser_sirpac_stream_context* scontext=(librdf_parser_sirpac_stream_context*)context;

  if(scontext) {
    if(scontext->fh) {
      char buffer[LINE_BUFFER_LEN];
      int status;
      
      /* throw away any remaining data, to prevent EPIPE signal */
      while(!feof(scontext->fh)) {
	fgets(buffer, LINE_BUFFER_LEN, scontext->fh);
      }
      status=(pclose(scontext->fh) & 0xff00)>>8;
      if(status)
        librdf_parser_error(scontext->pcontext->parser,
                            "SiRPAC command '%s' exited with status %d\n",
                            scontext->command, status);
      scontext->fh=NULL;
    }

    if(scontext->next)
      librdf_free_statement(scontext->next);

    if(scontext->command)
      LIBRDF_FREE(cstring, scontext->command);

    LIBRDF_FREE(librdf_parser_sirpac_context, scontext);
  }
}


/**
 * librdf_parser_sirpac_get_feature - Get SiRPAC parser features
 * @context: parser context
 * @feature: feature URI string
 * 
 * Allows the querying of the rdf:aboutEach and rdf:aboutEachPrefix
 * features
 * 
 * Return values: "yes", "no" or NULL on unknown or illegal feature
 **/
static const char *
librdf_parser_sirpac_get_feature(void *context, librdf_uri *feature)
{
  librdf_parser_sirpac_context* pcontext=(librdf_parser_sirpac_context*)context;
  if(!feature)
    return NULL;

  if(!librdf_uri_equals(feature, LIBRDF_MS_aboutEach_URI))
    return pcontext->feature_aboutEach ? "yes" : "no";

  if(!librdf_uri_equals(feature, LIBRDF_MS_aboutEachPrefix_URI))
    return pcontext->feature_aboutEachPrefix ? "yes" : "no";
  
  return NULL;
}


/**
 * librdf_parser_sirpac_set_feature - Set SiRPAC parser features
 * @context: parser context
 * @feature: feature URI string
 * @value: feature value string
 * 
 * Allows the setting of the rdf:aboutEach and rdf:aboutEachPrefix
 * features.
 * 
 * Return value: Non 0 on failure, negative on illegal feature or value.
 **/
static int
librdf_parser_sirpac_set_feature(void *context, librdf_uri *feature, 
                                 const char *value) 
{
  librdf_parser_sirpac_context* pcontext=(librdf_parser_sirpac_context*)context;

  if(!value)
    return -1;
  
  if(!librdf_uri_equals(feature, LIBRDF_MS_aboutEach_URI)) {
    pcontext->feature_aboutEach=(strcmp(value, "yes") == 0);
    pcontext->feature_aboutEachPrefix = pcontext->feature_aboutEach;
    return 0;
  }

  if(!librdf_uri_equals(feature, LIBRDF_MS_aboutEachPrefix_URI)) {
    pcontext->feature_aboutEachPrefix=(strcmp(value, "yes") == 0);
    pcontext->feature_aboutEach = pcontext->feature_aboutEachPrefix;
    return 0;
  }

  return -1;
}



#ifdef JAVA_SIRPACSTANFORD_JAR
/**
 * librdf_parser_sirpac_stanford_register_factory - Register the SiRPAC RDF parser with the RDF parser factory
 * @factory: prototype rdf parser factory
 **/
static void
librdf_parser_sirpac_stanford_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_sirpac_context);
  
  factory->init  = librdf_parser_sirpac_stanford_init;
  factory->parse_uri_as_stream = librdf_parser_sirpac_parse_uri_as_stream;
  factory->parse_uri_into_model = librdf_parser_sirpac_parse_uri_into_model;
  factory->get_feature = librdf_parser_sirpac_get_feature;
  factory->set_feature = librdf_parser_sirpac_set_feature;
}
#endif

#ifdef JAVA_SIRPACW3C_JAR
/**
 * librdf_parser_sirpac_w3c_register_factory - Register the SiRPAC RDF parser with the RDF parser factory
 * @factory: prototype rdf parser factory
 **/
static void
librdf_parser_sirpac_w3c_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_sirpac_context);
  
  factory->init  = librdf_parser_sirpac_w3c_init;
  factory->parse_uri_as_stream = librdf_parser_sirpac_parse_uri_as_stream;
  factory->parse_uri_into_model = librdf_parser_sirpac_parse_uri_into_model;
  factory->get_feature = librdf_parser_sirpac_get_feature;
  factory->set_feature = librdf_parser_sirpac_set_feature;
}
#endif

/**
 * librdf_parser_sirpac_constructor - Initialise the SiRPAC RDF parser module
 **/
void
librdf_parser_sirpac_constructor(void)
{
#ifdef JAVA_SIRPACSTANFORD_JAR
  librdf_parser_register_factory("sirpac-stanford", NULL, NULL,
                                 &librdf_parser_sirpac_stanford_register_factory);
#endif

#ifdef JAVA_SIRPACW3C_JAR
  librdf_parser_register_factory("sirpac-w3c", NULL, NULL,
                                 &librdf_parser_sirpac_w3c_register_factory);
#endif
}
