/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example2.c - Example code importing triples from a text file
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


/* FIXME: Yeah?  What about it? */
#define LINE_BUFFER_LEN 1024

int
main(int argc, char *argv[]) 
{
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
    statement=librdf_new_statement();
    if(!statement)
      break;
    
    librdf_statement_set_subject(statement, 
                                 librdf_new_node_from_uri_string(s));

    librdf_statement_set_predicate(statement,
                                   librdf_new_node_from_uri_string(p));

    if(librdf_heuristic_object_is_literal(o))
      librdf_statement_set_object(statement,
                                  librdf_new_node_from_literal(o, NULL, 0, 0));
    else
      librdf_statement_set_object(statement, 
                                  librdf_new_node_from_uri_string(o));


    librdf_model_add_statement(model, statement);
  }
  fclose(fh);


  fprintf(output, "%s: Resulting model is:\n", program);
  librdf_model_print(model, output);

  
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
