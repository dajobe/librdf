/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_model.h - RDF Model definition
 *
 * $Id$
 *
 * Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
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

  /* usage count of this instance
   * Used by other redland classes such as iterator, stream
   * via  librdf_model_add_reference librdf_model_remove_reference
   * The usage count of model after construction is 1.
   */
  int usage;
  
  librdf_list*     sub_models;

  void *context;
  struct librdf_model_factory_s* factory;
};

/** A Model Factory */
struct librdf_model_factory_s {
  librdf_world *world;
  struct librdf_model_factory_s* next;
  char* name;
  
  /* the rest of this structure is populated by the
     model-specific register function */
  size_t context_length;
  
  /* init the factory */
  void (*init)(void);

  /* terminate the factory */
  void (*terminate)(void);
  
  /* create a new model */
  int (*create)(librdf_model* model, librdf_storage* storage, librdf_hash* options);
  
  /* copy a model */
  /* clone is assumed to do leave the new model in the same state
   * after an init() method on an existing model - i.e ready to
   * use but closed.
   */
  librdf_model* (*clone)(librdf_model* new_model);

  /* destroy model */
  void (*destroy)(librdf_model* model);
  
  /* return the number of statements in the model for model */
  int (*size)(librdf_model* model);
  
  /* add a statement to the model from the given model */
  int (*add_statement)(librdf_model* model, librdf_statement* statement);
  
  /* add a statement to the model from the given model */
  int (*add_statements)(librdf_model* model, librdf_stream* statement_stream);
  
  /* remove a statement from the model  */
  int (*remove_statement)(librdf_model* model, librdf_statement* statement);
  
  /* check if statement in model  */
  int (*contains_statement)(librdf_model* model, librdf_statement* statement);
  /* check for [node, property, ?] */
  int (*has_arc_in)(librdf_model *model, librdf_node *node, librdf_node *property);
  /* check for [?, property, node] */
  int (*has_arc_out)(librdf_model *model, librdf_node *node, librdf_node *property);

  
  /* serialise the model in model  */
  librdf_stream* (*serialise)(librdf_model* model);
  
  /* serialise the results of a query */
  librdf_stream* (*find_statements)(librdf_model* model, librdf_statement* statement);
  librdf_stream* (*find_statements_with_options)(librdf_model* model, librdf_statement* statement, librdf_node* context_node, librdf_hash* options);

  /* return a list of Nodes marching given arc, target */
  librdf_iterator* (*get_sources)(librdf_model* model, librdf_node *arc, librdf_node *target);

  /* return a list of Nodes marching given source, target */
  librdf_iterator* (*get_arcs)(librdf_model* model, librdf_node *source, librdf_node *target);

  /* return a list of Nodes marching given source, target */
  librdf_iterator* (*get_targets)(librdf_model* model, librdf_node *source, librdf_node *target);

  /* return list of properties to/from a node */
  librdf_iterator* (*get_arcs_in)(librdf_model *model, librdf_node *node);
  librdf_iterator* (*get_arcs_out)(librdf_model *model, librdf_node *node);

  /* add a statement to the model from the context */
  int (*context_add_statement)(librdf_model* model, librdf_node* context, librdf_statement *statement);
  
  /* remove a statement from the context  */
  int (*context_remove_statement)(librdf_model* model, librdf_node* context, librdf_statement *statement);

  /* list statements in a context  */
  librdf_stream* (*context_serialize)(librdf_model* model, librdf_node* context);

  /* query the model */
  librdf_stream* (*query)(librdf_model* model, librdf_query* query);

  /* sync the model to the storage - OPTIONAL */
  void (*sync)(librdf_model* model);

  /* add a statement from the context - OPTIONAL (librdf_model will
   * implement using context_add_statement if missing) 
   */
  int (*context_add_statements)(librdf_model* model, librdf_node* context, librdf_stream *stream);

  /* remove a statement from the context - OPTIONAL (librdf_model will
   * implement using context_remove_statement if missing) 
   */
  int (*context_remove_statements)(librdf_model* model, librdf_node* context);

  /* get the single storage for this model if there is one - OPTIONAL */
  librdf_storage* (*get_storage)(librdf_model* model);

  /* search for statement in a context - OPTIONAL (rdf_model will do
   * it using find_statements if missing)
   */
  librdf_stream* (*find_statements_in_context)(librdf_model* model, librdf_statement* statement, librdf_node* context_node);

  /* return an iterator of context nodes in the store - OPTIONAL
   * (returning NULL)
   */
  librdf_iterator* (*get_contexts)(librdf_model* model);

  /* features - OPTIONAL */
  librdf_node* (*get_feature)(librdf_model* model, librdf_uri* feature);
  int (*set_feature)(librdf_model* model, librdf_uri* feature, librdf_node* value);

};

#include <rdf_model_storage.h>

/* module init */
void librdf_init_model(librdf_world *world);

  /* module terminate */
void librdf_finish_model(librdf_world *world);

/* class methods */
void librdf_model_register_factory(const char *name, void (*factory) (librdf_model_factory*));
librdf_model_factory* librdf_get_model_factory(const char *name);

void librdf_model_add_reference(librdf_model *model);
void librdf_model_remove_reference(librdf_model *model);

#endif


/* constructors */

/* Create a new Model */
REDLAND_API librdf_model* librdf_new_model(librdf_world *world, librdf_storage *storage, char* options_string);
REDLAND_API librdf_model* librdf_new_model_with_options(librdf_world *world, librdf_storage *storage, librdf_hash* options);

/* Create a new Model from an existing Model - CLONE */
REDLAND_API librdf_model* librdf_new_model_from_model(librdf_model* model);

/* destructor */
REDLAND_API void librdf_free_model(librdf_model *model);


/* functions / methods */
REDLAND_API int librdf_model_size(librdf_model* model);

/* add statements */
REDLAND_API int librdf_model_add(librdf_model* model, librdf_node* subject, librdf_node* predicate, librdf_node* object);
REDLAND_API int librdf_model_add_string_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* predicate, const unsigned char* string, char *xml_language, int is_wf_xml);
REDLAND_API int librdf_model_add_typed_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* predicate, const unsigned char* string, char *xml_language, librdf_uri *datatype_uri);
REDLAND_API int librdf_model_add_statement(librdf_model* model, librdf_statement* statement);
REDLAND_API int librdf_model_add_statements(librdf_model* model, librdf_stream* statement_stream);

/* remove statements */
REDLAND_API int librdf_model_remove_statement(librdf_model* model, librdf_statement* statement);

/* containment */
/* check for exact statement match */
REDLAND_API int librdf_model_contains_statement(librdf_model* model, librdf_statement* statement);
/* check for [node, property, ?] */
REDLAND_API int librdf_model_has_arc_in(librdf_model *model, librdf_node *node, librdf_node *property);
/* check for [?, property, node] */
REDLAND_API int librdf_model_has_arc_out(librdf_model *model, librdf_node *node, librdf_node *property);


/* list the entire model as a stream of statements */
REDLAND_API librdf_stream* librdf_model_as_stream(librdf_model* model);
/* DEPRECATED serialise the entire model */
REDLAND_API REDLAND_DEPRECATED librdf_stream* librdf_model_serialise(librdf_model* model);

/* queries */

REDLAND_API librdf_stream* librdf_model_find_statements(librdf_model* model, librdf_statement* statement);

#define LIBRDF_MODEL_FIND_OPTION_MATCH_SUBSTRING_LITERAL "http://feature.librdf.org/model-find-match-substring-literal"

REDLAND_API librdf_stream* librdf_model_find_statements_with_options(librdf_model* model, librdf_statement* statement, librdf_node* context_node, librdf_hash* options);
REDLAND_API librdf_iterator* librdf_model_get_sources(librdf_model *model, librdf_node *arc, librdf_node *target);
REDLAND_API librdf_iterator* librdf_model_get_arcs(librdf_model *model, librdf_node *source, librdf_node *target);
REDLAND_API librdf_iterator* librdf_model_get_targets(librdf_model *model, librdf_node *source, librdf_node *arc);
REDLAND_API librdf_node* librdf_model_get_source(librdf_model *model, librdf_node *arc, librdf_node *target);
REDLAND_API librdf_node* librdf_model_get_arc(librdf_model *model, librdf_node *source, librdf_node *target);
REDLAND_API librdf_node* librdf_model_get_target(librdf_model *model, librdf_node *source, librdf_node *arc);

/* return list of properties to/from a node */
REDLAND_API librdf_iterator* librdf_model_get_arcs_in(librdf_model *model, librdf_node *node);
REDLAND_API librdf_iterator* librdf_model_get_arcs_out(librdf_model *model, librdf_node *node);



/* submodels */
REDLAND_API int librdf_model_add_submodel(librdf_model* model, librdf_model* sub_model);
REDLAND_API int librdf_model_remove_submodel(librdf_model* model, librdf_model* sub_model);


REDLAND_API void librdf_model_print(librdf_model *model, FILE *fh);

/* statement contexts */
REDLAND_API int librdf_model_context_add_statement(librdf_model* model, librdf_node* context, librdf_statement* statement);
REDLAND_API int librdf_model_context_add_statements(librdf_model* model, librdf_node* context, librdf_stream* stream);
REDLAND_API int librdf_model_context_remove_statement(librdf_model* model, librdf_node* context, librdf_statement* statement);
REDLAND_API int librdf_model_context_remove_statements(librdf_model* model, librdf_node* context);
REDLAND_API librdf_stream* librdf_model_context_as_stream(librdf_model* model, librdf_node* context);
REDLAND_API REDLAND_DEPRECATED librdf_stream* librdf_model_context_serialize(librdf_model* model, librdf_node* context);

/* query language */
REDLAND_API librdf_stream* librdf_model_query(librdf_model* model, librdf_query* query);
REDLAND_API librdf_stream* librdf_model_query_string(librdf_model* model, const char *name, librdf_uri* uri, const unsigned char *query_string);

REDLAND_API void librdf_model_sync(librdf_model* model);

REDLAND_API librdf_storage* librdf_model_get_storage(librdf_model *model);


/* find statements in a given context */
REDLAND_API librdf_stream* librdf_model_find_statements_in_context(librdf_model* model, librdf_statement* statement, librdf_node* context_node);

REDLAND_API librdf_iterator* librdf_model_get_contexts(librdf_model* model);


#define LIBRDF_MODEL_FEATURE_CONTEXTS "http://feature.librdf.org/model-contexts"
/* features */
REDLAND_API librdf_node* librdf_model_get_feature(librdf_model* model, librdf_uri* feature);
REDLAND_API int librdf_model_set_feature(librdf_model* model, librdf_uri* feature, librdf_node* value);

#ifdef __cplusplus
}
#endif

#endif
