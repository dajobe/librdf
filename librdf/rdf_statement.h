/*
 * RDF Statement definition
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


#ifndef RDF_STATEMENT_H
#define RDF_STATEMENT_H

#include <rdf_node.h>
#include <rdf_uri.h>
#include <rdf_assertion_context.h>

typedef struct 
{
  rdf_node* subject;
  rdf_node* predicate;
  rdf_node* object;
  rdf_uri*  provenance; /* ha ha */
  int       count;
  rdf_assertion_context *context;
}
rdf_statement;


/* class methods */
void init_rdf_statement(void);

/* initialising functions / constructors */

/* Create a new Statement. */
rdf_statement* rdf_new_statement(void);

/* destructor */
void rdf_free_statement(rdf_statement* statement);


/* functions / methods */

rdf_node* rdf_statement_get_subject(rdf_statement *statement);
int rdf_statement_set_subject(rdf_statement *statement, rdf_node *subject);

rdf_node* rdf_statement_get_predicate(rdf_statement *statement);
int rdf_statement_set_predicate(rdf_statement *statement, rdf_node *predicate);

rdf_node* rdf_statement_get_object(rdf_statement *statement);
int rdf_statement_set_object(rdf_statement *statement, rdf_node *object);

int rdf_statement_add_assertion_context(rdf_statement *statement, rdf_assertion_context *context);
rdf_assertion_context* rdf_statement_remove_assertion_context(rdf_statement *statement);

/* convert to a string */
char *rdf_statement_to_string(rdf_statement *statement);

#endif
