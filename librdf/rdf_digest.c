/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_digest.c - RDF Digest Factory / Digest implementation
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
#include <rdf_digest.h>
#include <rdf_uri.h>


/* prototypes for helper functions */
static void librdf_delete_digest_factories(void);

/** List of digest factories */
static librdf_digest_factory *digests=NULL;


/* helper functions */
static void
librdf_delete_digest_factories(void)
{
  librdf_digest_factory *factory, *next;
  
  for(factory=digests; factory; factory=next) {
    next=factory->next;
    LIBRDF_FREE(librdf_digest_factory, factory->name);
    LIBRDF_FREE(librdf_digest_factory, factory);
  }
}


/**
 * librdf_digest_register_factory - Register a hash factory
 * @name: the name of the hash
 * @factory: function to be called to register the factory parameters
 * 
 **/
void
librdf_digest_register_factory(const char *name,
			       void (*factory) (librdf_digest_factory*))
{
  librdf_digest_factory *d, *digest;
  char *name_copy;

  LIBRDF_DEBUG2(librdf_digest_register_factory,
		"Received registration for digest %s\n", name);
  
  digest=(librdf_digest_factory*)LIBRDF_CALLOC(librdf_digest_factory, 1,
					       sizeof(librdf_digest_factory));
  if(!digest)
    LIBRDF_FATAL1(librdf_digest_register_factory,
		  "Out of memory\n");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(name)+1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_digest, digest);
    LIBRDF_FATAL1(librdf_digest_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  digest->name=name_copy;
        
  for(d = digests; d; d = d->next ) {
    if(!strcmp(d->name, name_copy)) {
      LIBRDF_FATAL2(librdf_digest_register_factory,
		    "digest %s already registered\n", d->name);
    }
  }

  
        
  /* Call the digest registration function on the new object */
  (*factory)(digest);


  LIBRDF_DEBUG4(librdf_digest_register_factory,
		"%s has context size %d and digest size %d\n", name,
		digest->context_length, digest->digest_length);
  
  digest->next = digests;
  digests = digest;
  
}


/**
 * librdf_get_digest_factory - get a digest factory
 * @name: the name of the factory
 * 
 * Return value: the factory or NULL if not found
 **/
librdf_digest_factory*
librdf_get_digest_factory(const char *name) 
{
  librdf_digest_factory *factory;

  /* return 1st digest if no particular one wanted - why? */
  if(!name) {
    factory=digests;
    if(!factory) {
      LIBRDF_DEBUG1(librdf_get_digest_factory, "No digests available\n");
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
      LIBRDF_DEBUG2(librdf_get_digest_factory,
		    "No digest with name %s found\n", name);
      return NULL;
    }
  }
  
  return factory;
}


/**
 * librdf_new_digest - Constructor - create a new librdf_digest object
 * @factory: the digest factory to use to create this digest
 * 
 * Return value: new &librdf_digest object or NULL
 **/
librdf_digest*
librdf_new_digest(librdf_digest_factory *factory)
{
  librdf_digest* d;

  d=(librdf_digest*)LIBRDF_CALLOC(librdf_digest, 1, sizeof(librdf_digest));
  if(!d)
    return NULL;
        
  d->context=(char*)LIBRDF_CALLOC(digest_context, 1, factory->context_length);
  if(!d->context) {
    librdf_free_digest(d);
    return NULL;
  }
        
  d->digest=(unsigned char*)LIBRDF_CALLOC(digest_digest, 1,
					  factory->digest_length);
  if(!d->digest) {
    librdf_free_digest(d);
    return NULL;
  }
  
  d->factory=factory;
  
  return d;
}


/**
 * librdf_free_digest - Destructor- destroy a librdf_digest object
 * @digest: the digest
 * 
 **/
void
librdf_free_digest(librdf_digest *digest) 
{
  if(digest->context)
    LIBRDF_FREE(digest_context, digest->context);
  if(digest->digest)
    LIBRDF_FREE(digest_digest, digest->digest);
  LIBRDF_FREE(librdf_digest, digest);
}



/* methods */

/**
 * librdf_digest_init - (Re)initialise the librdf_digest object
 * @digest: the digest
 * 
 **/
void
librdf_digest_init(librdf_digest* digest) 
{
  digest->factory->init(digest->context);
}


/**
 * librdf_digest_update - Add more data to the librdf_digest object
 * @digest: the digest
 * @buf: the data buffer
 * @length: the length of the data
 * 
 **/
void
librdf_digest_update(librdf_digest* digest, unsigned char *buf, size_t length) 
{
  digest->factory->update(digest->context, buf, length);
}


/**
 * librdf_digest_final - Finish the digesting of data
 * @digest: the digest
 * 
 * The digest can now be returned via librdf_digest_get_digest().
 **/
void
librdf_digest_final(librdf_digest* digest) 
{
  void* digest_data;
  
  digest->factory->final(digest->context);
  
  digest_data=(*(digest->factory->get_digest))(digest->context);
  memcpy(digest->digest, digest_data, digest->factory->digest_length);
}


/**
 * librdf_digest_get_digest - Get the calculated digested value.
 * @digest: the digest
 * 
 * Return value: pointer to the memory containing the digest.  It will
 * be digest_factory->digest_length bytes in length.
 *
 **/
void*
librdf_digest_get_digest(librdf_digest* digest)
{
  return digest->digest;
}


/**
 * librdf_digest_to_string - Get a string representation of the digest object
 * @digest: the digest
 * 
 * Return value: a newly allocated string that represents the digest.
 * This must be released by the caller using free() 
 **/
char *
librdf_digest_to_string(librdf_digest* digest)
{
  unsigned char* data=digest->digest;
  int mdlen=digest->factory->digest_length;
  char* b;
  int i;
        
  b=(char*)LIBRDF_MALLOC(cstring, 1+(mdlen<<1));
  if(!b) {
    LIBRDF_DEBUG1(librdf_digest_to_string, "Out of memory\n");
    return NULL;
  }
  
  for(i=0; i<mdlen; i++)
    sprintf(b+(i<<1), "%02x", (unsigned int)data[i]);
  b[i<<1]='\0';
  
  return(b);
}


/**
 * librdf_digest_print - Print the digest to a FILE handle
 * @digest: the digest
 * @fh: file handle
 *
 **/
void
librdf_digest_print(librdf_digest* digest, FILE* fh)
{
  char *s=librdf_digest_to_string(digest);
  
  if(!s)
    return;
  fputs(s, fh);
  LIBRDF_FREE(cstring, s);
}


/**
 * librdf_init_digest - Initialise the librdf_digest class
 **/
void
librdf_init_digest(void) 
{
#ifdef HAVE_OPENSSL_DIGESTS
  librdf_digest_openssl_constructor();
#endif
#ifdef HAVE_LOCAL_MD5_DIGEST
  md5_constructor();
#endif
#ifdef HAVE_LOCAL_RIPEMD160_DIGEST
  rmd160_constructor();
#endif
#ifdef HAVE_LOCAL_SHA1_DIGEST
  sha1_constructor();
#endif
}


/**
 * librdf_finish_digest - Terminate the librdf_digest class
 **/
void
librdf_finish_digest(void) 
{
  librdf_delete_digest_factories();
}


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_digest_factory* factory;
  librdf_digest* d;
  char *test_data="http://www.ilrt.bristol.ac.uk/people/cmdjb/";
  char *test_digest_types[]={"MD5", "SHA1", "FAKE", "RIPEMD160", NULL};
  int i;
  char *type;
  char *program=argv[0];
        
  
  /* initialise digest module */
  librdf_init_digest();
  
  for(i=0; (type=test_digest_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s digest\n", program, type);
    factory=librdf_get_digest_factory(type);
    if(!factory) {
      fprintf(stderr, "%s: No digest factory called %s\n", program, type);
      continue;
    }
    
    d=librdf_new_digest(factory);
    if(!d) {
      fprintf(stderr, "%s: Failed to create new digest type %s\n", program, type);
      continue;
    }
    fprintf(stderr, "%s: Initialising digest type %s\n", program, type);
    librdf_digest_init(d);
                
    fprintf(stderr, "%s: Writing data into digest\n", program);
    librdf_digest_update(d, (unsigned char*)test_data, strlen(test_data));
                
    fprintf(stderr, "%s: Finishing digesting\n", program);
    librdf_digest_final(d);
    
    fprintf(stderr, "%s: %s digest of data is: ", program, type);
    librdf_digest_print(d, stderr);
    fprintf(stderr, "\n");
    
    fprintf(stderr, "%s: Freeing digest\n", program);
    librdf_free_digest(d);
  }
  
  
  /* finish digest module */
  librdf_finish_digest();
  
#ifdef LIBRDF_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
