/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example8.c - Redland example code SPARQL querying and formatting
 *
 * Copyright (C) 2010, David Beckett http://www.dajobe.org/
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
 * USAGE:
 *   To get json variable bindings
 *     ./example8 http://librdf.org/redland.rdf json 'select * where { ?s ?p ?o}'
 *   To get json triples (graph formatted):
 *     ./example8 http://librdf.org/redland.rdf json 'construct {?s ?p ?o} where { ?s ?p ?o}'
 * 
 */


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include <redland.h>

/* one prototype needed */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  char *program = argv[0];
  librdf_world* world;
  librdf_storage *storage;
  librdf_model* model = NULL;
  const char *parser_name;
  librdf_parser* parser;
  librdf_query* query;
  librdf_query_results* results;
  librdf_uri *uri;
  const unsigned char *query_string = NULL;
  const char *format;
  unsigned char* string = NULL;
  size_t length;
  
  world = librdf_new_world();
  librdf_world_open(world);

  if(argc != 4) {
    fprintf(stderr, "USAGE: %s DATA-URI FORMAT|- QUERY-STRING\n", program);
    return 1; 
  }
  
  uri = librdf_new_uri(world, (const unsigned char*)argv[1]);
  format = argv[2];
  if(!strcmp(format, "-"))
    format = NULL;
  query_string = (const unsigned char*)argv[3];
  
  storage = librdf_new_storage(world, "memory", "test", NULL);
  if(storage)
    model = librdf_new_model(world, storage, NULL);

  if(!model || !storage) {
    fprintf(stderr, "%s: Failed to make model or storage\n", program);
    return 1;
  }

  parser_name = librdf_parser_guess_name2(world, NULL, NULL, librdf_uri_as_string(uri));
  parser = librdf_new_parser(world, parser_name, NULL, NULL);
  librdf_parser_parse_into_model(parser, uri, NULL, model);
  librdf_free_parser(parser);
  librdf_free_uri(uri);

  query = librdf_new_query(world, "sparql", NULL, query_string, NULL);
  
  results = librdf_model_query_execute(model, query);
  if(!results) {
    fprintf(stderr, "%s: Query of model with '%s' failed\n", 
            program, query_string);
    return 1;
  }

  if(librdf_query_results_is_bindings(results) ||
     librdf_query_results_is_boolean(results)) {
    string = librdf_query_results_to_counted_string2(results, format,
               NULL, NULL, NULL, &length);
    if(!string) {
      fprintf(stderr, "%s: Failed to turn query results to format '%s'\n",
              program, format);
      return 1;
    }

  } else {
    /* triples */
    librdf_serializer* serializer;
    librdf_stream* stream;

    serializer = librdf_new_serializer(world, format, NULL, NULL);
    if(!serializer) {
      fprintf(stderr,
              "%s: Failed to create triples serializer for format '%s'\n",
              program, format);
      return 1;
    }

    stream = librdf_query_results_as_stream(results);
    string = librdf_serializer_serialize_stream_to_counted_string(serializer,
               NULL, stream, &length);
    librdf_free_stream(stream);

    if(!string) {
      fprintf(stderr, "%s: Failed to turn query results to format '%s'\n",
              program, format);
      return 1;
    }
  }

  fprintf(stderr, "%s: query returned %ld bytes string\n", program, length);
  fwrite(string, sizeof(char), length, stdout);

  free(string);

  librdf_free_query_results(results);
  librdf_free_query(query);
  
  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_free_world(world);

  /* keep gcc -Wall happy */
  return(0);
}
