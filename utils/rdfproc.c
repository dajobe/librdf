/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdfproc.c - Redland RDF processor
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


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include <rdfproc_getopt.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <unistd.h>

#include <redland.h>

#ifdef NEED_OPTIND_DECLARATION
extern int optind;
extern char *optarg;
#endif

/* one prototype needed */
int main(int argc, char *argv[]);

static  char *program=NULL;


enum command_type {
  CMD_PRINT,
  CMD_CONTAINS,
  CMD_FIND,
  CMD_SOURCES,
  CMD_ARCS,
  CMD_TARGETS,
  CMD_SOURCE,
  CMD_ARC,
  CMD_TARGET,
  CMD_ADD,
  CMD_REMOVE,
  CMD_ADD_TYPED,
  CMD_PARSE_MODEL,
  CMD_PARSE_STREAM,
  CMD_ARCS_IN,
  CMD_ARCS_OUT,
  CMD_HAS_ARC_IN,
  CMD_HAS_ARC_OUT,
  CMD_QUERY,
  CMD_SERIALIZE
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
  {CMD_FIND, "find", 3, 3, 0},
  {CMD_SOURCES, "sources", 2, 2, 0},
  {CMD_ARCS, "arcs", 2, 2, 0},
  {CMD_TARGETS, "targets", 2, 2, 0},
  {CMD_SOURCE, "source", 2, 2, 0},
  {CMD_ARC, "arc", 2, 2, 0},
  {CMD_TARGET, "target", 2, 2, 0},
  {CMD_ADD, "add", 3, 4, 1},
  {CMD_REMOVE, "remove", 3, 4, 1},
  {CMD_ADD_TYPED, "add-typed", 5, 6, 1},
  {CMD_PARSE_MODEL, "parse", 2, 3, 1},
  {CMD_PARSE_STREAM, "parse-stream", 2, 3, 1},
  {CMD_ARCS_IN, "arcs-in", 1, 1, 0},
  {CMD_ARCS_OUT, "arcs-out", 1, 1, 0},
  {CMD_HAS_ARC_IN, "has-arc-in", 2, 2, 0},
  {CMD_HAS_ARC_OUT, "has-arc-out", 2, 2, 0},
  {CMD_QUERY, "query", 3, 3, 0},
  {CMD_SERIALIZE, "serialize", 0, 3, 0},
  {(enum command_type)-1, NULL}  
};
 

#ifdef HAVE_GETOPT_LONG
#define HELP_TEXT(short, long, description) "  -" #short ", --" long "  " description "\n"
#define HELP_ARG(short, long) "--" #long
#else
#define HELP_TEXT(short, long, description) "  -" #short "  " description "\n"
#define HELP_ARG(short, long) "-" #short
#endif


#define GETOPT_STRING "chv"

#ifdef HAVE_GETOPT_LONG
static struct option long_options[] =
{
  /* name, has_arg, flag, val */
  {"contexts", 0, 0, 'c'},
  {"help", 0, 0, 'h'},
  {"version", 0, 0, 'v'},
  {NULL, 0, 0, 0}
};
#endif

static const char *title_format_string="Redland RDF processor utility %s\n";

  


int
main(int argc, char *argv[]) 
{
  librdf_world* world;
  librdf_parser* parser;
  librdf_serializer* serializer;
  librdf_storage *storage;
  librdf_model* model;
  librdf_statement* partial_statement;
  librdf_node *source, *arc, *target, *node;
  librdf_stream* stream;
  librdf_iterator* iterator;
  librdf_uri *uri;
  librdf_uri *base_uri;
  librdf_query *query;
  librdf_hash *options;
  int usage=0;
  int help=0;
  int contexts=0;
  char *identifier=NULL;
  char *cmd=NULL;
  char *name;
  char *mime_type;
  int cmd_index= -1;
  int count;
  int type;
  int result;
  char *p;
  int i;
  int rc;

  program=argv[0];
  if((p=strrchr(program, '/')))
    program=p+1;
  else if((p=strrchr(program, '\\')))
    program=p+1;
  argv[0]=program;

  world=librdf_new_world();
  librdf_world_open(world);

  while (!usage && !help)
  {
    int c;
#ifdef HAVE_GETOPT_LONG
    int option_index = 0;

    c = getopt_long (argc, argv, GETOPT_STRING, long_options, &option_index);
#else
    c = getopt (argc, argv, GETOPT_STRING);
#endif
    if (c == -1)
      break;

    switch (c) {
      case 0:
      case '?': /* getopt() - unknown option */
        usage=1;
        break;
        
      case 'c':
        contexts=1;
        break;

      case 'h':
        help=1;
        break;

      case 'v':
        fputs(librdf_version_string, stdout);
        fputc('\n', stdout);
        exit(0);
    }
    
  }

  if(usage || help)
    goto usage;

  if(optind == argc) {
    usage=2; /* Title and usage */
    goto usage;
  }

  argv+=optind;
  argc-=optind;
  
  identifier=argv[0];
  argv++;
  argc--;

  if(!argc) {
    fprintf(stderr, "%s: No command given\n", program);
    usage=1;
    goto usage;
  }
  
  cmd=argv[0];
  
  for(i=0; commands[i].name; i++)
    if(!strcmp(cmd, commands[i].name)) {
      cmd_index=i;
      break;
    }
  if(cmd_index<0) {
    fprintf(stderr, "%s: No such command %s\n", program, argv[0]);
    usage=1;
    goto usage;
  }
    
  argv++;
  argc--;
    
  if(argc < commands[cmd_index].min_args) {
    fprintf(stderr, "%s: Command %s needs %d arguments\n", program, 
            cmd, commands[cmd_index].min_args);
    usage=1;
  } else if(argc > commands[cmd_index].max_args) {
    fprintf(stderr, "%s: Command %s given more than %d arguments\n",
            program, cmd, commands[cmd_index].max_args);
    usage=1;
  } /* otherwise is just fine and argv points to remaining args */


  usage:
  if(usage) {
    if(usage>1)
      fprintf(stderr, title_format_string, librdf_version_string);
    fprintf(stderr, "Try `%s " HELP_ARG(h, help) "' for more information.\n",
                    program);
    exit(1);
  }

  if(help) {
    printf("Usage: %s [OPTIONS] <store> COMMANDS\n", program);
    printf(title_format_string, librdf_version_string);
    printf("Process RDF.\n");
    printf("\nMain options:\n");
    printf(HELP_TEXT(c, "contexts        ", "Use store with Redland contexts"));
    printf(HELP_TEXT(h, "help            ", "Print this help, then exit"));
    printf(HELP_TEXT(v, "version         ", "Print the Redland version"));
    printf("\nCommands:\n");
    printf("  parse URI PARSER [BASE URI]               Parse syntax at URI into\n");
    printf("                                            the model using PARSER\n");
    printf("  parse-stream URI PARSER [BASE URI]        Parse URI as stream using PARSER.\n");
    printf("  print                                     Prints all the statements\n");
    printf("  serialize [NAME [URI [MIME-TYPE]]]        Serializes to a syntax (RDF/XML)\n");
#if 0
    /* Not a query -> results table query, so leave out for now */
    printf("  query NAME URI|- QUERY-STRING             Query for matching statements\n");
#endif
    printf("  find SUBJECT|- PREDICATE|- OBJECT|-       Find matching statements\n");
    printf("                                            where - matches any node.\n");
    printf("  contains SUBJECT PREDICATE OBJECT         Check if statement is in the model.\n");
    printf("  add | remove SUBJECT PREDICATE OBJECT [CONTEXT] Add/remove statement to/from model.\n");
    printf("  add-typed SUBJECT PREDICATE OBJECT OBJECT-LANG OBJECT-URI [CONTEXT]  Add datatyped statement to model.\n");
    printf("  sources | targets | arcs NODE1 NODE2      Query for matching nodes\n");
    printf("  source | target | arc NODE1 NODE2         Query for 1 matching node\n");
    printf("  arcs-in | arcs-out NODE                   Show properties in/out of NODE\n");
    printf("  has-arc-in | has-arc-out NODE ARC         Test for property in/out of NODE\n");
    printf("\nReport bugs to <redland-dev@lists.librdf.org>.\n");
    printf("Redland home page: http://www.redland.opensource.ac.uk/\n");
    exit(0);
  }
  
  type=commands[cmd_index].type;

  options=librdf_new_hash(world, NULL);
  librdf_hash_put_strings(options, "hash-type", "bdb");
  librdf_hash_put_strings(options, "dir", ".");
  if(contexts)
    librdf_hash_put_strings(options, "contexts", "yes");

  if(commands[cmd_index].write) {
    librdf_hash_put_strings(options, "write", "yes");
    if (type == CMD_PARSE_MODEL || type == CMD_PARSE_STREAM)
      librdf_hash_put_strings(options, "new", "yes");
  }
  
  storage=librdf_new_storage_with_options(world, "hashes", identifier, 
                                          options);

  if(!storage) {
    fprintf(stderr, "%s: Failed to open BerkelyDB storage %s\n", program,
            identifier);
    return(1);
  }

  model=librdf_new_model(world, storage, NULL);
  if(!model) {
    fprintf(stderr, "%s: Failed to create model\n", program);
    return(1);
  }


  /* Do this or gcc moans */
  stream=NULL;
  iterator=NULL;
  parser=NULL;
  serializer=NULL;
  source=NULL;
  arc=NULL;
  target=NULL;
  uri=NULL;
  node=NULL;
  query=NULL;

  switch(type) {
    case CMD_PRINT:
      librdf_model_print(model, stdout);
      break;
    case CMD_PARSE_MODEL:
    case CMD_PARSE_STREAM:
      uri=librdf_new_uri(world, argv[0]);
      if(!uri) {
        fprintf(stderr, "%s: Failed to create URI from %s\n", program, argv[0]);
        break;
      }
      
      parser=librdf_new_parser(world, argv[1], NULL, NULL);
      if(!parser) {
        fprintf(stderr, "%s: Failed to create new parser %s\n", program, argv[1]);
        librdf_free_uri(uri);
        break;
      }
      fprintf(stdout, "%s: Parsing URI %s with %s parser\n", program,
              librdf_uri_as_string(uri), argv[1]);
      
      if(argv[2]) {
        base_uri=librdf_new_uri(world, argv[2]);
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
          librdf_statement *statement=librdf_stream_get_object(stream);
          if(!statement) {
            fprintf(stderr, "%s: librdf_stream_next returned NULL\n", program);
            break;
          }
          
          librdf_model_add_statement(model, statement);
          count++;
          librdf_stream_next(stream);
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

    case CMD_SERIALIZE:
      /* args are name (optional), uri (may be NULL), mime_type (optional) */

      uri=NULL;
      name=NULL;
      mime_type=NULL;

      if(!argc)
        goto serialize;
      
      if(strcmp(argv[0], "-"))
        name=argv[0];
        
      if(argc < 2)
        goto serialize;

      if(strcmp(argv[1], "-")) {
        uri=librdf_new_uri(world, argv[1]);
        if(!uri) {
          fprintf(stderr, "%s: Failed to create URI from %s\n", program, argv[1]);
          break;
        }
      }
  
      if(argc ==3) {
        if(strcmp(argv[2], "-"))
          mime_type=argv[2];
      }

    serialize:
      serializer=librdf_new_serializer(world, name, mime_type, uri);
      if(!serializer) {
        fprintf(stderr, "%s: Failed to create new serializer name %s, uri %s, mime type %s\n", program, argv[0], argv[1], argv[2]);
        if(uri)
          librdf_free_uri(uri);
        break;
      }

      librdf_serializer_serialize_model(serializer, stdout, NULL, model);

      librdf_free_serializer(serializer);
      if(uri)
        librdf_free_uri(uri);

      break;
      
    case CMD_QUERY:
      /* args are name, uri (may be NULL), query_string/mime_type */

      if(!strcmp(argv[1], "-"))
        uri=NULL;
      else {
        uri=librdf_new_uri(world, argv[1]);
        if(!uri) {
          fprintf(stderr, "%s: Failed to create URI from %s\n", program, argv[1]);
          break;
        }
      }

      query=librdf_new_query(world, argv[0], uri, argv[2]);
      goto printmatching;
      break;

    case CMD_CONTAINS:
    case CMD_FIND:
    case CMD_ADD:
    case CMD_REMOVE:
    case CMD_ADD_TYPED:
      if(!strcmp(argv[0], "-"))
        source=NULL;
      else
        source=librdf_new_node_from_uri_string(world, argv[0]);
      
      if(!strcmp(argv[1], "-"))
        arc=NULL;
      else
        arc=librdf_new_node_from_uri_string(world, argv[1]);

      if(type == CMD_ADD_TYPED) {
        char *lang=(strcmp(argv[3], "-")) ? argv[3] : NULL;
        librdf_uri* dt_uri=librdf_new_uri(world, argv[4]);
        target=librdf_new_node_from_typed_literal(world, argv[2], lang, dt_uri);
        librdf_free_uri(dt_uri);
      } else {
        if(!strcmp(argv[2], "-"))
          target=NULL;
        else {
          if(librdf_heuristic_object_is_literal(argv[2]))
            target=librdf_new_node_from_literal(world, argv[2], NULL, 0);
          else
            target=librdf_new_node_from_uri_string(world, argv[2]);
        }
      }
      
      
      partial_statement=librdf_new_statement(world);
      librdf_statement_set_subject(partial_statement, source);
      librdf_statement_set_predicate(partial_statement, arc);
      librdf_statement_set_object(partial_statement, target);
      
      if(type != CMD_FIND) {
        if(!source || !arc || !target) {
          fprintf(stderr, "%s: cannot have missing statement parts for %s\n", program, cmd);
          librdf_free_statement(partial_statement);
        break;
        }
      }

    printmatching:
      switch(type) {
        case CMD_CONTAINS:
          if(librdf_model_contains_statement(model, partial_statement))
            fprintf(stdout, "%s: the model contains the statement\n", program);
        else
          fprintf(stdout, "%s: the model does not contain the statement\n", program);
          break;
          
        case CMD_FIND:
        case CMD_QUERY:
          /* Print out matching statements */
          if(type==CMD_FIND)
            stream=librdf_model_find_statements(model, partial_statement);
          else
            stream=librdf_model_query(model, query);
          if(!stream) {
            fprintf(stderr, "%s: %s returned NULL stream\n", program,
                    ((type==CMD_FIND) ? "librdf_model_find_statements" : "librdf_model_query"));
          } else {
            count=0;
            while(!librdf_stream_end(stream)) {
              librdf_statement *statement=librdf_stream_get_object(stream);
              librdf_node *context_node=librdf_stream_get_context(stream);
              if(!statement) {
                fprintf(stderr, "%s: librdf_stream_next returned NULL\n", program);
                break;
              }
            
              fputs("Matched statement: ", stdout);
              librdf_statement_print(statement, stdout);
              if(context_node) {
                fputs(" with context ", stdout);
                librdf_node_print(context_node, stdout);
              }
              fputc('\n', stdout);
            
              count++;
              librdf_stream_next(stream);
          }
            librdf_free_stream(stream);  
            fprintf(stderr, "%s: matching statements: %d\n", program, count);
          }
          break;
          
        case CMD_ADD:
        case CMD_ADD_TYPED:
          if(argv[3] && contexts) {
            librdf_node* context_node=librdf_new_node_from_uri_string(world, argv[3]);
            rc=librdf_model_context_add_statement(model, context_node,
                                                  partial_statement);
            librdf_free_node(context_node);
          } else {
            if(argv[3])
              fprintf(stderr, "%s: WARNING: Cannot add in context with contexts not enabled (-c)\n", program);
            rc=librdf_model_add_statement(model, partial_statement);
          }
          if(rc)
            fprintf(stdout, "%s: failed to add statement to model\n", program);
          else
            fprintf(stdout, "%s: added statement to model\n", program);
          break;
          
        case CMD_REMOVE:
          if(argv[3] && contexts) {
            librdf_node* context_node=librdf_new_node_from_uri_string(world, argv[3]);
            rc=librdf_model_context_remove_statement(model, context_node,
                                                     partial_statement);
            librdf_free_node(context_node);
          } else {
            if(argv[3])
              fprintf(stderr, "%s: WARNING: Cannot remove in context with contexts not enabled (-c)\n", program);
            rc=librdf_model_remove_statement(model, partial_statement);
          }
          if(rc)
            fprintf(stdout, "%s: failed to remove statement from model\n", program);
          else
            fprintf(stdout, "%s: removed statement from model\n", program);
        break;
        
        default:
          fprintf(stderr, "Unexpected command %d\n", type);
          
      } /* end inner switch */
      
      /* also frees the nodes */
      librdf_free_statement(partial_statement);
      break;
      
    case CMD_SOURCES:
      arc=librdf_new_node_from_uri_string(world, argv[0]);
      if(librdf_heuristic_object_is_literal(argv[1]))
        target=librdf_new_node_from_literal(world, argv[1], NULL, 0);
      else
        target=librdf_new_node_from_uri_string(world, argv[1]);
      
      iterator=librdf_model_get_sources(model, arc, target);
      if(!iterator) {
        fprintf(stderr, "%s: Failed to get sources\n", program);
        break;
      }
      
      /* FALLTHROUGH */
    case CMD_ARCS:
      if(!iterator) {
        source=librdf_new_node_from_uri_string(world, argv[0]);
        if(librdf_heuristic_object_is_literal(argv[1]))
          target=librdf_new_node_from_literal(world, argv[1], NULL, 0);
        else
          target=librdf_new_node_from_uri_string(world, argv[1]);
        iterator=librdf_model_get_arcs(model, source, target);
        if(!iterator) {
          fprintf(stderr, "Failed to get arcs\n");
          break;
        }
      }
      
      /* FALLTHROUGH */
    case CMD_TARGETS:
      if(!iterator) {
        source=librdf_new_node_from_uri_string(world, argv[0]);
        arc=librdf_new_node_from_uri_string(world, argv[1]);
        iterator=librdf_model_get_targets(model, source, arc);
        if(!iterator) {
          fprintf(stderr, "%s: Failed to get targets\n", program);
          break;
        }
      }
      
      /* (Common code) Print out nodes */
      count=0;
      while(!librdf_iterator_end(iterator)) {
        librdf_node *context_node=librdf_iterator_get_context(iterator);
        node=(librdf_node*)librdf_iterator_get_object(iterator);
        if(!node) {
          fprintf(stderr, "%s: librdf_iterator_get_object returned NULL\n",
                  program);
          break;
        }
        
        fputs("Matched node: ", stdout);
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
      fprintf(stderr, "%s: matching nodes: %d\n", program, count);
      
      if(source)
        librdf_free_node(source);
      if(arc)
        librdf_free_node(arc);
      if(target)
        librdf_free_node(target);
      break;
      
    case CMD_SOURCE:
      arc=librdf_new_node_from_uri_string(world, argv[0]);
      if(librdf_heuristic_object_is_literal(argv[1]))
        target=librdf_new_node_from_literal(world, argv[1], NULL, 0);
      else
        target=librdf_new_node_from_uri_string(world, argv[1]);

      node=librdf_model_get_source(model, arc, target);
      if(!node) {
        fprintf(stderr, "%s: Failed to get source\n", program);
        librdf_free_node(arc);
        librdf_free_node(target);
        break;
      }
      
      /* FALLTHROUGH */
    case CMD_ARC:
      if(!node) {
        source=librdf_new_node_from_uri_string(world, argv[0]);
        if(librdf_heuristic_object_is_literal(argv[1]))
          target=librdf_new_node_from_literal(world, argv[1], NULL, 0);
      else
        target=librdf_new_node_from_uri_string(world, argv[1]);
        node=librdf_model_get_arc(model, source, target);
        if(!node) {
          fprintf(stderr, "Failed to get arc\n");
          librdf_free_node(source);
          librdf_free_node(target);
          break;
        }
      }
      
      /* FALLTHROUGH */
    case CMD_TARGET:
      if(!node) {
        source=librdf_new_node_from_uri_string(world, argv[0]);
        arc=librdf_new_node_from_uri_string(world, argv[1]);
        node=librdf_model_get_target(model, source, arc);
        if(!node) {
          fprintf(stderr, "%s: Failed to get target\n", program);
          librdf_free_node(source);
          librdf_free_node(arc);
          break;
        }
      }
      
      /* (Common code) Print out node */
      if(node) {
        fputs("Matched node: ", stdout);
        librdf_node_print(node, stdout);
        fputc('\n', stdout);
        
        librdf_free_node(node);
      }
      if(source)
        librdf_free_node(source);
      if(arc)
        librdf_free_node(arc);
      if(target)
        librdf_free_node(target);
      break;


      
    case CMD_ARCS_IN:
    case CMD_ARCS_OUT:
      source=librdf_new_node_from_uri_string(world, argv[0]);
      iterator=(type == CMD_ARCS_IN) ? librdf_model_get_arcs_in(model, source) :
                                       librdf_model_get_arcs_out(model, source);
      if(!iterator) {
        fprintf(stderr, "%s: Failed to get arcs in/out\n", program);
        librdf_free_node(source);
        break;
      }

      count=0;
      while(!librdf_iterator_end(iterator)) {
        librdf_node *context_node=librdf_iterator_get_context(iterator);
        node=(librdf_node*)librdf_iterator_get_object(iterator);
        if(!node) {
          fprintf(stderr, "%s: librdf_iterator_get_next returned NULL\n",
                  program);
          break;
        }
        
        fputs("Matched arc: ", stdout);
        librdf_node_print(node, stdout);
        if(context_node) {
          fputs(" with context ", stdout);
          librdf_node_print(context_node, stdout);
        }
        fputc('\n', stdout);
        
        librdf_free_node(node);
        count++;
        librdf_iterator_next(iterator);
      }
      librdf_free_iterator(iterator);
      fprintf(stderr, "%s: matching arcs: %d\n", program, count);
      
      librdf_free_node(source);
      break;

      
    case CMD_HAS_ARC_IN:
    case CMD_HAS_ARC_OUT:
      source=librdf_new_node_from_uri_string(world, argv[0]);
      arc=librdf_new_node_from_uri_string(world, argv[1]);
      result=(type == CMD_HAS_ARC_IN) ? librdf_model_has_arc_in(model, arc, source) :
                                        librdf_model_has_arc_out(model, source, arc);
      if(result)
        fprintf(stdout, "%s: the model contains the arc\n", program);
      else
        fprintf(stdout, "%s: the model does not contain the arc\n", program);
      
      librdf_free_node(source);
      librdf_free_node(arc);
      break;


    default:
      fprintf(stderr, "%s: Unknown command %d\n", program, type);
      return(1);
  }


  if(query)
    librdf_free_query(query);
  
  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_free_world(world);

#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
