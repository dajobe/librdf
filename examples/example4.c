/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example4.c - Redland example code querying stored triples
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
#include <ctype.h>
#include <string.h>

#include <redland.h>

/* one prototype needed */
int main(int argc, char *argv[]);


enum command_type {
  CMD_PRINT,
  CMD_STATEMENTS,
  CMD_SOURCES,
  CMD_ARCS,
  CMD_TARGETS
};

typedef struct
{
  enum command_type type;
  char *name;
  int nargs; /* how many args needed? */
} command;

static command commands[]={
  {CMD_PRINT, "print", 0},
  {CMD_STATEMENTS, "statements", 3},
  {CMD_SOURCES, "sources", 2},
  {CMD_ARCS, "arcs", 2},
  {CMD_TARGETS, "targets", 2},
  {-1, NULL}  
};
 

  


int
main(int argc, char *argv[]) 
{
  char *program=argv[0];
  librdf_storage *storage;
  librdf_model* model;
  librdf_statement* partial_statement;
  librdf_node *source, *arc, *target;
  librdf_stream* stream;
  librdf_iterator* iterator;
  int usage;
  char *identifier;
  int cmd;
  char *query;
  int count;
  

  librdf_init_world(NULL, NULL);


  /* Skip program name */
  argv++;
  argc--;


  usage=0;
  cmd= -1;
  query=NULL; /* will be set except for PRINT */
  identifier=NULL;
  
  if(!argc) {
    /* no identifier given at all */
    fprintf(stderr, "%s: No identifier given\n", program);
    usage=1;
  } else {
    identifier=argv[0];
    argv++;
    argc--;

    if(!argc) {
      fprintf(stderr, "%s: No command given\n", program);
      usage=1;
    } else {
      int i;
      
      for(i=0; commands[i].name; i++)
        if(!strcmp(argv[0], commands[i].name)) {
          cmd=i;
          break;
        }
      if(cmd<0) {
        fprintf(stderr, "%s: No such command %s\n", program, argv[0]);
        usage=1;
      } else {
        argv++;
        argc--;

        if(argc < commands[cmd].nargs) {
          fprintf(stderr, "%s: command %s needs %d arguments\n", program, 
                  commands[cmd].name, commands[cmd].nargs);
          usage=1;
        } else if(argc > commands[cmd].nargs) {
          fprintf(stderr, "%s: command %s given extra arguments\n", program,
                  commands[cmd].name);
          usage=1;
        } /* otherwise is just fine and argv points to remaining args */
      }
    }
  }
  

  
  if(usage) {
    fprintf(stdout, "USAGE: %s: <BDB name> COMMANDS\n", program);
    fprintf(stdout, "  print\n    Prints all the statements\n");
    fprintf(stdout, "  statements SUBJECT|- PREDICATE|- OBJECT|-\n    Query for matching statements\n    where - is used to match anything.\n");
    fprintf(stdout, "  sources | targets | arcs NODE1 NODE2\n    Query for matching nodes\n");
    return(1);
  }


  storage=librdf_new_storage("hashes", identifier, "hash-type='bdb',dir='.',write='no'");
  if(!storage) {
    fprintf(stderr, "%s: Failed to open storage (read only)\n", program);
    return(1);
  }

  model=librdf_new_model(storage, NULL);
  if(!model) {
    fprintf(stderr, "%s: Failed to create model\n", program);
    return(1);
  }


  /* Do this or gcc moans */
  stream=NULL;
  iterator=NULL;
  source=NULL;
  arc=NULL;
  target=NULL;
  
  switch(commands[cmd].type) {
  case CMD_PRINT:
    librdf_model_print(model, stdout);
    break;
  case CMD_STATEMENTS:
    if(!strcmp(argv[0], "-"))
      source=NULL;
    else
      source=librdf_new_node_from_uri_string(argv[0]);

    if(!strcmp(argv[1], "-"))
      arc=NULL;
    else
      arc=librdf_new_node_from_uri_string(argv[1]);

    if(!strcmp(argv[2], "-"))
      target=NULL;
    else {
      if(librdf_heuristic_object_is_literal(argv[2]))
        target=librdf_new_node_from_literal(argv[2], NULL, 0, 0);
      else
        target=librdf_new_node_from_uri_string(argv[2]);
    }
    
    partial_statement=librdf_new_statement();
    if(source)
      librdf_statement_set_subject(partial_statement, source);
    if(arc)
      librdf_statement_set_predicate(partial_statement, arc);
    if(target)
      librdf_statement_set_object(partial_statement, target);

    /* Print out matching statements */
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
        
        fputs("Matched statement: ", stdout);
        librdf_statement_print(statement, stdout);
        fputc('\n', stdout);
        
        librdf_free_statement(statement);
        count++;
      }
      librdf_free_stream(stream);  
      fprintf(stderr, "%s: got %d matching statements\n", program, count);
    }
    /* also frees the nodes */
    librdf_free_statement(partial_statement);

    break;
  case CMD_SOURCES:
    arc=librdf_new_node_from_uri_string(argv[0]);
    if(librdf_heuristic_object_is_literal(argv[1]))
      target=librdf_new_node_from_literal(argv[1], NULL, 0, 0);
    else
      target=librdf_new_node_from_uri_string(argv[1]);

    iterator=librdf_model_get_sources(model, arc, target);
    if(!iterator) {
      fprintf(stderr, "Failed to get sources\n");
      break;
    }

    /* FALLTHROUGH */
  case CMD_ARCS:
    if(!iterator) {
      source=librdf_new_node_from_uri_string(argv[0]);
      if(librdf_heuristic_object_is_literal(argv[1]))
        target=librdf_new_node_from_literal(argv[1], NULL, 0, 0);
      else
        target=librdf_new_node_from_uri_string(argv[1]);
      iterator=librdf_model_get_arcs(model, source, target);
      if(!iterator) {
        fprintf(stderr, "Failed to get arcs\n");
        break;
      }
    }
    
    /* FALLTHROUGH */
  case CMD_TARGETS:
    if(!iterator) {
      source=librdf_new_node_from_uri_string(argv[0]);
      arc=librdf_new_node_from_uri_string(argv[1]);
      iterator=librdf_model_get_targets(model, source, arc);
      if(!iterator) {
        fprintf(stderr, "Failed to get targets\n");
        break;
      }
    }

    /* (Common code) Print out nodes */
    count=0;
    while(librdf_iterator_have_elements(iterator)) {
      librdf_node *node;
      
      node=librdf_iterator_get_next(iterator);
      if(!node) {
        fprintf(stderr, "%s: librdf_iterator_get_next returned NULL\n", program);
        break;
      }
      
      fputs("Matched node: ", stdout);
      librdf_node_print(node, stdout);
      fputc('\n', stdout);
      
      librdf_free_node(node);
      count++;
    }
    librdf_free_iterator(iterator);
    fprintf(stderr, "%s: got %d matching nodes\n", program, count);

    if(source)
      librdf_free_node(source);
    if(arc)
      librdf_free_node(arc);
    if(target)
      librdf_free_node(target);
    break;
  default:
    fprintf(stderr, "Unknown command number %d\n", cmd);
    return(1);
  }


  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_destroy_world();

#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
