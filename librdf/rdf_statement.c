/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_statement.c - RDF Statement implementation
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

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_node.h>
#include <rdf_statement.h>


/* class methods */

/**
 * librdf_init_statement:
 * 
 * Initialise the rdf_statement module
 **/
void
librdf_init_statement(void) 
{
}


/**
 * librdf_finish_statement:
 * 
 * Close down the rdf_statement module
 **/
void
librdf_finish_statement(void) 
{
}


/**
 * librdf_new_statement:
 * 
 * Constructor - create a new empty Statement
 * 
 * Return value: a new &librdf_statement or NULL on failure
 **/
librdf_statement*
librdf_new_statement(void) 
{
  librdf_statement* new_statement;
  
  new_statement = (librdf_statement*)LIBRDF_CALLOC(librdf_statement, 1, sizeof(librdf_statement));
  if(!new_statement)
    return NULL;

  return new_statement;
}


/**
 * librdf_new_statement_from_statement:
 * @statement: &librdf_statement to copy
 * 
 * Copy constructor - Create a new Statement from an existing Statement
 * 
 * Return value: a new &librdf_statement with copy or NULL on failure
 **/
librdf_statement*
librdf_new_statement_from_statement(librdf_statement* statement)
{
  librdf_statement* new_statement;
  
  new_statement = librdf_new_statement();
  if(!new_statement)
	  return NULL;

  new_statement->subject=librdf_new_node_from_node(statement->subject);
  new_statement->predicate=librdf_new_node_from_node(statement->predicate);
  new_statement->object=librdf_new_node_from_node(statement->object);

  if(!new_statement->subject || !new_statement->predicate || 
     !new_statement->object) {
	  librdf_free_statement(new_statement);
	  return NULL;
  }
  return new_statement;
}


/**
 * librdf_free_statement:
 * @statement: &librdf_statement object
 * 
 * Destructor - release the resources for the statement.
 **/
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

/**
 * librdf_statement_get_subject:
 * @statement: &librdf_statement object
 * 
 * Get the statement subject
 * 
 * Return value: a pointer to the &librdf_node of the statement subject - 
 * NOTE this is a shared copy and must be copied if used by the caller.
 **/
librdf_node*
librdf_statement_get_subject(librdf_statement *statement) 
{
  return statement->subject;
}


/**
 * librdf_statement_set_subject:
 * @statement: &librdf_statement object
 * @node: &librdf_node of subject
 * 
 * Set the statement subject.  The subject passed in becomes owned by
 * the statement object and must not be used by the caller after this call.
 **/
void
librdf_statement_set_subject(librdf_statement *statement, librdf_node *node)
{
  statement->subject=node;
}


/**
 * librdf_statement_get_predicate:
 * @statement: &librdf_statement object
 * 
 * Get the statement predicate
 * 
 * Return value: a pointer to the &librdf_node of the statement predicate - 
 * NOTE this is a shared copy and must be copied if used by the caller.
 **/
librdf_node*
librdf_statement_get_predicate(librdf_statement *statement) 
{
  return statement->predicate;
}


/**
 * librdf_statement_set_predicate:
 * @statement: &librdf_statement object
 * @node: &librdf_node of predicate
 * 
 * Set the statement predicate.  The predicate passed in becomes owned by
 * the statement object and must not be used by the caller after this call.
 **/
void
librdf_statement_set_predicate(librdf_statement *statement, librdf_node *node)
{
  statement->predicate=node;
}


/**
 * librdf_statement_get_object:
 * @statement: &librdf_statement object
 * 
 * Get the statement object
 * 
 * Return value: a pointer to the &librdf_node of the statement object - 
 * NOTE this is a shared copy and must be copied if used by the caller.
 **/
librdf_node*
librdf_statement_get_object(librdf_statement *statement) 
{
  return statement->object;
}


/**
 * librdf_statement_set_object:
 * @statement: &librdf_statement object
 * @node: &librdf_node of object
 * 
 * Set the statement object.  The object passed in becomes owned by
 * the statement object and must not be used by the caller after this call.
 **/
void
librdf_statement_set_object(librdf_statement *statement, librdf_node *node)
{
  statement->object=node;
}


/**
 * librdf_statement_get_context:
 * @statement: &librdf_statement object
 * 
 * Get the statement context.  FIXME: This is not supported properly yet.
 * 
 * Return value: A pointer to the context or NULL if no context is defined.
 **/
librdf_context*
librdf_statement_get_context(librdf_statement *statement) 
{
  return statement->context;
}


/**
 * librdf_statement_get_context:
 * @statement: &librdf_statement object
 * @context: &librdf_context statement context
 * 
 * Set the statement context.  FIXME: This is not supported properly yet.
 **/
void
librdf_statement_set_context(librdf_statement *statement, 
                             librdf_context *context)
{
  statement->context=context;
}


/**
 * librdf_statement_to_string:
 * @statement: the statement
 * 
 * Formats the statement as a string from newly allocated memory
 * that must be freed by the caller.
 * 
 * Return value: the string or NULL on failure.
 **/
char *
librdf_statement_to_string(librdf_statement *statement)
{
  char *subject_string, *predicate_string, *object_string;
  char *s;
  int statement_string_len;
  char *format;
  static char *null_string="(null)";

  if(statement->subject) {
    subject_string=librdf_node_to_string(statement->subject);
    if(!subject_string)
      return NULL;
  } else {
    subject_string=null_string;
  }

  
  if(statement->predicate) {
    predicate_string=librdf_node_to_string(statement->predicate);
    if(!predicate_string) {
      if(subject_string != null_string)
        LIBRDF_FREE(cstring, subject_string);
      return NULL;
    }
  } else {
    predicate_string=null_string;
  }
  

  if(statement->object) {
    object_string=librdf_node_to_string(statement->object);
    if(!object_string) {
      if(subject_string != null_string)
        LIBRDF_FREE(cstring, subject_string);
      if(predicate_string != null_string)
        LIBRDF_FREE(cstring, predicate_string);
      return NULL;
    }
  } else {
    object_string=null_string;
  }
  


#define LIBRDF_STATEMENT_FORMAT_STRING_LITERAL "{%s, %s, \"%s\"}"
#define LIBRDF_STATEMENT_FORMAT_RESOURCE_LITERAL "{%s, %s, %s}"
  statement_string_len=1 + strlen(subject_string) +   /* "{%s" */
                       2 + strlen(predicate_string) + /* ", %s" */
                       2 + strlen(object_string) +    /* ", %s" */
                       1 + 1;                         /* "}\0" */
  if(statement->object &&
     librdf_node_get_type(statement->object) == LIBRDF_NODE_TYPE_LITERAL) {
    format=LIBRDF_STATEMENT_FORMAT_STRING_LITERAL;
    statement_string_len+=2; /* Extra "" around literal */
  } else {
    format=LIBRDF_STATEMENT_FORMAT_RESOURCE_LITERAL;
  }
    
  s=(char*)LIBRDF_MALLOC(cstring, statement_string_len);
  if(s)
    sprintf(s, format, predicate_string, subject_string, object_string);

  /* always free allocated intermediate strings */
  if(subject_string != null_string)
    LIBRDF_FREE(cstring, subject_string);
  if(predicate_string != null_string)
    LIBRDF_FREE(cstring, predicate_string);
  if(object_string != null_string)
    LIBRDF_FREE(cstring, object_string);

  return s;
}


/**
 * librdf_statement_equals:
 * @statement1: first &librdf_statement
 * @statement2: second &librdf_statement
 * 
 * Check if two statements are identical.
 * 
 * Return value: non 0 if statements are identical.
 **/
int
librdf_statement_equals(librdf_statement* statement1, 
                        librdf_statement* statement2)
{
  /* FIXME: use digests? */

  if(!librdf_node_equals(statement1->subject, statement2->subject))
      return 0;

  if(!librdf_node_equals(statement1->predicate, statement2->predicate))
      return 0;

  if(!librdf_node_equals(statement1->object, statement2->object))
      return 0;

  /* FIXME: what about context? */

  return 1;
}


/**
 * librdf_statement_match:
 * @statement: statement
 * @partial_statement: statement with possible empty parts
 * 
 * Match a statement against a 'partial' statement, where some
 * parts (subject, predicate, object) of the statement can be empty
 * (NULL).  Empty parts match against any value, parts with values
 * must match exactly.
 * 
 * Return value: non 0 on match
 **/
int
librdf_statement_match(librdf_statement* statement, 
                       librdf_statement* partial_statement)
{
  if(partial_statement->subject &&
     !librdf_node_equals(statement->subject, partial_statement->subject))
      return 0;

  if(partial_statement->predicate &&
     !librdf_node_equals(statement->predicate, partial_statement->predicate))
      return 0;

  if(partial_statement->object &&
     !librdf_node_equals(statement->object, partial_statement->object))
      return 0;

  /* FIXME: what about context? */

  return 1;
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

  s=librdf_statement_to_string(statement);
  fprintf(stderr, "%s: Empty statement: %s\n", program, s);
  LIBRDF_FREE(cstring, s);

  librdf_statement_set_subject(statement, librdf_new_node_from_uri_string("http://www.ilrt.bris.ac.uk/people/cmdjb/"));
  librdf_statement_set_predicate(statement, librdf_new_node_from_uri_string("http://purl.org/dc/elements/1.1/#Creator"));
  librdf_statement_set_object(statement, librdf_new_node_from_literal("Dave Beckett", NULL, 0, 0));

  s=librdf_statement_to_string(statement);
  fprintf(stderr, "%s: Resulting statement: %s\n", program, s);
  LIBRDF_FREE(cstring, s);

  fprintf(stderr, "%s: Freeing statement\n", program);
  librdf_free_statement(statement);
  
#ifdef LIBRDF_DEBUG 
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}

#endif
