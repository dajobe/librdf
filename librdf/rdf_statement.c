/*
 * rdf_statement.c - RDF Statement implementation
 *
 * $Source$
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


#include <config.h>

#include <stdio.h>

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#define LIBRDF_INTERNAL 1
#include <rdf_config.h>
#include <rdf_node.h>
#include <rdf_statement.h>


/* class methods */

void
librdf_init_statement(void) 
{
}


/* constructor */
/* Create a new empty Statement */
librdf_statement*
librdf_new_statement(void) 
{
  librdf_statement* new_statement;
  
  new_statement = (librdf_statement*)LIBRDF_CALLOC(librdf_statement, 1, sizeof(librdf_statement));
  if(!new_statement)
    return NULL;

  return new_statement;
}


/* destructor */

void
librdf_free_statement(librdf_statement* statement)
{
  if(statement->subject)
    librdf_free_node(statement->subject);
  if(statement->predicate)
    librdf_free_node(statement->predicate);
  if(statement->object)
    librdf_free_node(statement->object);
  /* context NOT freed, it is not owned by this object */
  LIBRDF_FREE(librdf_statement, statement);
}



/* methods */

librdf_node*
librdf_statement_get_subject(librdf_statement *statement) 
{
  return statement->subject;
}


int
librdf_statement_set_subject(librdf_statement *statement, librdf_node *node)
{
  /*
  if(statement->subject)
    librdf_free_node(statement->subject);
  return (statement->subject=librdf_new_node_from_node(node)) != NULL;
  */
  statement->subject=node;
  return 0;
}


librdf_node*
librdf_statement_get_predicate(librdf_statement *statement) 
{
  return statement->predicate;
}


int
librdf_statement_set_predicate(librdf_statement *statement, librdf_node *node)
{
  /*
  if(statement->predicate)
    librdf_free_node(statement->predicate);
  return (statement->predicate=librdf_new_node_from_node(node)) != NULL;
  */
  statement->predicate=node;
  return 0;
}


librdf_node*
librdf_statement_get_object(librdf_statement *statement) 
{
  return statement->object;
}


int
librdf_statement_set_object(librdf_statement *statement, librdf_node *node)
{
  /*
  if(statement->object)
    librdf_free_node(statement->object);
  return (statement->object=librdf_new_node_from_node(node)) != NULL;
  */
  statement->object=node;
  return 0;
}


librdf_context*
librdf_statement_get_context(librdf_statement *statement) 
{
  return statement->context;
}


int
librdf_statement_set_context(librdf_statement *statement, librdf_context *context)
{
  statement->context=context;
  return 0;
}


/* allocates a new string since this is a _to_ method */
char *
librdf_statement_to_string(librdf_statement *statement)
{
  char *subject_string, *predicate_string, *object_string;
  char *s;
  int statement_string_len;
  char *format;

  subject_string=librdf_node_to_string(statement->subject);
  if(!subject_string)
    return NULL;
  
  predicate_string=librdf_node_to_string(statement->predicate);
  if(!predicate_string) {
    LIBRDF_FREE(cstring, subject_string);
    return NULL;
  }

  object_string=librdf_node_to_string(statement->object);
  if(!object_string) {
    LIBRDF_FREE(cstring, subject_string);
    LIBRDF_FREE(cstring, predicate_string);
    return NULL;
  }

#define LIBRDF_STATEMENT_FORMAT_STRING_LITERAL "{%s ,%s, \"%s\"}"
#define LIBRDF_STATEMENT_FORMAT_RESOURCE_LITERAL "{%s ,%s, %s}"
  statement_string_len=1 + strlen(subject_string) +   /* "{%s" */
                       2 + strlen(predicate_string) + /* " ,%s" */
                       2 + strlen(object_string) +    /* " ,%s" */
                       1 + 1;                         /* "}\0" */
  if(librdf_node_get_type(statement->object) == LIBRDF_NODE_TYPE_LITERAL) {
    format=LIBRDF_STATEMENT_FORMAT_STRING_LITERAL;
    statement_string_len+=2; /* Extra "" around literal */
  } else {
    format=LIBRDF_STATEMENT_FORMAT_RESOURCE_LITERAL;
  }
    
  s=(char*)LIBRDF_MALLOC(cstring, statement_string_len);
  if(s)
    sprintf(s, format, predicate_string, subject_string, object_string);

  /* always free intermediate strings */
  LIBRDF_FREE(cstring, subject_string);
  LIBRDF_FREE(cstring, predicate_string);
  LIBRDF_FREE(cstring, object_string);

  return s;
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_statement* statement;
  char *program=argv[0];
  char *s;
  
  /* initialise statement module */
  librdf_init_statement();

  fprintf(stderr, "%s: Creating statement\n", program);
  statement=librdf_new_statement();

  librdf_statement_set_subject(statement, librdf_new_node_from_uri_string("http://www.ilrt.bris.ac.uk/people/cmdjb/"));
  librdf_statement_set_predicate(statement, librdf_new_node_from_uri_string("http://purl.org/dc/elements/1.1/#Creator"));
  librdf_statement_set_object(statement, librdf_new_node_from_literal("Dave Beckett", NULL, 0));

  s=librdf_statement_to_string(statement);
  fprintf(stderr, "%s: Resulting statement: %s\n", program, s);
  LIBRDF_FREE(cstring, s);

  fprintf(stderr, "%s: Freeing statement\n", program);
  librdf_free_statement(statement);
  
  librdf_memory_report(stderr);
	
  /* keep gcc -Wall happy */
  return(0);
}

#endif
