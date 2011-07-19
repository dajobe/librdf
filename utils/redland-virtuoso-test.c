/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * redland-virtuoso-test.c - Test program to showcase the virtuoso storage
 *
 * Copyright (C) 2008, Openlink Software, http://www.openlinksw.com/
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
 */

/*
 * This program is an example of how to use the 'virtuoso' engine in Redland
 * to connect to the Virtuoso QUAD store.
 *
 * To run this program you need to have installed Virtuoso (5.0.8 or later)
 * on your system and configured an ODBC DSN to connect to your Virtuoso
 * database.
 *
 * There are several ways to pass connection information to this program:
 *
 * a) Default ODBC connection to DSN=Local Virtuoso
 *
 *    ./redland-virtuoso-test
 *
 * b) Using command line parameters:
 *
 *    ./redland-virtuoso-test "dsn='MyVirt',user='demo',password='demo'"
 *
 * c) Using environment variable:
 *
 *    VIRTUOSO_STORAGE_OPTIONS="dsn='Another DSN',user='myuid',password='mypwd'"
 *
 *    ./redland-virtuoso-test
 *
 *
 * You can learn more about Virtuoso from:
 *
 *   http://virtuoso.openlinksw.com/
 *
 *  and you can download the open source release of Virtuoso from:
 *
 *    http://sourceforge.net/projects/virtuoso
 *
 */

#define DEFAULT_STORAGE_OPTIONS		"dsn='Local Virtuoso',user='dba',password='dba'"
#define DEFAULT_CONTEXT			"http://red"
#define DEFAULT_IDENTIFIER		"http://red"

#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>

#include <redland.h>
#include <raptor.h>


int add_triple(librdf_world *world, librdf_node *context, librdf_model* model, const char *s, const char *p, const char *o);
int add_triple_typed(librdf_world *world, librdf_node *context, librdf_model* model, const char *s, const char *p, const char *o);
int print_query_results(librdf_world* world, librdf_model* model, librdf_query_results *results);
 
int PASSED=0;
int FAILED=0;


#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define FORMAT_ATTR(string_index, first_to_check_index) \
  __attribute__((__format__(__printf__, string_index, first_to_check_index)))
#else
#define FORMAT_ATTR(string_index, first_to_check_index)
#endif


/* make GCC happier by having 'printf' format attribute in prototype */
static void endTest(int OK, const char *message, ...) FORMAT_ATTR(2, 3);
static void startTest(int testID, const char *message, ...) FORMAT_ATTR(2, 3);


static void
endTest(int OK, const char *message, ...)
{
  va_list arguments;
  va_start(arguments, message);

  if(OK) {
    fprintf(stderr, "**PASSED**:");
    PASSED++;
  } else {
    fprintf(stderr, "**FAILED**:");
    FAILED++;
  }

  vfprintf(stderr, message, arguments);
  va_end(arguments);
}


static void
startTest(int testID, const char *message, ...)
{
  va_list arguments;
  va_start(arguments, message);

  fprintf(stderr, "%3d:", testID);

  vfprintf(stderr, message, arguments);
  va_end(arguments);
}


static void
getTotal(void)
{
  fprintf(stderr, "=============================================\n");
  fprintf(stderr, "PASSED:%3d  FAILED:%3d\n\n", PASSED, FAILED);
}


static int
log_handler(void *user_data, librdf_log_message *message)
{
  /* int code=message->code; */ /* The error code */
  raptor_locator *locator=(raptor_locator*)(message->locator);

  /* Do not handle messages below warning*/
  if(message->level < LIBRDF_LOG_WARN)
    return 0;

  if(message->level == LIBRDF_LOG_WARN)
    fprintf(stderr, ": Warning - ");
  else
    fprintf(stderr, ": Error - ");

  if(locator) { /* && message->facility == LIBRDF_FROM_PARSER) */
    raptor_locator_print(locator, stderr);
    fputc(':', stderr);
    fputc(' ', stderr);
  }
  fputs(message->message, stderr);
  fputc('\n', stderr);
  if(message->level >= LIBRDF_LOG_FATAL)
    exit(1);

  /* Handled */
  return 1;
}


static void
print_nodes(FILE* fh, librdf_iterator* iterator)
{
  int count;
  librdf_node* context_node;
  librdf_node* node;

  /* (Common code) Print out nodes */
  count=0;
  while(!librdf_iterator_end(iterator)) {
    context_node=(librdf_node*)librdf_iterator_get_context(iterator);
    node=(librdf_node*)librdf_iterator_get_object(iterator);
    if(!node) {
      fprintf(stderr, ": librdf_iterator_get_object returned NULL\n");
      break;
    }

    fputs("Matched node: ", fh);
    librdf_node_print(node, fh);
    if(context_node) {
      fputs(" with context ", fh);
      librdf_node_print(context_node, fh);
    }
    fputc('\n', fh);

    count++;
    librdf_iterator_next(iterator);
  }
  librdf_free_iterator(iterator);
  fprintf(stderr, ": matching nodes: %d\n", count);
}


static void
print_node(FILE* fh, librdf_node* node)
{
  if(node) {
    fputs("Matched node: ", fh);
    librdf_node_print(node, fh);
    fputc('\n', fh);
  }
}


int add_triple(librdf_world *world, librdf_node *context, librdf_model* model, const char *s, const char *p, const char *o)
{
  librdf_node *subject, *predicate, *object;
  librdf_statement* statement=NULL;
  int rc;

  if(librdf_heuristic_is_blank_node(s))
    subject=librdf_new_node_from_blank_identifier(world, (const unsigned char *)librdf_heuristic_get_blank_node(s));
  else
    subject=librdf_new_node_from_uri_string(world, (const unsigned char *)s);

  predicate=librdf_new_node_from_uri_string(world, (const unsigned char *)p);

  if(librdf_heuristic_is_blank_node(o))
    object=librdf_new_node_from_blank_identifier(world, (const unsigned char *)librdf_heuristic_get_blank_node(o));
  else
    object=librdf_new_node_from_uri_string(world, (const unsigned char *)o);

  statement=librdf_new_statement(world);
  librdf_statement_set_subject(statement, subject);
  librdf_statement_set_predicate(statement, predicate);
  librdf_statement_set_object(statement, object);

  rc=librdf_model_context_add_statement(model, context, statement);

  librdf_free_statement(statement);
  return rc;
}

int add_triple_typed(librdf_world *world, librdf_node *context, librdf_model* model, const char *s, const char *p, const char *o)
{
  librdf_node *subject, *predicate, *object;
  librdf_statement* statement=NULL;
  int rc;

  if(librdf_heuristic_is_blank_node(s))
    subject=librdf_new_node_from_blank_identifier(world, (const unsigned char *)librdf_heuristic_get_blank_node(s));
  else
    subject=librdf_new_node_from_uri_string(world, (const unsigned char *)s);

  predicate=librdf_new_node_from_uri_string(world, (const unsigned char *)p);

  if(librdf_heuristic_is_blank_node(o))
    object=librdf_new_node_from_blank_identifier(world, (const unsigned char *)librdf_heuristic_get_blank_node(o));
  else
    object=librdf_new_node_from_literal(world, (const unsigned char *)o, NULL, 0);

  statement=librdf_new_statement(world);
  librdf_statement_set_subject(statement, subject);
  librdf_statement_set_predicate(statement, predicate);
  librdf_statement_set_object(statement, object);

  rc=librdf_model_context_add_statement(model, context, statement);

  librdf_free_statement(statement);
  return rc;
}


int 
print_query_results(librdf_world* world, librdf_model* model, librdf_query_results *results)
{
  int i;
  char *name;
  librdf_stream* stream;
  librdf_serializer* serializer;
  const char *query_graph_serializer_syntax_name="rdfxml";

  if(librdf_query_results_is_bindings(results)) {
    fprintf(stdout, ": Query returned bindings results:\n");

    while(!librdf_query_results_finished(results)) {
      fputs("result: [", stdout);
      for(i=0; i<librdf_query_results_get_bindings_count(results); i++) {
	librdf_node *value=librdf_query_results_get_binding_value(results, i);
	name=(char*)librdf_query_results_get_binding_name(results, i);

	if(i>0)
	fputs(", ", stdout);
	fprintf(stdout, "%s=", name);
	if(value) {
	  librdf_node_print(value, stdout);
	  librdf_free_node(value);
	} else
	fputs("NULL", stdout);
      }
      fputs("]\n", stdout);

      librdf_query_results_next(results);
    }
    fprintf(stdout, ": Query returned %d results\n", librdf_query_results_get_count(results));
  } else if(librdf_query_results_is_boolean(results)) {
    fprintf(stdout, ": Query returned boolean result: %s\n", librdf_query_results_get_boolean(results) ? "true" : "false");
  } else if(librdf_query_results_is_graph(results)) {
    librdf_storage* tmp_storage;
    librdf_model* tmp_model;

    tmp_storage=librdf_new_storage(world, NULL, NULL, NULL);
    tmp_model=librdf_new_model(world, tmp_storage, NULL);

    fprintf(stdout, ": Query returned graph result:\n");

    stream=librdf_query_results_as_stream(results);
    if(!stream) {
      fprintf(stderr, ": Failed to get query results graph\n");
      return -1;
    }
    librdf_model_add_statements(tmp_model, stream);
    librdf_free_stream(stream);

    fprintf(stdout, ": Total %d triples\n", librdf_model_size(model));

    serializer=librdf_new_serializer(world, query_graph_serializer_syntax_name, NULL, NULL);
    if(!serializer) {
      fprintf(stderr, ": Failed to create serializer type %s\n", query_graph_serializer_syntax_name);
      return -1;
    }

    librdf_serializer_serialize_model_to_file_handle(serializer, stdout, NULL, tmp_model);
    librdf_free_serializer(serializer);
    librdf_free_model(tmp_model);
    librdf_free_storage(tmp_storage);
  } else {
    fprintf(stdout, ": Query returned unknown result format\n");
    return -1;
  }
  return 0;
}


int
main(int argc, char *argv[])
{
  librdf_world* world;
  librdf_storage *storage;
  librdf_model* model;
  librdf_node *node;
  librdf_node *subject, *predicate, *object;
  librdf_node* context_node=NULL;
  librdf_stream* stream;
  librdf_iterator* iterator;
  librdf_uri *base_uri=NULL;
  librdf_query *query;
  librdf_query_results *results;
  librdf_hash *options;
  int count;
  int rc;
  int transactions=0;
  const char* storage_name;
  const char* storage_options;
  const char* context;
  const char* identifier;
  const char* results_format;
  librdf_statement* statement=NULL;
  char* query_cmd=NULL;
  char* s;


  /*
   * Initialize
   */
  storage_name="virtuoso";
  results_format="xml";
  context=DEFAULT_CONTEXT;
  identifier=DEFAULT_IDENTIFIER;


  /*
   * Get connection options
   */
  if(argc == 2 && argv[1][0] != '\0')
    storage_options=argv[1];
  else if((s=getenv ("VIRTUOSO_STORAGE_OPTIONS")) != NULL)
    storage_options=s;
  else
    storage_options=DEFAULT_STORAGE_OPTIONS;


  world=librdf_new_world();

  librdf_world_set_logger(world, world, log_handler);

  librdf_world_open(world);

  options=librdf_new_hash(world, NULL);
  librdf_hash_open(options, NULL, 0, 1, 1, NULL);

  librdf_hash_put_strings(options, "contexts", "yes");
  transactions=1;

  librdf_hash_from_string(options, storage_options);

  storage=librdf_new_storage_with_options(world, storage_name, identifier, options);

  if(!storage) {
    fprintf(stderr, ": Failed to open %s storage '%s'\n", storage_name, identifier);
    return(1);
  }

  model=librdf_new_model(world, storage, NULL);
  if(!model) {
    fprintf(stderr, ": Failed to create model\n");
    return(1);
  }

  if(transactions)
    librdf_model_transaction_start(model);

  /* Do this or gcc moans */
  stream=NULL;
  iterator=NULL;
  subject=NULL;
  predicate=NULL;
  object=NULL;
  node=NULL;
  query=NULL;
  results=NULL;
  context_node=librdf_new_node_from_uri_string(world, (const unsigned char *)context);


  /**** Test 1 *******/
  startTest(1, " Remove all triples in <%s> context\n", context);
  {
    rc=librdf_model_context_remove_statements(model, context_node);

    if(rc)
      endTest(0, " failed to remove context triples from the graph\n");
    else
      endTest(1, " removed context triples from the graph\n");
  }


  /**** Test 2 *******/
  startTest(2, " Add triples to <%s> context\n", context);
  {
    rc=0;
    rc |= add_triple(world, context_node, model, "aa", "bb", "cc");
    rc |= add_triple(world, context_node, model, "aa", "bb1", "cc");
    rc |= add_triple(world, context_node, model, "aa", "a2", "_:cc");
    rc |= add_triple_typed(world, context_node, model, "aa", "a2", "cc");
    rc |= add_triple_typed(world, context_node, model, "mm", "nn", "Some long literal with language@en");
    rc |= add_triple_typed(world, context_node, model, "oo", "pp", "12345^^<http://www.w3.org/2001/XMLSchema#int>");
    if(rc)
      endTest(0, " failed add triple\n");
    else
      endTest(1, " add triple to context\n");
  }


  /**** Test 3 *******/
  startTest(3, " Print all triples in <%s> context\n", context);
  {
    raptor_iostream* iostr = raptor_new_iostream_to_file_handle(librdf_world_get_raptor(world), stdout);
    librdf_model_write(model, iostr);
    raptor_free_iostream(iostr);
    endTest(1, "\n");
  }


  /***** Test 4 *****/
  startTest(4, " Count of triples in <%s> context\n", context);
  {
    count=librdf_model_size(model);
    if(count >= 0)
      endTest(1, " graph has %d triples\n", count);
    else
      endTest(0, " graph has unknown number of triples\n");
  }


  /***** Test 5 *****/
  startTest(5, " Exec:  ARC  aa bb \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb");
    node=librdf_model_get_target(model, subject, predicate);
    librdf_free_node(subject);
    librdf_free_node(predicate);
    if(!node) {
      endTest(0, " Failed to get arc\n");
    } else {
      print_node(stdout, node);
      librdf_free_node(node);
      endTest(1, "\n");
    }
  }


  /***** Test 6 *****/
  startTest(6, " Exec:  ARCS  aa cc \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    object=librdf_new_node_from_uri_string(world, (const unsigned char*)"cc");
    iterator=librdf_model_get_arcs(model, subject, object);
    librdf_free_node(subject);
    librdf_free_node(object);
    if(!iterator) {
      endTest(0, " Failed to get arcs\n");
    } else {
      print_nodes(stdout, iterator);
      endTest(1, "\n");
    }
  }


  /***** Test 7 *****/
  startTest(7, " Exec:  ARCS-IN  cc \n");
  {
    object=librdf_new_node_from_uri_string(world, (const unsigned char*)"cc");
    iterator=librdf_model_get_arcs_in(model, object);
    librdf_free_node(object);
    if(!iterator) {
      endTest(0, " Failed to get arcs in\n");
    } else {
      int ok=1;
      count=0;
      while(!librdf_iterator_end(iterator)) {
        context_node=(librdf_node*)librdf_iterator_get_context(iterator);
        node=(librdf_node*)librdf_iterator_get_object(iterator); /*returns SHARED pointer */
        if(!node) {
          ok=0;
          endTest(ok, " librdf_iterator_get_next returned NULL\n");
          break;
        }

        fputs("Matched arc: ", stdout);
        librdf_node_print(node, stdout);
        if(context_node) {
          fputs(" with context ", stdout);
          librdf_node_print(context_node, stdout);
        }
        fputc('\n', stdout);

        count++;
        librdf_iterator_next(iterator);
      }
      librdf_free_iterator(iterator);
      endTest(ok, " matching arcs: %d\n", count);
    }
  }


  /***** Test 8 *****/
  startTest(8, " Exec:  ARCS-OUT  aa \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    iterator=librdf_model_get_arcs_out(model, subject);
    librdf_free_node(subject);
    if(!iterator)
      endTest(0, " Failed to get arcs out\n");
    else {
      int ok=1;
      count=0;
      while(!librdf_iterator_end(iterator)) {
        context_node=(librdf_node*)librdf_iterator_get_context(iterator);
        node=(librdf_node*)librdf_iterator_get_object(iterator);
        if(!node) {
          ok=0;
          endTest(ok, " librdf_iterator_get_next returned NULL\n");
          break;
        }

        fputs("Matched arc: ", stdout);
        librdf_node_print(node, stdout);
        if(context_node) {
          fputs(" with context ", stdout);
          librdf_node_print(context_node, stdout);
        }
        fputc('\n', stdout);

        count++;
        librdf_iterator_next(iterator);
      }
      librdf_free_iterator(iterator);
      endTest(ok, " matching arcs: %d\n", count);
    }
  }


  /***** Test 9 *****/
  startTest(9, " Exec:  CONTAINS aa bb1 cc \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb1");
    object=librdf_new_node_from_uri_string(world, (const unsigned char*)"cc");
    statement=librdf_new_statement(world);
    librdf_statement_set_subject(statement, subject);
    librdf_statement_set_predicate(statement, predicate);
    librdf_statement_set_object(statement, object);

    if(librdf_model_contains_statement(model, statement))
       endTest(1, " the graph contains the triple\n");
    else
       endTest(0, " the graph does not contain the triple\n");

    librdf_free_statement(statement);
  }


  /***** Test 10 *****/
  startTest(10, " Exec:  FIND aa - - \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    statement=librdf_new_statement(world);
    librdf_statement_set_subject(statement, subject);

    stream=librdf_model_find_statements_in_context(model, statement, context_node);

    if(!stream) {
        endTest(0, " FIND returned no results (NULL stream)\n");
    } else {
        librdf_node* ctxt_node=NULL;
        int ok=1;
        count=0;
        while(!librdf_stream_end(stream)) {
          librdf_statement *stmt=librdf_stream_get_object(stream);
          ctxt_node = librdf_stream_get_context2(stream);
          if(!stmt) {
              ok=0;
              endTest(ok, " librdf_stream_next returned NULL\n");
              break;
          }

          fputs("Matched triple: ", stdout);
          librdf_statement_print(stmt, stdout);
          if(context) {
              fputs(" with context ", stdout);
              librdf_node_print(ctxt_node, stdout);
          }
          fputc('\n', stdout);

          count++;
          librdf_stream_next(stream);
        }
        librdf_free_stream(stream);
        endTest(ok, " matching triples: %d\n", count);
    }

    librdf_free_statement(statement);
  }


  /***** Test 11 *****/
  startTest(11, " Exec:  HAS-ARC-IN cc bb \n");
  {
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb");
    object=librdf_new_node_from_uri_string(world, (const unsigned char*)"cc");

    if(librdf_model_has_arc_in(model, object, predicate))
        endTest(1, " the graph contains the arc\n");
      else
        endTest(0, " the graph does not contain the arc\n");

    librdf_free_node(predicate);
    librdf_free_node(object);
  }


  /***** Test 12 *****/
  startTest(12, " Exec:  HAS-ARC-OUT aa bb \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb");

    if(librdf_model_has_arc_out(model, subject, predicate))
        endTest(1, " the graph contains the arc\n");
      else
        endTest(0, " the graph does not contain the arc\n");

    librdf_free_node(predicate);
    librdf_free_node(subject);
  }


  /***** Test 13 *****/
  startTest(13, " Exec:  SOURCE  aa cc \n");
  {
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb");
    object=librdf_new_node_from_uri_string(world, (const unsigned char*)"cc");

    node=librdf_model_get_source(model, predicate, object);
    librdf_free_node(predicate);
    librdf_free_node(object);
    if(!node) {
      endTest(0, " Failed to get source\n");
    } else {
      print_node(stdout, node);
      librdf_free_node(node);
      endTest(1, "\n");
    }
  }


  /***** Test 14 *****/
  startTest(14, " Exec:  SOURCES  bb cc \n");
  {
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb");
    object=librdf_new_node_from_uri_string(world, (const unsigned char*)"cc");

    iterator=librdf_model_get_sources(model, predicate, object);
    librdf_free_node(predicate);
    librdf_free_node(object);
    if(!iterator) {
      endTest(0, " Failed to get sources\n");
    } else {
      print_nodes(stdout, iterator);
      endTest(1, "\n");
    }
  }


  /***** Test 15 *****/
  startTest(15, " Exec:  TARGET  aa bb \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb");

    node=librdf_model_get_target(model, subject, predicate);
    librdf_free_node(subject);
    librdf_free_node(predicate);
    if(!node) {
      endTest(0, " Failed to get target\n");
    } else {
      print_node(stdout, node);
      librdf_free_node(node);
      endTest(1, "\n");
    }
  }


  /***** Test 16 *****/
  startTest(16, " Exec:  TARGETS  aa bb \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb");

    iterator=librdf_model_get_targets(model, subject, predicate);
    librdf_free_node(subject);
    librdf_free_node(predicate);
    if(!iterator) {
      endTest(0, " Failed to get targets\n");
    } else {
      print_nodes(stdout, iterator);
      endTest(1, "\n");
    }
  }


  /***** Test 17 *****/
  startTest(17, " Exec:  REMOVE aa bb1 cc \n");
  {
    subject=librdf_new_node_from_uri_string(world, (const unsigned char*)"aa");
    predicate=librdf_new_node_from_uri_string(world, (const unsigned char*)"bb1");
    object=librdf_new_node_from_uri_string(world, (const unsigned char*)"cc");
    statement=librdf_new_statement(world);
    librdf_statement_set_subject(statement, subject);
    librdf_statement_set_predicate(statement, predicate);
    librdf_statement_set_object(statement, object);

    if(librdf_model_context_remove_statement(model, context_node, statement))
       endTest(0, " failed to remove triple from the graph\n");
    else
       endTest(1, " removed triple from the graph\n");

    librdf_free_statement(statement);
  }


  /***** Test 18 *****/
  query_cmd=(char *)"CONSTRUCT {?s ?p ?o} FROM <http://red> WHERE {?s ?p ?o}";
  startTest(18, " Exec:  QUERY \"%s\" \n", query_cmd);
  {
    query=librdf_new_query(world, (char *)"vsparql", NULL, (const unsigned char *)query_cmd, NULL);

    if(!(results=librdf_model_query_execute(model, query))) {
      endTest(0, " Query of model with '%s' failed\n", query_cmd);
      librdf_free_query(query);
      query=NULL;
    } else {
      stream=librdf_query_results_as_stream(results);

      if(!stream) {
        endTest(0, " QUERY returned no results (NULL stream)\n");
      } else {
        librdf_node* ctxt=NULL;
        count=0;
        while(!librdf_stream_end(stream)) {
          librdf_statement *stmt=librdf_stream_get_object(stream);  /*returns SHARED pointer */
          ctxt = librdf_stream_get_context2(stream);
          if(!stmt) {
              endTest(0, " librdf_stream_next returned NULL\n");
              break;
          }

          fputs("Matched triple: ", stdout);
          librdf_statement_print(stmt, stdout);
          if(ctxt) {
              fputs(" with context ", stdout);
              librdf_node_print(ctxt, stdout);
          }
          fputc('\n', stdout);

          count++;
          librdf_stream_next(stream);
        }
       librdf_free_stream(stream);

        endTest(1, " matching triples: %d\n", count);
        librdf_free_query_results(results);
      }
    }
    librdf_free_query(query);
  }


  /***** Test 19 *****/
  query_cmd=(char *)"SELECT * WHERE {graph <http://red> { ?s ?p ?o }}";
  startTest(19, " Exec1:  QUERY_AS_BINDINGS \"%s\" \n", query_cmd);
  {
    query=librdf_new_query(world, (char *)"vsparql", NULL, (const unsigned char *)query_cmd, NULL);

    if(!(results=librdf_model_query_execute(model, query))) {
        endTest(0, " Query of model with '%s' failed\n", query_cmd);
        librdf_free_query(query);
        query=NULL;
    } else {

        raptor_iostream *iostr;
        librdf_query_results_formatter *formatter;

        fprintf(stderr, "**: Formatting query result as '%s':\n", results_format);

        iostr = raptor_new_iostream_to_file_handle(librdf_world_get_raptor(world), stdout);
        formatter = librdf_new_query_results_formatter2(results, results_format,
                                                        NULL /* mime type */,
                                                        NULL /* format_uri */);

        base_uri = librdf_new_uri(world, (const unsigned char*)"http://example.org/");
        
        librdf_query_results_formatter_write(iostr, formatter, results, base_uri);
        librdf_free_query_results_formatter(formatter);
        raptor_free_iostream(iostr);
        librdf_free_uri(base_uri);

        endTest(1, "\n");
        librdf_free_query_results(results);
    }
    librdf_free_query(query);
  }


  /***** Test 20 *****/
  query_cmd=(char *)"SELECT * WHERE {graph <http://red> { ?s ?p ?o }}";
  startTest(20, " Exec2:  QUERY_AS_BINDINGS \"%s\" \n", query_cmd);
  {
    query=librdf_new_query(world, (char *)"vsparql", NULL, (const unsigned char *)query_cmd, NULL);

    if(!(results=librdf_model_query_execute(model, query))) {
       endTest(0, " Query of model with '%s' failed\n", query_cmd);
       librdf_free_query(query);
       query=NULL;
    } else {
       if(print_query_results(world, model, results))
         endTest(0, "\n");
       else
         endTest(1, "\n");

       librdf_free_query_results(results);
    }
    librdf_free_query(query);
  }

  getTotal();


  if(transactions)
    librdf_model_transaction_commit(model);

  librdf_free_node(context_node);
  librdf_free_node(context_node);
  librdf_free_hash(options);
  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_free_world(world);

#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
