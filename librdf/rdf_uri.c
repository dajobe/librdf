/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_uri.c - RDF URI Implementation
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


#include <rdf_config.h>

#include <stdio.h>

#define LIBRDF_INTERNAL 1
#include <librdf.h>

#include <rdf_uri.h>
#include <rdf_digest.h>


/* statics */
static librdf_digest_factory* librdf_uri_digest_factory;


/* class methods */


/**
 * librdf_init_uri - Initialise the librdf_uri class
 * @factory: &librdf_digest_factory
 * 
 * Initialises the librdf_uri class using the given
 * digest factory when calculating URI digests in librdf_uri_get_digest().
 **/
void
librdf_init_uri(librdf_digest_factory* factory) 
{
  librdf_uri_digest_factory=factory;
}



/**
 * librdf_finish_uri - Terminate the librdf_uri class
 **/
void
librdf_finish_uri(void)
{
  /* nothing */
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

  new_uri = (librdf_uri*)LIBRDF_CALLOC(librdf_uri, 1, 
                                         sizeof(librdf_uri));
  if(!new_uri)
    return NULL;

  new_uri->string_length=length=strlen(uri_string);

  new_string=(char*)LIBRDF_MALLOC(librdf_uri, length+1);
  if(!new_string) {
    LIBRDF_FREE(librdf_uri, new_uri);
    return 0;
  }
  
  strcpy(new_string, uri_string);
  new_uri->string=new_string;

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
  librdf_uri* new_uri;
  char *new_string;

  new_uri = (librdf_uri*)LIBRDF_CALLOC(librdf_uri, 1, 
				       sizeof(librdf_uri));
  if(!new_uri)
    return NULL;

  new_string=(char*)LIBRDF_MALLOC(librdf_uri, old_uri->string_length+1);
  if(!new_string) {
    LIBRDF_FREE(librdf_uri, new_uri);
    return 0;
  }

  strcpy(new_string, old_uri->string);
  new_uri->string=new_string;
  new_uri->string_length=old_uri->string_length;

  return new_uri;
}


/**
 * librdf_free_uri - Destructor - destroy a librdf_uri object
 * @uri: &librdf_uri object
 * 
 **/
void
librdf_free_uri (librdf_uri* uri) 
{
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
  char *s=(char*)LIBRDF_MALLOC(cstring, uri->string_length+1);
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

  digest_factory=librdf_get_digest_factory(NULL);
  librdf_init_uri(digest_factory);

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
  
  librdf_finish_digest();
  librdf_finish_uri();

#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
 
  /* keep gcc -Wall happy */
  return(0);
}

#endif
