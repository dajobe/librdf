/*
 * RDF Model definition
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


#ifndef RDF_MODEL_H
#define RDF_MODEL_H

#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_statement.h>
#include <rdf_storage.h>
#include <rdf_iterator.h>


struct rdf_model_s {
  rdf_storage*  storage;
  struct rdf_model_s* sub_models;
};
typedef struct rdf_model_s rdf_model;


/* class methods */
void rdf_init_model(void);


/* constructors */

/* Create a new Model */
rdf_model* rdf_new_model(void);

/* Create a new Model from an existing Model - CLONE */
rdf_model* rdf_new_model_from_model(rdf_model* model);

/* destructor */
void rdf_free_model(rdf_model *model);


/* functions / methods */
int rdf_model_size(rdf_model* model);

int rdf_model_add(rdf_model* model, rdf_node* subject, rdf_node* property, rdf_node* object);
int rdf_model_add_string_literal_statement(rdf_model* model, rdf_node* subject, rdf_node* property, char* string);
int rdf_model_add_statement(rdf_model* model, rdf_statement* statement);
int rdf_model_remove_statement(rdf_model* model, rdf_statement* statement);
int rdf_model_contains_statement(rdf_model* model, rdf_statement* statement);
/* returns an interator that returns all statements */
rdf_iterator* rdf_model_get_all_statements(rdf_model* model);

/* any of subject, property or object can be NULL */
/* returns count of number of matching statements */
int rdf_model_find(rdf_model* model, rdf_node* subject, rdf_node* property, rdf_node* object);
/* returns iterator object that returns matching rdf_statements */
rdf_iterator* rdf_model_find_(rdf_model* model, rdf_node* subject, rdf_node* property, rdf_node* object);

/* get/set the model source URI */
rdf_uri* rdf_model_get_source_uri(rdf_model* model);
int rdf_model_set_source_uri(rdf_model* model, rdf_uri *uri);

#endif
