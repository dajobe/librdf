/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser SiRPAC (via pipe) implementation
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



/**
 * librdf_parser_sirpac_init - Initialise the SiRPAC RDF parser
 * @context: context
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_sirpac_init(void *context) 
{
  return 0;
}
  

/**
 * librdf_parser_sirpac_parse_as_stream - Retrieve the RDF/XML content at URI and parse it into a librdf_stream
 * @context: serialisation context
 * @uri: URI of RDF content
 * 
 * FIXME: No error reporting provided 
 *
 * Return value: a new &librdf_stream or NULL if the parse failed.
 **/
static librdf_stream*
librdf_parser_sirpac_parse_as_stream(void *context, librdf_uri *uri) {
  /* Note: not yet used */
/*  librdf_parser_sirpac_context* pcontext=(librdf_parser_sirpac_context*)context; */
  librdf_parser_sirpac_stream_context* scontext;
  librdf_stream* stream;
  int command_len;
  char *command;
  FILE *fh;
  static const char *command_format_string="%s -classpath %s -Dorg.xml.sax.parser=com.microstar.xml.SAXDriver org.w3c.rdf.examples.ListStatements %s";
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

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_parser_sirpac_parse_as_stream, "Running command '%s'\n", command);
#endif

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
 * librdf_parser_sirpac_parse_into_model - Retrieve the RDF/XML content at URI and store in a librdf_model
 * @context: serialisation context
 * @uri: URI of RDF content
 * @model: &librdf_model
 * 
 * FIXME: No error reporting provided 
 * FIXME: Maybe should use lower level code directly.
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_sirpac_parse_into_model(void *context, librdf_uri *uri,
                                       librdf_model *model) {
  librdf_stream* stream;
  
  stream=librdf_parser_sirpac_parse_as_stream(context, uri);
  if(!stream)
    return 1;

  return librdf_model_add_statements(model, stream);
}


/*
 * librdf_parser_sirpac_get_next_statement - helper function to decode the output of the Java command to get the next statement
 * @context: serialisation context
 * 
 * Return value: the next &librdf_statement or NULL at end of stream.
 */
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


  if(feof(context->fh)) {
    int status=pclose(context->fh);

    if(status) {
      /* FIXME: something failed e.g. fork, exec or SiRPAC exited
       * with an error */
      fprintf(stderr, "SiRPAC command '%s' exited with status %d\n",
              context->command, status);
    }
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
      status=pclose(scontext->fh);
      if(status) {
        /* FIXME: something failed e.g. fork, exec or SiRPAC exited
         * with an error */
        fprintf(stderr, "SiRPAC command '%s' exited with status %d\n",
                scontext->command, status);
      }
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
 * librdf_parser_sirpac_register_factory - Register the SiRPAC RDF parser with the RDF parser factory
 * @factory: prototype rdf parser factory
 **/
static void
librdf_parser_sirpac_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_sirpac_context);
  
  factory->init  = librdf_parser_sirpac_init;
  factory->parse_as_stream = librdf_parser_sirpac_parse_as_stream;
  factory->parse_into_model = librdf_parser_sirpac_parse_into_model;
}


/**
 * librdf_parser_sirpac_constructor - Initialise the SiRPAC RDF parser module
 **/
void
librdf_parser_sirpac_constructor(void)
{
  librdf_parser_register_factory("sirpac", &librdf_parser_sirpac_register_factory);
}
