/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_uri.c - RDF URI Implementation
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


#include <rdf_config.h>

#include <stdio.h>

#define LIBRDF_INTERNAL 1
#include <redland.h>

#include <rdf_uri.h>
#include <rdf_digest.h>


/* statics */
static librdf_digest_factory* librdf_uri_digest_factory;


static librdf_hash* uris_hash=NULL;
static int uris_hash_allocated_here=0;


/* class methods */


/**
 * librdf_init_uri - Initialise the librdf_uri class
 * @digest_factory: &librdf_digest_factory
 * @hash: &librdf_hash
 * 
 * Initialises the librdf_uri class using the given
 * digest factory when calculating URI digests in librdf_uri_get_digest()
 * and using the given hash to store uris.
 **/
void
librdf_init_uri(librdf_digest_factory* digest_factory, librdf_hash* hash) 
{
  librdf_uri_digest_factory=digest_factory;

  /* If no default given, create an in memory hash */
  if(!hash) {
    hash=librdf_new_hash(NULL);
    if(!hash)
      LIBRDF_FATAL1(librdf_init_uri, "Failed to create URI hash from factory");
    
    if(librdf_hash_open(hash, NULL, 0, 1, 1, NULL))
      LIBRDF_FATAL1(librdf_init_uri, "Failed to open URI hash");

    /* remember to free it later */
    uris_hash_allocated_here=1;
  }

  uris_hash=hash;  
}



/**
 * librdf_finish_uri - Terminate the librdf_uri class
 **/
void
librdf_finish_uri(void)
{
  librdf_hash_close(uris_hash);

  if(uris_hash_allocated_here)
    librdf_free_hash(uris_hash);
}



/**
 * librdf_new_uri - Constructor - create a new librdf_uri object from a URI string
 * @uri_string: URI in string form
 * 
 * A new URI is constructed from a copy of the string.
 *
 * Return value: a new &librdf_uri object or NULL on failure
 **/
librdf_uri*
librdf_new_uri (char *uri_string)
{
  librdf_uri* new_uri;
  char *new_string;
  int length;
  librdf_hash_datum *key, *value;
  
  length=strlen(uri_string);

  key=librdf_new_hash_datum(uri_string, length);
  if(!key)
    return NULL;
  
  /* if existing URI found in hash, return it */
  if((value=librdf_hash_get_one(uris_hash, key))) {
    new_uri=*(librdf_uri**)value->data;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG3(librdf_new_uri, "Found existing URI %s in hash with current usage %d\n", uri_string, new_uri->usage);
#endif

    key->data=NULL;
    librdf_free_hash_datum(key);
    librdf_free_hash_datum(value);
    new_uri->usage++;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    if(new_uri->usage > new_uri->max_usage)
      new_uri->max_usage=new_uri->usage;
#endif    
    return new_uri;
  }
  

  /* otherwise create a new one */

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_new_uri, "Creating new URI %s in hash\n", uri_string);
#endif

  new_uri = (librdf_uri*)LIBRDF_CALLOC(librdf_uri, 1, sizeof(librdf_uri));
  if(!new_uri)
    return NULL;

  new_uri->string_length=length;

  new_string=(char*)LIBRDF_MALLOC(librdf_uri, length+1);
  if(!new_string) {
    LIBRDF_FREE(librdf_uri, new_uri);
    return NULL;
  }
  
  strcpy(new_string, uri_string);
  new_uri->string=new_string;

  new_uri->usage=1;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  new_uri->max_usage=1;
#endif

  /* store in hash: URI-string => (librdf_uri*) */
  if(librdf_hash_put(uris_hash, 
                     uri_string, length,
                     &new_uri, sizeof(librdf_uri*))) {
    LIBRDF_FREE(librdf_uri, new_uri);
    new_uri=NULL;
  } 

  /* Don't free data */
  key->data=NULL;
  librdf_free_hash_datum(key);

  return new_uri;
}


/**
 * librdf_new_uri_from_uri - Copy constructor - create a new librdf_uri object from an existing librdf_uri object
 * @old_uri: &librdf_uri object
 * 
 * Return value: a new &librdf_uri object or NULL on failure
 **/
librdf_uri*
librdf_new_uri_from_uri (librdf_uri* old_uri) {
  return librdf_new_uri (old_uri->string);
}


/**
 * librdf_free_uri - Destructor - destroy a librdf_uri object
 * @uri: &librdf_uri object
 * 
 **/
void
librdf_free_uri (librdf_uri* uri) 
{
  uri->usage--;
  
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_free_uri, "URI %s usage count now %d\n", uri->string, uri->usage);
#endif

  /* decrement usage, don't free if not 0 yet*/
  if(uri->usage)
    return;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_free_uri, "Deleting URI %s from hash, max usage was %d\n", uri->string, uri->max_usage);
#endif
  
  if(librdf_hash_delete(uris_hash, uri->string, uri->string_length) )
    LIBRDF_FATAL1(librdf_free_uri, "Hash deletion failed");


  if(uri->string)
    LIBRDF_FREE(cstring, uri->string);
  LIBRDF_FREE(librdf_uri, uri);
}


/**
 * librdf_uri_as_string - Get a pointer to the string representation of the URI
 * @uri: &librdf_uri object
 * 
 * Returns a shared pointer to the URI string representation. 
 * Note: does not allocate a new string so the caller must not free it.
 * 
 * Return value: string representation of URI
 **/
char*
librdf_uri_as_string (librdf_uri *uri) 
{
  return uri->string;
}


/**
 * librdf_uri_get_digest - Get a digest for the URI
 * @uri: &librdf_uri object
 * 
 * Generates a digest object for the URI.  The digest factory used is
 * determined at class initialisation time by librdf_init_uri().
 * 
 * Return value: new &librdf_digest object or NULL on failure.
 **/
librdf_digest*
librdf_uri_get_digest (librdf_uri* uri) 
{
  librdf_digest* d;
  
  d=librdf_new_digest_from_factory(librdf_uri_digest_factory);
  if(!d)
    return NULL;
  
  librdf_digest_init(d);
  librdf_digest_update(d, (unsigned char*)uri->string, uri->string_length);
  librdf_digest_final(d);
  
  return d;
}


/**
 * librdf_uri_print - Print the URI to the given file handle
 * @uri: &librdf_uri object
 * @fh: &FILE handle
 **/
void
librdf_uri_print (librdf_uri* uri, FILE *fh) 
{
  fputs(uri->string, fh);
}


/**
 * librdf_uri_to_string - Format the URI as a string
 * @uri: &librdf_uri object
 * 
 * Note: this method allocates a new string since this is a _to_ method
 * and the caller must free the resulting memory.
 *
 * Return value: string representation of the URI or NULL on failure
 **/
char*
librdf_uri_to_string (librdf_uri* uri)
{
  char *s;

  if(!uri)
    return NULL;
  
  s=(char*)LIBRDF_MALLOC(cstring, uri->string_length+1);
  if(!s)
    return NULL;

  strcpy(s, uri->string);
  return s;
}


/**
 * librdf_uri_equals - Compare two librdf_uri objects for equality
 * @first_uri: &librdf_uri object 1
 * @second_uri: &librdf_uri object 2
 * 
 * Return value: 0 if the objects are equal
 **/
int
librdf_uri_equals(librdf_uri* first_uri, librdf_uri* second_uri) 
{
  if(first_uri->string_length != second_uri->string_length)
    return 0;
  return !strcmp(first_uri->string, second_uri->string);
}



#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  char *hp_string="http://www.ilrt.bristol.ac.uk/people/cmdjb/";
  librdf_uri *uri1, *uri2;
  librdf_digest_factory* digest_factory;
  librdf_digest *d;
  char *program=argv[0];

  librdf_init_digest();
  librdf_init_hash();
  digest_factory=librdf_get_digest_factory(NULL);
  librdf_init_uri(digest_factory, NULL);

  fprintf(stderr, "%s: Creating new URI from string\n", program);
  uri1=librdf_new_uri(hp_string);
  if(!uri1) {
    fprintf(stderr, "%s: Failed to create URI from string '%s'\n", program, 
	    hp_string);
    return(1);
  }
  
  fprintf(stderr, "%s: Home page URI is ", program);
  librdf_uri_print(uri1, stderr);
  fputs("\n", stderr);
  
  fprintf(stderr, "%s: Creating URI from URI\n", program);
  uri2=librdf_new_uri_from_uri(uri1);
  if(!uri2) {
    fprintf(stderr, "%s: Failed to create new URI from old one\n", program);
    return(1);
  }

  fprintf(stderr, "%s: New URI is ", program);
  librdf_uri_print(uri2, stderr);
  fputs("\n", stderr);

  
  fprintf(stderr, "%s: Getting digest for URI\n", program);
  d=librdf_uri_get_digest(uri2);
  if(!d) {
    fprintf(stderr, "%s: Failed to get digest for URI %s\n", program, 
	    librdf_uri_as_string(uri2));
    return(1);
  }
  fprintf(stderr, "%s: Digest is: ", program);
  librdf_digest_print(d, stderr);
  fputs("\n", stderr);
  librdf_free_digest(d);


  fprintf(stderr, "%s: Freeing URIs\n", program);
  librdf_free_uri(uri1);
  librdf_free_uri(uri2);
  
  librdf_finish_uri();
  librdf_finish_hash();
  librdf_finish_digest();

#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
 
  /* keep gcc -Wall happy */
  return(0);
}

#endif
