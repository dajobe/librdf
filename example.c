/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example.c - Example code using RDF parser
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


#include <stdio.h>
#include <string.h>

#include <librdf.h>


/* one prototype needed */
int main(int argc, char *argv[]);



int
main(int argc, char *argv[]) 
{
  librdf_storage* storage;
  librdf_parser* parser;
  librdf_model* model;
  librdf_stream* stream;
  librdf_node *subject, *predicate;
  librdf_iterator* iterator;
  librdf_statement *partial_statement;
  char *program=argv[0];
  librdf_uri *uri;
  char *parser_name=NULL;
  int count;

  if(argc <2 || argc >3) {
    fprintf(stderr, "USAGE: %s: <RDF source URI> [rdf parser name]\n", program);
    return(1);
  }


  librdf_init_world(NULL, NULL);

  uri=librdf_new_uri(argv[1]);
  if(!uri) {
    fprintf(stderr, "%s: Failed to create URI\n", program);
    return(1);
  }

  storage=librdf_new_storage(NULL, NULL);
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }

  model=librdf_new_model(storage, NULL);
  if(!model) {
    fprintf(stderr, "%s: Failed to create model\n", program);
    return(1);
  }
  
      
  if(argc==3)
    parser_name=argv[2];

  parser=librdf_new_parser(parser_name);
  if(!parser) {
    fprintf(stderr, "%s: Failed to create new parser\n", program);
    return(1);
  }


  /* PARSE the URI as RDF/XML*/
  fprintf(stdout, "%s: Parsing URI %s\n", program, librdf_uri_as_string(uri));
  if(librdf_parser_parse_into_model(parser, uri, model)) {
    fprintf(stderr, "%s: Failed to parser into model\n", program);
    return(1);
  }
  librdf_free_parser(parser);


  /* Print out the model*/

  fprintf(stdout, "%s: Resulting model is:\n", program);
  librdf_model_print(model, stdout);


  /* Construct the query predicate (arc) and object (target) 
   * and partial statement bits
   */
  subject=librdf_new_node_from_uri_string("http://www.ilrt.bristol.ac.uk/people/cmdjb/");
  predicate=librdf_new_node_from_uri_string("http://purl.org/dc/elements/1.1/title");
  if(!subject || !predicate) {
    fprintf(stderr, "%s: Failed to create nodes for searching\n", program);
    return(1);
  }
  partial_statement=librdf_new_statement();
  librdf_statement_set_subject(partial_statement, subject);
  librdf_statement_set_predicate(partial_statement, predicate);


  /* QUERY TEST 1 - use find_statements to match */

  fprintf(stdout, "%s: Trying to find_statements\n", program);
  stream=librdf_model_find_statements(model, partial_statement);
  count=0;
  while(!librdf_stream_end(stream)) {
    librdf_statement *statement=librdf_stream_next(stream);
    if(!statement) {
      fprintf(stderr, "%s: librdf_stream_next returned NULL\n", program);
      break;
    }

    fputs("  Matched statement: ", stdout);
    librdf_statement_print(statement, stdout);
    fputc('\n', stdout);

    librdf_free_statement(statement);
    count++;
  }
  librdf_free_stream(stream);  
  fprintf(stderr, "%s: got %d matching statements\n", program, count);


  /* QUERY TEST 2 - use get_targets to do match */
  fprintf(stdout, "%s: Trying to get targets\n", program);
  iterator=librdf_model_get_targets(model, subject, predicate);
  if(!iterator)  {
    fprintf(stderr, "%s: librdf_model_get_targets failed to return iterator for searching\n", program);
    return(1);
  }

  count=0;
  while(librdf_iterator_have_elements(iterator)) {
    librdf_node *target;
    
    target=librdf_iterator_get_next(iterator);
    if(!target) {
      fprintf(stderr, "%s: librdf_iterator_get_next returned NULL\n", program);
      break;
    }

    fputs("  Matched target: ", stdout);
    librdf_node_print(target, stdout);
    fputc('\n', stdout);

    librdf_free_node(target);
    count++;
  }
  librdf_free_iterator(iterator);
  fprintf(stderr, "%s: got %d target nodes\n", program, count);

  librdf_free_statement(partial_statement);
  /* the above does this since they are still attached */
  /* librdf_free_node(predicate); */
  /* librdf_free_node(object); */

  librdf_free_model(model);

  librdf_free_storage(storage);

  librdf_free_uri(uri);

  librdf_destroy_world();

#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
