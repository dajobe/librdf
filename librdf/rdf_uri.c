/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_uri.c - RDF URI Implementation
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


#include <rdf_config.h>

#include <stdio.h>
#include <string.h> /* for strcat */
#include <ctype.h> /* for isalnum */

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <librdf.h>

#include <rdf_uri.h>
#include <rdf_digest.h>


/* class methods */


/**
 * librdf_init_uri - Initialise the librdf_uri class
 * @world: redland world object
 *
 **/
void
librdf_init_uri(librdf_world *world)
{
  /* If no default given, create an in memory hash */
  if(!world->uris_hash) {
    world->uris_hash=librdf_new_hash(world, NULL);
    if(!world->uris_hash)
      LIBRDF_FATAL1(librdf_init_uri, "Failed to create URI hash from factory");
    
    if(librdf_hash_open(world->uris_hash, NULL, 0, 1, 1, NULL))
      LIBRDF_FATAL1(librdf_init_uri, "Failed to open URI hash");

    /* remember to free it later */
    world->uris_hash_allocated_here=1;
  }
}



/**
 * librdf_finish_uri - Terminate the librdf_uri class
 * @world: redland world object
 **/
void
librdf_finish_uri(librdf_world *world)
{
  librdf_hash_close(world->uris_hash);

  if(world->uris_hash_allocated_here)
    librdf_free_hash(world->uris_hash);
}



/**
 * librdf_new_uri - Constructor - create a new librdf_uri object from a URI string
 * @world: redland world object
 * @uri_string: URI in string form
 * 
 * A new URI is constructed from a copy of the string.
 *
 * Return value: a new &librdf_uri object or NULL on failure
 **/
librdf_uri*
librdf_new_uri (librdf_world *world, 
                const char *uri_string)
{
  librdf_uri* new_uri;
  char *new_string;
  int length;
  librdf_hash_datum key, value; /* on stack - not allocated */
  librdf_hash_datum *old_value;
  
  length=strlen(uri_string);

  key.data=(char*)uri_string;
  key.size=length;

  /* if existing URI found in hash, return it */
  if((old_value=librdf_hash_get_one(world->uris_hash, &key))) {
    new_uri=*(librdf_uri**)old_value->data;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
    LIBRDF_DEBUG3(librdf_new_uri, "Found existing URI %s in hash with current usage %d\n", uri_string, new_uri->usage);
#endif

    librdf_free_hash_datum(old_value);
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

  new_uri->world=world;
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

  value.data=&new_uri; value.size=sizeof(librdf_uri*);
  
  /* store in hash: URI-string => (librdf_uri*) */
  if(librdf_hash_put(world->uris_hash, &key, &value)) {
    LIBRDF_FREE(librdf_uri, new_uri);
    new_uri=NULL;
  }

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
  return librdf_new_uri (old_uri->world, old_uri->string);
}


/**
 * librdf_new_uri_from_uri_local_name - Copy constructor - create a new librdf_uri object from an existing librdf_uri object and a local name
 * @old_uri: &librdf_uri object
 * @local_name: local name to append to URI
 * 
 * Return value: a new &librdf_uri object or NULL on failure
 **/
librdf_uri*
librdf_new_uri_from_uri_local_name (librdf_uri* old_uri, const char *local_name) {
  int len=old_uri->string_length + strlen(local_name) +1 ; /* +1 for \0 */
  char *new_string;
  librdf_uri* new_uri;
  
  new_string=(char*)LIBRDF_CALLOC(cstring, 1, len);
  if(!new_string)
    return NULL;

  strcpy(new_string, old_uri->string);
  strcat(new_string, local_name);

  new_uri=librdf_new_uri (old_uri->world, new_string);
  LIBRDF_FREE(cstring, new_string);

  return new_uri;
}


/**
 * librdf_new_uri_normalised_to_base - Constructor - create a new librdf_uri object from a URI string stripped of the source URI, made relative to the base URI
 * @uri_string: URI in string form
 * @source_uri: source URI to remove
 * @base_uri: base URI to add
 * 
 * Return value: a new &librdf_uri object or NULL on failure
 **/
librdf_uri*
librdf_new_uri_normalised_to_base(const char *uri_string,
                                  librdf_uri* source_uri,
                                  librdf_uri* base_uri) 
{
  int uri_string_len;
  int len;
  char *new_uri_string;
  librdf_uri *new_uri;
  librdf_world *world=source_uri->world;
                                    
  /* no URI or empty URI - easy, just make from base_uri */
  if((!uri_string || !*uri_string) && base_uri)
    return librdf_new_uri_from_uri(base_uri);

  /* not a fragment, and no match - easy */
  if(*uri_string != '#' &&
     strncmp(uri_string, source_uri->string, source_uri->string_length))
    return librdf_new_uri(world, uri_string);

  /* darn - is a fragment or matches, is a prefix of the source URI */

  /* move uri_string pointer to first non-matching char 
   * unless a fragment, when all of the uri_string will 
   * be appended
   */
  if(*uri_string != '#')
    uri_string += source_uri->string_length;

  /* size of remaining bytes to copy from uri_string */
  uri_string_len=strlen(uri_string);

  /* total bytes */
  len=uri_string_len + 1 + base_uri->string_length;

  new_uri_string=(char*)LIBRDF_MALLOC(cstring, len);
  if(!new_uri_string)
    return NULL;
  strncpy(new_uri_string, base_uri->string, base_uri->string_length);
  /* strcpy not strncpy since I want a \0 on the end */
  strcpy(new_uri_string + base_uri->string_length, uri_string);
  
  new_uri=librdf_new_uri(world, new_uri_string);
  LIBRDF_FREE(cstring, new_uri_string); /* always free this even on failure */

  return new_uri; /* new URI or NULL from librdf_new_uri failure */
}



/**
 * librdf_new_uri_relative_to_base - Constructor - create a new librdf_uri object from a URI string relative to a base URI
 * @base_uri: absolute base URI
 * @uri_string: relative URI string
 * 
 * NOTE: This method is not a full relative URI implementation but handles
 * common cases where uri_string is empty, "#id", "id" where id has
 * no "/" or "../" inside.  FIXME.
 *
 * Return value: a new &librdf_uri object or NULL on failure
 **/
librdf_uri*
librdf_new_uri_relative_to_base(librdf_uri* base_uri,
                                const char *uri_string) {
  const char *p;
  int uri_string_length;
  char *new_uri_string;
  librdf_uri* new_uri;
  librdf_world *world=base_uri->world;
                                  
  /* If URI string is empty, just copy base URI */
  if(!*uri_string)
    return librdf_new_uri_from_uri(base_uri);

  uri_string_length=strlen(uri_string);

  /* If URI string is a fragment #foo ... */
  if(*uri_string == '#') {

    /* Check if base URI has a fragment - # somewhere */
    for(p=base_uri->string;*p; p++)
      if(*p == '#')
        break;

    /* just append to base URI if it has no fragment */
    if(!*p)
      return librdf_new_uri_from_uri_local_name(base_uri, uri_string);

    /* otherwise, take the prefix of base URI and add the fragment part */
    new_uri_string=(char*)LIBRDF_MALLOC(cstring, (p-base_uri->string) + uri_string_length + 1);
    if(!new_uri_string)
      return NULL;
    strncpy(new_uri_string, base_uri->string, (p-base_uri->string));
    strcpy(new_uri_string + (p-base_uri->string), uri_string);
    new_uri=librdf_new_uri(world, new_uri_string);
    LIBRDF_FREE(cstring, new_uri_string);
    return new_uri;
  }
  

  /* If URI string is an absolute URI, just copy it 
   * FIXME - need to check what is allowed before a :
   */
  for(p=uri_string;*p; p++)
    if(!isalnum(*p))
       break;

  /* If first non-alphanumeric char is a ':' then probably a absolute URI
   * FIXME - wrong, but good enough for a short while...
   */
  if(*p && *p == ':')
    return librdf_new_uri(world, uri_string);


  /* Otherwise is a general URI relative to base URI */

  /* FIXME do this properly */

  /* Move p to the last / or : char in the base URI */
  for(p= base_uri->string + base_uri->string_length - 1;
      p > base_uri->string && *p != '/' && *p != ':';
      p--)
    ;

  new_uri_string=(char*)LIBRDF_MALLOC(cstring, (p-base_uri->string)+1+uri_string_length+1);
  if(!new_uri_string)
    return NULL;

  strncpy(new_uri_string, base_uri->string, p-base_uri->string+1);
  strcpy(new_uri_string + (p-base_uri->string) + 1, uri_string);
  new_uri=librdf_new_uri(world, new_uri_string);
  LIBRDF_FREE(cstring, new_uri_string);
  
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
  librdf_hash_datum key; /* on stack */

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
  
  key.data=uri->string;
  key.size=uri->string_length;
  if(librdf_hash_delete_all(uri->world->uris_hash, &key) )
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
  librdf_world *world=uri->world;
  librdf_digest* d;
  
  d=librdf_new_digest_from_factory(world, world->digest_factory);
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
 * Return value: non 0 if the objects are equal
 **/
int
librdf_uri_equals(librdf_uri* first_uri, librdf_uri* second_uri) 
{
  if(first_uri->string_length != second_uri->string_length)
    return 0;
  return !strcmp(first_uri->string, second_uri->string);
}


/**
 * librdf_uri_is_file_uri - Test if a URI points to a filename
 * @uri: &librdf_uri object
 * 
 * Return value: 0 if the URI points to a file
 **/
int
librdf_uri_is_file_uri(librdf_uri* uri) 
{
  /* FIXME: Almost certainly missing something subtle here */
  return strncmp(uri->string, "file:", 5)==0;
}


/**
 * librdf_uri_as_filename - Return pointer to filename part of URI
 * @uri: &librdf_uri object
 * 
 * Note - returns a pointer to a shared copy; this assumes that
 * the URI encodes the filename without changing any characters -
 * a simple but in general wrong assumption (e.g. files with spaces
 * or slashes).
 *
 * Return value: pointer to shared copy of name or NULL if the URI does not point to a file
 **/
const char*
librdf_uri_as_filename(librdf_uri* uri) 
{
  if(!librdf_uri_is_file_uri(uri))
    return NULL;
  return uri->string + 5;
}


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  char *hp_string="http://www.ilrt.bristol.ac.uk/people/cmdjb/";
  librdf_uri *uri1, *uri2, *uri3, *uri4, *uri5, *uri6, *uri7;
  librdf_digest *d;
  char *program=argv[0];
  const char *uri_string=  "file:/big/long/directory/blah#frag";
  const char *relative_uri_string1="#foo";
  const char *relative_uri_string2="bar";
  librdf_world *world;
  
  world=librdf_new_world();
  
  librdf_init_digest(world);
  librdf_init_hash(world);
  librdf_init_uri(world);

  fprintf(stderr, "%s: Creating new URI from string\n", program);
  uri1=librdf_new_uri(world, hp_string);
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

  uri3=librdf_new_uri(world, "file:/big/long/directory/");
  uri4=librdf_new_uri(world, "http://somewhere/dir/");
  fprintf(stderr, "%s: Source URI is ", program);
  librdf_uri_print(uri3, stderr);
  fputs("\n", stderr);
  fprintf(stderr, "%s: Base URI is ", program);
  librdf_uri_print(uri4, stderr);
  fputs("\n", stderr);
  fprintf(stderr, "%s: URI string is '%s'\n", program, uri_string);

  uri5=librdf_new_uri_normalised_to_base(uri_string, uri3, uri4);
  fprintf(stderr, "%s: Normalised URI is ", program);
  librdf_uri_print(uri5, stderr);
  fputs("\n", stderr);


  uri6=librdf_new_uri_relative_to_base(uri5, relative_uri_string1);
  fprintf(stderr, "%s: URI + Relative URI %s gives ", program, 
          relative_uri_string1);
  librdf_uri_print(uri6, stderr);
  fputs("\n", stderr);

  uri7=librdf_new_uri_relative_to_base(uri5, relative_uri_string2);
  fprintf(stderr, "%s: URI + Relative URI %s gives ", program, 
          relative_uri_string2);
  librdf_uri_print(uri7, stderr);
  fputs("\n", stderr);


  fprintf(stderr, "%s: Freeing URIs\n", program);
  librdf_free_uri(uri1);
  librdf_free_uri(uri2);
  librdf_free_uri(uri3);
  librdf_free_uri(uri4);
  librdf_free_uri(uri5);
  librdf_free_uri(uri6);
  librdf_free_uri(uri7);
  
  librdf_finish_uri(world);
  librdf_finish_hash(world);
  librdf_finish_digest(world);

  LIBRDF_FREE(librdf_world, world);

#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
 
  /* keep gcc -Wall happy */
  return(0);
}

#endif
