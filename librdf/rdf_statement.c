/*
 * RDF Statement implementation
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>

#include <stdio.h>

#ifdef STANDALONE
#define RDF_DEBUG 1
#endif

#include <rdf_config.h>
#include <rdf_node.h>
#include <rdf_statement.h>


/* class methods */

void
rdf_init_statement(void) 
{
}


/* constructor */
/* Create a new empty Statement */
rdf_statement*
rdf_new_statement(void) 
{
  rdf_statement* new_statement;
  
  new_statement = (rdf_statement*)RDF_CALLOC(rdf_statement, 1, sizeof(rdf_statement));
  if(!new_statement)
    return NULL;

  return new_statement;
}


/* destructor */

void
rdf_free_statement(rdf_statement* statement)
{
  if(statement->subject)
    rdf_free_node(statement->subject);
  if(statement->predicate)
    rdf_free_node(statement->predicate);
  if(statement->object)
    rdf_free_node(statement->object);
  /* context NOT freed, it is not owned by this object */
  RDF_FREE(rdf_statement, statement);
}



/* methods */

rdf_node*
rdf_statement_get_subject(rdf_statement *statement) 
{
  return statement->subject;
}


int
rdf_statement_set_subject(rdf_statement *statement, rdf_node *node)
{
  /*
  if(statement->subject)
    rdf_free_node(statement->subject);
  return (statement->subject=rdf_new_node_from_node(node)) != NULL;
  */
  statement->subject=node;
  return 0;
}


rdf_node*
rdf_statement_get_predicate(rdf_statement *statement) 
{
  return statement->predicate;
}


int
rdf_statement_set_predicate(rdf_statement *statement, rdf_node *node)
{
  /*
  if(statement->predicate)
    rdf_free_node(statement->predicate);
  return (statement->predicate=rdf_new_node_from_node(node)) != NULL;
  */
  statement->predicate=node;
  return 0;
}


rdf_node*
rdf_statement_get_object(rdf_statement *statement) 
{
  return statement->object;
}


int
rdf_statement_set_object(rdf_statement *statement, rdf_node *node)
{
  /*
  if(statement->object)
    rdf_free_node(statement->object);
  return (statement->object=rdf_new_node_from_node(node)) != NULL;
  */
  statement->object=node;
  return 0;
}


rdf_assertion_context*
rdf_statement_remove_assertion_context(rdf_statement *statement) 
{
  return statement->context;
}


int
rdf_statement_add_assertion_context(rdf_statement *statement, rdf_assertion_context *context)
{
  statement->context=context;
  return 0;
}


/* allocates a new string since this is a _to_ method */
char *
rdf_statement_to_string(rdf_statement *statement)
{
  char *subject_string, *predicate_string, *object_string;
  char *s;
  int statement_string_len;
  char *format;

  subject_string=rdf_node_to_string(statement->subject);
  if(!subject_string)
    return NULL;
  
  predicate_string=rdf_node_to_string(statement->predicate);
  if(!predicate_string) {
    RDF_FREE(cstring, subject_string);
    return NULL;
  }

  object_string=rdf_node_to_string(statement->object);
  if(!object_string) {
    RDF_FREE(cstring, subject_string);
    RDF_FREE(cstring, predicate_string);
    return NULL;
  }

#define RDF_STATEMENT_FORMAT_STRING_LITERAL "{%s ,%s, \"%s\"}"
#define RDF_STATEMENT_FORMAT_RESOURCE_LITERAL "{%s ,%s, %s}"
  statement_string_len=1 + strlen(subject_string) +   /* "{%s" */
                       2 + strlen(predicate_string) + /* " ,%s" */
                       2 + strlen(object_string) +    /* " ,%s" */
                       1 + 1;                         /* "}\0" */
  if(rdf_node_get_type(statement->object) == RDF_NODE_TYPE_LITERAL) {
    format=RDF_STATEMENT_FORMAT_STRING_LITERAL;
    statement_string_len+=2; /* Extra "" around literal */
  } else {
    format=RDF_STATEMENT_FORMAT_RESOURCE_LITERAL;
  }
    
  s=(char*)RDF_MALLOC(cstring, statement_string_len);
  if(!s) {
    RDF_FREE(cstring, subject_string);
    RDF_FREE(cstring, predicate_string);
    RDF_FREE(cstring, object_string);
    return NULL;
  }
  sprintf(s, format, predicate_string, subject_string, object_string);

  return s;
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  rdf_statement* statement;
  char *program=argv[0];
  char *s;
  
  /* initialise statement module */
  rdf_init_statement();

  fprintf(stderr, "%s: Creating statement\n", program);
  statement=rdf_new_statement();

  rdf_statement_set_subject(statement, rdf_new_node_from_uri_string("http://www.ilrt.bris.ac.uk/people/cmdjb/"));
  rdf_statement_set_predicate(statement, rdf_new_node_from_uri_string("http://purl.org/dc/elements/1.1/#Creator"));
  rdf_statement_set_object(statement, rdf_new_node_from_literal("Dave Beckett", NULL));

  s=rdf_statement_to_string(statement);
  fprintf(stderr, "%s: Resulting statement: %s\n", program, s);
  free(s);

  fprintf(stderr, "%s: Freeing statement\n", program);
  rdf_free_statement(statement);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
