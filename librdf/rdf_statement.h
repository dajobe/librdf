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
#include <rdf_context.h>

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

/* might be defined in other header file as a forward reference */
#ifndef LIBRDF_STATEMENT_DEFINED
typedef struct librdf_statement_s librdf_statement;
#define LIBRDF_STATEMENT_DEFINED 1
#endif



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
int librdf_statement_set_subject(librdf_statement *statement, librdf_node *subject);

librdf_node* librdf_statement_get_predicate(librdf_statement *statement);
int librdf_statement_set_predicate(librdf_statement *statement, librdf_node *predicate);

librdf_node* librdf_statement_get_object(librdf_statement *statement);
int librdf_statement_set_object(librdf_statement *statement, librdf_node *object);

int librdf_statement_set_context(librdf_statement *statement, librdf_context *context);
librdf_context* librdf_statement_get_context(librdf_statement *statement);

/* convert to a string */
char *librdf_statement_to_string(librdf_statement *statement);


#ifdef __cplusplus
}
#endif

#endif
