/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example.c - Redland example parsing RDF from a URI, storing on disk as BDB hashes and querying the results
 *
 * $Id$
 *
 * Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology, University of Bristol.
 *
 *    This package is Free Software available under either of two licenses
 *    (see FAQS.html to see why):
 * 
 * 1. The GNU Lesser General Public License (LGPL)
 * 
 *    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
 *    full license text.
 *      _________________________________________________________________
 * 
 *      Copyright (C) 2000 David Beckett, Institute for Learning and
 *      Research Technology, University of Bristol. All Rights Reserved.
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public License
 *      as published by the Free Software Foundation; either version 2 of
 *      the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *      USA
 *      _________________________________________________________________
 * 
 *    NOTE - under Term 3 of the LGPL, you may choose to license the entire
 *    library under the GPL. See COPYING for the full license text.
 * 
 * 2. The Mozilla Public License
 * 
 *    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
 *    license text.
 * 
 *    Under MPL section 13. I declare that all of the Covered Code is
 *    Multiple Licensed:
 *      _________________________________________________________________
 * 
 *      The contents of this file are subject to the Mozilla Public License
 *      version 1.1 (the "License"); you may not use this file except in
 *      compliance with the License. You may obtain a copy of the License
 *      at http://www.mozilla.org/MPL/
 * 
 *      Software distributed under the License is distributed on an "AS IS"
 *      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 *      the License for the specific language governing rights and
 *      limitations under the License.
 * 
 *      The Initial Developer of the Original Code is David Beckett.
 *      Portions created by David Beckett are Copyright (C) 2000 David
 *      Beckett, Institute for Learning and Research Technology, University
 *      of Bristol. All Rights Reserved.
 * 
 *      Alternatively, the contents of this file may be used under the
 *      terms of the GNU Lesser General Public License, in which case the
 *      provisions of the LGPL License are applicable instead of those
 *      above. If you wish to allow use of your version of this file only
 *      under the terms of the LGPL License and not to allow others to use
 *      your version of this file under the MPL, indicate your decision by
 *      deleting the provisions above and replace them with the notice and
 *      other provisions required by the LGPL License. If you do not delete
 *      the provisions above, a recipient may use your version of this file
 *      under either the MPL or the LGPL License.
 */


#include <stdio.h>
#include <string.h>

#include <redland.h>


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

  storage=librdf_new_storage("hashes", "test", "hash_type='bdb',dir='.'");
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
    fprintf(stderr, "%s: Failed to parse RDF into model\n", program);
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
  if(!stream) {
    fprintf(stderr, "%s: librdf_model_find_statements returned NULL stream\n", program);
  } else {
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
  }


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
