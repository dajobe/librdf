/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_model.h - RDF Model definition
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



#ifndef LIBRDF_MODEL_H
#define LIBRDF_MODEL_H

#include <rdf_uri.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDF_INTERNAL

struct librdf_model_s {
  librdf_world *world;
  
  /* these two are alternatives (probably should be a union) */

  /* 1. model is stored here */
  librdf_storage*  storage;
  
  /* 2. model is built from a list of sub models */
  librdf_list*     sub_models;
};

#endif

/* class methods */
void librdf_init_model(librdf_world *world);
void librdf_finish_model(librdf_world *world);


/* constructors */

/* Create a new Model */
librdf_model* librdf_new_model(librdf_world *world, librdf_storage *storage, char* options_string);
librdf_model* librdf_new_model_with_options(librdf_world *world, librdf_storage *storage, librdf_hash* options);

/* Create a new Model from an existing Model - CLONE */
librdf_model* librdf_new_model_from_model(librdf_model* model);

/* destructor */
void librdf_free_model(librdf_model *model);


/* functions / methods */
int librdf_model_size(librdf_model* model);

/* add statements */
int librdf_model_add(librdf_model* model, librdf_node* subject, librdf_node* predicate, librdf_node* object);
int librdf_model_add_string_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* predicate, char* string, char *xml_language, int is_wf_xml);
int librdf_model_add_typed_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* predicate, char* string, char *xml_language, librdf_uri *datatype_uri);
int librdf_model_add_statement(librdf_model* model, librdf_statement* statement);
int librdf_model_add_statements(librdf_model* model, librdf_stream* statement_stream);

/* remove statements */
int librdf_model_remove_statement(librdf_model* model, librdf_statement* statement);

/* containment */
/* check for exact statement match */
int librdf_model_contains_statement(librdf_model* model, librdf_statement* statement);
/* check for [node, property, ?] */
int librdf_model_has_arc_in(librdf_model *model, librdf_node *node, librdf_node *property);
/* check for [?, property, node] */
int librdf_model_has_arc_out(librdf_model *model, librdf_node *node, librdf_node *property);


/* serialise the entire model */
librdf_stream* librdf_model_serialise(librdf_model* model);

/* queries */

librdf_stream* librdf_model_find_statements(librdf_model* model, librdf_statement* statement);

librdf_iterator* librdf_model_get_sources(librdf_model *model, librdf_node *arc, librdf_node *target);
librdf_iterator* librdf_model_get_arcs(librdf_model *model, librdf_node *source, librdf_node *target);
librdf_iterator* librdf_model_get_targets(librdf_model *model, librdf_node *source, librdf_node *arc);
librdf_node* librdf_model_get_source(librdf_model *model, librdf_node *arc, librdf_node *target);
librdf_node* librdf_model_get_arc(librdf_model *model, librdf_node *source, librdf_node *target);
librdf_node* librdf_model_get_target(librdf_model *model, librdf_node *source, librdf_node *arc);

/* return list of properties to/from a node */
librdf_iterator* librdf_model_get_arcs_in(librdf_model *model, librdf_node *node);
librdf_iterator* librdf_model_get_arcs_out(librdf_model *model, librdf_node *node);



/* submodels */
int librdf_model_add_submodel(librdf_model* model, librdf_model* sub_model);
int librdf_model_remove_submodel(librdf_model* model, librdf_model* sub_model);


void librdf_model_print(librdf_model *model, FILE *fh);

/* statement groups */
int librdf_model_add_statements_group(librdf_model* model, librdf_uri* group_uri, librdf_stream* stream);
int librdf_model_remove_statements_group(librdf_model* model, librdf_uri* group_uri);

/* query language */
librdf_stream* librdf_model_query(librdf_model* model, librdf_query* query);
librdf_stream* librdf_model_query_string(librdf_model* model, const char *name, librdf_uri* uri, const char *query_string);


#ifdef __cplusplus
}
#endif

#endif
