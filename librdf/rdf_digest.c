/*
 * RDF Digest Factory / Digest implementation
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>

#include <stdio.h>

#include <rdf_config.h>
#include <rdf_digest.h>
#include <rdf_uri.h>


/** List of digest factories */
static rdf_digest_factory *digests=NULL;


/** register a hash factory
 * @param name the name of the hash
 * @param factory function to be called to register the factor parameters
 */
void
rdf_digest_register_factory(const char *name,
                           void (*factory) (rdf_digest_factory*)
                           )
{
  rdf_digest_factory *d, *digest;
  char *name_copy;

  RDF_DEBUG2(rdf_digest_register_factory,
             "Received registration for digest %s\n", name);

  digest=(rdf_digest_factory*)RDF_CALLOC(rdf_digest_factory, sizeof(rdf_digest_factory), 1);
  if(!digest)
    RDF_FATAL(rdf_digest_register_factory, "Out of memory\n");

  name_copy=(char*)RDF_CALLOC(cstring, strlen(name)+1, 1);
  if(!name_copy) {
    RDF_FREE(rdf_digest, digest);
    RDF_FATAL(rdf_digest_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  digest->name=name_copy;

  for(d = digests; d; d = d->next ) {
    if(!strcmp(d->name, name_copy)) {
      RDF_FATAL2(rdf_digest_register_factory, "digest %s already registered\n", d->name);
    }
  }

  

  /* Call the digest registration function on the new object */
  (*factory)(digest);


  RDF_DEBUG4(rdf_digest_register_factory, "%s has context size %d and digest size %d\n", name, digest->context_length, digest->digest_length);

  digest->next = digests;
  digests = digest;
  
}

/** get a digest factory
 * @param name the name of the factor
 * @returns the factory or NULL if not found
 */
rdf_digest_factory*
get_rdf_digest_factory(const char *name) 
{
  rdf_digest_factory *factory;

  /* return 1st digest if no particular one wanted - why? */
  if(!name) {
    factory=digests;
    if(!factory) {
      RDF_DEBUG(get_rdf_digest_factory, "No (default) digests registered\n");
      return NULL;
    }
  } else {
    for(factory=digests; factory; factory=factory->next) {
      if(!strcmp(factory->name, name)) {
        break;
      }
    }
    /* else FACTORY with name digest_name not found */
    if(!factory) {
      RDF_DEBUG2(get_rdf_digest_factory, "No digest with name %s found\n",
              name);
      return NULL;
    }
  }
  
  return factory;
}


/** rdf_digest object constructor
 * @param factory the digest factory to use to create this digest
 */
rdf_digest*
new_rdf_digest(rdf_digest_factory *factory)
{
  rdf_digest* d;

  d=(rdf_digest*)RDF_CALLOC(rdf_digest, sizeof(rdf_digest), 1);
  if(!d)
    return NULL;

  d->context=(char*)RDF_CALLOC(digest_context, factory->context_length, 1);
  if(!d->context) {
    free_rdf_digest(d);
    return NULL;
  }

  d->digest=(unsigned char*)RDF_CALLOC(digest_digest, factory->digest_length, 1);
  if(!d->digest) {
    free_rdf_digest(d);
    return NULL;
  }

  d->factory=factory;
  
  return d;
}


/** rdf_digest object destructor
 * @param digest the digest
 */
void
free_rdf_digest(rdf_digest *digest) 
{
  if(digest->context)
    RDF_FREE(digest_context, digest->context);
  if(digest->digest)
    RDF_FREE(digest_digest, digest->digest);
  RDF_FREE(rdf_digest, digest);
}



/* methods */

/** initialise/reinitialise the digest
 * @param digest the digest
 */
void
rdf_digest_init(rdf_digest* digest) 
{
  (*(digest->factory->init))(digest->context);
}


/** add more data to the digest
 * @param digest the digest
 * @param buf the data buffer
 * @param the length of the data
 */
void
rdf_digest_update(rdf_digest* digest, unsigned char *buf, size_t length) 
{
  (*(digest->factory->update))(digest->context, buf, length);
}


/** finish digesting
 * @param digest the digest
 * @see rdf_digest_get_digest
 */
void
rdf_digest_final(rdf_digest* digest) 
{
  void* digest_data;
  
  (*(digest->factory->final))(digest->context);

  digest_data=(*(digest->factory->get_digest))(digest->context);
  memcpy(digest->digest, digest_data, digest->factory->digest_length);
}


/** return the digest data
 * @param digest the digest
 */
void*
rdf_digest_get_digest(rdf_digest* digest)
{
  return digest->digest;
}


/** convert the digest object to a string representation
 * @note assumes that the digest has been finalised
 * @node allocates a new string since this is a _to_ method
 * @returns an allocated string that must be freed by the caller
 */
char *
rdf_digest_to_string(rdf_digest* digest)
{
  unsigned char* data=digest->digest;
  int mdlen=digest->factory->digest_length;
  char* b;
  int i;
  
  b=(char*)RDF_MALLOC(cstring, 1+(mdlen<<1));
  if(!b) {
    RDF_DEBUG(rdf_digest_to_string, "Out of memory\n");
    return NULL;
  }
  
  for(i=0; i<mdlen; i++)
    sprintf(b+(i<<1), "%02x", (unsigned int)data[i]);
  b[i<<1]='\0';
  
  return(b);
}


/* fh is actually a FILE* */
void
rdf_digest_print(rdf_digest* digest, FILE* fh)
{
  char *s=rdf_digest_to_string(digest);
  
  if(!s)
    return;
  fputs(s, fh);
  RDF_FREE(cstring, s);
}


/* class initialisation */

void
init_rdf_digest(void) 
{
#ifdef HAVE_OPENSSL_DIGESTS
  rdf_digest_openssl_constructor();
#endif
#ifdef HAVE_LOCAL_SHA1_DIGEST
  sha1_constructor();
#endif
#ifdef HAVE_LOCAL_MD5_DIGEST
  md5_constructor();
#endif
#ifdef HAVE_LOCAL_RIPEMD160_DIGEST
  rmd160_constructor();
#endif
}



#ifdef RDF_DIGEST_TEST

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  rdf_digest_factory* factory;
  rdf_digest* d;
  char *test_data="http://www.ilrt.bristol.ac.uk/people/cmdjb/";
  char *test_digest_types[]={"MD5", "MD5-OPENSSL", "SHA1", "SHA1-OPENSSL", "FAKE", "RIPEMD160", "RIPEMD160-OPENSSL", NULL};
  int i;
  char *type;
  char *program=argv[0];
  
  
  /* initialise digest module */
  init_rdf_digest();

  for(i=0; (type=test_digest_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s digest\n", program, type);
    factory=get_rdf_digest_factory(type);
    if(!factory) {
      fprintf(stderr, "%s: No digest factory called %s\n", program, type);
      continue;
    }
    
    d=new_rdf_digest(factory);
    if(!d) {
      fprintf(stderr, "%s: Failed to create new digest type %s\n", program, type);
      continue;
    }
    fprintf(stderr, "%s: Initialising digest type %s\n", program, type);
    rdf_digest_init(d);

    fprintf(stderr, "%s: Writing data into digest\n", program);
    rdf_digest_update(d, (unsigned char*)test_data, strlen(test_data));

    fprintf(stderr, "%s: Finishing digesting\n", program);
    rdf_digest_final(d);

    fprintf(stderr, "%s: %s digest of data is: ", program, type);
    rdf_digest_print(d, stderr);
    fprintf(stderr, "\n");
    
    fprintf(stderr, "%s: Freeing digest\n", program);
    free_rdf_digest(d);
  }
  
    
  /* keep gcc -Wall happy */
  return(0);
}

#endif
