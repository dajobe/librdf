/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_query_rasqal.c - RDF Query with Rasqal
 *
 * Copyright (C) 2004-2008, David Beckett http://www.dajobe.org/
 * Copyright (C) 2004-2005, University of Bristol, UK http://www.bristol.ac.uk/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 * 
 */


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <redland.h>




typedef struct
{
  librdf_query *query;        /* librdf query object */
  librdf_model *model;
  rasqal_query *rq;
  rasqal_query_results *results;
  char *language;            /* rasqal query language name to use */
  unsigned char *query_string;
  librdf_uri *uri;           /* base URI or NULL */

  int errors;
  int warnings;
} librdf_query_rasqal_context;


/* prototypes for local functions */
static int rasqal_redland_init_triples_match(rasqal_triples_match* rtm, rasqal_triples_source *rts, void *user_data, rasqal_triple_meta *m, rasqal_triple *t);
static int rasqal_redland_triple_present(rasqal_triples_source *rts, void *user_data, rasqal_triple *t);
static void rasqal_redland_free_triples_source(void *user_data);


static void
librdf_query_rasqal_log_handler(void *data, raptor_log_message *message)
{
  librdf_world *world = (librdf_world *)data;
  librdf_log_level level;

  /* Map raptor2 fatal/error/warning log levels to librdf log levels,
     ignore others */
  switch(message->level) {
    case RAPTOR_LOG_LEVEL_FATAL:
      level = LIBRDF_LOG_FATAL;
      break;

    case RAPTOR_LOG_LEVEL_ERROR:
      level = LIBRDF_LOG_ERROR;
      break;

    case RAPTOR_LOG_LEVEL_WARN:
      level = LIBRDF_LOG_WARN;
      break;

    case RAPTOR_LOG_LEVEL_INFO:
    case RAPTOR_LOG_LEVEL_DEBUG:
    case RAPTOR_LOG_LEVEL_TRACE:
    case RAPTOR_LOG_LEVEL_NONE:
    default:
      return;
  }

  librdf_log_simple(world, 0, level, LIBRDF_FROM_QUERY, message->locator,
                    message->text);
}


/* functions implementing query api */


static int
librdf_query_rasqal_init(librdf_query* query, 
                         const char *name, librdf_uri* uri,
                         const unsigned char* query_string,
                         librdf_uri *base_uri)
{
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  size_t len;
  unsigned char *query_string_copy;
  
  context->query = query;
  context->language=context->query->factory->name;

  context->rq=rasqal_new_query(query->world->rasqal_world_ptr, context->language, NULL);
  if(!context->rq)
    return 1;

  rasqal_query_set_user_data(context->rq, query);

  rasqal_world_set_log_handler(query->world->rasqal_world_ptr, query->world,
                               librdf_query_rasqal_log_handler);

  len = strlen((const char*)query_string);
  query_string_copy = LIBRDF_MALLOC(unsigned char*, len + 1);
  if(!query_string_copy)
    return 1;
  strcpy((char*)query_string_copy, (const char*)query_string);

  context->query_string=query_string_copy;
  if(base_uri)
    context->uri=librdf_new_uri_from_uri(base_uri);

  return 0;
}


static void
librdf_query_rasqal_terminate(librdf_query* query)
{
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(context->rq)
    rasqal_free_query(context->rq);

  if(context->query_string)
    LIBRDF_FREE(char*, context->query_string);

  if(context->uri)
    librdf_free_uri(context->uri);

  if(context->model)
    librdf_free_model(context->model);
}


/*
 * This function and RASQAL_LITERAL_UDT first appears in Rasqal 0.9.17
 *
 * FIXME: Remove this code when minimum rasqal version >= 0.9.17
 */
#ifndef RASQAL_LITERAL_UDT_DEFINED
static rasqal_literal_type
rasqal_literal_get_rdf_term_type(rasqal_literal* l)
{
  rasqal_literal_type type = l->type;
  
  /* squash literal datatypes into one type: RDF Literal */
  if(type >= RASQAL_LITERAL_FIRST_XSD &&
     type <= RASQAL_LITERAL_LAST_XSD)
    type = RASQAL_LITERAL_STRING;

  if(type != RASQAL_LITERAL_URI &&
     type != RASQAL_LITERAL_STRING &&
     type != RASQAL_LITERAL_BLANK)
    type = RASQAL_LITERAL_UNKNOWN;

  return type;
}
#endif


static librdf_node*
rasqal_literal_to_redland_node(librdf_world *world, rasqal_literal* l)
{
  rasqal_literal_type type;

  if(!l)
    return NULL;
  
  /* FIXME: Workaround for Issue #0000519
   * http://bugs.librdf.org/mantis/view.php?id=519
   *
   * Remove this 'if' when RASQAL_MIN_VERSION is 0.9.30 or larger
   */
  if(l->type == RASQAL_LITERAL_INTEGER_SUBTYPE)
    type = RASQAL_LITERAL_STRING;
  else
    type = rasqal_literal_get_rdf_term_type(l);

  if(type == RASQAL_LITERAL_URI)
    return librdf_new_node_from_uri(world, (librdf_uri*)l->value.uri);
  else if (type == RASQAL_LITERAL_STRING)
    return librdf_new_node_from_typed_literal(world, 
                                              (unsigned char*)l->string, 
                                              l->language, 
                                              (librdf_uri*)l->datatype);
  else if (type == RASQAL_LITERAL_BLANK)
    return librdf_new_node_from_blank_identifier(world,
                                                 (unsigned char*)l->string);

  LIBRDF_DEBUG2("Could not convert literal type %d to librdf_node\n", type);
  return NULL;
}


rasqal_literal*
redland_node_to_rasqal_literal(librdf_world* world, librdf_node *node)
{
  rasqal_literal* l;
  
  if(librdf_node_is_resource(node)) {
    raptor_uri* uri=(raptor_uri*)librdf_new_uri_from_uri(librdf_node_get_uri(node));
    l=rasqal_new_uri_literal(world->rasqal_world_ptr, uri); /* transfer uri ownership to literal */
  } else if(librdf_node_is_literal(node)) {
    unsigned char *string;
    librdf_uri *uri;
    unsigned char *new_string;
    char *new_language=NULL;
    raptor_uri *new_datatype=NULL;
    size_t len;

    string=librdf_node_get_literal_value_as_counted_string(node, &len);
    new_string=(unsigned char*)rasqal_alloc_memory(len+1);
    if(!new_string)
      return NULL;
    strcpy((char*)new_string, (const char*)string);

    string=(unsigned char*)librdf_node_get_literal_value_language(node);
    if(string) {
      new_language=(char*)rasqal_alloc_memory(strlen((const char*)string)+1);
      if(!new_language) {
        rasqal_free_memory((void*)new_string);
        return NULL;
      }
      strcpy((char*)new_language, (const char*)string);
    }
    uri=librdf_node_get_literal_value_datatype_uri(node);
    if(uri)
      new_datatype=(raptor_uri*)librdf_new_uri_from_uri(uri);
    /* transfer new_string,new_language,new_datatype ownership to literal */
    l = rasqal_new_string_literal(world->rasqal_world_ptr, (const unsigned char*)new_string, new_language, new_datatype, NULL);
  } else {
    unsigned char *blank=librdf_node_get_blank_identifier(node);
    unsigned char *new_blank;
    if(!blank)
      return NULL;
    new_blank=(unsigned char*)rasqal_alloc_memory(strlen((const char*)blank)+1);
    if(!new_blank)
      return NULL;
    strcpy((char*)new_blank, (const char*)blank);
    /* transfer new_blank ownership to literal */
    l = rasqal_new_simple_literal(world->rasqal_world_ptr, RASQAL_LITERAL_BLANK, (const unsigned char*)new_blank);
  }

  return l;
}


typedef struct {
  librdf_world *world;
  librdf_query *query;
  librdf_model *model;
} rasqal_redland_triples_source_user_data;



static int
rasqal_redland_new_triples_source(rasqal_query* rdf_query,
                                  void *factory_user_data,
                                  void *user_data,
                                  rasqal_triples_source *rts)
{
  librdf_world *world=(librdf_world*)factory_user_data;
  rasqal_redland_triples_source_user_data* rtsc=(rasqal_redland_triples_source_user_data*)user_data;
  raptor_sequence *seq;
  librdf_query_rasqal_context *context;
  librdf_iterator* cit;

  rtsc->world = world;
  rtsc->query = (librdf_query*)rasqal_query_get_user_data(rdf_query);
  context = (librdf_query_rasqal_context*)rtsc->query->context;
  rtsc->model = context->model;

  seq = rasqal_query_get_data_graph_sequence(rdf_query);
  
  /* FIXME: queries with data graphs in them (such as FROM in SPARQL)
   * are deleted, so that there are no unexpected data loads
   */
  if(seq) {
    while(raptor_sequence_size(seq)) {
      rasqal_data_graph* dg=(rasqal_data_graph*)raptor_sequence_pop(seq);
      rasqal_free_data_graph(dg);
    }
  }

  if(librdf_model_supports_contexts(rtsc->model)) {
    /* Add all contexts (named graphs) to the query so Rasqal can bind them */
    cit = librdf_model_get_contexts(rtsc->model);
    while(!librdf_iterator_end(cit)) {
      librdf_node* node = (librdf_node*)librdf_iterator_get_object(cit);
      librdf_uri* uri;
      raptor_uri* source_uri;
      rasqal_data_graph* dg;

      uri = librdf_node_get_uri(node);
      source_uri = (raptor_uri*)raptor_new_uri(world->raptor_world_ptr,
                                               librdf_uri_as_string(uri));

      dg = rasqal_new_data_graph_from_uri(world->rasqal_world_ptr,
                                          source_uri, source_uri,
                                          RASQAL_DATA_GRAPH_NAMED,
                                          NULL, NULL, NULL);
      rasqal_query_add_data_graph(rdf_query, dg);

      raptor_free_uri(source_uri);
      librdf_iterator_next(cit);
    }
    librdf_free_iterator(cit);
  }

#ifdef RASQAL_TRIPLES_SOURCE_MIN_VERSION
  rts->version = 1;
#endif

  rts->init_triples_match=rasqal_redland_init_triples_match;
  rts->triple_present=rasqal_redland_triple_present;
  rts->free_triples_source=rasqal_redland_free_triples_source;

  return 0;
}


static int
rasqal_redland_triple_present(rasqal_triples_source *rts, void *user_data, 
                              rasqal_triple *t) 
{
  rasqal_redland_triples_source_user_data* rtsc=(rasqal_redland_triples_source_user_data*)user_data;
  librdf_node* nodes[3];
  librdf_statement *s;
  int rc;
  
  /* ASSUMPTION: all the parts of the triple are not variables */
  /* FIXME: and no error checks */
  nodes[0]=rasqal_literal_to_redland_node(rtsc->world, t->subject);
  nodes[1]=rasqal_literal_to_redland_node(rtsc->world, t->predicate);
  nodes[2]=rasqal_literal_to_redland_node(rtsc->world, t->object);

  s=librdf_new_statement_from_nodes(rtsc->world, nodes[0], nodes[1], nodes[2]);
  
  rc=librdf_model_contains_statement(rtsc->model, s);
  librdf_free_statement(s);
  return rc;
}



static void
rasqal_redland_free_triples_source(void *user_data)
{
  /* rasqal_redland_triples_source_user_data* rtsc=(rasqal_redland_triples_source_user_data*)user_data; */
}


#ifdef RASQAL_TRIPLES_SOURCE_FACTORY_MIN_VERSION
#define RTS_RET int
#else
#define RTS_RET void
#endif

static RTS_RET
rasqal_redland_register_triples_source_factory(rasqal_triples_source_factory *factory) 
{
#ifdef RASQAL_TRIPLES_SOURCE_FACTORY_MIN_VERSION
  factory->version = 1;
#endif
  factory->user_data_size=sizeof(rasqal_redland_triples_source_user_data);
  factory->new_triples_source=rasqal_redland_new_triples_source;

#ifdef RASQAL_TRIPLES_SOURCE_FACTORY_MIN_VERSION
  return 0;
#endif
}


typedef struct {
  librdf_node* nodes[3];
  librdf_node* origin;
  /* query statement, made from the nodes above (even when exact) */
  librdf_statement *qstatement;
  librdf_stream *stream;
} rasqal_redland_triples_match_context;


static rasqal_triple_parts
rasqal_redland_bind_match(struct rasqal_triples_match_s* rtm,
                          void *user_data,
                          rasqal_variable* bindings[4],
                          rasqal_triple_parts parts) 
{
  rasqal_redland_triples_match_context* rtmc=(rasqal_redland_triples_match_context*)rtm->user_data;
  rasqal_literal* l;
  librdf_statement* statement;
  rasqal_triple_parts result=(rasqal_triple_parts)0;
  librdf_world *world = rtmc->stream->world;

  statement=librdf_stream_get_object(rtmc->stream);
  if(!statement)
    return (rasqal_triple_parts)0;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG1("  matched statement ");
  librdf_statement_print(statement, stderr);
  fputc('\n', stderr);
#endif

  /* set 1 or 2 variable values from the fields of statement */

  if(bindings[0] && (parts & RASQAL_TRIPLE_SUBJECT)) {
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG1("binding subject to variable\n");
#endif
    l = redland_node_to_rasqal_literal(world,
                                       librdf_statement_get_subject(statement));
    rasqal_variable_set_value(bindings[0], l);
    result= RASQAL_TRIPLE_SUBJECT;
  }

  if(bindings[1] && (parts & RASQAL_TRIPLE_PREDICATE)) {
    if(bindings[0] == bindings[1]) {
      /* check matching(?x, ?x, ...) / subject=predicate */
      if(!librdf_node_equals(librdf_statement_get_subject(statement),
                             librdf_statement_get_predicate(statement)))
        return (rasqal_triple_parts)0;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("subject and predicate values match\n");
#endif
    } else {
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("binding predicate to variable\n");
#endif
      l = redland_node_to_rasqal_literal(world,
                                         librdf_statement_get_predicate(statement));
      rasqal_variable_set_value(bindings[1], l);
      result= (rasqal_triple_parts)(result | RASQAL_TRIPLE_PREDICATE);
    }
  }

  if(bindings[2] && (parts & RASQAL_TRIPLE_OBJECT)) {
    int bind=1;
    
    if(bindings[0] == bindings[2]) {
      /* check matching (?x, ..., ?x) / subject=object */
      if(!librdf_node_equals(librdf_statement_get_subject(statement),
                             librdf_statement_get_object(statement)))
        return (rasqal_triple_parts)0;
      bind=0;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("subject and object values match\n");
#endif
    }
    if((bindings[1] == bindings[2]) && 
       !(bindings[0] == bindings[1])) {
      /* check matching (..., ?x, ?x) / predicate=object
       * Don't do this if matching (?x, ?x, ...) / subject=predicate
       * was already done since that would mean the match was (?x, ?x, ?x)
       */
      if(!librdf_node_equals(librdf_statement_get_predicate(statement),
                             librdf_statement_get_object(statement)))
        return (rasqal_triple_parts)0;
      bind=0;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("predicate and object values match\n");
#endif
    }
    
    if(bind) {
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("binding object to variable\n");
#endif
      l = redland_node_to_rasqal_literal(world,
                                         librdf_statement_get_object(statement));
      rasqal_variable_set_value(bindings[2], l);
      result= (rasqal_triple_parts)(result | RASQAL_TRIPLE_OBJECT);
    }
  }

  /* Contexts */
  if(bindings[3] && (parts & RASQAL_TRIPLE_ORIGIN)) {
    int bind=1;
    librdf_node* context_node = librdf_stream_get_context2(rtmc->stream);
    
    if(bindings[0] == bindings[3]) {
      /* check matching (?x, ..., ...) in context ?x */
      if(!librdf_node_equals(librdf_statement_get_subject(statement),
                             context_node))
        return (rasqal_triple_parts)0;
      bind=0;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("subject and context values match\n");
#endif
    }

    if(bindings[1] == bindings[3]) {
      /* check matching (..., ?x,  ...) in context ?x */
      if(!librdf_node_equals(librdf_statement_get_predicate(statement),
                             context_node))
        return (rasqal_triple_parts)0;
      bind=0;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("predicate and context values match\n");
#endif
    }

    if(bindings[2] == bindings[3]) {
      /* check matching (..., ..., ?x) in context ?x */
      if(!librdf_node_equals(librdf_statement_get_object(statement),
                             context_node))
        return (rasqal_triple_parts)0;
      bind=0;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("object and context values match\n");
#endif
    }

    if(bind) {
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
      LIBRDF_DEBUG1("binding origin to variable\n");
#endif
      if(context_node)
        l = redland_node_to_rasqal_literal(world, context_node);
      else
        l=NULL;
      rasqal_variable_set_value(bindings[3], l);
      result= (rasqal_triple_parts)(result | RASQAL_TRIPLE_ORIGIN);
    }
  }

  return result;
}


static void
rasqal_redland_next_match(struct rasqal_triples_match_s* rtm,
                          void *user_data)
{
  rasqal_redland_triples_match_context* rtmc=(rasqal_redland_triples_match_context*)rtm->user_data;

  librdf_stream_next(rtmc->stream);
}

static int
rasqal_redland_is_end(struct rasqal_triples_match_s* rtm,
                      void *user_data)
{
  rasqal_redland_triples_match_context* rtmc=(rasqal_redland_triples_match_context*)rtm->user_data;

  return librdf_stream_end(rtmc->stream);
}


static void
rasqal_redland_finish_triples_match(struct rasqal_triples_match_s* rtm,
                                    void *user_data)
{
  rasqal_redland_triples_match_context* rtmc=(rasqal_redland_triples_match_context*)rtm->user_data;

  if(rtmc) {
    if(rtmc->stream) {
      librdf_free_stream(rtmc->stream);
      rtmc->stream=NULL;
    }
    if(rtmc->qstatement)
      librdf_free_statement(rtmc->qstatement);
    LIBRDF_FREE(rasqal_redland_triples_match_context, rtmc);
  }
}


static int
rasqal_redland_init_triples_match(rasqal_triples_match* rtm,
                                  rasqal_triples_source *rts, void *user_data,
                                  rasqal_triple_meta *m, rasqal_triple *t)
{
  rasqal_redland_triples_source_user_data* rtsc=(rasqal_redland_triples_source_user_data*)user_data;
  rasqal_redland_triples_match_context* rtmc;
  rasqal_variable* var;

  rtm->bind_match=rasqal_redland_bind_match;
  rtm->next_match=rasqal_redland_next_match;
  rtm->is_end=rasqal_redland_is_end;
  rtm->finish=rasqal_redland_finish_triples_match;

  rtmc = LIBRDF_CALLOC(rasqal_redland_triples_match_context*, 1, sizeof(*rtmc));
  if(!rtmc)
    return 1;

  rtm->user_data=rtmc;


  /* at least one of the triple terms is a variable and we need to
   * do a triplesMatching() aka librdf_model_find_statements
   *
   * redland find_statements will do the right thing and internally
   * pick the most efficient, indexed way to get the answer.
   */

  if((var=rasqal_literal_as_variable(t->subject))) {
    if(var->value)
      rtmc->nodes[0]=rasqal_literal_to_redland_node(rtsc->world, var->value);
    else
      rtmc->nodes[0]=NULL;
  } else
    rtmc->nodes[0]=rasqal_literal_to_redland_node(rtsc->world, t->subject);

  m->bindings[0]=var;
  

  if((var=rasqal_literal_as_variable(t->predicate))) {
    if(var->value)
      rtmc->nodes[1]=rasqal_literal_to_redland_node(rtsc->world, var->value);
    else
      rtmc->nodes[1]=NULL;
  } else
    rtmc->nodes[1]=rasqal_literal_to_redland_node(rtsc->world, t->predicate);

  m->bindings[1]=var;
  

  if((var=rasqal_literal_as_variable(t->object))) {
    if(var->value)
      rtmc->nodes[2]=rasqal_literal_to_redland_node(rtsc->world, var->value);
    else
      rtmc->nodes[2]=NULL;
  } else
    rtmc->nodes[2]=rasqal_literal_to_redland_node(rtsc->world, t->object);

  m->bindings[2]=var;
  

  if(t->origin) {
    if((var=rasqal_literal_as_variable(t->origin))) {
      if(var->value)
        rtmc->origin=rasqal_literal_to_redland_node(rtsc->world, var->value);
    } else
      rtmc->origin=rasqal_literal_to_redland_node(rtsc->world, t->origin);
    m->bindings[3]=var;
  }


  rtmc->qstatement=librdf_new_statement_from_nodes(rtsc->world, 
                                                   rtmc->nodes[0],
                                                   rtmc->nodes[1], 
                                                   rtmc->nodes[2]);
  if(!rtmc->qstatement)
    return 1;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG1("query statement: ");
  librdf_statement_print(rtmc->qstatement, stderr);
  if(rtmc->origin) {
    fput(" with context node: ", stderr);
    librdf_node_print(rtmc->origin, stderr);
  }
  fputc('\n', stderr);
#endif
  
  if(rtmc->origin)
    rtmc->stream=librdf_model_find_statements_in_context(rtsc->model, 
                                                         rtmc->qstatement,
                                                         rtmc->origin);
  else
    rtmc->stream=librdf_model_find_statements(rtsc->model, rtmc->qstatement);

  if(!rtmc->stream)
    return 1;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG1("rasqal_init_triples_match done\n");
#endif

  return 0;
}


static librdf_query_results*
librdf_query_rasqal_execute(librdf_query* query, librdf_model* model)
{
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  librdf_query_results* results;

  if (context->model)
    librdf_free_model(context->model);
  /* model is always non-NULL */
  context->model = model;
  librdf_model_add_reference(model);

  /* This assumes raptor's URI implementation is librdf_uri */
  if(rasqal_query_prepare(context->rq, context->query_string, 
                          (raptor_uri*)context->uri))
    return NULL;

  if(context->results)
    rasqal_free_query_results(context->results);
  
  context->results=rasqal_query_execute(context->rq);
  if(!context->results)
    return NULL;
  
  results = LIBRDF_MALLOC(librdf_query_results*, sizeof(*results));
  if(!results) {
    rasqal_free_query_results(context->results);
    context->results=NULL;
  } else {
    results->query=query;
  }
  
  return results;
}


static int
librdf_query_rasqal_get_limit(librdf_query* query)
{
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  return rasqal_query_get_limit(context->rq);
}


static int
librdf_query_rasqal_set_limit(librdf_query* query, int limit)
{
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  rasqal_query_set_limit(context->rq, limit);
  return 0;
}


static int
librdf_query_rasqal_get_offset(librdf_query* query)
{
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  return rasqal_query_get_offset(context->rq);
}


static int
librdf_query_rasqal_set_offset(librdf_query* query, int offset)
{
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  rasqal_query_set_offset(context->rq, offset);
  return 0;
}


static int
librdf_query_rasqal_results_get_count(librdf_query_results *query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  return rasqal_query_results_get_count(context->results);
}


static int
librdf_query_rasqal_results_next(librdf_query_results *query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return 1;
  
  return rasqal_query_results_next(context->results);
}


static int
librdf_query_rasqal_results_finished(librdf_query_results *query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return 1;
  
  return rasqal_query_results_finished(context->results);
}


static int
librdf_query_rasqal_results_get_bindings(librdf_query_results *query_results, 
                                         const char ***names, 
                                         librdf_node **values)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  rasqal_literal **literals;
  int rc;
  int i;

  if(!context->results)
    return 1;
  
  if(values) {
    rc=rasqal_query_results_get_bindings(context->results, (const unsigned char ***)names, &literals);
  } else
    rc=rasqal_query_results_get_bindings(context->results, (const unsigned char ***)names, NULL);

  if(rc || !values)
    return rc;

  for(i=0; i<rasqal_query_results_get_bindings_count(context->results); i++)
    values[i]=rasqal_literal_to_redland_node(query->world, literals[i]);

  return 0;
}


static librdf_node*
librdf_query_rasqal_results_get_binding_value(librdf_query_results *query_results, int offset)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  rasqal_literal* literal;

  literal=rasqal_query_results_get_binding_value(context->results, offset);

  return rasqal_literal_to_redland_node(query->world, literal);
}


static const char*
librdf_query_rasqal_results_get_binding_name(librdf_query_results *query_results, int offset)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return NULL;
  
  return (const char*)rasqal_query_results_get_binding_name(context->results, offset);
}


static librdf_node*
librdf_query_rasqal_results_get_binding_value_by_name(librdf_query_results *query_results, const char *name)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  rasqal_literal* literal;

  if(!context->results)
    return NULL;
  
  literal=rasqal_query_results_get_binding_value_by_name(context->results, (const unsigned char*)name);

  return rasqal_literal_to_redland_node(query->world, literal);
}


static int
librdf_query_rasqal_results_get_bindings_count(librdf_query_results *query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return 1;
  
  return rasqal_query_results_get_bindings_count(context->results);
}


static void
librdf_query_rasqal_free_results(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return;
  
  rasqal_free_query_results(context->results);
  context->results=NULL;
}


static int
librdf_query_rasqal_results_is_bindings(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return 1;
  
  return rasqal_query_results_is_bindings(context->results);
}
  

static int
librdf_query_rasqal_results_is_boolean(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return 1;
  
  return rasqal_query_results_is_boolean(context->results);
}


static int
librdf_query_rasqal_results_is_graph(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return 1;
  
  return rasqal_query_results_is_graph(context->results);
}


static int
librdf_query_rasqal_results_is_syntax(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return 1;
  
  return rasqal_query_results_is_syntax(context->results);
}


static int
librdf_query_rasqal_results_get_boolean(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;

  if(!context->results)
    return -1;
  
  return rasqal_query_results_get_boolean(context->results);
}


typedef struct {
  librdf_query *query;
  
  librdf_query_rasqal_context* qcontext; /* query context */

  librdf_statement* statement; /* current statement */
  int finished;
} librdf_query_rasqal_stream_context;


static int
librdf_query_rasqal_query_results_end_of_stream(void* context)
{
  librdf_query_rasqal_stream_context* scontext=(librdf_query_rasqal_stream_context*)context;

  return scontext->finished;
}

static int
librdf_query_rasqal_query_results_update_statement(void* context)
{
  librdf_query_rasqal_stream_context* scontext=(librdf_query_rasqal_stream_context*)context;
  librdf_world* world=scontext->query->world;
  librdf_node* node;
  
  raptor_statement *rstatement=rasqal_query_results_get_triple(scontext->qcontext->results);
  if(!rstatement)
    return 1;
  
  scontext->statement=librdf_new_statement(world);
  if(!scontext->statement)
    return 1;

  /* subject */
  
  if(rstatement->subject->type == RAPTOR_TERM_TYPE_BLANK) {
    node = librdf_new_node_from_blank_identifier(world, rstatement->subject->value.blank.string);
  } else if(rstatement->subject->type == RAPTOR_TERM_TYPE_URI) {
    node = librdf_new_node_from_uri_string(world,
                                           librdf_uri_as_string((librdf_uri*)rstatement->subject->value.uri));
  } else {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL,
               "Unknown Raptor subject identifier type %d",
               rstatement->subject->type);
    goto fail;
  }

  if(!node) {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL,
               "Could not create subject node");
    goto fail;
  }
  
  librdf_statement_set_subject(scontext->statement, node);

  /* predicate */

  if(rstatement->predicate->type == RAPTOR_TERM_TYPE_URI) {
    node = librdf_new_node_from_uri_string(world,
                                           librdf_uri_as_string((librdf_uri*)rstatement->predicate->value.uri));
  } else {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL,
               "Unknown Raptor predicate identifier type %d",
               rstatement->predicate->type);
    goto fail;
  }

  if(!node) {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL,
               "Could not create predicate node");
    goto fail;
  }
  
  librdf_statement_set_predicate(scontext->statement, node);
  
  /* object */

  if(rstatement->object->type == RAPTOR_TERM_TYPE_LITERAL) {
    node = librdf_new_node_from_typed_literal(world,
                                              rstatement->object->value.literal.string,
                                              (const char*)rstatement->object->value.literal.language,
                                              (librdf_uri*)rstatement->object->value.literal.datatype);
  } else if(rstatement->object->type == RAPTOR_TERM_TYPE_BLANK) {
    node = librdf_new_node_from_blank_identifier(world, rstatement->object->value.blank.string);
  } else if(rstatement->object->type == RAPTOR_TERM_TYPE_URI) {
    node = librdf_new_node_from_uri_string(world,
                                           librdf_uri_as_string((librdf_uri*)rstatement->object->value.uri));
  } else {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_PARSER, NULL,
               "Unknown Raptor object identifier type %d",
               rstatement->object->type);
    goto fail;
  }

  if(!node) {
    librdf_log(world,
               0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL,
               "Could not create object node");
    goto fail;
  }

  librdf_statement_set_object(scontext->statement, node);

  return 0; /* success */

  fail:
  librdf_free_statement(scontext->statement);
  scontext->statement=NULL;
  return 1;
}


static int
librdf_query_rasqal_query_results_next_statement(void* context)
{
  librdf_query_rasqal_stream_context* scontext=(librdf_query_rasqal_stream_context*)context;

  if(scontext->finished)
    return 1;
  
  if(scontext->statement) {
    librdf_free_statement(scontext->statement);
    scontext->statement=NULL;
  }

  scontext->finished = !scontext->qcontext->results;
  if(!scontext->finished)
    scontext->finished=rasqal_query_results_next_triple(scontext->qcontext->results);
  if(!scontext->finished)
    librdf_query_rasqal_query_results_update_statement(scontext);
  
  return scontext->finished;
}


static void*
librdf_query_rasqal_query_results_get_statement(void* context, int flags)
{
  librdf_query_rasqal_stream_context* scontext=(librdf_query_rasqal_stream_context*)context;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return scontext->statement;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return NULL;
      
    default:
      librdf_log(scontext->query->world,
                 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_QUERY, NULL,
                 "Unknown iterator method flag %d", flags);
      return NULL;
  }

}


static void
librdf_query_rasqal_query_results_finished(void* context)
{
  librdf_query_rasqal_stream_context* scontext=(librdf_query_rasqal_stream_context*)context;

  if(scontext) {
    if(scontext->statement)
      librdf_free_statement(scontext->statement);

    LIBRDF_FREE(librdf_query_rasqal_context, scontext);
  }
}


static librdf_stream*
librdf_query_rasqal_results_as_stream(librdf_query_results* query_results)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  librdf_query_rasqal_stream_context* scontext;
  librdf_stream *stream;

  if(!context->results)
    return NULL;
  
  scontext = LIBRDF_CALLOC(librdf_query_rasqal_stream_context*, 1,
                           sizeof(*scontext));
  if(!scontext)
    return NULL;

  scontext->query=query;
  scontext->qcontext=context;

  librdf_query_rasqal_query_results_update_statement(scontext);

  stream=librdf_new_stream(query->world,
                           (void*)scontext,
                           &librdf_query_rasqal_query_results_end_of_stream,
                           &librdf_query_rasqal_query_results_next_statement,
                           &librdf_query_rasqal_query_results_get_statement,
                           &librdf_query_rasqal_query_results_finished);
  if(!stream) {
    librdf_query_rasqal_query_results_finished((void*)scontext);
    return NULL;
  }

  return stream;  
}


static librdf_query_results_formatter*
librdf_query_rasqal_new_results_formatter(librdf_query_results* query_results,
                                          const char *name,
                                          const char *mime_type, 
                                          librdf_uri* format_uri)
{
  rasqal_world* rasqal_world_ptr;
  rasqal_query_results_formatter* formatter;
  librdf_query_results_formatter* qrf;

  rasqal_world_ptr = query_results->query->world->rasqal_world_ptr;

  formatter = rasqal_new_query_results_formatter(rasqal_world_ptr,
                                                 name,
                                                 mime_type,
                                                 (raptor_uri*)format_uri);
  if(!formatter)
    return NULL;

  qrf = LIBRDF_MALLOC(librdf_query_results_formatter*, sizeof(*qrf));
  if(!qrf) {
    rasqal_free_query_results_formatter(formatter);
    return NULL;
  }

  qrf->query_results = query_results;
  qrf->formatter = formatter;
  return qrf;
}


static void
librdf_query_rasqal_free_results_formatter(librdf_query_results_formatter* qrf) 
{
  rasqal_free_query_results_formatter(qrf->formatter);
  LIBRDF_FREE(librdf_query_results, qrf);
}


static int
librdf_query_rasqal_results_formatter_write(raptor_iostream *iostr,
                                            librdf_query_results_formatter* qrf,
                                            librdf_query_results* query_results,
                                            librdf_uri *base_uri)
{
  librdf_query *query=query_results->query;
  librdf_query_rasqal_context *context=(librdf_query_rasqal_context*)query->context;
  return rasqal_query_results_formatter_write(iostr, qrf->formatter,
                                              context->results, 
                                              (raptor_uri*)base_uri);
}


/* local function to register list query functions */

static void
librdf_query_rasqal_register_factory(librdf_query_factory *factory) 
{
  factory->context_length     = sizeof(librdf_query_rasqal_context);
  
  factory->init               = librdf_query_rasqal_init;
  factory->terminate          = librdf_query_rasqal_terminate;
  factory->execute            = librdf_query_rasqal_execute;
  factory->get_limit          = librdf_query_rasqal_get_limit;
  factory->set_limit          = librdf_query_rasqal_set_limit;
  factory->get_offset         = librdf_query_rasqal_get_offset;
  factory->set_offset         = librdf_query_rasqal_set_offset;

  factory->results_get_count           = librdf_query_rasqal_results_get_count;
  factory->results_next                = librdf_query_rasqal_results_next;
  factory->results_finished            = librdf_query_rasqal_results_finished;
  factory->results_get_bindings        = librdf_query_rasqal_results_get_bindings;
  factory->results_get_binding_value   = librdf_query_rasqal_results_get_binding_value;
  factory->results_get_binding_name    = librdf_query_rasqal_results_get_binding_name;
  factory->results_get_binding_value_by_name = librdf_query_rasqal_results_get_binding_value_by_name;
  factory->results_get_bindings_count         = librdf_query_rasqal_results_get_bindings_count;
  factory->free_results                       = librdf_query_rasqal_free_results;
  factory->results_is_bindings                = librdf_query_rasqal_results_is_bindings;
  factory->results_is_boolean                 = librdf_query_rasqal_results_is_boolean;
  factory->results_is_graph                   = librdf_query_rasqal_results_is_graph;
  factory->results_is_syntax                  = librdf_query_rasqal_results_is_syntax;
  factory->results_get_boolean                = librdf_query_rasqal_results_get_boolean;
  factory->results_as_stream                  = librdf_query_rasqal_results_as_stream;

  factory->new_results_formatter              = librdf_query_rasqal_new_results_formatter;
  factory->free_results_formatter             = librdf_query_rasqal_free_results_formatter;
  factory->results_formatter_write            = librdf_query_rasqal_results_formatter_write;
}


int
librdf_query_rasqal_constructor(librdf_world *world)
{
  unsigned int i;

  if(!world->rasqal_world_ptr) {
    world->rasqal_world_ptr=rasqal_new_world();
    world->rasqal_world_allocated_here = 1;

    if(!world->rasqal_world_ptr) {
      LIBRDF_FATAL1(world, LIBRDF_FROM_QUERY, "failed to initialize rasqal");
      return 1;
    }

    /* Make sure rasqal works with the same raptor instance as everyone else. */
    rasqal_world_set_raptor(world->rasqal_world_ptr, world->raptor_world_ptr);

    if(world->rasqal_world_ptr && world->rasqal_init_handler) {
      world->rasqal_init_handler(world->rasqal_init_handler_user_data,
                                 world->rasqal_world_ptr);
    }

    if(rasqal_world_open(world->rasqal_world_ptr)) {
      LIBRDF_FATAL1(world, LIBRDF_FROM_QUERY, "failed to initialize rasqal");
      return 1;
    }
  }


  rasqal_set_triples_source_factory(world->rasqal_world_ptr, rasqal_redland_register_triples_source_factory, world);

  /* enumerate from query language 1, so the default query language 0
   * is done last 
   */
  for(i = 1; 1; i++) {
    const raptor_syntax_description* desc = NULL;
    const char* uri_string = NULL;

    desc = rasqal_world_get_query_language_description(world->rasqal_world_ptr, i);
    if(!desc) {
      /* reached the end of the query languages, now register the default one */
      i = 0;

      desc = rasqal_world_get_query_language_description(world->rasqal_world_ptr, i);
      if(!desc) {
        LIBRDF_FATAL1(world, LIBRDF_FROM_QUERY, "failed to initialize rasqal");
        return 1;
      }
    }

    uri_string = desc->uri_strings_count ? desc->uri_strings[0] : NULL;

    librdf_query_register_factory(world, desc->names[0], (const unsigned char*)uri_string,
                                  &librdf_query_rasqal_register_factory);
    
    if(!i) /* registered default query, end */
      break;
  }


#if 0
  /* FIXME - this should be used but for now it is safe to assume
   * all query is done by Rasqal
   */

  /* enumerate query result formats */
  for(i=0; 1; i++) {
    const char *format_name=NULL;
    const char *format_label=NULL;
    const unsigned char *format_uri_string=NULL;
    const char *format_mime_type=NULL;

    if(rasqal_query_results_formats_enumerate_full(i, 
                                                   &format_name,
                                                   &format_label,
                                                   &format_uri_string, 
                                                   &format_mime_type))
      break;
    
    librdf_query_register_result_format(world, format_name, format_label,
                                        format_uri_string, format_mime_type,
                                        &librdf_query_rasqal_register_factory);
  }
#endif

  return 0;
}


/**
 * librdf_world_set_rasqal_init_handler:
 * @world: librdf_world object
 * @user_data: user data
 * @handler: handler to call
 *
 * Set the rasqal initialization handler to be called if a new rasqal_world is constructed.
 */
void
librdf_world_set_rasqal_init_handler(librdf_world* world,
                                     void* user_data,
                                     librdf_rasqal_init_handler handler)
{
  world->rasqal_init_handler = handler;
  world->rasqal_init_handler_user_data = user_data;
}


void
librdf_query_rasqal_destructor(librdf_world *world)
{
  if(world->rasqal_world_ptr && world->rasqal_world_allocated_here) {
    rasqal_free_world(world->rasqal_world_ptr);
    world->rasqal_world_ptr=NULL;
  }
}
