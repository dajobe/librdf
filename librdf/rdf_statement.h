/*
 * rdf_statement.h - RDF Statement definition
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



#ifndef LIBRDF_STATEMENT_H
#define LIBRDF_STATEMENT_H

#include <rdf_node.h>
#include <rdf_uri.h>
#include <rdf_assertion_context.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct 
{
  librdf_node* subject;
  librdf_node* predicate;
  librdf_node* object;
  librdf_uri*  provenance; /* ha ha */
  int       count;
  librdf_assertion_context *context;
}
librdf_statement;


/* class methods */
void librdf_init_statement(void);

/* initialising functions / constructors */

/* Create a new Statement. */
librdf_statement* librdf_new_statement(void);

/* destructor */
void librdf_free_statement(librdf_statement* statement);


/* functions / methods */

librdf_node* librdf_statement_get_subject(librdf_statement *statement);
int librdf_statement_set_subject(librdf_statement *statement, librdf_node *subject);

librdf_node* librdf_statement_get_predicate(librdf_statement *statement);
int librdf_statement_set_predicate(librdf_statement *statement, librdf_node *predicate);

librdf_node* librdf_statement_get_object(librdf_statement *statement);
int librdf_statement_set_object(librdf_statement *statement, librdf_node *object);

int librdf_statement_add_assertion_context(librdf_statement *statement, librdf_assertion_context *context);
librdf_assertion_context* librdf_statement_remove_assertion_context(librdf_statement *statement);

/* convert to a string */
char *librdf_statement_to_string(librdf_statement *statement);


#ifdef __cplusplus
}
#endif

#endif
