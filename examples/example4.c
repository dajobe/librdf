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
  CMD_CONTAINS,
  CMD_STATEMENTS,
  CMD_SOURCES,
  CMD_ARCS,
  CMD_TARGETS,
  CMD_ADD,
  CMD_REMOVE,
  CMD_PARSE_MODEL,
  CMD_PARSE_STREAM,
};

typedef struct
{
  enum command_type type;
  char *name;
  int min_args; /* min args needed? */
  int max_args; /* max args needed? */
  int write; /* write to db? */
} command;

static command commands[]={
  {CMD_PRINT, "print", 0, 0, 0,},
  {CMD_CONTAINS, "contains", 3, 3, 0},
  {CMD_STATEMENTS, "statements", 3, 3, 0},
  {CMD_SOURCES, "sources", 2, 2, 0},
  {CMD_ARCS, "arcs", 2, 2, 0},
  {CMD_TARGETS, "targets", 2, 2, 0},
  {CMD_ADD, "add", 3, 3, 1},
  {CMD_REMOVE, "remove", 3, 3, 1},
  {CMD_PARSE_MODEL, "parse", 2, 3, 1},
  {CMD_PARSE_STREAM, "parse-stream", 2, 3, 1},
  {-1, NULL}  
};
 

  


int
main(int argc, char *argv[]) 
{
  char *program=argv[0];
  librdf_parser* parser;
  librdf_storage *storage;
  librdf_model* model;
  librdf_statement* partial_statement;
  librdf_node *source, *arc, *target;
  librdf_stream* stream;
  librdf_iterator* iterator;
  librdf_uri *uri;
  librdf_uri *base_uri;
  int usage;
  char *identifier;
  char *cmd;
  int cmd_index;
  int count;
  int type;

  librdf_init_world(NULL, NULL);


  /* Skip program name */
  argv++;
  argc--;


  /* keep gcc happy */
  usage=0;
  cmd=NULL;
  cmd_index= -1;
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

      cmd=argv[0];
      
      for(i=0; commands[i].name; i++)
        if(!strcmp(cmd, commands[i].name)) {
          cmd_index=i;
          break;
        }
      if(cmd_index<0) {
        fprintf(stderr, "%s: No such command %s\n", program, argv[0]);
        usage=1;
      } else {
        argv++;
        argc--;

        if(argc < commands[cmd_index].min_args) {
          fprintf(stderr, "%s: command %s needs %d arguments\n", program, 
                  cmd, commands[cmd_index].min_args);
          usage=1;
        } else if(argc > commands[cmd_index].max_args) {
          fprintf(stderr, "%s: command %s given more than %d arguments\n",
                  program, cmd, commands[cmd_index].max_args);
          usage=1;
        } /* otherwise is just fine and argv points to remaining args */
      }
    }
  }
  

  
  if(usage) {
    fprintf(stdout, "USAGE: %s: <storage name> COMMANDS\n", program);
    fprintf(stdout, "  parse URI PARSER [BASE URI]               Parse the RDF/XML at URI into\n");
    fprintf(stdout, "                                            the model using PARSER\n");
    fprintf(stdout, "  print                                     Prints all the statements\n");
    fprintf(stdout, "  statements SUBJECT|- PREDICATE|- OBJECT|- Query for matching statements\n");
    fprintf(stdout, "                                            where - matches any node.\n");
    fprintf(stdout, "  contains SUBJECT PREDICATE OBJECT         Check if statement is in the model.\n");
    fprintf(stdout, "  add | remove SUBJECT PREDICATE OBJECT     Add/remove statement to/from model.\n");
    fprintf(stdout, "  sources | targets | arcs NODE1 NODE2      Query for matching nodes\n");
    return(1);
  }

  type=commands[cmd_index].type;
  

  if(commands[cmd_index].write)
    if (type == CMD_PARSE_MODEL || type == CMD_PARSE_STREAM)
      storage=librdf_new_storage("hashes", identifier, "hash-type='bdb',dir='.',write='yes',new='yes'");
    else
      storage=librdf_new_storage("hashes", identifier, "hash-type='bdb',dir='.',write='yes'");
  else
    storage=librdf_new_storage("hashes", identifier, "hash-type='bdb',dir='.',write='no'");

  if(!storage) {
    fprintf(stderr, "%s: Failed to open BDB storage\n", program);
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
  parser=NULL;
  source=NULL;
  arc=NULL;
  target=NULL;
  uri=NULL;

  switch(type) {
  case CMD_PRINT:
    librdf_model_print(model, stdout);
    break;
  case CMD_PARSE_MODEL:
  case CMD_PARSE_STREAM:
    uri=librdf_new_uri(argv[0]);
    if(!uri) {
      fprintf(stderr, "%s: Failed to create URI from %s\n", program, argv[0]);
      break;
    }

    parser=librdf_new_parser(argv[1], NULL, NULL);
    if(!parser) {
      fprintf(stderr, "%s: Failed to create new parser %s\n", program, argv[1]);
      librdf_free_uri(uri);
      break;
    }
    fprintf(stdout, "%s: Parsing URI %s with %s parser\n", program,
            librdf_uri_as_string(uri), argv[1]);
    
    if(argv[2]) {
      base_uri=librdf_new_uri(argv[2]);
      if(!base_uri) {
        fprintf(stderr, "%s: Failed to create base URI from %s\n", program, argv[2]);
        break;
      }
      fprintf(stderr, "%s: Using base URI %s\n", program,
              librdf_uri_as_string(base_uri));
    } else
      base_uri=librdf_new_uri_from_uri(uri);


    if (type == CMD_PARSE_MODEL) {
      librdf_parser_set_feature(parser, LIBRDF_MS_aboutEach_URI, "yes");
      librdf_parser_set_feature(parser, LIBRDF_MS_aboutEachPrefix_URI, "yes");
      if(librdf_parser_parse_into_model(parser, uri, base_uri, model)) {
        fprintf(stderr, "%s: Failed to parse RDF into model\n", program);
        librdf_free_parser(parser);
        librdf_free_uri(uri);
        librdf_free_uri(base_uri);
        break;
      }
    } else {
      /* must be CMD_PARSE_STREAM */
      librdf_parser_set_feature(parser, LIBRDF_MS_aboutEach_URI, "no");
      librdf_parser_set_feature(parser, LIBRDF_MS_aboutEachPrefix_URI, "no");
      if(!(stream=librdf_parser_parse_as_stream(parser, uri, base_uri))) {
        fprintf(stderr, "%s: Failed to parse RDF as stream\n", program);
        librdf_free_parser(parser);
        librdf_free_uri(uri);
        librdf_free_uri(base_uri);
        break;
      }

      count=0;
      while(!librdf_stream_end(stream)) {
        librdf_statement *statement=librdf_stream_next(stream);
        if(!statement) {
          fprintf(stderr, "%s: librdf_stream_next returned NULL\n", program);
          break;
        }
          
        librdf_model_add_statement(model, statement);
        count++;
      }
      librdf_free_stream(stream);  
      librdf_free_parser(parser);
      librdf_free_uri(uri);
      librdf_free_uri(base_uri);
      fprintf(stderr, "%s: Added %d statements\n", program, count);
      break;
         
    }
    
    librdf_free_parser(parser);
    librdf_free_uri(uri);
    librdf_free_uri(base_uri);
    break;
  case CMD_CONTAINS:
  case CMD_STATEMENTS:
  case CMD_ADD:
  case CMD_REMOVE:
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
    librdf_statement_set_subject(partial_statement, source);
    librdf_statement_set_predicate(partial_statement, arc);
    librdf_statement_set_object(partial_statement, target);

    if(type != CMD_STATEMENTS) {
      if(!source || !arc || !target) {
        fprintf(stderr, "%s: cannot have missing statement parts for %s\n", program, cmd);
        librdf_free_statement(partial_statement);
        break;
      }
    }

    switch(type) {
      case CMD_CONTAINS:
        if(librdf_model_contains_statement(model, partial_statement))
          fprintf(stdout, "%s: the model contains the statement\n", program);
        else
          fprintf(stdout, "%s: the model does not contain the statement\n", program);
        break;

    case CMD_STATEMENTS:
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
        fprintf(stderr, "%s: matching statements: %d\n", program, count);
      }
      break;
      
    case CMD_ADD:
      if(librdf_model_add_statement(model, partial_statement))
        fprintf(stdout, "%s: failed to add statement to model\n", program);
      else
        fprintf(stdout, "%s: added statement to model\n", program);
      break;

    case CMD_REMOVE:
      if(librdf_model_remove_statement(model, partial_statement))
        fprintf(stdout, "%s: failed to remove statement from model\n", program);
      else
        fprintf(stdout, "%s: removed statement from model\n", program);
      break;

    default:
      fprintf(stderr, "Unexpected command %d\n", type);

    } /* end inner switch */

    /* also frees the nodes */
    if(type != CMD_ADD)
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
      fprintf(stderr, "%s: Failed to get sources\n", program);
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
        fprintf(stderr, "%s: Failed to get targets\n", program);
        break;
      }
    }

    /* (Common code) Print out nodes */
    count=0;
    while(librdf_iterator_have_elements(iterator)) {
      librdf_node *node;
      
      node=librdf_iterator_get_next(iterator);
      if(!node) {
        fprintf(stderr, "%s: librdf_iterator_get_next returned NULL\n",
                program);
        break;
      }
      
      fputs("Matched node: ", stdout);
      librdf_node_print(node, stdout);
      fputc('\n', stdout);
      
      librdf_free_node(node);
      count++;
    }
    librdf_free_iterator(iterator);
    fprintf(stderr, "%s: matching nodes: %d\n", program, count);

    if(source)
      librdf_free_node(source);
    if(arc)
      librdf_free_node(arc);
    if(target)
      librdf_free_node(target);
    break;
  default:
    fprintf(stderr, "%s: Unknown command %d\n", program, type);
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
