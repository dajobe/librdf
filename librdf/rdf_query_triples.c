/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query_triples.c - RDF Query simple triple query language
 *
 * $Id$
 *
 * Copyright (C) 2002 David Beckett - http://purl.org/net/dajobe/
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
#include <sys/types.h>

#include <librdf.h>


typedef struct
{
  librdf_statement statement; /* statement to match */
  
} librdf_query_triples_context;


/* prototypes for local functions */
static int librdf_query_triples_init(librdf_query* query, const char *name, librdf_uri* uri, const unsigned char *query_string);
static int librdf_query_triples_open(librdf_query* queryl);
static int librdf_query_triples_close(librdf_query* query);
static librdf_stream* librdf_query_triples_run(librdf_query* query, librdf_model* mode);


static void librdf_query_triples_register_factory(librdf_query_factory *factory);


/*
 * librdf_query_triples_find_next_term - Locate end of triples query term
 * @string: the string to search
 * 
 * Find the character after a term like '[...]' or '"..."' or NULL
 * if not found, end of string 
 * 
 * Return value: pointer to the character in the string or NULL
 **/
static unsigned char *
librdf_query_triples_find_next_term(unsigned char *string) 
{
  unsigned char c;
  unsigned char delim='\0';

  if(!string)
    return NULL;
  
  switch(*string++) {
    case '"':
      delim='"';
      break;
    case '[':
      delim=']';
      break;
    case '-':
      return string;
    default:
      /* Bad value - includes '\0' */
      return NULL;
  }

  while((c=*string++)) {
    if(c == delim)
      break;
  }
  
  if(!c)
    string=NULL;
  
  return string;
}



/* functions implementing query api */


/**
 * librdf_query_triples_init - Initialise a triples query from the string
 * @query: the &librdf_query
 * @name: the query language name
 * @uri: the query language URI or NULL
 * @query_string: the query string
 * 
 * Parses the query string in the triples form to create an internal
 * representation, suitable for use in querying.
 *
 * The syntax of the query format is as follows:
 *   query     := subject ' ' predicate ' ' object
 *   subject   := null | uri
 *   predicate := null | uri
 *   object    := null | uri | literal
 *   null      : '-'
 *   uri       : '[' URI-string ']'
 *   literal   : ''' string '''
 * 
 * Return value: 
 **/
static int
librdf_query_triples_init(librdf_query* query, 
                          const char *name, librdf_uri* uri,
                          const unsigned char* query_string)
{
  librdf_query_triples_context *context=(librdf_query_triples_context*)query->context;
  int len;
  unsigned char *query_string_copy;
  unsigned char *cur, *p;
  librdf_node *subject, *predicate, *object;
  
  len=strlen((const char*)query_string);
  query_string_copy=(unsigned char*)LIBRDF_MALLOC(cstring, len+1);
  if(!query_string_copy)
    return 0;
  strcpy((char*)query_string_copy, (const char*)query_string);

  cur=query_string_copy;
  
  /* subject - NULL or URI */
  p=librdf_query_triples_find_next_term(cur);
  if(!p) {
    librdf_error(query->world, "Bad triples query language syntax - bad subject in '%s'", cur);
    LIBRDF_FREE(cstring, query_string_copy);
    return 0;
  }
  *p='\0';
  p++;
  if(strcmp((const char*)cur, "-")) {
    /* Expecting query_string='[URI]' */
    cur++; /* Move past '[' */
    p[-2]='\0';     /* Zap ']' */
    subject=librdf_new_node_from_uri_string(query->world, cur);
    if(!subject) {
      LIBRDF_FREE(cstring, query_string_copy);
      return 0;
    }
    librdf_statement_set_subject(&context->statement, subject);
  }
  cur=p;
  
  /* predicate - NULL or URI */
  p=(unsigned char*)librdf_query_triples_find_next_term(cur);
  if(!p) {
    librdf_error(query->world, "Bad triples query language syntax - bad predicate in '%s'", cur);
    LIBRDF_FREE(cstring, query_string_copy);
    librdf_free_node(subject);
    return 0;
  }
  *p='\0';
  p++;
  if(strcmp((const char*)cur, "-")) {
    /* Expecting cur='[URI]' */
    cur++; /* Move past '[' */
    p[-2]='\0';     /* Zap ']' */
    predicate=librdf_new_node_from_uri_string(query->world, cur);
    if(!predicate) {
      LIBRDF_FREE(cstring, query_string_copy);
      librdf_free_node(subject);
      return 0;
    }
    librdf_statement_set_predicate(&context->statement, predicate);
  }
  cur=p;
  
  /* object - NULL, literal or URI */
  p=librdf_query_triples_find_next_term(cur);
  if(!p) {
    librdf_error(query->world, "Bad triples query language syntax - bad object in '%s'", cur);
    LIBRDF_FREE(cstring, query_string_copy);
    librdf_free_node(subject);
    librdf_free_node(predicate);
    return 0;
  }
  *p='\0';
  p++;
  if(strcmp((const char*)cur, "-")) {
    /* Expecting cur='[URI]' or '"string"' */
    cur++; /* Move past '[' or '"' */
    p[-2]='\0';     /* Zap ']' or '"' */
    if(cur[-1] == '"')
      object=librdf_new_node_from_literal(query->world, cur, NULL, 0);
    else
      object=librdf_new_node_from_uri_string(query->world, cur);
    if(!object) {
      LIBRDF_FREE(cstring, query_string_copy);
      librdf_free_node(subject);
      librdf_free_node(predicate);
      return 0;
    }
    librdf_statement_set_object(&context->statement, object);
  }

  LIBRDF_FREE(cstring, query_string_copy);
  
  return 0;
}


static void
librdf_query_triples_terminate(librdf_query* query)
{
  librdf_query_triples_context *context=(librdf_query_triples_context*)query->context;
  librdf_node *node;

  node=librdf_statement_get_subject(&context->statement);
  if(node)
    librdf_free_node(node);

  node=librdf_statement_get_predicate(&context->statement);
  if(node)
    librdf_free_node(node);

  node=librdf_statement_get_object(&context->statement);
  if(node)
    librdf_free_node(node);

}


static int
librdf_query_triples_open(librdf_query* query)
{
/*  librdf_query_triples_context *context=(librdf_query_triples_context*)query->context; */
  
  return 0;
}


/**
 * librdf_query_triples_close:
 * @query: the query
 * 
 * Close the query list query, and free all content since there is no 
 * persistance.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_query_triples_close(librdf_query* query)
{
/*  librdf_query_triples_context* context=(librdf_query_triples_context*)query->context; */

  return 0;
}


static librdf_stream*
librdf_query_triples_run(librdf_query* query, librdf_model* model)
{
  librdf_query_triples_context* context=(librdf_query_triples_context*)query->context;

  return librdf_model_find_statements(model, &context->statement);
}

/* local function to register list query functions */

static void
librdf_query_triples_register_factory(librdf_query_factory *factory) 
{
  factory->context_length     = sizeof(librdf_query_triples_context);
  
  factory->init               = librdf_query_triples_init;
  factory->terminate          = librdf_query_triples_terminate;
  factory->open               = librdf_query_triples_open;
  factory->close              = librdf_query_triples_close;
  factory->run                = librdf_query_triples_run;
}


void
librdf_init_query_triples(void)
{
  librdf_query_register_factory("triples", NULL,
                                &librdf_query_triples_register_factory);
}
