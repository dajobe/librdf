/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_model.h - RDF Model definition
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



#ifndef LIBRDF_MODEL_H
#define LIBRDF_MODEL_H

#include <rdf_uri.h>

#ifdef __cplusplus
extern "C" {
#endif


struct librdf_model_s {
  /* these two are alternatives (probably should be a union) */

  /* 1. model is stored here */
  librdf_storage*  storage;
  
  /* 2. model is built from a list of sub models */
  librdf_list*     sub_models;
};



/* class methods */
void librdf_init_model(void);
void librdf_finish_model(void);


/* constructors */

/* Create a new Model */
librdf_model* librdf_new_model(librdf_storage *storage, char* options_string);
librdf_model* librdf_new_model_with_options(librdf_storage *storage, librdf_hash* options);

/* Create a new Model from an existing Model - CLONE */
librdf_model* librdf_new_model_from_model(librdf_model* model);

/* Create a new Model from a stream */
librdf_model* librdf_new_model_from_stream(librdf_stream* stream);

/* destructor */
void librdf_free_model(librdf_model *model);


/* functions / methods */
int librdf_model_size(librdf_model* model);

/* add statements */
int librdf_model_add(librdf_model* model, librdf_node* subject, librdf_node* predicate, librdf_node* object);
int librdf_model_add_string_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* predicate, char* string, char *xml_language, int xml_space, int is_wf_xml);
int librdf_model_add_statement(librdf_model* model, librdf_statement* statement);
int librdf_model_add_statements(librdf_model* model, librdf_stream* statement_stream);

/* remove statements */
int librdf_model_remove_statement(librdf_model* model, librdf_statement* statement);

/* containment */
int librdf_model_contains_statement(librdf_model* model, librdf_statement* statement);

/* serialise the entire model */
librdf_stream* librdf_model_serialise(librdf_model* model);

/* queries */

/* NOT YET USED
int librdf_model_find_statements_as_count(librdf_model* model, librdf_node* subject, librdf_node* predicate, librdf_node* object);
*/

librdf_stream* librdf_model_find_statements(librdf_model* model, librdf_statement* statement);

/* S, P, O Node versions - NOT YET USED:
int librdf_model_find_statements(librdf_model* input_model, librdf_model* output_model, librdf_node* subject, librdf_node* predicate, librdf_node* object);

librdf_stream* librdf_model_find_statements_as_stream(librdf_model* model, librdf_node* subject, librdf_node* predicate, librdf_node* object);
*/


/* submodels */
int librdf_model_add_submodel(librdf_model* model, librdf_model* sub_model);
int librdf_model_remove_submodel(librdf_model* model, librdf_model* sub_model);


void librdf_model_print(librdf_model *model, FILE *fh);

#ifdef __cplusplus
}
#endif

#endif
