/*
 * rdf_digest.h - RDF Digest Factory / Digest interfaces and definition
 *
 * $Source$
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



#ifndef LIBRDF_DIGEST_H
#define LIBRDF_DIGEST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* LIBRDF_DIGEST;

/* based on the GNUPG cipher/digest registration stuff */
struct librdf_digest_factory_s 
{
  struct librdf_digest_factory_s* next;
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
typedef struct librdf_digest_factory_s librdf_digest_factory;


typedef struct
{
  char *context;
  unsigned char *digest;
  librdf_digest_factory* factory;
} librdf_digest;


void
librdf_digest_register_factory(const char *name,
                            void (*factory) (librdf_digest_factory*)
                            );

librdf_digest_factory*
librdf_get_digest_factory(const char *name);


/* module init */
void librdf_init_digest(void);
                    
/* constructor */
librdf_digest* librdf_new_digest(librdf_digest_factory *factory);

/* destructor */
void librdf_free_digest(librdf_digest *digest);


/* methods */
void librdf_digest_init(librdf_digest* digest);
void librdf_digest_update(librdf_digest* digest, unsigned char *buf, size_t length);
void librdf_digest_final(librdf_digest* digest);
void* librdf_digest_get_digest(librdf_digest* digest);

char* librdf_digest_to_string(librdf_digest* digest);
void librdf_digest_print(librdf_digest* digest, FILE* fh);


/* in librdf_digest_openssl.c */
#ifdef HAVE_OPENSSL_DIGESTS
void librdf_digest_openssl_constructor(void);
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
