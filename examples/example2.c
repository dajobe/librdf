/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example2.c - Redland example code importing RDF as triples from a text file and storing them on disk as BDB hashes
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


#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <redland.h>

/* one prototype needed */
int main(int argc, char *argv[]);


/* FIXME: Yeah?  What about it? */
#define LINE_BUFFER_LEN 1024

int
main(int argc, char *argv[]) 
{
  librdf_world* world;
  librdf_storage* storage;
  librdf_model* model;
  char *program=argv[0];
  librdf_uri *uri;
  char buffer[LINE_BUFFER_LEN];
  FILE *fh;
  FILE *output=stdout;
  
  int line;

  if(argc !=2) {
    fprintf(stderr, "USAGE: %s: <RDF file>\n", program);
    return(1);
  }


  world=librdf_new_world();
  librdf_world_open(world);

  uri=librdf_new_uri(world, argv[1]);
  if(!uri) {
    fprintf(stderr, "%s: Failed to create URI\n", program);
    return(1);
  }

  storage=librdf_new_storage(world, "hashes", "test", "hash-type='bdb',dir='.'");
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }

  model=librdf_new_model(world, storage, NULL);
  if(!model) {
    fprintf(stderr, "%s: Failed to create model\n", program);
    return(1);
  }
  

  fh=fopen(argv[1], "r");
  if(!fh) {
    fprintf(stderr, "%s: Failed to open %s - \n", program, argv[1]);
    perror(NULL);
    return(1);
  }

  for(line=0; !feof(fh); line++) {
    char *p, *s, *o, *e, *ptr;
    librdf_statement* statement=NULL;


    if(!fgets(buffer, LINE_BUFFER_LEN, fh))
      continue;

/*    fprintf(stderr, "%d:%s\n", line, buffer);*/

    if(!(p=strstr(buffer, "triple(\"")))
      continue;
    
    if(p != buffer)
      continue;
    
    /* predicate found after >>triple("<< */
    p+=8;

    if(!(s=strstr(p, "\", \"")))
      continue;
    *s='\0';
    s+=4;
    
    /* subject found after >>", "<< */

    if(!(o=strstr(s, "\", ")))
      continue;
    *o='\0';
    o+=4;

    /* object found after >>", << */


    if(!(e=strstr(o, "\")")))
      continue;
    /* zap end */
    *e='\0';

    /* trim trailing spaces at end of object */
    for(ptr=e-1; isspace(*ptr); ptr--)
      *ptr='\0';
    

    /* got all statement parts now */
    statement=librdf_new_statement(world);
    if(!statement)
      break;
    
    librdf_statement_set_subject(statement, 
                                 librdf_new_node_from_uri_string(world, s));

    librdf_statement_set_predicate(statement,
                                   librdf_new_node_from_uri_string(world, p));

    if(librdf_heuristic_object_is_literal(o))
      librdf_statement_set_object(statement,
                                  librdf_new_node_from_literal(world, o, NULL, 0));
    else
      librdf_statement_set_object(statement, 
                                  librdf_new_node_from_uri_string(world, o));


    librdf_model_add_statement(model, statement);
  }
  fclose(fh);


  fprintf(output, "%s: Resulting model is:\n", program);
  librdf_model_print(model, output);

  
  librdf_free_model(model);

  librdf_free_storage(storage);

  librdf_free_uri(uri);

  librdf_free_world(world);

#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
