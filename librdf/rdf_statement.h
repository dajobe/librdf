/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_statement.h - RDF Statement definition
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



#ifndef LIBRDF_STATEMENT_H
#define LIBRDF_STATEMENT_H


#ifdef __cplusplus
extern "C" {
#endif


#ifdef LIBRDF_INTERNAL

struct librdf_statement_s
{
  librdf_world* world;
  librdf_node* subject;
  librdf_node* predicate;
  librdf_node* object;
};

/* convienience macros - internal */
#define LIBRDF_NODE_STATEMENT_SUBJECT(s)   ((s)->subject)
#define LIBRDF_NODE_STATEMENT_PREDICATE(s) ((s)->predicate)
#define LIBRDF_NODE_STATEMENT_OBJECT(s)    ((s)->object)

#endif


/* class methods */
void librdf_init_statement(librdf_world *world);
void librdf_finish_statement(librdf_world *world);


/* initialising functions / constructors */

/* Create a new Statement. */
librdf_statement* librdf_new_statement(librdf_world* world);

/* Create a new Statement from an existing Statement - CLONE */
librdf_statement* librdf_new_statement_from_statement(librdf_statement* statement);
/* Create a new Statement from existing Nodes */
librdf_statement* librdf_new_statement_from_nodes(librdf_world *world, librdf_node* subject, librdf_node* predicate, librdf_node* object);

/* Init a statically allocated statement */
void librdf_statement_init(librdf_world *world, librdf_statement *statement);

/* Clear a statically allocated statement */
void librdf_statement_clear(librdf_statement *statement);

/* destructor */
void librdf_free_statement(librdf_statement* statement);


/* functions / methods */

librdf_node* librdf_statement_get_subject(librdf_statement *statement);
void librdf_statement_set_subject(librdf_statement *statement, librdf_node *subject);

librdf_node* librdf_statement_get_predicate(librdf_statement *statement);
void librdf_statement_set_predicate(librdf_statement *statement, librdf_node *predicate);

librdf_node* librdf_statement_get_object(librdf_statement *statement);
void librdf_statement_set_object(librdf_statement *statement, librdf_node *object);

/* if statement has all fields */
int librdf_statement_is_complete(librdf_statement *statement);

/* convert to a string */
char *librdf_statement_to_string(librdf_statement *statement);
/* print it prettily */
void librdf_statement_print(librdf_statement *statement, FILE *fh);

/* compare two statements */
int librdf_statement_equals(librdf_statement* statement1, librdf_statement* statement2);
/* match statement against one with partial content */
int librdf_statement_match(librdf_statement* statement, librdf_statement* partial_statement);

/* serialising/deserialising */
size_t librdf_statement_encode(librdf_statement* statement, unsigned char *buffer, size_t length);
size_t librdf_statement_encode_parts(librdf_statement* statement, librdf_node* context_node, unsigned char *buffer, size_t length, int fields);
size_t librdf_statement_decode(librdf_statement* statement, unsigned char *buffer, size_t length);
size_t librdf_statement_decode_parts(librdf_statement* statement, librdf_node** context_node, unsigned char *buffer, size_t length);


#ifdef LIBRDF_INTERNAL

/* Or-ed to indicate statement parts via fields arguments to
 *   librdf_statement_encode() 
 *   librdf_new_stream_from_node_iterator()
 */
typedef enum {
  LIBRDF_STATEMENT_SUBJECT   = 1 << 0,
  LIBRDF_STATEMENT_PREDICATE = 1 << 1,
  LIBRDF_STATEMENT_OBJECT    = 1 << 2,

  /* must be a combination of all of the above */
  LIBRDF_STATEMENT_ALL       = (LIBRDF_STATEMENT_SUBJECT|
                                LIBRDF_STATEMENT_PREDICATE|
                                LIBRDF_STATEMENT_OBJECT)
} librdf_statement_part;


#endif

#ifdef __cplusplus
}
#endif

#endif
