/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_statement.h - RDF Statement definition
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



#ifndef LIBRDF_STATEMENT_H
#define LIBRDF_STATEMENT_H


#ifdef __cplusplus
extern "C" {
#endif


struct librdf_statement_s
{
  librdf_node* subject;
  librdf_node* predicate;
  librdf_node* object;
  librdf_context *context;
};



/* class methods */
void librdf_init_statement(void);
void librdf_finish_statement(void);


/* initialising functions / constructors */

/* Create a new Statement. */
librdf_statement* librdf_new_statement(void);

/* Create a new Statement from an existing Statement - CLONE */
librdf_statement* librdf_new_statement_from_statement(librdf_statement* statement);

/* destructor */
void librdf_free_statement(librdf_statement* statement);


/* functions / methods */

librdf_node* librdf_statement_get_subject(librdf_statement *statement);
void librdf_statement_set_subject(librdf_statement *statement, librdf_node *subject);

librdf_node* librdf_statement_get_predicate(librdf_statement *statement);
void librdf_statement_set_predicate(librdf_statement *statement, librdf_node *predicate);

librdf_node* librdf_statement_get_object(librdf_statement *statement);
void librdf_statement_set_object(librdf_statement *statement, librdf_node *object);

librdf_context* librdf_statement_get_context(librdf_statement *statement);
void librdf_statement_set_context(librdf_statement *statement, librdf_context *context);

/* convert to a string */
char *librdf_statement_to_string(librdf_statement *statement);
/* print it prettily */
void librdf_statement_print(librdf_statement *statement, FILE *fh);

/* compare two statements */
int librdf_statement_equals(librdf_statement* statement1, librdf_statement* statement2);
/* match statement against one with partial content */
int librdf_statement_match(librdf_statement* statement, librdf_statement* partial_statement);


#ifdef __cplusplus
}
#endif

#endif
