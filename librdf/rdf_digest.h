/*
 * RDF Digest Factory / Digest interfaces and definition
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


#ifndef RDF_DIGEST_H
#define RDF_DIGEST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* RDF_DIGEST;

/* based on the GNUPG cipher/digest registration stuff */
struct rdf_digest_factory_s 
{
  struct rdf_digest_factory_s* next;
  char *   name;

  /* the rest of this structure is populated by the
     digest-specific register function */
  size_t  context_length;
  size_t  digest_length;

  /* functions (over context) */
  void (*init)( void *c );
  void (*update)( void *c, unsigned char *buf, size_t nbytes );
  void (*final)( void *c );
  unsigned char *(*get_digest)( void *c );
};
typedef struct rdf_digest_factory_s rdf_digest_factory;


typedef struct
{
  char *context;
  unsigned char *digest;
  rdf_digest_factory* factory;
} rdf_digest;


void
rdf_digest_register_factory(const char *name,
                            void (*factory) (rdf_digest_factory*)
                            );

rdf_digest_factory*
rdf_get_digest_factory(const char *name);


/* module init */
void rdf_init_digest(void);
                    
/* constructor */
rdf_digest* rdf_new_digest(rdf_digest_factory *factory);

/* destructor */
void rdf_free_digest(rdf_digest *digest);


/* methods */
void rdf_digest_init(rdf_digest* digest);
void rdf_digest_update(rdf_digest* digest, unsigned char *buf, size_t length);
void rdf_digest_final(rdf_digest* digest);
void* rdf_digest_get_digest(rdf_digest* digest);

char* rdf_digest_to_string(rdf_digest* digest);
void rdf_digest_print(rdf_digest* digest, FILE* fh);


/* in rdf_digest_openssl.c */
#ifdef HAVE_OPENSSL_DIGESTS
void rdf_digest_openssl_constructor(void);
#endif

/* in sha1.c */
#ifdef HAVE_LOCAL_SHA1_DIGEST
void sha1_constructor(void);
#endif

/* in md5.c */
#ifdef HAVE_LOCAL_MD5_DIGEST
void md5_constructor(void);
#endif

/* in ripemd160.c */
#ifdef HAVE_LOCAL_RIPEMD160_DIGEST
void rmd160_constructor(void);
#endif


#ifdef __cplusplus
}
#endif

#endif
