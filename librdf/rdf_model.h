/*
 * rdf_model.h - RDF Model definition
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



#ifndef LIBRDF_MODEL_H
#define LIBRDF_MODEL_H

#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_statement.h>
#include <rdf_storage.h>
#include <rdf_iterator.h>

#ifdef __cplusplus
extern "C" {
#endif


struct librdf_model_s {
  librdf_storage*  storage;
  struct librdf_model_s* sub_models;
};
typedef struct librdf_model_s librdf_model;


/* class methods */
void librdf_init_model(void);


/* constructors */

/* Create a new Model */
librdf_model* librdf_new_model(void);

/* Create a new Model from an existing Model - CLONE */
librdf_model* librdf_new_model_from_model(librdf_model* model);

/* destructor */
void librdf_free_model(librdf_model *model);


/* functions / methods */
int librdf_model_size(librdf_model* model);

int librdf_model_add(librdf_model* model, librdf_node* subject, librdf_node* property, librdf_node* object);
int librdf_model_add_string_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* property, char* string);
int librdf_model_add_statement(librdf_model* model, librdf_statement* statement);
int librdf_model_remove_statement(librdf_model* model, librdf_statement* statement);
int librdf_model_contains_statement(librdf_model* model, librdf_statement* statement);
/* returns an interator that returns all statements */
librdf_iterator* librdf_model_get_all_statements(librdf_model* model);

/* any of subject, property or object can be NULL */
/* returns count of number of matching statements */
int librdf_model_find(librdf_model* model, librdf_node* subject, librdf_node* property, librdf_node* object);
/* returns iterator object that returns matching librdf_statements */
librdf_iterator* librdf_model_find_(librdf_model* model, librdf_node* subject, librdf_node* property, librdf_node* object);

/* get/set the model source URI */
librdf_uri* librdf_model_get_source_uri(librdf_model* model);
int librdf_model_set_source_uri(librdf_model* model, librdf_uri *uri);

int librdf_model_add_submodel(librdf_model* model, librdf_model* sub_model);
int librdf_model_remove_submodel(librdf_model* model, librdf_model* sub_model);


#ifdef __cplusplus
}
#endif

#endif
