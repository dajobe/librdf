/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example2.c - Example code importing triples from a text file
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
#include <ctype.h>
#include <string.h>

#include <librdf.h>

/* one prototype needed */
int main(int argc, char *argv[]);


/* FIXME: Yeah?  What about it? */
#define LINE_BUFFER_LEN 1024

int
main(int argc, char *argv[]) 
{
  librdf_storage_factory* sfactory;
  librdf_storage* storage;
  librdf_model* model;
  char *program=argv[0];
  librdf_uri *uri;
  char buffer[LINE_BUFFER_LEN];
  FILE *fh;
  int line;

  if(argc !=2) {
    fprintf(stderr, "USAGE: %s: <RDF file>\n", program);
    return(1);
  }


  librdf_init_world(NULL);

  uri=librdf_new_uri(argv[1]);
  if(!uri) {
    fprintf(stderr, "%s: Failed to create URI\n", program);
    return(1);
  }

  sfactory=librdf_get_storage_factory(NULL);
  if(!sfactory) {
    fprintf(stderr, "%s: Failed to get any storage factory\n", program);
    return(1);
  }

  storage=librdf_new_storage(sfactory, NULL);
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }

  model=librdf_new_model(storage, NULL);
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
    int object_is_literal;
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

    fprintf(stderr,"%d: Found statement\n", line);

    /* FIXME: I have no idea if the object points to a literal or a resource */
  
    object_is_literal=1; /* assume the worst */
    
    /* Try to guess if resource / literal from string by assuming it 
     * is a URL if it matches ^[isalnum()]+:[^isblank()]+$
     * This will be fooled by literals of form 'thing:non-blank-thing'
     */
    for(ptr=o; *ptr; ptr++) {
      /* Find first non alphanumeric */
      if(!isalnum(*ptr)) {
	/* Better be a ':' */
	if(*ptr == ':') {
	  /* check rest of string has no spaces */
	  for(;*ptr; ptr++)
	    if(isspace(*ptr))
	      break;
	  /* reached end, thus probably not a literal */
	  if(!*ptr)
	    object_is_literal=0;
	  break;
	}
      }
    }

    /* got all statement parts now */
    statement=librdf_new_statement();
    if(!statement)
      break;
    
    librdf_statement_set_subject(statement, 
                                 librdf_new_node_from_uri_string(s));

    librdf_statement_set_predicate(statement,
                                   librdf_new_node_from_uri_string(p));

    if(object_is_literal) {
      librdf_statement_set_object(statement,
                                  librdf_new_node_from_literal(o, NULL, 0));
    } else {
      librdf_statement_set_object(statement, 
                                  librdf_new_node_from_uri_string(o));
    }


    librdf_model_add_statement(model, statement);
  }
  fclose(fh);


  fprintf(stderr, "%s: Resulting model is:\n", program);
  librdf_model_print(model, stderr);

  
  librdf_free_model(model);

  librdf_free_storage(storage);

  librdf_free_uri(uri);

  librdf_destroy_world();

#ifdef LIBRDF_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
